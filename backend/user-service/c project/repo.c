#include <mongoc/mongoc.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include "model.h"
#include "repo.h"

typedef struct {
    mongoc_client_t* client;
    mongoc_collection_t* collection;
    FILE* logger;
} Repository;

Repository* New(FILE* logger) {

    //hardcoded for now
    const char* dburi = "mongodb://localhost:27017/";


    if (dburi == NULL) {
        fprintf(logger, "Error: MONGO_DB_URI environment variable is not set\n");
        return NULL;
    }

    // Create MongoDB client
    mongoc_client_t* client = mongoc_client_new(dburi);
    if (!client) {
        fprintf(logger, "Error: Failed to create MongoDB client\n");
        mongoc_cleanup();
        return NULL;
    }

    // Test connection (MongoDB C driver doesn't use an explicit connect step)
    bson_error_t error;
    if (!mongoc_client_get_server_status(client, NULL, NULL, &error)) {
        fprintf(logger, "Error: Failed to connect to MongoDB: %s\n", error.message);
        mongoc_client_destroy(client);
        mongoc_cleanup();
        return NULL;
    }

    // Allocate memory for the Repository struct
    Repository* repo = (Repository*)malloc(sizeof(Repository));
    if (repo == NULL) {
        fprintf(logger, "Error: Failed to allocate memory for repository\n");
        mongoc_client_destroy(client);
        mongoc_cleanup();
        return NULL;
    }

    // Set up the Repository struct fields
    repo->client = client;
    repo->logger = logger;

    return repo;
}

// Cleanup function to release resources
void Cleanup(Repository* repo) {
    if (repo) {
        mongoc_client_destroy(repo->client);
        free(repo);
    }
}

int adduser(User *user) {

    printf("KRKAAAAAAAAAAAAN.\n");

    FILE* log = fopen("log.txt", "w");
    if (log == NULL) {
        printf("Error opening file!\n");
    }

    Repository *repo = New(log); 
    const char *db_name = "users";
    const char *collection_name = "users";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);
    printf("SMIRDIIIIIIIIM.\n");

    bson_t* query = BCON_NEW(
        "$or", "[",
        "{", "username", BCON_UTF8(user->username), "}",
        "{", "email", BCON_UTF8(user->email), "}",
        "]"
    );

    mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(repo->collection, query, NULL, NULL);
    const bson_t* doccpy;
    if (mongoc_cursor_next(cursor, &doccpy)) {
        fprintf(repo->logger, "Error: Duplicate username or email found.\n");
        printf("Error: Duplicate username or email found.\n");
        bson_destroy(query);
        mongoc_cursor_destroy(cursor);
        Cleanup(repo);
        if (log) {
            fclose(log);
        }
        return 1;
    }

    bson_t* doc = bson_new();
    BSON_APPEND_UTF8(doc, "username", user->username);
    BSON_APPEND_UTF8(doc, "first_name", user->first_name);
    BSON_APPEND_UTF8(doc, "last_name", user->last_name);
    BSON_APPEND_UTF8(doc, "email", user->email);
    //plaintext
    BSON_APPEND_UTF8(doc, "password", user->password);
    BSON_APPEND_INT32(doc, "active", 0);

    bson_error_t error;
    if (!mongoc_collection_insert_one(repo->collection, doc, NULL, NULL, &error)) {
        fprintf(repo->logger, "Error: Insert failed\n");
        printf("Error: Insert failed\n");
    }
    else {
        fprintf(repo->logger, "Document inserted successfully.\n");
        printf("Document inserted successfully.\n");
    }

    bson_destroy(doc);

    FILE* payload_file = fopen("email_payload.txt", "w");
    if (payload_file) {
        fprintf(payload_file, "To: %s\r\n"
            "From: trello clone\r\n"
            "Subject: Test Email\r\n"
            "\r\n"
            "Egy asszonynak kilenc lanya nem gyozi szamlalni.\r\n", user->email);
        fclose(payload_file);
        fprintf(repo->logger, "Written to the payload file.\n");
    }
    else {
        fprintf(repo->logger, "Failed to open payload file for writing.\n");
        printf("Failed to open payload file for writing.\n");

        return 2;
    }

    payload_file = fopen("email_payload.txt", "r");
    if (!email(user, payload_file)) {
        printf("ubicu se bukvalno.\n");
        fprintf(repo->logger, "Email sent successfully.\n");
        printf("Email sent successfully.\n");
    }
    else {
        fprintf(repo->logger, "Failed to send email.\n");
        printf("Failed to send email.\n");

        return 3;
    }

    Cleanup(repo);
    if (log) {
        fclose(log);
    }
    return 0;

}

int repo() {

    mongoc_init();
    getchar();
    mongoc_cleanup();

    return 0;

}

int email(User *user, FILE* payload_file) {

    if (!payload_file) {
        fprintf(stderr, "Failed to open payload file for reading\n");
        return 3;
    }

    CURL* curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "smtp://smtp.gmail.com:587");
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        //hardcoded for now
        curl_easy_setopt(curl, CURLOPT_USERNAME, "nikola.birclin@gmail.com");
        //hardcoded for now :(
        curl_easy_setopt(curl, CURLOPT_PASSWORD, "rjxx axrp qkye vsee");
        //hardcoded for now
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, "nikola.birclin@gmail.com");

        printf("nagy a pusztulas.\n");

        struct curl_slist* recipients = NULL;
        recipients = curl_slist_append(recipients, user->email);
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_READDATA, payload_file);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        printf("csarda mellet akasztofa.\n");

        res = curl_easy_perform(curl);
        printf("bazmeg.\n");
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

            curl_slist_free_all(recipients);
            curl_easy_cleanup(curl);
            fclose(payload_file);

            return 1;
        }

        printf("curl ok?.\n");

        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);
        fclose(payload_file);

        printf("curl ok????????????.\n");

        return 0;
    }
    fclose(payload_file);

    return 2;
}