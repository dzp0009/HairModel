#include "HairImage.h"

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/flann/flann.hpp>

#include <math.h>

#include <QProgressDialog>
#include <QPainter>
//#include <QtSvg/QSvgGenerator>
#include <QVector>
#include <QQueue>
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QString>

#include "QDXUT.h"

#include "HairStrandModel.h"
#include "HairUtil.h"


//#include <taucs.h>

using namespace cv;

HairImage::HairImage()
{
}


HairImage::~HairImage()
{
}


///////////////////////////////////////////////////////////////
// Loading and Saving
///////////////////////////////////////////////////////////////


// Load color image from file.
bool HairImage::loadColor(QString filename)
{
	Size oldSize = m_colorData.size();

	//m_colorData = imread(filename.toStdString(), CV_LOAD_IMAGE_COLOR);
	if (!HairUtil::loadRgbFromRgba(filename, m_colorData))
	{
		qCritical() << "OpenCV is unable to read image from file.";
		return false;
	}

	m_colorFilename = filename;

	if (m_colorData.size() != oldSize)
	{
		// Create an empty (all 1) mask
		m_maskData = Mat::ones(m_colorData.size(), CV_8UC1) * 255;

		// Create an empty (all 1) face mask
		m_faceMaskData = Mat::zeros(m_colorData.size(), CV_8UC1);

		// Clear coverage data
		m_coverageData = Mat::zeros(m_colorData.size(), CV_8UC1);

		m_prepData			= Mat();
		m_hairColorData		= Mat();
		m_orientData		= Mat();
		m_confidenceData	= Mat();
		m_varianceData		= Mat();
		m_maxRespData		= Mat();
		m_tensorData		= Mat();
		m_featureData		= Mat();
		m_frontDepthData	= Mat();
		m_midDepthData		= Mat();
		m_backDepthData		= Mat();
		m_traceMaskData		= Mat();
		m_debug1Data		= Mat();
		m_debug2Data		= Mat();

		m_midColorData		= Mat();
		m_midMaskData		= Mat();
	}

	return true;
}


bool HairImage::loadHairColor(QString filename)
{
	//m_hairColorData = imread(filename.toStdString(), CV_LOAD_IMAGE_COLOR);
	//if (!HairUtil::loadRgbFromRgba(filename, m_hairColorData))
	if (!HairUtil::loadRgba(filename, m_hairColorData, m_maskData))
	{
		qCritical() << "Cannot read image from file.";
		return false;
	}

	return true;
}

bool HairImage::loadMidColor(QString filename)
{
	m_midColorData = imread(filename.toStdString(), CV_LOAD_IMAGE_COLOR);

	return (!m_midColorData.empty());
}

bool HairImage::loadMidMask(QString filename)
{
	Mat temp = imread(filename.toStdString(), CV_LOAD_IMAGE_GRAYSCALE);
	if (temp.empty())
		return false;

	temp.convertTo(m_midMaskData, CV_32FC1, 1.0/255.0);

	return true;
}

bool HairImage::loadMask(QString filename)
{
	m_maskData = imread(filename.toStdString(), CV_LOAD_IMAGE_GRAYSCALE);

	if (!m_maskData.data)
	{
		qCritical() << "OpenCV is unable to read image from file.";
		return false;
	}

	return true;
}


bool HairImage::loadFaceMask(QString filename)
{
	m_faceMaskData = imread(filename.toStdString(), CV_LOAD_IMAGE_GRAYSCALE);

	if (!m_faceMaskData.data)
	{
		qCritical() << "Unable to load face mask image.";
		return false;
	}

	return true;
}


bool HairImage::loadFaceMaskFromPts(QString filename)
{
	Q_ASSERT(m_colorData.data != NULL);

	QFile file(filename);

	if (!file.open(QIODevice::ReadOnly))
		return false;

	char buffer[256];
	file.readLine(buffer, 256);

	int numPts;
	sscanf_s(buffer, "%d\n", &numPts);

	Point* pPoints = new Point[numPts];

	for (int i = 0; i < numPts; i++)
	{
		file.readLine(buffer, 256);

		float x = 0, y = 0;
		sscanf_s(buffer, "%f %f\n", &x, &y);

		pPoints[i].x = x;
		pPoints[i].y = y;
	}

	// Draw polygon
	m_faceMaskData = Mat::zeros(m_colorData.size(), CV_8UC1);

	fillConvexPoly(m_faceMaskData, pPoints, numPts, Scalar(255));

	m_debug2Data = m_faceMaskData.clone();

	return true;
}


// Load orientation data according to Menglei's format:
// 0 ~ 255 is uniformly mapped to Pi ~ 0
bool HairImage::loadOrientationChai(QString filename)
{
	Mat temp = imread(filename.toStdString(), CV_LOAD_IMAGE_GRAYSCALE);

	if (!temp.data)
	{
		qCritical() << "OpenCV is unable to read orientation image from file.";
		return false;
	}

	temp.convertTo(m_orientData, CV_32FC1, -CV_PI / 255.0, CV_PI);

	return true;
}


// Load orientation data from Lvdi's format
bool HairImage::loadOrientationWang(QString filename)
{
	Mat orientData = cv::imread(filename.toStdString(), CV_LOAD_IMAGE_COLOR);

	// Normalize
	orientData.convertTo(orientData, CV_32FC3, 1/255.0);

	m_orientData = Mat(orientData.size(), CV_32FC1);
	m_debug1Data  = Mat(orientData.size(), CV_32FC1);
	Mat outChannels[] = { m_orientData, m_debug1Data };
	int fromTo[] = {0, 0, 1, 1};
	mixChannels(&orientData, 1, outChannels, 2, fromTo, 2);

	m_orientData *= CV_PI;
	m_debug1Data.convertTo(m_debug1Data, CV_8UC1, 255.0f);

	if (m_colorData.size() != m_orientData.size())
	{
		m_colorData = Mat(m_orientData.size(), CV_8UC3);
	}

	return true;
}


bool HairImage::saveStrandsSVG(QString filename)
{
	//QSvgGenerator svg;
	//svg.setFileName(filename);
	//svg.setSize(QSize(width(), height()));
	//svg.setViewBox(QRect(0, 0, width(), height()));
	//svg.setTitle("Hair strands");

	////dstImage->fill(0);

	//QPainter painter;
	//painter.begin(&svg);
	////painter.setRenderHint(QPainter::Antialiasing, true);

	//int ptBuffSize = 1000;
	//QPointF* points = new QPointF[ptBuffSize];
	//for (int i = 0; i < m_strands.count(); i++)
	//{
	//	int nVert = m_strands[i].numVertices();

	//	if (nVert > ptBuffSize)
	//	{
	//		delete[] points;
	//		points = new QPointF[nVert];
	//		ptBuffSize = nVert;
	//	}

	//	for (int j = 0; j < nVert; j++)
	//	{
	//		points[j].setX(m_strands[i].getVertex(j)->x);
	//		points[j].setY(m_strands[i].getVertex(j)->y);
	//	}
	//	painter.setPen(QPen(Qt::black, 1));
	//	painter.drawPolyline(points, nVert);

	//	QPointF initPt(m_strands[i].getInitVertex()->x,
	//		m_strands[i].getInitVertex()->y);
	//	painter.setPen(QPen(Qt::red, 2));
	//	painter.drawPoint(initPt);
	//}

	//painter.end();

	//delete[] points;

	//return true;
	return false;
}


void HairImage::clearCoverage()
{
	if (m_colorData.data != NULL)
	{
		m_coverageData = Mat::zeros(m_colorData.size(), CV_8UC1);
	}
}

///////////////////////////////////////////////////////////////
// Display content on UI
///////////////////////////////////////////////////////////////


void HairImage::drawImage(const HairImageViewParam& params, QImage* dstImage) const
{
	Q_ASSERT(dstImage != NULL);

	switch (params.viewMode)
	{
	case HairImageColor:
		drawColor(params, dstImage);
		break;

	case HairImageOrient:
		drawOrientation(params, dstImage);
		break;

	case HairImageConfidence:
		drawConfidence(params, dstImage);
		break;

	case HairImageCoverage:
		draw8UDataToQImage(m_coverageData, dstImage);
		break;

	case HairImageDepth:
		drawDepth(params, dstImage);
		break;

	case HairImageDebug1:
		draw8UDataToQImage(m_debug1Data, dstImage);
		break;

	case HairImageDebug2:
		draw8UDataToQImage(m_debug2Data, dstImage);
		break;
	}
}


void HairImage::drawColor(const HairImageViewParam& params, QImage* dstImage) const
{
	if ((params.toggleOrigPrep  && m_colorData.empty()) ||
		(!params.toggleOrigPrep && m_prepData.empty()))
	{
		dstImage->fill(0);
		return;
	}

	Q_ASSERT(dstImage->width() == m_colorData.cols);
	Q_ASSERT(dstImage->height() == m_colorData.rows);

	QVector<Mat> input;
	QVector<int> fromTo;

	if (params.toggleOrigPrep)
	{
		// Show original image
		input.append(m_colorData);
		fromTo.append(0); fromTo.append(0);
		fromTo.append(1); fromTo.append(1);
		fromTo.append(2); fromTo.append(2);
	}
	else
	{
		// Show preprocessed image
		input.append(m_prepData);
		fromTo.append(0); fromTo.append(0);
		fromTo.append(0); fromTo.append(1);
		fromTo.append(0); fromTo.append(2);
	}

	if (params.showMaskOpacity)
	{
		input.append(m_maskData);
	}
	else
	{
		input.append(Mat::ones(m_colorData.size(), CV_8UC1) * 255);
	}
	fromTo.append(input[0].channels());
	fromTo.append(3);

	Mat dstData(m_colorData.size(), CV_8UC4, dstImage->bits());
	mixChannels(input.data(), 2, &dstData, 1, fromTo.data(), 4);
}



void HairImage::drawOrientation(const HairImageViewParam& params, QImage* dstImage) const
{
	if ((params.toggleOrientTensor  && m_orientData.empty()) ||
		(!params.toggleOrientTensor && m_tensorData.empty()))
	{
		dstImage->fill(0);
		return;
	}

	Q_ASSERT(dstImage->width() == m_orientData.cols);
	Q_ASSERT(dstImage->height() == m_orientData.rows);

	Mat tempImg(m_orientData.size(), CV_32FC3);
	
	if (params.toggleOrientTensor)
	{
		// Show orientation map
		// We first convert orientation to HLS...
		for (int y = 0; y < tempImg.rows; y++)
		{
			Vec3f* pHls = tempImg.ptr<Vec3f>(y);
			const float* pOrient  = m_orientData.ptr<float>(y);

			for (int x = 0; x < tempImg.cols; x++)
			{
				if (HairUtil::isOrientValid(pOrient[x]))
				{
					pHls[x][0] = 2.0f * pOrient[x] * 180.0f / 3.1415926f;
					pHls[x][1] = 0.5f;
					pHls[x][2] = 1.0f;
				}
				else
				{
					// Invalid orientation...
					pHls[x][0] = pHls[x][1] = pHls[x][2] = 0.0f;
				}
			}
		}

		// Then convert HLS to BGR...
		cvtColor(tempImg, tempImg, CV_HLS2RGB);
	}
	else
	{
		// Show tensor map
		int fromTo[] = { 0,0, 1,1, 3,2 };
		
		mixChannels(&m_tensorData, 1, &tempImg, 1, fromTo, 3);
	}

	// Prepare alpha channel
	Mat tempAlpha = Mat::ones(m_orientData.size(), CV_32FC1);

	if (params.showMaskOpacity)
	{
		m_maskData.convertTo(tempAlpha, CV_32FC1, 1.0/255.0);
	}

	if (params.showMaxRespOpacity && m_confidenceData.data)
	{
		tempAlpha = tempAlpha.mul(m_confidenceData);
	}

	// Combine RGBA channels
	tempImg.convertTo(tempImg, CV_8UC3, 255.0);
	tempAlpha.convertTo(tempAlpha, CV_8UC1, 255.0);

	Mat input[]  = { tempImg, tempAlpha };
	int fromTo[] = { 0,0, 1,1, 2,2, 3,3 };
	
	Mat dstData(m_orientData.size(), CV_8UC4, dstImage->bits());
	mixChannels(input, 2, &dstData, 1, fromTo, 4);
}


void HairImage::drawConfidence(const HairImageViewParam& params, QImage* dstImage) const
{
	if (m_confidenceData.empty())
	{
		dstImage->fill(0);
		return;
	}

	Q_ASSERT(m_confidenceData.cols == dstImage->width());
	Q_ASSERT(m_confidenceData.rows == dstImage->height());

	Mat temp;
	m_confidenceData.convertTo(temp, CV_8UC1, 255.0);
	cvtColor(temp, temp, CV_GRAY2RGB);

	Mat dstData(m_confidenceData.size(), CV_8UC4, dstImage->bits());
	cvtColor(temp, dstData, CV_RGB2RGBA);
}


void HairImage::drawDepth(const HairImageViewParam& params, QImage* dstImage) const
{
	dstImage->fill(0);
}

///////////////////////////////////////////////////////////////
// Image processing
///////////////////////////////////////////////////////////////

// Preprocessing
void HairImage::preprocessImage(const PreprocessParam& params, 
	const cv::Mat& src, cv::Mat& dst)
{
	// Convert to grayscale
	Mat gray;
	cvtColor(src, gray, CV_RGB2GRAY);

	if (params.preprocessMethod == PreprocessNone)
	{
		dst = gray.clone();
	}
	else if (params.preprocessMethod == PreprocessGaussian)
	{
		GaussianBlur(gray, dst, Size(0,0), params.sigmaHigh);
	}
	else if (params.preprocessMethod == PreprocessDoG)
	{
		gray.convertTo(gray, CV_32FC1);

		Mat allFreq, lowFreq;
		GaussianBlur(gray, allFreq, Size(0,0), params.sigmaHigh);
		GaussianBlur(gray, lowFreq, Size(0,0), params.sigmaLow);
		dst = allFreq - lowFreq;

		dst.convertTo(dst, CV_8U, 1, 127.5);
	}
	else if (params.preprocessMethod == PreprocessBilateral)
	{
		QProgressDialog progressDlg;
		progressDlg.setLabelText("Performing bilateral filtering...");
		progressDlg.setRange(0, 2);
		progressDlg.setModal(true);
		progressDlg.show();
		progressDlg.setValue(1);

		bilateralFilter(gray, dst, 0, params.sigmaLow, params.sigmaHigh);
	}
}


void HairImage::calcOrientation(const PreprocessParam& prepParams, 
								const OrientationParam& orientParams)
{
	Q_ASSERT(m_colorData.data != NULL);

	preprocessImage(prepParams, m_colorData, m_prepData);

	if (orientParams.method == OrientationCanny)
	{
		calcOrientationCanny(orientParams, m_prepData);
	}
	else if (orientParams.method == OrientationGabor)
	{
		calcOrientationGabor(orientParams, m_prepData, m_orientData);
	}

	HairUtil::orient2tensor(m_orientData, m_tensorData);
}


void HairImage::refineOrientation(const OrientationParam& orientParams)
{
	refineOrientation(orientParams, m_orientData);
}

void HairImage::refineOrientation(const OrientationParam& orientParams, Mat& dst)
{
	Q_ASSERT(m_confidenceData.data != NULL);

	calcOrientationGabor(orientParams, m_confidenceData, dst);

	HairUtil::orient2tensor(dst, m_tensorData);
}


void HairImage::calcOrientationCanny(const OrientationParam& orientParams, 
	const cv::Mat& src)
{

}


// Estimate orientation at a single pixel using single Gabor kernel (for DEBUGGING)
void HairImage::calcOrientationAt(int x, int y, 
								  const PreprocessParam& prepParams, 
								  const OrientationParam& orientParams, 
								  float* pOrient, float* pResponseArray)
{
	Q_ASSERT(m_colorData.data != NULL);
	Q_ASSERT(pOrient);
	Q_ASSERT(pResponseArray);

	if (m_prepData.empty())
		preprocessImage(prepParams, m_colorData, m_prepData);

	int kRadiusX = orientParams.kernelWidth / 2;
	int kRadiusY = orientParams.kernelHeight / 2;

	Range rowSpan(y - kRadiusY, y + kRadiusY + 1);
	Range colSpan(x - kRadiusX, x + kRadiusX + 1);

	Mat roi = m_prepData.rowRange(rowSpan).colRange(colSpan);
	roi.convertTo(roi, CV_32FC1);

	Q_ASSERT(roi.cols == orientParams.kernelWidth);
	Q_ASSERT(roi.rows == orientParams.kernelHeight);

	Mat kernel;
	
	float maxResponse = 0.0f;
	float bestOrient  = 0.0f;
	for (int ch = 0; ch < orientParams.numKernels; ch++)
	{
		// Compute oriented kernel
		float theta = CV_PI * (float)ch / (float)orientParams.numKernels;

		// Cosine Gabor (positive)
		createGaborKernel(orientParams.kernelWidth, orientParams.kernelHeight, theta,
			orientParams.sigmaX, orientParams.sigmaY, orientParams.lambda,
			orientParams.phase, kernel);

		Mat conv = roi.mul(kernel);
		
		float resp = max(sum(conv)[0], 0.0);

		if (resp > maxResponse)
		{
			maxResponse = resp;
			bestOrient = theta;
		}

		pResponseArray[ch] = resp;
	}
	*pOrient = bestOrient;
}


//
void HairImage::calcOrientationGabor(const OrientationParam& params, 
	const cv::Mat& src, cv::Mat& dst)
{
	Q_ASSERT(!src.empty());

	QProgressDialog progressDlg;
	progressDlg.setLabelText("Performing oriented filtering...");
	progressDlg.setRange(0, params.numKernels);
	progressDlg.setModal(true);
	progressDlg.show();


	float phase		= params.phase;
	float phaseStep = CV_PI * 2.0 / (float)params.numPhases;

	dst				= Mat::zeros(src.size(), CV_32F);
	m_varianceData	= Mat::zeros(src.size(), CV_32F);
	m_maxRespData	= Mat::zeros(src.size(), CV_32F);

	Mat kernelImage;

	// Try Gabor kernel of each phase
	for (int iPhase = 0; iPhase < params.numPhases; iPhase++)
	{
		// Array of responses to each orientation
		std::vector<Mat> respArray;

		// Try Gabor kernel of each orientation
		for (int iOrient = 0; iOrient < params.numKernels; iOrient++)
		{
			// Create Gabor kernel
			Mat kernel;
			const float theta = CV_PI * (float)iOrient / (float)params.numKernels;
			createGaborKernel(params.kernelWidth, params.kernelHeight, theta,
							  params.sigmaX, params.sigmaY, params.lambda,
							  phase, kernel);

			// Filter and store response
			Mat response;
			filter2D(src, response, CV_32F, kernel);

			respArray.push_back(response.clone());

			// Update progress bar
			progressDlg.setValue(iOrient);

			kernelImage.push_back(kernel);
		}

		// Statistics: calc per-pixel variance...
		for (int y = 0; y < src.rows; y++)
		{
			for (int x = 0; x < src.cols; x++)
			{
				// Find max response at the pixel
				float maxResp    = 0.0f;
				float bestOrient = 0.0f;

				for (int iOrient = 0; iOrient < params.numKernels; iOrient++)
				{
					float& resp = respArray[iOrient].ptr<float>(y)[x];

					if (resp < 0.0f)
					{
						resp = 0.0f;
					}
					else if (resp > maxResp)
					{
						maxResp = resp;
						bestOrient = CV_PI * (float)iOrient / (float)params.numKernels;
					}
				}

				// Calculate variance
				float variance = 0.0f;
				for (int iOrient = 0; iOrient < params.numKernels; iOrient++)
				{
					float orient = CV_PI * (float)iOrient / (float)params.numKernels;
					float orientDiff = HairUtil::diffOrient(orient, bestOrient);

					float respDiff = respArray[iOrient].ptr<float>(y)[x] - maxResp;

					variance += orientDiff * respDiff * respDiff;
				}
				// Standard variance
				variance = sqrtf(variance);

				// Update overall variance/orientation if necessary
				if (variance > m_varianceData.ptr<float>(y)[x])
				{
					dst.ptr<float>(y)[x]   = bestOrient;
					m_varianceData.ptr<float>(y)[x] = variance;
					m_maxRespData.ptr<float>(y)[x]	= maxResp;
				}
			}
		}

		phase += phaseStep;
	}

	// Normalize variance and max response
	float maxAllResponse = 0.0f;
	float maxAllVariance = 0.0f;

	for (int y = 0; y < src.rows; y++)
	{
		for (int x = 0; x < src.cols; x++)
		{
			if (m_varianceData.ptr<float>(y)[x] > maxAllVariance)
				maxAllVariance = m_varianceData.ptr<float>(y)[x];

			if (m_maxRespData.ptr<float>(y)[x] > maxAllResponse)
				maxAllResponse = m_maxRespData.ptr<float>(y)[x];
		}
	}

	qDebug() << "Max response:" << maxAllResponse;
	qDebug() << "Max variance:" << maxAllVariance;

	m_maxRespData /= maxAllResponse;
	m_maxRespData.convertTo(m_debug1Data, CV_8U, 255.0f);

	m_varianceData /= maxAllVariance;
	m_varianceData.convertTo(m_debug2Data, CV_8U, 255.0f);

	// Save kernel image to file for debugging
	kernelImage.convertTo(kernelImage, CV_8U, 127.5f, 127.5f);
	imwrite("kernels.png", kernelImage);
}

void HairImage::calcConfidence(const ConfidenceParam& params)
{
	// Save confidence map
	if (params.confidMethod == ConfidenceMyVariance)
	{
		Q_ASSERT(m_varianceData.data);
		m_confidenceData = m_varianceData.clone();
	}
	else if (params.confidMethod == ConfidenceMaxResponse)
	{
		Q_ASSERT(m_maxRespData.data);
		m_confidenceData = m_maxRespData.clone();
	}

	// Clamp confidence
	for (int y = 0; y < m_confidenceData.rows; y++)
	{
		for (int x = 0; x < m_confidenceData.cols; x++)
		{
			float& c = m_confidenceData.at<float>(y, x);

			c = (c - params.clampConfidLow) / (params.clampConfidHigh - params.clampConfidLow);
			if (c != c)
				c = 0.0f;
			else if (c < 0.0f)
				c = 0.0f;
			else if (c > 1.0f)
				c = 1.0f;
		}
	}
}


void HairImage::smoothOrientation(const SmoothingParam& params)
{
	switch (params.method)
	{
	case SmoothingMultilateral:
		smoothOrientationMultilateral(params);
		break;

	case SmoothingConfidDiffuse:
		smoothOrientationConfidDiffuse(params);
		break;

	case SmoothingBlur:
		smoothOrientationBlur(params, m_orientData, m_orientData);
		break;
	}
}

// Perform orientation field smoothing
// This is based on a "multilateral" filtering of the underlying
// tensor map.
void HairImage::smoothOrientationMultilateral(const SmoothingParam& params)
{
	Q_ASSERT(!m_tensorData.empty());
	Q_ASSERT(!m_varianceData.empty());

	QProgressDialog progressDlg;
	progressDlg.setLabelText("Performing orientation smoothing...");
	progressDlg.setRange(0, m_tensorData.rows - 1);
	progressDlg.setModal(true);
	progressDlg.show();

	// Prepare Gaussian kernels
	Mat_<float> kSpatial = createGaussianKernel(params.sigmaSpatial);
	Mat_<float> kOrient  = createGaussianKernel(params.sigmaOrientation);
	Mat_<float> kColor	 = createGaussianKernel(params.sigmaColor);
	Mat_<float> kConfid	 = createGaussianKernel(params.sigmaConfidence);
	
	const int kRadius = kSpatial.cols / 2;

	Mat smoothData(m_tensorData.size(), CV_32FC4);

	// Trilateral filtering on tensor map
	for (int y = 0; y < m_tensorData.rows; y++)
	{
		for (int x = 0; x < m_tensorData.cols; x++)
		{
			Vec4f sumTensor(0, 0, 0, 0);
			float sumWeight = 0;

			//int   confid = m_debug1Data.ptr<uchar>(y)[x];
			float orient  = m_orientData.ptr<float>(y)[x];
			float var	  = m_varianceData.ptr<float>(y)[x];
			float maxResp = (float)m_debug1Data.ptr<uchar>(y)[x] / 255.0f;

			// Convolve
			for (int i = -kRadius; i <= kRadius; i++)
			{
				const int ky = y + i;
				if (ky < 0 || ky >= m_tensorData.rows)
					continue;

				// Spatial weight along Y
				const float wY = kSpatial(i+kRadius);

				for (int j = -kRadius; j <= kRadius; j++)
				{
					const int kx = x + j;
					if (kx < 0 || kx >= m_tensorData.cols)
						continue;

					float weight = 1.0f;

					if (params.useSpatial)
					{
						weight *= kSpatial(j+kRadius);	// wX
						weight *= wY;
					}
					if (params.useConfidence)
					{
						float varRatio = m_varianceData.ptr<float>(ky)[kx] / var;
						weight *= expf(-varRatio / 
									   (2.0f*params.sigmaConfidence*params.sigmaConfidence));
					}
					if (params.useOrientation)
					{
						if (params.useMaxRespClamp == false || maxResp > params.maxRespClampThreshold)
						//if (params.useVarClamp == false || var < params.varClampThreshold)
						/*if (var < params.varClampThreshold &&
							m_varianceData.ptr<float>(ky)[kx] < params.varClampThreshold)*/
						{
							float diff = HairUtil::diffOrient(orient, m_orientData.ptr<float>(ky)[kx]);
							float sigmaRad = params.sigmaOrientation * CV_PI / 180.0f;
							weight *= expf(-diff*diff / (2.0f*sigmaRad*sigmaRad));
						}
					}
					if (params.useColor)
					{
						// TODO
					}

					sumTensor += weight * m_tensorData.ptr<Vec4f>(ky)[kx];
					sumWeight += weight;
				}
			}

			// Normalize
			smoothData.ptr<Vec4f>(y)[x] = sumTensor * (1.0f/sumWeight);
		} // for x

		progressDlg.setValue(y);
	} // for y

	m_tensorData = smoothData.clone();

	HairUtil::tensor2orient(m_tensorData, m_orientData);
}


// Diffuse orientation from high-confident areas to low-confident areas
void HairImage::smoothOrientationConfidDiffuse(const SmoothingParam& params)
{
	Q_ASSERT(!m_tensorData.empty());
	Q_ASSERT(!m_confidenceData.empty());

	QProgressDialog progressDlg;
	progressDlg.setLabelText("Performing orientation smoothing...");
	progressDlg.setRange(0, m_tensorData.rows);
	progressDlg.setModal(true);
	progressDlg.show();

	// Create a diffusion weight map by clamping current confidence map
	Mat confidData = m_confidenceData.clone();
	for (int y = 0; y < confidData.rows; y++)
	{
		for (int x = 0; x < confidData.cols; x++)
		{
			float& c = confidData.at<float>(y, x);
			if (c < params.confidThreshold)
				c = 0.0f;
		}
	}

	// Prepare Gaussian kernel
	Mat_<float> kSpatial = createGaussianKernel(params.sigmaSpatial);

	const int kRadius = kSpatial.cols / 2;

	qDebug() << "Kernel size:" << kSpatial.cols;

	Mat smoothTensorData;
	Mat smoothConfidData;

	for (int iter = 0; iter < params.diffuseIterations; iter++)
	{
		smoothTensorData = Mat::zeros(m_tensorData.size(), CV_32FC4);
		smoothConfidData = Mat::zeros(m_tensorData.size(), CV_32FC1);

		// Diffuse by convolution
		for (int y = 0; y < m_tensorData.rows; y++)
		{
			for (int x = 0; x < m_tensorData.cols; x++)
			{
				//float& confid = confidData.at<float>(y, x);

				// Keep high-confident pixels untouched
				if (m_confidenceData.at<float>(y, x) > params.confidThreshold)
				{
					smoothTensorData.at<Vec4f>(y, x) = m_tensorData.at<Vec4f>(y, x);
					smoothConfidData.at<float>(y, x) = confidData.at<float>(y, x);
					continue;
				}

				Vec4f sumTensor = Vec4f(0, 0, 0, 0);
				float sumWeightTensor = 0.0f;

				float sumConfid = 0.0f;
				float sumWeightConfid = 0.0f;

				// Convolve
				for (int i = -kRadius; i <= kRadius; i++)
				{
					const int ky = y + i;
					if (ky < 0 || ky >= m_tensorData.rows)
						continue;

					// Spatial weight along Y
					const float wY = kSpatial(i+kRadius);

					for (int j = -kRadius; j <= kRadius; j++)
					{
						const int kx = x + j;
						if (kx < 0 || kx >= m_tensorData.cols)
							continue;

						// Spatial weight
						float wSpatial = wY * kSpatial(j+kRadius);

						// Confidence weight
						float wConfid  = confidData.at<float>(ky, kx);

						sumTensor += m_tensorData.at<Vec4f>(ky, kx) * wSpatial * wConfid;
						sumWeightTensor += wSpatial * wConfid;

						sumConfid += wConfid * wSpatial;
						sumWeightConfid += wSpatial;
					}
				}

				// Normalize
				if (sumWeightTensor > FLT_MIN)
				{
					smoothTensorData.at<Vec4f>(y, x) = sumTensor * (1.0f/sumWeightTensor);
					smoothConfidData.at<float>(y, x) = sumConfid / sumWeightConfid;
				}
			}

			progressDlg.setValue(y);
		}

		m_tensorData = smoothTensorData.clone();
		confidData = smoothConfidData.clone();
	}
	
	smoothConfidData.convertTo(m_debug1Data, CV_8UC1, 255.0f);

	HairUtil::tensor2orient(m_tensorData, m_orientData);
}


void HairImage::smoothOrientationBlur(const SmoothingParam& params, const cv::Mat& srcOrient, cv::Mat& dstOrient)
{
	Q_ASSERT(!srcOrient.empty());
	Q_ASSERT(!m_confidenceData.empty());

	QProgressDialog progressDlg;
	progressDlg.setLabelText("Performing orientation smoothing...");
	progressDlg.setRange(0, params.diffuseIterations);
	progressDlg.setModal(true);
	progressDlg.show();

	Mat smoothTensorData = Mat::zeros(srcOrient.size(), CV_32FC4);

	for (int y = 0; y < srcOrient.rows; y++)
	{
		for (int x = 0; x < srcOrient.cols; x++)
		{
			float& c = m_confidenceData.at<float>(y, x);
			if (c > params.confidThreshold)
			{
				smoothTensorData.at<Vec4f>(y, x) = HairUtil::orient2tensor(
					srcOrient.at<float>(y, x)) * c;
			}
		}
	}

	for (int iter = 0; iter < params.diffuseIterations; iter++)
	{
		progressDlg.setValue(iter);

		GaussianBlur(smoothTensorData, smoothTensorData, Size(-1, -1), params.sigmaSpatial);

		for (int y = 0; y < srcOrient.rows; y++)
		{
			for (int x = 0; x < srcOrient.cols; x++)
			{
				float& c = m_confidenceData.at<float>(y, x);
				if (c > params.confidThreshold)
				{
					smoothTensorData.at<Vec4f>(y, x) = HairUtil::orient2tensor(
						srcOrient.at<float>(y, x)) * c;
				}
			}
		}
	}

	// Probably will cause problem
	//m_tensorData = smoothTensorData.clone();

	HairUtil::tensor2orient(smoothTensorData, dstOrient);
}


// Detect line features using Jakob's method.
void HairImage::detectLineFeatures(const FeatureDetectParam& params)
{
	Q_ASSERT(m_prepData.data != NULL);
	Q_ASSERT(m_orientData.data != NULL);

	Mat_<float>& src = (Mat_<float>&)m_confidenceData;

	Mat temp = Mat::zeros(src.size(), CV_32FC1);

	float minM = FLT_MAX;
	float maxM = 0.0f;

	for (int y = 1; y < m_prepData.rows - 1; y++)
	{
		for (int x = 1; x < m_prepData.cols - 1; x++)
		{
			float m = src(y, x);

			// Clamp pixels with too low variance
			float varLevel;
			if (m > params.highConfidence)
				varLevel = 1.0f;
			else if (m > params.lowConfidence)
				varLevel = 0.25f;
			else
				continue;

			float m_max = calcJakobM(x, y, params.neighborOffset, src);

			if (m > m_max &&
				(m - m_max) / m > params.ridgeDiffThreshold)
				temp.ptr<float>(y)[x] = varLevel;
		}
	}

	temp.convertTo(m_debug2Data, CV_8U, 255.0);

	m_featureData = m_debug2Data.clone();
}


// According to [Jakob2009], calculate maximum intensity of the two
// neighboring locations orthogonal to the local orientation of the
// given pixel.
template<typename T>
T HairImage::calcJakobM(int x, int y, float offset, const Mat_<T>& src)
{
	const float orient = m_orientData.ptr<float>(y)[x];

	Vec2f dir = HairUtil::orient2direction(orient + CV_PI * 0.5f);

	float x1 = (float)x + offset * dir[0];
	float y1 = (float)y + offset * dir[1];
	float x2 = (float)x - offset * dir[0];
	float y2 = (float)y - offset * dir[1];

	return max(HairUtil::sampleLinear<T>(src, x1, y1), 
			   HairUtil::sampleLinear<T>(src, x2, y2));
}

//
//template<typename T>
//T HairImage::sampleLinear(float x, float y, const Mat_<T>& src)
//{
//	int nx0 = int(x);
//	int ny0 = int(y);
//	int nx1 = nx0 + 1;
//	int ny1 = ny0 + 1;
//
//	if (nx0 < 0) nx0 = 0;
//	else if (nx0 >= src.cols) nx0 = src.cols - 1;
//
//	if (ny0 < 0) ny0 = 0;
//	else if (ny0 >= src.rows) ny0 = src.rows - 1;
//
//	if (nx1 < 0) nx1 = 0;
//	else if (nx1 >= src.cols) nx1 = src.cols - 1;
//
//	if (ny1 < 0) ny1 = 0;
//	else if (ny1 >= src.rows) ny1 = src.rows - 1;
//
//	float u = x - (float)nx0;
//	float v = y - (float)ny0;
//
//	if (u < 0.0f) u = 0.0f;
//	else if (u > 1.0f) u = 1.0f;
//
//	if (v < 0.0f) v = 0.0f;
//	else if (v > 1.0f) v = 1.0f;
//		
//	T v00 = src(ny0, nx0);
//	T v10 = src(ny0, nx1);
//	T v01 = src(ny1, nx0);
//	T v11 = src(ny1, nx1);
//
//	T vx0 = v00*(1 - u) + v10*u;
//	T vx1 = v01*(1 - u) + v11*u;
//
//	return vx0*(1 - v) + vx1*v;
//}
//
//// NOTE: only works for CV_8UC1
//float HairImage::sample8ULinear(float x, float y, const cv::Mat& src)
//{
//	if (x < 0) x = 0;
//	else if (x >= src.cols - 1) x = float(src.cols - 1) - 0.00001f;
//
//	if (y < 0) y = 0;
//	else if (y >= src.rows - 1) y = float(src.rows - 1) - 0.00001f;
//
//	int nx0 = int(x);
//	int ny0 = int(y);
//	int nx1 = nx0 + 1;
//	int ny1 = ny0 + 1;
//
//	float u = x - (float)nx0;
//	float v = y - (float)ny0;
//
//	float v00 = src.ptr<uchar>(ny0)[nx0];
//	float v10 = src.ptr<uchar>(ny0)[nx1];
//	float v01 = src.ptr<uchar>(ny1)[nx0];
//	float v11 = src.ptr<uchar>(ny1)[nx1];
//
//	float vx0 = v00*(1 - u) + v10*u;
//	float vx1 = v01*(1 - u) + v11*u;
//
//	return vx0*(1 - v) + vx1*v;
//}


///////////////////////////////////////////////////////////////
// Geometry generation
///////////////////////////////////////////////////////////////


void HairImage::traceStrandsOld(const TracingParam& params, HairStrandModel* pStrandModel)
{
	Q_ASSERT(m_maskData.data != NULL);
	Q_ASSERT(m_orientData.data != NULL);
	Q_ASSERT(m_tensorData.data != NULL);
	Q_ASSERT(pStrandModel);

	pStrandModel->clear();

	// Generate initial seeds
	const int initNumSeeds = int(float(width()*height()) * params.seedDensity);
	qDebug() << "Initial seeds:" << initNumSeeds;

	Mat seeds(initNumSeeds, 2, CV_32F);
	srand(19840428);
	for (int i = 0; i < initNumSeeds; i++)
	{
		float x_int  = (float)(rand() % width());
		float x_frac = (float)rand() / (float)RAND_MAX;
		float y_int  = (float)(rand() % height());
		float y_frac = (float)rand() / (float)RAND_MAX;

		seeds.ptr<float>(i)[0] = x_int + x_frac;
		seeds.ptr<float>(i)[1] = y_int + y_frac;
	}

	// Relaxation
	for (int i = 0; i < params.numRelaxations; i++)
		relaxSeeds(seeds, params.seedDensity, params.relaxForce);

	// Pick only valid seeds and trace strand
	for (int i = 0; i < initNumSeeds; i++)
	{
		float x = seeds.ptr<float>(i)[0];
		float y = seeds.ptr<float>(i)[1];

		if (isInsideHairRegion(x, y))
		{
			// Valid seed
			Strand strand;
			traceSingleStrandOld(x, y, params, &strand);

			if (strand.length() > params.minLength)
				pStrandModel->addStrand(strand);
		}
	}

	pStrandModel->updateBuffers();
}


void HairImage::traceSingleStrandOld(float x, float y, const TracingParam& params, Strand* pStrand)
{
	Q_ASSERT(m_tensorData.data != NULL);
	Q_ASSERT(pStrand);

	pStrand->clear();

	// Add initial vertex
	StrandVertex vertex;
	vertex.position = XMFLOAT3(x, y, 0);
	//vertex.color	= XMFLOAT4(1, 0, 0, 1);
	getColorAt(x, y, (float*)(&(vertex.color)));

	pStrand->appendVertex(vertex);

	// Calculate tracing directions
	Vec2f dirForward  = HairUtil::orient2direction(getOrientationAt(x, y));
	Vec2f dirBackward = -dirForward;

	bool bTraceForward  = true;
	bool bTraceBackward = true;

	// Start tracing in opposite directions
	while (pStrand->length() < params.maxLength && (bTraceForward || bTraceBackward))
	{
		// Forward tracing
		if (bTraceForward)
		{
			vertex = pStrand->vertices()[pStrand->numVertices() - 1];
			
			vertex.position.x += params.stepLength * dirForward[0];
			vertex.position.y += params.stepLength * dirForward[1];

			if (isInsideHairRegion(vertex.position.x, vertex.position.y))
			{
				getColorAt(vertex.position.x, vertex.position.y, (float*)(&(vertex.color)));

				pStrand->appendVertex(vertex, params.stepLength);

				// Calculate new direction
				Vec2f newDir = HairUtil::orient2direction(getOrientationAt(vertex.position.x, vertex.position.y));

				float cosTheta = newDir.dot(dirForward);
				if (cosTheta < 0.0f)
				{
					newDir = -newDir;
					cosTheta = newDir.dot(dirForward);
				}
				dirForward = newDir;

				// Check max bend angle
				if (acosf(cosTheta) > params.maxBendAngle)
					bTraceForward = false;
			}
			else
			{
				bTraceForward = false;
			}
		}

		// Backward tracing
		if (bTraceBackward)
		{
			vertex = pStrand->vertices()[0];
			
			vertex.position.x += params.stepLength * dirBackward[0];
			vertex.position.y += params.stepLength * dirBackward[1];

			if (isInsideHairRegion(vertex.position.x, vertex.position.y))
			{
				getColorAt(vertex.position.x, vertex.position.y, (float*)(&(vertex.color)));

				pStrand->prependVertex(vertex, params.stepLength);

				// Calculate new direction
				Vec2f newDir = HairUtil::orient2direction(getOrientationAt(vertex.position.x, vertex.position.y));

				float cosTheta = newDir.dot(dirBackward);
				if (cosTheta < 0.0f)
				{
					newDir = -newDir;
					cosTheta = newDir.dot(dirBackward);
				}
				dirBackward = newDir;

				// Check max bend angle
				if (acosf(cosTheta) > params.maxBendAngle)
					bTraceBackward = false;
			}
			else
			{
				bTraceBackward = false;
			}
		}
	}
}


// The newest sparse strands tracing algorithm
void HairImage::traceSparseStrands(const TracingParam& params, HairStrandModel* pStrandModel)
{
	Q_ASSERT(m_orientData.data != NULL);
	Q_ASSERT(m_confidenceData.data != NULL);
	Q_ASSERT(m_featureData.data != NULL);

	//
	// Collect initial seeds from feature map
	//
	std::vector<Vec2i> initSeeds;

	for (int y = 0; y < m_featureData.rows; y++)
		for (int x = 0; x < m_featureData.cols; x++)
			if (m_featureData.at<uchar>(y, x) == 255)
				initSeeds.push_back(Vec2i(x, y));

	//
	// (Optionally) shuffle seeds order
	//
	if (params.randomSeedOrder)
		HairUtil::shuffleOrder(initSeeds);

	pStrandModel->clear();

	for (int i = 0; i < initSeeds.size(); i++)
	{
		// Trace each seeds
		Strand strand;
		traceSingleSparseStrand((float)initSeeds[i][0] + 0.5f, 
			(float)initSeeds[i][1] + 0.5f, params, &strand);

		if (strand.numVertices() > 1)
		{
			updateTracedPixels(Vec2f((float)initSeeds[i][0] + 0.5f, 
									 (float)initSeeds[i][1] + 0.5f), params);

			if (params.lengthClamping == false ||
				strand.length() > params.minLength)
				pStrandModel->addStrand(strand);
		}
	}

	pStrandModel->updateBuffers();
}


void HairImage::traceSingleSparseStrand(float x, float y, const TracingParam& params, Strand* pStrand)
{
	int nX = (int)x, nY = (int)y;

	// Tracing must starts from a (feature==255) pixel
	if (m_featureData.at<uchar>(nY, nX) != 255)
		return;

	if (getCoverageAt(x, y) > 0)
		return;

	// Correct initial position
	Vec2f correctPos = correctPosToRidge(Vec2f(x, y), params);
	x = correctPos[0];
	y = correctPos[1];

	// Add initial vertex
	StrandVertex vertex;
	vertex.position = XMFLOAT3(x, y, 0);
	vertex.color	= XMFLOAT4(1, 1, 1, 1);

	pStrand->clear();
	pStrand->appendVertex(vertex);

	// Calculate tracing directions
	Vec2f dir = HairUtil::orient2direction(getOrientationAt(x, y));

	traceOneDirectionSparse(params, TraceForward, dir, pStrand);
	traceOneDirectionSparse(params, TraceBackward, -dir, pStrand);
}


void HairImage::traceOneDirectionSparse(const TracingParam& params, const TraceMode mode, const cv::Vec2f initDir, Strand* pStrand)
{
	Q_ASSERT(pStrand);
	Q_ASSERT(pStrand->numVertices() > 0);

	Vec2f pos = (mode == TraceForward) ?
		Vec2f((const float*)&(pStrand->vertices()[pStrand->numVertices() - 1].position)) :
		Vec2f((const float*)&(pStrand->vertices()[0]));

	Vec2f dir = initDir;
	
	// Tracing status
	int  health   = params.healthPoint;
	bool occluded = false;

	QQueue<Vec2f> lastPosArray;

	while (true)
	{
		// Calculate new position
		pos = pos + params.stepLength * dir;

		// Check termination conditions
		if (!isInsideHairRegion(pos[0], pos[1]))
			break;
		if (health < 0)
			break;
		
		// Calculate tracing direction at new position
		float diffAngle;
		Vec2f newDir = calcTraceDirection(pos, dir, &diffAngle);

		bool confident = (getConfidenceAt(pos[0], pos[1]) > params.lowConfidence) &&
						 (getCoverageAt(pos[0], pos[1]) == 0);

		if (confident)
		{
			if (diffAngle < params.maxBendAngle)
			{
				dir		 = newDir;
				occluded = false;
				health	 = params.healthPoint;
				// Correct position
				pos = correctPosToRidge(pos, params);
			}
			else
			{
				// TODO: estimate tracing direction
				occluded = true;
				health	 = health - 1;
			}
		}
		else
		{
			// TODO: estimate tracing direction
			health = health - 1;
		}

		// Add vertex to strand
		StrandVertex vertex;
		vertex.position.x = pos[0];
		vertex.position.y = pos[1];
		vertex.position.z = occluded ? 1.0f : 0.0f;
		vertex.color.x = confident ? 1.0f : 0.5f;
		vertex.color.y = 0.5f;
		vertex.color.z = occluded ? 0.5f : 1.0f;
		vertex.color.w = 1.0f;

		if (mode == TraceForward)
			pStrand->appendVertex(vertex, params.stepLength);
		else if (mode == TraceBackward)
			pStrand->prependVertex(vertex, params.stepLength);

		// Update feature/coverage data at traced locations
		lastPosArray.enqueue(pos);
		if (lastPosArray.count() > 5)
			updateTracedPixels(lastPosArray.dequeue(), params);
	}

	// Update feature/coverage data at traced locations
	while (!lastPosArray.isEmpty())
		updateTracedPixels(lastPosArray.dequeue(), params);
}


// Along a direction normal to local orientation, find nearest maximum
// in confidence map with sub-pixel accuracy...
Vec2f HairImage::correctPosToRidge(const cv::Vec2f& pos, const TracingParam& params)
{
	if (params.correctToRidge == false)
		return pos;

	const float orient = getOrientationAt(pos[0], pos[1]);

	if (HairUtil::isOrientValid(orient) == false)
		return pos;

	Vec2f dir = HairUtil::orient2direction(orient + CV_PI * 0.5f);

	Vec2f posLeft  = pos + params.ridgeRadius * dir;
	Vec2f posRight = pos - params.ridgeRadius * dir;

	float cLeft  = HairUtil::sampleLinear<float>(m_confidenceData, posLeft[0], posLeft[1]);
	float cRight = HairUtil::sampleLinear<float>(m_confidenceData, posRight[0], posRight[1]);
	float cMid   = HairUtil::sampleLinear<float>(m_confidenceData, pos[0], pos[1]);

	// Not near a ridge, do nothing
	if (cMid < cLeft || cMid < cRight)
		return pos;

	if (cLeft > cRight)
	{
		// Maximum is between posLeft and pos
		float a = cMid - cRight;
		float b = cLeft - cRight;
		float d = 0.5f * b * params.ridgeRadius / a;
		if (d != d)
			d = 0;
		else if (d > params.maxCorrection)
			d = params.maxCorrection;

		return pos + d * dir;
	}
	else
	{
		// Maximum is between pos and posRight
		float a = cMid - cLeft;
		float b = cRight - cLeft;
		float d = 0.5f * b * params.ridgeRadius / a;
		if (d != d)
			d = 0;
		else if (d > params.maxCorrection)
			d = params.maxCorrection;

		return pos - d * dir;
	}
}


// Update feature/coverage data at traced locations
void HairImage::updateTracedPixels(const cv::Vec2f& pos, const TracingParam& params)
{
	float centerOrient = getOrientationAt(pos[0], pos[1]);

	int nX = (int)pos[0], nY = (int)pos[1];

	int fromX = max(nX - 2, 0);
	int toX   = min(nX + 2, m_featureData.cols - 1);
	int fromY = max(nY - 2, 0);
	int toY   = min(nY + 2, m_featureData.rows - 1);

	for (int y = fromY; y <= toY; y++)
	{
		for (int x = fromX; x <= toX; x++)
		{
			uchar& flag = m_featureData.at<uchar>(y, x);
			if (flag == 255)
			{
				flag = 128;
			}

			uchar& coverage = m_coverageData.at<uchar>(y, x);
			if (coverage < 255)
			{
				// Calculate coverage as a falloff from centerline
				float u = (float)x + 0.5f - pos[0];
				float v = (float)y + 0.5f - pos[1];

				float w = expf(-(u*u + v*v) / (params.coverageSigma*params.coverageSigma));

				float newCoverage = min(255.0f, (float)coverage + w*params.centerCoverage);

				coverage = (uchar)newCoverage;
			}
		}
	}
}


// OUT OF DATE: use traceSparseStrands(...) instead.
void HairImage::traceStrandsByFeatures(const TracingParam& params, HairStrandModel* pStrandModel)
{
	Q_ASSERT(m_orientData.data != NULL);
	Q_ASSERT(m_varianceData.data != NULL);
	Q_ASSERT(m_featureData.data != NULL);

	// Collect high var pixels in feature mask as initial seeds for hair growing
	std::vector<Vec2i> initSeeds;

	for (int y = 0; y < m_featureData.rows; y++)
	{
		for (int x = 0; x < m_featureData.cols; x++)
		{
			if (m_featureData.at<uchar>(y, x) == 255)
			{
				initSeeds.push_back(Vec2i(x, y));
			}
		}
	}

	if (params.randomSeedOrder)
		HairUtil::shuffleOrder(initSeeds);

	pStrandModel->clear();

	m_coverageData = Mat::zeros(m_featureData.size(), CV_8UC1);

	for (int i = 0; i < initSeeds.size(); i++)
	{
		// Trace each seeds
		Strand strand;
		traceSingleStrandByFeature((float)initSeeds[i][0] + 0.5f, 
								   (float)initSeeds[i][1] + 0.5f, params, &strand);

		if (strand.numVertices() > 1)
		{
			if (params.lengthClamping == false ||
				(strand.length() > params.minLength &&
				 strand.length() < params.maxLength))
			pStrandModel->addStrand(strand);
		}
	}

	// TEST STRAND
	//Strand calibStrand;
	//StrandVertex vertex;
	//vertex.position = XMFLOAT3(0, 0, 0);
	//vertex.color	= XMFLOAT4(1, 1, 1, 1);
	//calibStrand.appendVertex(vertex);
	//vertex.position = XMFLOAT3(1, 0, 0);
	//vertex.color	= XMFLOAT4(1, 0, 0, 1);
	//calibStrand.appendVertex(vertex);
	//vertex.position = XMFLOAT3(0, 1, 0);
	//vertex.color	= XMFLOAT4(0, 1, 0, 1);
	//calibStrand.appendVertex(vertex);
	//vertex.position = XMFLOAT3(1, 1, 0);
	//vertex.color	= XMFLOAT4(1, 1, 0, 1);
	//calibStrand.appendVertex(vertex);
	//pStrandModel->addStrand(calibStrand);

	pStrandModel->updateBuffers();
}

// Trace a single hair strand according to both orientation and variance
void HairImage::traceSingleStrandByFeature(float x, float y, const TracingParam& params, Strand* pStrand)
{
	int nx = (int)x;
	int ny = (int)y;

	// The pixel in feature mask must not have been visited before
	if (m_featureData.at<uchar>(ny, nx) == 0)
		return;

	// The variance at the pixel must be higher than HIGH threshold
	if (m_confidenceData.at<float>(ny, nx) < params.highConfidence)
		return;

	// OPTIONAL: correct (x,y) to local variance ridge
	// ...

	// The following tracing procedure is almost same as a standard
	// method, except we add an additional termination condition:
	// variance below LOW threshold for hysteresis.

	// Add initial vertex
	StrandVertex vertex;
	vertex.position = XMFLOAT3(x, y, 0);
	vertex.color	= XMFLOAT4(1, 1, 1, 1);

	pStrand->clear();
	pStrand->appendVertex(vertex);

	// Calculate tracing directions
	Vec2f dirForward  = HairUtil::orient2direction(getOrientationAt(x, y));
	Vec2f dirBackward = -dirForward;

	traceOneDirection(params, TraceForward, dirForward, pStrand);
	traceOneDirection(params, TraceBackward, dirBackward, pStrand);

	// After tracing, we need to mark the feature mask pixels along the strand
	// as VISITED.
	// NOTE: we only modify pixels with similar orientation as the strand's

	//for (int i = 0; i < pStrand->numVertices(); i++)
	//{
	//	// For each vertex, search in a 3x3 region...
	//	nx = int(pStrand->vertices()[i].position.x);
	//	ny = int(pStrand->vertices()[i].position.y);

	//	float centerOrient = m_orientData.at<float>(ny, nx);

	//	int fromX = max(nx - 1, 0);
	//	int toX   = min(nx + 1, m_featureData.cols - 1);
	//	int fromY = max(ny - 1, 0);
	//	int toY   = min(ny + 1, m_featureData.rows - 1);

	//	for (int y = fromY; y <= toY; y++)
	//	{
	//		for (int x = fromX; x <= toX; x++)
	//		{
	//			if (m_featureData.at<uchar>(y, x) > 0 &&
	//				diffOrient(centerOrient, m_orientData.at<float>(y, x)) < params.orientThreshold)
	//			{
	//				m_featureData.at<uchar>(y, x) = 0;
	//			}
	//			// Update coverage data
	//			//if (m_coverageData.at<uchar>(y, x) < 255)
	//			//	m_coverageData.at<uchar>(y, x)++;
	//		}
	//	}
	//}

	m_debug2Data = m_featureData.clone();
}


// Trace the strand at one direction (forward or backward)
void HairImage::traceOneDirection(const TracingParam& params, const TraceMode mode, 
								  const Vec2f initDir, Strand* pStrand)
{
	Q_ASSERT(pStrand);
	Q_ASSERT(pStrand->numVertices() > 0);

	enum VertexType
	{
		NormalVertex,
		UncertainVertex,
		OccludedVertex,
	};

	StrandVertex	vertex;
	VertexType		vertexType;

	int		numLaTerm	= 0;
	Vec2f	direction	= initDir;

	// We keep record of the last 5 added vertex positions so that we
	// can flag those visited pixels but not affecting the current
	// tracing pixels...(if you know what I mean)
	QQueue<XMFLOAT3>	lastAddedPos;

	while (true)
	{
		// Get last vertex
		if (mode == TraceForward)
			vertex = pStrand->vertices()[pStrand->numVertices() - 1];
		else if (mode == TraceBackward)
			vertex = pStrand->vertices()[0];
		else
			return;	// ERROR

		// Calculate new vertex location
		float newX = vertex.position.x + params.stepLength * direction[0];
		float newY = vertex.position.y + params.stepLength * direction[1];

		// Terminate tracing if out of hair region
		if (!isInsideHairRegion(newX, newY))
			break;

		// Calculate new direction
		Vec2f newDir = HairUtil::orient2direction(getOrientationAt(newX, newY));
		float cosine = newDir.dot(direction);
		if (cosine < 0.0f)
		{
			newDir = -newDir;
			cosine = -cosine;
		}
		float bendAngle = acosf(cosine);

		// Check whether the new orientation is coherent with previous one and 
		// is confident enough.

		// PROBLEM IDENTIFIED!!!!! 
		// GaussianBlur confidence map during EVERY tracing step...ARE YOU CRAZY???
		float confid = m_confidenceData.at<float>(newY, newX);

		Mat confidDepthData = m_confidenceData.clone();
		GaussianBlur(confidDepthData, confidDepthData, Size(-1,-1), 1);

		float confidDepth = confidDepthData.at<float>(newY, newX);

		bool isCoherent  = (bendAngle < params.maxBendAngle); // RENAME PARAM?
		bool isConfident = (confid    > params.lowConfidence); // RENAME PARAM?
		bool isVisited   = (m_featureData.at<uchar>(newY, newX) == 0);

		if (isVisited)
		{
			if (isConfident)
			{
				if (isCoherent)
				{
					direction  = newDir;
				}
				else
				{
					// Confident, but incoherent. Trace as occluded
					vertexType = OccludedVertex;
					//vertex.position.z = 1.0f;
				}
				numLaTerm  = numLaTerm + 1;
			}
			else
			{
				if (isCoherent)
				{
					// Not confident, but coherent. Keep tracing
					numLaTerm  = numLaTerm + 1;
					direction  = newDir;
				}
				else
				{
					// Neither confident nor coherent.
					numLaTerm  = numLaTerm + 1;
				}
			}
		}
		else
		{
			if (isConfident)
			{
				if (isCoherent)
				{
					// Both confident and coherent. Trace normally
					numLaTerm  = 0;
					vertexType = NormalVertex;
					//vertex.position.z = 0.0f;
					direction  = newDir;
				}
				else
				{
					// Confident, but incoherent. Trace as occluded
					numLaTerm  = numLaTerm + 1;
					vertexType = OccludedVertex;
					//vertex.position.z = 1.0f;
				}
			}
			else
			{
				if (isCoherent)
				{
					// Not confident, but coherent. Keep tracing
					numLaTerm  = numLaTerm + 1;
					direction  = newDir;
				}
				else
				{
					// Neither confident nor coherent.
					numLaTerm  = numLaTerm + 1;
				}
			}
		}

		vertex.color.x = isConfident ? 1.0f : 0.2f;
		vertex.color.y = isCoherent  ? 1.0f : 0.2f;
		vertex.color.z = (vertexType == OccludedVertex) ? 1.0f : 0.2f;

		if (numLaTerm > params.healthPoint)
			break;

		// Add vertex to the strand
		vertex.position.x = newX;
		vertex.position.y = newY;
		if (vertexType == OccludedVertex)
		{
			//...vertex.position.z = 
		}
		else
		{
			vertex.position.z = confidDepth;
		}

		if (mode == TraceForward)
			pStrand->appendVertex(vertex, params.stepLength);
		else if (mode == TraceBackward)
			pStrand->prependVertex(vertex, params.stepLength);

		lastAddedPos.enqueue(vertex.position);
		if (lastAddedPos.count() > 5)
		{
			// Mark pixels around the oldest vertex as VISITED.
			XMFLOAT3 pos = lastAddedPos.dequeue();

			/*
			// For each vertex, search in a 3x3 region...
			int nx = pos.x;
			int ny = pos.y;

			float centerOrient = m_orientData.at<float>(ny, nx);

			int fromX = max(nx - 1, 0);
			int toX   = min(nx + 1, m_featureData.cols - 1);
			int fromY = max(ny - 1, 0);
			int toY   = min(ny + 1, m_featureData.rows - 1);

			for (int y = fromY; y <= toY; y++)
			{
				for (int x = fromX; x <= toX; x++)
				{
					if (m_featureData.at<uchar>(y, x) > 0 &&
						diffOrient(centerOrient, m_orientData.at<float>(y, x)) < params.orientThreshold)
					{
						m_featureData.at<uchar>(y, x) = 0;
					}
					// Update coverage data
					//if (m_coverageData.at<uchar>(y, x) < 255)
					//	m_coverageData.at<uchar>(y, x)++;
				}
			}
			*/
		}
	}

	// Marking all the remaining vertices in the queue
	while (!lastAddedPos.isEmpty())
	{
		// Mark pixels around the oldest vertex as VISITED.
		XMFLOAT3 pos = lastAddedPos.dequeue();

		/*
		// For each vertex, search in a 3x3 region...
		int nx = pos.x;
		int ny = pos.y;

		float centerOrient = m_orientData.at<float>(ny, nx);

		int fromX = max(nx - 1, 0);
		int toX   = min(nx + 1, m_featureData.cols - 1);
		int fromY = max(ny - 1, 0);
		int toY   = min(ny + 1, m_featureData.rows - 1);

		for (int y = fromY; y <= toY; y++)
		{
			for (int x = fromX; x <= toX; x++)
			{
				if (m_featureData.at<uchar>(y, x) > 0 &&
					diffOrient(centerOrient, m_orientData.at<float>(y, x)) < params.orientThreshold)
				{
					m_featureData.at<uchar>(y, x) = 0;
				}
				// Update coverage data
				//if (m_coverageData.at<uchar>(y, x) < 255)
				//	m_coverageData.at<uchar>(y, x)++;
			}
		}
		*/
	}
}

Vec2f HairImage::calcTraceDirection(const Vec2f& pos, const Vec2f& prevDir, float* pDiffAngle)
{
	Vec2f dir = HairUtil::orient2direction(getOrientationAt(pos[0], pos[1]));

	float cosine = dir.dot(prevDir);
	if (cosine < 0.0f)
	{
		dir = -dir;
		cosine = -cosine;
	}

	if (pDiffAngle)
		*pDiffAngle = acosf(cosine);

	return dir;
}


void HairImage::traceAuxStrands(const TracingParam& params, HairStrandModel* pStrandModel)
{
	Q_ASSERT(m_orientData.data);

	if (m_coverageData.empty())
	{
		m_coverageData = Mat::zeros(m_orientData.size(), CV_8UC1);
	}

	// Collect high var pixels in feature mask as initial seeds for hair growing
	std::vector<Vec2i> initSeeds;

	for (int y = 0; y < m_coverageData.rows; y++)
		for (int x = 0; x < m_coverageData.cols; x++)
			if (m_coverageData.at<uchar>(y, x) <= params.minAuxCoverage)
				initSeeds.push_back(Vec2i(x, y));

	qDebug() << "Init seeds:" << initSeeds.size() << "(" << 
		100.0f*initSeeds.size()/float(m_coverageData.cols*m_coverageData.rows) << "%%)";

	// OPTIONAL: shuffle seeds order
	if (params.randomSeedOrder)
		HairUtil::shuffleOrder(initSeeds);

	pStrandModel->clear();

	for (int i = 0; i < initSeeds.size(); i++)
	{
		int nX = initSeeds[i][0];
		int nY = initSeeds[i][1];
		float fX = (float)nX + 0.5f;
		float fY = (float)nY + 0.5f;

		// Trace must starts from whose coverage below given threshold
		if (m_coverageData.at<uchar>(nY, nX) > params.minAuxCoverage)
			continue;

		// Trace each seeds
		Strand strand;
		traceSingleAuxStrand(fX, fY, params, &strand);

		if (strand.numVertices() > 1)
		{
			updateTracedPixels(Vec2f(fX, fY), params);

			if (params.lengthClamping == false ||
				strand.length() > params.minLength)
				pStrandModel->addStrand(strand);
		}
	}

	pStrandModel->updateBuffers();
}


// Trace a single auxiliary strand from the (smoothed) orientation map
void HairImage::traceSingleAuxStrand(float x, float y, const TracingParam& params, Strand* pStrand)
{
	int nx = (int)x;
	int ny = (int)y;

	// Add initial vertex
	StrandVertex vertex;
	vertex.position = XMFLOAT3(x, y, 0);
	vertex.color	= XMFLOAT4(0, 0, 1, 1);

	pStrand->clear();
	pStrand->appendVertex(vertex);

	// Calculate tracing directions
	Vec2f dir = HairUtil::orient2direction(getOrientationAt(x, y));
	if (HairUtil::isDirectionValid(dir) == false)
		return;

	traceOneDirectionAux(params, TraceForward, dir, pStrand);
	traceOneDirectionAux(params, TraceBackward, -dir, pStrand);
}


void HairImage::traceOneDirectionAux(const TracingParam& params, const TraceMode mode, const cv::Vec2f initDir, Strand* pStrand)
{
	Q_ASSERT(pStrand);
	Q_ASSERT(pStrand->numVertices() > 0);

	Vec2f pos = (mode == TraceForward) ?
		Vec2f((const float*)&(pStrand->vertices()[pStrand->numVertices() - 1].position)) :
	Vec2f((const float*)&(pStrand->vertices()[0]));

	Vec2f dir = initDir;

	// Tracing status
	int  health   = params.healthPoint;

	QQueue<Vec2f> lastPosArray;

	while (true)
	{
		// Calculate new position
		pos = pos + params.stepLength * dir;

		// Check termination conditions
		if (!isInsideHairRegion(pos[0], pos[1]))
			break;
		if (health < 0)
			break;

		// Calculate tracing direction at new position
		float diffAngle;
		Vec2f newDir = calcTraceDirection(pos, dir, &diffAngle);

		if (HairUtil::isDirectionValid(newDir) && 
			diffAngle < params.maxBendAngle &&
			getCoverageAt(pos[0], pos[1]) < params.maxAuxCoverage)
		{
			dir		 = newDir;
			health	 = params.healthPoint;
		}
		else
		{
			health	 = health - 1;
		}

		// Add vertex to strand
		StrandVertex vertex;
		vertex.position.x = pos[0];
		vertex.position.y = pos[1];
		vertex.position.z = 0.0f;
		vertex.color.x = 0.0f;
		vertex.color.y = 0.0f;
		vertex.color.z = 1.0f;
		vertex.color.w = 1.0f;

		if (mode == TraceForward)
			pStrand->appendVertex(vertex, params.stepLength);
		else if (mode == TraceBackward)
			pStrand->prependVertex(vertex, params.stepLength);

		// Update feature/coverage data at traced locations
		lastPosArray.enqueue(pos);
		if (lastPosArray.count() > 5)
			updateTracedPixels(lastPosArray.dequeue(), params);
	}

	// Update feature/coverage data at traced locations
	while (!lastPosArray.isEmpty())
		updateTracedPixels(lastPosArray.dequeue(), params);
}


bool HairImage::isInsideHairRegion(float x, float y)
{
	int nx = (x >= 0.0f) ? int(x) : int(x - 1.0f);
	int ny = (y >= 0.0f) ? int(y) : int(y - 1.0f);

	if (nx >= 0 && nx < width() &&
		ny >= 0 && ny < height() &&
		m_maskData.ptr<uchar>(ny)[nx] > 0)
	{
		return true;
	}

	return false;
}


// Sample orientation map (using nearest neighbor)
float HairImage::getOrientationAt(float x, float y)
{
	return m_orientData.ptr<float>(int(y))[int(x)];
}

float HairImage::getConfidenceAt(float x, float y)
{
	return m_confidenceData.at<float>(y, x);
}

uchar HairImage::getCoverageAt(float x, float y)
{
	return m_coverageData.at<uchar>(y, x);
}


void HairImage::getColorAt(float x, float y, float* pRgba)
{
	Vec3b srcColor = m_colorData.ptr<Vec3b>(int(y))[int(x)];

	pRgba[0] = (float)srcColor[2] / 255.0f;
	pRgba[1] = (float)srcColor[1] / 255.0f;
	pRgba[2] = (float)srcColor[0] / 255.0f;
	pRgba[3] = 1.0f;
}



void HairImage::relaxSeeds(Mat& seeds, float density, float relaxForce)
{
	// TODO: relaxation
	flann::KDTreeIndexParams params(4);
	flann::Index kdTree(seeds, params);

	const float RR    = 1.0f / (density * CV_PI);
	const float R     = sqrtf(RR);
	const float RRR   = R * RR;
	const float maxRR = 4 * R * R;

	Mat forces = Mat::zeros(1, seeds.rows, CV_32FC2);
	Mat indices(32, 1, CV_32S);
	Mat dists(32, 1, CV_32F);
	for (int i = 0; i < seeds.rows; i++)
	{
		// Search nearby seeds
		Vec2f seed = seeds.ptr<Vec2f>(i)[0];
		Mat query(1, 2, CV_32F);
		query.ptr<float>(0)[0] = seed[0];
		query.ptr<float>(0)[1] = seed[1];

		flann::SearchParams searchParams;
		int n = kdTree.radiusSearch(query, indices, dists, maxRR, 32);
		//kdTree.knnSearch(query, indices, dists, 8);
		
		//const int n = indices.cols;
		for (int j = 0; j < n; j++)
		{
			// Add force from each neighbor seed
			int index = indices.ptr<int>(0)[j];
			if (index == i)
				continue;	// ignore the impact from itself

			Vec2f neighbor = seeds.ptr<Vec2f>(index)[0];
			
			float dd  = dists.ptr<float>(0)[j];
			float d   = sqrtf(dd);
			float ddd = dd*d;

			Vec2f force = (seed - neighbor);
			forces.ptr<Vec2f>(0)[i] += force * (relaxForce * RRR / ddd);
		}
	}

	// move seeds by the force upon
	for (int i = 0; i < seeds.rows; i++)
	{
		seeds.ptr<Vec2f>(i)[0] += forces.ptr<Vec2f>(0)[i];
	}	
}


// Sample hair strand color from input image
void HairImage::sampleStrandColor(const SampleColorParam& params, HairStrandModel* pStrandModel)
{
	Q_ASSERT(pStrandModel);
	Q_ASSERT(m_colorData.data != NULL || m_hairColorData.data != NULL);

	if (m_hairColorData.data)
		sampleStrandColor(m_hairColorData, m_maskData, params, 0, pStrandModel->numStrands() - 1, pStrandModel);
	else
		sampleStrandColor(m_colorData, m_maskData, params, 0, pStrandModel->numStrands() - 1, pStrandModel);
}

void HairImage::sampleStrandColor(const Mat& colorData, const cv::Mat& maskData, const SampleColorParam& params, 
	const int firstIdx, const int lastIdx, HairStrandModel* pStrandModel)
{
	if (pStrandModel->numStrands() < 1)
		return;

	if (firstIdx > lastIdx || firstIdx < 0 || lastIdx >= pStrandModel->numStrands())
		return;

	// Pre-compute a filtering kernel
	float* kernel = NULL;
	int kernelSize = params.blurRadius*2 + 1;
	if (kernelSize > 1)
	{
		kernel = new float[kernelSize];
		float sumW = 0.0f;
		for (int i = 0; i < kernelSize; i++)
		{
			float sigma = (float)params.blurRadius * 0.4f;
			float r = (i - params.blurRadius);

			kernel[i] = exp(-(r*r) / (sigma*sigma));
			sumW += kernel[i];
		}
		// Normalize
		for (int i = 0; i < kernelSize; i++)
		{
			kernel[i] /= sumW;
		}
	}
	
	// Process strand by strand
	for (int i = firstIdx; i <= lastIdx; i++)
	{
		Strand* pStrand = pStrandModel->getStrandAt(i);

		for (int j = 0; j < pStrand->numVertices(); j++)
		{
			XMFLOAT3& pos = pStrand->vertices()[j].position;

			Vec4b srcColor = HairUtil::sampleLinear<Vec4b>(colorData, pos.x, pos.y);

			pStrand->vertices()[j].color = XMFLOAT4(
				(float)srcColor[2] / 255.0f,
				(float)srcColor[1] / 255.0f,
				(float)srcColor[0] / 255.0f,
				params.sampleAlpha ? 
					(float)HairUtil::sampleLinear<uchar>(maskData, pos.x, pos.y) / 255.0f : 1.0f
				);
		}

		if (params.blurRadius > 0)
		{
			// Blur color along each strand
			Strand newStrand = *pStrand;

			for (int j = 0; j < pStrand->numVertices(); j++)
			{
				XMFLOAT4& dstColor = newStrand.vertices()[j].color;
				dstColor = XMFLOAT4(0, 0, 0, 0);

				for (int k = 0; k < kernelSize; k++)
				{
					int idx = j + k - params.blurRadius;
					if (idx < 0)
						idx = 0;
					else if (idx >= pStrand->numVertices())
						idx = pStrand->numVertices() - 1;

					XMFLOAT4& srcColor = pStrand->vertices()[idx].color;

					dstColor.x += srcColor.x * kernel[k];
					dstColor.y += srcColor.y * kernel[k];
					dstColor.z += srcColor.z * kernel[k];
					dstColor.w += srcColor.w * kernel[k];
				}
			}
			*pStrand = newStrand;
		}
	}
}


//////////////////////////////////////////////////////////////////
// Utility functions
//////////////////////////////////////////////////////////////////

// Convert certain hair image data to a QImage buffer.
void HairImage::draw8UDataToQImage(const cv::Mat& src, QImage* dst) const
{
	Q_ASSERT(dst != NULL);

	if (src.empty())
	{
		dst->fill(0);
		return;
	}

	Q_ASSERT(dst->width()  == src.cols);
	Q_ASSERT(dst->height() == src.rows);
	Q_ASSERT(dst->format() == QImage::Format_ARGB32);
	Q_ASSERT(src.depth() == CV_8U);

	Mat dstData(m_colorData.size(), CV_8UC4, dst->bits());

	if (src.channels() == 1)
	{
		Mat rgbData;
		cvtColor(src, rgbData, CV_GRAY2RGB);
		cvtColor(rgbData, dstData, CV_RGB2RGBA);
	}
	else if (src.channels() == 3)
	{
		cvtColor(src, dstData, CV_RGB2RGBA);
	}
	else if (src.channels() == 4)
	{
		src.copyTo(dstData);
	}
	else
	{
		Q_ASSERT(src.channels()==1 || src.channels()==3 || src.channels()==4);
	}
}


Mat_<float> HairImage::createGaussianKernel(float sigma)
{
	int kRadius = int(3.0f * sigma);
	int kSize   = kRadius*2 + 1;

	float sigmaSq2 = 2.0f * sigma * sigma;

	Mat_<float> kernel(1, kSize);
	for (int i = 0; i < kSize; i++)
	{
		float x = i - kRadius;
		kernel(i) = expf(-(x*x)/sigmaSq2);
	}
	return kernel;
}


void HairImage::createGaborKernel(int width, int height, float theta, 
								  float sigmaX, float sigmaY, float lambda, 
								  float phase, cv::Mat& kernel)
{
	if (kernel.cols != width || kernel.rows != height || kernel.type() != CV_32F)
	{
		kernel.create(height, width, CV_32F);
	}

	const float sigmaXSq = sigmaX * sigmaX;
	const float sigmaYSq = sigmaY * sigmaY;
	const float sinTheta = sinf(theta);
	const float cosTheta = cosf(theta);

	for (int row = 0; row < height; row++)
	{
		float tempY = (float)(row - height/2);

		float* pValues = kernel.ptr<float>(row);

		for (int col = 0; col < width; col++)
		{
			float tempX = (float)(col - width/2);
			float x = tempX*cosTheta - tempY*sinTheta;
			float y = tempX*sinTheta + tempY*cosTheta;

			float compWave  = cosf(2.0f * CV_PI * x / lambda + phase);
			float compGauss = expf(-0.5f * (x*x / sigmaXSq + y*y / sigmaYSq));

			pValues[col] = compGauss * compWave;
		}
	}
}



// Calculate front/middle/back depth maps
void HairImage::calcTriDepthMaps(const TracingParam& params, HairStrandModel* pStrandModel)
{
	Q_ASSERT(pStrandModel);

	QProgressDialog progressDlg;
	progressDlg.setLabelText("Calculating tri-depth map...");
	progressDlg.setRange(0, 100);
	progressDlg.setModal(true);
	progressDlg.show();
	progressDlg.setValue(0);

	const float borderSigma = 0.1f;
	const float faceWeightThres = 0.1f;

	// Calculate front depth map and traced hair mask
	calcFrontDepth(params, pStrandModel);

	progressDlg.setValue(25);

	// Calculate depth weight from the proximity to outer silhouette and to the face area
	Mat borderWeight;
	GaussianBlur(m_traceMaskData, borderWeight, Size(-1, -1), 1.0);

	Mat faceWeight;
	m_faceMaskData.convertTo(faceWeight, CV_32F, 1.0/255.0);
	GaussianBlur(faceWeight, faceWeight, Size(-1, -1), 5.0);

	for (int y = 0; y < borderWeight.rows; y++)
	{
		for (int x = 0; x < borderWeight.cols; x++)
		{
			float& w = borderWeight.at<float>(y, x);
			float& faceW = faceWeight.at<float>(y, x);

			w = (faceW > faceWeightThres || m_traceMaskData.at<float>(y,x) < FLT_MIN) ? 
				0 : expf(-(w - 0.5f)*(w - 0.5f) / (borderSigma*borderSigma));
		}
	}

	Mat hairFaceMask;
	if (m_midMaskData.data != NULL)
		hairFaceMask = m_midMaskData.clone();
	else
		hairFaceMask;// NOTE! // max(m_traceMaskData, faceWeight);

	progressDlg.setValue(33);

	// Calculate middle depth map.
	m_midDepthData = m_frontDepthData.clone();
	calcMidDepth(borderWeight, hairFaceMask, m_midDepthData);

	progressDlg.setValue(75);

	// Calculate back depth map
	calcBackDepth(params.dsSilhoutteRadius, params.dsSilhoutteDepth, 
		params.esBackHalfScale, params.esBackHalfOffset, hairFaceMask);

	progressDlg.setValue(100);
}

// Calculate the "membrane" middle depth map.
void HairImage::calcMidDepth(const cv::Mat& borderWeight, const cv::Mat& maskData, cv::Mat& depthData)
{
	// Due to issues with TAUCS, we use simple scattered data interp instead...

	std::vector<Vec3f> borderPoints;
	for (int y = 0; y < borderWeight.rows; y++)
	{
		for (int x = 0; x < borderWeight.cols; x++)
		{
			if (borderWeight.at<float>(y, x) > 0.1f)
			{
				borderPoints.push_back(Vec3f(x, y, depthData.at<float>(y, x)));
			}
		}
	}

	for (int y = 0; y < maskData.rows; y++)
	{
		for (int x = 0; x < maskData.cols; x++)
		{
			if (borderWeight.at<float>(y, x) > 0.1f)
				continue;

			if (maskData.at<float>(y, x) <= FLT_MIN)
				continue;

			float depth = 0;
			float weight = 0;

			for (int i = 0; i < borderPoints.size(); i++)
			{
				Vec3f& pt = borderPoints[i];

				float dx = pt[0] - (float)x;
				float dy = pt[1] - (float)y;

				float distSqInv = 1.0f / (dx*dx + dy*dy);

				depth  += pt[2] * distSqInv;
				weight += distSqInv;
			}

			depthData.at<float>(y, x) = depth / weight;
		}
	}
}

// Calculate a depth map for the front half of hair from existing hair strands model.
void HairImage::calcFrontDepth(const TracingParam& params, HairStrandModel* pStrandModel)
{
	Q_ASSERT(pStrandModel);
	Q_ASSERT(m_colorData.data != NULL);

	Mat depthData  = Mat::zeros(m_colorData.size(), CV_32FC1);
	Mat weightData = Mat::zeros(m_colorData.size(), CV_32FC1);

	for (int i = 0; i < pStrandModel->numStrands(); i++)
	{
		Strand* pStrand = pStrandModel->getStrandAt(i);

		for (int j = 0; j < pStrand->numVertices(); j++)
		{
			XMFLOAT3& pos = pStrand->vertices()[j].position;

			int nX = (int)pos.x, nY = (int)pos.y;

			int fromX = max(nX - 1, 0);
			int toX   = min(nX + 1, m_colorData.cols - 1);
			int fromY = max(nY - 1, 0);
			int toY   = min(nY + 1, m_colorData.rows - 1);

			for (int y = fromY; y <= toY; y++)
			{
				for (int x = fromX; x <= toX; x++)
				{
					// Calculate coverage as a falloff from centerline
					float u = (float)x + 0.5f - pos.x;
					float v = (float)y + 0.5f - pos.y;
					float w = expf(-(u*u + v*v) / 
							  (params.dsDepthMapSigma*params.dsDepthMapSigma));

					depthData.at<float>(y, x)  += w * pos.z;
					weightData.at<float>(y, x) += w;
				}
			}
		}
	}

	for (int y = 0; y < depthData.rows; y++)
	{
		for (int x = 0; x < depthData.cols; x++)
		{
			float& d = depthData.at<float>(y, x);
			float& w = weightData.at<float>(y, x);

			if (w > FLT_MIN)
			{
				d = d / w;
				w = 1.0f;//min(w, 1.0f);
			}
			else
			{
				d = 0;
				w = 0;
			}
		}
	}

	expandDepthData(depthData, weightData);

	m_frontDepthData = depthData.clone();
	m_traceMaskData  = weightData.clone();
}


// Calculate a depth map for the back half of hair from extract hair region mask.
void HairImage::calcBackDepth(float radius, float depth, float scale, float offset, const Mat& maskData)
{
	Q_ASSERT(m_midDepthData.data != NULL);

	Mat binMask;
	maskData.convertTo(binMask, CV_8U, 255.0);

	Mat depthData;

	// Calculate a distance map to region boundary
	distanceTransform(binMask, depthData, CV_DIST_L2, CV_DIST_MASK_PRECISE);

	float maxDist = 0;
	for (int y = 0; y < depthData.rows; y++)
	{
		for (int x = 0; x < depthData.cols; x++)
		{
			float d = depthData.at<float>(y, x);
			if (d > maxDist)
				maxDist = d;
		}
	}
	float maxDistSq = maxDist * maxDist;

	m_backDepthData = Mat::zeros(depthData.size(), CV_32FC1);

	// Pre-compute Spherical falloff
	float tA = maxDist - radius;			// horizontal triangle edge
	float tB = tA * radius / (2.0f * depth); // vertical triangle edge
	float tC = tA*tA + tB*tB;			// circle radius squared
	float tD = depth - tB;				// Y of circle center

	// Convert distance to depth using a spherical boundary fall-off profile
	for (int y = 0; y < depthData.rows; y++)
	{
		for (int x = 0; x < depthData.cols; x++)
		{
			float& d = depthData.at<float>(y, x);

			if (d <= 0)
			{
				d = 0;
			}
			else if (d <= radius)
			{
				// Quadratic falloff
				float t = d / radius;
				d = depth * t * t;
			}
			else
			{
				float t = maxDist - d;
				d = tD + sqrtf(abs(tC - t*t));
			}

			//float r = maxDist - d;

			//d = sqrtf(abs(maxDistSq - r*r));

			d *= scale;

			// Add to middle depth layer
			m_backDepthData.at<float>(y, x) = d + m_midDepthData.at<float>(y, x);
		}
	}

	// Diffuse...?

	// Visualize for debugging
	float minDepth = FLT_MAX;
	float maxDepth = -FLT_MAX;
	for (int y = 0; y < m_backDepthData.rows; y++)
	{
		for (int x = 0; x < m_backDepthData.cols; x++)
		{
			float d = m_backDepthData.at<float>(y, x);
			if (d > maxDepth)
				maxDepth = d;
			if (d < minDepth)
				minDepth = d;
		}
	}
	Mat temp = (m_backDepthData - minDepth) / (maxDepth - minDepth);
	temp.convertTo(m_debug1Data, CV_8U, 255.0);
}


void HairImage::genDenseStrands(const TracingParam& params, HairStrandModel* pSrcModel, HairStrandModel* pDstModel)
{
	Q_ASSERT(pSrcModel);
	Q_ASSERT(pDstModel);

	calcTriDepthMaps(params, pSrcModel);

	pDstModel->clear();

	//
	// Front half dense strands
	//
	SampleColorParam colorParams;
	colorParams.blurRadius = 2.0f;
	colorParams.sampleAlpha = true;

	QProgressDialog progressDlg;
	progressDlg.setLabelText("Tracing front layers...");
	progressDlg.setRange(0, params.dsNumFrontLayers);

	Mat depthData  = m_frontDepthData.clone();
	Mat alphaMaskData = m_maskData.clone();
	Mat maskData   = m_traceMaskData.clone();
	Mat colorData  = m_hairColorData.clone();
	Mat orientData = m_orientData.clone();
	Mat tensorData;
	HairUtil::orient2tensor(orientData, tensorData);

	for (int i = 0; i < params.dsNumFrontLayers; i++)
	{
		progressDlg.setValue(i);

		// Increase depth and shrink mask accordingly
		depthData += params.dsLayerThickness;
		shrinkDenseDepthMask(depthData, m_midDepthData, DepthLessThan, 
							 params.dsMaskShrinkDelta, maskData);

		// smooth orientation...
		GaussianBlur(tensorData, tensorData, Size(-1, -1), 0.5f);
		HairUtil::tensor2orient(tensorData, orientData);

		// Trace hair strands
		const int firstStrandIdx = pDstModel->numStrands();
		genDenseStrandsLayer(params, depthData, maskData, orientData, pDstModel);
		const int lastStrandIdx  = pDstModel->numStrands() - 1;

		// Sample hair color
		sampleStrandColor(colorData, alphaMaskData, colorParams, firstStrandIdx, lastStrandIdx, pDstModel);

		// smooth & darken color
		GaussianBlur(colorData, colorData, Size(-1, -1), 0.5);
		GaussianBlur(alphaMaskData, alphaMaskData, Size(-1, -1), 0.5);
		colorData *= 0.95f;

		colorParams.blurRadius += 2.0f;

	}

	//
	// TODO: middle layer.....
	//
	if (m_midColorData.data != NULL)
	{
		colorData = m_midColorData.clone();
		orientData = m_midOrientData.clone();
	}
	else
	{
		colorData = m_hairColorData.clone();
		orientData = m_orientData.clone();
	}

	if (m_midMaskData.data != NULL)
	{
		m_midMaskData.convertTo(alphaMaskData, CV_8UC1, 255.0f);
		maskData = m_midMaskData.clone();
	}
	else
	{
		alphaMaskData = m_maskData.clone();
		maskData = m_traceMaskData.clone();
	}

	depthData  = m_midDepthData.clone();
	HairUtil::orient2tensor(orientData, tensorData);
	colorParams.blurRadius = 2.0f;

	// Progress bar dialog
	progressDlg.setLabelText("Tracing middle layers...");
	progressDlg.setRange(0, params.esNumMidLayers);

	for (int i = 0; i < params.esNumMidLayers; i++)
	{
		progressDlg.setValue(i);

		// Trace hair strands
		const int firstStrandIdx = pDstModel->numStrands();
		genDenseStrandsLayer(params, depthData, maskData, orientData, pDstModel);
		const int lastStrandIdx  = pDstModel->numStrands() - 1;

		// Sample hair color
		sampleStrandColor(colorData, alphaMaskData, colorParams, firstStrandIdx, lastStrandIdx, pDstModel);

		// Smooth & darken color
		GaussianBlur(colorData, colorData, Size(-1, -1), 1.0);
		colorData *= 0.9f;
		colorParams.blurRadius += 2.0f;

		depthData += params.dsLayerThickness;
	}


	//
	// Back half dense strands
	//
	if (m_midColorData.data != NULL)
	{
		colorData = m_midColorData.clone();
		orientData = m_midOrientData.clone();
	}
	else
	{
		colorData = m_hairColorData.clone();
		orientData = m_orientData.clone();
	}

	if (m_midMaskData.data != NULL)
	{
		m_midMaskData.convertTo(alphaMaskData, CV_8UC1, 255.0f);
		maskData = m_midMaskData.clone();
	}
	else
	{
		alphaMaskData = m_maskData.clone();
		maskData = m_traceMaskData.clone();
	}

	depthData  = m_backDepthData.clone();
	HairUtil::orient2tensor(orientData, tensorData);
	colorParams.blurRadius = 2.0f;

	// Progress bar dialog
	progressDlg.setLabelText("Tracing back layers...");
	progressDlg.setRange(0, params.dsNumBackLayers);

	depthData += 0.9f * params.dsLayerThickness;
	for (int i = 0; i < params.dsNumBackLayers; i++)
	{
		progressDlg.setValue(i);

		// Decrease depth and shrink mask accordingly
		depthData -= params.dsLayerThickness;
		shrinkDenseDepthMask(depthData, m_midDepthData, DepthGreaterThan, 
							 params.dsMaskShrinkDelta, maskData);

		// Smooth orientation...
		GaussianBlur(tensorData, tensorData, Size(-1, -1), 0.5f);
		HairUtil::tensor2orient(tensorData, orientData);

		// Trace hair strands
		const int firstStrandIdx = pDstModel->numStrands();
		genDenseStrandsLayer(params, depthData, maskData, orientData, pDstModel);
		const int lastStrandIdx  = pDstModel->numStrands() - 1;

		// Sample hair color
		sampleStrandColor(colorData, alphaMaskData, colorParams, firstStrandIdx, lastStrandIdx, pDstModel);

		// Smooth & darken color
		GaussianBlur(colorData, colorData, Size(-1, -1), 1.0);
		colorData *= 0.9f;
		colorParams.blurRadius += 2.0f;

	}

	HairFilterParam filterParams;
	filterParams.sigmaDepth = 3.0f;
	pDstModel->filterDepth(filterParams);

	pDstModel->updateBuffers();
}


// Generate one single layer of dense strands.
void HairImage::genDenseStrandsLayer(const TracingParam&	params, 
									 const cv::Mat&			depthData, 
									 const cv::Mat&			maskData, 
									 const cv::Mat&			orientData,
									 HairStrandModel*		pStrandModel)
{
	Mat coverageData = Mat::zeros(depthData.size(), CV_8UC1);

	// Prepare seed points
	std::vector<Vec2i> initSeeds;

	for (int y = 0; y < depthData.rows; y++)
		for (int x = 0; x < depthData.cols; x++)
			if (maskData.at<float>(y, x) > FLT_MIN)
				initSeeds.push_back(Vec2i(x, y));

	if (params.randomSeedOrder)
		HairUtil::shuffleOrder(initSeeds);

	for (int i = 0; i < initSeeds.size(); i++)
	{
		int nX = initSeeds[i][0];
		int nY = initSeeds[i][1];

		// Calculate a per-strand random depth bias
		float depthBias = params.dsDepthVariation * params.dsLayerThickness * 
						  ((float)rand() / (float)RAND_MAX - 0.5f);

		// Skip locations with high coverage.
		if (coverageData.at<uchar>(nY, nX) >= params.dsMinCoverage)
			continue;

		float orient = orientData.at<float>(nY, nX);
		if (HairUtil::isOrientValid(orient) == false)
			continue;

		float x = (float)nX + 0.5f;
		float y = (float)nY + 0.5f;
		float z = HairUtil::sampleLinear<float>(depthData, x, y) + depthBias;

		StrandVertex vertex;
		vertex.position = XMFLOAT3(x, y, z);
		vertex.color	= XMFLOAT4(1, 0.7f, 0, 1);

		Strand strand;
		strand.appendVertex(vertex);

		Vec2f dir = HairUtil::orient2direction(orient);

		traceOneDirectionDense(params, depthData, maskData, orientData, coverageData,
							   TraceForward, dir, depthBias, &strand);
		traceOneDirectionDense(params, depthData, maskData, orientData, coverageData,
							   TraceBackward, -dir, depthBias, &strand);

		if (strand.numVertices() > 1)
		{
			updateTracedCoverage(Vec2f(x, y), params, coverageData);

			pStrandModel->addStrand(strand);
		}
	}
}


void HairImage::traceOneDirectionDense(const TracingParam& params, 
	const cv::Mat& depthData, const cv::Mat& maskData, 
	const cv::Mat& orientData, cv::Mat& coverageData, 
	const TraceMode mode, const cv::Vec2f initDir, 
	float depthVar, Strand* pStrand)
{
	Vec2f pos = (mode == TraceForward) ?
		Vec2f((const float*)&(pStrand->vertices()[pStrand->numVertices() - 1].position)) :
	Vec2f((const float*)&(pStrand->vertices()[0]));

	Vec2f dir = initDir;

	int health = params.dsHealthPoint;

	QQueue<Vec2f> lastPosArray;

	while (true)
	{
		// Calculate new position
		pos = pos + params.dsStepLen * dir;

		int nX = (int)pos[0];
		int nY = (int)pos[1];

		// Check termination conditions

		//if (!isInsideHairRegion(pos[0], pos[1]))
		//	break;
		if (nX < 0 || nX >= maskData.cols ||
			nY < 0 || nY >= maskData.rows)
			break;
		if (maskData.at<float>(nY, nX) <= FLT_MIN)
			break;
		if (health < 0)
			break;

		// Calculate tracing direction at new position
		Vec2f newDir = HairUtil::orient2direction(orientData.at<float>(nY, nX));
		float cosine = newDir.dot(dir);
		if (cosine < 0.0f)
		{
			newDir = -newDir;
			cosine = -cosine;
		}

		if (HairUtil::isDirectionValid(newDir) && 
			coverageData.at<uchar>(nY, nX) < params.dsMaxCoverage)
		{
			dir		 = newDir;
			health	 = params.dsHealthPoint;
		}
		else
		{
			health	 = health - 1;
		}

		// Add vertex to strand
		StrandVertex vertex;
		vertex.position.x = pos[0];
		vertex.position.y = pos[1];
		vertex.position.z = depthVar + HairUtil::sampleLinear<float>(depthData, pos[0], pos[1]);
		vertex.color = XMFLOAT4(1, 0.7f, 0, 1);

		if (mode == TraceForward)
			pStrand->appendVertex(vertex, params.dsStepLen);
		else if (mode == TraceBackward)
			pStrand->prependVertex(vertex, params.dsStepLen);

		// Update feature/coverage data at traced locations
		lastPosArray.enqueue(pos);
		if (lastPosArray.count() > 5)
			updateTracedCoverage(lastPosArray.dequeue(), params, coverageData);
	}

	// Update feature/coverage data at traced locations
	while (!lastPosArray.isEmpty())
		updateTracedCoverage(lastPosArray.dequeue(), params, coverageData);
}


void HairImage::updateTracedCoverage(const cv::Vec2f& pos, const TracingParam& params, cv::Mat& coverageData)
{
	int nX = (int)pos[0], nY = (int)pos[1];

	int fromX = max(nX - 2, 0);
	int toX   = min(nX + 2, m_featureData.cols - 1);
	int fromY = max(nY - 2, 0);
	int toY   = min(nY + 2, m_featureData.rows - 1);

	for (int y = fromY; y <= toY; y++)
	{
		for (int x = fromX; x <= toX; x++)
		{
			uchar& coverage = coverageData.at<uchar>(y, x);
			if (coverage < 255)
			{
				// Calculate coverage as a falloff from centerline
				float u = (float)x + 0.5f - pos[0];
				float v = (float)y + 0.5f - pos[1];

				float w = expf(-(u*u + v*v) / 
								(params.dsCoverageSigma*params.dsCoverageSigma));

				float newCoverage = min(255.0f, (float)coverage + w*params.dsCenterCoverage);

				coverage = (uchar)newCoverage;
			}
		}
	}
}



void HairImage::expandDepthData(Mat& depthData, Mat& maskData)
{
	Mat tempDepth  = depthData.mul(maskData);
	Mat tempWeight = maskData.clone();

	const float sigma = 1.0f;

	GaussianBlur(tempDepth, tempDepth, Size(-1, -1), sigma);
	GaussianBlur(tempWeight, tempWeight, Size(-1, -1), sigma);

	for (int y = 0; y < tempDepth.rows; y++)
	{
		for (int x = 0; x < tempDepth.cols; x++)
		{
			if (maskData.at<float>(y, x) > FLT_MIN)
				continue;

			float& d = tempDepth.at<float>(y, x);
			float& w = tempWeight.at<float>(y, x);

			if (w > FLT_MIN)
			{
				depthData.at<float>(y, x) = d / w;
			}
		}
	}
}


void HairImage::shrinkDenseDepthMask(const cv::Mat& srcDepth, const cv::Mat& dstDepth, 
	DepthCompareOperation op, float delta, cv::Mat& mask)
{
	for (int y = 0; y < srcDepth.rows; y++)
	{
		for (int x = 0; x < srcDepth.cols; x++)
		{
			float& m = mask.at<float>(y, x);

			if (m > FLT_MIN)
			{
				float srcD = srcDepth.at<float>(y, x);
				float dstD = dstDepth.at<float>(y, x);

				if ((op == DepthGreaterThan && dstD - srcD > delta) ||
					(op == DepthLessThan && srcD - dstD > delta))
				{
					m = 0;
				}
			}
		}
	}
}


// add a hacked depth falloff at hair silhouette.
void HairImage::addSilhouetteDepthFalloff(float radius, float depth, HairStrandModel* pStrandModel)
{
	Q_ASSERT(m_maskData.data);

	Mat distMap(m_maskData.size(), CV_8UC1);
	distanceTransform(m_maskData, distMap, CV_DIST_L2, CV_DIST_MASK_PRECISE);

	// Calculate a depth offset map
	for (int y = 0; y < distMap.rows; y++)
	{
		for (int x = 0; x < distMap.cols; x++)
		{
			float& d = distMap.at<float>(y, x);

			if (d <= 0.0f)
			{
				d = depth;
			}
			else if (d >= radius)
			{
				d = 0.0f;
			}
			else
			{
				float temp = CV_PI * d / radius;
				d = 0.5f * depth * (cosf(temp) + 1.0f);
			}
		}
	}

	// Debug...
	distMap.convertTo(m_debug2Data, CV_8UC1, 255.0f / depth);

	// Apply depth offset to strand vertices
	for (int i = 0; i < pStrandModel->numStrands(); i++)
	{
		Strand* strand = pStrandModel->getStrandAt(i);

		for (int j = 0; j < strand->numVertices(); j++)
		{
			XMFLOAT3& pos = strand->vertices()[j].position;

			pos.z += HairUtil::sampleLinear<float>(distMap, pos.x, pos.y);
		}
	}

	pStrandModel->updateBuffers();
}

void HairImage::calcTempSmoothOrientation(const PreprocessParam& prepParams,
	const OrientationParam& orientParams, 
	int nRefinIters, const ConfidenceParam& confidParams, const SmoothingParam& smoothParams)
{
	preprocessImage(prepParams, m_midColorData, m_prepData);

	calcOrientationGabor(orientParams, m_prepData, m_midOrientData);
	calcConfidence(confidParams);

	// Orientation refinement.
	for (int i = 0; i < nRefinIters; i++)
	{
		refineOrientation(orientParams, m_midOrientData);
		calcConfidence(confidParams);
	}

	// Smoothing
	smoothOrientationBlur(smoothParams, m_midOrientData, m_midOrientData);
}
	/*
{
	Q_ASSERT(!m_midColorData.empty());

	QProgressDialog progressDlg;
	progressDlg.setLabelText("Performing oriented filtering...");
	progressDlg.setRange(0, orientParams.numKernels);
	progressDlg.setModal(true);
	progressDlg.show();

	float phase		= orientParams.phase;
	float phaseStep = CV_PI * 2.0 / (float)orientParams.numPhases;

	m_midOrientData	= Mat::zeros(m_midColorData.size(), CV_32F);
	m_varianceData	= Mat::zeros(m_midColorData.size(), CV_32F);
	m_maxRespData	= Mat::zeros(m_midColorData.size(), CV_32F);

	// Array of responses to each orientation
	std::vector<Mat> respArray;

	// Try Gabor kernel of each orientation
	for (int iOrient = 0; iOrient < orientParams.numKernels; iOrient++)
	{
		// Create Gabor kernel
		Mat kernel;
		const float theta = CV_PI * (float)iOrient / (float)orientParams.numKernels;
		createGaborKernel(orientParams.kernelWidth, orientParams.kernelHeight, theta,
			orientParams.sigmaX, orientParams.sigmaY, orientParams.lambda,
			phase, kernel);

		// Filter and store response
		Mat response;
		filter2D(m_midColorData, response, CV_32F, kernel);

		respArray.push_back(response.clone());

		// Update progress bar
		progressDlg.setValue(iOrient);
	}

	// Statistics: calc per-pixel variance...
	for (int y = 0; y < m_midColorData.rows; y++)
	{
		for (int x = 0; x < m_midColorData.cols; x++)
		{
			// Find max response at the pixel
			float maxResp    = 0.0f;
			float bestOrient = 0.0f;

			for (int iOrient = 0; iOrient < orientParams.numKernels; iOrient++)
			{
				float& resp = respArray[iOrient].ptr<float>(y)[x];

				if (resp < 0.0f)
				{
					resp = 0.0f;
				}
				else if (resp > maxResp)
				{
					maxResp = resp;
					bestOrient = CV_PI * (float)iOrient / (float)orientParams.numKernels;
				}
			}

			// Calculate variance
			float variance = 0.0f;
			for (int iOrient = 0; iOrient < orientParams.numKernels; iOrient++)
			{
				float orient = CV_PI * (float)iOrient / (float)params.numKernels;
				float orientDiff = HairUtil::diffOrient(orient, bestOrient);

				float respDiff = respArray[iOrient].ptr<float>(y)[x] - maxResp;

				variance += orientDiff * respDiff * respDiff;
			}
			// Standard variance
			variance = sqrtf(variance);

			// Update overall variance/orientation if necessary
			if (variance > m_varianceData.ptr<float>(y)[x])
			{
				dst.ptr<float>(y)[x]   = bestOrient;
				m_varianceData.ptr<float>(y)[x] = variance;
				m_maxRespData.ptr<float>(y)[x]	= maxResp;
			}
		}
	}

	// Normalize variance and max response
	float maxAllResponse = 0.0f;
	float maxAllVariance = 0.0f;

	for (int y = 0; y < src.rows; y++)
	{
		for (int x = 0; x < src.cols; x++)
		{
			if (m_varianceData.ptr<float>(y)[x] > maxAllVariance)
				maxAllVariance = m_varianceData.ptr<float>(y)[x];

			if (m_maxRespData.ptr<float>(y)[x] > maxAllResponse)
				maxAllResponse = m_maxRespData.ptr<float>(y)[x];
		}
	}

	qDebug() << "Max response:" << maxAllResponse;
	qDebug() << "Max variance:" << maxAllVariance;

	m_maxRespData /= maxAllResponse;
	m_maxRespData.convertTo(m_debug1Data, CV_8U, 255.0f);

	m_varianceData /= maxAllVariance;
	m_varianceData.convertTo(m_debug2Data, CV_8U, 255.0f);

}*/