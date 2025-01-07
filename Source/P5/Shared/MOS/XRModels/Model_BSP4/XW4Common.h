// -------------------------------------------------------------------
//  The XW Fileformat
// -------------------------------------------------------------------

#ifndef _INC_XW4COMMON
#define _INC_XW4COMMON

#include "../MCC/MCC.h"
#include "../../XR/XRClass.h"
#include "../../XR/Solids/XRThinSolid.h"

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
		[FACES],			CBSP4_Face
		[FACEINDICES],		uint32
		[BSPNODES],			CBSP4_Node
		[PORTALS],			CBSP4_Portal
		[MAPPINGS],			CXR_PlaneMapping
		[LIGHTMAPSINFO],	CBSP4_LightMapInfo
		[LIGHTMAPS],		-
		[LIGHTVOLUMEINFO],	CBSP4_LightVolumeInfo
		[LIGHTVOLUME],		-
		[LIGHTVERTINFO],	CBSP4_LightVerticeInfo
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

class CBSP4_CoreFace
{
public:
//	int32 m_iiEdges;

	uint32 m_iiVertices : 24;	// Index to first vertex-index
	uint32 m_nVertices : 8;		// Number vertices
//	uint16 m_iMapping;
	uint16 m_iSurface;
	uint16 m_Flags;

	uint32 m_iPlane;
//	uint16 m_iiNormals;		// Index to first normal-index
	uint16 m_iBackMedium;	// What's behind the face.
//	uint16 m_iFrontMedium;	// What's in front of the face.

//	uint16 m_iLightInfo;	// Index of first lightmap/lightvertices
//	uint16 m_nLightInfo;

	CBSP4_CoreFace();
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


class CBSP4_Node					// 14 bytes
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

	uint32 m_iiFaces;
	uint32 m_iPlane;			// 0  -> This is a leaf, otherwize a node


	inline bool IsNode() const { return (m_iPlane != 0); }
	inline bool IsLeaf() const { return (m_iPlane == 0); }
	inline bool IsStructureLeaf() const { return false; }
	inline bool IsNavigationLeaf() const { return false; }

	CBSP4_Node()
	{
		m_iPlane = 0;

		m_iNodeFront = 0;
		m_iNodeBack = 0;

//		m_iNodeParent = 0;
		m_iiFaces = 0;
//		m_Bound.m_iBoundNodeBack = 0;

//		m_Flags = 0;
//		m_iPortalLeaf = 0;
	}

	void GetPortalInfo(const CBSP4_Node& _SrcNode)
	{
//		m_Flags |= _SrcNode.m_Flags & (XW_NODE_PORTAL | XW_NODE_STRUCTURE | XW_NODE_NAVIGATION);
//		m_iPortalLeaf = _SrcNode.m_iPortalLeaf;
//		m_iNavigationLeaf = _SrcNode.m_iNavigationLeaf;
//		m_nPortals = _SrcNode.m_nPortals;
//		m_iiPortals = _SrcNode.m_iiPortals;
	}

	CStr GetString() const
	{
		MAUTOSTRIP(CBSP4_Node_GetString, CStr());
		return CStrF("iPlane %d, iFront %d, iBack %d, iiF %d",
			m_iPlane, m_iNodeFront, m_iNodeBack, m_iiFaces);
	}

	void Read(CCFile* _pF, int _Ver);
	void Write(CCFile* _pF) const;
};


#endif // _INC_XW4COMMON

