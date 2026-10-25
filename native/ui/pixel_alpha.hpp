#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
namespace rt {
// Android Bitmap uses premultiplied RGB; SDL_BLENDMODE_BLEND expects straight RGB.
inline void straightAlpha(uint8_t* data,size_t bytes,bool whiteMask=false){
    for(size_t i=0;i+3<bytes;i+=4){unsigned alpha=data[i+3];if(!alpha)continue;
        if(whiteMask)data[i]=data[i+1]=data[i+2]=255;
        else if(alpha<255)for(int c=0;c<3;++c)data[i+c]=uint8_t(std::min(255u,(unsigned(data[i+c])*255+alpha/2)/alpha));
    }
}
}