#include "structs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, const char** argv) {
    FILE* fp;
    char* line = NULL;
    size_t len = 0, read;
    char addr[9], tmp[2], action, filename[FILENAME_LENGTH] = {0};

    for (int i = 1; i < argc; i+=2) {
        if (strcmp(argv[i], "-F") == 0) {
            strcpy(filename, argv[i+1]);
        }
    }
    if (filename[0] == 0) {
        printf("You need to give a filename using the '-F' flag.");
        return 1;
    }
    if ((fp = fopen(filename, "r")) == NULL) {
        printf("Could not open file. (%s)\n", filename);
        return 1;
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        sscanf(line, "%s %s\n", addr, tmp);
        action = tmp[0];
        // printf("%s %c\n", addr, action);
    }

    fclose(fp);
    if (line) {
        free(line);
    }
    return 0;
}