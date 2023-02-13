#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <opencv2/opencv.hpp>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#define INBUF_SIZE 8192

class H264_Stream_Decode {
public:
    H264_Stream_Decode(FILE* f);
    ~H264_Stream_Decode();
    bool next();
    cv::Mat get_frame();
private:
    void init_BGR();
    bool decode();

private:
    FILE* f;
    const AVCodec* codec;
    AVCodecParserContext* parser;
    AVCodecContext* dec_ctx;
    AVFrame* frame;
    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t* data;
    size_t data_size;
    int eof;
    AVPacket* pkt;
    SwsContext* sws_ctx;
    AVFrame* frame_BGR;
    unsigned char* frame_buffer;
};