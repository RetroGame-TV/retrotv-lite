#pragma once
#include "save_store.hpp"
namespace rt {
// Only packaged paths may be selected. Snapshot metadata never supplies a path.
inline std::string selectSnesCore(const std::string& legacy,const std::string& fast,const SaveStore& saves,bool resume){
    if(!resume)return std::filesystem::exists(fast)?fast:legacy;
    auto identity=saves.stateCore();
    if(identity.empty())throw std::runtime_error("Partida salva ausente; original preservado");
    auto end=identity.find_last_of(':');
    for(const auto& path:{fast,legacy})
        if(std::filesystem::exists(path)&&end!=std::string::npos&&identity.substr(end+1)==std::to_string(fileCrc(path)))return path;
    throw std::runtime_error("Núcleo da partida salva não está neste APK; arquivos preservados");
}
inline void preserveSnesSaves(const SaveStore& saves){
    // Permanent first-upgrade copies, separate from rolling .prev backups.
    for(bool state:{false,true})for(const auto& suffix:{std::string(),std::string(".prev")}){
        auto source=saves.path(state)+suffix,backup=source+".before-snes-lagfix";
        if(std::filesystem::exists(source)&&!std::filesystem::exists(backup))atomicWrite(backup,readBounded(source),false);
    }
}
}
