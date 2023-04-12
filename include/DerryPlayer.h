#ifndef DERRYPLAYER_DERRYPLAYER_H
#define DERRYPLAYER_DERRYPLAYER_H

#include <cstring>
#include <pthread.h>
#include "AudioChannel.h"
#include "VideoChannel.h" // 可以直接访问函数指针
#include "util.h"
#include <iostream>
#include <string>
#include "VideoChannel.h"

// ffmpeg是纯c写的，必须采用c的编译方式，否则奔溃
extern "C"
{
#include <libavutil/time.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
};

class DerryPlayer
{
private:
    char *data_source = 0; // 指针 请赋初始值
    pthread_t pid_prepare;
    pthread_t pid_start;
    AVFormatContext *formatContext = nullptr;
    VideoChannel *video_channel = nullptr;
    
    bool isPlaying; // 是否播放
    RenderCallback renderCallback;
    int duration; // TODO 第七节课增加 总时长

    pthread_mutex_t seek_mutex; // TODO 第七节课增加 3.1
    pthread_t pid_stop;

public:
    DerryPlayer(const char *data_source);

    ~DerryPlayer();

    void prepare();
    void prepare_();

    void start();
    void start_();

    void setRenderCallback(RenderCallback renderCallback);

    int getDuration();

    void seek(int play_value);

    void stop();

    void stop_(DerryPlayer *);
};

#endif // DERRYPLAYER_DERRYPLAYER_H