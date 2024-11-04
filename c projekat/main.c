#include <microhttpd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "model.h"

#define PORT 8080

// Function to handle requests and send responses
int answer_to_connection(void* cls, struct MHD_Connection* connection,
    const char* url, const char* method, const char* version,
    const char* upload_data, size_t* upload_data_size, void** con_cls) {

    // Define a simple "route" for "/user"
    if (strcmp(url, "/user") == 0) {
        const char* response_str = "User route";
        struct MHD_Response* response = MHD_create_response_from_buffer(strlen(response_str),
            (void*)response_str,
            MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*"); // CORS header
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    // Default 404 not found response
    const char* not_found = "404 - Not Found";
    struct MHD_Response* response = MHD_create_response_from_buffer(strlen(not_found),
        (void*)not_found,
        MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);
    return ret;
}

int main() {
    struct MHD_Daemon* daemon;

    // Set the PORT from environment variable, fallback to default PORT 8080
    char* port_env = getenv("PORT");
    int port = port_env ? atoi(port_env) : PORT;

    // Start the HTTP server
    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, port, NULL, NULL,
        &answer_to_connection, NULL, MHD_OPTION_END);

    if (NULL == daemon) {
        printf("Failed to start server\n");
        return 1;
    }

    printf("Server running on port %d\n", port);

    // Run indefinitely (could also add logic to handle graceful shutdowns)
    getchar();

    MHD_stop_daemon(daemon);
    return 0;
}
