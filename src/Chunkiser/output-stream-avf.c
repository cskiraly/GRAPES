/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <libavformat/avformat.h>
#include <stdio.h>
#include <string.h>

//#include "dbg.h"
#include "int_coding.h"
#include "payload.h"
#include "config.h"
#include "ffmpeg_compat.h"
#include "dechunkiser_iface.h"

struct dechunkiser_ctx {
  enum CodecID video_codec_id;
  enum CodecID audio_codec_id;
  int streams;
  int selected_streams;
  int width, height;
  int channels;
  int sample_rate, frame_size;
  AVRational video_time_base;
  AVRational audio_time_base;

  char *output_format;
  char *output_file;
  int64_t prev_pts, prev_dts;
  AVFormatContext *outctx;
};

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
    case 129:
      return CODEC_ID_MP3;
    case 130:
      return CODEC_ID_AAC;
    case 131:
      return CODEC_ID_AC3;
    case 132:
      return CODEC_ID_VORBIS;
    default:
      fprintf(stderr, "Unknown codec %d\n", mytype);
      return 0;
  }
}
  
static AVFormatContext *format_init(struct dechunkiser_ctx *o)
{
  AVFormatContext *of;
  AVOutputFormat *outfmt;
  
  av_register_all();

  outfmt = av_guess_format(o->output_format, o->output_file, NULL);
  of = avformat_alloc_context();
  if (of == NULL) {
    return NULL;
  }
  of->oformat = outfmt;

  return of;
}

static AVFormatContext *format_gen(struct dechunkiser_ctx *o, const uint8_t *data)
{
  uint8_t codec;

  codec = data[0];
  if ((codec < 128) && ((o->streams & 0x01) == 0)) {
    int width, height, frame_rate_n, frame_rate_d;

    o->streams |= 0x01;
    o->video_codec_id = libav_codec_id(codec);
    video_payload_header_parse(data, &codec, &width, &height, &frame_rate_n, &frame_rate_d);
    o->width = width;
    o->height= height;
    o->video_time_base.den = frame_rate_n;
    o->video_time_base.num = frame_rate_d;
  } else if ((codec > 128) && ((o->streams & 0x02) == 0)) {
    uint8_t channels;
    int sample_rate, frame_size;

    audio_payload_header_parse(data, &codec, &channels, &sample_rate, &frame_size);
    o->streams |= 0x02;
    o->audio_codec_id = libav_codec_id(codec);
    o->sample_rate = sample_rate;
    o->channels = channels;
    o->frame_size = frame_size;
    o->audio_time_base.num = frame_size;
    o->audio_time_base.den = sample_rate;
  }

  if (o->streams == o->selected_streams) {
    AVCodecContext *c;

    o->outctx = format_init(o);
    if (o->streams & 0x01) {
      av_new_stream(o->outctx, 0);
      c = o->outctx->streams[o->outctx->nb_streams - 1]->codec;
      c->codec_id = o->video_codec_id;
      c->codec_type = CODEC_TYPE_VIDEO;
      c->width = o->width;
      c->height= o->height;
      c->time_base.den = o->video_time_base.den;
      c->time_base.num = o->video_time_base.num;
      o->outctx->streams[0]->avg_frame_rate.num = o->video_time_base.den;
      o->outctx->streams[0]->avg_frame_rate.den = o->video_time_base.num;
      c->pix_fmt = PIX_FMT_YUV420P;
    }
    if (o->streams & 0x02) {
      av_new_stream(o->outctx, 1);
      c = o->outctx->streams[o->outctx->nb_streams - 1]->codec;
      c->codec_id = o->audio_codec_id;
      c->codec_type = CODEC_TYPE_AUDIO;
      c->sample_rate = o->sample_rate;
      c->channels = o->channels;
      c->frame_size = o->frame_size;
      c->time_base.num = o->audio_time_base.num;
      c->time_base.den = o->audio_time_base.den;
    }
    o->prev_pts = 0;
    o->prev_dts = 0;

    return o->outctx;
  }

  return NULL;
}

static struct dechunkiser_ctx *avf_init(const char *fname, const char *config)
{
  struct dechunkiser_ctx *out;
  struct tag *cfg_tags;

  out = malloc(sizeof(struct dechunkiser_ctx));
  if (out == NULL) {
    return NULL;
  }

  memset(out, 0, sizeof(struct dechunkiser_ctx));
  out->output_format = strdup("nut");
  out->selected_streams = 0x01;
  if (fname) {
    out->output_file = strdup(fname);
  } else {
    out->output_file = strdup("/dev/stdout");
  }
  cfg_tags = config_parse(config);
  if (cfg_tags) {
    const char *format;

    format = config_value_str(cfg_tags, "format");
    if (format) {
      out->output_format = strdup(format);
    }
    format = config_value_str(cfg_tags, "media");
    if (format) {
      if (!strcmp(format, "video")) {
        out->selected_streams = 0x01;
      } else if (!strcmp(format, "audio")) {
        out->selected_streams = 0x02;
      } else if (!strcmp(format, "av")) {
        out->selected_streams = 0x03;
      }
    }
  }
  free(cfg_tags);

  return out;
}

static void avf_write(struct dechunkiser_ctx *o, int id, uint8_t *data, int size)
{
  int header_size;
  int frames, i, media_type;
  uint8_t *p;

  if (data[0] == 0) {
    fprintf(stderr, "Error! strange chunk: %x!!!\n", data[0]);
    return;
  } else if (data[0] < 127) {
    header_size = VIDEO_PAYLOAD_HEADER_SIZE;
    media_type = 1;
  } else {
    header_size = AUDIO_PAYLOAD_HEADER_SIZE;
    media_type = 2;
  }
  if (o->outctx == NULL) {
    o->outctx = format_gen(o, data);
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
  if ((o->streams & media_type) == 0) {
    return;		/* Received a chunk for a non-selected stream */
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
    pkt.stream_index = (media_type == 2) && (((o->streams & 0x01) == 0x01));
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

static void avf_close(struct dechunkiser_ctx *s)
{
  int i;

  av_write_trailer(s->outctx);
  url_fclose(s->outctx->pb);

  for (i = 0; i < s->outctx->nb_streams; i++) {
    av_metadata_free(&s->outctx->streams[i]->metadata);
    av_free(s->outctx->streams[i]->codec);
    av_free(s->outctx->streams[i]->info);
    av_free(s->outctx->streams[i]);
  }
  av_metadata_free(&s->outctx->metadata);
  free(s->outctx);
  free(s->output_format);
  free(s->output_file);
  free(s);
}

struct dechunkiser_iface out_avf = {
  .open = avf_init,
  .write = avf_write,
  .close = avf_close,
};
