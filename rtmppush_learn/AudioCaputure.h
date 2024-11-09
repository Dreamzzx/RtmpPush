#pragma once
#include <qiodevice.h>
#include "qaudiodeviceinfo.h"
#include "SwrResample.h"
#include <qaudioinput.h>

typedef struct M_Format {
    int sample_rate;
    int chanel_layout;
    AVSampleFormat sample_fmt;
}M_Format;


class AudioCapture :public QIODevice
{
    Q_OBJECT;

public:
	AudioCapture(){}
	~AudioCapture(){
        Stop();
    }

    inline void OpenWrite() { write_flag = true; }
    inline void CloseWrite() { write_flag = false; }

    qint64 readData(char* data, qint64 maxlen) override
    {
        Q_UNUSED(data)
            Q_UNUSED(maxlen)
            return 0; // '��ʵ�ʴ��豸�ж�ȡ���ݣ���Ϊ���Ǵ��������������
    }

    void Pace2Resample(const char* data, qint64 len);
    void Pace2Encode(const char*data,int len);

    qint64 writeData(const char* data, qint64 len) override;

    void InitDecDataSize(int len);

    void Start(const QAudioDeviceInfo& audioInfo);

    void Stop();

    M_Format& format()
    {
        return dst_format;
    }

signals:
	
	void AFrameAvailable(char *	data,qint64 len);

private:
    QAudioFormat m_Format;
    qint32 m_maxAmplitude;

    SwrResample *m_Swr;
    M_Format dst_format;
    
    const int nb_sample = 1024; //ȡ1024��������
    int nb_sample_size = 0; //nb_sample���������Ӧ���ֽ���
    int dst_nb_sample_size = 0;

    //������ز�����
    char* src_swr_data = nullptr;
    int nb_swr_remain = 0;
    
    //�ز�����������������������
    char* dst_encode_data = nullptr;
    int dst_encode_nb_sample_size = 0;
    int nb_encode_remain = 0;

    QAudioInput* audioInput = nullptr;
    
    bool write_flag = false;
};

