#pragma once

#include <QColor>
#include <QImage>
#include <QWidget>
#include <QtGui>


class PolygonWidget : public QWidget
{
	Q_OBJECT

signals:
	void weightsChanged(const std::vector<float>&);

public:
	PolygonWidget(QWidget *parent = 0);
	PolygonWidget(int edgecount, QWidget *parent = 0);
	~PolygonWidget(void);

	void	setVertexCount(const int n);
	int		vertexCount() const {return m_vertexCount;}

	const std::vector<float>& weights() const { return m_weights; }

	void	setWeights(const std::vector<float>& w);

	QSize	minimumSizeHint() const { return QSize(64, 64); }

protected:
	void	mousePressEvent(QMouseEvent *event);
	void	mouseMoveEvent(QMouseEvent *event);
	void	paintEvent(QPaintEvent *event);
	void	resizeEvent(QResizeEvent *event);

private:

	int					m_vertexCount;
	QVector<QPointF>	m_polyPoints;
	QVector<QPointF>	m_textPoints;
	double				m_radius;	

	std::vector<float>	m_weights;
	double				m_currX;
	double				m_currY;


	int		m_borderWidth;
	QColor	m_borderColor;
	QColor	m_bgColor;
	
	int		m_padding;
	

	void	init();

	void	updatePolyPoints();
	void	updateXYfromMVC();
	void	updateMVCfromXY();

	void	validateXY();
	void	normalizeWeights();
};

