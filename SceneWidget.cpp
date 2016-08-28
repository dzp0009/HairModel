#include "SceneWidget.h"

#include "MyScene.h"

SceneWidget::SceneWidget(QWidget* parent)
	: QDXWidget(parent)
{
	setFocusPolicy(Qt::ClickFocus);
	setMouseTracking(true);
	setBackgroundColor(QColor(64, 64, 64));

	QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	policy.setHorizontalStretch(1);
	policy.setVerticalStretch(1);

	setSizePolicy(policy);
}


SceneWidget::SceneWidget(MyScene* pScene, QWidget* parent /* = NULL */)
	: QDXWidget(parent), m_pScene(pScene)
{
	setFocusPolicy(Qt::ClickFocus);
	setMouseTracking(true);
	setBackgroundColor(QColor(64, 64, 64));
}


void SceneWidget::resizeEvent(QResizeEvent* event)
{
	QDXWidget::resizeEvent(event);

	if (m_pScene)
		m_pScene->setWindow(width(), height());

	QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	policy.setHorizontalStretch(1);
	policy.setVerticalStretch(1);

	setSizePolicy(policy);
}


void SceneWidget::render()
{
	initRenderTarget();

	if (m_pScene)
		m_pScene->render();

	swapChain()->Present(0, 0);
}


bool SceneWidget::event(QEvent* event)
{
	if (m_pScene && m_pScene->handleEvent(event))
		return true;

	return QDXWidget::event(event);
}