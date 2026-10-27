#include <SDL.h>
#include "ui/model.hpp"
#include "ui/renderer.hpp"
#include "platform.hpp"
#include "emulation/session.hpp"
#include "input/frame_input.hpp"
#include <map>
#include <cstdio>
using namespace rt;
int main(int, char**) {
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON,"1");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER,"opengles2");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"1");
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_GAMECONTROLLER)!=0)return 1;
    auto* window=SDL_CreateWindow("RetroTV Lite",0,0,1280,720,SDL_WINDOW_FULLSCREEN_DESKTOP);
    auto* renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if(!renderer){SDL_Quit();return 2;}
    SDL_RenderSetLogicalSize(renderer,1280,720);
    Platform platform; auto* atlas=platform.font(renderer); UI ui(renderer,atlas,platform.call("fontMetrics"),[&](const std::string& uri){return platform.cover(renderer,uri);}); Model model;
    model.minimal=platform.call("setting","minimal")=="1";
    model.performance=platform.call("setting","performance")=="1";
    model.phoneEnabled=platform.call("setting","phoneSync")=="1";std::string lastPhoneContext;
    auto savedTab=platform.call("setting","tab");if(!savedTab.empty())model.tab=std::clamp(std::atoi(savedTab.c_str()),0,8);if(model.tab==1||model.tab==2)model.tab=0;model.headerFocus=model.tab;
    model.sidebar=false; // open on the main action (first row or "Adicionar jogos"), not on a header tab
    model.lastContentTab=model.tab;
    auto savedPositions=split(platform.call("setting","positions"),',');if(savedPositions.size()==10)for(int i=0;i<10;++i)model.positions[i]=std::max(0,std::atoi(savedPositions[i].c_str()));model.selection=model.positions[model.tab];
    auto saveFocus=[&]{model.positions[model.tab]=model.selection;platform.call("setting","tab\t"+std::to_string(model.lastContentTab));std::string value;for(auto n:model.positions){if(!value.empty())value+=",";value+=std::to_string(n);}platform.call("setting","positions\t"+value);};
    auto loadCatalog=[&] {
        const auto old=model.selected()?model.selected()->id:"";
        ui.clearCovers();ui.clearArt();platform.call("clearArtwork");
        model.games.clear();
        for(const auto& row:split(platform.call("catalog"),'\n')) {
            auto p=split(row,'\t'); if(p.size()!=10)continue;
            Game g;g.id=p[0];g.title=p[1];g.system=p[2];g.uri=p[3];g.core=p[4];g.favorite=p[5]=="1";
            g.recent=std::strtoll(p[6].c_str(),nullptr,10);g.played=std::strtoll(p[7].c_str(),nullptr,10);g.available=p[8]=="1";
            g.cover=p[9];
            model.games.push_back(g);
        }
        auto rows=model.visible();for(size_t i=0;i<rows.size();++i)if(model.games[rows[i]].id==old)model.selection=int(i);
    };
    loadCatalog(); std::string revision=platform.call("revision"); uint32_t lastRefresh=0;
    Session session(renderer);ui.pausedBackdrop=[&]{session.draw();};int pendingAction=-1,remapTarget=-1;uint64_t nextFrame=0;
    const std::array<int,16> defaultMapping={SDL_CONTROLLER_BUTTON_B,SDL_CONTROLLER_BUTTON_Y,SDL_CONTROLLER_BUTTON_BACK,SDL_CONTROLLER_BUTTON_START,SDL_CONTROLLER_BUTTON_DPAD_UP,SDL_CONTROLLER_BUTTON_DPAD_DOWN,SDL_CONTROLLER_BUTTON_DPAD_LEFT,SDL_CONTROLLER_BUTTON_DPAD_RIGHT,SDL_CONTROLLER_BUTTON_A,SDL_CONTROLLER_BUTTON_X,SDL_CONTROLLER_BUTTON_LEFTSHOULDER,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,-2,-3,SDL_CONTROLLER_BUTTON_LEFTSTICK,SDL_CONTROLLER_BUTTON_RIGHTSTICK};
    std::map<SDL_JoystickID,std::array<int,16>> mappings;std::array<SDL_JoystickID,2> players{{-1,-1}};
    auto controllerKey=[](SDL_GameController* c){char guid[64];SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(SDL_GameControllerGetJoystick(c)),guid,sizeof(guid));return std::string("pad-")+guid;};
    std::array<int16_t,16> remotePulse{},remoteHeld{};
    // TV remote (and Google TV remote) keys mapped to RetroPad ids while a game runs.
    auto remoteKey=[](SDL_Keycode k){return k==SDLK_UP?4:k==SDLK_DOWN?5:k==SDLK_LEFT?6:k==SDLK_RIGHT?7:(k==SDLK_RETURN||k==SDLK_SELECT)?8:k==SDLK_z?0:k==SDLK_SPACE?3:k==SDLK_TAB?2:-1;};
    bool running=true,background=false; std::map<SDL_JoystickID,SDL_GameController*> controllers;
    // RetroTV Pad over RFCOMM/Wi-Fi becomes a virtual SDL game controller, so menus, remapping and players treat it like a gamepad.
    SDL_Joystick* phonePad=nullptr;int phoneIndex=-1;std::string phoneCode;bool phoneLinked=false;
    auto phoneInput=[&]{
        auto value=platform.call("phoneInput");if(value.empty())return;
        if(value=="off"){if(phonePad)SDL_JoystickClose(phonePad);phonePad=nullptr;if(phoneIndex>=0)SDL_JoystickDetachVirtual(phoneIndex);phoneIndex=-1;return;}
        unsigned buttons=0;int dpad=0,lx=0,ly=0,rx=0,ry=0;
        if(std::sscanf(value.c_str(),"%x %d %d %d %d %d",&buttons,&dpad,&lx,&ly,&rx,&ry)!=6)return;
        if(!phonePad){
            SDL_VirtualJoystickDesc desc;SDL_zero(desc);desc.version=SDL_VIRTUAL_JOYSTICK_DESC_VERSION;desc.type=SDL_JOYSTICK_TYPE_GAMECONTROLLER;
            desc.naxes=SDL_CONTROLLER_AXIS_MAX;desc.nbuttons=16;desc.button_mask=(1u<<16)-1;desc.axis_mask=(1u<<SDL_CONTROLLER_AXIS_MAX)-1;desc.name="RetroTV Pad";
            phoneIndex=SDL_JoystickAttachVirtualEx(&desc);if(phoneIndex<0)return;phonePad=SDL_JoystickOpen(phoneIndex);if(!phonePad)return;
        }
        // Pad bits follow the standard gamepad order used by its HID mode: A B X Y L1 R1 L2 R2 Select Start L3 R3.
        static const int bits[12]={SDL_CONTROLLER_BUTTON_A,SDL_CONTROLLER_BUTTON_B,SDL_CONTROLLER_BUTTON_X,SDL_CONTROLLER_BUTTON_Y,SDL_CONTROLLER_BUTTON_LEFTSHOULDER,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,-1,-1,SDL_CONTROLLER_BUTTON_BACK,SDL_CONTROLLER_BUTTON_START,SDL_CONTROLLER_BUTTON_LEFTSTICK,SDL_CONTROLLER_BUTTON_RIGHTSTICK};
        for(int i=0;i<12;++i)if(bits[i]>=0)SDL_JoystickSetVirtualButton(phonePad,bits[i],(buttons>>i)&1);
        SDL_JoystickSetVirtualButton(phonePad,SDL_CONTROLLER_BUTTON_GUIDE,(buttons>>16)&1); // Pad "Home" key
        SDL_JoystickSetVirtualButton(phonePad,SDL_CONTROLLER_BUTTON_MISC1,(buttons>>17)&1); // Pad "Retornar" key
        SDL_JoystickSetVirtualButton(phonePad,SDL_CONTROLLER_BUTTON_DPAD_UP,dpad&1);SDL_JoystickSetVirtualButton(phonePad,SDL_CONTROLLER_BUTTON_DPAD_DOWN,(dpad>>1)&1);
        SDL_JoystickSetVirtualButton(phonePad,SDL_CONTROLLER_BUTTON_DPAD_LEFT,(dpad>>2)&1);SDL_JoystickSetVirtualButton(phonePad,SDL_CONTROLLER_BUTTON_DPAD_RIGHT,(dpad>>3)&1);
        const int axes[4]={lx,ly,rx,ry};for(int i=0;i<4;++i)SDL_JoystickSetVirtualAxis(phonePad,i,Sint16(std::clamp(axes[i]*258,-32767,32767)));
        SDL_JoystickSetVirtualAxis(phonePad,SDL_CONTROLLER_AXIS_TRIGGERLEFT,(buttons>>6)&1?32767:0);SDL_JoystickSetVirtualAxis(phonePad,SDL_CONTROLLER_AXIS_TRIGGERRIGHT,(buttons>>7)&1?32767:0);
    };
    auto refreshPlayback=[&]{
        auto reader=[&](const std::string& key){return platform.call("setting",key);};
        auto profile=readPlaybackProfile(model.profileGame,reader,model.profileSystem);
        model.profileAspect=profile.aspect;model.profileFilter=profile.filter;model.profileVolume=profile.volume;
        model.profileHint=model.profileSystem?"Padrão para novos jogos deste console. Ajustes do jogo têm prioridade.":"Suas escolhas ficam salvas para este jogo.";
        if(session.active())session.applyPlayback(readPlaybackProfile(session.game,reader));
    };
    auto openPlayback=[&](const Game& game){model.profileGame=game;model.profileSystem=false;model.screen=Screen::Playback;model.option=0;refreshPlayback();};
    // Home/Guide: back to Início. In a game it pauses on "Salvar e voltar à biblioteca", so a stray press never drops progress.
    auto goHome=[&]{
        if(session.active()){if(session.host.state==SessionState::Running)session.pause();model.screen=Screen::QuickMenu;model.option=7;return;}
        model.selectTab(0);model.screen=Screen::Library;model.sidebar=false;model.row=0;model.inResults=false;model.fromSearch=false;
    };
    auto action=[&](Action a) {
        if(model.screen==Screen::Playback){
            if(a==Action::Back||(a==Action::Ok&&model.option==5)){model.screen=session.active()?Screen::QuickMenu:Screen::Details;model.option=session.active()?5:3;return;}
            if(a==Action::Up)model.option=std::max(0,model.option-1);
            if(a==Action::Down)model.option=std::min(5,model.option+1);
            if(a==Action::Ok){
                auto write=[&](const std::string& field,const std::string& value){platform.call("setting",playbackKey(field,model.profileSystem,model.profileGame)+"\t"+value);};
                if(model.option==0)model.profileSystem=!model.profileSystem;
                if(model.option==1)write("aspect",nextAspect(model.profileAspect));
                if(model.option==2)write("filter",model.profileFilter=="nearest"?"linear":"nearest");
                if(model.option==3)write("volume",std::to_string(model.profileVolume==100?0:std::min(100,model.profileVolume+25)));
                if(model.option==4){write("aspect","");write("filter","");write("volume","");if(!model.profileSystem)platform.call("setting","aspect-"+model.profileGame.id+"\t");}
                refreshPlayback();
            }
            return;
        }
        if(!session.active()&&model.screen==Screen::Library&&model.sidebar&&a==Action::Back){saveFocus();running=false;return;}
        if(!session.active()&&model.screen==Screen::Library&&!model.sidebar&&model.visible().empty()&&a==Action::Ok) {
            if(!model.games.empty()){if(model.selection==0){model.selectTab(0);model.sidebar=false;}else platform.call("request","choose");return;}
            if(model.selection==0){platform.call("request","choose");return;}
            Game demo;demo.id="retrotv-demo-cc0";demo.title="Teste original RetroTV";demo.system="NES";demo.uri="demo:retrotv-pad-test.nes";demo.core="quicknes";
            ui.clearCovers();ui.clearArt();platform.call("clearArtwork");if(!session.launch(demo,platform,false)){model.screen=Screen::Error;model.message=session.error;}else{nextFrame=0;model.sessionGame=session.game;model.screen=Screen::QuickMenu;model.option=0;}return;
        }
        if(model.screen==Screen::Controls){
            if(a==Action::Back){remapTarget=-1;model.screen=session.active()?Screen::QuickMenu:Screen::Settings;model.option=0;return;}
            if(a==Action::Up)model.option=std::max(0,model.option-1);
            if(a==Action::Down)model.option=std::min(15,model.option+1);
            if(a==Action::Ok){remapTarget=model.option;model.message="Pressione o botão do gamepad para este comando.\nVoltar cancela; START + SELECT sempre abre o menu.";}
            return;
        }
        if(!session.active() && model.screen==Screen::Confirm){
            if(a==Action::Ok){platform.call("request",pendingAction==100?"restore":"bios");model.screen=Screen::Settings;}
            if(a==Action::Back)model.screen=Screen::Settings;
            return;
        }
        if(session.active()) {
            if(model.screen==Screen::Error){if(a==Action::Ok||a==Action::Back){model.screen=Screen::QuickMenu;model.option=0;}return;}
            if(model.screen==Screen::Diagnostics){if(a==Action::Back){model.screen=Screen::QuickMenu;model.option=0;}return;}
            if(model.screen==Screen::Confirm){
                if(a==Action::Back){model.screen=Screen::QuickMenu;return;}
                if(a!=Action::Ok)return;
                bool ok=true;
                if(pendingAction==1)ok=session.save(true);
                if(pendingAction==2)ok=session.load();
                if(pendingAction==3)session.host.reset();
                if(pendingAction==7){session.finish(platform);model.selectTab(model.lastContentTab);loadCatalog();model.screen=Screen::Library;return;}
                model.screen=ok?Screen::QuickMenu:Screen::Error;model.message=session.error;return;
            }
            if(session.host.state==SessionState::Running){
                if(a==Action::Menu||a==Action::Back){session.pause();model.screen=Screen::QuickMenu;model.option=0;}return;
            }
            if(a==Action::Up)model.option=std::max(0,model.option-1);
            if(a==Action::Down)model.option=std::min(8,model.option+1);
            if(a==Action::Back||(a==Action::Ok&&model.option==0)){ui.clearCovers();ui.clearArt();platform.call("clearArtwork");session.resume();nextFrame=0;return;}
            if(a==Action::Ok){
                if(model.option==1||model.option==2||model.option==3||model.option==7){pendingAction=model.option;model.screen=Screen::Confirm;model.message=model.option==7?"Salvar Quick Resume e sair da sessão?\nSe o núcleo não suportar snapshot, manter SRAM.\nOK confirma · Voltar cancela":"Confirmar esta operação?\nSave anterior fica em arquivo .prev.\nAo carregar estado, SRAM atual é preservada.\nOK confirma · Voltar cancela";}
                if(model.option==4){model.screen=Screen::Controls;model.option=0;model.message="OK: remapear com um botão do gamepad.\nConfiguração salva por modelo de controle.";return;}
                if(model.option==5){openPlayback(session.game);return;}
                if(model.option==6&&!session.host.switchDisc()){model.screen=Screen::Error;model.message="Troca de disco não disponível neste núcleo.";}
                if(model.option==8){model.screen=Screen::Diagnostics;model.message=session.metrics();}
            }
            return;
        }
        if(model.screen==Screen::Settings) {
            if(a==Action::Back){model.navigate(a);return;}
            if(a==Action::Up)model.option=std::max(0,model.option-1);
            if(a==Action::Down)model.option=std::min(11,model.option+1);
            if(a==Action::Ok && model.option==2)model.minimal=!model.minimal;
            if(a==Action::Ok && model.option==3)model.performance=!model.performance;
            if(a==Action::Ok && model.option==0)platform.call("request","choose");
            if(a==Action::Ok && model.option==1)platform.call("request","scan");
            if(a==Action::Ok) {
                platform.call("setting",std::string("minimal\t")+(model.minimal?"1":"0"));
                platform.call("setting",std::string("performance\t")+(model.performance?"1":"0"));
                if(model.option==7){model.screen=Screen::Diagnostics;model.message=platform.call("diagnostics");}
                if(model.option==8){model.screen=Screen::Diagnostics;model.message="RetroTV Lite · 0.7.3-dev\nCódigo original GPL-2.0-or-later\nSDL2: zlib · QuickNES: GPL-2.0-or-later\nFontes e licenças acompanham o projeto.\nSem jogos comerciais ou BIOS incluídos.\nVoltar retorna à biblioteca.";}
                if(model.option==9){Game demo;demo.id="retrotv-demo-cc0";demo.title="Teste original RetroTV";demo.system="NES";demo.uri="demo:retrotv-pad-test.nes";demo.core="quicknes";ui.clearCovers();ui.clearArt();platform.call("clearArtwork");if(!session.launch(demo,platform,false)){model.screen=Screen::Error;model.message=session.error;}else{nextFrame=0;model.sessionGame=session.game;model.screen=Screen::QuickMenu;model.option=0;}}
                if(model.option==4)platform.call("request","export");
                if(model.option==5){pendingAction=100;model.screen=Screen::Confirm;model.message="Restaurar um backup substituirá os saves atuais.\nUma cópia completa anterior será preservada.\nConfirme apenas backups do RetroTV.\nOK: escolher backup · Voltar: cancelar";}
                if(model.option==6){model.screen=Screen::Controls;model.option=0;model.message="OK: remapear com um botão do gamepad.\nConfiguração salva por modelo de controle.";}
                if(model.option==11){model.screen=Screen::Diagnostics;model.message=platform.call("phone","toggle");}
                if(model.option==10){pendingAction=101;model.screen=Screen::Confirm;model.message="Importar sua BIOS PS1 de 512 KiB?\nA anterior será preservada em .prev.\nRegião e compatibilidade devem ser conferidas.\nOK: escolher arquivo · Voltar: cancelar";}
            }
            return;
        }
        if(model.screen==Screen::Details && a==Action::Ok) {
            auto* g=model.selected(); if(!g)return;
            if(model.option==2){bool leave=model.tab==3&&g->favorite;platform.call("request","favorite\t"+g->id+"\t"+(g->favorite?"0":"1"));loadCatalog();if(leave)model.screen=Screen::Library;return;}
            if(model.option==3){openPlayback(*g);return;}
            if(model.option==4){model.screen=model.fromSearch?Screen::Search:Screen::Library;return;}
            bool resume=model.option==1;
            ui.clearCovers();ui.clearArt();platform.call("clearArtwork");if(!session.launch(*g,platform,resume)){model.screen=Screen::Error;model.message=session.error+"\nVoltar retorna à biblioteca.";}
            else {nextFrame=0;model.sessionGame=session.game;model.screen=Screen::QuickMenu;model.option=0;}
            return;
        }
        model.navigate(a);
        if(model.screen==Screen::Details){auto* selected=model.selected();model.resumeAvailable=selected&&platform.call("resume",selected->id)=="1";if((model.resumeFirst||(a==Action::Ok&&model.tab==1))&&model.resumeAvailable)model.option=1;model.resumeFirst=false;}
        saveFocus();
    };
    while(running) {
        // Pace first: the phone queue, SDL event pump and game sampling must all see input that arrived in the wait.
        if(session.host.state==SessionState::Running)
            waitForGameFrame(nextFrame,SDL_GetPerformanceCounter(),SDL_GetPerformanceFrequency(),SDL_Delay);
        // Per-frame JNI polling only while a phone is connected; the link state is refreshed every 500 ms.
        if(phoneLinked||phonePad)phoneInput();
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type==SDL_QUIT)running=false;
            if(e.type==SDL_RENDER_DEVICE_RESET){session.invalidateFrame();ui.clearCovers();SDL_DestroyTexture(atlas);atlas=platform.font(renderer);ui.setAtlas(atlas);}
            if(e.type==SDL_RENDER_TARGETS_RESET)ui.clearArt();
            if(e.type==SDL_APP_LOWMEMORY){ui.clearCovers();ui.clearArt();model.performance=true;platform.call("setting","performance\t1");}
            if(e.type==SDL_APP_WILLENTERBACKGROUND){background=true;saveFocus();if(session.active()){session.pause();session.save(true);model.screen=Screen::QuickMenu;model.option=0;}}
            if(e.type==SDL_APP_DIDENTERFOREGROUND)background=false;
            if(e.type==SDL_CONTROLLERDEVICEADDED) {auto* c=SDL_GameControllerOpen(e.cdevice.which);if(c){auto id=SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(c));controllers[id]=c;mappings[id]=defaultMapping;auto saved=split(platform.call("setting",controllerKey(c)),',');if(saved.size()==16)for(int i=0;i<16;++i){auto v=std::atoi(saved[i].c_str());if(v>=-3&&v<SDL_CONTROLLER_BUTTON_MAX)mappings[id][i]=v;}
                // The phone pad (RetroTV channel) is always player 1; whoever held it, e.g. Google TV's "virtual-remote"
                // (a gamepad-class device with remote keys only), moves to player 2 and stays usable as a fallback.
                if(phonePad&&id==SDL_JoystickInstanceID(phonePad)){if(players[0]>=0&&players[1]<0)players[1]=players[0];players[0]=id;}
                else for(auto& slot:players)if(slot<0){slot=id;break;}}}
            if(e.type==SDL_CONTROLLERDEVICEREMOVED) {auto it=controllers.find(e.cdevice.which);if(it!=controllers.end()){SDL_GameControllerClose(it->second);controllers.erase(it);}mappings.erase(e.cdevice.which);for(auto& id:players)if(id==e.cdevice.which)id=-1;if(session.active()){session.pause();model.screen=Screen::Error;model.message="Controle desconectado. Reconecte para continuar.\nOK ou Voltar abre o menu rápido.";}}
            // Remote keys count while held (a quick tap still lasts one frame through remotePulse).
            if(e.type==SDL_KEYUP){int id=remoteKey(e.key.keysym.sym);if(id>=0)remoteHeld[id]=0;}
            if(e.type==SDL_KEYDOWN) {
                if(session.host.state==SessionState::Running){int id=remoteKey(e.key.keysym.sym);if(id>=0){remotePulse[id]=1;remoteHeld[id]=1;}}
                switch(e.key.keysym.sym) {
                    case SDLK_UP:action(Action::Up);break;case SDLK_DOWN:action(Action::Down);break;
                    case SDLK_LEFT:action(Action::Left);break;case SDLK_RIGHT:action(Action::Right);break;
                    case SDLK_RETURN:case SDLK_KP_ENTER:case SDLK_SELECT:action(Action::Ok);break;
                    case SDLK_ESCAPE:case SDLK_AC_BACK:action(Action::Back);break;
                    case SDLK_MENU:case SDLK_F1:action(Action::Menu);break;default:break;
                }
            }
            if(e.type==SDL_CONTROLLERBUTTONDOWN) {
                if(model.screen==Screen::Controls && remapTarget>=0){auto it=controllers.find(e.cbutton.which);if(it!=controllers.end()){mappings[e.cbutton.which][remapTarget]=e.cbutton.button;std::string value;for(auto v:mappings[e.cbutton.which]){if(!value.empty())value+=",";value+=std::to_string(v);}platform.call("setting",controllerKey(it->second)+"\t"+value);remapTarget=-1;model.message="Mapeamento salvo para este modelo de gamepad.";}continue;}
                if(e.cbutton.button==SDL_CONTROLLER_BUTTON_GUIDE){goHome();continue;}
                // The phone's "Retornar" key: one step back everywhere (in a game, Back opens the pause menu). It never
                // closes RetroTV from the home header, which would also drop the phone's own connection.
                if(e.cbutton.button==SDL_CONTROLLER_BUTTON_MISC1&&phonePad&&e.cbutton.which==SDL_JoystickInstanceID(phonePad)){
                    if(!(!session.active()&&model.screen==Screen::Library&&model.sidebar))action(Action::Back);continue;}
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
        if(SDL_GetTicks()-lastRefresh>500){auto next=platform.call("revision");if(next!=revision){loadCatalog();revision=next;}model.phoneEnabled=platform.call("setting","phoneSync")=="1";phoneCode=model.phoneEnabled?platform.call("phoneOverlay"):"";phoneLinked=model.phoneEnabled&&platform.call("phoneLinked")=="1";lastRefresh=SDL_GetTicks();}
        if(model.phoneEnabled){
            Game* contextGame=session.active()?&session.game:((model.screen==Screen::Library||model.screen==Screen::Details||model.screen==Screen::Playback)?model.selected():nullptr);
            std::string context=(contextGame?contextGame->system:"")+"\t"+(contextGame?contextGame->title:"")+"\t"+(session.host.state==SessionState::Running?"running":session.active()?"paused":"library");
            if(context!=lastPhoneContext){platform.call("controllerContext",context);lastPhoneContext=context;}
        }else lastPhoneContext.clear();
        if(session.host.state==SessionState::Running){
            sampleGamepads(controllers,players,mappings,session.host.buttons,session.host.axes);
            auto* keys=SDL_GetKeyboardState(nullptr);auto& buttons=session.host.buttons[0];
            for(int id=0;id<16;++id)buttons[id]|=remotePulse[id]|remoteHeld[id];remotePulse={};
            buttons[RETRO_DEVICE_ID_JOYPAD_UP]|=keys[SDL_SCANCODE_UP];buttons[RETRO_DEVICE_ID_JOYPAD_DOWN]|=keys[SDL_SCANCODE_DOWN];buttons[RETRO_DEVICE_ID_JOYPAD_LEFT]|=keys[SDL_SCANCODE_LEFT];buttons[RETRO_DEVICE_ID_JOYPAD_RIGHT]|=keys[SDL_SCANCODE_RIGHT];buttons[RETRO_DEVICE_ID_JOYPAD_A]|=keys[SDL_SCANCODE_RETURN]||keys[SDL_SCANCODE_X]||keys[SDL_SCANCODE_SELECT];buttons[RETRO_DEVICE_ID_JOYPAD_B]|=keys[SDL_SCANCODE_Z];buttons[RETRO_DEVICE_ID_JOYPAD_START]|=keys[SDL_SCANCODE_SPACE];buttons[RETRO_DEVICE_ID_JOYPAD_SELECT]|=keys[SDL_SCANCODE_TAB];
            // Launch/resume can happen in the event pump after the pacing check above.
            if(!nextFrame)nextFrame=SDL_GetPerformanceCounter();
            session.tick();if(session.host.state==SessionState::Paused&&!session.error.empty()){model.screen=Screen::Error;model.message=session.error;}session.draw();if(!phoneCode.empty())ui.phoneCode(phoneCode);SDL_RenderPresent(renderer);session.presented();
            nextFrame+=uint64_t(double(SDL_GetPerformanceFrequency())/session.host.av.timing.fps);if(SDL_GetPerformanceCounter()>nextFrame+SDL_GetPerformanceFrequency()/4)nextFrame=SDL_GetPerformanceCounter();
        }else {remoteHeld={};model.sessionActive=session.active();ui.draw(model,platform.call("status"));if(!phoneCode.empty())ui.phoneCode(phoneCode);SDL_RenderPresent(renderer);SDL_Delay(model.performance?20:4);}
    }
    session.finish(platform);for(auto& c:controllers)SDL_GameControllerClose(c.second);
    ui.release();SDL_DestroyTexture(atlas);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 0;
}
