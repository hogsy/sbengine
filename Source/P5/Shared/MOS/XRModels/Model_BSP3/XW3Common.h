// -------------------------------------------------------------------
//  The XW Fileformat
// -------------------------------------------------------------------

#ifndef _INC_XW3COMMON
#define _INC_XW3COMMON

#include "../MCC/MCC.h"
#include "../../XR/XRClass.h"
#include "../../XR/Solids/XRThinSolid.h"

#include "../Model_BSP/XWCommon.h"

#define XW_VERSION	0x0124

#define XW_MAXUSERPORTALS 256

//#define MODEL_BSP3_USEKNITSTRIP
#define MODEL_BSP3_MAXKNITSTRIP		253

#ifdef	PLATFORM_PS2
#include "../../RndrPS2/MRndrPS2.h"
#endif

// -------------------------------------------------------------------

/*
	This stuff is probably outdated, use DFB and look in a real XW file instead.

[IMAGELIST]

[BSPMODELS]
	[BSPMODEL]
		//[MODELINFO]
			:
		[PLANES]
		[VERTICES],			CVec3Dfp32
		[VERTEXINDICES],	uint32
		[FACES],			CBSP3_Face
		[FACEINDICES],		uint32
		[BSPNODES],			CBSP3_Node
		[PORTALS],			CBSP3_Portal
		[MAPPINGS],			CXR_PlaneMapping
		[LIGHTMAPSINFO],	CBSP3_LightMapInfo
		[LIGHTMAPS],		-
		[LIGHTVOLUMEINFO],	CBSP3_LightVolumeInfo
		[LIGHTVOLUME],		-
		[LIGHTVERTINFO],	CBSP3_LightVerticeInfo
		[LIGHTVERTICES],	-

		[VISIBILITYTREE]	Canceled!

[OBJECTATTRIB]

*/

/*
Current restrictions:

Max vertices		-
Max faces			-
Max nodes:			65536
Max leaves:			65536
Max planes			65536
Max txt.mappings	65536
Max face verts.		16
Max vis. lists:		-
Max portals			-

Max lightinfo		65536
*/

// -------------------------------------------------------------------
#define XW_FACE_TRANSPARENT 1
#define XW_FACE_VOLUMEINSIDE 2
#define XW_FACE_LIGHTMAP 4
#define XW_FACE_LIGHTVERTICES 8
#define XW_FACE_STRUCTURE 16
#define XW_FACE_SMOOTHNORMALS 32
#define XW_FACE_USERPORTAL 64	// Only user in XWC
#define XW_FACE_PHYSICAL 128
#define XW_FACE_VISIBLE 256
#define XW_FACE_LIGHTMULTIPASS 512
#define XW_FACE_LIGHTBUMPMAP 1024
#define XW_FACE_NAVIGATION 2048

#define XW_FACE_VERSION 0x0203

class CBSP3_CoreFace
{
public:
	int32 m_iiEdges;

	uint32 m_iiVertices:24;	// Index to first vertex-index
	uint32 m_nVertices:8;		// Number vertices

	uint32 m_iMapping;
	uint16 m_iSurface;
	uint16 m_Flags;

	uint32 m_iPlane;
	uint32 m_iiNormals;		// Index to first normal-index
	uint16 m_iBackMedium;	// What's behind the face.
	uint16 m_iFrontMedium;	// What's in front of the face.

	uint32 m_iLightInfo:28;	// Index of first lightmap/lightvertices
	uint32 m_nLightInfo:4;

	CBSP3_CoreFace();
	void Read(CCFile* _pF, int _Version);
	void Write(CCFile* _pF);

#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif
};

// -------------------------------------------------------------------
class CBSP3_Edge
{
public:
	uint32 m_iV[2];		// Start and End vertex for this edge

	CBSP3_Edge() {};

	CBSP3_Edge(int _iv0, int _iv1)
	{
		m_iV[0] = _iv0;
		m_iV[1] = _iv1;
	}

	void Read(CCFile* _pF)
	{
		_pF->ReadLE(m_iV[0]);
		_pF->ReadLE(m_iV[1]);
	}

	void Write(CCFile* _pF)
	{
		_pF->WriteLE(m_iV[0]);
		_pF->WriteLE(m_iV[1]);
	}
};

// -------------------------------------------------------------------
class CBSP3_EdgeFaces
{
public:
	uint32 m_iFaces[2];	// Faces using this edge.  0x80000000 == no face, 0x40000000 == used backwards/flipped

	CBSP3_EdgeFaces()
	{
		m_iFaces[0] = 0x80000000;
		m_iFaces[1] = 0x80000000;
	}

	int IsValid(int _iRef) { return (m_iFaces[_iRef] & 0x80000000) ? 0 : 1; };
	int GetFlip(int _iRef) { return (m_iFaces[_iRef] & 0x40000000) ? 1 : 0; };
	int GetFace(int _iRef) { return m_iFaces[_iRef] & ~(0x40000000); };
	void SetFace(int _iRef, int _iFace, int _bFlip) { m_iFaces[_iRef] = _iFace | ((_bFlip) ? 0x40000000 : 0); };

	void Read(CCFile* _pF)
	{
		_pF->ReadLE(m_iFaces[0]);
		_pF->ReadLE(m_iFaces[1]);
	}

	void Write(CCFile* _pF)
	{
		_pF->WriteLE(m_iFaces[0]);
		_pF->WriteLE(m_iFaces[1]);
	}
};

// -------------------------------------------------------------------
#define XW_NODE_PORTAL 1			// Leaf with either structure or navigation portals.
#define XW_NODE_STRUCTURE 2
#define XW_NODE_ENABLE 4
#define XW_NODE_FOGENABLE 8
//#define XW_NODE_NAVIGATION 4

#define XW_NODE_PLANETYPESHIFT		4
#define XW_NODE_PLANETYPEAND		(0x3 << XW_NODE_PLANETYPESHIFT)
#define XW_NODE_PLANETYPEANDSIGN	(0x7 << XW_NODE_PLANETYPESHIFT)

#define XW_NODE_PLANEANY			(0x0 << XW_NODE_PLANETYPESHIFT)
#define XW_NODE_PLANENEG			(0x4 << XW_NODE_PLANETYPESHIFT)

#define XW_NODE_PLANEX				(0x1 << XW_NODE_PLANETYPESHIFT)
#define XW_NODE_PLANEY				(0x2 << XW_NODE_PLANETYPESHIFT)
#define XW_NODE_PLANEZ				(0x3 << XW_NODE_PLANETYPESHIFT)
#define XW_NODE_PLANEXNEG			(0x5 << XW_NODE_PLANETYPESHIFT)
#define XW_NODE_PLANEYNEG			(0x6 << XW_NODE_PLANETYPESHIFT)
#define XW_NODE_PLANEZNEG			(0x7 << XW_NODE_PLANETYPESHIFT)

#define XW_NODE_VERSION				0x0202


class CBSP3_Node					// 20 bytes
{
public:
	// Ugly!
	union
	{
		uint16 m_iNodeFront;	// Node
		uint16 m_nFaces;		// Leaf
	};
	union
	{
		uint16 m_iNodeBack;		// Node
		uint16 m_iMedium;		// Leaf
	};

	uint16 m_iNodeParent;		// neg. => bug.  0 == Root
	union
	{
		uint32 m_iiFaces;
		struct
		{
			uint16 m_iBoundNodeFront;
			uint16 m_iBoundNodeBack;
		} m_Bound;
	};

	uint32 m_iPlane;			// 0  -> This is a leaf, otherwize a node

	uint16 m_Flags;
	uint16 m_iPortalLeaf;


	inline bool IsNode() const { return (m_iPlane != 0); }
	inline bool IsLeaf() const { return (m_iPlane == 0); }
	inline bool IsStructureLeaf() const { return (m_Flags & (XW_NODE_PORTAL | XW_NODE_STRUCTURE)) == (XW_NODE_PORTAL | XW_NODE_STRUCTURE); }
	inline bool IsNavigationLeaf() const { return false; }

	CBSP3_Node()
	{
		m_iPlane = 0;

		m_iNodeFront = 0;
		m_iNodeBack = 0;

		m_iNodeParent = 0;
		m_iiFaces = 0;
		m_Bound.m_iBoundNodeBack = 0;

		m_Flags = 0;
		m_iPortalLeaf = 0;
	}

	void GetPortalInfo(const CBSP3_Node& _SrcNode)
	{
		m_Flags |= _SrcNode.m_Flags & (XW_NODE_PORTAL | XW_NODE_STRUCTURE /*| XW_NODE_NAVIGATION*/);
		m_iPortalLeaf = _SrcNode.m_iPortalLeaf;
//		m_iNavigationLeaf = _SrcNode.m_iNavigationLeaf;
//		m_nPortals = _SrcNode.m_nPortals;
//		m_iiPortals = _SrcNode.m_iiPortals;
	}

	CStr GetString() const
	{
		MAUTOSTRIP(CBSP3_Node_GetString, CStr());
		return CStrF("iPlane %d, iFront %d, iBack %d, iParent %d, iiF %d, PF %d, iPL %d",
			m_iPlane, m_iNodeFront, m_iNodeBack, m_iNodeParent, m_iiFaces, m_Flags, m_iPortalLeaf);
	}

	void Read(CCFile* _pF, int _Ver);
	void Write(CCFile* _pF) const;

#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif
};

// -------------------------------
#define XW_PORTALLEAF_STRUCTURE		1
#define XW_PORTALLEAF_NAVIGATION	2

#define XW_PORTALLEAF_NUMAMBIENT		8

#define XW_PORTALLEAF_VERSION		0x0201

class CBSP3_PortalLeaf
{
public:
	uint32 m_iNode;				// Corresponding node
	uint32 m_Flags;
	uint16 m_nPortals;			// Number of portals
	uint16 m_iiPortals;			// Index into portal-indices.
	CBox3Dfp32 m_BoundBox;
	uint32 m_ContainsMedium;	// Mediums present in all sub-leaves. 
								// (i.e, if any leaf has water, water will be set here.)
	uint32 m_iMedium;			// Medium of the structure in this leaf.
	uint32 m_iPVS;
	uint32 m_iPHS;
	uint32 m_iOctaAABBNode;		// index into OctaAABB nodes
	uint16 m_PVSBitLen;
	uint16 m_PHSBitLen;
	uint8 m_AmbientVol[XW_PORTALLEAF_NUMAMBIENT];

	uint16 m_iLightVolume;
	uint16 m_nLightVolume;

	CBSP3_PortalLeaf()
	{
		m_iNode = 0;
		m_Flags = 0;
		m_nPortals = 0;
		m_iiPortals = 0;
		m_BoundBox.m_Min = 0;
		m_BoundBox.m_Max = 0;
		m_ContainsMedium = 0;
		m_iMedium = 0;
		m_iPVS = 0;
		m_iPHS = 0;
		m_PVSBitLen = 0;
		m_PHSBitLen = 0;
		m_iLightVolume = 0;
		m_nLightVolume = 0;
		m_iOctaAABBNode = 0;
		FillChar(m_AmbientVol, sizeof(m_AmbientVol), 0);
	}

	void Read(CCFile* _pF, int _bReadVersion);
	void Write(CCFile* _pF, int _bReadVersion) const;
};


#define XW_PORTAL_VERSION 0x0101

// -------------------------------
class CBSP3_Portal : public CXR_IndexedPortal			// 16 bytes.
{
public:
	uint32 m_iNodeFront;
	uint32 m_iNodeBack;
	uint32 m_iPlane;
	uint16 m_iRPortal;		// Used during rendering for temporary portal information.
	uint16 m_iFogPortal;

	CBSP3_Portal();
	void Read(CCFile* _pF, int _Version);
	void Write(CCFile* _pF) const;

/*	int GetEnterFlags(int _iNode)
	{
		if (_iNode == m_iNodeFront)
			return (m_Flags >> XW_PORTAL_ENTERFRONTSHIFT) & XW_PORTAL_ENTERAND;
		else
			return (m_Flags >> XW_PORTAL_ENTERBACKSHIFT) & XW_PORTAL_ENTERAND;
	}*/
};

// -------------------------------
class CBSP3_FogPortalFace
{
public:
	uint32 m_iiVertices;
	uint32 m_nVertices;

	CBSP3_FogPortalFace()
	{
		m_iiVertices = 0;
		m_nVertices = 0;
	}

	void Read(CCFile* _pF);
	void Write(CCFile* _pF) const;
};

// -------------------------------
class CBSP3_FogPortal
{
public:
	int32 m_iVertices;
	int32 m_nVertices;
	int32 m_iFaces;
	int32 m_nFaces;

	CBSP3_FogPortal();
	void Read(CCFile* _pF);
	void Write(CCFile* _pF) const;
};



// -------------------------------
/*class CBSP3_Leaf : public CBSP3_Node
{
public:
	uint32 m_iiFaces;
	uint16 m_nFaces;
	uint16 m_Medium;
	int16 m_iVisList;		// -1 == No vis.list

	CBSP3_Leaf()
	{
		m_iiFaces = 0;
		m_nFaces = 0;
		m_iNodeParent = 0;
		m_iVisList = -1;
	}
};
*/

// -------------------------------
class CBSP3_LightMapInfo
{
public:
/*	int32 m_LightID;
	int32 m_Scale;
	fp32 m_OffsetU;
	fp32 m_OffsetV;
	uint16 m_LMCOffsetU;
	uint16 m_LMCOffsetV;
	uint16 m_iLMC;*/

	uint8 m_Scale;
	uint8 m_LMCOffsetXHalf;
	uint8 m_LMCOffsetYHalf;
	uint8 m_iLMC;
	uint8 m_LMCWidthHalf;
	uint8 m_LMCHeightHalf;

	void Read(CCFile* _pF, int _Version);
	void Write(CCFile* _pF) const;

#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif
};

// -------------------------------
#define XW_LIGHTMAPCONTAINER_VERSION		0x0100

class CBSP3_LightMapContainer : public CReferenceCount
{
public:
	class CBSP3_LMDesc
	{
	public:
		uint8 m_Width;		// i.e, Max width is 255*2 = 510
		uint8 m_Height;
		uint32 m_DataPos;

		int GetWidth() const { return m_Width; };
		int GetHeight() const { return m_Height; };

		void Read(CCFile* _pFile, int _Version);
		void Write(CCFile* _pFile);
	};

	uint32 m_Format;			// Format of all lightmaps in the container.

	TThinArray<CBSP3_LMDesc> m_lLMDesc;
	TThinArray<uint8> m_lLMData;

	CBSP3_LightMapContainer();
	~CBSP3_LightMapContainer();

	void Create(TArray<spCImage> _lspLightMaps, int _Format = IMAGE_FORMAT_BGR8);	// Used in XWC when writing XW:s.
	TArray<spCImage> GetLightMapImages(int _Format = IMAGE_FORMAT_RGB32_F);		// Used in XWC when reading XW:s.

	void GetLightMap(int _iLightMap, CImage& _RefImage);	// Creates a "reference image" in _RefImage, no image data is moved/copied.

	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile);
};

// -------------------------------
class CBSP3_LightVerticesInfo
{
public:
	int32 m_LightID;
	int32 m_iLightVertices;
	int32 m_nLightVertices;
	int32 __m_Padding;

	void Read(CCFile* _pF)
	{
		_pF->ReadLE(m_LightID);
		_pF->ReadLE(m_iLightVertices);
		_pF->ReadLE(m_nLightVertices);
		_pF->ReadLE(__m_Padding);
	}

	void Write(CCFile* _pF) const
	{
		_pF->WriteLE(m_LightID);
		_pF->WriteLE(m_iLightVertices);
		_pF->WriteLE(m_nLightVertices);
		_pF->WriteLE(__m_Padding);
	}
	
#ifndef CPU_LITTLEENDIAN
	void SwapLE()
	{
		::SwapLE(m_LightID);
		::SwapLE(m_iLightVertices);
		::SwapLE(m_nLightVertices);
		::SwapLE(__m_Padding);
	}
#endif
};



class CBSP3_LightVolume
{
public:
	uint16 m_iLight;
	uint16 m_iVBFP;
	uint16 m_nVBFP;
	CBox3Dfp32 m_BoundBox;
};


class CBSP3_ShadowVolume		// This is a per light and PL structure. Bad name, but it started out as a shadow volume only structure.
{
public:
	uint16 m_iPL;
	uint16 m_iLight;
	
	uint32	m_Bloat1;
	uint32	m_Bloat2;
	uint16	m_Bloat3;
	uint16	m_Bloat4;
//	uint32 m_iVBase;
//	uint32 m_iTriBase;
//	uint16 m_nVertices;
//	uint16 m_nTriangles;

	uint16 m_iSVNextPL;			// Single link list, points to next PL for m_iLight
	uint16 m_iSVNextLight;		// Single link list, points to next light for m_iPL

	CBox3Dfp32 m_BoundBoxSV;
	CBox3Dfp32 m_BoundBoxLight;

	uint16 m_iLMC;				// Index of first Surface/Light cluster (CBSP3_SLCluster)
	uint16 m_nLMC;
	uint16 m_iPlaneLightBound;	// Index into CBSP3_LightData::m_lPlanes
	uint16 m_nPlaneLightBound;

	void Clear()
	{
		m_iPL = 0;
		m_iLight = 0;
//		m_iVBase = 0;
//		m_iTriBase = 0;
//		m_nVertices = 0;
//		m_nTriangles = 0;

		m_iSVNextPL = 0;
		m_iSVNextLight = 0;

		m_BoundBoxSV.m_Min = 0;
		m_BoundBoxSV.m_Max = 0;
		m_BoundBoxLight.m_Min = 0;
		m_BoundBoxLight.m_Max = 0;

		m_iLMC = 0;
		m_nLMC = 0;
		m_iPlaneLightBound = 0;
		m_nPlaneLightBound = 0;
	}

	void Read(CCFile* _pF)
	{
		_pF->ReadLE(m_iPL);
		_pF->ReadLE(m_iLight);
//		_pF->ReadLE(m_iVBase);
//		_pF->ReadLE(m_iTriBase);
//		_pF->ReadLE(m_nVertices);
//		_pF->ReadLE(m_nTriangles);
		_pF->ReadLE(m_iSVNextPL);
		_pF->ReadLE(m_iSVNextLight);
		m_BoundBoxSV.Read(_pF);
		m_BoundBoxLight.Read(_pF);
		_pF->ReadLE(m_iLMC);
		_pF->ReadLE(m_nLMC);
		_pF->ReadLE(m_iPlaneLightBound);
		_pF->ReadLE(m_nPlaneLightBound);
	}

	void Write(CCFile* _pF) const
	{
#ifndef	PLATFORM_CONSOLE
		_pF->WriteLE(m_iPL);
		_pF->WriteLE(m_iLight);
//		_pF->WriteLE(m_iVBase);
//		_pF->WriteLE(m_iTriBase);
//		_pF->WriteLE(m_nVertices);
//		_pF->WriteLE(m_nTriangles);
		_pF->WriteLE(m_iSVNextPL);
		_pF->WriteLE(m_iSVNextLight);
		m_BoundBoxSV.Write(_pF);
		m_BoundBoxLight.Write(_pF);
		_pF->WriteLE(m_iLMC);
		_pF->WriteLE(m_nLMC);
		_pF->WriteLE(m_iPlaneLightBound);
		_pF->WriteLE(m_nPlaneLightBound);
#endif
	}
	
#ifndef CPU_LITTLEENDIAN
	void SwapLE()
	{
		::SwapLE(m_iPL);
		::SwapLE(m_iLight);
//		::SwapLE(m_iVBase);
//		::SwapLE(m_iTriBase);
//		::SwapLE(m_nVertices);
//		::SwapLE(m_nTriangles);
		::SwapLE(m_iSVNextPL);
		::SwapLE(m_iSVNextLight);
		m_BoundBoxSV.SwapLE();
		m_BoundBoxLight.SwapLE();
		::SwapLE(m_iLMC);
		::SwapLE(m_nLMC);
		::SwapLE(m_iPlaneLightBound);
		::SwapLE(m_nPlaneLightBound);
	}
#endif
};

class CBSP3_SLCluster			// Surface/Light cluster
{
public:
	uint16 m_iSurface;
	uint16 m_iLight;
	uint16 m_nFaces;
	uint16 m_nSBFaces;
	uint32 m_iiFaces;			// Start index in m_liSLCFaces
	uint32 m_iiSBFaces;			// Start index in m_liSLCFaces
	CBox3Dfp32 m_BoundBox;
	uint16 m_iSQ;				// Shader queue index
	uint16 __m_Padd_0;

	void Clear()
	{
		m_iSurface = 0;
		m_iLight = 0;
		m_nFaces = 0;
		m_nSBFaces = 0;
		m_iiFaces = 0;
		m_iiSBFaces = 0;
		m_BoundBox.m_Min = 0;
		m_BoundBox.m_Max = 0;
		m_iSQ = 0;
		__m_Padd_0 = 0;
	}

	void Read(CCFile* _pF)
	{
		_pF->ReadLE(m_iSurface);
		_pF->ReadLE(m_iLight);
		_pF->ReadLE(m_nFaces);
		_pF->ReadLE(m_nSBFaces);
		_pF->ReadLE(m_iiFaces);
		_pF->ReadLE(m_iiSBFaces);
		m_BoundBox.Read(_pF);
		_pF->ReadLE(m_iSQ);
		_pF->ReadLE(__m_Padd_0);
	}

	void Write(CCFile* _pF) const
	{
		_pF->WriteLE(m_iSurface);
		_pF->WriteLE(m_iLight);
		_pF->WriteLE(m_nFaces);
		_pF->WriteLE(m_nSBFaces);
		_pF->WriteLE(m_iiFaces);
		_pF->WriteLE(m_iiSBFaces);
		m_BoundBox.Write(_pF);
		_pF->WriteLE(m_iSQ);
		_pF->WriteLE(__m_Padd_0);
	}
	
#ifndef CPU_LITTLEENDIAN
	void SwapLE()
	{
		::SwapLE(m_iSurface);
		::SwapLE(m_iLight);
		::SwapLE(m_nFaces);
		::SwapLE(m_nSBFaces);
		::SwapLE(m_iiFaces);
		::SwapLE(m_iiSBFaces);
		m_BoundBox.SwapLE();
		::SwapLE(m_iSQ);
		::SwapLE(__m_Padd_0);
	}
#endif
};

class CBSP3_ShaderQueueElement
{
public:
	uint16 m_iLight;
	uint16 m_iSurface;

	void Clear()
	{
		m_iLight = 0;
		m_iSurface = 0;
	}

	void Read(CCFile* _pF)
	{
		_pF->ReadLE(m_iLight);
		_pF->ReadLE(m_iSurface);
	}

	void Write(CCFile* _pF) const
	{
		_pF->WriteLE(m_iLight);
		_pF->WriteLE(m_iSurface);
	}
	
#ifndef CPU_LITTLEENDIAN
	void SwapLE()
	{
		::SwapLE(m_iLight);
		::SwapLE(m_iSurface);
	}
#endif
};

class CBSP3_LightStrip
{
public:
	uint16	m_iStart;
	uint16	m_nLen:8;		// Length of strip
	uint16	m_iVB:4;		// Which vertexbuffer to use (local)
	uint16	m_iLMC:4;		// Which lightmap to use
};

class CBSP3_LMCGameData
{
public:
	uint16	m_iStrip;
	uint16	m_nStrip;
};

class CBSP3_LMCRenderData
{
public:
	uint8	m_iTex:4;
	uint8	m_iVB:4;
};

// Data for each Surface/Light cluster (all lightstrips)
class CBSP3_LMCData
{
public:
#ifdef	PLATFORM_CONSOLE
	TThinArray<CBSP3_LightStrip>	m_lStrips;
#else
	TArray<CBSP3_LightStrip>	m_lStrips;
#endif
	TThinArray<uint16>			m_lPrimStream;
	
//	int				m_VBID;

	CBSP3_LMCData& operator = ( const CBSP3_LMCData& _Other )
	{
		m_lStrips	= _Other.m_lStrips;

		return *this;
	}

	void Write(CDataFile* _pF) const;
	void Read(CDataFile* _pF);
};

class CBSP3_LightVB : public CReferenceCount
{
public:
#ifdef	PLATFORM_CONSOLE
	TThinArray<CVec3Dfp32>	m_lPosition;
	TThinArray<CVec2Dfp32>	m_lTexCoord;
#else
	TArray<CVec3Dfp32>	m_lPosition;
	TArray<CVec2Dfp32>	m_lTexCoord;
#endif

//	uint32	m_VBID;	// Used when rendering ingame

	CBSP3_LightVB& operator = (const CBSP3_LightVB& _Other )
	{
		m_lPosition	= _Other.m_lPosition;
		m_lTexCoord	= _Other.m_lTexCoord;
//		m_VBID	= _Other.m_VBID;
		return *this;
	}

	void Write(CDataFile* _pF) const;
	void Read(CDataFile* _pF);
};

typedef TPtr<CBSP3_LightVB>	spCBSP3_LightVB;

class CBSP3_LightData : public CReferenceCount
{
public:
	TThinArray<CXR_Light>		m_lLights;			// Temp...   I think
#ifdef PLATFORM_CONSOLE

	TThinArray<CBSP3_LMCRenderData>	m_lLMCLightmap;
	TThinArray<CBSP3_ShadowVolume>	m_lSV;
	TThinArray<uint16>				m_lSVVBID;
	TThinArray<CPlane3Dfp32> 		m_lPlanes;
	TThinArray<CRC_Attributes>		m_lAttributes;
#else

	TArray<CBSP3_LMCRenderData>		m_lLMCLightmap;
	TArray<CBSP3_ShadowVolume>		m_lSV;
	TArray<uint16>					m_lSVVBID;
	TArray<CPlane3Dfp32>				m_lPlanes;
	TArray<CRC_Attributes>			m_lAttributes;
#endif

	CBSP3_LightData();
	TThinArray<CXR_ThinSolid>		m_lLightBoundSolids;
	TThinArray<CBSP3_LightVolume>	m_lLightVolumes;

	// Shadow volumes
	TThinArray<uint16>				m_liSVFirstPL;		// First SV for each PL.
	TThinArray<uint16>				m_lnSVPL;			// Number of SV for each PL.
	TThinArray<uint16>				m_liSVFirstLight;	// First SV for each light

#ifdef PLATFORM_CONSOLE
	TThinArray<CBSP3_LightVB>		m_lLightVB;
	TThinArray<CBSP3_LMCGameData>	m_lLMCGameData;
	TThinArray<CBSP3_LightStrip>	m_lLightStrips;
#else
	TArray<CBSP3_LightVB>			m_lLightVB;
	TArray<CBSP3_LMCGameData>		m_lLMCGameData;
	TArray<CBSP3_LightStrip>		m_lLightStrips;
#endif

	int GetSVCount() const
	{
		return m_lSV.Len();
	}
	void SetSVVBIDCount( int _Count )
	{
		m_lSVVBID.SetLen( _Count );
		m_plSVVBID	= m_lSVVBID.GetBasePtr();
	}

	// public ptrs to private data
	CBSP3_LMCRenderData* m_plLMCLightmap;
	CBSP3_ShadowVolume* m_plSV;
	uint16* m_plSVVBID;
	CPlane3Dfp32* m_plPlanes;
	CRC_Attributes* m_plAttributes;

//	TThinArray<CBSP3_ShaderQueueElement> m_lShaderQueue;
//	TThinArray<CXR_VertexBuffer*> m_lpShaderQueueVBChain;	// Used during rendering. No IO.

	spCTextureContainer_Plain	m_spLMTC;

	CXR_Light* GetLights();
	void GetClusters();
	void GetVertexBuffers();
	void ReadLights( class CCFile* _pFile );
	void ReadClusters( class CCFile* _pFile );
	void ReadVertexBuffers( class CCFile* _pFile );

	CStr	m_Filename;
	int		m_LightsPos, m_LightsVer, m_LightsCount;
	int		m_VertexBufferPos;
	int		m_ClusterPos;
	TThinArray<uint16> m_lClusterLength;
	TThinArray<uint16> m_lVertexBufferSize;
	int16	m_ClusterCount;
	int8	m_VertexBufferCount;
	int8	m_KeepData;


	int m_Flags;

	void Get(int _iLocal, CRC_BuildVertexBuffer& _VB, int _Flags);
	void Release( int _iLocal );

	void OnPostPrecache();

	int GetPrimStreamCount( int _iLMC );
	int GetPrimStreamCount_KnitStrip( int _iLMC );
	void CreatePrimStream( int _iLMC, uint16** _ppDest );
	void CreatePrimStream_KnitStrip( int _iLMC, uint16** _ppDest );

	void ReadLightmap(CDataFile* _pF, bool _bTools = false);
	void Read(CDataFile* _pF);
	void Write(CDataFile* _pF);
	static TPtr<CBSP3_LightData> FindAndRead(CDataFile* _pF);
};

typedef TPtr<CBSP3_LightData> spCBSP3_LightData;

// ***********************************************************************************************
// ***********************************************************************************************
// ***********************************************************************************************

// Classes used to generate lightmap data.. not used in-game

class CBSP3_LightFaceInfo
{
public:
	uint32	m_iLightMap;
	uint16	m_iLight;	// Light index
	uint16	m_iFace;		// Face index
	uint16	m_iSLC;		// Surface/Light cluster index

	uint8	m_Width;	// Width of lightmap texture (divided by 2)
	uint8	m_Height;	// Height of lightmap texture (divided by 2)
	uint8	m_ScaleShift;

	class CXR_BSP_Model *	m_pModel;
};

class CBSP3_LightGenData : public CReferenceCount
{
public:
	TArray<CBSP3_LightFaceInfo>	m_lLightFaceInfo;
	TArray<spCImage> m_lspLightMaps;
	TArray<CBSP_LightMapInfo>	m_lLightMapInfo;

	TArray<CBSP3_LMCData>	m_lLMCData;
	TArray<spCBSP3_LightVB>	m_lLightVB;


	spCTextureContainer_Plain	m_spLMTC;
	void Write(CDataFile* _pF);
};

typedef TPtr<CBSP3_LightGenData>	spCBSP3_LightGenData;

class CXW_LMCluster2 : public CReferenceCount
{
public:
	int32	m_Width;
	int32	m_Height;

	TArray<CRct>	m_lRects;
	TArray<uint32>	m_liLightFaceInfo;	// Indices into LightFaceInfo array

	void AddIndex( uint32 _Index )
	{
		m_liLightFaceInfo.Add( _Index );
	}
};

typedef TPtr<CXW_LMCluster2>	spCXW_LMCluster2;

// ***********************************************************************************************
// ***********************************************************************************************
// ***********************************************************************************************

#endif // _INC_XW2COMMON

