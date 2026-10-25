#pragma once
#include "model.hpp"
#include <functional>
#include <cmath>
#include <cstdlib>
namespace rt {
struct PlaybackProfile { std::string aspect="4:3", filter="nearest"; int volume=100; };
using PreferenceReader=std::function<std::string(const std::string&)>;
inline std::string playbackKey(const std::string& field,bool system,const Game& game) {return "playback-"+field+"-"+(system?"system-"+game.system:"game-"+game.id);}
inline bool validAspect(const std::string& value){return value=="4:3"||value=="native"||value=="wide";}
inline PlaybackProfile readPlaybackProfile(const Game& game,const PreferenceReader& read,bool consoleOnly=false) {
    PlaybackProfile out;
    auto apply=[&](bool system){
        auto aspect=read(playbackKey("aspect",system,game));
        if(!system&&aspect.empty())aspect=read("aspect-"+game.id); // Preserve previous per-game choices.
        if(validAspect(aspect))out.aspect=aspect;
        auto filter=read(playbackKey("filter",system,game));if(filter=="nearest"||filter=="linear")out.filter=filter;
        auto volume=read(playbackKey("volume",system,game));
        if(!volume.empty()&&volume.size()<=3&&volume.find_first_not_of("0123456789")==std::string::npos){int value=std::atoi(volume.c_str());if(value<=100)out.volume=value;}
    };
    apply(true);if(!consoleOnly)apply(false);return out;
}
inline std::string nextAspect(const std::string& value){return value=="4:3"?"native":value=="native"?"wide":"4:3";}
inline const char* aspectLabel(const std::string& value){return value=="native"?"Pixels nativos":value=="wide"?"Tela cheia":"Clássico · 4:3";}
struct VideoBounds { int x,y,w,h; };
inline VideoBounds videoBounds(const std::string& aspect,int width,int height){
    if(aspect=="wide")return {0,0,1280,720};
    double ratio=aspect=="native"&&width>0&&height>0?double(width)/height:4.0/3.0;
    int w=1280,h=720;if(ratio>1280.0/720.0)h=std::max(1,int(std::round(1280/ratio)));else w=std::max(1,int(std::round(720*ratio)));
    return {(1280-w)/2,(720-h)/2,w,h};
}
inline int16_t scaledSample(int16_t sample,int volume){return int16_t(int(sample)*std::clamp(volume,0,100)/100);}
}