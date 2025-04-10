#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include<QString>
#include<QSize>


struct AVFormatContext;
struct AVCodecContext;
struct AVRational;
struct AVPacket;
struct AVFrame;
struct SwsContext;
struct AVBufferRef;
class QImage;


class VideoDecoder
{
public:
    VideoDecoder();
    ~VideoDecoder();


    bool open(const QString& url=  QString());
    QImage read();
    void close();
    bool isEnd;
    const qint64& pts();

private:
    void showError(int err);                      // 显示ffmpeg执行错误时的错误信息
    qreal rationalToDouble(AVRational* rational); // 将AVRational转换为double
    void clear();                                 // 清空读取缓冲
    void free();                                  // 释放

private:
    AVFormatContext* m_formatContext = nullptr;
    AVCodecContext* m_codecContext = nullptr;
    SwsContext* m_swsContext = nullptr;
    AVPacket* m_packet = nullptr;
    AVFrame*  m_frame  = nullptr;                 // 解码后的视频帧
    int m_videoIndex = 0;
    qint64 m_totalTime = 0;//总时长和总帧数
    qint64 m_totalFrames  = 0;
    qint64 m_obtainFrames = 0;//当前已经获取的帧数
    qint64 m_pts          = 0;//当前时间戳
    qreal  m_frameRate    = 0; //帧率,这是个浮点数,qreal是一种qt的浮点数
    QSize  m_size;              //分辨率大小,QSize是QT中的一个用于表示二维的类
    char * m_error = nullptr;
    bool m_end = false;
    uchar * m_buffer = nullptr; //这是用来存储yuv转rgba之后的数据

};

#endif // VIDEODECODER_H
