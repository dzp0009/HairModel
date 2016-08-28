#pragma once

#include "QDXRenderGroup.h"

class MorphController;

class HairMorphRenderer : public QDXRenderGroup
{
public:
	HairMorphRenderer(MorphController* pMorphCtrl);
	~HairMorphRenderer();

	bool	create(ID3DX11Effect* pEffect);
	void	release();

	void	render();

	void	setStrandRandWeight(float weight);
	void	setHairWidth(float width);

private:

	ID3DX11EffectTechnique*			m_hTechnique;
	ID3DX11EffectMatrixVariable*	m_hWorld;
	ID3DX11EffectScalarVariable*	m_hHairWidth;
	ID3DX11EffectScalarVariable*	m_hStrandRandWeight;

	ID3DX11EffectScalarVariable*	m_hMorphWeights;
	ID3DX11EffectScalarVariable*	m_hNumSources;

	ID3D11InputLayout*				m_pVertexLayout;

	MorphController*				m_pMorphCtrl;

	float							m_strandRandWeight;
	float							m_hairWidth;
};

