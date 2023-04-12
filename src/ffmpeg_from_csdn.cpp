extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavutil/frame.h>
#include <libavcodec/packet.h>
}
#include <stdio.h>
#include <linux/fb.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <poll.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <wchar.h>
#include <pthread.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>

#define FILE_NAME "/home/marc/udp_ffmpeg_player/media/06.mp4"
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

uint8_t *out_buffer;
#define MAX_AUDIO_FRAME_SIZE 1024 * 100
static uint8_t *audio_chunk;
static unsigned int audio_len = 0;
static unsigned char *audio_pos;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // 互斥锁

// 保存音频数据链表
struct AUDIO_DATA
{
    unsigned char *audio_buffer;
    int audio_size;
    struct AUDIO_DATA *next;
};
// 定义一个链表头
struct AUDIO_DATA *list_head = NULL;
struct AUDIO_DATA *List_CreateHead(struct AUDIO_DATA *head);                             // 创建链表头
void List_AddNode(struct AUDIO_DATA *head, unsigned char *audio_buffer, int audio_size); // 添加节点
void List_DelNode(struct AUDIO_DATA *head, unsigned char *audio_buffer);                 // 删除节点
int List_GetNodeCnt(struct AUDIO_DATA *head);                                            // 遍历
int List_GetNode(struct AUDIO_DATA *head, char *audio_buff, int *audio_size);

int file_stat = 1;

void AudioCallback(void *userdata, Uint8 *stream, int len)
{
    SDL_memset(stream, 0, len);
    if (audio_len <= 0)
    {
        return;
    }
    len = (len > audio_len ? audio_len : len);
    SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
    audio_pos += len;
    audio_len -= len;
    // printf("len=%d\n",len);
}

void *Audio_decode(void *arg)
{
    int res;
    int audio_size;
    char audio_buff[4096 * 3];
    while (1)
    {
        res = List_GetNode(list_head, audio_buff, &audio_size);
        if (res == 0)
        {
            audio_chunk = (uint8_t *)audio_buff; // 指向音频数据 (PCM data)
            while (audio_len > 0)
            {
            }                                        // 等待数据处理完
            audio_len = audio_size;                  // 音频长度
            audio_pos = (unsigned char *)audio_buff; // 当前播放位置
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("格式:./app 文件名\n");
        return 0;
    }
    // 获取媒体文件名
    char *file_name = argv[1];
    printf("name:%s\n", file_name);

    /*SDL初始化*/
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);

    /*获取ffmpeg配置信息*/
    printf("pth:%s\n", avcodec_configuration());

    /* 初始化所有组件 旧版本的要求 */
    // av_register_all();

    /*打开文件*/
    // 解码器上下文
    AVCodecContext *pCodecCtx;
    // 音视频封装格式结构体信息
    AVFormatContext *ps = NULL;

    // 打开文件, avformat可以直接打开rtsp,rtmp,http等协议
    // avformat_open_input
    int res = avformat_open_input(&ps, file_name, NULL, NULL);
    if (res != 0)
    {
        printf("open err: %d\n", res);
        return 0;
    }
    /*寻找解码信息*/
    avformat_find_stream_info(ps, NULL);
    int64_t time = ps->duration;
    printf("time:%ld s\n", time / 1000000);
    /*打印有关输入或输出格式的详细信息*/
    av_dump_format(ps, 0, file_name, 0);
    /*寻找视频流信息*/
    int videostream = -1;
    int audiostream = -1;
    AVCodec *vcodec;
    videostream = av_find_best_stream(ps, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    printf("video=%d\n", videostream);
    /*寻找音频流信息*/
    audiostream = av_find_best_stream(ps, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    printf("audio=%d\n", audiostream);
    AVStream *stream;
    int frame_rate;
    if (videostream != -1) // 判断是否找到视频流数据
    {
        /*寻找视频解码器*/
        AVStream *stream = ps->streams[videostream];
        vcodec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!vcodec)
        {
            printf("未找到视频解码器\n");
            return -1;
        } /*申请AVCodecContext空间。需要传递一个编码器，也可以不传，但不会包含编码器。*/
        res = avcodec_open2(stream->codec, vcodec, NULL);
        if (res)
        {
            printf("打开解码器失败\n");
            return -1;
        }
        frame_rate = stream->avg_frame_rate.num / stream->avg_frame_rate.den; // 每秒多少帧
        printf("fps=%d\n", frame_rate);
        printf("视频流ID=%#x\n", vcodec->id); // 音频流
    }
    /*音频流数据处理*/
    AVCodec *audcodec;
    AVStream *audstream;
    SwrContext *swrCtx;          // 保存重采样数据，即解码的信息
    uint64_t out_channel_layout; // 声道
    int out_sample_fmt;          // 采样格式
    int out_sample_rate;         // 采样率
    int out_nb_samples;          // 样本数量
    int out_channels;            // 通道数量
    uint64_t in_channel_layout;  // 输入音频声道
    SDL_AudioSpec desired;       // SDL音频格式信息
    AVFrame *audioframe;         // 保存音频数据
    int out_buffer_size;         // 音频缓冲区大小
    if (audiostream >= 0)        // 判断是否有音频流
    {
        /*寻找音频解码器*/
        audstream = ps->streams[audiostream];
        audcodec = avcodec_find_decoder(audstream->codecpar->codec_id);
        if (!audcodec)
        {
            printf("audcodec failed\n");
            return -1;
        }
        /*申请音频AVCodecContext空间。需要传递一个编码器，也可以不传，但不会包含编码器。*/
        pCodecCtx = audstream->codec; // 解码器上下文
        res = avcodec_open2(audstream->codec, audcodec, NULL);
        if (res)
        {
            printf("未找到音频解码器\n");
            return -1;
        }
        printf("音频流ID=%#x\n", audcodec->id); // 音频流
        printf("配置音频参数\n");
        // 输出音频参数
        out_channel_layout = AV_CH_LAYOUT_STEREO; // 声道格式
        out_sample_fmt = AV_SAMPLE_FMT_S16;       // AV_SAMPLE_FMT_S32;//;//采样格式
        printf("pCodecCtx->sample_rate=%d\n", pCodecCtx->sample_rate);
        out_sample_rate = pCodecCtx->sample_rate; // 采样率，多为44100
        /*样本数量*/
        printf("frame_size=%d\n", pCodecCtx->frame_size);
        if (pCodecCtx->frame_size > 0)
            out_nb_samples = pCodecCtx->frame_size;
        else if (audcodec->id == AV_CODEC_ID_AAC)
            out_nb_samples = 1024; /*样本数量nb_samples: AAC-1024 MP3-1152  格式大小 */
        else if (audcodec->id == AV_CODEC_ID_MP3)
            out_nb_samples = 1152;
        else
            out_nb_samples = 1024;
        out_channels = av_get_channel_layout_nb_channels(out_channel_layout);                                                // 通道个数
        out_buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, (AVSampleFormat)out_sample_fmt, 1); // 获取缓冲区大小
        out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE);
        memset(out_buffer, 0, out_buffer_size);
        printf("声道格式:%ld\n", out_channel_layout);
        printf("采样格式:%d\n", out_sample_fmt);
        printf("样本数量:%d\n", out_nb_samples);
        printf("采样率:%d\n", out_sample_rate);
        printf("通道个数:%d\n", out_channels);
        printf("缓冲区大小:%d\n", out_buffer_size);
        // 输入音频参数
        in_channel_layout = av_get_default_channel_layout(pCodecCtx->channels); // 输入声道格式
        swrCtx = swr_alloc();
        /*对解码数据进行重采样*/
        swrCtx = swr_alloc_set_opts(swrCtx, out_channel_layout, (AVSampleFormat)out_sample_fmt, out_sample_rate, /*输入音频格式*/
                                    in_channel_layout, pCodecCtx->sample_fmt, pCodecCtx->sample_rate,            /*输出音频格式*/
                                    0, NULL);
        swr_init(swrCtx); // 初始化swrCtx

        printf("输入音频格式:%d\n", in_channel_layout);
        printf("输入采样格式:%d\n", pCodecCtx->sample_fmt);
        printf("输入采样率:%d\n", pCodecCtx->sample_rate);

        /*设置音频数据格式*/
        desired.freq = out_sample_rate;   /*采样率*/
        desired.format = AUDIO_S16SYS;    /*无符号16位*/
        desired.channels = out_channels;  /*声道*/
        desired.samples = out_nb_samples; /*样本数1024*/
        desired.silence = 0;              /*静音值*/
        desired.callback = AudioCallback;
        SDL_OpenAudio(&desired, NULL);
        SDL_PauseAudio(0); /*开始播放音频，1为播放静音值*/
        // 分配内存
        audioframe = av_frame_alloc(); /*分配音频帧*/
        printf("音频数据初始化完成");
    }
    // 视频解码
    AVFrame *frame = av_frame_alloc();    /*分配视频帧*/
    AVFrame *frameYUV = av_frame_alloc(); /*申请yuv空间*/
    /*分配空间，进行图像转换*/
    int width = ps->streams[videostream]->codecpar->width;
    int height = ps->streams[videostream]->codecpar->height;
    int fmt = ps->streams[videostream]->codecpar->format; /*流格式*/
    printf("fmt=%d\n", fmt);
    int size = avpicture_get_size(AV_PIX_FMT_RGB24, width, height);
    unsigned char *buff = NULL;
    printf("w=%d,h=%d,size=%d\n", width, height, size);
    buff = (unsigned char *)av_malloc(size);
    /*计算一帧空间大小*/
    avpicture_fill((AVPicture *)frameYUV, buff, AV_PIX_FMT_RGB24, width, height);
    /*转换上下文*/

    struct SwsContext *swsctx = sws_getContext(width, height, (AVPixelFormat)fmt, width, height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
    /*读帧*/
    int go = 0;
    int go_audio;
    list_head = List_CreateHead(list_head); // 创建链表头
    /*创建音频处理线程*/
    pthread_t pthid;
    pthread_create(&pthid, NULL, Audio_decode, (void *)ps);
    pthread_detach(pthid); // 设置为分离属性
    /*创建窗口*/
    SDL_Window *window = SDL_CreateWindow("SDL_VIDEO", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
    /*创建渲染器*/
    SDL_Renderer *render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    /*清空渲染器*/
    SDL_RenderClear(render);
    /*创建纹理*/
    SDL_Texture *sdltext = SDL_CreateTexture(render, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, width, height);
    bool quit = true;
    SDL_Event event;
    printf("read fream buff\n");
    // 初始化转码器
    AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket)); /*分配包*/
    av_init_packet(packet);                                     // 初始化
    int i = 0;
    int index = 0;
    long video_pts_time = 0;
    long audio_pts_time = 0;               // 音频解码时间
    time = (1000000 / frame_rate - 10000); // 时间
    printf("time=%d\n", time);
    while ((av_read_frame(ps, packet) >= 0) && (quit))
    {
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT)
        {
            quit = false;
            continue;
        }
        if (packet->stream_index == videostream) /*判断是否为视频*/
        {
            res = avcodec_send_packet(ps->streams[videostream]->codec, packet);
            if (res)
            {
                av_packet_unref(packet); // 释放这个pkt
                continue;
            }

            res = avcodec_receive_frame(ps->streams[videostream]->codec, frame);
            if (res)
            {
                av_packet_unref(packet); // 释放这个pkt
                continue;
            }

            sws_scale(swsctx, frame->data, frame->linesize, 0, height, frameYUV->data, frameYUV->linesize);

            // sws_scale(swsctx, (const uint8_t **)frame->data, frame->linesize, 0, height, (const uint8_t **)frameYUV->data, frameYUV->linesize);
            video_pts_time = packet->pts;
            // printf("视频=%ld\n",video_pts_time);
            SDL_UpdateTexture(sdltext, NULL, buff, width * 3);
            SDL_RenderCopy(render, sdltext, NULL, NULL); // 拷贝纹理到渲染器
            SDL_RenderPresent(render);                   // 渲染
            usleep(time);
        }
        if (packet->stream_index == audiostream) // 如果为音频标志
        {
            if (audiostream < 0)
                continue;
            res = avcodec_send_packet(pCodecCtx, packet);
            if (res)
            {
                printf("avcodec_send_packet failed,res=%d\n", res);
                av_packet_unref(packet); // 释放这个pkt
                continue;
            }
            res = avcodec_receive_frame(pCodecCtx, audioframe);
            if (res)
            {
                printf("avcodec_receive_frame failed,res=%d\n", res);
                av_packet_unref(packet); // 释放这个pkt
                continue;
            }
            // 数据格式转换
            res = swr_convert(swrCtx, &out_buffer, out_buffer_size,                      /*重采样之后的数据*/
                              (const uint8_t **)audioframe->data, audioframe->nb_samples /*重采样之前数据*/
            );
            audio_pts_time = packet->pts;
            // printf("音频：%ld\n",audio_pts_time);
            if (res > 0)
            {
                // audio_chunk =out_buffer; //指向音频数据 (PCM data)
                // while(audio_len>0){}//等待数据处理完
                // audio_len =audioframe->nb_samples;//out_buffer_size;//音频长度
                // audio_pos =out_buffer;//当前播放位置
                List_AddNode(list_head, out_buffer, out_buffer_size); // 添加节点
            }
        }
        // 释放数据包
        av_packet_unref(packet);
    }
    sws_freeContext(swsctx);
    av_frame_free(&frame);
    av_frame_free(&frameYUV);
    avformat_free_context(ps);
    return 0;
}
/*创建链表头*/
struct AUDIO_DATA *List_CreateHead(struct AUDIO_DATA *head)
{
    if (head == NULL)
    {
        head = (AUDIO_DATA *)malloc(sizeof(struct AUDIO_DATA));
        head->next = NULL;
    }
    return head;
}

/*添加节点*/
void List_AddNode(struct AUDIO_DATA *head, unsigned char *audio_buffer, int audio_size)
{
    struct AUDIO_DATA *tmp = head;
    struct AUDIO_DATA *new_node;
    pthread_mutex_lock(&mutex);
    /*找到链表尾部*/
    while (tmp->next)
    {
        tmp = tmp->next;
    }
    /*插入新的节点*/
    new_node = (AUDIO_DATA *)malloc(sizeof(struct AUDIO_DATA));
    new_node->audio_size = audio_size;
    new_node->audio_buffer = (unsigned char *)malloc(audio_size); // 分配保存音频数据大小空间
    memcpy(new_node->audio_buffer, audio_buffer, audio_size);
    new_node->next = NULL;
    /*将新节点接入到链表*/
    tmp->next = new_node;
    pthread_mutex_unlock(&mutex);
}
/*
函数功能:删除节点
*/
void List_DelNode(struct AUDIO_DATA *head, unsigned char *audio_buffer)
{
    struct AUDIO_DATA *tmp = head;
    struct AUDIO_DATA *p;
    /*找到链表中要删除的节点*/
    pthread_mutex_lock(&mutex);
    while (tmp->next)
    {
        p = tmp;
        tmp = tmp->next;
        if (tmp->audio_buffer == audio_buffer)
        {
            p->next = tmp->next;
            free(tmp->audio_buffer);
            free(tmp);
            break;
        }
    }
    pthread_mutex_unlock(&mutex);
}
/*
函数功能:遍历链表，得到节点总数量
*/
int List_GetNodeCnt(struct AUDIO_DATA *head)
{
    int cnt = 0;
    struct AUDIO_DATA *tmp = head;
    pthread_mutex_lock(&mutex);
    while (tmp->next)
    {
        tmp = tmp->next;
        cnt++;
    }
    pthread_mutex_unlock(&mutex);
    return cnt;
}
/*
从链表头取数据
*/
int List_GetNode(struct AUDIO_DATA *head, char *audio_buff, int *audio_size)
{
    struct AUDIO_DATA *tmp = head;
    struct AUDIO_DATA *ptemp = head;
    pthread_mutex_lock(&mutex);
    while (tmp->next != NULL)
    {
        ptemp = tmp;
        tmp = tmp->next;

        if (tmp != NULL)
        {
            *audio_size = tmp->audio_size;
            memcpy(audio_buff, tmp->audio_buffer, tmp->audio_size);
            ptemp->next = tmp->next;
            free(tmp->audio_buffer);
            free(tmp);
            pthread_mutex_unlock(&mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&mutex);
    return -1;
}
