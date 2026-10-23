#pragma once
#include "host.hpp"
#include "save_store.hpp"
#include "../platform.hpp"
#include "../ui/model.hpp"
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
    uint32_t started=0,lastFlush=0;
    std::vector<double> times;
public:
    Host host;
    Game game;
    std::string error;
    unsigned frames=0,underruns=0,overruns=0;
    double loadMs=0;
    bool stretch=false;
    explicit Session(SDL_Renderer* r):renderer(r){}
    ~Session(){release();}
    void release(){host.close();saves.reset();if(stream)SDL_FreeAudioStream(stream);stream=nullptr;if(device)SDL_CloseAudioDevice(device);device=0;if(frame)SDL_DestroyTexture(frame);frame=nullptr;width=height=0;}
    bool active() const {return host.state==SessionState::Running||host.state==SessionState::Paused;}
    bool launch(Game g,Platform& platform,bool resume){
        release();error.clear();uint64_t before=SDL_GetPerformanceCounter();game=std::move(g);frames=underruns=overruns=0;times.clear();
        try {
            if(game.system=="PlayStation")throw std::runtime_error("PS1 requer ponte VFS/discos validada. Não será copiado para RAM.");
            const auto corePath=platform.call("path","native")+"/lib"+game.core+"_libretro.so";
            if(!std::filesystem::exists(corePath))throw std::runtime_error("Núcleo não empacotado: "+game.core+". Consulte a matriz de compatibilidade.");
            const auto answer=platform.call("request","open\t"+game.uri);
            if(answer.rfind("ERR",0)==0||answer.empty())throw std::runtime_error(answer);
            const int fd=std::stoi(answer);std::vector<uint8_t> content;uint8_t buffer[65536];
            while(true){ssize_t n=read(fd,buffer,sizeof(buffer));if(n<0){close(fd);throw std::runtime_error("USB removido ou ilegível");}if(n==0)break;if(content.size()+size_t(n)>32*1024*1024){close(fd);throw std::runtime_error("ROM maior que 32 MiB; formato não permitido neste carregador");}content.insert(content.end(),buffer,buffer+n);}
            close(fd);
            const auto identity=game.id+":"+std::to_string(crc32(content.data(),content.size()))+":"+std::to_string(content.size());
            const auto saveDir=platform.call("path","saves")+"/"+game.id;std::filesystem::create_directories(saveDir);
            host.video=[this](const void* data,unsigned w,unsigned h,size_t pitch,retro_pixel_format pf){
                auto target=pf==RETRO_PIXEL_FORMAT_XRGB8888?SDL_PIXELFORMAT_XRGB8888:SDL_PIXELFORMAT_RGB565;
                if(!frame||width!=int(w)||height!=int(h)||format!=target){if(frame)SDL_DestroyTexture(frame);frame=SDL_CreateTexture(renderer,target,SDL_TEXTUREACCESS_STREAMING,w,h);width=int(w);height=int(h);format=target;}
                if(!frame)return;
                if(pf==RETRO_PIXEL_FORMAT_0RGB1555){converted.resize(w*h);for(unsigned y=0;y<h;++y){auto* row=reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(data)+y*pitch);for(unsigned x=0;x<w;++x){auto p=row[x];converted[y*w+x]=uint16_t(((p&0x7fe0)<<1)|(p&0x1f));}}SDL_UpdateTexture(frame,nullptr,converted.data(),w*2);}
                else SDL_UpdateTexture(frame,nullptr,data,int(pitch));
            };
            if(!host.open(corePath,game.uri,std::move(content),platform.call("path","bios"),saveDir))throw std::runtime_error(host.error);
            saves=std::make_unique<SaveStore>(saveDir,identity,host.coreIdentity+":"+std::to_string(fileCrc(corePath)));
            auto sram=saves->read(false);if(!sram.empty()&&!host.setSram(sram))throw std::runtime_error("SRAM incompatível; original preservado");
            if(resume){auto state=saves->read(true);if(state.empty()||!host.unserialize(state))throw std::runtime_error("Quick Resume indisponível/incompatível. Use iniciar normalmente.");if(!sram.empty())host.setSram(sram);}
            SDL_AudioSpec wanted{},obtained{};wanted.freq=int(host.av.timing.sample_rate);wanted.format=AUDIO_S16SYS;wanted.channels=2;wanted.samples=1024;
            device=SDL_OpenAudioDevice(nullptr,0,&wanted,&obtained,SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
            if(!device)throw std::runtime_error(std::string("Áudio indisponível: ")+SDL_GetError());
            stream=SDL_NewAudioStream(AUDIO_S16SYS,2,wanted.freq,obtained.format,obtained.channels,obtained.freq);if(!stream)throw std::runtime_error("Conversor de áudio indisponível");
            host.audio=[this,obtained](const int16_t* data,size_t n){
                SDL_AudioStreamPut(stream,data,int(n*4));uint8_t buffer[16384];int bytes;
                if(SDL_GetQueuedAudioSize(device)>unsigned(obtained.freq*4/4)){SDL_ClearQueuedAudio(device);++overruns;}
                while((bytes=SDL_AudioStreamGet(stream,buffer,sizeof(buffer)))>0)SDL_QueueAudio(device,buffer,bytes);
            };
            SDL_PauseAudioDevice(device,0);started=lastFlush=SDL_GetTicks();loadMs=1000.0*double(SDL_GetPerformanceCounter()-before)/SDL_GetPerformanceFrequency();return true;
        }catch(const std::exception& e){error=e.what();release();return false;}
    }
    void pause(){host.pause();if(device){SDL_PauseAudioDevice(device,1);SDL_ClearQueuedAudio(device);}}
    void resume(){host.resume();if(device)SDL_PauseAudioDevice(device,0);}
    bool save(bool snapshot){try{if(!saves)return false;auto ram=host.sram();if(!ram.empty())saves->write(false,ram);if(snapshot){auto state=host.serialize();if(state.empty())throw std::runtime_error("Núcleo não suporta save state");saves->write(true,state);}return true;}catch(const std::exception& e){error=e.what();return false;}}
    bool load(){try{if(!saves)return false;auto current=host.sram();if(!current.empty())saves->write(false,current);auto state=saves->read(true);if(!host.unserialize(state))throw std::runtime_error("Estado recusado pelo núcleo");if(!current.empty())host.setSram(current);return true;}catch(const std::exception& e){error=e.what();return false;}}
    void finish(Platform& p){if(!active())return;save(true);p.call("request","record\t"+game.id+"\t"+std::to_string((SDL_GetTicks()-started)/1000));release();}
    void tick(){auto start=SDL_GetPerformanceCounter();host.run();++frames;if(device&&SDL_GetQueuedAudioSize(device)==0)++underruns;times.push_back(1000.0*double(SDL_GetPerformanceCounter()-start)/SDL_GetPerformanceFrequency());if(times.size()>600)times.erase(times.begin());if(SDL_GetTicks()-lastFlush>10000){save(false);lastFlush=SDL_GetTicks();}}
    void draw(){SDL_SetRenderDrawColor(renderer,0,0,0,255);SDL_RenderClear(renderer);if(frame){SDL_Rect dst=stretch?SDL_Rect{0,0,1280,720}:SDL_Rect{160,0,960,720};SDL_RenderCopy(renderer,frame,nullptr,&dst);}}
    std::string metrics()const{double ms=0;for(auto v:times)ms+=v;auto sorted=times;std::sort(sorted.begin(),sorted.end());return "Frames: "+std::to_string(frames)+"\nCusto retro_run médio: "+std::to_string(times.empty()?0:ms/times.size())+" ms\np95 retro_run: "+std::to_string(sorted.empty()?0:sorted[size_t((sorted.size()-1)*0.95)])+" ms\nÁudio underruns: "+std::to_string(underruns)+" / overruns: "+std::to_string(overruns)+"\nCarga: "+std::to_string(loadMs)+" ms";}
};
}
