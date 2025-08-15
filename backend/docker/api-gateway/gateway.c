#include <microhttpd.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define TARGET_HOST "http://localhost:8081"
#define PORT 8443

struct MemoryStruct {
    char *memory;
    size_t size;
};

// Struct to hold both body and headers of the response
struct ResponseData {
    struct MemoryStruct body;
    struct curl_slist *headers;
};

struct ConnectionInfo {
    char* json_data;
    size_t json_size;
};

// Write callback for the body
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) return 0; // out of memory
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

// Write callback for headers
static size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
    size_t realsize = nitems * size;
    struct ResponseData *resp = (struct ResponseData *)userdata;

    // Skip the HTTP status line (e.g., HTTP/1.1 200 OK)
    char *colon = memchr(buffer, ':', realsize);
    if (colon) {
        char header_line[1024];
        snprintf(header_line, sizeof(header_line), "%.*s", (int)(realsize - 2), buffer); // remove \r\n
        resp->headers = curl_slist_append(resp->headers, header_line);
    }
    return realsize;
}

// Collect headers from MHD and forward them to libcurl
static int header_iterator(void *cls, enum MHD_ValueKind kind, const char *key, const char *value) {
    struct curl_slist **headers = (struct curl_slist **)cls;
    char header[1024];
    snprintf(header, sizeof(header), "%s: %s", key, value);
    *headers = curl_slist_append(*headers, header);
    return MHD_YES;
}

int parse_parameters(void* cls, enum MHD_ValueKind kind, const char* key, const char* value) {

    char* query = (char*)cls;
    int i = strlen(query);

    if (key && value) {
        snprintf(query + i, 1024 - i, "%s", key);
        i = strlen(query);
        snprintf(query + i, 1024 - i, "=");
        i = strlen(query);
        snprintf(query + i, 1024 - i, "%s", value);
        i = strlen(query);
        snprintf(query + i, 1024 - i, "&");
        i = strlen(query);
    }

    return MHD_YES;
}

static enum MHD_Result handle_request(void *cls,
                                      struct MHD_Connection *connection,
                                      const char *url,
                                      const char *method,
                                      const char *version,
                                      const char *upload_data,
                                      size_t *upload_data_size,
                                      void **con_cls) {
    if (strcmp(method, "OPTIONS") == 0) {
        struct MHD_Response* response = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type");
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }
    
    if (*con_cls == NULL) {
        struct ConnectionInfo* conn_info = calloc(1, sizeof(struct ConnectionInfo));
        *con_cls = (void*)conn_info;
        return MHD_YES;
    }

    printf("processing data.\n");

    struct ConnectionInfo* conn_info = (struct ConnectionInfo*)(*con_cls);
    // If there's upload data, accumulate it
    if (*upload_data_size > 0) {
        // Reallocate memory to store incoming data
        conn_info->json_data = realloc(conn_info->json_data, conn_info->json_size + *upload_data_size + 1);
        memcpy(conn_info->json_data + conn_info->json_size, upload_data, *upload_data_size);
        conn_info->json_size += *upload_data_size;
        conn_info->json_data[conn_info->json_size] = '\0';  // Null-terminate
        *upload_data_size = 0;  // Signal that we've processed this data
        printf("KRKAAAAAAN.\n");
        return MHD_YES;
    }
    
    printf("data processed\n");
    
    char query[1024] = {};
    query[0] = '?';
    MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, parse_parameters, query);

    CURL *curl = curl_easy_init();
    printf("zlaja piskimic\n");
    if (!curl) return MHD_NO;

    // Build target URL
    char target_url[1512];
    snprintf(target_url, sizeof(target_url), "%s%s%s", TARGET_HOST, url, query);
    printf("query tew zene: %s\n", query);
    printf("nigger url: %s\n", target_url);
    printf("zlaja kakimic\n");
    curl_easy_setopt(curl, CURLOPT_URL, target_url);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
    printf("le kokan\n");

    // Forward incoming headers
    struct curl_slist *headers = NULL;
    MHD_get_connection_values(connection, MHD_HEADER_KIND, header_iterator, &headers);
    if (headers) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    printf("le unjan\n");

    // Capture response body and headers
    struct ResponseData resp;
    resp.body.memory = malloc(1);
    resp.body.size = 0;
    resp.headers = NULL;

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&resp.body);

    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&resp);

    if (strcmp(method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, conn_info->json_data);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, conn_info->json_size);
    }
    printf("le preform request pls\n");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    printf("beszarom a gatyaim\n");

    long http_code = 500;
    if (res == CURLE_OK) curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    // Build MHD response with the forwarded body
    struct MHD_Response *response = MHD_create_response_from_buffer(resp.body.size,
                                                                    resp.body.memory,
                                                                    MHD_RESPMEM_MUST_FREE);

    // Forward headers back to client
    struct curl_slist *h = resp.headers;
    while (h) {
        char *colon = strchr(h->data, ':');
        if (colon) {
            *colon = '\0';
            const char *header_name = h->data;
            const char *header_value = colon + 2; // skip ": "
            MHD_add_response_header(response, header_name, header_value);
        }
        h = h->next;
    }

    // Cleanup
    curl_slist_free_all(headers);
    curl_slist_free_all(resp.headers);
    curl_easy_cleanup(curl);

    enum MHD_Result ret = MHD_queue_response(connection, http_code, response);
    MHD_destroy_response(response);
    return ret;
}

int main() {
    curl_global_init(CURL_GLOBAL_ALL);

    struct MHD_Daemon *daemon;
    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
                              &handle_request, NULL, MHD_OPTION_END);
    if (!daemon) {
        curl_global_cleanup();
        return 1;
    }

    printf("Transparent API Gateway listening on port %d...\n", PORT);
    pause(); // Press Enter to stop

    MHD_stop_daemon(daemon);
    curl_global_cleanup();
    return 0;
}
