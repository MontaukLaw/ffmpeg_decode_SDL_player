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

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libavcodec/avcodec.h>
}

using namespace std;
DerryPlayer *player = nullptr;

const char *data_source = "/home/marc/udp_ffmpeg_player/media/06.mp4";
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // 静态初始化 锁

// 函数指针 实现  渲染工作
void renderFrame(uint8_t *src_data, int width, int height, int src_lineSize)
{
    pthread_mutex_lock(&mutex);
    // TODO 渲染工作
    // cout << "renderFrame running " << endl;
    // cout << "renderFrame: " << width << "x" << height << endl;
    pthread_mutex_unlock(&mutex);
}

int main()
{
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