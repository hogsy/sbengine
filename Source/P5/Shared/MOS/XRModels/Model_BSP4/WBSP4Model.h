#ifndef _INC_WBSP4MODEL
#define _INC_WBSP4MODEL

#include "../Model_BSP/XWCommon.h"
#include "XW4Common.h"
#include "../../XR/XR.h"
#include "../../XR/Solids/MCSolid.h"
#include "../../XR/Solids/XRIndexedSolid.h"
#include "../../XR/XRVBContainer.h"
#include "../Model_BSP2/WBSP2Model.h"

class CXR_VBManager;
class CXR_VertexBuffer;

#define MODEL_BSP_EPSILON 0.001f
#define MODEL_BSP_MAXFACEVERTICES	32

// -------------------------------------------------------------------
typedef CBSP4_CoreFace CBSP4_Face;

class CBSP4_EnumContext
{
public:
	// Enumeration tagging data
	TThinArray<uint8>	m_lFaceTagList;
	TThinArray<uint32>	m_lFaceUntagList;
	uint8* m_pFTag;
	uint32*	m_pFUntag;

	// Face enumeration data
	uint32* m_piEnum;
	int	m_nEnum;
	int	m_MaxEnum;
	int	m_nUntagEnum;
	// PL enumeration data
	uint16* m_piEnumPL;
	int	m_nEnumPL;
	int	m_MaxEnumPL;
	bool m_bEnumError;

	// Enumeration parameters
	int	m_EnumQuality;
	int	m_EnumMedium;
	int	m_EnumFaceFlags;
	CBox3Dfp32 m_EnumBox;
	CVec3Dfp32 m_EnumSphere;
	fp32 m_EnumSphereR;

	// PCS enumeration data
	class CPotColSet* m_pPCS;
	int	m_iObj;
	int	m_bIN1N2Flags;
	CBox3Dfp32*	m_pTransformedBox;
	class CBoxInterleaved* m_pTransformedBoxInterleaved;
	CVec3Dfp32 m_TransformedCenter;

	void Untag()
	{
		for(int i = 0; i < m_nUntagEnum; i++)
		{
			m_pFTag[m_pFUntag[i]]	= 0;
		}
		m_nUntagEnum	= 0;
	}

	void SetupPCSEnum(CPotColSet* _pPCS, int _iObj, int _bIN1N2Flags, const CVec3Dfp32& _Center, CBox3Dfp32* _pTransformedBox, class CBoxInterleaved* _pTransformedBoxInterleaved)
	{
		m_pPCS	= _pPCS;
		m_iObj	= _iObj;
		m_bIN1N2Flags	= _bIN1N2Flags;
		m_pTransformedBox	= _pTransformedBox;
		m_pTransformedBoxInterleaved	= _pTransformedBoxInterleaved;
		m_TransformedCenter	= _Center;
	}

	void SetupFaceEnum(uint32* _piEnum, int _MaxEnum)
	{
		m_piEnum	= _piEnum;
		m_MaxEnum	= _MaxEnum;
		m_nEnum		= 0;
	}

	void SetupPLEnum(uint16* _piEnum, int _MaxEnum)
	{
		m_piEnumPL	= _piEnum;
		m_MaxEnumPL	= _MaxEnum;
		m_nEnumPL		= 0;
	}

	void Create(int _nMaxFaces, int _EnumQuality, int _EnumMedium = -1, int _EnumFaceFlags = -1)
	{
		int lDataSize = (_nMaxFaces + 7) / 8;
		if(m_lFaceTagList.Len() < lDataSize)
		{
			m_lFaceTagList.SetLen(lDataSize);
			m_lFaceUntagList.SetLen(lDataSize);
			m_pFTag	= m_lFaceTagList.GetBasePtr();
			m_pFUntag	= m_lFaceUntagList.GetBasePtr();
			memset(m_pFTag, 0, m_lFaceTagList.Len());
		}

		m_bEnumError	= false;
		m_EnumQuality	= _EnumQuality;
		m_EnumMedium	= _EnumMedium;
		m_EnumFaceFlags	= _EnumFaceFlags;
		m_nEnum			= 0;
		m_MaxEnum		= 0;
		m_nEnumPL		= 0;
		m_MaxEnumPL		= 0;
		m_nUntagEnum	= 0;

		m_piEnum	= 0;
		m_piEnumPL	= 0;
		m_pPCS		= 0;

#ifndef M_RTM
		for(int i = 0; i < m_lFaceTagList.Len(); i++)
			M_ASSERT(m_pFTag[i] == 0, "?");
#endif
	}
	CBSP4_EnumContext() : m_pFTag(0), m_pFUntag(0)
	{
	}
};


// -------------------------------------------------------------------
class CXR_Model_BSP4;

class CXR_Model_BSP4 : 
	public CXR_PhysicsModel
{
public:

	DECLARE_OPERATOR_NEW

	MRTC_DECLARE;

public:
	// Settings
	static int ms_Enable;

	// Const.
	TThinArray<CPlane3Dfp32> m_lPlanes;
	TThinArray<CVec3Dfp32> m_lVertices;
	TArray<uint32> m_liVertices;
	TThinArray<CBSP4_Face> m_lFaces;
	TThinArray<uint32> m_liFaces;
	TThinArray<CBSP4_Node> m_lNodes;

	TArray<spCXW_Surface> m_lspSurfaces;
	TArray<CXR_MediumDesc> m_lMediums;

	mutable TPtr<TObjectPool<CBSP4_EnumContext> > m_spEnumContextPool;

	// -------------------------------------------------------------------
	// Pointers into the lists above.
	CPlane3Dfp32* m_pPlanes;
	CVec3Dfp32* m_pVertices;
	uint32* m_piVertices;
	CBSP4_Face* m_pFaces;
	uint32* m_piFaces;
	CBSP4_Node* m_pNodes;
	CXR_MediumDesc* m_pMediums;

	CBox3Dfp32		m_OctaAABBBox;
	CBSP_OctaAABBList m_lOctaAABBNodes;
	CMat4Dfp32		m_OctaAABBToU16;
	vec128			m_OctaAABBScale;
	vec128			m_OctaAABBTranslate;

protected:
	fp32 m_BoundRadius;
	CBox3Dfp32 m_BoundBox;

	void InitBound();

	// -------------------------------------------------------------------
	// Face enumeration.
	enum
	{
		ENUM_HQ = 1,
		ENUM_FACEFLAGS = 2,
		ENUM_MEDIUMFLAGS = 4,
	};

	void EnumFaces_Sphere_r(CXR_PhysicsContext* _pPhysContext, class CBSP4_EnumContext* _pEnumContext, int _iNode) const;
	void EnumFaces_Box_r(CXR_PhysicsContext* _pPhysContext, class CBSP4_EnumContext* _pEnumContext, int _iNode) const;
	void EnumFaces_All_r(CXR_PhysicsContext* _pPhysContext, class CBSP4_EnumContext* _pEnumContext, int _iNode) const;

	int EnumFaces_Sphere(CXR_PhysicsContext* _pPhysContext, class CBSP4_EnumContext* _pEnumContext, int _iNode) const;
	int EnumFaces_Box(CXR_PhysicsContext* _pPhysContext, class CBSP4_EnumContext* _pEnumContext, int _iNode) const;
	int EnumFaces_All(CXR_PhysicsContext* _pPhysContext, class CBSP4_EnumContext* _pEnumContext, int _iNode) const;

	// -------------------------------------------------------------------

	int NumFacesInTree(int _iNode);
	int GetLeaf(const CVec3Dfp32& _v);

	virtual void Clear();
	virtual void InitializeListPtrs();

public:
	CXR_Model_BSP4();
	~CXR_Model_BSP4();
	virtual void Create(const char* _pParam, CDataFile* _pDFile, CCFile* _pFile, const CBSP_CreateInfo& _CreateInfo);
	void Create_PhongMap();
	void Create_PostRead();

	// Overrides
	virtual aint GetParam(int _Param);
	virtual void SetParam(int _Param, aint _Value);
	virtual int GetParamfv(int _Param, fp32* _pRetValues);
	virtual void SetParamfv(int _Param, const fp32* _pValues);

	virtual int GetModelClass() { return CXR_MODEL_CLASS_BSP4; };
	virtual fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState = NULL);
	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState = NULL);

	virtual void PreRender(CXR_Engine* _pEngine, CXR_ViewClipInterface* _pViewClip,
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0);
	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
			const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags);
	virtual void OnPrecache(CXR_Engine* _pEngine, int _iVariation);

	CStr GetInfo();

protected:


protected:
	// -------------------------------------------------------------------
	// Physics internal functions

	void __Phys_RenderFace(int _iFace, const CMat4Dfp32& _WMat, CWireContainer* _pWC, int _Col) const;

	static bool __Phys_IntersectSphere_Polygon(const CVec3Dfp32* _pV, const uint32* _piV, int _nV,
		const CPlane3Dfp32& _Plane, const CVec3Dfp32& _Pos, fp32 _Radius, CCollisionInfo* _pCollisionInfo);

	bool __Phys_IntersectSphere_CheckFace(int _iFace, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, CCollisionInfo* _pCollisionInfo) const;
	bool __Phys_IntersectSphere_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo) const;

	bool __Phys_IntersectSphere_Triangle(const CVec3Dfp32& _V0, const CVec3Dfp32& _V1, const CVec3Dfp32& _V2, const CVec3Dfp32& _Pos, fp32 _Radius, CCollisionInfo* _pCollisionInfo) const;

	bool __Phys_IntersectOBB_CheckFace(int _iFace, const CPhysOBB& _BoxStart, const CPhysOBB& _Box, CCollisionInfo* _pCollisionInfo) const;
	bool __Phys_IntersectOBB_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CPhysOBB& _BoxStart, const CPhysOBB& _Box, const CBox3Dfp32 _Bound, int _MediumFlags, CCollisionInfo* _pCollisionInfo) const;
	bool __Phys_IntersectOBB_i(CXR_PhysicsContext* _pPhysContext, int _iNode, const CPhysOBB& _BoxStart, const CPhysOBB& _Box, const CBox3Dfp32& _Bound, int _MediumFlags, CCollisionInfo* _pCollisionInfo);

	int __Phys_CollideSolid_i(CXR_PhysicsContext* _pPhysContext, int _iNode, const CXR_IndexedSolidDesc32& _SolidDescr, int _nVertices, const CMat4Dfp32& _BSP2Transform, const CVec3Dfp32& _Offset, const CBox3Dfp32& _Bound, int _MediumFlags, CCollisionInfo* _pCollisionInfo, int _nMaxCollisions) const;
	int __Phys_CollideBox_i(CXR_PhysicsContext* _pPhysContext, int _iNode, const CPhysOBB& _Box, const CBox3Dfp32& _Bound, int _MediumFlags, CCollisionInfo* _pCollisionInfo, int _nMaxCollisions) const;
	int __Phys_CollideCapsule_i(CXR_PhysicsContext* _pPhysContext, int _iNode, const CPhysCapsule& _Capsule, const CBox3Dfp32& _Bound, int _MediumFlags, CCollisionInfo* _pCollisionInfo, int _nMaxCollisions) const;

	bool __Phys_TraceLine_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, class CPhysLineContext* _pLineContext, CCollisionInfo* _pCollisionInfo) const;
	bool __Phys_TraceLine_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MediumFlags, class CPhysLineContext* _pLineContext) const;

	int __Phys_GetCombinedMediumFlags_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CBox3Dfp32* _pBox) const;
	int __Phys_GetCombinedMediumFlags_i(CXR_PhysicsContext* _pPhysContext, int _iNode, const CBox3Dfp32* _pBox) const;
	
	void CollectPCS_r(CXR_PhysicsContext* _pPhysContext, CBSP4_EnumContext* _pEnumContext, int _iNode) const;
	void CollectPCS_i(CXR_PhysicsContext* _pPhysContext, CBSP4_EnumContext* _pEnumContext, int _iNode) const;
	void CollectPCS_IsLeaf(CXR_PhysicsContext* _pPhysContext, CBSP4_EnumContext* _pEnumContext, const CBSP4_Node* _pNode) const;

public:
	// -------------------------------------------------------------------
	// Physics overrides
	virtual void Phys_GetBound_Sphere(const CMat4Dfp32& _Pos, CVec3Dfp32& _v0, fp32& _Radius);
	virtual void Phys_GetBound_Box(const CMat4Dfp32& _Pos, CBox3Dfp32& _Box);

	virtual void Phys_Init(CXR_PhysicsContext* _pPhysContext);
	virtual bool Phys_IntersectLine(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);
	virtual bool Phys_IntersectSphere(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);
	virtual bool Phys_IntersectBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _BoxOrigin, const CPhysOBB& _BoxDest, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);

	virtual int Phys_CollideBSP2(CXR_PhysicsContext* _pPhysContext, class CXR_IndexedSolidContainer32 *_pSolidContainer, const CMat4Dfp32& _BSP2Transform, const CVec3Dfp32& _Offset, int _MediumFlags, CCollisionInfo* _pCollisionInfo, int _nMaxCollisions);
	virtual int Phys_CollideBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _Box, int _MediumFlags, CCollisionInfo* _pCollisionInfo, int _nMaxCollisions);
	virtual int Phys_CollideCapsule(CXR_PhysicsContext* _pPhysContext, const CPhysCapsule& _Capsule,int _MediumFlags, CCollisionInfo* _pCollisionInfo, int _nMaxCollisions);

	virtual void CollectPCS(CXR_PhysicsContext* _pPhysContext, const uint8 _IN1N2Flags, CPotColSet *_pcs, const int _iObj, const int _MediumFlags );

	virtual int Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0);
	virtual void Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, CXR_MediumDesc& _RetMedium);
	virtual int Phys_GetCombinedMediumFlags(CXR_PhysicsContext* _pPhysContext, const CBox3Dfp32& _Box);



public:
	virtual int& GetGlobalEnable() { return ms_Enable; };
};

typedef TPtr<CXR_Model_BSP4> spCXR_Model_BSP4;

///////////////////////////////////////////////////////////////////////////////
class CBoxInterleaved
{
public:
	fp32 m_MinX, m_MaxX;
	fp32 m_MinY, m_MaxY;
	fp32 m_MinZ, m_MaxZ;

	CBoxInterleaved() {}

	explicit CBoxInterleaved(const CBox3Dfp32& _Box)
	{
		Init(_Box);
	}

	void Init(const CBox3Dfp32& _Box)
	{
		m_MinX = _Box.m_Min.k[0];
		m_MinY = _Box.m_Min.k[1];
		m_MinZ = _Box.m_Min.k[2];
		m_MaxX = _Box.m_Max.k[0];
		m_MaxY = _Box.m_Max.k[1];
		m_MaxZ = _Box.m_Max.k[2];
	}
};

static M_INLINE void GetBoxMinMaxDistance(const CPlane3Dfp32& _Plane, 
                                   const CBoxInterleaved& _Box, 
                                   fp32& _MinDistance, fp32& _MaxDistance)
{
	register const uint32* pPlaneSign = (uint32*)&_Plane;
	register const fp32* pPlane = (const fp32*)&_Plane;
	register const fp32* pBox = (const fp32*)&_Box;

	register uint32 ix, iy, iz;
	register fp32 xMin, xMax, yMin, yMax, zMin, zMax;

	ix = (pPlaneSign[0]>>31); // check sign of normal.x
	xMin = pBox[ix] * pPlane[0];
	xMax = pBox[1-ix] * pPlane[0];	//1-0=1

	iy = 2+(pPlaneSign[1]>>31); // check sign of normal.y
	yMin = pBox[iy] * pPlane[1];
	yMax = pBox[5-iy] * pPlane[1];	//5-2=3

	iz = 4+(pPlaneSign[2]>>31); // check sign of normal.z
	zMin = pBox[iz] * pPlane[2];
	zMax = pBox[9-iz] * pPlane[2];	//9-4=5

	_MinDistance = xMin + yMin + zMin + pPlane[3];
	_MaxDistance = xMax + yMax + zMax + pPlane[3];
}

///////////////////////////////////////////////////////////////////////////////

#endif // _INC_WBSP4MODEL

