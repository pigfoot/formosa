#ifndef __STR_CODEC_H__
#define __STR_CODEC_H__

int combine_high_low_bits(char c);
void qp_decode_str(char *src);
void qp_encode_str(char *src);
void base64_encode_str(char *src);
void base64_decode_str(char *src);
int decode_line(char *dst, const char *src);

#endif
