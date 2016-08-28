#pragma once

#include "QDXScene.h"
#include "QDXObjectManipulator.h"

#include <QColor>

#include "HairRenderer.h"
#include "MorphController.h"

class QDXCoordFrame;
class QDXRenderGroup;
class QDXMeshObject;
class QDXModelCamera;
class QDXImageSprite;
class BodyModel;
class StrokesSprite;

class HairStrandModel;
class ImageRenderer;
class HeadRenderer;
class ImageLayerRenderer;
class OrigLayerRenderer;
class HairAORenderer;
class BodyLayerRenderer;

class HairHierarchy;

class HeadMorphMesh;
class HairMorphHierarchy;

class HeadMorphRenderer;
class HairMorphRenderer;


enum StrokeMode
{
	NullStroke = -1,
	DeleteStroke = 0,
	CombStroke,
	CutStroke,
};


struct RenderToFileParams
{
	int		msSampleCount;
	int		msQuality;
};


class MyScene : public QDXScene
{
	Q_OBJECT

public:
	MyScene(void);
	~MyScene(void);

	bool		create();
	void		release();

	bool		loadBackgroundLayer(QString imgName, float z);
	bool		loadLargeBackgroundLayer(QString imgName, float z);
	bool		loadBodyModel(QString colorFile, QString depthFile, float z);
	bool		loadOriginalLayer(QString imgName, QString maskName, float z);

	bool		loadFlatHairLayer(QString imgName, float z);
	bool		loadHeightHairLayer(QString depthName, float z);

	void		render();
	bool		renderToFile(QString filename, const RenderToFileParams& params);

	bool		batchRenderVideo(QString filename);

	float		calcVertexOcclusionByHair(const XMFLOAT3* pPosition, const XMFLOAT3* pNormal);
	
	bool		calcHeadOcclusionByHair();
	bool		calcNeckOcclusionByHair();
	bool		calcBodyOcclusionByHair();

	bool		handleEvent(QEvent* event);

	void		setWindow(int width, int height);

	QDXImageSprite*		imageSprite() { return m_pImgSprite; }
	StrokesSprite*		strokesSprite() { return m_pStrokesSprite; }
	QDXImageSprite*		originalLayer() { return m_pOrigLayerSprite; }
	QDXImageSprite*		backgroundLayer() { return m_pBgLayerSprite; }
	QDXImageSprite*		largeBackgroundLayer() { return m_pLargeBgLayerSprite; }
	BodyModel*			bodyModel() { return m_pBodyModel; }
	HairStrandModel*	strandModel() { return m_pStrandModel; }
	HairStrandModel*	auxStrandModel() { return m_pAuxStrandModel; }
	HairStrandModel*	denseStrandModel() { return m_pDenseStrandModel; }

	QDXImageSprite*		flatHairLayer() { return m_pFlatHairSprite; }
	BodyModel*			heightHairLayer() { return m_pHeightHairSprite; }

	HairRenderer*		hairRenderer() { return m_pHairRenderer; }
	HeadRenderer*		headRenderer() { return m_pHeadRenderer; }
	HeadRenderer*		neckRenderer() { return m_pNeckRenderer; }

	MorphController*	morphController() { return &m_morphCtrl; }

	HeadMorphMesh*		headMorphModel() { return m_pHeadMorphModel; }
	HairMorphHierarchy*	hairMorphHierarchy() { return m_pMorphHierarchy; }

	HeadMorphRenderer*	headMorphRenderer() { return m_pHeadMorphRenderer; }
	HairMorphRenderer*	hairMorphRenderer() { return m_pHairMorphRenderer; }

	bool		loadHeadMesh(QString filename);
	bool		loadNeckMesh(QString filename);
	bool		loadHeadTexture(QString filename);
	bool		loadHeadTransform(QString filename);

	bool		loadTransferMatrix(QString filename, float srcHeight, float dstHeight);

	QDXModelCamera*	camera() { return m_pCamera; }

	bool		isCoordFrameVisible() const;
	void		setCoordFrameVisible(bool visible);

	bool		isImageVisible() const;
	void		setImageVisible(bool visible);

	void		setImageOpacity(float opacity);
	float		imageOpacity() const;

	void		setHeadVisible(bool visible);

	HairRenderMode	hairRenderMode() const { return m_pHairRenderer->renderMode(); }
	void		setHairRenderMode(HairRenderMode mode) { m_pHairRenderer->setRenderMode(mode); }

	QColor		hairColor() const;
	void		setHairColor(QColor color);

	void		setBodyLayerDepth(float depth);

	void		setBodyAO(float weight);
	void		setHairAO(float weight);

	void		resetCamera();
	void		resetCamera(int windowWidth, int windowHeight);

	void		setStrokeRadius(int r) { m_strokeRadius = r; }
	void		setStrokeIntensity(float i) { m_strokeIntensity = i; }
	void		setStrokeFuzziness(int f) { m_strokeFuzziness = f; }

signals:

	void		imageDoubleClicked(float, float);

private:

	void		strokeDrawn();

	void		deleteStrokeDrawn();
	void		combStrokeDrawn();
	void		cutStrokeDrawn();

	void		transformJointAngleXY(const XMFLOAT3* origVerts, const XMFLOAT3* newVerts,
										int vId, XMFLOAT2* pNewDir);

	void		normalizeFloat2(XMFLOAT2& vec);

	QDXModelCamera*		m_pCamera;
	QDXImageSprite*		m_pImgSprite;
	StrokesSprite*		m_pStrokesSprite;
	QDXCoordFrame*		m_pCoordFrame;

	// stroke
	StrokeMode			m_strokeMode;
	int					m_strokeRadius;
	float				m_strokeIntensity;
	int					m_strokeFuzziness;
	int					m_strokeLevel;

	QDXImageSprite*		m_pBgLayerSprite;
	QDXImageSprite*		m_pLargeBgLayerSprite;
	QDXImageSprite*		m_pOrigLayerSprite;

	QDXObjectManipulator	m_manipulator;

	HairStrandModel*	m_pStrandModel;
	HairStrandModel*	m_pAuxStrandModel;	// auxiliary hair strands
	HairStrandModel*	m_pDenseStrandModel;

	QDXMeshObject*		m_pHeadModel;
	QDXMeshObject*		m_pNeckModel;
	BodyModel*			m_pBodyModel;

	// For comparison with alternatives
	QDXImageSprite*		m_pFlatHairSprite;
	BodyModel*			m_pHeightHairSprite;

	ImageRenderer*		m_pImgRenderer;
	ImageRenderer*		m_pStrokesRenderer;
	HairRenderer*		m_pHairRenderer;
	HeadRenderer*		m_pHeadRenderer;
	HeadRenderer*		m_pNeckRenderer;
	ImageLayerRenderer*	m_pLayerRenderer;
	OrigLayerRenderer*	m_pOrigLayerRenderer;
	HairAORenderer*		m_pHairAORenderer;
	BodyLayerRenderer*	m_pBodyLayerRenderer;

	// hair morphing related

	MorphController		m_morphCtrl;

	HeadMorphMesh*		m_pHeadMorphModel;
	HeadMorphRenderer*	m_pHeadMorphRenderer;

	//HairHierarchy*		m_pSrcHierarchy;
	//HairHierarchy*		m_pDstHierarchy;

	//HairHierarchy*		m_pSrcHierarchies[MAX_NUM_MORPH_SRC];

	HairMorphHierarchy*	m_pMorphHierarchy;
	HairMorphRenderer*	m_pHairMorphRenderer;


	ID3DX11Effect*					m_pEffect;
	ID3DX11EffectMatrixVariable*	m_hView;
	ID3DX11EffectMatrixVariable*	m_hProjection;
	ID3DX11EffectMatrixVariable*	m_hViewProjection;
	ID3DX11EffectScalarVariable*	m_hTime;
	ID3DX11EffectVectorVariable*	m_hCameraPos;

	ID3DX11EffectVectorVariable*	m_hViewportSize;
	ID3DX11EffectScalarVariable*	m_hBodyDepth;

	ID3DX11EffectScalarVariable*	m_hBodyAO;
	ID3DX11EffectScalarVariable*	m_hHairAO;
};

