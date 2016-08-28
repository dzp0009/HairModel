#include "HairLayers.h"

#include <QFileDialog>
#include <QFileInfo>

#include "SceneWidget.h"
#include "MyScene.h"
#include "HairStrandModel.h"
#include "HeadMorphMesh.h"
#include "HeadMorphRenderer.h"
#include "HairMorphRenderer.h"

#include "HairUtil.h"

#include "SimpleInterpolator.h"
#include "HairClusterer.h"
#include "HairHierarchy.h"

#include "HairMorphHierarchy.h"

#include "LxConsole.h"


//////////////////////////////////////////////////////////////////
// out-dated test cases
//////////////////////////////////////////////////////////////////

void testBonneelInterpolator()
{
	//printf("Starting...");

	//cv::Mat img1 = cv::imread("D:/interp_1.png", CV_LOAD_IMAGE_GRAYSCALE);
	//cv::Mat img2 = cv::imread("D:/interp_2.png", CV_LOAD_IMAGE_GRAYSCALE);

	//if (img1.size() != img1.size())
	//{
	//	printf("Images must have the same size!\n");
	//	return;
	//}

	//const int w = img1.cols, h = img1.rows;

	//std::vector<Vector<2,double> > samplesPos(w*h);
	//std::vector<double> values1(w*h, 0.);
	//std::vector<double> values2(w*h, 0.);
	//std::vector<double> valuesR(w*h, 0.);

	//cv::Mat mat1(img1.size(), CV_64FC1, values1.data());
	//cv::Mat mat2(img2.size(), CV_64FC1, values2.data());

	//img1.convertTo(mat1, CV_64FC1, 1.0/255.0);
	//img2.convertTo(mat2, CV_64FC1, 1.0/255.0);

	//for (int y = 0; y < h; y++)
	//	for (int x = 0; x < w; x++)
	//		samplesPos[y*w+x] = Vector<2,double>(x, y);

	//Interpolator<2,double> interp(samplesPos, values1,  // source distribution, in R^2 
	//	samplesPos, values2,							// target distribution
	//	sqrDistLinear, rbfFuncLinear, interpolateBinsLinear, // how to represent and move the particles
	//	2);												// particle spread : distance to 2nd nearest neighbor at each sample point
	//	
	//interp.precompute();


	//int N = 10; // intermediate steps
	//for (int p=0; p<N; p++)
	//{
	//	double alpha = p/((double)N-1.);
	//	interp.interpolate(alpha, samplesPos, valuesR);

	//	cv::Mat imgR(img1.size(), CV_64FC1, valuesR.data());
	//	imgR.convertTo(imgR, CV_8UC1, 255.0f);
	//	cv::imwrite(QString("result_%1.png").arg(p).toStdString(), imgR);
	//}
	//printf("DONE.\n");
}

void testSdfInterp()
{
	using namespace cv;

	Mat img1 = imread("D:/shape1.png", CV_LOAD_IMAGE_GRAYSCALE);
	Mat img2 = imread("D:/shape2.png", CV_LOAD_IMAGE_GRAYSCALE);

	// calculate SDF
	Mat sdf1, sdf2;
	HairUtil::calcSdfFromMask(img1, sdf1);
	HairUtil::calcSdfFromMask(img2, sdf2);

	Mat sdf, prevSdf, diff;

	//addWeighted(sdf1, 0.9, sdf2, 0.1, 1, sdf);
	//diff = sdf - sdf1;

	//HairUtil::debugSdf("prev", sdf1);
	//HairUtil::debugSdf("curr", sdf);
	//HairUtil::debugSdf("diff", diff*10.0f);

	int n = 9;
	for (int i = 0; i < n; i++)
	{
		float w = (float)i / (n-1);

		addWeighted(sdf1, 1.0f - w, sdf2, w, 1, sdf);

		if (i > 0)
		{
			diff = sdf - prevSdf;
			HairUtil::saveSdfAsRGB(QString("D:/diff_%1.png").arg(i), diff*10.0f);
		}
		prevSdf = sdf.clone();

		HairUtil::saveSdfAsRGB(QString("D:/sdf_%1.png").arg(i), sdf);
	}
}


// Current test function
void HairLayers::on_actionTest_triggered()
{
	m_scene->hairMorphHierarchy()->testSSSEffect();


	//HairHierarchy* pHierarchy = m_scene->hairMorphModel()->srcHierarchy();
	//pHierarchy->setNumLevels(3);
	//
	//HairStrandModel resamModel;
	//pHierarchy->level(0).resampleTo(1, 32, &resamModel);

	//HairClusterer clusterer;
	//clusterer.clusterK(resamModel, 100, pHierarchy->level(1));

	//pHierarchy->level(0) = resamModel;
	//pHierarchy->level(0).updateBuffers();

	//pHierarchy->level(1).updateBuffers();

	//pHierarchy->setCurrentLevelIndex(1);
}


