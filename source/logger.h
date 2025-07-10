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
#pragma once

typedef enum {
    LOG_LEVEL_NONE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_MAX
} LogLevel;

typedef enum {
    LOG_OUTPUT_NONE,
    LOG_OUTPUT_SCREEN,
    LOG_OUTPUT_FILE,
    LOG_OUTPUT_BOTH,
    LOG_OUTPUT_MAX
} LogOutput;

void loggerInit(LogLevel level, LogOutput output, const char* filename);
void loggerSetLevel(LogLevel level);
void loggerSetOutput(LogOutput output, const char* filename);
void loggerClose(void);

void logDebug(const char* fmt, ...);
void logInfo(const char* fmt, ...);
void logWarn(const char* fmt, ...);
void logError(const char* fmt, ...);
