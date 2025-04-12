#ifndef PLAYIMAGE_H
#define PLAYIMAGE_H

#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions_3_3_Core>
#include <QImage>
#include <QMutex>


class PlayImage : public QOpenGLWidget, public  QOpenGLFunctions_3_3_Core

{
    Q_OBJECT
public:
     explicit PlayImage(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    void updateImage(const QImage& image);
    //void updatePixmap(const QPixmap& pixmap);
    ~PlayImage() override;



protected:
    void initializeGL() override;               // 初始化gl
    void resizeGL(int w, int h) override;       // 窗口尺寸变化
    void paintGL() override;                    // 刷新显示



private:
    QOpenGLShaderProgram* m_program = nullptr;
    QOpenGLTexture* m_texture = nullptr;

    GLuint VBO = 0;       // 顶点缓冲对象,负责将数据从内存放到缓存，一个VBO可以用于多个VAO
    GLuint VAO = 0;       // 顶点数组对象,任何随后的顶点属性调用都会储存在这个VAO中，一个VAO可以有多个VBO
    GLuint EBO = 0;       // 元素缓冲对象,它存储 OpenGL 用来决定要绘制哪些顶点的索引
    QSize  m_size;
    QSizeF  m_zoomSize;
    QPointF m_pos;
};

#endif // PLAYIMAGE_H
