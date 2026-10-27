#include "input/frame_input.hpp"
#include "emulation/host.hpp"
#include "emulation/save_store.hpp"
#include <iostream>
#include <stdexcept>
using namespace rt;
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(int argc,char** argv){
    SDL_SetMainReady();
    try{
        check(argc==3,"frame_input_tests core original-demo");
        check(SDL_Init(SDL_INIT_GAMECONTROLLER|SDL_INIT_EVENTS)==0,SDL_GetError());
        std::array<int,2> indexes;
        std::array<SDL_Joystick*,2> joysticks;
        std::array<SDL_JoystickID,2> players;
        std::map<SDL_JoystickID,SDL_GameController*> controllers;
        const std::array<int,16> mapping={SDL_CONTROLLER_BUTTON_B,SDL_CONTROLLER_BUTTON_Y,SDL_CONTROLLER_BUTTON_BACK,SDL_CONTROLLER_BUTTON_START,SDL_CONTROLLER_BUTTON_DPAD_UP,SDL_CONTROLLER_BUTTON_DPAD_DOWN,SDL_CONTROLLER_BUTTON_DPAD_LEFT,SDL_CONTROLLER_BUTTON_DPAD_RIGHT,SDL_CONTROLLER_BUTTON_A,SDL_CONTROLLER_BUTTON_X,SDL_CONTROLLER_BUTTON_LEFTSHOULDER,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,-2,-3,SDL_CONTROLLER_BUTTON_LEFTSTICK,SDL_CONTROLLER_BUTTON_RIGHTSTICK};
        std::map<SDL_JoystickID,std::array<int,16>> mappings;
        for(int port=0;port<2;++port){
            SDL_VirtualJoystickDesc desc;SDL_zero(desc);desc.version=SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
            desc.type=SDL_JOYSTICK_TYPE_GAMECONTROLLER;desc.naxes=SDL_CONTROLLER_AXIS_MAX;desc.nbuttons=SDL_CONTROLLER_BUTTON_MAX;
            desc.axis_mask=(1u<<SDL_CONTROLLER_AXIS_MAX)-1;desc.button_mask=(1u<<SDL_CONTROLLER_BUTTON_MAX)-1;desc.name="Regression virtual pad";
            indexes[port]=SDL_JoystickAttachVirtualEx(&desc);check(indexes[port]>=0,SDL_GetError());
            joysticks[port]=SDL_JoystickOpen(indexes[port]);check(joysticks[port],SDL_GetError());players[port]=SDL_JoystickInstanceID(joysticks[port]);
            controllers[players[port]]=SDL_GameControllerOpen(indexes[port]);check(controllers[players[port]],SDL_GetError());mappings[players[port]]=mapping;
        }
        GameButtons buttons;GameAxes axes;
        auto sample=[&]{SDL_PumpEvents();sampleGamepads(controllers,players,mappings,buttons,axes);};
        auto button=[&](int port,int key,bool down){check(SDL_JoystickSetVirtualButton(joysticks[port],key,down)==0,SDL_GetError());};
        // Inject in the pacing callback, deterministically: no sleep/timing threshold or physical radio involved.
        uint64_t next=200;
        waitForGameFrame(next,100,1000,[&](Uint32 delay){check(delay==100,"wrong pacing interval");button(0,SDL_CONTROLLER_BUTTON_DPAD_RIGHT,true);button(1,SDL_CONTROLLER_BUTTON_A,true);});
        sample();check(buttons[0][7]==1&&buttons[1][8]==1,"input arriving during pacing missed this frame/player 2");
        // Every press and every release is sampled in its own frame, while player 2 stays held.
        for(int id=0;id<16;++id){
            if(mapping[id]<0)continue;
            button(0,SDL_CONTROLLER_BUTTON_DPAD_RIGHT,false);button(0,mapping[id],true);sample();
            check(buttons[0][id]==1&&buttons[1][8]==1,"digital press or player 2 lost");
            button(0,mapping[id],false);sample();check(buttons[0][id]==0&&buttons[1][8]==1,"release lost");
        }
        button(0,SDL_CONTROLLER_BUTTON_DPAD_UP,true);button(0,SDL_CONTROLLER_BUTTON_DPAD_RIGHT,true);sample();
        check(buttons[0][4]&&buttons[0][7],"diagonal lost");
        SDL_JoystickSetVirtualAxis(joysticks[0],SDL_CONTROLLER_AXIS_LEFTX,7999);sample();check(axes[0][0]==0,"deadzone changed");
        SDL_JoystickSetVirtualAxis(joysticks[0],SDL_CONTROLLER_AXIS_LEFTX,24000);
        SDL_JoystickSetVirtualAxis(joysticks[1],SDL_CONTROLLER_AXIS_RIGHTY,-24000);
        SDL_JoystickSetVirtualAxis(joysticks[0],SDL_CONTROLLER_AXIS_TRIGGERLEFT,32767);sample();
        check(axes[0][0]==24000&&axes[1][3]==-24000&&buttons[0][12]==1,"axes/triggers mixed across players");
        auto remapped=mappings;remapped[players[0]][8]=SDL_CONTROLLER_BUTTON_Y;button(0,SDL_CONTROLLER_BUTTON_Y,true);SDL_PumpEvents();
        sampleGamepads(controllers,players,remapped,buttons,axes);check(buttons[0][8]==1,"remapping lost");
        auto disconnected=controllers;disconnected.erase(players[0]);sampleGamepads(disconnected,players,mappings,buttons,axes);
        for(auto value:buttons[0])check(value==0,"disconnected port retained a button");for(auto value:axes[0])check(value==0,"disconnected port retained an axis");check(buttons[1][8]==1,"disconnect cleared player 2");
        // Consume the sampled SDL state through the real Libretro host and original CC0 NES program.
        for(int port=0;port<2;++port){
            for(int key=0;key<SDL_CONTROLLER_BUTTON_MAX;++key)button(port,key,false);
            for(int axis=0;axis<SDL_CONTROLLER_AXIS_MAX;++axis)SDL_JoystickSetVirtualAxis(joysticks[port],axis,0);
        }
        Host host;uint32_t rendered=0;unsigned frames=0;
        host.video=[&](const void* data,unsigned,unsigned height,size_t pitch,retro_pixel_format){rendered=crc32(static_cast<const uint8_t*>(data),pitch*height);++frames;};
        check(host.open(argv[1],argv[2],readBounded(argv[2]),".","."),host.error.c_str());
        auto tick=[&]{sample();host.buttons=buttons;host.axes=axes;host.run();};
        for(int i=0;i<120;++i)tick();auto neutral=rendered;
        button(0,SDL_CONTROLLER_BUTTON_DPAD_RIGHT,true);for(int i=0;i<8;++i)tick();
        check(rendered!=neutral,"sampled right input did not move the real NES frame");
        button(0,SDL_CONTROLLER_BUTTON_DPAD_RIGHT,false);for(int i=0;i<4;++i)tick();auto stopped=rendered;
        for(int i=0;i<16;++i)tick();check(rendered==stopped,"sampled release left the NES character moving");
        check(frames>=148,"real core did not consume sampled frames");host.close();
        for(int port=1;port>=0;--port){SDL_GameControllerClose(controllers[players[port]]);SDL_JoystickClose(joysticks[port]);SDL_JoystickDetachVirtual(indexes[port]);}
        SDL_Quit();std::cout<<"SDL late-frame sampling, all mapped digital edges, diagonal, axes, triggers, remapping, player 2 and disconnect passed. Real QuickNES rendered movement and stopped after release (148 frames).\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<"\n";SDL_Quit();return 1;}
}
