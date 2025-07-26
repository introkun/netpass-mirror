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

#include "music.h"
#include "api.h"
#include "config.h"

#include <opus/opusfile.h>
#include <stdlib.h>
#include <string.h>

#define MUSIC_CHANNEL 8
#define OPUS_RATE (48000.f)
#define NUM_BUFFERS 3
#define OPUS_BUFFERSIZE (16 * 1024) // 16K stereo frames = 32K int16_t samples

bool stop_playing = false;
Thread music_thread = 0;
char curfilename[20] = {0};

void wait_for_state(bool playing) {
	int count = 0;
	while (ndspChnIsPlaying(MUSIC_CHANNEL) == !playing && count < 100000) {
		svcSleepThread(10);
		count++;
	}
}

__attribute__((optimize("O3")))
u64 fill_opus_buffer(OggOpusFile* opus_file, int16_t* buffer, int samples_to_read) {
	u64 samples_written = 0;

	while (samples_to_read > 0) {
		int read = op_read(opus_file, buffer, samples_to_read, NULL);
		if (read < 0) {
			return read;
		} else if (read == 0) {
			op_pcm_seek(opus_file, 0);
			continue;
		}

		samples_written += read;
		samples_to_read -= read;
		buffer += read;
	}

	return samples_written; // number of int16_t mono samples
}

__attribute__((optimize("O3")))
void play_thread(void* p) {
	OggOpusFile* opus_file = p;

	// Allocate enough memory for stereo buffers
	s16* buffers_mem = linearAlloc(OPUS_BUFFERSIZE * sizeof(s16) * NUM_BUFFERS);
	if (!buffers_mem) {
		printf("Failed to allocate audio buffers\n");
		op_free(opus_file);
		threadExit(1);
		return;
	}

	s16* buffers[NUM_BUFFERS];
	for (int i = 0; i < NUM_BUFFERS; i++) {
		buffers[i] = buffers_mem + i * OPUS_BUFFERSIZE;
	}

	ndspWaveBuf wavebuf[NUM_BUFFERS];
	memset(wavebuf, 0, sizeof(wavebuf));

	// NDSP setup
	ndspChnReset(MUSIC_CHANNEL);
	ndspChnWaveBufClear(MUSIC_CHANNEL);
	ndspSetOutputMode(NDSP_OUTPUT_MONO);
	ndspChnSetInterp(MUSIC_CHANNEL, NDSP_INTERP_POLYPHASE);
	ndspChnSetRate(MUSIC_CHANNEL, OPUS_RATE);
	ndspChnSetFormat(MUSIC_CHANNEL, NDSP_FORMAT_MONO_PCM16);

	// Initial buffer fill
	for (int i = 0; i < NUM_BUFFERS; i++) {
		int read = fill_opus_buffer(opus_file, buffers[i], OPUS_BUFFERSIZE);
		if (read <= 0) {
			printf("Music Fail: %d\n", read);
			goto fail;
		}

		DSP_FlushDataCache(buffers[i], read * sizeof(s16));
		wavebuf[i].nsamples = read;
		wavebuf[i].data_vaddr = buffers[i];
		ndspChnWaveBufAdd(MUSIC_CHANNEL, &wavebuf[i]);
	}

	// now start the loop
	stop_playing = false;
	wait_for_state(true);

	// Playback loop
	while (!stop_playing) {
		svcSleepThread((u64)1000 * 10);
		// do nothing if the channel is paused
		if (ndspChnIsPaused(MUSIC_CHANNEL)) {
			continue;
		}

		for (int i = 0; i < NUM_BUFFERS; i++) {
			if (wavebuf[i].status == NDSP_WBUF_DONE) {
				int read = fill_opus_buffer(opus_file, buffers[i], OPUS_BUFFERSIZE);
				if (read < 0) {
					printf("Music Fail: %d\n", read);
					goto fail;
				}
				if (read == 0) {
					op_pcm_seek(opus_file, 0);
					read = fill_opus_buffer(opus_file, buffers[i], OPUS_BUFFERSIZE);
				}

				DSP_FlushDataCache(buffers[i], read * sizeof(s16));
				wavebuf[i].nsamples = read;
				wavebuf[i].data_vaddr = buffers[i];
				ndspChnWaveBufAdd(MUSIC_CHANNEL, &wavebuf[i]);
			}
		}
	}

fail:
	linearFree(buffers_mem);
	ndspChnReset(MUSIC_CHANNEL);
	op_free(opus_file);
	threadExit(0);
}


Result playMusic(const char* filename) {
	Result res = 0;
	// if we are already playing this file
	if (strcmp(filename, curfilename) == 0) return res;
	strncpy(curfilename, filename, sizeof(curfilename) - 1);
	curfilename[sizeof(curfilename) - 1] = '\0';
	stopMusic();
	if (!config.bg_music) {
		return res;
	}
	// first open the file
	char f[50];
	snprintf(f, 50, "romfs:/music/%s.opus", filename);
	OggOpusFile* opus_file = op_open_file(f, (int*)&res);
	if (!opus_file) return res;
	music_thread = threadCreate(play_thread, opus_file, 26*1024, main_thread_prio()-10, -2, false);
	return res;
}

void stopMusic(void) {
	stop_playing = true;
	wait_for_state(false);
	if (music_thread) {
		threadJoin(music_thread, U64_MAX);
		threadFree(music_thread);
		music_thread = 0;
	}
}

void toggleBgMusic(void) {
	config.bg_music = !config.bg_music;
	if (!config.bg_music) {
		stopMusic();
	} else {
		char filename[sizeof(curfilename)];
		strncpy(filename, curfilename, sizeof(filename) - 1);
		filename[sizeof(filename) - 1] = '\0';
		memset(curfilename, 0, sizeof(curfilename));
		playMusic(filename);
	}
	configWrite();
}

void musicInit(void) {
	ndspInit();
}

void musicExit(void) {
	stopMusic();
	ndspExit();
}
