#include "HeadMorphRenderer.h"

#include "HeadMorphMesh.h"
#include "QDXMeshObject.h"
#include "MorphController.h"

HeadMorphRenderer::HeadMorphRenderer(MorphController* pMorphCtrl)
	: m_pVertexLayout(NULL), m_hTechnique(NULL), m_pMorphCtrl(pMorphCtrl)
{
	setName("HeadMorphRenderer");

	for (int i = 0; i < MAX_NUM_MORPH_SRC; i++)
	{
		m_hSrcTextures[i] = NULL;
		m_pSrcTextures[i] = NULL;
	}

	m_isLocked = false;
}


HeadMorphRenderer::~HeadMorphRenderer(void)
{
	release();
}


bool HeadMorphRenderer::create(ID3DX11Effect* pEffect)
{
	release();

	m_hTechnique	= pEffect->GetTechniqueByName("HeadMorphSimple");
	m_hWorld		= pEffect->GetVariableByName("g_world")->AsMatrix();
	m_hMorphWeights = pEffect->GetVariableByName("g_morphWeights")->AsScalar();
	m_hNumSources	= pEffect->GetVariableByName("g_numSources")->AsScalar();

	for (int i = 0; i < MAX_NUM_MORPH_SRC; i++)
	{
		QString varName = QString("g_headSrcTex%1").arg(i);
		m_hSrcTextures[i] = pEffect->GetVariableByName(varName.toAscii())->AsShaderResource();
	}

	if (!QDXUT::createInputLayoutFromTechnique(m_hTechnique, 0, MorphMeshVertex::s_inputElementDesc,
		MorphMeshVertex::s_numElements, &m_pVertexLayout))
	{
		printf("ERROR: failed to create input layout for MorphMeshVertex!\n");
		release();
		return false;
	}

	return true;
}


void HeadMorphRenderer::release()
{
	SAFE_RELEASE(m_pVertexLayout);

	for (int i = 0; i < MAX_NUM_MORPH_SRC; i++)
		SAFE_RELEASE(m_pSrcTextures[i]);
}



void HeadMorphRenderer::render()
{
	ID3D11DeviceContext* pContext = QDXUT::immediateContext();

	pContext->IASetInputLayout(m_pVertexLayout);

	foreach (QDXObject* obj, m_objects)
	{
		if (obj->isVisible())
		{
			m_hWorld->SetMatrix((float*)m_pMorphCtrl->worldTransform());
			for (int i = 0; i < MAX_NUM_MORPH_SRC; i++)
				m_hSrcTextures[i]->SetResource(m_pSrcTextures[i]);
			
			const std::vector<float>& weights = m_isLocked ? m_lockedWeights : 
															 m_pMorphCtrl->weights();
			if (!weights.empty())
				m_hMorphWeights->SetFloatArray(weights.data(), 0, weights.size());
			m_hNumSources->SetInt(weights.size());

			m_hTechnique->GetPassByIndex(0)->Apply(0, pContext);

			obj->render();
		}
	}
}


bool HeadMorphRenderer::loadHeadTexture(int srcIdx, QString fileName)
{
	D3DX11_IMAGE_LOAD_INFO loadInfo;
	ZeroMemory(&loadInfo, sizeof(loadInfo));
	loadInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	loadInfo.Format	   = DXGI_FORMAT_R8G8B8A8_UNORM;// SRGB?
	loadInfo.MipLevels = 1;
	loadInfo.Usage	   = D3D11_USAGE_IMMUTABLE;

	SAFE_RELEASE(m_pSrcTextures[srcIdx]);
	if (S_OK != D3DX11CreateShaderResourceViewFromFileA(
				QDXUT::device(), fileName.toAscii(), &loadInfo,
				NULL, &(m_pSrcTextures[srcIdx]), NULL))
	{
		return false;
	}

	return true;
}


void HeadMorphRenderer::lock()
{
	m_lockedWeights = m_pMorphCtrl->weights();
	m_isLocked = true;
}


void HeadMorphRenderer::unlock()
{
	m_isLocked = false;
}