#pragma once
#include <SDL.h>
#include "model.hpp"
#include "playback_profile.hpp"
#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <sstream>
namespace rt {
class UI {
    SDL_Renderer* renderer; SDL_Texture* atlas; SDL_Texture* shapes=nullptr;
    std::array<float,448> advances{};
    std::function<SDL_Texture*(const std::string&)> loadCover;
    struct Cover { SDL_Texture* texture=nullptr; uint32_t polled=0; };
    std::map<std::string,Cover> covers;
    float scroll=0,focusLift=0;int previousTab=-1,previousSelection=-1;uint32_t lastDraw=0;
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
        auto it=covers.find(game.cover);if(it==covers.end()&&covers.size()<6)it=covers.emplace(game.cover,Cover{}).first;if(it==covers.end())return nullptr;
        auto& c=it->second;auto now=SDL_GetTicks();if(!c.texture&&now-c.polled>150){c.polled=now;c.texture=loadCover(game.cover);}return c.texture;
    }
    void mask(int x,int y,int w,int h,int r){corner(x,y,r,0,theme.background,true);corner(x+w-r,y,r,1,theme.background,true);corner(x,y+h-r,r,2,theme.background,true);corner(x+w-r,y+h-r,r,3,theme.background,true);}
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
        if(!value.empty())text(x+w-222,y+17,value,16,focus?0x65547E:theme.muted,true,179);
        chevron(x+w-28,y+29,focus?theme.background:theme.muted);
    }
    void menuTitle(const std::string& tag,const std::string& title,const std::string& description){
        eyebrow(62,149,tag);titleLines(60,185,title,38,345,2);titleLines(63,285,description,18,322,3,26);device(76,454,178,theme.minimal?theme.muted:0x71647D,theme.background,"SNES");line(410,152,410,623,theme.card,1);
    }
public:
    Theme theme=Theme::classic();std::function<void()> pausedBackdrop;
    UI(SDL_Renderer* r,SDL_Texture* a,const std::string& metrics,std::function<SDL_Texture*(const std::string&)> loader):renderer(r),atlas(a),loadCover(std::move(loader)){
        advances.fill(20);std::istringstream input(metrics);std::string value;size_t i=0;while(i<advances.size()&&std::getline(input,value,','))advances[i++]=std::max(1.f,std::strtof(value.c_str(),nullptr));makeShapes();
    }
    void setAtlas(SDL_Texture* texture){atlas=texture;makeShapes();}
    void clearCovers(){for(auto& c:covers)SDL_DestroyTexture(c.second.texture);covers.clear();}
    void release(){clearCovers();if(shapes)SDL_DestroyTexture(shapes);shapes=nullptr;}
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
    void gear(int x,int y,uint32_t c){for(int i=0;i<8;++i){float a=i*3.14159265f/4;circle(x+int(std::cos(a)*10),y+int(std::sin(a)*10),3,c);}circle(x,y,10,c);circle(x,y,5,theme.background);}
    void device(int x,int y,int s,uint32_t ink,uint32_t body,const std::string& system="NES"){
        if(system=="Game Boy"){round(x,y,s*3/4,s,s/10,ink);round(x+3,y+3,s*3/4-6,s-6,s/10,body);round(x+s/8,y+s/8,s/2,s*2/5,8,ink);round(x+s/8+3,y+s/8+3,s/2-6,s*2/5-6,5,body);plus(x+s/5,y+s*3/4,ink);circle(x+s/2,y+s*4/5,5,ink);circle(x+s*5/8,y+s*7/10,5,ink);return;}
        round(x,y,s*3/2,s*3/5,s/5,ink);round(x+3,y+3,s*3/2-6,s*3/5-6,s/5,body);int cy=y+s*3/10;
        round(x+s/4-5,cy-s/9,11,s*2/9,3,ink);round(x+s/4-s/9,cy-5,s*2/9,11,3,ink);
        circle(x+s*6/5,cy-9,s/18,ink);circle(x+s*21/20,cy+9,s/18,ink);round(x+s*3/5,cy+7,s/8,4,2,ink);round(x+s*4/5,cy+7,s/8,4,2,ink);
        if(system=="SNES"||system=="PlayStation"){circle(x+s*6/5,cy+13,s/18,ink);circle(x+s*4/3,cy+1,s/18,ink);}
    }
    void footer(Model& m,const std::string& status){
        line(60,665,1220,665,theme.card);keycap(60,685,31,"OK");text(101,684,m.screen==Screen::Playback?"Alterar":m.screen==Screen::Library?"Selecionar":"Confirmar",15,theme.muted);
        keycap(261,685,54,"Voltar");text(326,684,"Retornar",15,theme.muted);keycap(467,685,49,"Menu");text(527,684,"Opções",15,theme.muted);
        text(854,686,status,13,theme.muted,false,365);
    }
    void header(Model& m){
        wordmark();text(991,45,std::to_string(m.games.size())+(m.games.size()==1?" jogo na biblioteca":" jogos na biblioteca"),15,theme.muted,false,229);
        const int widths[]={94,125,121,129,80,91,144,111,154,111};int x=58;
        for(int i=0;i<10;++i){bool current=m.tab==i;int w=widths[i]-6;
            if(i==4)line(x-6,118,x-6,140,theme.card);
            if(current){round(x,105,w,43,14,m.sidebar?theme.text:theme.card);if(!m.sidebar)round(x+16,145,w-32,2,1,theme.accent);}
            auto ink=current?(m.sidebar?theme.background:theme.text):theme.muted;
            if(i==9){gear(x+23,127,ink);text(x+42,114,"Ajustes",17,ink,current,w-45);}else centered(x,113,w,tabs[i],18,ink,current);
            x+=widths[i];
        }
    }
    void poster(const Game& game,int x,int y,int w,int h){
        if(auto* texture=artwork(game)){SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);SDL_Rect dst{x,y,w,h};SDL_RenderCopy(renderer,texture,nullptr,&dst);return;}
        uint32_t hue=game.system=="NES"?0x684553:game.system=="SNES"?0x574A74:game.system=="Mega Drive"?0x34536A:game.system=="Arcade"?0x6D573C:0x45615D;if(theme.minimal)hue=0x333139;
        round(x,y,w,h,13,hue);SDL_Rect previousClip{};bool clipped=SDL_RenderIsClipEnabled(renderer);SDL_RenderGetClipRect(renderer,&previousClip);SDL_Rect clip{x,y+14,w,h-28};if(clipped){SDL_Rect intersection{};SDL_IntersectRect(&clip,&previousClip,&intersection);clip=intersection;}SDL_RenderSetClipRect(renderer,&clip);
        for(int dy=14;dy<h-14;dy+=4)rect(x,y+dy,w,std::min(4,h-14-dy),mix(hue,0x211E28,float(dy)/h*.65f));
        SDL_RenderSetClipRect(renderer,clipped?&previousClip:nullptr);
        text(x+16,y+15,game.system,13,0xE1DDE9,true,w-30);device(x+w/6,y+h*2/5,w*4/9,0xDBD4E6,mix(hue,0x211E28,.3f),game.system);
        text(x+16,y+h-65,"Sua coleção",12,0xCEC5D7,false,w-30);text(x+16,y+h-43,game.title,18,0xFFFFFF,true,w-32);mask(x,y,w,h,13);
    }
    void empty(Model& m){
        m.selection=std::clamp(m.selection,0,1);bool all=m.games.empty();eyebrow(67,209,all?"SUA BIBLIOTECA COMEÇA AQUI":"NESTA COLEÇÃO");
        text(64,247,all?"Seus clássicos.":"Ainda sem jogos.",48,theme.text,true,660);text(65,307,all?"Prontos para outra partida.":"Há muito para descobrir.",32,theme.text,true,655);
        text(67,377,all?"Adicione seu acervo e escolha o próximo jogo.":"Adicione jogos ou explore outra coleção.",19,theme.muted,false,630);
        actionButton(68,446,232,all?"Adicionar jogos":"Ver biblioteca",!m.sidebar&&m.selection==0,true,true,all);actionButton(318,446,214,all?"Testar controle":"Adicionar jogos",!m.sidebar&&m.selection==1,false);
        text(69,532,"Na sua TV. No seu tempo.",16,theme.muted);
        if(!theme.minimal){circle(975,361,151,0x211D2A);circle(975,361,121,0x292332);round(842,242,229,159,23,0x63516E);round(834,232,229,159,23,0xB9A1E2);round(848,246,201,129,13,0x24212B);round(864,262,169,91,8,0x342B43);
            int x=898,y=290;rect(x,y,14,14,theme.accent);rect(x+43,y,14,14,theme.accent);rect(x+14,y+16,43,12,theme.accent);rect(x+6,y+29,13,12,theme.accent);rect(x+49,y+29,13,12,theme.accent);
            round(917,402,62,7,3,0x7E6B8C);round(893,418,110,6,3,0x45374F);device(853,458,166,0xCFC0E8,0x211D2A,"SNES");circle(1129,268,4,theme.accent);plus(803,396,0x6E5C7B);
        }else device(817,318,232,theme.text,theme.background,m.tab>=4&&m.tab<=8?tabs[m.tab]:"SNES");
        text(68,601,"NES   /   SNES   /   MEGA DRIVE   /   ARCADE   /   PLAYSTATION",13,theme.muted,true,720);
    }
    void library(Model& m,float dt){
        auto list=m.visible();auto* game=m.selected();if(!game){empty(m);return;}if(theme.minimal){minimalLibrary(m,list,*game);return;}
        int target=std::clamp(m.selection-2,0,std::max(0,int(list.size())-5));
        if(previousTab!=m.tab||std::abs(target-scroll)>2.5f){scroll=float(target);previousTab=m.tab;}
        scroll+=(target-scroll)*std::min(1.f,dt*18.f);if(std::abs(target-scroll)<.002f)scroll=float(target);
        if(previousSelection!=m.selection){focusLift=0;previousSelection=m.selection;}
        focusLift+=((m.sidebar?0.f:1.f)-focusLift)*std::min(1.f,dt*20.f);
        int first=int(std::floor(scroll));std::set<std::string> keep;
        for(int i=first;i<std::min(first+6,int(list.size()));++i)if(!m.games[list[i]].cover.empty())keep.insert(m.games[list[i]].cover);
        for(auto it=covers.begin();it!=covers.end();)if(!keep.count(it->first)){SDL_DestroyTexture(it->second.texture);it=covers.erase(it);}else++it;
        backdrop(*game);round(65,185,58,25,8,0x30273D);centered(65,187,58,game->system,13,theme.accent,true);text(137,187,systemName(game->system),15,theme.muted,false,770);
        text(62,212,game->title,47,theme.text,true,1090);
        std::string meta=game->available?(game->played?std::to_string(game->played/60)+" min jogados":"Ainda não jogado"):"USB desconectado";if(game->favorite)meta+="   ·   Favorito";
        text(66,275,meta,17,game->available?theme.muted:0xE6BC8B);
        text(65,309,m.tab>=4?"Jogos da coleção":"Sua coleção",15,theme.text,true);text(1110,310,std::to_string(m.selection+1)+" / "+std::to_string(list.size()),14,theme.muted,false,108);
        SDL_Rect clip{49,330,1182,326};SDL_RenderSetClipRect(renderer,&clip);
        for(int i=first;i<std::min(first+6,int(list.size()));++i){auto& item=m.games[list[i]];bool current=i==m.selection;float lift=current?focusLift:0;int w=195+int(10*lift),h=260+int(13*lift),x=65+int(std::round((i-scroll)*231))-(w-195)/2,y=613-h;
            if(current){round(x-8,y-8,w+16,h+16,20,m.sidebar?0x28242F:0x302739);round(x-4,y-4,w+8,h+8,16,m.sidebar?0x6B5B7C:theme.accent);round(x-2,y-2,w+4,h+4,14,theme.background);}
            poster(item,x,y,w,h);
            if(current&&!m.sidebar){circle(x+w-25,y+25,15,theme.accent);play(x+w-29,y+25,10,theme.background);}
            text(x,629,item.title,current?18:16,current?theme.text:theme.muted,current,w);
        }
        SDL_RenderSetClipRect(renderer,nullptr);
    }
    void minimalLibrary(Model& m,const std::vector<size_t>& list,const Game& selected){
        eyebrow(65,188,"EXPLORAR");text(813,190,systemName(selected.system),16,theme.muted,true,365);
        int start=std::clamp(m.selection-2,0,std::max(0,int(list.size())-6));
        for(int i=start;i<std::min(start+6,int(list.size()));++i){int y=237+(i-start)*62;bool focus=i==m.selection;if(focus)round(61,y-1,606,47,23,m.sidebar?theme.card:theme.text);text(82,y+6,m.games[list[i]].title,24,focus&&!m.sidebar?theme.background:theme.text,true,547);}
        round(827,242,282,168,20,theme.text);round(830,245,276,162,17,theme.background);round(855,270,227,25,10,theme.text);round(858,273,221,19,7,theme.background);
        for(int i=0;i<3;++i)round(856,319+i*12,57,3,1,theme.muted);round(938,330,64,37,8,theme.text);round(941,333,58,31,5,theme.background);round(1020,330,52,37,8,theme.text);round(1023,333,46,31,5,theme.background);
        line(967,411,967,438,theme.muted,2);device(819,451,201,theme.text,theme.background,selected.system);centered(798,603,350,std::to_string(m.selection+1)+" de "+std::to_string(list.size())+" jogos",15,theme.muted);
    }
    void details(Model& m){
        auto* g=m.selected();if(!g)return;poster(*g,69,168,270,360);eyebrow(70,557,g->system);text(70,590,std::to_string(g->played/60)+" min jogados",17,theme.muted);
        eyebrow(435,173,systemName(g->system));titleLines(432,206,g->title,43,748,2);text(436,313,g->available?"Da sua biblioteca local":"Reconecte o USB para jogar",17,g->available?theme.muted:0xE6BC8B);
        actionButton(436,353,337,"Jogar agora",m.option==0,true);actionButton(793,353,389,m.resumeAvailable?"Continuar partida":"Sem partida salva",m.option==1,false,m.resumeAvailable);
        row(445,g->favorite?"Remover dos favoritos":"Adicionar aos favoritos","",m.option==2,436,746);
        row(516,"Imagem e áudio","Por jogo / console",m.option==3,436,746);
        text(462,607,"Voltar à biblioteca",18,m.option==4?theme.accent:theme.muted,m.option==4,650);if(m.option==4)chevron(1153,621,theme.accent);
    }
    void settings(Model& m){
        menuTitle("AJUSTES","Do seu jeito.","Sua coleção e suas preferências, em um só lugar.");
        const char* labels[]={"Adicionar pasta de jogos","Atualizar biblioteca","Aparência","Modo leve","Exportar saves","Restaurar saves","Controles e botões","Diagnóstico","Sobre o RetroTV","Testar controle","Importar BIOS · PlayStation"};
        int first=std::clamp(m.option-3,0,5);
        for(int i=first;i<11&&i<first+6;++i){std::string value=i==2?(m.minimal?"Minimal":"Galeria"):i==3?(m.performance?"Ativo":"Desativado"):i==9?"Demo original":"";row(166+(i-first)*75,labels[i],value,m.option==i);}
        text(453,628,std::to_string(m.option+1)+" / 11",13,theme.muted);round(1209,167,3,430,1,theme.card);round(1209,167+first*34,3,260,1,theme.muted);
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
        if(!theme.minimal&&!g.id.empty())poster(g,83,347,174,232);else device(74,450,184,theme.muted,theme.background,g.system);
        line(410,153,410,623,theme.card);
        const char* labels[]={"Continuar partida","Salvar estado","Carregar estado","Reiniciar jogo","Controles","Imagem e áudio","Trocar disco","Salvar e voltar à biblioteca","Diagnóstico da partida"};
        int first=std::clamp(m.option-3,0,3);for(int i=first;i<9&&i<first+6;++i)row(166+(i-first)*75,labels[i],"",m.option==i);
        text(453,628,std::to_string(m.option+1)+" / 9",13,theme.muted);
    }
    void draw(Model& m,const std::string& status){
        auto now=SDL_GetTicks();float dt=lastDraw?std::min(.1f,(now-lastDraw)/1000.f):.016f;lastDraw=now;theme=(m.minimal||m.performance)?Theme::minimalTheme():Theme::classic();
        color(theme.background);SDL_RenderClear(renderer);
        if(m.screen==Screen::QuickMenu&&m.sessionActive&&!theme.minimal&&pausedBackdrop){pausedBackdrop();SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);SDL_SetRenderDrawColor(renderer,22,21,25,245);SDL_Rect overlay{0,0,1280,720};SDL_RenderFillRect(renderer,&overlay);SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);}
        if(m.screen==Screen::Library){header(m);library(m,dt);}else{wordmark();
            if(m.screen==Screen::Details)details(m);
            else if(m.screen==Screen::Settings)settings(m);
            else if(m.screen==Screen::Playback)playback(m);
            else if(m.screen==Screen::QuickMenu)quick(m);
            else if(m.screen==Screen::Controls){menuTitle("GAMEPAD","Cada botão conta.","Escolha um comando e pressione OK para remapear.");const char* keys[]={"B","Y","SELECT","START","Cima","Baixo","Esquerda","Direita","A","X","L1","R1","L2","R2","L3","R3"};int first=std::clamp(m.option-3,0,10);for(int i=first;i<16&&i<first+6;++i)row(166+(i-first)*75,std::string("Botão ")+keys[i],"",m.option==i);text(453,628,m.message.substr(0,m.message.find('\n')),13,theme.muted,false,730);}
            else{menuTitle(m.screen==Screen::Error?"ATENÇÃO":m.screen==Screen::Diagnostics?"DIAGNÓSTICO":"CONFIRMAÇÃO",m.screen==Screen::Error?"Vamos resolver.":m.screen==Screen::Diagnostics?"Por dentro.":"Tudo certo?","Voltar retorna ao menu anterior.");round(451,166,731,454,18,theme.panel);size_t start=0;int y=189;
                while(start<m.message.size()&&y<573){auto end=m.message.find('\n',start);std::string lineText=m.message.substr(start,end-start);while(width(lineText,18)>675&&y<573){size_t cut=lineText.size();while(cut>0&&width(lineText.substr(0,cut),18)>675)--cut;while(cut>0&&(static_cast<unsigned char>(lineText[cut])&0xc0)==0x80)--cut;if(!cut)break;text(476,y,lineText.substr(0,cut),18,theme.muted);lineText=lineText.substr(cut);y+=29;}text(476,y,lineText,18,theme.muted,false,675);if(end==std::string::npos)break;start=end+1;y+=39;}
            }
        }
        footer(m,status);if(theme.minimal)clearCovers();
    }
};
}