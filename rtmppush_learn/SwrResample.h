#pragma once

extern "C" {
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

class SwrResample
{
public:
	SwrResample(){}
	~SwrResample() {}

	int Init(int64_t src_ch_layout, int64_t dst_ch_layout ,
		int src_sample_rate,int dst_sample_rate,
		enum AVSampleFormat src_sample_format,enum AVSampleFormat dst_sample_format,
		int src_nb_sample);

	int WriteInput(const char* data, uint64_t len);

	int SwrConvert(char** data);
	
	void Close();
private:
	struct SwrContext* swr_ctx = nullptr;

	uint8_t** src_data_ = nullptr;
	uint8_t** dst_data_ = nullptr;

	int src_nb_channel, dst_nb_channel;
	int src_linesize, dst_linesize;

	int src_rate_;
	int dst_rate_;

	enum AVSampleFormat src_sample_fmt_;
	enum AVSampleFormat dst_sample_fmt_;

	int src_nb_sample_,dst_nb_sample_;
	int max_dst_nb_sample_;
};

