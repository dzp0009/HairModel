#pragma once

#include <vector>
#include <list>

#include "QDXObject.h"

#include "MorphController.h"


// uniform sampled strand vertex count
#define NUM_UNISAM_VERTICES		48

// A single vertex in a strand
struct StrandVertex
{
	XMFLOAT3	position;		// pos 0
	XMFLOAT3	tangent;		// pos 1
	XMFLOAT4	color;			// color 0
	XMFLOAT2	texcoord;
	XMFLOAT4	shading;		// color 1

// for N-way morphing
#if MAX_NUM_MORPH_SRC > 2
	XMFLOAT3	pos2;			
	XMFLOAT4	color2;
#endif
#if MAX_NUM_MORPH_SRC > 3
	XMFLOAT3	pos3;
	XMFLOAT4	color3;
#endif
#if MAX_NUM_MORPH_SRC > 4
	XMFLOAT3	pos4;
	XMFLOAT4	color4;
#endif

	static const D3D11_INPUT_ELEMENT_DESC	s_elemDesc[];
	static const int						s_numElem;
};


struct HairFilterParam
{
	float		sigmaDepth;
};


struct HairGeoNoiseParam
{
	float		frequency;
	float		amplitude;
};


// A single hair strand
class Strand
{
public:
	Strand();

	void				createEmpty(int numVerts);

	void				appendVertex(const StrandVertex& vertex, float segLen = -1.0f);
	void				prependVertex(const StrandVertex& vertex, float segLen = -1.0f);

	void				reserve(int capacity) { m_vertices.reserve(capacity); }

	void				clear()			 { m_vertices.clear(); m_length = 0; }

	StrandVertex*		vertices()		 { return m_vertices.data(); }
	const StrandVertex* vertices() const { return m_vertices.data(); }
	int					numVertices() const { return m_vertices.size(); }
	
	float				length() const	 { return m_length; }
	float				updateLength();

	void				resample(int nSamples);

	int					clusterID() const { return m_clusterId; }
	void				setClusterID(int id) { m_clusterId = id; }

	float				weight() const { return m_weight; }
	void				setWeight(float w) { m_weight = w; }

	void				setColor(const XMFLOAT4& color);

	void				trim(int vId);

	void				calcReferenceFrames();

private:

	void	calcFrameByRotate(const XMFLOAT3& srcTangent, const XMFLOAT3& srcNormal, const XMFLOAT3& srcBinormal,
							  const XMFLOAT3& dstTangent, XMFLOAT3& dstNormal, XMFLOAT3& dstBinormal);

	void	calcMinRotation(const XMFLOAT3& vec1, const XMFLOAT3& vec2, XMFLOAT4X4& mat);

	std::vector<StrandVertex>	m_vertices;
	float				m_length;

	int					m_clusterId;
	float				m_weight;		// used for EMD
};


// Full hair model with many hair strands
class HairStrandModel : public QDXObject
{
public:
	HairStrandModel();
	~HairStrandModel();

	HairStrandModel(const HairStrandModel&);
	HairStrandModel& operator=(const HairStrandModel&);

	void	createEmpty(int numStrands);
	void	createEmptyUnisam(int numStrands, int vertsPerStrand);

	bool	load(QString filename, bool updateBuffs = true);
	bool	save(QString filename);

	bool	loadGeometry(QString filename);
	bool	saveGeometry(QString filename);

	void	release();
	void	render();

	void	addStrand(const Strand& strand);

	int		numStrands() const { return m_strands.size(); }

	void	reserve(int capacity) { m_strands.reserve(capacity); }

	bool	isEmpty() const { return m_strands.empty(); }

	void	clear();

	Strand*	getStrandAt(int i) { return &(m_strands[i]); }
	const Strand* getStrandAt(int i) const { return &(m_strands[i]); }

	void	filterDepth(const HairFilterParam& params);

	void	reverseOrder();
	
	void	filterColorAlpha(const HairFilterParam& params);
	void	filterGeometry(const HairFilterParam& params);
	void	addGeometryNoise(const HairGeoNoiseParam& params);

	void	setStrandWidth(float width) { m_strandWidth = width; }
	float	strandWidth() const { return m_strandWidth; }

	void	rectifyVertices(const SourceTransform& transform);

	bool	isUniformSampled(int nVertsPerStrand) const;


	bool	updateBuffers();
	bool	updateDebugBuffers();


	void	calcRootNbrsByANN(int numNbrs);
	void	clearRootNbrs();
	bool	hasNeighborStrandIDs() const { return !m_nbrStrandIDs.empty(); }

	std::vector< std::vector<int> >& neighborStrandIDs() { return m_nbrStrandIDs; }
	const std::vector< std::vector<int> >& neighborStrandIDs() const { return m_nbrStrandIDs; }

protected:

	void	calcTangents();

	std::vector<Strand>	m_strands;

	std::vector< std::vector<int> >	m_nbrStrandIDs;

	ID3D11Buffer*	m_pVertexBuffer;
	ID3D11Buffer*	m_pIndexBuffer;
	uint			m_vertexCount;
	uint			m_indexCount;

	float			m_strandWidth;
};

