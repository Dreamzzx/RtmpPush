#include "RtmpPush.h"
#include <qdebug.h>

void RtmpPush::OpenFormat(const std::string url)
{
	av_log_set_level(AV_LOG_VERBOSE);
	av_register_all();
	avformat_network_init();

	url_ = url;
	
	avformat_alloc_output_context2(&ic, NULL, "flv", url.c_str());
	if (!ic)
	{
		std::cout << "ic alloc failed!" << std::endl;
		return;
	}
	
	ic->duration = 0;
}

void RtmpPush::InitVideoCodecPar(AVMediaType  codec_type, AVCodecID codec_id,
								int width,int height,int format,int fps,
								const uint8_t* extradata,int extradata_size)
{
	video_stream = avformat_new_stream(ic, NULL);
	if (!video_stream)
	{
		std::cout << "new video_stream failed!";
		return;
	}

	video_stream->codecpar->codec_type = codec_type;
	video_stream->codecpar->codec_id = codec_id;
	video_stream->codecpar->bit_rate = 2000000;
	video_stream->codecpar->width = width;
	video_stream->codecpar->height = height;
	video_stream->codecpar->format = format;
	
	//添加SPS PPS 信息
	if (extradata_size > 0)
	{
		video_stream->codecpar->extradata = (uint8_t*)av_mallocz(extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
		memcpy(video_stream->codecpar->extradata, extradata, extradata_size);
		video_stream->codecpar->extradata_size = extradata_size;
	}
}

void RtmpPush::InitAudioCodecPar(AVMediaType codec_type, AVCodecID codec_id,int channels,
								int format,int sample_rate,const uint8_t* extradata,int extradata_size)
{
	audio_stream = avformat_new_stream(ic, NULL);
	if (!audio_stream)
	{
		std::cout << "new audio_stream failed!";
		return;
	}

	audio_stream->codecpar->codec_type = codec_type;
	audio_stream->codecpar->codec_id = codec_id;
	audio_stream->codecpar->channels = channels;
	audio_stream->codecpar->format = format;
	audio_stream->codecpar->sample_rate = sample_rate;
	audio_stream->codecpar->profile = FF_PROFILE_AAC_HE;
	audio_stream->time_base = { 1,sample_rate };

	//添加SPS PPS 信息
	if (extradata_size > 0)
	{
		audio_stream->codecpar->extradata = (uint8_t*)av_mallocz(extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
		memcpy(audio_stream->codecpar->extradata, extradata, extradata_size);
		audio_stream->codecpar->extradata_size = extradata_size;
	}
}

void RtmpPush::WriteHeader()
{
	if (avio_open(&ic->pb, url_.c_str(), AVIO_FLAG_WRITE) < 0)
	{
		std::cout << "open url_ is failed";
		return;
	}

	av_dump_format(ic, 0, url_.c_str(), 1);

	/*for (unsigned int i = 0; i < ic->nb_streams; i++) {
		AVStream* stream = ic->streams[i];
		AVCodecParameters* codecpar = stream->codecpar;

		if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			av_log(NULL, AV_LOG_INFO, "Audio Stream %d: Codec: %d, Sample Rate: %d Hz, Channels: %d, Bitrate: %ld, Format: %d\n",
				i, codecpar->codec_id, codecpar->sample_rate, codecpar->channels, codecpar->bit_rate, codecpar->format);
		}
	}*/

	if (avformat_write_header(ic, NULL) < 0)
	{
		std::cout << "write header failed";
		return;
	}

	auto now = std::chrono::system_clock::now();
	start_time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

} 


void RtmpPush::PushPacket(RtmpPush::MediaType type, uint8_t* data, int size)
{
	std::lock_guard<std::mutex> lock(ffmpeg_mutex);
	
	bool is_video = (type == MediaType::VIDEO);

	AVStream* stream = is_video ? video_stream : audio_stream;

	AVPacket *pkt = av_packet_alloc();

	auto now = std::chrono::system_clock::now();
	auto now_time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
	pkt->dts = pkt->pts = (double)(now_time - start_time) / (av_q2d(is_video ? video_stream->time_base : audio_stream->time_base) * 1000);
	
	pkt->data = data;
	pkt->size = size;
	pkt->stream_index = stream->index;

	if (av_interleaved_write_frame(ic, pkt) < 0)
	{
		qDebug() << "Error muxing" << (is_video ? " video" : "audio ") << "packet\n";
	}

	//释放pkt
	av_packet_free(&pkt);
	pkt = nullptr;
}																												

void RtmpPush::PushClose()
{
	//输入流结尾数据
	av_write_trailer(ic);

	//释放流
	if (audio_stream)
	{
		avcodec_parameters_free(&audio_stream->codecpar);
		audio_stream = nullptr;
	}

	if (video_stream)
	{
		avcodec_parameters_free(&video_stream->codecpar);
		video_stream = nullptr;
	}

	////关闭输入流
	//if (ic && (ic->oformat->flags & AVFMT_NOFILE))
	//{
	//	avio_closep(&ic->pb);
	//}

	avio_closep(&ic->pb);


	if (ic)
	{
		avformat_free_context(ic);
		ic = nullptr;
	}
}
