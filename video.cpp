#include "video.h"

int video::init(std::string cam_url,std::string input_format,std::string video_size,std::string framerate){
    avdevice_register_all();
    url = cam_url;
    // 1、打开视频文件
    options = nullptr;
    input_fmt = const_cast<AVInputFormat*>(av_find_input_format("video4linux2"));//const_cast用来去掉const属性
    fmt_ctx = avformat_alloc_context();
    if(input_fmt == nullptr) {
        printf("Could not find video4linux2\n");
        exit(1);
    }

    av_dict_set(&options, "input_format", input_format.c_str(), 0); //指定输入格式
    av_dict_set(&options, "video_size", video_size.c_str(), 0);//设置流的分辨率
    av_dict_set(&options, "framerate", framerate.c_str(), 0);//设置流的帧率

    int err_code = avformat_open_input(&fmt_ctx, url.c_str(), input_fmt, &options);//打开摄像头
    if (err_code < 0) {
        av_strerror(err_code, buf, 1024);
        //qDebug("Couldn't open file %s: %d(%s)", "test_file", err_code, buf);
        std::cout << "ErrorCode : " << err_code << buf << "\n";
        std::cout << " Could not open camera" << url << "\n";
        exit(1);
    } else {
        std::cout << "successful open video device" << url << "\n";
    }


    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        std::cout << "Could not find stream information\n";
        exit(1);
    }
    av_dump_format(fmt_ctx, 0, "video4linux2", 0);//显示摄像头即将采集的图片信息
}

/**
 * @brief video::ifSps
 * @param nalu 输入nalu的地址判断（包括nalu头，完整的nalu）
 * @param len
 * @return
 */
int video::ifSps(uint8_t *nalu,int len){
    return (nalu[0] & 0b00011111) == 6;
}

/**
 * @brief video::findSps
 * @param sp 从sp之后查找SPS
 * @param len
 * @return 返回sp之后的第一个Sps的位置
 */
uint8_t * video::findSps(uint8_t *sp,int len){

}

int video::startCode4(char* buf)
{
    if(buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1)
        return 1;
    else
        return 0;
}
/**
 * @brief video::getH264Frame
 * @param frame
 * @param len
 * @return
 */
int video::getH264Frame(uint8_t *frame,int len){
    av_init_packet(&avpkt);

    av_read_frame(fmt_ctx, &avpkt);//经过测试，第一次读取出来的avpkt是不完整的，需要舍去

    //每一个avpkt都是包含0x0000 0001的，包含nalu和nalu之间的间隔
    //fileIns.writeStream("../out/h264/stream.h264",(char *)avpkt.data,avpkt.size);
    if(!startCode4((char *)avpkt.buf->data)){//测试发现h264摄像头中nalu单元中间隔的都是4字节的长度
        std::cout << "a incomplete frame" << std::endl;
        return -1;
    }
    memset(frame,0,avpkt.buf->size);
    memcpy(frame,(char *)avpkt.buf->data,avpkt.buf->size);
    return avpkt.buf->size;
}

video::video(){

}
