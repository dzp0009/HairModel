// A hair image that contains various data e.g. color/depth/orientation.

#pragma once

#include <opencv/cv.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <QString>
#include <QImage>
#include <QList>

#include "HairImageCommon.h"

class HairStrandModel;
class Strand;



// Class for Hair Image
class HairImage
{
public:
	HairImage();
	~HairImage();

	bool	loadColor			(QString filename);
	bool	loadHairColor		(QString filename);
	bool	loadMask			(QString filename);
	bool	loadFaceMask		(QString filename);
	bool	loadFaceMaskFromPts	(QString filename);
	bool	loadOrientationChai	(QString filename);
	bool	loadOrientationWang	(QString filename);
	bool	loadOrientationMask	(QString filename);
	bool	loadStrands			(QString filename);
	bool	saveStrandsSVG		(QString filename);

	bool	loadMidColor		(QString filename);
	bool	loadMidMask			(QString filename);

	bool	saveOrientation		(QString filename);
	bool	saveVariance		(QString filename);

	bool	saveMidOrientation	(QString filename);
	
	// Property getters.
	bool	isColorNull()	const	{ return m_colorData.data == NULL; }
	bool	isOrientNull()	const	{ return m_orientData.data == NULL; }

	bool	isMidColorNull()const	{ return m_midColorData.empty(); }
	bool	isMidMaskNull()	const	{ return m_midMaskData.empty(); }

	int		width()			const	{ return m_colorData.cols; }
	int		height()		const	{ return m_colorData.rows; }
	
	// Display image on screen
	void	drawImage		(const HairImageViewParam& params, QImage* dstImage) const;

	// Calculate orientation map from color input
	void	calcOrientation(const PreprocessParam& prepParams, 
							const OrientationParam& orientParams);

	void	calcConfidence(const ConfidenceParam& params);

	void	refineOrientation(const OrientationParam& orientParams);
	void	refineOrientation(const OrientationParam& orientParams, cv::Mat& dst);

	void	smoothOrientation(const SmoothingParam& params);

	void	detectLineFeatures(const FeatureDetectParam& params);

	void	sampleStrandColor(const SampleColorParam& params, HairStrandModel* pStrandModel);

	void	calcTriDepthMaps(const TracingParam& params, HairStrandModel* pStrandModel);

	void	calcFrontDepth(const TracingParam& params, HairStrandModel* pStrandModel);
	void	calcMidDepth(const cv::Mat& borderWeight, const cv::Mat& maskData, cv::Mat& depthData);
	void	calcBackDepth(float radius, float depth, float scale, float offset, const cv::Mat& maskData);

	//----------------------------------------------------
	// Hair tracing
	//----------------------------------------------------

	void	traceSparseStrands(const TracingParam& params, HairStrandModel* pStrandModel);
	void	traceSingleSparseStrand(float x, float y, const TracingParam& params, Strand* pStrand);

	void	traceAuxStrands(const TracingParam& params, HairStrandModel* pStrandModel);
	void	traceSingleAuxStrand(float x, float y, const TracingParam& params, Strand* pStrand);

	void	clearCoverage();

	void	genDenseStrands(const TracingParam& params, HairStrandModel* pSrcModel, HairStrandModel* pDstModel);

	void	genDenseStrandsLayer(const TracingParam& params, const cv::Mat& depthData, 
								 const cv::Mat& maskData, const cv::Mat& orientData,
								 HairStrandModel* pStrandModel);

	//----------------------------------------------------
	// Depth hack
	//----------------------------------------------------

	void	addSilhouetteDepthFalloff(float radius, float depth, HairStrandModel* pStrandModel);

	//----------------------------------------------------
	// Volumetric hair synthesis
	//----------------------------------------------------

	void	genBackHalfStrandsMirror(HairStrandModel* pStrandModel);

	// rush funcs

	void	calcTempSmoothOrientation(const PreprocessParam& prepParams, const OrientationParam& orientParams, int nRefinIters, const ConfidenceParam& confidParams, const SmoothingParam& smoothParams);

	//----------------------------------------------------
	// For Debugging
	//----------------------------------------------------

	void	calcOrientationAt(int x, int y, const PreprocessParam& prepParams, 
		const OrientationParam& orientParams,
		float* pOrient, float* pResponseArray);

	//----------------------------------------------------
	// Out of date
	//----------------------------------------------------

	QString	colorFilename() const	{ return m_colorFilename; }

	void	traceStrandsByFeatures(const TracingParam& params, HairStrandModel* pStrandModel);
	void	traceSingleStrandByFeature(float x, float y, const TracingParam& params, Strand* pStrand);

	// Trace hair strands from orientation map
	void	traceStrandsOld(const TracingParam& params, HairStrandModel* pStrandModel);
	void	traceSingleStrandOld(float x, float y, const TracingParam& params, Strand* pStrand);

signals:

	void	operationStarted(const QString&, int);
	void	operationProgressed(int);

private:

	enum TraceMode
	{
		TraceForward,
		TraceBackward,
	};

	void	traceOneDirectionSparse(const TracingParam& params, const TraceMode mode, const cv::Vec2f initDir, Strand* pStrand);
	void	traceOneDirectionAux(const TracingParam& params, const TraceMode mode, const cv::Vec2f dir, Strand* pStrand);
	void	traceOneDirectionDense(const TracingParam& params, const cv::Mat& depthData, 
								   const cv::Mat& maskData, const cv::Mat& orientData,
								   cv::Mat& coverageData, const TraceMode mode, 
								   const cv::Vec2f dir, float depthVar, Strand* pStrand);


	cv::Vec2f	calcTraceDirection(const cv::Vec2f& pos, const cv::Vec2f& prevDir, float* pDiffAngle);
	
	cv::Vec2f	estimateTraceDirection(const cv::Vec2f& pos, const cv::Vec2f& prevDir, float* pDiffAngle);
	
	void	updateTracedPixels(const cv::Vec2f& pos, const TracingParam& params);

	void	updateTracedCoverage(const cv::Vec2f& pos, const TracingParam& params, cv::Mat& coverageData);

	// Correct tracing position to nearest ridge with sub-pixel accuracy.
	cv::Vec2f	correctPosToRidge(const cv::Vec2f& pos, const TracingParam& params);

	void	getColorAt(float x, float y, float* pRgba);
	float	getOrientationAt(float x, float y);
	float	getConfidenceAt(float x, float y);
	uchar	getCoverageAt(float x, float y);

	//float	sample8ULinear(float x, float y, const cv::Mat& src);

	//template<typename T>
	//T		sampleLinear(float x, float y, const cv::Mat_<T>& src);

	bool	isInsideHairRegion(float x, float y);

	void	preprocessImage(const PreprocessParam& params, const cv::Mat& src, cv::Mat& dst);

	void	calcOrientationCanny(const OrientationParam& orientParams, const cv::Mat& src);
	void	calcOrientationGabor(const OrientationParam& orientParams, const cv::Mat& src, cv::Mat& dst);

	void	smoothOrientationMultilateral(const SmoothingParam& params);
	void	smoothOrientationConfidDiffuse(const SmoothingParam& params);
	void	smoothOrientationBlur(const SmoothingParam& params, const cv::Mat& srcOrient, cv::Mat& dstOrient);

	template<typename T>
	T		calcJakobM(int x, int y, float offset, const cv::Mat_<T>& src);

	// Precompute kernels
	cv::Mat_<float>	createGaussianKernel(float sigma);
	void	createGaborKernel(int width, int height, float theta, float sigmaX, 
							  float sigmaY, float lambda, float phase, cv::Mat& kernel);

	void	drawColor		(const HairImageViewParam& params, QImage* dstImage) const;
	void	drawOrientation (const HairImageViewParam& params, QImage* dstImage) const;
	void	drawConfidence	(const HairImageViewParam& params, QImage* dstImage) const;
	void	drawDepth		(const HairImageViewParam& params, QImage* dstImage) const;

	void	draw8UDataToQImage(const cv::Mat& src, QImage* dst) const;

	void	relaxSeeds(cv::Mat& seeds, float density, float relaxForce);

	void	sampleStrandColor(const cv::Mat& colorData, const cv::Mat& maskData, const SampleColorParam& params, 
							  const int firstIdx, const int lastIdx, HairStrandModel* pStrandModel);

	void	expandDepthData(cv::Mat& depthData, cv::Mat& maskData);

	enum DepthCompareOperation
	{
		DepthGreaterThan,
		DepthLessThan,
	};
	void	shrinkDenseDepthMask(const cv::Mat& srcDepth, const cv::Mat& dstDepth, DepthCompareOperation op,
								 float delta, cv::Mat& mask);



	//----------------------------------------------------
	// Out of date
	//----------------------------------------------------

	void	traceOneDirection(const TracingParam& params, const TraceMode mode, const cv::Vec2f dir, Strand* pStrand);

private:
	cv::Mat		m_colorData;		// original image
	cv::Mat		m_hairColorData;	// which provides foreground hair color
	cv::Mat		m_prepData;
	cv::Mat		m_maskData;
	cv::Mat		m_faceMaskData;
	cv::Mat		m_orientData;		// orientation angle (0 ~ pi)
	cv::Mat		m_confidenceData;	// orientation confidence map
	cv::Mat		m_varianceData;		// variance
	cv::Mat		m_maxRespData;		// max response
	cv::Mat		m_tensorData;		// orientation tensor
	cv::Mat		m_featureData;		// detected line feature mask
	cv::Mat		m_coverageData;		// coverage of traced hair strands
	cv::Mat		m_frontDepthData;
	cv::Mat		m_midDepthData;
	cv::Mat		m_backDepthData;
	cv::Mat		m_traceMaskData;	// mask calc from traced strands
	cv::Mat		m_debug1Data;
	cv::Mat		m_debug2Data;

	cv::Mat		m_midColorData;
	cv::Mat		m_midMaskData;
	cv::Mat		m_midOrientData;

	QString		m_colorFilename;
};

