#include "emulation/host.hpp"
#include "emulation/save_store.hpp"
#include <iostream>
#include <stdexcept>
using namespace rt;
void check(bool c,const char* text){if(!c)throw std::runtime_error(text);}
int main(int argc,char**argv){
    try{
        if(argc!=4)throw std::runtime_error("host_tests core demo scratch");
        check(stagedName("file:///data/user/0/x/files/Games/Break%20An%20Egg.md","Mega Drive")=="content.md","file URI extension");
        check(stagedName("content://ext/tree/a/document/primary%3AGames%2FSonic.GEN","Mega Drive")=="content.gen","SAF extension");
        check(stagedName("content://ext/document/1234","Master System")=="content.sms"&&stagedName("content://x/a.b/c%2F..","Mega Drive")=="content.md","fallback extension");
        auto rom=readBounded(argv[2]);check(rom.size()==24592,"unexpected demo size");
        Host bad;check(!bad.open("missing-core",argv[2],rom,".","."),"missing core accepted");check(bad.state==SessionState::Failed,"failure state");
        Host h;unsigned frames=0;size_t samples=0;uint32_t first=0,last=0;
        h.video=[&](const void* p,unsigned w,unsigned height,size_t pitch,retro_pixel_format){check(w>0&&height>0,"empty frame");++frames;last=crc32(static_cast<const uint8_t*>(p),pitch*height);if(!first)first=last;};
        h.audio=[&](const int16_t*,size_t n){samples+=n;};
        check(h.open(argv[1],argv[2],rom,".","."),h.error.c_str());check(h.state==SessionState::Running,"launch state");
        for(int i=0;i<120;++i)h.run();auto snapshot=h.serialize();check(!snapshot.empty(),"real core serialize failed");
        h.pause();auto before=frames;h.run();check(frames==before,"paused core ran");h.resume();
        h.buttons[0][RETRO_DEVICE_ID_JOYPAD_RIGHT]=1;for(int i=0;i<120;++i)h.run();check(first!=last,"controller did not alter rendered frame");check(h.unserialize(snapshot),"real core restore failed");
        h.buttons[1][RETRO_DEVICE_ID_JOYPAD_A]=1;h.reset();h.run();check(frames>=241&&samples>1000,"missing video/audio callbacks");
        SaveStore store(argv[3],"game:crc","QuickNES:pin");store.write(true,snapshot);check(store.read(true)==snapshot,"save roundtrip");
        auto ram=h.sram();check(!ram.empty(),"battery-backed SRAM unavailable");ram[0]=0x5a;check(h.setSram(ram),"SRAM write rejected");store.write(false,ram);check(store.read(false)==ram,"SRAM disk roundtrip failed");
        h.close();check(h.open(argv[1],argv[2],rom,".","."),"SRAM reopen failed");check(h.setSram(store.read(false)),"SRAM restore rejected");for(int i=0;i<5;++i)h.run();check(h.sram()[0]==0x5a,"SRAM did not survive core reopen");
        // Read the exact old RTVSAVE2 SRAM metadata shape after a core rebuild.
        std::string legacyMeta="game:crc\nQuickNES:old-version:old-build\nsram";
        std::vector<uint8_t> legacy{'R','T','V','S','A','V','E','2'};
        auto append32=[&](uint32_t value){for(int i=0;i<4;++i)legacy.push_back(uint8_t(value>>(8*i)));};
        append32(uint32_t(legacyMeta.size()));append32(uint32_t(ram.size()));append32(crc32(ram.data(),ram.size()));
        legacy.insert(legacy.end(),legacyMeta.begin(),legacyMeta.end());legacy.insert(legacy.end(),ram.begin(),ram.end());atomicWrite(store.path(false),legacy);
        SaveStore updated(argv[3],"game:crc","QuickNES:new-version:new-build");
        check(updated.read(false)==ram&&h.setSram(updated.read(false)),"legacy SRAM did not survive a core rebuild");
        bool snapshotRejected=false;try{updated.read(true);}catch(...){snapshotRejected=true;}check(snapshotRejected,"snapshot from a different core build accepted");
        bool otherCoreRejected=false;try{SaveStore other(argv[3],"game:crc","OtherCore:pin");other.read(false);}catch(...){otherCoreRejected=true;}check(otherCoreRejected,"SRAM from a different core family accepted");
        updated.write(false,ram);check(store.read(false)==ram,"stable SRAM metadata not backward-readable");
        check(!h.setSram(std::vector<uint8_t>(ram.size()+1)),"wrong SRAM payload size accepted by host");
        auto changed=snapshot;changed[0]^=1;store.write(true,changed);check(std::filesystem::exists(store.path(true)+".prev"),"no recovery backup");
        bool rejected=false;try{SaveStore wrong(argv[3],"different-game","QuickNES:pin");wrong.read(true);}catch(...){rejected=true;}check(rejected,"wrong content accepted");
        auto corrupt=readBounded(store.path(true));corrupt.back()^=1;atomicWrite(store.path(true),corrupt);rejected=false;try{store.read(true);}catch(...){rejected=true;}check(rejected,"corrupt payload accepted");
        auto validBackup=readBounded(store.path(true)+".prev");store.write(true,snapshot);check(readBounded(store.path(true)+".prev")==validBackup,"corrupt current destroyed valid backup");check(std::filesystem::exists(store.path(true)+".rejected"),"corrupt save not preserved");
        h.close();check(h.state==SessionState::Menu,"did not return to menu");
        for(int i=0;i<10;++i){check(h.open(argv[1],argv[2],rom,".","."),"reopen failed");h.run();h.close();}
        std::cout<<"REAL QuickNES: 241+ frames, audio, input, pause/resume, reset, states, 10 reopen cycles passed. Save integrity/backup/mismatch tests passed.\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
