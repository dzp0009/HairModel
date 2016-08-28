#pragma once

// A mesh that can morph between two source meshes

#include "QDXObject.h"
#include "QDXMesh.h"

#include <vector>

#include "MorphController.h"


struct MorphMeshVertex
{
	XMFLOAT3	position;	// pos 0
    XMFLOAT3	normal;
	XMFLOAT2	texcoord;
	XMFLOAT3	pos1;

#if MAX_NUM_MORPH_SRC > 2
	XMFLOAT3	pos2;
#endif
#if MAX_NUM_MORPH_SRC > 3
	XMFLOAT3	pos3;
#endif
#if MAX_NUM_MORPH_SRC > 4
	XMFLOAT3	pos4;
#endif

    static const D3D11_INPUT_ELEMENT_DESC s_inputElementDesc[];
	static const int s_numElements;
};


class HeadMorphMesh : public QDXObject
{
public:
	HeadMorphMesh();
	~HeadMorphMesh();

	virtual bool	create();
	virtual void	release();
	virtual void	render();

	bool	loadAndRectifySource(int srcIdx, QString meshName, const SourceTransform& transform);
	void	clear();

private:

	void	rectifyVertices(int srcIdx, const SourceTransform& transform);

	ID3D11Buffer*					m_pVertexBuffer;
	ID3D11Buffer*					m_pIndexBuffer;

	std::vector<MorphMeshVertex>	m_vertices;
	std::vector<DWORD>				m_indices;

	XMFLOAT3		m_center;
};

