#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QGLWidget>
#include <QGLFunctions>
#include "datasink.h"
#include "vclinxvideocapture.h"

class OpenGLWidget : public QGLWidget, public QGLFunctions
{
    Q_OBJECT
protected:
    DataSink localVideoBuffer;
    unsigned char* localVideoBufferDrawer;

    /* Shader ID's */
    //GLuint local_yuv_shader;
    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint program;

    bool supports_npot;
    bool multiTexture;
    bool stretchScreen;
    bool support_vsync;
    GLuint textures[3];

    GLint linked;
    void drawLocal();

    void loadFile(const char *fn, std::string &str);
    void loadShader();
    VCLinxVideoCapture videoCapturer;
public:
    OpenGLWidget(QWidget *parent = 0);
    ~OpenGLWidget();

    virtual void resizeGL(int w, int h);
    virtual void paintGL();
    virtual void initializeGL();

public slots:
    void receiveLocalVideoImg(VideoImage* img);
};

#endif // OPENGLWIDGET_H
