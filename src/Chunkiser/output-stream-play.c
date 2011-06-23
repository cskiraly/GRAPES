/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2011 Daniele Frisanco
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <alsa/asoundlib.h>

#include "int_coding.h"
#include "payload.h"
#include "config.h"
#include "dechunkiser_iface.h"

#ifndef MAX_STREAMS
#define MAX_STREAMS 20
#endif
#ifndef CODEC_TYPE_VIDEO
#define CODEC_TYPE_VIDEO AVMEDIA_TYPE_VIDEO
#define CODEC_TYPE_AUDIO AVMEDIA_TYPE_AUDIO
#endif




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
  struct controls *c1;
  
  int playback_handleopened;
  snd_pcm_t *playback_handle;
  snd_pcm_hw_params_t *hw_params;
  int end;
  GdkPixmap *screen;

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



struct controls {
  GtkWidget *d_area;
  GdkRectangle u_area;
  GdkGC *gc;
};
  





snd_pcm_format_t sample_fmt_to_snd_pcm_format( enum AVSampleFormat  sample_fmt){
  switch((sample_fmt)){
    case AV_SAMPLE_FMT_U8  :return SND_PCM_FORMAT_U8;
    case AV_SAMPLE_FMT_S16 :return SND_PCM_FORMAT_S16;
    case AV_SAMPLE_FMT_S32 :return SND_PCM_FORMAT_S32;
    default		       :return SND_PCM_FORMAT_UNKNOWN;
    //case AV_SAMPLE_FMT_FLT :return SND_PCM_FORMAT_FLOAT;
    //case AV_SAMPLE_FMT_DBL :
    //case AV_SAMPLE_FMT_NONE : return SND_PCM_FORMAT_UNKNOWN;break;
    //case AV_SAMPLE_FMT_NB :	
  }
}


void  prepare_audio ( snd_pcm_t *playback_handle,snd_pcm_hw_params_t *hw_params,const snd_pcm_format_t format,int channels ,int freq)
{ /*http://www.equalarea.com/paul/alsa-audio.html*/

  int err;
		
  if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
    fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
      snd_strerror (err));
    exit (1);
  }
		
  if ((err = snd_pcm_hw_params_any (playback_handle, hw_params)) < 0) {
    fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
      snd_strerror (err));
    exit (1);
  } 
  if ((err = snd_pcm_hw_params_set_access (playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    fprintf (stderr, "cannot set access type (%s)\n",
      snd_strerror (err));
    exit (1);
  }

  if ((err = snd_pcm_hw_params_set_format (playback_handle, hw_params, format)) < 0) {
    fprintf (stderr, "cannot set sample format (%s)\n",
      snd_strerror (err));
    exit (1);
  }

  if ((err = snd_pcm_hw_params_set_rate_resample (playback_handle, hw_params, freq)) < 0) {		
    fprintf (stderr, "cannot set sample rate (%s)\n",
      snd_strerror (err));
    exit (1);
  } 

  if ((err = snd_pcm_hw_params_set_channels (playback_handle, hw_params,channels)) < 0) {
    fprintf (stderr, "cannot set channel count (%s)\n",
      snd_strerror (err)); 
    exit (1);
  }
	
  if ((err = snd_pcm_hw_params (playback_handle, hw_params)) < 0) {
    fprintf (stderr, "cannot set parameters (%s)\n",
      snd_strerror (err));
    exit (1);
  }

  snd_pcm_hw_params_free (hw_params);

  if ((err = snd_pcm_prepare (playback_handle)) < 0) {
    fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
      snd_strerror (err));
    exit (1);
  }
}





static int audio_write_packet(AVFormatContext *s1,snd_pcm_t *playback_handle, AVPacket *pkt)
{ 
  int res;
  int data_size=AVCODEC_MAX_AUDIO_FRAME_SIZE,len1;
  uint8_t *outbuf;
  outbuf = av_malloc(AVCODEC_MAX_AUDIO_FRAME_SIZE*2);

  if (pkt->size == 0)
    return 0;

  while (pkt->size > 0) {
    data_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    len1 = avcodec_decode_audio3(s1->streams[pkt->stream_index]->codec, (int16_t *)outbuf, & data_size, pkt);
    if (len1 < 0) {
      fprintf(stderr, "Error while decoding\n");
      // exit(1);
    }
    if (data_size > 0) {
      /* if a frame has been decoded, output it */			
      data_size /= s1->streams[pkt->stream_index]->codec->channels*pow(2,s1->streams[pkt->stream_index]->codec->sample_fmt);//OK 8 16 e 32

      if ((res = snd_pcm_writei (playback_handle, outbuf,data_size)) != data_size) {
        fprintf (stderr, "write to audio interface failed (%s)\n",snd_strerror (res));
      }
    }
    pkt->size -= len1;
    pkt->data += len1;
  }
  return 0;
}



static struct SwsContext *rescaler_context(AVCodecContext *c)
{
  int w, h;

  w = c->width;
  h = c->height;
  return sws_getContext(w, h, c->pix_fmt, w, h,PIX_FMT_RGB24,
                        SWS_BICUBIC, NULL, NULL, NULL);//prepara un contesto che trasformera i frames nel formato rgb24
}





uint8_t *frame_display(void *h,struct controls *c1, AVPacket pkt,GdkPixmap *screen)
{ 
  AVFormatContext *ctx = h;
  int res = 1, decoded,height,width;
  static struct SwsContext *swsctx;
  AVFrame pic;
  static AVFrame rgb;
  struct controls *c = c1;
  avcodec_get_frame_defaults(&pic);
 
  res = avcodec_decode_video2(ctx->streams[pkt.stream_index]->codec,&pic, &decoded, &pkt);//legge da pkt decodifica e scrive in pic
  if (res < 0) {
    return NULL;
  }

  if (decoded) {
    height=ctx->streams[pkt.stream_index]->codec->height;
    width=ctx->streams[pkt.stream_index]->codec->width;

    if (swsctx == NULL) { 
      swsctx = rescaler_context(ctx->streams[pkt.stream_index]->codec);
      avcodec_get_frame_defaults(&rgb);
      avpicture_alloc((AVPicture*)&rgb,PIX_FMT_RGB24,width,height);//alloca un frame di tipo rgb24
      }
    if (swsctx) {
      sws_scale(swsctx,(const uint8_t* const*) pic.data, pic.linesize, 0,
                height, rgb.data, rgb.linesize);//prende il frame in pic e lo converte e lo mette in rgb

      gdk_draw_rgb_image(screen, c->gc, 0, 0, width, height,
                         GDK_RGB_DITHER_MAX, rgb.data[0], width*3 );
  		
      gtk_widget_draw(GTK_WIDGET(c->d_area), &c->u_area);
			
      return rgb.data[0]; 
    }
  }

  return NULL;
}




static gint delete_event(GtkWidget * widget, GdkEvent * event, struct dechunkiser_ctx * data)
{
 
  data->end=1;
  return (TRUE);
}

static gboolean expose_event(GtkWidget * widget, GdkEventExpose * event,
		struct dechunkiser_ctx * data)
{
  if ((data->screen != NULL) && (!data->end)) {
    gdk_draw_pixmap(widget->window,
		    widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
		    data->screen, event->area.x, event->area.y,
		    event->area.x, event->area.y, event->area.width,
		    event->area.height);
  }

  return TRUE;
}

static gint configure_event(GtkWidget * widget, GdkEventConfigure * event,
		struct dechunkiser_ctx * data)
{
  if (!data->screen) {
    data->screen = gdk_pixmap_new(widget->window, widget->allocation.width,
		            widget->allocation.height, -1);
    gdk_draw_rectangle(data->screen, widget->style->white_gc, TRUE, 0, 0,
		       widget->allocation.width, widget->allocation.height);
  }
  
  return TRUE;
}

void *window_prepare(struct dechunkiser_ctx *o)
{ int w;
  int h;
  GdkGC *gc1;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *window;
  GdkColor black;
  GdkColor white;
  char buff[80];
  gint expid, confid;
  struct controls *c;
  w=o->width;
  h=o->height;
  c = malloc(sizeof(struct controls));
  if (c == NULL) {
    return NULL;
  }

  gdk_rgb_init();
  gtk_widget_set_default_colormap(gdk_rgb_get_cmap());
  gtk_widget_set_default_visual(gdk_rgb_get_visual());

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(window), w, h + 50);

  vbox = gtk_vbox_new(FALSE, 0);
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(vbox));


 
  //drawing area
  c->d_area = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(c->d_area), w, h);
  expid = gtk_signal_connect(GTK_OBJECT(c->d_area), "expose_event",
			     GTK_SIGNAL_FUNC(expose_event),o );
  confid =
	gtk_signal_connect(GTK_OBJECT(c->d_area), "configure_event",
			   GTK_SIGNAL_FUNC(configure_event),o );
  gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(c->d_area), FALSE, FALSE, 5);
  gtk_widget_show(c->d_area);

  sprintf(buff, "qua posso scrivere qualcosa");
  label = gtk_label_new(buff);
  gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(label), FALSE, FALSE, 5);
  gtk_widget_show(label);

  gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(hbox), FALSE, FALSE, 5);

  gtk_widget_show(hbox);
  gtk_widget_show(vbox);
  gtk_widget_show(window);
  gtk_signal_connect(GTK_OBJECT(window), "delete_event",
		     GTK_SIGNAL_FUNC(delete_event),o);

  c->u_area.x = 0;
  c->u_area.y = 0;
  c->u_area.width = c->d_area->allocation.width;
  c->u_area.height = c->d_area->allocation.height;
  c->gc = gdk_gc_new(c->d_area->window);
  gc1 = gdk_gc_new(c->d_area->window);

  gdk_color_black(gdk_colormap_get_system(), &black);
  gdk_color_white(gdk_colormap_get_system(), &white);
  gdk_gc_set_foreground(c->gc, &black);
  gdk_gc_set_background(c->gc, &white);
  gdk_gc_set_foreground(gc1, &white);
  gdk_gc_set_background(gc1, &black);

  return c;
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
      AVCodec *vCodec; 
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
      vCodec = avcodec_find_decoder(o->outctx->streams[o->outctx->nb_streams - 1]->codec->codec_id);
      if(!vCodec) {
       fprintf (stderr, "Video unsupported codec\n");
       return NULL;
      }
      if(avcodec_open(o->outctx->streams[o->outctx->nb_streams - 1]->codec, vCodec)<0){
       fprintf (stderr, "could not open video codec\n");
       return NULL; // Could not open codec
      }

      o->c1 = window_prepare(o);
    }
    if (o->streams & 0x02) { 
      AVCodec  *aCodec;
      av_new_stream(o->outctx, 1);
      c = o->outctx->streams[o->outctx->nb_streams - 1]->codec;
      c->codec_id = o->audio_codec_id;
      c->codec_type = CODEC_TYPE_AUDIO;
      c->sample_rate = o->sample_rate;
      c->channels = o->channels;
      c->frame_size = o->frame_size;
      c->time_base.num = o->audio_time_base.num;
      c->time_base.den = o->audio_time_base.den;

      aCodec = avcodec_find_decoder( o->outctx->streams[o->outctx->nb_streams - 1]->codec->codec_id);
      if(!aCodec) {
       fprintf (stderr, "Audio unsupported codec\n");
       return NULL;
      }
      if(avcodec_open(o->outctx->streams[o->outctx->nb_streams - 1]->codec, aCodec)<0){
       fprintf (stderr, "could not open audio codec\n");
       return NULL; // Could not open codec
      }
    }
    o->prev_pts = 0;
    o->prev_dts = 0;

    return o->outctx;
  }
  return NULL;
}

static struct dechunkiser_ctx *play_init(const char *fname, const char *config)
{
  struct dechunkiser_ctx *out;
  struct tag *cfg_tags;
  int err;
  const char * device_name="hw";
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
  out->playback_handleopened=0;
  out->end=0;
  free(cfg_tags); 
  
  gtk_init(NULL, NULL);
 

  //apro la periferica audio
  
  if ((err = snd_pcm_open (&out->playback_handle, device_name, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
   fprintf (stderr, "cannot open audio device %s (%s)\n", 
	    device_name,
	    snd_strerror (err)); 
   exit (1);
  }

  return out;
}

static void play_write(struct dechunkiser_ctx *o, int id, uint8_t *data, int size)
{
  int header_size;
  int frames, i, media_type;
  uint8_t *p;

  if (gtk_events_pending()) {
	gtk_main_iteration_do(FALSE);}

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
    url_fopen(&o->outctx->pb, o->output_file, URL_WRONLY/*AVIO_FLAG_WRITE*/ );
  
    if (o->outctx->pb == NULL) {
      fprintf(stderr, "Cannot open %s!\n", o->output_file);
    }

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

    if(pkt.stream_index==0){
      AVPacket pktcopy=pkt;//Concorrenza quindi me lo copio
      unsigned char * rgbbuff ; 
      rgbbuff = frame_display(o->outctx,o->c1,pktcopy,o->screen);
      }
      else {
        int channels=o->channels;
        int sample_rate=o->sample_rate;
        snd_pcm_format_t snd_pcm_fmt=sample_fmt_to_snd_pcm_format(o->outctx->streams[pkt.stream_index]->codec->sample_fmt);

        if (snd_pcm_fmt==SND_PCM_FORMAT_UNKNOWN){
          fprintf (stderr, "sample format not supported\n");
	  return;}
        if (o->playback_handleopened==0){
          prepare_audio(o->playback_handle,o->hw_params,snd_pcm_fmt,channels,sample_rate);
          o->playback_handleopened=1;}
        audio_write_packet(o->outctx,o->playback_handle, &pkt);	
      }
  }
}

static void play_close(struct dechunkiser_ctx *s)
{
  int i;
  snd_pcm_close (s->playback_handle);
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

struct dechunkiser_iface out_play = {
  .open = play_init,
  .write = play_write,
  .close = play_close,
};
