#pragma once
#include <SDL.h>
#include <array>
#include <map>
#include <cstdint>
#include <cstdlib>
namespace rt {
// Wait before the phone/event pump and sampling, so input received during pacing belongs to this frame.
template<class Delay> inline void waitForGameFrame(uint64_t& next,uint64_t now,uint64_t frequency,Delay delay){
    if(!next)next=now;
    if(now<next)delay(Uint32((next-now)*1000/frequency));
}
using GameButtons=std::array<std::array<int16_t,16>,2>;
using GameAxes=std::array<std::array<int16_t,4>,2>;
// Caller pumps SDL after pacing. Keep each player's mapping/trigger/deadzone behavior in one place.
inline void sampleGamepads(const std::map<SDL_JoystickID,SDL_GameController*>& controllers,
    const std::array<SDL_JoystickID,2>& players,const std::map<SDL_JoystickID,std::array<int,16>>& mappings,
    GameButtons& buttons,GameAxes& axes){
    buttons={};axes={};
    for(int port=0;port<2;++port){
        auto controller=controllers.find(players[port]);auto mapping=mappings.find(players[port]);
        if(controller==controllers.end()||mapping==mappings.end())continue;
        for(int id=0;id<16;++id){auto key=mapping->second[id];
            if(key>=0)buttons[port][id]=SDL_GameControllerGetButton(controller->second,SDL_GameControllerButton(key));
            else if(key==-2||key==-3)buttons[port][id]=SDL_GameControllerGetAxis(controller->second,key==-2?SDL_CONTROLLER_AXIS_TRIGGERLEFT:SDL_CONTROLLER_AXIS_TRIGGERRIGHT)>8000;
        }
        for(int axis=0;axis<4;++axis){auto value=SDL_GameControllerGetAxis(controller->second,SDL_GameControllerAxis(axis));axes[port][axis]=std::abs(int(value))<8000?0:value;}
    }
}
}
