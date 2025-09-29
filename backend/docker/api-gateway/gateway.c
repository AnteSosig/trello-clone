#include <microhttpd.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define TARGET_HOST "http://localhost"
#define PORT 8443

int SERVICE_COUNT;

struct ServiceAddressKeyValue {
    char *service;
    int address;
};

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

char* load_file(const char* filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    char *buf = malloc(len + 1);
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
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
        MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, PATCH, DELETE, OPTIONS");
        MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type, Authorization");
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    if (strcmp(url, "/demonstracija") == 0 && strcmp(method, "GET") == 0) {
        const char *page = "<html><body><h1>HTTPS DEMONSTRACIJA</h1></body></html>";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(page), (void *)page, MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response); 
        return ret;
    }
    
    if (*con_cls == NULL) {
        struct ConnectionInfo* conn_info = calloc(1, sizeof(struct ConnectionInfo));
        *con_cls = (void*)conn_info;
        return MHD_YES;
    }

    printf("=== PROCESSING REQUEST ===\n");
    printf("URL: %s\n", url);
    printf("Method: %s\n", method);
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
    int url_len = strlen(url);
    int service_name_len = 0;
    for (int i = 1; i < url_len; ++i) {
        if (url[i] == '/') {
            break;
        }
        ++service_name_len;
    }
    if (service_name_len == 0) {
        const char* not_found = "404 - Not Found";
        struct MHD_Response* response = MHD_create_response_from_buffer(strlen(not_found), (void*)not_found, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
        MHD_destroy_response(response);
        return ret;
    }
    char service_name[service_name_len + 1];
    for (int i = 0; i < service_name_len; ++i) {
        service_name[i] = url[i + 1];
    }
    service_name[service_name_len] = '\0';
    printf("=== SERVICE ROUTING ===\n");
    printf("Extracted service name: '%s'\n", service_name);
    printf("Service name length: %d\n", service_name_len);

    struct ServiceAddressKeyValue **routing_table = (struct ServiceAddressKeyValue **)cls;
    printf("=== ROUTING TABLE DEBUG ===\n");
    printf("SERVICE_COUNT: %d\n", SERVICE_COUNT);
    for (int i = 0; i < SERVICE_COUNT; ++i) {
        if (routing_table[i]->service) {
	        printf("Service[%d]: '%s'\n", i, routing_table[i]->service);
        }
        if (routing_table[i]->address) {
            printf("Port[%d]: %d\n", i, routing_table[i]->address);
        }
	}
    int port = -1;
    char *service_first_char;
    printf("=== SERVICE LOOKUP ===\n");
    printf("Looking for service: '%s'\n", service_name);
    for (int i = 0; i < SERVICE_COUNT; ++i) {
        printf("Comparing with routing_table[%d]->service: '%s'\n", i, routing_table[i]->service);
        if (strcmp(routing_table[i]->service, service_name) == 0) {
            port = routing_table[i]->address;
            service_first_char = &routing_table[i]->service[0];
            printf("MATCH FOUND! Port: %d\n", port);
            break;
        }
    }
    if (port == -1) {
        printf("=== SERVICE NOT FOUND ERROR ===\n");
        printf("Failed to find service: '%s'\n", service_name);
        const char* not_found = "404 - Not Found";
        struct MHD_Response* response = MHD_create_response_from_buffer(strlen(not_found), (void*)not_found, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
        MHD_destroy_response(response);
        return ret;
    }
    printf("target service port pls: %d\n", port);
    char endpoint[url_len - service_name_len + 1];
    for (int i = 0; i < url_len - service_name_len; ++i) {
        endpoint[i] = url[1 + service_name_len + i];
    }
    endpoint[url_len - service_name_len] = '\0';
    printf("endpointovic %s\n", endpoint);

    //snprintf(target_url, sizeof(target_url), "%s:%d%s%s", TARGET_HOST, port, endpoint, query);
    // Use Docker service names instead of localhost
    char service_host[256];
    if (strcmp(service_name, "user") == 0) {
        snprintf(service_host, sizeof(service_host), "http://user-service");
    } else if (strcmp(service_name, "project") == 0) {
        snprintf(service_host, sizeof(service_host), "http://project-service");
    } else if (strcmp(service_name, "task") == 0) {
        snprintf(service_host, sizeof(service_host), "http://task-service");
    } else if (strcmp(service_name, "notification") == 0) {
        snprintf(service_host, sizeof(service_host), "http://notification-service");
    } else {
        snprintf(service_host, sizeof(service_host), "%s", TARGET_HOST);
    }
    
    snprintf(target_url, sizeof(target_url), "%s:%d%s%s", service_host, port, endpoint, query);
    printf("=== CURL REQUEST SETUP ===\n");
    printf("Query: %s\n", query);
    printf("Target URL: %s\n", target_url);
    printf("Endpoint: %s\n", endpoint);
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

    if (strcmp(method, "POST") == 0 || strcmp(method, "PATCH") == 0 || strcmp(method, "PUT") == 0) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, conn_info->json_data);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, conn_info->json_size);
        printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA%s\n", conn_info->json_data);
    }
    printf("le preform request pls\n");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    printf("=== CURL RESPONSE ===\n");
    printf("CURL result: %s (%d)\n", curl_easy_strerror(res), res);

    long http_code = 500;
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        printf("HTTP Response Code: %ld\n", http_code);
    } else {
        printf("CURL ERROR: %s\n", curl_easy_strerror(res));
    }

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
            const char *header_value = colon + 2; // skip ":"
            MHD_add_response_header(response, header_name, header_value);
        }
        h = h->next;
    }

    // Cleanup
    curl_slist_free_all(headers);
    curl_slist_free_all(resp.headers);
    curl_easy_cleanup(curl);

    const char* existing_cors = MHD_get_response_header(response, "Access-Control-Allow-Origin");
    if (!existing_cors) {
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, PATCH, PUT, DELETE, OPTIONS");
        MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type, Authorization");
    }

    enum MHD_Result ret = MHD_queue_response(connection, (unsigned int)http_code, response);
    MHD_destroy_response(response);
    return ret;
}


int main() {
    // Force flush stdout immediately
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    
    // Also write to a debug file
    FILE *debug = fopen("debug.log", "w");
    if (debug) {
        fprintf(debug, "=== API GATEWAY STARTING ===\n");
        fflush(debug);
    }
    
    printf("=== API GATEWAY STARTING ===\n");
    fflush(stdout);
    fprintf(stderr, "=== API GATEWAY STARTING (STDERR) ===\n");
    fflush(stderr);

    const char *var_name = "SERVICES";
    char *services_env = getenv(var_name);
    if (!services_env) {
        fprintf(stderr, "Cannot find services\n");
        fflush(stderr);
        return 1;
    }
    printf("Found SERVICES environment: %s\n", services_env);
    fflush(stdout);
	char *services = malloc(strlen(services_env) + 1);
	if (!services) {
	    return -1;
	}
	strcpy(services, services_env);
	printf("Services: %s\n", services);
	
	int service_count = 1;
	for (int i = 0; i < strlen(services); ++i) {
	    if (services[i] == ':') {
		++service_count;
	    }
	}
	char *service_list[service_count];
	for (int i = 0; i < service_count; ++i) {
	    service_list[i] = malloc(strlen(services) + 1);
	    memset(service_list[i], 0, strlen(services) + 1);
	}
	int fill_count = 0;
	int list_char_count = 0;
	for (int i = 0; i < service_count; ++i) {
	    while (fill_count < strlen(services)) {
            printf("%c\n", services[fill_count]);
            if (services[fill_count] == ':') {
                ++fill_count;
                list_char_count = 0;
                break;
            }
            service_list[i][list_char_count] = services[fill_count];
            ++fill_count;
            ++list_char_count;
	    }
	}
	for (int i = 0; i < service_count; ++i) {
	    printf("kesten pire: %szlaja\n", service_list[i]);
	}
    free(services);

    struct ServiceAddressKeyValue *routing_table[service_count];
	for (int i = 0; i < service_count; ++i) {
        int service_name_len = strlen(service_list[i]);
        char suiseiseki[service_name_len + 6];
        snprintf(suiseiseki, service_name_len + 6, "%s%s", service_list[i], "_PORT");
        printf("sui: %s\n", suiseiseki);

        char *service = getenv(suiseiseki);
        if (!service) {
            continue;
        }
        printf("%s\n", service);
        int number;
        int result = sscanf(service, "%d", &number);
        if (result != 1) {
            continue;
        }

	    routing_table[i] = malloc(sizeof(struct ServiceAddressKeyValue));
        for (int j = 0; service_list[i][j] != '\0'; j++) {
            if (service_list[i][j] >= 'A' && service_list[i][j] <= 'Z') {
                service_list[i][j] = service_list[i][j] + ('a' - 'A');
            }
        }
        routing_table[i]->service = service_list[i];
        printf("nigerski krek:%s\n", service_list[i]);
        routing_table[i]->address = number;
        printf("nigerski krek:%d\n", number);
	}
	for (int i = 0; i < service_count; ++i) {
        if (routing_table[i]->service) {
	        printf("krompir pire: %szlaja\n", routing_table[i]->service);
        }
        if (routing_table[i]->address) {
            printf("krompir pire: %dzlaja\n", routing_table[i]->address);
        }
	}
    SERVICE_COUNT = service_count;

    printf("=== LOADING SSL CERTIFICATES ===\n");
    char *cert = load_file("cert.pem");
    char *key  = load_file("key.pem");
    
    printf("Certificate loaded: %s\n", cert ? "YES" : "NO");
    printf("Key loaded: %s\n", key ? "YES" : "NO");

    if (!cert || !key) {
        fprintf(stderr, "Failed to load cert.pem or key.pem\n");
        return 2;
    }
    printf("SSL certificates loaded successfully\n");

    printf("=== INITIALIZING CURL ===\n");
    curl_global_init(CURL_GLOBAL_ALL);
    printf("CURL initialized successfully\n");

    printf("=== SETTING UP PORT ===\n");
    char* port_env = getenv("PORT");
    int gateway_port;
    if (!port_env) {
        fprintf(stderr, "No port given\n");
        gateway_port = PORT;
        printf("Using default port: %d\n", gateway_port);
    } else {
        int result = sscanf(port_env, "%d", &gateway_port);
        printf("Port from environment: %d\n", gateway_port);
    }

    printf("=== STARTING MHD DAEMON ===\n");
    printf("Gateway port: %d\n", gateway_port);
    printf("Routing table prepared with %d services\n", SERVICE_COUNT);
    
    struct MHD_Daemon *daemon = MHD_start_daemon(
        MHD_USE_SELECT_INTERNALLY | MHD_USE_TLS,
        gateway_port,
        NULL, NULL,
        &handle_request, routing_table,
        MHD_OPTION_HTTPS_MEM_CERT, cert,
        MHD_OPTION_HTTPS_MEM_KEY, key,
        MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int)30,
        MHD_OPTION_END);
    
    if (!daemon) {
        printf("=== MHD DAEMON FAILED TO START ===\n");
        perror("MHD_start_daemon");
        curl_global_cleanup();
        free(cert);
        free(key);
        for (int i = 0; i < SERVICE_COUNT; ++i) {
            free(service_list[i]);
        }
        for (int i = 0; i < SERVICE_COUNT; ++i) {
            free(routing_table[i]);
        }
        return 3;
    }
    printf("=== MHD DAEMON STARTED SUCCESSFULLY ===\n");

    printf("Transparent API Gateway listening on port %d...\n", gateway_port);
    pause();

    MHD_stop_daemon(daemon);
    curl_global_cleanup();
    free(cert);
    free(key);
    for (int i = 0; i < SERVICE_COUNT; ++i) {
        free(service_list[i]);
    }
    for (int i = 0; i < SERVICE_COUNT; ++i) {
        free(routing_table[i]);
    }
    return 0;
}
