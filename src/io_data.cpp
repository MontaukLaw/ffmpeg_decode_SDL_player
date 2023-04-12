#include "user_comm.h"

static FILE *input_file = nullptr;
static FILE *output_file = nullptr;

int32_t open_input_output_file(const char *input_name, const char *output_name)
{
    if (strlen(input_name) == 0 || strlen(output_name) == 0)
    {
        std::cerr << "Input or output file name is empty!" << std::endl;
        return -1;
    }

    close_input_output_file();

    // 以只读打开输入文件
    input_file = fopen(input_name, "rb");
    if (input_file == nullptr)
    {
        std::cerr << "Open input file failed!" << std::endl;
        return -1;
    }

    // 以可写打开输出文件
    output_file = fopen(output_name, "wb");
    if (output_file == nullptr)
    {
        std::cerr << "Open output file failed!" << std::endl;
        return -1;
    }
}

// 关闭输入输出文件
void close_input_output_file()
{
    if (input_file != nullptr)
    {
        fclose(input_file);
        input_file = nullptr;
    }

    if (output_file != nullptr)
    {
        fclose(output_file);
        output_file = nullptr;
    }
}

int32_t read_yuv_to_frame(AVFrame *frame)
{
    int32_t frame_width = frame->width;
    int32_t frame_height = frame->height;
    int32_t luma_stride = frame->linesize[0];
    int32_t chroma_stride = frame->linesize[1];
    int32_t frame_size = frame_width * frame_height * 3 / 2;
    int32_t read_size = 0;
    if (frame_width == luma_stride)
    {
        // 如果luma的stride和宽度相等，那么直接读取一帧数据
        read_size += fread(frame->data[0], 1,
                           frame_width * frame_height, input_file);
        read_size += fread(frame->data[1], 1,
                           frame_width * frame_height / 4, input_file);

        read_size += fread(frame->data[2], 1,
                           frame_width * frame_height / 4, input_file);
    }
    else
    {
        // 如果luma的stride和宽度不相等，那么需要一行一行的读取
        for (size_t i = 0; i < frame_height; i++)
        {
            read_size += fread(frame->data[0] + i * luma_stride, 1,
                               frame_width, input_file);
        }
        for (size_t i = 0; i < frame_height / 2; i++)
        {
            read_size += fread(frame->data[1] + i * chroma_stride, 1,
                               frame_width / 2, input_file);
        }
    }

    if (read_size != frame_size)
    {
        std::cerr << "Read yuv to frame failed!" << std::endl;
        return -1;
    }
}

void write_packet_to_file(AVPacket *packet)
{
    fwrite(packet->data, 1, packet->size, output_file);
}

int32_t end_of_file()
{
    return feof(input_file);
}

int32_t read_data_to_buf(uint8_t *buf, int32_t size, int32_t &out_size)
{
    int32_t read_size = fread(buf, 1, size, input_file);
    if (read_size == 0)
    {
        std::cerr << "Read data to buf failed!" << std::endl;
        return -1;
    }

    out_size = read_size;
    return 0;
}

int32_t write_frame_to_yuv(AVFrame *frame)
{
    uint8_t **pBuf = frame->data;
    int *pStride = frame->linesize;
    for (size_t i = 0; i < 3; i++)
    {
        int32_t width = (i = 0 ? frame->width : frame->width / 2);
        int32_t height = (i = 0 ? frame->height : frame->height / 2);

        for (size_t j = 0; j < height; j++)
        {
            fwrite(pBuf[i], 1, width, output_file);
            pBuf[i] += pStride[i];
        }
    }
}
