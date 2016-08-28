#include "HeadMorphMesh.h"
#include "MorphController.h"

#include "CoordUtil.h"

#include <assimp.hpp>
#include <aiScene.h>
#include <aiPostProcess.h>

#include <QFile>

const D3D11_INPUT_ELEMENT_DESC MorphMeshVertex::s_inputElementDesc[] =
{
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
#if MAX_NUM_MORPH_SRC > 2
	{"TEXCOORD", 2, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
#endif
#if MAX_NUM_MORPH_SRC > 3
	{"TEXCOORD", 3, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
#endif
#if MAX_NUM_MORPH_SRC > 4
	{"TEXCOORD", 4, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
#endif
};

const int MorphMeshVertex::s_numElements = sizeof(s_inputElementDesc) / sizeof(s_inputElementDesc[0]);



HeadMorphMesh::HeadMorphMesh() :
	m_pVertexBuffer(NULL), m_pIndexBuffer(NULL)
{
	setName("HeadMorphMesh");
}


HeadMorphMesh::~HeadMorphMesh()
{
	release();
}


bool HeadMorphMesh::create()
{
	release();

	if (m_vertices.empty() || m_indices.empty())
		return true;

	ID3D11Device* pDevice = QDXUT::device();

	// Create vertex buffer
	D3D11_BUFFER_DESC buffDesc;
	ZeroMemory(&buffDesc, sizeof(buffDesc));
	buffDesc.Usage      = D3D11_USAGE_IMMUTABLE;
	buffDesc.ByteWidth  = sizeof(MorphMeshVertex) * m_vertices.size();
	buffDesc.BindFlags  = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem    = m_vertices.data();

	if (S_OK != pDevice->CreateBuffer(&buffDesc, &initData, &m_pVertexBuffer))
	{
		printf("ERROR: failed to create mesh vertex buffer!\n");
		release();
		return false;
	}

	// Create index buffer
	buffDesc.ByteWidth  = sizeof(DWORD) * m_indices.size();
	buffDesc.BindFlags  = D3D11_BIND_INDEX_BUFFER;
	initData.pSysMem    = m_indices.data();
	if (S_OK != pDevice->CreateBuffer(&buffDesc, &initData, &m_pIndexBuffer))
	{
		printf("ERROR: failed to create mesh index buffer!\n");
		release();
		return false;
	}

	return true;
}


void HeadMorphMesh::release()
{
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);

	QDXObject::release();
}


void HeadMorphMesh::render()
{
	if (m_indices.empty())
		return;

	ID3D11DeviceContext* pContext = QDXUT::immediateContext();

    UINT stride = sizeof(MorphMeshVertex);
    UINT offset = 0;
    pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
    pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pContext->DrawIndexed(m_indices.size(), 0, 0);
}


bool HeadMorphMesh::loadAndRectifySource(int srcIdx, QString meshName,
										const SourceTransform& transform)
{
	release();

    // Load mesh data from file using AssImp
    Assimp::Importer importer;
    const aiScene* pScene = importer.ReadFile(meshName.toStdString(),
        aiProcess_GenSmoothNormals  |
        aiProcess_Triangulate       |
        aiProcess_MakeLeftHanded    |
        aiProcess_FlipWindingOrder  |
        aiProcess_OptimizeMeshes    |
        aiProcess_JoinIdenticalVertices);

    if (!pScene || !pScene->HasMeshes())
    {
        printf("ERROR: invalid input meshes!\n");
        return false;
    }

	printf("Loading head morphing meshes...");

	const aiMesh* pMesh = pScene->mMeshes[0];

	int numVertices = pMesh->mNumVertices;
	int numIndices  = pMesh->mNumFaces * 3;

	// Load vertices
 	m_vertices.resize(numVertices); // NOTE!!
	for (int i = 0; i < pMesh->mNumVertices; i++)
	{
		MorphMeshVertex& vertex = m_vertices[i];

		if (srcIdx == 0)
		{
			memcpy(&(vertex.position), &(pMesh->mVertices[i]), sizeof(XMFLOAT3));
			memcpy(&(vertex.normal),   &(pMesh->mNormals[i]), sizeof(XMFLOAT3));
			memcpy(&(vertex.texcoord), &(pMesh->mTextureCoords[0][i]), sizeof(XMFLOAT2));
		}
		else if (srcIdx == 1)
			memcpy(&(vertex.pos1), &(pMesh->mVertices[i]), sizeof(XMFLOAT3));
#if MAX_NUM_MORPH_SRC > 2
		else if (srcIdx == 2)
			memcpy(&(vertex.pos2), &(pMesh->mVertices[i]), sizeof(XMFLOAT3));
#endif
#if MAX_NUM_MORPH_SRC > 3
		else if (srcIdx == 3)
			memcpy(&(vertex.pos3), &(pMesh->mVertices[i]), sizeof(XMFLOAT3));
#endif
#if MAX_NUM_MORPH_SRC > 4
		else if (srcIdx == 4)
			memcpy(&(vertex.pos4), &(pMesh->mVertices[i]), sizeof(XMFLOAT3));
#endif
	}

	// Load faces (indices)
 	m_indices.resize(numIndices);
    for (int i = 0; i < pMesh->mNumFaces; i++)
    {
		m_indices[i*3+0] = pMesh->mFaces[i].mIndices[0];
		m_indices[i*3+1] = pMesh->mFaces[i].mIndices[1];
		m_indices[i*3+2] = pMesh->mFaces[i].mIndices[2];
    }

	rectifyVertices(srcIdx, transform);

	// debug: calculate center coordinates
	float avgX = 0, avgY = 0, avgZ = 0;
	float minX = 0, minY = 0, minZ = 0;
	float maxX = 0, maxY = 0, maxZ = 0;
	for (int i = 0; i < m_vertices.size(); i++)
	{
		float x = m_vertices[i].position.x;
		float y = m_vertices[i].position.y;
		float z = m_vertices[i].position.z;

		avgX += x;
		avgY += y;
		avgZ += z;

		if (x < minX) minX = x;
		if (y < minY) minY = y;
		if (z < minZ) minZ = z;
		if (x > maxX) maxX = x;
		if (y > maxY) maxY = y;
		if (z > maxZ) maxZ = z;
	}
	avgX /= (float)m_vertices.size();
	avgY /= (float)m_vertices.size();
	avgZ /= (float)m_vertices.size();

	printf("MIN: %f %f %f\n", minX, minY, minZ);
	printf("MAX: %f %f %f\n", maxX, maxY, maxZ);
	printf("AVG: %f %f %f\n", avgX, avgY, avgZ);

	// set scalp space center
	CoordUtil::setScalpSpaceCenter(XMFLOAT3(avgX, avgY, avgZ));

	if (!create())
	{
		printf("ERROR: failed to create DX buffers!\n");
		return false;
	}


	printf("Done.\n");

	return true;
}


void HeadMorphMesh::clear()
{
	m_vertices.clear();
	m_indices.clear();

	release();
}


void HeadMorphMesh::rectifyVertices(int srcIdx, const SourceTransform& transform)
{
	// invert rotation
	XMVECTOR det;
	XMMATRIX mRot = XMMatrixRotationQuaternion(XMLoadFloat4(&transform.rotation));
	mRot = XMMatrixInverse(&det, mRot);

	for (int i = 0; i < m_vertices.size(); i++)
	{
		//MorphMeshVertex& v = m_vertices[i];

		XMFLOAT3* pPos;

		if (srcIdx == 0) 
			pPos = &(m_vertices[i].position);
		else if (srcIdx == 1) 
			pPos = &(m_vertices[i].pos1);
#if MAX_NUM_MORPH_SRC > 2
		else if (srcIdx == 2) 
			pPos = &(m_vertices[i].pos2);
#endif
#if MAX_NUM_MORPH_SRC > 3
		else if (srcIdx == 3) 
			pPos = &(m_vertices[i].pos3);
#endif
#if MAX_NUM_MORPH_SRC > 4
		else if (srcIdx == 4) 
			pPos = &(m_vertices[i].pos4);
#endif

		XMStoreFloat3(pPos, XMVector3TransformCoord(XMLoadFloat3(pPos), mRot));
	}

	clearTransform();
}

