#include <mongoc/mongoc.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include "model.h"
#include "repo.h"
#include "SHA.h"

typedef struct {
    mongoc_client_t* client;
    mongoc_collection_t* collection;
    FILE* logger;
} Repository;

FILE* log;

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
        mongoc_collection_destroy(repo->collection);
        mongoc_client_destroy(repo->client);
        free(repo);
    }
}

int password_hash(const char* password, char* password_location) {

    SHA1Context sha;
    int err;
    uint8_t Message_Digest[20];

    err = SHA1Reset(&sha);
    if (err) {
        fprintf(stderr, "SHA1Reset failed with error code %d\n", err);
        return 1;
    }

    err = SHA1Input(&sha, (const unsigned char*)password, strlen(password));
    if (err) {
        fprintf(stderr, "SHA1Input failed with error code %d\n", err);
        return 2;
    }

    err = SHA1Result(&sha, Message_Digest);
    if (err)
    {
        fprintf(stderr, "SHA1Result Error %d, could not compute message digest.\n", err);
        return 3;
    }

    char hashedvalue[20 * 2 + 1];
    for (int i = 0; i < 20; i++) {
        sprintf(&hashedvalue[i * 2], "%02x", Message_Digest[i]);
    }
    hashedvalue[20 * 2] = '\0';
    (const char*)hashedvalue;
    printf(hashedvalue);
    printf("\n");
    strncpy(password_location, hashedvalue, 41);
    printf((const char*)password_location);
    printf("\n");

    return 0;
}

int activation_hash(const char* email, const char* username, char* activation_link) {

    Repository* repo = New(log);
    const char* db_name = "users";
    const char* collection_name = "links";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);
    fprintf(repo->logger, "Generating hash...\n");
    printf("Generating hash...\n");

    SHA1Context sha;
    int err;
    uint8_t Message_Digest[20];

    err = SHA1Reset(&sha);
    if (err) {
        fprintf(repo->logger, "SHA1Reset failed with error code %d\n", err);
        fprintf(stderr, "SHA1Reset failed with error code %d\n", err);
        Cleanup(repo);

        return 1;
    }

    // Input data
    int e_length = strlen(email);
    int u_length = strlen(username);
    char* forhash = (char*)malloc(e_length + u_length);
    int i, j;
    for (i = 0; i < e_length; ++i) {
        forhash[i] = email[i];
    }
    for (j = 0; j < u_length; ++j) {
        forhash[i + j] = username[j];
    }
    //forhash[e_length + u_length] = '\0';

    err = SHA1Input(&sha, (const unsigned char*)forhash, strlen(forhash));
    if (err) {
        fprintf(repo->logger, "SHA1Input failed with error code %d\n", err);
        fprintf(stderr, "SHA1Input failed with error code %d\n", err);
        free(forhash);
        Cleanup(repo);

        return 2;
    }
    free(forhash);

    err = SHA1Result(&sha, Message_Digest);
    if (err)
    {
        fprintf(repo->logger, "SHA1Result Error %d, could not compute message digest.\n", err);
        fprintf(stderr, "SHA1Result Error %d, could not compute message digest.\n", err);
        Cleanup(repo);

        return 3;
    }

    char hashedvalue[20 * 2 + 1];
    for (int i = 0; i < 20; i++) {
        sprintf(&hashedvalue[i * 2], "%02x", Message_Digest[i]);
    }
    hashedvalue[20 * 2] = '\0';
    (const char*)hashedvalue;
    printf(hashedvalue);
    printf("\n");
    strncpy(activation_link, hashedvalue, 41);
    printf((const char*)activation_link);
    printf("\n");

    bson_t* doc = bson_new();
    BSON_APPEND_UTF8(doc, "username", username);
    BSON_APPEND_UTF8(doc, "email", email);
    BSON_APPEND_UTF8(doc, "link", activation_link);
    BSON_APPEND_UTF8(doc, "type", "activation");
    BSON_APPEND_INT32(doc, "active", 1);

    bson_error_t error;
    if (!mongoc_collection_insert_one(repo->collection, doc, NULL, NULL, &error)) {
        fprintf(repo->logger, "Error: Insert failed\n");
        printf("Error: Insert failed\n");
        bson_destroy(doc);
        Cleanup(repo);

        return 4;
    }
    else {
        fprintf(repo->logger, "Document inserted successfully.\n");
        printf("Document inserted successfully.\n");
    }

    bson_destroy(doc);
    Cleanup(repo);

    return 0;
}

int emailto(char email[], FILE* payload_file) {

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
        recipients = curl_slist_append(recipients, email);
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

            return 1;
        }

        printf("curl ok?.\n");

        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);

        printf("curl ok????????????.\n");

        return 0;
    }

    return 2;
}

int adduser(User *user) {

    Repository *repo = New(log); 
    const char *db_name = "users";
    const char *collection_name = "users";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);
    printf("Dodavanje korisnika.\n");

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

        return 1;
    }

    bson_destroy(query);
    mongoc_cursor_destroy(cursor);

    bson_t* doc = bson_new();
    BSON_APPEND_UTF8(doc, "username", user->username);
    BSON_APPEND_UTF8(doc, "first_name", user->first_name);
    BSON_APPEND_UTF8(doc, "last_name", user->last_name);
    BSON_APPEND_UTF8(doc, "email", user->email);
    char password[41];
    password_hash(user->password, password);
    BSON_APPEND_UTF8(doc, "password", password);
    BSON_APPEND_UTF8(doc, "role", user->role);
    BSON_APPEND_INT32(doc, "active", 0);

    bson_error_t error;
    if (!mongoc_collection_insert_one(repo->collection, doc, NULL, NULL, &error)) {
        fprintf(repo->logger, "Error: Insert failed\n");
        printf("Error: Insert failed\n");
        bson_destroy(doc);
        Cleanup(repo);

        return 2;
    }
    else {
        fprintf(repo->logger, "Document inserted successfully.\n");
        printf("Document inserted successfully.\n");
    }

    bson_destroy(doc);

    char activation_link[41];
    if (!activation_hash(user->email, user->username, activation_link)) {
        fprintf(repo->logger, "Hash generated successfully.\n");
        printf("Hash generated successfully.\n");
    }
    else {
        fprintf(repo->logger, "Failed to generate hash.\n");
        printf("Failed to generate hash.\n");

        return 5;
    }

    FILE* payload_file = fopen("email_payload.txt", "w");
    if (payload_file) {
        fprintf(payload_file, "To: %s\r\n"
            "From: trello clone\r\n"
            "Subject: Test Email\r\n"
            "\r\n"
            "Vas aktivacioni kod: http://localhost:3000/activate?link=%s\r\n", user->email, (const char*)activation_link);
        fclose(payload_file);
        fprintf(repo->logger, "Written to the payload file.\n");
    }
    else {
        fprintf(repo->logger, "Failed to open payload file for writing.\n");
        printf("Failed to open payload file for writing.\n");
        Cleanup(repo);

        return 3;
    }

    payload_file = fopen("email_payload.txt", "r");
    if (!emailto(user->email, payload_file)) {
        printf("ubicu se bukvalno.\n");
        fprintf(repo->logger, "Email sent successfully.\n");
        printf("Email sent successfully.\n");
    }
    else {
        fprintf(repo->logger, "Failed to send email.\n");
        printf("Failed to send email.\n");
        fclose(payload_file);
        Cleanup(repo);

        return 4;
    }
    fclose(payload_file);

    Cleanup(repo);

    return 0;

}

int activate_user(const char* username, const char* email) {

    Repository* repo = New(log);
    const char* db_name = "users";
    const char* collection_name = "users";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);
    printf("Aktiviranje korisnika.\n");

    bson_t* filter = BCON_NEW(
        "$and", "[",
        "{", "username", BCON_UTF8(username), "}",
        "{", "email", BCON_UTF8(email), "}",
        "]"
    );
    printf("Oof.\n");

    // Define the update operation
    bson_t* update = BCON_NEW("$set", "{", "active", BCON_INT32(1), "}");
    printf("Oof.\n");

    // Perform the update
    bson_error_t error;
    int result = mongoc_collection_update_one(
        repo->collection,  // Collection handle
        filter,      // Filter document
        update,      // Update document
        NULL,        // No additional options
        NULL,        // No reply document needed
        &error       // Error object
    );
    printf("Oof.\n");

    if (result) {
        printf("Document updated successfully.\n");
    }
    else {
        fprintf(stderr, "Update failed: %s\n", error.message);
        printf("I hate standard error.\n");
        Cleanup(repo);

        return 1;
    }

    FILE* payload_file = fopen("confirmation.txt", "w");
    if (payload_file) {
        fprintf(payload_file, "To: %s\r\n"
            "From: trello clone\r\n"
            "Subject: Test Email\r\n"
            "\r\n"
            "Vas nalog je aktiviran.\r\n", username);
        fclose(payload_file);
        fprintf(repo->logger, "Written to the payload file.\n");
    }
    else {
        fprintf(repo->logger, "Failed to open payload file for writing.\n");
        printf("Failed to open payload file for writing.\n");
        Cleanup(repo);

        return 3;
    }

    payload_file = fopen("confirmation.txt", "r");
    if (!emailto(email, payload_file)) {
        printf("ubicu se bukvalno.\n");
        fprintf(repo->logger, "Email sent successfully.\n");
        printf("Email sent successfully.\n");
    }
    else {
        fprintf(repo->logger, "Failed to send email.\n");
        printf("Failed to send email.\n");
        fclose(payload_file);
        Cleanup(repo);

        return 4;
    }
    fclose(payload_file);

    Cleanup(repo);

    return 0;
}

int deactivate_code(const char* link) {

    Repository* repo = New(log);
    const char* db_name = "users";
    const char* collection_name = "links";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);
    printf("Deaktiviranje koda.\n");

    bson_t* filter = BCON_NEW(
        "$and", "[",
        "{", "link", BCON_UTF8(link), "}",
        "{", "type", BCON_UTF8("activation"), "}",
        "]"
    );

    // Define the update operation
    bson_t* update = BCON_NEW("$set", "{", "active", BCON_INT32(0), "}");

    // Perform the update
    bson_error_t error;
    int result = mongoc_collection_update_one(
        repo->collection,  // Collection handle
        filter,      // Filter document
        update,      // Update document
        NULL,        // No additional options
        NULL,        // No reply document needed
        &error       // Error object
    );

    if (result) {
        printf("Document updated successfully2.\n");
    }
    else {
        fprintf(stderr, "Update failed: %s\n", error.message);
        Cleanup(repo);

        return 1;
    }

    Cleanup(repo);

    return 0;
}

int check_activation(const char* link) {

    Repository* repo = New(log);
    const char* db_name = "users";
    const char* collection_name = "links";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);
    printf("Provera aktivacionog koda.\n");

    const char* type = "activation";

    // Build the query
    bson_t* query = BCON_NEW(
        "$and", "[",
        "{", "link", BCON_UTF8(link), "}",
        "{", "type", BCON_UTF8(type), "}",
        "]"
    );

    mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(repo->collection, query, NULL, NULL);
    const bson_t* doc;
    char username[50];
    char email[50];
    int res1 = 1;
    int res2 = 1;
    int already_activated = 0;
    while (mongoc_cursor_next(cursor, &doc)) {
        // Extract and print "username" field
        bson_iter_t iter;
        if (bson_iter_init(&iter, doc)) {
            if (bson_iter_find(&iter, "username")) {
                const char* found_username = bson_iter_utf8(&iter, NULL);
                printf("Found user: %s\n", found_username);
                strncpy(username, found_username, sizeof(username));
                username[sizeof(username) - 1] = '\0';
                res1 = 0;
            }

            // Extract other fields
            if (bson_iter_find(&iter, "email")) {
                const char* found_email = bson_iter_utf8(&iter, NULL);
                printf("With email: %s\n", found_email);
                strncpy(email, found_email, sizeof(email));
                email[sizeof(email) - 1] = '\0';
                res2 = 0;
            }
            if (bson_iter_find(&iter, "active")) {
                int active = bson_iter_int32(&iter);
                printf("Active: %d\n", active);
                if (!active) {
                    already_activated = 1;
                }
            }
        }
    }
    if (already_activated) {
        printf("Link already used\n");
        bson_destroy(query);
        mongoc_cursor_destroy(cursor);

        return 1;
    }

    if (!res1 && !res2) {
        int activated = activate_user((const char*)username, (const char*)email);
        printf("%s a %s\n", (const char*)username, (const char*)email);
        printf("%d\n", activated);
        if (activated) {
            printf("Activation failed\n");
            bson_destroy(query);
            mongoc_cursor_destroy(cursor);

            return 3;
        }
        int deactivated = deactivate_code(link);
        if (deactivated) {
            printf("Link deactivation failed\n");
            bson_destroy(query);
            mongoc_cursor_destroy(cursor);

            return 4;
        }
    }
    else {
        printf("No such link\n");
        bson_destroy(query);
        mongoc_cursor_destroy(cursor);

        return 2;
    }

    bson_destroy(query);
    mongoc_cursor_destroy(cursor);

    return 0;
}

int parse_credentials_from_json(const cJSON* json, char role[]) {

    cJSON* username_or_email = cJSON_GetObjectItem(json, "username_or_email");
    cJSON* password = cJSON_GetObjectItem(json, "password");
    if (!cJSON_IsString(username_or_email) || !cJSON_IsString(password)) {
        return 1;
    }

    char hashed_password[41];
    password_hash(password->valuestring, hashed_password);

    Repository* repo = New(log);
    const char* db_name = "users";
    const char* collection_name = "users";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);
    printf("Trazenje korisnika.\n");

    // Build the query
    bson_t* query = BCON_NEW(
        "$and", "[",
        "{", "email", BCON_UTF8(username_or_email->valuestring), "}",
        "{", "password", BCON_UTF8(hashed_password), "}",
        "]"
    );
    printf("Trazenje nigera.\n");

    mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(repo->collection, query, NULL, NULL);
    const bson_t* doc;
    if (mongoc_cursor_next(cursor, &doc)) {
        bson_iter_t iter;
        if (bson_iter_init(&iter, doc)) {
            if (bson_iter_find(&iter, "role")) {
                const char* found_role = bson_iter_utf8(&iter, NULL);
                printf("Found user: %s\n", found_role);
                strncpy(role, found_role, sizeof(role));
                role[sizeof(role) - 1] = '\0';
            }
            if (bson_iter_find(&iter, "active")) {
                int active = bson_iter_int32(&iter);
                printf("Active: %d\n", active);
                if (!active) {
                    return 1;
                }
            }
        }
        fprintf(repo->logger, "User found.\n");
        printf("User found.\n");
        bson_destroy(query);
        mongoc_cursor_destroy(cursor);
        Cleanup(repo);

        return 0;
    }
    printf("Trazenje korisnika.\n");

    bson_destroy(query);
    mongoc_cursor_destroy(cursor);

    bson_t* query2 = BCON_NEW(
        "$and", "[",
        "{", "username", BCON_UTF8(username_or_email->valuestring), "}",
        "{", "password", BCON_UTF8(hashed_password), "}",
        "]"
    );

    mongoc_cursor_t* cursor2 = mongoc_collection_find_with_opts(repo->collection, query2, NULL, NULL);
    const bson_t* doc2;
    if (mongoc_cursor_next(cursor2, &doc2)) {
        bson_iter_t iter2;
        if (bson_iter_init(&iter2, doc2)) {
            if (bson_iter_find(&iter2, "role")) {
                const char* found_role = bson_iter_utf8(&iter2, NULL);
                printf("Found user: %s\n", found_role);
                strncpy(role, found_role, sizeof(role));
                role[sizeof(role) - 1] = '\0';
            }
            if (bson_iter_find(&iter2, "active")) {
                int active = bson_iter_int32(&iter2);
                printf("Active: %d\n", active);
                if (!active) {
                    return 1;
                }
            }
        }
        fprintf(repo->logger, "User found.\n");
        printf("User found.\n");
        bson_destroy(query2);
        mongoc_cursor_destroy(cursor2);
        Cleanup(repo);

        return 0;
    }

    bson_destroy(query2);
    mongoc_cursor_destroy(cursor2);
    Cleanup(repo);

    return 1;
}

int find_users(const char* name, User users[], int size, int* number_of_results) {

    Repository* repo = New(log);
    const char* db_name = "users";
    const char* collection_name = "users";
    repo->collection = mongoc_client_get_collection(repo->client, db_name, collection_name);
    printf("Trazenje korisnika.\n");
    printf("IHATENIGGERS %d\n", size);

    bson_t* query = BCON_NEW(
        "$and", "[",
        "{", "username", "{", "$regex", BCON_UTF8(name), "}", "}",
        "{", "active", BCON_INT32(1), "}",
        "]"
    );

    mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(repo->collection, query, NULL, NULL);
    const bson_t* doc;
    while (mongoc_cursor_next(cursor, &doc) && *number_of_results < size) {
        bson_iter_t iter;

        // Initialize the User struct
        memset(&users[*number_of_results], 0, sizeof(User));

        // Parse the document fields into the User struct
        if (bson_iter_init_find(&iter, doc, "username") && BSON_ITER_HOLDS_UTF8(&iter)) {
            strncpy(users[*number_of_results].username, bson_iter_utf8(&iter, NULL), sizeof(users[*number_of_results].username) - 1);
        }
        if (bson_iter_init_find(&iter, doc, "first_name") && BSON_ITER_HOLDS_UTF8(&iter)) {
            strncpy(users[*number_of_results].first_name, bson_iter_utf8(&iter, NULL), sizeof(users[*number_of_results].first_name) - 1);
        }
        if (bson_iter_init_find(&iter, doc, "last_name") && BSON_ITER_HOLDS_UTF8(&iter)) {
            strncpy(users[*number_of_results].last_name, bson_iter_utf8(&iter, NULL), sizeof(users[*number_of_results].last_name) - 1);
        }

        printf("number of results: %d\n", *number_of_results);
        ++(*number_of_results);
    }
    printf("nigger %d\n", *number_of_results);

    if (mongoc_cursor_error(cursor, NULL)) {
        fprintf(stderr, "Error occurred while iterating over the cursor.\n");

        return 1;
    }

    bson_destroy(query);
    mongoc_cursor_destroy(cursor);
    Cleanup(repo);

    return 0;
}

int repo() {

    log = fopen("log.txt", "w");
    if (!log) {
        printf("Error opening file!\n");
        return 1;
    }

    mongoc_init();
    getchar();
    mongoc_cleanup();

    if (log) {
        fclose(log);
    }

    return 0;

}
