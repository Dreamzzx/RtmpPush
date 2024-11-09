#pragma once

#define H264_PROFILE_BASELINE		66
#define H264_PROFILE_MAIN			77
#define H264_PROFILE_HIGH			100

class VideoEncode
{
public:
	struct VCodecConfig
	{
		int code_type;
		int code_id;
		int width;
		int height;                                                                                                                                                                                                                                                                                                 
		int fps;
		int format;
	};
public:
	VideoEncode(){};
	virtual ~VideoEncode() {};

	virtual void InitEncode(int width,int height,int fps,int bitrate,const char* profile) = 0;
	virtual unsigned int Encode(unsigned char* src_buf,unsigned char* dst_buf) = 0;
	virtual void StopEncode() = 0;
	inline VCodecConfig &GetCodecConfig() { return codec_config; }


protected:
	VCodecConfig codec_config;
};