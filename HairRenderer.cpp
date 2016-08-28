#include "HairRenderer.h"

#include "HairStrandModel.h"
#include "Marschner.h"

HairRenderer::HairRenderer()
	: m_pVertexLayout(NULL), 
	m_pTexHairNoiseSRV(NULL), m_pTexNormalGradientSRV(NULL), m_pTexTipNoiseSRV(NULL),
	m_pMarschnerTex1(NULL), m_pMarschnerTex2(NULL),
	m_pMarschnerSRV1(NULL), m_pMarschnerSRV2(NULL)
{
	m_name = QString("Hair Renderer");

	m_hairColor  = XMFLOAT4(1, 1, 1, 1);

	m_tangentTexWeight = 0.5f;
	m_tangentTexScaleX = 1.0f;
	m_tangentTexScaleY = 1.0f;

	m_normalGradWeight = 0.5f;
	m_strandRandWeight = 0.2f;
	m_forceOpaque	   = 0.5f;

	m_ka = 0.7f;
	m_kd = 1.0f;
	m_kr = 1.0f;

	XMStoreFloat4x4(&m_transferMat, XMMatrixIdentity());

	m_renderMode = HairRenderGlobalColor;
}


HairRenderer::~HairRenderer()
{
	release();
}

bool HairRenderer::create(ID3DX11Effect* pEffect)
{
	release();

	m_hTechStrandColor	= pEffect->GetTechniqueByName("StrandColor");
	m_hTechGlobalColor	= pEffect->GetTechniqueByName("StrandGlobalColor");
	m_hTechTangent		= pEffect->GetTechniqueByName("StrandTangent");
	m_hTechLighting		= pEffect->GetTechniqueByName("StrandLighting");
	m_hTechMarschner	= pEffect->GetTechniqueByName("StrandMarschner");
	m_hWorld			= pEffect->GetVariableByName("g_world")->AsMatrix();
	m_hTransferMat		= pEffect->GetVariableByName("g_transferMat")->AsMatrix();
	m_hHairColor		= pEffect->GetVariableByName("g_hairColor")->AsVector();
	m_hHairWidth		= pEffect->GetVariableByName("g_hairWidth")->AsScalar();
	m_hTangentTexWeight	= pEffect->GetVariableByName("g_tangentTexWeight")->AsScalar();
	m_hTangentTexScaleX	= pEffect->GetVariableByName("g_tangentTexScaleX")->AsScalar();
	m_hTangentTexScaleY	= pEffect->GetVariableByName("g_tangentTexScaleY")->AsScalar();
	m_hNormalGradWeight = pEffect->GetVariableByName("g_normalGradWeight")->AsScalar();
	m_hStrandRandWeight	= pEffect->GetVariableByName("g_strandRandWeight")->AsScalar();
	m_hForceOpaque		= pEffect->GetVariableByName("g_forceOpaque")->AsScalar();
	m_hKa				= pEffect->GetVariableByName("g_ka")->AsScalar();
	m_hKd				= pEffect->GetVariableByName("g_kd")->AsScalar();
	m_hKr				= pEffect->GetVariableByName("g_kr")->AsScalar();
	m_hTexHairNoise		= pEffect->GetVariableByName("g_texHairNoise")->AsShaderResource();
	m_hTexNormalGradient= pEffect->GetVariableByName("g_texNormalGradient")->AsShaderResource();
	m_hMarschner1		= pEffect->GetVariableByName("g_marschner1")->AsShaderResource();
	m_hMarschner2		= pEffect->GetVariableByName("g_marschner2")->AsShaderResource();

	setRenderMode(m_renderMode);

	if (!QDXUT::createInputLayoutFromTechnique(m_hTechnique, 0, StrandVertex::s_elemDesc,
		StrandVertex::s_numElem, &m_pVertexLayout))
	{
		release();
		return false;
	}

	// Load noise texture from file
	D3DX11_IMAGE_LOAD_INFO loadInfo;
	ZeroMemory(&loadInfo, sizeof(loadInfo));
	loadInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	loadInfo.Format	   = DXGI_FORMAT_R8G8B8A8_UNORM;

	if (S_OK != D3DX11CreateShaderResourceViewFromFile(QDXUT::device(), L"hair_noise.png", 
		&loadInfo, NULL, &m_pTexHairNoiseSRV, NULL))
	{
		release();
		return false;
	}

	if (S_OK != D3DX11CreateShaderResourceViewFromFile(QDXUT::device(), L"hair_normal_gradient.png",
		&loadInfo, NULL, &m_pTexNormalGradientSRV, NULL))
	{
		release();
		return false;
	}

	// Precompute Marschner's model textures.
	XMFLOAT3 absorption(0.5f, 0.5f, 0.5f);
	if (!Marschner::CreateTextures(256, 256, 1.55f, &absorption, -0.16f, 0.12f,
								   &m_pMarschnerTex1, &m_pMarschnerTex2))
	{
		release();
		return false;
	}

	// Create SRVs
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	if (S_OK != QDXUT::device()->CreateShaderResourceView(m_pMarschnerTex1, &srvDesc, &m_pMarschnerSRV1) ||
		S_OK != QDXUT::device()->CreateShaderResourceView(m_pMarschnerTex2, &srvDesc, &m_pMarschnerSRV2))
	{
		release();
		return false;
	}

	return true;
}


void HairRenderer::release()
{
	SAFE_RELEASE(m_pVertexLayout);
	SAFE_RELEASE(m_pTexHairNoiseSRV);
	SAFE_RELEASE(m_pTexNormalGradientSRV);
	SAFE_RELEASE(m_pTexTipNoiseSRV);

	SAFE_RELEASE(m_pMarschnerSRV1);
	SAFE_RELEASE(m_pMarschnerSRV2);
	SAFE_RELEASE(m_pMarschnerTex1);
	SAFE_RELEASE(m_pMarschnerTex2);

	m_hTechnique = NULL;
}


void HairRenderer::render()
{
	ID3D11DeviceContext* pContext = QDXUT::immediateContext();

	pContext->IASetInputLayout(m_pVertexLayout);

	foreach (QDXObject* obj, m_objects)
	{
		if (obj->isVisible())
		{
			//HairStrandModel* pStrandModel = (HairStrandModel*)obj;
			m_hWorld->SetMatrix((float*)obj->worldTransform());
			m_hHairColor->SetFloatVector((float*)&m_hairColor);
			m_hHairWidth->SetFloat(1.0f);//pStrandModel->strandWidth());

			m_hTangentTexWeight->SetFloat(m_tangentTexWeight);
			m_hTangentTexScaleX->SetFloat(m_tangentTexScaleX);
			m_hTangentTexScaleY->SetFloat(m_tangentTexScaleY);
			m_hTexHairNoise->SetResource(m_pTexHairNoiseSRV);

			m_hNormalGradWeight->SetFloat(m_normalGradWeight);
			m_hTexNormalGradient->SetResource(m_pTexNormalGradientSRV);

			m_hStrandRandWeight->SetFloat(m_strandRandWeight);
			m_hForceOpaque->SetFloat(m_forceOpaque);

			m_hKa->SetFloat(m_ka);
			m_hKd->SetFloat(m_kd);
			m_hKr->SetFloat(m_kr);

			m_hTransferMat->SetMatrix((const float*)&m_transferMat);

			m_hMarschner1->SetResource(m_pMarschnerSRV1);
			m_hMarschner2->SetResource(m_pMarschnerSRV2);

			m_hTechnique->GetPassByIndex(0)->Apply(0, pContext);

			obj->render();
		}
	}
}


void HairRenderer::setRenderMode(HairRenderMode mode)
{
	m_renderMode = mode;

	switch (mode)
	{
	case HairRenderStrandColor:
		m_hTechnique = m_hTechStrandColor;
		break;

	case HairRenderGlobalColor:
		m_hTechnique = m_hTechGlobalColor;
		break;

	case HairRenderTangent:
		m_hTechnique = m_hTechTangent;
		break;

	case HairRenderLighting:
		m_hTechnique = m_hTechLighting;
		break;

	case HairRenderMarschner:
		m_hTechnique = m_hTechMarschner;
		break;
	}
}


void HairRenderer::setHairColor(const float* rgba)
{
	m_hairColor = XMFLOAT4(rgba);
}

void HairRenderer::setTangentTexture(float weight, float scaleX, float scaleY)
{
	m_tangentTexWeight = weight;
	m_tangentTexScaleX = scaleX;
	m_tangentTexScaleY = scaleY;
}

void HairRenderer::setNormalGradient(float weight)
{
	m_normalGradWeight = weight;
}


void HairRenderer::resetTransferMat()
{
	XMStoreFloat4x4(&m_transferMat, XMMatrixIdentity());
}


void HairRenderer::setTransferMat(float* pMat)
{
	m_transferMat = XMFLOAT4X4(pMat);
}