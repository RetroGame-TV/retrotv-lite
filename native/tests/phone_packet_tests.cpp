#include "input/phone_packet.hpp"
#include <stdexcept>
#include <iostream>
using namespace rt;
void check(bool v){if(!v)throw std::runtime_error("packed phone input mismatch");}
int main(){try{
    check(!decodePhonePacket(0).present);check(decodePhonePacket(UINT64_MAX).off);
    for(unsigned mask: {0u,1u,2u,4u,8u,16u,32u,64u,128u,256u,512u,1024u,2048u,65536u,131072u,0x30fffu})
        for(int axis=-127;axis<=127;++axis)for(unsigned dpad=0;dpad<16;++dpad){
            auto value=(1ull<<54)|mask|(uint64_t(dpad)<<18)|(uint64_t(uint8_t(axis))<<22)|(uint64_t(127)<<30)|(uint64_t(255)<<38);
            auto p=decodePhonePacket(value);check(p.present&&!p.off&&p.buttons==mask&&p.dpad==int(dpad)&&p.lx==axis&&p.ly==127&&p.rx==-1&&p.ry==0);
        }
    check(!decodePhonePacket((1ull<<54)|0x1000).present);check(!decodePhonePacket((1ull<<54)|(128ull<<22)).present);check(!decodePhonePacket(1ull<<55).present);
    std::cout<<"Packed JNI: all button bits, 16 d-pad states, signed axes, neutral/disconnect and malformed values passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
