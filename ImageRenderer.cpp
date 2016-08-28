#include "ImageRenderer.h"
#include "QDXImageSprite.h"

ImageRenderer::ImageRenderer()
	: m_pVertexLayout(NULL), m_opacity(1.0f)
{
	m_name = QString("Image Renderer");
}


ImageRenderer::~ImageRenderer()
{
	release();
}


bool ImageRenderer::create(ID3DX11Effect* pEffect)
{
	release();

	m_hTechnique = pEffect->GetTechniqueByName("ImageDefault");
	m_hWorld	 = pEffect->GetVariableByName("g_world")->AsMatrix();
	m_hOpacity	 = pEffect->GetVariableByName("g_imageOpacity")->AsScalar();
	m_hTexture	 = pEffect->GetVariableByName("g_texture")->AsShaderResource();
	m_hIsSelected= pEffect->GetVariableByName("g_selected")->AsScalar();

	// Create vertex layout
	if (!QDXUT::createInputLayoutFromTechnique(m_hTechnique, 0, QDXImageQuadVertex::s_elemDesc,
		QDXImageQuadVertex::s_numElem, &m_pVertexLayout))
	{
		release();
		return false;
	}

	return true;
}


void ImageRenderer::release()
{
	SAFE_RELEASE(m_pVertexLayout);
	m_hTechnique = NULL;
}


void ImageRenderer::render()
{
	ID3D11DeviceContext* pContext = QDXUT::immediateContext();

	pContext->IASetInputLayout(m_pVertexLayout);

	m_hOpacity->SetFloat(m_opacity);

	foreach (QDXObject* obj, m_objects)
	{
		QDXImageSprite* pImgSprite = (QDXImageSprite*)obj;
		if (!pImgSprite->isVisible())
			continue;

		m_hWorld->SetMatrix((float*)pImgSprite->worldTransform());
		m_hTexture->SetResource(pImgSprite->textureSRV());
		m_hIsSelected->SetBool(pImgSprite->isSelected());

		m_hTechnique->GetPassByIndex(0)->Apply(0, pContext);

		pImgSprite->render();
	}
}
