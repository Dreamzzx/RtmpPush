#include "AacEncoder.h"
#include <iostream>
#include <qdebug.h>

AacEnconder::AacEnconder()
{
}

AacEnconder::~AacEnconder()
{
}

static int check_fmt(const AVCodec* codec, enum AVSampleFormat sample_fmt)
{
	const enum AVSampleFormat* p = codec->sample_fmts;

	while (*p != AV_SAMPLE_FMT_NONE)
	{
		if (*p == sample_fmt)
			return 1;
		*p++;
	}
	return 0;
}

static int check_sample_rate(const AVCodec* codec, int  sample_rate)
{
	const int* p;

	if (!codec->supported_samplerates)
		return 0;

	p = codec->supported_samplerates;

	while (*p)
	{
		if (*p == sample_rate)
			return 1;
		p++;
	}

	return 0;
}


int AacEnconder::InitEncode(int channels_layout,int sample_rate , AVSampleFormat sample_fmt,int64_t bit_rate)
{
	 
	av_register_all();

	const AVCodec* codec = nullptr;

	codec = avcodec_find_encoder_by_name("libfdk_aac");
	if (!codec)
	{
		std::cout << "find aac codec failed" << std::endl;
		return -1;
	}

	qDebug() << "codec name:" << codec->name;
	qDebug() << "codec long name:" << codec->long_name;

	audioCodecCtx = avcodec_alloc_context3(codec);
	if (!audioCodecCtx)
	{
		std::cout << "alloc audioCodecCtx failed" << std::endl;
		return -1;
	}

	//编码器上下文参数
	audioCodecCtx->channel_layout = channels_layout;
	audioCodecCtx->channels = av_get_channel_layout_nb_channels(audioCodecCtx->channel_layout);
	audioCodecCtx->sample_rate = sample_rate;
	audioCodecCtx->sample_fmt = sample_fmt;
	audioCodecCtx->bit_rate = bit_rate;
	audioCodecCtx->profile = FF_PROFILE_AAC_HE;


	//查看是否支持fmt
	if (!check_fmt(codec, audioCodecCtx->sample_fmt))
	{
		qDebug() << "Encoder not support this fmt";
		return -1;
	}
	
	//查看是否支持sample_rate
	if (!check_sample_rate(codec,audioCodecCtx->sample_rate))
	{
		qDebug() << "Encoder not support this sample_rate";
		return -1;
	}

	audioCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	if (avcodec_open2(audioCodecCtx, codec, NULL) < 0)
	{
		std::cout << "can not open codec" << std::endl;
		return -1;
	}

	frame = av_frame_alloc();
	if (!frame)
	{
		std::cout << "alloc frame is failed" << std::endl;
		return -1;
	}

	pkt = av_packet_alloc();
	if (!pkt)
	{
		std::cout << "alloc pkt is failed" << std::endl;
		return -1;
	}

	//一个音频帧大小
	frame_byte_size = audioCodecCtx->frame_size * audioCodecCtx->channels * 2;

	frame->nb_samples = audioCodecCtx->frame_size;
	frame->format = audioCodecCtx->sample_fmt;
	frame->channel_layout = audioCodecCtx->channel_layout;

	int ret = av_frame_get_buffer(frame, 0);

#ifdef WRITE_CAPTURE_AAC
	if (!aac_out_file) {
		aac_out_file = fopen("ouput.aac", "wb");
		if (aac_out_file == nullptr)
		{

		}
	}
#endif // WRITE_CAPTURE_AAC

	//外部获取编码器信息参数
	CodecConfig.codec_type = static_cast<int> (AVMediaType::AVMEDIA_TYPE_AUDIO);
	CodecConfig.codec_id = static_cast<int>(AV_CODEC_ID_AAC);
	CodecConfig.format = audioCodecCtx->sample_fmt;
	CodecConfig.channel = audioCodecCtx->channels;
	CodecConfig.sample_rate = audioCodecCtx->sample_rate;

	return 0;
}

int AacEnconder::Encode(const char* src_buf, int src_len, unsigned char* dst_buf)
{
	if (src_len != frame_byte_size)
	{
		qDebug() << "src_len is not" << frame_byte_size;
	}

	//检查采样的音频格式是否为平面格式
	//平面格式是将左右通道数据分别存在数组中，而交错格式则将左右通道存储在同一个数组中；
	//平面格式处理数据更加高效，交错格式播放时更加高效
	int planer = av_sample_fmt_is_planar(audioCodecCtx->sample_fmt);
	//这里没有使用平面格式
	if (planer)
	{

	}
	else 
	{
		memcpy(frame->data[0], src_buf, src_len);
	}

	int ret;

	ret = avcodec_send_frame(audioCodecCtx,frame);
	if (ret)
	{
		qDebug() << "Send Frame is failed";
		return -1;
	}

	ret = avcodec_receive_packet(audioCodecCtx, pkt);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
		qDebug() << "Error encoding audio frame " << ret;
		return 0;
	}
	else if (ret < 0) {
		qDebug() << "Error encoding audio frame" << ret;
		exit(1);
	}
	
	int pkt_len = pkt->size;

	if (dst_buf != nullptr)
	{
		memcpy(dst_buf, pkt->data, pkt->size);
	}

#ifdef WRITE_CAPTURE_AAC
	fwrite(pkt->data, 1, pkt->size, aac_out_file);
#endif
	av_packet_unref(pkt);
	return pkt_len;
}

int AacEnconder::StopEncode()
{
	if (frame)
	{
		av_frame_free(&frame);
	}

	if (pkt)
	{
		av_packet_free(&pkt);
	}

	if (audioCodecCtx)
	{
		avcodec_free_context(&audioCodecCtx);
	}

	return 0;
}

