#include "../include/user_comm.h"

static void usage(const char *progname)
{
    std::cout << "usage:" << std::string(progname) << " input yuv output file code_name" << std::endl;
}

int main_bak(int argc, char **argv)
{
    if (argc < 4)
    {
        usage(argv[0]);
        return -1;
    }

    // 第一个参数是播放的文件名字
    char *input_file_name = argv[1];

    // 第二个参数是输出的文件名字
    char *output_file_name = argv[2];

    // 第三个参数是解码器的名字
    char *codec_name = argv[3];

    std::cout << "Input file:" << std::string(input_file_name) << std::endl;
    std::cout << "Output file:" << std::string(output_file_name) << std::endl;
    std::cout << "Codec name:" << std::string(codec_name) << std::endl;

    // 打开输入输出文件
    int32_t ret = open_input_output_file(input_file_name, output_file_name);
    if (ret < 0)
    {
        return ret;
    }

    ret = init_video_encoder(codec_name);
    if (ret < 0)
    {
        std::cerr << "init video encoder failed" << std::endl;
        return ret;
    }

    // 编码50个帧
    ret = encoding(50);
    if (ret < 0)
    {
        std::cerr << "encoding failed" << std::endl;
        goto failed;
    }

failed:
    destory_video_encoder();
    close_input_output_file();
    return 0;
}

int main(){

}
