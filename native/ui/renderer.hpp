#pragma once
#include <SDL.h>
#include "model.hpp"
namespace rt {
class UI {
    SDL_Renderer* renderer; SDL_Texture* atlas;
public:
    Theme theme=Theme::classic();
    UI(SDL_Renderer* r,SDL_Texture* a):renderer(r),atlas(a){}
    void color(uint32_t c) { SDL_SetRenderDrawColor(renderer,(c>>16)&255,(c>>8)&255,c&255,255); }
    void rect(int x,int y,int w,int h,uint32_t c) { color(c); SDL_Rect box{x,y,w,h}; SDL_RenderFillRect(renderer,&box); }
    void text(int x,int y,const std::string& s,int size=24,uint32_t c=0) {
        if(!atlas)return; if(!c)c=theme.text;
        SDL_SetTextureColorMod(atlas,(c>>16)&255,(c>>8)&255,c&255);
        size_t i=0; int count=0;
        while(i<s.size() && count<95) {
            unsigned ch=static_cast<unsigned char>(s[i++]);
            if((ch&0xe0)==0xc0 && i<s.size()) ch=((ch&31)<<6)|(static_cast<unsigned char>(s[i++])&63);
            else if(ch>=0xe0) { while(i<s.size() && (static_cast<unsigned char>(s[i])&0xc0)==0x80)++i; ch='?'; }
            if(ch<32||ch>255)ch='?'; unsigned index=ch-32;
            SDL_Rect src{int(index%16)*64,int(index/16)*64,64,64},dst{x,y,size*2,size*2};
            SDL_RenderCopy(renderer,atlas,&src,&dst); x+=size; ++count;
        }
    }
    void button(int y,const std::string& title,bool focus) {
        rect(312,y,860,48,focus?theme.accent:theme.card); text(330,y+3,title,21,focus?theme.background:theme.text);
    }
    void draw(Model& m,const std::string& status) {
        theme=(m.minimal||m.performance)?Theme::minimalTheme():Theme::classic();
        color(theme.background); SDL_RenderClear(renderer);
        rect(40,34,6,38,theme.accent); text(62,28,"RETROTV",29); text(278,39,"LITE",16,theme.accent);
        text(815,39,m.performance?"MODO PERFORMANCE":"SUA BIBLIOTECA. SEU RITMO.",13,theme.muted);
        rect(40,90,1200,1,theme.card);
        for(int i=0;i<10;++i) {
            if(i==m.tab)rect(40,114+i*48,230,43,m.sidebar?theme.accent:theme.card);
            text(57,114+i*48,tabs[i],18,i==m.tab&&m.sidebar?theme.background:theme.text);
        }
        if(m.screen==Screen::Library) {
            auto* g=m.selected(); auto list=m.visible();
            text(312,108,g?g->system:"VAMOS COMEÇAR",16,theme.accent);
            text(312,143,g?g->title.substr(0,28):"Seu próximo jogo está no USB",25);
            text(312,200,g?(g->available?"OK para ver opções e jogar":"USB indisponível. Reconecte a pasta."):"Escolha uma subpasta com jogos do seu acervo.",15,theme.muted);
            text(312,248,std::to_string(list.size())+" jogos nesta coleção",14,theme.muted);
            if(list.empty()) {
                rect(312,310,860,240,theme.card); text(345,345,"Tudo pronto para a sua coleção",23);
                text(345,410,"Menu > Biblioteca USB > Escolher pasta",17,theme.accent);
                text(345,456,"Sem ROMs, BIOS ou capas comerciais incluídas.",14,theme.muted);
            } else {
                int start=(m.selection/5)*5;
                for(int row=0;row<5 && start+row<int(list.size());++row) {
                    auto& game=m.games[list[start+row]]; bool focus=!m.sidebar && start+row==m.selection;
                    rect(312,294+row*58,860,52,focus?theme.accent:theme.card);
                    text(332,300+row*58,game.title.substr(0,31),20,focus?theme.background:theme.text);
                    text(1010,305+row*58,game.favorite?"FAV":game.system.substr(0,9),12,focus?theme.background:theme.muted);
                }
            }
        } else if(m.screen==Screen::Details) {
            auto* g=m.selected(); if(g) {
                text(312,117,g->system,17,theme.accent); text(312,164,g->title.substr(0,29),25);
                text(312,219,"Tempo jogado: "+std::to_string(g->played/60)+" min  |  núcleo: "+g->core,14,theme.muted);
                const char* labels[]={"Jogar / iniciar normalmente","Continuar do Quick Resume","Favorito","Voltar para biblioteca"};
                for(int i=0;i<4;++i)button(302+i*61,i==2?(g->favorite?"Remover dos favoritos":"Adicionar aos favoritos"):labels[i],m.option==i);
            }
        } else if(m.screen==Screen::Settings) {
            text(312,113,"Do seu jeito",28); text(312,167,"Configurações locais · sem conta · sem nuvem",15,theme.muted);
            std::vector<std::string> items={"Biblioteca USB: escolher pasta","Biblioteca USB: atualizar / cancelar scan",std::string("Tema: ")+(m.minimal?"Minimal":"Classic"),std::string("Performance: ")+(m.performance?"ligado":"desligado"),"Exportar backup de saves","Restaurar backup de saves","Controles e remapeamento","Diagnóstico","Sobre e licenças","Testar demonstração original NES"};
            int first=std::max(0,m.option-5);
            for(int i=first;i<int(items.size())&&i<first+6;++i)button(222+(i-first)*59,items[i],m.option==i);
        } else if(m.screen==Screen::QuickMenu) {
            text(312,118,"Pausa · no seu tempo",28);
            const char* items[]={"Continuar","Salvar estado","Carregar estado","Reiniciar jogo","Controles","Proporção: alternar 4:3 / tela cheia","Trocar disco","Salvar e sair para biblioteca","Diagnóstico da sessão"};
            int first=std::max(0,m.option-5);for(int i=first;i<9&&i<first+6;++i)button(215+(i-first)*61,items[i],m.option==i);
        } else {
            text(312,118,m.screen==Screen::Error?"Não foi possível continuar":m.screen==Screen::Diagnostics?"Diagnóstico local":m.screen==Screen::Controls?"Controles":"RetroTV Lite",26);
            int y=196; size_t start=0;
            while(start<m.message.size() && y<598) { auto end=m.message.find('\n',start); text(312,y,m.message.substr(start,end-start).substr(0,57),15,theme.muted); if(end==std::string::npos)break; start=end+1; y+=39; }
        }
        rect(40,649,1200,1,theme.card); text(44,670,"OK selecionar   Voltar retornar   Menu opções",14,theme.muted); text(748,670,status.substr(0,40),13,theme.accent);
    }
};
}
