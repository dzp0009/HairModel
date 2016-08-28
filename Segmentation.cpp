#include "Segmentation.h"

#include "QDXUT.h"
#include <QImage>
#include <QMessageBox>

#include "Segmentation/MeanShift/msImageProcessor.h"
#include "Segmentation/graphCut/GraphCut.h"
#include "Segmentation/matting/HairCut.h"

bool SegmentAllAndSave(QImage* originalImage, QImage* strokeIdImage, QString outputPath, bool bMatting)
{
	if (!originalImage || !strokeIdImage || 
		originalImage->isNull() || strokeIdImage->isNull())
		return false;

	if (originalImage->size() != strokeIdImage->size())
		return false;

	// Segment face region
	
	QImage guideImage(originalImage->size(), QImage::Format_ARGB32);
	QRgb foreValue = qRgba(255, 0, 0, 255);
	QRgb midValue  = qRgba(0, 0, 0, 0);
	QRgb backValue = qRgba(0, 0, 255, 255);

	// Generate guide image for face region
	for (int y = 0; y < guideImage.height(); y++)
	{
		for (int x = 0; x < guideImage.width(); x++)
		{
			int strokeId = qRed(strokeIdImage->pixel(x, y));

			if (strokeId == 2 /*Face*/)
				guideImage.setPixel(x, y, foreValue);
			else if (strokeId == 0)
				guideImage.setPixel(x, y, midValue);
			else
				guideImage.setPixel(x, y, backValue);
		}
	}

	QImage maskImage(originalImage->size(), QImage::Format_ARGB32);
	QImage resultImage(originalImage->size(), QImage::Format_ARGB32);

	if (!SegmentRegion(originalImage, &guideImage, &maskImage, &resultImage))
	{
		QMessageBox::critical(NULL, "Error", "Failed to segment face region.");
		return false;
	}

	// Save result
	resultImage.save(outputPath + "segment_face.png");
	
	// Segment hair region

	// Generate guide image for hair region
	for (int y = 0; y < guideImage.height(); y++)
	{
		for (int x = 0; x < guideImage.width(); x++)
		{
			int strokeId = qRed(strokeIdImage->pixel(x, y));

			if (strokeId == 1 /*Hair*/)
				guideImage.setPixel(x, y, foreValue);
			else if (strokeId == 0)
				guideImage.setPixel(x, y, midValue);
			else
				guideImage.setPixel(x, y, backValue);
		}
	}

	if (!SegmentRegion(originalImage, &guideImage, &maskImage, &resultImage))
	{
		QMessageBox::critical(NULL, "Error", "Failed to segment hair region.");
		return false;
	}

	resultImage.save(outputPath + "raw_hair.png");

	// Matting
	if (bMatting == false)
	{
		resultImage.save(outputPath + "segment_hair.png");
		return true;
	}

	bool initState = HairCutInitialize();
	mwArray* alphaArray = new mwArray;
	mwArray* foregroundArray = new mwArray;
	mwArray* backgroundArray = new mwArray;

	int imageWidth = originalImage->width();
	int imageHeight = originalImage->height();

	mwSize dims[3] = {imageHeight, imageWidth, 3};
	mwArray* imageArray = new mwArray(3, dims, mxDOUBLE_CLASS);
	mwArray* guideMaskArray = new mwArray(imageHeight, imageWidth, mxLOGICAL_CLASS);
	mwArray* guideValueArray = new mwArray(imageHeight, imageWidth, mxLOGICAL_CLASS);
	double *imageData = new double[imageHeight * imageWidth * 3];
	mxLogical *guideMaskData = new mxLogical[imageHeight * imageWidth];
	mxLogical *guideValueData = new mxLogical[imageHeight * imageWidth];
	for(int heightI = 0; heightI < imageHeight; heightI++){
		for(int widthI = 0; widthI < imageWidth; widthI++){
			QRgb imageColor = originalImage->pixel(widthI, heightI);
			QRgb guideColor = guideImage.pixel(widthI, heightI);
			imageData[widthI * imageHeight + heightI] = qRed(imageColor) / 255.;
			imageData[widthI * imageHeight + heightI + imageWidth * imageHeight] = qGreen(imageColor) / 255.;
			imageData[widthI * imageHeight + heightI + imageWidth * imageHeight * 2] = qBlue(imageColor) / 255.;
			if(qAlpha(guideColor) != 0){
				guideMaskData[widthI * imageHeight + heightI] = true;
				if(qRed(guideColor) == 255)
					guideValueData[widthI * imageHeight + heightI] = true;
				else
					guideValueData[widthI * imageHeight + heightI] = false;
			}
			else{
				guideMaskData[widthI * imageHeight + heightI] = false;
				guideValueData[widthI * imageHeight + heightI] = false;
			}
		}
	}
	imageArray->SetData(imageData, imageHeight * imageWidth * 3);
	guideMaskArray->SetLogicalData(guideMaskData, imageHeight * imageWidth);
	guideValueArray->SetLogicalData(guideValueData, imageHeight * imageWidth);
	delete[] imageData;
	delete[] guideMaskData;
	delete[] guideValueData;

	QImage foreImage(originalImage->size(), QImage::Format_ARGB32);
	runMatting(3, *alphaArray, *foregroundArray, *backgroundArray,
		*imageArray, *guideMaskArray, *guideValueArray);
	double *alphaData = new double[imageHeight * imageWidth];
	double *foregroundData = new double[imageHeight * imageWidth * 3];
	double *backgroundData = new double[imageHeight * imageWidth * 3];
	alphaArray->GetData(alphaData, imageHeight * imageWidth);
	foregroundArray->GetData(foregroundData, imageHeight * imageWidth * 3);
	backgroundArray->GetData(backgroundData, imageHeight * imageWidth * 3);
	for(int heightI = 0; heightI < imageHeight; heightI++){
		for(int widthI = 0; widthI < imageWidth; widthI++){
			double alphaValue = MAX(MIN(alphaData[widthI * imageHeight + heightI], 1.), 0.);
			double redValue = MAX(MIN(foregroundData[widthI * imageHeight + heightI], 1.), 0.);
			double greenValue = MAX(MIN(foregroundData[widthI * imageHeight + heightI + imageWidth * imageHeight], 1.), 0.);
			double blueValue = MAX(MIN(foregroundData[widthI * imageHeight + heightI + imageWidth * imageHeight * 2], 1.), 0.);
			QRgb imageColor = qRgba(
				(int)(redValue * 255.),
				(int)(greenValue * 255.),
				(int)(blueValue * 255.),
				(int)(alphaValue * 255.));
			foreImage.setPixel(widthI, heightI, imageColor);
		}
	}
	delete[] alphaData;
	delete[] foregroundData;
	delete[] backgroundData;

	foreImage.save(outputPath + "cut_region.png");

	return true;
}


bool PerformHairMatting()
{
	return true;
}


bool SegmentRegion(QImage* originalImage, QImage* guideImage, QImage* maskImage, QImage* resultImage)
{
	// segment into super pixel:

	// Prepare raw image array.
	int imageWidth = originalImage->width();
	int imageHeight = originalImage->height();
	byte *imageData = new byte[imageWidth * imageHeight * 3];
	byte *resultData = new byte[imageWidth * imageHeight * 3];
	for(int widthI = 0; widthI < imageWidth; widthI++){
		for(int heightI = 0; heightI < imageHeight; heightI++){
			int offset = (heightI * imageWidth + widthI) * 3;
			QRgb colorValue = originalImage->pixel(widthI, heightI);
			imageData[offset] = (uchar)qRed(colorValue);
			imageData[offset + 1] = (uchar)qGreen(colorValue);
			imageData[offset + 2] = (uchar)qBlue(colorValue);
		}
	}

	// Do mean-shift.
	msImageProcessor finalSegmenter;

	int *finalRegionArray;
	float *finalModeArray;
	int *finalMpcArray;
	finalSegmenter.DefineImage(imageData, COLOR, imageHeight, imageWidth);
	finalSegmenter.Segment(1, 0.5, 10, HIGH_SPEEDUP);
	finalSegmenter.GetResults(resultData);
	
	int finalSegmentNumber = finalSegmenter.GetRegions(&finalRegionArray, &finalModeArray, &finalMpcArray);

	// Calculate segments neighbor info.
	int *finalSegmentSizeArray = new int[finalSegmentNumber];
	int *finalSegmentIndexArray = new int[imageWidth * imageHeight];
	QRgb *finalSegmentColorArray = new QRgb[finalSegmentNumber];
	int *finalSegmentNeighborSizeArray = new int[finalSegmentNumber];
	QVector<int> *finalSegmentNeighborIndexArray = new QVector<int>[finalSegmentNumber];
	QVector<double> *finalSegmentNeighborLengthArray = new QVector<double>[finalSegmentNumber];
	int *segmentGuideArray = new int[finalSegmentNumber];
	double *segmentWeightArray = new double[finalSegmentNumber];
	for(int widthI = 0; widthI < imageWidth; widthI++){
		for(int heightI = 0; heightI < imageHeight; heightI++){
			int offset = heightI * imageWidth + widthI;
			finalSegmentIndexArray[offset] = finalRegionArray[offset];
		}
	}
	for(int segmentI = 0; segmentI < finalSegmentNumber; segmentI++)
		finalSegmentSizeArray[segmentI] = finalMpcArray[segmentI];
	for(int segmentI = 0; segmentI < finalSegmentNumber; segmentI++)
		finalSegmentNeighborSizeArray[segmentI] = -1;
	for(int widthI = 0; widthI < imageWidth; widthI++){
		for(int heightI = 0; heightI < imageHeight; heightI++){
			int offset = heightI * imageWidth + widthI;
			int segmentIndex = finalSegmentIndexArray[offset];
			if(finalSegmentNeighborSizeArray[segmentIndex] == 0)
				continue;
			QRgb colorValue = qRgba(resultData[offset * 3], resultData[offset * 3 + 1], resultData[offset * 3 + 2], 255);
			finalSegmentColorArray[segmentIndex] = colorValue;
			finalSegmentNeighborSizeArray[segmentIndex] = 0;
		}
	}
	double SQRT2RE = 0.707106781187;
	for(int widthI = 0; widthI < imageWidth; widthI++){
		for(int heightI = 0; heightI < imageHeight; heightI++){
			int offset = heightI * imageWidth + widthI;
			int segmentIndex = finalSegmentIndexArray[offset];
			for(int neighborI = 0; neighborI < 4; neighborI++){
				int widthD, heightD;
				double neighborWeight;
				switch(neighborI){
				case 0:
					widthD = -1;
					heightD = 1;
					neighborWeight = SQRT2RE;
					break;
				case 1:
					widthD = 0;
					heightD = 1;
					neighborWeight = 1.;
					break;
				case 2:
					widthD = 1;
					heightD = 1;
					neighborWeight = SQRT2RE;
					break;
				case 3:
					widthD = 1;
					heightD = 0;
					neighborWeight = 1.;
					break;
				}
				int neighborWidthI = widthI + widthD;
				int neighborHeightI = heightI + heightD;
				if(neighborWidthI < 0 || neighborWidthI >= imageWidth || neighborHeightI < 0 || neighborHeightI >= imageHeight)
					continue;
				int neighborOffset = neighborHeightI * imageWidth + neighborWidthI;
				int neighborIndex = finalSegmentIndexArray[neighborOffset];
				if(neighborIndex != segmentIndex){
					int neighborI;
					for(neighborI = 0; neighborI < finalSegmentNeighborSizeArray[segmentIndex]; neighborI++){
						if(finalSegmentNeighborIndexArray[segmentIndex].at(neighborI) == neighborIndex){
							finalSegmentNeighborLengthArray[segmentIndex][neighborI] += neighborWeight;
							break;
						}
					}
					if(neighborI == finalSegmentNeighborSizeArray[segmentIndex]){
						finalSegmentNeighborSizeArray[segmentIndex]++;
						finalSegmentNeighborIndexArray[segmentIndex].append(neighborIndex);
						finalSegmentNeighborLengthArray[segmentIndex].append(neighborWeight);
					}
					for(neighborI = 0; neighborI < finalSegmentNeighborSizeArray[neighborIndex]; neighborI++){
						if(finalSegmentNeighborIndexArray[neighborIndex].at(neighborI) == segmentIndex){
							finalSegmentNeighborLengthArray[neighborIndex][neighborI] += neighborWeight;
							break;
						}
					}
					if(neighborI == finalSegmentNeighborSizeArray[neighborIndex]){
						finalSegmentNeighborSizeArray[neighborIndex]++;
						finalSegmentNeighborIndexArray[neighborIndex].append(segmentIndex);
						finalSegmentNeighborLengthArray[neighborIndex].append(neighborWeight);
					}
				}
			}
		}
	}
	delete[] imageData;
	delete[] resultData;
	delete[] finalRegionArray;
	delete[] finalModeArray;
	delete[] finalMpcArray;

	// Graphcut:

	// Init graphcut instance.
	GraphCut* finalGraphCut = new GraphCut(finalSegmentNumber,
		finalSegmentSizeArray,
		finalSegmentColorArray,
		finalSegmentNeighborSizeArray,
		finalSegmentNeighborIndexArray,
		finalSegmentNeighborLengthArray);

	// Calculate per-segment guide value.
	for(int segmentI = 0; segmentI < finalSegmentNumber; segmentI++)
		segmentGuideArray[segmentI] = 0;
	for(int widthI = 0; widthI < imageWidth; widthI++){
		for(int heightI = 0; heightI < imageHeight; heightI++){
			int offset = heightI * imageWidth + widthI;
			QRgb guideColor = guideImage->pixel(widthI, heightI);
			int guideValue;
			if(qAlpha(guideColor)){
				if(qRed(guideColor))
					guideValue = 1;
				else
					guideValue = -1;
			}
			else
				guideValue = 0;
			int segmentIndex = finalSegmentIndexArray[offset];
			segmentGuideArray[segmentIndex] += guideValue;
		}
	}
	for(int segmentI = 0; segmentI < finalSegmentNumber; segmentI++){
		double avgGuide = (double)segmentGuideArray[segmentI] / finalSegmentSizeArray[segmentI];
		int guideValue = 0;
		if(avgGuide > 0)
			guideValue = 1;
		else if(avgGuide < 0)
			guideValue = -1;
		segmentGuideArray[segmentI] = guideValue;
	}

	// Do graph cut.
	finalGraphCut->setGuide(segmentGuideArray);
	finalGraphCut->refine();
	int *alphaArray = finalGraphCut->getSegment();

	// Get result mask.
	for(int widthI = 0; widthI < imageWidth; widthI++){
		for(int heightI = 0; heightI < imageHeight; heightI++){
			int offset = heightI * imageWidth + widthI;
			int segmentIndex = finalSegmentIndexArray[offset];
			int alphaValue = alphaArray[segmentIndex];
			maskImage->setPixel(widthI, heightI, qRgba(255 * alphaValue, 0, 0, 255));
		}
	}

	// Get result image.
	for(int heightI = 0; heightI < imageHeight; heightI++){
		for(int widthI = 0; widthI < imageWidth; widthI++){
			if(qRed(maskImage->pixel(widthI, heightI)))
				resultImage->setPixel(widthI, heightI, originalImage->pixel(widthI, heightI));
			else
				resultImage->setPixel(widthI, heightI, qRgba(0, 0, 0, 0));
		}
	}

	return true;
}