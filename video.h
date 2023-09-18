#ifndef VIDEO_H
#define VIDEO_H
#include <iostream>
#include "file.h"

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/mem.h>
#include <libavutil/imgutils.h>
}

class video
{
public:
    int init(std::string cam_url,std::string input_format,std::string video_size,std::string framerate);
    int getH264Frame(uint8_t *frame,int len);
    int ifSps(uint8_t *nalu,int len);
    int startCode4(char* buf);
    uint8_t * findSps(uint8_t *sp,int len);
    video();

public:
    std::string url;
private:
    int ret;
    AVFormatContext *fmt_ctx = NULL;
    AVDictionary *options = NULL;
    AVInputFormat* input_fmt = NULL;
    const AVCodec *codec;
    AVCodecContext *codeCtx = NULL;
    AVStream *stream = NULL;
    int stream_index1;
    AVPacket avpkt;
    int frame_count;
    AVFrame *frame;
    SwsContext *swsContext;
    AVPicture g_oPicture = {0};
    char buf[1024];
    file fileIns;
};

#endif // VIDEO_H
