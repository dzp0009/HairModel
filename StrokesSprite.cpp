#include "StrokesSprite.h"


StrokesSprite::StrokesSprite(void)
{
	m_strokeId = -1 ;
	m_strokeRadius = 6;

	m_strokeColors[0] = qRgba(255, 255, 255, 255);
	m_strokeColors[1] = qRgba(64, 255, 0, 128);
	m_strokeColors[2] = qRgba(255, 0, 0, 128);
	m_strokeColors[3] = qRgba(0, 0, 255, 255);

	m_numColors = 4;
}


StrokesSprite::~StrokesSprite(void)
{
}


bool StrokesSprite::create(int width, int height)
{
	if (!QDXImageSprite::create(width, height))
		return false;

	m_strokeIdImage = QImage(width, height, QImage::Format_ARGB32);
	ZeroMemory(m_strokeIdImage.bits(), m_strokeIdImage.byteCount());

	return true;
}


void StrokesSprite::drawDot(float x, float y)
{
	drawDot(x, y, m_strokeId, m_strokeRadius);
}


void StrokesSprite::drawDot(float x, float y, int strokeId, int strokeRadius)
{
	if (strokeId < 0 || strokeId >= m_numColors)
		return;

	int centerX = int(x + 0.5f);
	int centerY = int(y + 0.5f);

	m_isDirty = false;

	int r2 = (strokeRadius+1)*(strokeRadius+1);
	for (int sy = centerY - strokeRadius; sy <= centerY + strokeRadius; sy++)
	{
		if (sy < 0 || sy >= m_image.height())
			continue;

		int dy2 = (sy - centerY)*(sy - centerY);

		for (int sx = centerX - strokeRadius; sx <= centerX + strokeRadius; sx++)
		{
			if (sx < 0 || sx >= m_image.width())
				continue;

			int dx2 = (sx - centerX)*(sx - centerX);
			if (dx2 + dy2 > r2)
				continue;

			if (strokeId < m_numColors)
				m_image.setPixel(sx, sy, m_strokeColors[strokeId]);

			m_strokeIdImage.setPixel(sx, sy, qRgba(strokeId, strokeId, strokeId, 255));
		}
	}

	m_isDirty = true;
}


void StrokesSprite::clearImage()
{
	ZeroMemory(m_strokeIdImage.bits(), m_strokeIdImage.byteCount());
	ZeroMemory(m_image.bits(), m_image.byteCount());
	m_isDirty = true;
}