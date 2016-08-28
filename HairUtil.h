#pragma once

// Provides static utility functions.

#include "QDXUT.h"

#include <string>

#include <opencv/cv.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <QObject>
#include <QStringList>
#include <QList>
#include <QTime>

class QDXLog : public QObject
{
	Q_OBJECT

public:
	void	msg(QString str);
	void	err(QString str);

signals:
	void	msgAdded(QString);
	void	errAdded(QString);

private:

	enum LogType
	{
		LogMessage,
		LogError,
	};

	struct LogItem
	{
		LogType		type;
		QTime		time;
		QString		str;
	};

	QList<LogItem>	m_logs;
};

class HairUtil
{
public:

	static bool			loadRgbFromRgba(QString filename, cv::Mat& img);
	static bool			loadRgba(QString filename, cv::Mat& rgbImg, cv::Mat& alphaImg);

	static bool			loadFloatMat(QString filename, cv::Mat& mat);
	static bool			saveFloatMat(QString filename, cv::Mat& mat);

	static unsigned int	random(unsigned int maximum);

	static bool			isOrientValid(const float orient);
	static bool			isDirectionValid(const cv::Vec2f dir);
	static bool			isTensorValid(const cv::Vec4f tensor);

	static float		diffOrient(const float o1, const float o2);

	// Conversion between orientation and direction
	static cv::Vec2f	orient2direction(const float orient);
	static float		direction2orient(const cv::Vec2f& dir);

	// Conversion between orientation and tensor
	static cv::Vec4f	orient2tensor(const float orient);
	static float		tensor2orient(const cv::Vec4f& tensor);

	// Conversion between tensor and direction
	static cv::Vec2f	tensor2direction(const cv::Vec4f& tensor);
	static cv::Vec4f	direction2tensor(const cv::Vec2f& dir);

	static void			orient2tensor(const cv::Mat& orientData, cv::Mat& tensorData);
	static void			tensor2orient(const cv::Mat& tensorData, cv::Mat& orientData);

	//static float		fltNaN();
	//static double		dblNaN();

	static const float		InvalidOrient;
	static const cv::Vec2f	InvalidDirection;
	static const cv::Vec4f	InvalidTensor;

	template<typename T>
	static T			sampleLinear(const cv::Mat& src, float x, float y);

	template<typename T>
	static void			shuffleOrder(std::vector<T>& elements);


	static XMVECTOR		randomNormal(const XMVECTOR tangent);

	static XMVECTOR		calcNextNormal(const XMVECTOR tangent, const XMVECTOR newTangent, const XMVECTOR normal);

	// morphing related

	static void			calcSdfFromMask(const cv::Mat& mask, cv::Mat& sdf);

	static void			debugSdf(const QString& winName, const cv::Mat& sdf);
	static void			saveSdfAsRGB(const QString& fileName, const cv::Mat& sdf);

private:

	// Find the largest eigenvalue and eigenvector of a 2x2 matrix using the Power method
	static float		largestEigen(const float* triMat2, float* eigenval, float* eigenvec2, float epsilon = 0.01f);
};


// NOTE: We imagine the image occupying a range from (0.0, 0.0) to ((float)w, (float)h)
// and thus (0.5, 0.5) is the center of pixel (0, 0) and so on.
template<typename T>
T HairUtil::sampleLinear(const cv::Mat& src, float x, float y)
{
	Mat sample;
	getRectSubPix(src, Size(1,1), Point2f(x - 0.5f, y - 0.5f), sample);

	return sample.at<T>(0, 0);
}

template<typename T>
void HairUtil::shuffleOrder(std::vector<T>& elements)
{
	int n = elements.size();

	for (int i = 0; i < n; i++)
	{
		int j = HairUtil::random(n);

		T temp = elements[i];
		elements[i] = elements[j];
		elements[j] = temp;
	}
}