/*
 *  Copyright (c) 2011 Andrea Zito
 *
 *  This is free software; see lgpl-2.1.txt
 *
 *
 *  Delegate cloud_handler for MySql based on the MySql C Connector library.
 *  Supported parameters (*required):
 *
 *  - mysql_host*:  the mysql server host
 *  - mysql_user*:  the mysql user
 *  - mysql_pass*:  the mysql password
 *  - mysql_db*:    the mysql database to use
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include <mysql.h>

#include "net_helper.h"
#include "request_handler.h"
#include "cloud_helper_iface.h"
#include "config.h"


#define CLOUD_NODE_ADDR "0.0.0.0"

/***********************************************************************
 * Interface prototype for cloud_helper_delegate
 ***********************************************************************/
struct delegate_iface {
  void* (*cloud_helper_init)(struct nodeID *local, const char *config);
  int (*get_from_cloud)(void *context, const char *key, uint8_t *header_ptr,
                        int header_size, int free_header);
  int (*get_from_cloud_default)(void *context, const char *key,
                                uint8_t *header_ptr, int header_size, int free_header,
                                uint8_t *defval_ptr, int defval_size, int free_defval);
  int (*put_on_cloud)(void *context, const char *key, uint8_t *buffer_ptr,
                      int buffer_size, int free_buffer);
  struct nodeID* (*get_cloud_node)(void *context, uint8_t variant);
  time_t (*timestamp_cloud)(void *context);
  int (*is_cloud_node)(void *context, struct nodeID* node);
  int (*wait4cloud)(void *context, struct timeval *tout);
  int (*recv_from_cloud)(void *context, uint8_t *buffer_ptr, int buffer_size);
};


struct mysql_cloud_context {
  struct req_handler_ctx *req_handler;
  MYSQL *mysql;
  time_t last_rsp_timestamp;
};


/* Definition of request structure */
enum mysql_helper_operation {PUT=0, GET=1};
struct mysql_request {
  enum mysql_helper_operation op;
  char *key;

  /* For GET operations this point to the header.
     For PUT this is the pointer to the actual data */
  uint8_t *data;
  int data_length;
  int free_data;

  uint8_t *default_value;
  int default_value_length;
  int free_default_value;
  struct mysql_cloud_context *helper_ctx;
};
typedef struct mysql_request mysql_request_t;


/* Definition of response structure */
enum mysql_helper_status {SUCCESS=0, ERROR=1};
struct mysql_get_response {
  enum mysql_helper_status status;
  uint8_t *data;
  uint8_t *current_byte;

  int data_length;
  int read_bytes;
  time_t last_timestamp;
};
typedef struct mysql_get_response mysql_get_response_t;



static const char* parse_required_param(struct tag *cfg_tags, const char *name)
{
  const char *arg;
  arg = config_value_str(cfg_tags, name);
  if (!arg) {
    fprintf(stderr, "mysql_delegate_helper: missing required parameter " \
            "'%s'\n", name);
    return NULL;
  }
  return arg;
}


static MYSQL* init_mysql(MYSQL **mysql) {
  /* initialize library */
  if (mysql_library_init(0, NULL, NULL)) {
    fprintf(stderr,
            "mysql_delegate_helper: could not initialize MySQL library\n");
    return NULL;
  }

  /* initialize mysql object */
  *mysql = NULL;
  *mysql = mysql_init(*mysql);

  return *mysql;
}

static int init_database(MYSQL *mysql)
{
  char query[256];
  int error;
  snprintf(query, 256, "CREATE TABLE IF NOT EXISTS cloud ("   \
           "  cloud_key VARCHAR(255),"\
           "  cloud_value BLOB," \
           "  timestamp INT UNSIGNED," \
           "  counter INT UNSIGNED," \
           "  PRIMARY KEY (cloud_key))");
  error = mysql_query(mysql, query);
  if (error) {
    fprintf(stderr,
            "mysql_delegate_helper: error creating table: %s\n",
            mysql_error(mysql));
    return 1;
  }

  return 0;
}


static void deallocate_context(struct mysql_cloud_context *ctx)
{
  req_handler_destroy(ctx->req_handler);
  mysql_close(ctx->mysql);
  mysql_library_end();
}

static void free_request(void *req_ptr)
{
  mysql_request_t *req;
  req = (mysql_request_t *) req_ptr;
  if (req->free_data) free(req->data);

  if (req->free_default_value > 0)
    free(req->default_value);

  free(req);
  return;
}

static void free_response(mysql_get_response_t *rsp)
{
  if (rsp->data) free(rsp->data);

  free(rsp);
}

/***********************************************************************
 * Implementation of request processing
 ***********************************************************************/
int process_get_operation(void *req_data, void **rsp_data)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  mysql_request_t *req;
  mysql_get_response_t *rsp;
  char query[256];
  unsigned long *field_len;
  int err;

  req = (mysql_request_t *) req_data;

  snprintf(query, 127, "SELECT cloud_value, timestamp FROM cloud WHERE " \
           "cloud_key='%s'", req->key);
  err = mysql_query(req->helper_ctx->mysql, query);
  if (err) {
    fprintf(stderr,
            "mysql_delegate_helper: error retrieving key: %s\n",
            mysql_error(req->helper_ctx->mysql));
    return -1;
  }

  rsp = malloc(sizeof(mysql_get_response_t));
  if (!rsp) return -1;

  /* initialize response */
  *rsp_data = rsp;
  rsp->status = ERROR;
  rsp->data = NULL;
  rsp->current_byte = NULL;
  rsp->data_length = 0;
  rsp->read_bytes = 0;
  rsp->last_timestamp = -1;

  result = mysql_store_result(req->helper_ctx->mysql);
  if (result) {
    row = mysql_fetch_row(result);
    if (row) {
      char ts_str[21];
      char *value;
      int value_len;

      field_len = mysql_fetch_lengths(result);
      value_len = field_len[0];
      value = row[0];

      /* reserve space for value and header */
      rsp->data_length = value_len + req->data_length;
      rsp->data = calloc(rsp->data_length, sizeof(char));
      rsp->current_byte = rsp->data;

      if (req->data_length > 0)
        memcpy(rsp->data, req->data, req->data_length);
      memcpy(rsp->data + req->data_length, value, value_len);
      memcpy(ts_str, row[1], field_len[1]);
      ts_str[field_len[1]] = '\0';
      rsp->last_timestamp = strtol(ts_str, NULL, 10);
      rsp->status = SUCCESS;
    } else {
      /* Since there was no value for the specified key. If the caller specified a
         default value, use that */
      if (req->default_value) {
        /* reserve space for value and header */
        rsp->data_length = req->default_value_length + req->data_length;
        rsp->data = calloc(rsp->data_length, sizeof(char));
        rsp->current_byte = rsp->data;

        if (req->data_length > 0)
          memcpy(rsp->data, req->data, req->data_length);

        memcpy(rsp->data + req->data_length,
               req->default_value, req->default_value_length);

        rsp->last_timestamp = 0;
        rsp->status = SUCCESS;
      }
    }
    mysql_free_result(result);
  }

  return 0;
}

int process_put_operation(struct mysql_cloud_context *ctx,
                          mysql_request_t *req)
{
  char raw_stmt[] = "INSERT INTO cloud(cloud_key,cloud_value,timestamp, counter)" \
    "VALUES('%s', '%s', %ld, 0) ON DUPLICATE KEY UPDATE "                  \
    "cloud_value='%s', timestamp=%ld, counter=counter+1";
  char *stmt;
  char *escaped_value;
  int stmt_length;
  int escaped_length;
  int err;
  time_t now;

  escaped_value = calloc(req->data_length*2+1, sizeof(char));

  escaped_length = mysql_real_escape_string(ctx->mysql,
                                            escaped_value,
                                            req->data,
                                            req->data_length);


  /* reserve space for the statement: len(value/ts)+len(key)+len(SQL_cmd) */
  stmt_length = 2*escaped_length + 2*20 + strlen(req->key) + strlen(raw_stmt);
  stmt = calloc(stmt_length, sizeof(char));

  now = time(NULL);
  stmt_length = snprintf(stmt, stmt_length, raw_stmt, req->key, escaped_value,
                         now, escaped_value, now);

  err = mysql_real_query(ctx->mysql, stmt, stmt_length);
  if (err) {
    fprintf(stderr,
            "mysql_delegate_helper: error setting key: %s\n",
            mysql_error(ctx->mysql));
    return 1;
  }

  return 0;
}


/***********************************************************************
 * Implementation of interface delegate_iface
 ***********************************************************************/
void* cloud_helper_init(struct nodeID *local, const char *config)
{
  struct mysql_cloud_context *ctx;
  struct tag *cfg_tags;

  const char *mysql_host;
  const char *mysql_user;
  const char *mysql_pass;
  const char *mysql_db;


  ctx = malloc(sizeof(struct mysql_cloud_context));
  memset(ctx, 0, sizeof(struct mysql_cloud_context));
  cfg_tags = config_parse(config);

  /* Parse fundametal parameters */
  if (!(mysql_host=parse_required_param(cfg_tags, "mysql_host"))) {
    deallocate_context(ctx);
    return NULL;
  }

  if (!(mysql_user=parse_required_param(cfg_tags, "mysql_user"))) {
    deallocate_context(ctx);
    return NULL;
  }

  if (!(mysql_pass=parse_required_param(cfg_tags, "mysql_pass"))) {
    deallocate_context(ctx);
    return NULL;
  }

  if (!(mysql_db=parse_required_param(cfg_tags, "mysql_db"))) {
    deallocate_context(ctx);
    return NULL;
  }

  ctx->mysql = init_mysql(&ctx->mysql);
  if (!ctx->mysql) {
    deallocate_context(ctx);
    return NULL;
  }

  if (!mysql_real_connect(ctx->mysql,
                          mysql_host,
                          mysql_user,
                          mysql_pass,
                          mysql_db,
                          0, NULL, 0)) {
    fprintf(stderr,"mysql_delegate_helper: error connecting to db: %s\n",
            mysql_error(ctx->mysql));
    deallocate_context(ctx);
    return NULL;
  }

  if (init_database(ctx->mysql) != 0) {
    deallocate_context(ctx);
    return NULL;
  }

  ctx->req_handler = req_handler_init();
  if (!ctx->req_handler) {
    deallocate_context(ctx);
    return NULL;
  }

  return ctx;
}

int get_from_cloud_default(void *context, const char *key,
                           uint8_t *header_ptr, int header_size, int free_header,
                           uint8_t *defval_ptr, int defval_size, int free_defval)
{
  struct mysql_cloud_context *ctx;
  mysql_request_t *request;
  int err;

  ctx = (struct mysql_cloud_context *) context;
  request = malloc(sizeof(mysql_request_t));

  if (!request) return 1;

  request->op = GET;
  request->key = strdup(key);
  request->data = header_ptr;
  request->data_length = header_size;
  request->free_data = free_header;
  request->default_value = defval_ptr;
  request->default_value_length = defval_size;
  request->free_default_value = free_defval;
  request->helper_ctx = ctx;

  err = req_handler_add_request(ctx->req_handler, &process_get_operation,
                                request, &free_request);
  if (err) free_request(request);

  return err;
}

int get_from_cloud(void *context, const char *key, uint8_t *header_ptr,
                   int header_size, int free_header)
{
  return get_from_cloud_default(context, key, header_ptr, header_size,
                                free_header, NULL, 0, 0);
}


int put_on_cloud(void *context, const char *key, uint8_t *buffer_ptr,
                 int buffer_size, int free_buffer)
{
  struct mysql_cloud_context *ctx;
  mysql_request_t *request;
  int res;

  ctx = (struct mysql_cloud_context *) context;
  request = malloc(sizeof(mysql_request_t));

  if (!request) return 1;

  request->op = PUT;
  request->key = strdup(key);
  request->data = buffer_ptr;
  request->data_length = buffer_size;
  request->free_data = free_buffer;
  request->default_value = NULL;
  request->default_value_length = 0;
  request->free_default_value = 0;
  res = process_put_operation(ctx, request);
  free_request(request);
  return res;
}

struct nodeID* get_cloud_node(void *context, uint8_t variant)
{
  return create_node(CLOUD_NODE_ADDR, variant);
}

time_t timestamp_cloud(void *context)
{
  struct mysql_cloud_context *ctx;
  ctx = (struct mysql_cloud_context *) context;

  return ctx->last_rsp_timestamp;
}

int is_cloud_node(void *context, struct nodeID* node)
{
  char ipaddr[96];
  node_ip(node, ipaddr, 96);
  return strcmp(ipaddr, CLOUD_NODE_ADDR) == 0;
}

int wait4cloud(void *context, struct timeval *tout)
{
  struct mysql_cloud_context *ctx;
  mysql_get_response_t *rsp;

  ctx = (struct mysql_cloud_context *) context;

  rsp = req_handler_wait4response(ctx->req_handler, tout);

  if (rsp) {
    if (rsp->status == SUCCESS) {
      ctx->last_rsp_timestamp = rsp->last_timestamp;
      return 1;
    } else {
      /* there was some error with the request */
      req_handler_remove_response(ctx->req_handler);
      free_response(rsp);
      return -1;
    }
  } else {
    return 0;
  }
}

int recv_from_cloud(void *context, uint8_t *buffer_ptr, int buffer_size)
{
  struct mysql_cloud_context *ctx;
  mysql_get_response_t *rsp;
  int remaining;
  int toread;

  ctx = (struct mysql_cloud_context *) context;

  rsp = (mysql_get_response_t *) req_handler_get_response(ctx->req_handler);
  if (!rsp) return -1;

  /* If do not have further data just remove the request */
  if (rsp->read_bytes == rsp->data_length) {
    req_handler_remove_response(ctx->req_handler);
    free_response(rsp);
    return 0;
  }

  remaining = rsp->data_length - rsp->read_bytes;
  toread = (remaining <= buffer_size)? remaining : buffer_size;

  memcpy(buffer_ptr, rsp->current_byte, toread);
  rsp->current_byte += toread;
  rsp->read_bytes += toread;

  /* remove the response only if the read bytes are less the the allocated
     buuffer otherwise the client can't know when a single response finished */
  if (rsp->read_bytes == rsp->data_length && rsp->read_bytes < buffer_size){
    free_response(rsp);
    req_handler_remove_response(ctx->req_handler);
  }

  return toread;
}

struct delegate_iface delegate_impl = {
  .cloud_helper_init = &cloud_helper_init,
  .get_from_cloud = &get_from_cloud,
  .get_from_cloud_default = &get_from_cloud_default,
  .put_on_cloud = &put_on_cloud,
  .get_cloud_node = &get_cloud_node,
  .timestamp_cloud = &timestamp_cloud,
  .is_cloud_node = &is_cloud_node,
  .wait4cloud = &wait4cloud,
  .recv_from_cloud = &recv_from_cloud
};
