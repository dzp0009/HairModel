#include "MorphController.h"

#include <QFile>

MorphController::MorphController(void)
	: m_lockTransform(false)
{
}


MorphController::~MorphController(void)
{
}


void MorphController::setWeights(const std::vector<float>& weights)
{
	if (weights.size() != m_weights.size())
		printf("WARNING: weights.size do not match!\n");

	const int minSize = std::min(m_weights.size(), weights.size());
	for (int i = 0; i < minSize; i++)
		m_weights[i] = weights[i];
	for (int i = minSize; i < m_weights.size(); i++)
		m_weights[i] = 0;

	if (!m_lockTransform)
		updateCurrTransforms();
}


void MorphController::updateCurrTransforms()
{
	XMVECTOR loc = XMVectorZero();
	XMVECTOR rot = XMVectorZero();
	XMVECTOR scale = XMVectorZero();

	for (int i = 0; i < m_srcTransforms.size(); i++)
	{
		const float w = m_weights[i];
		loc = XMVectorAdd(loc, XMLoadFloat3(&m_srcTransforms[i].location)*w);
		rot = XMVectorAdd(rot, XMLoadFloat4(&m_srcTransforms[i].rotation)*w);
		scale = XMVectorAdd(scale, XMLoadFloat3(&m_srcTransforms[i].scale)*w);
	}
	
	XMMATRIX mat = XMMatrixTransformation(XMVectorZero(), XMQuaternionIdentity(),
					scale, XMVectorZero(), rot, loc);

	XMStoreFloat4x4(&m_mWorld, mat);

	//XMVECTOR loc = XMVectorLerp(XMLoadFloat3(&m_srcLocation), 
	//							XMLoadFloat3(&m_dstLocation), m_t);
	//XMVECTOR rot = XMVectorLerp(XMLoadFloat4(&m_srcRotation),
	//							XMLoadFloat4(&m_dstRotation), m_t);
	//XMVECTOR scale = XMVectorLerp(XMLoadFloat3(&m_srcScale), 
	//							  XMLoadFloat3(&m_dstScale), m_t);

	//XMMATRIX mat = XMMatrixTransformation(XMVectorZero(), XMQuaternionIdentity(),
	//				scale, XMVectorZero(), rot, loc);

	//XMStoreFloat4x4(&m_mWorld, mat);
}


bool MorphController::addSourceFromFile(QString coefFileName, int imgHeight)
{
	QFile file(coefFileName);
	if (!file.open(QIODevice::ReadOnly))
	{
		printf("ERROR: failed to open coefficients file.\n");
		return false;
	}

	SourceTransform transform;
	transform.imageHeight = imgHeight;

	// rotation
	XMFLOAT4X4 mat;
	ZeroMemory(&mat, sizeof(float)*16);
	mat._44 = 1.0f;

	char buffer[256];
	file.readLine(buffer, 256);
	sscanf_s(buffer, "%f %f %f\n", &mat._11, &mat._12, &mat._13);
	file.readLine(buffer, 256);
	sscanf_s(buffer, "%f %f %f\n", &mat._21, &mat._22, &mat._23);
	file.readLine(buffer, 256);
	sscanf_s(buffer, "%f %f %f\n", &mat._31, &mat._32, &mat._33);

	XMVECTOR qRot = XMQuaternionRotationMatrix(XMLoadFloat4x4(&mat));
	
	XMStoreFloat4(&transform.rotation, qRot);
	transform.rotation.z = -transform.rotation.z;	// correction for diff handedness

	// Translation
	float x = 0, y = 0, z = 0;
	file.readLine(buffer, 256);
	sscanf_s(buffer, "%f %f %f\n", &x, &y, &z);
	transform.location = XMFLOAT3(x, y - imgHeight, -z);

	// Scaling
	float scale = 1.0f;
	file.readLine(buffer, 256);
	sscanf_s(buffer, "%f\n", &scale);
	transform.scale = XMFLOAT3(scale, scale, scale);

	m_srcTransforms.push_back(transform);

	m_weights.resize(m_srcTransforms.size());

	return true;
}


void MorphController::clear()
{
	m_srcTransforms.clear();
	m_weights.clear();
}

//bool MorphController::loadTransforms(QString srcFileName, int srcImgHeight, QString dstFileName, int dstImgHeight)
//{
//	m_srcImgHeight = srcImgHeight;
//	m_dstImgHeight = dstImgHeight;
//
//	// SRC
//	QFile srcFile(srcFileName);
//	if (!srcFile.open(QIODevice::ReadOnly))
//	{
//		printf("ERROR: failed to load source transform file.\n");
//		return false;
//	}
//
//	// rotation
//	XMFLOAT4X4 mat;
//	ZeroMemory(&mat, sizeof(float)*16);
//	mat._44 = 1.0f;
//
//	char buffer[256];
//	srcFile.readLine(buffer, 256);
//	sscanf_s(buffer, "%f %f %f\n", &mat._11, &mat._12, &mat._13);
//	srcFile.readLine(buffer, 256);
//	sscanf_s(buffer, "%f %f %f\n", &mat._21, &mat._22, &mat._23);
//	srcFile.readLine(buffer, 256);
//	sscanf_s(buffer, "%f %f %f\n", &mat._31, &mat._32, &mat._33);
//
//	XMVECTOR qRot = XMQuaternionRotationMatrix(XMLoadFloat4x4(&mat));
//	
//	XMStoreFloat4(&m_srcRotation, qRot);
//	m_srcRotation.z = -m_srcRotation.z;	// correction for diff handedness
//
//	// Translation
//	float x = 0, y = 0, z = 0;
//	srcFile.readLine(buffer, 256);
//	sscanf_s(buffer, "%f %f %f\n", &x, &y, &z);
//	m_srcLocation = XMFLOAT3(x, y - srcImgHeight, -z);
//
//	// Scaling
//	float scale = 1.0f;
//	srcFile.readLine(buffer, 256);
//	sscanf_s(buffer, "%f\n", &scale);
//	m_srcScale = XMFLOAT3(scale, scale, scale);
//
//
//	// DST
//	QFile dstFile(dstFileName);
//	if (!dstFile.open(QIODevice::ReadOnly))
//	{
//		printf("ERROR: failed to load target transform file.\n");
//		return false;
//	}
//
//	// rotation
//	dstFile.readLine(buffer, 256);
//	sscanf_s(buffer, "%f %f %f\n", &mat._11, &mat._12, &mat._13);
//	dstFile.readLine(buffer, 256);
//	sscanf_s(buffer, "%f %f %f\n", &mat._21, &mat._22, &mat._23);
//	dstFile.readLine(buffer, 256);
//	sscanf_s(buffer, "%f %f %f\n", &mat._31, &mat._32, &mat._33);
//
//	qRot = XMQuaternionRotationMatrix(XMLoadFloat4x4(&mat));
//	
//	XMStoreFloat4(&m_dstRotation, qRot);
//	m_dstRotation.z = -m_dstRotation.z;	// correction for diff handedness
//
//	// Translation
//	dstFile.readLine(buffer, 256);
//	sscanf_s(buffer, "%f %f %f\n", &x, &y, &z);
//	m_dstLocation = XMFLOAT3(x, y - dstImgHeight, -z);
//
//	// Scaling
//	dstFile.readLine(buffer, 256);
//	sscanf_s(buffer, "%f\n", &scale);
//	m_dstScale = XMFLOAT3(scale, scale, scale);
//
//
//	updateCurrTransforms();
//
//	return true;
//}