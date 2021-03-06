#include "OrigLayerRenderer.h"
#include "QDXImageSprite.h"

OrigLayerRenderer::OrigLayerRenderer()
	: m_pVertexLayout(NULL)
{
}



OrigLayerRenderer::~OrigLayerRenderer()
{
	release();
}


bool OrigLayerRenderer::create(ID3DX11Effect* pEffect)
{
	release();

	m_hTechnique = pEffect->GetTechniqueByName("OrigImageLayer");
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



void OrigLayerRenderer::release()
{
	SAFE_RELEASE(m_pVertexLayout);
	m_hTechnique = NULL;
}



void OrigLayerRenderer::render()
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
