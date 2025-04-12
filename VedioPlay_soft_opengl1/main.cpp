#include "mainwindow.h"
#include <QDebug>
#include <QApplication>
#include "videodecoder.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    VideoDecoder decoder;
    if(!decoder.open("C:/Users/18526/Desktop/dd.mp4"))
    {
        qDebug()<<"打开文件失败";
    }
    MainWindow w;

    w.show();
    return a.exec();
}
