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

#include <3ds.h>
#include "quirc/lib/quirc.h"

typedef enum : u32 {
	QR_METHOD_VERIFY = 1,
	QR_METHOD_JOIN,
} QrMethods;

typedef struct {
	u8* start;
	u8* cur;
	u8* end;
	u32 size;
} QrBuffer;

void qr_buffer_from_quirc_data(QrBuffer* buffer, struct quirc_data* data);
u32 qr_read_u32(QrBuffer* buffer);
u32 qr_read_string(QrBuffer* buffer, char* string, u32 length);
bool qr_buf_equal(QrBuffer* buffer, u8* buf, u32 len);
Result qr_verify(QrBuffer* buffer);
