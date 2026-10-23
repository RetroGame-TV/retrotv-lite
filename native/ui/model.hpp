#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace rt {
enum class Screen { Library, Details, Settings, QuickMenu, Confirm, Error, Diagnostics, Controls };
enum class Action { Up, Down, Left, Right, Ok, Back, Menu, Favorite };
struct Game {
    std::string id, title, system, uri, core;
    bool favorite = false, available = true;
    int64_t recent = 0, played = 0;
};
inline const std::array<const char*, 10> tabs = {"Início", "Continuar", "Recentes", "Favoritos", "NES", "SNES", "Mega Drive", "Arcade", "PlayStation", "Configurações"};
struct Theme {
    uint32_t background, panel, card, accent, text, muted;
    int cacheMiB; bool minimal;
    static Theme classic() { return {0x0D141C,0x141F2A,0x1D2A36,0x73E0B4,0xF2F6F8,0xA7B8C7,8,false}; }
    static Theme minimalTheme() { return {0x101215,0x101215,0x1C2025,0x73E0B4,0xF5F7FA,0xB3BDC7,2,true}; }
};
struct Model {
    Screen screen = Screen::Library;
    int tab = 0, selection = 0, option = 0, lastContentTab = 0;
    bool sidebar = true, performance = false, minimal = false;
    std::array<int,10> positions{};
    std::vector<Game> games;
    std::string message;
    std::vector<size_t> visible() const {
        std::vector<size_t> out;
        for(size_t i=0;i<games.size();++i) {
            const auto& g=games[i];
            if(tab==3 && !g.favorite) continue;
            if((tab==1||tab==2) && g.recent==0) continue;
            if(tab>=4 && tab<=8 && g.system!=tabs[tab]) continue;
            out.push_back(i);
        }
        if(tab==1||tab==2) std::stable_sort(out.begin(),out.end(),[&](size_t a,size_t b){return games[a].recent>games[b].recent;});
        return out;
    }
    Game* selected() { auto rows=visible(); if(rows.empty()) return nullptr; selection=std::clamp(selection,0,int(rows.size()-1)); return &games[rows[selection]]; }
    void selectTab(int value) { positions[tab]=selection; tab=std::clamp(value,0,9); selection=positions[tab]; if(tab==9) { screen=Screen::Settings; option=0; } else lastContentTab=tab; }
    void navigate(Action a) {
        if(screen==Screen::Error) { if(a==Action::Back||a==Action::Ok) screen=Screen::Library; return; }
        if(screen==Screen::Details) { if(a==Action::Back) screen=Screen::Library; else if(a==Action::Up) option=std::max(0,option-1); else if(a==Action::Down) option=std::min(3,option+1); return; }
        if(screen!=Screen::Library) { if(a==Action::Back) { screen=Screen::Library; sidebar=true; if(tab==9)selectTab(lastContentTab); } return; }
        if(a==Action::Menu) { screen=Screen::Settings; option=0; return; }
        if(sidebar) {
            if(a==Action::Up) selectTab(tab-1);
            if(a==Action::Down) selectTab(tab+1);
            if(a==Action::Right||a==Action::Ok) sidebar=false;
        } else {
            const auto count=visible().size();
            if(a==Action::Left||a==Action::Back) sidebar=true;
            if(a==Action::Down && count) selection=std::min(selection+1,int(count-1));
            if(a==Action::Up) selection=std::max(selection-1,0);
            if(a==Action::Right && count) selection=std::min(selection+1,int(count-1));
            if(a==Action::Ok && selected()) { screen=Screen::Details; option=0; }
        }
    }
};
inline std::string coreFor(const std::string& system) {
    if(system=="NES") return "quicknes";
    if(system=="SNES") return "snes9x2005";
    if(system=="Mega Drive"||system=="Master System") return "picodrive";
    if(system=="Game Boy") return "gambatte";
    if(system=="Arcade") return "fbneo";
    if(system=="PlayStation") return "pcsx_rearmed";
    return "";
}
struct Profile { int maxRomMiB=32; bool disc=false; float aspect=4.f/3.f; int cacheMiB=8; };
inline Profile profileFor(const std::string& system, bool performance) {
    Profile p; p.disc=(system=="PlayStation"); p.maxRomMiB=p.disc?0:32; p.cacheMiB=performance?2:8; return p;
}
}
