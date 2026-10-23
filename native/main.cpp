#include <SDL.h>
#include "ui/model.hpp"
#include "ui/renderer.hpp"
#include "platform.hpp"
#include <map>
using namespace rt;
int main(int, char**) {
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON,"1");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER,"opengles2");
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_GAMECONTROLLER)!=0)return 1;
    auto* window=SDL_CreateWindow("RetroTV Lite",0,0,1280,720,SDL_WINDOW_FULLSCREEN_DESKTOP);
    auto* renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if(!renderer){SDL_Quit();return 2;}
    SDL_RenderSetLogicalSize(renderer,1280,720);
    Platform platform; auto* atlas=platform.font(renderer); UI ui(renderer,atlas); Model model;
    model.minimal=platform.call("setting","minimal")=="1";
    model.performance=platform.call("setting","performance")=="1";
    auto loadCatalog=[&] {
        const auto old=model.selected()?model.selected()->id:"";
        model.games.clear();
        for(const auto& row:split(platform.call("catalog"),'\n')) {
            auto p=split(row,'\t'); if(p.size()!=9)continue;
            Game g;g.id=p[0];g.title=p[1];g.system=p[2];g.uri=p[3];g.core=p[4];g.favorite=p[5]=="1";
            g.recent=std::strtoll(p[6].c_str(),nullptr,10);g.played=std::strtoll(p[7].c_str(),nullptr,10);g.available=p[8]=="1";
            model.games.push_back(g);
        }
        auto rows=model.visible();for(size_t i=0;i<rows.size();++i)if(model.games[rows[i]].id==old)model.selection=int(i);
    };
    loadCatalog(); std::string revision=platform.call("revision"); uint32_t lastRefresh=0;
    bool running=true,background=false; std::map<SDL_JoystickID,SDL_GameController*> controllers;
    auto action=[&](Action a) {
        if(model.screen==Screen::Settings) {
            if(a==Action::Back){model.navigate(a);return;}
            if(a==Action::Up)model.option=std::max(0,model.option-1);
            if(a==Action::Down)model.option=std::min(8,model.option+1);
            if(a==Action::Ok && model.option==2)model.minimal=!model.minimal;
            if(a==Action::Ok && model.option==3)model.performance=!model.performance;
            if(a==Action::Ok && model.option==0)platform.call("request","choose");
            if(a==Action::Ok && model.option==1)platform.call("request","scan");
            if(a==Action::Ok) {
                platform.call("setting",std::string("minimal\t")+(model.minimal?"1":"0"));
                platform.call("setting",std::string("performance\t")+(model.performance?"1":"0"));
                if(model.option==7){model.screen=Screen::Diagnostics;model.message=platform.call("diagnostics");}
                if(model.option==8){model.screen=Screen::Diagnostics;model.message="RetroTV Lite · 0.1.0-dev\nCódigo original GPL-2.0-or-later\nSDL2: zlib · QuickNES: GPL-2.0-or-later\nFontes e licenças acompanham o projeto.\nSem jogos comerciais ou BIOS incluídos.\nVoltar retorna à biblioteca.";}
            }
            return;
        }
        if(model.screen==Screen::Details && a==Action::Ok) {
            auto* g=model.selected(); if(!g)return;
            if(model.option==2){platform.call("request","favorite\t"+g->id+"\t"+(g->favorite?"0":"1"));loadCatalog();return;}
            if(model.option==3){model.screen=Screen::Library;return;}
            model.screen=Screen::Error;model.message="Núcleo ainda não iniciado.\nIntegração Libretro no próximo marco.\nVoltar retorna à biblioteca.";return;
        }
        model.navigate(a);
    };
    while(running) {
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type==SDL_QUIT)running=false;
            if(e.type==SDL_APP_WILLENTERBACKGROUND)background=true;
            if(e.type==SDL_APP_DIDENTERFOREGROUND)background=false;
            if(e.type==SDL_CONTROLLERDEVICEADDED) {auto* c=SDL_GameControllerOpen(e.cdevice.which);if(c)controllers[SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(c))]=c;}
            if(e.type==SDL_CONTROLLERDEVICEREMOVED) {auto it=controllers.find(e.cdevice.which);if(it!=controllers.end()){SDL_GameControllerClose(it->second);controllers.erase(it);}}
            if(e.type==SDL_KEYDOWN) {
                switch(e.key.keysym.sym) {
                    case SDLK_UP:action(Action::Up);break;case SDLK_DOWN:action(Action::Down);break;
                    case SDLK_LEFT:action(Action::Left);break;case SDLK_RIGHT:action(Action::Right);break;
                    case SDLK_RETURN:case SDLK_KP_ENTER:action(Action::Ok);break;
                    case SDLK_ESCAPE:case SDLK_AC_BACK:action(Action::Back);break;
                    case SDLK_MENU:case SDLK_F1:action(Action::Menu);break;default:break;
                }
            }
            if(e.type==SDL_CONTROLLERBUTTONDOWN) {
                switch(e.cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_DPAD_UP:action(Action::Up);break;case SDL_CONTROLLER_BUTTON_DPAD_DOWN:action(Action::Down);break;
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:action(Action::Left);break;case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:action(Action::Right);break;
                    case SDL_CONTROLLER_BUTTON_A:action(Action::Ok);break;case SDL_CONTROLLER_BUTTON_B:action(Action::Back);break;
                    case SDL_CONTROLLER_BUTTON_START:action(Action::Menu);break;default:break;
                }
            }
        }
        if(background){SDL_Delay(40);continue;}
        if(SDL_GetTicks()-lastRefresh>500){auto next=platform.call("revision");if(next!=revision){loadCatalog();revision=next;}lastRefresh=SDL_GetTicks();}
        ui.draw(model,platform.call("status"));SDL_RenderPresent(renderer);SDL_Delay(model.performance?20:4);
    }
    for(auto& c:controllers)SDL_GameControllerClose(c.second);
    SDL_DestroyTexture(atlas);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 0;
}
