#include "./include/map.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "./include/table.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: map <outfile> <infiles...>\n");
        return 1;
    }

    const char *outfile = argv[1];

    table_t *table = table_init();
    if (table == NULL) {
        perror("Error allocating table");
        return 1;
    }

    for (int i = 2; i < argc; i++) {
        if (map_log(table, argv[i]) != 0) {
            table_free(table);
            perror("map_log failed");
            return 1;
        }
    }

    if (table_to_file(table, outfile) != 0) {
        table_free(table);
        perror("Failed to write table to file");
        return 1;
    }

    table_free(table);
    return 0;
}

int map_log(table_t *table, const char file_path[MAX_PATH]) {
    if (table == NULL || file_path == NULL) {
        perror("Table or file path is invalid");
        return 1;
    }

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("Failed to open file");
        return 1;
    }

    char buffer[100];

    while (fgets(buffer, sizeof(buffer), file) != NULL) {

        char time[20];
        char ip[IP_LEN];
        char method[8];
        char endpoint[37];
        char status[4];

        if (sscanf(buffer, "%19[^,], %15[^,], %7[^,], %36[^,], %3s", time, ip, method, endpoint, status) == 5) {
            bucket_t *bucket = table_get(table, ip);
            if (bucket == NULL) {
                bucket_t *new_bucket = bucket_init(ip);
                if (new_bucket == NULL) {
                    fclose(file);
                    perror("Newly made bucket is invalid");
                    return 1;
                }
                new_bucket->requests = 1;
                if (table_add(table, new_bucket) != 0) {
                    free(new_bucket);
                    fclose(file);
                    perror("Failed to add bucket to table");
                    return 1;
                }
            } else {
                bucket->requests++;
            }
        }
    }
    fclose(file);
    return 0;
}
