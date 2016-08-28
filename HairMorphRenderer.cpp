#include "HairMorphRenderer.h"

#include "HeadMorphMesh.h"
#include "HairStrandModel.h"
#include "MorphController.h"

HairMorphRenderer::HairMorphRenderer(MorphController* pMorphCtrl)
	: m_pVertexLayout(NULL), m_pMorphCtrl(pMorphCtrl)
{
	setName("HairMorphRenderer");

	m_strandRandWeight = 0;
	m_hairWidth = 1.0f;
}


HairMorphRenderer::~HairMorphRenderer()
{
	release();
}


bool HairMorphRenderer::create(ID3DX11Effect* pEffect)
{
	release();

	m_hTechnique	= pEffect->GetTechniqueByName("StrandMorphColor");
	m_hWorld		= pEffect->GetVariableByName("g_world")->AsMatrix();
	m_hHairWidth	= pEffect->GetVariableByName("g_hairWidth")->AsScalar();
	m_hStrandRandWeight	= pEffect->GetVariableByName("g_strandRandWeight")->AsScalar();
	m_hMorphWeights = pEffect->GetVariableByName("g_morphWeights")->AsScalar();
	m_hNumSources	= pEffect->GetVariableByName("g_numSources")->AsScalar();

	if (!QDXUT::createInputLayoutFromTechnique(m_hTechnique, 0, StrandVertex::s_elemDesc,
		StrandVertex::s_numElem, &m_pVertexLayout))
	{
		release();
		return false;
	}

	return true;
}


void HairMorphRenderer::release()
{
	SAFE_RELEASE(m_pVertexLayout);

	m_hTechnique = NULL;
}


void HairMorphRenderer::render()
{
	ID3D11DeviceContext* pContext = QDXUT::immediateContext();

	pContext->IASetInputLayout(m_pVertexLayout);

	foreach (QDXObject* obj, m_objects)
	{
		if (obj->isVisible())
		{
			// Set *interpolated* world transform here
			m_hWorld->SetMatrix((float*)m_pMorphCtrl->worldTransform());
			
			const std::vector<float>& weights = m_pMorphCtrl->weights();
			if (!weights.empty())
				m_hMorphWeights->SetFloatArray(weights.data(), 0, weights.size());
			m_hNumSources->SetInt(weights.size());

			m_hStrandRandWeight->SetFloat(m_strandRandWeight);

			m_hHairWidth->SetFloat(m_hairWidth);//pModel->morphStrandModel()->strandWidth());

			m_hTechnique->GetPassByIndex(0)->Apply(0, pContext);
			obj->render();
		}
	}
}


void HairMorphRenderer::setStrandRandWeight(float w)
{
	m_strandRandWeight = w;
}


void HairMorphRenderer::setHairWidth(float width)
{
	m_hairWidth = width;
}