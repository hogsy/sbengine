#include "PCH.h"

#include "MRTC.h"
#include "MCC.h"
#include "XRNavGraph.h"
#include "../MSystem/MSystem.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_NavGraph (Static data)
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT_DYNAMIC(CXR_NavGraph, CReferenceCount);
MRTC_IMPLEMENT_DYNAMIC(CXR_NavGraph_Writer, CXR_NavGraph);

//Set max size of hash and dimensions of boxes and initialize all elements
CProximityHash::CProximityHash(int _iSize, const CVec3Dint16& _Dims, const CVec3Dint16& _WorldDims)
{
	MAUTOSTRIP(CProximityHash_ctor, MAUTOSTRIP_VOID);
	//Set element size
	m_lHashElts.SetLen(_iSize);
	
	//Set dimensions
	m_iXDim = _Dims[0];
	m_iYDim = _Dims[1];
	m_iZDim = _Dims[2];

	//Calculate number of boxes in each dimension
	int nXBoxes = (_WorldDims[0] / _Dims[0]) + ((_WorldDims[0] % _Dims[0]) ? 1 : 0);
	int nYBoxes = (_WorldDims[1] / _Dims[1]) + ((_WorldDims[1] % _Dims[1]) ? 1 : 0);
	int nZBoxes = (_WorldDims[2] / _Dims[2]) + ((_WorldDims[2] % _Dims[2]) ? 1 : 0);

	//Set hash size
	m_liHash.SetLen(nXBoxes * nYBoxes * nZBoxes);
	for (int i = 0; i < m_liHash.Len(); i++)
		m_liHash[i] = -1;

	//Set dimension factors
	m_iYZ = nYBoxes * nZBoxes;
	m_iZ = nZBoxes;

	//Reset iterator and first empty index
	m_iNextElt = -1;
	m_iFirstEmpty = (_iSize > 0) ? 0 : -1;
};


//Get hash list index given position
int16 CProximityHash::HashValue(const CVec3Dint16& _Pos)
{
	MAUTOSTRIP(CProximityHash_HashValue, 0);
	return (_Pos[0] / m_iXDim) * m_iYZ + (_Pos[1] / m_iYDim) * m_iZ + (_Pos[2] / m_iZDim);
};


//Add node with given index at given position. Fails if position is invalid or hash is full
bool CProximityHash::Add(int16 _iIndex, const CVec3Dint16& _Pos)
{
	MAUTOSTRIP(CProximityHash_Add, false);
	//Check if element array is full
	if (m_iFirstEmpty >= m_lHashElts.Len())
		return false;

	//Check if position is valid
	int iHash = HashValue(_Pos);	
	if (!m_liHash.ValidPos(iHash))
		return false;

	//Add node index to element list
	m_lHashElts[m_iFirstEmpty].m_iIndex = _iIndex;		

	//Insert node into hash list
	m_lHashElts[m_iFirstEmpty].m_iNext = m_liHash[iHash];		
	m_liHash[iHash] = m_iFirstEmpty;

	m_iFirstEmpty++;
	return true;
};


//Get first node index from box which encapsules given position. Returns -1 if there is no nodes im box.
int16 CProximityHash::GetFirst(const CVec3Dint16& _Pos)
{
	MAUTOSTRIP(CProximityHash_GetFirst, 0);
	int iHash = HashValue(_Pos);
	
	//Valid hash value?
	if (m_liHash.ValidPos(iHash))
	{
		m_iNextElt = m_liHash[iHash];
		return GetNext();
	}
	else
	{
		m_iNextElt = -1;
		return -1;
	}
};

//Get next node index (assuming GetFirst have been called). Returns -1 if there is no more nodes im box.
int16 CProximityHash::GetNext()
{
	MAUTOSTRIP(CProximityHash_GetNext, 0);
	if (m_iNextElt == -1)
	{
		//No more elements
		return -1;
	}
	else
	{
		int16 iRes = m_lHashElts[m_iNextElt].m_iIndex;
		m_iNextElt = m_lHashElts[m_iNextElt].m_iNext;
		return iRes;
	}
};


CXR_NavGraph_Node::CXR_NavGraph_Node()
{
	MAUTOSTRIP(CXR_NavGraph_Node_ctor, MAUTOSTRIP_VOID);
	m_iiEdges = 65535;	
	m_nEdges = 0;
	m_HeightLevels = 0;
	m_Flags = 0;
};

#ifndef CPU_LITTLEENDIAN
void CXR_NavGraph_Node::SwapLE()
{
	::SwapLE(m_iiEdges);
	m_GridPos.SwapLE();
}
#endif

//Backwards compatibility copy methods (sets missing members to default values)
void CXR_NavGraph_Node::BCCopy(const CXR_NavGraph_Node_Version0& _OldNode)
{
	m_iiEdges = _OldNode.m_iiEdges;
	m_nEdges = _OldNode.m_nEdges;
	m_GridPos = _OldNode.m_GridPos;
	m_HeightLevels = 0;
	m_Flags	= FLAGS_ISEDGEDESTINATION; //Although this might be untrue, it's safer to assume this than the reverse
};



const int CXR_NavGraph_Edge::COST_FACTOR = 64;
const int CXR_NavGraph_Edge::MAX_COST = (1 << 6) - 1;
const int CXR_NavGraph_Edge::DISTANCE_FACTOR = 4;

CXR_NavGraph_Edge::CXR_NavGraph_Edge()
{
	MAUTOSTRIP(CXR_NavGraph_Edge_ctor, MAUTOSTRIP_VOID);
	m_iFrom = 65535;	
	m_iTo = 65535;	
};


#ifndef CPU_LITTLEENDIAN
void CXR_NavGraph_Edge::SwapLE()
{
	::SwapLE(m_iFrom);
	::SwapLE(m_iTo);
	::SwapLE(m_Cost);
	::SwapLE(m_Sizes);
}
#endif


//Backwards compatibility copy methods (sets missing members to default values)
void CXR_NavGraph_Edge::BCCopy(const CXR_NavGraph_Edge_Version0& _OldEdge)
{
	m_iFrom = _OldEdge.m_iFrom;
	m_iTo = _OldEdge.m_iTo;
	m_Cost = _OldEdge.m_Cost;
	m_Sizes = (1 << CXR_NavGraph::SIZE_DEFAULT);
};



//CXR_NavGraph/////////////////////////////////////////////////////////////////////////////////////////
CXR_NavGraph::CXR_NavGraph()
{
	MAUTOSTRIP(CXR_NavGraph_ctor, MAUTOSTRIP_VOID);
	m_MinDist = 1;
	m_SupportedSizes = (1 << SIZE_DEFAULT);
};


//Copy constructor (does not copy hash)
CXR_NavGraph::CXR_NavGraph(CXR_NavGraph * _pGraph)
{
	MAUTOSTRIP(CXR_NavGraph_ctor_2, MAUTOSTRIP_VOID);
	if (!_pGraph)
		return;

	m_lNodes.SetLen(_pGraph->m_lNodes.Len());
	m_lEdges.SetLen(_pGraph->m_lEdges.Len());

	//Copy nodes
	int i;
	for (i = 0; i < m_lNodes.Len(); i++)
	{
		m_lNodes[i].m_GridPos = _pGraph->m_lNodes[i].m_GridPos;
		m_lNodes[i].m_iiEdges = _pGraph->m_lNodes[i].m_iiEdges;
		m_lNodes[i].m_nEdges = _pGraph->m_lNodes[i].m_nEdges;
		m_lNodes[i].m_HeightLevels = _pGraph->m_lNodes[i].m_HeightLevels;
		m_lNodes[i].m_Flags = _pGraph->m_lNodes[i].m_Flags;
	};

	//Copy edges
	for (i = 0; i < m_lEdges.Len(); i++)
	{
		m_lEdges[i].m_iFrom = _pGraph->m_lEdges[i].m_iFrom;
		m_lEdges[i].m_iTo = _pGraph->m_lEdges[i].m_iTo;
		m_lEdges[i].m_Cost = _pGraph->m_lEdges[i].m_Cost;
		m_lEdges[i].m_Sizes = _pGraph->m_lEdges[i].m_Sizes;
	}

	m_MinDist = _pGraph->m_MinDist;	
	m_SupportedSizes = _pGraph->m_SupportedSizes;
};


int CXR_NavGraph::GetMinDistance()
{
	MAUTOSTRIP(CXR_NavGraph_GetMinDistance, 0);
	return m_MinDist;
};

int CXR_NavGraph::GetSupportedSizes()
{
	MAUTOSTRIP(CXR_NavGraph_GetSupportedSizes, 0);
	return m_SupportedSizes;
};



void CXR_NavGraph::Read(CDataFile* _pDFile)
{
	MAUTOSTRIP(CXR_NavGraph_Read, MAUTOSTRIP_VOID);
	//VERSION
	int16 Version;
	CStr sNext = _pDFile->GetNext();
	{
		if (sNext == CStr("NAVGRAPH_VERSION")) 
		{
			//Get version
			_pDFile->GetFile()->ReadLE(Version);
		}
		else
		{
			ConOutL("(CXR_NavGraph::Read) Non-version tagged version. Size info etc won't be set");
			Version = VERSION0;
		}
	}
	//GENERAL DATA
	{
		if ((sNext != CStr("NAVGRAPH_DATA")) && !_pDFile->GetNext("NAVGRAPH_DATA")) 
		{
			ConOutL("(CXR_NavGraph::Read) ERROR Failed to read .xng, no NAVGRAPH_DATA entry");
			return;
		}
		_pDFile->GetFile()->ReadLE(m_MinDist);
		switch (Version)
		{
		case VERSION1:
			_pDFile->GetFile()->ReadLE(m_SupportedSizes);
			break;
		case VERSION0:
			m_SupportedSizes = (1 << SIZE_DEFAULT);
			break;
		}
	}
	//NODES
	{
		if (!_pDFile->GetNext("NAVGRAPH_NODES")) 
		{
			ConOutL("(CXR_NavGraph::Read) ERROR Failed to read .xng, no NAVGRAPH_NODES entry");
			return;
		}
		switch (Version)
		{
		case VERSION1:
			{
				int nLen = _pDFile->GetUserData();
				m_lNodes.SetLen(nLen); 
				_pDFile->GetFile()->ReadLE((int8 *)(m_lNodes.GetBasePtr()), sizeof(CXR_NavGraph_Node) * nLen);
#ifndef CPU_LITTLEENDIAN
				for (int i=0; i<nLen; i++)
					m_lNodes[i].SwapLE();
#endif
			}
			break;
		case VERSION0:
			{
				//Must use special node class to read to temporary list, then copy to actual node list
				int nLen = _pDFile->GetUserData();
				TThinArray<CXR_NavGraph_Node_Version0> lTempNodes;
				lTempNodes.SetLen(nLen); 
				_pDFile->GetFile()->ReadLE((int8 *)(lTempNodes.GetBasePtr()), sizeof(CXR_NavGraph_Node_Version0) * nLen);

				//SwapLE and copy to actual node list
				m_lNodes.SetLen(nLen);
				for (int i=0; i<nLen; i++)
				{
#ifndef CPU_LITTLEENDIAN
					lTempNodes[i].SwapLE();
#endif
					m_lNodes[i].BCCopy(lTempNodes[i]);
				}
			}
			break;
		}
	}
	//EDGES
	{
		if (!_pDFile->GetNext("NAVGRAPH_EDGES")) 
		{
			ConOutL("(CXR_NavGraph::Read) ERROR Failed to read .xng, no NAVGRAPH_EDGES entry");
			return;
		}
		switch (Version)
		{
		case VERSION1:
			{
				int nLen = _pDFile->GetUserData();
				m_lEdges.SetLen(nLen); 
				_pDFile->GetFile()->ReadLE((int8 *)(m_lEdges.GetBasePtr()), sizeof(CXR_NavGraph_Edge) * nLen);
#ifndef CPU_LITTLEENDIAN
				for (int i=0; i<nLen; i++)
					m_lEdges[i].SwapLE();
#endif
			}
			break;
		case VERSION0:
			{
				//Must use special egde class to read to temporary list, then copy to actual edge list
				int nLen = _pDFile->GetUserData();
				TThinArray<CXR_NavGraph_Edge_Version0> lTempEdges;
				lTempEdges.SetLen(nLen); 
				_pDFile->GetFile()->ReadLE((int8 *)(lTempEdges.GetBasePtr()), sizeof(CXR_NavGraph_Edge_Version0) * nLen);

				//SwapLE and copy to actual edge list
				m_lEdges.SetLen(nLen);
				for (int i=0; i<nLen; i++)
				{
#ifndef CPU_LITTLEENDIAN
					lTempEdges[i].SwapLE();
#endif
					m_lEdges[i].BCCopy(lTempEdges[i]);
				}
			}
			break;
		}
	}
};

void CXR_NavGraph::BuildHash(const CVec3Dint16& _Dims, const CVec3Dint16& _WorldDims)
{
	MAUTOSTRIP(CXR_NavGraph_BuildHash, MAUTOSTRIP_VOID);
	m_NodeHash = CProximityHash(m_lNodes.Len(), _Dims, _WorldDims);
	for (int i = 0; i < m_lNodes.Len(); i++)
	{
		m_NodeHash.Add(i, m_lNodes[i].m_GridPos);
	}
};


//Size group handling stuff

//Map for <Size group> -> (<basesize>, <height>). <Basesize> is "square radius" in navgrid cells (i.e. a size 3x3 is 
//square radius 1, 5x5 -> 2 etc.), <Height> is minimum (practical) movement height (i.e. crouching for a normal character)
const int CXR_NavGraph::ms_lSizeGroupSize[NUM_SIZEGROUPS][2] = 
{
	{1, 4},		//SIZE_DEFAULT
	{3, 12},	//SIZE_MEDIUMHIGH
	{4, 4},		//SIZE_MEDIUMLOW
	{8, 12},	//SIZE_LARGE
};

//Size group names
const char * CXR_NavGraph::ms_lTranslateSizeGroup[] = 
{
	"Default",
	"MediumHigh",
	"MediumLow",
	"Large",
	NULL,
};

//Check if first given size group is bigger or equally big in all dimensions than second size group
bool CXR_NavGraph::SizeGroupSubsumes(int _SizeGroup1, int _SizeGroup2)
{
	return ((ms_lSizeGroupSize[_SizeGroup1][0] >= ms_lSizeGroupSize[_SizeGroup2][0]) &&
			(ms_lSizeGroupSize[_SizeGroup1][1] >= ms_lSizeGroupSize[_SizeGroup2][1]));
};


//Square radius in navgrid cells of given size group (i.e. 3x3 base -> square radius 1, 5x5 -> 2 etc)
int CXR_NavGraph::GetBaseSize(int _iSizeGroup)
{
	return ms_lSizeGroupSize[_iSizeGroup][0];
};

//Get minimum practical movement height (i.e. crouch for most characters) in navgrid cells for given size group
int CXR_NavGraph::GetHeight(int _iSizeGroup)
{
	return ms_lSizeGroupSize[_iSizeGroup][1];
};




//CXR_NavGraph_Writer///////////////////////////////////////////////////////////////////////////////////

CXR_NavGraph_Writer::CXR_NavGraph_Writer()
	: CXR_NavGraph()
{
};


CXR_NavGraph_Writer::CXR_NavGraph_Writer(CXR_NavGraph * _pGraph)
	: CXR_NavGraph(_pGraph)
{
}

//JK-NOTE: Gnu compiler requires destructor or it will complain at link-time
CXR_NavGraph_Writer::~CXR_NavGraph_Writer()
{
	MAUTOSTRIP(CXR_NavGraph_Writer_dtor, MAUTOSTRIP_VOID);
}


//Write navgraph to file
void CXR_NavGraph_Writer::Write(CDataFile* _pDFile)
{
	MAUTOSTRIP(CXR_NavGraph_Writer_Write, MAUTOSTRIP_VOID);
	//VERSION
	{
		_pDFile->BeginEntry("NAVGRAPH_VERSION");
		_pDFile->GetFile()->WriteLE((int16)VERSION1);
		_pDFile->EndEntry(0);
	}
	//GENERAL DATA
	{
		_pDFile->BeginEntry("NAVGRAPH_DATA");
		_pDFile->GetFile()->WriteLE(m_MinDist);
		_pDFile->GetFile()->WriteLE(m_SupportedSizes);
		_pDFile->EndEntry(0);
	}
	// NODES
	{
		_pDFile->BeginEntry("NAVGRAPH_NODES");
		_pDFile->GetFile()->WriteLE((int8*)(m_lNodes.GetBasePtr()), sizeof(CXR_NavGraph_Node) * m_lNodes.Len());
		_pDFile->EndEntry(m_lNodes.Len());
	}

	// EDGES
	{
		_pDFile->BeginEntry("NAVGRAPH_EDGES");
		_pDFile->GetFile()->WriteLE((int8*)(m_lEdges.GetBasePtr()), sizeof(CXR_NavGraph_Edge) * m_lEdges.Len());
		_pDFile->EndEntry(m_lEdges.Len());
	}
};




//Navgraph node and edge old versions, for backwards compatibility when reading old graphs
//Nodes and edges are stored as raw data.

//Version 0 (original)///////////////////////////////////////////////////////////////////////////////
CXR_NavGraph_Node_Version0::CXR_NavGraph_Node_Version0()
{
	m_iiEdges = 65535;	
	m_nEdges = 0;
};
#ifndef CPU_LITTLEENDIAN
void CXR_NavGraph_Node_Version0::SwapLE()
{
	::SwapLE(m_iiEdges);
	m_GridPos.SwapLE();
}
#endif

CXR_NavGraph_Edge_Version0::CXR_NavGraph_Edge_Version0()
{
	m_iFrom = 65535;	
	m_iTo = 65535;	
};
#ifndef CPU_LITTLEENDIAN
void CXR_NavGraph_Edge_Version0::SwapLE()
{
	::SwapLE(m_iFrom);
	::SwapLE(m_iTo);
	::SwapLE(m_Cost);
}
#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_NavGraph_Instance (Dynamic data, game-instance specific, static link/tile dependencies)
|__________________________________________________________________________________________________
\*************************************************************************************************/


//CXR_NavGraph_TileLinkStateDependency/////////////////////////////////////////

int CXR_NavGraph_TileEdgeStateDependency::operator=(int _i)
{
	MAUTOSTRIP(CXR_NavGraph_TileEdgeStateDependency_operator_assign, 0);
	m_TilePos = _i;
	m_iiEdgeStates = _i;
	m_nEdgeStates = _i;
	return _i;
};


