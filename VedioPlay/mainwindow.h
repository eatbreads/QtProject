#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include "videodecoder.h"
#include "readthread.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE
class ReadThread;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:


    void on_videoPlayButton_clicked();
    void on_playState(ReadThread::PlayState state);
    void on_selectButton_clicked();

    void on_pauseButton_clicked();

private:
    Ui::MainWindow *ui;
    VideoDecoder * decoder;
    ReadThread* m_readThread = nullptr;
};
#endif // MAINWINDOW_H
