#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../source/logger.h"

#define LOG_FILE_PATH "test_log.txt"
#define TEST_LOG_PATH "./log/test_log.txt"

static void truncate_log_file() {
    FILE* f = fopen(TEST_LOG_PATH, "w");
    assert(f && "Failed to truncate log file");
    fclose(f);
}

static void assert_log_contains(const int expectedDebug, const int expectedInfo, const int expectedWarn,
    const int expectedError) {
    char line[512];
    int foundDebug = 0, foundInfo = 0, foundWarn = 0, foundError = 0;

    FILE* f = fopen(TEST_LOG_PATH, "r");
    assert(f && "Failed to open log file");

    rewind(f);

    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "[DEBUG]")) foundDebug++;
        if (strstr(line, "[INFO]"))  foundInfo++;
        if (strstr(line, "[WARN]"))  foundWarn++;
        if (strstr(line, "[ERROR]")) foundError++;
    }
    fclose(f);

    printf("foundDebug %d, expectedDebug %d\n", foundDebug, expectedDebug);
    assert(foundDebug == expectedDebug && "Unexpected number of debug log entries");
    assert(foundInfo  == expectedInfo  && "Unexpected number of info log entries");
    assert(foundWarn  == expectedWarn && "Unexpected number of warn log entries");
    assert(foundError == expectedError && "Unexpected number of error log entries");
}

static void test_logger_file_write() {
    loggerInit(LOG_LEVEL_DEBUG, LOG_OUTPUT_FILE, LOG_FILE_PATH);
    logDebug("debug message");
    logInfo("info message");
    loggerClose();
    printf("[PASS] test_logger_file_write\n");
}

static void test_logger_debug() {
    loggerInit(LOG_LEVEL_DEBUG, LOG_OUTPUT_FILE, LOG_FILE_PATH);
    logDebug("debug message");
    logInfo("info message");
    logWarn("warn message");
    logError("error message");

    loggerClose();
    printf("[PASS] test_logger_debug\n");
}

static void test_logger_info() {
    loggerInit(LOG_LEVEL_INFO, LOG_OUTPUT_FILE, LOG_FILE_PATH);
    logDebug("debug message");
    logInfo("info message");
    logWarn("warn message");
    logError("error message");

    loggerClose();
    printf("[PASS] test_logger_info\n");
}

static void test_logger_warn() {
    loggerInit(LOG_LEVEL_WARN, LOG_OUTPUT_FILE, LOG_FILE_PATH);
    logDebug("debug message");
    logInfo("info message");
    logWarn("warn message");
    logError("error message");

    loggerClose();
    printf("[PASS] test_logger_warn\n");
}

static void test_logger_error() {
    loggerInit(LOG_LEVEL_ERROR, LOG_OUTPUT_FILE, LOG_FILE_PATH);
    logDebug("debug message");
    logInfo("info message");
    logWarn("warn message");
    logError("error message");

    loggerClose();
    printf("[PASS] test_logger_error\n");
}

static void test_logger_fallback_to_info_for_incorrect_level_1() {
    loggerInit(10, LOG_OUTPUT_FILE, LOG_FILE_PATH);
    logDebug("debug message");
    logInfo("info message");
    logWarn("warn message");
    logError("error message");

    loggerClose();
    printf("[PASS] test_logger_fallback_to_info_for_incorrect_level_1\n");
}

static void test_logger_fallback_to_info_for_incorrect_level_2() {
    loggerInit(-10, LOG_OUTPUT_FILE, LOG_FILE_PATH);
    logDebug("debug message");
    logInfo("info message");
    logWarn("warn message");
    logError("error message");

    loggerClose();
    printf("[PASS] test_logger_fallback_to_info_for_incorrect_level_2\n");
}

static void test_logger_empty_file_output() {
    loggerInit(LOG_LEVEL_DEBUG, LOG_OUTPUT_FILE, LOG_FILE_PATH);
    logInfo("info message");
    loggerInit(LOG_LEVEL_DEBUG, LOG_OUTPUT_FILE, NULL);  // Disable output
    logInfo("info message");
    loggerClose();
    printf("[PASS] test_logger_empty_file_output\n");
}

static void test_logger_fallback_to_screen_for_incorrect_output_1() {
    loggerInit(LOG_LEVEL_DEBUG, 10, LOG_FILE_PATH);
    logDebug("debug message");
    logInfo("info message");
    logWarn("warn message");
    logError("error message");

    loggerClose();
    printf("[PASS] test_logger_fallback_to_screen_for_incorrect_output_1\n");
}

static void test_logger_fallback_to_screen_for_incorrect_output_2() {
    loggerInit(LOG_LEVEL_DEBUG, -10, LOG_FILE_PATH);
    logDebug("debug message");
    logInfo("info message");
    logWarn("warn message");
    logError("error message");

    loggerClose();
    printf("[PASS] test_logger_fallback_to_screen_for_incorrect_output_2\n");
}

static void test_logger_level_both_logs_to_file() {
    loggerInit(LOG_LEVEL_DEBUG, LOG_OUTPUT_BOTH, LOG_FILE_PATH);
    logDebug("debug message");
    logInfo("info message");
    logWarn("warn message");
    logError("error message");

    loggerClose();
    printf("[PASS] test_logger_level_both_logs_to_file\n");
}

static void test_logger_level_screen_doesnt_log_to_file() {
    loggerInit(LOG_LEVEL_DEBUG, LOG_OUTPUT_SCREEN, LOG_FILE_PATH);
    logDebug("debug message");
    logInfo("info message");
    logWarn("warn message");
    logError("error message");

    loggerClose();
    printf("[PASS] test_logger_level_screen_doesnt_log_to_file\n");
}

int main() {
    truncate_log_file();
    test_logger_file_write();
    assert_log_contains(1, 1, 0, 0);

    truncate_log_file();
    test_logger_empty_file_output();
    assert_log_contains(0, 2, 0, 0);

    truncate_log_file();
    test_logger_debug();
    assert_log_contains(1, 1, 1, 1);

    truncate_log_file();
    test_logger_info();
    assert_log_contains(0, 1, 1, 1);

    truncate_log_file();
    test_logger_warn();
    assert_log_contains(0, 0, 1, 1);

    truncate_log_file();
    test_logger_error();
    assert_log_contains(0, 0, 0, 1);

    truncate_log_file();
    test_logger_fallback_to_info_for_incorrect_level_1();
    assert_log_contains(0, 1, 1, 1);

    truncate_log_file();
    test_logger_fallback_to_info_for_incorrect_level_2();
    assert_log_contains(0, 1, 1, 1);

    truncate_log_file();
    assert_log_contains(0, 0, 0, 0);
    test_logger_fallback_to_screen_for_incorrect_output_1();
    assert_log_contains(0, 0, 0, 0);

    truncate_log_file();
    test_logger_fallback_to_screen_for_incorrect_output_2();
    assert_log_contains(0, 0, 0, 0);

    truncate_log_file();
    test_logger_level_both_logs_to_file();
    assert_log_contains(1, 1, 1, 1);

    truncate_log_file();
    test_logger_level_screen_doesnt_log_to_file();
    assert_log_contains(0, 0, 0, 0);

    printf("All tests passed!\n");
    return 0;
}
