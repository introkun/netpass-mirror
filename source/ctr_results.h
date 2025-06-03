/**
 * NetPass
 * Copyright (C) 2025 Sorunome
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

#include <stdint.h>
#include <stddef.h>

const char* get_level_string(int32_t res);
const char* get_level_description(int32_t res);
void get_level_formatted(char* dest, size_t size, int32_t res);
const char* get_summary_string(int32_t res);
const char* get_summary_description(int32_t res);
void get_summary_formatted(char* dest, size_t size, int32_t res);
const char* get_module_string(int32_t res);
const char* get_module_description(int32_t res);
void get_module_formatted(char* dest, size_t size, int32_t res);
const char* get_description_string(int32_t res);
const char* get_description_description(int32_t res);
void get_description_formatted(char* dest, size_t size, int32_t res);
