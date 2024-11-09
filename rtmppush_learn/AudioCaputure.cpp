#include "AudioCaputure.h"
#include "qmessagebox.h"
#include <qdebug.h>

void AudioCapture::Pace2Resample(const char* data, qint64 len)
{
	if (len != nb_sample_size)
	{
		qDebug() << "len is not nb_sample_size";
		return;
	}

	m_Swr->WriteInput(data,len);
	char* result_data = nullptr;
	int rlen = m_Swr->SwrConvert(&result_data);

	if (dst_encode_data)
	{
		Pace2Encode(result_data,rlen);
	}
}

void AudioCapture::Pace2Encode(const char* data, int len)
{
	int pre_remian = nb_encode_remain + len; 
	char* data_curr =const_cast<char*>(data);
	int len_curr = len;
	while (true) {
		if (pre_remian < dst_encode_nb_sample_size)
		{
			memcpy(dst_encode_data + nb_encode_remain, data_curr, len);
			nb_encode_remain = pre_remian;
			break;
		}
		else
		{
			int missing_len = dst_encode_nb_sample_size - nb_encode_remain;
			memcpy(dst_encode_data + nb_encode_remain, data_curr, missing_len);

			emit AFrameAvailable(dst_encode_data, dst_encode_nb_sample_size);

			nb_encode_remain = 0;

			pre_remian = pre_remian - dst_encode_nb_sample_size; 
			data_curr = data_curr + missing_len;
			len_curr = len - missing_len;

		}
	}
}

qint64 AudioCapture::writeData(const char* data, qint64 len)
{
	if (write_flag)
	{
		int pre_remain = nb_swr_remain + len;
		char* data_curr = const_cast<char*>(data);
		int len_curr = len;
		while (true)
		{
			//正常收集数据
			if (pre_remain < nb_sample_size)
			{
				memcpy(src_swr_data+nb_swr_remain, data_curr, len);
				nb_swr_remain = pre_remain;
				break;
			}
			//将缺失的补上（比如采集1024 第一次采集900 剩余的通过以下算法补上）
			else
			{
				int missing_len = nb_sample_size - nb_swr_remain;
				memcpy(src_swr_data + nb_swr_remain,data_curr ,missing_len);
				
				//重采样
				Pace2Resample(src_swr_data,nb_sample_size);

				nb_swr_remain = 0; //采集完一组 将计数重置为0

				pre_remain = pre_remain - nb_sample_size;//索引回到下一组需要采集数据的头部
				data_curr = data_curr + missing_len;//将数据补上后 剩下的data属于下一组
				len_curr = len - missing_len;//本来采集一次len 采集了需要补的 还剩下的
		
			}
		}
	
	}

    return qint64();
}

void AudioCapture::InitDecDataSize(int len)
{
	if (dst_encode_data)
	{
		delete[] dst_encode_data;
		dst_encode_data = nullptr;
		dst_encode_nb_sample_size = 0;
	}
	dst_encode_nb_sample_size = len;
	dst_encode_data = new char[dst_encode_nb_sample_size];
}

void AudioCapture::Start(const QAudioDeviceInfo& audioInfo)
{
    m_Format = audioInfo.preferredFormat();
    AVSampleFormat sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_S16;

	switch (m_Format.sampleSize())
	{
	case 8:
		switch (m_Format.sampleType())
		{
		case QAudioFormat::UnSignedInt:
			m_maxAmplitude = 255;
			sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_U8;
			break;
		case QAudioFormat::SignedInt:
			m_maxAmplitude = 127;
			sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_U8;
			break;
		default:
			break;
		}
		break;
	case 16:
		switch (m_Format.sampleType())
		{
		case QAudioFormat::SignedInt:
			m_maxAmplitude = 65535;
			sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_S16;
			break;
		case QAudioFormat::UnSignedInt:
			m_maxAmplitude = 32767;
			sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_S16;
			break;
		default:
			break;
		}
	case 32:
		switch (m_Format.sampleType())
		{
		case QAudioFormat::UnSignedInt:
			m_maxAmplitude = 0xffffffff;
			break;
		case QAudioFormat::SignedInt:
			m_maxAmplitude = 0x7fffffff;
			sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_S32;
			break;
		case QAudioFormat::Float:
			m_maxAmplitude = 0x7fffffff; 
			sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_FLT;
		default:
			break;
		}
	default:
		break;
	}

	//检测format是否支持
	if (!audioInfo.isFormatSupported(m_Format))
	{
		QMessageBox::warning(nullptr,"waring","format is not support");
		return;
	}

	qDebug() << "default devicdName: " << audioInfo.deviceName() << " : ";
	qDebug() << "sameple: " << m_Format.sampleRate() << " : ";
	qDebug() << "channel:  " << m_Format.channelCount() << " : ";
	qDebug() << "fmt: " << m_Format.sampleSize() << " : ";
	qDebug() << "bytesPerFrame" << m_Format.bytesPerFrame();

	m_Swr = new SwrResample();

	//目标和源采样格式

	//AV_CH_LAYOUT_MONO:单声道布局   AV_CH_LAYOUT_STEREO:立体声布局
	int channel_count = m_Format.channelCount();
	int64_t src_ch_layout = (channel_count == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO);
	int64_t dst_ch_layout = src_ch_layout;

	int src_rate = m_Format.sampleRate();
	int dst_rate = 44100;

	AVSampleFormat src_sample_fmt = sample_fmt;
	AVSampleFormat dst_sample_fmt = AV_SAMPLE_FMT_S16;

	dst_format.chanel_layout = dst_ch_layout;
	dst_format.sample_fmt = dst_sample_fmt;
	dst_format.sample_rate = dst_rate;

	nb_sample_size = m_Format.channelCount() * (m_Format.sampleSize() / 8)* nb_sample;
	src_swr_data = new char[nb_sample_size];

	m_Swr->Init(src_ch_layout,dst_ch_layout,src_rate,dst_rate,src_sample_fmt,dst_sample_fmt, nb_sample);
	
	audioInput = new QAudioInput(audioInfo,m_Format);
	audioInput->start(this);

	this->open(QIODevice::WriteOnly);

	
}

void AudioCapture::Stop()
{
	if (audioInput)
	{
		audioInput->stop();
		delete audioInput;
		audioInput = nullptr;
	}

	if (m_Swr)
	{
		m_Swr->Close();
		delete m_Swr;
		m_Swr = nullptr;
	}

	if (src_swr_data)
	{
		delete[] src_swr_data;
		src_swr_data = nullptr;
	}

	if (dst_encode_data)
	{
		delete[] dst_encode_data;
		dst_encode_data = nullptr;
		dst_encode_nb_sample_size = 0;
	}

	this->close();
}
