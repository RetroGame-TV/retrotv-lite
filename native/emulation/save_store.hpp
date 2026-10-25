#pragma once
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif
namespace rt {
inline uint32_t crc32(const uint8_t* p,size_t n){uint32_t crc=~0u;while(n--){crc^=*p++;for(int bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320u&-(crc&1));}return ~crc;}
inline uint32_t fileCrc(const std::string& path){std::ifstream file(path,std::ios::binary);if(!file)throw std::runtime_error("Arquivo indisponível");uint32_t crc=~0u;char buffer[8192];while(file){file.read(buffer,sizeof(buffer));for(std::streamsize i=0;i<file.gcount();++i){crc^=uint8_t(buffer[i]);for(int j=0;j<8;++j)crc=(crc>>1)^(0xedb88320u&-(crc&1));}}return ~crc;}
inline std::vector<uint8_t> readBounded(const std::string& path,size_t max=34*1024*1024){std::ifstream f(path,std::ios::binary|std::ios::ate);if(!f)return {};auto n=f.tellg();if(n<0||uint64_t(n)>max)throw std::runtime_error("Arquivo excede limite seguro");std::vector<uint8_t> b(static_cast<size_t>(n));f.seekg(0);if(!b.empty()&&!f.read(reinterpret_cast<char*>(b.data()),n))throw std::runtime_error("Leitura incompleta");return b;}
inline void replaceFile(const std::string& from,const std::string& to){
#ifdef _WIN32
    if(!MoveFileExA(from.c_str(),to.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Falha na gravação atômica");
#else
    if(std::rename(from.c_str(),to.c_str())!=0)throw std::runtime_error("Falha na gravação atômica");
#endif
}
inline void atomicWrite(const std::string& path,const std::vector<uint8_t>& b,bool backupCurrent=true){
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    const auto tmp=path+".tmp";FILE* f=std::fopen(tmp.c_str(),"wb");if(!f)throw std::runtime_error("Sem espaço/permissão para salvar");
    bool ok=b.empty()||std::fwrite(b.data(),1,b.size(),f)==b.size();ok=std::fflush(f)==0&&ok;
#ifdef _WIN32
    ok=_commit(_fileno(f))==0&&ok;
#else
    ok=fsync(fileno(f))==0&&ok;
#endif
    ok=std::fclose(f)==0&&ok;if(!ok)throw std::runtime_error("Save incompleto: original preservado");
    if(backupCurrent&&std::filesystem::exists(path)){
        std::filesystem::copy_file(path,path+".prev.tmp",std::filesystem::copy_options::overwrite_existing);
        replaceFile(path+".prev.tmp",path+".prev");
    }
    replaceFile(tmp,path);
#ifndef _WIN32
    int dir=open(std::filesystem::path(path).parent_path().c_str(),O_RDONLY|O_DIRECTORY);if(dir>=0){fsync(dir);close(dir);}
#endif
}
class SaveStore {
    std::string directory,gameIdentity,coreIdentity;
    static std::string family(const std::string& value){return value.substr(0,value.find(':'));}
    static void put(std::vector<uint8_t>& b,uint32_t n){for(int i=0;i<4;++i)b.push_back(uint8_t(n>>(8*i)));}
    static uint32_t get(const std::vector<uint8_t>& b,size_t pos){return uint32_t(b.at(pos))|(uint32_t(b.at(pos+1))<<8)|(uint32_t(b.at(pos+2))<<16)|(uint32_t(b.at(pos+3))<<24);}
public:
    SaveStore(std::string dir,std::string game,std::string core):directory(std::move(dir)),gameIdentity(std::move(game)),coreIdentity(std::move(core)){}
    std::string path(bool state) const {return directory+(state?"/resume.state":"/progress.sram");}
    bool exists(bool state) const {return std::filesystem::exists(path(state));}
    void write(bool state,const std::vector<uint8_t>& payload){
        if(payload.empty())return;if(payload.size()>32*1024*1024)throw std::runtime_error("Save grande demais");
        const std::string metadata=gameIdentity+"\n"+(state?coreIdentity:family(coreIdentity))+"\n"+(state?"state":"sram");
        std::vector<uint8_t> out{'R','T','V','S','A','V','E','2'};put(out,uint32_t(metadata.size()));put(out,uint32_t(payload.size()));put(out,crc32(payload.data(),payload.size()));
        out.insert(out.end(),metadata.begin(),metadata.end());out.insert(out.end(),payload.begin(),payload.end());
        bool backupCurrent=true;
        if(exists(state)){try{read(state);}catch(...){backupCurrent=false;std::filesystem::copy_file(path(state),path(state)+".rejected",std::filesystem::copy_options::overwrite_existing);}}
        atomicWrite(path(state),out,backupCurrent);
    }
    std::vector<uint8_t> read(bool state) const {
        auto b=readBounded(path(state));if(b.empty())return {};
        if(b.size()<20||std::string(b.begin(),b.begin()+8)!="RTVSAVE2")throw std::runtime_error("Save inválido: arquivo preservado");
        auto meta=get(b,8),size=get(b,12),crc=get(b,16);
        if(meta>4096||size>32*1024*1024||b.size()!=20ull+meta+size)throw std::runtime_error("Tamanho do save inválido");
        std::string metadata(b.begin()+20,b.begin()+20+meta);
        if(state){if(metadata!=gameIdentity+"\n"+coreIdentity+"\nstate")throw std::runtime_error("Save de outro conteúdo ou versão do núcleo");}
        else {
            auto first=metadata.find('\n'),last=metadata.rfind('\n');
            if(first==std::string::npos||last==first||metadata.find('\n',first+1)!=last||metadata.substr(0,first)!=gameIdentity||metadata.substr(last+1)!="sram"||family(metadata.substr(first+1,last-first-1))!=family(coreIdentity))
                throw std::runtime_error("Save de outro conteúdo ou família de núcleo");
        }
        std::vector<uint8_t> payload(b.begin()+20+meta,b.end());if(crc32(payload.data(),payload.size())!=crc)throw std::runtime_error("Checksum do save inválido");return payload;
    }
};
}
