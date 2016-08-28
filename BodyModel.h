#pragma once

#include "QDXImageSprite.h"

class BodyModel : public QDXImageSprite
{
public:
	BodyModel();
	~BodyModel();

	bool	loadColorDepth(QString colorFile, QString depthFile);
	bool	loadColorNoDepth(QString colorFile);
	bool	loadDepth(QString depthFile);

	void	clearDepth();

	void	release();
	void	render();

	bool	updateVertexBuffer();

	QDXImageQuadVertex* vertices() { return m_vertices; }

	int		numVertices() const { return m_vertexCount; }

private:
	ID3D11Buffer*	m_pVB;
	ID3D11Buffer*	m_pIB;

	QDXImageQuadVertex*	m_vertices;

	int				m_vertexCount;
	int				m_indexCount;
};

