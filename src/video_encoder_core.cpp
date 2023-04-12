#include "user_comm.h"

// static AVCodec *codecer = nullptr;
static AVCodecContext *codec_context = nullptr;
static AVFrame *frame = nullptr;
static AVPacket *packet = nullptr;

int32_t init_video_encoder(const char *codec_name)
{
    if (strlen(codec_name) == 0)
    {
        std::cerr << "Codec name is empty!" << std::endl;
        return -1;
    }

    // 查找编码器
    // h264_v4l2m2m
    // const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    const AVCodec *codec = avcodec_find_encoder_by_name(codec_name);
    if (codec == nullptr)
    {
        std::cerr << "Can not find encoder by name: " << std::string(codec_name) << std::endl;
        return -1;
    }

    // std::cout << "codec name:" << codec->name << std::endl;

    // 创建编码器上下文
    codec_context = avcodec_alloc_context3(codec);
    if (codec_context == nullptr)
    {
        std::cerr << "Can not alloc codec context!" << std::endl;
        return -1;
    }

    // 设置编码器参数
    codec_context->profile = FF_PROFILE_H264_HIGH;
    codec_context->bit_rate = 20000;
    codec_context->width = 1920;
    codec_context->height = 1080;
    codec_context->gop_size = 10;
    codec_context->time_base = (AVRational){1, 25};
    codec_context->framerate = (AVRational){25, 1};
    codec_context->max_b_frames = 3;
    codec_context->pix_fmt = AV_PIX_FMT_YUV420P;

    if (codec->id == AV_CODEC_ID_H264)
    {
        std::cout << "yes its h264" << std::endl;
        av_opt_set(codec_context->priv_data, "preset", "slow", 0);
    }

    // 打开编码器
    int32_t ret = avcodec_open2(codec_context, codec, nullptr);
    if (ret < 0)
    {
        std::cerr << "Can not open codec!" << std::endl;
        return -1;
    }

    // 创建数据包
    packet = av_packet_alloc();
    if (packet == nullptr)
    {
        std::cerr << "Can not alloc packet!" << std::endl;
        return -1;
    }

    // 创建帧
    frame = av_frame_alloc();
    if (frame == nullptr)
    {
        std::cerr << "Can not alloc frame!" << std::endl;
        return -1;
    }

    frame->format = codec_context->pix_fmt;
    frame->width = codec_context->width;
    frame->height = codec_context->height;

    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0)
    {
        std::cerr << "Can not alloc frame buffer!" << std::endl;
        return -1;
    }

    std::cout << "Init video encoder success!" << std::endl;
    return 0;
}

void destory_void_encoder()
{
    avcodec_free_context(&codec_context);
}

static int32_t encode_frame(bool flushing)
{
    int32_t result = 0;
    if (!flushing)
    {
        std::cout << "Send frame to encoder!" << frame->pts << std::endl;
    }

    // 发送帧到编码器
    result = avcodec_send_frame(codec_context, flushing ? nullptr : frame);
    if (result < 0)
    {
        std::cerr << "Can not send frame to encoder!" << std::endl;
        return -1;
    }

    while (result >= 0)
    {
        // 从编码器获取编码后的数据包
        result = avcodec_receive_packet(codec_context, packet);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF)
        {
            return 0;
        }
        else if (result < 0)
        {
            std::cerr << "Can not receive packet from encoder!" << std::endl;
            return -1;
        }

        if (flushing)
        {
            std::cout << "Flush packet!" << packet->pts << std::endl;
            write_packet_to_file(packet);
        }
    }

    return 0;
}

int32_t encoding(int32_t frame_cnt)
{
    int result = 0;
    for (size_t i = 0; i < frame_cnt; i++)
    {
        result = av_frame_make_writable(frame);
        if (result < 0)
        {
            std::cerr << "Can not make frame writable!" << std::endl;
            return -1;
        }

        // 从文件中读取YUV数据
        result = read_yuv_to_frame(frame);
        if (result < 0)
        {
            std::cerr << "Can not read yuv to frame!" << std::endl;
            return -1;
        }
        frame->pts = i;

        // 编码帧
        result = encode_frame(false);
        if (result < 0)
        {
            std::cerr << "Can not encode frame!" << std::endl;
            return -1;
        }
    }

    // 编码帧尾
    result = encode_frame(true);
    if (result < 0)
    {
        std::cerr << "Can not encode frame!" << std::endl;
        return -1;
    }

    return 0;
}

void destory_video_encoder()
{
    avcodec_free_context(&codec_context);
    av_frame_free(&frame);
    av_packet_free(&packet);
}
