#pragma once

#include "QDXRenderGroup.h"


enum HairRenderMode
{
	HairRenderStrandColor,
	HairRenderGlobalColor,
	HairRenderTangent,
	HairRenderLighting,
	HairRenderMarschner,
};

class HairRenderer : public QDXRenderGroup
{
public:
	HairRenderer();
	~HairRenderer();

	bool	create(ID3DX11Effect* pEffect);
	void	release();

	void	render();

	HairRenderMode	renderMode() const { return m_renderMode; }
	void			setRenderMode(HairRenderMode mode);

	XMFLOAT4		hairColor() const { return m_hairColor; }
	void			setHairColor(const float* rgba);

	void			setTangentTexture(float weight, float scaleX, float scaleY);

	void			setNormalGradient(float weight);

	void			setStrandRandWeight(float weight) { m_strandRandWeight = weight; }
	void			setForceOpaque(float weight) { m_forceOpaque = weight; }

	void			setKa(float ka) { m_ka = ka; }
	void			setKd(float kd) { m_kd = kd; }
	void			setKr(float kr) { m_kr = kr; }

	void			resetTransferMat();
	void			setTransferMat(float* pMat);

private:

	ID3DX11EffectTechnique*			m_hTechnique;
	ID3DX11EffectTechnique*			m_hTechStrandColor;
	ID3DX11EffectTechnique*			m_hTechGlobalColor;
	ID3DX11EffectTechnique*			m_hTechTangent;
	ID3DX11EffectTechnique*			m_hTechLighting;
	ID3DX11EffectTechnique*			m_hTechMarschner;

	ID3DX11EffectMatrixVariable*	m_hWorld;
	ID3DX11EffectMatrixVariable*	m_hTransferMat;
	
	ID3DX11EffectVectorVariable*	m_hHairColor;
	ID3DX11EffectScalarVariable*	m_hHairWidth;

	ID3DX11EffectScalarVariable*	m_hTangentTexScaleX;
	ID3DX11EffectScalarVariable*	m_hTangentTexScaleY;

	ID3DX11EffectScalarVariable*	m_hTangentTexWeight;
	ID3DX11EffectScalarVariable*	m_hNormalGradWeight;
	ID3DX11EffectScalarVariable*	m_hStrandRandWeight;
	ID3DX11EffectScalarVariable*	m_hForceOpaque;

	ID3DX11EffectScalarVariable*	m_hKa;
	ID3DX11EffectScalarVariable*	m_hKd;
	ID3DX11EffectScalarVariable*	m_hKr;

	ID3DX11EffectShaderResourceVariable*	m_hTexHairNoise;
	ID3DX11EffectShaderResourceVariable*	m_hTexNormalGradient;

	ID3DX11EffectShaderResourceVariable*	m_hMarschner1;
	ID3DX11EffectShaderResourceVariable*	m_hMarschner2;

	ID3D11InputLayout*				m_pVertexLayout;

	ID3D11ShaderResourceView*		m_pTexHairNoiseSRV;
	ID3D11ShaderResourceView*		m_pTexNormalGradientSRV;
	ID3D11ShaderResourceView*		m_pTexTipNoiseSRV;

	ID3D11Texture2D*				m_pMarschnerTex1;
	ID3D11Texture2D*				m_pMarschnerTex2;

	ID3D11ShaderResourceView*		m_pMarschnerSRV1;
	ID3D11ShaderResourceView*		m_pMarschnerSRV2;

	HairRenderMode	m_renderMode;

	XMFLOAT4		m_hairColor;

	float			m_tangentTexScaleX;
	float			m_tangentTexScaleY;
	
	float			m_tangentTexWeight;
	float			m_normalGradWeight;
	float			m_strandRandWeight;
	float			m_forceOpaque;

	float			m_ka;
	float			m_kd;
	float			m_kr;

	XMFLOAT4X4		m_transferMat;
};

