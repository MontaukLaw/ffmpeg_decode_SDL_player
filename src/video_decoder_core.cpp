#include "user_comm.h"

static void usage(const char *progname)
{
    std::cout << "usage:" << std::string(progname) << " input yuv output file code_name" << std::endl;
}

// static AVCodec *codec = nullptr;
static AVCodecContext *codec_ctx = nullptr;
static AVCodecParserContext *parser_ctx = nullptr;

static AVFrame *frame = nullptr;
static AVPacket *pkt = nullptr;

int32_t init_video_decoder()
{
    const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec)
    {
        std::cerr << "can not find decoder" << std::endl;
        return -1;
    }

    parser_ctx = av_parser_init(codec->id);
    if (!parser_ctx)
    {
        std::cerr << "can not find parser" << std::endl;
        return -1;
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx)
    {
        std::cerr << "can not alloc context" << std::endl;
        return -1;
    }

    int32_t result = avcodec_open2(codec_ctx, codec, nullptr);
    if (result < 0)
    {
        std::cerr << "can not open codec" << std::endl;
        return -1;
    }

    frame = av_frame_alloc();
    if (!frame)
    {
        std::cerr << "can not alloc frame" << std::endl;
        return -1;
    }

    pkt = av_packet_alloc();
    if (!pkt)
    {
        std::cerr << "can not alloc packet" << std::endl;
        return -1;
    }

    return 0;
}

void destory_video_decoder()
{
    avcodec_free_context(&codec_ctx);
    av_parser_close(parser_ctx);
    av_frame_free(&frame);
    av_packet_free(&pkt);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        usage(argv[0]);
        return -1;
    }
    char *input_file_name = argv[1];
    char *output_file_name = argv[2];
    std::cout << "Input file:" << std::string(input_file_name) << std::endl;
    std::cout << "Output file:" << std::string(output_file_name) << std::endl;

    int32_t ret = open_input_output_file(input_file_name, output_file_name);
    if (ret < 0)
    {
        return ret;
    }

    ret = init_video_decoder();
    if (ret < 0)
    {
        return ret;
    }

    ret = decoding();
    if (ret < 0)
    {
        return ret;
    }

    destory_video_decoder();
    close_input_output_file();

    return 0;
}

static int32_t decode_packet(bool flushing)
{
    int32_t result = 0;
    if (flushing)
    {
        pkt->data = nullptr;
        pkt->size = 0;
    }

    std::cout << "decode packet size: " << pkt->size << std::endl;
    result = avcodec_send_packet(codec_ctx, pkt);
    if (result < 0)
    {
        std::cerr << "Error sending a packet for decoding" << std::endl;
        return -1;
    }

    while (result >= 0)
    {
        frame = av_frame_alloc();
        result = avcodec_receive_frame(codec_ctx, frame);
        if (result == AVERROR(EAGAIN))
        {
            continue;
        }
        else if (result != 0)
        {
            if (frame)
            {
                av_frame_free(&frame);
            }
            return -1;
        }

        if (flushing)
        {
            std::cout << "flushing: ";
        }
        std::cout << "write frame pic_num: " << frame->coded_picture_number << std::endl;
        write_frame_to_yuv(frame);
    }
    return 0;
}

int32_t decoding()
{
    uint8_t inbuf[INBUF_SIZE] = {0};
    int32_t result = 0;
    uint8_t *data = nullptr;

    int32_t data_size = 0;
    while (!end_of_file())
    {
        result = read_data_to_buf(inbuf, INBUF_SIZE, data_size);
        if (result < 0)
        {
            std::cerr << "read data to buf failed" << std::endl;
            return -1;
        }

        data = inbuf;
        while (data_size > 0)
        {
            // std::cout << "data_size:" << data_size << std::endl;
            result = av_parser_parse2(parser_ctx, codec_ctx,
                                      &pkt->data, &pkt->size,
                                      data, data_size, AV_NOPTS_VALUE,
                                      AV_NOPTS_VALUE, 0);
            if (result < 0)
            {
                std::cerr << "Error while parsing" << std::endl;
                return -1;
            }
            else
            {
                std::cout << "parse result:" << result << std::endl;
            }

            data += result;
            data_size -= result;

            if (pkt->size)
            {
                std::cout << "write packet size: " << pkt->size << std::endl;
                decode_packet(false);
            }
        }
    }
    decode_packet(true);
    return 0;
}
