#include "HairAORenderer.h"

#include "HairStrandModel.h"

HairAORenderer::HairAORenderer(void)
	: m_pVertexLayout(NULL)
{
	m_hairColor = XMFLOAT4(0, 0, 0, 1);

	XMStoreFloat4x4(&m_transferMat, XMMatrixIdentity());
}


HairAORenderer::~HairAORenderer(void)
{
	release();
}

bool HairAORenderer::create(ID3DX11Effect* pEffect)
{
	release();

	m_hTechnique	= pEffect->GetTechniqueByName("StrandAO");
	m_hWorld		= pEffect->GetVariableByName("g_world")->AsMatrix();
	m_hTransferMat	= pEffect->GetVariableByName("g_transferMat")->AsMatrix();
	m_hHairColor	= pEffect->GetVariableByName("g_hairColor")->AsVector();
	m_hHairWidth	= pEffect->GetVariableByName("g_hairWidth")->AsScalar();

	if (!QDXUT::createInputLayoutFromTechnique(m_hTechnique, 0, StrandVertex::s_elemDesc,
		StrandVertex::s_numElem, &m_pVertexLayout))
	{
		release();
		return false;
	}

	return true;
}


void HairAORenderer::release()
{
	SAFE_RELEASE(m_pVertexLayout);
}


void HairAORenderer::render()
{
	ID3D11DeviceContext* pContext = QDXUT::immediateContext();

	pContext->IASetInputLayout(m_pVertexLayout);

	foreach (QDXObject* obj, m_objects)
	{
		if (obj->isVisible())
		{
			HairStrandModel* pStrandModel = (HairStrandModel*)obj;
			m_hWorld->SetMatrix((float*)pStrandModel->worldTransform());
			m_hHairColor->SetFloatVector((float*)&m_hairColor);
			m_hHairWidth->SetFloat(pStrandModel->strandWidth());

			m_hTransferMat->SetMatrix((const float*)&m_transferMat);

			m_hTechnique->GetPassByIndex(0)->Apply(0, pContext);

			pStrandModel->render();
		}
	}
}


void HairAORenderer::resetTransferMat()
{
	XMStoreFloat4x4(&m_transferMat, XMMatrixIdentity());
}


void HairAORenderer::setTransferMat(float* pMat)
{
	m_transferMat = XMFLOAT4X4(pMat);
}