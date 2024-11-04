#include <mongoc/mongoc.h>
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

    // Initialize MongoDB C driver
    mongoc_init();

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
    mongoc_cleanup();
}

int adduser(User *user) {

    FILE* log = fopen("log.txt", "w");
    if (log == NULL) {
        printf("Error opening file!\n");
    }

    Repository *repo = New(log); 
    const char *db_name = "users";
    const char *collection_name = "users";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);

    bson_t* doc = bson_new();
    BSON_APPEND_UTF8(doc, "username", user->username);
    BSON_APPEND_UTF8(doc, "first_name", user->first_name);
    BSON_APPEND_UTF8(doc, "last_name", user->last_name);
    BSON_APPEND_UTF8(doc, "email", user->email);
    //plaintext
    BSON_APPEND_UTF8(doc, "password", user->password);

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
    Cleanup(repo);
    if (log) {
        fclose(log);
    }
    return 0;

}