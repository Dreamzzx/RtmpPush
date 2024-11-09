#pragma once

#include <QObject>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>
#include <QVideoFrame>

class QCameraSurface  : public QAbstractVideoSurface
{
	Q_OBJECT

public:
    QCameraSurface(QObject *parent);
	~QCameraSurface();

	virtual bool isFormatSupported(const QVideoSurfaceFormat& format) const override;
	virtual bool present(const QVideoFrame& frame) override;
	virtual bool start(const QVideoSurfaceFormat& format) override;
	virtual void stop() override;
	virtual QList<QVideoFrame::PixelFormat> supportedPixelFormats(
		QAbstractVideoBuffer::HandleType type = QAbstractVideoBuffer::NoHandle) const override;

signals:
	void frameAvailable(QVideoFrame &frame);


};
