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
    /* yuv texture */
    typedef struct _yuvTexture{
        GLuint textures[3];
        struct {
            int width;
            int height;
        }textureResolutions[3];

        unsigned planes;
        uint32_t fourcc;
        int width;
        int height;

        GLuint vertexBufferObjects;
        GLuint textureBufferObjects[4];
    }YUVTexture;

    YUVTexture* localYUVTexture;
    GLfloat localValues[16];

    /* Shader ID's */
    //GLuint local_yuv_shader;
    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint program;
    GLuint vao;

    GLint u_pos;

    bool supports_npot;
    bool multiTexture;
    GLuint textures[3];

    GLint linked;
    void drawWithShaders(YUVTexture* texture, float *left, float *top, float *right, float *bottom);
    void updateTextureFromBuffer (unsigned char* buffer, int width, int height, YUVTexture* texture);
    void drawLocal();
    //unsigned int loadShader (GLuint type);
    YUVTexture* createYUVTexture(uint32_t fourcc, int width, int height);
    void deleteYUVTexture(YUVTexture** texture);
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
