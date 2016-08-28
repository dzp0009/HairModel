#pragma once

#include "QDXRenderGroup.h"

#include "MorphDef.h"

class MorphController;

class HeadMorphRenderer : public QDXRenderGroup
{
public:
	HeadMorphRenderer(MorphController* pMorphCtrl);
	~HeadMorphRenderer();

	bool	create(ID3DX11Effect* pEffect);
	void	release();

	void	render();

	bool	loadHeadTexture(int srcIdx, QString fileName);

	void	lock();
	void	unlock();

private:

	ID3DX11EffectTechnique*					m_hTechnique;

	ID3DX11EffectMatrixVariable*			m_hWorld;
	ID3DX11EffectScalarVariable*			m_hMorphWeights;
	ID3DX11EffectScalarVariable*			m_hNumSources;

	ID3D11InputLayout*						m_pVertexLayout;

	ID3DX11EffectShaderResourceVariable*	m_hSrcTextures[MAX_NUM_MORPH_SRC];
	ID3D11ShaderResourceView*				m_pSrcTextures[MAX_NUM_MORPH_SRC];

	MorphController*	m_pMorphCtrl;

	bool				m_isLocked;
	std::vector<float>	m_lockedWeights;
};

