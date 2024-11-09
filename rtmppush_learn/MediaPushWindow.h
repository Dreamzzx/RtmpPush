#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_MediaPushWindow.h"
#include <qscopedpointer.h>
#include <QCamera>
#include <qlist.h>
#include <qcamerainfo.h>
#include <qcameraviewfinder.h>
#include <QAction>
#include <QAudioDeviceInfo>
#include <QAudioInput>
#include "QCameraSurface.h"
#include "RtmpPush.h"
#include "AudioEncode.h"
#include "AacEncoder.h"
#include "AudioCaputure.h"

#define USE_FFMPEG_VIDEO_ENCODE 1

//#define WRITE_CAPTURE_YUV     //写YUV文件

//#define WRITE_CAPTURE_RGB     //写RGB文件

#ifdef USE_FFMPEG_VIDEO_ENCODE
#include "VideoEncodeFF.h"
#endif // USE_FFMPEG_VIDEO_ENCODE

#define RGB2Y(r,g,b) \
	((unsigned char)((66 * r + 129 * g + 25 * b + 128) >> 8)+16)

#define RGB2U(r,g,b) \
	((unsigned char)((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128)

#define RGB2V(r,g,b) \
	((unsigned char)((112 * r - 94 * g - 18 * b + 128) >> 8) + 128)


enum VideoSolutions
{

};

class MediaPushWindow : public QMainWindow
{
    Q_OBJECT
public:
    MediaPushWindow(QWidget *parent = nullptr);
    ~MediaPushWindow();
public slots:

    void start();

    void updateCameraDeviceMenu(QAction* action);
    void updateCameraDeviceCombox(int index);

    void updateMicDeviceMenu(QAction* action);
    void updateMicDeviceCombox(int index);

    void recvVFrame(QVideoFrame& frame);
    void recvAFrame(const char* data, qint64 len);
    
    void stop();
private:
    void setCamera(const QCameraInfo &cameraInfo);

    void setMic(const QAudioDeviceInfo& audioInfo);
private:
    Ui::RtmpPushClass ui;

    inline unsigned char clip_value(unsigned char value, unsigned char min_value, unsigned char max_value)
    {
        if (value > max_value)
            return max_value;
        else if (value < min_value)
            return min_value;
        else
            return value;
    }

    QScopedPointer<QCamera> m_camera;
    QScopedPointer<AudioCapture> m_mic;
    QScopedPointer<QCameraSurface> m_cameraSurface;
    QScopedPointer<VideoEncodeFF>  m_videoEncode;
    QScopedPointer<AacEnconder> m_audioEncode;
    QScopedPointer<RtmpPush> rtmpPush;


    QList <QCameraInfo> cameraInfos;
    QList<QAudioDeviceInfo> audioInfos;

    int width_ = 0;
    int height_ = 0;

    bool showUserViewFinder = false;

    unsigned char* dst_yuv_420 = nullptr;

#ifdef WRITE_CAPTURE_RGB
    FILE* rgb_out_file = nullptr;   
#endif // WRITE_CAPTURE_RGB


#ifdef WRITE_CAPTURE_YUV
    FILE* yuv_out_file = nullptr;
#endif // WRITE_CAPTURE_YUV


    unsigned char* video_encoder_data = nullptr;
    unsigned char* audio_encoder_data = nullptr;

    bool start_flag = false;
};
