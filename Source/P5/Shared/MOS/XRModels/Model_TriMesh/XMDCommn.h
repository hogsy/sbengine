
#ifndef _INC_XMDCommn
#define _INC_XMDCommn

#include "MCC.h"
#include "../../XR/XRSurf.h"
#include "../../XR/XRSkeleton.h"
#include "../../MOS.h"
#include "../../XR/Solids/XRThinSolid.h"
#include "../../MSystem/Raster/MTextureContainers.h"

#define CTM_VERSION	0x0303

#define CTM_VFRAME_VERSION	0x0205
#define CTM_TVFRAME_VERSION	0x0201
#define CTM_BDINFLUENCE_VERSION	0x0200
#define CTM_MISCENTRY_VERSION 0x0002
#define CTM_CLUSTER_VERSION	0x0201
#define CTM_VARIATION_VERSION 0x0100
#define CTM_CLUSTERREF_VERSION 0x0100
#define CTM_CLSTREE_VERSION	0x0100
#define CTM_VERTEXBUFFER_VERSION 0x0100
#define CTM_BONEDEFORM_VERSION 0x0100

#define CTM_VERTEX_8BIT 0

#define CTM_VERTEX_8BIT 0
#define CTM_VERTEX_16BIT 1
#define CTM_VERTEX_FLOAT 2
#define CTM_MAXLOD 4
#define CTM_MAXNODECHILDREN 8

#define CTM_MAXVERTEXGROUPBONES 4

#define CTM_CREATESHADOWEDGES_NOOPENEDGESWARNINGS 1

#ifdef PLATFORM_CONSOLE
#define DTriMeshArray TThinArray
#else
#define DTriMeshArray TArray
#endif

/*
// -------------------------------------------------------------------

IMAGELIST
	IMAGE
	IMAGE
	:
	:
VERTEXCLUSTERS
	VERTEXCLUSTER
	VERTEXCLUSTER
	:
TVERTEXCLUSTERS
	TVERTEXCLUSTER
	TVERTEXCLUSTER
	:
NODEINDICES
NODES
TRICLUSTERS
TRIANGLES
PRIMITIVES

// -------------------------------------------------------------------
*/
class CTM_BDVertexInfo
{
public:
	uint8 m_Flags;				// Unused
	uint8 m_nBones;
	uint16 m_iBoneInfluence;	// Index into array of CTM_BDInfluence

	void Read(CCFile* _pF);
#ifndef PLATFORM_CONSOLE
	void Write(CCFile* _pF);
#endif

#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif
};

class CTM_BDInfluence
{
public:
	uint16 m_iBone;
	fp32 m_Influence;			// 0..1

	void Read(CCFile* _pF, int _Version);
#ifndef PLATFORM_CONSOLE
	void Write(CCFile* _pF);
#endif
	
	static void ReadArray(CCFile* _pF, int _Version, CTM_BDInfluence* _pBDI, int _nBDI);
};

/*class CTM_BDVertexGroup
{
public:
	uint32 m_iBaseVertex;
	uint32 m_nVertices;
	uint32 m_nBones;
	CTM_BDInfluence m_BoneInfluence[CTM_MAXVERTEXGROUPBONES];

	void Read(CCFile* _pF);
	void Write(CCFile* _pF);
};
*/
// -------------------------------------------------------------------
class CTM_VertexFrame : public CReferenceCount
{
public:
	CBox3Dfp32 m_BoundBox;		// Före m_Translate har adderats.
	fp32 m_BoundRadius;			// Före m_Translate har adderats.
	CVec3Dfp32 m_Translate;
//	fp32 m_Scale;

	fp32 m_Duration; // Added by Mondelore.

#ifdef PLATFORM_CONSOLE
	TThinArrayAlign<CVec3Dfp32, 16> m_lVertices;
#else
	DTriMeshArray<CVec3Dfp32> m_lVertices;
#endif
	DTriMeshArray<CVec3Dfp32> m_lNormals;
	DTriMeshArray<CVec3Dfp32> m_lTangentU;	// Created by CreateTangents, currently no IO
	DTriMeshArray<CVec3Dfp32> m_lTangentV;	// Created by CreateTangents, currently no IO

	uint32 m_FilePos:31;
	uint32 m_bHasVertices:1;
	uint32 m_bFileTangents:1;
	void ReadVertices(CCFile* _pF, const CBox3Dfp32 &_BoundBox);
	void ReadNormals(CCFile* _pF);
	void ReadTangents(CCFile* _pF);

	CTM_VertexFrame()
	{
		m_FilePos = 0;
		m_bHasVertices = 0;
		m_bFileTangents = 0;
	}

	void OldRead(CCFile* _pF);
	void Read(CCFile* _pF, const CBox3Dfp32 &_BoundBox, bool _bDelayLoad);
#ifndef PLATFORM_CONSOLE
	TPtr<CTM_VertexFrame> Duplicate(const int32* _piV = NULL, int _nV = 0);
	void Write(CCFile* _pF, const CBox3Dfp32 &_BoundBox);
#endif

	CTM_VertexFrame &operator = (const CTM_VertexFrame &_Src)
	{
		m_BoundBox = _Src.m_BoundBox;
		m_BoundRadius = _Src.m_BoundRadius;
		m_Translate = _Src.m_Translate;

		m_Duration = _Src.m_Duration;

		m_lVertices.Destroy();
		m_lNormals.Destroy();
		m_lTangentU.Destroy();
		m_lTangentV.Destroy();

		m_lVertices.Add(_Src.m_lVertices);
		m_lNormals.Add(_Src.m_lNormals);
		m_lTangentU.Add(_Src.m_lTangentU);
		m_lTangentV.Add(_Src.m_lTangentV);

		return *this;
	}
};

typedef TPtr<CTM_VertexFrame> spCTM_VertexFrame;

// -------------------------------------------------------------------
class CTM_TVertexFrame : public CReferenceCount
{
public:
	DTriMeshArray<CVec2Dfp32> m_lVertices;
	CVec4Dfp32 m_Bound;

	CTM_TVertexFrame()
	{
		m_FilePos = 0;
		m_bHasVertices = 0;
	}

	CTM_TVertexFrame &operator = (const CTM_TVertexFrame &_Src)
	{
		m_lVertices.Destroy();
		m_lVertices.Add(_Src.m_lVertices);
		return *this;
	}

	uint32 m_FilePos:31;
	uint32 m_bHasVertices:1;
	void ReadVertices(CCFile* _pF);

	void Read(CCFile* _pF, int _Vertsion, bool _bDelayLoad);
#ifndef PLATFORM_CONSOLE
	TPtr<CTM_TVertexFrame> Duplicate(const int32* _piV = NULL, int _nV = 0);
	void Write(CCFile* _pF);
#endif
};

typedef TPtr<CTM_TVertexFrame> spCTM_TVertexFrame;

// -------------------------------------------------------------------
class CTM_Triangle			// 6 bytes
{
public:	
	uint16 m_iV[3];

	CTM_Triangle()
	{
	}

	CTM_Triangle(int _iV0, int _iV1, int _iV2)
	{
		m_iV[0] = _iV0;
		m_iV[1] = _iV1;
		m_iV[2] = _iV2;
	}

	CTM_Triangle(const uint16* _piV)
	{
		m_iV[0] = _piV[0];
		m_iV[1] = _piV[1];
		m_iV[2] = _piV[2];
	}

	bint IsDegenerate() const
	{
		if (m_iV[0] == m_iV[1])
			return true;
		if (m_iV[0] == m_iV[2])
			return true;
		if (m_iV[1] == m_iV[2])
			return true;
		return false;
	}

	bint IsSame(const CTM_Triangle &_Other) const
	{
		if (_Other.m_iV[0] == m_iV[0])
		{
			return _Other.m_iV[1] == m_iV[1] && _Other.m_iV[2] == m_iV[2];

		}
		if (_Other.m_iV[0] == m_iV[1])
		{
			return _Other.m_iV[1] == m_iV[2] && _Other.m_iV[2] == m_iV[0];

		}
		if (_Other.m_iV[0] == m_iV[2])
		{
			return _Other.m_iV[1] == m_iV[0] && _Other.m_iV[2] == m_iV[1];

		}
		return false;
	}

	bool IsReversed(const CTM_Triangle& _Other) const
	{
		if(m_iV[0] == _Other.m_iV[0])
		{
			if(m_iV[1] == _Other.m_iV[2] && m_iV[2] == _Other.m_iV[1])
				return true;
		}
		else if(m_iV[1] == _Other.m_iV[1])
		{
			if(m_iV[0] == _Other.m_iV[2] && m_iV[2] == _Other.m_iV[0])
				return true;
		}
		else if(m_iV[2] == _Other.m_iV[2])
		{
			if(m_iV[1] == _Other.m_iV[0] && m_iV[0] == _Other.m_iV[1])
				return true;
		}

		return false;
	}

	void Read(CDataFile* _pDFile);
#ifndef PLATFORM_CONSOLE
	void Write(CDataFile* _pDFile);
#endif
};

// -------------------------------------------------------------------
class CTM_Triangle32
{
public:	
	uint32 m_iV[3];

	bint IsDegenerate() const
	{
		if (m_iV[0] == m_iV[1])
			return true;
		if (m_iV[0] == m_iV[2])
			return true;
		if (m_iV[1] == m_iV[2])
			return true;
		return false;
	}

	bint IsSame(const CTM_Triangle32 &_Other) const
	{
		if (_Other.m_iV[0] == m_iV[0])
		{
			return _Other.m_iV[1] == m_iV[1] && _Other.m_iV[2] == m_iV[2];

		}
		if (_Other.m_iV[0] == m_iV[1])
		{
			return _Other.m_iV[1] == m_iV[2] && _Other.m_iV[2] == m_iV[0];

		}
		if (_Other.m_iV[0] == m_iV[2])
		{
			return _Other.m_iV[1] == m_iV[0] && _Other.m_iV[2] == m_iV[1];

		}
		return false;
	}

	bool IsReversed(const CTM_Triangle32& _Other) const
	{
		if(m_iV[0] == _Other.m_iV[0])
		{
			if(m_iV[1] == _Other.m_iV[2] && m_iV[2] == _Other.m_iV[1])
				return true;
		}
		else if(m_iV[1] == _Other.m_iV[1])
		{
			if(m_iV[0] == _Other.m_iV[2] && m_iV[2] == _Other.m_iV[0])
				return true;
		}
		else if(m_iV[2] == _Other.m_iV[2])
		{
			if(m_iV[1] == _Other.m_iV[0] && m_iV[0] == _Other.m_iV[1])
				return true;
		}

		return false;
	}

	CTM_Triangle32()
	{
	}

	CTM_Triangle32(int _iV0, int _iV1, int _iV2)
	{
		m_iV[0] = _iV0;
		m_iV[1] = _iV1;
		m_iV[2] = _iV2;
	}

	CTM_Triangle32(const uint32* _piV)
	{
		m_iV[0] = _piV[0];
		m_iV[1] = _piV[1];
		m_iV[2] = _piV[2];
	}

	void Read(CDataFile* _pDFile);
#ifndef PLATFORM_CONSOLE
	void Write(CDataFile* _pDFile);
#endif
};

// -------------------------------------------------------------------
class CTM_Edge
{
public:
	uint16 m_iV[2];
};

class CTM_EdgeTris
{
public:
	uint16 m_iTri[2];
};

class CTM_TriEdges
{
public:
	uint16 m_iEdges[3];
};

// -------------------------------------------------------------------
enum
{
//	CTM_CLUSTERCOMP_VB = 1,
//	CTM_CLUSTERCOMP_VERTICES = 2,
//	CTM_CLUSTERCOMP_PRIMITIVES = 4,
//	CTM_CLUSTERCOMP_MIW = 8,

//	CTM_CLUSTERHIBERNATE_NODESTROY = 1,
//	CTM_CLUSTERHIBERNATE_RELEASE_VERTICES = 2,
//	CTM_CLUSTERHIBERNATE_RELEASE_PRIMITIVES = 4,

	CTM_CLUSTERFLAGS_SHADOWVOLUME = 1,
	CTM_CLUSTERFLAGS_SHADOWVOLUMESOFTWARE = 2,
	CTM_CLUSTERFLAGS_DUALSIDED = 4,
	CTM_CLUSTERFLAGS_BACKLIGHTSELFSHADOW = 8,
};

class CTM_ShadowData;
typedef TPtr<CTM_ShadowData> spCTM_ShadowData;

class CTM_Cluster
{
public:
	CTM_Cluster();
	struct { // Bundled together for easier VPU access
		uint16 m_nIBPrim;
		uint16 m_iVBVert;
		uint32 m_iVB:10;
		uint32 m_iIB:10;
		uint32 m_Flags:4;
		uint32 m_OcclusionIndex:8;
	};
	uint32 m_iSurface;
	uint16 m_iIBOffset;
	uint16 m_nVBVert;
	uint16 m_iEdges;
	uint16 m_nEdges;

	CBox3Dfp32 m_BoundBox;
	fp32 m_BoundRadius;

	TThinArray<uint16> m_lBDMatrixMap;
	int32 m_iSharedMatrixMap;

	dllvirtual int GetNumBDMatrixMap(class CTriangleMeshCore* _pTriCore);
	dllvirtual uint16 *GetBDMatrixMap(class CTriangleMeshCore* _pTriCore);

	void Read(CDataFile* _pDFile, uint _Version);
#ifndef PLATFORM_CONSOLE
	void Write(CDataFile* _pDFile);
#endif
};

class CTM_VertexBuffer : public CReferenceCount
{
public:
	TThinArray<CTM_VertexFrame> m_lVFrames;
	TThinArray<CTM_TVertexFrame> m_lTVFrames;

#ifdef PLATFORM_PS3
	TThinArrayAlign<CTM_BDVertexInfo, 16> m_lBDVertexInfo;
#else
	DTriMeshArray<CTM_BDVertexInfo> m_lBDVertexInfo;		// Per vertex bone-deformation
#endif

#ifdef PLATFORM_PS3
	TThinArrayAlign<CTM_BDInfluence, 16> m_lBDInfluence;
#else
	DTriMeshArray<CTM_BDInfluence> m_lBDInfluence;			// Per vertex bone-deformation
#endif
//	TThinArray<uint16> m_lBDMatrixMap;
//	int32 m_iSharedMatrixMap;

	TThinArray<uint32> m_lMatrixIndices;
	TThinArray<fp32> m_lMatrixWeights;

#ifdef PLATFORM_PS3
	TThinArrayAlign<CTM_Triangle, 16> m_lTriangles;
#else
	DTriMeshArray<CTM_Triangle> m_lTriangles;
#endif

	DTriMeshArray<uint16> m_lPrimitives;

	DTriMeshArray<CTM_Edge> m_lEdges;
#ifdef PLATFORM_PS3
	TThinArrayAlign<CTM_EdgeTris,16> m_lEdgeTris;
#else
	DTriMeshArray<CTM_EdgeTris> m_lEdgeTris;
#endif
	DTriMeshArray<CTM_TriEdges> m_lTriEdges;

#ifndef PLATFORM_CONSOLE
	DTriMeshArray<CTM_Triangle32> m_lTriangles32;
	void CreateTriFromTri32();

	DTriMeshArray<uint8> m_lShadowGroup;
#endif

//	CBox3Dfp32 m_BoundBox;
//	fp32 m_BoundRadius;
	fp32 m_TotalDuration;

	uint32 m_VBID;
//	uint32 m_iSurface;

	uint32 m_FileVersion;
	uint32 m_FilePosBoneDeform;
	uint32 m_FilePosTriangles;
	uint32 m_FilePosPrimitives;
	uint32 m_FilePosEdges;
	uint32 m_FilePosEdgeTris;
	uint32 m_FilePosTriEdges;
	uint32 m_bDelayLoad:1;
	uint32 m_bSave_Vertices:1;
	uint32 m_bSave_Normals:1;
	uint32 m_bSave_TangentU:1;
	uint32 m_bSave_TangentV:1;
	uint32 m_bSave_TVertices:1;
	uint32 m_bSave_BoneDeform:1;
	uint32 m_bSave_BoneDeformMatrixMap:1;
	uint32 m_bSave_VBBoneDeform:1;
	uint32 m_bSave_Triangles:1;
	uint32 m_bSave_Primitives:1;
	uint32 m_bSave_Edges:1;
	uint32 m_bSave_EdgeTris:1;
	uint32 m_bSave_TriEdges:1;
	uint32 m_bHaveBones:1;
	uint32 m_bHaveBoneMatrixMap:1;

	uint32 m_VBFlags:4;
//	uint32 m_OcclusionIndex:8;
	

	CTM_VertexBuffer();

	CTM_VertexBuffer & operator = (const CTM_VertexBuffer &_Src)
	{
		m_lVFrames.Destroy();
		m_lTVFrames.Destroy();
		m_lBDVertexInfo.Destroy();
		m_lBDInfluence.Destroy();
//		m_lBDMatrixMap.Destroy();

		m_lMatrixIndices.Destroy();
		m_lMatrixWeights.Destroy();

		m_lTriangles.Destroy();
	#ifndef PLATFORM_CONSOLE
		m_lTriangles32.Destroy();
		m_lShadowGroup.Destroy();
	#endif

		m_lPrimitives.Destroy();
		m_lEdges.Destroy();
		m_lTriEdges.Destroy();
		m_lEdgeTris.Destroy();


		m_lVFrames.Destroy();
		m_lTVFrames.Destroy();

		m_TotalDuration = _Src.m_TotalDuration;

		m_lBDVertexInfo.Add(_Src.m_lBDVertexInfo);		// Per vertex bone-deformation
		m_lBDInfluence.Add(_Src.m_lBDInfluence);			// Per vertex bone-deformation
//		m_lBDMatrixMap.Add(_Src.m_lBDMatrixMap);

		m_lMatrixIndices.Add(_Src.m_lMatrixIndices);
		m_lMatrixWeights.Add(_Src.m_lMatrixWeights);

		m_lTriangles.Add(_Src.m_lTriangles);
	#ifndef PLATFORM_CONSOLE
		m_lTriangles32.Add(_Src.m_lTriangles32);
		m_lShadowGroup.Add(_Src.m_lShadowGroup);
	#endif

		m_lPrimitives.Add(_Src.m_lPrimitives);

		// Edge information, CreateEdges must be run to get this data.
		
		m_lEdges.Add(_Src.m_lEdges);
		m_lTriEdges.Add(_Src.m_lTriEdges);
		m_lEdgeTris.Add(_Src.m_lEdgeTris);
//		m_iSharedMatrixMap = _Src.m_iSharedMatrixMap;
//		m_OcclusionIndex = _Src.m_OcclusionIndex;

		m_VBID = _Src.m_VBID;

		m_VBFlags = _Src.m_VBFlags;
//		m_iSurface = _Src.m_iSurface;

//		m_BoundRadius = _Src.m_BoundRadius;
//		m_BoundBox = _Src.m_BoundBox;

		return *this;
	}

#ifndef PLATFORM_CONSOLE
	dllvirtual TPtr<CTM_VertexBuffer> DuplicateVertices(class CTriangleMeshCore* _pTriCore, const int32* _piV = NULL, int _nV = 0);		// Doesn't copy triangles.
	dllvirtual TPtr<CTM_VertexBuffer> Duplicate(class CTriangleMeshCore* _pTriCore);
	void SetVFrames(int _nFrames);
	void SetTVFrames(int _nFrames);
#endif


	void ReadBoneDeform_v2(CCFile* _pF);
	void ReadBoneDeformOld(CCFile* _pF, CTM_Cluster* _pC);
	void ReadBoneDeformOldBDMatrix(CCFile* _pF, CTM_Cluster* _pC);		// Legacy crap :(
	void ReadTriangles(CCFile* _pF);
	void ReadPrimitives(CCFile* _pF);
	void ReadEdges(CCFile* _pF);
	void ReadEdgeTris(CCFile* _pF);
	void ReadTriEdges(CCFile* _pF);

	dllvirtual int GetNumVertices(class CTriangleMeshCore* _pTriCore, CCFile * _pFile = NULL);
	dllvirtual int GetNumPrimitives(class CTriangleMeshCore* _pTriCore, CCFile * _pFile = NULL);
	dllvirtual int GetNumTriangles(class CTriangleMeshCore* _pTriCore, CCFile * _pFile = NULL);
	dllvirtual int GetNumEdges(class CTriangleMeshCore* _pTriCore, CCFile * _pFile = NULL);
	dllvirtual int GetNumTriEdges(class CTriangleMeshCore* _pTriCore, CCFile * _pFile = NULL);
	dllvirtual int GetNumEdgeTris(class CTriangleMeshCore* _pTriCore, CCFile * _pFile = NULL);
	dllvirtual int GetNumBDVertexInfo(class CTriangleMeshCore* _pTriCore, CCFile * _pFile = NULL);
	dllvirtual int GetNumBDInfluence(class CTriangleMeshCore* _pTriCore, CCFile * _pFile = NULL);

	dllvirtual CVec3Dfp32* GetVertexPtr(class CTriangleMeshCore* _pTriCore, int _iFrame, CCFile * _pFile = NULL);
	dllvirtual CVec3Dfp32* GetNormalPtr(class CTriangleMeshCore* _pTriCore, int _iFrame, CCFile * _pFile = NULL);
	dllvirtual CVec2Dfp32* GetTVertexPtr(class CTriangleMeshCore* _pTriCore, int _iFrame, CCFile * _pFile = NULL);
	dllvirtual CVec3Dfp32* GetTangentUPtr(class CTriangleMeshCore* _pTriCore, int _iFrame, CCFile * _pFile = NULL);
	dllvirtual CVec3Dfp32* GetTangentVPtr(class CTriangleMeshCore* _pTriCore, int _iFrame, CCFile * _pFile = NULL);
	dllvirtual uint16 *GetTriangles(class CTriangleMeshCore* _pTriCore, CCFile * _pFile = NULL);
	dllvirtual uint16 *GetPrimitives(class CTriangleMeshCore* _pTriCore, CCFile * _pFile = NULL);
	dllvirtual CTM_Edge *GetEdges(class CTriangleMeshCore* _pTriCore, CCFile * _pFile = NULL);
	dllvirtual CTM_TriEdges *GetTriEdges(class CTriangleMeshCore* _pTriCore, CCFile * _pFile = NULL);
	dllvirtual CTM_EdgeTris *GetEdgeTris(class CTriangleMeshCore* _pTriCore, CCFile * _pFile = NULL);
	dllvirtual CTM_BDVertexInfo *GetBDVertexInfo(class CTriangleMeshCore* _pTriCore, CCFile * _pFile = NULL);
	dllvirtual CTM_BDInfluence *GetBDInfluence(class CTriangleMeshCore* _pTriCore, CCFile * _pFile = NULL);

	dllvirtual int GetNumFrames();
	dllvirtual int GetNumTFrames();

#ifndef PLATFORM_CONSOLE
	dllvirtual CTM_VertexFrame* GetFrame(int _iFrame);
	dllvirtual CTM_TVertexFrame* GetTFrame(int _iFrame);
#endif

	// Added by Mondelore.
	void GetFrameAndTimeFraction(const CMTime& _Time, int& _iFrame0, int& _iFrame1, fp32& _Fraction);

//	CTM_Vertex_fp32* GetVertexPtr(int _iFrame);

	void Read(class CTriangleMeshCore* _pTriCore, CDataFile* _pDFile, int _Version);
	void ReadOld(class CTriangleMeshCore* _pTriCore, class CTM_Cluster* _pC, CDataFile* _pDFile, int _bOldVersion);

#ifndef PLATFORM_CONSOLE
	void Write(class CTriangleMeshCore* _pTriCore, CDataFile* _pDFile);
	int AddEdge(class CTriangleMeshCore* _pTriCore, int _iv0, int _iv1, int _iTri);
	int AddShadowEdge(class CTriangleMeshCore* _pTriCore, int _iv0, int _iv1, int _iTri, TArray<int> _lHideIndex);
	int AddEdge(class CTriangleMeshCore* _pTriCore, int _iv0, int _iv1, int _iTri, int *_iSubCluster);
	void CreateEdges(class CTriangleMeshCore* _pTriCore);
	void CreateShadowEdges(class CTriangleMeshCore* _pTriCore, CWireContainer* _pDebugWires, TArray<int> _lHideIndex, int _Flags);
	void CreateEdges(class CTriangleMeshCore* _pTriCore, int *_iSubCluster);
#endif
 
	void CreateMatrixIW(class CTriangleMeshCore* _pTriCore, CCFile * _pFile = NULL);
	void InternalCreateTangents(class CTriangleMeshCore* _pTriCore, CCFile* _pFile = NULL);

//	bool BruteForceIntersect(class CTriangleMeshCore* _pTriCore, const CVec3Dfp32 &_Start,const CVec3Dfp32 &_End,int _iFrame = 0);
//	bool BruteForceIntersect(class CTriangleMeshCore* _pTriCore, const CVec3Dfp32 &_Start,const CVec3Dfp32 &_End,CVec3Dfp32 &_Pos,int _iFrame = 0);

	void SetMaxBoneInfluenceNum(uint8 _nInf = 4);
};

typedef TPtr<CTM_VertexBuffer> spCTM_VertexBuffer;

class CTM_ShadowData : public CReferenceCount
{
public:
	TThinArrayAlign<CTM_Edge, 16>		m_lEdge;
	TThinArrayAlign<int32, 16>			m_lEdgeCluster;		// Number of edges for each cluster
	TThinArrayAlign<CTM_Triangle, 16>	m_lTriangle;
	TThinArrayAlign<int32, 16>			m_lTriangleCluster; // Number of Triangles for each cluster
	class CRenderData
	{
	public:
		uint16 m_iShadowCluster;
		uint16 m_iRenderCluster;
	};
	TThinArrayAlign<CRenderData, 16>	m_lRenderData; // Indexed with cluster Index
	TThinArrayAlign<int, 16> m_lnVertices;
	TThinArrayAlign<int, 16> m_lnVBVertices;

	CTM_Cluster m_Cluster;
	CTM_VertexBuffer m_VertexBuffer;

	void Read(class CTriangleMeshCore* _pTriCore, CDataFile* _pDFile, int _Version);
#ifndef PLATFORM_CONSOLE
	void Write(class CTriangleMeshCore* _pTriCore, CDataFile* _pDFile);
#endif

// Collision stuff
	class CBTNode
	{
	public:

		// Note that box coordinates are stored in center-dim instead of min-max
		uint8		m_lBox[6];
		uint16		m_iChildren;
		uint16		m_iiTriangles;
		uint16		m_nTriangles;

#ifndef PLATFORM_CONSOLE	
		void Write(CCFile* _pF);
#endif
		void Read(CCFile* _pF,int _Version);
		void SwapLE();
	};

	fp32	m_InvBoundRadius;

	TThinArray<CBTNode>	m_lBoxNodes;
	TThinArray<uint16>	m_liBTTriangles;

#ifndef PLATFORM_CONSOLE
	void CreateBoxTree(class CTriangleMeshCore* _pTriCore, uint8 _MaxGeneration = 0,uint16 _MinFaces = 8,int _iFrame = 0);
#endif

	bool IntersectLine(class CTriangleMeshCore* _pTriCore, const CVec3Dfp32 & _Start,const CVec3Dfp32 & _End,uint16 _Mask = 0,int _iFrame = 0);
	bool IntersectLine(class CTriangleMeshCore* _pTriCore, const CVec3Dfp32 & _Start,const CVec3Dfp32 & _End,CVec3Dfp32 &_Pos,CPlane3Dfp32 &_Plane, uint16 _Mask,int * _piCluster,int _iFrame = 0);

#ifndef M_RTM
	bool IsBoxTreeValid() const;
#endif
};

// -------------------------------------------------------------------
class CTM_Variation
{
public:
	CStr m_Name;
	uint16 m_nClusters;
	uint16 m_iClusterRef;

	void Read(CCFile* _pF, int _Version);
#ifndef PLATFORM_CONSOLE
	void Write(CCFile* _pF);
#endif
};

class CTM_ClusterRef
{
public:
	uint16 m_iCluster;
	uint16 m_iSurface;

	void Read(CCFile* _pF, int _Version);
#ifndef PLATFORM_CONSOLE
	void Write(CCFile* _pF);
#endif
	void SwapLE();
};

// -------------------------------------------------------------------
/*
#define CTM_NODE_VERSION		0x0102

class CTM_Node
{
public:
	CVec3Dfp32 m_LocalCenter;
	uint16 m_Flags;
	int16 m_iiNodeChildren;
	int16 m_nChildren;
	int16 m_iNodeParent;
	fp32 m_RotationScale;
	fp32 m_MovementScale;
	int16 m_iRotationSlot;
	int16 m_iMovementSlot;
	int16 m_iVFrameCounter;
	int16 m_iTVFrameCounter;
	int16 m_iVertexCluster[CTM_MAXLOD];

	CTM_Node();
	void Read(CDataFile* _pDFile);
	void Write(CDataFile* _pDFile);
};

// -------------------------------------------------------------------
#define CTM_ATTACH_VERSION		0x0102

class CTM_AttachPointInfo
{
	uint16 m_iNode;
	uint16 __m_Padding;
	CVec3Dfp32 m_Vertex;
};
*/
// -------------------------------------------------------------------

/*
enum
{
	CTM_MESHFLAGS_EDGES			= 1,
	CTM_MESHFLAGS_MATRIXIW		= 2,	// Bone-deformation arrays for hw transform.
	CTM_MESHFLAGS_TANGENTS		= 4,	// Tangent space vectors for per-pixel lighting.
	CTM_MESHFLAGS_COMPRESSED	= 8,	// Model is based on compressed clusters
	CTM_MESHFLAGS_VERTEXANIM	= 16,
	CTM_MESHFLAGS_TVERTEXANIM	= 32,
	CTM_MESHFLAGS_SKELETONANIM	= 64,
	CTM_MESHFLAGS_SHADOW		= 256,	// Mesh optimized for render-to-texture shadows

	CTM_MESHFLAGS_IO_MASK		= ~(CTM_MESHFLAGS_COMPRESSED | CTM_MESHFLAGS_EDGES | CTM_MESHFLAGS_MATRIXIW | CTM_MESHFLAGS_TANGENTS)
};
*/

class CTriangleMeshCore
{
private:
	/*
#if defined(PLATFORM_DOLPHIN) && !defined(USE_VIRTUAL_MEMORY)
	CBlockManager::CBlock* m_pCompressedClusters;
	uint32                 m_nCompressedClusterSize;
#else
	TArray<uint8>          m_lCompressedClusters;
#endif
*/
public:
	DTriMeshArray<fp32> m_lLODBias;


#ifdef PLATFORM_CONSOLE
	DTriMeshArray<CTM_VertexBuffer> m_lVertexBuffers;
#else
	DTriMeshArray<spCTM_VertexBuffer> m_lspVertexBuffers;
#endif
	DTriMeshArray<CTM_Variation> m_lVariations;
	DTriMeshArray<CTM_ClusterRef> m_lClusterRefs;
	DTriMeshArray<CTM_Cluster> m_lClusters;

//	fp64 m_ClusterTouchTime;
//	fp32 m_ClusterTouchTime;	// JK-NOTE: Do we _really_ need fp64 precision for this?
//	int m_nClusters;
//	int m_nActiveClusters;	// Number of clusters that are decompressed (when 0 we can release the cluster list)

	CStr m_FileName;

	spCTM_ShadowData m_spShadowData;

	DTriMeshArray<spCXW_Surface> m_lspSurfaces;
	TThinArray<CXR_ThinSolid> m_lSolids;

	TThinArray<TCapsule<fp32> > m_lCapsules;

	spCXR_Skeleton m_spSkeleton;

	uint32 m_RenderFlags;
//	uint32 m_MeshFlags;
	uint32 m_bVertexAnim:1;
	uint32 m_bTVertexAnim:1;
	uint32 m_bMatrixPalette:1;
	uint32 m_iLOD:4;
#ifndef M_RTM
	CStr m_MeshName;
#endif
	const char* GetMeshName();

	fp32 m_BoundRadius;
	fp32 m_GlobalScale;
	CBox3Dfp32 m_BoundBox;

/*	TThinArray<int16> m_liNodes;
	TThinArray<CTM_Node> m_lNodes;
	CTM_AttachPointInfo* m_pAttachPoints;*/

	spCTextureContainer_Plain m_spTC;
#ifndef	PLATFORM_CONSOLE
	CKeyContainer m_Keys;
#endif

	CTriangleMeshCore();
	~CTriangleMeshCore();

	void Clear();
#ifndef PLATFORM_CONSOLE
	void Write(CDataFile* _pDFile);
#endif
	void Read(CDataFile* _pDFile);

//	void DetermineStatic();
//	virtual bool DecompressAll(int _Flags = 0);

#ifndef PLATFORM_CONSOLE
	void CreateTriFromTri32();
#endif
	dllvirtual void CreateDefaultVariation();

	dllvirtual CTM_Cluster* GetCluster(int _iC);
	CTM_VertexBuffer* GetVertexBuffer(int _iVB);

	M_INLINE int GetNumClusters()
	{
		return m_lClusters.Len();
	}

	M_INLINE int GetNumVertexBuffers()
	{
#ifdef PLATFORM_CONSOLE
		return m_lVertexBuffers.Len();
#else
        return m_lspVertexBuffers.Len();
#endif
	}

	void ClearVertexBuffers()
	{
#ifdef PLATFORM_CONSOLE
		m_lVertexBuffers.Clear();
#else
		m_lspVertexBuffers.Clear();
#endif
	}

	bool HasBoxTree() const
	{
		return ( (m_spShadowData != NULL) && (m_spShadowData->m_lBoxNodes.Len() > 0) );
	}

//	void CreateEdges();
//	void CreateBDArrays();
//	void CreateTangents();
//	void CreateMatrixIW();

	/*
	bool HaveCompressedClusters() const;
	void DestroyCompressedClusters();
	*/
};

// -------------------------------------------------------------------
#endif // _INC_XMDCommn
