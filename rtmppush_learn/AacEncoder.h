#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include "AudioEncode.h"
#define WRITE_CAPTURE_AAC

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
}

class AacEnconder :public AudioEncode
{
public:
	AacEnconder();
	~AacEnconder();

	int InitEncode(int channels_layout, int sample_rate, AVSampleFormat sample_fmt, int64_t bit_rate);
	int Encode(const char* src_buf,int src_len,unsigned char*dst_buf);
	int StopEncode();
	
	const uint8_t* GetExtradata() { return audioCodecCtx->extradata; }
	const int GetExtradatasize() { return audioCodecCtx->extradata_size; }

public:
	int frame_byte_size;

private:
	AVFrame* frame = nullptr;
	AVPacket* pkt = nullptr;
	AVCodecContext* audioCodecCtx = nullptr;

#ifdef WRITE_CAPTURE_AAC
	FILE* aac_out_file = nullptr;

#endif // WRITE_CAPTURE_YUV
};
