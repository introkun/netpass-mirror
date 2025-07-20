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

    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "[DEBUG]")) foundDebug++;
        if (strstr(line, "[INFO]"))  foundInfo++;
        if (strstr(line, "[WARN]"))  foundWarn++;
        if (strstr(line, "[ERROR]")) foundError++;
    }
    fclose(f);

    //printf("foundDebug %d, expectedDebug %d\n", foundDebug, expectedDebug);
    assert(foundDebug == expectedDebug && "Unexpected number of debug log entries");
    assert(foundInfo  == expectedInfo  && "Unexpected number of info log entries");
    assert(foundWarn  == expectedWarn && "Unexpected number of warn log entries");
    assert(foundError == expectedError && "Unexpected number of error log entries");
}

static void write_logs() {
    logDebug("debug message\n");
    logInfo("info message\n");
    logWarn("warn message\n");
    logError("error message\n");
}

#define RUN_TEST(fn, expectedD, expectedI, expectedW, expectedE) do { \
    truncate_log_file(); \
    fn(); \
    assert_log_contains(expectedD, expectedI, expectedW, expectedE); \
    printf("[PASS] %s\n", #fn); \
} while (0)

#define RUN_TEST_WITH_1_PARAM(fn, param, expectedD, expectedI, expectedW, expectedE) do { \
    truncate_log_file(); \
    fn(param); \
    assert_log_contains(expectedD, expectedI, expectedW, expectedE); \
    printf("[PASS] %s(%d)\n", #fn, param); \
} while (0)

static void test_logger_file_write() {
    loggerInit(LOG_LEVEL_DEBUG, LOG_OUTPUT_FILE, LOG_FILE_PATH);
    logDebug("debug message");
    logInfo("info message");
    loggerClose();
}

static void test_logger_debug() {
    loggerInit(LOG_LEVEL_DEBUG, LOG_OUTPUT_FILE, LOG_FILE_PATH);
    write_logs();

    loggerClose();
}

static void test_logger_info() {
    loggerInit(LOG_LEVEL_INFO, LOG_OUTPUT_FILE, LOG_FILE_PATH);
    write_logs();

    loggerClose();
}

static void test_logger_warn() {
    loggerInit(LOG_LEVEL_WARN, LOG_OUTPUT_FILE, LOG_FILE_PATH);
    write_logs();

    loggerClose();
}

static void test_logger_error() {
    loggerInit(LOG_LEVEL_ERROR, LOG_OUTPUT_FILE, LOG_FILE_PATH);
    write_logs();

    loggerClose();
}

static void test_logger_fallback_to_info_for_incorrect_level(int level) {
    loggerInit(level, LOG_OUTPUT_FILE, LOG_FILE_PATH);
    write_logs();
    loggerClose();
}

static void test_logger_empty_file_output() {
    loggerInit(LOG_LEVEL_DEBUG, LOG_OUTPUT_FILE, LOG_FILE_PATH);
    logInfo("info message");
    loggerInit(LOG_LEVEL_DEBUG, LOG_OUTPUT_FILE, NULL);  // Disable output
    logInfo("info message");
    loggerClose();
}

static void test_logger_fallback_to_screen_for_incorrect_output(int output) {
    loggerInit(LOG_LEVEL_DEBUG, output, LOG_FILE_PATH);
    write_logs();
    loggerClose();
}

static void test_logger_level_both_logs_to_file() {
    loggerInit(LOG_LEVEL_DEBUG, LOG_OUTPUT_BOTH, LOG_FILE_PATH);
    write_logs();

    loggerClose();
}

static void test_logger_level_screen_doesnt_log_to_file() {
    loggerInit(LOG_LEVEL_DEBUG, LOG_OUTPUT_SCREEN, LOG_FILE_PATH);
    write_logs();

    loggerClose();
}

int main() {
    RUN_TEST(test_logger_file_write, 1, 1, 0, 0);
    RUN_TEST(test_logger_empty_file_output, 0, 2, 0, 0);
    RUN_TEST(test_logger_debug, 1, 1, 1, 1);
    RUN_TEST(test_logger_info, 0, 1, 1, 1);
    RUN_TEST(test_logger_warn, 0, 0, 1, 1);
    RUN_TEST(test_logger_error, 0, 0, 0, 1);

    RUN_TEST_WITH_1_PARAM(test_logger_fallback_to_info_for_incorrect_level, 10, 0, 1, 1, 1);
    RUN_TEST_WITH_1_PARAM(test_logger_fallback_to_info_for_incorrect_level, -10, 0, 1, 1, 1);
    RUN_TEST_WITH_1_PARAM(test_logger_fallback_to_screen_for_incorrect_output, 10, 0, 0, 0, 0);
    RUN_TEST_WITH_1_PARAM(test_logger_fallback_to_screen_for_incorrect_output, -10, 0, 0, 0, 0);

    RUN_TEST(test_logger_level_both_logs_to_file, 1, 1, 1, 1);
    RUN_TEST(test_logger_level_screen_doesnt_log_to_file, 0, 0, 0, 0);

    printf("All tests passed!\n");
    return 0;
}
