#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../source/logger.h"

void test_logger_file_write() {
    loggerInit(LOG_LEVEL_DEBUG, LOG_OUTPUT_FILE, "test_log.txt");
    logDebug("debug message");
    logInfo("info message");
    loggerClose();

    FILE* f = fopen("./log/test_log.txt", "r");
    assert(f && "Failed to open log file");

    char line[512];
    int foundDebug = 0, foundInfo = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "debug")) foundDebug += 1;
        if (strstr(line, "info")) foundInfo += 1;
    }

    fclose(f);

    assert(foundDebug && "Missing debug log");
    assert(foundInfo && "Missing info log");
    printf("[PASS] test_logger_file_write\n");
}

void test_logger_empty_file_output() {
    loggerInit(LOG_LEVEL_DEBUG, LOG_OUTPUT_FILE, "test_log.txt");
    logInfo("info message");
    loggerInit(LOG_LEVEL_DEBUG, LOG_OUTPUT_FILE, NULL);
    logInfo("info message");
    loggerClose();

    FILE* f = fopen("./log/test_log.txt", "r");
    assert(f && "Failed to open log file");

    char line[512];
    int foundDebug = 0, foundInfo = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "debug")) foundDebug += 1;
        if (strstr(line, "info")) foundInfo += 1;
    }

    fclose(f);

    assert(foundDebug == 0 && "Found debug log");
    assert(foundInfo == 2 && "Missing 2x info log");

    printf("[PASS] test_logger_empty_file_output\n");
}

int main() {
    test_logger_file_write();
    test_logger_empty_file_output();
    printf("All tests passed!\n");
    return 0;
}
