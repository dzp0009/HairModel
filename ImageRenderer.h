#pragma once

#include "QDXRenderGroup.h"

class ImageRenderer : public QDXRenderGroup
{
public:
	ImageRenderer();
	~ImageRenderer();

	bool	create(ID3DX11Effect* pEffect);
	void	release();

	void	render();

	void	setImageOpacity(float opacity)	{ m_opacity = opacity; }
	float	imageOpacity() const			{ return m_opacity; }

private:
	ID3DX11EffectTechnique*					m_hTechnique;
	ID3DX11EffectMatrixVariable*			m_hWorld;
	ID3DX11EffectScalarVariable*			m_hOpacity;
	ID3DX11EffectShaderResourceVariable*	m_hTexture;
	ID3DX11EffectScalarVariable*			m_hIsSelected;

	ID3D11InputLayout*	m_pVertexLayout;

	float	m_opacity;
};

