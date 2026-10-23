#include "ui/model.hpp"
#include <iostream>
#include <stdexcept>
using namespace rt;
void check(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
int main(){
    Model m;m.navigate(Action::Down);m.navigate(Action::Right);check(!m.sidebar&&m.selection==1,"empty actions unreachable");m.navigate(Action::Left);check(m.selection==0,"empty primary action unreachable");m.navigate(Action::Up);check(m.sidebar,"cannot return to navigation");m.navigate(Action::Right);check(m.tab==1,"horizontal tabs unreachable");m.navigate(Action::Down);m.navigate(Action::Ok);check(m.screen==Screen::Library,"empty collection entered details");
    m.games={{"1","One","NES","one","quicknes",false,true,2,0},{"2","Two","NES","two","quicknes",true,true,9,0},{"3","Three","SNES","three","snes9x2005",false,true,0,0}};
    m.selectTab(4);m.selection=0;m.sidebar=false;m.navigate(Action::Right);m.navigate(Action::Left);check(m.selected()->id=="1","left navigation failed");m.navigate(Action::Right);m.navigate(Action::Ok);check(m.selected()->id=="2","wrong selection");
    m.navigate(Action::Back);check(m.selected()->id=="2","details lost context");m.selectTab(5);m.selectTab(4);check(m.selected()->id=="2","tab lost focus");
    m.selectTab(3);check(m.visible().size()==1&&m.selected()->id=="2","favorites filtering");
    m.selectTab(2);m.selection=0;check(m.selected()->id=="2","recents ordering");
    m.selectTab(9);check(m.screen==Screen::Settings,"settings did not open");m.navigate(Action::Back);check(m.screen==Screen::Library&&m.tab==2,"settings lost previous collection");
    m.games.clear();check(m.selected()==nullptr,"removed USB caused invalid index");
    m.games={{"minimal","Minimal","NES","one","quicknes"},{"minimal2","Minimal 2","NES","two","quicknes"}};m.screen=Screen::Library;m.tab=0;m.selection=0;m.sidebar=false;m.minimal=true;m.navigate(Action::Down);check(m.selection==1,"minimal list navigation failed");m.navigate(Action::Up);check(m.selection==0&&!m.sidebar,"minimal list lost focus");m.navigate(Action::Up);check(m.sidebar,"minimal cannot reach navigation");
    check(coreFor("Wii").empty()&&coreFor("N64").empty(),"unproven platform enabled");
    check(profileFor("PlayStation",true).maxRomMiB==0,"disc permitted in ROM buffer");
    check(Theme::minimalTheme().cacheMiB<Theme::classic().cacheMiB,"performance budget");
    std::cout<<"9 navigation, filtering and memory policy scenarios passed\n";
}
