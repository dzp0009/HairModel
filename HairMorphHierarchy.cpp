#include "HairMorphHierarchy.h"

#include <list>

HairMorphHierarchy::HairMorphHierarchy() : m_currLvlIdx(-1)
{
	m_levels.reserve(3);
	m_sources.reserve(MAX_NUM_MORPH_SRC);
	m_pairFlows.reserve(MAX_NUM_MORPH_SRC);

	m_bUseStrandWeights = false;
}


HairMorphHierarchy::~HairMorphHierarchy(void)
{
	release();
}


void HairMorphHierarchy::release()
{
	for (int i = 0; i < numLevels(); i++)
		m_levels[i].release();

	for (int i = 0; i < m_sources.size(); i++)
		m_sources[i].release();
}


void HairMorphHierarchy::render()
{
	if (m_currLvlIdx >= 0 && m_currLvlIdx < numLevels())
		m_levels[m_currLvlIdx].render();
}


void HairMorphHierarchy::clear()
{
	m_sources.clear();
	m_levels.clear();

	m_pairFlows.clear();

	m_currLvlIdx = -1;
}


// Calculate all pairwise EMD flows between sources.
bool HairMorphHierarchy::calcAllSourceFlows(bool useStrandWeights)
{
	printf("Using strand weights: %d\n", useStrandWeights);

	// validation
	if (m_sources.size() < 2)
	{
		printf("ERROR: no enough sources to calculate flows!\n");
		return false;
	}

	const int numLevels = m_sources[0].numLevels();
	for (int i = 1; i < m_sources.size(); i++)
	{
		if (m_sources[i].numLevels() != numLevels)
		{
			printf("ERROR: source levels do not match (%d != %d).\n", m_sources[i].numLevels(), numLevels);
			return false;
		}
	}

	m_pairFlows.resize(m_sources.size() - 1);
	for (int i = 0; i < m_sources.size() - 1; i++)
	{
		const int srcIdx  = i;
		const int numDsts = m_sources.size() - i - 1;

		m_pairFlows[srcIdx].resize(numDsts);
		for (int j = 0; j < numDsts; j++)
		{
			// calculate flows between each pair of sources

			const int dstIdx = srcIdx + 1 + j;

			printf("Calculating flow from %d to %d...\n", srcIdx, dstIdx);

			int lvl = numLevels - 1;
			printf(">>> LEVEL %d <<<\n", lvl);
			HairFlows flows;
			flows.calcFlowsEMD(m_sources[srcIdx].level(lvl), 
								m_sources[dstIdx].level(lvl), m_bUseStrandWeights);
			//flows.calcFlowsNearestRoot(m_sources[srcIdx].level(lvl),
			//							m_sources[dstIdx].level(lvl));

			for (lvl = numLevels - 2; lvl >= 0; lvl--)
			{
				printf(">>> LEVEL %d <<<\n", lvl);

				HairFlows clusFlows = flows;
				flows.calcFlowsClusteredEMD(m_sources[srcIdx].level(lvl),
											m_sources[dstIdx].level(lvl),
											clusFlows, m_bUseStrandWeights);
			}
			m_pairFlows[srcIdx][j] = flows;

			printf("\n");
		}
	}

	return true;
}


HairFlows* HairMorphHierarchy::getFlows(int srcIdx, int dstIdx)
{
	if (srcIdx < 0 || srcIdx >= m_pairFlows.size())
		return NULL;

	int j = dstIdx - srcIdx - 1;
	if (j < 0 || j >= m_pairFlows[srcIdx].size())
		return NULL;

	return &(m_pairFlows[srcIdx][j]);
}


// Generate morphable hair strands based on existing hair flows.
void HairMorphHierarchy::generateStrands()
{
	if (m_sources.size() < 1)
	{
		printf("ERROR: no source hair to generate from!\n");
		return;
	}
	else if (m_sources.size() == 1)
	{
		m_levels.resize(m_sources[0].numLevels());
		for (int i = 0; i < m_sources[0].numLevels(); i++)
		{
			m_levels[i] = m_sources[0].level(i);
			m_levels[i].updateBuffers();
			//m_levels[i].updateDebugBuffers();
		}
	}
	else if (m_sources.size() == 2)
	{
		// 2-way morphing
		HairFlows* pFlows = getFlows(0, 1);
		if (!pFlows || pFlows->isEmpty())
		{
			printf("ERROR: flows from %d to %d do not exist!\n", 0, 1);
			return;
		}
		m_levels.resize(1);
		m_levels[0].clear();
		m_levels[0].createEmptyUnisam(pFlows->numFlows(), NUM_UNISAM_VERTICES);

		printf("Generating %d morphable strands...", pFlows->numFlows());

		for (int iFlow = 0; iFlow < pFlows->numFlows(); iFlow++)
		{
			const int srcSId0 = pFlows->flows()[iFlow].ids[0];
			const int srcSId1 = pFlows->flows()[iFlow].ids[1];

			StrandVertex* srcVerts0 = m_sources[0].level(0).getStrandAt(srcSId0)->vertices();
			StrandVertex* srcVerts1 = m_sources[1].level(0).getStrandAt(srcSId1)->vertices();
			StrandVertex* dstVerts = m_levels[0].getStrandAt(iFlow)->vertices();

			for (int i = 0; i < NUM_UNISAM_VERTICES; i++)
			{
				dstVerts[i] = srcVerts0[i];
				dstVerts[i].tangent = srcVerts1[i].position;
				dstVerts[i].shading = srcVerts1[i].color;
			}
		}
		m_levels[0].updateBuffers();
	}
	else
	{
		// N-way morphing...
		std::vector<NWayFlow> nwFlows;
		genNWayFromTwoWays(nwFlows);

		m_levels.resize(1);
		m_levels[0].clear();
		m_levels[0].createEmptyUnisam(nwFlows.size(), NUM_UNISAM_VERTICES);

		printf("Generating %d morphable strands...", nwFlows.size());

		const int numSources = m_sources.size();
		for (int flowId = 0; flowId < nwFlows.size(); flowId++)
		{
			StrandVertex* vertsSrcs[MAX_NUM_MORPH_SRC];
			for (int srcId = 0; srcId < numSources; srcId++)
			{
				const int strandId = nwFlows[flowId].ids[srcId];
				vertsSrcs[srcId] = m_sources[srcId].level(0).getStrandAt(strandId)->vertices();
			}
			StrandVertex* vertsDst = m_levels[0].getStrandAt(flowId)->vertices();

			for (int i = 0; i < NUM_UNISAM_VERTICES; i++)
			{
				vertsDst[i].position = vertsSrcs[0][i].position;
				vertsDst[i].color    = vertsSrcs[0][i].color;
				vertsDst[i].tangent  = vertsSrcs[1][i].position;
				vertsDst[i].shading  = vertsSrcs[1][i].color;
#if MAX_NUM_MORPH_SRC > 2
				if (numSources > 2)
				{
					vertsDst[i].pos2   = vertsSrcs[2][i].position;
					vertsDst[i].color2 = vertsSrcs[2][i].color;
				}
#endif
#if MAX_NUM_MORPH_SRC > 3
				if (numSources > 3)
				{
					vertsDst[i].pos3   = vertsSrcs[3][i].position;
					vertsDst[i].color3 = vertsSrcs[3][i].color;
				}
#endif
#if MAX_NUM_MORPH_SRC > 4
				if (numSources > 4)
				{
					vertsDst[i].pos4   = vertsSrcs[4][i].position;
					vertsDst[i].color4 = vertsSrcs[4][i].color;
				}
#endif
			}
		}
		m_levels[0].updateBuffers();
	}

	m_currLvlIdx = 0;
}


void HairMorphHierarchy::genNWayFromTwoWays(std::vector<NWayFlow>& nwFlows)
{
	const int numSources = m_sources.size();

	// build a node nbr graph from pairwise flows
	printf("Building node neighborhoods...");
	std::vector<std::vector<GNode> > nodeGroups(numSources);
	for (int i = 0; i < numSources; i++)
		nodeGroups[i].resize(m_sources[i].level(0).numStrands());

	int maxDegree = 0;
	for (int srcIdx = 0; srcIdx < numSources - 1; srcIdx++)
	{
		for (int dstIdx = srcIdx + 1; dstIdx < numSources; dstIdx++)
		{
			HairFlows* hairFlows = getFlows(srcIdx, dstIdx);
			if (!hairFlows)
			{
				printf("ERROR: flows from %d to %d do not exist!\n", srcIdx, dstIdx);
				return;
			}

			for (int iFlow = 0; iFlow < hairFlows->numFlows(); iFlow++)
			{
				const MyFlow<2>& flow = hairFlows->flows()[iFlow];

				// add nbr info and weights to both relevant nodes
				GNodeNbr nbr;
				nbr.groupId = dstIdx;
				nbr.nodeId  = flow.ids[1];
				nbr.weight  = flow.amount;
				nodeGroups[srcIdx][flow.ids[0]].nbrs.push_back(nbr);

				nbr.groupId = srcIdx;
				nbr.nodeId  = flow.ids[0];
				nodeGroups[dstIdx][flow.ids[1]].nbrs.push_back(nbr);

				// statistics
				int degree = nodeGroups[srcIdx][flow.ids[0]].nbrCount();
				if (degree > maxDegree)
					maxDegree = degree;
			}
		}
	}
	// Sort node neighbors by weight
	for (int groupId = 0; groupId < numSources; groupId++)
	{
		for (int nodeId = 0; nodeId < nodeGroups[groupId].size(); nodeId++)
		{
			std::vector<GNodeNbr>& nbrs = nodeGroups[groupId][nodeId].nbrs;
			std::sort(nbrs.begin(), nbrs.end(), &HairMorphHierarchy::CompareNbr);
		}
	}
	// ...
	printf("DONE. (Max degree = %d)\n", maxDegree);
	
	int numPasses = 16;

	NWayFlow emptyFlow;
	for (int i = 0; i < MAX_NUM_MORPH_SRC; i++)
		emptyFlow.ids[i] = -1;
	emptyFlow.amount = 0;

	for (int pass = 0; pass < numPasses; pass++)
	{
		printf("Calculating N-way flows (pass %d)...", pass);
		int numAdded = 0;
		for (int groupId = 0; groupId < numSources; groupId++)
		{
			const int numNodes = nodeGroups[groupId].size();

			for (int nodeId = 0; nodeId < numNodes; nodeId++)
			{
				if (nodeGroups[groupId][nodeId].visitCount > 0)
					continue;

				NWayFlow nwFlow = emptyFlow;
				if (findNWayFlow(nodeGroups, groupId, nodeId, 1, pass, nwFlow))
				{
					for (int i = 0; i < numSources; i++)
						nodeGroups[i][nwFlow.ids[i]].visitCount++;
					nwFlows.push_back(nwFlow);
					numAdded++;
				}
			} // for iNode
		} // for iGroup
		printf("%d added.\n", numAdded);

		if (numAdded == 0)
			break;
	} // for iPass
	printf("%d flows generated.\n", nwFlows.size());
}


bool HairMorphHierarchy::findNWayFlow(std::vector<std::vector<GNode> >& nodeGroups,
									  int groupId, int nodeId, int depth, int pass, 
									  NWayFlow& flow)
{
	GNode& node = nodeGroups[groupId][nodeId];
	if (flow.ids[groupId] != (-1) || node.visitCount > pass)
		return false;

	flow.ids[groupId] = nodeId;

	// check whether the flow is already complete
	if (depth == m_sources.size())
		return true;
	
	for (int i = 0; i < node.nbrCount(); i++)
	{
		GNodeNbr& nbr = node.nbrs[i];
		if (findNWayFlow(nodeGroups, nbr.groupId, nbr.nodeId, depth+1, pass, flow))
			return true;
	}
	return false;
}


void HairMorphHierarchy::testClusterEffect()
{
	if (m_levels.size() < 2)
	{
		printf("BUld hierarchy first!\n");
		return;
	}

	printf("Calculating coordinate frames along cluster centers...\n");

	for (int i = 0; i < m_levels[1].numStrands(); i++)
		m_levels[1].getStrandAt(i)->calcReferenceFrames();

	printf("Representing strands by relative coords...\n");

	for (int sId = 0; sId < m_levels[0].numStrands(); sId++)
	{
		Strand* strand = m_levels[0].getStrandAt(sId);
		if (strand->clusterID() < 0)
		{
			printf("*");
			continue;
		}

		float theta = 0.5f;
		float phase = 1.0f;
		float period = 0.008f;
		float magnitude = 20.0f;

		Strand* cluster = m_levels[1].getStrandAt(strand->clusterID());

		// calculate root offset on cluster's ref frame


		float curveLen = 0;
		for (int vId = 1; vId < strand->numVertices(); vId++)
		{
			XMFLOAT3& pos      = strand->vertices()[vId].position;
			XMFLOAT3& refPos   = cluster->vertices()[vId].position;
			XMFLOAT3& tangent  = cluster->vertices()[vId].pos2;
			XMFLOAT3& normal   = cluster->vertices()[vId].pos3;
			XMFLOAT3& binormal = cluster->vertices()[vId].pos4;

			if (sId == 100)
			{
				printf("NORMAL: %f %f %f\n", normal.x, normal.y, normal.z);
			}

			float dx = pos.x - strand->vertices()[vId-1].position.x;
			float dy = pos.y - strand->vertices()[vId-1].position.y;
			float dz = pos.z - strand->vertices()[vId-1].position.z;

			float segLen = sqrtf(dx*dx + dy*dy + dz*dz);
			curveLen += segLen;

			float scale = 1.0f;//expf(-vId*0.05f);

			// concentrating...
			pos.x = (pos.x - refPos.x)*scale + refPos.x;
			pos.y = (pos.y - refPos.y)*scale + refPos.y;
			pos.z = (pos.z - refPos.z)*scale + refPos.z;

			// target location on reference frame
			float offset = magnitude * sinf(curveLen*period + phase);
			XMFLOAT3 target;
			target.x = refPos.x + normal.x*offset;
			target.y = refPos.y + normal.y*offset;
			target.z = refPos.z + normal.z*offset;

			// final pos is averaged with target
			float tw = std::max(0.0f, std::min(1.0f, (curveLen - 100.0f) / 100.0f));
			pos.x = (1.0f - tw)*pos.x + tw*target.x;
			pos.y = (1.0f - tw)*pos.y + tw*target.y;
			pos.z = (1.0f - tw)*pos.z + tw*target.z;
		}
	}

	m_levels[0].updateBuffers();
}


// Test Screen-Space Strand effect
void HairMorphHierarchy::testSSSEffect()
{
	if (m_levels[0].isEmpty())
		return;

	std::vector<XMFLOAT3> newVertices(NUM_UNISAM_VERTICES);
	for (int i = 0; i < m_levels[0].numStrands(); i++)
	{
		Strand* strand = m_levels[0].getStrandAt(i);
		StrandVertex* vertices = strand->vertices();

		float curveLen = 0;
		for (int vId = 1; vId < strand->numVertices(); vId++)
		{
			// calculate current curve length
			XMFLOAT3& pos = strand->vertices()[vId].position;

			float dx = pos.x - strand->vertices()[vId-1].position.x;
			float dy = pos.y - strand->vertices()[vId-1].position.y;
			float dz = pos.z - strand->vertices()[vId-1].position.z;

			float len = sqrtf(dx*dx + dy*dy + dz*dz);
			curveLen += len;

			// screen-sapce tangent/normal direction
			float normX = -dy;
			float normY = dx;
			float normLen = sqrtf(normX*normX + normY*normY);
			if (normLen < 0.00001f)
				continue;

			normX /= normLen;
			normY /= normLen;

			// offset vertex
			float period = (curveLen / 200.0f) * 0.02f;
			float magnitude = 0.0f;
			if (curveLen > 50.0f)
			{
				if (curveLen < 150.0f)
				{
					magnitude = 0.5f*(1.0f - cosf(3.1415927f*(curveLen - 50.0f)/100.0f));
				}
				else
				{
					magnitude = 1.0f;
				}
			}
			float sinTerm = 5.0f * magnitude * sinf(curveLen * period);

			float offset = sinTerm;

			newVertices[vId].x = pos.x + offset*normX;
			newVertices[vId].y = pos.y + offset*normY;
			newVertices[vId].z = pos.z;
		}

		// update vertices
		for (int vId = 1; vId < strand->numVertices(); vId++)
			vertices[vId].position = newVertices[vId];
	}
	m_levels[0].updateBuffers();
}