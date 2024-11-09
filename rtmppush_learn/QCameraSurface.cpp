#include "QCameraSurface.h"

QCameraSurface::QCameraSurface(QObject* parent) : QAbstractVideoSurface(parent)
{
}

QCameraSurface::~QCameraSurface()
{
}

bool QCameraSurface::isFormatSupported(const QVideoSurfaceFormat& format) const
{
	return QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat()) != QImage::Format_Invalid;
}

bool QCameraSurface::present(const QVideoFrame& frame)
{
	if (!frame.isValid())
	{
		stop();
		return false;
	}

	QVideoFrame cloneFrame(frame);
	emit frameAvailable(cloneFrame);

	return true;
}

bool QCameraSurface::start(const QVideoSurfaceFormat& format)
{
	//要通过QCamera::start()启动
	if (QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat()) != QImage::Format_Invalid
		&& !format.frameSize().isEmpty()) {
		QAbstractVideoSurface::start(format);
		return true;
	}
	return false;
}

void QCameraSurface::stop()
{
	QAbstractVideoSurface::stop();
}

QList<QVideoFrame::PixelFormat> QCameraSurface::supportedPixelFormats(QAbstractVideoBuffer::HandleType type) const
{
	if (type == QAbstractVideoBuffer::NoHandle)
	{
		return QList<QVideoFrame::PixelFormat>() << QVideoFrame::Format_RGB32
			<< QVideoFrame::Format_ARGB32
			<< QVideoFrame::Format_ARGB32_Premultiplied
			<< QVideoFrame::Format_RGB555
			<< QVideoFrame::Format_RGB565;

	}
	else
		return QList<QVideoFrame::PixelFormat>();
}
