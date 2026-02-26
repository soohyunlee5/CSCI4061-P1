#include "./include/reduce.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "./include/table.h"
#include <dirent.h>

int main(int argc, char *argv[]) {
  // Check for correct number of arguments
  if (argc != 5) {
    printf("Usage: reduce <read dir> <out file> <start ip> <end ip>\n");
    return 1;
  }

  // IP Part
  char *finishPtr1, *finishPtr2;

  // Convert strings to numbers from argv[3] and argv[4]
  long start_ip = strtol(argv[3], &finishPtr1, 10);
  long end_ip = strtol(argv[4], &finishPtr2, 10);

  // If either string had non-digit characters, and
  // pointers are not at the null terminator, they are invalid
  if (*finishPtr1 != '\0' || *finishPtr2 != '\0') {
    printf("reduce: invalid IP range\n");
    return 1;
  }

  // Initialize the table
  table_t *table = table_init();
  if (table == NULL) {
    perror("Error allocating table");
    return 1;
  }

  // Directory part
  // Open the directory
  DIR *dir = opendir(argv[1]);

  // NullErrorHandling
  if (dir == NULL) {
    table_free(table);
    perror("Error opending directory");
    return 1;
  }

  struct dirent *direc;
  char finalPath[MAX_PATH];

  // Read every file in the read dir
  while ((direc = readdir(dir)) != NULL) {
    // Skip '.' and '..' in directory
    if (strcmp(direc->d_name, ".") == 0 || strcmp(direc->d_name, "..") == 0) {
      continue;
    }

    // Copy the base directory into empty string
    strcpy(finalPath, argv[1]);
    // Append '/' to the end of string
    strcat(finalPath, "/");
    // Append file name to the end
    strcat(finalPath, direc->d_name);

    // Get a table from binary file
    table_t *curr_table = table_from_file(finalPath);
    if (curr_table == NULL) {
      continue;
    }

    // Merge curr_table into table
    for (int i = 0; i < TABLE_LEN; i++) {
      bucket_t *curr = curr_table->buckets[i];

      while (curr != NULL) {
        long ipAddress = atol(curr->ip);

        // If IP exists, add requests
        if (ipAddress >= start_ip && ipAddress < end_ip) {
          bucket_t *IP_bucket = table_get(table, curr->ip);

          if (IP_bucket != NULL) { 
            IP_bucket->requests += curr->requests;
          } else {
            bucket_t *new_bucket = bucket_init(curr->ip);
            if (new_bucket != NULL) { 
              new_bucket->requests = curr->requests;
              table_add(table, new_bucket);
            }
          }
        }
        curr = curr->next;
      }
    }

    table_free(curr_table);
  }
  
  // Close the directory
  closedir(dir);

  // Write the table to the output file
  if (table_to_file(table, argv[2]) != 0) {
    perror("failed to write a file");
    table_free(table);
    return 1;
  }

  table_free(table);
  
  return 0;
}

int reduce_file(table_t *table, const char file_path[MAX_PATH],
                const int start_ip, const int end_ip) {
  (void)table;
  (void)file_path;
  (void)start_ip;
  (void)end_ip;
  return 0;
}
