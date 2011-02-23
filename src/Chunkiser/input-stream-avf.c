/*
 *  Copyright (c) 2009 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <libavformat/avformat.h>
#include <stdbool.h>
#include <string.h>

//#include "dbg.h"
#include "int_coding.h"
#include "payload.h"
#include "config.h"
#include "chunkiser_iface.h"

#define STATIC_BUFF_SIZE 1000 * 1024
struct chunkiser_ctx {
  AVFormatContext *s;
  int loop;	//loop on input file infinitely
  uint64_t streams;
  int64_t last_ts;
  int64_t base_ts;
  AVBitStreamFilterContext *bsf[MAX_STREAMS];
};

static uint8_t codec_type(enum CodecID cid)
{
  switch (cid) {
    case CODEC_ID_MPEG1VIDEO:
    case CODEC_ID_MPEG2VIDEO:
      return 1;
    case CODEC_ID_H261:
      return 2;
    case CODEC_ID_H263P:
    case CODEC_ID_H263:
      return 3;
    case CODEC_ID_MJPEG:
      return 4;
    case CODEC_ID_MPEG4:
      return 5;
    case CODEC_ID_FLV1:
      return 6;
    case CODEC_ID_SVQ3:
      return 7;
    case CODEC_ID_DVVIDEO:
      return 8;
    case CODEC_ID_H264:
      return 9;
    case CODEC_ID_THEORA:
    case CODEC_ID_VP3:
      return 10;
    case CODEC_ID_SNOW:
      return 11;
    case CODEC_ID_VP6:
      return 12;
    case CODEC_ID_DIRAC:
      return 13;
    case CODEC_ID_MP2:
    case CODEC_ID_MP3:
      return 129;
    case CODEC_ID_AAC:
      return 130;
    case CODEC_ID_AC3:
      return 131;
    case CODEC_ID_VORBIS:
      return 132;
    default:
      fprintf(stderr, "Unknown codec ID %d\n", cid);
      return 0;
  }
}

static void audio_header_fill(uint8_t *data, AVStream *st)
{
  audio_payload_header_write(data, codec_type(st->codec->codec_id), st->codec->channels, st->codec->sample_rate, st->codec->frame_size);
}

static void video_header_fill(uint8_t *data, AVStream *st)
{
  int num, den;

  num = st->avg_frame_rate.num;
  den = st->avg_frame_rate.den;
//fprintf(stderr, "Rate: %d/%d\n", num, den);
  if (num == 0) {
    num = st->r_frame_rate.num;
    den = st->r_frame_rate.den;
  }
  if (num > (1 << 16)) {
    num /= 1000;
    den /= 1000;
  }
  video_payload_header_write(data, codec_type(st->codec->codec_id), st->codec->width, st->codec->height, num, den);
}

static void frame_header_fill(uint8_t *data, int size, AVPacket *pkt, AVStream *st, AVRational new_tb, int64_t base_ts)
{
  int32_t pts, dts;

  if (pkt->pts != AV_NOPTS_VALUE) {
    pts = av_rescale_q(pkt->pts, st->time_base, new_tb),
    pts += av_rescale_q(base_ts, AV_TIME_BASE_Q, new_tb);
  } else {
    pts = -1;
  }
  //dprintf("pkt->pts=%ld PTS=%d",pkt->pts, pts);
  if (pkt->dts != AV_NOPTS_VALUE) {
    dts = av_rescale_q(pkt->dts, st->time_base, new_tb);
    dts += av_rescale_q(base_ts, AV_TIME_BASE_Q, new_tb);
  } else {
    fprintf(stderr, "No DTS???\n");
    dts = 0;
  }
  //dprintf(" DTS=%d\n",dts);
  frame_header_write(data, size, pts, dts);
}

static int input_stream_rewind(struct chunkiser_ctx *s)
{
  int ret;

  ret = av_seek_frame(s->s,-1,0,0);
  s->base_ts = s->last_ts;

  return ret;
}


/* Interface functions */

static struct chunkiser_ctx *avf_open(const char *fname, int *period, const char *config)
{
  struct chunkiser_ctx *desc;
  int i, res;
  struct tag *cfg_tags;
  int video_streams = 0, audio_streams = 1;

  avcodec_register_all();
  av_register_all();

  desc = malloc(sizeof(struct chunkiser_ctx));
  if (desc == NULL) {
    return NULL;
  }
  res = av_open_input_file(&desc->s, fname, NULL, 0, NULL);
  if (res < 0) {
    fprintf(stderr, "Error opening %s: %d\n", fname, res);

    return NULL;
  }

  desc->s->flags |= AVFMT_FLAG_GENPTS;
  res = av_find_stream_info(desc->s);
  if (res < 0) {
    fprintf(stderr, "Cannot find codec parameters for %s\n", fname);

    return NULL;
  }
  desc->streams = 0;
  desc->last_ts = 0;
  desc->base_ts = 0;
  desc->loop = 0;
  cfg_tags = config_parse(config);
  if (cfg_tags) {
    const char *media;

    config_value_int(cfg_tags, "loop", &desc->loop);
    media = config_value_str(cfg_tags, "media");
    if (media) {
      if (!strcmp(media, "audio")) {
        audio_streams = 0;
        video_streams = 1;
      } else if (!strcmp(media, "video")) {
        audio_streams = 1;
        video_streams = 0;
      } else if (!strcmp(media, "av")) {
        audio_streams = 0;
        video_streams = 0;
      }
    }
  }
  free(cfg_tags);
  for (i = 0; i < desc->s->nb_streams; i++) {
    if (desc->s->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
      if (video_streams++ == 0) {
        desc->streams |= 1ULL << i;
      }
      fprintf(stderr, "Video Frame Rate = %d/%d --- Period: %"PRIu64"\n",
              desc->s->streams[i]->r_frame_rate.num,
              desc->s->streams[i]->r_frame_rate.den,
              av_rescale(1000000, desc->s->streams[i]->r_frame_rate.den, desc->s->streams[i]->r_frame_rate.num));
      *period = av_rescale(1000000, desc->s->streams[i]->r_frame_rate.den, desc->s->streams[i]->r_frame_rate.num);
    }
    if (desc->s->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO) {
      if (audio_streams++ == 0) {
        desc->streams |= 1ULL << i;
      }
    }
    if (desc->s->streams[i]->codec->codec_id == CODEC_ID_MPEG4) {
      desc->bsf[i] = av_bitstream_filter_init("dump_extra");
    } else if (desc->s->streams[i]->codec->codec_id == CODEC_ID_H264) {
      desc->bsf[i] = av_bitstream_filter_init("h264_mp4toannexb");
    } else {
      desc->bsf[i] = NULL;
    }
  }

  dump_format(desc->s, 0, fname, 0);

  return desc;
}

static void avf_close(struct chunkiser_ctx *s)
{
  int i;

  for (i = 0; i < s->s->nb_streams; i++) {
    if (s->bsf[i]) {
      av_bitstream_filter_close(s->bsf[i]);
    }
  }
  av_close_input_file(s->s);
  free(s);
}

static uint8_t *avf_chunkise(struct chunkiser_ctx *s, int id, int *size, uint64_t *ts)
{
  AVPacket pkt;
  AVRational new_tb;
  int res;
  uint8_t *data;
  int header_size;

  res = av_read_frame(s->s, &pkt);
  if (res < 0) {
    if (s->loop) {
      if (input_stream_rewind(s) >= 0) {
        *size = 0;
        *ts = s->last_ts;

        return NULL;
      }
    }
    fprintf(stderr, "AVPacket read failed: %d!!!\n", res);
    *size = -1;

    return NULL;
  }
  if ((s->streams & (1ULL << pkt.stream_index)) == 0) {
    *size = 0;
    *ts = s->last_ts;
    av_free_packet(&pkt);

    return NULL;
  }
  if (s->bsf[pkt.stream_index]) {
    AVPacket new_pkt= pkt;
    int res;

    res = av_bitstream_filter_filter(s->bsf[pkt.stream_index],
                                     s->s->streams[pkt.stream_index]->codec,
                                     NULL, &new_pkt.data, &new_pkt.size,
                                     pkt.data, pkt.size, pkt.flags & AV_PKT_FLAG_KEY);
    if(res > 0){
      av_free_packet(&pkt);
      new_pkt.destruct= av_destruct_packet;
    } else if(res < 0){
      fprintf(stderr, "%s failed for stream %d, codec %s: ",
                      s->bsf[pkt.stream_index]->filter->name,
                      pkt.stream_index,
                      s->s->streams[pkt.stream_index]->codec->codec->name);
      fprintf(stderr, "%d\n", res);
      *size = 0;

      return NULL;
    }
    pkt= new_pkt;
  }

  switch (s->s->streams[pkt.stream_index]->codec->codec_type) {
    case CODEC_TYPE_VIDEO:
      header_size = VIDEO_PAYLOAD_HEADER_SIZE;
      break;
    case CODEC_TYPE_AUDIO:
      header_size = AUDIO_PAYLOAD_HEADER_SIZE;
      break;
    default:
      /* Cannot arrive here... */
      fprintf(stderr, "Internal chunkiser error!\n");
      exit(-1);
  }
  *size = pkt.size + header_size + FRAME_HEADER_SIZE;
  data = malloc(*size);
  if (data == NULL) {
    *size = -1;
    av_free_packet(&pkt);

    return NULL;
  }
  switch (s->s->streams[pkt.stream_index]->codec->codec_type) {
    case CODEC_TYPE_VIDEO:
      video_header_fill(data, s->s->streams[pkt.stream_index]);
      new_tb.den = s->s->streams[pkt.stream_index]->avg_frame_rate.num;
      new_tb.num = s->s->streams[pkt.stream_index]->avg_frame_rate.den;
      if (new_tb.num == 0) {
        new_tb.den = s->s->streams[pkt.stream_index]->r_frame_rate.num;
        new_tb.num = s->s->streams[pkt.stream_index]->r_frame_rate.den;
      }
      break;
    case CODEC_TYPE_AUDIO:
      audio_header_fill(data, s->s->streams[pkt.stream_index]);
      new_tb = (AVRational){s->s->streams[pkt.stream_index]->codec->frame_size, s->s->streams[pkt.stream_index]->codec->sample_rate};
      break;
    default:
      /* Cannot arrive here... */
      fprintf(stderr, "Internal chunkiser error!\n");
      exit(-1);
  }
  data[header_size - 1] = 1;
  frame_header_fill(data + header_size, *size - header_size - FRAME_HEADER_SIZE, &pkt, s->s->streams[pkt.stream_index], new_tb, s->base_ts);

  memcpy(data + header_size + FRAME_HEADER_SIZE, pkt.data, pkt.size);
  *ts = av_rescale_q(pkt.dts, s->s->streams[pkt.stream_index]->time_base, AV_TIME_BASE_Q);
  //dprintf("pkt.dts=%ld TS1=%lu" , pkt.dts, *ts);
  *ts += s->base_ts;
  //dprintf(" TS2=%lu\n",*ts);
  s->last_ts = *ts;
  av_free_packet(&pkt);

  return data;
}

#if 0
int chunk_read_avs1(void *s_h, struct chunk *c)
{
    AVFormatContext *s = s_h;
    static AVPacket pkt;
    static int inited;
    AVStream *st;
    int res;
    int cnt;
    static uint8_t static_buff[STATIC_BUFF_SIZE];
    uint8_t *p, *pcurr;
    static uint8_t *p1;
    static struct chunk c2;
    int f1;
    static int f2;

    if (p1) {
        c2.id = c->id;
        *c = c2;
        p1 = NULL;

        return f2;
    }

    p = static_buff;
    p1 = static_buff + STATIC_BUFF_SIZE / 2;
    if (inited == 0) {
        inited = 1;
        res = av_read_frame(s, &pkt);
        if (res < 0) {
            fprintf(stderr, "First read failed: %d!!!\n", res);

            return 0;
        }
        if ((pkt.flags & PKT_FLAG_KEY) == 0) {
            fprintf(stderr, "First frame is not key frame!!!\n");

            return 0;
        }
    }
    cnt = 0; f1 = 0; f2 = 0;
    c->stride_size = 2;
    c2.stride_size = 2;
    pcurr = p1;
    if (pkt.size > 0) {
        memcpy(p, pkt.data, pkt.size);
        c->frame[0] = p;
        c->frame_len[0] = pkt.size;
        f1++;
        p += pkt.size;
    }
    while (1) {
        res = av_read_frame(s, &pkt);
        if (res >= 0) {
            st = s->streams[pkt.stream_index];
            if (pkt.flags & PKT_FLAG_KEY) {
                cnt++;
                if (cnt == 2) {
                    return f1;
                }
            }
            memcpy(pcurr, pkt.data, pkt.size);
            if (pcurr == p) {
                c->frame[f1] = pcurr;
                c->frame_len[f1] = pkt.size;
                p += pkt.size;
                pcurr = p1;
                f1++;
            } else {
                c2.frame[f2] = pcurr;
                c2.frame_len[f2] = pkt.size;
                p1 += pkt.size;
                pcurr = p;
                f2++;
            }
        } else {
            pkt.size = 0;

            return f1;
        }
    }

    return 0;
}
#endif

struct chunkiser_iface in_avf = {
  .open = avf_open,
  .close = avf_close,
  .chunkise = avf_chunkise,
};
