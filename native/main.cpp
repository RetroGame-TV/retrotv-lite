#include <SDL.h>
#include "ui/model.hpp"
#include "ui/renderer.hpp"
#include "platform.hpp"
#include "emulation/session.hpp"
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
    Session session(renderer);int pendingAction=-1;uint64_t nextFrame=0;
    bool running=true,background=false; std::map<SDL_JoystickID,SDL_GameController*> controllers;
    auto action=[&](Action a) {
        if(session.active()) {
            if(model.screen==Screen::Error){if(a==Action::Ok||a==Action::Back){model.screen=Screen::QuickMenu;model.option=0;}return;}
            if(model.screen==Screen::Controls||model.screen==Screen::Diagnostics){if(a==Action::Back){model.screen=Screen::QuickMenu;model.option=0;}return;}
            if(model.screen==Screen::Confirm){
                if(a==Action::Back){model.screen=Screen::QuickMenu;return;}
                if(a!=Action::Ok)return;
                bool ok=true;
                if(pendingAction==1)ok=session.save(true);
                if(pendingAction==2)ok=session.load();
                if(pendingAction==3)session.host.reset();
                if(pendingAction==7){session.finish(platform);loadCatalog();model.screen=Screen::Library;return;}
                model.screen=ok?Screen::QuickMenu:Screen::Error;model.message=session.error;return;
            }
            if(session.host.state==SessionState::Running){
                if(a==Action::Menu||a==Action::Back){session.pause();model.screen=Screen::QuickMenu;model.option=0;}return;
            }
            if(a==Action::Up)model.option=std::max(0,model.option-1);
            if(a==Action::Down)model.option=std::min(8,model.option+1);
            if(a==Action::Back||(a==Action::Ok&&model.option==0)){session.resume();nextFrame=0;return;}
            if(a==Action::Ok){
                if(model.option==1||model.option==2||model.option==3||model.option==7){pendingAction=model.option;model.screen=Screen::Confirm;model.message=model.option==7?"Salvar Quick Resume e sair da sessão?\nSe o núcleo não suportar snapshot, manter SRAM.\nOK confirma · Voltar cancela":"Confirmar esta operação?\nSave anterior fica em arquivo .prev.\nAo carregar estado, SRAM atual é preservada.\nOK confirma · Voltar cancela";}
                if(model.option==4){model.screen=Screen::Controls;model.message="P1/P2: ordem de conexão dos gamepads.\nMenu ou Voltar: menu rápido recuperável.\nSTART + SELECT: menu rápido pelo gamepad.\nD-pad + OK: navegação da biblioteca.\nDesconectar gamepad pausa a sessão.\nRemapeamento: próximo marco.\nVoltar: menu rápido.";}
                if(model.option==5)session.stretch=!session.stretch;
                if(model.option==6&&!session.host.switchDisc()){model.screen=Screen::Error;model.message="Troca de disco não disponível neste núcleo.";}
                if(model.option==8){model.screen=Screen::Diagnostics;model.message=session.metrics();}
            }
            return;
        }
        if(model.screen==Screen::Settings) {
            if(a==Action::Back){model.navigate(a);return;}
            if(a==Action::Up)model.option=std::max(0,model.option-1);
            if(a==Action::Down)model.option=std::min(9,model.option+1);
            if(a==Action::Ok && model.option==2)model.minimal=!model.minimal;
            if(a==Action::Ok && model.option==3)model.performance=!model.performance;
            if(a==Action::Ok && model.option==0)platform.call("request","choose");
            if(a==Action::Ok && model.option==1)platform.call("request","scan");
            if(a==Action::Ok) {
                platform.call("setting",std::string("minimal\t")+(model.minimal?"1":"0"));
                platform.call("setting",std::string("performance\t")+(model.performance?"1":"0"));
                if(model.option==7){model.screen=Screen::Diagnostics;model.message=platform.call("diagnostics");}
                if(model.option==8){model.screen=Screen::Diagnostics;model.message="RetroTV Lite · 0.1.0-dev\nCódigo original GPL-2.0-or-later\nSDL2: zlib · QuickNES: GPL-2.0-or-later\nFontes e licenças acompanham o projeto.\nSem jogos comerciais ou BIOS incluídos.\nVoltar retorna à biblioteca.";}
                if(model.option==9){Game demo;demo.id="retrotv-demo-cc0";demo.title="Teste original RetroTV";demo.system="NES";demo.uri="demo:retrotv-pad-test.nes";demo.core="quicknes";if(!session.launch(demo,platform,false)){model.screen=Screen::Error;model.message=session.error;}else{nextFrame=0;model.screen=Screen::QuickMenu;model.option=0;}}
            }
            return;
        }
        if(model.screen==Screen::Details && a==Action::Ok) {
            auto* g=model.selected(); if(!g)return;
            if(model.option==2){platform.call("request","favorite\t"+g->id+"\t"+(g->favorite?"0":"1"));loadCatalog();return;}
            if(model.option==3){model.screen=Screen::Library;return;}
            bool resume=model.option==1;
            if(!session.launch(*g,platform,resume)){model.screen=Screen::Error;model.message=session.error+"\nVoltar retorna à biblioteca.";}
            else {nextFrame=0;model.screen=Screen::QuickMenu;model.option=0;}
            return;
        }
        model.navigate(a);
    };
    while(running) {
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type==SDL_QUIT)running=false;
            if(e.type==SDL_APP_WILLENTERBACKGROUND){background=true;if(session.active()){session.pause();session.save(true);model.screen=Screen::QuickMenu;model.option=0;}}
            if(e.type==SDL_APP_DIDENTERFOREGROUND)background=false;
            if(e.type==SDL_CONTROLLERDEVICEADDED) {auto* c=SDL_GameControllerOpen(e.cdevice.which);if(c)controllers[SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(c))]=c;}
            if(e.type==SDL_CONTROLLERDEVICEREMOVED) {auto it=controllers.find(e.cdevice.which);if(it!=controllers.end()){SDL_GameControllerClose(it->second);controllers.erase(it);}if(session.active()){session.pause();model.screen=Screen::Error;model.message="Controle desconectado. Reconecte para continuar.\nOK ou Voltar abre o menu rápido.";}}
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
                if(session.host.state==SessionState::Running){
                    auto it=controllers.find(e.cbutton.which);
                    if(it!=controllers.end()&&SDL_GameControllerGetButton(it->second,SDL_CONTROLLER_BUTTON_START)&&SDL_GameControllerGetButton(it->second,SDL_CONTROLLER_BUTTON_BACK))action(Action::Menu);
                    continue;
                }
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
        if(session.host.state==SessionState::Running){
            session.host.buttons={};session.host.axes={};int port=0;
            const int mapping[16]={SDL_CONTROLLER_BUTTON_B,SDL_CONTROLLER_BUTTON_Y,SDL_CONTROLLER_BUTTON_BACK,SDL_CONTROLLER_BUTTON_START,SDL_CONTROLLER_BUTTON_DPAD_UP,SDL_CONTROLLER_BUTTON_DPAD_DOWN,SDL_CONTROLLER_BUTTON_DPAD_LEFT,SDL_CONTROLLER_BUTTON_DPAD_RIGHT,SDL_CONTROLLER_BUTTON_A,SDL_CONTROLLER_BUTTON_X,SDL_CONTROLLER_BUTTON_LEFTSHOULDER,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,-1,-1,SDL_CONTROLLER_BUTTON_LEFTSTICK,SDL_CONTROLLER_BUTTON_RIGHTSTICK};
            for(auto& item:controllers){if(port>=2)break;for(int id=0;id<16;++id)if(mapping[id]>=0)session.host.buttons[port][id]=SDL_GameControllerGetButton(item.second,SDL_GameControllerButton(mapping[id]));for(int axis=0;axis<4;++axis){auto value=SDL_GameControllerGetAxis(item.second,SDL_GameControllerAxis(axis));session.host.axes[port][axis]=std::abs(int(value))<8000?0:value;}++port;}
            auto* keys=SDL_GetKeyboardState(nullptr);auto& buttons=session.host.buttons[0];
            buttons[RETRO_DEVICE_ID_JOYPAD_UP]|=keys[SDL_SCANCODE_UP];buttons[RETRO_DEVICE_ID_JOYPAD_DOWN]|=keys[SDL_SCANCODE_DOWN];buttons[RETRO_DEVICE_ID_JOYPAD_LEFT]|=keys[SDL_SCANCODE_LEFT];buttons[RETRO_DEVICE_ID_JOYPAD_RIGHT]|=keys[SDL_SCANCODE_RIGHT];buttons[RETRO_DEVICE_ID_JOYPAD_A]|=keys[SDL_SCANCODE_RETURN]||keys[SDL_SCANCODE_X];buttons[RETRO_DEVICE_ID_JOYPAD_B]|=keys[SDL_SCANCODE_Z];buttons[RETRO_DEVICE_ID_JOYPAD_START]|=keys[SDL_SCANCODE_SPACE];buttons[RETRO_DEVICE_ID_JOYPAD_SELECT]|=keys[SDL_SCANCODE_TAB];
            auto now=SDL_GetPerformanceCounter();if(!nextFrame)nextFrame=now;
            if(now<nextFrame)SDL_Delay(Uint32((nextFrame-now)*1000/SDL_GetPerformanceFrequency()));
            session.tick();session.draw();SDL_RenderPresent(renderer);
            nextFrame+=uint64_t(double(SDL_GetPerformanceFrequency())/session.host.av.timing.fps);if(SDL_GetPerformanceCounter()>nextFrame+SDL_GetPerformanceFrequency()/4)nextFrame=SDL_GetPerformanceCounter();
        }else {ui.draw(model,platform.call("status"));SDL_RenderPresent(renderer);SDL_Delay(model.performance?20:4);}
    }
    session.finish(platform);for(auto& c:controllers)SDL_GameControllerClose(c.second);
    SDL_DestroyTexture(atlas);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 0;
}
