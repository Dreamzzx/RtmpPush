#include "VideoEncodeFF.h"
#include <QDebug>

void VideoEncodeFF::InitEncode(int width, int height, int fps, int bitrate, const char* profile)
{
	int ret = 0;
	if (width <= 0 || height <= 0)
		return;

	av_register_all();  //新版不需要在调用此API 新版会在链接时自动完成注册

	const AVCodec* codec = NULL;

	codec = avcodec_find_encoder_by_name("libx264");


	if (codec == NULL)
	{
		printf("find encoder is failed\n");
		return;
	}

	qDebug() << "codec name: " << codec->name;
	qDebug() << "codec long name: " << codec->long_name;

	videoCodecCtx = avcodec_alloc_context3(codec);
	if (!videoCodecCtx)
	{
		printf("alloc videoCodecCtx is failed\n");
		return;
	}

	pkt = av_packet_alloc();
	if (!pkt)
	{
		printf("alloc pkt is failed\n");
		return;
	}

	//设置编码参数
	videoCodecCtx->width = width;
	videoCodecCtx->height = height;
	videoCodecCtx->bit_rate = bitrate;
	videoCodecCtx->time_base = { 1,fps };
	videoCodecCtx->framerate = { fps,1 };
	videoCodecCtx->gop_size = 10;
	videoCodecCtx->max_b_frames = 0; //降低编解码延迟
	videoCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	std::string strProfile = profile;
	if (strProfile == "main")
	{
		videoCodecCtx->profile = FF_PROFILE_H264_MAIN;
	}
	else if (strProfile == "base")
	{
		videoCodecCtx->profile == FF_PROFILE_H264_BASELINE;
	}
	else
	{
		videoCodecCtx->profile == FF_PROFILE_H264_HIGH;
	}

	//h264编码器预设和调整
	ret = av_opt_set(videoCodecCtx->priv_data, "preset", "ultrafast", 0);
	if (ret != 0)
	{
		printf("set preset as ultrafast is failed\n");
		return;
	}
	ret = av_opt_set(videoCodecCtx->priv_data, "tune", "zerolatency", 0);

	ret = avcodec_open2(videoCodecCtx, codec, NULL);
	if (ret != 0)
	{
		printf("open codec is failed\n");
		return;
	}

	frame = av_frame_alloc();
	if (!frame)
	{
		printf("alloc frame is failed\n");
		return;
	}
	frame->format = videoCodecCtx->pix_fmt;
	frame->width = videoCodecCtx->width;
	frame->height = videoCodecCtx->height;

	ret = av_frame_get_buffer(frame, 0);
	if (ret != 0)
	{
		printf("alloc frame buffer is failed\n");
		return;
	}

	//清空frame中的yuv分量
	memset(frame->data[0], 0, videoCodecCtx->height * frame->linesize[0]);//Y
	memset(frame->data[1], 0, videoCodecCtx->height / 2 * frame->linesize[1]);//U
	memset(frame->data[2], 0, videoCodecCtx->height / 2 * frame->linesize[2]);//V

	//sps pps
	if (videoCodecCtx->flags & AV_CODEC_FLAG_GLOBAL_HEADER)
	{
		//
	}
	else
	{
		//送入空的frame以获得产生SPS和PPS的空pkt
		if (avcodec_send_frame(videoCodecCtx, frame) != 0)
		{
			printf("send frame is failed\n");
			av_frame_free(&frame);
			avcodec_free_context(&videoCodecCtx);
			return;
		}

		if (avcodec_receive_packet(videoCodecCtx, pkt) != 0)
		{
			printf("rcv pkt is failed\n");
			av_frame_free(&frame);
			avcodec_free_context(&videoCodecCtx);
			return;
		}
	}

	int frame_type = pkt->data[4] & 0x1f;
	if (frame_type == 7 && receive_first_frame)
	{
		//sps pps
		CopySpsPps(pkt->data,pkt->size);
		receive_first_frame = false;
	}

	//保存编码信息，用于外部获取
	codec_config.code_type = static_cast<int>(AVMEDIA_TYPE_VIDEO);
	codec_config.code_id = static_cast<int>(AV_CODEC_ID_H264);//将枚举转化为整型
	codec_config.format = videoCodecCtx->pix_fmt;
	codec_config.fps = fps;
	codec_config.height = videoCodecCtx->height;
	codec_config.width = videoCodecCtx->width;
}

//返回编码大小
unsigned int VideoEncodeFF::Encode(unsigned char* src_buf, unsigned char* dst_buf)
{
	int ylen = frame->linesize[0] * videoCodecCtx->height;
	int ulen = frame->linesize[1] * videoCodecCtx->height / 2;
	int vlen = frame->linesize[2] * videoCodecCtx->height / 2;
	memcpy(frame->data[0], src_buf, ylen);
	memcpy(frame->data[1], src_buf + ylen, ulen);
	memcpy(frame->data[2], src_buf + ylen + ulen, vlen);

	frame->pts = pts;
	pts++;

	int ret = avcodec_send_frame(videoCodecCtx, frame);
	if (ret < 0)
	{
		printf("send frame is failed!\n");
		return -1;
	}

	ret = avcodec_receive_packet(videoCodecCtx, pkt);
	if (ret != 0)
	{
		printf("rec pkt is failed!\n");
		return -1;
	}

#ifdef WRITE_CAPUTER_H264
	fwrite(pkt->data,1,pkt->size,h264_out_file);
#endif // WRITE_CAPUTER_H264
	if (dst_buf != NULL)
	{
		memcpy(dst_buf, pkt->data, pkt->size);
	}
	ret = pkt->size;
	av_packet_unref(pkt);

	return ret;
}

void VideoEncodeFF::StopEncode()
{
	avcodec_free_context(&videoCodecCtx);
	av_frame_free(&frame);
	av_packet_free(&pkt);

#ifdef WRITE_CAPTURE_264
	if (h264_out_file) {
		fclose(h264_out_file);
		h264_out_file = nullptr;
	}
#endif

	return;
}

void VideoEncodeFF::CopySpsPps(uint8_t* src, int size)
{
	int sps_pps_size = size;
	
	//查找SPS PPS起始码
	for (int i = 0; i < size - 4; i++)
	{
		if (src[i] == 0 && src[i + 1] == 0 && src[i + 2] == 0 && src[i + 3] == 1 && (src[i + 4] & 0x1f) == 5)
		{
			sps_pps_size = i + 1;
			break;
		}

		if (src[i] == 0 && src[i + 1] == 0 && src[i + 2] == 1 && (src[i + 3] & 0x1f) == 5)
		{
			sps_pps_size = i + 1;
			break;
		}
	}

	videoCodecCtx->extradata = (uint8_t*)av_mallocz(sps_pps_size);
	videoCodecCtx->extradata_size = sps_pps_size;
	memcpy(videoCodecCtx->extradata, src, sps_pps_size);
	return;
}
