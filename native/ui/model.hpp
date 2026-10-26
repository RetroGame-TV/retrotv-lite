#pragma once
#include <algorithm>
#include <cctype>
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace rt {
enum class Screen { Library, Details, Settings, QuickMenu, Confirm, Error, Diagnostics, Controls, Playback, Search };
enum class Action { Up, Down, Left, Right, Ok, Back, Menu, Favorite };
struct Game {
    std::string id, title, system, uri, core;
    bool favorite = false, available = true;
    int64_t recent = 0, played = 0;
    std::string cover;
};
inline const std::array<const char*, 10> tabs = {"Início", "Continuar", "Recentes", "Favoritos", "NES", "SNES", "Mega Drive", "Arcade", "PlayStation", "Configurações"};
// Header codes: collection tabs keep their index; 10 is search. Home rows: 0 continue, 1 consoles, 2 whole library.
constexpr int SearchItem = 10;
inline const std::array<const char*, 4> keyRows = {"abcdefghij", "klmnopqrst", "uvwxyz.-:&", "1234567890"};
// Lowercase ASCII and strip Latin-1 accents (UTF-8 0xC3 xx) so "pokemon" finds "Pokémon".
inline std::string fold(const std::string& s) {
    std::string out;
    for(size_t i=0;i<s.size();++i) {
        auto c=static_cast<unsigned char>(s[i]);
        if(c==0xC3&&i+1<s.size()) {
            auto d=static_cast<unsigned char>(s[++i])|0x20;
            out+=d>=0xA0&&d<=0xA5?'a':d==0xA7?'c':d>=0xA8&&d<=0xAB?'e':d>=0xAC&&d<=0xAF?'i':d==0xB1?'n':d>=0xB2&&d<=0xB6?'o':d>=0xB9&&d<=0xBC?'u':d==0xBD||d==0xBF?'y':'?';
        } else out+=char(c<128?std::tolower(c):c);
    }
    return out;
}
struct Theme {
    uint32_t background, panel, card, accent, text, muted;
    int cacheMiB; bool minimal;
    static Theme classic() { return {0x161519,0x1E1D23,0x29272F,0xB699FF,0xF7F5FA,0xA4A0AF,8,false}; }
    static Theme minimalTheme() { return {0x101012,0x19191C,0x27272B,0xF4F3F6,0xF7F5FA,0x9B99A2,2,true}; }
};
struct Model {
    Screen screen = Screen::Library;
    int tab = 0, selection = 0, option = 0, lastContentTab = 0;
    bool sidebar = true, performance = false, minimal = false, phoneEnabled = false;
    int headerFocus = 0, row = 0, key = 0, result = 0;
    std::array<int,3> rowCol{};
    std::string query;
    bool inResults = false, fromSearch = false, resumeFirst = false;
    std::array<int,10> positions{};
    std::vector<Game> games;
    std::string message;
    Game sessionGame; bool sessionActive=false,resumeAvailable=false;
    Game profileGame; bool profileSystem=false; std::string profileAspect="4:3",profileFilter="nearest",profileHint; int profileVolume=100;
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
    bool home() const { return tab==0 && !minimal && !performance && !games.empty(); }
    std::vector<int> headerItems() const { std::vector<int> items{0,3}; if(tab>=4&&tab<=8)items.push_back(tab); items.push_back(SearchItem); items.push_back(9); return items; }
    std::vector<size_t> continueRow() const {
        std::vector<size_t> out; for(size_t i=0;i<games.size();++i) if(games[i].recent>0) out.push_back(i);
        std::stable_sort(out.begin(),out.end(),[&](size_t a,size_t b){return games[a].recent>games[b].recent;}); if(out.size()>8) out.resize(8); return out;
    }
    std::vector<int> homeRows() const { std::vector<int> rows; if(!continueRow().empty()) rows.push_back(0); rows.push_back(1); if(!games.empty()) rows.push_back(2); return rows; }
    int rowSize(int kind) const { return kind==0?int(continueRow().size()):kind==1?5:int(games.size()); }
    std::vector<size_t> searchResults() const {
        auto q=fold(query); while(!q.empty()&&q.back()==' ') q.pop_back();
        std::vector<std::pair<int,size_t>> hits;
        for(size_t i=0;i<games.size();++i) { auto title=fold(games[i].title); auto at=title.find(q);
            if(q.empty()) hits.push_back({1,i}); else if(at==0) hits.push_back({0,i}); else if(at!=std::string::npos||fold(games[i].system).find(q)==0) hits.push_back({1,i}); }
        std::stable_sort(hits.begin(),hits.end(),[&](const auto& a,const auto& b){return a.first!=b.first?a.first<b.first:fold(games[a.second].title)<fold(games[b.second].title);});
        std::vector<size_t> out; for(auto& h:hits) out.push_back(h.second); return out;
    }
    void toHeader() { sidebar=true; headerFocus=tab; }
    void openDetails(const std::string& id) {
        positions[tab]=selection; tab=0; lastContentTab=0; auto rows=visible();
        for(size_t i=0;i<rows.size();++i) if(games[rows[i]].id==id) selection=int(i);
        screen=Screen::Details; option=0;
    }
    void searchNavigate(Action a) {
        auto results=searchResults();
        if(a==Action::Back) { screen=Screen::Library; fromSearch=false; inResults=false; sidebar=true; headerFocus=SearchItem; return; }
        if(inResults) {
            if(a==Action::Left||results.empty()) { inResults=false; return; }
            if(a==Action::Up) result=std::max(0,result-1);
            if(a==Action::Down) result=std::min(result+1,int(results.size())-1);
            if(a==Action::Ok) { fromSearch=true; openDetails(games[results[std::clamp(result,0,int(results.size())-1)]].id); }
            return;
        }
        int r=key/10, c=key%10, group=c<=3?0:c<=6?1:2;
        if(a==Action::Up) r=std::max(0,r-1);
        if(a==Action::Down) { r=std::min(4,r+1); if(r==4) c=group==0?1:group==1?5:8; }
        if(a==Action::Left) { if(r==4) c=group==2?5:1; else c=std::max(0,c-1); }
        if(a==Action::Right) { if((r<4&&c==9)||(r==4&&group==2)) { if(!results.empty()) { inResults=true; result=std::min(result,int(results.size())-1); } } else if(r==4) c=group==0?5:8; else ++c; }
        key=r*10+c;
        if(a==Action::Ok) {
            if(r<4) { if(query.size()<32) query+=keyRows[r][c]; }
            else if(group==0) { if(!query.empty()&&query.back()!=' '&&query.size()<32) query+=' '; }
            else if(group==1) { if(!query.empty()) query.pop_back(); }
            else query.clear();
            result=0;
        }
    }
    Game* selected() { auto rows=visible(); if(rows.empty()) return nullptr; selection=std::clamp(selection,0,int(rows.size()-1)); return &games[rows[selection]]; }
    void selectTab(int value) { positions[tab]=selection; tab=std::clamp(value,0,9); selection=positions[tab]; if(tab==9) { screen=Screen::Settings; option=0; } else lastContentTab=tab; }
    void navigate(Action a) {
        if(screen==Screen::Error) { if(a==Action::Back||a==Action::Ok) screen=Screen::Library; return; }
        if(screen==Screen::Search) { searchNavigate(a); return; }
        if(screen==Screen::Details) {
            if(a==Action::Back)screen=fromSearch?Screen::Search:Screen::Library;
            if(a==Action::Left&&option==1)option=0;
            if(a==Action::Right&&option==0&&resumeAvailable)option=1;
            if(a==Action::Down)option=option<2?2:std::min(4,option+1);
            if(a==Action::Up)option=option<=2?0:option-1;
            return;
        }
        if(screen!=Screen::Library) { if(a==Action::Back) { screen=Screen::Library; sidebar=true; bool fromSettings=tab==9; if(fromSettings)selectTab(lastContentTab); headerFocus=fromSettings?9:tab; } return; }
        if(a==Action::Menu) { screen=Screen::Settings; option=0; return; }
        if(sidebar) {
            auto items=headerItems(); int at=0; for(size_t i=0;i<items.size();++i) if(items[i]==headerFocus) at=int(i);
            if(a==Action::Left||a==Action::Right) { at=std::clamp(at+(a==Action::Right?1:-1),0,int(items.size())-1); headerFocus=items[at]; if(headerFocus<=8&&headerFocus!=tab) selectTab(headerFocus); }
            if(a==Action::Ok&&headerFocus==SearchItem) { screen=Screen::Search; inResults=false; return; }
            if(a==Action::Ok&&headerFocus==9) { selectTab(9); return; }
            if(a==Action::Down||(a==Action::Ok&&headerFocus<=8)) { sidebar=false; headerFocus=tab; }
        } else if(home()) {
            auto rows=homeRows(); row=std::clamp(row,0,int(rows.size())-1); int kind=rows[row]; int count=rowSize(kind);
            if(a==Action::Up) { if(row==0) toHeader(); else --row; }
            if(a==Action::Back) toHeader();
            if(a==Action::Down) row=std::min(row+1,int(rows.size())-1);
            kind=rows[row]; count=rowSize(kind);
            if(a==Action::Left) rowCol[kind]=std::max(0,rowCol[kind]-1);
            if(a==Action::Right) rowCol[kind]=std::min(rowCol[kind]+1,std::max(0,count-1));
            if(kind==2) selection=std::clamp(rowCol[2],0,std::max(0,count-1));
            if(a==Action::Ok&&count) {
                if(kind==0) { resumeFirst=true; openDetails(games[continueRow()[std::clamp(rowCol[0],0,count-1)]].id); }
                if(kind==1) { selectTab(4+rowCol[1]); sidebar=false; }
                if(kind==2) { screen=Screen::Details; option=0; }
            }
        } else {
            if(tab>=4&&tab<=8&&a==Action::Back) { int console=tab-4; selectTab(0); sidebar=false; rowCol[1]=console; auto rows=homeRows(); for(size_t i=0;i<rows.size();++i) if(rows[i]==1) row=int(i); return; }
            const auto count=visible().size();
            if((minimal||performance)&&count) {
                if(a==Action::Left||a==Action::Back||(a==Action::Up&&selection==0))toHeader();
                else if(a==Action::Up)selection=std::max(0,selection-1);
                if(a==Action::Down||a==Action::Right)selection=std::min(selection+1,int(count-1));
                if(a==Action::Ok&&selected()){screen=Screen::Details;option=0;}
                return;
            }
            if(a==Action::Up||a==Action::Back) toHeader();
            if(a==Action::Left) selection=std::max(selection-1,0);
            if(a==Action::Right) selection=std::min(selection+1,count?int(count-1):1);
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
