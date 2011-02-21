/*
 *  Copyright (c) 2010 Csaba Kiraly
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#include "chunk.h"
#include "chunkiser.h"
#include "net_helper.h"

static char out_opts[1024];
static char *out_ptr = out_opts;
static char in_opts[1024];
static char *in_ptr = in_opts;
static int udp_port;
static int out_udp_port;

static void help(const char *name)
{
  fprintf(stderr, "Usage: %s [options] <input> <output>\n", name);
  fprintf(stderr, "options: t:u:f:rRdlavVUT\n");
  fprintf(stderr, "\t -u <port>: use the UDP chunkiser (on <port>) for input\n");
  fprintf(stderr, "\t -P <port>: use the UDP chunkiser (on <port>) for output\n");
  fprintf(stderr, "\t -f <fmt>: use the <fmt> format for ouptut (libav-based)\n");
  fprintf(stderr, "\t -r: use RAW output\n");
  fprintf(stderr, "\t -R: use RAW output, removing the libav payload header\n");
  fprintf(stderr, "\t -d: use the dummy chunkiser\n");
  fprintf(stderr, "\t -l: loop\n");
  fprintf(stderr, "\t -a: audio-only in the libav ouptut\n");
  fprintf(stderr, "\t -v: video-only in the libav output\n");
  fprintf(stderr, "\t -V: audio/video in the libav output\n");
  fprintf(stderr, "\t -U: use RAW output, removing the UDP payload header\n");
  fprintf(stderr, "\t -T: use RAW output, removing the RTP payload heade\n");
  fprintf(stderr, "\t -I <config>: specify the chunkiser config\n");
  fprintf(stderr, "\t -O <config>: specify the dechunkiser config\n");
}

static char *addopt(char *opts, char *opts_ptr, const char *tag, const char *value)
{
  if (opts_ptr != opts) {
    *opts_ptr++ = ',';
  }
  opts_ptr += sprintf(opts_ptr, "%s=%s", tag, value);
  return opts_ptr;
}

static int cmdline_parse(int argc, char *argv[])
{
  int o;

  while ((o = getopt(argc, argv, "tlP:u:f:rRdlavVUTO:I:")) != -1) {
    char port[8];

    switch(o) {
      case 'l':
        in_ptr = addopt(in_opts, in_ptr, "loop", "1");
        break;
      case 'P':
        if (out_udp_port == 0) {
          out_ptr = addopt(out_opts, out_ptr, "dechunkiser", "udp");
        }
        sprintf(port, "port%d", out_udp_port);
        out_udp_port++;
        out_ptr = addopt(out_opts, out_ptr, port, optarg);
        break;
      case 't':
        in_ptr = addopt(in_opts, in_ptr, "chunkiser", "ts");
        break;
      case 'u':
        if (udp_port == 0) {
          in_ptr = addopt(in_opts, in_ptr, "chunkiser", "udp");
        }
        sprintf(port, "port%d", udp_port);
        udp_port++;
        in_ptr = addopt(in_opts, in_ptr, port, optarg);
        break;
      case 'f':
        out_ptr = addopt(out_opts, out_ptr, "format", optarg);
        break;
      case 'r':
        out_ptr = addopt(out_opts, out_ptr, "dechunkiser", "raw");
        break;
      case 'R':
        out_ptr = addopt(out_opts, out_ptr, "dechunkiser", "raw");
        out_ptr = addopt(out_opts, out_ptr, "payload", "avf");
        break;
      case 'U':
        out_ptr = addopt(out_opts, out_ptr, "dechunkiser", "raw");
        out_ptr = addopt(out_opts, out_ptr, "payload", "udp");
        break;
      case 'T':
        out_ptr = addopt(out_opts, out_ptr, "dechunkiser", "raw");
        out_ptr = addopt(out_opts, out_ptr, "payload", "rtp");
        break;
      case 'd':
        in_ptr = addopt(in_opts, in_ptr, "chunkiser", "dummy");
        break;
      case 'a':
        in_ptr = addopt(in_opts, in_ptr, "chunkiser", "avf");
        in_ptr = addopt(in_opts, in_ptr, "media", "audio");
        out_ptr = addopt(out_opts, out_ptr, "dechunkiser", "avf");
        out_ptr = addopt(out_opts, out_ptr, "media", "audio");
        break;
      case 'v':
        in_ptr = addopt(in_opts, in_ptr, "chunkiser", "avf");
        in_ptr = addopt(in_opts, in_ptr, "media", "video");
        out_ptr = addopt(out_opts, out_ptr, "dechunkiser", "avf");
        out_ptr = addopt(out_opts, out_ptr, "media", "video");
        break;
      case 'V':
        in_ptr = addopt(in_opts, in_ptr, "chunkiser", "avf");
        in_ptr = addopt(in_opts, in_ptr, "media", "av");
        out_ptr = addopt(out_opts, out_ptr, "dechunkiser", "avf");
        out_ptr = addopt(out_opts, out_ptr, "media", "av");
        break;
      case 'O':
        out_ptr += sprintf(out_ptr, "%s", optarg);
        break;
      case 'I':
        in_ptr += sprintf(in_ptr, "%s", optarg);
        break;
      default:
        fprintf(stderr, "Error: unknown option %c\n", o);

        exit(-1);
    }
  }

  return optind - 1;
}

static void in_wait(const int *fd)
{
  int my_fd[10];
  int i = 0;
  
  if (fd == NULL) {
    return;
  }
  while(fd[i] != -1) {
    my_fd[i] = fd[i];
    i++;
  }
  my_fd[i] = -1;

  wait4data(NULL, NULL, my_fd);
}

int main(int argc, char *argv[])
{
  int period, done, id;
  struct input_stream *input;
  struct output_stream *output;
  const int *in_fds;

  if (argc < 3) {
    help(argv[0]);

    return -1;
  }
  argv += cmdline_parse(argc, argv);
  input = input_stream_open(argv[1], &period, in_opts);
  if (input == NULL) {
    fprintf(stderr, "Cannot open input %s\n", argv[1]);

    return -1;
  }
  if (period == 0) {
    in_fds = input_get_fds(input);
  } else {
    in_fds = NULL;
  }
  output = out_stream_init(argv[2], out_opts);
  if (output == NULL) {
    fprintf(stderr, "Cannot open output %s\n", argv[2]);

    return -1;
  }

  done = 0; id = 0;
  while(!done) {
    int res;
    struct chunk c;

    in_wait(in_fds);
    c.id = id;
    res = chunkise(input, &c);
    if (res > 0) {
      fprintf(stderr,"chunk %d: %d %llu\n", id++, c.size, c.timestamp);
      chunk_write(output, &c);
    } else if (res < 0) {
      done = 1;
    }
    free(c.data);
  }
  input_stream_close(input);
  out_stream_close(output);

  return 0;
}
