#pragma once

#include "QDXWidget.h"

class MyScene;

class SceneWidget : public QDXWidget
{
public:
	SceneWidget(QWidget* parent = NULL);
	SceneWidget(MyScene* pScene, QWidget* parent = NULL);

	MyScene*	scene() { return m_pScene; }

protected:

	virtual void	render();

	virtual void	resizeEvent(QResizeEvent* event);

	virtual bool	event(QEvent* event);

private:

	MyScene*		m_pScene;
};

