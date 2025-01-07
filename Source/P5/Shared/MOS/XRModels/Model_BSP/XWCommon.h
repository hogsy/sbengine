// -------------------------------------------------------------------
//  The XW Fileformat
// -------------------------------------------------------------------

#ifndef _INC_XWCOMMON
#define _INC_XWCOMMON

#include "../MCC/MCC.h"
#include "../../XR/XRClass.h"
#include "XWCommon_VPUShared.h"
#define XW_VERSION	0x0124

#define XW_MAXUSERPORTALS 256

#define XW_PVS_NOCOMPRESS
#define XW_PVS_VERSION		0x0200

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
		[FACES],			CBSP_Face
		[FACEINDICES],		uint32
		[BSPNODES],			CBSP_Node
		[PORTALS],			CBSP_Portal
		[MAPPINGS],			CXR_PlaneMapping
		[LIGHTMAPSINFO],	CBSP_LightMapInfo
		[LIGHTMAPS],		-
		[LIGHTVOLUMEINFO],	CBSP_LightVolumeInfo
		[LIGHTVOLUME],		-
		[LIGHTVERTINFO],	CBSP_LightVerticeInfo
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
#define XW_FACE_COMPILERPOLYGONSHADOWVOLUME 4096		// Only used in XWC to say that the face need a shadow volume constructed from it. (i.e, the shadow for the face will not be constructed from a solid.)
#define XW_FACE_VERTEXLIGHTMAP 8192
#define XW_FACE_SHADOWCASTER 16384
#define XW_FACE_DELETEAFTERBSP 32768					// Used in xwc only

#define XW_FACE_VERSION 0x0203

class CPhysLineContext
{
public:
	CVec3Dfp32	m_V0;
	CVec3Dfp32	m_V1;
	fp32	m_InvV1V0;
};

class CBSP_CoreFace
{
public:
	int32 m_iiEdges;

	union
	{
		uint32 m_VertexIO;
		struct
		{
			uint32 m_iiVertices:24;	// Index to first vertex-index
			uint32 m_nVertices:8;	// Number vertices
		};
	};

	uint32 m_iMapping;
	uint16 m_iSurface;
	uint16 m_Flags;

	uint32 m_iPlane;
	uint32 m_iiNormals;		// Index to first normal-index
	uint16 m_iBackMedium;	// What's behind the face.
	uint16 m_iFrontMedium;	// What's in front of the face.

	union
	{
		uint32 m_LightInfoIO;
		struct
		{
			uint32 m_iLightInfo:28;	// Index of first lightmap/lightvertices
			uint32 m_nLightInfo:4;
		};
	};

	CBSP_CoreFace();
	void Read(CCFile* _pF, int _Version);
	void Write(CCFile* _pF);

#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif
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


class CBSP_Node					// 20 bytes
{
public:
	uint32 m_iPlane;			// 0  -> This is a leaf, otherwize a node

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

	uint16 m_Flags;
	uint16 m_iPortalLeaf;
	uint16 m_Padding0;


	inline bool IsNode() const { return (m_iPlane != 0); }
	inline bool IsLeaf() const { return (m_iPlane == 0); }
	inline bool IsStructureLeaf() const { return (m_Flags & (XW_NODE_PORTAL | XW_NODE_STRUCTURE)) == (XW_NODE_PORTAL | XW_NODE_STRUCTURE); }
	inline bool IsNavigationLeaf() const { return false; }

	CBSP_Node()
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

	void GetPortalInfo(const CBSP_Node& _SrcNode)
	{
		m_Flags |= _SrcNode.m_Flags & (XW_NODE_PORTAL | XW_NODE_STRUCTURE/* | XW_NODE_NAVIGATION*/);
		m_iPortalLeaf = _SrcNode.m_iPortalLeaf;
//		m_iNavigationLeaf = _SrcNode.m_iNavigationLeaf;
//		m_nPortals = _SrcNode.m_nPortals;
//		m_iiPortals = _SrcNode.m_iiPortals;
	}

	CStr GetString() const
	{
		MAUTOSTRIP(CBSP_Node_GetString, CStr());
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

class CBSP_PortalLeaf
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

	CBSP_PortalLeaf()
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
class CBSP_Portal : public CXR_IndexedPortal			// 16 bytes.
{
public:
	uint16 m_Flags;
	uint32 m_iPlane;
	uint32 m_iNodeFront;
	uint32 m_iNodeBack;
	uint16 m_iRPortal;		// Used during rendering for temporary portal information.
	uint16 m_iFogPortal;

	CBSP_Portal();
	void Read(CCFile* _pF, int _Version);
	void Write(CCFile* _pF) const;

};

// -------------------------------
class CBSP_FogPortalFace
{
public:
	uint32 m_iiVertices;
	uint32 m_nVertices;

	CBSP_FogPortalFace()
	{
		m_iiVertices = 0;
		m_nVertices = 0;
	}

	void Read(CCFile* _pF);
	void Write(CCFile* _pF) const;
};

// -------------------------------
class CBSP_FogPortal
{
public:
	int32 m_iVertices;
	int32 m_nVertices;
	int32 m_iFaces;
	int32 m_nFaces;

	CBSP_FogPortal();
	void Read(CCFile* _pF);
	void Write(CCFile* _pF) const;
};



// -------------------------------
/*class CBSP_Leaf : public CBSP_Node
{
public:
	uint32 m_iiFaces;
	uint16 m_nFaces;
	uint16 m_Medium;
	int16 m_iVisList;		// -1 == No vis.list

	CBSP_Leaf()
	{
		m_iiFaces = 0;
		m_nFaces = 0;
		m_iNodeParent = 0;
		m_iVisList = -1;
	}
};
*/

// -------------------------------
#define XW_LIGHTMAPINFO_VERSION	4
class CBSP_LightMapInfo
{
public:
/*	int32 m_LightID;
	int32 m_Scale;
	fp32 m_OffsetU;
	fp32 m_OffsetV;
	uint16 m_LMCOffsetU;
	uint16 m_LMCOffsetV;
	uint16 m_iLMC;*/

	// Version 3
//	fp2 m_IntensityScale;
//	uint8 m_Scale;
//	uint8 m_LMCOffsetXHalf;
//	uint8 m_LMCOffsetYHalf;
//	uint8 m_iLMC;
//	uint8 m_LMCWidthHalf;
//	uint8 m_LMCHeightHalf;

	// Version 4
	fp2 m_IntensityScale;
	uint8 m_ScaleShift;
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

class CBSP_LightMapContainer : public CReferenceCount
{
public:
	class CBSP_LMDesc
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

	TThinArray<CBSP_LMDesc> m_lLMDesc;
	TThinArray<uint8> m_lLMData;

	CBSP_LightMapContainer();
	~CBSP_LightMapContainer();

	void Create(TArray<spCImage> _lspLightMaps, int _Format = IMAGE_FORMAT_BGR8);	// Used in XWC when writing XW:s.
	TArray<spCImage> GetLightMapImages(int _Format = IMAGE_FORMAT_RGB32_F);		// Used in XWC when reading XW:s.

	void GetLightMap(int _iLightMap, CImage& _RefImage);	// Creates a "reference image" in _RefImage, no image data is moved/copied.

	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile);
};

// -------------------------------
#define XW_LIGHTVOLUME_VERSION		0x0100
#define XW_LIGHTVOLUME_MAXGRIDSIZE	8

class CBSP_LightVolumeInfo : public CXR_LightVolume
{
public:
	int32 m_LightID;
	CBox3Dfp32 m_VolumeSize;
	CVec3Dfp32 m_CellSize;
	CVec3Dfp32 m_CellSizeInv;
	CVec3Dint16 m_GridSize;
	uint8 m_nGrids;
	uint32 m_CellsPerGrid;
	uint32 m_iCells;
	CXR_LightGridPoint* m_pCells;
	int16 m_iPortalLeaf;
	CBSP_LightVolumeInfo* m_pNext;

	CBSP_LightVolumeInfo()
	{
		m_LightID = 0;
		m_nGrids = 0;
		m_CellsPerGrid = 0;
		m_iCells = 0;
		m_iPortalLeaf = 0;
		m_pCells = NULL;
		m_pNext = NULL;
	}

	void Read(CCFile* _pF);
	void Write(CCFile* _pF) const;

	virtual CXR_LightVolume* GetNext() { return m_pNext; };
	virtual const CXR_LightVolume* GetNext() const { return m_pNext; };
	virtual fp32 Light_EvalVertex(const CXR_LightID* _pIDMap, const CVec3Dfp32& _V, CVec3Dfp32& _LDir, CVec3Dfp32& _LColor);
	virtual CPixel32 Light_Vertex(const CXR_LightID* _pIDMap, const CVec3Dfp32& _V, const CVec3Dfp32* _pN);
	virtual void Light_Array(const CXR_LightID* _pIDMap, int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN = NULL, const CMat4Dfp32* _pWMat = NULL, int _Flags = 0);
	virtual void Light_EvalPoint(const CVec3Dfp32& _V, CVec4Dfp32* _pAxis) {};
	virtual void Light_EvalPointArrayMax(const CVec3Dfp32* _pV, int _nV, CVec4Dfp32* _pAxis) {};
};

// -------------------------------
#define XR_LIGHTOCTTREE_NODEVERSION 0x0100
#define XR_LIGHTOCTTREE_POINTVERSION 0x0100
#define XR_LIGHTOCTTREE_VERSION 0x0101

#if defined(PLATFORM_PS2) && defined(COMPILER_CODEWARRIOR)
# define GET_UNALIGNED_INT16(x)    ( (uint16)(((uint8*)&(x))[0]) + (((uint16)(((uint8*)&(x))[1]))<<8) )
# define SET_UNALIGNED_INT16(x, v) { ((uint8*)&(x))[0] = uint8(v); ((uint8*)&(x))[1] = uint8((v)>>8); }
# pragma push
# pragma pack(1)
#else
# define GET_UNALIGNED_INT16(x)    (x)
# define SET_UNALIGNED_INT16(x, v) { (x) = (v); }
# pragma pack(push, 1) // MH: Det här funkar nog inte på GC/PS2. Har inte tid att fixa detta nu...
//# pragma pack(1)
#endif

class CBSP_LightOcttreeNode				// 17 bytes
{
private:
#ifdef COMPILER_GNU
	uint8 m_SubNodeTypes __attribute__ ((__packed__));
	uint16 m_iSubNodes __attribute__ ((__packed__));
#else
	uint8 m_SubNodeTypes;
	uint16 m_iSubNodes;
#endif

public:
	void Read(CCFile* _pF, int _Ver);
	void Write(CCFile* _pF) const;
#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif

	inline uint8  GetSubNodeTypes() const              { return m_SubNodeTypes; }
	inline void   SetSubNodeTypes(uint8 _SubNodeTypes) { m_SubNodeTypes = _SubNodeTypes; }
	inline uint16 GetSubNodes() const                  { return GET_UNALIGNED_INT16(m_iSubNodes); }
	inline void   SetSubNodes(uint16 _iSubNodes)       { SET_UNALIGNED_INT16(m_iSubNodes, _iSubNodes); }
};

#if defined(PLATFORM_PS2) && defined(COMPILER_CODEWARRIOR)
#pragma pop
#else
#pragma pack(pop)
#endif


class CBSP_LightOcttree : public CReferenceCount, public CXR_LightVolume
{
public:
	TThinArray<CBSP_LightOcttreeNode> m_lNodes;
	TThinArray<CXR_LightGridPoint> m_lPoints;

	uint32 m_RootSize;
	uint32 m_MinLeafSize;
	CVec3Dfp32 m_RootPos;

	void Read(CDataFile* _pF);
	void Write(CDataFile* _pF) const;

	static TPtr<CBSP_LightOcttree> FindAndRead(CDataFile* _pF);

	// CXR_LightVolume overrides
	virtual class CXR_LightVolume* GetNext() { return NULL; }
	virtual const class CXR_LightVolume* GetNext() const { return NULL; };
	virtual fp32 Light_EvalVertex(const class CXR_LightID* _pIDMap, const CVec3Dfp32& _V, CVec3Dfp32& _LDir, CVec3Dfp32& _LColor);
	virtual CPixel32 Light_Vertex(const class CXR_LightID* _pIDMap, const CVec3Dfp32& _V, const CVec3Dfp32* _pN);
	virtual void Light_Array(const class CXR_LightID* _pIDMap, int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN = NULL, const CMat4Dfp32* _pWMat = NULL, int _Flags = 0);
	virtual void Light_EvalPoint(const CVec3Dfp32& _V, CVec4Dfp32* _pAxis){};
	virtual void Light_EvalPointArrayMax(const CVec3Dfp32* _pV, int _nV, CVec4Dfp32* _pAxis) {};
};

// -------------------------------
class CBSP_LightVerticesInfo
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

// -------------------------------
enum
{
	XW_LMCLUSTER_FACE = 0,
	XW_LMCLUSTER_SB,

	XW_LMCLUSTER_VERSION = 0x0200,
	XW_LMCLUSTERELEMENT_VERSION = 0x0201,
};

#if defined(PLATFORM_PS2) && defined(COMPILER_CODEWARRIOR)
# pragma push
# pragma pack(1)
#else
# pragma pack(push, 1)
//# pragma pack(1)
#endif

class CXW_LMCElement
{
private:
	union
	{
		struct
		{
			uint32 m_iLightInfo : 28;
			uint32 m_nLightInfo : 4;
		};
		uint32 m_LightInfoIO;
	};

public:
	CXW_LMCElement()
	{
	}

	CXW_LMCElement(int _iLightInfo, int _nLightInfo)
	{
		m_iLightInfo = _iLightInfo;
		m_nLightInfo = _nLightInfo;
	}

	void Read(CCFile* _pF, int _Version);
	void Write(CCFile* _pF) const;

	inline uint8  GetLightInfoN() const             { return m_nLightInfo; }
	inline void   SetLightInfoN(uint8 _nLightInfo)  { m_nLightInfo = _nLightInfo; }
	inline int	  GetLightInfoI() const             { return m_iLightInfo; }
	inline void   SetLightInfoI(uint16 _iLightInfo) { m_iLightInfo = _iLightInfo; }
};

#if defined(PLATFORM_PS2) && defined(COMPILER_CODEWARRIOR)
# pragma pop
#else
# pragma pack(pop)
#endif

class CXW_LMCluster : public CReferenceCount
{
public:
	int32 m_Width;
	int32 m_Height;
	TArray<CXW_LMCElement> m_lElements;

	void AddLightInfo(int _iLightInfo, int _nLightInfo)
	{
		m_lElements.Add(CXW_LMCElement(_iLightInfo, _nLightInfo));
	}

	void Read(CCFile* _pF);
	void Write(CCFile* _pF) const;
};

typedef TPtr<CXW_LMCluster> spCXW_LMCluster;

// -------------------------------
class CWObjectAttributes : public CReferenceCount
{
	int m_nKeys;
	int m_KeySize;
	int32* m_pKeyData;
	// nKeys * (int32 KeyNameOffset, int32 KeyValueOffset)

public:

	CWObjectAttributes();
	CWObjectAttributes(const TArray<CStr>& _lKeyNames, const TArray<CStr>& _lKeyValues);
	~CWObjectAttributes();

	int GetnKeys();
	int GetKeyIndex(CStr _Key);
	CStr GetKeyName(int _iKey);
	CStr GetKeyValue(int _iKey);

	void Write(CCFile* _pFile) const;
	void Read(CCFile* _pFile);
};

typedef TPtr<CWObjectAttributes> spCWObjectAttributes;

// -------------------------------
class CBSP_CreateInfo
{
public:
	mutable TPtr<CXR_Model> m_spMaster;
	int m_iSubModel;
	int m_nSubModels;
	int m_Flags;

	enum
	{
		FLAGS_MAKEPHYSSURFACESVISIBLE = 1,
	};

	CBSP_CreateInfo()
	{
		m_iSubModel = 0;
		m_nSubModels = 0;
		m_Flags = 0;
	}
};


#endif // _INC_XWCOMMON

