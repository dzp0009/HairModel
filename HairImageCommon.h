// Common structures and enumerations in HairImage

#ifndef HAIR_IMAGE_COMMON_H
#define HAIR_IMAGE_COMMON_H


//////////////////////////////////////////////////////////////////

enum PreprocessMethod
{
	PreprocessNone,
	PreprocessGaussian,
	PreprocessDoG,
	PreprocessBilateral,
};

// Image preprocessing parameters
struct PreprocessParam
{
	PreprocessMethod	preprocessMethod;
	float				sigmaHigh;
	float				sigmaLow;
};

//////////////////////////////////////////////////////////////////

enum OrientationMethod
{
	OrientationCanny,
	OrientationGabor,
	OrientationMultiGabor,
};

enum GradientType
{
	GradientSobel,
	GradientScharr,
};

// Method used to compute confidence map
enum ConfidenceMethod
{
	ConfidenceMyVariance,
	ConfidenceMaxResponse,
};

// Orientation computing parameters
struct OrientationParam
{
	OrientationMethod	method;
	GradientType		gradType;

	// Oriented kernels
	int					numKernels;
	int					kernelWidth;
	int					kernelHeight;

	// Gabor params
	int					numPhases;
	float				sigmaX;
	float				sigmaY;
	float				lambda;
	float				phase;
	bool				saveKernels;
};

struct ConfidenceParam
{
	ConfidenceMethod	confidMethod;
	float				clampConfidLow;
	float				clampConfidHigh;
};

//////////////////////////////////////////////////////////////////

// Orientation smoothing method
enum SmoothingMethod
{
	SmoothingMultilateral,
	SmoothingConfidDiffuse,
	SmoothingBlur,
};

struct SmoothingParam
{
	SmoothingMethod method;

	// Multilateral params
	bool	useSpatial;
	bool	useOrientation;
	bool	useColor;
	bool	useConfidence;

	float	sigmaSpatial;
	float	sigmaOrientation;
	float	sigmaColor;
	float	sigmaConfidence;

	bool	useVarClamp;
	float	varClampThreshold;

	bool	useMaxRespClamp;
	float	maxRespClampThreshold;

	// Confidence diffusion params
	float	confidThreshold;
	int		diffuseIterations;
};

//////////////////////////////////////////////////////////////////

struct TracingParam
{
	// Relaxation-based seed generation
	float	seedDensity;
	int		numRelaxations;
	float	relaxForce;

	// General tracing params
	float	stepLength;
	float	maxLength;
	float	minLength;
	float	maxBendAngle;

	// Hysteresis thresholding
	float	highConfidence;
	float	lowConfidence;

	// Hysteresis terminating
	int		healthPoint;
	int		lateTermDirSmooth;

	// Feature mask updating
	bool	randomSeedOrder;
	bool	lengthClamping;

	// Correction to ridge
	bool	correctToRidge;
	float	ridgeRadius;
	float	maxCorrection;

	// Coverage update
	float	centerCoverage;
	float	coverageSigma;

	// Auxiliary strands
	int		minAuxCoverage;
	int		maxAuxCoverage;

	// Dense strands
	int		dsNumFrontLayers;
	int		dsNumBackLayers;
	float	dsLayerThickness;
	float	dsDepthMapSigma;
	float	dsStepLen;
	int		dsHealthPoint;
	float	dsCenterCoverage;
	float	dsCoverageSigma;
	int		dsMinCoverage;
	int		dsMaxCoverage;
	float	dsDepthVariation;
	float	dsSilhoutteRadius;
	float	dsSilhoutteDepth;
	bool	dsLengthClamping;
	float	dsMinLength;
	float	dsMaskShrinkDelta;

	// Extra params
	float	esBackHalfScale;
	float	esBackHalfOffset;

	int		esOrientRefinIters;

	int		esNumMidLayers;
	//QString	esMidColorFile;
	//QString esMidMaskFile;

	//OrientationParam esOrientParams;
};

//////////////////////////////////////////////////////////////////

struct FeatureDetectParam
{
	float	neighborOffset;
	float	ridgeDiffThreshold;
	float	highConfidence;
	float	lowConfidence;
};

//////////////////////////////////////////////////////////////////


struct SampleColorParam
{
	int		blurRadius;
	bool	sampleAlpha;
};

//////////////////////////////////////////////////////////////////

enum HairImageView
{
	HairImageColor = 0,
	HairImageOrient,
	HairImageConfidence,
	HairImageCoverage,
	HairImageDepth,
	HairImageDebug1,
	HairImageDebug2,
};


struct HairImageViewParam
{
	HairImageView	viewMode;

	bool	toggleOrigPrep;
	bool	toggleOrientTensor;
	bool	showMaskOpacity;
	bool	showMaxRespOpacity;
	bool	clampConfidence;

	// Threshold for confidence clamping
	float	lowConfidence;
	float	highConfidence;
};

#endif