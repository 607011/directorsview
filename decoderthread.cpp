// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QtCore/QDebug>

#include "decoderthread.h"
#include "util.h"
#include "semaphores.h"

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
}

class DecoderThreadPrivate {
public:
    explicit DecoderThreadPrivate(void)
        : doAbort(false)
        , fmtCtx(nullptr)
        , videoDecCtx(nullptr)
        , videoEncCtx(nullptr)
        , videoEncBitrate(0)
        , w(0)
        , h(0)
        , videoStreamIdx(-1)
        , videoStream(nullptr)
        , imgConvertCtx(nullptr)
        , videoDstBufsize(0)
        , videoFrameCount(0)
        , frame(av_frame_alloc())
        , frameRGB(av_frame_alloc())
        , frameEnc(av_frame_alloc())
        , frameBuffer(nullptr)
    {
        memset(videoDstData, 0, 4 * sizeof(uint8_t *));
        memset(videoDstLinesize, 0, 4 * sizeof(int));
    }
    bool doAbort;
    AVFormatContext *fmtCtx;
    AVCodecContext *videoDecCtx;
    AVCodecContext *videoEncCtx;
    int videoEncBitrate;
    int w;
    int h;
    int videoStreamIdx;
    AVStream *videoStream;
    SwsContext *imgConvertCtx;
    int videoDstBufsize;
    int videoFrameCount;
    AVFrame *frame;
    AVFrame *frameRGB;
    AVFrame *frameEnc;
    uint8_t *frameBuffer;
    AVPacket pkt;
    AVPacket encPkt;
    uint8_t *videoDstData[4];
    int videoDstLinesize[4];
    int rc;

    virtual ~DecoderThreadPrivate() {
        av_frame_free(&frame);
        av_frame_free(&frameRGB);
        av_freep(&videoDstData[0]);
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
    d->rc = avformat_open_input(&d->fmtCtx, src_filename, nullptr, nullptr);
    if (d->rc < 0)
        return false;
    emit durationChanged(1000 * d->fmtCtx->duration / AV_TIME_BASE);
    d->rc = avformat_find_stream_info(d->fmtCtx, nullptr);
    if (d->rc < 0)
        return false;
    d->rc = av_find_best_stream(d->fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (d->rc < 0)
        return false;
    d->videoStreamIdx = d->rc;
    d->videoStream = d->fmtCtx->streams[d->videoStreamIdx];
    if (!d->videoStream)
        return false;
    d->videoDecCtx = d->videoStream->codec;
    AVStream *st = d->fmtCtx->streams[d->videoStreamIdx];
    AVCodecContext *dec_ctx = st->codec;
    AVCodec *dec = avcodec_find_decoder(dec_ctx->codec_id);
    if (!dec)
        return false;
    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "refcounted_frames", "1", 0);
    d->rc = avcodec_open2(dec_ctx, dec, &opts);
    if (d->rc < 0)
        return false;
    d->w = d->videoDecCtx->width;
    d->h = d->videoDecCtx->height;
    d->videoDstBufsize = av_image_alloc(
                d->videoDstData, d->videoDstLinesize,
                d->w, d->h, d->videoDecCtx->pix_fmt, 1);
    av_dump_format(d->fmtCtx, 0, src_filename, 0);
    const int numBytes = avpicture_get_size(AV_PIX_FMT_ARGB, d->w, d->h);
    d->frameBuffer = new uint8_t[numBytes];
    avpicture_fill((AVPicture *)d->frameRGB, d->frameBuffer, AV_PIX_FMT_RGB24, d->w, d->h);
    return true;
}


void DecoderThread::abort(void)
{
    Q_D(DecoderThread);
    d->doAbort = true;
    // terminate();
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
    int gotFrame = 0;
    while (!d->doAbort) {
        AVPacket origPkt = d->pkt;
        d->rc = av_read_frame(d->fmtCtx, &d->pkt);
        if (d->rc < 0)
            break;
        do {
            int ret = decodePacket(gotFrame);
            if (ret < 0)
                break;
            d->pkt.data += ret;
            d->pkt.size -= ret;
        } while (d->pkt.size > 0);
        av_free_packet(&origPkt);
    }
    qDebug().nospace() << "DecoderThread " << (d->doAbort ? "canceled" : "finished decoding uncached frames") << ".";
    if (!d->doAbort) {
        d->pkt.data = nullptr;
        d->pkt.size = 0;
        do  {
            decodePacket(gotFrame);
        } while (gotFrame && !d->doAbort);
    }
    qDebug() << "DecoderThread ending ...";
}


int DecoderThread::decodePacket(int &gotFrame)
{
    Q_D(DecoderThread);
    static const AVRational ms = {1, 1000};
    int ret = 0;
    int decoded = d->pkt.size;
    gotFrame = 0;
    ret = avcodec_decode_video2(d->videoDecCtx, d->frame, &gotFrame, &d->pkt);
    if (ret < 0)
        return ret;
    if (gotFrame) {
        qint64 t = av_rescale_q(d->pkt.pts, d->fmtCtx->streams[d->pkt.stream_index]->time_base, ms);
        emit positionChanged(t);
        d->imgConvertCtx = sws_getCachedContext(d->imgConvertCtx, d->w, d->h, d->videoDecCtx->pix_fmt, d->w, d->h, AV_PIX_FMT_BGRA, SWS_BICUBIC, nullptr, nullptr, nullptr);
        if (d->imgConvertCtx == nullptr) {
            qFatal("Cannot initialize the conversion context!");
            return -1;
        }
        sws_scale(d->imgConvertCtx, d->frame->data, d->frame->linesize, 0, d->h, d->frameRGB->data, d->frameRGB->linesize);
        QImage img(d->w, d->h, QImage::Format_ARGB32);
        for (int y = 0; y < d->h; ++y) {
            uint8_t *const dst  = img.scanLine(y);
            if (!dst)
                qFatal("Out of memory error.");
            const uint8_t *src = d->frameRGB->data[0] + y * d->frameRGB->linesize[0];
            memcpy(dst, src, 4 * d->w);
        }
        av_frame_unref(d->frame);
        gFramesProduced.acquire();
        emit frameReady(img, d->videoFrameCount);
        ++d->videoFrameCount;
    }
    return decoded;
}
