#include "base64.h"
// from https://nachtimwald.com/2017/11/18/base64-encode-and-decode-in-c/

size_t b64_encoded_size(size_t inlen) {
	size_t ret;

	ret = inlen;
	if (inlen % 3 != 0)
		ret += 3 - (inlen % 3);
	ret /= 3;
	ret *= 4;

	return ret;
}

char* b64encode(u8* in, size_t len) {
	const char b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";

	if (in == NULL || len == 0) return NULL;

	size_t elen = b64_encoded_size(len);
	char* out = malloc(elen + 1);
	out[elen] = '\0';

	for (size_t i = 0, j = 0; i < len; i += 3, j += 4) {
		size_t v = in[i];
		v = in[i];
		v = i+1 < len ? v << 8 | in[i+1] : v << 8;
		v = i+2 < len ? v << 8 | in[i+2] : v << 8;

		out[j]   = b64chars[(v >> 18) & 0x3F];
		out[j+1] = b64chars[(v >> 12) & 0x3F];
		if (i+1 < len) {
			out[j+2] = b64chars[(v >> 6) & 0x3F];
		} else {
			out[j+2] = '\0';
		}
		if (i+2 < len) {
			out[j+3] = b64chars[v & 0x3F];
		} else {
			out[j+3] = '\0';
		}
	}
	return out;
}