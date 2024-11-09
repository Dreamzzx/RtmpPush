#pragma once

class AudioEncode
{

public:
	struct ACodecConfig
	{
		int codec_type;
		int codec_id;
		int sample_rate;
		int channel;
		int format;
	};

	inline  virtual ACodecConfig& GetCodecConfig() { return CodecConfig; }


protected:
	ACodecConfig CodecConfig;

};