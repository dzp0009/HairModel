#include "BodyLayerRenderer.h"
#include "QDXImageSprite.h"


BodyLayerRenderer::BodyLayerRenderer()
	: m_pVertexLayout(NULL)
{
}


BodyLayerRenderer::~BodyLayerRenderer()
{
	release();
}


bool BodyLayerRenderer::create(ID3DX11Effect* pEffect)
{
	release();

	m_hTechnique = pEffect->GetTechniqueByName("BodyLayer");
	m_hWorld	 = pEffect->GetVariableByName("g_world")->AsMatrix();
	m_hTexture	 = pEffect->GetVariableByName("g_texture")->AsShaderResource();

	// Create vertex layout
	if (!QDXUT::createInputLayoutFromTechnique(m_hTechnique, 0, QDXImageQuadVertex::s_elemDesc,
		QDXImageQuadVertex::s_numElem, &m_pVertexLayout))
	{
		release();
		return false;
	}

	return true;
}



void BodyLayerRenderer::release()
{
	SAFE_RELEASE(m_pVertexLayout);
	m_hTechnique = NULL;
}



void BodyLayerRenderer::render()
{
	ID3D11DeviceContext* pContext = QDXUT::immediateContext();

	pContext->IASetInputLayout(m_pVertexLayout);

	foreach (QDXObject* obj, m_objects)
	{
		QDXImageSprite* pImgSprite = (QDXImageSprite*)obj;
		if (!pImgSprite->isVisible())
			continue;

		m_hWorld->SetMatrix((float*)pImgSprite->worldTransform());
		m_hTexture->SetResource(pImgSprite->textureSRV());

		m_hTechnique->GetPassByIndex(0)->Apply(0, pContext);

		pImgSprite->render();
	}
}
