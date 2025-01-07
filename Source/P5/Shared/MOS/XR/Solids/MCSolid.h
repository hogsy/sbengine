
#ifndef _INC_MCSolid
#define _INC_MCSolid

#include "../XRClass.h"
#include "../XRSurf.h"

#define CSB_EPSILON 0.001f

#define CSB_TESS_PHYSICS 8

class CXR_SurfaceContext;
class CXR_VBManager;
#ifndef PLATFORM_CONSOLE
class COgrRenderBuffer;
#endif

/*
// -------------------------------------------------------------------
#define CSB_COMPACTVERTEXSIZE 127.0f

class CSB_CompactVertex
{
public:
	int8 m_V[3];
	int8 m_N[3];

	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile);
};

// -------------------------------------------------------------------
class CSB_LightMap : public CReferenceCount
{
public:

	DECLARE_OPERATOR_NEW;


	uint32 m_Width;
	uint32 m_Height;
	CVec3Dfp32 m_Scale;
	CVec3Dfp32 m_ScaleN;
	CVec3Dfp32 m_Offset;
	TArray<CSB_CompactVertex> m_lVertices;

	void Decompress(int _iRow, CVec3Dfp32* _pDestV, CVec3Dfp32* _pDestN, int _nV);

	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile);
};

typedef TPtr<CSB_LightMap> spCSB_LightMap;*/

// -------------------------------------------------------------------
//  CSB_BoundNode
// -------------------------------------------------------------------
class CSB_BoundNode
{
public:
	uint8 m_STMin[2];
	uint8 m_STMax[2];
	CBox3Dfp32 m_Box;
	int16 m_iNodes[2];
};

// -------------------------------------------------------------------
//  CSplineBrush_Edge
// -------------------------------------------------------------------
#define CSB_EDGE_LINE	1
#define CSB_EDGE_LOCKED	2

#define CSB_EDGE_MATROWS 4

class CSplineBrush_Edge
{
public:
	int16 m_Flags;
	int16 m_Tess;		// > 0 == Fixed tesselation.
	fp32 m_Mat[CSB_EDGE_MATROWS][3];
	uint16 m_iV[2];
	int16 m_iFaces[2];	// SIGNED!
	fp32 m_Len;
	fp32 m_dVdT;		// In theory: t = [0..1], Max(Lim(dt->0, arccos((F(t)-F(t-dt) * (F(t+dt) - F(t))) / dt))

	CSplineBrush_Edge()
	{
		m_Flags = 0;
		m_Tess = 0;
		m_iFaces[0] = -1;
		m_iFaces[1] = -1;
	}

	CSplineBrush_Edge(fp32 _Len, const CVec3Dfp32 r0, const CVec3Dfp32& r1 = 0, const CVec3Dfp32& r2 = 0, const CVec3Dfp32& r3 = 0);

	void SetMatrix(const CVec3Dfp32& r0, const CVec3Dfp32& r1 = 0, const CVec3Dfp32& r2 = 0, const CVec3Dfp32& r3 = 0);

	void SetVectors(const CVec3Dfp32 &v0, const CVec3Dfp32 &v1, const CVec3Dfp32 &v2)
	{
		SetMatrix(v0, v1 - v0 + v2, -v2);
	}

	void SetVectors4(const CVec3Dfp32 &v0, const CVec3Dfp32 &v1, const CVec3Dfp32 &dv0, const CVec3Dfp32 &dv1);

	void GetVectors(CVec3Dfp32 &v0, CVec3Dfp32 &v1, CVec3Dfp32 &v2) const
	{
		v0 = CVec3Dfp32(m_Mat[0][0], m_Mat[0][1], m_Mat[0][2]);
		v1 = CVec3Dfp32(m_Mat[1][0], m_Mat[1][1], m_Mat[1][2]);
		v2 = CVec3Dfp32(-m_Mat[2][0], -m_Mat[2][1], -m_Mat[2][2]);
		v1 += v0;
		v1 -= v2;
	}

	void GetVectors4(CVec3Dfp32 &v0, CVec3Dfp32 &v1, CVec3Dfp32 &_dv0, CVec3Dfp32 &_dv1) const;

	void GetBendVector(CVec3Dfp32& _Dst) const
	{
		_Dst.k[0] = -m_Mat[2][0];
		_Dst.k[1] = -m_Mat[2][1];
		_Dst.k[2] = -m_Mat[2][2];
	}

	CVec3Dfp32 GetBendVector() const
	{
		CVec3Dfp32 Dst;
		Dst.k[0] = -m_Mat[2][0];
		Dst.k[1] = -m_Mat[2][1];
		Dst.k[2] = -m_Mat[2][2];
		return Dst;
	}

	void AddFace(int _iFace)
	{
		if (m_iFaces[0] < 0)
			m_iFaces[0] = _iFace;
		else
			m_iFaces[1] = _iFace;
	}

	void DeleteFaceIndex(int _iFace)
	{
		if (m_iFaces[0] == _iFace)
		{
			m_iFaces[0] = m_iFaces[1];
			m_iFaces[0] = -1;
		}
		if (m_iFaces[1] == _iFace)
			m_iFaces[1] = -1;

		if (m_iFaces[0] >= 0 && m_iFaces[0] > _iFace) m_iFaces[0]--;
		if (m_iFaces[1] >= 0 && m_iFaces[1] > _iFace) m_iFaces[1]--;
	}

	inline void AddEdge(CVec3Dfp32& _Dest, fp32 _t, fp32 _Scale) const
	{
	/*	if (_pE->m_Flags & CSB_EDGE_LINE)
		{
			fp32 T0 = _Scale;
			fp32 T1 = _Scale*_t;
			_Dest.k[0] += _pE->m_Mat[0][0]*T0 + _pE->m_Mat[1][0]*T1;
			_Dest.k[1] += _pE->m_Mat[0][1]*T0 + _pE->m_Mat[1][1]*T1;
			_Dest.k[2] += _pE->m_Mat[0][2]*T0 + _pE->m_Mat[1][2]*T1;
		}
		else*/
	/*	{
			if (_t < CSB_TESS_EPSILON)
			{
				_Dest.k[0] += _pE->m_Mat[0][0] * _Scale;
				_Dest.k[1] += _pE->m_Mat[0][1] * _Scale;
				_Dest.k[2] += _pE->m_Mat[0][2] * _Scale;
			}
			else if (_t > 1.0f-CSB_TESS_EPSILON)
			{
				_Dest.k[0] += (_pE->m_Mat[0][0] + _pE->m_Mat[1][0] + _pE->m_Mat[2][0] + _pE->m_Mat[3][0]) * _Scale;
				_Dest.k[1] += (_pE->m_Mat[0][1] + _pE->m_Mat[1][1] + _pE->m_Mat[2][1] + _pE->m_Mat[3][1]) * _Scale;
				_Dest.k[2] += (_pE->m_Mat[0][2] + _pE->m_Mat[1][2] + _pE->m_Mat[2][2] + _pE->m_Mat[3][2]) * _Scale;
			}
			else*/
			{
				fp32 T0 = _Scale;
				fp32 T1 = _Scale*_t;
				fp32 T2 = _Scale*_t*_t;
				fp32 T3 = _Scale*_t*_t*_t;
				_Dest.k[0] += m_Mat[0][0]*T0 + m_Mat[1][0]*T1 + m_Mat[2][0]*T2 + m_Mat[3][0]*T3;
				_Dest.k[1] += m_Mat[0][1]*T0 + m_Mat[1][1]*T1 + m_Mat[2][1]*T2 + m_Mat[3][1]*T3;
				_Dest.k[2] += m_Mat[0][2]*T0 + m_Mat[1][2]*T1 + m_Mat[2][2]*T2 + m_Mat[3][2]*T3;
			}
	}

	inline void AddEdge(CVec3Dfp32& _Dest, fp32 _t) const
	{
		fp32 T1 = _t;
		fp32 T2 = _t*_t;
		fp32 T3 = _t*_t*_t;
		_Dest.k[0] += m_Mat[0][0] + m_Mat[1][0]*T1 + m_Mat[2][0]*T2 + m_Mat[3][0]*T3;
		_Dest.k[1] += m_Mat[0][1] + m_Mat[1][1]*T1 + m_Mat[2][1]*T2 + m_Mat[3][1]*T3;
		_Dest.k[2] += m_Mat[0][2] + m_Mat[1][2]*T1 + m_Mat[2][2]*T2 + m_Mat[3][2]*T3;
	}

	inline void AddEdge_dXYZdT(CVec3Dfp32& _Dest, fp32 _t, fp32 _Scale) const
	{
		fp32 T1 = _Scale;
		fp32 T2 = 2.0f*_Scale*_t;
		fp32 T3 = 3.0f*_Scale*_t*_t;
		_Dest.k[0] += m_Mat[1][0]*T1 + m_Mat[2][0]*T2 + m_Mat[3][0]*T3;
		_Dest.k[1] += m_Mat[1][1]*T1 + m_Mat[2][1]*T2 + m_Mat[3][1]*T3;
		_Dest.k[2] += m_Mat[1][2]*T1 + m_Mat[2][2]*T2 + m_Mat[3][2]*T3;
	}

	void Flip();
	fp32 GetLength(int _SampleResolution = 100.0f);

	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile);
};

// -------------------------------------------------------------------
#define XW_FACE_SYNCLOCKED_S		32		// XW_FACE_SMOOTHNORMALS, allways
#define XW_FACE_SYNCLOCKED_T		64		// XW_FACE_USERPORTAL, never

#define XW_FACE_MAPPING_BIT0		2048
#define XW_FACE_MAPPING_BIT1		4096

#define XW_FACE_PHYSRESSHIFT_BIT0	16384	// Phys tessellation is CSB_TESS_PHYSICS >> ResShift
#define XW_FACE_PHYSRESSHIFT_BIT1	32768

#define XW_FACE_SPLINELOADMASK		(~(XW_FACE_LIGHTMAP | XW_FACE_LIGHTVERTICES | XW_FACE_LIGHTBUMPMAP | XW_FACE_LIGHTMULTIPASS | XW_FACE_USERPORTAL | XW_FACE_SMOOTHNORMALS))

// -------------------------------------------------------------------
#define XW_FACE_MAPPING_SHIFT		11
#define XW_FACE_MAPPING_MASK		3

#define XW_FACE_MAPPING_ST			0
#define XW_FACE_MAPPING_XYZ			1
#define XW_FACE_MAPPING_FIXEDST		2

#define XW_FACE_MAPPINGMATRIXCOLUMNS	2
#define XW_FACE_MAPPINGMATRIXROWS		4

// -------------------------------------------------------------------
class CSplineBrush_Face
{
public:
	uint16 m_Flags;
	unsigned int m_nEdges : 8;
	unsigned int m_PhysTessShiftS : 4;
	unsigned int m_PhysTessShiftT : 4;
	uint32 m_iiEdges;
	uint32 m_iSurface;
	uint32 m_TextureID;		// NOT READ/WRITED-ED.
	uint32 m_iLMInfo;		// LM-Container's lightmap-info.

	uint16 m_nLightInfo;
	uint16 m_iLightInfo;
	uint16 m_iCullPlanes;		// Index into m_lCullPlanes
	uint16 m_nCullPlanes;
	uint16 m_iLightingPlane;	// Index into m_lCullPlanes

	CPixel32 m_Color;

	int32 m_iBoundNode;		// Index into m_lBoundNodes

	// Mapping
	fp32 m_Mapping[4][2];	// 2x4 matrix. (2 columns, 4 rows)

	// NOTE: In the engine the result is UV-coordinates, otherwize Texture-coordinates. (as in pixels)
	//
	// If ST mapping:
	// 
	// [U]       [S]
	// [V] = M * [T]
	//           [1]
	//           [0]
	//
	// If XYZ-mapping:
	// [U]       [X]
	// [V] = M * [Y]
	//           [Z]
	//           [1]

	// Lightmap-Mapping
	fp32 m_LightMapping[2][2];	// 2x2 matrix

	// NOTE: In the engine the result is UV-coordinates, otherwize Texture-coordinates. (as in pixels)
	//
	// [LightU] = [M[0][0], M[1][0]] * [U]
	//			                       [1]
	//
	// [LightV] = [M[0][1], M[1][1]] * [V]
	//			                       [1]

	CBox3Dfp32 m_BoundBox;

	CVec3Dfp32 m_LightUVec;
	CVec3Dfp32 m_LightVVec;

	CXW_Surface* m_pSurface;
	int32 m_DynLightMask;
	uint16 m_iVBQueue;
	uint16 m_iVB;
	uint32 m_iPrim;
	uint16 m_nPrim;
	uint16 m_iVBBaseVertex;

	CSplineBrush_Face()
	{
		m_Flags = 0;
		m_TextureID = 0;
		m_iLightInfo = 0;
		m_iCullPlanes = 0;
		m_nCullPlanes = 0;
		m_iLightingPlane = 0;
		m_iiEdges = 0;
		m_nEdges = 0;
		m_iBoundNode = -1;
#ifndef M_RTM
		m_Color = 0xffffffff;
#endif
		m_pSurface = NULL;
		m_DynLightMask = 0;
		m_iVBQueue = 0;
		m_iVB = 0;
		m_iPrim = 0;
		m_nPrim = 0;
		m_iVBBaseVertex = 0;
		FillChar(&m_Mapping, sizeof(m_Mapping), 0);
	}

	CSplineBrush_Face(int _iiE, int _nE)
	{
		m_Flags = 0;
		m_TextureID = 0;
		m_iLightInfo = 0;
		m_iCullPlanes = 0;
		m_nCullPlanes = 0;
		m_iLightingPlane = 0;
		m_iiEdges = _iiE;
		m_nEdges = _nE;
		m_iBoundNode = -1;
#ifndef M_RTM
		m_Color = 0xffffffff;
#endif
		m_pSurface = NULL;
		m_DynLightMask = 0;
		m_iVBQueue = 0;
		m_iVB = 0;
		m_iPrim = 0;
		m_nPrim = 0;
		m_iVBBaseVertex = 0;
		FillChar(&m_Mapping, sizeof(m_Mapping), 0);
	}

	CVec2Dfp32 EvalUV_ST(const class CSplineBrush* _pSB, fp32 _s, fp32 _t) const;
	CVec2Dfp32 EvalUV_XYZ(const class CSplineBrush* _pSB, const CVec3Dfp32& _v) const;
	CVec2Dfp32 EvalUV(const class CSplineBrush* _pSB, fp32 _s, fp32 _t, const CVec3Dfp32& _v) const;
	CVec2Dfp32 EvalUVLM(fp32 _s, fp32 _t, const CVec3Dfp32& _v) const;
	void EvalUV(int _nV, const CVec3Dfp32* _pV, const CVec2Dfp32* _pST, CVec2Dfp32* _pTV) const;
	void EvalUVLM(int _nV, const CVec3Dfp32* _pV, const CVec2Dfp32* _pST, CVec2Dfp32* _pTV, CVec2Dfp32* _pLMTV) const;
	void TransformMapping(const CVec2Dfp32 &_O, const CVec2Dfp32 &_S, float _Rot);
	void ScaleMapping(const CVec2Dfp32& _Scale);

	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile) const;
};

// -------------------------------------------------------------------
#define CSB_STATIC_TESSELATION		1

class CSplineBrush : public CReferenceCount
{
public:
	TArray<CVec3Dfp32> m_lVertices;
	TArray<CSplineBrush_Edge> m_lEdges;
	TArray<uint32> m_liEdges;
	TThinArray<CPlane3Dfp32> m_lCullPlanes;
	TArray<CSplineBrush_Face> m_lFaces;
	TArray<CStr> m_lFaceSurf;

	TThinArray<fp32> m_lEdgeTess;
	TThinArray<fp32> m_lEdgeMinTess;

	TArray<CSB_BoundNode> m_lBoundNodes;

	int32 m_Flags;
	int32 m_LastViewRendered;
	int32 m_DynLightMask;

	CBox3Dfp32 m_BoundVertexBox;
	CBox3Dfp32 m_BoundBox;
	CVec3Dfp32 m_BoundPos;
	fp32 m_BoundRadius;
	fp32 m_MinTessDistance;

	CMat4Dfp32 m_TextureTransform;

	static CXW_SurfaceKeyFrame m_TmpSurfKeyFrame;


	DECLARE_OPERATOR_NEW;

	CSplineBrush();

	int AddEdge(int _iv0, int _iv1, const CVec3Dfp32& _v);
	int AddEdge4(int _iv0, int _iv1, const CVec3Dfp32& _dv0, const CVec3Dfp32& _dv1);
	int AddQuad(int _ie0, int _ie1, int _ie2, int _ie3);
	void Face_Delete(int _iFace);

	CVec3Dfp32 EvalXYZ(int iFace, fp32 _s, fp32 _t);
	void EvalXYZ(int iFace, fp32 _s, fp32 _t, CVec3Dfp32& _Dst);
	void EvalXYZ_dXYZ(int _iFace, fp32 _s, fp32 _t, CVec3Dfp32& _dXYZdS, CVec3Dfp32& _dXYZdT);
	CVec3Dfp32 EvalNormal(int iFace, fp32 _s, fp32 _t);
	void EvalTangentSpace(int iFace, fp32 _s, fp32 _t, CVec3Dfp32& _N, CVec3Dfp32& _TangU, CVec3Dfp32& _TangV);
	void EvalNormal(int _iFace, int _nV, const CVec2Dfp32* _pST, CVec3Dfp32* _pN, bool _bNormalize);
	void EvalTangentSpaceArrays(int _iFace, int _nV, const CVec2Dfp32* _pST, CVec3Dfp32* _pN, CVec3Dfp32* _pTangU, CVec3Dfp32* _pTangV);

	int EnumBoundNodes_r(int _iFace, int _iNode, const CBox3Dfp32& _Box, uint32* _pST);
	int EnumBoundNodesLine_r(int _iFace, int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, uint32* _pST);
	int EnumBoundNodes(int _iFace, const CBox3Dfp32& _Box, uint32* _pST);
	int EnumBoundNodesLine(int _iFace, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, uint32* _pST);

	void GetBoundBox(int _iFace, int _nS, int _nT, fp32 _s, fp32 _t, fp32 _sStep, fp32 _tStep, CVec3Dfp32& _vMin, CVec3Dfp32& _vMax);
	int BuildBoundNodes_r(int _iFace, int _S, int _T, int _nS, int _nT, fp32 _sStep, fp32 _tStep);
	void BuildBoundNodes();
	void UpdateBoundBox();
	void UpdatePhysTess();

	void CreateFaceMeshes();
	
	void ModelChanged(bool _bCompiledBrush = false);	// True if loaded binary into the engine.
	void CleanUp();

#ifndef M_RTM
	void UpdateSurfaces(CXR_SurfaceContext *_pContext);
	void SetSurface(CXW_Surface *_pSurface, int _iFace);
	CXW_Surface *GetSurface(int _iPlane);
	CStr GetSurfaceName(int _iPlane);
#endif

	void Edge_Flip(int _iE);

	bool TraceLine(const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int& _iFace, CVec3Dfp32* _pPos = NULL);
	bool Phys_IntersectLine(const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);

//	void Face_CalcVertexMap(int _iFace, int _nBumps, int _Width, int _Height);
	bool Face_IsVisible(int _iFace, const CVec3Dfp32& _VP);
	bool Face_IsFlatS(int _iFace);
	bool Face_IsFlatT(int _iFace);
	bool Face_IsPlanar(int _iFace, CPlane3Dfp32& _Plane);
	int Face_GetFence(int _iFace, int _MaxVerts, CVec3Dfp32* _pDst);

	bool Face_TraceLine(int _iFace, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, CVec3Dfp32& _Pos, bool _bIntersectBackSide = true);
	bool Phys_FaceIntersectLine(int _iFace, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, CCollisionInfo* _pCollisionInfo = NULL);

#ifndef PLATFORM_CONSOLE
	bool Ogr_TraceLine(const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int& _iFace, CVec3Dfp32* _pPos = NULL);
	fp32 Ogr_IntersectRay(const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dir);					// > 0  ==  Hit
	int Ogr_IntersectFaces(const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dir, fp32 &_Dist);	// -1 == No hit
#endif

	void GetTesselationSpaceExt(int _iFace, int& _nV, int& _nPrim, int& _nS, int& _nT);
	void GetTesselationSpace(int _iFace, int& _nV, int& _nPrim);

	CVec3Dfp32* Tesselate(int _iFace, int _MaxV, CVec3Dfp32* _pV, CVec2Dfp32* _pTV, int& _nS, int& _nT, CXR_VBManager* _pVBM = NULL);
	void Tesselate(int _iFace, fp32 _nS, fp32 _nT, fp32 _sStep, fp32 _tStep, int _MaxV, CVec3Dfp32* _pV, CVec2Dfp32* _pTV = NULL);
	void Tesselate(fp32 _sSize, fp32 _tSize, int _iFace, CVec3Dfp32* _pV, CVec2Dfp32* _pTV, int _MaxV, int& _nS, int& _nT, const CVec2Dfp32& _UVScale = 1.0f);

	fp32 InitMinTessDistance();
	fp32 GetMinTesselation(const CSplineBrush_Edge* _pE);
	fp32 GetMaxTesselation(const CSplineBrush_Edge* _pE);
	fp32 GetTesselation(const CSplineBrush_Edge* _pE, const CVec3Dfp32& _ViewPos);
	fp32 GetTesselation(const CSplineBrush_Edge* _pE, fp32 _Level);
	void SetAllFacesVisible();
	void InitMinTesselation();
	bool InitTesselation(const CVec3Dfp32& _ViewPos);
	void InitTesselation(fp32 _Level);					// 0..1, 1==Max, 0==InitMinTesselation

#ifndef PLATFORM_CONSOLE
	bool Ogr_InitTesselation(const CVec3Dfp32& _ViewPos);
#endif

	void Parse(CKeyContainer*);
	bool ParseKey(CStr _Key, CStr _Value);
	void GetKeys(CKeyContainer*) const;

	void BuildTestSurfaces();

	void Read(CDataFile* _pDFile);
	void Write(CDataFile* _pDFile);

	CSplineBrush &operator =(const CSplineBrush &);
	void CalcScale(CVec2Dfp32* _pPreScale, bool _bRecord);
	void FixXYZPlane(int _iFace);
	void FixXYZPlanes();
	void Apply(const CMat4Dfp32 &_Mat, bool _bSnap, bool _bTextureLock = false);
	void MoveEdge(int _iIndex, const CVec3Dfp32 &_Bend0, const CVec3Dfp32 &_Bend1);
	void MoveVertex(int _iIndex, const CVec3Dfp32 &_pPos, bool _bHard);
	void MoveTexture(int _iIndex, const CVec2Dfp32 &_O, const CVec2Dfp32 &_S, float _Rot);
	void Snap(float _fGrid);
	CVec2Dfp32 GetMappingScale(int _iFace);
	void GetBoxMappingPlane(int _iFace, fp32 _Rot, CVec3Dfp32& _U, CVec3Dfp32& _V);
	void SetMapping(int _iFace, const CVec2Dfp32 &_O, const CVec2Dfp32 &_S, float _Rot);
	void GetMapping(int _iFace, CVec2Dfp32 &_O, CVec2Dfp32 &_S, float& _Rot, CVec3Dfp32 &_U, CVec3Dfp32 &_V);
	int GetMappingType(int _iFace);
	bool SetMappingType(int _iFace, int _iMapping);

#ifndef PLATFORM_CONSOLE
	int GetMappingTypeOGR(int _iFace);
	bool SetMappingTypeOGR(int _iFace, int _iMappingOGR);
#endif

	TPtr<CSplineBrush> Duplicate() const;

	// Rendering
protected:
#ifndef PLATFORM_CONSOLE
	void InternalRenderFace(CRenderContext* _pRC, CXR_VBManager *_pVBM, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat, int _iFace, CPixel32 _Col, int _iType, CMTime _AnimTime, COgrRenderBuffer &_RenderBuffer);
#endif
	void InternalRenderEdge(CRenderContext* _pRC, int _iEdge, CPixel32 _Col = 0xffffffff);

public:
	void RenderEdges(CRenderContext* _pRC, CMat4Dfp32& _WMat, CMat4Dfp32& _VMat, CPixel32 _Col);
#ifndef PLATFORM_CONSOLE
	void Render(CRenderContext* _pRC, CXR_VBManager *_pVBM, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, CPixel32 _Col, int _iType, CMTime _AnimTime, COgrRenderBuffer &_RenderBuffer);
#endif
};

typedef TPtr<CSplineBrush> spCSplineBrush;


#endif // _INC_MCSolid
