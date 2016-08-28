#pragma once

#include "QDXObject.h"
#include "HairStrandModel.h"

#include <vector>

class HairHierarchy : public QDXObject
{
public:
	HairHierarchy();
	~HairHierarchy();

	bool	load(QString filename, bool updateBuffers);

	int		numLevels() const					{ return m_levels.size(); }
	
	int		currentLevelIndex() const			{ return m_currLvlIdx; }
	void	setCurrentLevelIndex(int i)			{ if (i >= 0 && i < numLevels()) m_currLvlIdx = i; }

	HairStrandModel&		level(int i)		{ return m_levels[i]; }
	HairStrandModel&		currentLevel()		{ return m_levels[m_currLvlIdx]; }

	const HairStrandModel&	level(int i) const	{ return m_levels[i]; }
	const HairStrandModel&	currentLevel() const{ return m_levels[m_currLvlIdx]; }

	bool	isEmpty() const { return m_levels.empty(); }

	void	release();
	void	render();

	void	clear();

	// methods to build pyramid
	
	void	buildByFixedK(std::vector<int> levelSizes);

	void	calcStrandWeights();

private:

	std::vector<HairStrandModel>	m_levels;

	int		m_currLvlIdx;
};

