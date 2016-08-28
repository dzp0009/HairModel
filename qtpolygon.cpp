#include "qtpolygon.h"

#define PI 3.1415927


PolygonWidget::PolygonWidget(QWidget *parent) : QWidget(parent), 
	m_vertexCount(3)
{
	init();
}



PolygonWidget::PolygonWidget(int edgecount, QWidget *parent) : 
	QWidget(parent), m_vertexCount(edgecount)
{
	init();
}


void PolygonWidget::init()
{
	m_weights.resize(m_vertexCount);
	normalizeWeights();

	m_padding		= 16;
	m_borderWidth	= 1;
	m_borderColor	= QColor(32,32,32);
	m_bgColor		= QColor(16, 16, 16, 32);

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}


PolygonWidget::~PolygonWidget(void)
{
}


void PolygonWidget::setVertexCount(const int n)
{
	if (n < 0)
		return;

	m_weights.resize(n);
	normalizeWeights();

	m_vertexCount = n;
	updatePolyPoints();
	updateXYfromMVC();
	updateMVCfromXY();

	update();
}


void PolygonWidget::setWeights(const std::vector<float>& w)
{
	m_weights = w;
	normalizeWeights();

	m_vertexCount = m_weights.size();
	updatePolyPoints();
	updateXYfromMVC();

	update();
}


void PolygonWidget::normalizeWeights()
{
	if (m_weights.size() < 1)
		return;

	float sum = 0;
	for (int i = 0; i < m_weights.size(); i++)
		sum += m_weights[i];

	if (sum < 0.001f)
	{
		m_weights[0] = 1.0f;
		for (int i = 1; i < m_weights.size(); i++)
			m_weights[i] = 0;
	}
	else
	{
		for (int i = 0; i < m_weights.size(); i++)
			m_weights[i] /= sum;
	}
}


void PolygonWidget::updatePolyPoints()
{
	m_polyPoints.resize(m_weights.size());
	m_textPoints.resize(m_weights.size());

	if (m_weights.size() == 1)
	{
		m_polyPoints[0] = QPointF(0,0);
		m_textPoints[0] = QPointF(0, -9.0);
		return;
	}

	double textRadius = m_radius + 9.0;

	for(int i = 0; i < m_vertexCount; i++)
	{
		double theta = (double)i*2*PI/(double)m_vertexCount - PI/2.0;
		m_polyPoints[i] = QPointF(m_radius*cos(theta), m_radius*sin(theta));
		m_textPoints[i] = QPointF(textRadius*cos(theta)-4, textRadius*sin(theta)+4);
	}
}


void PolygonWidget::paintEvent(QPaintEvent *event)
{
	QPainter* painter = new QPainter(this);
	int width = this->width();
	int height = this->height();
	painter->setWindow(-width/2.0f, -height/2.0f,width,height);
	painter->setRenderHint(QPainter::Antialiasing,true);
	
	if (m_vertexCount > 1)
	{
		// draw background polygon
		painter->setPen(QPen(m_borderColor, m_borderWidth));
		painter->setBrush(QBrush(m_bgColor));
		painter->drawConvexPolygon(m_polyPoints);

		// draw weight lines
		for (int i = 0; i < m_polyPoints.size(); i++)
		{
			painter->setPen(QColor(255,255,255,255.0*m_weights[i]));
			painter->drawLine(m_currX, m_currY, m_polyPoints[i].x(), m_polyPoints[i].y());
		}

		// draw vertices and their IDs
		painter->setPen(QColor(32,32,32,255));
		painter->setBrush(QBrush(QColor(32,32,32,255)));
		for (int i = 0; i < m_textPoints.size(); i++)
		{
			painter->drawText(m_textPoints[i], QString("%1").arg(i));
			painter->drawEllipse(m_polyPoints[i],2,2);
		}
	}

	if (m_vertexCount > 0)
	{
		// draw text of current weights beside cursor
		const double lineSpace = 12.0;
		painter->setPen(QPen(QColor(192,192,192)));
		double lineX = m_currX + 10, lineY = m_currY - (double)m_vertexCount*0.5*lineSpace + 0.7*lineSpace;
		for (int i = 0; i < m_vertexCount; i++)
		{
			painter->drawText(lineX, lineY, QString("%1").arg(m_weights[i],0,'f',3)); 
			lineY += lineSpace;
		}

		// draw cursor
		painter->setPen(QPen(QColor(51,153,255), 2));
		painter->setBrush(QBrush(QColor(48,48,48)));
		painter->drawEllipse(QPointF(m_currX, m_currY),3,3);
	}
}


void PolygonWidget::resizeEvent(QResizeEvent *event)
{
	int oldwidth = event->oldSize().width();
	int oldheight = event->oldSize().height();

	m_radius = (this->width() > this->height()) ? 
				(this->height() / 2.0f - m_borderWidth - 1.0) : 
				(this->width() / 2.0f - m_borderWidth - 1.0);
	m_radius -= m_padding;

	updatePolyPoints();
	updateXYfromMVC();
}


void PolygonWidget::mousePressEvent(QMouseEvent *event)
{
	mouseMoveEvent(event);
}


void PolygonWidget::mouseMoveEvent(QMouseEvent *event)
{
	m_currX = (double)event->x() - this->width() / 2.0f;
	m_currY = (double)event->y() - this->height() / 2.0f;
	validateXY();
	
	updateMVCfromXY();
	update();
}


void PolygonWidget::validateXY()
{
	if (m_vertexCount < 2)
	{
		m_currX = m_currY = 0;
		return;
	}
	if (m_vertexCount == 2)
	{
		m_currX = m_polyPoints[0].x();
		if(m_currY <= m_polyPoints[0].y())
			m_currY = m_polyPoints[0].y();
		else if(m_currY >= m_polyPoints[1].y())
			m_currY = m_polyPoints[1].y();

		return;
	}

	bool inside = true;
	QPointF *interpoint = new QPointF();
	QLineF interedge;
	for(int i=0;i<m_vertexCount;i++)
	{
		QPointF p1 = m_polyPoints[i];
		QPointF p2;
		if(i == m_vertexCount-1)
			p2 = m_polyPoints[0];
		else
			p2 = m_polyPoints[i+1];
		interedge = QLineF(p1,p2);

		if(interedge.intersect(QLineF(QPointF(0,0),QPointF(m_currX,m_currY)),interpoint) == QLineF::BoundedIntersection)
		{
			inside = false;
			break;
		}
	}

	if(!inside)
	{
		double t;
		QPointF curr(m_currX,m_currY);
		double angle1 = QLineF(interedge.p1(),curr).angleTo(interedge);
		double angle2 = QLineF(interedge.p2(),interedge.p1()).angleTo(QLineF(interedge.p2(),curr));
		if(cos(angle1 * PI / 180.0f) < 0)
		{
			t=0.0f;
		}
		else if(cos(angle2 * PI / 180.0f) < 0)
		{
			t=1.0f;
		}
		else
		{
			double dist = QLineF(interedge.p1(),curr).length();
			double len = interedge.length();
			t = dist * cos(angle1 * PI / 180.0) / len; 
		}
		QPointF q(interedge.pointAt(t));	
		m_currX = q.x();
		m_currY = q.y();
	}
}


void PolygonWidget::updateMVCfromXY()
{
	if (m_vertexCount < 2)
	{
		if (m_vertexCount == 1)
		{
			m_weights[0] = 1.0f;
			emit weightsChanged(m_weights);
		}

		return;
	}

	QVector<double> w;
	double wsum = 0;
	int onedge = -1;

	QPointF v0(m_currX,m_currY);

	for(int i=0;i<m_vertexCount;i++)
	{
		if(v0 == m_polyPoints[i])
		{
			onedge = i;
			break;
		}
		QPointF vi = m_polyPoints[i];
		QPointF vp = (i==0)?(m_polyPoints[m_vertexCount-1]):(m_polyPoints[i-1]);
		QPointF vn = (i==m_vertexCount-1)?(m_polyPoints[0]):(m_polyPoints[i+1]);
		double ai1 = QLineF(v0,vp).angleTo(QLineF(v0,vi));
		double ai  = QLineF(v0,vi).angleTo(QLineF(v0,vn));
		double dist = QLineF(v0,vi).length();

		double wi = (tan(ai1 * PI / 360.0) + tan(ai * PI / 360.0)) / dist;
		w<<wi;
		wsum += wi;
	}

	if(onedge == -1)
	{
		for(int i=0; i<m_vertexCount; i++)
			m_weights[i] = (w[i] / wsum);
	}
	else
	{
		for(int i=0; i<m_vertexCount; i++)
			m_weights[i] = (i == onedge) ? 1.0 : 0.0;
	}

	emit weightsChanged(m_weights);
}


void PolygonWidget::updateXYfromMVC()
{
	double x = 0,y = 0;
	for(int i = 0; i < m_vertexCount; i++)
	{
		x += m_weights[i] * m_polyPoints[i].x();
		y += m_weights[i] * m_polyPoints[i].y();
	}
	m_currX = x;
	m_currY = y;
}