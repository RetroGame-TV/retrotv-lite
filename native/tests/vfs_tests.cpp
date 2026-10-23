#include "emulation/vfs.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <fcntl.h>
#ifdef _WIN32
#include <windows.h>
#include <winioctl.h>
#endif
using namespace rt;
int main(int argc,char** argv){
    if(argc!=2)return 1;auto path=std::string(argv[1])+"/large-disc-test.bin";std::filesystem::create_directories(argv[1]);
    constexpr int64_t offset=5LL*1024*1024*1024;
#ifdef _WIN32
    auto handle=CreateFileA(path.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(handle==INVALID_HANDLE_VALUE)return 2;
    DWORD returned=0;if(!DeviceIoControl(handle,FSCTL_SET_SPARSE,nullptr,0,nullptr,0,&returned,nullptr)){CloseHandle(handle);return 3;}
    LARGE_INTEGER target;target.QuadPart=offset;SetFilePointerEx(handle,target,nullptr,FILE_BEGIN);char marker='R';DWORD written;WriteFile(handle,&marker,1,&written,nullptr);CloseHandle(handle);
#else
    std::ofstream out(path,std::ios::binary);out.seekp(offset);out.put('R');out.close();
#endif
    Vfs v;v.openRead=[&](const std::string& p)->int{if(p!="/__retrotv_saf__/disc.chd")return -1;
#ifdef _WIN32
      return _open(path.c_str(),_O_RDONLY|_O_BINARY);
#else
      return ::open(path.c_str(),O_RDONLY);
#endif
    };
    auto& api=v.table;auto* file=api.open("/__retrotv_saf__/disc.chd",RETRO_VFS_FILE_ACCESS_READ,0);if(!file)return 4;
    bool ok=api.size(file)==offset+1&&api.seek(file,offset,RETRO_VFS_SEEK_POSITION_START)==offset;
    char value=0;ok=api.read(file,&value,1)==1&&value=='R'&&ok;ok=api.write(file,&value,1)==-1&&ok;api.close(file);
    ok=!api.open("../outside",RETRO_VFS_FILE_ACCESS_READ,0)&&ok;ok=!api.open("/__retrotv_saf__/disc.chd",RETRO_VFS_FILE_ACCESS_WRITE,0)&&ok;
    std::filesystem::remove(path);std::cout<<(ok?"5 GiB sparse random seek/read and read-only VFS passed\n":"VFS failed\n");return ok?0:5;
}
