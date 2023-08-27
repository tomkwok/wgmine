#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "../../base64/include/libbase64.h"
#include "../../base64/lib/codecs.h"
#include "config.h"
#include "../../base64/lib/env.h"

// Function declarations:
#define BASE64_CODEC_FUNCS(arch)	\
	BASE64_ENC_FUNCTION(arch);	\
	BASE64_DEC_FUNCTION(arch);	\

BASE64_CODEC_FUNCS(avx)
BASE64_CODEC_FUNCS(neon64)

void
codec_choose (struct codec *codec, int flags)
{
	#ifdef HAVE_AVX
		codec->enc = base64_stream_encode_avx;
		codec->dec = base64_stream_decode_avx;
	#endif

	#ifdef HAVE_NEON64
		codec->enc = base64_stream_encode_neon64;
		codec->dec = base64_stream_decode_neon64;
	#endif
}
