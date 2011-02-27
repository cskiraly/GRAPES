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

struct log_info {
  int i_frames[16];
  int p_frames[16];
  int b_frames[16];
  int frame_number;
  FILE *log;
};

struct chunkiser_ctx {
  AVFormatContext *s;
  int loop;	//loop on input file infinitely
  uint64_t streams;
  int64_t last_ts;
  int64_t base_ts;
  uint8_t *i_chunk;
  uint8_t *p_chunk;
  uint8_t *b_chunk;
  int i_chunk_size;
  int p_chunk_size;
  int b_chunk_size;
  int p_ready;
  int b_ready;
  AVBitStreamFilterContext *bsf[MAX_STREAMS];

  struct log_info chunk_log;
};

static void chunk_print(FILE *log, int id, int *frames)
{
  int i = 0;

  fprintf(log, "Chunk %d:", id);
  while (frames[i] != -1) {
    fprintf(log, " %d", frames[i++]);
  }
  fprintf(log, "\n");
  frames[0] = -1;
}

static void frame_add(int *frames, int frame_id)
{
  int i = 0;

  while (frames[i] != -1) i++;
  frames[i] = frame_id;
  frames[i + 1] = -1;
}

static int frame_type(AVPacket *pkt)
{
  if ((pkt->dts == AV_NOPTS_VALUE) || (pkt->pts == AV_NOPTS_VALUE)) {
    return -1;
  }
  
  if (pkt->flags & PKT_FLAG_KEY) {
    return FF_I_TYPE;
  }
  if (pkt->dts == pkt->pts) {
    return FF_B_TYPE;
  }

  return FF_P_TYPE;
}

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
  data[VIDEO_PAYLOAD_HEADER_SIZE - 1] = 0;
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

static struct chunkiser_ctx *ipb_open(const char *fname, int *period, const char *config)
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
  desc->i_chunk = NULL;
  desc->i_chunk_size = 0;
  desc->p_chunk = NULL;
  desc->p_chunk_size = 0;
  desc->b_chunk = NULL;
  desc->b_chunk_size = 0;
  desc->p_ready = 0;
  desc->b_ready = 0;
  desc->chunk_log.i_frames[0] = -1;
  desc->chunk_log.p_frames[0] = -1;
  desc->chunk_log.b_frames[0] = -1;
  desc->chunk_log.frame_number = 0;
  desc->chunk_log.log = fopen("chunk_log.txt", "w");
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

static void ipb_close(struct chunkiser_ctx *s)
{
  int i;

  for (i = 0; i < s->s->nb_streams; i++) {
    if (s->bsf[i]) {
      av_bitstream_filter_close(s->bsf[i]);
    }
  }
  av_close_input_file(s->s);
  free(s->i_chunk);
  free(s->p_chunk);
  free(s->b_chunk);
  fclose(s->chunk_log.log);
  free(s);
}

static uint8_t *ipb_chunkise(struct chunkiser_ctx *s, int id, int *size, uint64_t *ts)
{
  AVPacket pkt;
  AVRational new_tb;
  int res;
  uint8_t *data, *result;

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
  if (frame_type(&pkt) < 0) {
    fprintf(stderr, "Strange frame type!!!\n");
    *size = 0;
    *ts = s->last_ts;
    av_free_packet(&pkt);

    return NULL;
  }
  s->chunk_log.frame_number++;
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
      new_pkt.flags = pkt.flags;
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

  *size = 0;
  result = NULL; 
  switch(frame_type(&pkt)) {
    case FF_I_TYPE:
      fprintf(stderr, "I Frame!\n");
      result = s->i_chunk;
      *size = s->i_chunk_size;
      if (*size) chunk_print(s->chunk_log.log, id, s->chunk_log.i_frames);
      s->i_chunk = NULL;
      s->p_ready = 1;
      s->b_ready = 1;
      if (s->i_chunk == NULL) {

        s->i_chunk = malloc(VIDEO_PAYLOAD_HEADER_SIZE);
        s->i_chunk_size = VIDEO_PAYLOAD_HEADER_SIZE;
        video_header_fill(s->i_chunk, s->s->streams[pkt.stream_index]);
      }
      frame_add(s->chunk_log.i_frames, s->chunk_log.frame_number);
      s->i_chunk_size += pkt.size + FRAME_HEADER_SIZE;
      s->i_chunk = realloc(s->i_chunk, s->i_chunk_size);
      data = s->i_chunk + (s->i_chunk_size - (pkt.size + FRAME_HEADER_SIZE));
      break;
    case FF_P_TYPE:
      fprintf(stderr, "P Frame!\n");
      if (s->p_ready) {
        result = s->p_chunk;
        *size = s->p_chunk_size;
        s->p_chunk = NULL;
        s->p_ready = 0;
        if (*size) chunk_print(s->chunk_log.log, id, s->chunk_log.p_frames);
      }
      if (s->p_chunk == NULL) {
        s->p_chunk = malloc(VIDEO_PAYLOAD_HEADER_SIZE);
        s->p_chunk_size = VIDEO_PAYLOAD_HEADER_SIZE;
        video_header_fill(s->p_chunk, s->s->streams[pkt.stream_index]);
      }
      frame_add(s->chunk_log.p_frames, s->chunk_log.frame_number);
      s->p_chunk_size += pkt.size + FRAME_HEADER_SIZE;
      s->p_chunk = realloc(s->p_chunk, s->p_chunk_size);
      data = s->p_chunk + (s->p_chunk_size - (pkt.size + FRAME_HEADER_SIZE));
      break;
    case FF_B_TYPE:
      fprintf(stderr, "B Frame!\n");
      if (s->b_ready) {
        result = s->b_chunk;
        *size = s->b_chunk_size;
        s->b_chunk = NULL;
        s->b_ready = 0;
        if (*size) chunk_print(s->chunk_log.log, id, s->chunk_log.b_frames);
      }
      if (s->b_chunk == NULL) {
        s->b_chunk = malloc(VIDEO_PAYLOAD_HEADER_SIZE);
        s->b_chunk_size = VIDEO_PAYLOAD_HEADER_SIZE;
        video_header_fill(s->b_chunk, s->s->streams[pkt.stream_index]);
      }
      frame_add(s->chunk_log.b_frames, s->chunk_log.frame_number);
      s->b_chunk_size += pkt.size + FRAME_HEADER_SIZE;
      s->b_chunk = realloc(s->b_chunk, s->b_chunk_size);
      data = s->b_chunk + (s->b_chunk_size - (pkt.size + FRAME_HEADER_SIZE));
      break;
  }

  new_tb.den = s->s->streams[pkt.stream_index]->avg_frame_rate.num;
  new_tb.num = s->s->streams[pkt.stream_index]->avg_frame_rate.den;
  if (new_tb.num == 0) {
    new_tb.den = s->s->streams[pkt.stream_index]->r_frame_rate.num;
    new_tb.num = s->s->streams[pkt.stream_index]->r_frame_rate.den;
  }
  frame_header_fill(data, pkt.size, &pkt, s->s->streams[pkt.stream_index], new_tb, s->base_ts); // PKT Size???
  memcpy(data + FRAME_HEADER_SIZE, pkt.data, pkt.size);
  *ts = av_rescale_q(pkt.dts, s->s->streams[pkt.stream_index]->time_base, AV_TIME_BASE_Q);
  //dprintf("pkt.dts=%ld TS1=%lu" , pkt.dts, *ts);
  *ts += s->base_ts;
  //dprintf(" TS2=%lu\n",*ts);
  s->last_ts = *ts;
  av_free_packet(&pkt);

  return result;
}

struct chunkiser_iface in_ipb = {
  .open = ipb_open,
  .close = ipb_close,
  .chunkise = ipb_chunkise,
};
