TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

DEFINES += QT_DEPRECATED_WARNINGS QT_DEPRECATED_WARNINGS QT_NO_KEYWORDS

SOURCES += \
        file.cpp \
        main.cpp \
        rtp.cpp \
        rtsp.cpp \
        video.cpp

HEADERS += \
    file.h \
    rtp.h \
    rtsp.h \
    video.h


INCLUDEPATH += /usr/local/ffmpeg/include


LIBS += -L/usr/local/ffmpeg/lib -lavdevice -lavcodec -lavformat -lavutil -lswscale -lpthread
