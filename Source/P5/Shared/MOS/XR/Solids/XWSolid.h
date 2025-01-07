
#ifndef _INC_XWSOLID
#define _INC_XWSOLID

#ifdef PLATFORM_CONSOLE
#error "This file should not be included on a console"
#endif

//#include "../XWCommon.h"
#include "../XRClass.h"
#include "../XRSurf.h"
#include "MDA_Hash2D.h"

class CRenderContext;
class CXR_SurfaceContext;
class CXR_VBManager;
class COgrRenderBuffer;
class CXR_PlaneMapping;

// Sync with Node.h
#define CSOLID_FLAGS_STRUCTURE	1
#define CSOLID_FLAGS_DETAIL		2
#define CSOLID_FLAGS_NAVIGATION	128
#define CSOLID_FLAGS_ALLTYPEBITS (CSOLID_FLAGS_NAVIGATION | CSOLID_FLAGS_STRUCTURE)

#define CSOLID_OPERATOR_OR		0
#define CSOLID_OPERATOR_AND		1
#define CSOLID_OPERATOR_ANDNOT	2

#define CSOLID_MAXPLANES 2048

enum
{
	CSOLID_MODELMASK_DEFAULT = 1,
	CSOLID_MODELMASK_DYNAMICS = 2,

	CSOLID_MODELMASK_ALLBITS = 0xffff,
};

// -------------------------------------------------------------------
//  CSolid_Mapping
// -------------------------------------------------------------------
class CSolid_Mapping;
typedef TPtr<CSolid_Mapping> spCSolid_Mapping;

class CSolid_Mapping : public CReferenceCount
{
public:
	DECLARE_OPERATOR_NEW;
	

	CXW_Surface* m_pSurface;
	CStr m_SurfName;

	CSolid_Mapping();

	virtual void GenerateUV(const CPlane3Dfp64* _pPlane, const CVec3Dfp64* _pV, const uint16* _piV, int _nV, CVec2Dfp32* _pDestTV) pure;
	virtual void MoveTexture(const CPlane3Dfp64 &_Plane, const CVec2Dfp32 &_O, const CVec2Dfp32 &_S, float _Rot, const CVec3Dfp32 *_Ref) {}
	virtual void GetTextureParams(const CPlane3Dfp64 &_Plane, CVec2Dfp32 &_O, CVec2Dfp32 &_S, float &_Rot, CVec3Dfp32 &_U, CVec3Dfp32 &_V) {};

	virtual bool IsEqual(const CSolid_Mapping* _pMap) pure;

	virtual void SetMappingPlane(CXR_PlaneMapping& _Mapping, const CPlane3Dfp64& _Plane) pure;
	virtual CXR_PlaneMapping GetMappingPlane(const CPlane3Dfp64& _Plane) pure;
	virtual int GetMemorySize() pure;
	virtual int GetMemoryUse() pure;

	virtual CFStr GetKey() const pure;

	virtual spCSolid_Mapping Duplicate() const;

	virtual int GetType() const pure;

	MACRO_OPERATOR_TPTR(CSolid_Mapping);

protected:
	void InternalDuplicate(CSolid_Mapping *) const;
};

// -------------------------------------------------------------------
/*class CSolid_MappingXW : public CSolid_Mapping
{
public:

	virtual GenerateUV(const CPlane3Dfp64* _pPlane, const CVec3Dfp64* _pV, const uint32* _piV, int _nV, CVec2Dfp32* _pDestTV);
};
*/
// -------------------------------------------------------------------
//  CSolid_MappingMAP
// -------------------------------------------------------------------
class CSolid_MappingMAP;
typedef TPtr<CSolid_MappingMAP> spCSolid_MappingMAP;

class CSolid_MappingMAP : public CSolid_Mapping
{
public:
	CXR_BoxMapping m_MapMapping;

	virtual spCSolid_Mapping Duplicate() const;

	virtual void GenerateUV(const CPlane3Dfp64* _pPlane, const CVec3Dfp64* _pV, const uint16* _piV, int _nV, CVec2Dfp32* _pDestTV);
	virtual void MoveTexture(const CPlane3Dfp64 &_Plane, const CVec2Dfp32 &_O, const CVec2Dfp32 &_S, float _Rot, const CVec3Dfp32 *_Ref);
	virtual void GetTextureParams(const CPlane3Dfp64 &_Plane, CVec2Dfp32 &_O, CVec2Dfp32 &_S, float &_Rot, CVec3Dfp32 &_U, CVec3Dfp32 &_V);

	virtual bool IsEqual(const CSolid_Mapping* _pMap);

	virtual void SetMappingPlane(CXR_PlaneMapping& _Mapping, const CPlane3Dfp64& _Plane);
	virtual CXR_PlaneMapping GetMappingPlane(const CPlane3Dfp64& _Plane);
	virtual int GetMemorySize();
	virtual int GetMemoryUse();

	virtual int GetType() const;
	virtual CFStr GetKey() const;
};

// -------------------------------------------------------------------
//  CSolid_MappingPlane
// -------------------------------------------------------------------
class CSolid_MappingPlane;
typedef TPtr<CSolid_MappingPlane> spCSolid_MappingPlane;

class CSolid_MappingPlane : public CSolid_Mapping
{
public:
	CXR_PlaneMapping m_PlaneMap;

	virtual spCSolid_Mapping Duplicate() const;

	virtual void GenerateUV(const CPlane3Dfp64* _pPlane, const CVec3Dfp64* _pV, const uint16* _piV, int _nV, CVec2Dfp32* _pDestTV);
	virtual void MoveTexture(const CPlane3Dfp64 &_Plane, const CVec2Dfp32 &_O, const CVec2Dfp32 &_S, float _Rot, const CVec3Dfp32 *_Ref);
	virtual void GetTextureParams(const CPlane3Dfp64 &_Plane, CVec2Dfp32 &_O, CVec2Dfp32 &_S, float &_Rot, CVec3Dfp32 &_U, CVec3Dfp32 &_V);

	virtual bool IsEqual(const CSolid_Mapping* _pMap);

	virtual void SetMappingPlane(CXR_PlaneMapping& _Mapping, const CPlane3Dfp64& _Plane);
	virtual CXR_PlaneMapping GetMappingPlane(const CPlane3Dfp64& _Plane);
	virtual int GetMemorySize();
	virtual int GetMemoryUse();

	virtual int GetType() const;
	virtual CFStr GetKey() const;
};

// -------------------------------------------------------------------
//  CSolid_Plane
// -------------------------------------------------------------------
class CSolid;

class CSolid_Plane;
typedef TPtr<CSolid_Plane> spCSolid_Plane;

#define CSOLID_PLANEFLAGS_CSGFACE	1

class CSolid_Plane : public CReferenceCount
{
public:
	
	DECLARE_OPERATOR_NEW;
	

	int m_Flags;
	CPlane3Dfp64 m_Plane;
	spCSolid_Mapping m_spMapping;
	CVec3Dfp64 m_v0;
	CVec3Dfp64 m_v1;
	CVec3Dfp64 m_v2;
	
	CSolid* m_pOriginalSolid;		// Solid this plane originates from when CSG operations has been performed.

	void operator= (const CSolid_Plane& _Plane);

	CSolid_Plane();
	CSolid_Plane(const CPlane3Dfp64& _Plane);
	CSolid_Plane(const CSolid_Plane& _Plane);
	spCSolid_Plane Duplicate() const;

	void ParseString(const char* _pStr);
	CStr GetString();
	CStr GetString2();

	bool ParsePlane(CStr _Key, CStr _Value);

	int GetMemorySize();
	int GetMemoryUse();
};

// -------------------------------------------------------------------
class CSolid_Face : public CReferenceCount
{
public:
	DECLARE_OPERATOR_NEW;
	
//	TArray<uint32> m_liVertices;
	spCSolid_Plane m_spPlane;
	CPixel32 m_Color;
	uint16 m_iInvalid;
	uint16 m_nV;
	uint16 m_iiV;		// Same is used for texcoords

	CSolid_Face();

	void operator= (const CSolid_Face& _Face);

	int GetMemorySize();
	int GetMemoryUse();
	void OptimizeMemory();
};

typedef TPtr<CSolid_Face> spCSolid_Face;

// -------------------------------------------------------------------
class CSolid_Edge
{
public:
	uint16 m_iV[2];

	CSolid_Edge();
	CSolid_Edge(int _iv0, int _iv1);
};

class CSolid_EdgeFaces
{
public:
	int16 m_iFaces[2];		// MUST BE SIGNED!!!

	CSolid_EdgeFaces();
};

// -------------------------------------------------------------------
//  CSolid
// -------------------------------------------------------------------
class CSolid;
typedef TPtr<CSolid> spCSolid;

class CSolid : public CReferenceCount
{
//	void CutFace(TArray<CVec3Dfp64>& _lVertices, const CPlane3Dfp64& _pi);

	static int AddTempVertex(CVec3Dfp64* pV8, CVec3Dfp32* _pV, int& _nV, int _MaxV, const CVec3Dfp64& _v);
	static int AddTempEdge(CSolid_Edge* pE, CSolid_EdgeFaces* pEF, int& _nE, int _MaxE, int _iv0, int _iv1, int _iFace);

public:
	
	DECLARE_OPERATOR_NEW;
	

	// Links
	CSolid* m_pPrev;
	spCSolid m_spNext;

	// Get end of chain
	CSolid* GetTail()
	{
		CSolid* pSolid = this;
		while(pSolid->m_spNext != NULL)
			pSolid = pSolid->m_spNext;
		return pSolid;
	}

	// Unlink this from chain
	void Unlink()
	{
		if (m_pPrev) m_pPrev->m_spNext = m_spNext;
		if (m_spNext != NULL) m_spNext->m_pPrev = m_pPrev;
		m_spNext = NULL;
		m_pPrev = NULL;
	}

	// Unlink this from chain
	void UnlinkPrev()
	{
		if (m_pPrev) m_pPrev->m_spNext = m_spNext;
		m_pPrev = NULL;
	}

	// Unlink this from chain but keeping tail.
	void UnlinkWithTail()
	{
		if (m_pPrev) m_pPrev->m_spNext = NULL;
		m_pPrev = NULL;
	}

	void LinkChainAfter(spCSolid _spLink)
	{
		if (!_spLink) Error("LinkAfter", "spLink == NULL");
		UnlinkPrev();
		CSolid* pTail = this;
		while(pTail->m_spNext != NULL) pTail = pTail->m_spNext;
		pTail->m_spNext = _spLink->m_spNext;
		if (pTail->m_spNext != NULL) pTail->m_spNext->m_pPrev = pTail;
		m_pPrev = _spLink;
		_spLink->m_spNext = this;
	}

	void LinkAfter(spCSolid _spLink)
	{
		if (!_spLink) Error("LinkAfter", "spLink == NULL");
		Unlink();
		m_spNext = _spLink->m_spNext;
		if (m_spNext != NULL) m_spNext->m_pPrev = this;
		m_pPrev = _spLink;
		_spLink->m_spNext = this;
	}

	void LinkBefore(spCSolid _spLink)
	{
		if (!_spLink) Error("LinkAfter", "spLink == NULL");
		Unlink();
		m_spNext = _spLink;
		m_pPrev = _spLink->m_pPrev;
		if (m_pPrev) m_pPrev->m_spNext = this;
		_spLink->m_pPrev = this;
	}

	int GetChainLen()
	{
		int nLinks = 0;
		CSolid* pS = this;
		while(pS)
		{
			nLinks++;
			pS = pS->m_spNext;
		}
		return nLinks;
	}

	void OptimizeChain()
	{
		CSolid* pS = this;
		while(pS)
		{
			pS->OptimizeMemory();
			pS = pS->m_spNext;
		}
	}

	CStr GetString() { return CStrF("%.8x, p %.8x, n %.8x", this, m_pPrev, m_spNext); };

	// Solid
	TArray<spCSolid_Plane> m_lspPlanes;

	uint16 m_Flags;
	uint16 m_ModelMask;
	CXR_MediumDesc m_Medium;
//	int m_MediumFlags;

	// Mesh
protected:
//	TArray<CVec3Dfp32> m_lVertices;
	TArray<CVec3Dfp64> m_lVerticesD;
	TArray<CSolid_Edge> m_lEdges;
	TArray<CSolid_Face> m_lFaces;
	TArray<CVec2Dfp32> m_lTVertices;
	TArray<uint16> m_liVertices;

	CSolid_Face& GetFaceNonConst(int _iFace);
	CVec2Dfp32* GetFaceTVerticesNonConst(const CSolid_Face&);

public:
	// Boundsphere
	fp32 m_BoundRadius;
	CVec3Dfp32 m_BoundPos;

	// Boundbox
	CVec3Dfp32 m_BoundMin;
	CVec3Dfp32 m_BoundMax;

	int32 m_TempSurfaceID;
	int32 m_iNode;						// Skeleton-attach node for trimesh.

	// Construct/Destruct
	CSolid();
	CSolid(const CSolid& _Solid);
	~CSolid();
	void virtual operator= (const CSolid& _Solid);
	spCSolid Duplicate() const;

	int AddPlane(const CPlane3Dfp64& _Plane, bool _bCheckDuplicates = false);
	int AddPlane(spCSolid_Plane _spPlane, bool _bCheckDuplicates = false);
	int AddPlanesFromBox(const CBox3Dfp64& _Box);

	void AddPlane(CStr);
	bool ParsePlane(CStr _Key, CStr _Value);

	static int IsParsableKey(CStr &_Key);
	CStr GetPlaneStr(int _iPlane);
	void GetPlaneKey(int _iPlane, CStr& _Key, CStr& _Val);
	int GetNumPlanes() const;
	int GetNumFaces() const;	// Should always return the same number as GetNumPlanes()
	int GetNumEdges() const;
	const CSolid_Plane& GetPlane(int _iPlane) const;
	const CSolid_Face& GetFace(int _iFace) const;
	const CSolid_Edge& GetEdge(int _iEdge) const;
	TArray<CVec3Dfp64> GetVertexArray() const;
	const uint16* GetFaceVertexIndices(int _iFace) const;
	const uint16* GetFaceVertexIndices(const CSolid_Face& _Face) const;
	const CVec2Dfp32* GetFaceTVertices(int _iFace) const;
	const CVec2Dfp32* GetFaceTVertices(const CSolid_Face& _Face) const;
	CVec3Dfp64 GetFaceCenter(int _iFace);

	// State updates
	void UpdateBound();
	void UpdateMesh();
	void CreateFromBox(const CVec3Dfp64& _VMin, const CVec3Dfp64& _VMax);
	void CreateFromMesh();
	void UpdateSurface(int _iPlane);
	void UpdateSurfaces(CXR_SurfaceContext *_pContext);

	void SetSurface(CXW_Surface *_pSurface, int _iPlane = -1, int _bUpdateMapping = true);
	CXW_Surface *GetSurface(int _iPlane);
	CStr GetSurfaceName(int _iPlane);

	void SetFaceColor(int _iFace, int _iCol);
	void SetFaceColors();

	int GetMappingType(int _iFace);
	bool SetMappingType(int _iFace, int _iMapping);

	fp64 GetVolume();
	fp64 GetArea(int _iFace);

	// Hit-test
	bool IntersectLocalLine(const CVec3Dfp64 &_v0, const CVec3Dfp64 &_v1);
	fp32 IntersectFace(const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dir, int _iFace);		// > 0  ==  Hit
	fp32 Ogr_IntersectRay(const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dir);					// > 0  ==  Hit
	int Ogr_IntersectFaces(const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dir, fp32 &_Dist);	// -1 == No hit

	// CSG
	int CSG_GetCutMask(const CPlane3Dfp64& _Plane) const;				// Get mask of how this intersects _Plane
	bool CSG_Intersection(const CSolid& _Solid) const;					// Does this and that intersect?
	bool CSG_Inside(const CSolid& _Solid) const;						// that inside this?
	int CSG_VertexInSolid(const CVec3Dfp64& _v, fp64 _Epsilon) const;		// 1 outside, 2 inside, 4 onsurface
	int CSG_PolygonOutsideSolid(const CVec3Dfp64* _pV, const uint32* _piV, int _nV, fp64 _Epsilon) const;
	int CSG_PolygonInSolid(const CVec3Dfp64* _pV, const uint32* _piV, int _nV, fp64 _Epsilon) const;	// 1 outside, 2 inside, 4 onsurface
	int CSG_GetPlaneIndex(const CPlane3Dfp64& _Plane, fp64 _Epsilon) const;
	bool CSG_PlaneExists(const CPlane3Dfp64& _Plane, fp64 _Epsilon) const;
	bool CSG_InversePlaneExists(const CPlane3Dfp64& _Plane, fp64 _Epsilon) const;
//	bool CSG_PolygonCovered(const CVec3Dfp64* _pV, const uint32* _piV, int _nV, fp64 _Epsilon) const;
	bool CSG_VertexExists(const CVec3Dfp64& _v, fp64 _Epsilon) const;		// Does this vertex exist in this solid?
	void CSG_CopyAttributes(const CSolid& _Solid);
	int CSG_CompareMediums(const CSolid& _Solid) const;					// Compares medium "hardness"
	void CSG_GetMediumFromSurfaces();
	int CSG_EqualMediums(const CSolid& _Solid) const;					// Compares all medium attributes
	bool CSG_CanClip(const CSolid& _Solid, bool _bHeedStructure = true) const;

	spCSolid CSG_CutBack(const CSolid_Plane* _pPlane) const;			// Get part of solid that's behind the plane
	spCSolid CSG_CutFront(const CSolid_Plane* _pPlane) const;			// Get part of solid that's in front of the plane
	spCSolid CSG_And(const CSolid& _Solid) const;
	spCSolid CSG_AndNotClip(const TArray<spCSolid_Plane>& _lspPlanes, int _ClipPlaneFlags, bool _bOptimize = true) const;
	spCSolid CSG_AndNot(const CSolid& _Solid, int _ClipPlaneFlags, bool _bOptimize = true) const;
	spCSolid CSG_Or(const CSolid& _Oper);
	spCSolid CSG_Merge(const CSolid& _Oper, bool _bIgnoreTexture = false) const;

	static spCSolid CSG_CutBackChain(spCSolid _spSrc, const CSolid_Plane* _pPlane);			// Get part of solids that's behind the plane
	static spCSolid CSG_CutFrontChain(spCSolid _spSrc, const CSolid_Plane* _pPlane);		// Get part of solids that's in front of the plane
	static spCSolid CSG_MergeChain(spCSolid _spSrc, bool _bIgnoreTexture = false, bool _bLog = false, IProgress* _pProgress = NULL);
	static spCSolid CSG_OrChain(spCSolid _spSrc, bool _bHeedStructure = true, bool _bLog = false, IProgress* _pProgress = NULL, bool _bIgnoreTexture = false, int _MaxClipResult = 0x7fffffff, bool _bPostOrMerge = true);
	static spCSolid CSG_AndNotChain(spCSolid _spSrc, spCSolid _Oper, bool _bOptimize, bool _bLog = false, IProgress* _pProgress = NULL);
	static spCSolid CSG_AndChain(spCSolid _spSrc, spCSolid _Oper, bool _bLog = false, IProgress* _pProgress = NULL);

	// Rendering
	void RenderEdges(CRenderContext* _pRC, const CMat4Dfp32* _pMatrix, const CPixel32& _Col) const;
	void RenderEdges(CXR_VBManager* _pVBM, const CMat4Dfp32* _pMatrix, const CPixel32& _Col) const;
	void RenderEdges(CWireContainer* _pWC, const CMat4Dfp32* _pMatrix, const CPixel32& _Col) const;
	void RenderWire(CRenderContext* _pRC, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CPixel32& _Col);
	void RenderWire(CXR_VBManager* _pVBM, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CPixel32& _Col);
	void RenderWire(CWireContainer* _pWC, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CPixel32& _Col);

	void RenderSolidNoCull(CRenderContext* _pRC, CXR_VBManager *_pVBM, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CPixel32& _Col, int _iType, CMTime _AnimTime, COgrRenderBuffer &_RenderBuffer);
	void RenderSolid(CRenderContext* _pRC, CXR_VBManager *_pVBM, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CPixel32& _Col, int _iType, CMTime _AnimTime, COgrRenderBuffer &_RenderBuffer);
	void Render(CRenderContext* _pRC, CXR_VBManager *_pVBM, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CPixel32& _Col, int _iType, CMTime _AnimTime, COgrRenderBuffer &_RenderBuffer);

	void RenderSolidNoCull(CRenderContext* _pRC, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CPixel32& _Col, int _iType = 7, CMTime _AnimTime = CMTime());

	// Modify
	void Apply(const CMat4Dfp32 &, bool _bSnap, int _TextureLock = 0);
	void MoveVertex(int _iIndex, const CVec3Dfp32 &, bool _bSnap);
	void MoveTexture(int _iIndex, const CVec2Dfp32 &_O, const CVec2Dfp32 &_S, float _Rot, const CVec3Dfp32 *_Ref);
	void MoveTexture(int _iIndex, const CMat4Dfp32& _Mat);
	void FitTexture(int _iFace, int _iBase);
	void TileTexture(int _iFace, TArray<CVec3Dfp32> &_lVert, TArray<CVec2Dfp32> &_lTV);
	void SetMapping(int _iFace, CXR_PlaneMapping *_pMapping);
	
	// Binary IO, no surface/medium properites, fp32 precision.
	void ReadGeometry(CCFile* _pF);
	void WriteGeometry(CCFile* _pF);
	void ReadGeometry2(CCFile* _pF);
	void WriteGeometry2(CCFile* _pF);

	void Read(CCFile* _pDFile);
	void Write(CCFile* _pDFile);

	static void WriteXMP(TArray<spCSolid> _lspSolids, CStr _FileName);		// Write array of solid chains
	static void WriteXMP(CSolid* _pSolids, CStr _FileName);					// Write solid chain

	void OptimizeMemory();
	int GetMemorySize();
	int GetMemoryUse();
};

// -------------------------------------------------------------------
//  CSolidNode
// -------------------------------------------------------------------
class CSolidNode;
typedef TPtr<CSolidNode> spCSolidNode;

class CSolidNode : public CReferenceCount
{
public:
	
	DECLARE_OPERATOR_NEW;
	

//	int m_Operator;
	spCSolid m_spSolid;								// ONE solid
	TArray<spCSolidNode> m_lspChildren;

	int m_TreeOper;
	spCSolid m_spTreeResult;						// Chain of solids

	CSolidNode();
	void PurgeRedundantNodes_r();
	spCSolid GetAll_r();
	int GetSolidCount_r();

	void CSG_FreeTreeResult_r();
	void CSG_Evaluate_r(int _FlagsAndAnyOf, int _FlagsAnd, int _FlagsEqual, bool _bNoCSGOr, int _ModelMask, int _MediumMask = -1);			// Result is placed in m_spTreeResult
	void CSG_GetMediumFromSurfaces_r();

	void Apply_r(const CMat4Dfp32& _Mat, bool _bSnap, int _TextureLock = 0);

	spCSolidNode Duplicate();
};


// -------------------------------------------------------------------
//  CSolid_SpaceEnum
// -------------------------------------------------------------------
class CSolid_SpaceEnum : public CReferenceCount
{
	CHash2D m_Hash0;
	CHash2D m_Hash1;
	CHash2D m_Hash2;
	CHash2D m_Hash3;

public:
	
	DECLARE_OPERATOR_NEW;
	

	CSolid_SpaceEnum();
	~CSolid_SpaceEnum();
	void Create(int _nObjects);
	void Insert(int _ID, const CSolid* _pSolid);
	void Remove(int _ID);
	int EnumerateBox(const CVec3Dfp32& _Min, const CVec3Dfp32& _Max, int32* _pEnumRetIDs, int _MaxEnumIDs);
	int EnumerateBox(const CSolid* _pSolid, int32* _pEnumRetIDs, int _MaxEnumIDs);

	CStr GetString() const;
	void Log() const;
};

#endif // _INC_XWSOLID
