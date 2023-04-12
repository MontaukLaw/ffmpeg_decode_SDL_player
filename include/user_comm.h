#ifndef MY_APPLICATION_FFMPEG_PLAYER_USER_COMM_H
#define MY_APPLICATION_FFMPEG_PLAYER_USER_COMM_H

#include <string>
#include <unistd.h>
#include <cstring>
#include <cstdlib>

#include <stdio.h>
#include <iostream>

#include "io_data.h"
#include "udp_rev.h"
#include "video_encoder_core.h"


extern "C"
{
#include <libavcodec/packet.h>
#include <pthread.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avio.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

#include "video_decoder_core.h"

#define THREAD_MAIN 1
#define THREAD_CHILD 2

#define FFMPEG_CAN_NOT_OPEN_URL 1
#define FFMPEG_CAN_NOT_FIND_STREAMS 2
#define FFMPEG_FIND_DECODER_FAIL 3
#define FFMPEG_ALLOC_CODEC_CONTEXT_FAIL 4
#define FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL 6
#define FFMPEG_OPEN_DECODER_FAIL 7
#define FFMPEG_NO_MEDIA 8

#define PORT 54321
#define UDP_PACKET_LEN 1400
#define DATA_BUFFER_LEN 500000
#define INBUF_SIZE 128

#endif // MY_APPLICATION_FFMPEG_PLAYER_USER_COMM_H