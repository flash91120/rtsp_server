#include "rtsp.h"

int rtspInterface::initRtsp(std::string serverRTSPPort,std::string serverRtpPort,std::string serverRtcpPort){

    this->serverRTSPPort = atoi(serverRTSPPort.c_str());
    this->serverRtpPort = atoi(serverRtpPort.c_str());
    this->serverRtcpPort = atoi(serverRtcpPort.c_str());

    serverSockfd = createTcpSocket();
    if(serverSockfd < 0){
        printf("failed to create tcp socket\n");
        return -1;
    }

    if(bindSocketAddr(serverSockfd, "0.0.0.0", atoi(serverRTSPPort.c_str())) < 0){
        printf("failed to bind addr\n");
        return -1;
    }

    if(listen(serverSockfd, 10) < 0){
        printf("failed to listen\n");
        return -1;
    }

    serverRtpSockfd = createUdpSocket();
    serverRtcpSockfd = createUdpSocket();
    if(serverRtpSockfd < 0 || serverRtcpSockfd < 0)
    {
        printf("failed to create udp socket\n");
        return -1;
    }

    if(bindSocketAddr(serverRtpSockfd, "0.0.0.0", atoi(serverRtpPort.c_str())) < 0 ||
            bindSocketAddr(serverRtcpSockfd, "0.0.0.0", atoi(serverRtpPort.c_str())) < 0){
        printf("failed to bind addr\n");
        return -1;
    }

    printf("rtsp://127.0.0.1:%d\n", atoi(serverRTSPPort.c_str()));
}

void rtspInterface::startRtspServer(){

    while(1)
    {
        printf("waiting for client\n");
        acceptClient();

        if(clientSockfd < 0){
            printf("failed to accept client\n");
            return;
        }else {
            printf("clientSockfd is %d;",clientSockfd);
        }

        printf("accept client;client ip:%s,client port:%d\n", (char *)clientIP, clientPort);

        doClient(/*clientSockfd, clientIp, clientPort, serverRtpSockfd, serverRtcpSockfd*/);
    }
}


int rtspInterface::createTcpSocket()
{
    int sockfd;
    int on = 1;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
        return -1;

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

    return sockfd;
}

int rtspInterface::createUdpSocket()
{
    int sockfd;
    int on = 1;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
        return -1;

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

    return sockfd;
}

int rtspInterface::bindSocketAddr(int sockfd, const char* ip, int port)
{
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if(bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0)
        return -1;

    return 0;
}

void rtspInterface::acceptClient()
{
    socklen_t len = 0;
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    len = sizeof(addr);

    clientSockfd = accept(serverSockfd, (struct sockaddr *)&addr, &len);
    if(clientSockfd < 0){
        std::cout << "error to accept client" << std::endl;
        return;
    }

    clientIP = inet_ntoa(addr.sin_addr);
    clientPort = ntohs(addr.sin_port);

}

int rtspInterface::startCode3(char* buf)
{
    if(buf[0] == 0 && buf[1] == 0 && buf[2] == 1)
        return 1;
    else
        return 0;
}

int rtspInterface::startCode4(char* buf)
{
    if(buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1)
        return 1;
    else
        return 0;
}

char* rtspInterface::findNextStartCode(char* buf, int len)
{
    int i;

    if(len < 3)
        return NULL;

    for(i = 0; i < len-3; ++i)
    {
        if(startCode3(buf) || startCode4(buf))
            return buf;

        ++buf;
    }

    if(startCode3(buf))
        return buf;

    return NULL;
}

int rtspInterface::getFrameFromH264File(int fd, char* frame, int size)
{
    int rSize, frameSize;
    char* nextStartCode;

    if(fd < 0)
        return fd;

    rSize = read(fd, frame, size);
    if(!startCode3(frame) && !startCode4(frame))
        return -1;

    nextStartCode = findNextStartCode(frame+3, rSize-3);
    if(!nextStartCode)
    {
        //lseek(fd, 0, SEEK_SET);
        //frameSize = rSize;
        return -1;
    }
    else
    {
        frameSize = (nextStartCode-frame);
        lseek(fd, frameSize-rSize, SEEK_CUR);
    }

    return frameSize;
}

int rtspInterface::rtpSendH264Frame(int socket, const char* ip, int16_t port,
                                    struct RtpPacket* rtpPacket, uint8_t* frame, uint32_t frameSize)
{
    uint8_t naluType; // nalu第一个字节
    int sendBytes = 0;
    int ret;

    naluType = frame[0];

    if (frameSize <= RTP_MAX_PKT_SIZE) // nalu长度小于最大包场：单一NALU单元模式
    {
        /*
         *   0 1 2 3 4 5 6 7 8 9
         *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         *  |F|NRI|  Type   | a single NAL unit ... |
         *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         */
        memcpy(rtpPacket->payload, frame, frameSize);
        ret = rtpSendPacket(socket, (char *)ip, port, rtpPacket, frameSize);
        if(ret < 0)
            return -1;

        rtpPacket->rtpHeader.seq++;
        sendBytes += ret;
        if ((naluType & 0x1F) == 7 || (naluType & 0x1F) == 8) // 如果是SPS、PPS就不需要加时间戳
            goto out;
    }
    else // nalu长度小于最大包场：分片模式
    {
        /*
         *  0                   1                   2
         *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
         * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         * | FU indicator  |   FU header   |   FU payload   ...  |
         * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         */

        /*
         *     FU Indicator
         *    0 1 2 3 4 5 6 7
         *   +-+-+-+-+-+-+-+-+
         *   |F|NRI|  Type   |
         *   +---------------+
         */

        /*
         *      FU Header
         *    0 1 2 3 4 5 6 7
         *   +-+-+-+-+-+-+-+-+
         *   |S|E|R|  Type   |
         *   +---------------+
         */

        int pktNum = frameSize / RTP_MAX_PKT_SIZE;       // 有几个完整的包
        int remainPktSize = frameSize % RTP_MAX_PKT_SIZE; // 剩余不完整包的大小
        int i, pos = 1;

        /* 发送完整的包 */
        for (i = 0; i < pktNum; i++)
        {
            rtpPacket->payload[0] = (naluType & 0x60) | 28;
            rtpPacket->payload[1] = naluType & 0x1F;

            if (i == 0) //第一包数据
                rtpPacket->payload[1] |= 0x80; // start
            else if (remainPktSize == 0 && i == pktNum - 1) //最后一包数据
                rtpPacket->payload[1] |= 0x40; // end

            memcpy(rtpPacket->payload+2, frame+pos, RTP_MAX_PKT_SIZE);
            ret = rtpSendPacket(socket, (char *)ip, port, rtpPacket, RTP_MAX_PKT_SIZE+2);
            if(ret < 0)
                return -1;

            rtpPacket->rtpHeader.seq++;
            sendBytes += ret;
            pos += RTP_MAX_PKT_SIZE;
        }

        /* 发送剩余的数据 */
        if (remainPktSize > 0)
        {
            rtpPacket->payload[0] = (naluType & 0x60) | 28;
            rtpPacket->payload[1] = naluType & 0x1F;
            rtpPacket->payload[1] |= 0x40; //end

            memcpy(rtpPacket->payload+2, frame+pos, remainPktSize+2);
            ret = rtpSendPacket(socket, (char *)ip, port, rtpPacket, remainPktSize+2);
            if(ret < 0)
                return -1;

            rtpPacket->rtpHeader.seq++;
            sendBytes += ret;
        }
    }

out:

    return sendBytes;
}

/**
 * @brief getLineFromBuf
 * @param buf
 * @param line
 * @return
 */
char* rtspInterface::getLineFromBuf(char* buf, char* line,char *way)
{
    char *p = buf;
    char *p1 = line;
    if(way == "method"){//返回第一行
        while(*p != '\n')
        {
            *line = *p;
            line++;
            p++;
        }
        *line = '\n';
        ++line;
        *line = '\0';
        line = p1;
        return p1;
    }

    char * start = strstr(buf,way);//查找buf中way首次出现的地址
    p = start;
    while(*p != '\n')
    {
        *line = *p;
        line++;
        p++;
    }
    *line = '\n';
    ++line;
    *line = '\0';
    line = p1;
    return p1;
}

int rtspInterface::handleCmd_OPTIONS(char* result, int cseq)
{
    sprintf(result, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %d\r\n"
                    "Public: OPTIONS, DESCRIBE, SETUP, PLAY\r\n"
                    "\r\n",
            cseq);

    return 0;
}

int rtspInterface::handleCmd_DESCRIBE(char* result, int cseq, char* url)
{
    char sdp[500] = {0};
    char localIp[100] = {0};

    sscanf(url, "rtsp://%[^:]:", localIp);

    sprintf(sdp, "v=0\r\n"
                 "o=- 9%ld 1 IN IP4 %s\r\n"
                 "t=0 0\r\n"
                 "a=control:*\r\n"
                 "m=video 0 RTP/AVP 96\r\n"
                 "a=rtpmap:96 H264/90000\r\n"
                 "a=control:track0\r\n",
            time(NULL), localIp);

    sprintf(result, "RTSP/1.0 200 OK\r\nCSeq: %d\r\n"
                    "Content-Base: %s\r\n"
                    "Content-type: application/sdp\r\n"
                    "Content-length: %d\r\n\r\n"
                    "%s",
            cseq,
            url,
            strlen(sdp),
            sdp);

    return 0;
}

int rtspInterface::handleCmd_SETUP(char* result, int cseq, int clientRtpPort)
{
    sprintf(result, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %d\r\n"
                    "Transport: RTP/AVP/UDP;unicast;client_port=%d-%d;server_port=%d-%d\r\n"
                    "Session: 66334873\r\n"
                    "\r\n",
            cseq,
            clientRtpPort,
            clientRtpPort+1,
            serverRtpPort,
            serverRtcpPort);

    return 0;
}

int rtspInterface::handleCmd_PLAY(char* result, int cseq)
{
    sprintf(result, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %d\r\n"
                    "Range: npt=0.000-\r\n"
                    "Session: 66334873; timeout=0\r\n\r\n",
            cseq);

    return 0;
}

int rtspInterface::handleCmd_TEARDOWN(char* result, int cseq)
{
    sprintf(result, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %d\r\n"
                    "Range: npt=0.000-\r\n"
                    "Session: 66334873; timeout=0\r\n\r\n",
            cseq);

    return 0;
}


void rtspInterface::sendFrame(){
    frame = (uint8_t *)malloc(500000);
    rtpPacket = (struct RtpPacket*)malloc(500000);

    char lastData[1024] = {0};
    int pos = 0;
    int flag = 0;
    //int fd = open(H264_FILE_NAME, O_RDONLY);
    //assert(fd > 0);
    rtpHeaderInit(rtpPacket, 0, 0, 0, RTP_VESION, RTP_PAYLOAD_TYPE_H264, 0,
                  0, 0, 0x88923423);

    printf("start play\n");
    printf("client ip:%s\n", clientIP);
    printf("client port:%d\n", clientRtpPort);

    struct sockaddr_in clientAddr;
    socklen_t sockLen = sizeof (clientAddr);
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(clientPort);
    clientAddr.sin_addr.s_addr = inet_addr(clientIP);

    while (1){
        memset(rBuf,0,BUF_MAX_SIZE);
        recvLen = recvfrom(clientSockfd,rBuf,sizeof (rBuf)-1,MSG_DONTWAIT,(struct sockaddr *)&clientAddr,(socklen_t *)&sockLen);
        if(recvLen > 0){
            memcpy(lastData+pos,rBuf,recvLen);
            pos += recvLen;
            while (1) {
                recvLen = recvfrom(clientSockfd,rBuf,sizeof (rBuf)-1,MSG_DONTWAIT,(struct sockaddr *)&clientAddr,(socklen_t *)&sockLen);
                if(recvLen <= 0){
                    break;
                }
                memcpy(lastData+pos,rBuf,recvLen);
                pos += recvLen;
            }
            flag = 1;
        }
        if(flag){
            lastData[pos] = '\0';
            printf("---------------C->S--------------\n");
            printf("%s", lastData);
            /* 解析方法 */
            bufPtr = getLineFromBuf(lastData, line,"method");
            if(sscanf(line, "%s %s %s\r\n", method, url, version) != 3)
            {
                printf("parse err\n");
                //goto out;
            }
            /* 解析序列号 */
            bufPtr = getLineFromBuf(lastData, line,"CSeq");
            if(sscanf(line, "CSeq: %d\r\n", &cseq) != 1)
            {
                printf("parse err\n");
                //goto out;
            }
            if(!strcmp(method, "TEARDOWN")){
                std::cout << "a client is offline" << std::endl;
                break;
            }
            if(!strcmp(method, "OPTIONS")){
                if(handleCmd_OPTIONS(sBuf, cseq)){
                    printf("failed to handle options\n");
                    break;
                }
                printf("---------------S->C--------------\n");
                printf("%s", sBuf);
                send(clientSockfd, sBuf, strlen(sBuf), 0);
            }
            memset(lastData,0,1024);
            pos = 0;
            flag = 0;
        }

        //frameSize = getFrameFromH264File(fd, frame, 500000);
        frameSize = getFrame(&frame);
        //file f;
        //f.writeStream("../out/aa.h264",frame,frameSize);
        if(frameSize < 0)
        {
            std::cout << "Discard incomplete frames" << std::endl;
            continue;
            //break;
        }

        if(startCode3((char *)frame))
            startCode = 3;
        else
            startCode = 4;

        frameSize -= startCode;
        rtpSendH264Frame(serverRtpSockfd, clientIP, clientRtpPort,
                         rtpPacket, (uint8_t *)frame+startCode, frameSize);
        rtpPacket->rtpHeader.timestamp += 90000/25;

        //usleep(1000*1000/25);
    }
    memset(rBuf,0,BUF_MAX_SIZE);
    memset(sBuf,0,BUF_MAX_SIZE);
    if(frame){
        free(frame);
    }    if(rtpPacket){
        free(rtpPacket);
    }
}

void rtspInterface::doClient(){
    rBuf = (char *)malloc(BUF_MAX_SIZE);
    sBuf = (char *)malloc(BUF_MAX_SIZE);
    while(1){
        int recvLen;

        recvLen = recv(clientSockfd, rBuf, BUF_MAX_SIZE, 0);
        if(recvLen <= 0){
            printf("err to recv msg from client\n");
            goto out;
        }

        rBuf[recvLen] = '\0';
        printf("---------------C->S--------------\n");
        printf("%s", rBuf);

        /* 解析方法 */
        bufPtr = getLineFromBuf(rBuf, line,(char*)"method");
        if(sscanf(line, "%s %s %s\r\n", method, url, version) != 3)
        {
            printf("parse err\n");
            goto out;
        }

        /* 解析序列号 */
        bufPtr = getLineFromBuf(rBuf, line,(char *)"CSeq");
        if(sscanf(line, "CSeq: %d\r\n", &cseq) != 1)
        {
            printf("parse err\n");
            goto out;
        }

        /* 如果是SETUP，那么就先解析client_port */
        if(!strcmp(method, "SETUP"))
        {
            while(1)
            {
                bufPtr = getLineFromBuf(rBuf, line,(char *)"Transport");
                if(!strncmp(line, "Transport:", strlen("Transport:")))
                {
                    if(sscanf(line, "Transport: RTP/AVP/UDP;unicast;client_port=%d-%d\r\n",
                              &clientRtpPort, &clientRtcpPort)){
                        break;
                    }
                    if(sscanf(line, "Transport: RTP/AVP;unicast;client_port=%d-%d\r\n",
                              &clientRtpPort, &clientRtcpPort)){
                        break;
                    }
                }
            }
        }

        if(!strcmp(method, "OPTIONS"))
        {
            if(handleCmd_OPTIONS(sBuf, cseq))
            {
                printf("failed to handle options\n");
                goto out;
            }
        }
        else if(!strcmp(method, "DESCRIBE"))
        {
            if(handleCmd_DESCRIBE(sBuf, cseq, url))
            {
                printf("failed to handle describe\n");
                goto out;
            }
        }
        else if(!strcmp(method, "SETUP"))
        {
            if(handleCmd_SETUP(sBuf, cseq, clientRtpPort))
            {
                printf("failed to handle setup\n");
                goto out;
            }
        }
        else if(!strcmp(method, "PLAY"))
        {
            if(handleCmd_PLAY(sBuf, cseq))
            {
                printf("failed to handle play\n");
                goto out;
            }
        }
        else
        {
            goto out;
        }

        printf("---------------S->C--------------\n");
        printf("%s", sBuf);
        send(clientSockfd, sBuf, strlen(sBuf), 0);

        /* 开始播放，发送RTP包 */
        if(!strcmp(method, "PLAY")){
            sendFrame();
            printf("sendThread is finish\n");
            goto out;
        }
    }
out:
    printf("do client is finished\n");
    close(clientSockfd);
    clientSockfd  = -1;
    if(rBuf){
        free(rBuf);
        rBuf = nullptr;
    }
    if(sBuf){
        free(sBuf);
        sBuf = nullptr;
    }

}
