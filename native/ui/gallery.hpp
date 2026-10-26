#pragma once
#include <SDL.h>
#include "model.hpp"
#include "playback_profile.hpp"
#include "iso.hpp"
#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <sstream>
namespace rt {
class UI {
    SDL_Renderer* renderer; SDL_Texture* atlas; SDL_Texture* shapes=nullptr;
    std::array<float,448> advances{};
    ConsoleArt art;
    std::function<SDL_Texture*(const std::string&)> loadCover;
    struct Cover { SDL_Texture* texture=nullptr; uint32_t polled=0; };
    std::map<std::string,Cover> covers;
    float scroll=0,focusLift=0,homeScroll=0;std::array<float,3> rowScroll{};size_t coverCap=6;std::string statusLine;int previousTab=-1,previousSelection=-1;uint32_t lastDraw=0;
    static uint32_t mix(uint32_t a,uint32_t b,float t){auto ch=[&](int n){return int(((a>>n)&255)*(1-t)+((b>>n)&255)*t);};return(ch(16)<<16)|(ch(8)<<8)|ch(0);}
    void makeShapes(){
        if(shapes)SDL_DestroyTexture(shapes);shapes=nullptr;
        auto* surface=SDL_CreateRGBSurfaceWithFormat(0,68,136,32,SDL_PIXELFORMAT_RGBA32);if(!surface)return;
        SDL_FillRect(surface,nullptr,SDL_MapRGBA(surface->format,255,255,255,0));SDL_Rect inversePad{0,68,68,68};SDL_FillRect(surface,&inversePad,SDL_MapRGBA(surface->format,255,255,255,255));
        auto* pixels=static_cast<uint8_t*>(surface->pixels);
        for(int y=0;y<64;++y)for(int x=0;x<64;++x){float d=std::sqrt((x-31.5f)*(x-31.5f)+(y-31.5f)*(y-31.5f));auto alpha=Uint8(255*std::clamp(32.f-d,0.f,1.f));
            for(int part=0;part<2;++part){auto* p=pixels+(y+2+part*68)*surface->pitch+(x+2)*4;p[0]=p[1]=p[2]=255;p[3]=part?255-alpha:alpha;}}
        shapes=SDL_CreateTextureFromSurface(renderer,surface);SDL_FreeSurface(surface);if(shapes)SDL_SetTextureBlendMode(shapes,SDL_BLENDMODE_BLEND);
    }
    unsigned glyph(const std::string& s,size_t& i)const{unsigned ch=static_cast<unsigned char>(s[i++]);if((ch&0xe0)==0xc0&&i<s.size())ch=((ch&31)<<6)|(static_cast<unsigned char>(s[i++])&63);else if(ch>=0xe0){while(i<s.size()&&(static_cast<unsigned char>(s[i])&0xc0)==0x80)++i;ch='?';}return ch<32||ch>255?'?':ch;}
    void corner(int x,int y,int radius,int quadrant,uint32_t c,bool inverse=false){if(!shapes)return;SDL_SetTextureColorMod(shapes,(c>>16)&255,(c>>8)&255,c&255);SDL_Rect src{2+(quadrant%2)*32,2+(quadrant/2)*32+(inverse?68:0),32,32},dst{x,y,radius,radius};SDL_RenderCopy(renderer,shapes,&src,&dst);}
    SDL_Texture* artwork(const Game& game){
        if(game.cover.empty()||theme.minimal)return nullptr;
        auto it=covers.find(game.cover);if(it==covers.end()&&covers.size()<coverCap)it=covers.emplace(game.cover,Cover{}).first;if(it==covers.end())return nullptr;
        auto& c=it->second;auto now=SDL_GetTicks();if(!c.texture&&now-c.polled>150){c.polled=now;c.texture=loadCover(game.cover);}return c.texture;
    }
    void mask(int x,int y,int w,int h,int r,uint32_t c=0){if(!c)c=theme.background;corner(x,y,r,0,c,true);corner(x+w-r,y,r,1,c,true);corner(x,y+h-r,r,2,c,true);corner(x+w-r,y+h-r,r,3,c,true);}
    void wordmark(){text(60,32,"retro",32,theme.text,true);int x=60+width("retro",32,true);text(x,32,"tv",32,theme.accent,true);int badge=x+width("tv",32,true)+15;round(badge,46,36,18,5,theme.card);centered(badge,46,36,"LITE",10,theme.muted,true);}
    void chevron(int x,int y,uint32_t c){line(x,y-5,x+5,y,c,2);line(x+5,y,x,y+5,c,2);}
    void keycap(int x,int y,int w,const std::string& label){round(x,y,w,24,7,theme.card);centered(x,y+2,w,label,12,theme.text,true);}
    void backdrop(const Game& game){
        auto* texture=artwork(game);if(!texture)return;
        SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);SDL_SetTextureAlphaMod(texture,73);SDL_Rect src{0,80,240,160},dst{650,150,630,420};SDL_RenderCopy(renderer,texture,&src,&dst);SDL_SetTextureAlphaMod(texture,255);
        SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
        for(int x=650;x<1280;x+=6){int a=int(255*(1-std::sin((x-650)/630.f*3.14159265f)));SDL_SetRenderDrawColor(renderer,22,21,25,Uint8(a));SDL_Rect wash{x,150,6,420};SDL_RenderFillRect(renderer,&wash);}
        for(int y=150;y<570;y+=6){int a=int(255*(1-std::sin((y-150)/420.f*3.14159265f)));SDL_SetRenderDrawColor(renderer,22,21,25,Uint8(a));SDL_Rect wash{650,y,630,6};SDL_RenderFillRect(renderer,&wash);}
        SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);
    }
    std::string systemName(const std::string& s)const{if(s=="NES")return "Nintendo Entertainment System";if(s=="SNES")return "Super Nintendo";if(s=="Mega Drive")return "Sega Mega Drive";if(s=="PlayStation")return "Sony PlayStation";if(s=="Arcade")return "Arcade · CPS1 / CPS2";return s;}
    void eyebrow(int x,int y,const std::string& label){text(x,y,label,13,theme.accent,true);}
    void actionButton(int x,int y,int w,const std::string& title,bool focus,bool primary,bool enabled=true,bool add=false){
        if(focus)round(x-3,y-3,w+6,64,18,theme.text);
        auto bg=primary?theme.accent:focus?theme.text:theme.card;round(x,y,w,58,15,bg);
        auto ink=enabled?(primary||focus?theme.background:theme.text):theme.muted;
        if(primary){if(add)plus(x+27,y+29,ink);else play(x+23,y+29,10,ink);text(x+45,y+13,title,21,ink,true,w-60);}else centered(x,y+13,w,title,20,ink,true);
    }
    void row(int y,const std::string& title,const std::string& value,bool focus,int x=451,int w=731){
        round(x,y,w,59,13,focus?theme.text:theme.panel);
        if(focus)round(x+12,y+18,3,23,1,theme.accent);
        text(x+28,y+14,title,20,focus?theme.background:theme.text,focus,w-70-(value.empty()?0:210));
        if(!value.empty()){int vw=std::min(179,width(value,16,true));text(x+w-48-vw,y+17,value,16,focus?0x65547E:theme.muted,true,179);} // right-aligned next to the chevron
        chevron(x+w-28,y+29,focus?theme.background:theme.muted);
    }
    void menuTitle(const std::string& tag,const std::string& title,const std::string& description){
        eyebrow(62,149,tag);titleLines(60,185,title,38,345,2);titleLines(63,285,description,18,322,3,26);consoleArt(56,420,330,205,"SNES");line(410,152,410,623,theme.card,1);
    }
public:
    Theme theme=Theme::classic();std::function<void()> pausedBackdrop;
    UI(SDL_Renderer* r,SDL_Texture* a,const std::string& metrics,std::function<SDL_Texture*(const std::string&)> loader):renderer(r),atlas(a),art(r),loadCover(std::move(loader)){
        advances.fill(20);std::istringstream input(metrics);std::string value;size_t i=0;while(i<advances.size()&&std::getline(input,value,','))advances[i++]=std::max(1.f,std::strtof(value.c_str(),nullptr));makeShapes();
    }
    void setAtlas(SDL_Texture* texture){atlas=texture;art.clear();makeShapes();}
    void clearCovers(){for(auto& c:covers)SDL_DestroyTexture(c.second.texture);covers.clear();}
    void clearArt(){art.clear();}
    void release(){clearCovers();art.clear();if(shapes)SDL_DestroyTexture(shapes);shapes=nullptr;}
    void color(uint32_t c){SDL_SetRenderDrawColor(renderer,(c>>16)&255,(c>>8)&255,c&255,255);}
    void rect(int x,int y,int w,int h,uint32_t c){if(w<=0||h<=0)return;color(c);SDL_Rect box{x,y,w,h};SDL_RenderFillRect(renderer,&box);}
    void line(int x,int y,int x2,int y2,uint32_t c,int weight=1){color(c);for(int i=0;i<weight;++i)SDL_RenderDrawLine(renderer,x,y+i,x2,y2+i);}
    void circle(int x,int y,int radius,uint32_t c){if(!shapes)return;SDL_SetTextureColorMod(shapes,(c>>16)&255,(c>>8)&255,c&255);SDL_Rect src{2,2,64,64},dst{x-radius,y-radius,radius*2+1,radius*2+1};SDL_RenderCopy(renderer,shapes,&src,&dst);}
    void round(int x,int y,int w,int h,int r,uint32_t c){r=std::min(r,std::min(w,h)/2);rect(x+r,y,w-r*2,h,c);rect(x,y+r,r,h-r*2,c);rect(x+w-r,y+r,r,h-r*2,c);corner(x,y,r,0,c);corner(x+w-r,y,r,1,c);corner(x,y+h-r,r,2,c);corner(x+w-r,y+h-r,r,3,c);}
    int width(const std::string& s,int size,bool bold=false)const{float w=0;size_t i=0;while(i<s.size())w+=advances[glyph(s,i)-32+(bold?224:0)]*size/40.f;return int(std::ceil(w));}
    void text(int x,int y,const std::string& s,int size=24,uint32_t c=0,bool bold=false,int limit=1200){
        if(!atlas||limit<=0)return;if(!c)c=theme.text;SDL_SetTextureColorMod(atlas,(c>>16)&255,(c>>8)&255,c&255);
        bool shortened=width(s,size,bold)>limit;int contentLimit=shortened?std::max(0,limit-width("...",size,bold)):limit;float pen=float(x);size_t i=0;
        auto render=[&](unsigned index){SDL_Rect src{int(index%16)*64+4,int(index/16)*64,60,64},dst{int(std::round(pen)),y,int(std::round(size*60/40.)),int(std::round(size*64/40.))};SDL_RenderCopy(renderer,atlas,&src,&dst);pen+=advances[index]*size/40.f;};
        while(i<s.size()){unsigned index=glyph(s,i)-32+(bold?224:0);if(pen+advances[index]*size/40.f>x+contentLimit)break;render(index);}
        if(shortened&&limit>=width("...",size,bold))for(int k=0;k<3;++k)render('.'-32+(bold?224:0));
    }
    void centered(int x,int y,int w,const std::string& s,int size,uint32_t c,bool bold=false){int actual=std::min(w,width(s,size,bold));text(x+(w-actual)/2,y,s,size,c,bold,w);}
    void titleLines(int x,int y,const std::string& title,int size,int limit,int lines,int leading=0){
        if(!leading)leading=size+9;std::istringstream words(title);std::string word,lineText;int lineNo=0;bool bold=size>=26;
        while(words>>word){std::string next=lineText.empty()?word:lineText+" "+word;if(width(next,size,bold)>limit&&!lineText.empty()&&lineNo<lines-1){text(x,y+lineNo*leading,lineText,size,0,bold,limit);++lineNo;lineText=word;}else lineText=next;}
        text(x,y+lineNo*leading,lineText,size,0,size>=26,limit);
    }
    void play(int x,int y,int size,uint32_t c){color(c);for(int i=0;i<size;++i){int half=int((size-i)*.6f);SDL_RenderDrawLine(renderer,x+i,y-half,x+i,y+half);}}
    void plus(int x,int y,uint32_t c){round(x-8,y-1,17,3,1,c);round(x-1,y-8,3,17,1,c);}
    void gear(int x,int y,uint32_t c,uint32_t hole){for(int i=0;i<8;++i){float a=i*3.14159265f/4;circle(x+int(std::cos(a)*10),y+int(std::sin(a)*10),3,c);}circle(x,y,10,c);circle(x,y,5,hole);}
    void magnifier(int x,int y,uint32_t c,uint32_t hole){circle(x,y,9,c);circle(x,y,6,hole);for(int i=0;i<6;++i)rect(x+5+i,y+5+i,3,3,c);}
    void focusRing(int x,int y,int w,int h,int r){round(x-6,y-6,w+12,h+12,r+6,theme.accent);round(x-3,y-3,w+6,h+6,r+3,theme.background);}
    void consoleArt(int x,int y,int w,int h,const std::string& system){
        uint32_t accent=theme.minimal?theme.accent:system=="NES"||system=="Mega Drive"?0xE0505F:system=="PlayStation"?0x5FCF96:system=="Arcade"?0x72C6EE:theme.accent;
        art.draw(system,x,y,w,h,theme.minimal,accent);
    }
    /** Pairing code for RetroTV Pad over Wi-Fi, shown above anything, including a running game. */
    void phoneCode(const std::string& code){
        // Top right, under the header: reads like a notification and stays clear of the console art and the actions.
        SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);round(912,100,316,108,24,theme.panel);round(912,100,6,108,3,theme.accent);
        text(938,116,"Código do RetroTV Pad",15,theme.muted,true,270);text(936,140,code,48,theme.text,true,270);
    }
    void footer(Model& m,const std::string& status){
        line(60,665,1220,665,theme.card);keycap(60,685,31,"OK");text(101,684,m.screen==Screen::Playback?"Alterar":m.screen==Screen::Library?"Selecionar":"Confirmar",15,theme.muted);
        keycap(261,685,54,"Voltar");text(326,684,"Retornar",15,theme.muted);keycap(467,685,49,"Menu");text(527,684,"Opções",15,theme.muted);
        text(854,686,status,13,theme.muted,false,365);
    }
    void header(Model& m){
        wordmark();bool library=m.screen==Screen::Library;int x=250;
        for(int code:m.headerItems()){if(code==SearchItem||code==9)continue;std::string label=code==0?"Início":code==3?"Favoritos":tabs[code];
            bool current=library&&m.tab==code,focus=library&&m.sidebar&&m.headerFocus==code;int w=width(label,18,current||focus)+40;
            if(focus)round(x,34,w,44,22,theme.text);else if(current)round(x,34,w,44,22,theme.card);
            centered(x,45,w,label,18,focus?theme.background:current?theme.text:theme.muted,current||focus);x+=w+6;}
        bool sf=(library&&m.sidebar&&m.headerFocus==SearchItem)||m.screen==Screen::Search;const int sx=938,sw=200;uint32_t sInk=sf?theme.background:theme.muted;
        round(sx,34,sw,44,22,sf?theme.text:theme.panel);magnifier(sx+28,55,sInk,sf?theme.text:theme.panel);text(sx+50,45,m.query.empty()||m.screen==Screen::Search?"Buscar":m.query,18,sInk,sf,sw-66);
        bool gf=library&&m.sidebar&&m.headerFocus==9;circle(1182,56,22,gf?theme.text:theme.panel);gear(1182,56,gf?theme.background:theme.muted,gf?theme.text:theme.panel);
    }
    void poster(const Game& game,int x,int y,int w,int h,uint32_t under=0){
        if(auto* texture=artwork(game)){SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);SDL_Rect dst{x,y,w,h};SDL_RenderCopy(renderer,texture,nullptr,&dst);return;}
        uint32_t hue=game.system=="NES"?0x684553:game.system=="SNES"?0x574A74:game.system=="Mega Drive"?0x34536A:game.system=="Arcade"?0x6D573C:0x45615D;if(theme.minimal)hue=0x333139;
        // Bottom half already in the gradient's end colour, so the rounded base has no lighter stripe under the title.
        round(x,y,w,h,13,hue);round(x,y+h/2,w,h-h/2,13,mix(hue,0x211E28,.65f*float(h-14)/h));SDL_Rect previousClip{};bool clipped=SDL_RenderIsClipEnabled(renderer);SDL_RenderGetClipRect(renderer,&previousClip);SDL_Rect clip{x,y+14,w,h-28};if(clipped){SDL_Rect intersection{};SDL_IntersectRect(&clip,&previousClip,&intersection);clip=intersection;}SDL_RenderSetClipRect(renderer,&clip);
        for(int dy=14;dy<h-14;dy+=4)rect(x,y+dy,w,std::min(4,h-14-dy),mix(hue,0x211E28,float(dy)/h*.65f));
        SDL_RenderSetClipRect(renderer,clipped?&previousClip:nullptr);
        if(h>=150){text(x+14,y+13,game.system,13,0xE1DDE9,true,w-28);consoleArt(x+6,y+38,w-12,h-56,game.system);} // the title is always shown beside or below the poster
        else consoleArt(x+4,y+4,w-8,h-8,game.system);
        mask(x,y,w,h,13,under);
    }
    void empty(Model& m){
        m.selection=std::clamp(m.selection,0,1);bool all=m.games.empty();
        text(64,247,all?"Seus clássicos.":"Ainda sem jogos.",48,theme.text,true,660);text(65,307,all?"Prontos para outra partida.":"Há muito para descobrir.",32,theme.text,true,655);
        text(67,377,all?"Adicione seu acervo e escolha o próximo jogo.":"Adicione jogos ou explore outra coleção.",19,theme.muted,false,630);
        actionButton(68,446,232,all?"Adicionar jogos":"Ver biblioteca",!m.sidebar&&m.selection==0,true,true,all);actionButton(318,446,214,all?"Testar controle":"Adicionar jogos",!m.sidebar&&m.selection==1,false);
        if(!theme.minimal&&!(m.tab>=4&&m.tab<=8)){circle(975,361,151,0x211D2A);circle(975,361,121,0x292332);round(842,242,229,159,23,0x63516E);round(834,232,229,159,23,0xB9A1E2);round(848,246,201,129,13,0x24212B);round(864,262,169,91,8,0x342B43);
            int x=898,y=290;rect(x,y,14,14,theme.accent);rect(x+43,y,14,14,theme.accent);rect(x+14,y+16,43,12,theme.accent);rect(x+6,y+29,13,12,theme.accent);rect(x+49,y+29,13,12,theme.accent);
            round(917,402,62,7,3,0x7E6B8C);round(893,418,110,6,3,0x45374F);consoleArt(826,428,290,176,"SNES");circle(1129,268,4,theme.accent);plus(803,396,0x6E5C7B);
        }else consoleArt(760,214,430,370,m.tab>=4&&m.tab<=8?tabs[m.tab]:"SNES");
        text(68,601,statusLine,14,theme.muted,false,720);
    }
    void library(Model& m,float dt){
        auto list=m.visible();auto* game=m.selected();if(!game){empty(m);return;}if(theme.minimal){minimalLibrary(m,list,*game);return;}
        if(m.home()){home(m,dt);return;}coverCap=6;
        int target=std::clamp(m.selection-2,0,std::max(0,int(list.size())-5));
        if(previousTab!=m.tab||std::abs(target-scroll)>2.5f){scroll=float(target);previousTab=m.tab;}
        scroll+=(target-scroll)*std::min(1.f,dt*18.f);if(std::abs(target-scroll)<.002f)scroll=float(target);
        if(previousSelection!=m.selection){focusLift=0;previousSelection=m.selection;}
        focusLift+=((m.sidebar?0.f:1.f)-focusLift)*std::min(1.f,dt*20.f);
        int first=int(std::floor(scroll));std::set<std::string> keep;
        for(int i=first;i<std::min(first+6,int(list.size()));++i)if(!m.games[list[i]].cover.empty())keep.insert(m.games[list[i]].cover);
        for(auto it=covers.begin();it!=covers.end();)if(!keep.count(it->first)){SDL_DestroyTexture(it->second.texture);it=covers.erase(it);}else++it;
        backdrop(*game);text(62,170,game->title,47,theme.text,true,1090);
        std::string meta=game->available?(game->played?std::to_string(game->played/60)+" min jogados":"Ainda não jogado"):"USB desconectado";if(game->favorite)meta+="   ·   Favorito";
        text(66,233,systemName(game->system)+"   ·   "+meta,17,game->available?theme.muted:0xE6BC8B,false,1090);
        SDL_Rect clip{49,330,1182,326};SDL_RenderSetClipRect(renderer,&clip);
        for(int i=first;i<std::min(first+6,int(list.size()));++i){auto& item=m.games[list[i]];bool current=i==m.selection;float lift=current?focusLift:0;int w=195+int(10*lift),h=260+int(13*lift),x=65+int(std::round((i-scroll)*231))-(w-195)/2,y=613-h;
            if(current){round(x-8,y-8,w+16,h+16,20,m.sidebar?0x28242F:0x302739);round(x-4,y-4,w+8,h+8,16,m.sidebar?0x6B5B7C:theme.accent);round(x-2,y-2,w+4,h+4,14,theme.background);}
            poster(item,x,y,w,h);
            if(current&&!m.sidebar){circle(x+w-25,y+25,15,theme.accent);play(x+w-29,y+25,10,theme.background);}
            text(x,629,item.title,current?18:16,current?theme.text:theme.muted,current,w);
        }
        SDL_RenderSetClipRect(renderer,nullptr);
    }
    void home(Model& m,float dt){
        coverCap=12;auto rows=m.homeRows();int focusRow=std::clamp(m.row,0,int(rows.size())-1);
        const int heights[]={190,232,282};int tops[3]{},y=0;for(size_t i=0;i<rows.size();++i){tops[i]=y;y+=heights[rows[i]];}
        float target=float(std::clamp(tops[focusRow]+heights[rows[focusRow]]-598,0,tops[focusRow]));homeScroll+=(target-homeScroll)*std::min(1.f,dt*14.f);if(std::abs(target-homeScroll)<.5f)homeScroll=target;
        auto recent=m.continueRow();auto list=m.visible();std::set<std::string> keep;
        SDL_Rect clip{0,98,1280,622};SDL_RenderSetClipRect(renderer,&clip);
        for(size_t r=0;r<rows.size();++r){int kind=rows[r],top=118+tops[r]-int(homeScroll);if(top>720||top+heights[kind]<98)continue;
            bool rowFocus=!m.sidebar&&int(r)==focusRow;int col=m.rowCol[kind],y0=top+36;
            text(64,top,kind==0?"Continuar":kind==1?"Consoles":"Biblioteca",17,rowFocus?theme.text:theme.muted,true);
            if(kind==0){const int w=360,h=132,step=384;int first=std::clamp(col-1,0,std::max(0,int(recent.size())-3));
                for(int i=first;i<std::min(first+4,int(recent.size()));++i){auto& g=m.games[recent[i]];int x=64+(i-first)*step;bool f=rowFocus&&i==col;if(!g.cover.empty())keep.insert(g.cover);
                    if(f)focusRing(x,y0,w,h,18);uint32_t card=f?theme.card:theme.panel;round(x,y0,w,h,18,card);poster(g,x+14,y0+14,78,104,card);
                    text(x+110,y0+30,g.title,21,theme.text,true,w-128);text(x+110,y0+64,g.system,15,theme.muted,false,w-128);
                    if(f){circle(x+w-36,y0+h-36,18,theme.accent);play(x+w-41,y0+h-36,11,theme.background);}}}
            if(kind==1){const int w=212,h=176,step=230;
                for(int i=0;i<5;++i){int x=64+i*step;bool f=rowFocus&&i==col;std::string name=tabs[4+i];int count=0;for(auto& g:m.games)if(g.system==name)++count;
                    if(f)focusRing(x,y0,w,h,18);round(x,y0,w,h,18,f?theme.card:theme.panel);consoleArt(x+10,y0+6,w-20,h-50,name);
                    text(x+18,y0+h-38,name,18,theme.text,true,w-70);std::string n=std::to_string(count);text(x+w-18-width(n,16),y0+h-37,n,16,theme.muted);}}
            if(kind==2){const int w=150,h=200,step=164; // 7 posters end where the console row ends (x 1196)
               float t=float(std::clamp(col-3,0,std::max(0,int(list.size())-7)));rowScroll[2]+=(t-rowScroll[2])*std::min(1.f,dt*16.f);if(std::abs(t-rowScroll[2])<.002f)rowScroll[2]=t;
                int first=int(std::floor(rowScroll[2]));
                for(int i=first;i<std::min(first+8,int(list.size()));++i){auto& g=m.games[list[i]];int x=64+int(std::round((i-rowScroll[2])*step));bool f=rowFocus&&i==col;if(!g.cover.empty())keep.insert(g.cover);
                    if(f)focusRing(x,y0,w,h,14);poster(g,x,y0,w,h);text(x,y0+h+12,g.title,16,f?theme.text:theme.muted,f,w);}}
        }
        // Scrolled rows fade out under the header instead of being cut on a hard line.
        if(homeScroll>1.f){SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);auto bg=theme.background;
            for(int i=0;i<28;++i){SDL_SetRenderDrawColor(renderer,(bg>>16)&255,(bg>>8)&255,bg&255,Uint8(255-i*9));SDL_RenderDrawLine(renderer,0,98+i,1279,98+i);}
            SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);}
        SDL_RenderSetClipRect(renderer,nullptr);
        for(auto it=covers.begin();it!=covers.end();)if(!keep.count(it->first)){SDL_DestroyTexture(it->second.texture);it=covers.erase(it);}else++it;
    }
    // Search: field, an on-screen keyboard for the remote and live results; Right from the last key column reaches the results.
    void search(Model& m){
        auto results=m.searchResults();
        round(60,108,1160,62,20,theme.panel);magnifier(96,139,theme.muted,theme.panel);
        if(m.query.empty())text(124,121,"Nome do jogo",26,theme.muted);
        else{text(124,119,m.query,28,theme.text,true,1060);if(!m.inResults&&SDL_GetTicks()/530%2==0)rect(127+width(m.query,28,true),126,2,30,theme.accent);}
        const int kx=60,ky=196,ks=48,gap=6;
        for(int r=0;r<4;++r)for(int c=0;c<10;++c){int x=kx+c*(ks+gap),y=ky+r*(ks+gap);bool f=!m.inResults&&m.key==r*10+c;
            round(x,y,ks,ks,12,f?theme.text:theme.card);centered(x,y+11,ks,std::string(1,char(std::toupper(static_cast<unsigned char>(keyRows[r][c])))),21,f?theme.background:theme.text,true);}
        const char* wide[]={"Espaço","Apagar","Limpar"};const int start[]={0,4,7},span[]={4,3,3};int group=m.key%10<=3?0:m.key%10<=6?1:2;
        for(int g=0;g<3;++g){int x=kx+start[g]*(ks+gap),w=span[g]*ks+(span[g]-1)*gap,y=ky+4*(ks+gap);bool f=!m.inResults&&m.key/10==4&&group==g;
            round(x,y,w,ks,12,f?theme.text:theme.card);centered(x,y+13,w,wide[g],17,f?theme.background:theme.text,true);}
        const int rx=620,rw=600;
        if(results.empty()){text(rx,210,"Nenhum jogo encontrado",20,theme.muted);return;}
        int first=std::clamp(m.result-3,0,std::max(0,int(results.size())-7));
        for(int i=first;i<std::min(first+7,int(results.size()));++i){auto& g=m.games[results[i]];int y=ky+(i-first)*62;bool f=m.inResults&&i==m.result;
            round(rx,y,rw,54,14,f?theme.text:theme.panel);text(rx+22,y+13,g.title,21,f?theme.background:theme.text,true,rw-190);text(rx+rw-20-width(g.system,15),y+17,g.system,15,f?0x65547E:theme.muted);}
    }
    void minimalLibrary(Model& m,const std::vector<size_t>& list,const Game& selected){
        centered(790,190,400,systemName(selected.system),16,theme.muted,true); // same box as the art below
        int start=std::clamp(m.selection-2,0,std::max(0,int(list.size())-6));
        for(int i=start;i<std::min(start+6,int(list.size()));++i){int y=237+(i-start)*62;bool focus=i==m.selection;if(focus)round(61,y-1,606,47,23,m.sidebar?theme.card:theme.text);text(82,y+6,m.games[list[i]].title,24,focus&&!m.sidebar?theme.background:theme.text,true,547);}
        consoleArt(790,222,400,368,selected.system);centered(790,603,400,std::to_string(m.selection+1)+" de "+std::to_string(list.size())+" jogos",15,theme.muted);
    }
    void details(Model& m){
        auto* g=m.selected();if(!g)return;poster(*g,64,168,270,360);
        eyebrow(435,173,systemName(g->system));titleLines(432,206,g->title,43,748,2);
        // The line under the title fills the gap above the actions; play time used to sit under the poster.
        if(!g->available)text(436,303,"Reconecte o USB para jogar",17,0xE6BC8B);
        else text(436,303,g->played>=60?std::to_string(g->played/60)+" min jogados":"Ainda não jogado",17,theme.muted);
        // Without a saved game there is nothing to resume: one full-width action instead of a dead button.
        if(m.resumeAvailable){actionButton(436,353,337,"Jogar agora",m.option==0,true);actionButton(793,353,389,"Continuar partida",m.option==1,false);}
        else actionButton(436,353,746,"Jogar agora",m.option==0,true);
        row(445,g->favorite?"Remover dos favoritos":"Adicionar aos favoritos","",m.option==2,436,746);
        row(516,"Imagem e áudio","Por jogo / console",m.option==3,436,746);
        text(462,607,"Voltar à biblioteca",18,m.option==4?theme.accent:theme.muted,m.option==4,650);if(m.option==4)chevron(1153,621,theme.accent);
    }
    void settings(Model& m){
        menuTitle("AJUSTES","Do seu jeito.","Sua coleção e suas preferências, em um só lugar.");
        const char* labels[]={"Adicionar pasta de jogos","Atualizar biblioteca","Aparência","Modo leve","Exportar saves","Restaurar saves","Controles e botões","Diagnóstico","Sobre o RetroTV","Testar controle","Importar BIOS · PlayStation","Controle pelo celular"};
        int first=std::clamp(m.option-3,0,6);
        for(int i=first;i<12&&i<first+6;++i){std::string value=i==2?(m.minimal?"Minimal":"Galeria"):i==3?(m.performance?"Ativo":"Desativado"):i==9?"Demo original":i==11?(m.phoneEnabled?"Ativo":"Desativado"):"";row(166+(i-first)*75,labels[i],value,m.option==i);}
        text(453,628,std::to_string(m.option+1)+" / 12",13,theme.muted);round(1209,167,3,430,1,theme.card);round(1209,167+first*28,3,230,1,theme.muted);
    }
    void playback(Model& m){
        menuTitle("IMAGEM E ÁUDIO","Uma boa partida.",m.profileGame.title);text(453,123,m.profileSystem?"PADRÃO DO CONSOLE":"AJUSTES DESTE JOGO",12,theme.accent,true);
        const std::string labels[]={"Aplicar a","Proporção da tela","Imagem","Volume do jogo","Restaurar padrões","Voltar"};
        const std::string values[]={m.profileSystem?m.profileGame.system:"Este jogo",aspectLabel(m.profileAspect),m.profileFilter=="nearest"?"Pixels nítidos":"Suave",std::to_string(m.profileVolume)+"%","",""};
        for(int i=0;i<6;++i)row(166+i*75,labels[i],values[i],m.option==i);
        text(453,629,m.profileHint,13,theme.muted,false,730);
    }
    void quick(Model& m){
        const auto& g=m.sessionGame;eyebrow(64,147,"PAUSA");titleLines(61,183,g.title.empty()?"Sua partida":g.title,34,332,2);text(65,288,g.system,15,theme.accent,true);
        if(!theme.minimal&&!g.id.empty())poster(g,83,347,174,232);else consoleArt(56,400,330,215,g.system);
        line(410,153,410,623,theme.card);
        const char* labels[]={"Continuar partida","Salvar estado","Carregar estado","Reiniciar jogo","Controles","Imagem e áudio","Trocar disco","Salvar e voltar à biblioteca","Diagnóstico da partida"};
        int first=std::clamp(m.option-3,0,3);for(int i=first;i<9&&i<first+6;++i)row(166+(i-first)*75,labels[i],"",m.option==i);
        text(453,628,std::to_string(m.option+1)+" / 9",13,theme.muted);
    }
    void draw(Model& m,const std::string& status){
        auto now=SDL_GetTicks();float dt=lastDraw?std::min(.1f,(now-lastDraw)/1000.f):.016f;lastDraw=now;theme=(m.minimal||m.performance)?Theme::minimalTheme():Theme::classic();
        color(theme.background);SDL_RenderClear(renderer);
        if(m.screen==Screen::QuickMenu&&m.sessionActive&&!theme.minimal&&pausedBackdrop){pausedBackdrop();SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);SDL_SetRenderDrawColor(renderer,22,21,25,245);SDL_Rect overlay{0,0,1280,720};SDL_RenderFillRect(renderer,&overlay);SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);}
        statusLine=status;
        if(m.screen==Screen::Library){header(m);library(m,dt);}else if(m.screen==Screen::Search){header(m);search(m);}else{wordmark();
            if(m.screen==Screen::Details)details(m);
            else if(m.screen==Screen::Settings)settings(m);
            else if(m.screen==Screen::Playback)playback(m);
            else if(m.screen==Screen::QuickMenu)quick(m);
            else if(m.screen==Screen::Controls){menuTitle("GAMEPAD","Cada botão conta.","Escolha um comando e pressione OK para remapear.");const char* keys[]={"B","Y","SELECT","START","Cima","Baixo","Esquerda","Direita","A","X","L1","R1","L2","R2","L3","R3"};int first=std::clamp(m.option-3,0,10);for(int i=first;i<16&&i<first+6;++i)row(166+(i-first)*75,std::string("Botão ")+keys[i],"",m.option==i);text(453,628,m.message.substr(0,m.message.find('\n')),13,theme.muted,false,730);}
            else{menuTitle(m.screen==Screen::Error?"ATENÇÃO":m.screen==Screen::Diagnostics?"DIAGNÓSTICO":"CONFIRMAÇÃO",m.screen==Screen::Error?"Vamos resolver.":m.screen==Screen::Diagnostics?"Por dentro.":"Tudo certo?","Voltar retorna ao menu anterior.");round(451,166,731,454,18,theme.panel);size_t start=0;int y=189;
                while(start<m.message.size()&&y<573){auto end=m.message.find('\n',start);std::string lineText=m.message.substr(start,end-start);while(width(lineText,18)>675&&y<573){size_t cut=lineText.size();while(cut>0&&width(lineText.substr(0,cut),18)>675)--cut;while(cut>0&&(static_cast<unsigned char>(lineText[cut])&0xc0)==0x80)--cut;if(!cut)break;text(476,y,lineText.substr(0,cut),18,theme.muted);lineText=lineText.substr(cut);y+=29;}text(476,y,lineText,18,theme.muted,false,675);if(end==std::string::npos)break;start=end+1;y+=39;}
            }
        }
        if(m.screen!=Screen::Library&&m.screen!=Screen::Search&&m.screen!=Screen::Details)footer(m,status);if(theme.minimal)clearCovers();
    }
};
}