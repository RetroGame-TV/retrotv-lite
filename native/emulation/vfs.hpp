#pragma once
#include <libretro.h>
#include <cstdio>
#include <functional>
#include <string>
#include <limits>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
struct retro_vfs_file_handle { FILE* file; std::string path; };
namespace rt {
// One VFS per active session. Providers return seekable read-only descriptors.
class Vfs {
    static inline Vfs* active=nullptr;
    static retro_vfs_file_handle* open(const char* p,unsigned mode,unsigned){
        if(!active||!p||mode!=RETRO_VFS_FILE_ACCESS_READ)return nullptr;
        int fd=active->openRead(p);if(fd<0)return nullptr;
#ifdef _WIN32
        FILE* f=_fdopen(fd,"rb");if(!f){_close(fd);return nullptr;}
#else
        FILE* f=fdopen(fd,"rb");if(!f){::close(fd);return nullptr;}
#endif
        return new retro_vfs_file_handle{f,p};
    }
    static int64_t seek(retro_vfs_file_handle* h,int64_t offset,int origin){if(!h)return -1;int whence=origin==RETRO_VFS_SEEK_POSITION_START?SEEK_SET:origin==RETRO_VFS_SEEK_POSITION_CURRENT?SEEK_CUR:SEEK_END;
#ifdef _WIN32
        if(_fseeki64(h->file,offset,whence)!=0)return -1;return _ftelli64(h->file);
#else
        if(fseeko(h->file,offset,whence)!=0){if(active)active->ioFailed=true;return -1;}return ftello(h->file);
#endif
    }
    static int64_t tell(retro_vfs_file_handle* h){return seek(h,0,RETRO_VFS_SEEK_POSITION_CURRENT);}
    static int64_t size(retro_vfs_file_handle* h){auto at=tell(h);if(at<0)return -1;auto n=seek(h,0,RETRO_VFS_SEEK_POSITION_END);seek(h,at,RETRO_VFS_SEEK_POSITION_START);return n;}
    static int stat(const char* path,int32_t* n){auto* h=open(path,RETRO_VFS_FILE_ACCESS_READ,0);if(!h)return 0;auto count=size(h);close(h);if(count<0)return 0;if(n)*n=int32_t(std::min<int64_t>(count,INT32_MAX));return RETRO_VFS_STAT_IS_VALID;}
    static int close(retro_vfs_file_handle* h){if(!h)return -1;auto ret=std::fclose(h->file);delete h;return ret;}
public:
    std::function<int(const std::string&)> openRead;
    bool ioFailed=false;
    retro_vfs_interface table{};
    Vfs(){
        active=this;table.get_path=[](retro_vfs_file_handle* h){return h?h->path.c_str():nullptr;};
        table.open=open;table.close=close;table.size=size;table.tell=tell;table.seek=seek;
        table.read=[](retro_vfs_file_handle* h,void* dst,uint64_t n)->int64_t{if(!h||n>SIZE_MAX)return -1;auto got=std::fread(dst,1,size_t(n),h->file);if(got==0&&std::ferror(h->file)){if(active)active->ioFailed=true;return -1;}return int64_t(got);};
        table.write=[](retro_vfs_file_handle*,const void*,uint64_t)->int64_t{return -1;};table.flush=[](retro_vfs_file_handle*){return 0;};
        table.remove=[](const char*){return -1;};table.rename=[](const char*,const char*){return -1;};table.truncate=[](retro_vfs_file_handle*,int64_t)->int64_t{return -1;};
        table.stat=stat;table.mkdir=[](const char*){return -1;};table.opendir=[](const char*,bool)->retro_vfs_dir_handle*{return nullptr;};table.readdir=[](retro_vfs_dir_handle*){return false;};table.dirent_get_name=[](retro_vfs_dir_handle*)->const char*{return nullptr;};table.dirent_is_dir=[](retro_vfs_dir_handle*){return false;};table.closedir=[](retro_vfs_dir_handle*){return -1;};
    }
    ~Vfs(){if(active==this)active=nullptr;}
};
}
