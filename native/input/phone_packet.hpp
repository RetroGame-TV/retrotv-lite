#pragma once
#include <cstdint>
namespace rt {
struct PhonePacket {
    bool present=false,off=false;
    unsigned buttons=0;int dpad=0,lx=0,ly=0,rx=0,ry=0;
};
inline PhonePacket decodePhonePacket(uint64_t value){
    PhonePacket p;if(!value)return p;
    if(value==UINT64_MAX){p.off=true;return p;}
    if((value>>55)||!(value&(1ull<<54))||(value&0xf000))return p;
    p.buttons=unsigned(value&0x30fff);p.dpad=int((value>>18)&15);
    p.lx=int8_t(value>>22);p.ly=int8_t(value>>30);p.rx=int8_t(value>>38);p.ry=int8_t(value>>46);
    if(p.lx==-128||p.ly==-128||p.rx==-128||p.ry==-128)return {};
    p.present=true;return p;
}
}
