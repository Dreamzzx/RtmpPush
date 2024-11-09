#pragma once
#include "VideoEncode.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
}

class VideoEncodeFF :public VideoEncode
{
public:
	VideoEncodeFF(){};
	~VideoEncodeFF()  override {};

	void InitEncode(int width, int height, int fps, int bitrate, const char* profile) override;
	unsigned int Encode(unsigned char* src_buf, unsigned char* dst_buf) override;
	void StopEncode() override;

	const uint8_t* const GetExtradata()
	{
		return videoCodecCtx->extradata;
	}

	const int const GetExtradatasize()
	{
		return videoCodecCtx->extradata_size;
	}

	void CopySpsPps(uint8_t* src, int size);
	
private:
	AVPacket* pkt;
	AVFrame* frame;
	AVCodecContext* videoCodecCtx;
#ifdef WRITE_CAPTURE_H264
	File* h264_out_file = nullptr;
#endif // WRITE_CAPTURE_H264

	uint64_t pts = 0;
	
	bool receive_first_frame = true;
};

