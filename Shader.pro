#-------------------------------------------------
#
# Project created by QtCreator 2016-05-11T14:09:16
#
#-------------------------------------------------

QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Shader
TEMPLATE = app


SOURCES += main.cpp\
        openglwidget.cpp \
    datasink.cpp \
    vclinxvideocapture.cpp

HEADERS  += openglwidget.h \
    datasink.h \
    vclinxvideocapture.h

LIBS += -L/usr/local/lib -lpt -lh323_x86_64_ -lasound -lswscale -lavcodec -lavutil

DEFINES += GL_GLEXT_PROTOTYPES

CONFIG += c++11
