/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <libavformat/avformat.h>
#include <stdio.h>
#include <string.h>

//#include "dbg.h"
#include "payload.h"
#include "dechunkiser_iface.h"

struct output_stream {
  const char *output_format;
  const char *output_file;
  int64_t prev_pts, prev_dts;
  AVFormatContext *outctx;
};

static struct output_stream out;

static enum CodecID libav_codec_id(uint8_t mytype)
{
  switch (mytype) {
    case 1:
      return CODEC_ID_MPEG2VIDEO;
    case 2:
      return CODEC_ID_H261;
    case 3:
      return CODEC_ID_H263P;
    case 4:
      return CODEC_ID_MJPEG;
    case 5:
      return CODEC_ID_MPEG4;
    case 6:
      return CODEC_ID_FLV1;
    case 7:
      return CODEC_ID_SVQ3;
    case 8:
      return CODEC_ID_DVVIDEO;
    case 9:
      return CODEC_ID_H264;
    case 10:
      return CODEC_ID_THEORA;
    case 11:
      return CODEC_ID_SNOW;
    case 12:
      return CODEC_ID_VP6;
    case 13:
      return CODEC_ID_DIRAC;
    default:
      fprintf(stderr, "Unknown codec %d\n", mytype);
      return 0;
  }
}

static AVFormatContext *format_init(struct output_stream *o, const uint8_t *data)
{
  AVFormatContext *of;
  AVCodecContext *c;
  AVOutputFormat *outfmt;
  int width, height, frame_rate_n, frame_rate_d;
  uint8_t codec;

  av_register_all();

  payload_header_parse(data, &codec, &width, &height, &frame_rate_n, &frame_rate_d);
  //dprintf("Frame size: %dx%d -- Frame rate: %d / %d\n", width, height, frame_rate_n, frame_rate_d);
  outfmt = av_guess_format(o->output_format, o->output_file, NULL);
  of = avformat_alloc_context();
  if (of == NULL) {
    return NULL;
  }
  of->oformat = outfmt;
  av_new_stream(of, 0);
  c = of->streams[0]->codec;
  c->codec_id = libav_codec_id(codec);
  c->codec_type = CODEC_TYPE_VIDEO;
  c->width = width;
  c->height= height;
  c->time_base.den = frame_rate_n;
  c->time_base.num = frame_rate_d;
  of->streams[0]->avg_frame_rate.num = frame_rate_n;
  of->streams[0]->avg_frame_rate.den = frame_rate_d;
  c->pix_fmt = PIX_FMT_YUV420P;

  o->prev_pts = 0;
  o->prev_dts = 0;

  return of;
}

static struct output_stream *avf_init(const char *config)
{
  out.output_format = "nut";
  if (config) {
    out.output_file = strdup(config);
  } else {
    out.output_file = "/dev/stdout";
  }
  out.outctx = NULL;

  return &out;
}

static void avf_write(struct output_stream *o, int id, uint8_t *data, int size)
{
  const int header_size = VIDEO_PAYLOAD_HEADER_SIZE; 
  int frames, i;
  uint8_t *p;

  if (data[0] > 127) {
    fprintf(stderr, "Error! Non video chunk: %x!!!\n", data[0]);
    return;
  }
  if (o->outctx == NULL) {
    o->outctx = format_init(o, data);
    if (o->outctx == NULL) {
      fprintf(stderr, "Format init failed\n");

      return;
    }
    av_set_parameters(o->outctx, NULL);
    snprintf(o->outctx->filename, sizeof(o->outctx->filename), "%s", o->output_file);
    dump_format(o->outctx, 0, o->output_file, 1);
    url_fopen(&o->outctx->pb, o->output_file, URL_WRONLY);
    av_write_header(o->outctx);
  }

  frames = data[header_size - 1];
  p = data + header_size + FRAME_HEADER_SIZE * frames;
  for (i = 0; i < frames; i++) {
    AVPacket pkt;
    int64_t pts, dts;
    int frame_size;

    frame_header_parse(data + header_size + FRAME_HEADER_SIZE * i,
                       &frame_size, &pts, &dts);
    //dprintf("Frame %d PTS1: %d\n", i, pts);
    av_init_packet(&pkt);
    pkt.stream_index = 0;	// FIXME!
    if (pts != -1) {
      pts += (pts < o->prev_pts - ((1LL << 31) - 1)) ? ((o->prev_pts >> 32) + 1) << 32 : (o->prev_pts >> 32) << 32;
      //dprintf(" PTS2: %d\n", pts);
      o->prev_pts = pts;
      //dprintf("Frame %d has size %d --- PTS: %lld DTS: %lld\n", i, frame_size,
      //                                       av_rescale_q(pts, outctx->streams[0]->codec->time_base, AV_TIME_BASE_Q),
      //                                       av_rescale_q(dts, outctx->streams[0]->codec->time_base, AV_TIME_BASE_Q));
      pkt.pts = av_rescale_q(pts, o->outctx->streams[0]->codec->time_base, o->outctx->streams[0]->time_base);
    } else {
      pkt.pts = AV_NOPTS_VALUE;
    }
    dts += (dts < o->prev_dts - ((1LL << 31) - 1)) ? ((o->prev_dts >> 32) + 1) << 32 : (o->prev_dts >> 32) << 32;
    o->prev_dts = dts;
    pkt.dts = av_rescale_q(dts, o->outctx->streams[0]->codec->time_base, o->outctx->streams[0]->time_base);
    pkt.data = p;
    p += frame_size;
    pkt.size = frame_size;
    av_interleaved_write_frame(o->outctx, &pkt);
  }
}

struct dechunkiser_iface out_avf = {
  .open = avf_init,
  .write = avf_write,
};
