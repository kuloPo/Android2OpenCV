#include "h264.h"

#include <stdexcept>

H264_Stream_Decode::H264_Stream_Decode(FILE* f) {
	this->f = f;
    memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    pkt = av_packet_alloc();
    if (!pkt) {
        throw std::runtime_error("av_packet_alloc() failed");
    }

    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        throw std::runtime_error("Codec not found");
    }

    parser = av_parser_init(codec->id);
    if (!parser) {
        throw std::runtime_error("parser not found");
    }

    dec_ctx = avcodec_alloc_context3(codec);
    if (!dec_ctx) {
        throw std::runtime_error("Could not allocate video codec context");
    }

    if (avcodec_open2(dec_ctx, codec, NULL) < 0) {
        throw std::runtime_error("Could not open codec");
    }

    frame = av_frame_alloc();
    if (!frame) {
        throw std::runtime_error("Could not allocate video frame");
    }

    sws_ctx = nullptr;
    frame_BGR = nullptr;
    frame_buffer = nullptr;
    data_size = 0;
}

H264_Stream_Decode::~H264_Stream_Decode() {
    av_parser_close(parser);
    avcodec_free_context(&dec_ctx);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    sws_freeContext(sws_ctx);
    av_frame_free(&frame_BGR);
    av_free(frame_buffer);
}

bool H264_Stream_Decode::decode() {
    int ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        throw std::runtime_error("Error sending a packet for decoding");
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return false;
        }
        else if (ret < 0) {
            throw std::runtime_error("Error during decoding");
        }

        return true;
    }
}

bool H264_Stream_Decode::next() {
    do {
        if (!data_size) {
            data_size = fread(inbuf, 1, INBUF_SIZE, f);
            eof = !data_size;

            data = inbuf;
        }
        while (data_size > 0 || eof) {
            ret = av_parser_parse2(parser, dec_ctx, &pkt->data, &pkt->size,
                data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (ret < 0) {
                throw std::runtime_error("Error while parsing");
            }
            data += ret;
            data_size -= ret;

            if (pkt->size) {
                if (decode()) {
                    return true;
                }
                else {
                    continue;
                }
            }
            else if (eof)
                return false;
        }
    } while (!eof);
}

cv::Mat H264_Stream_Decode::get_frame() {
    if (!sws_ctx) {
        init_BGR();
    }
    sws_scale(sws_ctx, (uint8_t const* const*)frame->data,
        frame->linesize, 0, dec_ctx->height,
        frame_BGR->data, frame_BGR->linesize);
    cv::Mat img(frame->height, frame->width, CV_8UC3, frame_BGR->data[0]);
    return img;
}

void H264_Stream_Decode::init_BGR() {
    sws_ctx = sws_getContext
    (
        dec_ctx->width,
        dec_ctx->height,
        dec_ctx->pix_fmt,
        dec_ctx->width,
        dec_ctx->height,
        AV_PIX_FMT_BGR24,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );
    if (!sws_ctx) {
        throw std::runtime_error("Could not get sws context");
    }

    frame_BGR = av_frame_alloc();
    if (!frame_BGR) {
        throw std::runtime_error("Could not allocate BGR frame");
    }

    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, dec_ctx->width, dec_ctx->height, 1);
    frame_buffer = (uint8_t*)av_malloc(num_bytes);
    int response = av_image_fill_arrays(frame_BGR->data,
        frame_BGR->linesize,
        frame_buffer,
        AV_PIX_FMT_BGR24,
        dec_ctx->width,
        dec_ctx->height,
        1);

    if (response < 0) {
        throw std::runtime_error("av_image_fill_arrays() Failed");
    }
    frame_BGR->width = dec_ctx->width;
    frame_BGR->height = dec_ctx->height;
}
