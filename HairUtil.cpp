#ifndef _CRT_RAND_S
#define _CRT_RAND_S
#endif

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include <string>

#include <QImage>
#include <QFile>
#include <QIODevice>

#include "HairUtil.h"

using namespace cv;


void QDXLog::msg(QString str)
{
	LogItem item;
	item.type = LogMessage;
	item.time = QTime::currentTime();
	item.str  = str;

	m_logs.append(item);

	emit msgAdded(str);
}

void QDXLog::err(QString str)
{
	LogItem item;
	item.type = LogError;
	item.time = QTime::currentTime();
	item.str  = str;

	m_logs.append(item);

	emit errAdded(str);
}


const float HairUtil::InvalidOrient		= -4.0f * CV_PI;
const Vec2f	HairUtil::InvalidDirection	= Vec2f(0, 0);
const Vec4f HairUtil::InvalidTensor		= Vec4f(0, 0, 0, 0);


// Load an RGB image without pre-multiplying alpha
bool HairUtil::loadRgbFromRgba(QString filename, cv::Mat& img)
{
	QImage qImg(filename);

	if (qImg.isNull())
		return false;

	qImg = qImg.convertToFormat(QImage::Format_ARGB32);

	Mat tempImg(qImg.height(), qImg.width(), CV_8UC4, qImg.bits());

	img.create(tempImg.size(), CV_8UC3);

	for (int y = 0; y < img.rows; y++)
	{
		for (int x = 0; x < img.cols; x++)
		{
			Vec4b& rgba = tempImg.at<Vec4b>(y, x);
			Vec3b& rgb  = img.at<Vec3b>(y, x);

			rgb[0] = rgba[0];
			rgb[1] = rgba[1];
			rgb[2] = rgba[2];
		}
	}

	return true;
}

bool HairUtil::loadRgba(QString filename, cv::Mat& rgbImg, cv::Mat& alphaImg)
{
	QImage qImg(filename);

	if (qImg.isNull())
		return false;

	qImg = qImg.convertToFormat(QImage::Format_ARGB32);

	Mat tempImg(qImg.height(), qImg.width(), CV_8UC4, qImg.bits());

	rgbImg.create(tempImg.size(), CV_8UC3);
	alphaImg.create(tempImg.size(), CV_8UC1);

	for (int y = 0; y < tempImg.rows; y++)
	{
		for (int x = 0; x < tempImg.cols; x++)
		{
			Vec4b& rgba  = tempImg.at<Vec4b>(y, x);
			Vec3b& rgb   = rgbImg.at<Vec3b>(y, x);
			byte&  alpha = alphaImg.at<byte>(y, x);

			rgb[0] = rgba[0];
			rgb[1] = rgba[1];
			rgb[2] = rgba[2];
			alpha  = rgba[3];
		}
	}

	return true;
}

// Generate a random number in [0, maximum)
unsigned int HairUtil::random(unsigned int maximum)
{
	unsigned r;
	rand_s(&r);

	return r % maximum;
}


//float HairUtil::fltNaN()
//{
//	unsigned int raw = 0x7fc00000;
//	return *(float*)&raw;
//}
//
//
//double HairUtil::dblNaN()
//{
//	unsigned long long raw = 0x7ff0000000000000;
//	return *(double*)&raw;
//}


bool HairUtil::isOrientValid(const float orient)
{
	return (orient >= -2.0f*CV_PI) && (orient <= 2.0f*CV_PI);
}

bool HairUtil::isDirectionValid(const cv::Vec2f dir)
{
	return (abs(dir[0]) + abs(dir[1]) > FLT_MIN);
}

bool HairUtil::isTensorValid(const cv::Vec4f tensor)
{
	return (abs(tensor[0]) + abs(tensor[1]) + abs(tensor[3]) > FLT_MIN);
}


// Difference between two orientations
float HairUtil::diffOrient(const float o1, const float o2)
{
	if (isOrientValid(o1) && isOrientValid(o2))
	{
		float diff = o1 - o2;

		return min(abs(diff), min(abs(diff-(float)CV_PI), abs(diff+(float)CV_PI)));
	}
		
	return FLT_MAX;
}

// Conversion between orientation and direction
Vec2f HairUtil::orient2direction(const float orient)
{
	if (isOrientValid(orient))
	{
		return Vec2f(sinf(orient), cosf(orient));
	}
	return InvalidDirection;
}

float HairUtil::direction2orient(const Vec2f& dir)
{
	if (isDirectionValid(dir))
	{
		float angle = atan2f(dir[0], dir[1]);
		return (angle < 0) ? angle + CV_PI : angle;
	}
	return InvalidOrient;
}

// Conversion between orientation and tensor
Vec4f HairUtil::orient2tensor(const float orient)
{
	if (isOrientValid(orient))
	{
		float dx = sinf(orient);
		float dy = cosf(orient);

		return Vec4f(dx*dx, dx*dy, dx*dy, dy*dy);
	}
	return InvalidTensor;
}

float HairUtil::tensor2orient(const Vec4f& tensor)
{
	if (isTensorValid(tensor))
	{
		float eigen;
		float eigenVec[2];

		largestEigen((float*)&tensor, &eigen, eigenVec, 0.01f);

		float angle = atan2f(eigenVec[0], eigenVec[1]);

		return (angle < 0) ? angle + CV_PI : angle;
	}
	return InvalidOrient;
}

// Conversion between tensor and direction
Vec2f HairUtil::tensor2direction(const Vec4f& tensor)
{
	if (isTensorValid(tensor))
	{
		float eigen;
		float eigenVec[2];

		largestEigen((float*)&tensor, &eigen, eigenVec, 0.01f);

		return Vec2f(eigenVec[0], eigenVec[1]);
	}
	return InvalidDirection;
}

Vec4f HairUtil::direction2tensor(const Vec2f& dir)
{
	return Vec4f(dir[0]*dir[0], dir[0]*dir[1], dir[0]*dir[1], dir[1]*dir[1]);
}


// Find the largest eigenvalue and eigenvector of a 2x2 matrix using the Power method
float HairUtil::largestEigen(const float* triMat2, float* eigenval, float* eigenvec2, float epsilon)
{
	const int MAX_ITERATION = 32;

	float A[4] = { triMat2[0], triMat2[1], triMat2[2], triMat2[3] };

	float X[2] = { 1, 0.01 };//{ A[0], A[1] };

	// Initial iteration
	float x0 = A[0]*X[0] + A[1]*X[1];
	float x1 = A[2]*X[0] + A[3]*X[1];

	float c  = sqrtf(x0*x0 + x1*x1);

	X[0] = x0 / c;
	X[1] = x1 / c;

	float err = FLT_MAX;
	for (int i = 0; i < MAX_ITERATION; i++)
	{
		x0 = A[0]*X[0] + A[1]*X[1];
		x1 = A[2]*X[0] + A[3]*X[1];

		float c_new = sqrtf(x0*x0 + x1*x1);

		err = abs((c_new - c) / c_new);
		c   = c_new;

		X[0] = x0 / c;
		X[1] = x1 / c;

		if (err < epsilon)
			break;
	}

	*eigenval = c;
	eigenvec2[0] = X[0];
	eigenvec2[1] = X[1];

	return err;
}


void HairUtil::orient2tensor(const cv::Mat& orientData, cv::Mat& tensorData)
{
	Q_ASSERT(!orientData.empty());
	
	if (tensorData.empty())
		tensorData.create(orientData.size(), CV_32FC4);

	Q_ASSERT(orientData.size() == tensorData.size());

	for (int y = 0; y < tensorData.rows; y++)
	{
		const float* pOrients = orientData.ptr<float>(y);
		Vec4f*		 pTensors = tensorData.ptr<Vec4f>(y);

		for (int x = 0; x < tensorData.cols; x++)
		{
			pTensors[x] = orient2tensor(pOrients[x]);
		}
	}
}


void HairUtil::tensor2orient(const cv::Mat& tensorData, cv::Mat& orientData)
{
	Q_ASSERT(!tensorData.empty());

	if (orientData.empty())
		orientData.create(tensorData.size(), CV_32FC1);

	Q_ASSERT(orientData.size() == tensorData.size());

	for (int y = 0; y < tensorData.rows; y++)
	{
		const Vec4f* pTensors = tensorData.ptr<Vec4f>(y);
		float*		 pOrients = orientData.ptr<float>(y);

		for (int x = 0; x < tensorData.cols; x++)
		{
			pOrients[x] = tensor2orient(pTensors[x]);
		}
	}
}


XMVECTOR HairUtil::randomNormal(const XMVECTOR tangent)
{
	XMVECTOR binormal = XMVectorSet(
		(float)rand() / (float)RAND_MAX - 0.5f,
		(float)rand() / (float)RAND_MAX - 0.5f,
		(float)rand() / (float)RAND_MAX - 0.5f, 1.0f);

	binormal = XMVector3Normalize(binormal);

	XMVECTOR normal = XMVector3Cross(tangent, binormal);

	return XMVector3Normalize(normal);
}


XMVECTOR HairUtil::calcNextNormal(const XMVECTOR tangent, const XMVECTOR newTangent, const XMVECTOR normal)
{
	// Calculate minimum rotation from tangent to newTangent
	XMVECTOR vA = XMVector3Cross(tangent, newTangent);
	vA = XMVector3Normalize(vA);

	XMFLOAT3 tempA;
	XMStoreFloat3(&tempA, vA);

	float sq_x = tempA.x*tempA.x;
	float sq_y = tempA.y*tempA.y;
	float sq_z = tempA.z*tempA.z;
	
	float cos = XMVectorGetX(XMVector3Dot(tangent, newTangent));
	if (cos > 1.0f)
		cos = 1.0f;
	else if (cos < -1.0f)
		cos = -1.0f;

	float cos1 = 1.0f - cos;
	float xycos1 = tempA.x*tempA.y*cos1;
	float yzcos1 = tempA.y*tempA.z*cos1;
	float zxcos1 = tempA.x*tempA.z*cos1;

	float sin = sqrtf(1.0f - cos*cos);
	float xsin = tempA.x*sin;
	float ysin = tempA.y*sin;
	float zsin = tempA.z*sin;

	XMMATRIX mR = XMMatrixSet(
		sq_x + (1.0f-sq_x)*cos, xycos1 + zsin,          zxcos1 - ysin,          0,
		xycos1 - zsin,          sq_y + (1.0f-sq_y)*cos, yzcos1 + xsin,          0,
		zxcos1 + ysin,          yzcos1 - xsin,          sq_z + (1.0f-sq_z)*cos, 0,
		0,                      0,                      0,                      1.0f);

	// Transform previous normal
	XMVECTOR newNormal = XMVector3TransformNormal(normal, mR);

	return XMVector3Normalize(newNormal);
}


bool HairUtil::loadFloatMat(QString filename, cv::Mat& mat)
{
	QFile file(filename);
	if (!file.open(QFile::ReadOnly))
		return false;

	int width = -1, height = -1;

	file.read((char*)&width, sizeof(int));
	file.read((char*)&height, sizeof(int));

	if (width < 1 || height < 1)
		return false;

	mat.create(height, width, CV_32FC1);

	for (int y = 0; y < height; y++)
	{
		file.read((char*)mat.ptr<float>(y), sizeof(float)*width);
	}

	file.close();

	return true;
}


bool HairUtil::saveFloatMat(QString filename, cv::Mat& mat)
{
	QFile file(filename);
	if (!file.open(QFile::WriteOnly))
		return false;

	int width = mat.rows, height = mat.cols;

	if (width < 1 || height < 1)
		return false;

	file.write((char*)&width, sizeof(int));
	file.write((char*)&height, sizeof(int));

	for (int y = 0; y < height; y++)
	{
		file.write((char*)mat.ptr<float>(y), sizeof(float)*width);
	}

	file.close();

	return true;
}


// Calculate signed distance field from binary mask
// mask==0 indicates "interior" where dist<0; 
// mask==255 indicates "exterior" where dist>=0.
void HairUtil::calcSdfFromMask(const Mat& mask, Mat& sdf)
{
	// extract boundary
	Mat boundary = Mat::ones(mask.size(), CV_8UC1)*255;

	for (int y = 1; y < boundary.rows - 1; y++)
	{
		for (int x = 1; x < boundary.cols - 1; x++)
		{
			if (mask.at<byte>(y, x) > 0)
			{
				if (mask.at<byte>(y, x-1) == 0 ||
					mask.at<byte>(y, x+1) == 0 ||
					mask.at<byte>(y-1, x) == 0 ||
					mask.at<byte>(y+1, x) == 0)
				{
					boundary.at<byte>(y, x) = 0;
				}
			}
		}
	}

	// calculate distance field
	distanceTransform(boundary, sdf, CV_DIST_L2, CV_DIST_MASK_PRECISE);

	// calculate signs
	for (int y = 0; y < sdf.rows; y++)
	{
		for (int x = 0; x < sdf.cols; x++)
		{
			if (mask.at<byte>(y, x) == 0)
				sdf.at<float>(y, x) *= -1;
		}
	}
}


// Visualize a signed distance field for debugging
void HairUtil::debugSdf(const QString& winName, const cv::Mat& sdf)
{
	Mat img = Mat::zeros(sdf.size(), CV_8UC3);

	for (int y = 0; y < sdf.rows; y++)
	{
		for (int x = 0; x < sdf.cols; x++)
		{
			const float d = sdf.at<float>(y, x);
			byte clr = (byte)min(255.0f, 64.0f + abs(d)*2.0f);
			if (d >= 0)
				img.at<Vec3b>(y, x)[0] = clr;
			else
				img.at<Vec3b>(y, x)[2] = clr;
		}
	}

	imshow(winName.toStdString(), img);
}


void HairUtil::saveSdfAsRGB(const QString& fileName, const cv::Mat& sdf)
{
	Mat img = Mat::zeros(sdf.size(), CV_8UC3);

	for (int y = 0; y < sdf.rows; y++)
	{
		for (int x = 0; x < sdf.cols; x++)
		{
			const float d = sdf.at<float>(y, x);
			byte clr = (byte)min(255.0f, 64.0f + abs(d)*2.0f);
			if (d >= 0)
				img.at<Vec3b>(y, x)[0] = clr;
			else
				img.at<Vec3b>(y, x)[2] = clr;
		}
	}

	imwrite(fileName.toStdString(), img);
}