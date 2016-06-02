#include "openglwidget.h"
#include <QDebug>
#include <GL/glu.h>
#include <GL/glut.h>
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

void OpenGLWidget::drawWithShaders(OpenGLWidget::YUVTexture* texture,
                            float *left, float *top, float *right, float *bottom)
{
}

void OpenGLWidget::updateTextureFromBuffer (unsigned char* buffer, int width, int height, OpenGLWidget::YUVTexture* texture)
{
}

void OpenGLWidget::drawLocal()
{
    if (!localVideoBufferDrawer)
        return;

    //qDebug() << "Draw local now";

    unsigned long size, width, height;
    int64_t timestamp;
    static uint32_t clip = 0;

    if (localVideoBuffer.PopUpData(localVideoBufferDrawer, size, timestamp, width, height)) {

        //memset(localVideoBufferDrawer + width*height, 128, width*height/4);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //bind texture with data
        glEnable(GL_TEXTURE_2D);

        for (int idx = 0; idx < 3; idx++) {

            if (multiTexture)
                glActiveTextureARB(GL_TEXTURE0_ARB+idx);

            uint8_t* tmp = localVideoBufferDrawer;
            if (idx == 1)
                tmp += width*height;
            else if (idx == 2)
                tmp += width*height*5/4;

            glBindTexture(GL_TEXTURE_2D, textures[idx]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, (idx > 0 ? (width/2) : width), (idx > 0 ? (height/2) : height),
                         0, GL_RED, GL_UNSIGNED_BYTE, tmp);

            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);	// Linear Filtering
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
        }

        glUseProgram(program);
        glUniform1i(glGetUniformLocation(program, "yTexture"), 0);
        glUniform1i(glGetUniformLocation(program, "uTexture"), 1);
        glUniform1i(glGetUniformLocation(program, "vTexture"), 2);

        glBegin(GL_QUADS);

        glTexCoord2f(0.0, 0.0); glVertex2d(-1.0, 1.0);
        glTexCoord2f(0.0, 1.0); glVertex2d(-1.0, -1.0);
        glTexCoord2f(1.0, 1.0); glVertex2d(1.0, -1.0);
        glTexCoord2f(1.0, 0.0); glVertex2d(1.0, 1.0);

        glEnd();

        glUseProgram(0);
        glDisable(GL_TEXTURE_2D);

        glFlush();
    }
}

OpenGLWidget::YUVTexture *OpenGLWidget::createYUVTexture(uint32_t fourcc, int width, int height)
{
    if (width <= 0 || height <= 0)
        return NULL;

    OpenGLWidget::YUVTexture* textures = new OpenGLWidget::YUVTexture;
    if (!textures)
        return NULL;

    memset(textures, 0, sizeof(OpenGLWidget::YUVTexture));
    textures->fourcc = fourcc;
    textures->width = width;
    textures->height = height;

    switch (textures->fourcc) {
    case MAKEFOURCC('Y', 'V', '1', '2'):
        textures->planes = 3;

        if (supports_npot) {
            textures->textureResolutions[0].width = width;
            textures->textureResolutions[0].height = height;
            textures->textureResolutions[1].width = width / 2;
            textures->textureResolutions[1].height = height / 2;
            textures->textureResolutions[2].width = width / 2;
            textures->textureResolutions[2].height = height / 2;
        } else {
            textures->textureResolutions[0].width = GetAlignedSize(width);
            textures->textureResolutions[0].height = GetAlignedSize(height);
            textures->textureResolutions[1].width = GetAlignedSize(width / 2);
            textures->textureResolutions[1].height = GetAlignedSize(height / 2);
            textures->textureResolutions[2].width = GetAlignedSize(width / 2);
            textures->textureResolutions[2].height = GetAlignedSize(height / 2);
        }

        for (unsigned i = 0; i < textures->planes; i++) {

            glGenTextures(1, &textures->textures[i]);
            glBindTexture(GL_TEXTURE_2D, textures->textures[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, textures->textureResolutions[i].width, textures->textureResolutions[i].height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL); // y_pixels);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        break;
    default:
        delete textures;
        textures = NULL;
        break;
    }

    return textures;
}

void OpenGLWidget::deleteYUVTexture(OpenGLWidget::YUVTexture **texture)
{
    if (!(*texture))
        return;

    OpenGLWidget::YUVTexture* obj = *texture;
    *texture = NULL;

    glDeleteBuffers(1, &obj->vertexBufferObjects);
    glDeleteBuffers(obj->planes, obj->textureBufferObjects);
    glDeleteTextures(obj->planes, obj->textures);

    delete obj;
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
    , localYUVTexture(NULL)
{
}

OpenGLWidget::~OpenGLWidget()
{
    if (localVideoBufferDrawer) {
        delete localVideoBufferDrawer;
        localVideoBufferDrawer = NULL;
    }

    deleteYUVTexture(&localYUVTexture);
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
