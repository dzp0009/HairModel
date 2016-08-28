#include "MyScene.h"

#include <QFile>
#include <QIODevice>
#include <QMouseEvent>
#include <QImage>
#include <QSettings>
#include <QMessageBox>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <ANN/ANN.h>

#include "QDXMeshObject.h"
#include "QDXCamera.h"
#include "QDXLight.h"
#include "QDXCoordFrame.h"
#include "QDXRenderGroup.h"
#include "QDXImageSprite.h"
#include "QProgressDialog"

#include "HairStrandModel.h"
#include "StrokesSprite.h"
#include "ImageRenderer.h"
#include "HairRenderer.h"
#include "HeadRenderer.h"
#include "ImageLayerRenderer.h"
#include "OrigLayerRenderer.h"
#include "HairAORenderer.h"
#include "BodyModel.h"
#include "BodyLayerRenderer.h"

#include "HeadMorphMesh.h"
#include "HairMorphHierarchy.h"
#include "HairHierarchy.h"
#include "HeadMorphRenderer.h"
#include "HairMorphRenderer.h"


MyScene::MyScene(void)
	: m_pEffect(NULL)
{
	// Add renderer
	m_pImgRenderer = new ImageRenderer();
	addRenderGroup(m_pImgRenderer);

	m_pLayerRenderer = new ImageLayerRenderer();
	addRenderGroup(m_pLayerRenderer);

	m_pHeadRenderer = new HeadRenderer();
	addRenderGroup(m_pHeadRenderer);

	m_pNeckRenderer = new HeadRenderer();
	m_pNeckRenderer->setRenderMode(HeadRenderNeckMode);
	addRenderGroup(m_pNeckRenderer);

	m_pBodyLayerRenderer = new BodyLayerRenderer();
	addRenderGroup(m_pBodyLayerRenderer);

	m_pHairRenderer = new HairRenderer();
	addRenderGroup(m_pHairRenderer);

	m_pOrigLayerRenderer = new OrigLayerRenderer();
	addRenderGroup(m_pOrigLayerRenderer);

	m_pHairAORenderer = new HairAORenderer();
	m_pHairAORenderer->setVisible(false);	// only called internally
	addRenderGroup(m_pHairAORenderer);

	m_pHeadMorphRenderer = new HeadMorphRenderer(&m_morphCtrl);
	addRenderGroup(m_pHeadMorphRenderer);

	m_pHairMorphRenderer = new HairMorphRenderer(&m_morphCtrl);
	addRenderGroup(m_pHairMorphRenderer);

	m_pStrokesRenderer = new ImageRenderer();
	addRenderGroup(m_pStrokesRenderer);

	// Add objects

	m_pCoordFrame = new QDXCoordFrame();
	addObject(m_pCoordFrame, NULL);

	m_pImgSprite = new QDXImageSprite();
	m_pImgSprite->create(16, 16);
	addObject(m_pImgSprite, m_pImgRenderer);

	m_pStrokesSprite = new StrokesSprite();
	m_pStrokesSprite->create(16, 16);
	m_pStrokesSprite->setLocation(0, 0, -0.5f);
	m_pStrokesSprite->setVisible(true);
	addObject(m_pStrokesSprite, m_pStrokesRenderer);

	m_pBgLayerSprite = new QDXImageSprite();
	m_pBgLayerSprite->create(16, 16);
	addObject(m_pBgLayerSprite, m_pLayerRenderer);

	m_pLargeBgLayerSprite = new QDXImageSprite();
	m_pLargeBgLayerSprite->create(16, 16);
	addObject(m_pLargeBgLayerSprite, m_pLayerRenderer);

	m_pOrigLayerSprite = new QDXImageSprite();
	m_pOrigLayerSprite->create(16, 16);
	addObject(m_pOrigLayerSprite, m_pOrigLayerRenderer);

	m_pBodyModel = new BodyModel();
	addObject(m_pBodyModel, m_pBodyLayerRenderer);

	m_pDenseStrandModel = new HairStrandModel();
	addObject(m_pDenseStrandModel, m_pHairRenderer);

	m_pAuxStrandModel = new HairStrandModel();
	addObject(m_pAuxStrandModel, m_pHairRenderer);

	m_pStrandModel = new HairStrandModel();
	addObject(m_pStrandModel, m_pHairRenderer);

	m_pHeadModel = new QDXMeshObject();
	addObject(m_pHeadModel, m_pHeadRenderer);

	m_pNeckModel = new QDXMeshObject();
	addObject(m_pNeckModel, m_pNeckRenderer);

	m_pFlatHairSprite = new QDXImageSprite();
	m_pFlatHairSprite->create(16, 16);
	//m_pFlatHairSprite->setVisible(false);
	addObject(m_pFlatHairSprite, m_pLayerRenderer);

	m_pHeightHairSprite = new BodyModel();
	addObject(m_pHeightHairSprite, m_pBodyLayerRenderer);

	// Also add hair strands to AO renderer
	addObject(m_pStrandModel, m_pHairAORenderer);
	addObject(m_pAuxStrandModel, m_pHairAORenderer);
	addObject(m_pDenseStrandModel, m_pHairAORenderer);

	// morphing related
	
	m_pHeadMorphModel = new HeadMorphMesh;
	addObject(m_pHeadMorphModel, m_pHeadMorphRenderer);

	//for (int i = 0; i < MAX_NUM_MORPH_SRC; i++)
	//{
	//	m_pSrcHierarchies[i] = new HairHierarchy;
	//	addObject(m_pSrcHierarchies[i], m_pHairRenderer);
	//}

	m_pMorphHierarchy = new HairMorphHierarchy;
	addObject(m_pMorphHierarchy, m_pHairMorphRenderer);	

	// Add camera
	m_pCamera = new QDXModelCamera();

	XMFLOAT3 eye = XMFLOAT3(0.0f, 0.0f, -1000.0f);
	XMFLOAT3 at  = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_pCamera->setEyeLookAt(&eye, &at);
	m_pCamera->setFocalLength(24);
	m_pCamera->setClipPlanes(0.02f, 2000.0f);
	m_pCamera->setProjectionMode(CameraOrthographic);
	m_pCamera->setName("Default Camera");
	addCamera(m_pCamera);

	// Disable stroke mode by default
	m_strokeMode = NullStroke;
	m_strokeRadius = 8;
	m_strokeIntensity = 1;
	m_strokeFuzziness = 2;
	m_strokeLevel = 0;
}


MyScene::~MyScene(void)
{
}


void MyScene::resetCamera()
{
	int w, h;
	if (m_pBgLayerSprite->width() > m_pImgSprite->width())
	{
		w = m_pBgLayerSprite->width();
		h = m_pBgLayerSprite->height();
	}
	else
	{
		w = m_pImgSprite->width();
		h = m_pImgSprite->height();
	}

	resetCamera(w, h);
}


// Set camera to look at the center of a WxH 2D region of the X-Y plane
void MyScene::resetCamera(int windowWidth, int windowHeight)
{
	float x = 0.5f * (float)windowWidth;
	float y = -0.5f * (float)windowHeight;

	XMFLOAT3 eye = XMFLOAT3(x, y, -1000.0f);
	XMFLOAT3 at  = XMFLOAT3(x, y, 0.0f);
	XMFLOAT3 up	 = XMFLOAT3(0, 1.0f, 0);
	m_pCamera->setEyeLookAtUp(&eye, &at, &up);

	if (m_pBgLayerSprite->height() > 0)
	{
		float fov = 2.0f * atanf(0.5f * windowHeight / 1000.0f);
		m_pCamera->setFieldOfView(fov);
	}
}


bool MyScene::create()
{
	release();

	// Create effect
	if (!QDXUT::createEffectFromFile("MyScene.fx", "fx_5_0", &m_pEffect))
	{
		qFatal("Failed to create effect!");
		release();
		return false;
	}
	m_hView			  = m_pEffect->GetVariableByName("g_view")->AsMatrix();
	m_hProjection	  = m_pEffect->GetVariableByName("g_projection")->AsMatrix();
	m_hViewProjection = m_pEffect->GetVariableByName("g_viewProjection")->AsMatrix();
	m_hTime			  = m_pEffect->GetVariableByName("g_time")->AsScalar();
	m_hViewportSize	  = m_pEffect->GetVariableByName("g_viewportSize")->AsVector();
	m_hBodyDepth	  = m_pEffect->GetVariableByName("g_bodyDepth")->AsScalar();
	m_hCameraPos	  = m_pEffect->GetVariableByName("g_cameraPos")->AsVector();
	m_hBodyAO		  = m_pEffect->GetVariableByName("g_bodyAO")->AsScalar();
	m_hHairAO		  = m_pEffect->GetVariableByName("g_hairAO")->AsScalar();

	return QDXScene::create(m_pEffect);
}


void MyScene::release()
{
	 QDXScene::release();

	 SAFE_RELEASE(m_pEffect);
}

void MyScene::setWindow(int width, int height)
{
	QDXScene::setWindow(width, height);

	if (m_hViewportSize)
	{
		float size[2];
		size[0] = (float)width;
		size[1] = (float)height;

		m_hViewportSize->SetFloatVector(size);
	}
}

void MyScene::render()
{
	XMMATRIX view	  = XMLoadFloat4x4(m_pCamera->viewMatrix());
	XMMATRIX proj	  = XMLoadFloat4x4(m_pCamera->projMatrix());
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);

	m_hView->SetMatrix((float*)&view);
	m_hProjection->SetMatrix((float*)&proj);
	m_hViewProjection->SetMatrix((float*)&viewProj);
	m_hCameraPos->SetFloatVector((const float*)m_pCamera->eye());

	QDXScene::render();

	if (m_pCoordFrame->isVisible())
	{
		m_pCoordFrame->setViewProjection((float*)&viewProj);
		m_pCoordFrame->render();
	}
}


bool MyScene::handleEvent(QEvent* event)
{
	if (m_pCamera->handleEvent(event))
		return true;

	if (m_manipulator.handleEvent(event))
	{
		//// Synchronize transforms between strand models
		//m_pStrandModel->setTransform(m_pDenseStrandModel->location(),
		//							m_pDenseStrandModel->rotation(),
		//							m_pDenseStrandModel->scale());
		//m_pAuxStrandModel->setTransform(m_pDenseStrandModel->location(),
		//							m_pDenseStrandModel->rotation(),
		//							m_pDenseStrandModel->scale());
		//m_pStrandModel->setLocation(m_pDenseStrandModel->location()->x,
		//							m_pDenseStrandModel->location()->y,
		//							m_pDenseStrandModel->location()->z);
		//m_pStrandModel->setRotation(m_pDenseStrandModel->rotation());
		//m_pStrandModel->setScale(m)

		return true;
	}

	if (event->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent* mouseEvent = (QMouseEvent*)event;

		if (mouseEvent->button() == Qt::LeftButton && 
			mouseEvent->modifiers() & Qt::ControlModifier)
		{
			// User drawing strokes
			XMFLOAT3 rayOrig, rayDir;
			m_pCamera->screenToRay(mouseEvent->x(), mouseEvent->y(), &rayOrig, &rayDir);

			float t, u, v;
			if (m_pStrokesSprite->intersectRay(&rayOrig, &rayDir, NULL, &t, &u, &v))
			{
				//printf("%f %f\n", u, v);
				
				m_pStrokesSprite->clearPoints();
				m_pStrokesSprite->addPoint(u, v);
				//m_pStrokesSprite->drawDot(u, v);

				return true;
			}
		}
		else if (mouseEvent->button() == Qt::RightButton)
		{
			// User selects/deselects image
			XMFLOAT3 rayOrig, rayDir;
			m_pCamera->screenToRay(mouseEvent->x(), mouseEvent->y(), &rayOrig, &rayDir);

			float t, u, v;
			if (m_pImgSprite->isVisible() && m_pImgSprite->intersectRay(&rayOrig, &rayDir, NULL, &t, &u, &v))
			{
				//qDebug() << u << v;
				m_pImgSprite->setSelected(true);
				m_manipulator.install(m_pImgSprite, m_pCamera);
				return true;
			}
			else
			{
				m_pImgSprite->setSelected(false);
				m_manipulator.uninstall();
			}
		}
	}
	else if (event->type() == QEvent::MouseButtonDblClick)
	{
		QMouseEvent* mouseEvent = (QMouseEvent*)event;

		if (mouseEvent->button() == Qt::LeftButton)
		{
			XMFLOAT3 rayOrig, rayDir;
			m_pCamera->screenToRay(mouseEvent->x(), mouseEvent->y(), &rayOrig, &rayDir);

			float t, u, v;
			if (m_pImgSprite->intersectRay(&rayOrig, &rayDir, NULL, &t, &u, &v))
			{
				emit imageDoubleClicked(u, v);
			}
		}
	}
	else if (event->type() == QEvent::MouseMove)
	{
		QMouseEvent* mouseEvent = (QMouseEvent*)event;

		//qDebug() << mouseEvent->x() << mouseEvent->y();

		if (mouseEvent->buttons() & Qt::LeftButton && 
			mouseEvent->modifiers() & Qt::ControlModifier)
		{
			// User drawing strokes
			XMFLOAT3 rayOrig, rayDir;
			m_pCamera->screenToRay(mouseEvent->x(), mouseEvent->y(), &rayOrig, &rayDir);

			float t, u, v;
			if (m_pStrokesSprite->intersectRay(&rayOrig, &rayDir, NULL, &t, &u, &v))
			{
				//printf("%f %f\n", u, v);

				m_pStrokesSprite->addPoint(u, v);

				m_pStrokesSprite->drawDot(u, v, m_strokeMode, 8);
				//m_pStrokesSprite->drawDot(u, v);

				return true;
			}
		}
	}
	else if (event->type() == QEvent::MouseButtonRelease)
	{
		QMouseEvent* mouseEvent = (QMouseEvent*)event;

			//printf("Stroke with %d points.\n", m_pStrokesSprite->numPoints());
		if (mouseEvent->button() == Qt::LeftButton)
		{
			printf("Stroke with %d points.\n", m_pStrokesSprite->numPoints());
			strokeDrawn();
			m_pStrokesSprite->clearImage();
		}
	}
	else if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent* keyEvent = (QKeyEvent*)event;

		if (keyEvent->key() == Qt::Key_C)
		{
			resetCamera();
			return true;
		}
		else if (keyEvent->key() == Qt::Key_A)
		{
			if (m_pDenseStrandModel->isSelected())
			{
				m_pDenseStrandModel->setSelected(false);
				m_pAuxStrandModel->setSelected(false);
				m_pStrandModel->setSelected(false);
				m_manipulator.uninstall();
			}
			else
			{
				m_pDenseStrandModel->setSelected(true);
				m_pAuxStrandModel->setSelected(true);
				m_pStrandModel->setSelected(true);
				m_manipulator.install(m_pDenseStrandModel, m_pCamera);
			}
		}
		else if (keyEvent->key() == Qt::Key_F1)
		{
			m_strokeMode = DeleteStroke;
			printf("DeleteStroke selected.\n");
		}
		else if (keyEvent->key() == Qt::Key_F2)
		{
			m_strokeMode = CombStroke;
			printf("CombStroke selected.\n");
		}
		else if (keyEvent->key() == Qt::Key_F3)
		{
			m_strokeMode = CutStroke;
			printf("CutStroke selected.\n");
		}
	}

	return false;
}


bool MyScene::isCoordFrameVisible() const
{
	return m_pCoordFrame->isVisible();
}


void MyScene::setCoordFrameVisible(bool visible)
{
	m_pCoordFrame->setVisible(visible);
}


bool MyScene::isImageVisible() const
{
	return m_pImgRenderer->isVisible();
}

void MyScene::setImageVisible(bool visible)
{
	m_pImgRenderer->setVisible(visible);
}

void MyScene::setImageOpacity(float opacity)
{
	m_pImgRenderer->setImageOpacity(opacity);
}


float MyScene::imageOpacity() const
{
	return m_pImgRenderer->imageOpacity();
}


QColor MyScene::hairColor() const
{
	XMFLOAT4 rgba = m_pHairRenderer->hairColor();
	return QColor::fromRgbF(rgba.x, rgba.y, rgba.z, rgba.w);
}


void MyScene::setHairColor(QColor color)
{
	float rgba[4] = {color.redF(), color.greenF(), color.blueF(), color.alphaF()};
	m_pHairRenderer->setHairColor(rgba);
}


bool MyScene::loadHeadMesh(QString filename)
{
	if (m_pHeadModel->loadMesh(filename) == false)
	{
		m_dbgMsg = "Failed to load mesh.";
		return false;
	}

	return true;
}

bool MyScene::loadNeckMesh(QString filename)
{
	return m_pNeckModel->loadMesh(filename);
}


bool MyScene::loadHeadTexture(QString filename)
{
	return m_pHeadRenderer->loadTextureFromFile(filename) &&
		   m_pNeckRenderer->loadTextureFromFile(filename);
}


bool MyScene::loadHeadTransform(QString filename)
{
	QFile file(filename);

	if (!file.open(QIODevice::ReadOnly))
		return false;

	char buffer[256];

	// First 9 numbers are Rotation, ignore them here
	float r;
	file.readLine(buffer, 256);
	sscanf_s(buffer, "%f %f %f\n", &r, &r, &r);
	file.readLine(buffer, 256);
	sscanf_s(buffer, "%f %f %f\n", &r, &r, &r);
	file.readLine(buffer, 256);
	sscanf_s(buffer, "%f %f %f\n", &r, &r, &r);

	// Translation
	float x = 0, y = 0, z = 0;
	file.readLine(buffer, 256);
	sscanf_s(buffer, "%f %f %f\n", &x, &y, &z);

	// Scaling
	float scale = 1.0f;
	file.readLine(buffer, 256);
	sscanf_s(buffer, "%f\n", &scale);

	float imgHeight = m_pBgLayerSprite->height();

	m_pHeadModel->setLocation(x, y - imgHeight, -z);
	m_pHeadModel->setScale(scale, scale, scale);

	// Same transform for the neck
	m_pNeckModel->setLocation(x, y - imgHeight, -z);
	m_pNeckModel->setScale(scale, scale, scale);

	return true;
}


bool MyScene::loadBackgroundLayer(QString imgName, float z)
{
	if (!m_pBgLayerSprite->loadImage(imgName))
		return false;

	m_pBgLayerSprite->setLocation(0, 0, z);

	return true;
}

bool MyScene::loadLargeBackgroundLayer(QString imgName, float z)
{
	if (!m_pLargeBgLayerSprite->loadImage(imgName))
		return false;

	float x = -0.5f * (m_pLargeBgLayerSprite->width() - m_pBgLayerSprite->width());
	float y = 0.5f * (m_pLargeBgLayerSprite->height() - m_pBgLayerSprite->height());

	m_pLargeBgLayerSprite->setLocation(x, y, z+0.5f);
	
	return true;
}


bool MyScene::loadFlatHairLayer(QString imgName, float z)
{
	if (!m_pFlatHairSprite->loadImage(imgName))
		return false;

	m_pFlatHairSprite->setLocation(0, 0, z);

	return true;
}


bool MyScene::loadHeightHairLayer(QString depthName, float z)
{
	if (!m_pHeightHairSprite->create(m_pFlatHairSprite->width(), m_pFlatHairSprite->height()))
		return false;

	QImage* pSrcImg = m_pFlatHairSprite->lockImage();

	QImage* pDstImg = m_pHeightHairSprite->lockImage();
	memcpy(pDstImg->bits(), pSrcImg->bits(), pSrcImg->byteCount());
	m_pHeightHairSprite->unlockImage();

	m_pFlatHairSprite->unlockImage();

	if (!m_pHeightHairSprite->loadDepth(depthName))
		return false;	

	m_pHeightHairSprite->setLocation(0, 0, z);

	return true;
}


bool MyScene::loadOriginalLayer(QString imgName, QString maskName, float z)
{
	QImage rgbImg(imgName);
	QImage maskImg(maskName);
	if (rgbImg.isNull() || maskImg.isNull())
		return false;

	rgbImg = rgbImg.convertToFormat(QImage::Format_ARGB32);
	maskImg = maskImg.convertToFormat(QImage::Format_ARGB32);

	cv::Vec4b* pRgbData = (cv::Vec4b*)rgbImg.bits();
	cv::Vec4b* pMaskData = (cv::Vec4b*)maskImg.bits();

	for (int i = 0; i < rgbImg.width()*rgbImg.height(); i++)
	{
		// Set the alpha of each pixel in original hair region to 0.
		if (pMaskData[i][3] > 0)
			pRgbData[i][3] = 0;
	}

	// Set image to layer sprite
	if (!m_pOrigLayerSprite->create(rgbImg.width(), rgbImg.height()))
		return false;

	QImage* pDstImg = m_pOrigLayerSprite->lockImage();
	memcpy(pDstImg->bits(), rgbImg.bits(), rgbImg.byteCount());
	m_pOrigLayerSprite->unlockImage();

	m_pOrigLayerSprite->setLocation(0, 0, z);

	return true;
}

/*
bool MyScene::loadBodyLayer(QString imgName, float z)
{
	// Blur boundary
	QImage img(imgName);
	if (img.isNull())
		return false;

	img = img.convertToFormat(QImage::Format_ARGB32);

	cv::Mat cvImg(img.height(), img.width(), CV_8UC4, img.bits());

	cv::Mat cvBlurred;
	GaussianBlur(cvImg, cvBlurred, cv::Size(-1,-1), 0.8);

	for (int y = 0; y < cvImg.rows; y++)
	{
		for (int x = 0; x < cvImg.cols; x++)
		{
			cv::Vec4b& blurred = cvBlurred.at<cv::Vec4b>(y, x);
			cv::Vec4b& origClr = cvImg.at<cv::Vec4b>(y, x);

			if (blurred[3] > 0 && blurred[3] < 255)
				origClr = blurred;
		}
	}

	if (!m_pBodyLayerSprite->create(img.width(), img.height()))
		return false;

	QImage* pDstImg = m_pBodyLayerSprite->lockImage();
	memcpy(pDstImg->bits(), img.bits(), img.byteCount());
	m_pBodyLayerSprite->unlockImage();

	m_pBodyLayerSprite->setLocation(0, 0, z);

	return true;
}*/

bool MyScene::loadBodyModel(QString colorFile, QString depthFile, float z)
{
	// Blur boundary
	QImage img(colorFile);
	if (img.isNull())
		return false;

	img = img.convertToFormat(QImage::Format_ARGB32);

	cv::Mat cvImg(img.height(), img.width(), CV_8UC4, img.bits());

	cv::Mat cvBlurred;
	GaussianBlur(cvImg, cvBlurred, cv::Size(-1,-1), 0.8);

	for (int y = 0; y < cvImg.rows; y++)
	{
		for (int x = 0; x < cvImg.cols; x++)
		{
			cv::Vec4b& blurred = cvBlurred.at<cv::Vec4b>(y, x);
			cv::Vec4b& origClr = cvImg.at<cv::Vec4b>(y, x);

			if (blurred[3] > 0 && blurred[3] < 255)
				origClr = blurred;
		}
	}

	if (!m_pBodyModel->create(img.width(), img.height()))
		return false;

	QImage* pDstImg = m_pBodyModel->lockImage();
	memcpy(pDstImg->bits(), img.bits(), img.byteCount());
	m_pBodyModel->unlockImage();

	if (depthFile.length() < 3)
		m_pBodyModel->clearDepth();
	else if (!m_pBodyModel->loadDepth(depthFile))
		return false;	

	m_pBodyModel->setLocation(0, 0, z);

	return true;
}

void MyScene::setBodyLayerDepth(float depth)
{
	m_hBodyDepth->SetFloat(depth);
}


void MyScene::setBodyAO(float weight)
{
	m_hBodyAO->SetFloat(weight);
}


void MyScene::setHairAO(float weight)
{
	m_hHairAO->SetFloat(weight);
}


bool MyScene::loadTransferMatrix(QString filename, float srcHeight, float dstHeight)
{
	QFile file(filename);

	if (!file.open(QIODevice::ReadOnly))
		return false;

	char buffer[256];

	XMFLOAT4X4 m;

	file.readLine(buffer, 256);
	//sscanf_s(buffer, "%f %f %f %f\n", &m._11, &m._12, &m._13, &m._14);
	sscanf_s(buffer, "%f %f %f %f\n", &m._11, &m._21, &m._31, &m._41);

	file.readLine(buffer, 256);
	//sscanf_s(buffer, "%f %f %f %f\n", &m._21, &m._22, &m._23, &m._24);
	sscanf_s(buffer, "%f %f %f %f\n", &m._12, &m._22, &m._32, &m._42);

	file.readLine(buffer, 256);
	//sscanf_s(buffer, "%f %f %f %f\n", &m._31, &m._32, &m._33, &m._34);
	sscanf_s(buffer, "%f %f %f %f\n", &m._13, &m._23, &m._33, &m._43);

	file.readLine(buffer, 256);
	//sscanf_s(buffer, "%f %f %f %f\n", &m._41, &m._42, &m._43, &m._44);
	sscanf_s(buffer, "%f %f %f %f\n", &m._14, &m._24, &m._34, &m._44);


	// Correct for different coordinate system

	XMMATRIX mSrc, mTemp, mDst;

	mSrc = XMMatrixMultiply(XMMatrixScaling(1.0f, 1.0f, -1.0f), 
							XMMatrixTranslation(0, srcHeight, 0));

	mTemp = XMLoadFloat4x4(&m);

	mDst = XMMatrixMultiply(XMMatrixScaling(1.0f, 1.0f, -1.0f),
							XMMatrixTranslation(0, -dstHeight, 0));

	XMStoreFloat4x4(&m, mSrc * mTemp * mDst);

	m_pHairRenderer->setTransferMat((float*)&m);

	return true;
}

bool MyScene::renderToFile(QString filename, const RenderToFileParams& params)
{
	if (m_pBgLayerSprite->width() < 1 || m_pBgLayerSprite->height() < 1)
		return false;

	const int width  = m_pBgLayerSprite->width();
	const int height = m_pBgLayerSprite->height();

	//
	// Create render target resources
	//
	ID3D11Texture2D*		pRenderTarget		= NULL;
	ID3D11RenderTargetView*	pRenderTargetView	= NULL;
	ID3D11Texture2D*		pDepthStencil		= NULL;
	ID3D11DepthStencilView*	pDepthStencilView	= NULL;

	// render target
	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Width		= width;
	texDesc.Height		= height;
	texDesc.MipLevels	= 1;
	texDesc.ArraySize	= 1;
	texDesc.Format		= DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.Usage		= D3D11_USAGE_DEFAULT;
	texDesc.BindFlags	= D3D11_BIND_RENDER_TARGET;
	texDesc.SampleDesc.Count   = params.msSampleCount;
	texDesc.SampleDesc.Quality = params.msQuality;

	if (S_OK != QDXUT::device()->CreateTexture2D(&texDesc, NULL, &pRenderTarget))
		return false;

	// render target view
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	ZeroMemory(&rtvDesc, sizeof(rtvDesc));
	rtvDesc.Format			= texDesc.Format;
	if (params.msSampleCount < 2)
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	else
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;

	if (S_OK != QDXUT::device()->CreateRenderTargetView(pRenderTarget, &rtvDesc, &pRenderTargetView))
		return false;

	// depth stencil
	texDesc.Format		= DXGI_FORMAT_D24_UNORM_S8_UINT;
	texDesc.BindFlags	= D3D11_BIND_DEPTH_STENCIL;
	if (S_OK != QDXUT::device()->CreateTexture2D(&texDesc, NULL, &pDepthStencil))
		return false;

	// depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	ZeroMemory(&dsvDesc, sizeof(dsvDesc));
	dsvDesc.Format		  = texDesc.Format;
	if (params.msSampleCount < 2)
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	else
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

	if (S_OK != QDXUT::device()->CreateDepthStencilView(pDepthStencil, &dsvDesc, &pDepthStencilView))
		return false;

	//
	// Setup camera and viewport
	//

	// Setup viewport
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width    = (float)width;
	viewport.Height   = (float)height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// Setup camera
	QDXModelCamera cam;// = (*m_pCamera);
	cam.setEyeLookAtUp(m_pCamera->eye(), m_pCamera->lookAt(), m_pCamera->up());
	cam.setPerspectiveFOV(m_pCamera->fieldOfView(), (float)width / (float)height,
						  m_pCamera->nearPlane(), m_pCamera->farPlane());
	cam.setProjectionMode(m_pCamera->projectionMode());
	cam.setWindowSize(width, height);

	//
	// Set render target and RENDER
	//

	ID3D11DeviceContext* pContext = QDXUT::immediateContext();

	pContext->OMSetRenderTargets(1, &pRenderTargetView, pDepthStencilView);
	pContext->RSSetViewports(1, &viewport);

	float clearColor[4] = {0};
	pContext->ClearRenderTargetView(pRenderTargetView, clearColor);
	pContext->ClearDepthStencilView(pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	XMMATRIX view	  = XMLoadFloat4x4(cam.viewMatrix());
	XMMATRIX proj	  = XMLoadFloat4x4(cam.projMatrix());
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);

	m_hView->SetMatrix((float*)&view);
	m_hProjection->SetMatrix((float*)&proj);
	m_hViewProjection->SetMatrix((float*)&viewProj);
	m_hCameraPos->SetFloatVector((const float*)cam.eye());

	QDXScene::render();

	//
	// Save result to image file
	//

	ID3D11Texture2D* pStageTexture = NULL;
	texDesc.Format		= DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.Usage		= D3D11_USAGE_STAGING;
	texDesc.BindFlags	= 0;
	texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	texDesc.SampleDesc.Count   = 1;
	texDesc.SampleDesc.Quality = 0;

	if (S_OK != QDXUT::device()->CreateTexture2D(&texDesc, NULL, &pStageTexture))
		return false;

	if (params.msSampleCount > 1)
	{
		ID3D11Texture2D* pSSTexture = NULL;	// single-sampled texture
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.CPUAccessFlags = 0;

		if (S_OK != QDXUT::device()->CreateTexture2D(&texDesc, NULL, &pSSTexture))
			return false;

		// Convert multisampled texture to single-sampled.
		pContext->ResolveSubresource(pSSTexture, 0, pRenderTarget, 0, texDesc.Format);

		pContext->CopyResource(pStageTexture, pSSTexture);

		SAFE_RELEASE(pSSTexture);
	}
	else
	{
		pContext->CopyResource(pStageTexture, pRenderTarget);
	}
	
	// Get pixel data
	QImage img(width, height, QImage::Format_ARGB32);
	
	D3D11_MAPPED_SUBRESOURCE mapped;
	pContext->Map(pStageTexture, 0, D3D11_MAP_READ, 0, &mapped);

	const uchar* pSrcBits = (const uchar*)mapped.pData;
	uchar*		 pDstBits = img.bits();
	int srcIdx = 0;
	int dstIdx = 0;

	for (int y = 0; y < height; y++)
	{
		memcpy(&(pDstBits[dstIdx]), &(pSrcBits[srcIdx]), 4*width);

		srcIdx += mapped.RowPitch;
		dstIdx += img.bytesPerLine();
	}

	pContext->Unmap(pStageTexture, 0);

	SAFE_RELEASE(pDepthStencilView);
	SAFE_RELEASE(pDepthStencil);
	SAFE_RELEASE(pRenderTargetView);
	SAFE_RELEASE(pRenderTarget);
	SAFE_RELEASE(pStageTexture);

	return img.save(filename);
}


float MyScene::calcVertexOcclusionByHair(const XMFLOAT3* pPosition, const XMFLOAT3* pNormal)
{
	const int texWidth    = 256;
	const int texHeight   = 256;
	const int msaaCount   = 8;
	const int msaaQuality = 32;

	const float fov	= 0.5f * XM_PI;
	const float zNear = 2.0f;
	const float zFar  = 1000.0f;

	////////////////////////////////////////
	// Create render target resources
	////////////////////////////////////////

	ID3D11Texture2D*		pRenderTarget		= NULL;
	ID3D11RenderTargetView*	pRenderTargetView	= NULL;
	ID3D11Texture2D*		pDepthStencil		= NULL;
	ID3D11DepthStencilView*	pDepthStencilView	= NULL;

	// render target
	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Width		= texWidth;
	texDesc.Height		= texHeight;
	texDesc.MipLevels	= 1;
	texDesc.ArraySize	= 1;
	texDesc.Format		= DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.Usage		= D3D11_USAGE_DEFAULT;
	texDesc.BindFlags	= D3D11_BIND_RENDER_TARGET;
	texDesc.SampleDesc.Count   = msaaCount;
	texDesc.SampleDesc.Quality = msaaQuality;

	if (S_OK != QDXUT::device()->CreateTexture2D(&texDesc, NULL, &pRenderTarget))
		return false;

	// render target view
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	ZeroMemory(&rtvDesc, sizeof(rtvDesc));
	rtvDesc.Format			= texDesc.Format;
	if (msaaCount < 2)
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	else
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;

	if (S_OK != QDXUT::device()->CreateRenderTargetView(pRenderTarget, &rtvDesc, &pRenderTargetView))
		return false;

	// depth stencil
	texDesc.Format		= DXGI_FORMAT_D24_UNORM_S8_UINT;
	texDesc.BindFlags	= D3D11_BIND_DEPTH_STENCIL;
	if (S_OK != QDXUT::device()->CreateTexture2D(&texDesc, NULL, &pDepthStencil))
		return false;

	// depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	ZeroMemory(&dsvDesc, sizeof(dsvDesc));
	dsvDesc.Format		  = texDesc.Format;
	if (msaaCount < 2)
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	else
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

	if (S_OK != QDXUT::device()->CreateDepthStencilView(pDepthStencil, &dsvDesc, &pDepthStencilView))
		return false;

	////////////////////////////////////////
	// Setup camera and viewport
	////////////////////////////////////////

	// Setup viewport
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width    = texWidth;
	viewport.Height   = texHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// Setup camera
	XMVECTOR pos = XMLoadFloat3(pPosition);
	XMVECTOR normal = XMVector3Normalize(XMLoadFloat3(pNormal));
	XMVECTOR at  = pos + normal;
	
	// generate random UP direction
	XMVECTOR up = XMVectorSet(	(float)rand()/(float)RAND_MAX,
								(float)rand()/(float)RAND_MAX,
								(float)rand()/(float)RAND_MAX, 1.0f);
	up = XMVector3Cross(normal, up);
	up = XMVector3Normalize(up);

	XMFLOAT3 vAt, vUp;
	XMStoreFloat3(&vAt, at);
	XMStoreFloat3(&vUp, up);

	QDXModelCamera cam;
	cam.setEyeLookAtUp(pPosition, &vAt, m_pCamera->up());  //&vUp);
	cam.setPerspectiveFOV(fov, (float)texWidth / (float)texHeight, zNear, zFar);
	cam.setWindowSize(texWidth, texHeight);

	//cam.setEyeLookAtUp(m_pCamera->eye(), m_pCamera->lookAt(), m_pCamera->up());
	//cam.setPerspectiveFOV(m_pCamera->fieldOfView(), (float)texWidth / (float)texHeight,
	//	m_pCamera->nearPlane(), m_pCamera->farPlane());
	//cam.setProjectionMode(m_pCamera->projectionMode());
	//cam.setWindowSize(texWidth, texHeight);

	////////////////////////////////////////
	// Set render target and RENDER
	////////////////////////////////////////

	ID3D11DeviceContext* pContext = QDXUT::immediateContext();

	pContext->OMSetRenderTargets(1, &pRenderTargetView, pDepthStencilView);
	pContext->RSSetViewports(1, &viewport);

	float clearColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	pContext->ClearRenderTargetView(pRenderTargetView, clearColor);
	pContext->ClearDepthStencilView(pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	XMMATRIX view	  = XMLoadFloat4x4(cam.viewMatrix());
	XMMATRIX proj	  = XMLoadFloat4x4(cam.projMatrix());
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);

	m_hView->SetMatrix((float*)&view);
	m_hProjection->SetMatrix((float*)&proj);
	m_hViewProjection->SetMatrix((float*)&viewProj);
	m_hCameraPos->SetFloatVector((const float*)cam.eye());

	m_pHairAORenderer->render();


	////////////////////////////////////////
	// Save result to image file
	////////////////////////////////////////

	ID3D11Texture2D* pStageTexture = NULL;
	texDesc.Format		= DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.Usage		= D3D11_USAGE_STAGING;
	texDesc.BindFlags	= 0;
	texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	texDesc.SampleDesc.Count   = 1;
	texDesc.SampleDesc.Quality = 0;

	if (S_OK != QDXUT::device()->CreateTexture2D(&texDesc, NULL, &pStageTexture))
		return false;

	if (msaaCount > 1)
	{
		ID3D11Texture2D* pSSTexture = NULL;	// single-sampled texture
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.CPUAccessFlags = 0;

		if (S_OK != QDXUT::device()->CreateTexture2D(&texDesc, NULL, &pSSTexture))
			return false;

		// Convert multisampled texture to single-sampled.
		pContext->ResolveSubresource(pSSTexture, 0, pRenderTarget, 0, texDesc.Format);

		pContext->CopyResource(pStageTexture, pSSTexture);

		SAFE_RELEASE(pSSTexture);
	}
	else
	{
		pContext->CopyResource(pStageTexture, pRenderTarget);
	}

	// Get pixel data
	QImage img(texWidth, texHeight, QImage::Format_ARGB32);

	D3D11_MAPPED_SUBRESOURCE mapped;
	pContext->Map(pStageTexture, 0, D3D11_MAP_READ, 0, &mapped);

	const uchar* pSrcBits = (const uchar*)mapped.pData;
	uchar*		 pDstBits = img.bits();
	int srcIdx = 0;
	int dstIdx = 0;

	for (int y = 0; y < texHeight; y++)
	{
		memcpy(&(pDstBits[dstIdx]), &(pSrcBits[srcIdx]), 4*texWidth);

		srcIdx += mapped.RowPitch;
		dstIdx += img.bytesPerLine();
	}

	pContext->Unmap(pStageTexture, 0);

	SAFE_RELEASE(pDepthStencilView);
	SAFE_RELEASE(pDepthStencil);
	SAFE_RELEASE(pRenderTargetView);
	SAFE_RELEASE(pRenderTarget);
	SAFE_RELEASE(pStageTexture);

	return img.save("D:/aotest.png");
}


bool MyScene::calcHeadOcclusionByHair()
{
	QDXBaseMesh* pMesh = m_pHeadModel->mesh();

	if (pMesh->numFaces() < 1)
		return false;

	QProgressDialog progressDlg;
	progressDlg.setLabelText("Calculating ambient occlusion...");
	progressDlg.setRange(0, pMesh->numVertices());
	progressDlg.setModal(true);
	progressDlg.show();


	const int texWidth    = 64;
	const int texHeight   = 64;
	const int msaaCount   = 1;//8;
	const int msaaQuality = 0;//32;

	const float fov	= 0.8f * XM_PI;
	const float zNear = 1.0f;
	const float zFar  = 1000.0f;

	////////////////////////////////////////
	// Create render target resources
	////////////////////////////////////////

	ID3D11Texture2D*		pRenderTarget		= NULL;
	ID3D11RenderTargetView*	pRenderTargetView	= NULL;
	ID3D11Texture2D*		pDepthStencil		= NULL;
	ID3D11DepthStencilView*	pDepthStencilView	= NULL;

	// render target
	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Width		= texWidth;
	texDesc.Height		= texHeight;
	texDesc.MipLevels	= 1;
	texDesc.ArraySize	= 1;
	texDesc.Format		= DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.Usage		= D3D11_USAGE_DEFAULT;
	texDesc.BindFlags	= D3D11_BIND_RENDER_TARGET;
	texDesc.SampleDesc.Count   = msaaCount;
	texDesc.SampleDesc.Quality = msaaQuality;

	if (S_OK != QDXUT::device()->CreateTexture2D(&texDesc, NULL, &pRenderTarget))
		return false;

	// render target view
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	ZeroMemory(&rtvDesc, sizeof(rtvDesc));
	rtvDesc.Format			= texDesc.Format;
	if (msaaCount < 2)
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	else
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;

	if (S_OK != QDXUT::device()->CreateRenderTargetView(pRenderTarget, &rtvDesc, &pRenderTargetView))
		return false;

	// depth stencil
	texDesc.Format		= DXGI_FORMAT_D24_UNORM_S8_UINT;
	texDesc.BindFlags	= D3D11_BIND_DEPTH_STENCIL;
	if (S_OK != QDXUT::device()->CreateTexture2D(&texDesc, NULL, &pDepthStencil))
		return false;

	// depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	ZeroMemory(&dsvDesc, sizeof(dsvDesc));
	dsvDesc.Format		  = texDesc.Format;
	if (msaaCount < 2)
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	else
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

	if (S_OK != QDXUT::device()->CreateDepthStencilView(pDepthStencil, &dsvDesc, &pDepthStencilView))
		return false;

	// Texture for staging...
	ID3D11Texture2D* pStageTexture = NULL;
	texDesc.Format		= DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.Usage		= D3D11_USAGE_STAGING;
	texDesc.BindFlags	= 0;
	texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	texDesc.SampleDesc.Count   = 1;
	texDesc.SampleDesc.Quality = 0;

	if (S_OK != QDXUT::device()->CreateTexture2D(&texDesc, NULL, &pStageTexture))
		return false;

	// Texture for resolving multisample
	ID3D11Texture2D* pSSTexture = NULL;	// single-sampled texture
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.CPUAccessFlags = 0;

	if (S_OK != QDXUT::device()->CreateTexture2D(&texDesc, NULL, &pSSTexture))
		return false;

	////////////////////////////////////////
	// Setup viewport
	////////////////////////////////////////

	// Setup viewport
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width    = texWidth;
	viewport.Height   = texHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	ID3D11DeviceContext* pContext = QDXUT::immediateContext();

	////////////////////////////////////////
	// Process each head mesh vertex
	////////////////////////////////////////

	float minAO = 1000.0f;
	float maxAO = -1000.0f;

	// Get pixel data
	QImage img(texWidth, texHeight, QImage::Format_ARGB32);

	for (int vId = 0; vId < pMesh->numVertices(); vId++)
	{
		progressDlg.setValue(vId);

		XMFLOAT3 vPos = pMesh->vertices()[vId].position;
		XMFLOAT3 vNormal = pMesh->vertices()[vId].normal;

		// Setup camera
		XMVECTOR pos = XMLoadFloat3(&vPos);
		XMVECTOR normal = XMVector3Normalize(XMLoadFloat3(&vNormal));

		// TRANSFORM
		XMMATRIX headTransform = XMLoadFloat4x4(m_pHeadModel->worldTransform());
		pos = XMVector3TransformCoord(pos, headTransform);
		normal = XMVector3TransformNormal(normal, headTransform);

		XMStoreFloat3(&vPos, pos);

		XMVECTOR at  = pos + normal;

		// generate random UP direction
		XMVECTOR up = XMVectorSet(	(float)rand()/(float)RAND_MAX,
			(float)rand()/(float)RAND_MAX,
			(float)rand()/(float)RAND_MAX, 1.0f);
		up = XMVector3Cross(normal, up);
		up = XMVector3Normalize(up);

		XMFLOAT3 vAt, vUp;
		XMStoreFloat3(&vAt, at);
		XMStoreFloat3(&vUp, up);

		QDXModelCamera cam;
		cam.setEyeLookAtUp(&vPos, &vAt, &vUp);
		cam.setPerspectiveFOV(fov, (float)texWidth / (float)texHeight, zNear, zFar);
		cam.setWindowSize(texWidth, texHeight);

		////////////////////////////////////////
		// RENDER
		////////////////////////////////////////


		float clearColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
		pContext->ClearRenderTargetView(pRenderTargetView, clearColor);
		pContext->ClearDepthStencilView(pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		XMMATRIX view	  = XMLoadFloat4x4(cam.viewMatrix());
		XMMATRIX proj	  = XMLoadFloat4x4(cam.projMatrix());
		XMMATRIX viewProj = XMMatrixMultiply(view, proj);

		m_hView->SetMatrix((float*)&view);
		m_hProjection->SetMatrix((float*)&proj);
		m_hViewProjection->SetMatrix((float*)&viewProj);
		m_hCameraPos->SetFloatVector((const float*)cam.eye());

		pContext->OMSetRenderTargets(1, &pRenderTargetView, pDepthStencilView);
		pContext->RSSetViewports(1, &viewport);

		m_pHairAORenderer->render();


		////////////////////////////////////////
		// Save result to image file
		////////////////////////////////////////

		// Convert multisampled texture to single-sampled.
		//pContext->ResolveSubresource(pSSTexture, 0, pRenderTarget, 0, texDesc.Format);

		//pContext->CopyResource(pStageTexture, pSSTexture);

		pContext->CopyResource(pStageTexture, pRenderTarget);


		D3D11_MAPPED_SUBRESOURCE mapped;
		pContext->Map(pStageTexture, 0, D3D11_MAP_READ, 0, &mapped);

		const uchar* pSrcBits = (const uchar*)mapped.pData;
		uchar*		 pDstBits = img.bits();
		int srcIdx = 0;
		int dstIdx = 0;

		for (int y = 0; y < texHeight; y++)
		{
			memcpy(&(pDstBits[dstIdx]), &(pSrcBits[srcIdx]), 4*texWidth);

			srcIdx += mapped.RowPitch;
			dstIdx += img.bytesPerLine();
		}

		pContext->Unmap(pStageTexture, 0);

		// Average pixel values...
		int sumValue = 0;
		int numPixels = 0;

		float r2 = (texHeight / 2) * (texHeight / 2);
		int idx = 0;
		cv::Vec4b* pPixels = (cv::Vec4b*)img.bits();
		for (int y = 0; y < texHeight; y++)
		{
			float ry2 = (y - texHeight / 2) * (y - texHeight / 2);

			for (int x = 0; x < texWidth; x++)
			{
				float rx2 = (x - texWidth / 2) * (x - texWidth / 2);

				if (rx2 + ry2 < r2)
				{
					sumValue += pPixels[idx][1];
					numPixels++;
				}
				idx++;
			}
		}

		float ao = ((float)sumValue / (float)numPixels) / 255.0f;
		pMesh->vertices()[vId].reserved.x = ao;

		if (ao < minAO)
			minAO = ao;
		if (ao > maxAO)
			maxAO = ao;
	}

	//QMessageBox::warning(NULL, "AO", QString("AO: %1 ~ %2").arg(minAO).arg(maxAO));
	
	SAFE_RELEASE(pDepthStencilView);
	SAFE_RELEASE(pDepthStencil);
	SAFE_RELEASE(pRenderTargetView);
	SAFE_RELEASE(pRenderTarget);
	SAFE_RELEASE(pStageTexture);
	SAFE_RELEASE(pSSTexture);

	pMesh->updateVertexBuffer();

	//return img.save("D:/aotest.png");

	return true;
}




bool MyScene::calcNeckOcclusionByHair()
{
	QDXBaseMesh* pMesh = m_pNeckModel->mesh();

	if (pMesh->numFaces() < 1)
		return false;

	QProgressDialog progressDlg;
	progressDlg.setLabelText("Calculating ambient occlusion...");
	progressDlg.setRange(0, pMesh->numVertices());
	progressDlg.setModal(true);
	progressDlg.show();


	const int texWidth    = 64;
	const int texHeight   = 64;
	const int msaaCount   = 1;//8;
	const int msaaQuality = 0;//32;

	const float fov	= 0.8f * XM_PI;
	const float zNear = 1.0f;
	const float zFar  = 1000.0f;

	////////////////////////////////////////
	// Create render target resources
	////////////////////////////////////////

	ID3D11Texture2D*		pRenderTarget		= NULL;
	ID3D11RenderTargetView*	pRenderTargetView	= NULL;
	ID3D11Texture2D*		pDepthStencil		= NULL;
	ID3D11DepthStencilView*	pDepthStencilView	= NULL;

	// render target
	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Width		= texWidth;
	texDesc.Height		= texHeight;
	texDesc.MipLevels	= 1;
	texDesc.ArraySize	= 1;
	texDesc.Format		= DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.Usage		= D3D11_USAGE_DEFAULT;
	texDesc.BindFlags	= D3D11_BIND_RENDER_TARGET;
	texDesc.SampleDesc.Count   = msaaCount;
	texDesc.SampleDesc.Quality = msaaQuality;

	if (S_OK != QDXUT::device()->CreateTexture2D(&texDesc, NULL, &pRenderTarget))
		return false;

	// render target view
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	ZeroMemory(&rtvDesc, sizeof(rtvDesc));
	rtvDesc.Format			= texDesc.Format;
	if (msaaCount < 2)
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	else
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;

	if (S_OK != QDXUT::device()->CreateRenderTargetView(pRenderTarget, &rtvDesc, &pRenderTargetView))
		return false;

	// depth stencil
	texDesc.Format		= DXGI_FORMAT_D24_UNORM_S8_UINT;
	texDesc.BindFlags	= D3D11_BIND_DEPTH_STENCIL;
	if (S_OK != QDXUT::device()->CreateTexture2D(&texDesc, NULL, &pDepthStencil))
		return false;

	// depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	ZeroMemory(&dsvDesc, sizeof(dsvDesc));
	dsvDesc.Format		  = texDesc.Format;
	if (msaaCount < 2)
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	else
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

	if (S_OK != QDXUT::device()->CreateDepthStencilView(pDepthStencil, &dsvDesc, &pDepthStencilView))
		return false;

	// Texture for staging...
	ID3D11Texture2D* pStageTexture = NULL;
	texDesc.Format		= DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.Usage		= D3D11_USAGE_STAGING;
	texDesc.BindFlags	= 0;
	texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	texDesc.SampleDesc.Count   = 1;
	texDesc.SampleDesc.Quality = 0;

	if (S_OK != QDXUT::device()->CreateTexture2D(&texDesc, NULL, &pStageTexture))
		return false;

	// Texture for resolving multisample
	ID3D11Texture2D* pSSTexture = NULL;	// single-sampled texture
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.CPUAccessFlags = 0;

	if (S_OK != QDXUT::device()->CreateTexture2D(&texDesc, NULL, &pSSTexture))
		return false;

	////////////////////////////////////////
	// Setup viewport
	////////////////////////////////////////

	// Setup viewport
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width    = texWidth;
	viewport.Height   = texHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	ID3D11DeviceContext* pContext = QDXUT::immediateContext();

	////////////////////////////////////////
	// Process each head mesh vertex
	////////////////////////////////////////

	float minAO = 1000.0f;
	float maxAO = -1000.0f;

	// Get pixel data
	QImage img(texWidth, texHeight, QImage::Format_ARGB32);

	for (int vId = 0; vId < pMesh->numVertices(); vId++)
	{
		progressDlg.setValue(vId);

		XMFLOAT3 vPos = pMesh->vertices()[vId].position;
		XMFLOAT3 vNormal = pMesh->vertices()[vId].normal;

		// Setup camera
		XMVECTOR pos = XMLoadFloat3(&vPos);
		XMVECTOR normal = XMVector3Normalize(XMLoadFloat3(&vNormal));

		// TRANSFORM
		XMMATRIX neckTransform = XMLoadFloat4x4(m_pNeckModel->worldTransform());
		pos = XMVector3TransformCoord(pos, neckTransform);
		normal = XMVector3TransformNormal(normal, neckTransform);

		XMStoreFloat3(&vPos, pos);

		XMVECTOR at  = pos + normal;

		// generate random UP direction
		XMVECTOR up = XMVectorSet(	(float)rand()/(float)RAND_MAX,
			(float)rand()/(float)RAND_MAX,
			(float)rand()/(float)RAND_MAX, 1.0f);
		up = XMVector3Cross(normal, up);
		up = XMVector3Normalize(up);

		XMFLOAT3 vAt, vUp;
		XMStoreFloat3(&vAt, at);
		XMStoreFloat3(&vUp, up);

		QDXModelCamera cam;
		cam.setEyeLookAtUp(&vPos, &vAt, &vUp);
		cam.setPerspectiveFOV(fov, (float)texWidth / (float)texHeight, zNear, zFar);
		cam.setWindowSize(texWidth, texHeight);

		////////////////////////////////////////
		// RENDER
		////////////////////////////////////////


		float clearColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
		pContext->ClearRenderTargetView(pRenderTargetView, clearColor);
		pContext->ClearDepthStencilView(pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		XMMATRIX view	  = XMLoadFloat4x4(cam.viewMatrix());
		XMMATRIX proj	  = XMLoadFloat4x4(cam.projMatrix());
		XMMATRIX viewProj = XMMatrixMultiply(view, proj);

		m_hView->SetMatrix((float*)&view);
		m_hProjection->SetMatrix((float*)&proj);
		m_hViewProjection->SetMatrix((float*)&viewProj);
		m_hCameraPos->SetFloatVector((const float*)cam.eye());

		pContext->OMSetRenderTargets(1, &pRenderTargetView, pDepthStencilView);
		pContext->RSSetViewports(1, &viewport);

		m_pHairAORenderer->render();


		////////////////////////////////////////
		// Save result to image file
		////////////////////////////////////////

		// Convert multisampled texture to single-sampled.
		//pContext->ResolveSubresource(pSSTexture, 0, pRenderTarget, 0, texDesc.Format);

		//pContext->CopyResource(pStageTexture, pSSTexture);

		pContext->CopyResource(pStageTexture, pRenderTarget);


		D3D11_MAPPED_SUBRESOURCE mapped;
		pContext->Map(pStageTexture, 0, D3D11_MAP_READ, 0, &mapped);

		const uchar* pSrcBits = (const uchar*)mapped.pData;
		uchar*		 pDstBits = img.bits();
		int srcIdx = 0;
		int dstIdx = 0;

		for (int y = 0; y < texHeight; y++)
		{
			memcpy(&(pDstBits[dstIdx]), &(pSrcBits[srcIdx]), 4*texWidth);

			srcIdx += mapped.RowPitch;
			dstIdx += img.bytesPerLine();
		}

		pContext->Unmap(pStageTexture, 0);

		// Average pixel values...
		int sumValue = 0;
		int numPixels = 0;

		float r2 = (texHeight / 2) * (texHeight / 2);
		int idx = 0;
		cv::Vec4b* pPixels = (cv::Vec4b*)img.bits();
		for (int y = 0; y < texHeight; y++)
		{
			float ry2 = (y - texHeight / 2) * (y - texHeight / 2);

			for (int x = 0; x < texWidth; x++)
			{
				float rx2 = (x - texWidth / 2) * (x - texWidth / 2);

				if (rx2 + ry2 < r2)
				{
					sumValue += pPixels[idx][1];
					numPixels++;
				}
				idx++;
			}
		}

		float ao = ((float)sumValue / (float)numPixels) / 255.0f;
		pMesh->vertices()[vId].reserved.x = ao;

		if (ao < minAO)
			minAO = ao;
		if (ao > maxAO)
			maxAO = ao;
	}

	//QMessageBox::warning(NULL, "AO", QString("AO: %1 ~ %2").arg(minAO).arg(maxAO));

	SAFE_RELEASE(pDepthStencilView);
	SAFE_RELEASE(pDepthStencil);
	SAFE_RELEASE(pRenderTargetView);
	SAFE_RELEASE(pRenderTarget);
	SAFE_RELEASE(pStageTexture);
	SAFE_RELEASE(pSSTexture);

	pMesh->updateVertexBuffer();

	//return img.save("D:/aotest.png");

	return true;
}


bool MyScene::calcBodyOcclusionByHair()
{
	QDXImageQuadVertex* pVertices = m_pBodyModel->vertices();

	if (m_pBodyModel->numVertices() < 1)
		return false;

	QProgressDialog progressDlg;
	progressDlg.setLabelText("Calculating ambient occlusion...");
	progressDlg.setRange(0, m_pBodyModel->numVertices());
	progressDlg.setModal(true);
	progressDlg.show();


	const int texWidth    = 64;
	const int texHeight   = 64;
	const int msaaCount   = 1;//8;
	const int msaaQuality = 0;//32;

	const float fov	= 0.8f * XM_PI;
	const float zNear = 1.0f;
	const float zFar  = 1000.0f;

	////////////////////////////////////////
	// Create render target resources
	////////////////////////////////////////

	ID3D11Texture2D*		pRenderTarget		= NULL;
	ID3D11RenderTargetView*	pRenderTargetView	= NULL;
	ID3D11Texture2D*		pDepthStencil		= NULL;
	ID3D11DepthStencilView*	pDepthStencilView	= NULL;

	// render target
	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Width		= texWidth;
	texDesc.Height		= texHeight;
	texDesc.MipLevels	= 1;
	texDesc.ArraySize	= 1;
	texDesc.Format		= DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.Usage		= D3D11_USAGE_DEFAULT;
	texDesc.BindFlags	= D3D11_BIND_RENDER_TARGET;
	texDesc.SampleDesc.Count   = msaaCount;
	texDesc.SampleDesc.Quality = msaaQuality;

	if (S_OK != QDXUT::device()->CreateTexture2D(&texDesc, NULL, &pRenderTarget))
		return false;

	// render target view
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	ZeroMemory(&rtvDesc, sizeof(rtvDesc));
	rtvDesc.Format			= texDesc.Format;
	if (msaaCount < 2)
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	else
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;

	if (S_OK != QDXUT::device()->CreateRenderTargetView(pRenderTarget, &rtvDesc, &pRenderTargetView))
		return false;

	// depth stencil
	texDesc.Format		= DXGI_FORMAT_D24_UNORM_S8_UINT;
	texDesc.BindFlags	= D3D11_BIND_DEPTH_STENCIL;
	if (S_OK != QDXUT::device()->CreateTexture2D(&texDesc, NULL, &pDepthStencil))
		return false;

	// depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	ZeroMemory(&dsvDesc, sizeof(dsvDesc));
	dsvDesc.Format		  = texDesc.Format;
	if (msaaCount < 2)
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	else
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

	if (S_OK != QDXUT::device()->CreateDepthStencilView(pDepthStencil, &dsvDesc, &pDepthStencilView))
		return false;

	// Texture for staging...
	ID3D11Texture2D* pStageTexture = NULL;
	texDesc.Format		= DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.Usage		= D3D11_USAGE_STAGING;
	texDesc.BindFlags	= 0;
	texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	texDesc.SampleDesc.Count   = 1;
	texDesc.SampleDesc.Quality = 0;

	if (S_OK != QDXUT::device()->CreateTexture2D(&texDesc, NULL, &pStageTexture))
		return false;

	// Texture for resolving multisample
	ID3D11Texture2D* pSSTexture = NULL;	// single-sampled texture
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.CPUAccessFlags = 0;

	if (S_OK != QDXUT::device()->CreateTexture2D(&texDesc, NULL, &pSSTexture))
		return false;

	////////////////////////////////////////
	// Setup viewport
	////////////////////////////////////////

	// Setup viewport
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width    = texWidth;
	viewport.Height   = texHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	ID3D11DeviceContext* pContext = QDXUT::immediateContext();

	////////////////////////////////////////
	// Process each head mesh vertex
	////////////////////////////////////////

	float minAO = 1000.0f;
	float maxAO = -1000.0f;

	// Get pixel data
	QImage img(texWidth, texHeight, QImage::Format_ARGB32);

	for (int vId = 0; vId < m_pBodyModel->numVertices(); vId++)
	{
		progressDlg.setValue(vId);

		XMFLOAT3 vPos = pVertices[vId].position;
		XMFLOAT3 vNormal(0, 0, -1);//pMesh->vertices()[vId].normal;

		// Setup camera
		XMVECTOR pos = XMLoadFloat3(&vPos);
		XMVECTOR normal = XMVector3Normalize(XMLoadFloat3(&vNormal));

		// TRANSFORM
		XMMATRIX bodyTransform = XMLoadFloat4x4(m_pBodyModel->worldTransform());
		pos = XMVector3TransformCoord(pos, bodyTransform);
		//normal = XMVector3TransformNormal(normal, neckTransform);

		XMStoreFloat3(&vPos, pos);

		XMVECTOR at  = pos + normal;

		// generate random UP direction
		XMVECTOR up = XMVectorSet(	(float)rand()/(float)RAND_MAX,
			(float)rand()/(float)RAND_MAX,
			(float)rand()/(float)RAND_MAX, 1.0f);
		up = XMVector3Cross(normal, up);
		up = XMVector3Normalize(up);

		XMFLOAT3 vAt, vUp;
		XMStoreFloat3(&vAt, at);
		XMStoreFloat3(&vUp, up);

		QDXModelCamera cam;
		cam.setEyeLookAtUp(&vPos, &vAt, &vUp);
		cam.setPerspectiveFOV(fov, (float)texWidth / (float)texHeight, zNear, zFar);
		cam.setWindowSize(texWidth, texHeight);

		////////////////////////////////////////
		// RENDER
		////////////////////////////////////////


		float clearColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
		pContext->ClearRenderTargetView(pRenderTargetView, clearColor);
		pContext->ClearDepthStencilView(pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		XMMATRIX view	  = XMLoadFloat4x4(cam.viewMatrix());
		XMMATRIX proj	  = XMLoadFloat4x4(cam.projMatrix());
		XMMATRIX viewProj = XMMatrixMultiply(view, proj);

		m_hView->SetMatrix((float*)&view);
		m_hProjection->SetMatrix((float*)&proj);
		m_hViewProjection->SetMatrix((float*)&viewProj);
		m_hCameraPos->SetFloatVector((const float*)cam.eye());

		pContext->OMSetRenderTargets(1, &pRenderTargetView, pDepthStencilView);
		pContext->RSSetViewports(1, &viewport);

		m_pHairAORenderer->render();


		////////////////////////////////////////
		// Save result to image file
		////////////////////////////////////////

		// Convert multisampled texture to single-sampled.
		//pContext->ResolveSubresource(pSSTexture, 0, pRenderTarget, 0, texDesc.Format);

		//pContext->CopyResource(pStageTexture, pSSTexture);

		pContext->CopyResource(pStageTexture, pRenderTarget);


		D3D11_MAPPED_SUBRESOURCE mapped;
		pContext->Map(pStageTexture, 0, D3D11_MAP_READ, 0, &mapped);

		const uchar* pSrcBits = (const uchar*)mapped.pData;
		uchar*		 pDstBits = img.bits();
		int srcIdx = 0;
		int dstIdx = 0;

		for (int y = 0; y < texHeight; y++)
		{
			memcpy(&(pDstBits[dstIdx]), &(pSrcBits[srcIdx]), 4*texWidth);

			srcIdx += mapped.RowPitch;
			dstIdx += img.bytesPerLine();
		}

		pContext->Unmap(pStageTexture, 0);

		// Average pixel values...
		int sumValue = 0;
		int numPixels = 0;

		float r2 = (texHeight / 2) * (texHeight / 2);
		int idx = 0;
		cv::Vec4b* pPixels = (cv::Vec4b*)img.bits();
		for (int y = 0; y < texHeight; y++)
		{
			float ry2 = (y - texHeight / 2) * (y - texHeight / 2);

			for (int x = 0; x < texWidth; x++)
			{
				float rx2 = (x - texWidth / 2) * (x - texWidth / 2);

				if (rx2 + ry2 < r2)
				{
					sumValue += pPixels[idx][1];
					numPixels++;
				}
				idx++;
			}
		}

		float ao = ((float)sumValue / (float)numPixels) / 255.0f;
		pVertices[vId].reserved.x = ao;

		if (ao < minAO)
			minAO = ao;
		if (ao > maxAO)
			maxAO = ao;
	}

	//QMessageBox::warning(NULL, "AO", QString("AO: %1 ~ %2").arg(minAO).arg(maxAO));

	SAFE_RELEASE(pDepthStencilView);
	SAFE_RELEASE(pDepthStencil);
	SAFE_RELEASE(pRenderTargetView);
	SAFE_RELEASE(pRenderTarget);
	SAFE_RELEASE(pStageTexture);
	SAFE_RELEASE(pSSTexture);

	m_pBodyModel->updateVertexBuffer();

	return true;
}



bool MyScene::batchRenderVideo(QString filename)
{
	QString path = filename.left(filename.lastIndexOf('/') + 1);

	QSettings file(filename, QSettings::IniFormat);

	int firstFrame = file.value("FirstFrame", 0).toInt();
	int lastFrame = file.value("LastFrame", 0).toInt();

	// Load SHD2
	QString fn = path + file.value("FinalSparseStrands", "").toString();
	if (!m_pStrandModel->load(fn))
		return false;

	fn = path + file.value("FinalDenseStrands", "").toString();
	if (!m_pAuxStrandModel->load(fn))
		return false;

	fn = path + file.value("FinalExtraStrands", "").toString();
	if (!m_pDenseStrandModel->load(fn))
		return false;

	// src/dst height
	float srcHeight = file.value("SrcHeight", 0).toFloat();
	float dstHeight = file.value("DstHeight", 0).toFloat();

	// Batch strings
	QString strTransferMatrix	= path + file.value("TransferMatrix", "").toString();
	QString strOriginalImage	= path + file.value("OriginalImage", "").toString();
	QString strOriginalMask		= path + file.value("OriginalHairRegion", "").toString();
	QString strHeadMesh			= path + file.value("HeadMesh", "").toString();
	QString strNeckMesh			= path + file.value("NeckMesh", "").toString();
	QString strHeadTransform	= path + file.value("HeadTransform", "").toString();
	QString strOutputFile		= path + file.value("OutputImage", "%1.bmp").toString();

	// Set visibility
	backgroundLayer()->setVisible(true);
	largeBackgroundLayer()->setVisible(false);
	originalLayer()->setVisible(true);
	m_pBodyModel->setVisible(false);
	m_pHeadModel->setVisible(true);
	headRenderer()->setRenderMode(HeadRenderDepthOnly);

	RenderToFileParams params;
	params.msSampleCount = 8;
	params.msQuality = 32;

	QProgressDialog proDlg;
	proDlg.setLabelText("Calculating ambient occlusion...");
	proDlg.setRange(firstFrame, lastFrame);
	proDlg.setModal(true);
	proDlg.show();


	// For each frame...
	for (int i = firstFrame; i <= lastFrame; i++)
	{
		proDlg.setValue(i);

		// Load hair transform
		if (!loadTransferMatrix(strTransferMatrix.arg(i), srcHeight, dstHeight))
			return false;

		// Load original frame
		if (!loadBackgroundLayer(strOriginalImage.arg(i, 3, 10, QChar('0')), 1000))
			return false;

		if (!loadOriginalLayer(strOriginalImage.arg(i, 3, 10, QChar('0')), strOriginalMask, -500))
			return false;
		
		// Load head mesh
		if (!loadHeadMesh(strHeadMesh.arg(i, 4, 10, QChar('0'))))
			return false;

		if (!loadNeckMesh(strNeckMesh.arg(i, 4, 10, QChar('0'))))
			return false;

		// Load head transform
		if (!loadHeadTransform(strHeadTransform.arg(i, 4, 10, QChar('0'))))
			return false;

		resetCamera();

		if (!renderToFile(strOutputFile.arg(i, 4, 10, QChar('0')), params))
			return false;
	}

	return true;
}


// respond to user stroke
void MyScene::strokeDrawn()
{
	if (m_pStrokesSprite->numPoints() < 1)
		return;

	switch (m_strokeMode)
	{
	case NullStroke:
		break;
	case DeleteStroke:
		deleteStrokeDrawn();
		break;
	case CombStroke:
		printf("Applying combing stroke...\n");
		combStrokeDrawn();
		break;
	case CutStroke:
		printf("Applying cut stroke...\n");
		cutStrokeDrawn();
		break;
	default:
		break;
	}
	m_pStrokesSprite->clearPoints();
}


void calcMorphVertexPosition(const StrandVertex& vertex, const std::vector<float>& weights, XMFLOAT3& pos)
{
	pos = vertex.position;
	pos.x *= weights[0];
	pos.y *= weights[0];
	pos.z *= weights[0];
	
	const int numSources = weights.size();
	switch (numSources)
	{
	case 2:
		pos.x += vertex.tangent.x * weights[1];
		pos.y += vertex.tangent.y * weights[1];
		pos.z += vertex.tangent.z * weights[1];
		break;

	case 3:
		pos.x += vertex.tangent.x * weights[1];
		pos.y += vertex.tangent.y * weights[1];
		pos.z += vertex.tangent.z * weights[1];
		pos.x += vertex.pos2.x * weights[2];
		pos.y += vertex.pos2.y * weights[2];
		pos.z += vertex.pos2.z * weights[2];
		break;

	case 4:
		pos.x += vertex.tangent.x * weights[1];
		pos.y += vertex.tangent.y * weights[1];
		pos.z += vertex.tangent.z * weights[1];
		pos.x += vertex.pos2.x * weights[2];
		pos.y += vertex.pos2.y * weights[2];
		pos.z += vertex.pos2.z * weights[2];
		pos.x += vertex.pos3.x * weights[3];
		pos.y += vertex.pos3.y * weights[3];
		pos.z += vertex.pos3.z * weights[3];
		break;

	case 5:
		pos.x += vertex.tangent.x * weights[1];
		pos.y += vertex.tangent.y * weights[1];
		pos.z += vertex.tangent.z * weights[1];
		pos.x += vertex.pos2.x * weights[2];
		pos.y += vertex.pos2.y * weights[2];
		pos.z += vertex.pos2.z * weights[2];
		pos.x += vertex.pos3.x * weights[3];
		pos.y += vertex.pos3.y * weights[3];
		pos.z += vertex.pos3.z * weights[3];
		pos.x += vertex.pos4.x * weights[4];
		pos.y += vertex.pos4.y * weights[4];
		pos.z += vertex.pos4.z * weights[4];
		break;
	}
}


void MyScene::deleteStrokeDrawn()
{
	printf("Applying delete stroke...");
	const int numPts = m_pStrokesSprite->numPoints();

	// add stroke points to a kd-tree
	ANNpointArray annPts = annAllocPts(numPts, 2);
	for (int i = 0; i < numPts; i++)
	{
		annPts[i][0] = m_pStrokesSprite->point(i).x;
		annPts[i][1] = m_pStrokesSprite->point(i).y;
	}
	ANNkd_tree* kdTree = new ANNkd_tree(annPts, numPts, 2);
	ANNpoint query = annAllocPt(2);
	ANNidx annIdx[1];
	ANNdist annDist[1];

	// Get transform matrices
	XMMATRIX world = XMLoadFloat4x4(m_morphCtrl.worldTransform());

	const float sqRad = m_strokeRadius * m_strokeRadius;

	int numSources = m_pMorphHierarchy->numSources();

	// for each hair strand	
	int numChanges = 0;
	HairStrandModel& model = m_pMorphHierarchy->level(0);
	for (int i = 0; i < model.numStrands(); i++)
	{
		StrandVertex* vertices = model.getStrandAt(i)->vertices();

		// for each vertex...
		for (int j = 0; j < model.getStrandAt(i)->numVertices(); j++)
		{
			// transform vertex to screen space...
			XMFLOAT3 pos;
			calcMorphVertexPosition(vertices[j], m_morphCtrl.weights(), pos);

			XMVECTOR vec = XMLoadFloat3(&pos);
			vec = XMVector3Transform(vec, world);

			XMStoreFloat3(&pos, vec);

			query[0] = pos.x;
			query[1] = -pos.y;

			if (kdTree->annkFRSearch(query, sqRad, 1, annIdx, annDist) > 0)
			{
				// found a valid vertex!
				model.getStrandAt(i)->trim(0);
				numChanges++;
			}
		}
	}

	delete kdTree;
	annDeallocPt(query);
	annDeallocPts(annPts);

	if (numChanges > 0)
		model.updateBuffers();

	printf("DONE.\n");
}

void MyScene::combStrokeDrawn()
{
	if (m_strokeLevel >= m_pMorphHierarchy->numLevels())
	{
		printf("ERROR: cannot apply stroke level!\n");
		return;
	}

	printf("Applying combing stroke...");
	const int numPts = m_pStrokesSprite->numPoints();
	XMFLOAT2* strokePts = m_pStrokesSprite->points();

	// add stroke points to a kd-tree
	ANNpointArray annPts = annAllocPts(numPts, 2);
	for (int i = 0; i < numPts; i++)
	{
		annPts[i][0] = m_pStrokesSprite->point(i).x;
		annPts[i][1] = m_pStrokesSprite->point(i).y;
	}
	ANNkd_tree* kdTree = new ANNkd_tree(annPts, numPts, 2);
	ANNpoint query = annAllocPt(2);
	ANNidx annIdx[1];
	ANNdist annDist[1];

	// Get transform matrices
	XMMATRIX world = XMLoadFloat4x4(m_morphCtrl.worldTransform());

	// Compute inverse transform
	XMVECTOR det;
	XMMATRIX invWorld = XMMatrixInverse(&det, world);

	// smooth stroke
	std::vector<XMFLOAT2> tempPts(numPts);
	tempPts.front() = strokePts[0];
	tempPts.back() = strokePts[numPts-1];
	for (int pass = 0; pass < 5; pass++)
	{
		for (int i = 1; i < numPts - 1; i++)
		{
			tempPts[i].x = strokePts[i-1].x*0.25f + strokePts[i].x*0.5f + strokePts[i+1].x*0.25f;
			tempPts[i].y = strokePts[i-1].y*0.25f + strokePts[i].y*0.5f + strokePts[i+1].y*0.25f;
		}
		memcpy(strokePts, tempPts.data(), sizeof(XMFLOAT2)*numPts);
	}

	// compute direction at each stroke point
	std::vector<XMFLOAT2> strokeDirs(numPts);
	strokeDirs[0].x = strokePts[1].x - strokePts[0].x;
	strokeDirs[0].y = strokePts[1].y - strokePts[0].y;
	for (int i = 1; i < numPts; i++)
	{
		strokeDirs[i].x = strokePts[i].x - strokePts[i-1].x;
		strokeDirs[i].y = strokePts[i].y - strokePts[i-1].y;
		strokeDirs[i].y = -strokeDirs[i].y;
	}
	// smooth direction
	std::vector<XMFLOAT2> tempDirs(numPts);
	tempDirs.front() = strokeDirs.front();
	tempDirs.back() = strokeDirs.back();
	for (int pass = 0; pass < 8; pass++)
	{
		tempDirs[0].x = 0.5f*(strokeDirs[0].x + strokeDirs[1].x);
		tempDirs[0].y = 0.5f*(strokeDirs[0].y + strokeDirs[1].y);
		tempDirs.back().x = 0.5f*(strokeDirs[numPts-1].x + strokeDirs[numPts-2].x);
		tempDirs.back().x = 0.5f*(strokeDirs[numPts-1].y + strokeDirs[numPts-2].y);
		for (int i = 1; i < numPts - 1; i++)
		{
			tempDirs[i].x = 0.333f*(strokeDirs[i-1].x + strokeDirs[i].x + strokeDirs[i+1].x);
			tempDirs[i].y = 0.333f*(strokeDirs[i-1].y + strokeDirs[i].y + strokeDirs[i+1].y);
		}
		memcpy(strokeDirs.data(), tempDirs.data(), sizeof(XMFLOAT2)*numPts);
	}
	// normalize and apply intensity
	for (int i = 0; i < numPts; i++)
	{
		float len = sqrtf(strokeDirs[i].x*strokeDirs[i].x + strokeDirs[i].y*strokeDirs[i].y);
		if (len > 0.00001f)
		{
			strokeDirs[i].x *= m_strokeIntensity / len;
			strokeDirs[i].y *= m_strokeIntensity / len;
		}
	}


	const float sqRad = m_strokeRadius * m_strokeRadius;

	// for each hair strand	
	int numChanges = 0;
	HairStrandModel& model = m_pMorphHierarchy->level(m_strokeLevel);

	std::vector<XMFLOAT3> projPos, newProjPos;
	projPos.reserve(NUM_UNISAM_VERTICES);
	newProjPos.reserve(NUM_UNISAM_VERTICES);

	for (int i = 0; i < model.numStrands(); i++)
	{
		Strand* strand = model.getStrandAt(i);
		StrandVertex* vertices = strand->vertices();

		// Transform to world space
		projPos.resize(strand->numVertices());
		for (int j = 0; j < strand->numVertices(); j++)
		{
			XMVECTOR vec = XMLoadFloat3(&(vertices[j].position));
			XMStoreFloat3(&(projPos[j]), XMVector3Transform(vec, world));
		}

		newProjPos.resize(strand->numVertices());
		newProjPos[0] = projPos[0];
		newProjPos[1] = projPos[1];

		// for each intermediate vertex...
		bool isModified = false;
		for (int j = 1; j < strand->numVertices() - 1; j++)
		{
			query[0] = projPos[j].x;
			query[1] = -projPos[j].y;

			if (kdTree->annkFRSearch(query, sqRad, 1, annIdx, annDist) > 0)
			{
				XMFLOAT2 newDir;
				transformJointAngleXY(projPos.data(), newProjPos.data(), j, &newDir);

				float dirLen = sqrtf(newDir.x*newDir.x + newDir.y*newDir.y);
				if (dirLen > 0.0000001f)
				{
					newDir.x /= dirLen;
					newDir.y /= dirLen;
				}

				// Add comb's influence (with a fall-off from stroke center)
				float w = expf(-3.0f*annDist[0]/sqRad);
				newDir.x += strokeDirs[annIdx[0]].x * w;
				newDir.y += strokeDirs[annIdx[0]].y * w;

				normalizeFloat2(newDir);
				
				// update next vertex
				newProjPos[j+1].x = newProjPos[j].x + newDir.x*dirLen;
				newProjPos[j+1].y = newProjPos[j].y + newDir.y*dirLen;
				newProjPos[j+1].z = projPos[j+1].z;

				isModified = true;
				numChanges++;
			}
			else
			{
				if (isModified)
				{
					XMFLOAT2 newDir;
					transformJointAngleXY(projPos.data(), newProjPos.data(), j, &newDir);

					newProjPos[j+1].x = newProjPos[j].x + newDir.x;
					newProjPos[j+1].y = newProjPos[j].y + newDir.y;
					newProjPos[j+1].z = projPos[j+1].z;
				}
				else
					newProjPos[j+1] = projPos[j+1];
			}
		} // end for each vertex

		// transform back to world space
		for (int j = 2; j < strand->numVertices(); j++)
		{
			XMVECTOR vec = XMLoadFloat3(&(newProjPos[j]));
			XMStoreFloat3(&(vertices[j].position), XMVector3Transform(vec, invWorld));

		}
	} // end for each strand

	delete kdTree;
	annDeallocPt(query);
	annDeallocPts(annPts);

	if (numChanges > 0)
		model.updateBuffers();

	printf("DONE.\n");
}


inline void MyScene::normalizeFloat2(XMFLOAT2& vec)
{
	float len = vec.x*vec.x + vec.y*vec.y;
	if (len > 0.0000001f)
	{
		vec.x /= len;
		vec.y /= len;
	}
}


void MyScene::transformJointAngleXY(const XMFLOAT3* origVerts, const XMFLOAT3* newVerts,
										int vId, XMFLOAT2* pNewDir)
{
	XMFLOAT2 origDir(origVerts[vId+1].x - origVerts[vId].x, 
					 origVerts[vId+1].y - origVerts[vId].y);

	XMFLOAT2 origTangent(origVerts[vId].x - origVerts[vId-1].x, 
						 origVerts[vId].y - origVerts[vId-1].y);
	float origLen = sqrtf(origTangent.x*origTangent.x + origTangent.y*origTangent.y);
	if (origLen < 0.000001f)
	{
		*pNewDir = origDir;
		return;
	}

	XMFLOAT2 newTangent(newVerts[vId].x - newVerts[vId-1].x, 
						newVerts[vId].y - newVerts[vId-1].y);
	float newLen = sqrtf(newTangent.x*newTangent.x + newTangent.y*newTangent.y);
	if (newLen < 0.0000001f)
	{
		*pNewDir = origDir;
		return;
	}

	// relative input angle
	origDir.x /= origLen;
	origDir.y /= origLen;
	XMFLOAT2 origNormal(origTangent.y, -origTangent.x);
	XMFLOAT2 origRelDir(origDir.x*origTangent.x + origDir.y*origTangent.y,
						origDir.x*origNormal.x  + origDir.y*origNormal.y);

	// absolute new angle
	newTangent.x /= newLen;
	newTangent.y /= newLen;
	XMFLOAT2 newNormal(newTangent.y, -newTangent.x);
	pNewDir->x = origRelDir.x*newTangent.x + origRelDir.y*newNormal.x;
	pNewDir->y = origRelDir.x*newTangent.y + origRelDir.y*newNormal.y;
}

void MyScene::cutStrokeDrawn()
{
	printf("Applying cut stroke...");
	const int numPts = m_pStrokesSprite->numPoints();

	// add stroke points to a kd-tree
	ANNpointArray annPts = annAllocPts(numPts, 2);
	for (int i = 0; i < numPts; i++)
	{
		annPts[i][0] = m_pStrokesSprite->point(i).x;
		annPts[i][1] = m_pStrokesSprite->point(i).y;
	}
	ANNkd_tree* kdTree = new ANNkd_tree(annPts, numPts, 2);
	ANNpoint query = annAllocPt(2);
	ANNidx annIdx[1];
	ANNdist annDist[1];

	// Get transform matrices
	XMMATRIX world = XMLoadFloat4x4(m_morphCtrl.worldTransform());

	const float sqRad = m_strokeRadius * m_strokeRadius;

	// for each hair strand	
	int numChanges = 0;
	HairStrandModel& model = m_pMorphHierarchy->level(0);
	for (int i = 0; i < model.numStrands(); i++)
	{
		StrandVertex* vertices = model.getStrandAt(i)->vertices();

		// for each vertex...
		for (int j = 0; j < model.getStrandAt(i)->numVertices(); j++)
		{
			// transform vertex to screen space...
			XMFLOAT3 pos;
			calcMorphVertexPosition(vertices[j], m_morphCtrl.weights(), pos);

			XMVECTOR vec = XMLoadFloat3(&pos);
			vec = XMVector3Transform(vec, world);

			XMStoreFloat3(&pos, vec);

			query[0] = pos.x;
			query[1] = -pos.y;

			if (kdTree->annkFRSearch(query, sqRad, 1, annIdx, annDist) > 0)
			{
				// found a valid vertex!

				// random location
				int d = cv::theRNG().uniform(-m_strokeFuzziness, m_strokeFuzziness+1);
				model.getStrandAt(i)->trim(j+d);
				numChanges++;
			}
		}
	}

	delete kdTree;
	annDeallocPt(query);
	annDeallocPts(annPts);

	if (numChanges > 0)
		model.updateBuffers();

	printf("DONE.\n");
}