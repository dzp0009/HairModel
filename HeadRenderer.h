#pragma once

#include "QDXRenderGroup.h"

enum HeadRenderMode
{
	HeadRenderSimple,
	HeadRenderTexcoord,
	HeadRenderTextured,
	HeadRenderDepthOnly,
	HeadRenderNeckMode,
};

class HeadRenderer : public QDXRenderGroup
{
public:
	HeadRenderer();
	~HeadRenderer();

	bool	create(ID3DX11Effect* pEffect);
	void	release();

	void	render();

	void	setRenderMode(HeadRenderMode mode);

	void	setNeckBlend(float blend) { m_neckBlend = blend; }

	bool	loadTextureFromFile(QString filename);

private:

	HeadRenderMode			m_renderMode;

	ID3DX11EffectTechnique*					m_hTechnique;
	ID3DX11EffectTechnique*					m_hTechSimple;
	ID3DX11EffectTechnique*					m_hTechTexcoord;
	ID3DX11EffectTechnique*					m_hTechTextured;
	ID3DX11EffectTechnique*					m_hTechNeckMode;
	ID3DX11EffectTechnique*					m_hTechDepthOnly;

	ID3DX11EffectMatrixVariable*			m_hWorld;
	ID3DX11EffectShaderResourceVariable*	m_hHeadTexture;
	ID3DX11EffectScalarVariable*			m_hNeckBlend;

	ID3D11InputLayout*						m_pVertexLayout;
	ID3D11ShaderResourceView*				m_pHeadTexture;

	float	m_neckBlend;
};

