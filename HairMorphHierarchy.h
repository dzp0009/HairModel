#pragma once

#include "HairHierarchy.h"
#include "HairFlows.h"

#include <vector>

typedef MyFlow<MAX_NUM_MORPH_SRC> NWayFlow;

// Hierarchy of HairMorphModels
class HairMorphHierarchy : public QDXObject
{
public:
	HairMorphHierarchy();
	~HairMorphHierarchy();

	void	release();
	void	render();

	void	clear();

	// basic accessors

	int		numLevels() const			{ return m_levels.size(); }

	int		currentLevelIndex() const	{ return m_currLvlIdx; }
	void	setCurrentLevelIndex(int i)	{ if (i >= 0 && i < numLevels()) m_currLvlIdx = i; }

	HairStrandModel&		level(int i)	{ return m_levels[i]; }

	bool	isEmpty() const				{ return m_levels[0].isEmpty(); }

	// access source hair models

	std::vector<HairHierarchy>&	sources() { return m_sources; }

	int		numSources() const			{ return m_sources.size(); }

	// calculate EMD flows

	bool	calcAllSourceFlows(bool useStrandWeights);

	// morphable strands generation

	void	generateStrands();

	HairFlows*	getFlows(int srcIdx, int dstIdx);

	void	testClusterEffect();
	void	testSSSEffect();

private:

	struct GNodeNbr
	{
		int		groupId;
		int		nodeId;
		float	weight;

		GNodeNbr() : groupId(-1), nodeId(-1), weight(0) {}
	};

	static bool CompareNbr(GNodeNbr a, GNodeNbr b) { return a.weight > b.weight; }

	struct GNode
	{
		std::vector<GNodeNbr> nbrs;
		int		visitCount;

		GNode() : visitCount(0) {}
		int		nbrCount() const { return nbrs.size(); }
	};


	void	genNWayFromTwoWays(std::vector<NWayFlow>& nwFlows);

	bool	findNWayFlow(std::vector<std::vector<GNode> >& nodeGroups,
						 int groupId, int nodeId, int depth, int pass, NWayFlow& flow);

	// pair-wise flows: m_pairFlows[i][j] is the flow from source i to i+1+j;
	std::vector<std::vector<HairFlows> >	m_pairFlows;

	std::vector<HairHierarchy>		m_sources;
	std::vector<HairStrandModel>	m_levels;

	int		m_currLvlIdx;

	bool	m_bUseStrandWeights;
};

