#pragma once

// Maintain morphing parameters/variables such as opacities, transforms etc.

#include "QDXUT.h"

#include "MorphDef.h"

struct SourceTransform
{
	XMFLOAT3	location;
	XMFLOAT3	scale;
	XMFLOAT4	rotation;
	int			imageHeight;
};


class MorphController
{
public:
	MorphController(void);
	~MorphController(void);

	bool	addSourceFromFile(QString coefFileName, int imgHeight);

	const SourceTransform& sourceTransform(int i) const { return m_srcTransforms[i]; }

	void	clear();

	const std::vector<float>& weights() const { return m_weights; }
	void	setWeights(const std::vector<float>& weights);

	bool	isTransformLocked() const { return m_lockTransform; }
	void	setTransformLocked(bool isLocked) { m_lockTransform = isLocked; }

	// access current transforms
	const XMFLOAT4X4*	worldTransform() const { return &m_mWorld; }


private:
	void		updateCurrTransforms();

	std::vector<SourceTransform>	m_srcTransforms;
	std::vector<float>				m_weights;

	// transforms at the current time instance
	XMFLOAT4X4	m_mWorld;

	bool		m_lockTransform;
};

