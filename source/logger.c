/**
* NetPass
 * Copyright (C) 2025 Introkun
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "logger.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>

#ifdef __3DS__
#include <3ds.h>
#else
#include <pthread.h>
#endif

#ifdef __3DS__
static LightLock logLock;
#define LOCK_INIT()   LightLock_Init(&logLock)
#define LOCK()        LightLock_Lock(&logLock)
#define UNLOCK()      LightLock_Unlock(&logLock)
#else
static pthread_mutex_t logLock = PTHREAD_MUTEX_INITIALIZER;
#define LOCK_INIT()   pthread_mutex_init(&logLock, NULL)
#define LOCK()        pthread_mutex_lock(&logLock)
#define UNLOCK()      pthread_mutex_unlock(&logLock)
#endif

#ifdef __3DS__
#define LOG_DIR_PATH "sdmc:/3ds/netpass/"
#else
#define LOG_DIR_PATH "./log/"
#endif

static LogLevel currentLevel = LOG_LEVEL_INFO;
static LogOutput currentOutput = LOG_OUTPUT_SCREEN;
static FILE* logFile = NULL;

static void ensureLogDirExists(void) {
    // Try to create the directory, ignore error if it exists
    mkdir(LOG_DIR_PATH, 0777);
}

static void buildLogFilePath(const char* filename, char* outPath, size_t maxLen) {
    snprintf(outPath, maxLen, "%s%s", LOG_DIR_PATH, filename);
}

void loggerInit(LogLevel level, LogOutput output, const char* filename) {
    if (level < LOG_LEVEL_NONE || level >= LOG_LEVEL_MAX) {
        level = LOG_LEVEL_INFO;
    }
    if (output < LOG_OUTPUT_NONE || output >= LOG_OUTPUT_MAX) {
        output = LOG_OUTPUT_SCREEN;
    }
    currentLevel = level;
    currentOutput = output;
    LOCK_INIT();

    if ((output == LOG_OUTPUT_FILE || output == LOG_OUTPUT_BOTH) && filename != NULL) {
        ensureLogDirExists();

        char fullPath[256];
        buildLogFilePath(filename, fullPath, sizeof(fullPath));

        logFile = fopen(fullPath, "w");
        if (!logFile) {
            // Failed to open log file - fallback to screen logging
            currentOutput = LOG_OUTPUT_SCREEN;
            printf("Failed to open log file at %s\n", fullPath);
        }
    }
}

void loggerClose(void) {
    LOCK();

    if (logFile) {
        fclose(logFile);
        logFile = NULL;
    }

    UNLOCK();
}

void loggerSetLevel(LogLevel level) {
    LOCK();
    if (level < LOG_LEVEL_NONE || level >= LOG_LEVEL_MAX) {
        printf("[WARN] Incorrect logger level. Falling back to INFO level.\n");
        level = LOG_LEVEL_INFO;
    }
    currentLevel = level;
    UNLOCK();
}

void loggerSetOutput(LogOutput output, const char* filename) {
    LOCK();
    if (output < LOG_OUTPUT_NONE || output >= LOG_OUTPUT_MAX) {
        printf("[WARN] Incorrect logger output. Falling back to screen output.\n");
        output = LOG_OUTPUT_SCREEN;
    }
    if ((output == LOG_OUTPUT_FILE || output == LOG_OUTPUT_BOTH) && filename == NULL && logFile == NULL) {
        printf("[WARN] Log filename is not specified and log file was not initialized. Falling back to screen output.\n");
        output = LOG_OUTPUT_SCREEN;
    }

    // Close existing log file if needed
    if (logFile) {
        fclose(logFile);
        logFile = NULL;
    }

    currentOutput = output;

    // Open new log file if required
    if ((output == LOG_OUTPUT_FILE || output == LOG_OUTPUT_BOTH) && filename != NULL) {
        ensureLogDirExists();

        char fullPath[256];
        buildLogFilePath(filename, fullPath, sizeof(fullPath));

        logFile = fopen(fullPath, "w");
        if (!logFile) {
            currentOutput = LOG_OUTPUT_SCREEN;
            printf("Failed to open log file at %s\n", fullPath);
        }
    }

    UNLOCK();
}

static void loggerWrite(LogLevel level, const char* tag, const char* fmt, va_list args) {
    if (level < currentLevel || currentOutput == LOG_OUTPUT_NONE)
        return;

    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    LOCK();

    if (currentOutput == LOG_OUTPUT_SCREEN || currentOutput == LOG_OUTPUT_BOTH) {
        printf("%s", buffer);
    }

    if ((currentOutput == LOG_OUTPUT_FILE || currentOutput == LOG_OUTPUT_BOTH) && logFile) {
        fprintf(logFile, "[%s] %s\n", tag, buffer);
        fflush(logFile);
    }

    UNLOCK();
}

// --- Log Level Macros ---
#define DEFINE_LOG_FUNCTION(fnName, enumName, tag)       \
void log##fnName(const char* fmt, ...) {             \
    va_list args;                                    \
    va_start(args, fmt);                             \
    loggerWrite(enumName, tag, fmt, args);           \
    va_end(args);                                    \
    }

DEFINE_LOG_FUNCTION(Debug, LOG_LEVEL_DEBUG, "DEBUG")
DEFINE_LOG_FUNCTION(Info,  LOG_LEVEL_INFO,  "INFO")
DEFINE_LOG_FUNCTION(Warn,  LOG_LEVEL_WARN,  "WARN")
DEFINE_LOG_FUNCTION(Error, LOG_LEVEL_ERROR, "ERROR")