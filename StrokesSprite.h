#pragma once

#include "QDXImageSprite.h"


class StrokesSprite : public QDXImageSprite
{
public:
	StrokesSprite(void);
	~StrokesSprite(void);

	virtual bool	create(int width, int height);

	void		drawDot(float x, float y);
	void		drawDot(float x, float y, int strokeId, int strokeRadius);

	int			strokeId() const { return m_strokeId; }
	void		setStrokeId(int id) { m_strokeId = id; }

	int			strokeRadius() const { return m_strokeRadius; }
	void		setStrokeRadius(int radius) { m_strokeRadius = radius; }

	QImage*		strokeIdImage() { return &m_strokeIdImage; }

	void		clearImage();


	// demo
	void		addPoint(float x, float y) { m_points.push_back(XMFLOAT2(x, y)); }
	void		clearPoints() { m_points.clear(); }

	XMFLOAT2&	point(int i) { return m_points[i]; }
	int			numPoints() const { return m_points.size(); }

	XMFLOAT2*	points() { return m_points.data(); }


protected:

	int			m_strokeId;
	int			m_strokeRadius;

	uint		m_strokeColors[4];
	int			m_numColors;

	QImage		m_strokeIdImage;	// stroke IDs


	std::vector<XMFLOAT2>	m_points;
};

