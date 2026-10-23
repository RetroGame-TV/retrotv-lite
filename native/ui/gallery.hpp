#pragma once
#include <SDL.h>
#include "model.hpp"
#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <sstream>
namespace rt {
// Original TV shell. No theme code or artwork from other launchers is used.
class UI {
    SDL_Renderer* renderer; SDL_Texture* atlas;
    std::array<float,448> advances{};
    std::function<SDL_Texture*(const std::string&)> loadCover;
    struct Cover { SDL_Texture* texture=nullptr; uint32_t polled=0; };
    std::map<std::string,Cover> covers;
    std::set<std::string> used;
    float scroll=0;
    int previousTab=-1;
    uint32_t lastDraw=0;
    static uint32_t mix(uint32_t a,uint32_t b,float t) {
        auto channel=[&](int shift){return int(((a>>shift)&255)*(1-t)+((b>>shift)&255)*t);};
        return (channel(16)<<16)|(channel(8)<<8)|channel(0);
    }
public:
    Theme theme=Theme::classic();
    UI(SDL_Renderer* r,SDL_Texture* a,const std::string& metrics,std::function<SDL_Texture*(const std::string&)> loader):renderer(r),atlas(a),loadCover(std::move(loader)) {
        advances.fill(16);std::istringstream input(metrics);std::string value;size_t i=0;
        while(i<advances.size()&&std::getline(input,value,','))advances[i++]=std::max(1.f,std::strtof(value.c_str(),nullptr));
    }
    void setAtlas(SDL_Texture* texture){atlas=texture;}
    void clearCovers(){for(auto& pair:covers)SDL_DestroyTexture(pair.second.texture);covers.clear();}
    void color(uint32_t c){SDL_SetRenderDrawColor(renderer,(c>>16)&255,(c>>8)&255,c&255,255);}
    void rect(int x,int y,int w,int h,uint32_t c){if(w<=0||h<=0)return;color(c);SDL_Rect box{x,y,w,h};SDL_RenderFillRect(renderer,&box);}
    void line(int x,int y,int x2,int y2,uint32_t c,int weight=2){color(c);for(int i=0;i<weight;++i)SDL_RenderDrawLine(renderer,x,y+i,x2,y2+i);}
    void circle(int x,int y,int radius,uint32_t c){color(c);for(int dy=-radius;dy<=radius;++dy){int dx=int(std::sqrt(double(radius*radius-dy*dy)));SDL_RenderDrawLine(renderer,x-dx,y+dy,x+dx,y+dy);}}
    void round(int x,int y,int w,int h,int radius,uint32_t c){radius=std::min(radius,std::min(w,h)/2);rect(x+radius,y,w-2*radius,h,c);rect(x,y+radius,radius,h-2*radius,c);rect(x+w-radius,y+radius,radius,h-2*radius,c);circle(x+radius,y+radius,radius,c);circle(x+w-radius-1,y+radius,radius,c);circle(x+radius,y+h-radius-1,radius,c);circle(x+w-radius-1,y+h-radius-1,radius,c);}
    unsigned glyph(const std::string& s,size_t& i)const {
        unsigned ch=static_cast<unsigned char>(s[i++]);
        if((ch&0xe0)==0xc0&&i<s.size())ch=((ch&31)<<6)|(static_cast<unsigned char>(s[i++])&63);
        else if(ch>=0xe0){while(i<s.size()&&(static_cast<unsigned char>(s[i])&0xc0)==0x80)++i;ch='?';}
        return ch<32||ch>255?'?':ch;
    }
    int width(const std::string& s,int size,bool bold=false)const{float out=0;size_t i=0;while(i<s.size()){auto index=glyph(s,i)-32+(bold?224:0);out+=advances[index]*size/32.f;}return int(std::ceil(out));}
    void text(int x,int y,const std::string& s,int size=24,uint32_t c=0,bool bold=false,int limit=1200) {
        if(!atlas)return;if(!c)c=theme.text;SDL_SetTextureColorMod(atlas,(c>>16)&255,(c>>8)&255,c&255);
        size_t i=0;float pen=float(x);int origin=x;
        while(i<s.size()) {unsigned index=glyph(s,i)-32+(bold?224:0);float advance=advances[index]*size/32.f;
            if(pen+advance>origin+limit){if(limit>30)text(int(pen),y,"...",size,c,bold,40);break;}
            SDL_Rect src{int(index%16)*64+4,int(index/16)*64,60,64},dst{int(pen),y,int(size*60/32.),size*2};
            SDL_RenderCopy(renderer,atlas,&src,&dst);pen+=advance;
        }
    }
    void centered(int x,int y,int w,const std::string& s,int size,uint32_t c,bool bold=false){text(x+(w-width(s,size,bold))/2,y,s,size,c,bold,w);}
    void play(int x,int y,int size,uint32_t c){color(c);for(int i=0;i<size;++i){int half=int((size-i)*.6f);SDL_RenderDrawLine(renderer,x+i,y-half,x+i,y+half);}}
    void plus(int x,int y,uint32_t c){round(x-8,y-1,17,3,1,c);round(x-1,y-8,3,17,1,c);}
    void gear(int x,int y,uint32_t c){for(int i=0;i<8;++i){float a=i*3.14159265f/4;circle(x+int(std::cos(a)*10),y+int(std::sin(a)*10),3,c);}circle(x,y,10,c);circle(x,y,5,theme.background);}
    void device(int x,int y,int scale,uint32_t ink,uint32_t body,const std::string& system="NES") {
        // Geometric controller/device symbols drawn in the app; not console brand assets.
        if(system=="Game Boy") {
            round(x,y,scale*3/4,scale,scale/10,ink);round(x+4,y+4,scale*3/4-8,scale-8,scale/10,body);
            round(x+scale/8,y+scale/8,scale/2,scale*2/5,8,ink);round(x+scale/8+3,y+scale/8+3,scale/2-6,scale*2/5-6,5,body);
            plus(x+scale/5,y+scale*3/4,ink);circle(x+scale/2,y+scale*4/5,5,ink);circle(x+scale*5/8,y+scale*7/10,5,ink);return;
        }
        round(x,y,scale*3/2,scale*3/5,scale/5,ink);round(x+3,y+3,scale*3/2-6,scale*3/5-6,scale/5,body);
        int cy=y+scale*3/10;round(x+scale/4-5,cy-scale/9,11,scale*2/9,3,ink);round(x+scale/4-scale/9,cy-5,scale*2/9,11,3,ink);
        circle(x+scale*6/5,cy-9,scale/18,ink);circle(x+scale*21/20,cy+9,scale/18,ink);
        round(x+scale*3/5,cy+7,scale/8,4,2,ink);round(x+scale*4/5,cy+7,scale/8,4,2,ink);
        if(system=="SNES"||system=="PlayStation"){circle(x+scale*6/5,cy+13,scale/18,ink);circle(x+scale*4/3,cy+1,scale/18,ink);}
    }
    void footer(const std::string& status,bool library) {
        line(56,651,1224,651,theme.card,1);
        round(58,674,26,23,6,theme.text);centered(58,673,26,"OK",12,theme.background,true);text(94,673,library?"Selecionar":"Confirmar",15,theme.muted);
        text(242,673,"Voltar",15,theme.text,true);text(309,673,"Retornar",15,theme.muted);
        text(438,673,"Menu",15,theme.text,true);text(501,673,"Opções",15,theme.muted);
        text(864,675,status,12,theme.muted,false,356);
    }
    void header(Model& m) {
        text(56,32,"retro",30,theme.text,true);text(127,32,"tv",30,theme.accent,true);text(171,46,"LITE",10,theme.muted,true);
        text(1000,43,m.performance?"modo performance":"feito para jogar",13,theme.muted,false,174);
        circle(1210,53,18,theme.panel);play(1206,53,10,theme.accent);
        const int widths[]={83,116,112,116,76,85,133,100,137,55};int x=54;
        for(int i=0;i<10;++i) {
            bool selected=m.tab==i;
            if(i==4){line(x-5,111,x-5,131,theme.card,1);}
            if(selected)round(x,99,widths[i]-8,42,14,m.sidebar?theme.accent:theme.card);
            if(i==9)gear(x+22,119,selected&&m.sidebar?theme.background:theme.muted);
            else centered(x,105,widths[i]-8,tabs[i],16,selected?(m.sidebar?theme.background:theme.text):theme.muted,selected);
            x+=widths[i];
        }
    }
    void coverImage(const Game& game,int x,int y,int w,int h,bool backdrop=false) {
        if(game.cover.empty()||theme.minimal)return;
        used.insert(game.cover);
        auto it=covers.find(game.cover);
        if(it==covers.end()&&covers.size()<6)it=covers.emplace(game.cover,Cover{}).first;
        if(it==covers.end())return;
        auto& cover=it->second;auto now=SDL_GetTicks();
        if(!cover.texture&&now-cover.polled>180){cover.polled=now;cover.texture=loadCover(game.cover);}
        if(cover.texture){SDL_Rect dst{x,y,w,h},crop{0,58,192,140};SDL_SetTextureBlendMode(cover.texture,SDL_BLENDMODE_BLEND);SDL_SetTextureAlphaMod(cover.texture,backdrop?65:255);SDL_RenderCopy(renderer,cover.texture,backdrop?&crop:nullptr,&dst);SDL_SetTextureAlphaMod(cover.texture,255);}
    }
    void poster(const Game& game,int x,int y,int w,int h) {
        uint32_t hue=game.system=="NES"?0x814C5A:game.system=="SNES"?0x66568B:game.system=="Mega Drive"?0x365D79:game.system=="Arcade"?0x786042:0x4A7069;
        if(theme.minimal)hue=0x35343B;
        round(x,y,w,h,9,hue);
        for(int dy=0;dy<h;dy+=3)rect(x,y+dy,w,std::min(3,h-dy),mix(hue,0x211F29,float(dy)/h*.7f));
        text(x+14,y+12,game.system,11,0xE1DDE9,true,w-25);
        device(x+w/6,y+h/3,w*4/9,0xD8D1E5,mix(hue,0x211F29,.3f),game.system);
        text(x+14,y+h-68,"RETROTV",10,0xC7BDCF,true,w-26);
        text(x+14,y+h-44,game.title,17,0xFFFFFF,true,w-28);
        coverImage(game,x,y,w,h);
        for(int dy=0;dy<9;++dy){int cut=9-int(std::sqrt(float(81-(9-dy)*(9-dy))));rect(x,y+dy,cut,1,theme.background);rect(x+w-cut,y+dy,cut,1,theme.background);rect(x,y+h-1-dy,cut,1,theme.background);rect(x+w-cut,y+h-1-dy,cut,1,theme.background);}
    }
    void chip(int x,int y,int w,const std::string& title,bool focus,bool primary=false) {
        round(x-3,y-3,w+6,56,19,focus?theme.text:theme.background);
        round(x,y,w,50,16,focus||primary?theme.accent:theme.card);
        centered(x,y+9,w,title,17,focus||primary?theme.background:theme.text,true);
    }
    void empty(Model& m) {
        m.selection=std::clamp(m.selection,0,1);
        bool all=m.games.empty();
        text(70,202,all?"APERTE START PARA COMEÇAR":"SUA COLEÇÃO",12,theme.accent,true);
        text(68,236,all?"Seus clássicos.":"Mais uma partida?",46,theme.text,true,620);
        text(68,291,all?"Um novo lugar para jogar.":"Encontre seu próximo favorito.",33,theme.text,true,640);
        text(71,363,all?"Conecte seu acervo. Escolha um jogo. Aproveite.":"Esta coleção ainda não tem jogos.",18,theme.muted);
        chip(72,437,224,"+  Adicionar jogos",!m.sidebar&&m.selection==0,true);
        chip(314,437,208,"Testar controle",!m.sidebar&&m.selection==1);
        text(72,511,"Tudo fica na sua TV. Sem conta, sem complicação.",14,theme.muted);
        if(!theme.minimal) {
            circle(964,366,162,0x211D2B);circle(964,366,132,0x292334);
            round(821,236,234,167,22,0x42354F);round(817,229,234,167,22,0xC0A3F2);
            round(832,244,204,134,12,0x24212B);round(848,259,172,93,7,0x342B43);
            int px=880,py=288;rect(px,py,15,15,theme.accent);rect(px+48,py,15,15,theme.accent);rect(px+16,py+17,47,14,theme.accent);rect(px+7,py+31,15,13,theme.accent);rect(px+57,py+31,15,13,theme.accent);
            round(904,406,68,8,4,0x766084);round(884,420,108,8,4,0x42354F);
            device(846,454,170,0xD0BFED,0x211D2B,"SNES");
            circle(1135,273,5,theme.accent);plus(784,401,0x766084);circle(1091,489,3,0x766084);
        }else device(824,300,232,theme.text,theme.background,m.tab>=4&&m.tab<=8?tabs[m.tab]:"SNES");
        text(72,586,"NES",12,theme.muted,true);text(140,586,"SNES",12,theme.muted,true);text(218,586,"MEGA DRIVE",12,theme.muted,true);text(355,586,"ARCADE",12,theme.muted,true);text(461,586,"PLAYSTATION",12,theme.muted,true);
    }
    void library(Model& m,float dt) {
        auto list=m.visible();auto* selected=m.selected();if(!selected){empty(m);return;}
        if(theme.minimal){minimalLibrary(m,list,*selected);return;}
        // Reuse the selected cover as a quiet backdrop; no second image decode/cache.
        coverImage(*selected,650,151,630,490,true);
        SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
        for(int x=650;x<1280;x+=4){int alpha=int(255*(1-std::sin((x-650)/630.f*3.14159265f)));SDL_SetRenderDrawColor(renderer,22,21,25,Uint8(alpha));SDL_Rect wash{x,151,4,490};SDL_RenderFillRect(renderer,&wash);}
        for(int y=151;y<641;y+=4){int alpha=int(255*(1-std::sin((y-151)/490.f*3.14159265f)));SDL_SetRenderDrawColor(renderer,22,21,25,Uint8(alpha));SDL_Rect wash{650,y,630,4};SDL_RenderFillRect(renderer,&wash);}
        SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);
        text(60,181,selected->system,14,theme.accent,true);
        text(59,211,selected->title,39,theme.text,true,1050);
        std::string meta=selected->available?"Pronto para jogar":"USB desconectado";
        if(selected->favorite)meta+="   ·   Favorito";
        if(selected->played)meta+="   ·   "+std::to_string(selected->played/60)+" min jogados";
        text(62,274,meta,15,theme.muted);
        text(1106,183,std::to_string(m.selection+1)+" / "+std::to_string(list.size()),14,theme.muted,false,120);
        int start=std::clamp(m.selection-2,0,std::max(0,int(list.size())-6));
        if(previousTab!=m.tab){scroll=float(start);previousTab=m.tab;}
        scroll+=(start-scroll)*std::min(1.f,dt*15.f);
        // The six-card window keeps texture storage bounded independently of library size.
        std::set<std::string> keep;for(int i=start;i<std::min(start+6,int(list.size()));++i)if(!m.games[list[i]].cover.empty())keep.insert(m.games[list[i]].cover);
        for(auto it=covers.begin();it!=covers.end();)if(!keep.count(it->first)){SDL_DestroyTexture(it->second.texture);it=covers.erase(it);}else++it;
        SDL_Rect clip{42,324,1196,310};SDL_RenderSetClipRect(renderer,&clip);
        for(int i=start;i<std::min(start+6,int(list.size()));++i) {
            auto& game=m.games[list[i]];bool active=i==m.selection;int x=60+int((i-scroll)*196),y=350,w=174,h=232;
            if(active){x-=4;y-=8;w+=8;h+=11;round(x-5,y-5,w+10,h+10,14,m.sidebar?0x65586F:theme.accent);round(x-2,y-2,w+4,h+4,11,theme.background);}
            poster(game,x,y,w,h);
            if(active&&!m.sidebar){round(x+w-42,y+12,29,29,14,theme.accent);play(x+w-32,y+26,10,theme.background);}
            text(x,y+h+15,game.title,14,active?theme.text:theme.muted,active,w);
        }
        SDL_RenderSetClipRect(renderer,nullptr);
    }
    void minimalLibrary(Model& m,const std::vector<size_t>& list,const Game& selected) {
        text(62,181,"EXPLORAR",13,theme.muted,true);text(800,180,selected.system,17,theme.text,true);
        int start=std::clamp(m.selection-2,0,std::max(0,int(list.size())-6));
        for(int i=start;i<std::min(start+6,int(list.size()));++i){int y=228+(i-start)*62;bool focused=i==m.selection;
            if(focused)round(58,y-1,605,47,23,m.sidebar?theme.card:theme.text);
            text(78,y+5,m.games[list[i]].title,25,focused&&!m.sidebar?theme.background:theme.text,true,553);
        }
        round(819,232,282,168,20,theme.text);round(823,236,274,160,17,theme.background);
        round(846,259,227,25,10,theme.text);round(850,263,219,17,7,theme.background);
        for(int i=0;i<3;++i)round(847,309+i*12,58,4,2,theme.muted);
        round(929,320,64,37,8,theme.text);round(933,324,56,29,5,theme.background);round(1011,320,52,37,8,theme.text);round(1015,324,44,29,5,theme.background);
        line(958,401,958,428,theme.muted,3);device(811,441,201,theme.text,theme.background,selected.system);
        centered(788,586,345,std::to_string(m.selection+1)+" de "+std::to_string(list.size())+" jogos",15,theme.muted);
    }
    void button(int y,const std::string& title,bool focus,const std::string& note="") {
        round(463,y,720,57,13,focus?theme.accent:theme.panel);
        text(484,y+10,title,19,focus?theme.background:theme.text,focus,654);
        if(!note.empty())text(1045,y+13,note,14,focus?theme.background:theme.muted,false,105);
        if(focus)text(1150,y+10,">",19,theme.background,true);
    }
    void menuTitle(const std::string& eyebrow,const std::string& title,const std::string& description) {
        text(61,141,eyebrow,12,theme.accent,true,350);text(59,180,title,36,theme.text,true,365);
        text(62,247,description,16,theme.muted,false,345);device(74,417,182,theme.minimal?theme.muted:0x635773,theme.background,"SNES");
        line(413,145,413,612,theme.card,1);
    }
    void draw(Model& m,const std::string& status) {
        auto now=SDL_GetTicks();float dt=lastDraw?std::min(.1f,(now-lastDraw)/1000.f):.016f;lastDraw=now;
        theme=(m.minimal||m.performance)?Theme::minimalTheme():Theme::classic();used.clear();color(theme.background);SDL_RenderClear(renderer);
        if(m.screen==Screen::Library) {header(m);library(m,dt);}
        else {
            text(56,32,"retrotv",28,theme.text,true);text(174,44,"LITE",10,theme.accent,true);
            if(m.screen==Screen::Details) {
                auto* g=m.selected();if(g){poster(*g,73,153,258,344);text(73,531,g->system,15,theme.accent,true);text(73,563,std::to_string(g->played/60)+" min jogados",14,theme.muted);
                    text(463,144,g->system,13,theme.accent,true);text(460,178,g->title,34,theme.text,true,726);
                    const char* labels[]={"Jogar agora","Continuar partida","Adicionar aos favoritos","Voltar à biblioteca"};
                    for(int i=0;i<4;++i)button(285+i*75,i==2&&g->favorite?"Remover dos favoritos":labels[i],m.option==i);
                }
            } else if(m.screen==Screen::Settings) {
                menuTitle("AJUSTES","Do seu jeito.","Sua biblioteca, suas preferências.");
                std::vector<std::string> items={"Adicionar pasta de jogos","Atualizar biblioteca",std::string("Visual: ")+(m.minimal?"Minimal":"Galeria"),std::string("Modo performance: ")+(m.performance?"ligado":"desligado"),"Exportar saves","Restaurar saves","Controles e botões","Diagnóstico","Sobre o RetroTV","Testar controle · demo original","Importar BIOS do PlayStation"};
                int first=std::clamp(m.option-3,0,5);for(int i=first;i<int(items.size())&&i<first+6;++i)button(154+(i-first)*74,items[i],m.option==i);
                text(465,613,std::to_string(m.option+1)+" de "+std::to_string(items.size()),12,theme.muted);
            } else if(m.screen==Screen::QuickMenu||m.screen==Screen::Controls) {
                bool controls=m.screen==Screen::Controls;menuTitle(controls?"GAMEPAD":"PAUSA",controls?"Cada botão conta.":"No seu tempo.",controls?"OK para remapear o controle.":"Sua partida espera por você.");
                const char* quick[]={"Continuar partida","Salvar estado","Carregar estado","Reiniciar jogo","Controles","Proporção: 4:3 / tela cheia","Trocar disco","Salvar e voltar à biblioteca","Diagnóstico da partida"};
                const char* keys[]={"B","Y","SELECT","START","Cima","Baixo","Esquerda","Direita","A","X","L1","R1","L2","R2","L3","R3"};
                int total=controls?16:9,first=std::clamp(m.option-3,0,total-6);
                for(int i=first;i<total&&i<first+6;++i)button(154+(i-first)*74,controls?std::string("Botão ")+keys[i]:quick[i],m.option==i);
                if(controls)text(465,613,m.message.substr(0,m.message.find('\n')),12,theme.muted,false,713);
            } else {
                std::string title=m.screen==Screen::Error?"Vamos resolver.":m.screen==Screen::Diagnostics?"Por dentro.":"Tudo certo?";
                menuTitle(m.screen==Screen::Error?"ATENÇÃO":m.screen==Screen::Diagnostics?"DIAGNÓSTICO":"CONFIRMAÇÃO",title,"Voltar sempre retorna ao menu.");
                round(460,150,726,454,19,theme.panel);size_t start=0;int y=174;
                while(start<m.message.size()&&y<557){auto end=m.message.find('\n',start);std::string row=m.message.substr(start,end-start);while(width(row,17)>665&&y<557){size_t cut=row.size();while(cut>0&&width(row.substr(0,cut),17)>665)--cut;while(cut>0&&(static_cast<unsigned char>(row[cut])&0xc0)==0x80)--cut;if(!cut)break;text(483,y,row.substr(0,cut),17,theme.muted);row=row.substr(cut);y+=31;}text(483,y,row,17,theme.muted,false,665);if(end==std::string::npos)break;start=end+1;y+=39;}
            }
        }
        footer(status,m.screen==Screen::Library);
        if(m.minimal||m.performance)clearCovers();
    }
};
}
