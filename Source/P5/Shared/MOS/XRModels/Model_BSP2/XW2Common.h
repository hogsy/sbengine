// -------------------------------------------------------------------
//  The XW Fileformat
// -------------------------------------------------------------------

#ifndef _INC_XW2COMMON
#define _INC_XW2COMMON

#include "../MCC/MCC.h"
#include "../../XR/XRClass.h"
#include "../../XR/Solids/XRThinSolid.h"
#include "XW2Common_VPUShared.h"
#define XW_VERSION	0x0124

#define XW_MAXUSERPORTALS 256

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
		[FACES],			CBSP2_Face
		[FACEINDICES],		uint32
		[BSPNODES],			CBSP2_Node
		[PORTALS],			CBSP2_Portal
		[MAPPINGS],			CXR_PlaneMapping
		[LIGHTMAPSINFO],	CBSP2_LightMapInfo
		[LIGHTMAPS],		-
		[LIGHTVOLUMEINFO],	CBSP2_LightVolumeInfo
		[LIGHTVOLUME],		-
		[LIGHTVERTINFO],	CBSP2_LightVerticeInfo
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

#define XW_FACE_VERSION 0x0203


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


class CBSP2_Node					// 20 bytes
{
public:
	// Ugly!
	union
	{
		uint32 m_iNodeFront;	// Node
		uint32 m_nFaces;		// Leaf
	};
	union
	{
		uint32 m_iNodeBack;		// Node
		uint32 m_iMedium;		// Leaf
	};

	uint32 m_iNodeParent;		// neg. => bug.  0 == Root
	union
	{
		uint32 m_NodeIO;
		struct
		{
			uint32 m_iiFaces:24;
			uint32 m_Flags:8;
		};
	};

	uint32 m_iPlane;			// 0  -> This is a leaf, otherwize a node

	uint16 m_iPortalLeaf;
	uint16 m_Padding0;


	inline bool IsNode() const { return (m_iPlane != 0); }
	inline bool IsLeaf() const { return (m_iPlane == 0); }
	inline bool IsStructureLeaf() const { return (m_Flags & (XW_NODE_PORTAL | XW_NODE_STRUCTURE)) == (XW_NODE_PORTAL | XW_NODE_STRUCTURE); }
	inline bool IsNavigationLeaf() const { return false; }

	CBSP2_Node()
	{
		m_iPlane = 0;

		m_iNodeFront = 0;
		m_iNodeBack = 0;

		m_iNodeParent = 0;
		m_iiFaces = 0;

		m_Flags = 0;
		m_iPortalLeaf = 0;
	}

	void GetPortalInfo(const CBSP2_Node& _SrcNode)
	{
		m_Flags |= _SrcNode.m_Flags & (XW_NODE_PORTAL | XW_NODE_STRUCTURE/* | XW_NODE_NAVIGATION*/);
		m_iPortalLeaf = _SrcNode.m_iPortalLeaf;
//		m_iNavigationLeaf = _SrcNode.m_iNavigationLeaf;
//		m_nPortals = _SrcNode.m_nPortals;
//		m_iiPortals = _SrcNode.m_iiPortals;
	}

	CStr GetString() const
	{
		MAUTOSTRIP(CBSP2_Node_GetString, CStr());
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

class CBSP2_PortalLeaf
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

	CBSP2_PortalLeaf()
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
class CBSP2_Portal : public CXR_IndexedPortal			// 16 bytes.
{
public:
	uint32 m_iNodeFront;
	uint32 m_iNodeBack;
	uint32 m_iPlane;
	uint16 m_iFogPortal;

	CBSP2_Portal();
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
class CBSP2_FogPortalFace
{
public:
	uint32 m_iiVertices;
	uint32 m_nVertices;

	CBSP2_FogPortalFace()
	{
		m_iiVertices = 0;
		m_nVertices = 0;
	}

	void Read(CCFile* _pF);
	void Write(CCFile* _pF) const;
};

// -------------------------------
class CBSP2_FogPortal
{
public:
	int32 m_iVertices;
	int32 m_nVertices;
	int32 m_iFaces;
	int32 m_nFaces;

	CBSP2_FogPortal();
	void Read(CCFile* _pF);
	void Write(CCFile* _pF) const;
};



// -------------------------------
/*class CBSP2_Leaf : public CBSP2_Node
{
public:
	uint32 m_iiFaces;
	uint16 m_nFaces;
	uint16 m_Medium;
	int16 m_iVisList;		// -1 == No vis.list

	CBSP2_Leaf()
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
class CBSP2_LightMapInfo
{
public:
/*	int32 m_LightID;
	int32 m_Scale;
	fp32 m_OffsetU;
	fp32 m_OffsetV;
	uint16 m_LMCOffsetU;
	uint16 m_LMCOffsetV;
	uint16 m_iLMC;*/

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

class CBSP2_LightMapContainer : public CReferenceCount
{
public:
	class CBSP2_LMDesc
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

	TThinArray<CBSP2_LMDesc> m_lLMDesc;
	TThinArray<uint8> m_lLMData;

	CBSP2_LightMapContainer();
	~CBSP2_LightMapContainer();

	void Create(TArray<spCImage> _lspLightMaps, int _Format = IMAGE_FORMAT_BGR8);	// Used in XWC when writing XW:s.
	TArray<spCImage> GetLightMapImages(int _Format = IMAGE_FORMAT_RGB32_F);		// Used in XWC when reading XW:s.

	void GetLightMap(int _iLightMap, CImage& _RefImage);	// Creates a "reference image" in _RefImage, no image data is moved/copied.

	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile);
};

// -------------------------------
class CBSP2_LightVerticesInfo
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



class CBSP2_LightVolume
{
public:
	uint16 m_iLight;
	uint16 m_iVBFP;
	uint16 m_nVBFP;
	CBox3Dfp32 m_BoundBox;
};

#define XW2_SHADOWVOLUME_VER	0x0100

class CBSP2_ShadowVolume		// This is a per light and PL structure. Bad name, but it started out as a shadow volume only structure.
{
public:
	uint16 m_iPL;
	uint16 m_iLight;
	uint32 m_iVBase;
	uint32 m_iTriBase;
	uint16 m_nVertices;
	uint16 m_nTriangles;

	uint16 m_iSVNextPL;			// Single link list, points to next PL for m_iLight
	uint16 m_iSVNextLight;		// Single link list, points to next light for m_iPL

	CBox3Dfp32 m_BoundBoxSV;
	CBox3Dfp32 m_BoundBoxLight;

	uint16 m_iSLC;				// Index of first Surface/Light cluster (CBSP2_SLCluster)
	uint16 m_nSLC;
	uint32 m_iPlaneLightBound;	// Index into CBSP2_LightData::m_lPlanes
	uint16 m_nPlaneLightBound;
	uint16 m_Padding0;

	void Clear()
	{
		m_iPL = 0;
		m_iLight = 0;
		m_iVBase = 0;
		m_iTriBase = 0;
		m_nVertices = 0;
		m_nTriangles = 0;

		m_iSVNextPL = 0;
		m_iSVNextLight = 0;

		m_BoundBoxSV.m_Min = 0;
		m_BoundBoxSV.m_Max = 0;
		m_BoundBoxLight.m_Min = 0;
		m_BoundBoxLight.m_Max = 0;

		m_iSLC = 0;
		m_nSLC = 0;
		m_iPlaneLightBound = 0;
		m_nPlaneLightBound = 0;
	}

	void Read(CCFile* _pF, int _nVer)
	{
		switch(_nVer)
		{
		case 0x0000:
			{
				_pF->ReadLE(m_iPL);
				_pF->ReadLE(m_iLight);
				_pF->ReadLE(m_iVBase);
				_pF->ReadLE(m_iTriBase);
				_pF->ReadLE(m_nVertices);
				_pF->ReadLE(m_nTriangles);
				_pF->ReadLE(m_iSVNextPL);
				_pF->ReadLE(m_iSVNextLight);
				m_BoundBoxSV.Read(_pF);
				m_BoundBoxLight.Read(_pF);
				_pF->ReadLE(m_iSLC);
				_pF->ReadLE(m_nSLC);
				uint16 iPlaneLightBound;
				_pF->ReadLE(iPlaneLightBound);
				m_iPlaneLightBound = iPlaneLightBound;
				_pF->ReadLE(m_nPlaneLightBound);
			}
			break;

		case 0x0100:
			{
				_pF->ReadLE(m_iPL);
				_pF->ReadLE(m_iLight);
				_pF->ReadLE(m_iVBase);
				_pF->ReadLE(m_iTriBase);
				_pF->ReadLE(m_nVertices);
				_pF->ReadLE(m_nTriangles);
				_pF->ReadLE(m_iSVNextPL);
				_pF->ReadLE(m_iSVNextLight);
				m_BoundBoxSV.Read(_pF);
				m_BoundBoxLight.Read(_pF);
				_pF->ReadLE(m_iSLC);
				_pF->ReadLE(m_nSLC);
				_pF->ReadLE(m_iPlaneLightBound);
				_pF->ReadLE(m_nPlaneLightBound);
				uint16 Padding0;
				_pF->ReadLE(Padding0);
			}
			break;

		default:
			Error_static("CBSP2_ShadowVolueme::Read", CStrF("Unsupported version %X", _nVer));
		}
	}

	void Write(CCFile* _pF) const
	{
		_pF->WriteLE(m_iPL);
		_pF->WriteLE(m_iLight);
		_pF->WriteLE(m_iVBase);
		_pF->WriteLE(m_iTriBase);
		_pF->WriteLE(m_nVertices);
		_pF->WriteLE(m_nTriangles);
		_pF->WriteLE(m_iSVNextPL);
		_pF->WriteLE(m_iSVNextLight);
		m_BoundBoxSV.Write(_pF);
		m_BoundBoxLight.Write(_pF);
		_pF->WriteLE(m_iSLC);
		_pF->WriteLE(m_nSLC);
		_pF->WriteLE(m_iPlaneLightBound);
		_pF->WriteLE(m_nPlaneLightBound);
		uint16 Padding0 = 0;
		_pF->WriteLE(Padding0);
	}
	
#ifdef CPU_BIGENDIAN
	void SwapLE()
	{
		::SwapLE(m_iPL);
		::SwapLE(m_iLight);
		::SwapLE(m_iVBase);
		::SwapLE(m_iTriBase);
		::SwapLE(m_nVertices);
		::SwapLE(m_nTriangles);
		::SwapLE(m_iSVNextPL);
		::SwapLE(m_iSVNextLight);
		m_BoundBoxSV.SwapLE();
		m_BoundBoxLight.SwapLE();
		::SwapLE(m_iSLC);
		::SwapLE(m_nSLC);
		::SwapLE(m_iPlaneLightBound);
		::SwapLE(m_nPlaneLightBound);
	}
#endif
};

#define XW_SLCFACE_VERSION 0x0100
#define	XW_SLC_VERSION	0x0101

class CBSP2_SLCluster			// Surface/Light cluster
{
public:
	uint16 m_iSurface;
	uint16 m_iLight;
	uint16 m_nFaces;
	uint16 m_iSQ;				// Shader queue index
	uint32 m_iiFaces;			// Start index in m_liSLCFaces
	CBox3Dfp32 m_BoundBox;

	void Clear()
	{
		m_iSurface = 0;
		m_iLight = 0;
		m_nFaces = 0;
		m_iiFaces = 0;
		m_BoundBox.m_Min = 0;
		m_BoundBox.m_Max = 0;
		m_iSQ = 0;
	}

	void Read(CCFile* _pF, int _Version)
	{
		switch(_Version)
		{
		case 0x0101:
			{
				_pF->ReadLE(m_iSurface);
				_pF->ReadLE(m_iLight);
				_pF->ReadLE(m_nFaces);
				_pF->ReadLE(m_iSQ);
				_pF->ReadLE(m_iiFaces);
				m_BoundBox.Read(_pF);
				break;
			}

		case 0:	// Old non-versioned format
			{
				_pF->ReadLE(m_iSurface);
				_pF->ReadLE(m_iLight);
				_pF->ReadLE(m_nFaces);
				uint16 nSBFaces;
				_pF->ReadLE(nSBFaces);		// Legacy junk
				_pF->ReadLE(m_iiFaces);
				uint32 iiSBFaces;
				_pF->ReadLE(iiSBFaces);		// Legacy junk
				m_BoundBox.Read(_pF);
				_pF->ReadLE(m_iSQ);
				uint16 Padd;
				_pF->ReadLE(Padd);			// Legacy junk
				break;
			};
		}
	}

	void Write(CCFile* _pF) const
	{
		_pF->WriteLE(m_iSurface);
		_pF->WriteLE(m_iLight);
		_pF->WriteLE(m_nFaces);
		_pF->WriteLE(m_iSQ);
		_pF->WriteLE(m_iiFaces);
		m_BoundBox.Write(_pF);
	}
	
#ifdef CPU_BIGENDIAN
	void SwapLE()
	{
		::SwapLE(m_iSurface);
		::SwapLE(m_iLight);
		::SwapLE(m_nFaces);
		::SwapLE(m_iiFaces);
		m_BoundBox.SwapLE();
		::SwapLE(m_iSQ);
	}
#endif
};

class CBSP2_ShaderQueueElement
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

class CBSP2_VBChain
{
public:
	CXR_VBIDChain* m_pChain;

	CBSP2_VBChain()
	{
		Clear();
	}

	void Clear()
	{
		m_pChain = NULL;
	}

	void SetToVB(CXR_VertexBuffer *_pVB)
	{
		_pVB->m_Flags |= CXR_VBFLAGS_VBIDCHAIN;
		_pVB->m_pVBChain = m_pChain;

	}

	void AddChain(CXR_VBIDChain *_pChain)
	{
		if (m_pChain)
		{
			CXR_VBIDChain *pLastChain = _pChain;

			while (pLastChain->m_pNextVB)
				pLastChain = pLastChain->m_pNextVB;

			pLastChain->m_pNextVB = m_pChain;
		}
		m_pChain = _pChain;
	}

	CXR_VBIDChain* GetVBIDChain()
	{
		return m_pChain;
	}
};

/*
class CBSP2_VBChain
{
public:
	mint m_Chain : (sizeof(void *) * 8 - 1);
	mint m_bVBIds : 1;

	CBSP2_VBChain()
	{
		Clear();
	}

	void Clear()
	{
		m_Chain = NULL;
		m_bVBIds = 0;
	}

	void SetToVB(CXR_VertexBuffer *_pVB)
	{
		M_ASSERT(!_pVB->m_pVBChain, "Must be so");
		if (m_bVBIds)
			_pVB->m_Flags |= CXR_VBFLAGS_VBIDCHAIN;
		else
			_pVB->m_Flags |= CXR_VBFLAGS_VBCHAIN;

		_pVB->m_pVBChain = (void *)(m_Chain << 1);
		
	}

	void AddChain(CXR_VBChain *_pChain)
	{
		if (!m_Chain)
		{
			m_bVBIds = false;			
		}
		else
		{
			M_ASSERT(!m_bVBIds, "Wrong type");
			CXR_VBChain *pLastChain = _pChain;

			while (pLastChain->m_pNextVB)
				pLastChain = pLastChain->m_pNextVB;

			pLastChain->m_pNextVB = GetVBChain();
		}
		m_Chain = ((mint)_pChain >> 1);
	}

	void AddChain(CXR_VBIDChain *_pChain)
	{
		if (!m_Chain)
		{
			m_bVBIds = true;			
		}
		else
		{
			M_ASSERT(m_bVBIds, "Wrong type");
			CXR_VBIDChain *pLastChain = _pChain;

			while (pLastChain->m_pNextVB)
				pLastChain = pLastChain->m_pNextVB;

			pLastChain->m_pNextVB = GetVBIDChain();
		}
		m_Chain = ((mint)_pChain >> 1);
	}

	CXR_VBChain *GetVBChain()
	{
		return (CXR_VBChain *)(m_Chain << 1);
	}

	CXR_VBIDChain *GetVBIDChain()
	{
		return (CXR_VBIDChain *)(m_Chain << 1);
	}

};
*/

// -------------------------------------------------------------------
class CBSP2_SLCVBInfo
{
public:
	uint16 m_VBID;
	uint16 m_IBID;
	uint16 m_iPrimTri;
	uint16 m_nPrimTri;
};

class CBSP2_SLCIBInfo
{
public:
	uint16 m_IBID;
	uint16 m_nPrim;
	uint32 m_nSLC;
	uint32 m_iSLC;
};

class CBSP2_SLCIBContainer : public CReferenceCount, public CXR_VBContainer
{
public:
	class CBSP2_LightData* m_pLD;
	class CXR_Model_BSP2* m_pModel;
	TThinArray<uint16> m_lTempPrim;

	CBSP2_SLCIBContainer();
	~CBSP2_SLCIBContainer();
	void Create(CBSP2_LightData* _pLD, CXR_Model_BSP2* _pModel);

	virtual int GetNumLocal();
	virtual int GetID(int _iLocal);
	virtual CFStr GetName(int _iLocal);
	virtual void Get(int _iLocal, CRC_BuildVertexBuffer& _VB, int _Flags);
	virtual void Release(int _iLocal);
};

#ifdef PLATFORM_CONSOLE
#define XW_LIGHTDATATHINARRAY TThinArray
#else
#define XW_LIGHTDATATHINARRAY TArray
#endif

// -------------------------------------------------------------------
class CBSP2_LightData : public CReferenceCount, public CXR_VBContainer
{
public:
	TThinArray<uint16> m_liFaceLights;
	TThinArrayAlign<CXR_Light, 16> m_lLights;				// Temp...   I think
	TThinArray<CXR_ThinSolid> m_lLightBoundSolids;
	TThinArray<CBSP2_LightVolume> m_lLightVolumes;

	// Shadow volumes
	TThinArray<uint16> m_liSVFirstPL;				// First SV for each PL.
	TThinArray<uint16> m_lnSVPL;					// Number of SV for each PL.
	TThinArray<uint16> m_liSVFirstLight;			// First SV for each light
#ifndef PLATFORM_CONSOLE
	TThinArray<CVec3Dfp32> m_lSVVertices;
	TThinArray<uint16> m_liSVPrim;
#endif
	CStr m_FileName;
	int m_FileOffsetVertices;
	int m_FileOffsetPrim;

	XW_LIGHTDATATHINARRAY<CBSP2_ShadowVolume> m_lSV;
	XW_LIGHTDATATHINARRAY<CBSP2_SLCluster> m_lSLC;
	XW_LIGHTDATATHINARRAY<uint32> m_liSLCFaces;					// SB indices have 12 bit brush #, 4 bit face nr.
	XW_LIGHTDATATHINARRAY<CPlane3Dfp32> m_lPlanes;

	TThinArray<CBSP2_ShaderQueueElement> m_lShaderQueue;
	TThinArray<CBSP2_SLCVBInfo> m_lSLCVBI;
	TThinArray<CBSP2_SLCIBInfo> m_lSLCIBI;
	TPtr<CBSP2_SLCIBContainer> m_spSLCIBC;

	const char *GetContainerSortName() 
	{
		return m_FileName.Str();
	}


	class CLightDataVBID
	{
	public:
		int32 m_VBID:31;
		int32 m_bNoRelease:1;
		void *m_pData;

		CLightDataVBID()
		{
			m_VBID = -1;
			m_pData = NULL;
		}
		~CLightDataVBID()
		{
			if (m_pData)
				MRTC_GetMemoryManager()->Free(m_pData);
		}
	};

	TThinArray<CLightDataVBID> m_lVBIds;

	~CBSP2_LightData();

	virtual int GetNumLocal();
	virtual int GetID(int _iLocal);
	virtual CFStr GetName(int _iLocal);
	virtual void Get(int _iLocal, CRC_BuildVertexBuffer& _VB, int _Flags);
	virtual void Release(int _iLocal);

	void AllocVBIds(CXR_Model_BSP2* _pModel);
	void FreeVBIds();

	void Read(CDataFile* _pF);
	void Write(CDataFile* _pF) const;

	template<class T>
	static TPtr<T> FindAndRead(CDataFile* _pDFile)
	{
		_pDFile->PushPosition();
		if (_pDFile->GetNext("UNIFIEDLIGHTDATA"))
		{
			if (!_pDFile->GetSubDir())
				Error_static("CBSP2_LightData::FindAndRead", "Invalid entry.");

			TPtr<T> spLightData = MNew(T);
			if (!spLightData)
				Error_static("CBSP2_LightOcttree::FindAndRead", "Out of memory.");

			spLightData->Read(_pDFile);
			_pDFile->PopPosition();
			return spLightData;
		}

		_pDFile->PopPosition();

		return NULL;
	}
};

typedef TPtr<CBSP2_LightData> spCBSP2_LightData;

// -------------------------------
#define XR_LIGHTFIELDOCTTREE_NODEVERSION 0x0100
#define XR_LIGHTFIELDOCTTREE_ELEMENTVERSION 0x0102
#define XR_LIGHTFIELDOCTTREE_VERSION 0x0100

class CBSP2_LightFieldOcttreeNode				// 17 bytes
{
private:
	M_BITFIELD2(uint32, m_SubNodeTypes, 8, m_iSubNodes, 16) m_Data;

public:
	void Read(CCFile* _pF, int _Ver);
	void Write(CCFile* _pF) const;
#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif

	inline uint8  GetSubNodeTypes() const              { return m_Data.m_SubNodeTypes; }
	inline void   SetSubNodeTypes(uint8 _SubNodeTypes) { m_Data.m_SubNodeTypes = _SubNodeTypes; }
	inline uint16 GetSubNodes() const                  { return m_Data.m_iSubNodes; }
	inline void   SetSubNodes(uint16 _iSubNodes)       { m_Data.m_iSubNodes = _iSubNodes; }
};

class CBSP2_LightFieldOcttree : public CReferenceCount, public CXR_LightVolume
{
public:
	TThinArray<CBSP2_LightFieldOcttreeNode> m_lNodes;
	TThinArray<CXR_LightFieldElement> m_lElements;

	uint32 m_RootSize;
	uint32 m_MinLeafSize;
	CVec3Dfp32 m_RootPos;

	void Read(CDataFile* _pF);
	void Write(CDataFile* _pF) const;

	static TPtr<CBSP2_LightFieldOcttree> FindAndRead(CDataFile* _pF);

	// CXR_LightVolume overrides
	virtual class CXR_LightVolume* GetNext() { return NULL; }
	virtual const class CXR_LightVolume* GetNext() const { return NULL; };
	virtual fp32 Light_EvalVertex(const class CXR_LightID* _pIDMap, const CVec3Dfp32& _V, CVec3Dfp32& _LDir, CVec3Dfp32& _LColor);
	virtual CPixel32 Light_Vertex(const class CXR_LightID* _pIDMap, const CVec3Dfp32& _V, const CVec3Dfp32* _pN);
	virtual void Light_Array(const class CXR_LightID* _pIDMap, int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN = NULL, const CMat4Dfp32* _pWMat = NULL, int _Flags = 0);
	virtual void Light_EvalPoint(const CVec3Dfp32& _V, CVec4Dfp32* _pAxis);
	virtual void Light_EvalPointArrayMax(const CVec3Dfp32* _pV, int _nV, CVec4Dfp32* _pAxis);
};

#endif // _INC_XW2COMMON

