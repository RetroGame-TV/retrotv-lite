#pragma once
#include <SDL.h>
#include <jni.h>
#include <string>
#include <vector>
namespace rt {
class Platform {
    JNIEnv* env() { return static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv()); }
public:
    std::string call(const char* method, const std::string& value="") {
        auto* e=env(); auto activity=static_cast<jobject>(SDL_AndroidGetActivity()); auto cls=e->GetObjectClass(activity);
        auto mid=e->GetMethodID(cls,method,"(Ljava/lang/String;)Ljava/lang/String;");
        auto arg=e->NewStringUTF(value.c_str());
        auto result=static_cast<jstring>(e->CallObjectMethod(activity,mid,arg)); std::string out;
        if(e->ExceptionCheck()) { e->ExceptionDescribe(); e->ExceptionClear(); }
        else if(result) { const char* s=e->GetStringUTFChars(result,nullptr); out=s; e->ReleaseStringUTFChars(result,s); }
        if(result)e->DeleteLocalRef(result); e->DeleteLocalRef(arg); e->DeleteLocalRef(cls); e->DeleteLocalRef(activity); return out;
    }
    SDL_Texture* font(SDL_Renderer* r) {
        auto* e=env(); auto activity=static_cast<jobject>(SDL_AndroidGetActivity()); auto cls=e->GetObjectClass(activity);
        auto mid=e->GetMethodID(cls,"fontAtlas","()[B"); auto bytes=static_cast<jbyteArray>(e->CallObjectMethod(activity,mid));
        if(!bytes||e->ExceptionCheck()) { e->ExceptionClear(); return nullptr; }
        auto data=e->GetByteArrayElements(bytes,nullptr);
        auto* surface=SDL_CreateRGBSurfaceWithFormatFrom(data,1024,1792,32,4096,SDL_PIXELFORMAT_RGBA32);
        auto* texture=SDL_CreateTextureFromSurface(r,surface); SDL_FreeSurface(surface);
        e->ReleaseByteArrayElements(bytes,data,JNI_ABORT); e->DeleteLocalRef(bytes); e->DeleteLocalRef(cls); e->DeleteLocalRef(activity);
        SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND); return texture;
    }
    SDL_Texture* cover(SDL_Renderer* r,const std::string& uri) {
        auto* e=env(); auto activity=static_cast<jobject>(SDL_AndroidGetActivity()); auto cls=e->GetObjectClass(activity);
        auto mid=e->GetMethodID(cls,"cover","(Ljava/lang/String;)[B"); auto arg=e->NewStringUTF(uri.c_str());
        auto bytes=static_cast<jbyteArray>(e->CallObjectMethod(activity,mid,arg)); SDL_Texture* texture=nullptr;
        if(e->ExceptionCheck()) e->ExceptionClear();
        else if(bytes&&e->GetArrayLength(bytes)==192*256*4) {
            auto data=e->GetByteArrayElements(bytes,nullptr);
            auto* surface=SDL_CreateRGBSurfaceWithFormatFrom(data,192,256,32,192*4,SDL_PIXELFORMAT_RGBA32);
            texture=SDL_CreateTextureFromSurface(r,surface); SDL_FreeSurface(surface); e->ReleaseByteArrayElements(bytes,data,JNI_ABORT);
        }
        if(bytes)e->DeleteLocalRef(bytes); e->DeleteLocalRef(arg); e->DeleteLocalRef(cls); e->DeleteLocalRef(activity); return texture;
    }
};
inline std::vector<std::string> split(const std::string& s,char delim) {
    std::vector<std::string> out; size_t start=0,pos;
    while((pos=s.find(delim,start))!=std::string::npos) { out.push_back(s.substr(start,pos-start)); start=pos+1; }
    out.push_back(s.substr(start)); return out;
}
}
