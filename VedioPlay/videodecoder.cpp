


#include "videodecoder.h"
#include <QDebug>
#include <QImage>
#include <QMutex>
#include <qdatetime.h>


extern "C" {        // 用C规则编译指定的代码
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

}


#define ERROR_LEN 1024  // 异常信息数组长度
#define PRINT_LOG 1


VideoDecoder::VideoDecoder()
{
    m_error = new char[ERROR_LEN];
}

VideoDecoder::~VideoDecoder()
{
    close();
}


bool VideoDecoder::open(const QString &url)
{
    if(url.isNull())return false;

    AVDictionary* dict = nullptr;
    av_dict_set(&dict, "rtsp_transport", "tcp", 0);      // 设置rtsp流使用tcp打开，如果打开失败错误信息为【Error number -135 occurred】可以切换（UDP、tcp、udp_multicast、http），比如vlc推流就需要使用udp打开
    av_dict_set(&dict, "max_delay", "3", 0);             // 设置最大复用或解复用延迟（以微秒为单位）。当通过【UDP】 接收数据时，解复用器尝试重新排序接收到的数据包（因为它们可能无序到达，或者数据包可能完全丢失）。这可以通过将最大解复用延迟设置为零（通过max_delayAVFormatContext 字段）来禁用。
    av_dict_set(&dict, "timeout", "1000000", 0);         // 以微秒为单位设置套接字 TCP I/O 超时，如果等待时间过短，也可能会还没连接就返回了。

    // 打开输入流并返回解封装上下文
    int ret = avformat_open_input(&m_formatContext,          // 返回解封装上下文
                                  url.toStdString().data(),  // 打开视频地址
                                  nullptr,                   // 如果非null，此参数强制使用特定的输入格式。自动选择解封装器（文件格式）
                                  &dict);                    // 参数设置
    // 释放参数字典
    if(dict)
    {
        av_dict_free(&dict);
    }
    // 打开视频失败
    if(ret < 0)
    {
        showError(ret);
        free();
        return false;
    }
    m_totalTime = m_formatContext->duration / (AV_TIME_BASE / 1000); // 计算视频总时长（毫秒）
#if PRINT_LOG
    qDebug() << QString("视频总时长：%1 ms，[%2]").arg(m_totalTime).arg(QTime::fromMSecsSinceStartOfDay(int(m_totalTime)).toString("HH:mm:ss zzz"));
#endif
    // 通过AVMediaType枚举查询视频流ID（也可以通过遍历查找），最后一个参数无用
    m_videoIndex = av_find_best_stream(m_formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if(m_videoIndex < 0)
    {
        showError(m_videoIndex);
        free();
        return false;
    }
    AVStream* videoStream = m_formatContext->streams[m_videoIndex];  // 通过查询到的索引获取视频流

    // 获取视频图像分辨率（AVStream中的AVCodecContext在新版本中弃用，改为使用AVCodecParameters）
    m_size.setWidth(videoStream->codecpar->width);
    m_size.setHeight(videoStream->codecpar->height);
    m_frameRate = rationalToDouble(&videoStream->avg_frame_rate);  // 视频帧率
    m_totalFrames = videoStream->nb_frames;


    // 通过解码器ID获取视频解码器（新版本返回值必须使用const）
    const AVCodec* codec = avcodec_find_decoder(videoStream->codecpar->codec_id);


#if PRINT_LOG
    qDebug() << QString("分辨率：[w:%1,h:%2] 帧率：%3  总帧数：%4  解码器：%5")
                    .arg(m_size.width()).arg(m_size.height()).arg(m_frameRate).arg(m_totalFrames).arg(codec->name);
#endif

    // 分配AVCodecContext并将其字段设置为默认值。
    m_codecContext = avcodec_alloc_context3(codec);
    if(!m_codecContext)
    {
#if PRINT_LOG
        qWarning() << "创建视频解码器上下文失败！";
#endif
        free();
        return false;
    }

    // 使用视频流的codecpar为解码器上下文赋值
    ret = avcodec_parameters_to_context(m_codecContext, videoStream->codecpar);
    if(ret < 0)
    {
        showError(ret);
        free();
        return false;
    }
    m_codecContext->flags2 |= AV_CODEC_FLAG2_FAST;    // 允许不符合规范的加速技巧。
    m_codecContext->thread_count = 8;                 // 使用8线程解码
    // 初始化解码器上下文，如果之前avcodec_alloc_context3传入了解码器，这里设置NULL就可以
    ret = avcodec_open2(m_codecContext, nullptr, nullptr);
    if(ret < 0)
    {
        showError(ret);
        free();
        return false;
    }

    // 分配AVPacket并将其字段设置为默认值。
    m_packet = av_packet_alloc();
    if(!m_packet)
    {
#if PRINT_LOG
        qWarning() << "av_packet_alloc() Error！";
#endif
        free();
        return false;
    }
    // 分配AVFrame并将其字段设置为默认值。
    m_frame = av_frame_alloc();
    if(!m_frame)
    {
#if PRINT_LOG
        qWarning() << "av_frame_alloc() Error！";
#endif
        free();
        return false;
    }
    // 分配图像空间
    int size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, m_size.width(), m_size.height(), 4);

    m_buffer = new uchar[size + 1000];    // 这里多分配1000个字节就基本不会出现拷贝超出的情况了，反正不缺这点内存
    //    m_image = new QImage(m_buffer, m_size.width(), m_size.height(), QImage::Format_RGBA8888);
    // 这种方式分配内存大部分情况下也可以，但是因为存在拷贝超出数组的情况，delete时也会报错
    m_end = false;
    return true;
}

QImage VideoDecoder::read()
{
    if(!m_formatContext)
    {
        return QImage();
    }
    // 读取下一帧数据
    int readRet = av_read_frame(m_formatContext, m_packet);
    if(readRet < 0)
    {
        avcodec_send_packet(m_codecContext, m_packet); // 读取完成后向解码器中传如空AVPacket，否则无法读取出最后几帧
    }
    else
    {
        if(m_packet->stream_index == m_videoIndex)     // 如果是图像数据则进行解码
        {
            // 计算当前帧时间（毫秒）
#if 1       // 方法一：适用于所有场景，但是存在一定误差
            m_packet->pts = qRound64(m_packet->pts * (1000 * rationalToDouble(&m_formatContext->streams[m_videoIndex]->time_base)));
            m_packet->dts = qRound64(m_packet->dts * (1000 * rationalToDouble(&m_formatContext->streams[m_videoIndex]->time_base)));
#else       // 方法二：适用于播放本地视频文件，计算每一帧时间较准，但是由于网络视频流无法获取总帧数，所以无法适用
            m_obtainFrames++;
            m_packet->pts = qRound64(m_obtainFrames * (qreal(m_totalTime) / m_totalFrames));
#endif \
    // 将读取到的原始数据包传入解码器
            int ret = avcodec_send_packet(m_codecContext, m_packet);
            if(ret < 0)
            {
                showError(ret);
            }
        }
    }
    av_packet_unref(m_packet);  // 释放数据包，引用计数-1，为0时释放空间
    int ret = avcodec_receive_frame(m_codecContext, m_frame);
    if(ret < 0)
    {
        av_frame_unref(m_frame);
        if(readRet < 0)
        {
            m_end = true;     // 当无法读取到AVPacket并且解码器中也没有数据时表示读取完成
        }
        return QImage();
    }

    m_pts = m_frame->pts;

    // 为什么图像转换上下文要放在这里初始化呢，是因为m_frame->format，如果使用硬件解码，解码出来的图像格式和m_codecContext->pix_fmt的图像格式不一样，就会导致无法转换为QImage
    if(!m_swsContext)
    {
        // 获取缓存的图像转换上下文。首先校验参数是否一致，如果校验不通过就释放资源；然后判断上下文是否存在，如果存在直接复用，如不存在进行分配、初始化操作
        m_swsContext = sws_getCachedContext(m_swsContext,
                                            m_frame->width,                     // 输入图像的宽度
                                            m_frame->height,                    // 输入图像的高度
                                            (AVPixelFormat)m_frame->format,     // 输入图像的像素格式
                                            m_size.width(),                     // 输出图像的宽度
                                            m_size.height(),                    // 输出图像的高度
                                            AV_PIX_FMT_RGBA,                    // 输出图像的像素格式
                                            SWS_BILINEAR,                       // 选择缩放算法(只有当输入输出图像大小不同时有效),一般选择SWS_FAST_BILINEAR
                                            nullptr,                            // 输入图像的滤波器信息, 若不需要传NULL
                                            nullptr,                            // 输出图像的滤波器信息, 若不需要传NULL
                                            nullptr);                          // 特定缩放算法需要的参数(?)，默认为NULL
        if(!m_swsContext)
        {
#if PRINT_LOG
            qWarning() << "sws_getCachedContext() Error！";
#endif
            free();
            return QImage();
        }
    }

    // AVFrame转QImage
    uchar* data[]  = {m_buffer};
    int    lines[4];
    av_image_fill_linesizes(lines, AV_PIX_FMT_RGBA, m_frame->width);  // 使用像素格式pix_fmt和宽度填充图像的平面线条大小。
    ret = sws_scale(m_swsContext,             // 缩放上下文
                    m_frame->data,            // 原图像数组
                    m_frame->linesize,        // 包含源图像每个平面步幅的数组
                    0,                        // 开始位置
                    m_frame->height,          // 行数
                    data,                     // 目标图像数组
                    lines);                   // 包含目标图像每个平面的步幅的数组
    QImage image(m_buffer, m_frame->width, m_frame->height, QImage::Format_RGBA8888);
    av_frame_unref(m_frame);

    return image;

}

void VideoDecoder::showError(int err)
{
#if PRINT_LOG
    memset(m_error, 0, ERROR_LEN);        // 将数组置零
    av_strerror(err, m_error, ERROR_LEN);
    qWarning() << "DecodeVideo Error：" << m_error;
#else
    Q_UNUSED(err)
#endif
}

bool VideoDecoder::isEnd()
{
    return m_end;
}
/**
 * @brief    返回当前帧图像播放时间
 * @return
 */
const qint64 &VideoDecoder::pts()
{
    return m_pts;
}
void VideoDecoder::close()
{
    clear();
    free();

    m_totalTime     = 0;
    m_videoIndex    = 0;
    m_totalFrames   = 0;
    m_obtainFrames  = 0;
    m_pts           = 0;
    m_frameRate     = 0;
    m_size          = QSize(0, 0);
}



/**
 * @brief 清空读取缓冲
 */
void VideoDecoder::clear()
{
    // 因为avformat_flush不会刷新AVIOContext (s->pb)。如果有必要，在调用此函数之前调用avio_flush(s->pb)。
    if(m_formatContext && m_formatContext->pb)
    {
        avio_flush(m_formatContext->pb);
    }
    if(m_formatContext)
    {
        avformat_flush(m_formatContext);   // 清理读取缓冲
    }
}
void VideoDecoder::free()
{
    // 释放上下文swsContext。
    if(m_swsContext)
    {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;             // sws_freeContext不会把上下文置NULL
    }
    // 释放编解码器上下文和与之相关的所有内容，并将NULL写入提供的指针
    if(m_codecContext)
    {
        avcodec_free_context(&m_codecContext);
    }
    // 关闭并失败m_formatContext，并将指针置为null
    if(m_formatContext)
    {
        avformat_close_input(&m_formatContext);
    }
    if(m_packet)
    {
        av_packet_free(&m_packet);
    }
    if(m_frame)
    {
        av_frame_free(&m_frame);
    }
    if(m_buffer)
    {
        delete [] m_buffer;
        m_buffer = nullptr;
    }
}
qreal VideoDecoder::rationalToDouble(AVRational* rational)
{
    qreal frameRate = (rational->den == 0) ? 0 : (qreal(rational->num) / rational->den);
    return frameRate;
}
