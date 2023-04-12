#ifndef __VIDEO_ENCODER_CORE_H_
#define __VIDEO_ENCODER_CORE_H_

#include <stdint.h>

// 初始化视频编码器
int32_t init_video_encoder(const char *codec_name);

void destory_video_encoder();

int32_t encoding(int32_t frame_cnt);

#endif
