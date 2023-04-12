#ifndef DERRYPLAYER_AUDIOCHANNEL_H
#define DERRYPLAYER_AUDIOCHANNEL_H

#include "BaseChannel.h"
extern "C" {
#include <libswresample/swresample.h> // 对音频数据进行转换（重采样）
}

class AudioChannel : public BaseChannel {

private:
    pthread_t pid_audio_decode;
    pthread_t pid_audio_play;

public:
    int out_channels;
    int out_sample_size;
    int out_sample_rate;
    int out_buffers_size;
    uint8_t *out_buffers = 0;
    SwrContext *swr_ctx = 0;

public:

    double audio_time; // TODO 音视频同步 1.1

public:
    AudioChannel(int stream_index, AVCodecContext *codecContext, AVRational rational);

    ~AudioChannel();

    void stop();

    void start();


    void audio_decode();

    void audio_play();

    int getPCM();
};

#endif //DERRYPLAYER_AUDIOCHANNEL_H
