#ifndef __IO_DATA_H_
#define __IO_DATA_H_

extern "C"
{
#include <libavcodec/avcodec.h>
}

#include <stdint.h>

void close_input_output_file();

int32_t open_input_output_file(const char *input_name, const char *output_name);

int32_t read_yuv_to_frame(AVFrame *frame);

void write_packet_to_file(AVPacket *packet);

int32_t read_data_to_buf(uint8_t *buf, int32_t size, int32_t &out_size);

int32_t write_frame_to_yuv(AVFrame *frame);

int32_t end_of_file();

#endif