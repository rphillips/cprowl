/*
   Copyright 2010 Ryan Phillips

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */ 
#include <curl/curl.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include "cprowl_config.h"
#include "queue.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define CPROWL_ADD_ENDPOINT     "https://prowl.weks.net/publicapi/add"
#define CPROWL_MAX_LENGTH_API   40
#define CPROWL_MAX_LENGTH_APP   256
#define CPROWL_MAX_LENGTH_EVENT 1024
#define CPROWL_MAX_LENGTH_DESC  10000
#define CPROWL_MAX_LENGTH_PRIORITY 5

typedef struct api_node {
    char api[CPROWL_MAX_LENGTH_API];
    SLIST_ENTRY(api_node) nodes;
} api_node_t;

typedef struct {
    SLIST_HEAD(api_node_head, api_node) api_list;
    char app[CPROWL_MAX_LENGTH_APP + 1];
    char event[CPROWL_MAX_LENGTH_EVENT + 1];
    char description[CPROWL_MAX_LENGTH_DESC + 1];
    char priority[CPROWL_MAX_LENGTH_PRIORITY + 1];
} cprowl_add_request_t;

/* Request Helper Functions */
static char* cprowl_request_get_api_string(cprowl_add_request_t *req);
static void  cprowl_request_add_init(cprowl_add_request_t *req);
static int   cprowl_request_add_apikey(cprowl_add_request_t *req, 
                                      const char *apikey);
static void  cprowl_request_free(cprowl_add_request_t *req);

/* RPC request */
static CURLcode cprowl_add(cprowl_add_request_t *req, int debug, 
                           int *http_error_code);

static void usage();

int main(int argc, char *argv[])
{
    int ch;
    int http_error_code;
    int debug = FALSE;
    CURLcode res;
    cprowl_add_request_t req;
    
    static struct option longopts[] = {
        { "api", required_argument, NULL, 'a' },
        { "name", required_argument, NULL, 'n' },
        { "event", required_argument, NULL, 'e' },
        { "description", required_argument, NULL, 'd' },
        { "priority", required_argument, NULL, 'p' },
        { "help", no_argument, NULL, 'h' },
        { "debug", no_argument, NULL, 'z' },
        { NULL, 0, NULL, 0 }
    };

    cprowl_request_add_init(&req);

    while ((ch = getopt_long(argc, argv, "p:a:n:e:d:hz", longopts, NULL)) != -1) {
        switch (ch) {
        case 'a':
            if (!cprowl_request_add_apikey(&req, optarg)) {
                fprintf(stderr, "invalid api key (%s)\n", optarg);
                goto done;
            }
            break;
        case 'n':
            strncpy(req.app, optarg, CPROWL_MAX_LENGTH_APP);
            break;
        case 'e':
            strncpy(req.event, optarg, CPROWL_MAX_LENGTH_EVENT);
            break;
        case 'd':
            strncpy(req.description, optarg, CPROWL_MAX_LENGTH_DESC);
            break;
        case 'p':
            strncpy(req.priority, optarg, CPROWL_MAX_LENGTH_PRIORITY);
            break;
        case 'z':
            debug = TRUE;
            break;
        case 'h':
        default:
            usage();
        }
    }

    /* argument validation */
    if (SLIST_EMPTY(&req.api_list)) {
        fprintf(stderr, "invalid api key\n");
        goto done;
    }

    /* init curl */
    curl_global_init(CURL_GLOBAL_ALL);

    /* perform rpc call */
    if ((res=cprowl_add(&req, debug, &http_error_code)) != CURLE_OK) {
        fprintf(stderr, "%s", curl_easy_strerror(res));
        goto done;
    }

    /* error handling */
    switch (http_error_code) {
        case 200:
            fprintf(stdout, "ok\n");
            break;
        case 401:
            fprintf(stdout, "authentication error\n");
            break;
        default:
            fprintf(stdout, "http_error_code = %d\n", http_error_code);
            break;
    }

done:
    cprowl_request_free(&req);
    return 0;
}

static void 
cprowl_request_add_init(cprowl_add_request_t *req)
{
    memset(req, 0, sizeof(*req));
    SLIST_INIT(&req->api_list);
    strcpy(req->app, "cprowl");
    strcpy(req->event, "event");
    strcpy(req->description, "description");
    strcpy(req->priority, "0");
}

static void 
cprowl_request_free(cprowl_add_request_t *req)
{
    while (!SLIST_EMPTY(&req->api_list)) {
        api_node_t *n = SLIST_FIRST(&req->api_list);
        SLIST_REMOVE_HEAD(&req->api_list, nodes);
        free(n);
    }
}

#define BLOCK_SIZE 1024

static char* 
cprowl_request_get_api_string(cprowl_add_request_t *req)
{
    api_node_t *node;
    char *str = NULL, *p;
    int mem_len = 0;
    int str_len = 0;

    /* this is not optimized */
    str = malloc(BLOCK_SIZE);
    str[0] = '\0';
    mem_len += BLOCK_SIZE;

    SLIST_FOREACH(node, &req->api_list, nodes) {
        if (mem_len < (str_len + strlen(node->api) + 3)) {
            str = realloc(str, BLOCK_SIZE);
        }
        strncat(str, node->api, CPROWL_MAX_LENGTH_API);
        if (SLIST_END(&req->api_list) != SLIST_NEXT(node, nodes))
            strcat(str, ",");
    }

    return str;
}

static int
cprowl_request_add_apikey(cprowl_add_request_t *req, 
                          const char *apikey)
{
    api_node_t *node;

    if (strlen(apikey) != CPROWL_MAX_LENGTH_API) {
        return FALSE;
    }

    node = malloc(sizeof(api_node_t));
    strncpy(node->api, optarg, CPROWL_MAX_LENGTH_API);
    SLIST_INSERT_HEAD(&req->api_list, node, nodes);
    return TRUE;
}

static size_t 
curl_write_cb(void *ptr, size_t size, size_t nmemb, void *stream)
{
    /* noop */
    return (size * nmemb);
}

static CURLcode 
cprowl_add(cprowl_add_request_t *req, int debug, int *http_error_code)
{
    CURL *curl;
    CURLcode res;
    struct curl_httppost *formpost=NULL;
    struct curl_httppost *lastptr=NULL;
    char *api_keys;

    *http_error_code = 0;

    if ((curl = curl_easy_init()) == NULL) {
        return CURLE_OUT_OF_MEMORY;
    }

    api_keys = cprowl_request_get_api_string(req);

    /* add post data */
    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "apikey",
                 CURLFORM_PTRCONTENTS,  api_keys,
                 CURLFORM_END);
    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_PTRNAME , "application",
                 CURLFORM_PTRCONTENTS,  req->app,
                 CURLFORM_END);
    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_PTRNAME , "event",
                 CURLFORM_PTRCONTENTS,  req->event,
                 CURLFORM_END);
    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_PTRNAME , "description",
                 CURLFORM_PTRCONTENTS,  req->description,
                 CURLFORM_END);
    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_PTRNAME , "priority",
                 CURLFORM_PTRCONTENTS,  req->priority,
                 CURLFORM_END);

    /* perform connection */
    curl_easy_setopt(curl, CURLOPT_URL, CPROWL_ADD_ENDPOINT);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, debug);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);

    res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, http_error_code);
    }

    curl_formfree(formpost);
    curl_easy_cleanup(curl);
    free(api_keys);
    return res;
}

static void usage()
{
    fprintf(stderr, "%s v%s : prowl client\n", CPROWL_NAME, CPROWL_VERSION);
    fprintf(stderr, "  usage:\n");
    fprintf(stderr, "    cprowl [-a apikey] [-n appname] [-e event] [-d description] [-p priority]\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    apikey:\n");
    fprintf(stderr, "      string : prowl api key\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    appname:\n");
    fprintf(stderr, "      string : application name (default: \"cprowl\")\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    event:\n");
    fprintf(stderr, "      string : event name (default: \"event\")\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    description:\n");
    fprintf(stderr, "      string : description (default: \"\")\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    priority:\n");
    fprintf(stderr, "      -2 : very low\n");
    fprintf(stderr, "      -1 : moderate\n");
    fprintf(stderr, "       0 : normal (default)\n");
    fprintf(stderr, "       1 : high\n");
    fprintf(stderr, "       2 : emergency\n");
    fprintf(stderr, "\n");
    exit(0);
}
