#ifndef _INC_XRNavGraph
#define _INC_XRNavGraph

#include "MRTC.h"
#include "MDA.h"
#include "MMath.h"
#include "MDA_Hash.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_NavGraph (Static data)
|__________________________________________________________________________________________________
\*************************************************************************************************/

//Hash which sorts navgraph nodes into box-partitioned 3D-space. 
//I'll make a template of this later perhaps.
class CProximityHash
{
private:
	class CHashElt
	{
	public:
		int16 m_iIndex;
		int16 m_iNext;
		
		CHashElt()
		{
			m_iIndex = -1;
			m_iNext = -1;
		};
	};

	//All elements in the hash
	TThinArray<CHashElt> m_lHashElts;

	//Index of first "empty" element in element list
	int16 m_iFirstEmpty;
	
	//Iteration hashelt index, when getting nodes of a box
	int16 m_iNextElt;

	//The dimensions of a hash box
	int m_iXDim;
	int m_iYDim;
	int m_iZDim;

	//The hash list (each slot holds the index to the first belonging element)
	TThinArray<int16> m_liHash;

	//Dimension factors: position (x,y,z) has hash value x * m_iYZ + y * m_iZ + z
	int m_iYZ;
	int m_iZ;

	//Get hash list index given position
	int16 HashValue(const CVec3Dint16& _Pos);

public:
	CProximityHash(){};

	//Set max size of hash, dimensions of boxes and dimensions of world. Initialize all elements
	CProximityHash(int _iSize, const CVec3Dint16& _Dims, const CVec3Dint16& _WorldDims);

	//Add node with given index at given position. Fails if position is invalid or hash is full
	bool Add(int16 _iIndex, const CVec3Dint16& _Pos);

	//Get first node index from box which encapsules given position. Returns -1 if there is no nodes im box.
	int16 GetFirst(const CVec3Dint16& _Pos);

	//Get next node index (assuming GetFirst have been called). Returns -1 if there is no more nodes im box.
	int16 GetNext();
};


class CXR_NavGraph_Node_Version0;

class CXR_NavGraph_Node //10 bytes
{
public:
	CXR_NavGraph_Node();

	uint16 m_iiEdges;			// Index of first edge-index in CXR_NavGraph::m_liEdges
	uint8 m_nEdges;				// Number of edges and edge-indices in CXR_NavGraph::m_liEdges following the first edge-index.
	CVec3Dint16 m_GridPos;		// Position in navgrid from which this graph is built
	uint8 m_HeightLevels : 4;	// Number of grid cells above position that can be considered "at" the position, used for large characters in slopes

	enum {
		FLAGS_ISEDGEDESTINATION = 0x1, //Are there any edges leading to this node?
	};
	uint8 m_Flags : 4;

#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif

	//Backwards compatibility copy methods (sets missing members to default values)
	void BCCopy(const CXR_NavGraph_Node_Version0& _OldNode);
};


class CXR_NavGraph_Edge_Version0;

class CXR_NavGraph_Edge //6 bytes
{
public:
	CXR_NavGraph_Edge();

	//Indices into CXR_NavGraph_Node::m_lNodes
	uint16 m_iFrom;		
	uint16 m_iTo;

	//Statically calculated cost of traversing edge
	uint8 m_Cost;

	//Size groups that can use this edge (flags)
	uint8 m_Sizes;

	//m_iCost * COST_FACTOR = approx. navgrid cost of traversing edge. Edge cost is clamped at MAX_COST, though
	static const int COST_FACTOR;
	static const int MAX_COST;

	//m_iCost * DISTANCE_FACTOR = minimum grid length of edge
	static const int DISTANCE_FACTOR;

#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif

	//Backwards compatibility copy methods (sets missing members to default values)
	void BCCopy(const CXR_NavGraph_Edge_Version0& _OldEdge);
};


//Navgraph base class
class CXR_NavGraph : public CReferenceCount
{
	MRTC_DECLARE;

public:
	//Size groups
	enum {
		SIZE_INVALID = -1, 

		SIZE_DEFAULT = 0,	//3x4 cells (24x32 units): Normal characters, small alien etc 
		SIZE_MEDIUMHIGH,	//7x12 cells (56x96 units): Riot guard etc
		SIZE_MEDIUMLOW,		//9x4 (72x32 units): Big alien etc		
		SIZE_LARGE,			//17x12 cells (136x96 units): Heavy guard etc

		NUM_SIZEGROUPS,
	};

	//Versions
	enum {
		VERSION0 = 0,		//Original version (no version info)
		VERSION1 = 0x100,	//Added supported sizes, edge sizes, node flags, node heightlevels
	};

protected:
	//Minimum normal distance between nodes
	int16 m_MinDist;

	//Supported size groups (flags)
	int8 m_SupportedSizes;

public:
	CXR_NavGraph();

	//Copy constructor (does not copy hash)
	CXR_NavGraph(CXR_NavGraph * _pGraph);

	TThinArray<CXR_NavGraph_Node> m_lNodes;
	TThinArray<CXR_NavGraph_Edge> m_lEdges;
	CProximityHash m_NodeHash;

	virtual int GetMinDistance();
	virtual int GetSupportedSizes();

	virtual void Read(CDataFile* _pDFile);
	virtual void BuildHash(const CVec3Dint16& _Dims, const CVec3Dint16& _WorldDims);


//Size group handling stuff
protected:
	//Map for <Size group index> -> (<basesize>, <height>). <Basesize> is "square radius" in navgrid cells (i.e. a size 3x3 is 
	//square radius 1, 5x5 -> 2 etc.), <Height> is minimum (practical) movement height (i.e. crouching for a normal character)
	static const int ms_lSizeGroupSize[NUM_SIZEGROUPS][2];

public:
	//Size group names
	static const char * ms_lTranslateSizeGroup[];

	//Check if first given size group is bigger or equally big in all dimensions than second size group
	static bool SizeGroupSubsumes(int _SizeGroup1, int _SizeGroup2);

	//Square radius in navgrid cells of given size group (i.e. 3x3 base -> square radius 1, 5x5 -> 2 etc)
	static int GetBaseSize(int _iSizeGroup);

	//Get minimum practical movement height (i.e. crouch for most characters) in navgrid cells for given size group
	static int GetHeight(int _iSizeGroup);
};


//Navgraph that can write itself
class CXR_NavGraph_Writer : public CXR_NavGraph
{
	MRTC_DECLARE;
public:
	CXR_NavGraph_Writer();
	~CXR_NavGraph_Writer();
	CXR_NavGraph_Writer(CXR_NavGraph * _pGraph);
	
	//Write navgraph to file
	virtual void Write(CDataFile* _pFile);
};




//Navgraph node and edge old versions, for backwards compatibility when reading old graphs
//Nodes and edges are stored as raw data.

//Version 0 (original)
class CXR_NavGraph_Node_Version0 
{
public:
	CXR_NavGraph_Node_Version0();
	uint16 m_iiEdges;
	uint8 m_nEdges;	
	CVec3Dint16 m_GridPos;
#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif
};
class CXR_NavGraph_Edge_Version0
{
public:
	CXR_NavGraph_Edge_Version0();
	uint16 m_iFrom;		
	uint16 m_iTo;
	uint8 m_Cost;
#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif
};



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_NavGraph_Instance (Dynamic data, game-instance specific, static Edge/tile dependencies)
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CXR_NavGraph_EdgeState		// 1 byte
{
public:
	uint8 m_bValid : 1;
	uint8 m_bTraversable : 1;
	uint8 m_Cost : 6;
};

//NOTE: The below is currently not in use!

class CXR_NavGraph_TileEdgeStateDependency
{
public:
	CVec3Dint16 m_TilePos;
	uint16 m_iiEdgeStates;		// Refering to EdgeStates via indices at m_liEdgeStates[m_iiEdgeStates..m_iiEdgeStates+m_nEdgeStates-1]
	uint8 m_nEdgeStates;

	int operator=(int _i);
};


//REMOVE; just to make below compile... 
class CDummy{};

class CXR_NavGraph_TileEdgeStateDependencyHash : public THash<CXR_NavGraph_TileEdgeStateDependency, TArray<CDummy> >
{
	// bla..
};


class CXR_NavGraph_Instance
{
public:
	TThinArray<CXR_NavGraph_EdgeState> m_lEdgeStates;	// Forward and backward traversability state separated.
														// Each state have two corresponding EdgeStates.
														// EdgeState (i) refer to Edge (i >> 1) and the direction is
														// backwards if (i & 1) != 0

	TThinArray<uint16> m_liEdgeStates;
	CXR_NavGraph_TileEdgeStateDependencyHash m_TileDependencyHash;
};



#endif
