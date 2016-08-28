#include "ImageWidget.h"

#include <QPainter>
#include <QSizePolicy>
#include <QMouseEvent>

ImageWidget::ImageWidget(QWidget* parent)
	: QWidget(parent)
{
	setAttribute(Qt::WA_StaticContents);
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	setFocusPolicy(Qt::ClickFocus);
}


ImageWidget::~ImageWidget(void)
{
}


void ImageWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);

	if (!m_image.isNull())
		painter.drawImage(0, 0, m_image);
}

void ImageWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
	emit doubleClicked(event->x(), event->y());
}


void ImageWidget::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Space)
	{
		this->move(100, 0);
	}
}


bool ImageWidget::createImage(int width, int height)
{
	m_image = QImage(width, height, QImage::Format_ARGB32);
	
	clear();

	setFixedSize(width, height);

	return true;
}


bool ImageWidget::saveImage(QString filename)
{
	return m_image.save(filename);
}