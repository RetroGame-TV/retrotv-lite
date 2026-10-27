#include "emulation/snes_core.hpp"
#include <iostream>
using namespace rt;
void check(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
template<class F> void rejects(F action){bool rejected=false;try{action();}catch(...){rejected=true;}check(rejected,"unsafe snapshot accepted");}
int main(int argc,char** argv){try{
    if(argc!=2)throw std::runtime_error("scratch directory required");
    auto root=std::string(argv[1]);std::filesystem::create_directories(root);
    auto legacy=root+"/legacy.so",fast=root+"/fast.so";
    atomicWrite(legacy,{1,2,3});atomicWrite(fast,{4,5,6});
    auto oldId="Snes9x 2005:old:"+std::to_string(fileCrc(legacy));
    auto newId="Snes9x 2005:new:"+std::to_string(fileCrc(fast));
    SaveStore old(root+"/saves","game:crc",oldId),updated(root+"/saves","game:crc",newId);
    // Distinct directory on every invocation: no unrelated files are deleted.
    std::vector<uint8_t> snapshot{10,20,30},ram{1,3,5,7};
    old.write(true,snapshot);old.write(false,ram);
    auto original=readBounded(old.path(true)),originalRam=readBounded(old.path(false));
    check(old.stateCore()==oldId,"identity discovery");
    check(selectSnesCore(legacy,fast,old,false)==fast,"new session did not use accelerated core");
    check(selectSnesCore(legacy,fast,updated,true)==legacy,"old snapshot did not select legacy binary");
    rejects([&]{updated.read(true);}); // No cross-binary state compatibility bypass.
    check(updated.read(false)==ram,"SRAM did not survive new core identity");
    preserveSnesSaves(updated);
    check(readBounded(old.path(true)+".before-snes-lagfix")==original,"missing permanent snapshot backup");
    check(readBounded(old.path(false)+".before-snes-lagfix")==originalRam,"missing permanent SRAM backup");
    // Rolling writes must not erase the original, even after repeated resume/save cycles.
    for(int i=0;i<8;++i){snapshot[0]=uint8_t(i);updated.write(true,snapshot);updated.write(false,ram);preserveSnesSaves(updated);}
    check(selectSnesCore(legacy,fast,updated,true)==fast,"new snapshot did not select new binary");
    check(updated.read(true)==snapshot,"updated state roundtrip");
    check(readBounded(old.path(true)+".before-snes-lagfix")==original,"original backup overwritten");
    check(readBounded(old.path(false)+".before-snes-lagfix")==originalRam,"SRAM backup overwritten");
    SaveStore wrongGame(root+"/saves","other-game",newId);
    rejects([&]{selectSnesCore(legacy,fast,wrongGame,true);});
    SaveStore unknown(root+"/saves","game:crc","Snes9x 2005:unknown:1234");unknown.write(true,snapshot);
    auto unknownBytes=readBounded(unknown.path(true));rejects([&]{selectSnesCore(legacy,fast,unknown,true);});
    check(readBounded(unknown.path(true))==unknownBytes,"unknown build was modified");
    auto broken=unknownBytes;broken.back()^=1;atomicWrite(unknown.path(true),broken);
    rejects([&]{selectSnesCore(legacy,fast,unknown,true);});
    check(readBounded(unknown.path(true))==broken,"corrupt snapshot modified");
    for(size_t n=1;n<20;++n){atomicWrite(unknown.path(true),std::vector<uint8_t>(n,0));rejects([&]{unknown.stateCore();});}
    SaveStore empty(root+"/empty","game:crc",newId);rejects([&]{selectSnesCore(legacy,fast,empty,true);});
    check(selectSnesCore(legacy,root+"/missing.so",empty,false)==legacy,"standard APK fallback");
    std::cout<<"SNES core selection, strict identity, SRAM continuity, permanent backups, repeated writes, foreign/corrupt/missing states passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
