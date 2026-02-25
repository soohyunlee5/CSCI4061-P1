#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "./include/table.h"

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: mapreduce <directory> <n mappers> <n reducers>\n");
        return 1;
    }

    const char *dir_path = argv[1];
    int n_mappers = atoi(argv[2]);
    int n_reducers = atoi(argv[3]);

    if (n_mappers == 0 && strcmp(argv[2], "0") != 0) {
        perror("Invalid number of mappers");
        return 1;
    }

    if (n_reducers == 0 && strcmp(argv[3], "0") != 0) {
        perror("Invalid number of reducers");
        return 1;
    }

    if (n_mappers < 1 || n_reducers < 1) {
        printf("mapreduce: cannot have less than one mapper or reducer\n");
        return 1;
    }

    mkdir("./intermediate", 0777);
    chmod("./intermediate", 0777);
    mkdir("./out", 0777);
    chmod("./out", 0777);

    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir");
        return 1;
    }

    size_t count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        count++;
    }

    rewinddir(dir);

    char **files = malloc(sizeof(char*) * count);
    size_t index = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        size_t len = strlen(dir_path) + strlen(entry->d_name) + 2;
        files[index] = malloc(len);
        if (files[index] == NULL) {
            perror("Failed to allocate memory");
            return 1;
        }

        snprintf(files[index], len, "%s/%s", dir_path, entry->d_name);
        index++;
    }

    pid_t *map_pid = malloc(sizeof(pid_t) * n_mappers);
    if (map_pid == NULL) {
        perror("malloc() failed");
        return 1;
    }

    int total_files = (int)count;

    for (int i = 0; i < n_mappers; i++) {
        int start = (i * total_files) / n_mappers;
        int end = ((i + 1) * total_files) / n_mappers;
        int file_count = end - start;

        char output_path[MAX_PATH];
        snprintf(output_path, sizeof(output_path), "./intermediate/%d.tbl", i);

        int argc_map = file_count + 3;
        char **args = malloc(sizeof(char*) * argc_map);
        if (args == NULL) {
            perror("malloc() failed");
            return 1;
        }

        args[0] = "./map";
        args[1] = output_path;

        for (int j = 0; j < file_count; j++) {
            args[j + 2] = files[start + j];
        }
        args[file_count + 2] = NULL;

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork() failed");
            return 1;
        }

        if (pid == 0) {
            execv("./map", args);
            perror("exec failed");
            _exit(1);
        }

        map_pid[i] = pid;
        free(args);
    }

    int map_fail = 0;
    for (int i = 0; i < n_mappers; i++) {
        int status = 0;
        if (waitpid(map_pid[i], &status, 0) < 0) {
            perror("waitpid() failed");
            map_fail = 1;
        }

        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            fprintf(stderr, "map child failed\n");
            map_fail = 1;
        }
    }

    if (map_fail == 1) {
        free(map_pid);
        return 1;
    }

    free(map_pid);

    pid_t *reduce_pid = malloc(sizeof(pid_t) * n_reducers);
    if (reduce_pid == NULL) {
        perror("malloc() failed");
        return 1;
    }

    const long KEYSPACE = 1L << 16;

    for (int i = 0; i < n_reducers; i++) {
        long ip_start = (i * KEYSPACE) / n_reducers;
        long ip_end = ((i + 1) * KEYSPACE) / n_reducers;

        char output_path[MAX_PATH];
        snprintf(output_path, sizeof(output_path), "./out/%d.tbl", i);

        char *start_buf = malloc(32);
        char *end_buf = malloc(32);
        snprintf(start_buf, 32, "%ld", ip_start);
        snprintf(end_buf, 32, "%ld", ip_end);

        char **args = malloc(sizeof(char*) * 6);
        if (args == NULL) {
            perror("malloc() failed");
            return 1;
        }

        args[0] = "./reduce";
        args[1] = "./intermediate";
        args[2] = output_path;
        args[3] = start_buf;
        args[4] = end_buf;
        args[5] = NULL;

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork() failed");
            return 1;
        }

        if (pid == 0) {
            execv("./reduce", args);
            perror("exec() failed");
            _exit(1);
        }

        reduce_pid[i] = pid;
        free(args);
    }

    int reduce_fail = 0;
    for (int i = 0; i < n_reducers; i++) {
        int status = 0;
        if (waitpid(reduce_pid[i], &status, 0) < 0) {
            perror("waitpid() failed");
            reduce_fail = 1;
        }

        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            fprintf(stderr, "reduce child failed\n");
            reduce_fail = 1;
        }
    }

    if (reduce_fail == 1) {
        free(reduce_pid);
        return 1;
    }

    free(reduce_pid);

    DIR *output_dir = opendir("./out");
    if (output_dir == NULL) {
        perror("Error opening directory");
        return 1;
    }

    struct dirent *output_entry;
    while ((output_entry = readdir(output_dir)) != NULL) {
        if (strcmp(output_entry->d_name, ".") == 0 || strcmp(output_entry->d_name, "..") == 0) {
            continue;
        }

        char output_path[MAX_PATH];
        snprintf(output_path, sizeof(output_path), "./out/%s", output_entry->d_name);

        table_t *table = table_from_file(output_path);
        if (table == NULL) {
            fprintf(stderr, "failed to read output table");
            return 1;
        }

        table_print(table);
        table_free(table);
    }

    closedir(output_dir);
    closedir(dir);
    return 0;
}
