#pragma once
#include "host.hpp"
#include "save_store.hpp"
#include "snes_core.hpp"
#include "vfs.hpp"
#include "../platform.hpp"
#include "../ui/model.hpp"
#include "../ui/playback_profile.hpp"
#include <SDL.h>
#include <memory>
#include <unistd.h>
namespace rt {
class Session {
    SDL_Renderer* renderer;
    SDL_Texture* frame=nullptr;
    SDL_AudioDeviceID device=0;
    SDL_AudioStream* stream=nullptr;
    int width=0,height=0;Uint32 format=0;
    std::vector<uint16_t> converted;
    std::unique_ptr<SaveStore> saves;
    std::unique_ptr<Vfs> vfs;
    uint32_t started=0,lastFlush=0;
    uint64_t playedMs=0;uint32_t lastActive=0;
    std::string activeCorePath;
    std::vector<double> times;
    std::vector<double> frameTimes;uint64_t lastPresented=0;
public:
    Host host;
    Game game;
    std::string error;
    unsigned frames=0,underruns=0,overruns=0;
    double loadMs=0;
    PlaybackProfile playback;
    std::vector<int16_t> scaledAudio;
    void applyPlayback(const PlaybackProfile& profile){playback=profile;if(frame)SDL_SetTextureScaleMode(frame,playback.filter=="linear"?SDL_ScaleModeLinear:SDL_ScaleModeNearest);}
    void invalidateFrame(){if(frame)SDL_DestroyTexture(frame);frame=nullptr;}
    explicit Session(SDL_Renderer* r):renderer(r){}
    ~Session(){release();}
    void release(){host.close();host.audio={};host.video={};host.vfs=nullptr;vfs.reset();saves.reset();if(stream)SDL_FreeAudioStream(stream);stream=nullptr;if(device)SDL_CloseAudioDevice(device);device=0;if(frame)SDL_DestroyTexture(frame);frame=nullptr;width=height=0;std::vector<uint16_t>().swap(converted);std::vector<int16_t>().swap(scaledAudio);}
    bool active() const {return host.state==SessionState::Running||host.state==SessionState::Paused;}
    bool launch(Game g,Platform& platform,bool resume){
        release();error.clear();uint64_t before=SDL_GetPerformanceCounter();game=std::move(g);applyPlayback(readPlaybackProfile(game,[&](const std::string& key){return platform.call("setting",key);}));frames=underruns=overruns=0;times.clear();frameTimes.clear();lastPresented=0;
        try {
            const bool disc=game.system=="PlayStation";
            const bool streamed=disc||game.system=="Arcade";
            auto corePath=platform.call("path","native")+"/lib"+game.core+"_libretro.so";
            if(!std::filesystem::exists(corePath))throw std::runtime_error("Núcleo não empacotado: "+game.core+". Consulte a matriz de compatibilidade.");
            const auto answer=platform.call("request","open\t"+game.uri);
            if(answer.rfind("ERR",0)==0||answer.empty())throw std::runtime_error(answer);
            const int fd=std::stoi(answer);std::vector<uint8_t> content;uint8_t buffer[65536];std::string contentPath=game.uri,discStamp;
            if(streamed){auto parts=split(platform.call("request","disc\t"+game.uri),'\t');if(parts.size()!=2){close(fd);throw std::runtime_error("Atualize o scan USB para preparar este disco");}contentPath=parts[0];discStamp=parts[1];}
            while(true){ssize_t n=read(fd,buffer,sizeof(buffer));if(n<0){close(fd);throw std::runtime_error("USB removido ou ilegível");}if(n==0)break;if(content.size()+size_t(n)>32*1024*1024){close(fd);throw std::runtime_error("ROM maior que 32 MiB; formato não permitido neste carregador");}content.insert(content.end(),buffer,buffer+n);if(streamed)break;}
            if(streamed){auto size=lseek64(fd,0,SEEK_END);if(size>65536&&lseek64(fd,size-65536,SEEK_SET)>=0){auto n=read(fd,buffer,sizeof(buffer));if(n>0)content.insert(content.end(),buffer,buffer+n);}}
            close(fd);
            const auto identity=game.id+":"+std::to_string(crc32(content.data(),content.size()))+":"+std::to_string(content.size())+":"+discStamp;
            const auto saveDir=platform.call("path","saves")+"/"+game.id;std::filesystem::create_directories(saveDir);
            if(game.core=="snes9x2005")corePath=selectSnesCore(corePath,platform.call("path","native")+"/libsnes9x2005_lowlatency_libretro.so",SaveStore(saveDir,identity,""),resume);
            activeCorePath=corePath;
            host.video=[this](const void* data,unsigned w,unsigned h,size_t pitch,retro_pixel_format pf){
                auto target=pf==RETRO_PIXEL_FORMAT_XRGB8888?SDL_PIXELFORMAT_XRGB8888:SDL_PIXELFORMAT_RGB565;
                if(!frame||width!=int(w)||height!=int(h)||format!=target){if(frame)SDL_DestroyTexture(frame);frame=SDL_CreateTexture(renderer,target,SDL_TEXTUREACCESS_STREAMING,w,h);width=int(w);height=int(h);format=target;SDL_SetTextureScaleMode(frame,playback.filter=="linear"?SDL_ScaleModeLinear:SDL_ScaleModeNearest);}
                if(!frame)return;
                if(pf==RETRO_PIXEL_FORMAT_0RGB1555){converted.resize(w*h);for(unsigned y=0;y<h;++y){auto* row=reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(data)+y*pitch);for(unsigned x=0;x<w;++x){auto p=row[x];converted[y*w+x]=uint16_t(((p&0x7fe0)<<1)|(p&0x1f));}}SDL_UpdateTexture(frame,nullptr,converted.data(),w*2);}
                else SDL_UpdateTexture(frame,nullptr,data,int(pitch));
            };
            const auto biosDir=platform.call("path","bios");
            if(streamed){
                if(disc&&!std::filesystem::exists(biosDir+"/scph5501.bin"))throw std::runtime_error("BIOS PS1 ausente. Importe seu arquivo em Configurações.");
                content.clear();content.shrink_to_fit();vfs=std::make_unique<Vfs>();vfs->openRead=[&platform,biosDir,saveDir](const std::string& path)->int{
                    if(path.rfind("/__retrotv_saf__/",0)==0){auto result=platform.call("request","vfs\t"+path);try{return result.rfind("ERR",0)==0?-1:std::stoi(result);}catch(...){return -1;}}
                    auto absolute=std::filesystem::path(path).lexically_normal().string();if(absolute.rfind(biosDir+"/",0)==0||absolute.rfind(saveDir+"/",0)==0)return ::open(absolute.c_str(),O_RDONLY);return -1;
                };host.vfs=&vfs->table;
            }
            const auto staging=streamed?std::string():platform.call("path","content")+"/"+stagedName(game.uri,game.system);
            if(!host.open(corePath,contentPath,std::move(content),biosDir,saveDir,staging))throw std::runtime_error(host.error);
            saves=std::make_unique<SaveStore>(saveDir,identity,host.coreIdentity+":"+std::to_string(fileCrc(corePath)));
            auto sram=saves->read(false);if(!sram.empty()&&!host.setSram(sram))throw std::runtime_error("SRAM incompatível; original preservado");
            if(game.core=="snes9x2005"&&corePath.find("_lowlatency_")!=std::string::npos)preserveSnesSaves(*saves);
            if(resume){auto state=saves->read(true);if(state.empty()||!host.unserialize(state))throw std::runtime_error("Quick Resume indisponível/incompatível. Use iniciar normalmente.");if(!sram.empty())host.setSram(sram);}
            SDL_AudioSpec wanted{},obtained{};wanted.freq=int(host.av.timing.sample_rate);wanted.format=AUDIO_S16SYS;wanted.channels=2;wanted.samples=1024;
            device=SDL_OpenAudioDevice(nullptr,0,&wanted,&obtained,SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
            if(!device)throw std::runtime_error(std::string("Áudio indisponível: ")+SDL_GetError());
            stream=SDL_NewAudioStream(AUDIO_S16SYS,2,wanted.freq,obtained.format,obtained.channels,obtained.freq);if(!stream)throw std::runtime_error("Conversor de áudio indisponível");
            host.audio=[this,obtained](const int16_t* data,size_t n){
                const int16_t* samples=data;if(playback.volume!=100){scaledAudio.resize(n*2);for(size_t i=0;i<n*2;++i)scaledAudio[i]=scaledSample(data[i],playback.volume);samples=scaledAudio.data();}SDL_AudioStreamPut(stream,samples,int(n*4));uint8_t buffer[16384];int bytes;
                if(SDL_GetQueuedAudioSize(device)>unsigned(obtained.freq*4/4)){SDL_ClearQueuedAudio(device);++overruns;}
                while((bytes=SDL_AudioStreamGet(stream,buffer,sizeof(buffer)))>0)SDL_QueueAudio(device,buffer,bytes);
            };
            SDL_PauseAudioDevice(device,0);started=lastFlush=lastActive=SDL_GetTicks();playedMs=0;platform.call("request","record\t"+game.id+"\t0");loadMs=1000.0*double(SDL_GetPerformanceCounter()-before)/SDL_GetPerformanceFrequency();return true;
        }catch(const std::exception& e){error=e.what();release();return false;}
    }
    void pause(){if(host.state==SessionState::Running)playedMs+=SDL_GetTicks()-lastActive;host.pause();if(device){SDL_PauseAudioDevice(device,1);SDL_ClearQueuedAudio(device);}}
    void resume(){lastActive=SDL_GetTicks();lastPresented=0;host.resume();if(device)SDL_PauseAudioDevice(device,0);}
    bool save(bool snapshot){try{if(!saves)return false;auto ram=host.sram();if(!ram.empty())saves->write(false,ram);if(snapshot){auto state=host.serialize();if(state.empty())throw std::runtime_error("Núcleo não suporta save state");saves->write(true,state);}return true;}catch(const std::exception& e){error=e.what();return false;}}
    bool load(Platform& platform){try{if(!saves)return false;
        if(game.core=="snes9x2005"){
            auto root=platform.call("path","native");auto selected=selectSnesCore(root+"/libsnes9x2005_libretro.so",root+"/libsnes9x2005_lowlatency_libretro.so",*saves,true);
            if(selected!=activeCorePath){
                // Preserve current battery saves before reopening the exact snapshot core.
                if(!save(false))return false;auto played=playedMs;auto selectedGame=game;
                if(!launch(selectedGame,platform,true))return false;pause();playedMs=played;return true;
            }
        }
        auto current=host.sram();auto state=saves->read(true);if(!current.empty())saves->write(false,current);if(!host.unserialize(state))throw std::runtime_error("Estado recusado pelo núcleo");if(!current.empty())host.setSram(current);return true;
    }catch(const std::exception& e){error=e.what();return false;}}
    void finish(Platform& p){if(!active())return;pause();save(true);p.call("request","record\t"+game.id+"\t"+std::to_string(playedMs/1000));release();}
    void tick(){auto start=SDL_GetPerformanceCounter();host.run();++frames;if(vfs&&vfs->ioFailed){pause();error="Erro de leitura/seek USB. Reconecte e reabra o jogo.\nSRAM e biblioteca permanecem preservados.";}if(device&&SDL_GetQueuedAudioSize(device)==0)++underruns;times.push_back(1000.0*double(SDL_GetPerformanceCounter()-start)/SDL_GetPerformanceFrequency());if(times.size()>600)times.erase(times.begin());if(SDL_GetTicks()-lastFlush>10000){save(false);lastFlush=SDL_GetTicks();}}
    void draw(){SDL_SetRenderDrawColor(renderer,0,0,0,255);SDL_RenderClear(renderer);if(frame){auto box=videoBounds(playback.aspect,width,height);SDL_Rect dst{box.x,box.y,box.w,box.h};SDL_RenderCopy(renderer,frame,nullptr,&dst);}}
    void presented(){auto now=SDL_GetPerformanceCounter();if(lastPresented){frameTimes.push_back(1000.0*double(now-lastPresented)/SDL_GetPerformanceFrequency());if(frameTimes.size()>600)frameTimes.erase(frameTimes.begin());}lastPresented=now;}
    std::string metrics()const{double ms=0;for(auto v:times)ms+=v;auto sorted=frameTimes;std::sort(sorted.begin(),sorted.end());auto elapsed=playedMs+(host.state==SessionState::Running?SDL_GetTicks()-lastActive:0);return "FPS da sessão local: "+std::to_string(elapsed?frames*1000.0/elapsed:0)+"\nFrametime p50 / p95: "+std::to_string(sorted.empty()?0:sorted[sorted.size()/2])+" / "+std::to_string(sorted.empty()?0:sorted[size_t((sorted.size()-1)*0.95)])+" ms\nCusto retro_run médio: "+std::to_string(times.empty()?0:ms/times.size())+" ms\nÁudio underruns / overruns: "+std::to_string(underruns)+" / "+std::to_string(overruns)+"\nCarga: "+std::to_string(loadMs)+" ms\nNúcleo: "+host.coreIdentity;}
};
}
