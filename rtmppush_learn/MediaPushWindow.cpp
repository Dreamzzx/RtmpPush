#define _CRT_SECURE_NO_WARNINGS
#include "MediaPushWindow.h"
#include "AudioCaputure.h"

Q_DECLARE_METATYPE(QCameraInfo)

MediaPushWindow::MediaPushWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    resize(600, 480);

    //获取摄像头
    QActionGroup* videoDevicesGroup = new QActionGroup(this);
    videoDevicesGroup->setExclusive(true);//添加互斥性
    cameraInfos = QCameraInfo::availableCameras();
    for(const QCameraInfo cameraInfo:cameraInfos)
    {
        QAction* videoDeviceAction = new QAction(cameraInfo.description(), videoDevicesGroup);
        videoDeviceAction->setCheckable(true);
        videoDeviceAction->setData(QVariant::fromValue(cameraInfo));
        if (cameraInfo == QCameraInfo::defaultCamera()) {
            videoDeviceAction->setChecked(true);
        }
        ui.menuVDevices->addAction(videoDeviceAction);
        ui.vDevices_box->addItem(cameraInfo.description(), QVariant::fromValue(cameraInfo));
    }
    //监听选择摄像头
    connect(videoDevicesGroup, &QActionGroup::triggered, this, &MediaPushWindow::updateCameraDeviceMenu);
    connect(ui.vDevices_box,QOverload<int>::of(&QComboBox::activated), this, &MediaPushWindow::updateCameraDeviceCombox);
    //获取麦克风
    QActionGroup* audioDevicesGroup = new QActionGroup(this);
    QAudioDeviceInfo default_device_info;
    audioDevicesGroup->setExclusive(true);
    audioInfos = QAudioDeviceInfo::availableDevices(QAudio::Mode::AudioInput);
    for (const QAudioDeviceInfo &audioInfo : audioInfos)
    {
        QAction* audioDeviceAction = new QAction(audioInfo.deviceName(), audioDevicesGroup);
        audioDeviceAction->setCheckable(true);
        audioDeviceAction->setData(QVariant::fromValue(audioInfo));
        if (audioInfo.deviceName() == QAudioDeviceInfo::defaultInputDevice().deviceName()); //无作用的if条件
        {
            audioDeviceAction->setChecked(true);
            default_device_info = audioInfo;
        }
       
        ui.menuADevices->addAction(audioDeviceAction);
        ui.aDevices_box->addItem(audioInfo.deviceName(), QVariant::fromValue(audioInfo));
    }
    
    //选择麦克风
    connect(audioDevicesGroup, &QActionGroup::triggered, this, &MediaPushWindow::updateMicDeviceMenu);
    connect(ui.aDevices_box, QOverload<int>::of(&QComboBox::activated), this, &MediaPushWindow::updateMicDeviceCombox);

    setCamera(cameraInfos.at(0));
    
    setMic(default_device_info);

    connect(ui.actionstart,&QAction::triggered,this,&MediaPushWindow::start);
    connect(ui.actionend, &QAction::triggered, this, &MediaPushWindow::stop);
}

void MediaPushWindow::start()
{
    ui.aDevices_box->setEnabled(false);
    ui.vDevices_box->setEnabled(false);
    ui.actionsetting->setEnabled(false);
    ui.actionstart->setEnabled(false);
    ui.actionend->setEnabled(true);
    ui.rtmp_edit->setEnabled(false);

    start_flag = true;

    video_encoder_data = new unsigned char[1024*1024];
    audio_encoder_data = new unsigned char[1024*2*2];
    m_mic->OpenWrite();
    m_audioEncode.reset(new AacEnconder());
    m_audioEncode->InitEncode(
        m_mic->format().chanel_layout,
        m_mic->format().sample_rate,
        m_mic->format().sample_fmt, 
        96000);
    
    m_mic->InitDecDataSize(m_audioEncode->frame_byte_size);
    


#ifdef WRITE_CAPUTURE_RGB
    if(!rgb_out_file)
    rgb_out_file = fopen("rgb_out.txt","w");
#endif // WRITE_CAPUTURE_RGB

#ifdef WRITE_CAPTURE_YUV
    if (!yuv_out_file)
        yuv_out_file = fopen("yuv_out.yuv", "wb");
#endif // WRITE_CAPTURE_YUV

    if (!m_videoEncode) {
        m_videoEncode.reset(new VideoEncodeFF());
        m_videoEncode->InitEncode(width_,height_,25,2000000,"main");
    }

    //打开推流地址
    rtmpPush.reset(new RtmpPush());
    rtmpPush->OpenFormat(ui.rtmp_edit->text().toStdString());

    //初始化VideoCodecPar
    VideoEncode::VCodecConfig &video_info = m_videoEncode->GetCodecConfig();
    rtmpPush->InitVideoCodecPar(static_cast<AVMediaType>(video_info.code_type), static_cast<AVCodecID>(video_info.code_id),
         video_info.width, video_info.height, video_info.fps,
        video_info.format, m_videoEncode->GetExtradata(), m_videoEncode->GetExtradatasize());
    //初始化AudioCodedPar
    AudioEncode::ACodecConfig& audio_info = m_audioEncode->GetCodecConfig();
    rtmpPush->InitAudioCodecPar(static_cast<AVMediaType>(audio_info.codec_type), static_cast<AVCodecID>(audio_info.codec_id),
        audio_info.channel, audio_info.format, audio_info.sample_rate,
        m_audioEncode->GetExtradata(), m_audioEncode->GetExtradatasize());
     
    rtmpPush->WriteHeader();
}

//menu
void MediaPushWindow::updateCameraDeviceMenu(QAction* action)
{
    setCamera(qvariant_cast<QCameraInfo>(action->data()));
}

//combox
void MediaPushWindow::updateCameraDeviceCombox(int index)
{
    setCamera(qvariant_cast<QCameraInfo>(ui.vDevices_box->itemData(index)));
}

void MediaPushWindow::updateMicDeviceMenu(QAction* action)
{
    setMic(qvariant_cast<QAudioDeviceInfo>(action->data()));
}

void MediaPushWindow::updateMicDeviceCombox(int index)
{
    setMic(qvariant_cast<QAudioDeviceInfo>(ui.aDevices_box->itemData(index)));
}

void MediaPushWindow::recvVFrame(QVideoFrame& frame)
{
    frame.map(QAbstractVideoBuffer::ReadOnly);


	QVideoFrame::PixelFormat pixelFormat = frame.pixelFormat();

	width_ = frame.width();
	height_ = frame.height();
	
	
	if (m_videoEncode)
	{
		if (pixelFormat == QVideoFrame::Format_RGB32)
		{
		
			int width = frame.width();
			int height = frame.height();

			if (dst_yuv_420 == nullptr)
			{
				dst_yuv_420 = new uchar[width * height * 3 / 2];
				memset(dst_yuv_420, 128, width * height * 3 / 2);
			}


		
			int planeCount = frame.planeCount();
			int mapBytes = frame.mappedBytes();
			int rgb32BytesPerLine = frame.bytesPerLine();
			const uchar* data = frame.bits();
			if (data == NULL)
			{
				return;
			}


			int idx = 0;
			int idxu = 0;
			int idxv = 0;
			for (int i = height-1;i >=0 ;i--)
			{
				for (int j = 0;j < width;j++)
				{
					uchar b = data[(width*i+j) * 4];
					uchar g = data[(width * i + j) * 4 + 1];
					uchar r = data[(width * i + j) * 4 + 2];
					uchar a = data[(width * i + j) * 4 + 3];
				
		

					uchar y = RGB2Y(r, g, b);
					uchar u = RGB2U(r, g, b);
					uchar v = RGB2V(r, g, b);

					dst_yuv_420[idx++] = clip_value(y,0,255);
					if (j % 2 == 0 && i % 2 == 0) {
				
						dst_yuv_420[width*height + idxu++] = clip_value(u,0,255);
						dst_yuv_420[width*height*5/4 + idxv++] = clip_value(v,0,255);
					}
				}
			}

		
			int len = m_videoEncode->Encode(dst_yuv_420, video_encoder_data);

			if (rtmpPush&&len>0)
			{
				rtmpPush->PushPacket(RtmpPush::MediaType::VIDEO,video_encoder_data, len);
			}
		}
		
#ifdef WRITE_CAPTURE_YUV
		if (yuv_out_file) {
			fwrite(dst_yuv_420, 1, width * height * 3 / 2, yuv_out_file);
		}
#endif // WRITE_CAPTURE_YUV

	}
	
	if (!showUserViewFinder)
	{
		QImage::Format qImageFormat = QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat());
		QImage videoImg(frame.bits(),
			frame.width(),
			frame.height(),
			qImageFormat);

		videoImg = videoImg.convertToFormat(qImageFormat).mirrored(false, true);

		ui.vframe_lable->setPixmap(QPixmap::fromImage(videoImg));
	}


}

void MediaPushWindow::recvAFrame(const char* data, qint64 len)
{
    if (m_audioEncode)
    {
        //rlen为编码后的大小
      int rlen =  m_audioEncode->Encode(data, len, audio_encoder_data);
        if (rtmpPush && rlen > 0)
        {
            //将编码后的数据 和 编码后的长度传入
            rtmpPush->PushPacket(RtmpPush::MediaType::AUDIO, audio_encoder_data, rlen);
        }
    }


}

void MediaPushWindow::stop()
{
    ui.aDevices_box->setEnabled(true);
    ui.vDevices_box->setEnabled(true);
    ui.actionsetting->setEnabled(true);
    ui.actionstart->setEnabled(true);
    ui.actionend->setEnabled(false);
    ui.rtmp_edit->setEnabled(true);

    //关闭麦克风
    m_mic->CloseWrite();

    //关闭AAC和H264编码器
    if (m_audioEncode)
    {
        m_audioEncode->StopEncode();
        m_audioEncode.reset(nullptr);
    }

    if (m_videoEncode)
    {
        m_videoEncode->StopEncode();
        m_videoEncode.reset(nullptr);
    }

    //释放接受的编码数据
    if (video_encoder_data)
    {
        delete[] video_encoder_data;
        video_encoder_data = nullptr;
    }

    if (audio_encoder_data)
    {
        delete[] audio_encoder_data;
        audio_encoder_data = nullptr;
    }

    start_flag = false;

    //关闭推流
    rtmpPush->PushClose();
}

void MediaPushWindow::setCamera(const QCameraInfo &cameraInfo)
{
    m_camera.reset(new QCamera(cameraInfo));

    if (0) {
   // m_camera->setViewfinder(ui.viewfinder);
    }
    else
    {
        //创建YUV采集器
        if (!m_cameraSurface)
        {
            m_cameraSurface.reset(new QCameraSurface(this));
        }
            m_camera->setViewfinder(m_cameraSurface.data());
            connect(m_cameraSurface.data(), &QCameraSurface::frameAvailable, this, &MediaPushWindow::recvVFrame);
    }

    m_camera.data()->start();
}

void MediaPushWindow::setMic(const QAudioDeviceInfo& audioInfo)
{
    m_mic.reset(new AudioCapture());

    //重采样后触发AFrameAvailable信号
    connect(m_mic.data(), &AudioCapture::AFrameAvailable, this, &MediaPushWindow::recvAFrame);

    m_mic->Start(audioInfo);
}

MediaPushWindow::~MediaPushWindow()
{}
