#include "host.hpp"
#include <cstring>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
static void* openLibrary(const char* path){return reinterpret_cast<void*>(LoadLibraryA(path));}
static void* symbol(void* handle,const char* name){return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle),name));}
static void closeLibrary(void* handle){FreeLibrary(static_cast<HMODULE>(handle));}
#else
#include <dlfcn.h>
static void* openLibrary(const char* path){return dlopen(path,RTLD_NOW|RTLD_LOCAL);}
static void* symbol(void* handle,const char* name){return dlsym(handle,name);}
static void closeLibrary(void* handle){dlclose(handle);}
#endif
namespace rt {
Host* Host::active=nullptr;
bool Host::open(const std::string& core,const std::string& path,std::vector<uint8_t> data,const std::string& bios,const std::string& saves){
    close();error.clear();state=SessionState::Launching;
    try {
        if(active && active!=this)throw std::runtime_error("Já existe uma sessão ativa");
        library=openLibrary(core.c_str());if(!library)throw std::runtime_error("Núcleo indisponível/ABI incompatível: "+core);
#define BIND(name) api.name=reinterpret_cast<decltype(api.name)>(symbol(library,"retro_" #name));if(!api.name)throw std::runtime_error("Símbolo ausente: retro_" #name)
        BIND(init);BIND(deinit);BIND(api_version);BIND(get_system_info);BIND(get_system_av_info);
        BIND(set_environment);BIND(set_video_refresh);BIND(set_audio_sample);BIND(set_audio_sample_batch);
        BIND(set_input_poll);BIND(set_input_state);BIND(set_controller_port_device);BIND(reset);BIND(run);
        BIND(load_game);BIND(unload_game);BIND(serialize_size);BIND(serialize);BIND(unserialize);BIND(get_memory_data);BIND(get_memory_size);
#undef BIND
        if(api.api_version()!=RETRO_API_VERSION)throw std::runtime_error("API Libretro incompatível");
        active=this;systemDir=bios;saveDir=saves;contentPath=path;rom=std::move(data);pixelFormat=RETRO_PIXEL_FORMAT_0RGB1555;variables.clear();discs={};
        api.set_environment(environment);api.set_video_refresh(videoCallback);api.set_audio_sample(sampleCallback);api.set_audio_sample_batch(audioCallback);api.set_input_poll(pollCallback);api.set_input_state(inputCallback);
        api.init();initialized=true;
        retro_system_info info{};api.get_system_info(&info);coreIdentity=std::string(info.library_name?info.library_name:"?")+":"+(info.library_version?info.library_version:"?");
        if(!info.need_fullpath && rom.empty())throw std::runtime_error("Conteúdo vazio ou grande demais para este núcleo");
        retro_game_info game{contentPath.c_str(),info.need_fullpath?nullptr:rom.data(),info.need_fullpath?0:rom.size(),nullptr};
        if(!api.load_game(&game))throw std::runtime_error("Núcleo recusou jogo. Verifique formato, BIOS e versão do ROM-set.");
        loaded=true;api.get_system_av_info(&av);
        if(av.timing.fps<10||av.timing.fps>240||av.timing.sample_rate<8000||av.timing.sample_rate>192000)throw std::runtime_error("Temporização inválida do núcleo");
        api.set_controller_port_device(0,RETRO_DEVICE_JOYPAD);api.set_controller_port_device(1,RETRO_DEVICE_JOYPAD);
        state=SessionState::Running;return true;
    }catch(const std::exception& e){error=e.what();close();state=SessionState::Failed;return false;}
}
void Host::close(){
    if(library){if(loaded && api.unload_game)api.unload_game();if(initialized && api.deinit)api.deinit();closeLibrary(library);}
    library=nullptr;loaded=false;initialized=false;if(active==this)active=nullptr;rom.clear();rom.shrink_to_fit();variables.clear();buttons={};axes={};api={};state=SessionState::Menu;
}
void Host::run(){if(state==SessionState::Running&&loaded)api.run();}
void Host::pause(){if(state==SessionState::Running){state=SessionState::Paused;buttons={};axes={};}}
void Host::resume(){if(state==SessionState::Paused)state=SessionState::Running;}
void Host::reset(){if(loaded)api.reset();}
std::vector<uint8_t> Host::serialize(){if(!loaded)return {};auto n=api.serialize_size();if(!n||n>32*1024*1024)return {};std::vector<uint8_t> out(n);if(!api.serialize(out.data(),n))out.clear();return out;}
bool Host::unserialize(const std::vector<uint8_t>& bytes){return loaded&&!bytes.empty()&&bytes.size()==api.serialize_size()&&api.unserialize(bytes.data(),bytes.size());}
std::vector<uint8_t> Host::sram(){if(!loaded)return {};auto n=api.get_memory_size(RETRO_MEMORY_SAVE_RAM);auto* ptr=static_cast<uint8_t*>(api.get_memory_data(RETRO_MEMORY_SAVE_RAM));if(!ptr||!n||n>16*1024*1024)return {};return {ptr,ptr+n};}
bool Host::setSram(const std::vector<uint8_t>& bytes){if(!loaded||bytes.size()!=api.get_memory_size(RETRO_MEMORY_SAVE_RAM))return false;auto* ptr=api.get_memory_data(RETRO_MEMORY_SAVE_RAM);if(!ptr)return false;std::memcpy(ptr,bytes.data(),bytes.size());return true;}
bool Host::switchDisc(){if(!discs.set_eject_state||!discs.get_num_images||!discs.set_image_index||!discs.get_image_index)return false;unsigned n=discs.get_num_images();if(n<2)return false;if(!discs.set_eject_state(true))return false;bool ok=discs.set_image_index((discs.get_image_index()+1)%n);return discs.set_eject_state(false)&&ok;}
void Host::videoCallback(const void* data,unsigned w,unsigned h,size_t pitch){if(active&&data&&data!=RETRO_HW_FRAME_BUFFER_VALID&&w&&h&&w<=1024&&h<=1024&&active->video)active->video(data,w,h,pitch,active->pixelFormat);}
size_t Host::audioCallback(const int16_t* data,size_t n){if(active&&active->audio&&data&&n<=16384)active->audio(data,n);return n;}
void Host::sampleCallback(int16_t left,int16_t right){int16_t frame[2]={left,right};audioCallback(frame,1);}
int16_t Host::inputCallback(unsigned port,unsigned device,unsigned index,unsigned id){
    if(!active||port>=2)return 0;
    if((device&RETRO_DEVICE_MASK)==RETRO_DEVICE_JOYPAD){if(id==RETRO_DEVICE_ID_JOYPAD_MASK){int16_t mask=0;for(unsigned i=0;i<16;++i)if(active->buttons[port][i])mask|=int16_t(1u<<i);return mask;}return id<16?active->buttons[port][id]:0;}
    if((device&RETRO_DEVICE_MASK)==RETRO_DEVICE_ANALOG&&index<2&&id<2)return active->axes[port][index*2+id];return 0;
}
bool Host::environment(unsigned cmd,void* data){
    if(!active)return false;auto& h=*active;
    switch(cmd){
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:{auto p=*static_cast<retro_pixel_format*>(data);if(p>RETRO_PIXEL_FORMAT_RGB565)return false;h.pixelFormat=p;return true;}
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:*static_cast<const char**>(data)=h.systemDir.c_str();return true;
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:*static_cast<const char**>(data)=h.saveDir.c_str();return true;
    case RETRO_ENVIRONMENT_GET_CAN_DUPE:*static_cast<bool*>(data)=true;return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE:{auto* v=static_cast<retro_variable*>(data);auto it=h.variables.find(v->key?v->key:"");v->value=it==h.variables.end()?nullptr:it->second.c_str();return v->value!=nullptr;}
    case RETRO_ENVIRONMENT_SET_VARIABLES:{auto* v=static_cast<retro_variable*>(data);for(;v&&v->key;++v){std::string text=v->value?v->value:"";auto start=text.find(';');if(start==std::string::npos)continue;start=text.find_first_not_of(' ',start+1);h.variables[v->key]=text.substr(start,text.find('|',start)-start);}return true;}
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:*static_cast<bool*>(data)=false;return true;
    case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:return true;
    case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:*static_cast<unsigned*>(data)=0;return true;
    case RETRO_ENVIRONMENT_GET_LANGUAGE:*static_cast<unsigned*>(data)=RETRO_LANGUAGE_ENGLISH;return true;
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:case RETRO_ENVIRONMENT_SET_MEMORY_MAPS:case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL:return true;
    case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO:h.av=*static_cast<retro_system_av_info*>(data);return true;
    case RETRO_ENVIRONMENT_SET_GEOMETRY:h.av.geometry=*static_cast<retro_game_geometry*>(data);return true;
    case RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE:*static_cast<int*>(data)=3;return true;
    case RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE:h.discs=*static_cast<retro_disk_control_callback*>(data);return true;
    case RETRO_ENVIRONMENT_GET_VFS_INTERFACE:{auto* info=static_cast<retro_vfs_interface_info*>(data);if(!h.vfs||info->required_interface_version>3)return false;info->required_interface_version=3;info->iface=h.vfs;return true;}
    case RETRO_ENVIRONMENT_SET_HW_RENDER:h.error="Núcleo exige renderização por hardware não validada";return false;
    case RETRO_ENVIRONMENT_SHUTDOWN:h.pause();return true;
    default:return false;
    }
}
}
