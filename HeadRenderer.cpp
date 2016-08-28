#include "HeadRenderer.h"

#include "QDXMeshObject.h"

HeadRenderer::HeadRenderer(void)
	: m_pVertexLayout(NULL), m_pHeadTexture(NULL)
{
	m_name = QString("Head renderer");

	m_renderMode = HeadRenderSimple;

	m_neckBlend = 0.02f;
}


HeadRenderer::~HeadRenderer(void)
{
	release();
}


bool HeadRenderer::create(ID3DX11Effect* pEffect)
{
	release();

	m_hTechSimple	= pEffect->GetTechniqueByName("HeadSimple");
	m_hTechTexcoord	= pEffect->GetTechniqueByName("HeadTexcoord");
	m_hTechTextured	= pEffect->GetTechniqueByName("HeadTextured");
	m_hTechNeckMode = pEffect->GetTechniqueByName("NeckTextured");
	m_hTechDepthOnly= pEffect->GetTechniqueByName("HeadDepthOnly");
	m_hWorld		= pEffect->GetVariableByName("g_world")->AsMatrix();
	m_hHeadTexture	= pEffect->GetVariableByName("g_headTexture")->AsShaderResource();
	m_hNeckBlend	= pEffect->GetVariableByName("g_neckBlend")->AsScalar();

	setRenderMode(m_renderMode);

	if (!QDXUT::createInputLayoutFromTechnique(m_hTechnique, 0, SimpleVertex::s_inputElementDesc,
		SimpleVertex::s_numElements, &m_pVertexLayout))
	{
		release();
		return false;
	}

	return true;
}

void HeadRenderer::release()
{
	SAFE_RELEASE(m_pVertexLayout);
	SAFE_RELEASE(m_pHeadTexture);
}


void HeadRenderer::render()
{
	ID3D11DeviceContext* pContext = QDXUT::immediateContext();

	pContext->IASetInputLayout(m_pVertexLayout);

	foreach (QDXObject* obj, m_objects)
	{
		if (obj->isVisible())
		{
			m_hWorld->SetMatrix((float*)obj->worldTransform());
			m_hHeadTexture->SetResource(m_pHeadTexture);
			m_hNeckBlend->SetFloat(m_neckBlend);

			m_hTechnique->GetPassByIndex(0)->Apply(0, pContext);

			obj->render();
		}
	}
}


void HeadRenderer::setRenderMode(HeadRenderMode mode)
{
	m_renderMode = mode;

	switch (mode)
	{
	case HeadRenderSimple:
		m_hTechnique = m_hTechSimple;
		break;

	case HeadRenderTexcoord:
		m_hTechnique = m_hTechTexcoord;
		break;

	case HeadRenderTextured:
		m_hTechnique = m_hTechTextured;
		break;

	case HeadRenderDepthOnly:
		m_hTechnique = m_hTechDepthOnly;
		break;

	case HeadRenderNeckMode:
		m_hTechnique = m_hTechNeckMode;
		break;
	}
}


bool HeadRenderer::loadTextureFromFile(QString filename)
{
	SAFE_RELEASE(m_pHeadTexture);

	D3DX11_IMAGE_LOAD_INFO loadInfo;
	ZeroMemory(&loadInfo, sizeof(loadInfo));
	loadInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	loadInfo.Format	   = DXGI_FORMAT_R8G8B8A8_UNORM;// SRGB?
	loadInfo.MipLevels = 1;
	loadInfo.Usage	   = D3D11_USAGE_IMMUTABLE;

	if (S_OK != D3DX11CreateShaderResourceViewFromFileA(
				QDXUT::device(), filename.toAscii(), &loadInfo,
				NULL, &m_pHeadTexture, NULL))
	{
		return false;
	}

	return true;
}