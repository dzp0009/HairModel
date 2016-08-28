// Simple image viewer widget based on Qt

#pragma once

#include <QWidget>
#include <QImage>

class ImageWidget : public QWidget
{
	Q_OBJECT

public:
	ImageWidget(QWidget* parent = NULL);
	~ImageWidget();

	// Methods to create empty image and filling data
	bool	createImage(int width, int height);

	void	clear() { m_image.fill(0); }
	
	QImage*	image() { return &m_image; }

	bool	saveImage(QString filename);

signals:
	void	doubleClicked(int x, int y);

protected:
	void	paintEvent(QPaintEvent* event);
	void	mouseDoubleClickEvent(QMouseEvent* event);
	void	keyPressEvent(QKeyEvent* event);

private:
	QImage	m_image;
	double	m_scale;
};

