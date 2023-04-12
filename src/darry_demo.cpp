#include <iostream>
#include <string>
#include "VideoChannel.h"
#include "DerryPlayer.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
#include <iostream>
#include <cstdlib>
#include <SDL.h>
extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libavcodec/avcodec.h>
}

#define PLAYER_WINDOWS_WIDTH 1280
#define PLAYER_WINDOWS_HEIGHT 720

using namespace std;
DerryPlayer *player = nullptr;

const char *data_source = "/home/marc/udp_ffmpeg_player/media/06.mp4";
// const char *data_source = "rtsp://192.168.10.26/live/1";
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // 静态初始化 锁

SDL_Window *window;
SDL_Renderer *render;
SDL_Texture *sdltext;

// 函数指针 实现  渲染工作
void renderFrame(uint8_t *src_data, int width, int height, int src_lineSize)
{
    pthread_mutex_lock(&mutex);

    // SDL_UpdateTexture(sdltext, NULL, src_data, width * 3);
    // cout << "src_lineSize " << src_lineSize << endl;
    SDL_UpdateTexture(sdltext, NULL, src_data, src_lineSize);
    SDL_RenderCopy(render, sdltext, NULL, NULL);
    // 拷贝纹理到渲染器
    SDL_RenderPresent(render);
    // 渲染
    // TODO 渲染工作
    // cout << "renderFrame running " << endl;
    // cout << "renderFrame: " << width << "x" << height << endl;
    pthread_mutex_unlock(&mutex);
}

int main()
{
    /*创建窗口*/
    window = SDL_CreateWindow("SDL_VIDEO", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, PLAYER_WINDOWS_WIDTH, PLAYER_WINDOWS_HEIGHT, SDL_WINDOW_SHOWN);
    /*创建渲染器*/
    render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    /*清空渲染器*/
    SDL_RenderClear(render);
    /*创建纹理*/
    // SDL_PIXELFORMAT_RGB24
    // SDL_PIXELFORMAT_RGBA8888
    // 这里居然还是涉及大小端? 为什么是ABGR8888
    sdltext = SDL_CreateTexture(render, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, PLAYER_WINDOWS_WIDTH, PLAYER_WINDOWS_HEIGHT);

    player = new DerryPlayer(data_source);
    cout << "1" << endl;
    player->setRenderCallback(renderFrame);
    cout << "2" << endl;
    player->prepare();
    cout << "3" << endl;
    sleep(1);
    player->start();
    cout << "4" << endl;
    getchar();
    return 0;
}