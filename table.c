#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./include/map.h"

bucket_t *bucket_init(const char ip[IP_LEN]) {
    if (ip == NULL) {
        return NULL;
    }

    bucket_t *bucket = malloc(sizeof(bucket_t));
    if (bucket == NULL) {
        return NULL;
    }

    int i;

    for (i = 0; i < IP_LEN-1 && ip[i] != '\0'; i++) {
        bucket->ip[i] = ip[i];
    }

    bucket->ip[i] = '\0'; 

    bucket->requests = 0;
    bucket->next = NULL;

    return bucket;
}

table_t *table_init() {
    table_t *table = malloc(sizeof(table_t));

    if(table == NULL) {
        return NULL;
    }

    for (int i = 0; i < TABLE_LEN; i++) {
        table->buckets[i] = NULL;
    }

    return table;
}

void table_print(const table_t *table) {
    if (table == NULL) {
        return; 
    }

    for (int i = 0; i < TABLE_LEN; i++) { 
        bucket_t *bucket = table->buckets[i];
        while (bucket != NULL) {
            printf('{%s} - {%d}\n', bucket->ip, bucket->requests);
            bucket = bucket->next;
        }
    }
}

void table_free(table_t *table) {
    if (table == NULL) {
        return;
    }

    for (int i = 0; i < TABLE_LEN; i++) { 
        bucket_t *bucket = table->buckets[i];
        while (bucket != NULL) {
            bucket_t *new_buc = bucket;
            bucket = bucket->next;
            free(new_buc);
        }
    }

    free(table);
}

int table_add(table_t *table, bucket_t *bucket) {
    if (table == NULL || bucket == NULL || bucket->ip[0] == '\0') {
        return -1;
    }

    if (table_get(table, bucket->ip) != NULL) { 
        return -1;
    } 

    int index = hash_ip(bucket->ip);
    if (index < 0) {
        return -1;
    }

    bucket->next = table->buckets[index];
    table->buckets[index] = bucket;

    return 0;
}

bucket_t *table_get(table_t *table, const char ip[IP_LEN]) {
    if (table == NULL || ip == NULL) {
        return NULL;
    }

    int index = hash_ip(ip);
    if (index < 0 ) return NULL;

    bucket_t *bucket = table->buckets[index];
    while (bucket != NULL) { 
        if (strcmp(bucket->ip, ip) == 0) {
            return bucket;
        }
        bucket = bucket->next;
    }

    return NULL;
}

int hash_ip(const char ip[IP_LEN]) {
    if (ip == NULL) {
        return -1;
    }

    int result = 0;
    for (int i = 0; i < IP_LEN && ip[i] != '\0'; i++) {
        result += ip[i];
    }

    if (result < 0) {
        return -1;
    }

    return sum % TABLE_LEN;
}

int table_to_file(table_t *table, const char out_file[MAX_PATH]) {
    if (table == NULL || out_file == NULL) {
        return -1;
    }

    FILE *file = fopen(out_file, "wb");
    if (file == NULL) return -1;

    for (int i = 0; i < TABLE_LEN; i++) {
        bucket_t *bucket = table->buckets[i];
        while (bucket != NULL) {
            if (fwrite(bucket, sizeof(bucket_t), 1, file) != 1) {
                fclose(file);
                return -1;
            }
            bucket = bucket->next;
        }
    }

    fclose(file);
    return 0;
}

table_t *table_from_file(const char in_file[MAX_PATH]) {
    if (in_file == NULL) return NULL;

    FILE *file = fopen(in_file, "rb");
    if (file == NULL) return NULL;

    table_t *table = table_init();
    if (table == NULL) {
        fclose(file);
        return NULL;
    }

    bucket_t bucket;

    while(fread(&bucket, sizeof(bucket_t), 1, file) == 1) { 
        bucket_t *temp_bucket = bucket_init(bucket.ip);
        if (temp_bucket != NULL) {
            temp_bucket->requests = bucket.requests;
            table_add(table, temp_bucket);
        }
    }

    fclose(file);
    return NULL;
}
