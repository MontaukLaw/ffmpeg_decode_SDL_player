#include "../include/DerryPlayer.h"

DerryPlayer::DerryPlayer(const char *data_source)
{
    // this->data_source = data_source;
    // 如果被释放，会造成悬空指针

    // 深拷贝
    // this->data_source = new char[strlen(data_source)];
    // Java: demo.mp4
    // C层：demo.mp4\0  C层会自动 + \0,  strlen不计算\0的长度，所以我们需要手动加 \0

    this->data_source = new char[strlen(data_source) + 1];
    strcpy(this->data_source, data_source); // 把源 Copy给成员

    cout << "media file name" << this->data_source << endl;

    pthread_mutex_init(&seek_mutex, nullptr); // TODO 第七节课增加 3.1
}

DerryPlayer::~DerryPlayer()
{
    if (data_source)
    {
        delete data_source;
        data_source = nullptr;
    }

    pthread_mutex_destroy(&seek_mutex); // TODO 第七节课增加 3.1
}

void *task_prepare(void *args)
{ // 此函数和DerryPlayer这个对象没有关系，你没法拿DerryPlayer的私有成员

    // avformat_open_input(0, this->data_source)

    auto *player = static_cast<DerryPlayer *>(args);
    player->prepare_();
    return nullptr; // 必须返回，坑，错误很难找
}

void DerryPlayer::prepare_()
{ // 此函数 是 子线程

    // 字典（键值对）
    AVDictionary *dictionary = nullptr;
    // 设置超时（5秒）
    // av_dict_set(&dictionary, "timeout", "5000000", 0); // 单位微妙
    // avformat_open_input
    int r = avformat_open_input(&formatContext, data_source, nullptr, nullptr);
    // int res = avformat_open_input(&ps, file_name, NULL, NULL);
    av_dict_free(&dictionary);

    if (r)
    {
        cout << "打开文件失败" << endl;
        return;
    }
    else
    {
        cout << "打开文件成功" << endl;
    }

    /**
     * 第二步：查找媒体中的音视频流的信息
     */
    r = avformat_find_stream_info(formatContext, nullptr);
    if (r < 0)
    {
        // 播放器收尾 1
        avformat_close_input(&formatContext);
        return;
    }
    else
    {
        cout << "查找媒体中的音视频流的信息成功" << endl;
        cout << "视频时长：" << formatContext->duration / 1000000 << "秒" << endl;
    }

    AVCodecContext *codecContext = nullptr;

    cout << "nb_streams:" << formatContext->nb_streams << endl;

    // 第三步：根据流信息，流的个数，用循环来找
    // for (int stream_index = 0; stream_index < formatContext->nb_streams; ++stream_index)
    // {
    int stream_index = 0;
    cout << "stream_index:" << stream_index << endl;

    // 第四步：获取媒体流（视频，音频）
    AVStream *stream = formatContext->streams[stream_index];

    if (stream == nullptr)
    {
        cout << "获取媒体流失败" << endl;
        return;
    }
    else
    {
        cout << "获取媒体流成功" << endl;
    }

    // 第五步：从上面的流中 获取 编码解码的【参数】
    AVCodecParameters *parameters = stream->codecpar;
    cout << "编码id: " << parameters->codec_id << endl;

    /**
     *  第六步：（根据上面的【参数】）获取编解码器
     */
    const AVCodec *codec = avcodec_find_decoder(parameters->codec_id);
    if (!codec)
    {
        // 播放器收尾 1
        avformat_close_input(&formatContext);
    }
    else
    {
        cout << "获取编解码器成功" << endl;
    }

    /**
     *  第七步：编解码器 上下文 （这个才是真正干活的）
     */
    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext)
    {

        // 播放器收尾 1
        avcodec_free_context(&codecContext); // 释放此上下文 avcodec 他会考虑到，你不用管*codec
        avformat_close_input(&formatContext);

        return;
    }
    else
    {
        cout << "编解码器 上下文 获取成功" << endl;
    }

    /**
     *  第八步：他目前是一张白纸（parameters copy codecContext）
     */
    r = avcodec_parameters_to_context(codecContext, parameters);
    if (r < 0)
    {
        // 播放器收尾 1
        avcodec_free_context(&codecContext); // 释放此上下文 avcodec 他会考虑到，你不用管*codec
        avformat_close_input(&formatContext);
        return;
    }
    else
    {
        cout << "parameters copy codecContext 成功" << endl;
    }

    // 设置解码格式
    // codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    // cout << "获取到的格式 " << codecContext->pix_fmt << endl;

    /**
     *  第九步：打开解码器
     */
    r = avcodec_open2(codecContext, codec, nullptr);
    if (r)
    {
        // 非0就是true
        avcodec_free_context(&codecContext); // 释放此上下文 avcodec 他会考虑到，你不用管*codec
        avformat_close_input(&formatContext);
        return;
    }
    else
    {
        cout << "打开解码器成功" << endl;
    }

    AVRational time_base = stream->time_base;

    /**
     * TODO 第十步：从编解码器参数中，获取流的类型 codec_type  ===  音频 视频
     */
    if (parameters->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO)
    {

        // // 虽然是视频类型，但是只有一帧封面
        // if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC)
        // {
        //     continue;
        // }

        // TODO 音视频同步 2.2 （视频独有的 fps值）
        AVRational fps_rational = stream->avg_frame_rate;
        int fps = av_q2d(fps_rational);

        // 是视频
        video_channel = new VideoChannel(stream_index, codecContext, time_base, fps);

        // 设置回调
        video_channel->setRenderCallback(renderCallback);
    }
    // }

    /**
     * TODO 第十一步: 如果流中 没有音频 也没有 视频 【健壮性校验】
     */
    if (!video_channel)
    {
        // TODO 播放器收尾 1
        if (codecContext)
        {
            avcodec_free_context(&codecContext); // 释放此上下文 avcodec 他会考虑到，你不用管*codec
        }
        avformat_close_input(&formatContext);
        return;
    }
}

void DerryPlayer::prepare()
{
    // 问题：当前的prepare函数，是子线程 还是 主线程 ？
    // 答：此函数是被MainActivity的onResume调用下来的（主线程）

    // 解封装 FFmpeg来解析  data_source 可以直接解析吗？
    // 答：data_source == 文件io流，  直播网络rtmp， 所以按道理来说，会耗时，所以必须使用子线程

    // 创建子线程
    pthread_create(&pid_prepare, 0, task_prepare, this);
}

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>  下面全部都是 start
void *task_start(void *args)
{
    auto *player = static_cast<DerryPlayer *>(args);
    player->start_();
    return nullptr; // 必须返回，坑，错误很难找
}

// TODO 第五节课 内存泄漏关键点（控制packet队列大小，等待队列中的数据被消费） 1
// 把 视频 音频 的压缩包(AVPacket *) 循环获取出来 加入到队列里面去
// 拿出数据packet, 放入队列
void DerryPlayer::start_()
{ // 子线程
    while (isPlaying)
    {

        // 解决方案：视频 我不丢弃数据，等待队列中数据 被消费 内存泄漏点1.1
        if (video_channel && video_channel->packets.size() > 100)
        {
            av_usleep(10 * 1000); // 单位 ：microseconds 微妙 10毫秒
            continue;
        }

        // cout << "start_() 读取视频压缩包" << endl;
        // AVPacket 可能是音频 也可能是视频（压缩包）
        AVPacket *packet = av_packet_alloc();
        int ret = av_read_frame(formatContext, packet);
        if (!ret)
        {
            // ret == 0
            // AudioChannel    队列
            // VideioChannel   队列

            // 把我们的 AVPacket* 加入队列， 音频 和 视频
            /*AudioChannel.insert(packet);
            VideioChannel.insert(packet);*/

            if (video_channel && video_channel->stream_index == packet->stream_index)
            {
                // cout << "插入视频包进队列" << endl;
                // 代表是视频
                video_channel->packets.insertToQueue(packet);
            }
        }
        else if (ret == AVERROR_EOF)
        {
            cout << "读到文件末尾了" << endl;
            // end of file == 读到文件末尾了 == AVERROR_EOF
            // TODO 1.3 内存泄漏点
            // TODO 表示读完了，要考虑释放播放完成，表示读完了 并不代表播放完毕，以后处理【同学思考 怎么处理】
            if (video_channel->packets.empty())
            {
                break; // 队列的数据被音频 视频 全部播放完毕了，我在退出
            }
        }
        else
        {
            cerr << "读取视频压缩包失败" << endl;
            break; // av_read_frame 出现了错误，结束当前循环
        }
    } // end while
    isPlaying = false;
    video_channel->stop();
}

void DerryPlayer::start()
{
    isPlaying = 1;

    // 视频：1.解码    2.播放
    // 1.把队列里面的压缩包(AVPacket *)取出来，然后解码成（AVFrame * ）原始包 ----> 保存队列
    // 2.把队列里面的原始包(AVFrame *)取出来， 视频播放
    if (video_channel)
    {
        // TODO 音视频同步 3.1
        // video_channel->setAudioChannel(audio_channel);
        video_channel->start();
    }

    // TODO 第四节课增加的
    // 视频：1.解码    2.播放
    // 1.把队列里面的压缩包(AVPacket *)取出来，然后解码成（AVFrame * ）原始包 ----> 保存队列
    // 2.把队列里面的原始包(AVFrame *)取出来， 音频播放
    // if (audio_channel) {
    //     audio_channel->start();
    // }

    // 把 音频和视频 压缩包 加入队列里面去
    // 创建子线程
    pthread_create(&pid_start, 0, task_start, this);
}

void DerryPlayer::setRenderCallback(RenderCallback renderCallback)
{
    this->renderCallback = renderCallback;
}

// TODO 第七节课增加 获取总时长
int DerryPlayer::getDuration()
{
    return duration; // 在调用此函数之前，必须给此duration变量赋值
}

void DerryPlayer::seek(int progress)
{
    // 健壮性判断
    if (progress < 0 || progress > duration)
    {
        // TODO 同学们自己去完成，给Java的回调
        return;
    }
    if (!formatContext)
    {
        // TODO 同学们自己去完成，给Java的回调
        return;
    }

    // formatContext 多线程， av_seek_frame内部会对我们的 formatContext上下文的成员做处理，安全的问题
    // 互斥锁 保证多线程情况下安全

    pthread_mutex_lock(&seek_mutex);

    // FFmpeg 大部分单位 == 时间基AV_TIME_BASE
    /**
     * 1.formatContext 安全问题
     * 2.-1 代表默认情况，FFmpeg自动选择 音频 还是 视频 做 seek，  模糊：0视频  1音频
     * 3. AVSEEK_FLAG_ANY（老实） 直接精准到 拖动的位置，问题：如果不是关键帧，B帧 可能会造成 花屏情况
     *    AVSEEK_FLAG_BACKWARD（则优  8的位置 B帧 ， 找附件的关键帧 6，如果找不到他也会花屏）
     *    AVSEEK_FLAG_FRAME 找关键帧（非常不准确，可能会跳的太多），一般不会直接用，但是会配合用
     */
    int r = av_seek_frame(formatContext, -1, progress * AV_TIME_BASE, AVSEEK_FLAG_FRAME);
    if (r < 0)
    {
        // TODO 同学们自己去完成，给Java的回调
        return;
    }

    // TODO 如果你的视频，假设出了花屏，AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME， 缺点：慢一些
    // 有一点点冲突，后面再看 （则优  | 配合找关键帧）
    // av_seek_frame(formatContext, -1, progress * AV_TIME_BASE, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);

    // 音视频正在播放，用户去 seek，我是不是应该停掉播放的数据  音频1frames 1packets，  视频1frames 1packets 队列

    // 这四个队列，还在工作中，让他们停下来， seek完成后，重新播放

    if (video_channel)
    {
        video_channel->packets.setWork(0); // 队列不工作
        video_channel->frames.setWork(0);  // 队列不工作
        video_channel->packets.clear();
        video_channel->frames.clear();
        video_channel->packets.setWork(1); // 队列继续工作
        video_channel->frames.setWork(1);  // 队列继续工作
    }

    pthread_mutex_unlock(&seek_mutex);
}

void *task_stop(void *args)
{
    auto *player = static_cast<DerryPlayer *>(args);
    player->stop_(player);
    return nullptr; // 必须返回，坑，错误很难找
}

void DerryPlayer::stop_(DerryPlayer *derryPlayer)
{
    isPlaying = false;
    pthread_join(pid_prepare, nullptr);
    pthread_join(pid_start, nullptr);

    // pid_prepare pid_start 就全部停止下来了  稳稳的停下来
    if (formatContext)
    {
        avformat_close_input(&formatContext);
        avformat_free_context(formatContext);
        formatContext = nullptr;
    }
    DELETE(video_channel);
    DELETE(derryPlayer);
}

void DerryPlayer::stop()
{

    // 如果是直接释放 我们的 prepare_ start_ 线程，不能暴力释放 ，否则会有bug

    // 让他 稳稳的停下来

    // 我们要等这两个线程 稳稳的停下来后，我再释放DerryPlayer的所以工作
    // 由于我们要等 所以会ANR异常

    // 所以我们我们在开启一个 stop_线程 来等你 稳稳的停下来
    // 创建子线程
    pthread_create(&pid_stop, nullptr, task_stop, this);
}
