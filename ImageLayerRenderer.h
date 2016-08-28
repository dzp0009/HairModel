#pragma once

#include "QDXRenderGroup.h"

class ImageLayerRenderer : public QDXRenderGroup
{
public:
	ImageLayerRenderer();
	~ImageLayerRenderer();

	bool	create(ID3DX11Effect* pEffect);
	void	release();

	void	render();

private:
	ID3DX11EffectTechnique*					m_hTechnique;
	ID3DX11EffectMatrixVariable*			m_hWorld;
	ID3DX11EffectShaderResourceVariable*	m_hTexture;
	
	ID3D11InputLayout*	m_pVertexLayout;
};

