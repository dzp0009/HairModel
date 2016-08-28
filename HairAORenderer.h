#pragma once

#include "QDXRenderGroup.h"

class HairAORenderer : public QDXRenderGroup
{
public:
	HairAORenderer();
	~HairAORenderer();

	bool	create(ID3DX11Effect* pEffect);
	void	release();

	void	render();

	void	resetTransferMat();
	void	setTransferMat(float* pMat);

private:

	ID3DX11EffectTechnique*			m_hTechnique;

	ID3DX11EffectMatrixVariable*	m_hWorld;
	ID3DX11EffectMatrixVariable*	m_hTransferMat;

	ID3DX11EffectVectorVariable*	m_hHairColor;
	ID3DX11EffectScalarVariable*	m_hHairWidth;

	ID3D11InputLayout*				m_pVertexLayout;

	XMFLOAT4		m_hairColor;
	XMFLOAT4X4		m_transferMat;
};

