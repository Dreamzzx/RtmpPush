#pragma once
#include <string.h>
#include <mutex>
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>
#include <libavutil/mem.h>  // 包含 av_mallocz 的头文件
}

class RtmpPush
{
public:
	enum MediaType
	{
		VIDEO = 0,
		AUDIO = 1
	};
public:
	RtmpPush() {};
	~RtmpPush(){};
	
	void OpenFormat(const std::string url);

	void InitVideoCodecPar(AVMediaType  codec_type, AVCodecID codec_id,
							int width, int height,int fps,int format, const uint8_t* extradata, int extradata_size);
	void InitAudioCodecPar(AVMediaType codec_type, AVCodecID codec_id, int channels,
		int format, int sample_rate, const uint8_t* extradata, int extradata_size);

	void WriteHeader();

	void PushPacket(RtmpPush::MediaType type, uint8_t* data,int size);

	void PushClose();

private:
	AVFormatContext* ic = nullptr;
	AVStream* audio_stream = nullptr;
	AVStream* video_stream = nullptr;
	
	int64_t audio_pts;
	int64_t video_pts;

	std::string url_;
	uint16_t start_time;

	std::mutex ffmpeg_mutex;
};

