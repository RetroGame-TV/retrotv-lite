#include "ui/model.hpp"
#include "ui/playback_profile.hpp"
#include "ui/pixel_alpha.hpp"
#include <map>
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
    m.screen=Screen::Details;m.option=0;m.resumeAvailable=false;m.navigate(Action::Right);check(m.option==0,"missing resume received focus");m.resumeAvailable=true;m.navigate(Action::Right);check(m.option==1,"resume action unreachable");m.navigate(Action::Down);check(m.option==2,"secondary actions unreachable");m.navigate(Action::Down);check(m.option==3,"playback editor unreachable");m.navigate(Action::Up);check(m.option==2,"detail return focus");
    Game game{"profile-test","Profile test","NES","test","quicknes"};std::map<std::string,std::string> prefs;
    auto read=[&](const std::string& key){auto it=prefs.find(key);return it==prefs.end()?std::string():it->second;};
    auto options=readPlaybackProfile(game,read);check(options.aspect=="4:3"&&options.filter=="nearest"&&options.volume==100,"unsafe playback defaults");
    prefs[playbackKey("aspect",true,game)]="native";prefs[playbackKey("filter",true,game)]="linear";prefs[playbackKey("volume",true,game)]="50";
    options=readPlaybackProfile(game,read);check(options.aspect=="native"&&options.filter=="linear"&&options.volume==50,"console defaults not inherited");
    prefs[playbackKey("aspect",false,game)]="wide";prefs[playbackKey("volume",false,game)]="0";
    options=readPlaybackProfile(game,read);check(options.aspect=="wide"&&options.volume==0,"game override ignored");
    options=readPlaybackProfile(game,read,true);check(options.aspect=="native"&&options.volume==50,"game override leaked into console editor");
    prefs[playbackKey("aspect",false,game)]="";prefs[playbackKey("volume",false,game)]="101";
    options=readPlaybackProfile(game,read);check(options.aspect=="native"&&options.volume==50,"reset or invalid preference damaged defaults");
    prefs["aspect-"+game.id]="wide";check(readPlaybackProfile(game,read).aspect=="wide","legacy aspect lost");
    check(videoBounds("4:3",256,240).x==160&&videoBounds("4:3",256,240).w==960,"4:3 bounds incorrect");
    check(videoBounds("native",256,240).w==768&&videoBounds("wide",256,240).w==1280,"native or wide bounds incorrect");
    check(videoBounds("native",0,0).w==960,"invalid video dimensions unsafe");
    check(scaledSample(-32768,25)==-8192&&scaledSample(32767,100)==32767&&scaledSample(123,0)==0,"audio scaling clipped or mute failed");
    uint8_t pixels[]={50,25,0,128,25,15,5,0};straightAlpha(pixels,sizeof(pixels));check(pixels[0]==100&&pixels[1]==50&&pixels[3]==128&&pixels[4]==25,"premultiplied image alpha incorrect");
    uint8_t font[]={80,80,80,80};straightAlpha(font,4,true);check(font[0]==255&&font[3]==80,"font coverage applied twice");
    std::cout<<"Navigation, video/audio profiles, alpha and memory policies passed\n";
}
