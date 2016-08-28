#include "HairStrandModel.h"

#include <QFile>
#include <QIODevice>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <ANN/ANN.h>

#include "HairUtil.h"

////////////////////////////////////////////////////////

const D3D11_INPUT_ELEMENT_DESC StrandVertex::s_elemDesc[] =
{
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"COLOR",	 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},

#if MAX_NUM_MORPH_SRC > 2
	{"TEXCOORD", 2, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"TEXCOORD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
#endif
#if MAX_NUM_MORPH_SRC > 3
	{"TEXCOORD", 4, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"TEXCOORD", 5, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
#endif
#if MAX_NUM_MORPH_SRC > 4
	{"TEXCOORD", 6, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"TEXCOORD", 7, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
#endif
};

const int StrandVertex::s_numElem = sizeof(s_elemDesc) / sizeof(s_elemDesc[0]);


////////////////////////////////////////////////////////

Strand::Strand() : m_length(0), m_clusterId(-1), m_weight(1)
{
}


void Strand::createEmpty(int numVerts)
{
	StrandVertex vert;
	memset(&vert, 0, sizeof(vert));
	//vert.color.w = 1.0f;		
	m_vertices.assign(numVerts, vert);

	m_length = 0;
	m_clusterId = -1;
}


void Strand::appendVertex(const StrandVertex& vertex, float segLen /* = -1.0f */)
{
	m_vertices.push_back(vertex);

	if (segLen > 0.0f)
	{
		m_length += segLen;
	}
	else if (m_vertices.size() > 1)
	{
		XMVECTOR v1 = XMLoadFloat3(&(m_vertices[m_vertices.size() - 2].position));
		XMVECTOR v2 = XMLoadFloat3(&(vertex.position));
		XMVECTOR dist = XMVector3Length(XMVectorSubtract(v1, v2));

		m_length += XMVectorGetX(dist);
	}
}


void Strand::prependVertex(const StrandVertex& vertex, float segLen /* = -1.0f */)
{
	m_vertices.insert(m_vertices.begin(), vertex);

	if (segLen > 0.0f)
	{
		m_length += segLen;
	}
	else if (m_vertices.size() > 1)
	{
		XMVECTOR v1 = XMLoadFloat3(&(m_vertices[1].position));
		XMVECTOR v2 = XMLoadFloat3(&(vertex.position));
		XMVECTOR dist = XMVector3Length(XMVectorSubtract(v1, v2));

		m_length += XMVectorGetX(dist);
	}
}


void Strand::resample(int nSamples)
{
	if (m_vertices.empty() || nSamples < 2)
		return;

	std::vector<StrandVertex> newVertices(nSamples);

	// naive
	newVertices.front() = m_vertices.front();
	newVertices.back()  = m_vertices.back();
	for (int i = 1; i < nSamples - 1; i++)
	{
		float t = (float)(m_vertices.size() - 1) * (float)i / (nSamples - 1);
		
		int j = (int)t;
		float w = t - (float)j;

		const StrandVertex& v1 = m_vertices[j];
		const StrandVertex& v2 = m_vertices[j+1];
		StrandVertex& newV = newVertices[i];
		newV = v1;
		XMStoreFloat3(&newV.position, XMVectorLerp(XMLoadFloat3(&v1.position), XMLoadFloat3(&v2.position), w));
		XMStoreFloat4(&newV.color, XMVectorLerp(XMLoadFloat4(&v1.color), XMLoadFloat4(&v2.color), w));
	}

	m_vertices = newVertices;
}


// (Re)calculate this strand's curve length.
float Strand::updateLength()
{
	m_length = 0;

	XMVECTOR prevPos = XMLoadFloat3(&(m_vertices[0].position));
	for (int i = 1; i < numVertices(); i++)
	{
		XMVECTOR currPos = XMLoadFloat3(&(m_vertices[i].position));
		m_length += XMVectorGetX(XMVector3Length(XMVectorSubtract(currPos, prevPos)));

		prevPos = currPos;
	}
	return m_length;
}


// Set a uniform color for all strand vertices (basically for debugging purpose).
void Strand::setColor(const XMFLOAT4& color)
{
	for (int i = 0; i < numVertices(); i++)
		m_vertices[i].color = color;
}


void Strand::trim(int vId)
{
	m_vertices.resize(std::min(std::max(vId + 1, 1), (int)m_vertices.size()));

	// also set tip alpha to 0
	if (m_vertices.size() > 1)
	{
		// smooth out tip alpha
		m_vertices.back().color.w = 0;
		m_vertices[m_vertices.size()-2].color.w *= 0.5f;
		//m_vertices.back().shading.w = 0;
		//m_vertices.back().color2.w = 0;
		//m_vertices.back().color3.w = 0;
		//m_vertices.back().color4.w = 0;
	}
}


void Strand::calcReferenceFrames()
{
	XMFLOAT3 stdTangent(0,1,0);
	XMFLOAT3 stdNormal(1,0,0);
	XMFLOAT3 stdBinormal(0,0,1);

	// root frame
	XMFLOAT3 tangent, normal, binormal;
	XMStoreFloat3(&tangent, XMVector3Normalize(XMLoadFloat3(&m_vertices[1].position) - 
											   XMLoadFloat3(&m_vertices[0].position)));
	calcFrameByRotate(stdTangent, stdNormal, stdBinormal, tangent, normal, binormal);

	m_vertices[0].pos2 = tangent;
	m_vertices[0].pos3 = normal;
	m_vertices[0].pos4 = binormal;

	const int numVerts = m_vertices.size();

	// middle frames
	for (int i = 1; i < numVerts - 1; i++)
	{
		XMStoreFloat3(&tangent, XMVector3Normalize(XMLoadFloat3(&m_vertices[i+1].position) -
												   XMLoadFloat3(&m_vertices[i-1].position)));
		calcFrameByRotate(m_vertices[i-1].pos2, m_vertices[i-1].pos3, m_vertices[i-1].pos4,
						  tangent, normal, binormal);
		m_vertices[i].pos2 = tangent;
		m_vertices[i].pos3 = normal;
		m_vertices[i].pos4 = binormal;
	}

	// tip frame
	XMStoreFloat3(&tangent, XMVector3Normalize(XMLoadFloat3(&m_vertices[numVerts-1].position) -
											   XMLoadFloat3(&m_vertices[numVerts-2].position)));
	calcFrameByRotate(m_vertices[numVerts-2].pos2, m_vertices[numVerts-2].pos3, 
					  m_vertices[numVerts-2].pos4, tangent, normal, binormal);
	m_vertices[numVerts-1].pos2 = tangent;
	m_vertices[numVerts-1].pos3 = normal;
	m_vertices[numVerts-1].pos4 = binormal;
}


void Strand::calcFrameByRotate(const XMFLOAT3& srcTangent, const XMFLOAT3& srcNormal, const XMFLOAT3& srcBinormal,
							  const XMFLOAT3& dstTangent, XMFLOAT3& dstNormal, XMFLOAT3& dstBinormal)
{
	XMFLOAT4X4 mat;
	calcMinRotation(srcTangent, dstTangent, mat);
	XMMATRIX M = XMLoadFloat4x4(&mat);

	// rotate normal/binormal
	XMVECTOR Normal = XMVector3TransformCoord(XMLoadFloat3(&srcNormal), M);
	XMVECTOR Binorm = XMVector3TransformCoord(XMLoadFloat3(&srcBinormal), M);

	// normalize
	XMStoreFloat3(&dstNormal, XMVector3Normalize(Normal));
	XMStoreFloat3(&dstBinormal, XMVector3Normalize(Binorm));
}


void Strand::calcMinRotation(const XMFLOAT3& vec1, const XMFLOAT3& vec2, XMFLOAT4X4& mat)
{
	// axis of rotation
	XMVECTOR v1 = XMLoadFloat3(&vec1);
	XMVECTOR v2 = XMLoadFloat3(&vec2);
	XMVECTOR vA = XMVector3Normalize(XMVector3Cross(v1, v2));

	XMFLOAT3 a;
	XMStoreFloat3(&a, vA);

	// helper variables
	float sq_x = a.x*a.x,   sq_y = a.y*a.y,   sq_z = a.z*a.z;
	float cos = XMVectorGetX(XMVector3Dot(v1, v2));
	if (cos > 1.0f) cos = 1.0f;
	else if (cos < -1.0f) cos = -1.0f;
	float cos1 = 1.0f - cos;
	float xycos1 = a.x*a.y*cos1,   yzcos1 = a.y*a.z*cos1,   zxcos1 = a.x*a.z*cos1;
	float sin = sqrtf(1 - cos*cos);
	float xsin = a.x*sin,    ysin = a.y*sin,    zsin = a.z*sin;

	// matrix
	memset(&mat, 0, sizeof(float)*16);
	mat._11 = sq_x + (1-sq_x)*cos;	mat._12 = xycos1 + zsin;			mat._13 = zxcos1 - ysin;
	mat._21 = xycos1 - zsin;		mat._22 = sq_y + (1-sq_y)*cos;		mat._23 = yzcos1 + xsin;
	mat._31 = zxcos1 + ysin;		mat._32 = yzcos1 - xsin;			mat._33 = sq_z + (1-sq_z)*cos;
	mat._44 = 1.0f;
}


////////////////////////////////////////////////////////

HairStrandModel::HairStrandModel()
	: m_pVertexBuffer(NULL), m_pIndexBuffer(NULL), 
	m_vertexCount(0), m_indexCount(0)
{
	m_strandWidth = 1.0f;
	setScale(1, -1, 1);
}


HairStrandModel::~HairStrandModel()
{
	release();
}


// copy ctor 
HairStrandModel::HairStrandModel(const HairStrandModel& model)
	: QDXObject(model), m_pVertexBuffer(NULL), m_pIndexBuffer(NULL),
	m_vertexCount(0), m_indexCount(0)
{
	m_strands = model.m_strands;
	m_strandWidth = model.m_strandWidth;

	if (model.m_pVertexBuffer && model.m_pIndexBuffer)
		updateBuffers();
}


HairStrandModel& HairStrandModel::operator=(const HairStrandModel& model)
{
	QDXObject::operator=(model);

	clear();
	m_strands = model.m_strands;
	m_strandWidth = model.m_strandWidth;

	if (model.m_pVertexBuffer && model.m_pIndexBuffer)
		updateBuffers();

	return *this;
}


void HairStrandModel::createEmpty(int numStrands)
{
	release();

	Strand strand;
	m_strands.assign(numStrands, strand);

	m_nbrStrandIDs.clear();
}


void HairStrandModel::createEmptyUnisam(int numStrands, int vertsPerStrand)
{
	release();

	Strand strand;
	strand.createEmpty(vertsPerStrand);
	m_strands.assign(numStrands, strand);

	m_nbrStrandIDs.clear();
}


// Load hair strands with geometry and color data
bool HairStrandModel::load(QString filename, bool updateBuffs)
{
	QTime timer;
	timer.start();

	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly))
		return false;

	uint numStrands = 0;
	file.read((char*)&numStrands, sizeof(uint));

	if (numStrands < 1)
		return false;

	clear();
	createEmpty(numStrands);

	printf("Loading %d hair strands...\n", numStrands);
	for (int i = 0; i < numStrands; i++)
	{
		uint numVertices = 0;
		file.read((char*)&numVertices, sizeof(uint));

		if (numVertices < 1)
			return false;

		m_strands[i].createEmpty(numVertices);
		StrandVertex* vertices = m_strands[i].vertices();

		for (int j = 0; j < numVertices; j++)
		{
			file.read((char*)&(vertices[j].position), sizeof(float)*3);
			// Flip Z
			vertices[j].position.z = -vertices[j].position.z;

			file.read((char*)&(vertices[j].color), sizeof(float)*4);
		}
	}
	file.close();

	if (updateBuffs)
		updateBuffers();

	printf("Hair loaded in %.3f seconds.\n", (float)timer.elapsed()/1000.f);

	calcRootNbrsByANN(32);

	return true;
}


// Save hair strands with geometry and color data
bool HairStrandModel::save(QString filename)
{
	if (m_strands.size() < 1)
		return false;

	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly))
		return false;

	uint numStrands = m_strands.size();
	file.write((const char*)&numStrands, sizeof(uint));

	for (int i = 0; i < m_strands.size(); i++)
	{
		uint numVertices = m_strands[i].numVertices();
		file.write((const char*)&numVertices, sizeof(uint));

		for (int j = 0; j < numVertices; j++)
		{
			XMFLOAT3 pos = m_strands[i].vertices()[j].position;
			pos.z = -pos.z;

			XMFLOAT4 clr = m_strands[i].vertices()[j].color;

			file.write((const char*)&pos, sizeof(float)*3);
			file.write((const char*)&clr, sizeof(float)*4);
		}
	}

	file.close();

	return true;
}


bool HairStrandModel::loadGeometry(QString filename)
{
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly))
		return false;

	uint numStrands = 0;
	file.read((char*)&numStrands, sizeof(uint));

	if (numStrands < 1)
		return false;

	clear();

	for (int i = 0; i < numStrands; i++)
	{
		uint numVertices = 0;
		file.read((char*)&numVertices, sizeof(uint));

		if (numVertices < 1)
			return false;

		Strand strand;

		StrandVertex vertex;
		vertex.color = XMFLOAT4(1, 1, 1, 1);
		
		for (int j = 0; j < numVertices; j++)
		{
			file.read((char*)&(vertex.position), sizeof(float)*3);

			// Flip Z
			vertex.position.z = -vertex.position.z;

			strand.appendVertex(vertex);
		}
		addStrand(strand);
	}
	file.close();

	updateBuffers();

	return true;
}


bool HairStrandModel::saveGeometry(QString filename)
{
	if (m_strands.size() < 1)
		return false;

	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly))
		return false;

	uint numStrands = m_strands.size();
	file.write((const char*)&numStrands, sizeof(uint));

	for (int i = 0; i < m_strands.size(); i++)
	{
		uint numVertices = m_strands[i].numVertices();
		file.write((const char*)&numVertices, sizeof(uint));

		for (int j = 0; j < numVertices; j++)
		{
			XMFLOAT3 pos = m_strands[i].vertices()[j].position;

			pos.z = -pos.z;

			file.write((const char*)&pos, sizeof(float)*3);
		}
	}

	file.close();

	return true;
}


void HairStrandModel::release()
{
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);
	m_vertexCount = 0;
	m_indexCount  = 0;
}


void HairStrandModel::render()
{
	if (!m_pVertexBuffer || !m_pIndexBuffer)
		return;

	ID3D11DeviceContext* pContext = QDXUT::immediateContext();

	UINT stride = sizeof(StrandVertex);
	UINT offset = 0;

	pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST_ADJ);
	pContext->DrawIndexed(m_indexCount, 0, 0);
}


void HairStrandModel::addStrand(const Strand& strand)
{
	m_strands.push_back(strand);
}


void HairStrandModel::clear()
{
	m_strands.clear();
	m_nbrStrandIDs.clear();
	release();
}


bool HairStrandModel::updateBuffers()
{
	// TODO: if vertex/index count haven't been changed...

	release();

	srand(20130119);

	//calcTangents();

	// Calculate total vertex/index count
	uint numVertices = 0;
	uint numIndices  = 0;
	for (int i = 0; i < numStrands(); i++)
	{
		numVertices += m_strands[i].numVertices();
		numIndices  += (m_strands[i].numVertices() - 1) * 4; // with adjacency
	}

	if (numVertices < 1 || numIndices < 2)
		return true;

	// Create vertex and index buffer data
	StrandVertex* pVertexData = new StrandVertex[numVertices];
	uint*		  pIndexData  = new uint[numIndices];

	int vId = 0, iId = 0;
	for (int i = 0; i < numStrands(); i++)
	{
		const StrandVertex* pVerts = m_strands[i].vertices();

		// Add root vertex of this strand
		pVertexData[vId] = pVerts[0];

		float randNum = (float)rand() / (float)RAND_MAX;
		pVertexData[vId].texcoord.x = randNum;
		pVertexData[vId].texcoord.y = 0.0f;

		vId++;

		// Add each segment
		for (int j = 1; j < m_strands[i].numVertices(); j++)
		{
			// index data
			pIndexData[iId++] = (j == 1) ? vId - 1 : vId - 2;
			pIndexData[iId++] = vId - 1;
			pIndexData[iId++] = vId;
			pIndexData[iId++] = (j == m_strands[i].numVertices()-1) ? vId : vId + 1;

			// vertex data
			pVertexData[vId] = pVerts[j];

			pVertexData[vId].texcoord.x = randNum;
			pVertexData[vId].texcoord.y = (float)j / (float)(m_strands[i].numVertices() - 1);
			//pVertexData[vId].texcoord.y = (float)qMin(j, m_strands[i].numVertices() - 1 - j);

			vId++;
		}
	}
	Q_ASSERT(vId == numVertices);
	Q_ASSERT(iId == numIndices);

	// Create D3D vertex buffer
	
	D3D11_BUFFER_DESC buffDesc = {0};
	buffDesc.BindFlags		= D3D11_BIND_VERTEX_BUFFER;
	buffDesc.ByteWidth		= sizeof(StrandVertex)*numVertices;
	//buffDesc.Usage		= D3D11_USAGE_IMMUTABLE;
	buffDesc.Usage			= D3D11_USAGE_DEFAULT;
	buffDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = pVertexData;

	if (S_OK != QDXUT::device()->CreateBuffer(&buffDesc, &initData, &m_pVertexBuffer))
	{
		SAFE_DELETE_ARRAY(pVertexData);
		SAFE_DELETE_ARRAY(pIndexData);
		release();
		return false;
	}

	// Create D3D index buffer

	buffDesc.BindFlags	= D3D11_BIND_INDEX_BUFFER;
	buffDesc.ByteWidth	= sizeof(uint)*numIndices;
	buffDesc.Usage		= D3D11_USAGE_IMMUTABLE;
	buffDesc.CPUAccessFlags = 0;

	initData.pSysMem	= pIndexData;

	if (S_OK != QDXUT::device()->CreateBuffer(&buffDesc, &initData, &m_pIndexBuffer))
	{
		SAFE_DELETE_ARRAY(pVertexData);
		SAFE_DELETE_ARRAY(pIndexData);
		release();
		return false;
	}

	SAFE_DELETE_ARRAY(pVertexData);
	SAFE_DELETE_ARRAY(pIndexData);

	m_vertexCount = numVertices;
	m_indexCount  = numIndices;

	return true;
}


// for DEBUG purpose
bool HairStrandModel::updateDebugBuffers()
{
	release();

	if (numStrands() < 1)
		return true;

	// Calculate total vertex/index count
	uint numVertices = 2*numStrands();
	uint numIndices  = 4*numStrands();

	std::vector<StrandVertex> vertices(numVertices);
	std::vector<uint>		  indices(numIndices);

	// generate random colors
	std::vector<XMFLOAT4> randColors(2000);
	randColors[0] = XMFLOAT4(0,0,0,1.0f);
	for (int i = 1; i < 2000; i++)
	{
		randColors[i].x = cv::theRNG().uniform(0.2f, 1.0f);
		randColors[i].y = cv::theRNG().uniform(0.2f, 1.0f);
		randColors[i].z = cv::theRNG().uniform(0.2f, 1.0f);
		randColors[i].w = 1.0f;
	}

	int vId = 0, iId = 0;
	for (int i = 0; i < numStrands(); i++)
	{
		const StrandVertex* pVerts = m_strands[i].vertices();

		// Add root vertex of this strand
		vertices[vId] = pVerts[0];

		float randNum = cv::theRNG().uniform(0.0f, 1.0f);
		vertices[vId].texcoord.x = randNum;
		vertices[vId].texcoord.y = 0.0f;
		vertices[vId].color = randColors[m_strands[i].clusterID() + 1];

		vId++;

		// Add segment
		// index data
		indices[iId++] = vId - 1;
		indices[iId++] = vId - 1;
		indices[iId++] = vId;
		indices[iId++] = vId;

		// vertex data
		vertices[vId] = pVerts[1];

		vertices[vId].texcoord.x = randNum;
		vertices[vId].texcoord.y = 1.0f;
		vertices[vId].color = randColors[m_strands[i].clusterID() + 1];

		vId++;
	}
	Q_ASSERT(vId == numVertices);
	Q_ASSERT(iId == numIndices);

	// Create D3D vertex buffer
	
	D3D11_BUFFER_DESC buffDesc = {0};
	buffDesc.BindFlags		= D3D11_BIND_VERTEX_BUFFER;
	buffDesc.ByteWidth		= sizeof(StrandVertex)*numVertices;
	//buffDesc.Usage		= D3D11_USAGE_IMMUTABLE;
	buffDesc.Usage			= D3D11_USAGE_DEFAULT;
	buffDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = vertices.data();

	if (S_OK != QDXUT::device()->CreateBuffer(&buffDesc, &initData, &m_pVertexBuffer))
	{
		release();
		return false;
	}

	// Create D3D index buffer

	buffDesc.BindFlags	= D3D11_BIND_INDEX_BUFFER;
	buffDesc.ByteWidth	= sizeof(uint)*numIndices;
	buffDesc.Usage		= D3D11_USAGE_IMMUTABLE;
	buffDesc.CPUAccessFlags = 0;

	initData.pSysMem	= indices.data();

	if (S_OK != QDXUT::device()->CreateBuffer(&buffDesc, &initData, &m_pIndexBuffer))
	{
		release();
		return false;
	}

	m_vertexCount = numVertices;
	m_indexCount  = numIndices;

	return true;
}


// Filter hair strands depth
void HairStrandModel::filterDepth(const HairFilterParam& params)
{
	for (int i = 0; i < m_strands.size(); i++)
	{
		cv::Mat depthData(1, m_strands[i].numVertices(), CV_32FC1);

		for (int j = 0; j < depthData.cols; j++)
		{
			depthData.ptr<float>(0)[j] = m_strands[i].vertices()[j].position.z;
		}

		cv::GaussianBlur(depthData, depthData, cv::Size(-1,-1), params.sigmaDepth);

		for (int j = 0; j < depthData.cols; j++)
		{
			m_strands[i].vertices()[j].position.z = depthData.ptr<float>(0)[j];
		}
	}
}


void HairStrandModel::reverseOrder()
{
	std::reverse(m_strands.begin(), m_strands.end());
}

void HairStrandModel::filterGeometry(const HairFilterParam& params)
{
	for (int i = 0; i < m_strands.size(); i++)
	{
		cv::Mat depthData(1, m_strands[i].numVertices(), CV_32FC3);

		for (int j = 0; j < depthData.cols; j++)
		{
			depthData.ptr<cv::Vec3f>(0)[j] = cv::Vec3f(
				m_strands[i].vertices()[j].position.x,
				m_strands[i].vertices()[j].position.y,
				m_strands[i].vertices()[j].position.z);
		}

		cv::GaussianBlur(depthData, depthData, cv::Size(-1,-1), params.sigmaDepth);

		for (int j = 0; j < depthData.cols; j++)
		{
			cv::Vec3f& pos = depthData.ptr<cv::Vec3f>(0)[j];
			m_strands[i].vertices()[j].position = XMFLOAT3(pos[0], pos[1], pos[2]);
		}
	}

	updateBuffers();
}

void HairStrandModel::filterColorAlpha(const HairFilterParam& params)
{
	for (int i = 0; i < m_strands.size(); i++)
	{
		cv::Mat depthData(1, m_strands[i].numVertices(), CV_32FC4);

		for (int j = 0; j < depthData.cols; j++)
		{
			depthData.ptr<cv::Vec4f>(0)[j] = cv::Vec4f(
				m_strands[i].vertices()[j].color.x,
				m_strands[i].vertices()[j].color.y,
				m_strands[i].vertices()[j].color.z,
				m_strands[i].vertices()[j].color.w);
		}

		cv::GaussianBlur(depthData, depthData, cv::Size(-1,-1), params.sigmaDepth);

		for (int j = 0; j < depthData.cols; j++)
		{
			cv::Vec4f& color = depthData.ptr<cv::Vec4f>(0)[j];
			m_strands[i].vertices()[j].color = XMFLOAT4(color[0], color[1], color[2], color[3]);
		}
	}

	updateBuffers();
}

void HairStrandModel::calcTangents()
{
	for (int i = 0; i < m_strands.size(); i++)
	{
		StrandVertex* verts = m_strands[i].vertices();
		int			  n		= m_strands[i].numVertices();

		// First vertex
		//
		XMVECTOR v0 = XMLoadFloat3(&verts[0].position);
		XMVECTOR v1 = XMLoadFloat3(&verts[1].position);
		XMVECTOR tangent = XMVector3Normalize(v1 - v0);

		XMStoreFloat3(&verts[0].tangent, tangent);

		// Intermediate vertices
		for (int j = 1; j < n - 1; j++)
		{
			v0 = XMLoadFloat3(&verts[j-1].position);
			v1 = XMLoadFloat3(&verts[j+1].position);
			tangent = XMVector3Normalize(v1 - v0);

			XMStoreFloat3(&verts[j].tangent, tangent);
		}

		// Last vertex
		//
		v0 = XMLoadFloat3(&verts[n-2].position);
		v1 = XMLoadFloat3(&verts[n-1].position);
		tangent = XMVector3Normalize(v1 - v0);

		XMStoreFloat3(&verts[n-1].tangent, tangent);
	}
}


// Add geometric noise as in [Bonneel et al. 09]
void HairStrandModel::addGeometryNoise(const HairGeoNoiseParam& params)
{
	for (int i = 0; i < m_strands.size(); i++)
	{
		StrandVertex* verts = m_strands[i].vertices();
		int			  n		= m_strands[i].numVertices();

		float phase = 2.0f * XM_PI * (float)rand()/(float)RAND_MAX;

		// Handle first vertex separately
		XMVECTOR tangent = XMLoadFloat3(&verts[0].tangent);
		XMVECTOR normal  = HairUtil::randomNormal(tangent);

		for (int j = 1; j < n; j++)
		{
			float t = float(j) / float(n - 1);

			XMVECTOR newTangent = XMLoadFloat3(&verts[j].tangent);
			XMVECTOR newNormal  = HairUtil::calcNextNormal(
								  tangent, newTangent, normal);

			float offset = params.amplitude * 
						   sinf(2.0f * XM_PI * (float)j * params.frequency + phase);

			XMVECTOR pos = XMLoadFloat3(&verts[j].position);
			
			pos += offset * newNormal;
			
			XMStoreFloat3(&verts[j].position, pos);

			tangent = newTangent;
			normal  = newNormal;
		}
	}

	updateBuffers();
}


// Transform the already transformed vertices back to object space.
void HairStrandModel::rectifyVertices(const SourceTransform& transform)
{
	XMMATRIX mInvY  = XMMatrixScaling(1, -1, 1);
	XMMATRIX mLoc   = XMMatrixTranslation(transform.location.x, transform.location.y, transform.location.z);
	XMMATRIX mRot   = XMMatrixRotationQuaternion(XMLoadFloat4(&transform.rotation));
	XMMATRIX mScale = XMMatrixScaling(transform.scale.x, transform.scale.y, transform.scale.y);

	XMMATRIX mTrans = XMMatrixMultiply(mScale, mRot);
	mTrans = XMMatrixMultiply(mTrans, mLoc);
	mTrans = XMMatrixMultiply(mTrans, mInvY);

	XMVECTOR det;
	mTrans = XMMatrixInverse(&det, mTrans);

	const int nStrands = numStrands();
	for (int i = 0; i < nStrands; i++)
	{
		Strand& strand = m_strands[i];
		const int nVerts = strand.numVertices();

		for (int j = 0; j < nVerts; j++)
		{
			XMVECTOR pos = XMLoadFloat3(&strand.vertices()[j].position);
			pos = XMVector3Transform(pos, mTrans);
			XMStoreFloat3(&strand.vertices()[j].position, pos);
		}
	}

	clearTransform();

	// debug
	float minX = 0, minY = 0, minZ = 0;
	float maxX = 0, maxY = 0, maxZ = 0;
	for (int i = 0; i < nStrands; i++)
	{
		float x = m_strands[i].vertices()[0].position.x;
		float y = m_strands[i].vertices()[0].position.y;
		float z = m_strands[i].vertices()[0].position.z;

		if (x < minX) minX = x;
		if (y < minY) minY = y;
		if (z < minZ) minZ = z;
		if (x > maxX) maxX = x;
		if (y > maxY) maxY = y;
		if (z > maxZ) maxZ = z;
	}
	printf("Hair min: %f %f %f\n", minX, minY, minZ);
	printf("Hair max: %f %f %f\n", maxX, maxY, maxZ);
}


bool HairStrandModel::isUniformSampled(int nVertsPerStrand) const
{
	for (int i = 0; i < numStrands(); i++)
	{
		if (m_strands[i].numVertices() != nVertsPerStrand)
			return false;
	}
	return true;
}


void HairStrandModel::calcRootNbrsByANN(int numNbrs)
{
	printf("Finding root neighbors using ANN...");

	// fill in root points
	ANNpointArray	annPts = annAllocPts(numStrands(), 3);
	for (int i = 0; i < numStrands(); i++)
	{
		const XMFLOAT3& pos = m_strands[i].vertices()[0].position;
		annPts[i][0] = pos.x;
		annPts[i][1] = pos.y;
		annPts[i][2] = pos.z;
	}

	ANNpoint		queryPt = annAllocPt(3);
	ANNidxArray		nnIdx   = new ANNidx[numNbrs+1];
	ANNdistArray	dists	= new ANNdist[numNbrs+1];
	
	ANNkd_tree*		kdTree  = new ANNkd_tree(annPts, numStrands(), 3);

	m_nbrStrandIDs.resize(numStrands());
	for (int i = 0; i < numStrands(); i++)
		m_nbrStrandIDs[i].clear();

	for (int i = 0; i < numStrands(); i++)
	{
		kdTree->annkSearch(annPts[i], numNbrs+1, nnIdx, dists);

		for (int j = 0; j < numNbrs+1; j++)
		{
			if (nnIdx[j] != i)
			{
				m_nbrStrandIDs[i].push_back(nnIdx[j]);

				// also add i to the nbrs of nnIdx[j]
				m_nbrStrandIDs[nnIdx[j]].push_back(i);
			}
		}
	}

	// remove duplicates...
	printf("...and...");

	for (int i = 0; i < numStrands(); i++)
	{
		std::vector<int>& nbrIDs = m_nbrStrandIDs[i];
		std::sort(nbrIDs.begin(), nbrIDs.end());
		nbrIDs.erase(std::unique(nbrIDs.begin(), nbrIDs.end()), nbrIDs.end());
	}

	printf("DONE.\n");

	delete[] nnIdx;
	delete[] dists;
	delete kdTree;

	annDeallocPts(annPts);
	annDeallocPt(queryPt);
	annClose();
}


void HairStrandModel::clearRootNbrs()
{
	m_nbrStrandIDs.clear();
}

