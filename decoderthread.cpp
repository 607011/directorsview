// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QtCore/QDebug>

#include "decoderthread.h"
#include "util.h"
#include "semaphores.h"

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
}

class DecoderThreadPrivate {
public:
    DecoderThreadPrivate(void)
        : doAbort(false)
        , fmt_ctx(nullptr)
        , video_dec_ctx(nullptr)
        , w(0)
        , h(0)
        , video_stream_idx(-1)
        , video_stream(nullptr)
        , img_convert_ctx(nullptr)
        , video_dst_bufsize(0)
        , video_frame_count(0)
        , frame(av_frame_alloc())
        , frameRGB(av_frame_alloc())
        , frameBuffer(nullptr)
    {
        memset(video_dst_data, 0, 4 * sizeof(uint8_t *));
        memset(video_dst_linesize, 0, 4 * sizeof(int));
    }
    bool doAbort;
    AVFormatContext *fmt_ctx;
    AVCodecContext *video_dec_ctx;
    int w;
    int h;
    int video_stream_idx;
    AVStream *video_stream;
    SwsContext *img_convert_ctx;
    int video_dst_bufsize;
    int video_frame_count;
    AVFrame *frame;
    AVFrame *frameRGB;
    uint8_t *frameBuffer;
    AVPacket pkt;
    uint8_t *video_dst_data[4];
    int video_dst_linesize[4];
    int rc;

    ~DecoderThreadPrivate() {
        av_frame_free(&frame);
        av_frame_free(&frameRGB);
        av_freep(&video_dst_data[0]);
    }
};


DecoderThread::DecoderThread(QObject *parent)
    : QThread(parent)
    , d_ptr(new DecoderThreadPrivate)
{
    av_register_all();
    qDebug() << "AVCodec version:" << avformat_version();
    qDebug() << "AVFormat configuration:" << avformat_configuration();
}


bool DecoderThread::openVideo(const QString &filename)
{
    Q_D(DecoderThread);
    const std::string &filename_str = filename.toStdString();
    const char *src_filename = filename_str.c_str();
    d->rc = avformat_open_input(&d->fmt_ctx, src_filename, nullptr, nullptr);
    if (d->rc < 0)
        return false;
    emit durationChanged(1000 * d->fmt_ctx->duration / AV_TIME_BASE);
    d->rc = avformat_find_stream_info(d->fmt_ctx, nullptr);
    if (d->rc < 0)
        return false;
    d->rc = av_find_best_stream(d->fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (d->rc < 0)
        return false;
    d->video_stream_idx = d->rc;
    d->video_stream = d->fmt_ctx->streams[d->video_stream_idx];
    if (!d->video_stream)
        return false;
    d->video_dec_ctx = d->video_stream->codec;
    AVStream *st = d->fmt_ctx->streams[d->video_stream_idx];
    AVCodecContext *dec_ctx = st->codec;
    AVCodec *dec = avcodec_find_decoder(dec_ctx->codec_id);
    if (!dec)
        return false;
    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "refcounted_frames", "1", 0);
    d->rc = avcodec_open2(dec_ctx, dec, &opts);
    if (d->rc < 0)
        return false;
    d->w = d->video_dec_ctx->width;
    d->h = d->video_dec_ctx->height;
    d->video_dst_bufsize = av_image_alloc(
                d->video_dst_data, d->video_dst_linesize,
                d->w, d->h, d->video_dec_ctx->pix_fmt, 1);
    av_dump_format(d->fmt_ctx, 0, src_filename, 0);
    const int numBytes = avpicture_get_size(AV_PIX_FMT_ARGB, d->w, d->h);
    d->frameBuffer = new uint8_t[numBytes];
    avpicture_fill((AVPicture *)d->frameRGB, d->frameBuffer, AV_PIX_FMT_RGB24, d->w, d->h);
    return true;
}


void DecoderThread::abort(void)
{
    Q_D(DecoderThread);
    d->doAbort = true;
    wait(5*1000);
}


void DecoderThread::run(void)
{
    Q_D(DecoderThread);
    d->doAbort = false;
    av_init_packet(&d->pkt);
    d->pkt.data = nullptr;
    d->pkt.size = 0;
    d->rc = 0;
    int got_frame = 0;
    while (!d->doAbort) {
        AVPacket orig_pkt = d->pkt;
        d->rc = av_read_frame(d->fmt_ctx, &d->pkt);
        if (d->rc < 0)
            break;
        do {
            int ret = decode_packet(got_frame, false);
            if (ret < 0)
                break;
            d->pkt.data += ret;
            d->pkt.size -= ret;
        } while (d->pkt.size > 0);
        av_free_packet(&orig_pkt);
    }
    d->pkt.data = nullptr;
    d->pkt.size = 0;
    do {
        decode_packet(got_frame, true);
    } while (!d->doAbort && got_frame);
}


int DecoderThread::decode_packet(int &got_frame, bool cached)
{
    Q_D(DecoderThread);
    Q_UNUSED(cached);
    static const AVRational millisecondbase = {1, 1000};
    int ret = 0;
    int decoded = d->pkt.size;
    got_frame = 0;
    ret = avcodec_decode_video2(d->video_dec_ctx, d->frame, &got_frame, &d->pkt);
    if (ret < 0)
        return ret;
    if (got_frame) {
        qint64 t = av_rescale_q(d->pkt.pts, d->fmt_ctx->streams[d->pkt.stream_index]->time_base, millisecondbase);
        emit positionChanged(t);
        ++d->video_frame_count;
        d->img_convert_ctx = sws_getCachedContext(d->img_convert_ctx, d->w, d->h, d->video_dec_ctx->pix_fmt, d->w, d->h, AV_PIX_FMT_BGRA, SWS_BICUBIC, nullptr, nullptr, nullptr);
        if (d->img_convert_ctx == nullptr) {
            qFatal("Cannot initialize the conversion context!");
            return -1;
        }
        sws_scale(d->img_convert_ctx, d->frame->data, d->frame->linesize, 0, d->h, d->frameRGB->data, d->frameRGB->linesize);
        QImage img(d->w, d->h, QImage::Format_ARGB32);
        for (int y = 0; y < d->h; ++y) {
            uint8_t *const dst  = img.scanLine(y);
            const uint8_t *src = d->frameRGB->data[0] + y * d->frameRGB->linesize[0];
            memcpy(dst, src, 4 * d->w);
        }
        av_frame_unref(d->frame);
        gFramesProduced.acquire();
        emit frameReady(img);
    }
    return decoded;
}
