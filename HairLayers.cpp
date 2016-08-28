#include "HairLayers.h"

#include <QFileDialog>
#include <QColorDialog>
#include <QMessageBox>
#include <QSettings>
#include <QKeyEvent>
#include <QProcess>
#include <QFileInfo>
#include <QDebug>

#include "QDXCamera.h"
#include "QDXImageSprite.h"

#include "SceneWidget.h"
#include "MyScene.h"
#include "HairStrandModel.h"
#include "HeadRenderer.h"
#include "BodyModel.h"
#include "StrokesSprite.h"

#include "HairHierarchy.h"
#include "HeadMorphMesh.h"
#include "HairMorphHierarchy.h"
#include "HeadMorphRenderer.h"
#include "HairMorphRenderer.h"

#include "HairUtil.h"

#include "SimpleInterpolator.h"
#include "qtpolygon.h"

#include "LxConsole.h"

//struct lemon::Invalid const lemon::INVALID;

//////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////

HairLayers::HairLayers(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	lvdiw::LxCreateConsole(L"Debug Console");

	QDXUT::initialize();

	ui.setupUi(this);

	printf("Initializing scene...");
	m_scene = new MyScene;
	m_scene->create();
	printf("DONE.\n");

	m_sceneWidget = new SceneWidget(m_scene);
	ui.verticalLayout_27->addWidget(m_sceneWidget);

	m_polyWidget = new PolygonWidget(0, NULL);
	m_polyWidget->setFixedHeight(192);
	ui.groupBoxNWayMorph->layout()->addWidget(m_polyWidget);

	loadSettings();

	connect(m_sceneWidget->scene(), SIGNAL(imageDoubleClicked(float,float)),
			this, SLOT(on_hairImage_doubleClicked(float,float)));
	connect(m_sceneWidget, SIGNAL(fpsUpdated(float)), this, SLOT(displayFps(float)));
	connect(m_polyWidget, SIGNAL(weightsChanged(const std::vector<float>&)),
			this, SLOT(morphWeightsChanged(const std::vector<float>&)));

	m_scene->strokesSprite()->create(1000, 1000);

	m_sceneWidget->startTimer(10);
}

HairLayers::~HairLayers()
{
	SAFE_DELETE(m_scene);

	saveSettings();

	QDXUT::cleanup();

	lvdiw::LxFreeConsole();
}


void HairLayers::loadSettings()
{
	QSettings s("HairLayers.ini", QSettings::IniFormat);

	restoreGeometry(s.value("WindowGeometry").toByteArray());
	ui.toolBox->setCurrentIndex(s.value("ToolBoxIndex", 0).toInt());
	ui.tabWidget->setCurrentIndex(s.value("TabWidgetIndex", 0).toInt());

	m_sceneWidget->setBackgroundColor(s.value("SceneBackgroundColor", 0).toUInt());

	// Default paths
	m_scnInputPath	= s.value("SceneInputPath", "").toString();
	m_imgInputPath  = s.value("ImageInputPath", "").toString();
	m_imgOutputPath = s.value("ImageOutputPath", "").toString();
	m_hairInputPath = s.value("HairInputPath", "").toString();
	m_hairOutputPath= s.value("HairOutputPath", "").toString();
	m_faceRecProgPath  = s.value("FaceRecProgramPath", "").toString();
	m_depthEstProgPath = s.value("DepthEstProgramPath", "").toString();

	// Preprocessing settings
	ui.comboPrepMethod->setCurrentIndex(s.value("PreprocessMethod", 0).toInt());
	ui.spinBoxPrepSigmaH->setValue(s.value("PrepSigmaH", 1.0).toDouble());
	ui.spinBoxPrepSigmaL->setValue(s.value("PrepSigmaL", 16.0).toDouble());

	// Orientation settings
	ui.comboOrientMethod->setCurrentIndex(s.value("OrientationMethod", 1).toInt());
	ui.spinBoxNumKernels->setValue(s.value("NumKernels", 16).toInt());
	ui.spinBoxKernelWidth->setValue(s.value("KernelWidth", 15).toInt());
	ui.spinBoxKernelHeight->setValue(s.value("KernelHeight", 15).toInt());
	ui.spinBoxNumAutoRefineOrient->setValue(s.value("AutoRefineOrient", 0).toInt());
	
	// Gabor kernel settings
	ui.spinBoxNumPhases->setValue(s.value("NumPhases", 1).toInt());
	ui.spinBoxGaborSigmaX->setValue(s.value("GaborSigmaX", 1.6).toDouble());
	ui.spinBoxGaborSigmaY->setValue(s.value("GaborSigmaY", 3).toDouble());
	ui.spinBoxGaborLambda->setValue(s.value("GaborLambda", 3).toDouble());
	ui.spinBoxPhase->setValue(s.value("GaborPhase", 0).toDouble());
	ui.checkBoxSaveKernels->setChecked(s.value("SaveKernels", false).toBool());

	// Confidence
	ui.comboConfidMethod->setCurrentIndex(s.value("ConfidenceMethod", 0).toInt());
	ui.spinBoxClampConfidLow->setValue(s.value("ClampConfidLow", 0.0).toDouble());
	ui.spinBoxClampConfidHigh->setValue(s.value("ClampConfidHigh", 1.0).toDouble());

	// Orientation smoothing
	ui.comboSmoothMethod->setCurrentIndex(s.value("SmoothMethod", 0).toInt());
	ui.spinBoxConfidThreshold->setValue(s.value("SmoothConfidThres",	 0.5).toDouble());
	ui.spinBoxDiffuseIterations->setValue(s.value("SmoothDiffuseIters",	1).toInt());
	ui.checkBoxSmoothSpatial->setChecked(s.value("SmoothSpatial", true).toBool());
	ui.checkBoxSmoothOrient->setChecked(s.value("SmoothOrientation", true).toBool());
	ui.checkBoxSmoothColor->setChecked(s.value("SmoothColor", true).toBool());
	ui.checkBoxSmoothConfidence->setChecked(s.value("SmoothConfidence", true).toBool());
	ui.spinBoxSigmaSpatial->setValue(s.value("SigmaSpatial", 2.0).toDouble());
	ui.spinBoxSigmaOrient->setValue(s.value("SigmaOrientation", 0.5).toDouble());
	ui.spinBoxSigmaColor->setValue(s.value("SigmaColor", 16.0).toDouble());
	ui.spinBoxSigmaConfidence->setValue(s.value("SigmaConfidence", 0.3).toDouble());

	// Feature detection
	ui.spinBoxNbrOffset->setValue(s.value("NeighborOffset", 1.0).toDouble());
	ui.spinBoxRidgeDiff->setValue(s.value("RidgeDiffThreshold", 5.0).toDouble());

	// Hair tracing settings
	ui.spinBoxDensity->setValue(s.value("TraceDensity", 0.02).toDouble());
	ui.spinBoxNumRelaxations->setValue(s.value("NumRelaxations", 4).toInt());
	ui.spinBoxRelaxForce->setValue(s.value("RelaxForce", 0.2).toDouble());
	ui.spinBoxTraceHighVar->setValue(s.value("TraceHighVar", 0.2).toDouble());
	ui.spinBoxTraceLowVar->setValue(s.value("TraceLowVar", 0.07).toDouble());
	ui.spinBoxLaTermSteps->setValue(s.value("TraceLatermSteps", 0).toInt());
	ui.spinBoxStepLen->setValue(s.value("TraceStepLen", 2).toDouble());
	ui.spinBoxMaxLen->setValue(s.value("TraceMaxLen", 100).toDouble());
	ui.spinBoxMinLen->setValue(s.value("TraceMinLen", 5).toDouble());
	ui.spinBoxMaxBend->setValue(s.value("TraceMaxBend", 1.0).toDouble());
	ui.checkBoxTraceRandomOrder->setChecked(s.value("TraceRandomOrder", true).toBool());
	ui.checkBoxTraceLengthClamp->setChecked(s.value("TraceLengthClamp", false).toBool());
	ui.spinBoxMinAuxCoverage->setValue(s.value("TraceMinAuxCoverage", 1).toInt());
	ui.spinBoxMaxAuxCoverage->setValue(s.value("TraceMaxAuxCoverage", 10).toInt());
	ui.checkBoxCorrectToRidge->setChecked(s.value("TraceCorrectToRidge", true).toBool());
	ui.spinBoxRidgeRadius->setValue(s.value("TraceRidgeRadius", 1.0).toDouble());
	ui.spinBoxMaxCorrection->setValue(s.value("TraceMaxCorrection", 0.5).toDouble());
	ui.spinBoxTraceCenterCov->setValue(s.value("TraceCenterCoverage", 64).toInt());
	ui.spinBoxTraceCovSigma->setValue(s.value("TraceCoverageSigma", 1.0f).toDouble());

	// (Dense strands)
	ui.spinBoxDsNumFrontLayers->setValue(s.value("DsNumFrontLayers", 1).toInt());
	ui.spinBoxDsNumBackLayers->setValue(s.value("DsNumBackLayers", 1).toInt());
	ui.spinBoxDsLayerThickness->setValue(s.value("DsLayerThickness", 1.5).toDouble());
	ui.spinBoxDsDepthMapSigma->setValue(s.value("DsDepthMapSigma", 1.0).toDouble());
	ui.spinBoxDsStepLen->setValue(s.value("DsStepLen", 2.0).toDouble());
	ui.spinBoxDsHealth->setValue(s.value("DsHealthPoint", 3).toInt());
	ui.spinBoxDsCenterCoverage->setValue(s.value("DsCenterCoverage", 32).toInt());
	ui.spinBoxDsCoverageSigma->setValue(s.value("DsCoverageSigma", 1.0).toDouble());
	ui.spinBoxDsMinCoverage->setValue(s.value("DsMinCoverage", 4).toInt());
	ui.spinBoxDsMaxCoverage->setValue(s.value("DsMaxCoverage", 48).toInt());
	ui.spinBoxDsDepthVar->setValue(s.value("DsDepthVariance", 0.5).toDouble());
	ui.checkBoxDsLenClamp->setChecked(s.value("DsLengthClamping", true).toBool());
	ui.spinBoxDsMinLen->setValue(s.value("DsMinLength", 9).toDouble());
	ui.spinBoxDsShrinkDelta->setValue(s.value("DsShrinkDelta", 3).toDouble());

	ui.spinBoxFalloffRadius->setValue(s.value("SilhouetteRadius", 16).toDouble());
	ui.spinBoxFalloffDepth->setValue(s.value("SilhouetteDepth", 16).toDouble());

	ui.spinBoxEsBackHalfOffset->setValue(s.value("EsBackHalfOffset", 0).toDouble());
	ui.spinBoxEsBackHalfScale->setValue(s.value("EsBackHalfScale", 1.5).toDouble());
	ui.spinBoxEsNumMidLayers->setValue(s.value("EsNumMidLayers", 2).toInt());

	// Rendering
	ui.spinBoxMsaaSamples->setValue(s.value("MsaaSamples", 1).toInt());
	ui.spinBoxMsaaQuality->setValue(s.value("MsaaQuality", 0).toInt());
	m_sceneWidget->setMultisampleQuality(ui.spinBoxMsaaSamples->value(), ui.spinBoxMsaaQuality->value());

	ui.spinBoxKa->setValue(s.value("Ka", 0.5f).toDouble());
	ui.spinBoxKd->setValue(s.value("Kd", 0.5f).toDouble());
	ui.spinBoxKr->setValue(s.value("Kr", 0.5f).toDouble());

	ui.spinBoxTanTexWeight->setValue(s.value("TangentTexWeight", 1.0f).toDouble());
	ui.spinBoxTanTexScaleX->setValue(s.value("TangentTexScaleX", 0.5f).toDouble());
	ui.spinBoxTanTexScaleY->setValue(s.value("TangentTexScaleY", 0.01f).toDouble());
	ui.spinBoxNormalGradWeight->setValue(s.value("NormalGradWeight", 0.5f).toDouble());
	ui.spinBoxStrandRandWeight->setValue(s.value("StrandRandWeight", 0.2f).toDouble());
	ui.spinBoxForceOpaque->setValue(s.value("ForceOpaque", 0.5f).toDouble());
	ui.spinBoxNeckBlend->setValue(s.value("NeckBlend", 0.02f).toDouble());

	ui.spinBoxHairWidth->setValue(s.value("HairWidth", 1).toDouble());
	ui.spinBoxStrandsDepthScale->setValue(s.value("StrandsDepthScale", 1).toDouble());
	ui.spinBoxStrandsDepthOffset->setValue(s.value("StrandsDepthOffset", -1.0).toDouble());
	ui.spinBoxFilterStrandsColorSigma->setValue(s.value("StrandsColorSigma", 3.0).toDouble());

	ui.spinBoxAuxHairWidth->setValue(s.value("AuxHairWidth", 1).toDouble());
	ui.spinBoxAuxStrandsDepthScale->setValue(s.value("AuxStrandsDepthScale", 1).toDouble());
	ui.spinBoxAuxStrandsDepthOffset->setValue(s.value("AuxStrandsDepthOffset", -0.5).toDouble());
	ui.spinBoxFilterAuxStrandsColorSigma->setValue(s.value("AuxStrandsColorSigma", 3.0).toDouble());

	// Camera
	ui.spinBoxCamFoV->setValue(s.value("CameraFoV", 90).toInt());
	ui.spinBoxCamZn->setValue(s.value("CameraZNear", 0.2).toDouble());
	ui.spinBoxCamZf->setValue(s.value("CameraZFar", 2000).toDouble());

	// Other viewing settings
	ui.checkBoxShowMask->setChecked(s.value("ShowMask", false).toBool());
	ui.checkBoxShowMaxResp->setChecked(s.value("ShowConfidenceMask", false).toBool());

	// Hair Editing
	ui.spinBoxGeoNoiseAmp->setValue(s.value("GeoNoiseAmplitude", 0.5).toDouble());
	ui.spinBoxGeoNoiseFreq->setValue(s.value("GeoNoiseFrequency", 0.01).toDouble());

	m_scene->setHairRenderMode((HairRenderMode)ui.comboHairRenderMode->currentIndex());
	m_scene->headRenderer()->setRenderMode((HeadRenderMode)ui.comboHeadRenderMode->currentIndex());

	// editing strokes
	ui.spinBoxStrokeRadius->setValue(s.value("StrokeRadius", 8).toInt());
	ui.spinBoxStrokeIntensity->setValue(s.value("StrokeIntensity", 1).toDouble());
	ui.spinBoxStrokeFuzziness->setValue(s.value("StrokeFuzziness", 2).toInt());

	// Morphing
	ui.spinBoxMorphStrandWidth->setValue(s.value("MorphStrandWidth", 1.0f).toDouble());
	ui.checkBoxUseStrandWeights->setChecked(s.value("UseStrandWeights", false).toBool());

	// Clustering
	ui.spinBoxNumPyrLevels->setValue(s.value("NumPyrLevels", 2).toInt());
	ui.spinBoxLvl0ClusterRatio->setValue(s.value("Lvl0ClusterRatio", 1).toInt());
	ui.spinBoxLvl1ClusterRatio->setValue(s.value("Lvl1ClusterRatio", 1).toInt());
	ui.spinBoxLvl2ClusterRatio->setValue(s.value("Lvl2ClusterRatio", 1).toInt());

	ui.checkBoxLvl0Relative->setChecked(s.value("Lvl0Relative", true).toBool());
	ui.checkBoxLvl1Relative->setChecked(s.value("Lvl1Relative", true).toBool());
	ui.checkBoxLvl2Relative->setChecked(s.value("Lvl2Relative", true).toBool());

	ui.checkBoxShowMorphSrcHair->setChecked(s.value("ShowMorphSrcHair", true).toBool());
	ui.checkBoxShowMorphDstHair->setChecked(s.value("ShowMorphDstHair", true).toBool());
	ui.checkBoxShowMorphHair->setChecked(s.value("ShowMorphHair", true).toBool());
	ui.checkBoxShowMorphHead->setChecked(s.value("ShowMorphHead", true).toBool());
}


void HairLayers::saveSettings()
{
	QSettings settings("HairLayers.ini", QSettings::IniFormat);

	settings.setValue("WindowGeometry",		saveGeometry());

	settings.setValue("SceneBackgroundColor", m_sceneWidget->backgroundColor().rgba());
	settings.setValue("ToolBoxIndex",		ui.toolBox->currentIndex());
	settings.setValue("TabWidgetIndex",		ui.tabWidget->currentIndex());

	// Default paths
	settings.setValue("SceneInputPath",		m_scnInputPath);
	settings.setValue("ImageInputPath",		m_imgInputPath);
	settings.setValue("ImageOutputPath",	m_imgOutputPath);
	settings.setValue("HairInputPath",		m_hairInputPath);
	settings.setValue("HairOutputPath",		m_hairOutputPath);
	settings.setValue("FaceRecProgramPath",	m_faceRecProgPath);
	settings.setValue("DepthEstProgramPath",m_depthEstProgPath);

	// Preprocessing
	settings.setValue("PreprocessMethod",	ui.comboPrepMethod->currentIndex());
	settings.setValue("PrepSigmaH",			ui.spinBoxPrepSigmaH->value());
	settings.setValue("PrepSigmaL",			ui.spinBoxPrepSigmaL->value());

	// Orientation
	settings.setValue("OrientationMethod",	ui.comboOrientMethod->currentIndex());
	settings.setValue("NumKernels",			ui.spinBoxNumKernels->value());
	settings.setValue("KernelWidth",		ui.spinBoxKernelWidth->value());
	settings.setValue("KernelHeight",		ui.spinBoxKernelHeight->value());
	settings.setValue("AutoRefineOrient",	ui.spinBoxNumAutoRefineOrient->value());

	// Gabor kernel
	settings.setValue("NumPhases",			ui.spinBoxNumPhases->value());
	settings.setValue("GaborSigmaX",		ui.spinBoxGaborSigmaX->value());
	settings.setValue("GaborSigmaY",		ui.spinBoxGaborSigmaY->value());
	settings.setValue("GaborLambda",		ui.spinBoxGaborLambda->value());
	settings.setValue("GaborPhase",			ui.spinBoxPhase->value());
	settings.setValue("SaveKernels",		ui.checkBoxSaveKernels->isChecked());

	// Confidence
	settings.setValue("ConfidenceMethod",	ui.comboConfidMethod->currentIndex());
	settings.setValue("ClampConfidLow",		ui.spinBoxClampConfidLow->value());
	settings.setValue("ClampConfidHigh",	ui.spinBoxClampConfidHigh->value());

	// Orientation Smoothing
	settings.setValue("SmoothMethod",		ui.comboSmoothMethod->currentIndex());
	settings.setValue("SmoothConfidThres",	ui.spinBoxConfidThreshold->value());
	settings.setValue("SmoothDiffuseIters",	ui.spinBoxDiffuseIterations->value());
	settings.setValue("SmoothSpatial",		ui.checkBoxSmoothSpatial->isChecked());
	settings.setValue("SmoothOrientation",	ui.checkBoxSmoothOrient->isChecked());
	settings.setValue("SmoothColor",		ui.checkBoxSmoothColor->isChecked());
	settings.setValue("SmoothConfidence",	ui.checkBoxSmoothConfidence->isChecked());
	settings.setValue("SigmaSpatial",		ui.spinBoxSigmaSpatial->value());
	settings.setValue("SigmaOrientation",	ui.spinBoxSigmaOrient->value());
	settings.setValue("SigmaColor",			ui.spinBoxSigmaColor->value());
	settings.setValue("SigmaConfidence",	ui.spinBoxSigmaConfidence->value());

	// Feature detection
	settings.setValue("NeighborOffset",		ui.spinBoxNbrOffset->value());
	settings.setValue("RidgeDiffThreshold", ui.spinBoxRidgeDiff->value());

	// Hair tracing
	settings.setValue("TraceDensity",		ui.spinBoxDensity->value());
	settings.setValue("NumRelaxations",		ui.spinBoxNumRelaxations->value());
	settings.setValue("RelaxForce",			ui.spinBoxRelaxForce->value());
	settings.setValue("TraceHighVar",		ui.spinBoxTraceHighVar->value());
	settings.setValue("TraceLowVar",		ui.spinBoxTraceLowVar->value());
	settings.setValue("TraceLatermSteps",	ui.spinBoxLaTermSteps->value());
	settings.setValue("TraceStepLen",		ui.spinBoxStepLen->value());
	settings.setValue("TraceMaxLen",		ui.spinBoxMaxLen->value());
	settings.setValue("TraceMinLen",		ui.spinBoxMinLen->value());
	settings.setValue("TraceMaxBend",		ui.spinBoxMaxBend->value());
	settings.setValue("TraceRandomOrder",	ui.checkBoxTraceRandomOrder->isChecked());
	settings.setValue("TraceLengthClamp",	ui.checkBoxTraceLengthClamp->isChecked());
	settings.setValue("TraceMinAuxCoverage",ui.spinBoxMinAuxCoverage->value());
	settings.setValue("TraceMaxAuxCoverage",ui.spinBoxMaxAuxCoverage->value());
	settings.setValue("TraceCorrectToRidge",ui.checkBoxCorrectToRidge->isChecked());
	settings.setValue("TraceRidgeRadius",	ui.spinBoxRidgeRadius->value());
	settings.setValue("TraceMaxCorrection",	ui.spinBoxMaxCorrection->value());
	settings.setValue("TraceCenterCoverage",ui.spinBoxTraceCenterCov->value());
	settings.setValue("TraceCoverageSigma",	ui.spinBoxTraceCovSigma->value());

	// (Dense strands tracing)
	settings.setValue("DsNumFrontLayers",	ui.spinBoxDsNumFrontLayers->value());
	settings.setValue("DsNumBackLayers",	ui.spinBoxDsNumBackLayers->value());
	settings.setValue("DsLayerThickness",	ui.spinBoxDsLayerThickness->value());
	settings.setValue("DsDepthMapSigma",	ui.spinBoxDsDepthMapSigma->value());
	settings.setValue("DsStepLen",			ui.spinBoxDsStepLen->value());
	settings.setValue("DsHealthPoint",		ui.spinBoxDsHealth->value());
	settings.setValue("DsCenterCoverage",	ui.spinBoxDsCenterCoverage->value());
	settings.setValue("DsCoverageSigma",	ui.spinBoxDsCoverageSigma->value());
	settings.setValue("DsMinCoverage",		ui.spinBoxDsMinCoverage->value());
	settings.setValue("DsMaxCoverage",		ui.spinBoxDsMaxCoverage->value());
	settings.setValue("DsDepthVariance",	ui.spinBoxDsDepthVar->value());
	settings.setValue("DsLengthClamping",	ui.checkBoxDsLenClamp->isChecked());
	settings.setValue("DsMinLength",		ui.spinBoxDsMinLen->value());
	settings.setValue("DsShrinkDelta",		ui.spinBoxDsShrinkDelta->value());

	settings.setValue("SilhouetteRadius",	ui.spinBoxFalloffRadius->value());
	settings.setValue("SilhouetteDepth",	ui.spinBoxFalloffDepth->value());

	settings.setValue("EsBackHalfOffset",	ui.spinBoxEsBackHalfOffset->value());
	settings.setValue("EsBackHalfScale",	ui.spinBoxEsBackHalfScale->value());
	settings.setValue("EsNumMidLayers",		ui.spinBoxEsNumMidLayers->value());

	// Rendering
	settings.setValue("MsaaSamples",		ui.spinBoxMsaaSamples->value());
	settings.setValue("MsaaQuality",		ui.spinBoxMsaaQuality->value());

	settings.setValue("Ka",					ui.spinBoxKa->value());
	settings.setValue("Kd",					ui.spinBoxKd->value());
	settings.setValue("Kr",					ui.spinBoxKr->value());

	settings.setValue("TangentTexWeight",	ui.spinBoxTanTexWeight->value());
	settings.setValue("TangentTexScaleX",	ui.spinBoxTanTexScaleX->value());
	settings.setValue("TangentTexScaleY",	ui.spinBoxTanTexScaleY->value());
	settings.setValue("NormalGradWeight",	ui.spinBoxNormalGradWeight->value());
	settings.setValue("StrandRandWeight",	ui.spinBoxStrandRandWeight->value());
	settings.setValue("ForceOpaque",		ui.spinBoxForceOpaque->value());
	settings.setValue("NeckBlend",			ui.spinBoxNeckBlend->value());

	settings.setValue("HairWidth",			ui.spinBoxHairWidth->value());
	settings.setValue("StrandsDepthScale",	ui.spinBoxStrandsDepthScale->value());
	settings.setValue("StrandsDepthOffset",	ui.spinBoxStrandsDepthOffset->value());
	settings.setValue("StrandsColorSigma",	ui.spinBoxFilterStrandsColorSigma->value());

	settings.setValue("AuxHairWidth",		ui.spinBoxAuxHairWidth->value());
	settings.setValue("AuxStrandsDepthScale",ui.spinBoxAuxStrandsDepthScale->value());
	settings.setValue("AuxStrandsDepthOffset",ui.spinBoxAuxStrandsDepthOffset->value());
	settings.setValue("AuxStrandsColorSigma",ui.spinBoxFilterAuxStrandsColorSigma->value());

	// Camera
	settings.setValue("CameraFoV",			ui.spinBoxCamFoV->value());
	settings.setValue("CameraZNear",		ui.spinBoxCamZn->value());
	settings.setValue("CameraZFar",			ui.spinBoxCamZf->value());

	// Other viewing settings
	settings.setValue("ShowMask",			ui.checkBoxShowMask->isChecked());
	settings.setValue("ShowConfidenceMask",	ui.checkBoxShowMaxResp->isChecked());

	// Hair Editing
	settings.setValue("GeoNoiseAmplitude",	ui.spinBoxGeoNoiseAmp->value());
	settings.setValue("GeoNoiseFrequency",	ui.spinBoxGeoNoiseFreq->value());

	// strokes
	settings.setValue("StrokeRadius",		ui.spinBoxStrokeRadius->value());
	settings.setValue("StrokeIntensity",	ui.spinBoxStrokeIntensity->value());
	settings.setValue("StrokeFuzziness",	ui.spinBoxStrokeFuzziness->value());

	// Morphing
	settings.setValue("MorphStrandWidth",	ui.spinBoxMorphStrandWidth->value());
	settings.setValue("UseStrandWeights",	ui.checkBoxUseStrandWeights->isChecked());

	// Clustering
	settings.setValue("NumPyrLevels",		ui.spinBoxNumPyrLevels->value());
	settings.setValue("Lvl0ClusterRatio",	ui.spinBoxLvl0ClusterRatio->value());
	settings.setValue("Lvl1ClusterRatio",	ui.spinBoxLvl1ClusterRatio->value());
	settings.setValue("Lvl2ClusterRatio",	ui.spinBoxLvl2ClusterRatio->value());

	settings.setValue("Lvl0Relative",		ui.checkBoxLvl0Relative->isChecked());
	settings.setValue("Lvl1Relative",		ui.checkBoxLvl1Relative->isChecked());
	settings.setValue("Lvl2Relative",		ui.checkBoxLvl2Relative->isChecked());

	settings.setValue("ShowMorphSrcHair",	ui.checkBoxShowMorphSrcHair->isChecked());
	settings.setValue("ShowMorphDstHair",	ui.checkBoxShowMorphDstHair->isChecked());
	settings.setValue("ShowMorphHair",		ui.checkBoxShowMorphHair->isChecked());
	settings.setValue("ShowMorphHead",		ui.checkBoxShowMorphHead->isChecked());
}


//--------------------------------------------------------------
// Actions
//--------------------------------------------------------------

// Load necessary files for a hair model (original image, mask, etc.)
void HairLayers::on_actionLoadHairModel_triggered()
{
	QString scnFilename = QFileDialog::getOpenFileName(this, "Load a hair model", 
		m_scnInputPath, "Scene files (*.txt)");

	if (scnFilename.isNull())
		return;

	m_scnInputPath = scnFilename.left(scnFilename.lastIndexOf('/') + 1);

	QSettings model(scnFilename, QSettings::IniFormat);

	// Original image
	if (model.contains("OriginalImage"))
	{
		QString fn = m_scnInputPath + model.value("OriginalImage", "").toString();
		if (!m_hairImage.loadColor(fn))
		{
			QMessageBox::critical(this, "Error", "Cannot load original image.");
			return;
		}
	}

	// Hair color image
	if (model.contains("HairColorImage"))
	{
		QString fn = m_scnInputPath + model.value("HairColorImage", "").toString();
		if (!m_hairImage.loadHairColor(fn))
		{
			QMessageBox::critical(this, "Error", "Cannot load hair color image.");
			return;
		}
	}

	// Hair mask image
	if (model.contains("HairMaskImage"))
	{
		QString fn = m_scnInputPath + model.value("HairMaskImage", "").toString();
		if (!m_hairImage.loadMask(fn))
		{
			QMessageBox::critical(this, "Error", "Cannot load hair mask image.");
			return;
		}
	}

	// Face boundary points
	if (model.contains("FaceBoundary"))
	{
		QString fn = m_scnInputPath + model.value("FaceBoundary", "").toString();
		if (!m_hairImage.loadFaceMaskFromPts(fn))
		{
			QMessageBox::critical(this, "Error", "Cannot load face boundary.");
			return;
		}
	}

	// Extra params for extra strand synthesis
	if (model.contains("EsColorFile"))
	{
		QString fn = m_scnInputPath + model.value("EsColorFile", "").toString();
		if (!m_hairImage.loadMidColor(fn))
		{
			QMessageBox::critical(this, "Error", "Cannot load mid color file.");
			return;
		}
	}

	if (model.contains("EsMaskFile"))
	{
		QString fn = m_scnInputPath + model.value("EsMaskFile", "").toString();
		if (!m_hairImage.loadMidMask(fn))
		{
			QMessageBox::critical(this, "Error", "Cannot load mid mask file.");
			return;
		}
	}


	// Strands geometry and color
	if (model.contains("FinalSparseStrands"))
	{
		QString fn = m_scnInputPath + model.value("FinalSparseStrands", "").toString();
		if (!m_scene->strandModel()->load(fn))
		{
			QMessageBox::critical(this, "Error", "Cannot load sparse strands.");
			return;
		}
	}

	if (model.contains("FinalDenseStrands"))
	{
		QString fn = m_scnInputPath + model.value("FinalDenseStrands", "").toString();
		if (!m_scene->auxStrandModel()->load(fn))
		{
			QMessageBox::critical(this, "Error", "Cannot load dense strands.");
			return;
		}
	}

	if (model.contains("FinalExtraStrands"))
	{
		QString fn = m_scnInputPath + model.value("FinalExtraStrands", "").toString();
		if (!m_scene->denseStrandModel()->load(fn))
		{
			QMessageBox::critical(this, "Error", "Cannot load extra strands.");
			return;
		}
		// DEBUG:
		m_scene->denseStrandModel()->reverseOrder();
	}

	// Load transfer matrix
	if (model.contains("TransferMatrix"))
	{
		QString fn = m_scnInputPath + model.value("TransferMatrix", "").toString();

		float srcHeight = model.value("SrcHeight", 0).toFloat();
		float dstHeight = model.value("DstHeight", 0).toFloat();

		if (!m_scene->loadTransferMatrix(fn, srcHeight, dstHeight))
		{
			QMessageBox::critical(this, "Error", "Cannot load transfer matrix.");
			return;
		}
	}
	else
	{
		m_scene->hairRenderer()->resetTransferMat();
	}

	ui.spinBoxForceOpaque->setValue(0.5);
	ui.spinBoxForceOpaque->setValue(0.0);

	hairImageUpdated();

	m_scene->resetCamera();

	setWindowTitle(QString("HairLayers - ") + m_scnInputPath);
}


// Load all necessary files for a hairless model (background/body layers, head mesh/textures etc.)
void HairLayers::on_actionLoadHairlessModel_triggered()
{
	QString filename = QFileDialog::getOpenFileName(this, "Load a hairless model", 
		m_scnInputPath, "Scene files (*.txt)");

	if (filename.isNull())
		return;

	m_scnInputPath = filename.left(filename.lastIndexOf('/') + 1);

	QSettings scnFile(filename, QSettings::IniFormat);

	// Background layer

	QString bgFile	= m_scnInputPath + scnFile.value("BackgroundLayerImage", "").toString();
	float	bgZ		= scnFile.value("BackgroundLayerZ", 0).toFloat();

	if (m_scene->loadBackgroundLayer(bgFile, bgZ) == false)
	{
		QMessageBox::critical(this, "Error", "Failed to load background layer.");
		return;
	}

	// Large background layer
	if (scnFile.contains("LargeBackgroundImage"))
	{
		QString fn  = m_scnInputPath + scnFile.value("LargeBackgroundImage", "").toString();

		if (m_scene->loadLargeBackgroundLayer(fn, bgZ) == false)
		{
			QMessageBox::critical(this, "Error", "Failed to load large background layer.");
			return;
		}
	}

	// Original target image layer
	if (scnFile.contains("OriginalImage"))
	{
		if (!scnFile.contains("OriginalHairRegion"))
		{
			QMessageBox::critical(this, "Error", "Provided original image but no hair region!");
			return;
		}

		QString fn = m_scnInputPath + scnFile.value("OriginalImage", "").toString();
		QString maskFn = m_scnInputPath + scnFile.value("OriginalHairRegion", "").toString();

		if (m_scene->loadOriginalLayer(fn, maskFn, -200) == false)
		{
			QMessageBox::critical(this, "Error", "Failed to load original image layer!");
			m_scene->originalLayer()->setVisible(false);
			return;
		}
		m_scene->originalLayer()->setVisible(true);
	}
	else
	{
		m_scene->originalLayer()->setVisible(false);
	}
	ui.checkBoxOrigLayer->setChecked(m_scene->originalLayer()->isVisible());

	// Body layer

	QString bodyColorFile = m_scnInputPath + scnFile.value("BodyColorImage", "").toString();
	QString bodyDepthFile = m_scnInputPath + scnFile.value("BodyDepthImage", "").toString();
	float	bodyZ	= scnFile.value("BodyLayerZ", 0).toFloat();

	if (!scnFile.contains("BodyDepthImage"))
		bodyDepthFile = "";
	if (m_scene->loadBodyModel(bodyColorFile, bodyDepthFile, bodyZ) == false)
	{
		QMessageBox::critical(this, "Error", "Failed to load body model.");
		return;
	}

	// Head model

	QString headMeshFile = m_scnInputPath + scnFile.value("HeadMesh", "").toString();

	if (m_scene->loadHeadMesh(headMeshFile) == false)
	{
		QMessageBox::critical(this, "Error", "Failed to load head mesh.");
		return;
	}

	// Neck model

	QString neckMeshFile = m_scnInputPath + scnFile.value("NeckMesh", "").toString();

	if (m_scene->loadNeckMesh(neckMeshFile) == false)
	{
		QMessageBox::critical(this, "Error", "Failed to load neck mesh.");
		return;
	}

	// Texture/transform for head & neck

	QString headTextureFile = m_scnInputPath + scnFile.value("HeadTexture", "").toString();

	if (m_scene->loadHeadTexture(headTextureFile) == false)
	{
		QMessageBox::critical(this, "Error", "Failed to load head mesh.");
		return;
	}

	QString headTransformFile = m_scnInputPath + scnFile.value("HeadTransform", "").toString();

	if (m_scene->loadHeadTransform(headTransformFile) == false)
	{
		QMessageBox::critical(this, "Error", "Failed to load head mesh.");
		return;
	}

	m_scene->resetCamera();

	ui.spinBoxBgLayerDepth->setValue(bgZ);
	ui.spinBoxBodyLayerDepth->setValue(bodyZ);
}


void HairLayers::on_actionSegmentation_triggered()
{
	//QImage orig(m_hairImage.colorFilename());
	//
	//bool bMatting = ui.actionEnableMatting->isChecked();
	//if (SegmentAllAndSave(&orig, m_scene->strokesSprite()->strokeIdImage(), m_imgInputPath, bMatting))
	//{
	//	QMessageBox::information(this, "Segmentation", "Segmentation finished. Results saved to files.");
	//}
}


void HairLayers::on_actionFaceReconstruct_triggered()
{
	// Make sure we have both original image and face region image.
	if (m_hairImage.isColorNull())
	{
		QMessageBox::warning(this, "HairLayers", "Load color image first!");
		return;
	}

	QFileInfo colorInfo(m_hairImage.colorFilename());
	QString colorName = colorInfo.fileName();
	QString faceName = "segment_face.png"; // hard-coded for now.

	// Call external program for face reconstruction
	if (!QFile::exists(m_faceRecProgPath))
	{
		m_faceRecProgPath = QFileDialog::getOpenFileName(this, "Where is the FaceReconstruct program?", 
			"", "Executables (*.exe *.bat)");

		if (m_faceRecProgPath.isNull())
		{
			QMessageBox::critical(this, "Error", "Cannot locate FaceReconstruct program.");
			return;
		}
	}
	QProcess faceProgram(this);
	QString workingPath = QFileInfo(m_faceRecProgPath).absolutePath() + "/";
	faceProgram.setWorkingDirectory(workingPath);
	
	QStringList args;
	args.append(m_imgInputPath);
	args.append(colorName);
	args.append(faceName);
	faceProgram.start(m_faceRecProgPath, args);
	if (!faceProgram.waitForFinished(-1))
	{
		QMessageBox::warning(this, "Error", "Some error may have occurred in DepthEst.");
		return;
	}

	// Check and load generated resources...
	m_scnInputPath = m_imgInputPath;

	// Background (as original)
	if (m_scene->loadBackgroundLayer(m_hairImage.colorFilename(), 0) == false)
	{
		QMessageBox::critical(this, "Error", "Failed to load background layer.");
		return;
	}

	// Head model
	QString headMeshFile = m_scnInputPath + "fit.obj";
	if (m_scene->loadHeadMesh(headMeshFile) == false)
	{
		QMessageBox::critical(this, "Error", "Failed to load head mesh.");
		return;
	}

	// Neck model
	QString neckMeshFile = m_scnInputPath + "neck.obj";
	if (m_scene->loadNeckMesh(neckMeshFile) == false)
	{
		QMessageBox::critical(this, "Error", "Failed to load neck mesh.");
		return;
	}

	// Texture/transform for head & neck
	QString headTextureFile = m_scnInputPath + "fit.jpg";
	if (m_scene->loadHeadTexture(headTextureFile) == false)
	{
		QMessageBox::critical(this, "Error", "Failed to load head mesh.");
		return;
	}

	QString headTransformFile = m_scnInputPath + "feature_coefficients.txt";
	if (m_scene->loadHeadTransform(headTransformFile) == false)
	{
		QMessageBox::critical(this, "Error", "Failed to load head mesh.");
		return;
	}

	m_scene->resetCamera();

	ui.checkBoxStrokes->setChecked(false);

	// Also load hair region image and face boundary
	if (!m_hairImage.loadHairColor(m_scnInputPath + "cut_region.png"))
	{
		QMessageBox::critical(this, "Error", "Cannot load hair color image.");
		return;
	}

	// Face boundary points
	if (!m_hairImage.loadFaceMaskFromPts(m_scnInputPath + "boundary_fit_2D.txt"))
	{
		QMessageBox::critical(this, "Error", "Cannot load face boundary.");
		return;
	}

}

void HairLayers::on_actionGen2DStrands_triggered()
{
	on_actionCalcOrientation_triggered();
	on_actionTraceStrands_triggered();
	on_actionCalcDenseOrientation_triggered();
	on_actionTraceAuxStrands_triggered();
}

void HairLayers::on_actionEstimateDepth_triggered()
{
	if (m_scene->strandModel()->numStrands() < 1 ||
		m_scene->auxStrandModel()->numStrands() < 1)
	{
		QMessageBox::warning(this, "Error", "Please generate 2D strands first.");
		return;
	}

	// Save to strands/auxStrands
	QString filename = m_scnInputPath + "strand.shd";
	if (!m_scene->strandModel()->saveGeometry(filename))
	{
		QMessageBox::critical(this, "Error", "Failed to save strands.");
		return;
	}
	
	filename = m_scnInputPath + "auxStrand.shd";
	if (!m_scene->auxStrandModel()->saveGeometry(filename))
	{
		QMessageBox::critical(this, "Error", "Failed to save aux strands.");
		return;
	}

	// Call external program for depth estimation
	if (!QFile::exists(m_depthEstProgPath))
	{
		m_depthEstProgPath = QFileDialog::getOpenFileName(this, "Where is the DepthEst program?", 
			"", "Executables (*.exe *.bat)");
		
		if (m_depthEstProgPath.isNull())
		{
			QMessageBox::critical(this, "Error", "Cannot locate DepthEst program.");
			return;
		}
	}
	QProcess depthProgram(this);
	QString workingPath = QFileInfo(m_depthEstProgPath).absolutePath() + "/";
	depthProgram.setWorkingDirectory(workingPath);
	depthProgram.start(m_depthEstProgPath);
	if (!depthProgram.waitForFinished(-1))
	{
		QMessageBox::warning(this, "Warning", "Some error may have occurred in DepthEst.");
	}

	// Read back strands with depth
	filename = m_scnInputPath + "strand_depth.shd";
	if (!m_scene->strandModel()->loadGeometry(filename))
	{
		QMessageBox::critical(this, "Error", "Failed to load strands.");
		return;
	}

	filename = m_scnInputPath + "auxStrand_depth.shd";
	if (!m_scene->auxStrandModel()->loadGeometry(filename))
	{
		QMessageBox::critical(this, "Error", "Failed to load aux strands.");
		return;
	}

	// Add silhouette depth offset
	on_buttonAddSilhouetteFalloff_clicked();
}

// Generate dense layered strands with color and alpha.
void HairLayers::on_actionGen3DStrands_triggered()
{
	on_buttonGenDenseStrands_clicked();

	on_buttonFilterStrandsColor_clicked();
	on_buttonFilterAuxStrandsColor_clicked();
	//on_buttonFilterDenseStrandsColor_clicked();

	ui.spinBoxForceOpaque->setValue(0.5);
	ui.spinBoxForceOpaque->setValue(0.0);
}


void HairLayers::on_actionBatchRenderVideo_triggered()
{
	QString filename = QFileDialog::getOpenFileName(this, "Batch video render file", 
		m_scnInputPath, "Scene files (*.txt)");

	if (filename.isNull())
		return;

	m_scnInputPath = filename.left(filename.lastIndexOf('/') + 1);

	if (!m_scene->batchRenderVideo(filename))
	{
		QMessageBox::critical(this, "Error", "Error");
		return;
	}
}


void HairLayers::on_actionLaunchDepthEst_triggered()
{
	// Call external program for depth estimation
	if (!QFile::exists(m_depthEstProgPath))
	{
		m_depthEstProgPath = QFileDialog::getOpenFileName(this, "Where is the DepthEst program?", 
			"", "Executables (*.exe *.bat)");

		if (m_depthEstProgPath.isNull())
		{
			QMessageBox::critical(this, "Error", "Cannot locate DepthEst program.");
			return;
		}
	}
	QProcess depthProgram(this);
	QString workingPath = QFileInfo(m_depthEstProgPath).absolutePath() + "/";
	depthProgram.setWorkingDirectory(workingPath);
	depthProgram.start(m_depthEstProgPath);
}


void HairLayers::on_actionLoadColorImage_triggered()
{
	QString filename = QFileDialog::getOpenFileName(this, "Load color image", 
		m_imgInputPath, "Images (*.png *.jpg *.bmp *.tif *.tiff)");

	if (filename.isNull())
		return;

	// Update image path.
	m_imgInputPath = filename.left(filename.lastIndexOf('/') + 1);

	if (m_hairImage.loadColor(filename) == false)
	{
		QMessageBox::critical(this, "Error", "Cannot load image.");
		return;
	}

	hairImageUpdated();
	
	ui.checkBoxStrokes->setChecked(true);
	m_scene->strokesSprite()->setVisible(true);
	m_scene->strokesSprite()->clearImage();


	m_scene->resetCamera();
}


void HairLayers::on_actionLoadFlatHair_triggered()
{
	m_scene->flatHairLayer()->setVisible(false);

	QString filename = QFileDialog::getOpenFileName(this, "Load color image", 
		m_scnInputPath, "Images (*.png *.jpg *.bmp *.tif *.tiff)");

	if (filename.isNull())
		return;

	if (!m_scene->loadFlatHairLayer(filename, ui.spinBoxAltZ->value()))
	{
		QMessageBox::critical(this, "Error", "Cannot load image.");
		return;
	}
	m_scene->heightHairLayer()->setVisible(false);
	m_scene->flatHairLayer()->setVisible(true);
}


void HairLayers::on_actionLoadHeightHair_triggered()
{
	m_scene->heightHairLayer()->setVisible(false);

	QString filename = QFileDialog::getOpenFileName(this, "Load color image", 
		m_scnInputPath, "Images (*.png *.jpg *.bmp *.tif *.tiff)");

	if (filename.isNull())
		return;

	if (!m_scene->loadHeightHairLayer(filename, ui.spinBoxAltZ->value()))
	{
		QMessageBox::critical(this, "Error", "Cannot load image.");
		return;
	}
	m_scene->flatHairLayer()->setVisible(false);
	m_scene->heightHairLayer()->setVisible(true);
}

void HairLayers::on_actionLoadMask_triggered()
{
	QString filename = QFileDialog::getOpenFileName(this, "Load mask image", 
		m_imgInputPath, "Images (*.png *.jpg *.bmp *.tif *.tiff)");

	if (filename.isNull())
		return;

	if (m_hairImage.loadMask(filename) == false)
	{
		QMessageBox::critical(this, "Error", "Cannot load mask.");
		return;
	}

	hairImageUpdated();

}


void HairLayers::on_actionLoadFaceMask_triggered()
{
	QString filename = QFileDialog::getOpenFileName(this, "Load face mask contour points", 
		m_imgInputPath, "Text files (*.txt)");

	if (filename.isNull())
		return;

	if (m_hairImage.loadFaceMaskFromPts(filename) == false)
	{
		QMessageBox::critical(this, "Error", "Cannot load mask.");
		return;
	}

	hairImageUpdated();
}


void HairLayers::on_actionLoadOrientChai_triggered()
{
	QString filename = QFileDialog::getOpenFileName(this, "Load orientation image", 
		m_imgInputPath, "Images (*.png *.jpg *.bmp *.tif *.tiff)");

	if (filename.isNull())
		return;

	if (m_hairImage.loadOrientationChai(filename) == false)
	{
		QMessageBox::critical(this, "Error", "Cannot load mask.");
		return;
	}

	hairImageUpdated();
}


void HairLayers::on_actionLoadOrientWang_triggered()
{
	QString filename = QFileDialog::getOpenFileName(this, "Load orientation image", 
		m_imgInputPath, "Images (*.png *.jpg *.bmp *.tif *.tiff)");

	if (filename.isNull())
		return;

	if (m_hairImage.loadOrientationWang(filename) == false)
	{
		QMessageBox::critical(this, "Error", "Cannot load mask.");
		return;
	}

	hairImageUpdated();
}

void HairLayers::on_actionSaveCurrentImage_triggered()
{
	QString filename = QFileDialog::getSaveFileName(this, "Save current view", 
		m_imgOutputPath, "Images (*.png *.jpg *.bmp)");

	if (filename.isNull())
		return;

	// Update image path.
	m_imgOutputPath = filename.left(filename.lastIndexOf('/') + 1);

	if (m_scene->imageSprite()->saveImage(filename) == false)
	{
		QMessageBox::critical(this, "Error", "Cannot save image.");
	}
}


//void HairLayers::on_buttonSaveHairSVG_clicked()
//{
//	QString filename = QFileDialog::getSaveFileName(this, "Save strands to SVG", 
//		m_imgOutputPath, "SVG (*.svg)");
//
//	if (filename.isNull())
//		return;
//
//	// Update image path.
//	m_imgOutputPath = filename.left(filename.lastIndexOf('/') + 1);
//
//	if (m_hairImage.saveStrandsSVG(filename) == false)
//	{
//		QMessageBox::critical(this, "Error", "Cannot save image.");
//	}
//}

void HairLayers::on_actionSaveSparseHairGeo_triggered()
{
	saveHairStrands(m_scene->strandModel(), false);
}

void HairLayers::on_actionSaveDenseHairGeo_triggered()
{
	saveHairStrands(m_scene->auxStrandModel(), false);
}

void HairLayers::on_actionSaveExtraHairGeo_triggered()
{
	saveHairStrands(m_scene->denseStrandModel(), false);
}

void HairLayers::on_actionSaveSparseHairGeoColor_triggered()
{
	saveHairStrands(m_scene->strandModel(), true);
}

void HairLayers::on_actionSaveDenseHairGeoColor_triggered()
{
	saveHairStrands(m_scene->auxStrandModel(), true);
}

void HairLayers::on_actionSaveExtraHairGeoColor_triggered()
{
	saveHairStrands(m_scene->denseStrandModel(), true);
}


void HairLayers::on_actionAutoSave3DStrands_triggered()
{
	QString fn = m_scnInputPath + "final_sparse.shd2";
	if (!m_scene->strandModel()->save(fn))
	{
		QMessageBox::warning(this, "Warning", "Sparse strands not saved.");
	}

	fn = m_scnInputPath + "final_dense.shd2";
	if (!m_scene->auxStrandModel()->save(fn))
	{
		QMessageBox::warning(this, "Warning", "Dense strands not saved.");
	}

	fn = m_scnInputPath + "final_extra.shd2";
	if (!m_scene->denseStrandModel()->save(fn))
	{
		QMessageBox::warning(this, "Warning", "Extra strands not saved.");
	}
}


void HairLayers::on_actionRenderToFile_triggered()
{
	QString filename = QFileDialog::getSaveFileName(this, "Render to image file", 
						m_scnInputPath, "PNG Images (*.png)");

	if (filename.isNull())
		return;

	RenderToFileParams params;
	params.msSampleCount = ui.spinBoxMsaaSamples->value();
	params.msQuality	 = ui.spinBoxMsaaQuality->value();

	if (!m_scene->renderToFile(filename, params))
	{
		QMessageBox::critical(this, "Error", "Failed to render to file.");
	}
}


void HairLayers::on_actionComputeAO_triggered()
{
	m_sceneWidget->stopTimer();

	m_scene->calcHeadOcclusionByHair();
	m_scene->calcNeckOcclusionByHair();
	m_scene->calcBodyOcclusionByHair();

	m_sceneWidget->startTimer(10);

	//const XMFLOAT3* pEye = m_scene->camera()->eye();
	//const XMFLOAT3* pAt = m_scene->camera()->lookAt();

	//XMFLOAT3 dir(pAt->x - pEye->x, pAt->y - pEye->y, pAt->z - pEye->z);

	//m_scene->calcVertexOcclusionByHair(
	//	m_scene->camera()->eye(), &dir);
}

void HairLayers::on_actionClearAO_triggered()
{

}


void HairLayers::saveHairStrands(HairStrandModel* pStrandModel, bool includeColor)
{
	QString filename = QFileDialog::getSaveFileName(this, "Save hair strands", 
		m_hairOutputPath, 
		includeColor ? "Color hair data (*.shd2)" : "Simple Hair Data (*.shd)");

	if (filename.isNull())
		return;

	// Update default path
	m_hairOutputPath = filename.left(filename.lastIndexOf('/') + 1);

	if (includeColor)
	{
		if (!pStrandModel->save(filename))
			QMessageBox::critical(this, "Error", "Failed to save strands with color.");
	}
	else
	{
		if (!pStrandModel->saveGeometry(filename))
			QMessageBox::critical(this, "Error", "Failed to save strands.");
	}
}


void HairLayers::on_actionLoadHairStrands_triggered()
{
	QString filename = QFileDialog::getOpenFileName(this, "Load hair strands", 
		m_hairInputPath, "Simple Hair Data (*.shd)");

	if (filename.isNull())
		return;

	// Update default path
	m_hairInputPath = filename.left(filename.lastIndexOf('/') + 1);

	if (m_scene->strandModel()->loadGeometry(filename) == false)
	{
		QMessageBox::critical(this, "Error", "Cannot load file.");
	}
}


void HairLayers::on_actionSaveHairStrands_triggered()
{
	QString filename = QFileDialog::getSaveFileName(this, "Save hair strands", 
		m_hairOutputPath, "Simple Hair Data (*.shd)");

	if (filename.isNull())
		return;

	// Update default path
	m_hairOutputPath = filename.left(filename.lastIndexOf('/') + 1);

	if (m_scene->strandModel()->saveGeometry(filename) == false)
	{
		QMessageBox::critical(this, "Error", "Cannot save file.");
	}
}


void HairLayers::on_actionLoadAuxStrands_triggered()
{
	QString filename = QFileDialog::getOpenFileName(this, "Load hair strands", 
		m_imgInputPath, "Simple Hair Data (*.shd)");

	if (filename.isNull())
		return;

	// Update default path
	m_hairInputPath = filename.left(filename.lastIndexOf('/') + 1);

	if (m_scene->auxStrandModel()->loadGeometry(filename) == false)
	{
		QMessageBox::critical(this, "Error", "Cannot load file.");
	}
}


void HairLayers::on_actionSaveAuxStrands_triggered()
{
	QString filename = QFileDialog::getSaveFileName(this, "Save hair strands", 
		m_imgOutputPath, "Simple Hair Data (*.shd)");

	if (filename.isNull())
		return;

	// Update default path
	m_hairOutputPath = filename.left(filename.lastIndexOf('/') + 1);

	if (m_scene->auxStrandModel()->saveGeometry(filename) == false)
	{
		QMessageBox::critical(this, "Error", "Cannot save file.");
	}
}


void HairLayers::on_actionLoadHeadMesh_triggered()
{
	QString filename = QFileDialog::getOpenFileName(this, "Load head mesh", 
		m_imgInputPath, "Mesh formats (*.obj)");

	if (filename.isNull())
		return;

	if (m_scene->loadHeadMesh(filename) == false)
	{
		QMessageBox::critical(this, "Error", "Cannot load head mesh!");
	}
}


void HairLayers::on_actionCalcOrientation_triggered()
{
	qDebug() << "CalcOrientation";

	if (m_hairImage.isColorNull())
	{
		QMessageBox::warning(this, "HairLayers", "Load color image first!");
		return;
	}

	// Retrieve params from UI

	PreprocessParam prepParams;
	prepParams.preprocessMethod = (PreprocessMethod)ui.comboPrepMethod->currentIndex();
	prepParams.sigmaHigh		= ui.spinBoxPrepSigmaH->value();
	prepParams.sigmaLow			= ui.spinBoxPrepSigmaL->value();	

	OrientationParam orientParams;
	orientParams.method = (OrientationMethod)ui.comboOrientMethod->currentIndex();
	orientParams.kernelWidth	= ui.spinBoxKernelWidth->value();
	orientParams.kernelHeight	= ui.spinBoxKernelHeight->value();
	orientParams.sigmaX			= ui.spinBoxGaborSigmaX->value();
	orientParams.sigmaY			= ui.spinBoxGaborSigmaY->value();
	orientParams.lambda			= ui.spinBoxGaborLambda->value();
	orientParams.phase			= ui.spinBoxPhase->value() * CV_PI / 180.0f;
	orientParams.numKernels		= ui.spinBoxNumKernels->value();
	orientParams.numPhases		= ui.spinBoxNumPhases->value();
	orientParams.saveKernels	= ui.checkBoxSaveKernels->isChecked();

	ConfidenceParam confidParams;
	confidParams.confidMethod		= (ConfidenceMethod)ui.comboConfidMethod->currentIndex();
	confidParams.clampConfidLow	= ui.spinBoxClampConfidLow->value();
	confidParams.clampConfidHigh	= ui.spinBoxClampConfidHigh->value();

	int numRefines = ui.spinBoxNumAutoRefineOrient->value();

	// Mid layer orientation first
	if (!m_hairImage.isMidColorNull())
	{
		SmoothingParam smoothParams;
		smoothParams.useSpatial		= ui.checkBoxSmoothSpatial->isChecked();
		smoothParams.useOrientation	= ui.checkBoxSmoothOrient->isChecked();
		smoothParams.useColor			= ui.checkBoxSmoothColor->isChecked();
		smoothParams.useConfidence	= ui.checkBoxSmoothConfidence->isChecked();
		smoothParams.sigmaSpatial		= ui.spinBoxSigmaSpatial->value();
		smoothParams.sigmaOrientation	= ui.spinBoxSigmaOrient->value();
		smoothParams.sigmaColor		= ui.spinBoxSigmaColor->value();
		smoothParams.sigmaConfidence	= ui.spinBoxSigmaConfidence->value();
		smoothParams.useVarClamp		= ui.checkBoxClampVar->isChecked();
		smoothParams.varClampThreshold= ui.spinBoxClampVar->value();
		smoothParams.useMaxRespClamp	= ui.checkBoxClampMaxResp->isChecked();
		smoothParams.maxRespClampThreshold=ui.spinBoxClampMaxResp->value();

		smoothParams.method			= (SmoothingMethod)ui.comboSmoothMethod->currentIndex();
		smoothParams.confidThreshold	= ui.spinBoxConfidThreshold->value();
		smoothParams.diffuseIterations= ui.spinBoxDiffuseIterations->value();

		m_hairImage.calcTempSmoothOrientation(prepParams, orientParams, numRefines, confidParams, smoothParams);
	}

	// Normal processing...
	m_hairImage.calcOrientation(prepParams, orientParams);
	m_hairImage.calcConfidence(confidParams);

	// Orientation refinement.
	for (int i = 0; i < numRefines; i++)
	{
		m_hairImage.refineOrientation(orientParams);
		m_hairImage.calcConfidence(confidParams);
	}

	hairImageUpdated();
}


void HairLayers::on_actionRefineOrientation_triggered()
{
	OrientationParam orientParams;
	orientParams.method = (OrientationMethod)ui.comboOrientMethod->currentIndex();
	orientParams.kernelWidth	= ui.spinBoxKernelWidth->value();
	orientParams.kernelHeight	= ui.spinBoxKernelHeight->value();
	orientParams.sigmaX			= ui.spinBoxGaborSigmaX->value();
	orientParams.sigmaY			= ui.spinBoxGaborSigmaY->value();
	orientParams.lambda			= ui.spinBoxGaborLambda->value();
	orientParams.phase			= ui.spinBoxPhase->value() * CV_PI / 180.0f;
	orientParams.numKernels		= ui.spinBoxNumKernels->value();
	orientParams.numPhases		= ui.spinBoxNumPhases->value();
	orientParams.saveKernels	= ui.checkBoxSaveKernels->isChecked();

	ConfidenceParam confidParams;
	confidParams.confidMethod	= (ConfidenceMethod)ui.comboConfidMethod->currentIndex();
	confidParams.clampConfidLow	= ui.spinBoxClampConfidLow->value();
	confidParams.clampConfidHigh= ui.spinBoxClampConfidHigh->value();

	m_hairImage.refineOrientation(orientParams);
	m_hairImage.calcConfidence(confidParams);

	hairImageUpdated();
}


void HairLayers::on_actionUpdateConfidence_triggered()
{
	ConfidenceParam params;
	params.confidMethod		= (ConfidenceMethod)ui.comboConfidMethod->currentIndex();
	params.clampConfidLow	= ui.spinBoxClampConfidLow->value();
	params.clampConfidHigh	= ui.spinBoxClampConfidHigh->value();

	m_hairImage.calcConfidence(params);

	hairImageUpdated();
}


void HairLayers::on_actionCalcDenseOrientation_triggered()
{
	SmoothingParam params;
	params.useSpatial		= ui.checkBoxSmoothSpatial->isChecked();
	params.useOrientation	= ui.checkBoxSmoothOrient->isChecked();
	params.useColor			= ui.checkBoxSmoothColor->isChecked();
	params.useConfidence	= ui.checkBoxSmoothConfidence->isChecked();
	params.sigmaSpatial		= ui.spinBoxSigmaSpatial->value();
	params.sigmaOrientation	= ui.spinBoxSigmaOrient->value();
	params.sigmaColor		= ui.spinBoxSigmaColor->value();
	params.sigmaConfidence	= ui.spinBoxSigmaConfidence->value();
	params.useVarClamp		= ui.checkBoxClampVar->isChecked();
	params.varClampThreshold= ui.spinBoxClampVar->value();
	params.useMaxRespClamp	= ui.checkBoxClampMaxResp->isChecked();
	params.maxRespClampThreshold=ui.spinBoxClampMaxResp->value();

	params.method			= (SmoothingMethod)ui.comboSmoothMethod->currentIndex();
	params.confidThreshold	= ui.spinBoxConfidThreshold->value();
	params.diffuseIterations= ui.spinBoxDiffuseIterations->value();

	m_hairImage.smoothOrientation(params);

	hairImageUpdated();
}



void HairLayers::on_actionDetectFeatures_triggered()
{
	FeatureDetectParam params;
	params.neighborOffset		= ui.spinBoxNbrOffset->value();
	params.ridgeDiffThreshold	= ui.spinBoxRidgeDiff->value();
	params.highConfidence			= ui.spinBoxTraceHighVar->value();
	params.lowConfidence			= ui.spinBoxTraceLowVar->value();

	m_hairImage.clearCoverage();
	m_hairImage.detectLineFeatures(params);

	hairImageUpdated();
}

void HairLayers::on_actionTraceStrands_triggered()
{
	FeatureDetectParam featureParams;
	featureParams.neighborOffset		= ui.spinBoxNbrOffset->value();
	featureParams.ridgeDiffThreshold	= ui.spinBoxRidgeDiff->value();
	featureParams.highConfidence			= ui.spinBoxTraceHighVar->value();
	featureParams.lowConfidence			= ui.spinBoxTraceLowVar->value();

	m_hairImage.clearCoverage();
	m_hairImage.detectLineFeatures(featureParams);

	TracingParam params;
	params.seedDensity		= ui.spinBoxDensity->value();
	params.numRelaxations	= ui.spinBoxNumRelaxations->value();
	params.relaxForce		= ui.spinBoxRelaxForce->value();
	params.highConfidence	= ui.spinBoxTraceHighVar->value();
	params.lowConfidence	= ui.spinBoxTraceLowVar->value();
	params.healthPoint	= ui.spinBoxLaTermSteps->value();
	params.stepLength		= ui.spinBoxStepLen->value();
	params.maxLength		= ui.spinBoxMaxLen->value();
	params.minLength		= ui.spinBoxMinLen->value();
	params.maxBendAngle		= CV_PI * ui.spinBoxMaxBend->value() / 180.0f;
	params.randomSeedOrder	= ui.checkBoxTraceRandomOrder->isChecked();
	params.lengthClamping	= ui.checkBoxTraceLengthClamp->isChecked();
	params.correctToRidge	= ui.checkBoxCorrectToRidge->isChecked();
	params.ridgeRadius		= ui.spinBoxRidgeRadius->value();
	params.maxCorrection	= ui.spinBoxMaxCorrection->value();
	params.centerCoverage	= ui.spinBoxTraceCenterCov->value();
	params.coverageSigma	= ui.spinBoxTraceCovSigma->value();

	params.minAuxCoverage	= ui.spinBoxMinAuxCoverage->value();
	params.maxAuxCoverage	= ui.spinBoxMaxAuxCoverage->value();

	m_hairImage.traceSparseStrands(params, m_scene->strandModel());

	hairImageUpdated();
}


void HairLayers::on_actionTraceAuxStrands_triggered()
{
	TracingParam params;
	params.seedDensity		= ui.spinBoxDensity->value();
	params.numRelaxations	= ui.spinBoxNumRelaxations->value();
	params.relaxForce		= ui.spinBoxRelaxForce->value();
	params.highConfidence		= ui.spinBoxTraceHighVar->value();
	params.lowConfidence		= ui.spinBoxTraceLowVar->value();
	params.healthPoint	= ui.spinBoxLaTermSteps->value();
	params.stepLength		= ui.spinBoxStepLen->value();
	params.maxLength		= ui.spinBoxMaxLen->value();
	params.minLength		= ui.spinBoxMinLen->value();
	params.maxBendAngle		= CV_PI * ui.spinBoxMaxBend->value() / 180.0f;
	params.randomSeedOrder	= ui.checkBoxTraceRandomOrder->isChecked();
	params.lengthClamping	= ui.checkBoxTraceLengthClamp->isChecked();
	params.correctToRidge	= ui.checkBoxCorrectToRidge->isChecked();
	params.ridgeRadius		= ui.spinBoxRidgeRadius->value();
	params.maxCorrection	= ui.spinBoxMaxCorrection->value();
	params.centerCoverage	= ui.spinBoxTraceCenterCov->value();
	params.coverageSigma	= ui.spinBoxTraceCovSigma->value();

	params.minAuxCoverage	= ui.spinBoxMinAuxCoverage->value();
	params.maxAuxCoverage	= ui.spinBoxMaxAuxCoverage->value();

	m_hairImage.traceAuxStrands(params, m_scene->auxStrandModel());

	hairImageUpdated();
}


void HairLayers::on_actionClearStrands_triggered()
{
	m_scene->strandModel()->clear();
}

void HairLayers::on_actionClearAuxStrands_triggered()
{
	m_scene->auxStrandModel()->clear();
}


void HairLayers::on_actionHairStroke_toggled(bool checked)
{
	if (checked)
	{
		// uncheck others
		ui.actionFaceStroke->setChecked(false);
		ui.actionBackgroundStroke->setChecked(false);

		m_scene->strokesSprite()->setStrokeId(1);
	}
	else
	{
		if (!ui.actionFaceStroke->isChecked() &&
			!ui.actionBackgroundStroke->isChecked())
		{
			m_scene->strokesSprite()->setStrokeId(-1);
		}
	}
}


void HairLayers::on_actionFaceStroke_toggled(bool checked)
{
	if (checked)
	{
		// uncheck others
		ui.actionHairStroke->setChecked(false);
		ui.actionBackgroundStroke->setChecked(false);

		m_scene->strokesSprite()->setStrokeId(2);
	}
	else
	{
		if (!ui.actionHairStroke->isChecked() &&
			!ui.actionBackgroundStroke->isChecked())
		{
			m_scene->strokesSprite()->setStrokeId(-1);
		}
	}
}


void HairLayers::on_actionBackgroundStroke_toggled(bool checked)
{
	if (checked)
	{
		// uncheck others
		ui.actionFaceStroke->setChecked(false);
		ui.actionHairStroke->setChecked(false);

		m_scene->strokesSprite()->setStrokeId(3);
	}
	else
	{
		if (!ui.actionFaceStroke->isChecked() &&
			!ui.actionHairStroke->isChecked())
		{
			m_scene->strokesSprite()->setStrokeId(-1);
		}
	}
}

//--------------------------------------------------------------
// Response to other UI events
//--------------------------------------------------------------

void HairLayers::on_buttonCalcOrientation_clicked()
{
	on_actionCalcOrientation_triggered();
}

void HairLayers::on_buttonRecalcOrientFromConfid_clicked()
{
	on_actionRefineOrientation_triggered();
}

void HairLayers::on_buttonUpdateConfidence_clicked()
{
	on_actionUpdateConfidence_triggered();
}

void HairLayers::on_buttonSmoothTensor_clicked()
{
	on_actionCalcDenseOrientation_triggered();
}

void HairLayers::on_buttonDetectFeatures_clicked()
{
	on_actionDetectFeatures_triggered();
}

void HairLayers::on_buttonTraceByFeatures_clicked()
{
	on_actionTraceStrands_triggered();
}

void HairLayers::on_buttonTraceAuxStrands_clicked()
{
	on_actionTraceAuxStrands_triggered();
}

void HairLayers::on_buttonClearStrands_clicked()
{
	on_actionClearStrands_triggered();
}

void HairLayers::on_buttonClearAuxStrands_clicked()
{
	on_actionClearAuxStrands_triggered();
}


void HairLayers::on_buttonCalcFrontDepth_clicked()
{
	TracingParam params;
	params.dsDepthMapSigma = 0.5f;

	m_hairImage.calcFrontDepth(params, m_scene->auxStrandModel());

	hairImageUpdated();
}

void HairLayers::on_buttonCalcBackDepth_clicked()
{
	//TracingParam params;

//	m_hairImage.calcBackDepth(params, m_scene->auxStrandModel());

	//hairImageUpdated();
}

void HairLayers::on_buttonGenDenseStrands_clicked()
{
	TracingParam params;
	params.seedDensity		= ui.spinBoxDensity->value();
	params.numRelaxations	= ui.spinBoxNumRelaxations->value();
	params.relaxForce		= ui.spinBoxRelaxForce->value();
	params.highConfidence	= ui.spinBoxTraceHighVar->value();
	params.lowConfidence	= ui.spinBoxTraceLowVar->value();
	params.healthPoint	= ui.spinBoxLaTermSteps->value();
	params.stepLength		= ui.spinBoxStepLen->value();
	params.maxLength		= ui.spinBoxMaxLen->value();
	params.minLength		= ui.spinBoxMinLen->value();
	params.maxBendAngle		= CV_PI * ui.spinBoxMaxBend->value() / 180.0f;
	params.randomSeedOrder	= ui.checkBoxTraceRandomOrder->isChecked();
	params.lengthClamping	= ui.checkBoxTraceLengthClamp->isChecked();
	params.correctToRidge	= ui.checkBoxCorrectToRidge->isChecked();
	params.ridgeRadius		= ui.spinBoxRidgeRadius->value();
	params.maxCorrection	= ui.spinBoxMaxCorrection->value();
	params.centerCoverage	= ui.spinBoxTraceCenterCov->value();
	params.coverageSigma	= ui.spinBoxTraceCovSigma->value();

	params.minAuxCoverage	= ui.spinBoxMinAuxCoverage->value();
	params.maxAuxCoverage	= ui.spinBoxMaxAuxCoverage->value();

	params.dsNumFrontLayers	= ui.spinBoxDsNumFrontLayers->value();
	params.dsNumBackLayers	= ui.spinBoxDsNumBackLayers->value();
	params.dsLayerThickness	= ui.spinBoxDsLayerThickness->value();
	params.dsDepthMapSigma	= ui.spinBoxDsDepthMapSigma->value();
	params.dsStepLen		= ui.spinBoxDsStepLen->value();
	params.dsHealthPoint	= ui.spinBoxDsHealth->value();
	params.dsCenterCoverage = ui.spinBoxDsCenterCoverage->value();
	params.dsCoverageSigma	= ui.spinBoxDsCoverageSigma->value();
	params.dsMinCoverage	= ui.spinBoxDsMinCoverage->value();
	params.dsMaxCoverage	= ui.spinBoxDsMaxCoverage->value();
	params.dsDepthVariation	= ui.spinBoxDsDepthVar->value();
	params.dsSilhoutteRadius= ui.spinBoxFalloffRadius->value();
	params.dsSilhoutteDepth	= ui.spinBoxFalloffDepth->value();
	params.dsLengthClamping = ui.checkBoxDsLenClamp->isChecked();
	params.dsMinLength		= ui.spinBoxDsMinLen->value();
	params.dsMaskShrinkDelta= ui.spinBoxDsShrinkDelta->value();

	params.esBackHalfScale	= ui.spinBoxEsBackHalfScale->value();
	params.esBackHalfOffset	= ui.spinBoxEsBackHalfOffset->value();

	params.esNumMidLayers	= ui.spinBoxEsNumMidLayers->value();

	m_hairImage.genDenseStrands(params, m_scene->auxStrandModel(), 
										m_scene->denseStrandModel());

	hairImageUpdated();
}

void HairLayers::on_buttonClearDenseStrands_clicked()
{
	m_scene->denseStrandModel()->clear();
}

void HairLayers::on_comboPrepMethod_currentIndexChanged(int idx)
{
	PreprocessMethod method = (PreprocessMethod)idx;

	if (method == PreprocessNone)
	{
		ui.spinBoxPrepSigmaH->setEnabled(false);
		ui.spinBoxPrepSigmaL->setEnabled(false);
		ui.labelPrepSigma1->setEnabled(false);
		ui.labelPrepSigma2->setEnabled(false);
		ui.labelPrepSigma1->setText("n/a");
		ui.labelPrepSigma2->setText("n/a");
		return;
	}
	else if (method == PreprocessGaussian)
	{
		ui.spinBoxPrepSigmaH->setEnabled(true);
		ui.spinBoxPrepSigmaL->setEnabled(false);
		ui.labelPrepSigma1->setEnabled(true);
		ui.labelPrepSigma2->setEnabled(false);
		ui.labelPrepSigma1->setText("Sigma:");
		ui.labelPrepSigma2->setText("n/a");
	}
	else if (method == PreprocessDoG)
	{
		ui.spinBoxPrepSigmaH->setEnabled(true);
		ui.spinBoxPrepSigmaL->setEnabled(true);
		ui.labelPrepSigma1->setEnabled(true);
		ui.labelPrepSigma2->setEnabled(true);
		ui.labelPrepSigma1->setText("Sigma H.:");
		ui.labelPrepSigma2->setText("Sigma L.:");
	}
	else if (method == PreprocessBilateral)
	{
		ui.spinBoxPrepSigmaH->setEnabled(true);
		ui.spinBoxPrepSigmaL->setEnabled(true);
		ui.labelPrepSigma1->setEnabled(true);
		ui.labelPrepSigma2->setEnabled(true);
		ui.labelPrepSigma1->setText("Sig. spt.:");
		ui.labelPrepSigma2->setText("Sig. clr:");
	}
}


// Old tracing algorithm
void HairLayers::on_buttonTraceStrands_clicked()
{
	qDebug() << "TraceHairStrands";

	if (m_hairImage.isOrientNull())
	{
		QMessageBox::warning(this, "HairLayers", "Calculate orientation map first!");
		return;
	}

	TracingParam params;
	params.seedDensity	= ui.spinBoxDensity->value();
	params.numRelaxations=ui.spinBoxNumRelaxations->value();
	params.relaxForce	= ui.spinBoxRelaxForce->value();
	params.stepLength	= ui.spinBoxStepLen->value();
	params.maxLength	= ui.spinBoxMaxLen->value();
	params.minLength	= ui.spinBoxMinLen->value();
	params.maxBendAngle	= CV_PI * ui.spinBoxMaxBend->value() / 180.0f;

	m_hairImage.traceStrandsOld(params, m_scene->strandModel());
}


void HairLayers::on_buttonBgColor_clicked()
{
	QColor origColor = m_sceneWidget->backgroundColor();
	QColor newColor = QColorDialog::getColor(origColor, this);

	if (newColor.isValid())
	{
		m_sceneWidget->setBackgroundColor(newColor);
	}
}


void HairLayers::on_buttonHairColor_clicked()
{
	QColor newColor = QColorDialog::getColor(m_scene->hairColor(), this);

	if (newColor.isValid())
	{
		m_scene->setHairColor(newColor);
	}
}


void HairLayers::on_buttonFilterStrandsColor_clicked()
{
	SampleColorParam params;
	params.blurRadius = ui.spinBoxFilterStrandsColorSigma->value();
	params.sampleAlpha = true;

	m_hairImage.sampleStrandColor(params, m_scene->strandModel());

	m_scene->strandModel()->updateBuffers();
}


void HairLayers::on_buttonFilterStrandsDepth_clicked()
{
	HairFilterParam params;
	params.sigmaDepth = ui.spinBoxFilterStrandsDepthSigma->value();

	m_scene->strandModel()->filterDepth(params);
	m_scene->strandModel()->updateBuffers();
}


void HairLayers::on_buttonFilterStrandsShape_clicked()
{
	// TODO...
}


void HairLayers::on_buttonFilterAuxStrandsColor_clicked()
{
	SampleColorParam params;
	params.blurRadius = ui.spinBoxFilterAuxStrandsColorSigma->value();
	params.sampleAlpha = true;

	m_hairImage.sampleStrandColor(params, m_scene->auxStrandModel());

	m_scene->auxStrandModel()->updateBuffers();
}


void HairLayers::on_buttonFilterAuxStrandsDepth_clicked()
{
	HairFilterParam params;
	params.sigmaDepth = ui.spinBoxFilterAuxStrandsDepthSigma->value();

	m_scene->auxStrandModel()->filterDepth(params);
	m_scene->auxStrandModel()->updateBuffers();
}


void HairLayers::on_buttonFilterDenseStrandsColor_clicked()
{
	SampleColorParam params;
	params.blurRadius = ui.spinBoxFilterDenseStrandsColorSigma->value();
	params.sampleAlpha = true;

	m_hairImage.sampleStrandColor(params, m_scene->denseStrandModel());

	m_scene->denseStrandModel()->updateBuffers();

}


void HairLayers::on_buttonFilterDenseStrandsDepth_clicked()
{
	HairFilterParam params;
	params.sigmaDepth = ui.spinBoxFilterDenseStrandsDepthSigma->value();

	m_scene->denseStrandModel()->filterDepth(params);
	m_scene->denseStrandModel()->updateBuffers();
}

void HairLayers::on_buttonMsaaApply_clicked()
{
	uint count = ui.spinBoxMsaaSamples->value();
	uint quality = ui.spinBoxMsaaQuality->value();

	m_sceneWidget->setMultisampleQuality(count, quality);
}


void HairLayers::on_spinBoxTanTexWeight_valueChanged(double val)
{
	m_scene->hairRenderer()->setTangentTexture(
		ui.spinBoxTanTexWeight->value(),
		ui.spinBoxTanTexScaleX->value(),
		ui.spinBoxTanTexScaleY->value());
}


void HairLayers::on_spinBoxTanTexScaleX_valueChanged(double val)
{
	m_scene->hairRenderer()->setTangentTexture(
		ui.spinBoxTanTexWeight->value(),
		ui.spinBoxTanTexScaleX->value(),
		ui.spinBoxTanTexScaleY->value());
}

void HairLayers::on_spinBoxTanTexScaleY_valueChanged(double val)
{
	m_scene->hairRenderer()->setTangentTexture(
		ui.spinBoxTanTexWeight->value(),
		ui.spinBoxTanTexScaleX->value(),
		ui.spinBoxTanTexScaleY->value());
}

void HairLayers::on_spinBoxNormalGradWeight_valueChanged(double val)
{
	m_scene->hairRenderer()->setNormalGradient(
		ui.spinBoxNormalGradWeight->value());
}


void HairLayers::on_spinBoxStrandRandWeight_valueChanged(double val)
{
	m_scene->hairRenderer()->setStrandRandWeight(val);

	m_scene->hairMorphRenderer()->setStrandRandWeight(val);
}

void HairLayers::on_spinBoxForceOpaque_valueChanged(double val)
{
	m_scene->hairRenderer()->setForceOpaque(
		ui.spinBoxForceOpaque->value());
}

void HairLayers::on_spinBoxHairWidth_valueChanged(double val)
{
	m_scene->strandModel()->setStrandWidth(val);
}

void HairLayers::on_spinBoxAuxHairWidth_valueChanged(double val)
{
	m_scene->auxStrandModel()->setStrandWidth(val);
}

void HairLayers::on_spinBoxDenseHairWidth_valueChanged(double val)
{
	m_scene->denseStrandModel()->setStrandWidth(val);
}

void HairLayers::on_spinBoxStrandsDepthScale_valueChanged(double scale)
{
	m_scene->strandModel()->setScale(1, -1, scale);
}

void HairLayers::on_spinBoxAuxStrandsDepthScale_valueChanged(double scale)
{
	m_scene->auxStrandModel()->setScale(1, -1, scale);
	m_scene->denseStrandModel()->setScale(1, -1, scale);
}

void HairLayers::on_spinBoxStrandsDepthOffset_valueChanged(double depth)
{
	m_scene->strandModel()->setLocation(0, 0, depth);
}

void HairLayers::on_spinBoxAuxStrandsDepthOffset_valueChanged(double depth)
{
	m_scene->auxStrandModel()->setLocation(0, 0, depth);
	m_scene->denseStrandModel()->setLocation(0, 0, depth);
}

void HairLayers::on_sliderImageOpacity_valueChanged(int val)
{
	float opacity = (float)val / 100.0f;

	m_scene->setImageOpacity(opacity);
}


void HairLayers::on_checkBoxImage_stateChanged(int state)
{
	m_scene->imageSprite()->setVisible(state);
}

void HairLayers::on_checkBoxHair_stateChanged(int state)
{
	m_scene->strandModel()->setVisible(state);
}

void HairLayers::on_checkBoxAuxHair_stateChanged(int state)
{
	m_scene->auxStrandModel()->setVisible(state);
}

void HairLayers::on_checkBoxDenseHair_stateChanged(int state)
{
	m_scene->denseStrandModel()->setVisible(state);
}

void HairLayers::on_checkBoxFrame_stateChanged(int state)
{
	m_scene->setCoordFrameVisible(state);
}

void HairLayers::on_checkBoxHead_stateChanged(int state)
{
	m_scene->headRenderer()->setVisible(state);
}

void HairLayers::on_checkBoxNeck_stateChanged(int state)
{
	m_scene->neckRenderer()->setVisible(state);
}

void HairLayers::on_checkBoxStrokes_stateChanged(int state)
{
	m_scene->strokesSprite()->setVisible(state);
}

void HairLayers::on_comboImageView_currentIndexChanged(int idx)
{
	hairImageUpdated();
}


void HairLayers::on_comboHairRenderMode_currentIndexChanged(int idx)
{
	m_scene->setHairRenderMode((HairRenderMode)idx);
}


void HairLayers::on_comboHeadRenderMode_currentIndexChanged(int idx)
{
	m_scene->headRenderer()->setRenderMode((HeadRenderMode)idx);
}

void HairLayers::on_hairImage_doubleClicked(float x, float y)
{
	statusBar()->showMessage(QString("Double clicked at (%1, %2)").arg(x).arg(y));

	// TEST: Trace single strand
	if (m_hairImage.isOrientNull())
	{
		QMessageBox::warning(this, "HairLayers", "Calculate orientation map first!");
		return;
	}

	TracingParam params;
	params.seedDensity		= ui.spinBoxDensity->value();
	params.numRelaxations	= ui.spinBoxNumRelaxations->value();
	params.relaxForce		= ui.spinBoxRelaxForce->value();
	params.highConfidence		= ui.spinBoxTraceHighVar->value();
	params.lowConfidence		= ui.spinBoxTraceLowVar->value();
	params.healthPoint	= ui.spinBoxLaTermSteps->value();
	params.stepLength		= ui.spinBoxStepLen->value();
	params.maxLength		= ui.spinBoxMaxLen->value();
	params.minLength		= ui.spinBoxMinLen->value();
	params.maxBendAngle		= CV_PI * ui.spinBoxMaxBend->value() / 180.0f;
	params.randomSeedOrder	= ui.checkBoxTraceRandomOrder->isChecked();
	params.lengthClamping	= ui.checkBoxTraceLengthClamp->isChecked();
	params.correctToRidge	= ui.checkBoxCorrectToRidge->isChecked();
	params.ridgeRadius		= ui.spinBoxRidgeRadius->value();
	params.maxCorrection	= ui.spinBoxMaxCorrection->value();
	params.centerCoverage	= ui.spinBoxTraceCenterCov->value();
	params.coverageSigma	= ui.spinBoxTraceCovSigma->value();

	params.minAuxCoverage	= ui.spinBoxMinAuxCoverage->value();
	params.maxAuxCoverage	= ui.spinBoxMaxAuxCoverage->value();


	Strand strand;
	m_hairImage.traceSingleSparseStrand(x, y, params, &strand);

	if (strand.numVertices() < 2)
		return;

	HairStrandModel* pModel = m_scene->strandModel();

	pModel->addStrand(strand);
	pModel->updateBuffers();

	hairImageUpdated();
}


void HairLayers::on_checkBoxToggleOrigPrep_stateChanged(int state)
{
	hairImageUpdated();
}

void HairLayers::on_checkBoxToggleOrientTensor_stateChanged(int state)
{
	hairImageUpdated();
}

void HairLayers::on_checkBoxShowMask_stateChanged(int state)
{
	hairImageUpdated();
}

void HairLayers::on_checkBoxShowMaxResp_stateChanged(int state)
{
	hairImageUpdated();
}

void HairLayers::on_spinBoxCamFoV_valueChanged(int val)
{
	m_scene->camera()->setFieldOfView(
		CV_PI * (float)val / 180.0f);
}

void HairLayers::on_spinBoxCamZn_valueChanged(double val)
{
	m_scene->camera()->setClipPlanes(
		ui.spinBoxCamZn->value(), ui.spinBoxCamZf->value());
}

void HairLayers::on_spinBoxCamZf_valueChanged(double val)
{
	m_scene->camera()->setClipPlanes(
		ui.spinBoxCamZn->value(), ui.spinBoxCamZf->value());
}

void HairLayers::on_checkBoxBgLayer_stateChanged(int state)
{
	m_scene->backgroundLayer()->setVisible(state);
}

void HairLayers::on_checkBoxBodyLayer_stateChanged(int state)
{
	m_scene->bodyModel()->setVisible(state);
}

void HairLayers::on_checkBoxLargeBgLayer_stateChanged(int state)
{
	m_scene->largeBackgroundLayer()->setVisible(state);
}

void HairLayers::on_checkBoxOrigLayer_stateChanged(int state)
{
	m_scene->originalLayer()->setVisible(state);
}

void HairLayers::on_spinBoxBgLayerDepth_valueChanged(double val)
{
	m_scene->backgroundLayer()->setLocation(0, 0, val);

	const XMFLOAT3* oldPos = m_scene->largeBackgroundLayer()->location();

	m_scene->largeBackgroundLayer()->setLocation(oldPos->x, oldPos->y, val+0.5);
}

void HairLayers::on_spinBoxBodyLayerDepth_valueChanged(double val)
{
	m_scene->bodyModel()->setLocation(0, 0, val);
	m_scene->setBodyLayerDepth(val);
}

void HairLayers::on_actionProcessImage_triggered()
{
	if (m_hairImage.isColorNull())
	{
		QMessageBox::warning(this, "HairLayers", "Load color image first!");
		return;
	}

	PreprocessParam prepParams;
	prepParams.preprocessMethod = (PreprocessMethod)ui.comboPrepMethod->currentIndex();
	prepParams.sigmaHigh		= ui.spinBoxPrepSigmaH->value();
	prepParams.sigmaLow			= ui.spinBoxPrepSigmaL->value();	

	OrientationParam orientParams;
	orientParams.method = (OrientationMethod)ui.comboOrientMethod->currentIndex();
	orientParams.kernelWidth	= ui.spinBoxKernelWidth->value();
	orientParams.kernelHeight	= ui.spinBoxKernelHeight->value();
	orientParams.sigmaX			= ui.spinBoxGaborSigmaX->value();
	orientParams.sigmaY			= ui.spinBoxGaborSigmaY->value();
	orientParams.lambda			= ui.spinBoxGaborLambda->value();
	orientParams.phase			= ui.spinBoxPhase->value() * CV_PI / 180.0f;
	orientParams.numKernels		= ui.spinBoxNumKernels->value();
	orientParams.numPhases		= ui.spinBoxNumPhases->value();
	orientParams.saveKernels	= ui.checkBoxSaveKernels->isChecked();

	m_hairImage.calcOrientation(prepParams, orientParams);

	ConfidenceParam confidParams;
	confidParams.confidMethod		= (ConfidenceMethod)ui.comboConfidMethod->currentIndex();
	confidParams.clampConfidLow		= ui.spinBoxClampConfidLow->value();
	confidParams.clampConfidHigh	= ui.spinBoxClampConfidHigh->value();

	m_hairImage.calcConfidence(confidParams);
	
	FeatureDetectParam featureParams;
	featureParams.neighborOffset		= ui.spinBoxNbrOffset->value();
	featureParams.ridgeDiffThreshold	= ui.spinBoxRidgeDiff->value();
	featureParams.highConfidence			= ui.spinBoxTraceHighVar->value();
	featureParams.lowConfidence			= ui.spinBoxTraceLowVar->value();

	m_hairImage.detectLineFeatures(featureParams);

	TracingParam traceParams;
	traceParams.seedDensity		= ui.spinBoxDensity->value();
	traceParams.numRelaxations	= ui.spinBoxNumRelaxations->value();
	traceParams.relaxForce		= ui.spinBoxRelaxForce->value();
	traceParams.highConfidence	= ui.spinBoxTraceHighVar->value();
	traceParams.lowConfidence	= ui.spinBoxTraceLowVar->value();
	traceParams.healthPoint	= ui.spinBoxLaTermSteps->value();
	traceParams.stepLength		= ui.spinBoxStepLen->value();
	traceParams.maxLength		= ui.spinBoxMaxLen->value();
	traceParams.minLength		= ui.spinBoxMinLen->value();
	traceParams.maxBendAngle	= CV_PI * ui.spinBoxMaxBend->value() / 180.0f;
	traceParams.randomSeedOrder	= ui.checkBoxTraceRandomOrder->isChecked();
	traceParams.lengthClamping	= ui.checkBoxTraceLengthClamp->isChecked();
	traceParams.correctToRidge	= ui.checkBoxCorrectToRidge->isChecked();
	traceParams.ridgeRadius		= ui.spinBoxRidgeRadius->value();
	traceParams.maxCorrection	= ui.spinBoxMaxCorrection->value();
	traceParams.centerCoverage	= ui.spinBoxTraceCenterCov->value();
	traceParams.coverageSigma	= ui.spinBoxTraceCovSigma->value();

	m_hairImage.traceStrandsByFeatures(traceParams, m_scene->strandModel());

	hairImageUpdated();
}


void HairLayers::on_buttonAddSilhouetteFalloff_clicked()
{
	float radius = ui.spinBoxFalloffRadius->value();
	float depth  = ui.spinBoxFalloffDepth->value();

	m_hairImage.addSilhouetteDepthFalloff(radius, depth, m_scene->strandModel());
	m_hairImage.addSilhouetteDepthFalloff(radius, depth, m_scene->auxStrandModel());
}


void HairLayers::on_spinBoxNeckBlend_valueChanged(double val)
{
	m_scene->neckRenderer()->setNeckBlend(val);
}


void HairLayers::on_buttonAddGeoNoise_clicked()
{
	HairGeoNoiseParam params;
	params.amplitude = ui.spinBoxGeoNoiseAmp->value();
	params.frequency = ui.spinBoxGeoNoiseFreq->value();

	HairStrandModel* model = m_scene->strandModel();
	if (model)
		model->addGeometryNoise(params);

	model = m_scene->auxStrandModel();
	if (model)
		model->addGeometryNoise(params);

	model = m_scene->denseStrandModel();
	if (model)
		model->addGeometryNoise(params);
}


void HairLayers::on_buttonGeoSmooth_clicked()
{
	HairFilterParam params;
	params.sigmaDepth = ui.spinBoxGeoSmoothSigma->value();

	HairStrandModel* model = m_scene->strandModel();
	if (model)
		model->filterGeometry(params);

	model = m_scene->auxStrandModel();
	if (model)
		model->filterGeometry(params);

	model = m_scene->denseStrandModel();
	if (model)
		model->filterGeometry(params);
}


void HairLayers::on_buttonColorSmooth_clicked()
{
	HairFilterParam params;
	params.sigmaDepth = ui.spinBoxColorSmoothSigma->value();

	HairStrandModel* model = m_scene->strandModel();
	if (model)
		model->filterColorAlpha(params);

	model = m_scene->auxStrandModel();
	if (model)
		model->filterColorAlpha(params);

	model = m_scene->denseStrandModel();
	if (model)
		model->filterColorAlpha(params);
}


void HairLayers::on_spinBoxStrokeRadius_valueChanged(int r)
{
	m_scene->setStrokeRadius(r);
}

void HairLayers::on_spinBoxStrokeIntensity_valueChanged(double i)
{
	m_scene->setStrokeIntensity(i);
}

void HairLayers::on_spinBoxStrokeFuzziness_valueChanged(int f)
{
	m_scene->setStrokeFuzziness(f);
}

void HairLayers::on_spinBoxKa_valueChanged(double val)
{
	m_scene->hairRenderer()->setKa(val);
}

void HairLayers::on_spinBoxKd_valueChanged(double val)
{
	m_scene->hairRenderer()->setKd(val);
}

void HairLayers::on_spinBoxKr_valueChanged(double val)
{
	m_scene->hairRenderer()->setKr(val);
}

void HairLayers::on_spinBoxBodyAO_valueChanged(double val)
{
	m_scene->setBodyAO(val);
}

void HairLayers::on_spinBoxHairAO_valueChanged(double val)
{
	m_scene->setHairAO(val);
}


// slots for hair morphing UIs

// Load both hair models for morphing
void HairLayers::on_buttonLoadMorphInput_clicked()
{
	int numSrc = m_scene->morphController()->weights().size();

	// collect folder names from user
	QStringList folders;
	for (int i = numSrc; i < MAX_NUM_MORPH_SRC; i++)
	{
		QString folderName = QFileDialog::getExistingDirectory(this, 
								QString("Load source %1").arg(i), m_scnInputPath);
		if (folderName.isNull()) 
			break;
		folders.append(folderName);
	}

	if (folders.count() < 1)
		return;

	QDir dir(folders[0]);
	if (dir.cdUp())
		m_scnInputPath = dir.absolutePath();

	for (int i = 0; i < folders.count(); i++)
	{
		const int srcIdx = numSrc + i;

		printf("Loading source %d...\n", srcIdx);

		QString fnCoeff = folders[i] + "\\feature_coefficients.txt";
		int imgHeight = QImage(folders[i] + "\\original.png").height();

		MorphController* morphCtrl = m_scene->morphController();
		if (!morphCtrl->addSourceFromFile(fnCoeff, imgHeight))
		{
			printf("ERROR: Failed to load transformation!\n");
			return;
		}

		QString fnMesh = folders[i] + "\\fit.obj";
		if (!m_scene->headMorphModel()->loadAndRectifySource(srcIdx, fnMesh, morphCtrl->sourceTransform(srcIdx)))
		{
			printf("ERROR: Failed to load head model!\n");
			return;
		}

		QString fnTexture = folders[i] + "\\head.png";
		if (!m_scene->headMorphRenderer()->loadHeadTexture(srcIdx, fnTexture))
		{
			printf("ERROR: Failed to load head texture!\n");
			return;
		}

		// Load hair model
		QString fnHair = folders[i] + "\\final_extra.shd2";

		m_scene->hairMorphHierarchy()->sources().push_back(HairHierarchy());
		HairHierarchy& hierarchy = m_scene->hairMorphHierarchy()->sources().back();
		hierarchy.load(fnHair, false);
		hierarchy.level(0).rectifyVertices(morphCtrl->sourceTransform(srcIdx));

		printf("\n");
	}

	m_scene->resetCamera(800, 800);

	setWindowTitle(QString("HairLayers - ") + m_scnInputPath);

	m_polyWidget->setVertexCount(m_scene->morphController()->weights().size());
}


void HairLayers::on_buttonClearMorphInput_clicked()
{
	m_scene->hairMorphHierarchy()->clear();
	m_scene->headMorphModel()->clear();
	m_scene->morphController()->clear();

	m_polyWidget->setVertexCount(0);
}


void HairLayers::morphWeightsChanged(const std::vector<float>& weights)
{
	m_scene->morphController()->setWeights(weights);

	//if (ui.checkBoxShowMorphSrc->isChecked())
	//{
	//	for (int i = 0; i < weights.size(); i++)
	//	{
	//		m_scene->srcHairHierarchy(i)->setVisible(weights[i] == 1.0f);
	//	}
	//}
}


void HairLayers::on_spinBoxMorphStrandWidth_valueChanged(double w)
{
	m_scene->hairMorphRenderer()->setHairWidth(w);
	//m_scene->hairMorphModel()->morphStrandModel()->setStrandWidth(w);
}


void HairLayers::on_spinBoxCurrHairLevel_valueChanged(int i)
{
	//for (int iSrc = 0; iSrc < MAX_NUM_MORPH_SRC; iSrc++)
	//	m_scene->srcHairHierarchy(iSrc)->setCurrentLevelIndex(i);

	m_scene->hairMorphHierarchy()->setCurrentLevelIndex(i);
}


void HairLayers::on_checkBoxShowMorphSrcHair_stateChanged(int i)
{
//	m_scene->srcHairHierarchy(0)->setVisible((bool)i);
}


void HairLayers::on_checkBoxShowMorphDstHair_stateChanged(int i)
{
	//m_scene->srcHairHierarchy(1)->setVisible((bool)i);
}


void HairLayers::on_checkBoxShowMorphHair_stateChanged(int i)
{
	m_scene->hairMorphHierarchy()->setVisible((bool)i);
}


void HairLayers::on_checkBoxShowMorphHead_stateChanged(int i)
{
	m_scene->headMorphRenderer()->setVisible((bool)i);
}

#define PARALLEL_CLUSTERING

int  s_num_levels;
int  s_cluster_ratios[3];
bool s_cluster_relative[3];

DWORD WINAPI buildPyramid(LPVOID lpParam)
{
	HairHierarchy* pHierarchy = (HairHierarchy*)lpParam;

	// calculate sizes for each level
	std::vector<int> levelSizes(s_num_levels);
	int lastSize = pHierarchy->level(0).numStrands();
	for (int i = 0; i < s_num_levels; i++)
	{
		levelSizes[i] = s_cluster_relative[i] ? 
						lastSize / s_cluster_ratios[i] : s_cluster_ratios[i];
		lastSize = levelSizes[i];
	}

	pHierarchy->buildByFixedK(levelSizes);

	// update buffers
	for (int i = 0; i < s_num_levels; i++)
		pHierarchy->level(i).updateBuffers();

	return 0;
}

// build pyramids for all source hair models using fixed k method
void HairLayers::on_buttonBuildPyrFixK_clicked()
{
	// collect parameters from UI
	s_num_levels = ui.spinBoxNumPyrLevels->value();

	s_cluster_ratios[0] = ui.spinBoxLvl0ClusterRatio->value();
	s_cluster_ratios[1] = ui.spinBoxLvl1ClusterRatio->value();
	s_cluster_ratios[2] = ui.spinBoxLvl2ClusterRatio->value();
	
	s_cluster_relative[0] = ui.checkBoxLvl0Relative->isChecked();
	s_cluster_relative[1] = ui.checkBoxLvl1Relative->isChecked();
	s_cluster_relative[2] = ui.checkBoxLvl2Relative->isChecked();

	const int numSources = m_scene->hairMorphHierarchy()->numSources();

	QTime timer;
	timer.start();

	// resize hair models if necessary
	std::vector<HairHierarchy>& sources = m_scene->hairMorphHierarchy()->sources();
	for (int srcIdx = 0; srcIdx < numSources; srcIdx++)
	{
		int newSize = s_cluster_ratios[0];
		if (s_cluster_relative[0])
			newSize = sources[srcIdx].level(0).numStrands() / newSize;

		if (newSize != sources[srcIdx].level(0).numStrands())
		{
			// randomly pick k strands from level 0 strands and discard others
			HairStrandModel model;
			model.reserve(newSize);
		
			std::vector<int> indices(sources[srcIdx].level(0).numStrands());
			for (int i = 0; i < indices.size(); i++)
				indices[i] = i;
			for (int i = 0; i < newSize; i++)
			{
				int j = cv::theRNG().uniform(i, indices.size());
				std::swap(indices[i], indices[j]);

				model.addStrand(*(sources[srcIdx].level(0).getStrandAt(indices[i])));
			}
			sources[srcIdx].level(0) = model;
			sources[srcIdx].level(0).calcRootNbrsByANN(32);
		}
	}

#ifdef PARALLEL_CLUSTERING
	HANDLE* threads = new HANDLE[numSources];
#endif

	for (int i = 0; i < numSources; i++)
	{
		HairHierarchy* pHierarchy = &(m_scene->hairMorphHierarchy()->sources()[i]);
#ifdef PARALLEL_CLUSTERING
		threads[i] = CreateThread(NULL, 0, buildPyramid, (void*)pHierarchy, 0, NULL);
#else
		buildPyramid((void*)pHierarchy);
#endif
	}

#ifdef PARALLEL_CLUSTERING
	WaitForMultipleObjects(numSources, threads, TRUE, INFINITE);
	for (int i = 0; i < numSources; i++)
		CloseHandle(threads[i]);

	delete[] threads;
#endif

	printf("\nAll clustering done. (%.3f)\n", timer.elapsed()/1000.0f);
}


// build pyramids for both src and dst hair models using adaptive method
void HairLayers::on_buttonBuildPyrAdaptive_clicked()
{

}


// Generate morphable hair using single scale EMD
void HairLayers::on_buttonSingleScaleEMD_clicked()
{
	//if (m_scene->srcHairHierarchy(0)->isEmpty() || m_scene->srcHairHierarchy(1)->isEmpty())
	//{
	//	printf("ERROR: src or dst hair is empty!\n");
	//	return;
	//}

	//const HairStrandModel& srcModel = m_scene->srcHairHierarchy(0)->level(0);
	//const HairStrandModel& dstModel = m_scene->srcHairHierarchy(1)->level(0);

	//bool useStrandWeight = ui.checkBoxUseStrandWeights->isChecked();
	//m_scene->hairMorphHierarchy()->level(0).calcFlowsEMD(srcModel, dstModel, useStrandWeight);
}


void HairLayers::on_buttonMultiScaleEMD_clicked()
{
	m_scene->hairMorphHierarchy()->calcAllSourceFlows(false);
	//if (m_scene->srcHairHierarchy(0)->numLevels() < 2 || 
	//	m_scene->srcHairHierarchy(0)->numLevels() != m_scene->srcHairHierarchy(1)->numLevels())
	//{
	//	printf("ERROR: src and dst must have equal and multiple levels!\n");
	//	return;
	//}

	//bool useStrandWeight = ui.checkBoxUseStrandWeights->isChecked();
	//m_scene->hairMorphHierarchy()->calcFlowsMSEMD(
	//	*(m_scene->srcHairHierarchy(0)), *(m_scene->srcHairHierarchy(1)), useStrandWeight);
}


void HairLayers::on_buttonGenMorphStrands_clicked()
{
	m_scene->hairMorphHierarchy()->generateStrands();
}


void HairLayers::on_checkBoxLockTransform_stateChanged(int s)
{
	m_scene->morphController()->setTransformLocked((bool)s);
}


void HairLayers::on_checkBoxLockHeadModel_stateChanged(int s)
{
	if (s != 0)
		m_scene->headMorphRenderer()->lock();
	else
		m_scene->headMorphRenderer()->unlock();
}


void HairLayers::on_actionLoadBackgroundLayer_triggered()
{
	// Background layer
	QString bgFile = QFileDialog::getOpenFileName(this, "Load background image", 
		m_scnInputPath, "Images (*.png *.jpg *.bmp *.tif *.tiff)");

	if (bgFile.isNull())
		return;

	float	bgZ		= 500;

	if (m_scene->loadBackgroundLayer(bgFile, bgZ) == false)
	{
		QMessageBox::critical(this, "Error", "Failed to load background layer.");
		return;
	}
}


void HairLayers::on_actionLoadBodyLayer_triggered()
{
	// Background layer
	QString bodyColorFile = QFileDialog::getOpenFileName(this, "Load body image", 
		m_scnInputPath, "Images (*.png *.jpg *.bmp *.tif *.tiff)");

	if (bodyColorFile.isNull())
		return;

	float bodyZ = 0;

	if (m_scene->loadBodyModel(bodyColorFile, "", bodyZ) == false)
	{
		QMessageBox::critical(this, "Error", "Failed to load body model.");
		return;
	}
}

//////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////


DWORD WINAPI HairLayers::buildPyramidFixK(LPVOID lpParam)
{
	//HairHierarchy* pHierarchy = (HairHierarchy*)lpParam;

	//// collect parameters from UI
	//int nLevels = ui.spinBoxNumPyrLevels->value();
	//int ratios[3] = { ui.spinBoxLvl0ClusterRatio->value(),
	//					ui.spinBoxLvl1ClusterRatio->value(),
	//					ui.spinBoxLvl2ClusterRatio->value() };
	//
	//bool isRelative[3] = { ui.checkBoxLvl0Relative->isChecked(), 
	//						ui.checkBoxLvl1Relative->isChecked(), 
	//						ui.checkBoxLvl2Relative->isChecked() };

	//// calculate sizes for each level
	//std::vector<int> levelSizes(nLevels);
	//int lastSize = pHierarchy->level(0).numStrands();
	//for (int i = 0; i < nLevels; i++)
	//{
	//	levelSizes[i] = isRelative[i] ? lastSize / ratios[i] : ratios[i];
	//	lastSize = levelSizes[i];
	//}

	//pHierarchy->buildByFixedK(levelSizes);

	// update buffers
	//for (int i = 0; i < nLevels; i++)
	//	pHierarchy->level(i).updateBuffers();
	return 0;
}


void HairLayers::hairImageUpdated()
{
	// Update ImageSprite if necessary
	QDXImageSprite* imgSprite = m_scene->imageSprite();
	if (m_hairImage.width() != imgSprite->width() ||
		m_hairImage.height() != imgSprite->height())
	{
		imgSprite->create(m_hairImage.width(), m_hairImage.height());
	}

	// Update StrokesSprite if necessary
	StrokesSprite* strokesSprite = m_scene->strokesSprite();
	if (m_hairImage.width() != strokesSprite->width() ||
		m_hairImage.height() != strokesSprite->height())
	{
		strokesSprite->create(m_hairImage.width(), m_hairImage.height());
		//strokesSprite->
	}

	HairImageViewParam params;
	params.viewMode = (HairImageView)ui.comboImageView->currentIndex();
	params.toggleOrigPrep	  = ui.checkBoxToggleOrigPrep->isChecked();
	params.toggleOrientTensor = ui.checkBoxToggleOrientTensor->isChecked();
	params.showMaskOpacity	  = ui.checkBoxShowMask->isChecked();
	params.showMaxRespOpacity = ui.checkBoxShowMaxResp->isChecked();

	//params.lowConfidence	  = ui.spinBox

	QImage* pDstImg = imgSprite->lockImage();
	if (!pDstImg)
		return;

	m_hairImage.drawImage(params, pDstImg);

	imgSprite->unlockImage();
}

void HairLayers::displayFps(float fps)
{
	statusBar()->showMessage(QString("Fps: %1").arg(fps));
}