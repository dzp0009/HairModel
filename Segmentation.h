#pragma once

#include <QString>

class QImage;

bool SegmentAllAndSave(QImage* originalImage, QImage* strokeIdImage, QString outputPath, bool bMatting);

bool PerformHairMatting();

bool SegmentRegion(QImage* originalImage, QImage* guideImage, QImage* maskImage, QImage* resultImage);