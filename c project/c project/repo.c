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
    printf("Initializing MongoDB client...\n");
    const char* dburi = "mongodb://localhost:27017/";

    if (dburi == NULL) {
        fprintf(logger, "Error: MONGO_DB_URI environment variable is not set\n");
        printf("Error: MONGO_DB_URI environment variable is not set\n");
        return NULL;
    }

    // Create MongoDB client
    mongoc_client_t* client = mongoc_client_new(dburi);
    if (!client) {
        fprintf(logger, "Error: Failed to create MongoDB client\n");
        printf("Error: Failed to create MongoDB client\n");
        mongoc_cleanup();
        return NULL;
    }
    printf("MongoDB client created successfully.\n");

    // Test connection
    bson_error_t error;
    if (!mongoc_client_get_server_status(client, NULL, NULL, &error)) {
        fprintf(logger, "Error: Failed to connect to MongoDB: %s\n", error.message);
        printf("Error: Failed to connect to MongoDB: %s\n", error.message);
        mongoc_client_destroy(client);
        mongoc_cleanup();
        return NULL;
    }
    printf("Connected to MongoDB server successfully.\n");

    // Allocate memory for the Repository struct
    Repository* repo = (Repository*)malloc(sizeof(Repository));
    if (repo == NULL) {
        fprintf(logger, "Error: Failed to allocate memory for repository\n");
        printf("Error: Failed to allocate memory for repository\n");
        mongoc_client_destroy(client);
        mongoc_cleanup();
        return NULL;
    }
    printf("Repository struct allocated successfully.\n");

    // Set up the Repository struct fields
    repo->client = client;
    repo->logger = logger;

    return repo;
}

// Cleanup function to release resources
void Cleanup(Repository* repo) {
    if (repo) {
        printf("Cleaning up MongoDB client and repository...\n");
        mongoc_client_destroy(repo->client);
        free(repo);
        printf("Cleanup completed.\n");
    }
}

int addproject(Project* project) {
    printf("Adding project to MongoDB...\n");
    FILE* log = fopen("log.txt", "w");
    if (log == NULL) {
        printf("Error opening log file!\n");
        return 1;
    }

    Repository* repo = New(log);
    if (repo == NULL) {
        printf("Error initializing repository.\n");
        fclose(log);
        return 1;
    }

    const char* db_name = "trello";
    const char* collection_name = "projects";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);
    if (repo->collection == NULL) {
        printf("Error: Could not get collection from database.\n");
        fclose(log);
        Cleanup(repo);
        return 1;
    }
    printf("Collection obtained successfully: %s\n", collection_name);

    bson_t* doc = bson_new();
    BSON_APPEND_UTF8(doc, "moderator", project->moderator);
    BSON_APPEND_UTF8(doc, "project", project->project);
    BSON_APPEND_UTF8(doc, "members", project->members);
    printf("Document prepared to insert: \nmoderator: %s\nproject: %s\nmembers: %s\n", project->moderator, project->project, project->members);

    bson_error_t error;
    if (!mongoc_collection_insert_one(repo->collection, doc, NULL, NULL, &error)) {
        fprintf(repo->logger, "Error: Insert failed\n");
        printf("Error: Insert failed. %s\n", error.message);
    }
    else {
        fprintf(repo->logger, "Document inserted successfully.\n");
        printf("Document inserted successfully.\n");
    }

    bson_destroy(doc);
    Cleanup(repo);
    fclose(log);
    return 0;
}

int repo() {
    printf("Initializing MongoDB...\n");
    mongoc_init();
    printf("MongoDB initialized.\n");

    getchar();
    mongoc_cleanup();
    printf("MongoDB cleanup completed.\n");

    return 0;
}
