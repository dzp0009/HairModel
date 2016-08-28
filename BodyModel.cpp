#include "BodyModel.h"

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <QImage>
#include <QIODevice>

BodyModel::BodyModel()
	: m_pVB(NULL), m_pIB(NULL), m_indexCount(0), 
	m_vertices(NULL), m_vertexCount(0)
{
	
}


BodyModel::~BodyModel()
{
	SAFE_DELETE_ARRAY(m_vertices);

	release();
}


bool BodyModel::loadColorDepth(QString colorFile, QString depthFile)
{
	// Load color data
	QImage tempImg;
	if (!tempImg.load(colorFile))
		return false;

	m_image = tempImg.convertToFormat(QImage::Format_ARGB32);

	if (!QDXImageSprite::create()) 
		return false;

	return loadDepth(depthFile);
}


bool BodyModel::loadColorNoDepth(QString colorFile)
{
	// Load color data
	QImage tempImg;
	if (!tempImg.load(colorFile))
		return false;

	m_image = tempImg.convertToFormat(QImage::Format_ARGB32);

	if (!QDXImageSprite::create()) 
		return false;

	clearDepth();

	return true;
}


bool BodyModel::loadDepth(QString depthFile)
{
	using namespace cv;
	//
	// Load depth data and create mesh
	//
	Mat depthData = imread(depthFile.toStdString(), CV_LOAD_IMAGE_GRAYSCALE);
	if (depthData.empty())
		return false;

	int quality = 10;

	int numCellsX = (float)width() / quality;
	int numCellsY = (float)height() / quality;

	float cellWidth = (float)width() / (float)numCellsX;
	float cellHeight = (float)height() / (float)numCellsY;

	int numVertsX = numCellsX + 1;
	int numVertsY = numCellsY + 1;

	// Calculate vertices

	int numVertices = numVertsX * numVertsY;

	SAFE_DELETE_ARRAY(m_vertices);
	m_vertices = new QDXImageQuadVertex[numVertices];

	int vId = 0;
	for (int i = 0; i < numVertsY; i++)
	{
		float v = (float)i / (float)(numVertsY - 1);
		float y = v * (float)height();

		for (int j = 0; j < numVertsX; j++)
		{
			float u = (float)j / (float)(numVertsX - 1);
			float x = u * (float)width();

			Mat sample;
			getRectSubPix(depthData, Size(1,1), Point2f(x-0.5f, y-0.5f), sample);

			float z = 127.5f - (float)sample.at<uchar>(0,0);

			m_vertices[vId].position = XMFLOAT3(x, -y, z);
			m_vertices[vId].texcoord = XMFLOAT2(u, v);

			m_vertices[vId].reserved = XMFLOAT3(1, 1, 1);

			//if (m_image.alp)

			vId++;
		}
	}

	// Calculate indices

	int numIndices = numCellsX * numCellsY * 2 * 3;

	uint* pIndices = new uint[numIndices];

	int iId = 0;
	//bool sl
	for (int i = 0; i < numCellsY; i++)
	{
		for (int j = 0; j < numCellsX; j++)
		{
			uint vTopLeft     = i * numVertsX + j;
			uint vTopRight    = vTopLeft + 1;
			uint vBottomLeft  = vTopLeft + numVertsX;
			uint vBottomRight = vBottomLeft + 1;

			pIndices[iId++] = vTopLeft;
			pIndices[iId++] = vTopRight;
			pIndices[iId++] = vBottomLeft;

			pIndices[iId++] = vBottomLeft;
			pIndices[iId++] = vTopRight;
			pIndices[iId++] = vBottomRight;
		}
	}

	//
	// Create D3D buffers
	//

	SAFE_RELEASE(m_pVB);
	SAFE_RELEASE(m_pIB);

	// Vertex buffer

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.ByteWidth	= sizeof(QDXImageQuadVertex) * numVertices;
	desc.Usage		= D3D11_USAGE_IMMUTABLE;
	desc.BindFlags	= D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = m_vertices;

	if (S_OK != QDXUT::device()->CreateBuffer(&desc, &initData, &m_pVB))
	{
		release();
		return false;
	}

	// Index buffer

	desc.ByteWidth	= sizeof(uint) * numIndices;
	desc.BindFlags	= D3D11_BIND_INDEX_BUFFER;

	initData.pSysMem = pIndices;

	if (S_OK != QDXUT::device()->CreateBuffer(&desc, &initData, &m_pIB))
	{
		release();
		return false;
	}

	delete[] pIndices;

	m_vertexCount = numVertices;
	m_indexCount = numIndices;

	return true;
}


bool BodyModel::updateVertexBuffer()
{
	if (m_vertexCount < 1 || m_vertices == NULL)
		return false;

	SAFE_RELEASE(m_pVB);

	// Vertex buffer

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.ByteWidth	= sizeof(QDXImageQuadVertex) * m_vertexCount;
	desc.Usage		= D3D11_USAGE_IMMUTABLE;
	desc.BindFlags	= D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = m_vertices;

	if (S_OK != QDXUT::device()->CreateBuffer(&desc, &initData, &m_pVB))
	{
		release();
		return false;
	}

	return true;
}


void BodyModel::clearDepth()
{
	//
	// Create a mesh with no depth info
	//

	SAFE_RELEASE(m_pVB);
	SAFE_RELEASE(m_pIB);

	QDXImageQuadVertex quad[4] = {
		{
			XMFLOAT3(0.0f, 0.0f, 0),
				XMFLOAT2(0.0f, 0.0f)
		},
		{
			XMFLOAT3((float)width(), 0.0f, 0),
				XMFLOAT2(1.0f, 0.0f)
			},
			{
				XMFLOAT3(0.0f, -(float)height(), 0),
					XMFLOAT2(0.0f, 1.0f)
			},
			{
				XMFLOAT3((float)width(), -(float)height(), 0),
					XMFLOAT2(1.0f, 1.0f)
				},
	};

	D3D11_BUFFER_DESC buffDesc;
	ZeroMemory(&buffDesc, sizeof(buffDesc));
	buffDesc.ByteWidth	= sizeof(QDXImageQuadVertex) * 4;
	buffDesc.Usage		= D3D11_USAGE_IMMUTABLE;
	buffDesc.BindFlags	= D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = quad;

	if (S_OK != QDXUT::device()->CreateBuffer(&buffDesc, &initData, &m_pVB))
	{
		release();
		return;
	}

	// Index buffer

	uint indices[6] = { 0, 1, 2,  2, 1, 3 };

	buffDesc.ByteWidth	= sizeof(uint) * 6;
	buffDesc.BindFlags	= D3D11_BIND_INDEX_BUFFER;

	initData.pSysMem = indices;

	if (S_OK != QDXUT::device()->CreateBuffer(&buffDesc, &initData, &m_pIB))
	{
		release();
		return;
	}

	m_indexCount = 6;
}


void BodyModel::release()
{
//	m_mesh.release();

	QDXImageSprite::release();

	SAFE_RELEASE(m_pVB);
	SAFE_RELEASE(m_pIB);

	m_indexCount = 0;
}


void BodyModel::render()
{
	if (!m_pTexture)
		return;

	ID3D11DeviceContext* pContext = QDXUT::immediateContext();

	if (m_isDirty)
	{
		// Copy image data from QImage to ID3D11Texture2D
		D3D11_MAPPED_SUBRESOURCE mapped;
		HRESULT hr = pContext->Map(m_pTexture, 0, 
			D3D11_MAP_WRITE_DISCARD, 0,
			&mapped);
		if (hr == S_OK)
		{
			uint iDst = 0;
			uchar* pDstData = (uchar*)(mapped.pData);
			// TODO: if (mapped.RowPitch == m_image.bytesPerLine)...
			for (int y = 0; y < m_image.height(); y++)
			{
				memcpy_s(&(pDstData[iDst]), mapped.RowPitch,
					m_image.scanLine(y), m_image.bytesPerLine());
				iDst += mapped.RowPitch;
			}
			pContext->Unmap(m_pTexture, 0);

			m_isDirty = false;
		}
	}

	// Render image quad
	const UINT stride = sizeof(QDXImageQuadVertex);
	const UINT offset = 0;

	pContext->IASetVertexBuffers(0, 1, &m_pVB, &stride, &offset);
	pContext->IASetIndexBuffer(m_pIB, DXGI_FORMAT_R32_UINT, 0);
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pContext->DrawIndexed(m_indexCount, 0, 0);
}
