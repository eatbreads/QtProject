#include "mainwindow.h"
#include "./ui_mainwindow.h"

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


}
MainWindow::~MainWindow()
{

    delete ui;
}


void MainWindow::on_comboBox_currentTextChanged(const QString &arg1)
{

}


void MainWindow::on_videoPlayButton_clicked()
{
    //decoder = new VideoDecoder(ui->comboBox->currentText());
}

