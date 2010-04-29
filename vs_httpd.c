/*
 * vs_httpd.c
 *
 * Very Simple HTTP server that can deliver static files very fast
 *
 * Copyright (C) 2010 Yoichi Kawasaki All rights reserved.
 * yokawasa at gmail dot com
 * yk55.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>    // stat
#include <event.h>
#include <evhttp.h>
#include "apr_getopt.h"
#include "apr_strings.h"
#include "apr_file_io.h"
#include "apr_file_info.h"

#define DEFAULT_DOCUMENT_ROOT  "./"
#define DEFAULT_HTTP_ADDR      "0.0.0.0"
#define DEFAULT_HTTP_PORT      8080
#define DEFAULT_INDEX_FILE     "index.html"
#define DEFAULT_MIME_TYPE      "application/ocet-stream"

typedef struct {
    char *svr_addr;          /* server address */
    unsigned int svr_port;   /* server port */
    char *doc_root;          /* document root */
    int daemonize;           /* daemonize option: 0-off, 1-on */
    int verbose;             /* verbose option: 0-off, 1-on */
} httpsvr_config;

/* main global memory pool*/
static apr_pool_t *g_mem_pool = NULL;
/* global httpsvr config */
static httpsvr_config *g_config = NULL;

/* code from mattows.c */
typedef struct EXT2MIME {
   const char *type;
   const char *ext[];
} ext2mime;
static const ext2mime ext_html = {"text/html",{"html","htm",NULL}};
static const ext2mime ext_gif  = {"image/gif",{"gif",NULL}};
static const ext2mime ext_png  = {"image/png",{"png",NULL}};
static const ext2mime ext_jpeg = {"image/jpeg",{"jpg","jpeg","jfif",NULL}};
static const ext2mime ext_css  = {"text/css",{"css",NULL}};
static const ext2mime* const extensions[] = {
   &ext_html, &ext_gif, &ext_png, &ext_jpeg, &ext_css, NULL
};
static const char *find_mime_type(char *buf)
{
   const ext2mime *e2m;
   int i,j;
   for(i=0; (e2m=extensions[i]); ++i)
      for(j=0; e2m->ext[j]; ++j)
         if(!strcasecmp(buf,e2m->ext[j]))
            return e2m->type;
   return DEFAULT_MIME_TYPE;
}

static void usage(void) {
    fprintf(stderr,
           "Usage: vs_httpd [-a address] [-p port] [-d documentroot]\n"
           "               [-D] [-v] [-h]\n"
           "Options:\n"
           "  -a address      : define server address (default: \"0.0.0.0\") \n"
           "  -p port         : define server port (default: 8080) \n"
           "  -d documentroot : define document root (default: \"./\")  \n"
           "  -D              : daemonize option 0-off,1-on (default: 0) \n"
           "  -v              : verbose option 0-off,1-on (default: 0) \n"
           "  -h              : list available command line options (this page) \n"
        );
    exit(1);
}

static void httpsvr_config_init(void)
{
    g_config = apr_pcalloc(g_mem_pool, sizeof(httpsvr_config));
    g_config->svr_addr = apr_pstrdup(g_mem_pool, DEFAULT_HTTP_ADDR);
    g_config->svr_port = DEFAULT_HTTP_PORT;
    g_config->doc_root = apr_pstrdup(g_mem_pool, DEFAULT_DOCUMENT_ROOT);
    g_config->daemonize = 0;
    g_config->verbose = 0;
}

static int exists(const char* path, char **complemented_path, int *filesize)
{
    struct stat sb;
    if(stat(path, &sb)<0){
        return 0;
    }
    if( S_ISREG(sb.st_mode)) {
        *complemented_path = apr_pstrdup(g_mem_pool, path);
         *filesize = (int)sb.st_size;
        return 1;
    }
    if(S_ISDIR(sb.st_mode)) {
        *complemented_path = apr_psprintf(g_mem_pool, "%s%s",path, DEFAULT_INDEX_FILE);
        if(stat(*complemented_path, &sb)==0) {
            *filesize = (int)sb.st_size;
            return 1;
        }
    }
    return 0;
}

void main_request_handler(struct evhttp_request *r, void *args)
{
    apr_status_t rv;
    struct evbuffer *evbuf;
    const char *path, *mimetype;
    char *complemented_path, *extbuf, *filebuf;
    int filesize = 0;

    /* check reqeust type. currently only suppoert GET */
    if (r->type != EVHTTP_REQ_GET) {
        fprintf(stdout, "only support GET request \n");
        evhttp_send_error(r, HTTP_BADREQUEST, "only support GET request");
        return;
    }
    path = apr_psprintf(g_mem_pool, "%s%s",
                g_config->doc_root, evhttp_request_uri(r));
    if(g_config->verbose) {
        fprintf(stderr, "req uri=%s\n", evhttp_request_uri(r) );
        fprintf(stderr, "req path=%s\n", path );
    }
    /* file or dir existence check */
    if (!exists(path, &complemented_path, &filesize)) {
        evhttp_send_error(r, HTTP_NOTFOUND, "file not found");
        return;
    }
    /* file's extension check */
    mimetype = apr_pstrdup(g_mem_pool, DEFAULT_MIME_TYPE);
    extbuf = strrchr(complemented_path,'.');
    if (extbuf) {
        ++extbuf;
        mimetype = find_mime_type(extbuf);
    }
    /* file read */
    filebuf = apr_palloc(g_mem_pool, filesize + 1);
    apr_file_t *file = NULL;
    rv = apr_file_open(&file, complemented_path,
                       APR_READ|APR_BINARY, APR_OS_DEFAULT, g_mem_pool);
    if (rv != APR_SUCCESS) {
        evhttp_send_error(r, HTTP_SERVUNAVAIL, "failed to open file");
        return;
    }
    apr_size_t len = filesize;
    rv = apr_file_read(file, filebuf, &len);
    if (rv != APR_SUCCESS) {
        evhttp_send_error(r, HTTP_SERVUNAVAIL, "failed to read file");
        return;
    }
    apr_file_close(file);

   if(g_config->verbose) {
        fprintf(stderr, "res mimetype=%s\n", mimetype);
        fprintf(stderr, "res file size=%d\n", len);
        fprintf(stderr, "res file output=%s\n", filebuf);
    }

    evbuf = evbuffer_new();
    if (!evbuf) {
        fprintf(stderr, "failed to create response buffer\n");
        evhttp_send_error(r, HTTP_SERVUNAVAIL, "failed to create response buffer");
        return;
    }
    evhttp_add_header(r->output_headers, "Content-Type",mimetype);
    evhttp_add_header(r->output_headers, "Content-Length", apr_psprintf(g_mem_pool,"%d",filesize));
    evbuffer_add(evbuf, filebuf, len);
    evhttp_send_reply(r, HTTP_OK, "", evbuf);
    evbuffer_free(evbuf);
}

int main(int argc, char **argv)
{
    char ch;
    apr_getopt_t *opt;
    const char *optarg;
    struct evhttp *httpd;

    /* apr initialize */
    apr_initialize();
    apr_pool_create(&g_mem_pool, NULL);
    /* server config initialize */
    httpsvr_config_init();

    /* option parse and get */
    if ( apr_getopt_init(&opt, g_mem_pool, argc, (const char * const *)argv) != APR_SUCCESS) {
        fprintf(stderr, "failed to init apr_getopt \n");
        exit(1);
    }
    while (apr_getopt(opt, "a:p:d:vDh", &ch, &optarg) == APR_SUCCESS) {
        switch (ch) {
        case 'a':
            g_config->svr_addr= (char*)apr_pstrdup(g_mem_pool, optarg);
            break;
        case 'p':
            if (sscanf(optarg, "%u", &(g_config->svr_port)) != 1) {
                fprintf(stderr, "invalid -p option\n");
                exit(1);
            }
            break;
        case 'd':
            g_config->doc_root=(char*)apr_pstrdup(g_mem_pool, optarg);
            break;
        case 'v':
            g_config->verbose=1;
            break;
        case 'D':
            g_config->daemonize=1;
            break;
        case 'h':
        default:
            usage();
            break;
        }
    }
    if(g_config->verbose) {
        fprintf(stderr, "svr_addr=%s\n", g_config->svr_addr);
        fprintf(stderr, "svr_port=%d\n", g_config->svr_port);
        fprintf(stderr, "doc_root=%s\n", g_config->doc_root);
        fprintf(stderr, "verbose=%d\n", g_config->verbose);
        fprintf(stderr, "daemonize=%d\n", g_config->daemonize);
    }
    /* document root */
    chdir(g_config->doc_root);
    /* daemonize */
    if ( g_config->daemonize ){
        pid_t daemon_pid =fork();
        if (daemon_pid < 0 ){
            fprintf(stderr, "daemonize failure\n");
            exit(1);
        }
        if (daemon_pid){
            /* scceeded in fork, then parent exit */
            exit(1);
        }
        /* move on to child */
        if (setsid() == -1)
            exit(1);
    }
    /* event driven http */
    event_init();
    httpd = evhttp_start(g_config->svr_addr, g_config->svr_port);
    evhttp_set_gencb(httpd, main_request_handler, NULL);
    event_dispatch();
    evhttp_free(httpd);

    /* apr destruction */
    apr_pool_destroy(g_mem_pool);
    apr_terminate();
    return 0;
}
