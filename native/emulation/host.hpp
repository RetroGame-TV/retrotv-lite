#pragma once
#include <libretro.h>
#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>
namespace rt {
enum class SessionState { Menu, Launching, Running, Paused, Failed };
class Host {
    void* library=nullptr;
    bool initialized=false,loaded=false;
    static Host* active;
    std::vector<uint8_t> rom;
    std::map<std::string,std::string> variables;
    std::string systemDir,saveDir,contentPath;
    struct Api {
        decltype(&retro_init) init=nullptr; decltype(&retro_deinit) deinit=nullptr;
        decltype(&retro_api_version) api_version=nullptr;
        decltype(&retro_get_system_info) get_system_info=nullptr;
        decltype(&retro_get_system_av_info) get_system_av_info=nullptr;
        decltype(&retro_set_environment) set_environment=nullptr;
        decltype(&retro_set_video_refresh) set_video_refresh=nullptr;
        decltype(&retro_set_audio_sample) set_audio_sample=nullptr;
        decltype(&retro_set_audio_sample_batch) set_audio_sample_batch=nullptr;
        decltype(&retro_set_input_poll) set_input_poll=nullptr;
        decltype(&retro_set_input_state) set_input_state=nullptr;
        decltype(&retro_set_controller_port_device) set_controller_port_device=nullptr;
        decltype(&retro_reset) reset=nullptr; decltype(&retro_run) run=nullptr;
        decltype(&retro_load_game) load_game=nullptr; decltype(&retro_unload_game) unload_game=nullptr;
        decltype(&retro_serialize_size) serialize_size=nullptr; decltype(&retro_serialize) serialize=nullptr;
        decltype(&retro_unserialize) unserialize=nullptr;
        decltype(&retro_get_memory_data) get_memory_data=nullptr; decltype(&retro_get_memory_size) get_memory_size=nullptr;
    } api;
    static bool environment(unsigned command,void* data);
    static void videoCallback(const void*,unsigned,unsigned,size_t);
    static size_t audioCallback(const int16_t*,size_t);
    static void sampleCallback(int16_t,int16_t);
    static void pollCallback() {}
    static int16_t inputCallback(unsigned,unsigned,unsigned,unsigned);
public:
    SessionState state=SessionState::Menu;
    retro_system_av_info av{};
    retro_pixel_format pixelFormat=RETRO_PIXEL_FORMAT_0RGB1555;
    retro_disk_control_callback discs{};
    std::array<std::array<int16_t,16>,2> buttons{};
    std::array<std::array<int16_t,4>,2> axes{};
    std::string error,coreIdentity;
    retro_vfs_interface* vfs=nullptr;
    std::function<void(const void*,unsigned,unsigned,size_t,retro_pixel_format)> video;
    std::function<void(const int16_t*,size_t)> audio;
    Host()=default; Host(const Host&)=delete;Host& operator=(const Host&)=delete;
    ~Host(){close();}
    bool open(const std::string& core,const std::string& path,std::vector<uint8_t> data,const std::string& bios,const std::string& saves);
    void run();void pause();void resume();void reset();void close();
    std::vector<uint8_t> serialize(); bool unserialize(const std::vector<uint8_t>&);
    std::vector<uint8_t> sram(); bool setSram(const std::vector<uint8_t>&);
    bool switchDisc();
};
}
