#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle(QString("ffmpeg+qt软解码并且使用QPainter绘制"));
    // 添加条目
    ui->comboBox->addItem("C:/Users/18526/Desktop/dd.mp4");
    ui->comboBox->addItem("第二项");
    ui->comboBox->addItem("第三项");


    m_readThread = new ReadThread();
    //connect(m_readThread, &ReadThread::updateImage, ui->playimage, &PlayImage::updateImage, Qt::DirectConnection);
    connect(m_readThread, &ReadThread::updateImage, ui->playimage, &PlayImage::updateImage);
    connect(m_readThread, &ReadThread::playState, this, &MainWindow::on_playState);


}
MainWindow::~MainWindow()
{

    delete ui;
}




void MainWindow::on_videoPlayButton_clicked()
{
    if (ui->videoPlayButton->text() == "开始播放")
    {
        m_readThread->open(ui->comboBox->currentText());
    }
    else
    {
        m_readThread->close();
    }
}


void MainWindow::on_selectButton_clicked()
{
    QString strName = QFileDialog::getOpenFileName(this, "选择播放视频~！", "/", "视频 (*.mp4 *.m4v *.mov *.avi *.flv);; 其它(*)");
    if (strName.isEmpty())
    {
        return;
    }
    ui->comboBox->setCurrentText(strName);
}


void MainWindow::on_pauseButton_clicked()
{
    if (ui->pauseButton->text() == "暂停")
    {
        m_readThread->pause(true);
        ui->pauseButton->setText("继续");
    }
    else
    {
        m_readThread->pause(false);
        ui->pauseButton->setText("暂停");
    }
}
void MainWindow::on_playState(ReadThread::PlayState state)
{
    if (state == ReadThread::play)
    {
        this->setWindowTitle(QString("正在播放：%1").arg(m_readThread->url()));
        ui->videoPlayButton->setText("停止播放");
    }
    else
    {
        ui->videoPlayButton->setText("开始播放");
        ui->pauseButton->setText("暂停");
        this->setWindowTitle(QString("Qt+ffmpeg视频播放（软解码）Demo V1"));
    }
}

