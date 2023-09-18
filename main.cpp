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

#include "video.h"
#include "rtsp.h"

#define SERVER_PORT      "8554"
#define SERVER_RTP_PORT  "55532"
#define SERVER_RTCP_PORT "55533"

class myrtsp:public rtspInterface{
public:
    myrtsp(){
        v.init("/dev/video0","h264","1280*720","25");
    }

    int getFrame(uint8_t **frame) override{
        return v.getH264Frame(*frame,0);
    }

private:
    video v;
};

int main(int argc, char* argv[]){
//    if(argc != 4){
//        printf("error format!\n");
//        printf("./src [SERVER_PORT] [SERVER_RTP_PORT] [SERVER_RTP_PORT]\n");
//        return 0;
//    }

    myrtsp rtsp;
    //rtsp.initRtsp(argv[1], argv[2], argv[3]);
    rtsp.initRtsp(SERVER_PORT, SERVER_RTP_PORT, SERVER_RTCP_PORT);
    rtsp.startRtspServer();

    return 0;
}
