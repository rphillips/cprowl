#include <curl/curl.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>

#define CPROWL_MAX_LENGTH_API   80
#define CPROWL_ADD_ENDPOINT     "https://prowl.weks.net/publicapi/add"
#define CPROWL_MAX_LENGTH_APP   256
#define CPROWL_MAX_LENGTH_EVENT 1024
#define CPROWL_MAX_LENGTH_DESC  10000

typedef struct {
    char api[CPROWL_MAX_LENGTH_API];
    char app[CPROWL_MAX_LENGTH_APP];
    char event[CPROWL_MAX_LENGTH_EVENT];
    char description[CPROWL_MAX_LENGTH_DESC];
    int  priority;
} cprowl_request_add_t;

CURLcode cprowl_add(cprowl_request_add_t *req, int *http_error_code);
void     usage();

int main(int argc, char *argv[])
{
    int ch;
    int http_error_code;
    CURLcode res;
    cprowl_request_add_t req = { "", "cprowl", "event", "description", 0 };
    
    static struct option longopts[] = {
        { "api", required_argument, NULL, 'a' },
        { "name", required_argument, NULL, 'n' },
        { "event", required_argument, NULL, 'e' },
        { "description", required_argument, NULL, 'd' },
        { "priority", required_argument, NULL, 'p' },
        { "help", no_argument, NULL, 'h' },
        { NULL, 0, NULL, 0 }
    };

    while ((ch = getopt_long(argc, argv, "p:a:n:e:d:h", longopts, NULL)) != -1) {
        switch (ch) {
        case 'a':
            strncpy(req.api, optarg, CPROWL_MAX_LENGTH_API);
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
            req.priority = atoi(optarg);
            break;
        case 'h':
        default:
            usage();
        }
    }

    /* argument validation */
    if (!strcmp(req.api, "")) {
        fprintf(stderr, "invalid api key");
        exit(1);
    }

    /* init curl */
    curl_global_init(CURL_GLOBAL_ALL);

    /* perform rpc call */
    if ((res=cprowl_add(&req, &http_error_code)) != CURLE_OK) {
        fprintf(stderr, "%s", curl_easy_strerror(res));
        exit(1);
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

    return 0;
}

CURLcode cprowl_add(cprowl_request_add_t *req, int *http_error_code)
{
    CURL *curl;
    CURLcode res;
    struct curl_httppost *formpost=NULL;
    struct curl_httppost *lastptr=NULL;

    if ((curl = curl_easy_init()) == NULL) {
        return CURLE_OUT_OF_MEMORY;
    }

    /* add post data */
    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "apikey",
                 CURLFORM_PTRCONTENTS,  req->api,
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

    /* perform connection */
    curl_easy_setopt(curl, CURLOPT_URL, CPROWL_ADD_ENDPOINT);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

    res = curl_easy_perform(curl);

    curl_formfree(formpost);
    curl_easy_cleanup(curl);
    return res;
}

void usage()
{
    fprintf(stderr, "cprowl : prowl client\n");
    fprintf(stderr, "  usage:\n");
    fprintf(stderr, "    cprowl [-a apikey] [-n appname] [-e event] [-d description] [-p priority]\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    apikey:\n");
    fprintf(stderr, "      prowl api key\n");
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
    exit(0);
}
