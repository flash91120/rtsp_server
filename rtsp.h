#ifndef RTSP_H
#define RTSP_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <pthread.h>
#include <iostream>

#include "video.h"
#include "rtp.h"

#define H264_FILE_NAME   "test.h264"
#define BUF_MAX_SIZE     (1024*1024)

class rtspInterface{
private:
    char method[40] = {0};
    char url[100] = {0};
    char version[40] = {0};
    char *bufPtr;
    char* rBuf = nullptr;
    char* sBuf = nullptr;
    char line[400] = {0};

    int cseq;
    int clientRtpPort,clientRtcpPort;
    int serverRTSPPort,serverRtpPort,serverRtcpPort;

    int clientSockfd;
    const char* clientIP;
    int clientPort;

    int serverSockfd;
    int serverRtpSockfd;
    int serverRtcpSockfd;

    int recvLen;

    pthread_t sendThread;

    int frameSize;
    int startCode;

    uint8_t* frame = nullptr;
    struct RtpPacket* rtpPacket = nullptr;

public:
    int initRtsp(std::string serverRTSPPort,std::string serverRtpPort,std::string serverRtcpPort);
    void startRtspServer();
    int createTcpSocket();
    int createUdpSocket();
    int bindSocketAddr(int sockfd, const char* ip, int port);
    void acceptClient();
    inline int startCode3(char* buf);
    inline int startCode4(char* buf);
    char* findNextStartCode(char* buf, int len);
    int getFrameFromH264File(int fd, char* frame, int size);
    int rtpSendH264Frame(int socket, const char* ip, int16_t port,
                         struct RtpPacket* rtpPacket, uint8_t* frame, uint32_t frameSize);
    char* getLineFromBuf(char* buf, char* line,char *way);
    void doClient();
    void sendFrame();
    //对外暴露的接口函数，自定义获取图像数据的方法
    virtual int getFrame(uint8_t **frame) = 0;

private:
    int handleCmd_OPTIONS(char* result, int cseq);
    int handleCmd_DESCRIBE(char* result, int cseq, char* url);
    int handleCmd_SETUP(char* result, int cseq, int clientRtpPort);
    int handleCmd_PLAY(char* result, int cseq);
    int handleCmd_TEARDOWN(char* result, int cseq);


};

#endif // RTSP_H
