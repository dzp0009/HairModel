#ifndef HAIRLAYERS_H
#define HAIRLAYERS_H

#include "Interpolator.h"

#include <QtGui/QMainWindow>
#include "ui_HairLayers.h"
#include "HairImage.h"

class SceneWidget;
class MyScene;
class HairHierarchy;

class PolygonWidget;

class HairLayers : public QMainWindow
{
	Q_OBJECT

public:
	HairLayers(QWidget *parent = 0, Qt::WFlags flags = 0);
	~HairLayers();

	void	loadSettings();
	void	saveSettings();

public slots:
	void	on_actionLoadHairModel_triggered();
	void	on_actionLoadHairlessModel_triggered();

	void	on_actionHairStroke_toggled(bool);
	void	on_actionFaceStroke_toggled(bool);
	void	on_actionBackgroundStroke_toggled(bool);

	void	on_actionSegmentation_triggered();
	void	on_actionFaceReconstruct_triggered();
	void	on_actionGen2DStrands_triggered();
	void	on_actionEstimateDepth_triggered();
	void	on_actionGen3DStrands_triggered();

	void	on_actionSaveSparseHairGeo_triggered();
	void	on_actionSaveDenseHairGeo_triggered();
	void	on_actionSaveExtraHairGeo_triggered();
	void	on_actionSaveSparseHairGeoColor_triggered();
	void	on_actionSaveDenseHairGeoColor_triggered();
	void	on_actionSaveExtraHairGeoColor_triggered();

	void	on_actionAutoSave3DStrands_triggered();
	void	on_actionRenderToFile_triggered();

	void	on_actionLaunchDepthEst_triggered();

	void	on_actionLoadColorImage_triggered();
	void	on_actionLoadMask_triggered();
	void	on_actionLoadFaceMask_triggered();
	void	on_actionLoadOrientChai_triggered();
	void	on_actionLoadOrientWang_triggered();
	void	on_actionLoadHeadMesh_triggered();

	void	on_actionLoadHairStrands_triggered();
	void	on_actionSaveHairStrands_triggered();
	void	on_actionLoadAuxStrands_triggered();
	void	on_actionSaveAuxStrands_triggered();

	void	on_actionSaveCurrentImage_triggered();

	void	on_actionBatchRenderVideo_triggered();

	void	on_actionCalcOrientation_triggered();
	void	on_actionRefineOrientation_triggered();
	void	on_actionUpdateConfidence_triggered();
	void	on_actionDetectFeatures_triggered();
	void	on_actionTraceStrands_triggered();
	void	on_actionClearStrands_triggered();
	void	on_actionCalcDenseOrientation_triggered();
	void	on_actionTraceAuxStrands_triggered();
	void	on_actionClearAuxStrands_triggered();

	void	on_actionLoadFlatHair_triggered();
	void	on_actionLoadHeightHair_triggered();

	void	on_actionProcessImage_triggered();

	void	on_hairImage_doubleClicked(float x, float y);

	void	on_actionComputeAO_triggered();
	void	on_actionClearAO_triggered();


	void	on_buttonCalcOrientation_clicked();
	void	on_buttonRecalcOrientFromConfid_clicked();
	void	on_buttonUpdateConfidence_clicked();
	void	on_buttonSmoothTensor_clicked();
	void	on_buttonDetectFeatures_clicked();
	void	on_buttonTraceByFeatures_clicked();
	void	on_buttonTraceStrands_clicked();
	void	on_buttonClearStrands_clicked();
	void	on_buttonTraceAuxStrands_clicked();
	void	on_buttonClearAuxStrands_clicked();

	void	on_buttonCalcFrontDepth_clicked();
	void	on_buttonCalcBackDepth_clicked();

	void	on_buttonGenDenseStrands_clicked();
	void	on_buttonClearDenseStrands_clicked();

	void	on_buttonAddSilhouetteFalloff_clicked();

	// Global strand shading
	void	on_buttonBgColor_clicked();
	void	on_buttonMsaaApply_clicked();
	void	on_buttonHairColor_clicked();
	void	on_spinBoxTanTexWeight_valueChanged(double);
	void	on_spinBoxTanTexScaleX_valueChanged(double);
	void	on_spinBoxTanTexScaleY_valueChanged(double);
	void	on_spinBoxNormalGradWeight_valueChanged(double);
	void	on_spinBoxStrandRandWeight_valueChanged(double);
	void	on_spinBoxForceOpaque_valueChanged(double);
	void	on_spinBoxKa_valueChanged(double);
	void	on_spinBoxKd_valueChanged(double);
	void	on_spinBoxKr_valueChanged(double);

	void	on_spinBoxBodyAO_valueChanged(double);
	void	on_spinBoxHairAO_valueChanged(double);

	// Sparse strands rendering
	void	on_spinBoxHairWidth_valueChanged(double);
	void	on_spinBoxStrandsDepthScale_valueChanged(double);
	void	on_spinBoxStrandsDepthOffset_valueChanged(double);
	void	on_buttonFilterStrandsColor_clicked();
	void	on_buttonFilterStrandsDepth_clicked();
	void	on_buttonFilterStrandsShape_clicked();
	
	// Dense strands rendering
	void	on_spinBoxAuxHairWidth_valueChanged(double);
	void	on_spinBoxAuxStrandsDepthScale_valueChanged(double);
	void	on_spinBoxAuxStrandsDepthOffset_valueChanged(double);
	void	on_buttonFilterAuxStrandsColor_clicked();
	void	on_buttonFilterAuxStrandsDepth_clicked();

	// Extra strands rendering
	void	on_spinBoxDenseHairWidth_valueChanged(double);
	void	on_buttonFilterDenseStrandsColor_clicked();
	void	on_buttonFilterDenseStrandsDepth_clicked();

	// Viewing params
	void	on_sliderImageOpacity_valueChanged(int);
	void	on_checkBoxImage_stateChanged(int);
	void	on_checkBoxHair_stateChanged(int);
	void	on_checkBoxAuxHair_stateChanged(int);
	void	on_checkBoxDenseHair_stateChanged(int);
	void	on_checkBoxFrame_stateChanged(int);
	void	on_checkBoxHead_stateChanged(int);
	void	on_checkBoxNeck_stateChanged(int);
	void	on_checkBoxStrokes_stateChanged(int);
	void	on_comboPrepMethod_currentIndexChanged(int idx);
	void	on_comboImageView_currentIndexChanged(int idx);
	void	on_comboHairRenderMode_currentIndexChanged(int idx);
	void	on_checkBoxToggleOrigPrep_stateChanged(int);
	void	on_checkBoxToggleOrientTensor_stateChanged(int);
	void	on_checkBoxShowMask_stateChanged(int);
	void	on_checkBoxShowMaxResp_stateChanged(int);
	void	on_comboHeadRenderMode_currentIndexChanged(int idx);

	void	on_spinBoxCamFoV_valueChanged(int);
	void	on_spinBoxCamZn_valueChanged(double);
	void	on_spinBoxCamZf_valueChanged(double);

	void	on_checkBoxBgLayer_stateChanged(int);
	void	on_checkBoxBodyLayer_stateChanged(int);
	void	on_checkBoxLargeBgLayer_stateChanged(int);
	void	on_checkBoxOrigLayer_stateChanged(int);
	void	on_spinBoxBgLayerDepth_valueChanged(double);
	void	on_spinBoxBodyLayerDepth_valueChanged(double);

	void	on_spinBoxNeckBlend_valueChanged(double);

	// Hair Editing
	void	on_buttonAddGeoNoise_clicked();
	void	on_buttonGeoSmooth_clicked();
	void	on_buttonColorSmooth_clicked();

	// Strokes
	void	on_spinBoxStrokeRadius_valueChanged(int);
	void	on_spinBoxStrokeIntensity_valueChanged(double);
	void	on_spinBoxStrokeFuzziness_valueChanged(int);

	// Hair morphing
	void	on_buttonLoadMorphInput_clicked();
	void	on_buttonClearMorphInput_clicked();
	void	on_spinBoxMorphStrandWidth_valueChanged(double);

	void	on_spinBoxCurrHairLevel_valueChanged(int);

	void	on_checkBoxShowMorphSrcHair_stateChanged(int);
	void	on_checkBoxShowMorphDstHair_stateChanged(int);
	void	on_checkBoxShowMorphHair_stateChanged(int);
	void	on_checkBoxShowMorphHead_stateChanged(int);

	void	on_buttonBuildPyrFixK_clicked();
	void	on_buttonBuildPyrAdaptive_clicked();

	void	on_buttonSingleScaleEMD_clicked();
	void	on_buttonMultiScaleEMD_clicked();

	void	on_buttonGenMorphStrands_clicked();

	void	on_checkBoxLockTransform_stateChanged(int);
	void	on_checkBoxLockHeadModel_stateChanged(int);

	void	on_actionLoadBackgroundLayer_triggered();
	void	on_actionLoadBodyLayer_triggered();

	void	on_actionTest_triggered();

	void	morphWeightsChanged(const std::vector<float>&);
	void	displayFps(float fps);

private:
	void	hairImageUpdated();

	void	loadHairStrands(HairStrandModel* pStrandModel, bool includeColor);
	void	saveHairStrands(HairStrandModel* pStrandModel, bool includeColor);

	DWORD WINAPI	buildPyramidFixK(LPVOID lpParam);


	Ui::HairLayersClass ui;

	SceneWidget*	m_sceneWidget;
	MyScene*		m_scene;

	PolygonWidget*	m_polyWidget;

	HairImage		m_hairImage;

	QString			m_scnInputPath;
	QString			m_scn2InputPath;

	QString			m_faceRecProgPath;
	QString			m_depthEstProgPath;

	QString			m_imgInputPath;
	QString			m_imgOutputPath;

	QString			m_hairInputPath;
	QString			m_hairOutputPath;

//	int				m_strokeMode;
};

#endif // HAIRLAYERS_H
