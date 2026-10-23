#include "ui/model.hpp"
#include <iostream>
#include <stdexcept>
using namespace rt;
void check(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
int main(){
    Model m; m.navigate(Action::Right);m.navigate(Action::Ok);check(m.screen==Screen::Library,"empty collection entered details");
    m.games={{"1","One","NES","one","quicknes",false,true,2,0},{"2","Two","NES","two","quicknes",true,true,9,0},{"3","Three","SNES","three","snes9x2005",false,true,0,0}};
    m.selectTab(4);m.sidebar=false;m.navigate(Action::Down);m.navigate(Action::Ok);check(m.selected()->id=="2","wrong selection");
    m.navigate(Action::Back);check(m.selected()->id=="2","details lost context");m.selectTab(5);m.selectTab(4);check(m.selected()->id=="2","tab lost focus");
    m.selectTab(3);check(m.visible().size()==1&&m.selected()->id=="2","favorites filtering");
    m.selectTab(2);m.selection=0;check(m.selected()->id=="2","recents ordering");
    m.selectTab(9);check(m.screen==Screen::Settings,"settings did not open");m.navigate(Action::Back);check(m.screen==Screen::Library&&m.tab==2,"settings lost previous collection");
    m.games.clear();check(m.selected()==nullptr,"removed USB caused invalid index");
    check(coreFor("Wii").empty()&&coreFor("N64").empty(),"unproven platform enabled");
    check(profileFor("PlayStation",true).maxRomMiB==0,"disc permitted in ROM buffer");
    check(Theme::minimalTheme().cacheMiB<Theme::classic().cacheMiB,"performance budget");
    std::cout<<"9 navigation, filtering and memory policy scenarios passed\n";
}
