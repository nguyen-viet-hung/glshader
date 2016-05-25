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
    /* Texture ID's */
    GLuint local_y_plane;
    GLuint local_u_plane;
    GLuint local_v_plane;

    /* Shader ID's */
    GLuint local_yuv_shader;
    void updateTextureData (GLint tex, unsigned char* data, int w, int h);
    void updateTexturesFromBuffer (unsigned char* buffer, int width, int height, GLint y_plane, GLint u_plane, GLint v_plane);
    void drawLocal();
    unsigned int loadShader (GLuint type);
    VCLinxVideoCapture videoCapturer;
public:
    OpenGLWidget(QWidget *parent = 0);
    ~OpenGLWidget();

    virtual void paintGL();
    virtual void initializeGL();

public slots:
    void receiveLocalVideoImg(VideoImage* img);
};

#endif // OPENGLWIDGET_H
