#include "openglwidget.h"
#include <QDebug>
#include <GL/glu.h>
#include <GL/glx.h>
#include "defines.h"
#include <string>
#include <fstream>

int CheckGLError(char *file, int line)
{
    GLenum glErr;
    int    retCode = 0;

    glErr = glGetError();
    while (glErr != GL_NO_ERROR)
    {
        const GLubyte* sError = gluErrorString(glErr);

        if (sError)
            qDebug() << "GL Error #" << glErr << "(" << gluErrorString(glErr) << ") " << " in File " << file << " at line: " << line;
        else
            qDebug() << "GL Error #" << glErr << " (no message available)" << " in File " << file << " at line: " << line;

        retCode = 1;
        glErr = glGetError();
    }
    return retCode;
}

#define CHECK_GL_ERROR() CheckGLError(__FILE__, __LINE__)

void OpenGLWidget::drawLocal()
{
    if (!localVideoBufferDrawer)
        return;

    //qDebug() << "Draw local now";

    unsigned long size, w, h;
    int64_t timestamp;

    if (localVideoBuffer.PopUpData(localVideoBufferDrawer, size, timestamp, w, h)) {

        //memset(localVideoBufferDrawer + width*height, 128, width*height/4);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //bind texture with data
        glEnable(GL_TEXTURE_2D);

//        if (support_vsync)
//            glXSwapIntervalMESA(1);

        for (int idx = 0; idx < 3; idx++) {

            if (multiTexture)
                glActiveTextureARB(GL_TEXTURE0_ARB+idx);

            uint8_t* tmp = localVideoBufferDrawer;
            if (idx == 1)
                tmp += w*h;
            else if (idx == 2)
                tmp += w*h*5/4;

            glBindTexture(GL_TEXTURE_2D, textures[idx]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, (idx > 0 ? (w/2) : w), (idx > 0 ? (h/2) : h),
                         0, GL_RED, GL_UNSIGNED_BYTE, tmp);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	// Linear Filtering
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// Linear Filtering
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }

        glUseProgram(program);
        glUniform1i(glGetUniformLocation(program, "yTexture"), 0);
        glUniform1i(glGetUniformLocation(program, "uTexture"), 1);
        glUniform1i(glGetUniformLocation(program, "vTexture"), 2);

        glBegin(GL_QUADS);

        if (stretchScreen) {
            glTexCoord2f(0.0, 0.0); glVertex2d(-1.0, 1.0);
            glTexCoord2f(0.0, 1.0); glVertex2d(-1.0, -1.0);
            glTexCoord2f(1.0, 1.0); glVertex2d(1.0, -1.0);
            glTexCoord2f(1.0, 0.0); glVertex2d(1.0, 1.0);
        } else {

            float i_ratio = (float)w / (float)h;
            float s_ratio = (float)width() / (float)height();

            float offset;

            if (i_ratio > s_ratio) {
                offset = (1.0 - (float)width()/(i_ratio*height())) / 2.0;
                glTexCoord2f(0.0, 0.0); glVertex2d(-1.0, 1.0 - offset);
                glTexCoord2f(0.0, 1.0); glVertex2d(-1.0, -1.0 + offset);
                glTexCoord2f(1.0, 1.0); glVertex2d(1.0, -1.0 + offset);
                glTexCoord2f(1.0, 0.0); glVertex2d(1.0, 1.0 - offset);
            } else if (i_ratio < s_ratio) {
                offset = (1.0 - (i_ratio*height())/((float)width())) / 2.0;
                glTexCoord2f(0.0, 0.0); glVertex2d(-1.0 + offset, 1.0);
                glTexCoord2f(0.0, 1.0); glVertex2d(-1.0 + offset, -1.0);
                glTexCoord2f(1.0, 1.0); glVertex2d(1.0 - offset, -1.0);
                glTexCoord2f(1.0, 0.0); glVertex2d(1.0 - offset, 1.0);
            } else {
                glTexCoord2f(0.0, 0.0); glVertex2d(-1.0, 1.0);
                glTexCoord2f(0.0, 1.0); glVertex2d(-1.0, -1.0);
                glTexCoord2f(1.0, 1.0); glVertex2d(1.0, -1.0);
                glTexCoord2f(1.0, 0.0); glVertex2d(1.0, 1.0);
            }
        }

        glEnd();

        glUseProgram(0);
        glDisable(GL_TEXTURE_2D);

//        if (support_vsync)
//            glXSwapIntervalMESA(0);

        glFlush();
    }
}

void OpenGLWidget::loadFile(const char *fn, std::string &str)
{
    std::ifstream in(fn);
    if(!in.is_open()){
        return;
    }
    char tmp[300];
    while(!in.eof()){
        in.getline(tmp,300);
        str += tmp;
        str += "\n";
    }// End function
}

void OpenGLWidget::loadShader ()
{
    glClearColor(0.5f, 0.5f, 1.0f, 0.0f);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);

    //create vertex shader
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    //create fragment shader
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    std::string vertexSourceCode;
    std::string fragmentSourceCode;

    loadFile("/home/devel/qt/Shader/vertex.shader", vertexSourceCode);
    loadFile("/home/devel/qt/Shader/fragment.shader", fragmentSourceCode);

    const char* adapter = vertexSourceCode.c_str();

    //attach source code to shader
    glShaderSourceARB(vertexShader, 1, &adapter, NULL);
    adapter = fragmentSourceCode.c_str();
    glShaderSourceARB(fragmentShader, 1, &adapter, NULL);

    //compile source code
    glCompileShaderARB(vertexShader);
    glCompileShaderARB(fragmentShader);

    //ok now, link into program
    program = glCreateProgram();

    glAttachShader(program, vertexShader);
    CHECK_GL_ERROR();
    glAttachShader(program, fragmentShader);
    CHECK_GL_ERROR();

    glLinkProgram(program);
    CHECK_GL_ERROR();

    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        glDeleteObjectARB(vertexShader);
        glDeleteShader(vertexShader);
        glDeleteObjectARB(fragmentShader);
        glDeleteShader(fragmentShader);
        return;
    }

    //create texture
    glGenTextures(3, textures);
}

OpenGLWidget::OpenGLWidget(QWidget *parent)
    : QGLWidget(parent)
    , localVideoBuffer("", 4, 1920*1080*4)
    , localVideoBufferDrawer(NULL)
    , stretchScreen(false)
{
}

OpenGLWidget::~OpenGLWidget()
{
    if (localVideoBufferDrawer) {
        delete localVideoBufferDrawer;
        localVideoBufferDrawer = NULL;
    }
}

void OpenGLWidget::resizeGL(int w, int h)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glViewport(0, 0, w, h);
}

void OpenGLWidget::paintGL()
{
    drawLocal();
}

void OpenGLWidget::initializeGL()
{
    initializeGLFunctions();
    showFullScreen();

    const char *extensions = (const char *)glGetString(GL_EXTENSIONS);

    supports_npot = HasExtension(extensions, "GL_ARB_texture_non_power_of_two") ||
            HasExtension(extensions, "GL_APPLE_texture_2D_limited_npot");

    multiTexture = HasExtension(extensions, "GL_ARB_multitexture");

    support_vsync = HasExtension(extensions, "GLX_MESA_swap_control");

    localVideoBufferDrawer = new unsigned char[1920*1080*4];

    loadShader();

    connect(&videoCapturer, SIGNAL(sendCapturedImg(VideoImage*)), this, SLOT(receiveLocalVideoImg(VideoImage*)));
    videoCapturer.openDevice(0);
}

void OpenGLWidget::receiveLocalVideoImg(VideoImage *img)
{
    localVideoBuffer.PushBackData(img->imgBuffer, img->width*img->height*3/2, 0, img->width, img->height);
    update();
}
