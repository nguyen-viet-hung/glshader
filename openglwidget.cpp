#include "openglwidget.h"
#include <QDebug>

void OpenGLWidget::updateTextureData (GLint tex, unsigned char* data, int w, int h)
{
    glEnable (GL_TEXTURE_RECTANGLE_ARB);
    glBindTexture (GL_TEXTURE_RECTANGLE_ARB, tex);

    glTexParameteri (GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage2D  (GL_TEXTURE_RECTANGLE_ARB, 0, 1, w , h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
}

void OpenGLWidget::updateTexturesFromBuffer (unsigned char* buffer, int width, int height, GLint y_plane, GLint u_plane, GLint v_plane)
{
    glActiveTextureARB (GL_TEXTURE0_ARB);
    glEnable (GL_TEXTURE_RECTANGLE_ARB);
    glBindTexture (GL_TEXTURE_RECTANGLE_ARB, y_plane);
    updateTextureData (y_plane, buffer, width, height);

    glActiveTextureARB (GL_TEXTURE1_ARB);
    glEnable (GL_TEXTURE_RECTANGLE_ARB);
    glBindTexture (GL_TEXTURE_RECTANGLE_ARB, u_plane);
    updateTextureData (u_plane, buffer + width * height, width / 2, height / 2);

    glActiveTextureARB (GL_TEXTURE2_ARB);
    glEnable (GL_TEXTURE_RECTANGLE_ARB);
    glBindTexture (GL_TEXTURE_RECTANGLE_ARB, v_plane);
    updateTextureData (v_plane, buffer + width * height + (width / 2) * (height / 2), width / 2, height / 2);
}

void OpenGLWidget::drawLocal()
{
    if (!localVideoBufferDrawer)
        return;

    //qDebug() << "Draw local now";

    unsigned long size, width, height;
    int64_t timestamp;

    if (localVideoBuffer.PopUpData(localVideoBufferDrawer, size, timestamp, width, height)) {

        glPushMatrix ();

        updateTexturesFromBuffer(localVideoBufferDrawer, width, height, local_y_plane, local_u_plane, local_v_plane);

        glEnable (GL_FRAGMENT_PROGRAM_ARB);
        glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, local_yuv_shader);

        double topy;
        double bottomy;
        double center_y = 0.0;
        double center_x = 0.0;

        topy = center_y - 1.0;
        bottomy = center_y + 1.0;

        glBegin (GL_QUADS);

            glColor4f (1.0, 1.0, 1.0, 0.0);

            glMultiTexCoord2fARB (GL_TEXTURE0_ARB, 0, height);
            glMultiTexCoord2fARB (GL_TEXTURE1_ARB, 0, height / 2);
            glMultiTexCoord2fARB (GL_TEXTURE2_ARB, 0, height / 2);
            glVertex2f (center_x - 1.6, topy);

            glMultiTexCoord2fARB (GL_TEXTURE0_ARB, width, height);
            glMultiTexCoord2fARB (GL_TEXTURE1_ARB, width / 2, height / 2);
            glMultiTexCoord2fARB (GL_TEXTURE2_ARB, width / 2, height / 2);
            glVertex2f (center_x + 1.6, topy);

            glColor4f (1.0, 1.0, 1.0, 0.0);

            glMultiTexCoord2fARB (GL_TEXTURE0_ARB, width, 0);
            glMultiTexCoord2fARB (GL_TEXTURE1_ARB, width / 2, 0);
            glMultiTexCoord2fARB (GL_TEXTURE2_ARB, width / 2, 0);
            glVertex2f (center_x + 1.6, bottomy);

            glMultiTexCoord2fARB (GL_TEXTURE0_ARB, 0, 0);
            glMultiTexCoord2fARB (GL_TEXTURE1_ARB, 0, 0);
            glMultiTexCoord2fARB (GL_TEXTURE2_ARB, 0, 0);
            glVertex2f (center_x - 1.6, bottomy);

        glEnd ();

        glDisable (GL_FRAGMENT_PROGRAM_ARB);

        glActiveTextureARB (GL_TEXTURE0_ARB); glDisable (GL_TEXTURE_RECTANGLE_ARB);
        glActiveTextureARB (GL_TEXTURE1_ARB); glDisable (GL_TEXTURE_RECTANGLE_ARB);
        glActiveTextureARB (GL_TEXTURE2_ARB); glDisable (GL_TEXTURE_RECTANGLE_ARB);

        glPopMatrix ();

        glFlush();
    }
}

unsigned int OpenGLWidget::loadShader (GLuint type)
{
    unsigned int shader_num;
    const char* textureProgram = "!!ARBfp1.0\n"
                                     "TEX result.color, fragment.texcoord[0], texture[0], 2D;\n"
                                     "END\n";

    glEnable (type);
    glGenProgramsARB (1, &shader_num);
    glBindProgramARB (type, shader_num);
    glProgramStringARB (type, GL_PROGRAM_FORMAT_ASCII_ARB,
                         strlen (textureProgram), (const GLbyte *) textureProgram);

    glDisable(type);

    return shader_num;
}

OpenGLWidget::OpenGLWidget(QWidget *parent)
    : QGLWidget(parent)
    , localVideoBuffer("", 4, 1920*1080*4)
    , localVideoBufferDrawer(NULL)
{
}

OpenGLWidget::~OpenGLWidget()
{
    if (localVideoBufferDrawer) {
        delete localVideoBufferDrawer;
        localVideoBufferDrawer = NULL;
    }
}

void OpenGLWidget::paintGL()
{
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    drawLocal();
}

void OpenGLWidget::initializeGL()
{
    initializeGLFunctions();
    showFullScreen();

    glGenTextures (1, &local_y_plane);
    glGenTextures (1, &local_u_plane);
    glGenTextures (1, &local_v_plane);

    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    local_yuv_shader = loadShader(GL_FRAGMENT_PROGRAM_ARB);
    localVideoBufferDrawer = new unsigned char[1920*1080*4];

    qDebug() << "local y texture = " << local_y_plane;
    qDebug() << "local u texture = " << local_u_plane;
    qDebug() << "local v texture = " << local_v_plane;
    qDebug() << "local yuv shader = " << local_yuv_shader;

    connect(&videoCapturer, SIGNAL(sendCapturedImg(VideoImage*)), this, SLOT(receiveLocalVideoImg(VideoImage*)));
    videoCapturer.openDevice(0);
}

void OpenGLWidget::receiveLocalVideoImg(VideoImage *img)
{
    localVideoBuffer.PushBackData(img->imgBuffer, img->width*img->height*5/4, 0, img->width, img->height);
    updateGL();
}
