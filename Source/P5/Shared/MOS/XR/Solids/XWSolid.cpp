#include "PCH.h"

#ifndef PLATFORM_CONSOLE
#include "MRTC.h"
#include "XWSolid.h"
#include "MFloat.h"
#include "../../MSystem/Raster/MRender.h"
#include "../../XR/XRClass.h"
#include "../../XR/XRSurfaceContext.h"
#include "../../XR/XRSurf.h"
#include "../../Classes/Render/MWireContainer.h"

#ifndef PLATFORM_CONSOLE
#include "../../Classes/Render/OgrRender.h"
#endif

#define CSOLID_MAXVERTICES 1024
#define CSOLID_MAXEDGES 1024


#define CSOLID_SNAPGRID (1.0f / 16.0f)
#define CSOLID_SNAPTRESH 0.01f

#define CSOLID_HASH0_SIZE 128					// => 128x128 hash
#define CSOLID_HASH0_BUCKET_SHIFT_SIZE	6		// => Buckets are sized (1 << 6) by (1 << 6). (64x64)

#define CSOLID_HASH1_SIZE 64					// => 64x64 hash
#define CSOLID_HASH1_BUCKET_SHIFT_SIZE	7		// => Buckets are sized (1 << 6) by (1 << 6). (128x128)

#define CSOLID_HASH2_SIZE 64					// => 64x64 hash
#define CSOLID_HASH2_BUCKET_SHIFT_SIZE	8		// => Buckets are sized (1 << 7) by (1 << 7). (256x256)

#define CSOLID_HASH3_SIZE 32					// => 32x32 hash
#define CSOLID_HASH3_BUCKET_SHIFT_SIZE	10		// => Buckets are sized (1 << 9) by (1 << 9). (1024x1024)

// #define CSOLID_PARANOIAPLANECHECK

#define UVMAP_DIGITS 8

//#pragma optimize("", off)
//#pragma inline_depth(0)

// -------------------------------------------------------------------

	IMPLEMENT_OPERATOR_NEW(CSolid_Mapping);
	IMPLEMENT_OPERATOR_NEW(CSolid_Plane);
	IMPLEMENT_OPERATOR_NEW(CSolid_Face);
	IMPLEMENT_OPERATOR_NEW(CSolid);
	IMPLEMENT_OPERATOR_NEW(CSolidNode);



// -------------------------------------------------------------------
static bool EqualPlanes(const CPlane3Dfp64& _p1, const CPlane3Dfp64& _p2, fp64 _Epsilon)
{
	MAUTOSTRIP(EqualPlanes, false);
	if ((M_Fabs(_p1.Distance(_p2.GetPointInPlane())) < _Epsilon) &&
		M_Fabs(_p1.n * _p2.n - 1.0f) < _Epsilon) return true;
	return false;
}

// -------------------------------------------------------------------
//  Winding stuff
// -------------------------------------------------------------------
TArray<CVec3Dfp64> GetWindingFromPlane(const CPlane3Dfp64& _Plane);
TArray<CVec3Dfp64> GetWindingFromPlane(const CPlane3Dfp64& _Plane)
{
	MAUTOSTRIP(GetWindingFromPlane, TArray<CVec3Dfp64>());
	TArray<CVec3Dfp64> lWinding;

	CVec3Dfp64 v;
	if (M_Fabs(_Plane.n.k[0]) > 1-0.01)
	{
		v.k[0] = 0;
		v.k[1] = 0;
		v.k[2] = 1;
	}
	else
	{
		v.k[0] = 1;
		v.k[1] = 0;
		v.k[2] = 0;
	}

	CVec3Dfp64 dv0, dv1, p;
	v.CrossProd(_Plane.n, dv0);
	dv0.Normalize();
	dv0.CrossProd(_Plane.n, dv1);
	p = _Plane.GetPointInPlane();

	dv0 *= 100000000.0;
	dv1 *= 100000000.0;

	lWinding.Add(p + dv0);
	lWinding.Add(p + dv1);
	lWinding.Add(p - dv0);
	lWinding.Add(p - dv1);
	return lWinding;
}

void CutWindingFront(TArray<CVec3Dfp64>& _lVertices, const CPlane3Dfp64& _pi);
void CutWindingFront(TArray<CVec3Dfp64>& _lVertices, const CPlane3Dfp64& _pi)
{
	MAUTOSTRIP(CutWindingFront, MAUTOSTRIP_VOID);
	// Keeps windings on the frontside of the plane

	int lVertSide[128];
	int nv = _lVertices.Len();
	if (!nv) return;
	CVec3Dfp64* pV = &_lVertices[0];

	int v;
	for (v = 0; v < nv; v++)
		lVertSide[v] = _pi.GetPlaneSide_Epsilon(pV[v], CSOLID_EPSILON); 

	const int MaxVClip = 128;
	CVec3Dfp64 VClip[MaxVClip];		// Enough?
	int nClip = 0;

	for(v = 0; v < nv; v++)
	{
		int v2 = v+1;
		if (v2 >= nv) v2 -= nv;
		if (lVertSide[v] >= 0)
		{
			VClip[nClip++] = pV[v];
			if ((lVertSide[v2] < 0) && (lVertSide[v] > 0))
			{
				CVec3Dfp64 CutPoint;
				_pi.GetIntersectionPoint(pV[v], pV[v2], CutPoint);
				VClip[nClip++] = CutPoint;
			}
		}
		else
		{
			if (lVertSide[v2] > 0)
			{
				CVec3Dfp64 CutPoint;
				_pi.GetIntersectionPoint(pV[v], pV[v2], CutPoint);
				VClip[nClip++] = CutPoint;
			}
		}

		if (nClip > MaxVClip-1) Error_static("CutWindingFront", "Too many vertices.");
	}

	if (nClip)
	{
		_lVertices.GrowLen(nClip);
		memmove(&_lVertices[0], &(VClip[0]), _lVertices.ListSize());
	}
	else
		_lVertices.Clear();
}

void CutWindingBack(TArray<CVec3Dfp64>& _lVertices, const CPlane3Dfp64& _pi);
void CutWindingBack(TArray<CVec3Dfp64>& _lVertices, const CPlane3Dfp64& _pi)
{
	MAUTOSTRIP(CutWindingBack, MAUTOSTRIP_VOID);
	// Keeps windings on the backside of the plane

	int lVertSide[128];
	int nv = _lVertices.Len();
	if (!nv) return;
	CVec3Dfp64* pV = &_lVertices[0];

	int v;
	for (v = 0; v < nv; v++)
		lVertSide[v] = _pi.GetPlaneSide_Epsilon(pV[v], CSOLID_EPSILON); 

	const int MaxVClip = 128;
	CVec3Dfp64 VClip[MaxVClip];		// Enough?
	int nClip = 0;

	for(v = 0; v < nv; v++)
	{
		int v2 = v+1;
		if (v2 >= nv) v2 -= nv;
		if (lVertSide[v] <= 0)
		{
			VClip[nClip++] = pV[v];
			if ((lVertSide[v2] > 0) && (lVertSide[v] < 0))
			{
				CVec3Dfp64 CutPoint;
				_pi.GetIntersectionPoint(pV[v], pV[v2], CutPoint);
				VClip[nClip++] = CutPoint;
			}
		}
		else
		{
			if (lVertSide[v2] < 0)
			{
				CVec3Dfp64 CutPoint;
				_pi.GetIntersectionPoint(pV[v], pV[v2], CutPoint);
				VClip[nClip++] = CutPoint;
			}
		}

		if (nClip > MaxVClip-1) Error_static("CutWindingBack", "Too many vertices.");
	}

	if (nClip)
	{
		_lVertices.GrowLen(nClip);
		memmove(&_lVertices[0], &(VClip[0]), _lVertices.ListSize());
	}
	else
		_lVertices.Clear();
}

// -------------------------------------------------------------------
//  CSolid_SpaceEnum
// -------------------------------------------------------------------


	IMPLEMENT_OPERATOR_NEW(CSolid_SpaceEnum);


CSolid_SpaceEnum::CSolid_SpaceEnum()
{
	MAUTOSTRIP(CSolid_SpaceEnum_ctor, MAUTOSTRIP_VOID);
}

CSolid_SpaceEnum::~CSolid_SpaceEnum()
{
	MAUTOSTRIP(CSolid_SpaceEnum_dtor, MAUTOSTRIP_VOID);
}

void CSolid_SpaceEnum::Create(int _nObjects)
{
	MAUTOSTRIP(CSolid_SpaceEnum_Create, MAUTOSTRIP_VOID);
	// The hash-table with the largest grid has a "large-list" for all unhashable objects.

	m_Hash0.Create(CSOLID_HASH0_SIZE, CSOLID_HASH0_BUCKET_SHIFT_SIZE, _nObjects, false);
	m_Hash1.Create(CSOLID_HASH1_SIZE, CSOLID_HASH1_BUCKET_SHIFT_SIZE, _nObjects, false);
	m_Hash2.Create(CSOLID_HASH2_SIZE, CSOLID_HASH2_BUCKET_SHIFT_SIZE, _nObjects, false);
	m_Hash3.Create(CSOLID_HASH3_SIZE, CSOLID_HASH3_BUCKET_SHIFT_SIZE, _nObjects, true);
}

void CSolid_SpaceEnum::Insert(int _ID, const CSolid* _pSolid)
{
	MAUTOSTRIP(CSolid_SpaceEnum_Insert, MAUTOSTRIP_VOID);
	Remove(_ID);

	CVec3Dfp32 VMin(_pSolid->m_BoundMin.k[0], _pSolid->m_BoundMin.k[1], _pSolid->m_BoundMin.k[2]);
	CVec3Dfp32 VMax(_pSolid->m_BoundMax.k[0], _pSolid->m_BoundMax.k[1], _pSolid->m_BoundMax.k[2]);

	// Figure out which hash-table should be used.
	int MaxSize = 0;
	if (VMax.k[0] - VMin.k[0] > MaxSize) MaxSize = VMax.k[0] - VMin.k[0];
	if (VMax.k[1] - VMin.k[1] > MaxSize) MaxSize = VMax.k[1] - VMin.k[1];
	MaxSize += 0.0001f;

	if (MaxSize < (1 << CSOLID_HASH0_BUCKET_SHIFT_SIZE))
		m_Hash0.Insert(_ID, VMin, VMax);
	else if (MaxSize < (1 << CSOLID_HASH1_BUCKET_SHIFT_SIZE))
		m_Hash1.Insert(_ID, VMin, VMax);
	else if (MaxSize < (1 << CSOLID_HASH2_BUCKET_SHIFT_SIZE))
		m_Hash2.Insert(_ID, VMin, VMax);
	else
		m_Hash3.Insert(_ID, VMin, VMax);
}

void CSolid_SpaceEnum::Remove(int _ID)
{
	MAUTOSTRIP(CSolid_SpaceEnum_Remove, MAUTOSTRIP_VOID);
	m_Hash0.Remove(_ID);
	m_Hash1.Remove(_ID);
	m_Hash2.Remove(_ID);
	m_Hash3.Remove(_ID);
}

int CSolid_SpaceEnum::EnumerateBox(const CVec3Dfp32& _Min, const CVec3Dfp32& _Max, int32* _pEnumRetIDs, int _MaxEnumIDs)
{
	MAUTOSTRIP(CSolid_SpaceEnum_EnumerateBox, 0);
	int n = m_Hash0.EnumerateBox(_Min, _Max, _pEnumRetIDs, _MaxEnumIDs);
	n += m_Hash1.EnumerateBox(_Min, _Max, &_pEnumRetIDs[n], _MaxEnumIDs-n);
	n += m_Hash2.EnumerateBox(_Min, _Max, &_pEnumRetIDs[n], _MaxEnumIDs-n);
	n += m_Hash3.EnumerateBox(_Min, _Max, &_pEnumRetIDs[n], _MaxEnumIDs-n);
	return n;
}

int CSolid_SpaceEnum::EnumerateBox(const CSolid* _pSolid, int32* _pEnumRetIDs, int _MaxEnumIDs)
{
	MAUTOSTRIP(CSolid_SpaceEnum_EnumerateBox_2, 0);
	CVec3Dfp32 VMin(_pSolid->m_BoundMin.k[0], _pSolid->m_BoundMin.k[1], _pSolid->m_BoundMin.k[2]);
	CVec3Dfp32 VMax(_pSolid->m_BoundMax.k[0], _pSolid->m_BoundMax.k[1], _pSolid->m_BoundMax.k[2]);
	return EnumerateBox(VMin, VMax, _pEnumRetIDs, _MaxEnumIDs);
}

CStr CSolid_SpaceEnum::GetString() const
{
	MAUTOSTRIP(CSolid_SpaceEnum_GetString, CStr());
	return m_Hash0.GetString() + ", " + m_Hash1.GetString() + ", " + m_Hash2.GetString();
}

void CSolid_SpaceEnum::Log() const
{
	MAUTOSTRIP(CSolid_SpaceEnum_Log, MAUTOSTRIP_VOID);
	LogFile("Hash0: " + m_Hash0.GetString());
	LogFile("Hash1: " + m_Hash1.GetString());
	LogFile("Hash2: " + m_Hash2.GetString());
	LogFile("Hash3: " + m_Hash3.GetString());
}

// -------------------------------------------------------------------
//  CSolid_Mapping
// -------------------------------------------------------------------
CSolid_Mapping::CSolid_Mapping()
{
	MAUTOSTRIP(CSolid_Mapping_ctor, MAUTOSTRIP_VOID);
	m_pSurface = NULL;
}

void CSolid_Mapping::InternalDuplicate(CSolid_Mapping *_pMap) const
{
	MAUTOSTRIP(CSolid_Mapping_InternalDuplicate, MAUTOSTRIP_VOID);
	_pMap->m_pSurface = m_pSurface;
	_pMap->m_SurfName = m_SurfName;
}

spCSolid_Mapping CSolid_Mapping::Duplicate() const
{
	MAUTOSTRIP(CSolid_Mapping_Duplicate, NULL);
	Error("Duplicate", "Illegal call.");
	return NULL;

/*	spCSolid_MappingMAP spMap = DNew(CSolid_MappingMAP) CSolid_MappingMAP;
	if (!spMap) MemError("Duplicate");
	InternalDuplicate(spMap);
	return spMap;*/
}

// -------------------------------------------------------------------
//  CSolid_MappingMAP
// -------------------------------------------------------------------
spCSolid_Mapping CSolid_MappingMAP::Duplicate() const
{
	MAUTOSTRIP(CSolid_MappingMAP_Duplicate, NULL);
	spCSolid_MappingMAP spMap = MNew(CSolid_MappingMAP);
	if (!spMap) MemError("Duplicate");
	spMap->m_MapMapping = m_MapMapping;
	InternalDuplicate(spMap);
	return (CSolid_Mapping*) spMap;
}

void CSolid_MappingMAP::SetMappingPlane(CXR_PlaneMapping& _Mapping, const CPlane3Dfp64& _Plane)
{
	MAUTOSTRIP(CSolid_MappingMAP_SetMappingPlane, MAUTOSTRIP_VOID);
	m_MapMapping.Create(_Mapping, 0.001f);
}

CXR_PlaneMapping CSolid_MappingMAP::GetMappingPlane(const CPlane3Dfp64& _Plane)
{
	MAUTOSTRIP(CSolid_MappingMAP_GetMappingPlane, CXR_PlaneMapping());
	CXR_PlaneMapping PMap;
	PMap.Create(m_MapMapping, _Plane);
	return PMap;
}

void CSolid_MappingMAP::GenerateUV(const CPlane3Dfp64* _pPlane, const CVec3Dfp64* _pV, const uint16* _piV, int _nV, CVec2Dfp32* _pDestTV)
{
	MAUTOSTRIP(CSolid_MappingMAP_GenerateUV, MAUTOSTRIP_VOID);
	CXR_PlaneMapping Mapping = GetMappingPlane(*_pPlane);

	CVec3Dfp32 ReferenceDim(256);
	if (m_pSurface)
		ReferenceDim = m_pSurface->GetTextureMappingReferenceDimensions();
	fp32 TxtWInv = 1.0f / ReferenceDim[0];
	fp32 TxtHInv = 1.0f / ReferenceDim[1];

	CVec3Dfp32 UVec = Mapping.m_U;
	CVec3Dfp32 VVec = Mapping.m_V;
	fp32 UVecLenSqrInv = 1.0f/(UVec*UVec);
	fp32 VVecLenSqrInv = 1.0f/(VVec*VVec);
	CVec2Dfp32 TMin(_FP32_MAX);
	CVec2Dfp32 TMax(-_FP32_MAX);
	int i;
	for(i = 0; i < _nV; i++)
	{
		CVec3Dfp32 vec(_pV[_piV[i]].k[0], _pV[_piV[i]].k[1], _pV[_piV[i]].k[2]);
		CVec2Dfp32 TV;

		fp32 UProj = (vec * UVec) * UVecLenSqrInv;
		fp32 VProj = (vec * VVec) * VVecLenSqrInv;
		TV.k[0] = (UProj + m_MapMapping.m_Offset.k[0]) * TxtWInv;
		TV.k[1] = (VProj + m_MapMapping.m_Offset.k[1]) * TxtHInv;
		if (TV.k[0] > TMax.k[0]) TMax.k[0] = TV.k[0];
		if (TV.k[1] > TMax.k[1]) TMax.k[1] = TV.k[1];
		if (TV.k[0] < TMin.k[0]) TMin.k[0] = TV.k[0];
		if (TV.k[1] < TMin.k[1]) TMin.k[1] = TV.k[1];
		_pDestTV[i] = TV;
	}

	// Calc center and subtract it from TV array.
	CVec2Dfp32 TMid = (TMin + TMax) * 0.5f;
	TMid.k[0] = RoundToInt(TMid.k[0]/16.0f)*16.0f;
	TMid.k[1] = RoundToInt(TMid.k[1]/16.0f)*16.0f;
	for(i = 0; i < _nV; i++)
		_pDestTV[i] -= TMid;
}

void CSolid_MappingMAP::MoveTexture(const CPlane3Dfp64 &_Plane, const CVec2Dfp32 &_O, const CVec2Dfp32 &_S, float _Rot, const CVec3Dfp32 *_Ref)
{
	MAUTOSTRIP(CSolid_MappingMAP_MoveTexture, MAUTOSTRIP_VOID);
	m_MapMapping.m_Offset += _O;
	m_MapMapping.m_Scale.k[0] *= _S.k[0];
	m_MapMapping.m_Scale.k[1] *= _S.k[1];
	if(_Ref)
	{
		CVec3Dfp64 v(_Ref->k[0], _Ref->k[1], _Ref->k[2]);
		uint16 i = 0;
		CVec2Dfp32 uv;
		GenerateUV(&_Plane, &v, &i, 1, &uv);

		CVec3Dfp32 ReferenceDim(256);
		if (m_pSurface)
			ReferenceDim = m_pSurface->GetTextureMappingReferenceDimensions();

		uv.k[0] *= ReferenceDim[0];
		uv.k[1] *= ReferenceDim[1];

		m_MapMapping.m_Offset -= uv;
		RotateElements(m_MapMapping.m_Offset.k[0], m_MapMapping.m_Offset.k[1], fp32(-_Rot / 360.0f));
		m_MapMapping.m_Offset += uv;
	}
	m_MapMapping.m_Rot += _Rot;
	
	//Snap
	m_MapMapping.m_Offset.k[0] = SnapFloat(m_MapMapping.m_Offset.k[0], 0.1f, 0.001f);
	m_MapMapping.m_Offset.k[1] = SnapFloat(m_MapMapping.m_Offset.k[1], 0.1f, 0.001f);
	m_MapMapping.m_Rot = SnapFloat(m_MapMapping.m_Rot, 1, 0.001f);
	m_MapMapping.m_Scale.k[0] = SnapFloat(m_MapMapping.m_Scale.k[0], 0.001f, 0.00001f);
	m_MapMapping.m_Scale.k[1] = SnapFloat(m_MapMapping.m_Scale.k[1], 0.001f, 0.00001f);
}

void CSolid_MappingMAP::GetTextureParams(const CPlane3Dfp64 &_Plane, CVec2Dfp32 &_O, CVec2Dfp32 &_S, float &_Rot, CVec3Dfp32 &_U, CVec3Dfp32 &_V)
{
	MAUTOSTRIP(CSolid_MappingMAP_GetTextureParams, MAUTOSTRIP_VOID);
	CXR_PlaneMapping m = GetMappingPlane(_Plane);
	_U = m.m_U;
	_V = m.m_V;

	_O = m_MapMapping.m_Offset;
	_S = m_MapMapping.m_Scale;
	_Rot = m_MapMapping.m_Rot;
}

bool CSolid_MappingMAP::IsEqual(const CSolid_Mapping* _pMap)
{
	MAUTOSTRIP(CSolid_MappingMAP_IsEqual, false);
	const CSolid_MappingMAP* pMap = TDynamicCast<const CSolid_MappingMAP>(_pMap);
	if (!pMap) return false;

	if (0 != m_SurfName.CompareNoCase(pMap->m_SurfName)) return false;
	return m_MapMapping.AlmostEqual(pMap->m_MapMapping, 0.001f);
}

int CSolid_MappingMAP::GetMemorySize()
{
	MAUTOSTRIP(CSolid_MappingMAP_GetMemorySize, 0);
	return sizeof(*this) + m_SurfName.Len()+16; // 16 = sizeof(CStrData)
}

int CSolid_MappingMAP::GetMemoryUse()
{
	MAUTOSTRIP(CSolid_MappingMAP_GetMemoryUse, 0);
	return sizeof(*this) + m_SurfName.Len()+16; // 16 = sizeof(CStrData)
}

int CSolid_MappingMAP::GetType() const
{
	MAUTOSTRIP(CSolid_MappingMAP_GetType, 0);
	return OGIER_MAPPINGTYPE_BOXMAPPED;
}

CFStr CSolid_MappingMAP::GetKey() const
{
	MAUTOSTRIP(CSolid_MappingMAP_GetKey, CFStr());
	return CFStrF("0, %s,%s, %s, %s,%s", 
		(char *)CFStr::GetFilteredString(m_MapMapping.m_Offset[0], UVMAP_DIGITS), 
		(char *)CFStr::GetFilteredString(m_MapMapping.m_Offset[1], UVMAP_DIGITS), 
		(char *)CFStr::GetFilteredString(m_MapMapping.m_Rot, UVMAP_DIGITS), 
		(char *)CFStr::GetFilteredString(m_MapMapping.m_Scale[0], UVMAP_DIGITS), 
		(char *)CFStr::GetFilteredString(m_MapMapping.m_Scale[1], UVMAP_DIGITS));
}

// -------------------------------------------------------------------
//  CSolid_MappingPlane
// -------------------------------------------------------------------
spCSolid_Mapping CSolid_MappingPlane::Duplicate() const
{
	MAUTOSTRIP(CSolid_MappingPlane_Duplicate, NULL);
	spCSolid_MappingPlane spMap = MNew(CSolid_MappingPlane);
	if (!spMap) MemError("Duplicate");
	spMap->m_PlaneMap = m_PlaneMap;
	InternalDuplicate(spMap);
	return (CSolid_Mapping*)spMap;
}

void CSolid_MappingPlane::GenerateUV(const CPlane3Dfp64* _pPlane, const CVec3Dfp64* _pV, const uint16* _piV, int _nV, CVec2Dfp32* _pDestTV)
{
	MAUTOSTRIP(CSolid_MappingPlane_GenerateUV, MAUTOSTRIP_VOID);
	const CXR_PlaneMapping& Mapping = m_PlaneMap;

	CVec3Dfp32 ReferenceDim(256);
	if (m_pSurface)
		ReferenceDim = m_pSurface->GetTextureMappingReferenceDimensions();
	fp32 TxtWInv = 1.0f / ReferenceDim[0];
	fp32 TxtHInv = 1.0f / ReferenceDim[1];

	CVec3Dfp32 UVec = Mapping.m_U;
	CVec3Dfp32 VVec = Mapping.m_V;
	fp32 UVecLenSqrInv = 1.0f/(UVec*UVec);
	fp32 VVecLenSqrInv = 1.0f/(VVec*VVec);
	CVec2Dfp32 TMin(_FP32_MAX);
	CVec2Dfp32 TMax(-_FP32_MAX);
	int i;
	for(i = 0; i < _nV; i++)
	{
		CVec3Dfp32 vec(_pV[_piV[i]].k[0], _pV[_piV[i]].k[1], _pV[_piV[i]].k[2]);
		CVec2Dfp32 TV;

		fp32 UProj = (vec * UVec) * UVecLenSqrInv;
		fp32 VProj = (vec * VVec) * VVecLenSqrInv;
		TV.k[0] = (UProj + Mapping.m_UOffset) * TxtWInv;
		TV.k[1] = (VProj + Mapping.m_VOffset) * TxtHInv;
		if (TV.k[0] > TMax.k[0]) TMax.k[0] = TV.k[0];
		if (TV.k[1] > TMax.k[1]) TMax.k[1] = TV.k[1];
		if (TV.k[0] < TMin.k[0]) TMin.k[0] = TV.k[0];
		if (TV.k[1] < TMin.k[1]) TMin.k[1] = TV.k[1];
		_pDestTV[i] = TV;
	}

	// Calc center and subtract it from TV array.
	CVec2Dfp32 TMid = (TMin + TMax) * 0.5f;
	TMid.k[0] = RoundToInt(TMid.k[0]/16.0f)*16.0f;
	TMid.k[1] = RoundToInt(TMid.k[1]/16.0f)*16.0f;
	for(i = 0; i < _nV; i++)
		_pDestTV[i] -= TMid;
}

void CSolid_MappingPlane::MoveTexture(const CPlane3Dfp64 &_Plane, const CVec2Dfp32 &_O, const CVec2Dfp32 &_S, float _Rot, const CVec3Dfp32 *_Ref)
{
	MAUTOSTRIP(CSolid_MappingPlane_MoveTexture, MAUTOSTRIP_VOID);

	m_PlaneMap.m_UOffset += _O.k[0];
	m_PlaneMap.m_VOffset += _O.k[1];
	m_PlaneMap.m_U *= _S.k[0];
	m_PlaneMap.m_V *= _S.k[1];
}

void CSolid_MappingPlane::GetTextureParams(const CPlane3Dfp64 &_Plane, CVec2Dfp32 &_O, CVec2Dfp32 &_S, float &_Rot, CVec3Dfp32 &_U, CVec3Dfp32 &_V)
{
	MAUTOSTRIP(CSolid_MappingPlane_GetTextureParams, MAUTOSTRIP_VOID);

	_O = CVec2Dfp32(m_PlaneMap.m_UOffset, m_PlaneMap.m_VOffset);
	_S = CVec2Dfp32(m_PlaneMap.m_U.Length(), m_PlaneMap.m_V.Length());
	_U = m_PlaneMap.m_U;
	_V = m_PlaneMap.m_V;
	_Rot = 0;
}

bool CSolid_MappingPlane::IsEqual(const CSolid_Mapping* _pMap)
{
	MAUTOSTRIP(CSolid_MappingPlane_IsEqual, false);
	const CSolid_MappingPlane* pMap = TDynamicCast<const CSolid_MappingPlane>(_pMap);
	if (!pMap) return false;

	if (0 != m_SurfName.CompareNoCase(pMap->m_SurfName)) return false;
	return m_PlaneMap.AlmostEqual(pMap->m_PlaneMap, 0.001f) != 0;
}

void CSolid_MappingPlane::SetMappingPlane(CXR_PlaneMapping& _Mapping, const CPlane3Dfp64& _Plane)
{
	MAUTOSTRIP(CSolid_MappingPlane_SetMappingPlane, MAUTOSTRIP_VOID);
	m_PlaneMap = _Mapping;
}

CXR_PlaneMapping CSolid_MappingPlane::GetMappingPlane(const CPlane3Dfp64& _Plane)
{
	MAUTOSTRIP(CSolid_MappingPlane_GetMappingPlane, CXR_PlaneMapping());
	return m_PlaneMap;
}

int CSolid_MappingPlane::GetMemorySize()
{
	MAUTOSTRIP(CSolid_MappingPlane_GetMemorySize, 0);
	return sizeof(*this) + m_SurfName.Len()+16; // 16 = sizeof(CStrData)
}

int CSolid_MappingPlane::GetMemoryUse()
{
	MAUTOSTRIP(CSolid_MappingPlane_GetMemoryUse, 0);
	return sizeof(*this) + m_SurfName.Len()+16; // 16 = sizeof(CStrData)
}

int CSolid_MappingPlane::GetType() const
{
	MAUTOSTRIP(CSolid_MappingPlane_GetType, 0);
	return OGIER_MAPPINGTYPE_PLANEMAPPED;
}

CFStr CSolid_MappingPlane::GetKey() const
{
	MAUTOSTRIP(CSolid_MappingPlane_GetKey, CFStr());
	return CFStrF("1, %s,%s,%s, %s, %s,%s,%s, %s", 
		(char*)CFStr::GetFilteredString(m_PlaneMap.m_U[0], UVMAP_DIGITS), 
		(char*)CFStr::GetFilteredString(m_PlaneMap.m_U[1], UVMAP_DIGITS), 
		(char*)CFStr::GetFilteredString(m_PlaneMap.m_U[2], UVMAP_DIGITS), 
		(char*)CFStr::GetFilteredString(m_PlaneMap.m_UOffset,  UVMAP_DIGITS), 
		(char*)CFStr::GetFilteredString(m_PlaneMap.m_V[0], UVMAP_DIGITS), 
		(char*)CFStr::GetFilteredString(m_PlaneMap.m_V[1], UVMAP_DIGITS), 
		(char*)CFStr::GetFilteredString(m_PlaneMap.m_V[2], UVMAP_DIGITS), 
		(char*)CFStr::GetFilteredString(m_PlaneMap.m_VOffset,  UVMAP_DIGITS));
}

// -------------------------------------------------------------------
//  CSolid_Plane
// -------------------------------------------------------------------
CSolid_Plane::CSolid_Plane()
{
	MAUTOSTRIP(CSolid_Plane_ctor, MAUTOSTRIP_VOID);
	m_Flags = 0;
	m_pOriginalSolid = NULL;
}

CSolid_Plane::CSolid_Plane(const CPlane3Dfp64& _Plane)
{
	MAUTOSTRIP(CSolid_Plane_ctor_2, MAUTOSTRIP_VOID);
	m_Flags = 0;
	m_Plane = _Plane;
	m_v0 = fp64(0);
	m_v1 = fp64(0);
	m_v2 = fp64(0);
	m_pOriginalSolid = NULL;
}

CSolid_Plane::CSolid_Plane(const CSolid_Plane& _Plane)
{
	MAUTOSTRIP(CSolid_Plane_ctor_3, MAUTOSTRIP_VOID);
	*this = _Plane;
}

void CSolid_Plane::operator =(const CSolid_Plane& _Plane)
{
	MAUTOSTRIP(CSolid_Plane_operator_assign, MAUTOSTRIP_VOID);
	m_Flags = _Plane.m_Flags;
	m_Plane = _Plane.m_Plane;
	if (_Plane.m_spMapping != NULL) 
		m_spMapping = _Plane.m_spMapping->Duplicate();
	else
		m_spMapping = NULL;

	m_v0 = _Plane.m_v0;
	m_v1 = _Plane.m_v1;
	m_v2 = _Plane.m_v2;
	m_pOriginalSolid = _Plane.m_pOriginalSolid;
}

spCSolid_Plane CSolid_Plane::Duplicate() const
{
	MAUTOSTRIP(CSolid_Plane_Duplicate, NULL);
	spCSolid_Plane spP = MNew(CSolid_Plane);
	if (!spP) MemError("Duplicate");

	*spP = *this;
	return spP;
}

void CSolid_Plane::ParseString(const char* _pStr)
{
	MAUTOSTRIP(CSolid_Plane_ParseString, MAUTOSTRIP_VOID);
//( 192 64 144 ) ( 196 -128 148 ) ( 192 -128 144 ) tech10_1 0 0 0 1.000000 1.000000
	mint len = strlen(_pStr);
	int pos = 0;
	char apan = 'A';
	CVec3Dfp64 v0;
	CVec3Dfp64 v1;
	CVec3Dfp64 v2;
	int k;
	for(k = 0; k < 3; k++)
	{
		pos = CStr::GoToDigit(_pStr, pos, len);
		v0.k[k] = M_AToF(&_pStr[pos]);
		pos = CStr::SkipADigit(_pStr, pos, len);
	}
	for(k = 0; k < 3; k++)
	{
		pos = CStr::GoToDigit(_pStr, pos, len);
		v1.k[k] = M_AToF(&_pStr[pos]);
		pos = CStr::SkipADigit(_pStr, pos, len);
	}
	for(k = 0; k < 3; k++)
	{
		pos = CStr::GoToDigit(_pStr, pos, len);
		v2.k[k] = M_AToF(&_pStr[pos]);
		pos = CStr::SkipADigit(_pStr, pos, len);
	}
	while ( (_pStr[pos] != ')') && (pos < len)) pos++;
	if (pos < len) pos++;
	while ( (_pStr[pos] == ' ') && (pos < len)) pos++;

	int p2 = pos;
	while ( (_pStr[pos] != ' ') && (pos < len)) pos++;

	TPtr<CSolid_MappingMAP> spMapping = MNew(CSolid_MappingMAP);
	if (!spMapping) MemError("ParseString");

	spMapping->m_SurfName.Capture(&_pStr[p2], pos-p2);
	spMapping->m_SurfName.MakeUpperCase();

	pos = CStr::GoToDigit(_pStr, pos, len);
	spMapping->m_MapMapping.m_Offset.k[0] = M_AToF(&_pStr[pos]);
	pos = CStr::SkipADigit(_pStr, pos, len);
	pos = CStr::GoToDigit(_pStr, pos, len);
	spMapping->m_MapMapping.m_Offset.k[1] = M_AToF(&_pStr[pos]);
	pos = CStr::SkipADigit(_pStr, pos, len);
	pos = CStr::GoToDigit(_pStr, pos, len);
	spMapping->m_MapMapping.m_Rot = M_AToF(&_pStr[pos]);
	pos = CStr::SkipADigit(_pStr, pos, len);
	pos = CStr::GoToDigit(_pStr, pos, len);
	spMapping->m_MapMapping.m_Scale.k[0] = M_AToF(&_pStr[pos]);
	pos = CStr::SkipADigit(_pStr, pos, len);
	pos = CStr::GoToDigit(_pStr, pos, len);
	spMapping->m_MapMapping.m_Scale.k[1] = M_AToF(&_pStr[pos]);
	pos = CStr::SkipADigit(_pStr, pos, len);

/*	m_v0 = v0.GetSnapped(CSOLID_SNAPGRID, CSOLID_SNAPTRESH);
	m_v1 = v1.GetSnapped(CSOLID_SNAPGRID, CSOLID_SNAPTRESH);
	m_v2 = v2.GetSnapped(CSOLID_SNAPGRID, CSOLID_SNAPTRESH);*/
	m_v0 = v0;
	m_v1 = v1;
	m_v2 = v2;
	m_Plane.Create(m_v0, m_v2, m_v1);

	m_spMapping = spMapping;
}

CStr CSolid_Plane::GetString()
{
	MAUTOSTRIP(CSolid_Plane_GetString, CStr());

/*	CVec3Dfp64 v0 = m_v0.GetSnapped(CSOLID_SNAPGRID, CSOLID_SNAPTRESH);
	CVec3Dfp64 v1 = m_v1.GetSnapped(CSOLID_SNAPGRID, CSOLID_SNAPTRESH);
	CVec3Dfp64 v2 = m_v2.GetSnapped(CSOLID_SNAPGRID, CSOLID_SNAPTRESH);*/
	CVec3Dfp64 v0 = m_v0;
	CVec3Dfp64 v1 = m_v1;
	CVec3Dfp64 v2 = m_v2;

	CSolid_MappingMAP* pM = TDynamicCast<CSolid_MappingMAP>((CReferenceCount*)m_spMapping);
	if(pM)
	{
		CVec2Dfp32 Ofs(SnapFloat(pM->m_MapMapping.m_Offset.k[0], CSOLID_SNAPGRID, CSOLID_SNAPTRESH), SnapFloat(pM->m_MapMapping.m_Offset.k[1], CSOLID_SNAPGRID, CSOLID_SNAPTRESH));
		fp32 Rot = SnapFloat(pM->m_MapMapping.m_Rot, CSOLID_SNAPGRID, CSOLID_SNAPTRESH);
		CVec2Dfp32 Scale(SnapFloat(pM->m_MapMapping.m_Scale.k[0], CSOLID_SNAPGRID / 10.0f, CSOLID_SNAPTRESH / 10.0f), SnapFloat(pM->m_MapMapping.m_Scale.k[1], CSOLID_SNAPGRID / 10.0f, CSOLID_SNAPTRESH / 10.0f));

		return CStrF("( %s %s %s ) ( %s %s %s ) ( %s %s %s ) %s %s %s %s", 
			(char *)CStr::GetFilteredString(v0[0]), (char *)CStr::GetFilteredString(v0[1]), (char *)CStr::GetFilteredString(v0[2]),
			(char *)CStr::GetFilteredString(v1[0]), (char *)CStr::GetFilteredString(v1[1]), (char *)CStr::GetFilteredString(v1[2]),
			(char *)CStr::GetFilteredString(v2[0]), (char *)CStr::GetFilteredString(v2[1]), (char *)CStr::GetFilteredString(v2[2]),
			(char *)pM->m_SurfName.LowerCase(),
			(char *)Ofs.GetFilteredString(1), 
			(char *)CFStr::GetFilteredString(Rot),
			(char *)Scale.GetFilteredString(1));
	}
	else
	{
		CSolid_Mapping* pM = m_spMapping;
		return CStrF("( %s %s %s ) ( %s %s %s ) ( %s %s %s ) %s 0 0 0 1 1", 
			(char *)CStr::GetFilteredString(v0[0]), (char *)CStr::GetFilteredString(v0[1]), (char *)CStr::GetFilteredString(v0[2]),
			(char *)CStr::GetFilteredString(v1[0]), (char *)CStr::GetFilteredString(v1[1]), (char *)CStr::GetFilteredString(v1[2]),
			(char *)CStr::GetFilteredString(v2[0]), (char *)CStr::GetFilteredString(v2[1]), (char *)CStr::GetFilteredString(v2[2]),
			(char *)pM->m_SurfName.LowerCase());
	}
}

CStr CSolid_Plane::GetString2()
{
	MAUTOSTRIP(CSolid_Plane_GetString2, CStr());
	CSolid_Mapping* pM = m_spMapping;
/*	CVec3Dfp64 v0 = m_v0;
	CVec3Dfp64 v1 = m_v1;
	CVec3Dfp64 v2 = m_v2;
	return CStrF("2, %s,%s,%s, %s,%s,%s, %s,%s,%s, %s, %s",
		CStr::GetFilteredString(v0[0], 18).Str(), 
		CStr::GetFilteredString(v0[1], 18).Str(), 
		CStr::GetFilteredString(v0[2], 18).Str(), 
		CStr::GetFilteredString(v1[0], 18).Str(), 
		CStr::GetFilteredString(v1[1], 18).Str(), 
		CStr::GetFilteredString(v1[2], 18).Str(), 
		CStr::GetFilteredString(v2[0], 18).Str(), 
		CStr::GetFilteredString(v2[1], 18).Str(), 
		CStr::GetFilteredString(v2[2], 18).Str(), 
		(char *)pM->m_SurfName,
		(char*)pM->GetKey());*/

	return CStrF("3, %s, %s, %s, %s, %s, %s",
		CStr::GetFilteredString(SnapFloat(m_Plane.n[0], 1.0, 0.00000000001), 18).Str(), 
		CStr::GetFilteredString(SnapFloat(m_Plane.n[1], 1.0, 0.00000000001), 18).Str(), 
		CStr::GetFilteredString(SnapFloat(m_Plane.n[2], 1.0, 0.00000000001), 18).Str(), 
		CStr::GetFilteredString(SnapFloat(m_Plane.d, 1.0, 0.00000000001), 18).Str(),
		(char *)pM->m_SurfName,
		(char*)pM->GetKey());
}

bool CSolid_Plane::ParsePlane(CStr _Key, CStr _Value)
{
	MAUTOSTRIP(CSolid_Plane_ParsePlane, false);
	CFStr Key(_Key);
	Key.MakeUpperCase();
	if (Key.CompareSubStr("BRUSH_PLANE_") == 0)
	{
		CFStr Val(_Value);
		int PlaneVersion = Val.GetIntSep(",");

		// Parse vertices
		switch(PlaneVersion)
		{
		case 2 :
			{
				CVec3Dfp64 v[3];
				for(int j = 0; j < 3; j++)
					for(int k = 0; k < 3; k++)
						v[j].k[k] = Val.Getfp64Sep(",");

				m_v0 = v[0].GetSnapped(CSOLID_SNAPGRID, CSOLID_SNAPTRESH);
				m_v1 = v[1].GetSnapped(CSOLID_SNAPGRID, CSOLID_SNAPTRESH);
				m_v2 = v[2].GetSnapped(CSOLID_SNAPGRID, CSOLID_SNAPTRESH);
				m_Plane.Create(m_v0, m_v2, m_v1);
			}
			break;

		case 3 :
			{
				m_Plane.n[0] = Val.Getfp64Sep(",");
				m_Plane.n[1] = Val.Getfp64Sep(",");
				m_Plane.n[2] = Val.Getfp64Sep(",");
				m_Plane.d = Val.Getfp64Sep(",");
			}
			break;

		default :
			Error("ParsePlane", CStrF("Unsupported plane version: %d", PlaneVersion));
		}

		CStr SurfName = Val.GetStrSep(",");
		SurfName.Trim();
		SurfName.MakeUpperCase();

		int Mapping = Val.GetIntSep(",");
		switch(Mapping)
		{
		case 0 :
			{
				TPtr<CSolid_MappingMAP> spMapping = MNew(CSolid_MappingMAP);
				if (!spMapping) MemError("ParseString");
				m_spMapping = spMapping;

				spMapping->m_MapMapping.m_Offset.k[0] = Val.Getfp64Sep(",");
				spMapping->m_MapMapping.m_Offset.k[1] = Val.Getfp64Sep(",");
				spMapping->m_MapMapping.m_Rot = Val.Getfp64Sep(",");
				spMapping->m_MapMapping.m_Scale.k[0] = Val.Getfp64Sep(",");
				spMapping->m_MapMapping.m_Scale.k[1] = Val.Getfp64Sep(",");
				break;
			}

		case 1 :
			{
				TPtr<CSolid_MappingPlane> spMapping = MNew(CSolid_MappingPlane);
				if (!spMapping) MemError("ParseString");
				m_spMapping = spMapping;

				spMapping->m_PlaneMap.m_U[0] = Val.Getfp64Sep(",");
				spMapping->m_PlaneMap.m_U[1] = Val.Getfp64Sep(",");
				spMapping->m_PlaneMap.m_U[2] = Val.Getfp64Sep(",");
				spMapping->m_PlaneMap.m_UOffset = Val.Getfp64Sep(",");
				spMapping->m_PlaneMap.m_V[0] = Val.Getfp64Sep(",");
				spMapping->m_PlaneMap.m_V[1] = Val.Getfp64Sep(",");
				spMapping->m_PlaneMap.m_V[2] = Val.Getfp64Sep(",");
				spMapping->m_PlaneMap.m_VOffset = Val.Getfp64Sep(",");
//			LogFile("PlaneMap " + spMapping->m_PlaneMap.m_U.GetString() + spMapping->m_PlaneMap.m_V.GetString());

				break;
			}

		default :
			{
				LogFile(CStrF("(CSolid_Plane::ParsePlane) Invalid mapping type %d, ignoring brush plane.", Mapping));
				return false;
			}
		}

		if (!m_spMapping)
			Error("ParsePlane", "Internal error.");

		m_spMapping->m_SurfName = SurfName;
		return true;
	}
	else
		return false;
}

int CSolid_Plane::GetMemorySize()
{
	MAUTOSTRIP(CSolid_Plane_GetMemorySize, 0);
	int size = sizeof(*this);
	if (m_spMapping != NULL) size += m_spMapping->GetMemorySize();
	return size;
}

int CSolid_Plane::GetMemoryUse()
{
	MAUTOSTRIP(CSolid_Plane_GetMemoryUse, 0);
	int size = sizeof(*this);
	if (m_spMapping != NULL) size += m_spMapping->GetMemoryUse();
	return size;
}

// -------------------------------------------------------------------
CSolid_Face::CSolid_Face()
{
	MAUTOSTRIP(CSolid_Face_ctor, MAUTOSTRIP_VOID);
	m_Color = 0xffffffff;
	m_iInvalid = 0;
	m_nV = 0;
	m_iiV = 0xffff;
}

void CSolid_Face::operator =(const CSolid_Face& _Face)
{
	MAUTOSTRIP(CSolid_Face_operator_assign_2, MAUTOSTRIP_VOID);
	//Notes:	Ignores the m_spPlane member variable
	//			Resets color


	m_iInvalid = 0;
	m_nV = _Face.m_nV;
	m_iiV = _Face.m_iiV;
	m_Color = 0xffffffff;
}

int CSolid_Face::GetMemorySize()
{
	MAUTOSTRIP(CSolid_Face_GetMemorySize, 0);
	return sizeof(*this);
}

int CSolid_Face::GetMemoryUse()
{
	MAUTOSTRIP(CSolid_Face_GetMemoryUse, 0);
	return sizeof(*this);
}

void CSolid_Face::OptimizeMemory()
{
	MAUTOSTRIP(CSolid_Face_OptimizeMemory, MAUTOSTRIP_VOID);
}

// -------------------------------------------------------------------
CSolid_Edge::CSolid_Edge()
{
	MAUTOSTRIP(CSolid_Edge_ctor, MAUTOSTRIP_VOID);
}

CSolid_Edge::CSolid_Edge(int _iv0, int _iv1)
{
	MAUTOSTRIP(CSolid_Edge_ctor_2, MAUTOSTRIP_VOID);
	m_iV[0] = _iv0;
	m_iV[1] = _iv1;
}

// -------------------------------------------------------------------
CSolid_EdgeFaces::CSolid_EdgeFaces()
{
	MAUTOSTRIP(CSolid_EdgeFaces_ctor, MAUTOSTRIP_VOID);
	m_iFaces[0] = -1;
	m_iFaces[1] = -1;
}

// -------------------------------------------------------------------
CSolid::CSolid()
{
	MAUTOSTRIP(CSolid_ctor, MAUTOSTRIP_VOID);
	m_iNode = 0;
	m_TempSurfaceID = 0;
	m_pPrev = NULL;
	m_Flags = 0;
	m_ModelMask = CSOLID_MODELMASK_ALLBITS;
	m_Medium.SetSolid();
//	m_MediumFlags = XW_MEDIUM_SOLID;
}

CSolid::~CSolid()
{
	MAUTOSTRIP(CSolid_dtor, MAUTOSTRIP_VOID);
	CSolid* pTail = GetTail();
	while(pTail && (pTail != this))
	{
		CSolid* pPrev = pTail->m_pPrev;
		if (!pPrev) break;
		pPrev->m_spNext = NULL;
		pTail = pPrev;
	}
}

CSolid::CSolid(const CSolid& _Solid)
{
	MAUTOSTRIP(CSolid_ctor_2, MAUTOSTRIP_VOID);
	m_pPrev = NULL;
	*this = _Solid;
}

void CSolid::operator= (const CSolid& _Solid)
{
	MAUTOSTRIP(CSolid_operator_assign, MAUTOSTRIP_VOID);
//	m_lVertices.Clear();
//	m_lVertices.Add(&_Solid.m_lVertices);
	m_lVerticesD.Clear();
	m_lVerticesD.Add(&_Solid.m_lVerticesD);
	m_lEdges.Clear();
	m_lEdges.Add(&_Solid.m_lEdges);
	m_liVertices.Clear();
	m_liVertices.Add(&_Solid.m_liVertices);
	m_lTVertices.Clear();
	m_lTVertices.Add(&_Solid.m_lTVertices);

	int nPlanes = _Solid.m_lspPlanes.Len();
	m_lspPlanes.SetLen(nPlanes);
	int i;
	for(i = 0; i < nPlanes; i++)
		m_lspPlanes[i] = _Solid.m_lspPlanes[i]->Duplicate();

	int nFaces = _Solid.m_lFaces.Len();
	m_lFaces.SetLen(nFaces);
	{
		const CSolid_Face* pFSrc = _Solid.m_lFaces.GetBasePtr();
		CSolid_Face* pFDst = m_lFaces.GetBasePtr();

		for(i = 0; i < nFaces; i++)
		{
			pFDst[i] = pFSrc[i];
			pFDst[i].m_spPlane = m_lspPlanes[i];
		}
	}

	m_BoundRadius = _Solid.m_BoundRadius;
	m_BoundPos = _Solid.m_BoundPos;
	m_BoundMin = _Solid.m_BoundMin;
	m_BoundMax = _Solid.m_BoundMax;
	m_iNode = _Solid.m_iNode;
	m_TempSurfaceID = _Solid.m_TempSurfaceID;
	m_Flags = _Solid.m_Flags;
	m_ModelMask = _Solid.m_ModelMask;
	m_Medium = _Solid.m_Medium;
}

// -------------------------------------------------------------------
int CSolid::AddTempVertex(CVec3Dfp64* _pV8, CVec3Dfp32* _pV, int& _nV, int _MaxV, const CVec3Dfp64& _v)
{
	MAUTOSTRIP(CSolid_AddTempVertex, 0);
	CVec3Dfp32 v4(_v.k[0], _v.k[1], _v.k[2]);
	for(int v = 0; v < _nV; v++)
		if (_pV[v].AlmostEqual(v4, CSOLID_EPSILON*10.0)) return v;

	if (_nV >= _MaxV) Error_static("CSolid::AddTempVertex", "Out of vertices.");

	_pV[_nV] = v4;
	_pV8[_nV] = _v;
	_nV++;
	return (_nV - 1);
}

int CSolid::AddTempEdge(CSolid_Edge* pE, CSolid_EdgeFaces* pEF, int& _nE, int _MaxE, int _iv0, int _iv1, int _iFace)
{
	MAUTOSTRIP(CSolid_AddTempEdge, 0);
	for(int e = 0; e < _nE; e++)
		if (((pE[e].m_iV[0] == _iv0) && (pE[e].m_iV[1] == _iv1)) ||
			((pE[e].m_iV[0] == _iv1) && (pE[e].m_iV[1] == _iv0)))
		{
			pEF[e].m_iFaces[1] = _iFace;
			return e;
		}

	if (_nE >= _MaxE) Error_static("CSolid::AddTempEdge", "Out of edges.");

	pE[_nE] = CSolid_Edge(_iv0, _iv1);
	pEF[_nE].m_iFaces[0] = _iFace;

	_nE++;
	return (_nE - 1);
}

// -------------------------------------------------------------------
spCSolid CSolid::Duplicate() const
{
	MAUTOSTRIP(CSolid_Duplicate, NULL);
	spCSolid spS = MNew(CSolid);
	if (!spS) MemError("Duplicate");
	*spS = *this;
	return spS;
}

int CSolid::AddPlane(const CPlane3Dfp64& _Plane, bool _bCheckDuplicates)
{
	MAUTOSTRIP(CSolid_AddPlane, 0);
	CVec3Dfp64 P(_Plane.GetPointInPlane());
	fp64 nlrecp = 1.0f / _Plane.n.Length();

	if (_bCheckDuplicates)
	{
		for(int i = 0; i < m_lspPlanes.Len(); i++)
		{
			if ((M_Fabs(m_lspPlanes[i]->m_Plane.Distance(P)) < 0.001f) &&
				(M_Fabs((_Plane.n*m_lspPlanes[i]->m_Plane.n) * nlrecp / m_lspPlanes[i]->m_Plane.n.Length() - 1.0) < 0.00001f))
				return i;
		}
	}

	spCSolid_Plane spP = MNew1(CSolid_Plane,_Plane);
	if (!spP) MemError("AddPlane");

	int Grow = Max(8, GetGEPow2(m_lspPlanes.Len()) >> 1);
	m_lspPlanes.SetGrow(Grow);
	return m_lspPlanes.Add(spP);
}

int CSolid::AddPlane(spCSolid_Plane _spPlane, bool _bCheckDuplicates)
{
	MAUTOSTRIP(CSolid_AddPlane_2, 0);
	CVec3Dfp64 P(_spPlane->m_Plane.GetPointInPlane());
	fp64 nlrecp = 1.0f / _spPlane->m_Plane.n.Length();

	if (_bCheckDuplicates)
	{
		for(int i = 0; i < m_lspPlanes.Len(); i++)
		{
			if ((M_Fabs(m_lspPlanes[i]->m_Plane.Distance(P)) < 0.001f) &&
				(M_Fabs((_spPlane->m_Plane.n*m_lspPlanes[i]->m_Plane.n) * nlrecp / m_lspPlanes[i]->m_Plane.n.Length() - 1.0) < 0.00001f))
				return i;
		}
	}

	int Grow = Max(8, GetGEPow2(m_lspPlanes.Len()) >> 1);
	m_lspPlanes.SetGrow(Grow);
	return m_lspPlanes.Add(_spPlane);
}

int CSolid::AddPlanesFromBox(const CBox3Dfp64& _Box)
{
	int iRet = m_lspPlanes.Len();

	for(int k = 0; k < 3; k++)
	{
		CVec3Dfp64 N(0);
		N.k[k] = 1.0;
		CPlane3Dfp64 Plane;
		Plane.CreateND(N, -_Box.m_Max[k]);
		AddPlane(Plane);
		N.k[k] = -1.0;
		Plane.CreateND(N, _Box.m_Min[k]);
		AddPlane(Plane);
	}

	return iRet;
}

void CSolid::AddPlane(CStr _s)
{
	MAUTOSTRIP(CSolid_AddPlane_3, MAUTOSTRIP_VOID);
	spCSolid_Plane spP = MNew(CSolid_Plane);
	if (!spP) MemError("AddPlane");
	spP->ParseString(_s);
	m_lspPlanes.Add(spP);
}

int CSolid::IsParsableKey(CStr &_Key)
{
	MAUTOSTRIP(CSolid_IsParsableKey, 0);
	CFStr Key(_Key);
	Key.MakeUpperCase();
	return Key.CompareSubStr("BRUSH_PLANE_") == 0;
}

bool CSolid::ParsePlane(CStr _Key, CStr _Value)
{
	MAUTOSTRIP(CSolid_ParsePlane, false);
	if(!IsParsableKey(_Key))
		return false;

	spCSolid_Plane spP = MNew(CSolid_Plane);
	if (!spP) MemError("ParsePlane");
	if (spP->ParsePlane(_Key, _Value))
		m_lspPlanes.Add(spP);
	else
		return false;
	return true;
}

CStr CSolid::GetPlaneStr(int _iPlane)
{
	MAUTOSTRIP(CSolid_GetPlaneStr, CStr());
	return m_lspPlanes[_iPlane]->GetString();
}

void CSolid::GetPlaneKey(int _iPlane, CStr& _Key, CStr& _Val)
{
	MAUTOSTRIP(CSolid_GetPlaneKey, MAUTOSTRIP_VOID);
	_Key = CStrF("BRUSH_PLANE_%d", _iPlane);
	_Val = m_lspPlanes[_iPlane]->GetString2();
}

int CSolid::GetNumPlanes() const
{
	return m_lspPlanes.Len();
}

int CSolid::GetNumFaces() const
{
	return m_lFaces.Len();
}

int CSolid::GetNumEdges() const
{
	return m_lEdges.Len();
}

const CSolid_Plane& CSolid::GetPlane(int _iPlane) const
{
	return *m_lspPlanes[_iPlane];
}

CSolid_Face& CSolid::GetFaceNonConst(int _iFace)
{
	return m_lFaces[_iFace];
}

const CSolid_Face& CSolid::GetFace(int _iFace) const
{
	return m_lFaces[_iFace];
}

const CSolid_Edge& CSolid::GetEdge(int _iEdge) const
{
	return m_lEdges[_iEdge];
}

TArray<CVec3Dfp64> CSolid::GetVertexArray() const
{
	return m_lVerticesD;
}

const uint16* CSolid::GetFaceVertexIndices(int _iFace) const
{
	return &m_liVertices[GetFace(_iFace).m_iiV];
}

const uint16* CSolid::GetFaceVertexIndices(const CSolid_Face& _Face) const
{
	return &m_liVertices[_Face.m_iiV];
}

CVec2Dfp32* CSolid::GetFaceTVerticesNonConst(const CSolid_Face& _Face)
{
	return &m_lTVertices[_Face.m_iiV];
}

const CVec2Dfp32* CSolid::GetFaceTVertices(int _iFace) const
{
	return &m_lTVertices[GetFace(_iFace).m_iiV];
}

const CVec2Dfp32* CSolid::GetFaceTVertices(const CSolid_Face& _Face) const
{
	return &m_lTVertices[_Face.m_iiV];
}

void CSolid::UpdateBound()
{
	MAUTOSTRIP(CSolid_UpdateBound, MAUTOSTRIP_VOID);
//	CVec3Dfp32::GetMinBoundSphere(m_lVertices.GetBasePtr(), m_BoundPos, m_BoundRadius, m_lVertices.Len());

	m_BoundPos = 0;
	m_BoundRadius = 0;
	m_BoundMin = 0;
	m_BoundMax = 0;
	if (!m_lVerticesD.Len())
		return;

	CVec3Dfp64 BoundPos(0);
	int v;
	for(v = 0; v < m_lVerticesD.Len(); v++)
		BoundPos += m_lVerticesD[v];
	BoundPos *= (1.0f / m_lVerticesD.Len());
	BoundPos.Assignfp32(m_BoundPos);

	for(v = 0; v < m_lVerticesD.Len(); v++)
	{
		fp32 d = m_lVerticesD[v].DistanceSqr(BoundPos);
		if (d > m_BoundRadius)
			m_BoundRadius = d;
	}
	
	m_BoundRadius = M_Sqrt( m_BoundRadius );

	CVec3Dfp64 BoundMin, BoundMax;
	CVec3Dfp64::GetMinBoundBox(m_lVerticesD.GetBasePtr(), BoundMin, BoundMax, m_lVerticesD.Len());
	BoundMin.Assignfp32(m_BoundMin);
	BoundMax.Assignfp32(m_BoundMax);

//	LogFile(m_BoundPos.GetString() + CStrF(", %f", m_BoundRadius));
}

// -------------------------------------------------------------------
#if 0

void CSolid::UpdateMesh()
{
	MAUTOSTRIP(CSolid_UpdateMesh, MAUTOSTRIP_VOID);
	bool bLogEnable = false;
	
	const int MaxV = 256;
	const int MaxE = 512;
	CVec3Dfp32 TempV4[MaxV];
	CVec3Dfp64 TempV8[MaxV];
	CSolid_Edge TempEdge[MaxE];
	CSolid_EdgeFaces TempEdgeFaces[MaxE];
	int nTempV = 0;
	int nTempE = 0;

//	TArray<CVec3Dfp64> Face[32];
	m_lspFaces.Clear();
	int nf = m_lspPlanes.Len();
	spCSolid_Plane* plspPlanes = m_lspPlanes.GetBasePtr();
	for (int f = 0; f < nf; f++)
	{
		TArray<CVec3Dfp64> Face;
		CSolid_Plane* pPlane = plspPlanes[f];
			
		Face.Clear();
		Face = GetWindingFromPlane(pPlane->m_Plane);

		// Trim
		for (int p = 0; (p < m_lspPlanes.Len()); p++)
		{
			if (f != p)
//			if (M_Fabs(m_lspPlanes[p]->m_Plane.n * pPlane->m_Plane.n) < (1.0 - CSOLID_EPSILON*100.0f))
			{
				CutWindingBack(Face, plspPlanes[p]->m_Plane);
				if (Face.Len() <= 2) break;
			}
		}

		// Check integrity by making sure at least one vertex is not on all other planes.
		{
			for(int p = 0; p < m_lspPlanes.Len(); p++)
				if (f != p)
					if ((4 == plspPlanes[p]->m_Plane.GetArrayPlaneSideMask_Epsilon(Face.GetBasePtr(), Face.Len(), CSOLID_EPSILON*100.0f)) &&
						!EqualPlanes(plspPlanes[f]->m_Plane, plspPlanes[p]->m_Plane, CSOLID_EPSILON*100.0f))
					{
						// Destroy face if all points were on another plane. (a plane this face was not constructed from)
//							LogFile("WARNING (CSolid::UpdateMesh): Bad face constructed. Ignoring.");
							Face.Clear();
							break;
					}
		}


		if (Face.Len() >= 3)
		{
			int nv = Face.Len();
			spCSolid_Face spF = MNew(CSolid_Face);
			if (!spF) MemError("UpdateMesh");
			spF->m_liVertices.SetGrow(1);
			spF->m_liVertices.SetLen(nv);
			uint32* piVertices = spF->m_liVertices.GetBasePtr();

			{
				for(int v = 0; v < nv; v++)
					piVertices[v] = AddTempVertex(TempV8, TempV4, nTempV, MaxV, Face[v]);
			}

/*			{
				spF->m_liEdges.SetGrow(1);
				spF->m_liEdges.SetLen(nv);
				int* piEdges = spF->m_liEdges.GetBasePtr();
				for(int v0 = 0; v0 < nv; v0++)
				{
					int v1 = v0+1;
					if (v1 >= nv) v1 = 0;
					piEdges[v0] = AddTempEdge(TempEdge, TempEdgeFaces, nTempE, MaxE, piVertices[v0], piVertices[v1], m_lspFaces.Len());
				}
			}*/

			spF->m_spPlane = m_lspPlanes[f];
			spF->m_lTVertices.SetLen(nv);
			if (spF->m_spPlane->m_spMapping)
				spF->m_spPlane->m_spMapping->GenerateUV(&spF->m_spPlane->m_Plane, Face.GetBasePtr(), g_IndexRamp32, nv, spF->m_lTVertices.GetBasePtr());

			m_lspFaces.Add(spF);
		}
//		else
//			LogFile("Invalid plane");

		// Logfile
		if (bLogEnable)
		{
			CStr s("Face: "); for (int v = 0; v < Face.Len(); v++) s += Face[v].GetString() + ", ";
			LogFile(s);
		}
	}

	// Copy vertices & edges to solid
	{
		m_lVertices.SetGrow(1);
		m_lVerticesD.SetGrow(1);
		m_lVertices.SetLen(nTempV);
		m_lVerticesD.SetLen(nTempV);
		CVec3Dfp32* pV4 = m_lVertices.GetBasePtr();
		CVec3Dfp64* pV8 = m_lVerticesD.GetBasePtr();
		for(int v = 0; v < nTempV; v++)
		{
			pV4[v] = TempV4[v];
			pV8[v] = TempV8[v];
		}
	}

	{
		m_lEdges.SetGrow(1);
		m_lEdges.SetLen(nTempE);
		CSolid_Edge* pEdges = m_lEdges.GetBasePtr();
		for(int e = 0; e < nTempE; e++)
			pEdges[e] = TempEdge[e];
	}

	{
		m_lspPlanes.Clear();
		m_lspPlanes.SetLen(m_lspFaces.Len());
		spCSolid_Plane* plspPlanes = m_lspPlanes.GetBasePtr();
		for(int iFace = 0; iFace < m_lspFaces.Len(); iFace++)
			plspPlanes[iFace]	= m_lspFaces[iFace]->m_spPlane;
	}

	UpdateBound();
}

#else

// -------------------------------------------------------------------
enum
{
	CSG_INVALID = 0xffff,
	CSG_REVERSE = 0x8000,
	CSG_INDEXAND = 0x7fff,
};

class CCSG_Edge		// 16 bytes
{
public:

	uint16 m_liV[2];
	uint16 m_liFace[2];
	uint16 m_liEdgePrev[2];
	uint16 m_liEdgeNext[2];

	void Create(int _iV0, int _iV1)
	{
		m_liV[0] = _iV0;
		m_liV[1] = _iV1;
		m_liFace[0] = CSG_INVALID;
		m_liFace[1] = CSG_INVALID;
		m_liEdgePrev[0] = CSG_INVALID;
		m_liEdgePrev[1] = CSG_INVALID;
		m_liEdgeNext[0] = CSG_INVALID;
		m_liEdgeNext[1] = CSG_INVALID;
	}

	int GetNextEdge(int _iFace) const
	{
		int iEdge;
		if (m_liFace[0] == _iFace)
			iEdge = m_liEdgeNext[0];
		else if (m_liFace[1] == _iFace)
			iEdge = m_liEdgeNext[1];
		else
			Error_static("CCSG_Edge::GetNextEdge", CStrF("Face %d not referenced by this edge. (%s)", _iFace, GetString().Str() ));

		if (iEdge == CSG_INVALID)
			Error_static("CCSG_Edge::GetNextEdge", CStrF("Face %d, next edge invalid. (%s)", _iFace, GetString().Str() ));
		return iEdge;
	};

	int GetPrevEdge(int _iFace) const
	{
		int iEdge;
		if (m_liFace[0] == _iFace)
			iEdge = m_liEdgePrev[0];
		else if (m_liFace[1] == _iFace)
			iEdge = m_liEdgePrev[1];
		else
			Error_static("CCSG_Edge::GetPrevEdge", CStrF("Face %d not referenced by this edge. (%s)", _iFace, GetString().Str() ));

		if (iEdge == CSG_INVALID)
			Error_static("CCSG_Edge::GetPrevEdge", CStrF("Face %d, next edge invalid. (%s)", _iFace, GetString().Str() ));
		return iEdge;
	};

	int GetSide(int _iFace) const
	{
		if (m_liFace[0] == _iFace)
			return 0;
		else if (m_liFace[1] == _iFace)
			return 1;
		else
		{
			Error_static("CCSG_Edge::GetSide", CStrF("Face %d not referenced by this edge. (%s)", _iFace, GetString().Str() ));
			return 0;
		}
	}

	void SetNext(int _iE, int _iFace)
	{
		if (m_liFace[0] == _iFace)
			m_liEdgeNext[0] = _iE;
		else if (m_liFace[1] == _iFace)
			m_liEdgeNext[1] = _iE;
		else
			Error_static("CCSG_Edge::SetNext", CStrF("Face %d not referenced by this edge. (%s)", _iFace, GetString().Str() ));
	}

	void SetPrev(int _iE, int _iFace)
	{
		if (m_liFace[0] == _iFace)
			m_liEdgePrev[0] = _iE;
		else if (m_liFace[1] == _iFace)
			m_liEdgePrev[1] = _iE;
		else
			Error_static("CCSG_Edge::SetPrev", CStrF("Face %d not referenced by this edge. (%s)", _iFace, GetString().Str() ));
	}

	int GetOtherFace(int _iFace) const
	{
		if (m_liFace[0] == _iFace)
			return m_liFace[1];
		else if (m_liFace[1] == _iFace)
			return m_liFace[0];
		else
		{
			Error_static("CCSG_Edge::GetOtherFace", CStrF("Face %d not referenced by this edge. (%s)", _iFace, GetString().Str() ));
			return 0;
		}
	}

	void SetOtherFace(int _iFaceNew, int _iFace)
	{
		if (m_liFace[0] == _iFace)
			m_liFace[1] = _iFaceNew;
		else if (m_liFace[1] == _iFace)
			m_liFace[0] = _iFaceNew;
		else
			Error_static("CCSG_Edge::SetOtherFace", CStrF("Face %d not referenced by this edge. (%s)", _iFace, GetString().Str() ));
	}

	int GetVert0(int _iFace) const
	{
		if (m_liFace[0] == _iFace)
			return m_liV[0];
		else if (m_liFace[1] == _iFace)
			return m_liV[1];
		else
		{
			Error_static("CCSG_Edge::GetVert0", CStrF("Face %d not referenced by this edge. (%s)", _iFace, GetString().Str() ));
			return 0;
		}
	}

	int GetVert1(int _iFace) const
	{
		if (m_liFace[0] == _iFace)
			return m_liV[1];
		else if (m_liFace[1] == _iFace)
			return m_liV[0];
		else
		{
			Error_static("CCSG_Edge::GetVert0", CStrF("Face %d not referenced by this edge. (%s)", _iFace, GetString().Str() ));
			return 0;
		}
	}

	static int GetLoopEdgeCount(CCSG_Edge* _pE, int _iE, int _iFace)
	{
		int nV = 0;
		int iE = _iE;
		do
		{
			nV++;
			iE = _pE[iE].GetNextEdge(_iFace);
		}
		while(iE != _iE);
		return nV;
	}

	CStr GetString() const
	{
		return CStrF("iV %d, %d iFace %d, %d, iEPrev %d, %d, iENext %d,%d", 
			m_liV[0], m_liV[1], m_liFace[0], m_liFace[1],
			m_liEdgePrev[0], m_liEdgePrev[1], m_liEdgeNext[0], m_liEdgeNext[1]);
	}

	static void CreateLoop(CCSG_Edge* _pE, const int* _liE, int _nE, int _iFace)
	{
		int lSides[64];
		int liE[64];

		if (_nE > 64)
			Error_static("CSG_Edge::CreateLoop", "Max 64.");


		{
			for(int i = 0; i < _nE; i++)
			{
				liE[i] = _liE[i] & CSG_INDEXAND;
				if (_liE[i] & CSG_REVERSE)
				{
					if (_pE[liE[i]].m_liFace[1] != CSG_INVALID)
						Error_static("CCSG_Edge::CreateLoop", CStrF("Edge %d already has a reverse face referenced. (%s)", liE[i], _pE[liE[i]].GetString().Str() ));
					lSides[i] = 1;
				}
				else
				{
					if (_pE[liE[i]].m_liFace[0] != CSG_INVALID)
						Error_static("CCSG_Edge::CreateLoop", CStrF("Edge %d already has a forward face referenced. (%s)", liE[i], _pE[liE[i]].GetString().Str() ));
					lSides[i] = 0;
				}
			}
		}

		{
			for(int i = 0; i < _nE; i++)
			{
				CCSG_Edge& E = _pE[liE[i]];
				E.m_liFace[lSides[i]] = _iFace; // | (_liE[i] & CSG_REVERSE);
				int iNext = (i < _nE-1) ? (i+1) : 0;
				int iPrev = (i > 0) ? (i-1) : _nE-1;
				E.m_liEdgeNext[lSides[i]] = liE[iNext];
				E.m_liEdgePrev[lSides[i]] = liE[iPrev];
			}
		}

		{
			int iV1 = _pE[liE[_nE-1]].GetVert1(_iFace);
			for(int i = 0; i < _nE; i++)
			{
				int iV0 = _pE[liE[i]].GetVert0(_iFace);
				if (iV0 != iV1)
					Error_static("CCSG_Edge::CreateLoop", CStrF("Vertex sanity check failed. (%d != %d), Face %d, Edge %s", iV1, iV0, _iFace, _pE[liE[i]].GetString().Str() ));
				iV1 = _pE[liE[i]].GetVert1(_iFace);
			}
			int iV0 = _pE[liE[0]].GetVert0(_iFace);
			if (iV0 != iV1)
				Error_static("CCSG_Edge::CreateLoop", CStrF("Vertex sanity check failed. (%d != %d), Face %d, Edge %s", iV1, iV0, _iFace, _pE[liE[0]].GetString().Str() ));
		}
	}

	static void CreateQuadLoop(CCSG_Edge* _pE, int _iE0, int _iE1, int _iE2, int _iE3, int _iFace)
	{
		int liE[4];
		liE[0] = _iE0;
		liE[1] = _iE1;
		liE[2] = _iE2;
		liE[3] = _iE3;
		CreateLoop(_pE, liE, 4, _iFace);
	}
};

template<class T, int TMax>
class TStaticAllocator
{
public:
	T m_lnRef[TMax];
	T m_liFree[TMax];
	int m_nFree;
	int m_nItems;
	int m_nAllocated;

	void Clear()
	{
		m_nFree = 0;
		m_nItems = 0;
		m_nAllocated = 0;
	}

	void FillAlloc(int _iStart, int _nItems, int _nRef)
	{
		for(int i = 0; i < _nItems; i++)
			m_lnRef[_iStart + i] = _nRef;
		m_nItems = _nItems;
		m_nAllocated = _nItems;
	}

	int Alloc()
	{
		if (!m_nFree)
		{
			if (m_nItems >= TMax)
				Error_static("TStaticAllocator::Alloc", "Out of items.");

			int iItem = m_nItems;
			m_lnRef[iItem] = 1;
			m_nItems++;
			m_nAllocated++;
			Validate();
			return iItem;
		}
		else
		{
			m_nFree--;
			int iItem = m_liFree[m_nFree];
			if (m_lnRef[iItem])
				Error_static("TStaticAllocator::AddRef", "Invalid free list item.");
			m_lnRef[iItem]++;
			m_nAllocated++;
			Validate();
			return iItem;
		}
	}

	void Free(int _iItem)
	{
		if (!m_lnRef[_iItem])
			return;

		m_lnRef[_iItem] = 0;
		m_liFree[m_nFree] = _iItem;
		m_nFree++;
		m_nAllocated--;
		Validate();
	}

	void AddRef(int _iItem)
	{
		if (!m_lnRef[_iItem])
			Error_static("TStaticAllocator::AddRef", "Item is not allocated.");
		m_lnRef[_iItem]++;
		Validate();
	}

	void DelRef(int _iItem)
	{
		if (!m_lnRef[_iItem])
			Error_static("TStaticAllocator::AddRef", "Item is not allocated.");
		m_lnRef[_iItem]--;
		if (!m_lnRef[_iItem])
		{
			m_liFree[m_nFree] = _iItem;
			m_nFree++;
			m_nAllocated--;
		}
		Validate();
	}

	void Validate()
	{
#ifdef _DEBUG
		int n = 0;
		for(int i = 0; i < m_nItems; i++)
			if (m_lnRef[i])
				n++;

		if (n != m_nAllocated)
		{
			int a = 1;
		}
#endif
	}
};

/*static int CSG_AllocItem(uint16* _pRef, int& _nItems, int _MaxItems)
{

}*/

static void UpdateMeshError(CStr _Msg)
{
	Error_static("CSolid::UpdateMesh", _Msg);
};

void CSolid::UpdateMesh()
{
	if (m_lspPlanes.Len() < 4)
	{
		m_lVerticesD.Destroy();;
		m_lEdges.Destroy();;
		m_lFaces.Destroy();;
		UpdateBound();
		return;
	}

	const int MaxEdges = CSOLID_MAXEDGES;
	const int MaxPlanes = CSOLID_MAXPLANES;
	const int MaxVertices = CSOLID_MAXVERTICES;
/*	TArray<CCSG_Edge> lTempE;	// 16kb
	TArray<CVec3Dfp64> lTempV;
	TArray<uint8> lTempERef;
	TArray<uint8> lTempVRef;
	TArray<fp64> lTempVClipDist;
	TArray<uint8> lTempVClipMask;
	TArray<uint16> liEdgeInsert;
	lTempE.SetLen(MaxEdges);
	lTempV.SetLen(MaxVertices);
	lTempERef.SetLen(MaxEdges);
	lTempVRef.SetLen(MaxVertices);
	lTempVClipDist.SetLen(MaxVertices);
	lTempVClipMask.SetLen(MaxVertices);
	liEdgeInsert.SetLen(MaxEdges);
	CCSG_Edge* pTempE = lTempE.GetBasePtr();
*/

	TStaticAllocator<uint16, MaxEdges> EAlloc;
	TStaticAllocator<uint16, MaxVertices> VAlloc;

	CCSG_Edge lTempE[MaxEdges];	// 16kb
	CCSG_Edge* pTempE = lTempE;
//	static uint16 lTempERef[MaxEdges];

	CVec3Dfp64 lTempV[MaxVertices];	// 24kb
//	static uint16 lTempVRef[MaxVertices];
	fp64 lTempVClipDist[MaxVertices];
	uint8 lTempVClipMask[MaxVertices];

	uint16 liEdgeInsert[MaxEdges];
	uint8 lEdgeDone[MaxEdges];

	// -------------------------------------------------------------------
	// Create box
	static CCSG_Edge lBoxE[12];
	static CVec3Dfp64 lBoxV[8];
	static bool bBoxInit = false;
	if(!bBoxInit)
	{
		CBox3Dfp64 Box(-10000000.0, 10000000.0);
		Box.GetVertices(lBoxV);

		lBoxE[0].Create(0,1);
		lBoxE[1].Create(1,3);
		lBoxE[2].Create(3,2);
		lBoxE[3].Create(2,0);
		lBoxE[4].Create(4,5);
		lBoxE[5].Create(5,7);
		lBoxE[6].Create(7,6);
		lBoxE[7].Create(6,4);
		lBoxE[8].Create(0,4);
		lBoxE[9].Create(1,5);
		lBoxE[10].Create(3,7);
		lBoxE[11].Create(2,6);
		CCSG_Edge::CreateQuadLoop(lBoxE, 0x0000,0x0001,0x0002,0x0003, 10000);
		CCSG_Edge::CreateQuadLoop(lBoxE, 0x8007,0x8006,0x8005,0x8004, 10001);
		CCSG_Edge::CreateQuadLoop(lBoxE, 0x8000,0x0008,0x0004,0x8009, 10002);
		CCSG_Edge::CreateQuadLoop(lBoxE, 0x8001,0x0009,0x0005,0x800a, 10003);
		CCSG_Edge::CreateQuadLoop(lBoxE, 0x8002,0x000a,0x0006,0x800b, 10004);
		CCSG_Edge::CreateQuadLoop(lBoxE, 0x8003,0x000b,0x0007,0x8008, 10005);
		bBoxInit = true;
	}

	memcpy(lTempE, lBoxE, 12 * sizeof(CCSG_Edge));
	memcpy(lTempV, lBoxV, 8 * sizeof(CVec3Dfp64));


/*	int nV = 8;
	int nE = 12;
	int nVExists = 8;*/
//	FillChar(&lTempVRef, 8, 3);
//	FillChar(&lTempERef, 12, 2);

	EAlloc.Clear();
	EAlloc.FillAlloc(0, 12, 2);
	VAlloc.Clear();
	VAlloc.FillAlloc(0, 8, 3);

	EAlloc.Validate();
	VAlloc.Validate();

	if (m_lspPlanes.Len() > MaxPlanes)
		UpdateMeshError(CStrF("Too many planes. (%d > %d)", m_lspPlanes.Len(), MaxPlanes));

	uint16 liPlaneEdges[MaxPlanes];
	int nCurrentFaces = 0;

	// -------------------------------------------------------------------
	// Cut the wire box

	for(int iP = 0; iP < m_lspPlanes.Len(); iP++)
	{
		CSolid_Plane* pP = m_lspPlanes[iP];
		const CPlane3Dfp64& P = pP->m_Plane;

		// ----------------------------
		// Classify all used vertices
		int FullClipMask = 0;
		int nVECheck = 0;
		for(int v = 0; v < VAlloc.m_nItems; v++)
		{
			if (!VAlloc.m_lnRef[v])
				continue;

			nVECheck++;

			fp64 d = P.Distance(lTempV[v]);
			lTempVClipDist[v] = d;
			if (d < -CSOLID_EPSILON)
			{
				lTempVClipMask[v] = 2;
				FullClipMask |= 2;
			}
			else if (d > CSOLID_EPSILON)
			{
				lTempVClipMask[v] = 1;
				FullClipMask |= 1;
			}
			else
				lTempVClipMask[v] = 4;
		}

		if (nVECheck != VAlloc.m_nAllocated)
			UpdateMeshError("Internal error. (14)");

		// All vertices on back side == nothing was cut
		if (FullClipMask == 2)
			continue;

		// ----------------------------
		// Clip edges
		int iEdgeFirstClip = CSG_INVALID;

		for(int e = 0; e < EAlloc.m_nItems; e++)
		{
			if (!EAlloc.m_lnRef[e])
				continue;

			CCSG_Edge& E = lTempE[e];
			uint iv0 = E.m_liV[0];
			uint iv1 = E.m_liV[1];
			if (iv0 >= VAlloc.m_nItems || iv1 >= VAlloc.m_nItems)
				UpdateMeshError("Internal error. (15)");

			int Mask0 = lTempVClipMask[iv0];
			int Mask1 = lTempVClipMask[iv1];
			int Mask = Mask0 | Mask1;
			if ((Mask & 3) == 3)
			{
				// Cut edge
				if (iEdgeFirstClip == CSG_INVALID)
					iEdgeFirstClip = e;

				CVec3Dfp64 VNew;
				lTempV[iv0].Lerp(lTempV[iv1], M_Fabs(lTempVClipDist[iv0]) / (M_Fabs(lTempVClipDist[iv0]) + M_Fabs(lTempVClipDist[iv1])), VNew);

//				if (nV >= MaxVertices)
//					UpdateMeshError("Internal error. (12)");
				int iVNew = VAlloc.Alloc();
				lTempV[iVNew] = VNew;
				lTempVClipDist[iVNew] = 0.0;
				lTempVClipMask[iVNew] = 4;

				if (Mask0 & 1)
				{
					VAlloc.DelRef(iv0);
					E.m_liV[0] = iVNew;
				}
				else if (Mask1 & 1)
				{
					VAlloc.DelRef(iv1);
					E.m_liV[1] = iVNew;
				}
				else
					UpdateMeshError("Internal error. (1)");
			}
			else if ((Mask & 3) != 2)
			{
				// Remove edge (we keep stuff on the backside of the clip planes)
				if (E.m_liFace[0] != CSG_INVALID)
				{
	                lTempE[E.m_liEdgePrev[0]].SetNext(E.m_liEdgeNext[0], E.m_liFace[0]);
		            lTempE[E.m_liEdgeNext[0]].SetPrev(E.m_liEdgePrev[0], E.m_liFace[0]);
				}
				if (E.m_liFace[1] != CSG_INVALID)
				{
	                lTempE[E.m_liEdgePrev[1]].SetNext(E.m_liEdgeNext[1], E.m_liFace[1]);
		            lTempE[E.m_liEdgeNext[1]].SetPrev(E.m_liEdgePrev[1], E.m_liFace[1]);
				}

				VAlloc.DelRef(iv0);
				VAlloc.DelRef(iv1);
/*				lTempVRef[iv0]--;
				if (!lTempVRef[iv0])
					nVExists--;
				lTempVRef[iv1]--;
				if (!lTempVRef[iv1])
					nVExists--;*/

				E.m_liV[0] = CSG_INVALID;
				E.m_liV[1] = CSG_INVALID;
				EAlloc.Free(e);
//				lTempERef[e] = 0;
			}

		}
		
		// ----------------------------
		// Reconstruct loops

		int nEdgeIns = 0;

//		MemSetW(&liPlaneEdges, CSG_INVALID, m_lspPlanes.Len());

		uint16 liBasePlaneEdges[6];
		{
			int nP = m_lspPlanes.Len();
			if (nP > MaxPlanes)
				UpdateMeshError("Internal error. (16)");

			for(int p = 0; p < nP; p++)
				liPlaneEdges[p] = CSG_INVALID;

			for(int i = 0; i < 6; i++)
				liBasePlaneEdges[i] = CSG_INVALID;
		}

		nCurrentFaces = 0;
		MemSet(lEdgeDone, 0, EAlloc.m_nItems);

		for(int e = 0; e < EAlloc.m_nItems; e++)
		{
			if (!EAlloc.m_lnRef[e])
				continue;

			for(int f = 0; f < 2; f++)
			{
				if (lEdgeDone[e] & (1 << f))
					continue;

				int iFace = lTempE[e].m_liFace[f];
				if (iFace == iP)
					continue;

				if (iFace >= 10000)
				{
					int p = iFace - 10000;
//					if (liBasePlaneEdges[p] != CSG_INVALID)
//						continue;
					if (liBasePlaneEdges[p] == CSG_INVALID)
						nCurrentFaces++;
					liBasePlaneEdges[p] = e;
				}
				else
				{
//					if (liPlaneEdges[iFace] != CSG_INVALID)
//						continue;
					if (liPlaneEdges[iFace] == CSG_INVALID)
						nCurrentFaces++;
					liPlaneEdges[iFace] = e;
				}

				// Check if this face has already been processed
/*				if ((iFace < 10000) && liPlaneEdges[iFace] != CSG_INVALID)
					continue;

				if (iFace < 10000)
					liPlaneEdges[iFace] = e;*/

//				nCurrentFaces++;

				int iE = e;
				do
				{
					int Side = lTempE[iE].GetSide(iFace);
					lEdgeDone[iE] |= (1 << Side);

					int iV1 = lTempE[iE].m_liV[1 ^ Side];
					int iE2 = lTempE[iE].GetNextEdge(iFace);
					int Side2 = lTempE[iE2].GetSide(iFace);
					int iV0 = lTempE[iE2].m_liV[0 ^ Side2];
					if (iV1 != iV0)
					{
						// Insert new edge
//						if (nE >= MaxEdges)
//							UpdateMeshError("Internal error. (13)");

						int iENew = EAlloc.Alloc();;
						CCSG_Edge& ENew = lTempE[iENew];
						ENew.Create(iV1, iV0);
						VAlloc.AddRef(iV0);
						VAlloc.AddRef(iV1);

//						lTempVRef[iV0]++;
//						lTempVRef[iV1]++;
						EAlloc.AddRef(iENew);
//						lTempERef[iENew] = 2;
//						nE++;

						ENew.m_liFace[0] = iFace;
						ENew.m_liFace[1] = iP;
						ENew.m_liEdgePrev[0] = iE;
						ENew.m_liEdgeNext[0] = iE2;
						lTempE[iE].m_liEdgeNext[Side] = iENew;
						lTempE[iE2].m_liEdgePrev[Side2] = iENew;

						liEdgeInsert[nEdgeIns++] = iENew;
					}
					iE = iE2;
				}
				while(iE != e);
			}
		}

		// ----------------------------
		// Merge dual edges
		{
RedoMerge:
			for(int e = 0; e < nEdgeIns-1; e++)
				for(int f = e+1; f < nEdgeIns; f++)
				{
					int iE0 = liEdgeInsert[e];
					int iE1 = liEdgeInsert[f];
					int iv0a = lTempE[iE0].GetVert0(iP);
					int iv1a = lTempE[iE0].GetVert1(iP);
					int iv0b = lTempE[iE1].GetVert0(iP);
					int iv1b = lTempE[iE1].GetVert1(iP);

					if ((iv0a == iv1b) && (iv1a == iv0b))
					{
						// Merge edges
						int iFace2 = lTempE[iE1].GetOtherFace(iP);
						int Side0 = lTempE[iE0].GetSide(iP);
						lTempE[iE0].m_liFace[Side0] = iFace2;
						const CCSG_Edge& E1 = lTempE[iE1];
						int Side = E1.GetSide(iFace2);
						lTempE[E1.m_liEdgePrev[Side]].SetNext(iE0, iFace2);
						lTempE[E1.m_liEdgeNext[Side]].SetPrev(iE0, iFace2);
						lTempE[iE0].SetPrev(E1.m_liEdgePrev[Side], iFace2);
						lTempE[iE0].SetNext(E1.m_liEdgeNext[Side], iFace2);
						VAlloc.DelRef(iv0b);
						VAlloc.DelRef(iv1b);
						EAlloc.Free(iE1);
//						lTempERef[iE1] = 0;

						// Remove edge e and f from liEdgeInsert
						int i = e;
						for(; i < f-1; i++)
							liEdgeInsert[i] = liEdgeInsert[i+1];
						for(; i < nEdgeIns-2; i++)
							liEdgeInsert[i] = liEdgeInsert[i+2];
						nEdgeIns -= 2;
						goto RedoMerge;
					}
				}
		}

		// ----------------------------
		// Create loop for clip plane

		if (nEdgeIns == 1)
		{
			// Wtf?
			int a = 1;
		}
		if (nEdgeIns == 2)
		{
			// Can't make a face with 2 edges, these should be the same edge
			int iE0 = liEdgeInsert[0];
			int iE1 = liEdgeInsert[1];
			int iv0a = lTempE[iE0].GetVert0(iP);
			int iv1a = lTempE[iE0].GetVert1(iP);
			int iv0b = lTempE[iE1].GetVert0(iP);
			int iv1b = lTempE[iE1].GetVert1(iP);
			if ((iv0a == iv1b) && (iv1a == iv0b))
			{
				// Merge edges
				int iFace2 = lTempE[iE1].GetOtherFace(iP);
				int Side0 = lTempE[iE0].GetSide(iP);
				lTempE[iE0].m_liFace[Side0] = iFace2;
				const CCSG_Edge& E1 = lTempE[iE1];
				int Side = E1.GetSide(iFace2);
				lTempE[E1.m_liEdgePrev[Side]].SetNext(iE0, iFace2);
				lTempE[E1.m_liEdgeNext[Side]].SetPrev(iE0, iFace2);
				lTempE[iE0].SetPrev(E1.m_liEdgePrev[Side], iFace2);
				lTempE[iE0].SetNext(E1.m_liEdgeNext[Side], iFace2);
//				lTempERef[iE1] = 0;

				VAlloc.DelRef(iv0b);
				VAlloc.DelRef(iv1b);
				EAlloc.Free(iE1);
			}
			else
			{
				// wtf??
				UpdateMeshError("Internal error. (3)");
			}
		}
		else if (nEdgeIns > 2)
		{
			// Find next/prev links for the new loop
			while(nEdgeIns)
			{
				if (nEdgeIns < 3)
				{
					// Wtf?
					int a = 1;
				}

				int iEStart = liEdgeInsert[nEdgeIns-1];
				nEdgeIns--;

				int iVStart = lTempE[iEStart].GetVert0(iP);
				int iE = iEStart;
				int iV1 = CSG_INVALID;
				do
				{
					iV1 = lTempE[iE].GetVert1(iP);

					// First, look for an edge with the right start vertex AND with an other face's next edge matching
					// the prev edge of the test edge's other face. Capishe?
					int e = 0;
					for(; e < nEdgeIns; e++)
						if ((lTempE[liEdgeInsert[e]].GetVert0(iP) == iV1) &&
							(lTempE[iE].GetPrevEdge(lTempE[iE].GetOtherFace(iP)) == 
							lTempE[liEdgeInsert[e]].GetNextEdge(lTempE[liEdgeInsert[e]].GetOtherFace(iP) )))
						{
							int iE2 = liEdgeInsert[e];
							lTempE[iE].SetNext(iE2, iP);
							lTempE[iE2].SetPrev(iE, iP);
							iV1 = lTempE[iE2].GetVert1(iP);
							iE = iE2;

							// Remove found edge from ins list
							for(int i = e; i < nEdgeIns-1; i++)
								liEdgeInsert[i] = liEdgeInsert[i+1];
							nEdgeIns--;

							e = 0x7fffffff;
							break;
						}

					// Didn't find an edge with next/prev edge mumbo jumbo matching, so just look for an edge with the
					// correct start vertex.
					if (e < 0x7fffffff)
					for(e = 0; e < nEdgeIns; e++)
						if (lTempE[liEdgeInsert[e]].GetVert0(iP) == iV1)
						{
							int iE2 = liEdgeInsert[e];
							lTempE[iE].SetNext(iE2, iP);
							lTempE[iE2].SetPrev(iE, iP);
							iV1 = lTempE[iE2].GetVert1(iP);
							iE = iE2;

							// Remove found edge from ins list
							for(int i = e; i < nEdgeIns-1; i++)
								liEdgeInsert[i] = liEdgeInsert[i+1];
							nEdgeIns--;

							e = 0x7fffffff;
							break;
						}

					if (e < 0x7fffffff)
					{
						UpdateMeshError("Internal error. (18)");
					}
				}
				while(iV1 != iVStart);

				lTempE[iE].SetNext(iEStart, iP);
				lTempE[iEStart].SetPrev(iE, iP);

				liPlaneEdges[iP] = iEStart;
			}
			nCurrentFaces++;
		}

	}

	// ----------------------------
	// Count remaining edges
/*	int nEExists = 0;
	{
		for(int e = 0; e < nE; e++)
		{
			if (!lTempERef[e])
				continue;

			nEExists++;

//			LogFile(CStrF("Edge %d: ", e) + lTempE[e].GetString());
		}
	}*/

	// ----------------------------
	// Create geometry
//	if (0)
	{
		// ----------------------------
		// Create vertex array and remapping table
		int nVExists = VAlloc.m_nAllocated;
		m_lVerticesD.SetLen(nVExists);
		CVec3Dfp64* pVDst = m_lVerticesD.GetBasePtr();

		uint16* piVRemap = liEdgeInsert;
		int nNewV = 0;
		for(int v = 0; v < VAlloc.m_nItems; v++)
		{
			if (VAlloc.m_lnRef[v])
			{
				if (nNewV >= nVExists)
					UpdateMeshError(CStrF("Vertex count missmatch. (%d != %d)", nNewV, nVExists));
				pVDst[nNewV] = lTempV[v];
				piVRemap[v] = nNewV;
				nNewV++;
			}
			else
				piVRemap[v] = CSG_INVALID;
		}

		// ----------------------------
		// Create edge array and edge remapping table
		int nEExists = EAlloc.m_nAllocated;
		m_lEdges.SetLen(nEExists);
		CSolid_Edge* pE = m_lEdges.GetBasePtr();

		uint16 liERemap[MaxEdges];
		int nENew = 0;
		for(int e = 0; e < EAlloc.m_nItems; e++)
		{
			if (EAlloc.m_lnRef[e])
			{
				if (nENew >= nEExists)
					UpdateMeshError("Internal error. (7)");

				pE[nENew].m_iV[0] = piVRemap[lTempE[e].m_liV[0]];
				pE[nENew].m_iV[1] = piVRemap[lTempE[e].m_liV[1]];

				liERemap[e] = nENew;
				nENew++;
			}
			else
				liERemap[e] = CSG_INVALID;
		}

		// ----------------------------
		// Get a start edges for faces and count number of real faces
		{
			int nP = m_lspPlanes.Len();
			if (nP > MaxPlanes)
				UpdateMeshError("Internal error. (17)");

			for(int p = 0; p < nP; p++)
				liPlaneEdges[p] = CSG_INVALID;
		}

		nCurrentFaces = 0;

		for(int e = 0; e < EAlloc.m_nItems; e++)
		{
			if (!EAlloc.m_lnRef[e])
				continue;

			for(int f = 0; f < 2; f++)
			{
				int iFace = lTempE[e].m_liFace[f];

				if (iFace >= 10000)
				{
					int b = 1;
/*					int p = iFace - 10000;
					if (liBasePlaneEdges[p] != CSG_INVALID)
						continue;
					liBasePlaneEdges[p] = e;*/
				}
				else
				{
					if (liPlaneEdges[iFace] != CSG_INVALID)
						continue;
					liPlaneEdges[iFace] = e;
					nCurrentFaces++;
				}

			}
		}


		// ----------------------------
	// Create faces
		m_lFaces.SetLen(nCurrentFaces);
		int nF = 0;
		int nP = m_lspPlanes.Len();

		int nVRef = 0;
		{
			for(int i = 0; i < nP; i++)
			{
				if (liPlaneEdges[i] == CSG_INVALID)
					continue;

				int nv = CCSG_Edge::GetLoopEdgeCount(pTempE, liPlaneEdges[i], i);
				if (nv < 3)
					UpdateMeshError("Internal error. (10)");

				nVRef += nv;
			}
		}

		m_liVertices.SetLen(nVRef);
		m_lTVertices.SetLen(nVRef);
		int iVBase = 0;
		
		for(int i = 0; i < nP; i++)
		{
			if (liPlaneEdges[i] == CSG_INVALID)
				continue;

			int nv = CCSG_Edge::GetLoopEdgeCount(pTempE, liPlaneEdges[i], i);
			if (nv < 3)
				UpdateMeshError("Internal error. (10)");

			CSolid_Face& Face = m_lFaces[nF];

//			spF->m_liVertices.SetGrow(1);
//			spF->m_liVertices.SetLen(nv);
//			spF->m_liEdges.SetLen(nv);

			Face.m_iiV = iVBase;
			Face.m_nV = nv;

			uint16* piVertices = &m_liVertices[iVBase];
			// Loop through edges
			{
//				uint32* piVertices = Face.m_liVertices.GetBasePtr();
//				int* piEdges = Face.m_liEdges.GetBasePtr();

				int iEStart = liPlaneEdges[i];
				int iE = iEStart;
				int v = 0;
				do
				{
					piVertices[v] = piVRemap[lTempE[iE].GetVert0(i)];
					if (piVertices[v] == CSG_INVALID || (uint(piVertices[v]) >= nNewV))
						UpdateMeshError("Internal error. (9a)");
//					piEdges[v] = liERemap[iE];
//					if (piEdges[v] == CSG_INVALID || (uint(piEdges[v]) >= nENew))
//						UpdateMeshError("Internal error. (9b)");
					iE = lTempE[iE].GetNextEdge(i);
					v++;
				}
				while(iE != iEStart);

				if (v != nv)
					UpdateMeshError("Internal error. (5)");
			}

			// Copy plane and mapping
			Face.m_spPlane = m_lspPlanes[i];
//			Face.m_lTVertices.SetLen(nv);
			if (Face.m_spPlane->m_spMapping)
				Face.m_spPlane->m_spMapping->GenerateUV(&Face.m_spPlane->m_Plane, m_lVerticesD.GetBasePtr(), piVertices, nv, &m_lTVertices[iVBase]);

			// Create stupid vertices
/*			{
				CPlane3Dfp64& Plane = Face.m_spPlane->m_Plane;
				Face.m_spPlane->m_v0 = Plane.GetPointInPlane();
				CVec3Dfp64 cross, binormal, tangent;
				if (M_Fabs(Plane.n.k[0]) > 0.5f)
					cross = CVec3Dfp64(0, 1, 0);
				else
					cross = CVec3Dfp64(1, 0, 0);
				Plane.n.CrossProd(cross, binormal);
				binormal.Normalize();
				Plane.n.CrossProd(binormal, tangent);
				Face.m_spPlane->m_v0.Combine(binormal, 100.0f, Face.m_spPlane->m_v2);
				Face.m_spPlane->m_v0.Combine(tangent, 100.0f, Face.m_spPlane->m_v1);
			}*/

			// Create stupid vertices
			Face.m_spPlane->m_v0 = pVDst[piVertices[0]];
			Face.m_spPlane->m_v1 = pVDst[piVertices[1]];
			Face.m_spPlane->m_v2 = pVDst[piVertices[2]];

			iVBase += nv;
			nF++;
		}

		if (iVBase != nVRef)
			UpdateMeshError("Internal error. (19)");

		if (nF != m_lFaces.Len())
			UpdateMeshError("Internal error. (8)");

	}

	// Rebuild plane array from planes in faces
	{
		m_lspPlanes.SetLen(m_lFaces.Len());
		spCSolid_Plane* plspPlanes = m_lspPlanes.GetBasePtr();
		for(int iFace = 0; iFace < m_lFaces.Len(); iFace++)
			plspPlanes[iFace] = m_lFaces[iFace].m_spPlane;
	}

	UpdateBound();	
}

#endif

void CSolid::SetFaceColor(int _iFace, int _iCol)
{
	MAUTOSTRIP(CSolid_SetFaceColor, MAUTOSTRIP_VOID);

	CSolid_Face& Face = GetFaceNonConst(_iFace);

//	if(_iCol != 0xffffffff)
//		Face.m_iInvalid = 1;
//	else
		Face.m_iInvalid = 0;
	Face.m_Color = _iCol;

	// Check for concave planes
	if((CSG_GetCutMask(m_lspPlanes[_iFace]->m_Plane) & 3) == 3)
	{
		Face.m_Color = CPixel32(0xffffef00);
		Face.m_iInvalid |= 2;
	}

	// Check for vertices out of plane.
	int nV = Face.m_nV;
	const uint16* piV = GetFaceVertexIndices(_iFace);
	const CPlane3Dfp64& Plane = m_lspPlanes[_iFace]->m_Plane;

	for(int j = 0; j < nV; j++)
	{
		if(Plane.GetPlaneSide_Epsilon(m_lVerticesD[piV[j]], 0.01f) != 0)
		{
			Face.m_Color = CPixel32(0xffff0000);
			Face.m_iInvalid |= 3;
			break;
		}
	}
}

void CSolid::SetFaceColors()
{
	MAUTOSTRIP(CSolid_SetFaceColors, MAUTOSTRIP_VOID);
	int nFaces = GetNumFaces();
	for(int i = 0; i < nFaces; i++)
		SetFaceColor(i, 0xffffffff);
}

int CSolid::GetMappingType(int _iFace)
{
	MAUTOSTRIP(CSolid_GetMappingType, 0);
	if(m_lspPlanes[_iFace] && m_lspPlanes[_iFace]->m_spMapping)
		return m_lspPlanes[_iFace]->m_spMapping->GetType();
	return -1;
}

bool CSolid::SetMappingType(int _iFace, int _iMapping)
{
	if(_iFace == -1)
	{
		int nFaces = m_lspPlanes.Len();
		for(int i = 0; i < nFaces; i++)
			SetMappingType(i, _iMapping);
		return false;
	}

	if(GetMappingType(_iFace) == _iMapping)
		return false;

	MAUTOSTRIP(CSolid_SetMappingType, false);
	CSolid_Plane* pP = m_lspPlanes[_iFace];

	CXR_PlaneMapping PMap;
	if (pP->m_spMapping)
		PMap = pP->m_spMapping->GetMappingPlane(pP->m_Plane);
	else
		PMap.CreateUnit();

	spCSolid_Mapping spOldMap = pP->m_spMapping;

	switch(_iMapping)
	{
	case OGIER_MAPPINGTYPE_PLANEMAPPED : 
		{
			pP->m_spMapping = MNew(CSolid_MappingPlane);
			if (!pP->m_spMapping) MemError("ParseString");
			break;
		}
	case OGIER_MAPPINGTYPE_BOXMAPPED : 
		{
			pP->m_spMapping = MNew(CSolid_MappingMAP);
			if (!pP->m_spMapping) MemError("ParseString");
			break;
		}
	}
	pP->m_spMapping->SetMappingPlane(PMap, pP->m_Plane);
	if (spOldMap)
	{
		pP->m_spMapping->m_pSurface = spOldMap->m_pSurface;
		pP->m_spMapping->m_SurfName = spOldMap->m_SurfName;
	}

	UpdateSurface(_iFace);
	return false;
}

void CSolid::CreateFromBox(const CVec3Dfp64& _VMin, const CVec3Dfp64& _VMax)
{
	MAUTOSTRIP(CSolid_CreateFromBox, MAUTOSTRIP_VOID);
	m_lspPlanes.Clear();
	m_lVerticesD.Clear();
	m_lEdges.Clear();
	m_lTVertices.Clear();
	m_lFaces.Clear();
	m_liVertices.Clear();
	m_Medium.Clear();

	m_lspPlanes.SetGrow(6);
	m_lVerticesD.SetGrow(8);
	m_lEdges.SetGrow(12);
	m_liVertices.SetGrow(24);
	m_lTVertices.SetGrow(24);
	m_lFaces.SetGrow(6);

	m_Flags = 0;
	m_ModelMask = ~0;

	m_lspPlanes.Add(MNew1(CSolid_Plane, CPlane3Dfp64(CVec3Dfp64( 1,  0,  0), -_VMax.k[0])));
	m_lspPlanes.Add(MNew1(CSolid_Plane, CPlane3Dfp64(CVec3Dfp64(-1,  0,  0),  _VMin.k[0])));
	m_lspPlanes.Add(MNew1(CSolid_Plane, CPlane3Dfp64(CVec3Dfp64( 0,  1,  0), -_VMax.k[1])));
	m_lspPlanes.Add(MNew1(CSolid_Plane, CPlane3Dfp64(CVec3Dfp64( 0, -1,  0),  _VMin.k[1])));
	m_lspPlanes.Add(MNew1(CSolid_Plane, CPlane3Dfp64(CVec3Dfp64( 0,  0,  1), -_VMax.k[2])));
	m_lspPlanes.Add(MNew1(CSolid_Plane, CPlane3Dfp64(CVec3Dfp64( 0,  0, -1),  _VMin.k[2])));
	m_lVerticesD.Add(CVec3Dfp64(_VMax.k[0], _VMin.k[1], _VMin.k[2]));
	m_lVerticesD.Add(CVec3Dfp64(_VMin.k[0], _VMax.k[1], _VMin.k[2]));
	m_lVerticesD.Add(CVec3Dfp64(_VMax.k[0], _VMax.k[1], _VMin.k[2]));
	m_lVerticesD.Add(CVec3Dfp64(_VMin.k[0], _VMin.k[1], _VMax.k[2]));
	m_lVerticesD.Add(CVec3Dfp64(_VMax.k[0], _VMax.k[1], _VMax.k[2]));
	m_lVerticesD.Add(CVec3Dfp64(_VMin.k[0], _VMin.k[1], _VMin.k[2]));
	m_lVerticesD.Add(CVec3Dfp64(_VMin.k[0], _VMax.k[1], _VMax.k[2]));
	m_lVerticesD.Add(CVec3Dfp64(_VMax.k[0], _VMin.k[1], _VMax.k[2]));

	static uint8 iEdgePair[24] = {0, 5, 7, 4, 1, 6, 4, 2, 3, 5, 4, 6, 1, 2, 6, 3, 0, 7, 3, 7, 5, 1, 2, 0};
	for(uint i = 0; i < 12; ++i)
		m_lEdges.Add(CSolid_Edge(iEdgePair[i * 2 + 0], iEdgePair[i * 2 + 1]));

	static uint8 iVert[24] = {7, 4, 2, 0, 1, 6, 3, 5, 6, 1, 2, 4, 0, 5, 3, 7, 4, 7, 3, 6, 5, 0, 2, 1};
	for(uint i = 0; i < 24; i++)
	{
		m_liVertices.Add(iVert[i]);
		m_lTVertices.Add(CVec2Dfp32(1.0f, 1.0f));
	}

	for(uint i = 0; i < 6; ++i)
	{
		CSolid_Face Face;
		Face.m_iiV = i * 4;
		Face.m_nV = 4;
		int iFace = m_lFaces.Add(Face);
		m_lFaces[i].m_spPlane = m_lspPlanes[i];

		static uint8 iPlaneData[6][3] = {{7, 4, 2}, {1, 6, 3}, {6, 1, 2}, {0, 5, 3}, {4, 7, 3}, {6, 0, 2}};
		m_lspPlanes[i]->m_v0 = m_lVerticesD[iPlaneData[i][0]];
		m_lspPlanes[i]->m_v1 = m_lVerticesD[iPlaneData[i][1]];
		m_lspPlanes[i]->m_v2 = m_lVerticesD[iPlaneData[i][2]];
	}
	
	m_lspPlanes.SetGrow(32);
	m_lVerticesD.SetGrow(32);
	m_lEdges.SetGrow(32);
	m_liVertices.SetGrow(32);
	m_lTVertices.SetGrow(32);
	m_lFaces.SetGrow(32);

	CVec3Dfp32 VMinfp32 = _VMin.Getfp32();
	CVec3Dfp32 VMaxfp32 = _VMax.Getfp32();

	m_BoundMin = VMinfp32;
	m_BoundMax = VMaxfp32;
	m_BoundPos = (VMinfp32 + VMaxfp32) * 0.5f;
	m_BoundRadius = (VMaxfp32 - m_BoundPos).Length();
}

void CSolid::CreateFromMesh()
{
	MAUTOSTRIP(CSolid_CreateFromMesh, MAUTOSTRIP_VOID);
	int nFaces = m_lFaces.Len();
	int i;
	for(i = 0; i < nFaces; i++)
	{
		const uint16* piV = GetFaceVertexIndices(i);
		m_lspPlanes[i]->m_v0 = m_lVerticesD[piV[0]];
		m_lspPlanes[i]->m_v1 = m_lVerticesD[piV[1]];
		m_lspPlanes[i]->m_v2 = m_lVerticesD[piV[2]];

		// Create plane
		m_lspPlanes[i]->m_Plane.Create(m_lspPlanes[i]->m_v0, m_lspPlanes[i]->m_v2, m_lspPlanes[i]->m_v1);
	}
	
	SetFaceColors();
	UpdateBound();

	for(i = 0; i < nFaces; i++)
	{
		UpdateSurface(i);
	}
}

void CSolid::UpdateSurface(int _iFace)
{
	MAUTOSTRIP(CSolid_UpdateSurface, MAUTOSTRIP_VOID);

	CSolid_Face& Face =	GetFaceNonConst(_iFace);
	if (Face.m_spPlane->m_spMapping)
	{
		const uint16* piV = GetFaceVertexIndices(Face);
		CVec2Dfp32* pTV = GetFaceTVerticesNonConst(Face);
		Face.m_spPlane->m_spMapping->GenerateUV(&Face.m_spPlane->m_Plane, m_lVerticesD.GetBasePtr(), piV, Face.m_nV, pTV);
	}
}


void CSolid::UpdateSurfaces(CXR_SurfaceContext *_pContext)
{
	MAUTOSTRIP(CSolid_UpdateSurfaces, MAUTOSTRIP_VOID);
	for(int p = 0; p < m_lspPlanes.Len(); p++)
	{
		int iIndex = _pContext->GetSurfaceID(m_lspPlanes[p]->m_spMapping->m_SurfName);
		if(iIndex != 0)
			m_lspPlanes[p]->m_spMapping->m_pSurface = _pContext->GetSurface(iIndex);
		else
			m_lspPlanes[p]->m_spMapping->m_pSurface = NULL;
		UpdateSurface(p);
	}
}

void CSolid::SetSurface(CXW_Surface *_pSurface, int _iPlane, int _bUpdateMapping)
{
	MAUTOSTRIP(CSolid_SetSurface, MAUTOSTRIP_VOID);
	if(_iPlane == -1)
	{
		int nr = m_lspPlanes.Len();
		for(int i = 0; i < nr; i++)
		{
			m_lspPlanes[i]->m_spMapping->m_SurfName = _pSurface->m_Name;
			m_lspPlanes[i]->m_spMapping->m_pSurface = _pSurface;
			if (_bUpdateMapping)
				UpdateSurface(i);
		}
	}
	else
	{
		m_lspPlanes[_iPlane]->m_spMapping->m_SurfName = _pSurface->m_Name;
		m_lspPlanes[_iPlane]->m_spMapping->m_pSurface = _pSurface;
		if (_bUpdateMapping)
			UpdateSurface(_iPlane);
	}
}

CXW_Surface *CSolid::GetSurface(int _iPlane)
{
	MAUTOSTRIP(CSolid_GetSurface, NULL);
	return m_lspPlanes[_iPlane]->m_spMapping->m_pSurface;
}

CStr CSolid::GetSurfaceName(int _iPlane)
{
	MAUTOSTRIP(CSolid_GetSurfaceName, CStr());
	return m_lspPlanes[_iPlane]->m_spMapping->m_SurfName;
}

// -------------------------------------------------------------------
fp64 TetrahedronVolume(const CVec3Dfp64& _v0, const CVec3Dfp64& _v1, const CVec3Dfp64& _v2, const CVec3Dfp64& _v3);
fp64 TetrahedronVolume(const CVec3Dfp64& _v0, const CVec3Dfp64& _v1, const CVec3Dfp64& _v2, const CVec3Dfp64& _v3)
{
	MAUTOSTRIP(TetrahedronVolume, 0.0f);
	CVec3Dfp64 a,b,c;
	_v1.Sub(_v0, a);
	_v2.Sub(_v0, b);
	_v3.Sub(_v0, c);
	return ((a / b) * c) / 6.0f;
}

fp64 CSolid::GetVolume()
{
	MAUTOSTRIP(CSolid_GetVolume, 0.0f);
	const CVec3Dfp64* pV = m_lVerticesD.GetBasePtr();
	int nV = m_lVerticesD.Len();
	CVec3Dfp64 C(0);
	for(int v = 0; v < nV; v++)
		C += pV[v];

	C *= 1.0 / nV;

	fp64 Vol = 0;
	for(int f = 0; f < m_lFaces.Len(); f++)
	{
		const CSolid_Face& Face = GetFace(f);
		const uint16* piV = GetFaceVertexIndices(Face);
		int nv = Face.m_nV;
		for(int v = 2; v < nv; v++)
			Vol += M_Fabs(TetrahedronVolume(C, pV[piV[0]], pV[piV[v-1]], pV[piV[v]]));
	}

	return Vol;
}

fp64 CSolid::GetArea(int _iFace)
{
	const CSolid_Face& Face = GetFace(_iFace);
	const CVec3Dfp64 &Normal = Face.m_spPlane->m_Plane.n;
	int nVert = Face.m_nV;

	// select largest abs coordinate to ignore for projection
	fp64 ax = Abs(Normal[0]);
	fp64 ay = Abs(Normal[1]);
	fp64 az = Abs(Normal[2]);

	int IgnoreCoord = 3;	// ignore z-coord
	if(ax > ay)
	{
		if (ax > az)
			IgnoreCoord = 1;    // ignore x-coord
	}
	else if (ay > az)
		IgnoreCoord = 2;   // ignore y-coord

	// compute area of the 2D projection
	fp64 Area = 0;

	const uint16* piV = GetFaceVertexIndices(Face);
	for(int i = 1, j = 2, k = 0; k < nVert;)
	{
		const CVec3Dfp64 &VI = m_lVerticesD[piV[i]];
		const CVec3Dfp64 &VJ = m_lVerticesD[piV[j]];
		const CVec3Dfp64 &VK = m_lVerticesD[piV[k]];

		i++;
		j++;
		k++;
		if(i == nVert)
			i = 0;
		if(j == nVert)
			j = 0;

		switch(IgnoreCoord)
		{
		case 1:
			Area += (VI[1] * (VJ[2] - VK[2]));
			continue;
		case 2:
			Area += (VI[0] * (VJ[2] - VK[2]));
			continue;
		case 3:
			Area += (VI[0] * (VJ[1] - VK[1]));
			continue;
		}
	}

	// scale to get area before projection
	switch(IgnoreCoord) 
	{
	case 1:
		Area = Abs(Area / (2 * ax));
		break;
	case 2:
		Area = Abs(Area / (2 * ay));
		break;
	case 3:
		Area = Abs(Area / (2 * az));
	}

	return Area;
}

CVec3Dfp64 CSolid::GetFaceCenter(int _iFace)
{
	const CSolid_Face& Face = GetFace(_iFace);
	const uint16* piV = GetFaceVertexIndices(Face);
	CVec3Dfp64 V = 0;

	int nVert = Face.m_nV;
	for(int i = 0; i < nVert; i++)
		V += m_lVerticesD[piV[i]];
	V *= 1.0f / nVert;
	return V;
}

// -------------------------------------------------------------------
bool CSolid::IntersectLocalLine(const CVec3Dfp64 &_v0, const CVec3Dfp64 &_v1)
{
	MAUTOSTRIP(CSolid_IntersectLocalLine, false);
	CVec3Dfp64 lv0 = _v0 + m_BoundPos.Getfp64();
	CVec3Dfp64 lv1 = _v1 + m_BoundPos.Getfp64();
//	CVec3Dfp64 lv0 = _v0;
//	CVec3Dfp64 lv1 = _v1;

	for(int p = 0; p < m_lspPlanes.Len(); p++)
	{
		CPlane3Dfp64* pPi = &m_lspPlanes[p]->m_Plane;

		fp64 d0 = pPi->Distance(lv0);
		fp64 d1 = pPi->Distance(lv1);
		if ((d0 > -CSOLID_EPSILON) && (d1 > -CSOLID_EPSILON))
			return false;
		if ((d0 > CSOLID_EPSILON) && (d1 < -CSOLID_EPSILON))
		{
			CVec3Dfp64 CutPoint;
			pPi->GetIntersectionPoint(lv0, lv1, CutPoint);
			lv0 = CutPoint;
		}
		else if ((d1 > CSOLID_EPSILON) && (d0 < -CSOLID_EPSILON))
		{
			CVec3Dfp64 CutPoint;
			pPi->GetIntersectionPoint(lv0, lv1, CutPoint);
			lv1 = CutPoint;
		}
	}

	return (lv1 - lv0).LengthSqr() > CSOLID_EPSILON;
}

fp32 CSolid::Ogr_IntersectRay(const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dir)
{
	MAUTOSTRIP(CSolid_Ogr_IntersectRay, 0.0f);
	CVec3Dfp64 v(_Dir.k[0], _Dir.k[1], _Dir.k[2]);
	v.Normalize();
	CVec3Dfp64 v2(v);
	v2 *= 10000000.0;
	CVec3Dfp64 v0(_Origin.k[0], _Origin.k[1], _Origin.k[2]);
	CVec3Dfp64 v1(v0);
	v1 += v2;

	int iPlane = -1;

	for(int p = 0; p < m_lspPlanes.Len(); p++)
	{
		CPlane3Dfp64* pPi = &m_lspPlanes[p]->m_Plane;

		fp64 dotp = pPi->n * v;
//		if (M_Fabs(dotp) < (1.0 - CSOLID_EPSILON))
		{
			// Not parallell
			fp64 d0 = pPi->Distance(v0);
			fp64 d1 = pPi->Distance(v1);
			if ((d0 > CSOLID_EPSILON) && (d1 > CSOLID_EPSILON)) return -1.0f;

			if ((d0 > CSOLID_EPSILON) && (d1 < -CSOLID_EPSILON))
			{
				CVec3Dfp64 CutPoint;
				pPi->GetIntersectionPoint(v0, v1, CutPoint);
				v0 = CutPoint;
				iPlane = p;
			}
			else if ((d1 > CSOLID_EPSILON) && (d0 < -CSOLID_EPSILON))
			{
				CVec3Dfp64 CutPoint;
				pPi->GetIntersectionPoint(v0, v1, CutPoint);
				v1 = CutPoint;
			}
		}
	}

	if(iPlane != -1)
		if(m_lspPlanes[iPlane]->m_spMapping && m_lspPlanes[iPlane]->m_spMapping->m_pSurface)
		{
			if(m_lspPlanes[iPlane]->m_spMapping->m_pSurface->m_Flags & XW_SURFFLAGS_OGRINVISIBLE)
				return -1;
		}

	return Length3(_Origin.k[0] - v0.k[0], _Origin.k[1] - v0.k[1], _Origin.k[2] - v0.k[2]);
}

fp32 CSolid::IntersectFace(const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dir, int _iFace)
{
	MAUTOSTRIP(CSolid_IntersectFace, 0.0f);
	CVec3Dfp64 v(_Dir.k[0], _Dir.k[1], _Dir.k[2]);
	v.Normalize();
	CVec3Dfp64 v2(v);
	v2 *= 10000000.0;
	CVec3Dfp64 v0(_Origin.k[0], _Origin.k[1], _Origin.k[2]);
	CVec3Dfp64 v1(v0);
	v1 += v2;

	const CSolid_Face& Face = GetFace(_iFace);
	CPlane3Dfp64* pPi = &m_lspPlanes[_iFace]->m_Plane;

	fp64 dotp = pPi->n * v;
//	if (M_Fabs(dotp) > (1.0 - CSOLID_EPSILON)) return -1;
	// Not parallell
	fp64 d0 = pPi->Distance(v0);
	fp64 d1 = pPi->Distance(v1);

	if (((d0 > CSOLID_EPSILON) && (d1 < -CSOLID_EPSILON)) ||
		((d1 > CSOLID_EPSILON) && (d0 < -CSOLID_EPSILON)))
	{
		const uint16* piV = GetFaceVertexIndices(Face);
		CVec3Dfp64 CutPoint;
		pPi->GetIntersectionPoint(v0, v1, CutPoint);

		int nv = Face.m_nV;
		int iv2 = piV[nv-1];
		for(int v = 0; v < nv; v++)
		{
			int iv1 = piV[v];
			CPlane3Dfp64 p;
			p.Create(m_lVerticesD[iv1], m_lVerticesD[iv2], v0);
			iv2 = iv1;
			if (p.Distance(CutPoint) > 0.0f) return -1;
		}

		fp32 d = Length3(_Origin.k[0] - CutPoint.k[0], _Origin.k[1] - CutPoint.k[1], _Origin.k[2] - CutPoint.k[2]);
		return d;
	}
	else
		return -1;
}

int CSolid::Ogr_IntersectFaces(const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dir, fp32 &_Dist)
{
	MAUTOSTRIP(CSolid_Ogr_IntersectFaces, 0);
	fp32 MinHit = _FP32_MAX;
	int MinFace = -1;

	int nF = GetNumFaces();
	for(int i = 0; i < nF; i++)
	{
		CXW_Surface* pSurf = GetSurface(i);
		if (pSurf && (pSurf->m_Flags & XW_SURFFLAGS_OGRINVISIBLE))
			continue;

		fp32 Dist = IntersectFace(_Origin, _Dir, i);
		if (Dist > 0.0f && Dist < MinHit)
		{
			MinHit = Dist;
			MinFace = i;
		}
	}
	_Dist = MinHit;
	return MinFace;
}

// -------------------------------------------------------------------
//  CONSTRUCTIVE SOLID GEOMETRY
// -------------------------------------------------------------------
int CSolid::CSG_GetCutMask(const CPlane3Dfp64& _Plane) const
{
	MAUTOSTRIP(CSolid_CSG_GetCutMask, 0);
	return _Plane.GetArrayPlaneSideMask_Epsilon(m_lVerticesD.GetBasePtr(), m_lVerticesD.Len(), CSOLID_EPSILON*100.0f);
}

bool CSolid::CSG_Intersection(const CSolid& _Solid) const
{
	MAUTOSTRIP(CSolid_CSG_Intersection, false);
	if (m_Medium.m_iPhysGroup != _Solid.m_Medium.m_iPhysGroup)
		return false;

	for(int i = 0; i < 3; i++)
	{
		if (m_BoundMin.k[i] - _Solid.m_BoundMax.k[i] > CSOLID_EPSILON*1000.0f) return false;
		if (_Solid.m_BoundMin.k[i] - m_BoundMax.k[i] > CSOLID_EPSILON*1000.0f) return false;
	}

	// Does solid2 intersect any of solid1's planes?
	int iP;
	for(iP = 0; iP < m_lspPlanes.Len(); iP++)
		if (!(_Solid.CSG_GetCutMask(m_lspPlanes[iP]->m_Plane) & 2)) return false;

	// Does solid1 intersect any of solid2's planes?
	for(iP = 0; iP < _Solid.m_lspPlanes.Len(); iP++)
		if (!(CSG_GetCutMask(_Solid.m_lspPlanes[iP]->m_Plane) & 2)) return false;

	// Try to find a plane that separates the solids by creating planes 
	// parallell to all edge-combinations from the solids.
	for(int iE1 = 0; iE1 < m_lEdges.Len(); iE1++)
		for(int iE2 = 0; iE2 < _Solid.m_lEdges.Len(); iE2++)
		{
			CPlane3Dfp64 Plane;
			CVec3Dfp64 e1,e2;
			m_lVerticesD[m_lEdges[iE1].m_iV[1]].Sub(m_lVerticesD[m_lEdges[iE1].m_iV[0]], e1);
			_Solid.m_lVerticesD[_Solid.m_lEdges[iE2].m_iV[1]].Sub(_Solid.m_lVerticesD[_Solid.m_lEdges[iE2].m_iV[0]], e2);
			e1.CrossProd(e2, Plane.n);

			// Check if the edges were parallell, then don't use this (invalid) plane.
			if (Plane.n.LengthSqr() < 0.01) continue;

			Plane.CreateNV(Plane.n, m_lVerticesD[m_lEdges[iE1].m_iV[1]]);
			int Mask1 = CSG_GetCutMask(Plane) & 3;
			int Mask2 = _Solid.CSG_GetCutMask(Plane) & 3;
			if (!(Mask1 & Mask2)) return false;		// If they didn't share a side, the plane separated the solids.
		}

	return true;
}

bool CSolid::CSG_Inside(const CSolid& _Solid) const
{
	MAUTOSTRIP(CSolid_CSG_Inside, false);
	// Are any vertices of solid2 outside any of solid1's planes?
	for(int iP = 0; iP < m_lspPlanes.Len(); iP++)
		if ((_Solid.CSG_GetCutMask(m_lspPlanes[iP]->m_Plane) & 1)) return false;

	return true;
}

int CSolid::CSG_VertexInSolid(const CVec3Dfp64& _v, fp64 _Epsilon) const
{
	MAUTOSTRIP(CSolid_CSG_VertexInSolid, 0);
	// NOTE: 1 outside, 2 inside, 4 onsurface

	bool bOn = false;
	for(int iP = 0; iP < m_lspPlanes.Len(); iP++)
		if (m_lspPlanes[iP]->m_Plane.Distance(_v) > _Epsilon) 
			return 1;
		else
			if (m_lspPlanes[iP]->m_Plane.Distance(_v) > -_Epsilon) bOn = true;

	return (bOn) ? 4 : 2;
}

int CSolid::CSG_PolygonOutsideSolid(const CVec3Dfp64* _pV, const uint32* _piV, int _nV, fp64 _Epsilon) const
{
	MAUTOSTRIP(CSolid_CSG_PolygonOutsideSolid, 0);
	for(int p = 0; p < m_lspPlanes.Len(); p++)
	{
		int Mask = -1;
		if (_piV)
		{
			for(int v = 0; v < _nV; v++)
				Mask &= m_lspPlanes[p]->m_Plane.GetPlaneSideMask_Epsilon(_pV[_piV[v]], _Epsilon);
		}
		else
		{
			for(int v = 0; v < _nV; v++)
				Mask &= m_lspPlanes[p]->m_Plane.GetPlaneSideMask_Epsilon(_pV[v], _Epsilon);
		}

		if (Mask & 5) return true;
	}
	return false;
}

int CSolid::CSG_PolygonInSolid(const CVec3Dfp64* _pV, const uint32* _piV, int _nV, fp64 _Epsilon) const
{
	MAUTOSTRIP(CSolid_CSG_PolygonInSolid, 0);
	// NOTE: 1 outside, 2 inside, 4 onsurface
	int Mask = 0;
	if (_piV)
		for(int v = 0; v < _nV; v++)
			Mask |= CSG_VertexInSolid(_pV[_piV[v]], _Epsilon);
	else
		for(int v = 0; v < _nV; v++)
			Mask |= CSG_VertexInSolid(_pV[v], _Epsilon);

	return Mask;
}

int CSolid::CSG_GetPlaneIndex(const CPlane3Dfp64& _Plane, fp64 _Epsilon) const
{
	MAUTOSTRIP(CSolid_CSG_GetPlaneIndex, 0);
	CVec3Dfp64 P = _Plane.GetPointInPlane();
	for(int iP = 0; iP < m_lspPlanes.Len(); iP++)
	{
		if ((M_Fabs(m_lspPlanes[iP]->m_Plane.Distance(P)) < _Epsilon) &&
			M_Fabs(m_lspPlanes[iP]->m_Plane.n * _Plane.n / (m_lspPlanes[iP]->m_Plane.n.Length() * _Plane.n.Length()) - 1.0f) < _Epsilon) return iP;
	}
	return -1;
}

bool CSolid::CSG_PlaneExists(const CPlane3Dfp64& _Plane, fp64 _Epsilon) const
{
	MAUTOSTRIP(CSolid_CSG_PlaneExists, false);
	CVec3Dfp64 P = _Plane.GetPointInPlane();
	for(int iP = 0; iP < m_lspPlanes.Len(); iP++)
	{
		if ((M_Fabs(m_lspPlanes[iP]->m_Plane.Distance(P)) < _Epsilon) &&
			M_Fabs(m_lspPlanes[iP]->m_Plane.n * _Plane.n - 1.0f) < _Epsilon) return true;
	}
	return false;
}

bool CSolid::CSG_InversePlaneExists(const CPlane3Dfp64& _Plane, fp64 _Epsilon) const
{
	MAUTOSTRIP(CSolid_CSG_InversePlaneExists, false);
	CPlane3Dfp64 P(_Plane);
	P.Inverse();
	return CSG_PlaneExists(P, _Epsilon);
}

/*bool CSolid::CSG_PolygonCovered(const CVec3Dfp64* _pV, const uint32* _piV, int _nV, fp64 _Epsilon) const
{
	MAUTOSTRIP(CSolid_CSG_PolygonCovered, NULL);
	int Mask = CSG_PolygonInSolid(_pV, _piV, _nv, _Epsilon);
	if (Mask == 1) return false;
	if (Mask & 4) return true;

	for(int iP = 0; iP < 
	
}*/

bool CSolid::CSG_VertexExists(const CVec3Dfp64& _v, fp64 _Epsilon) const
{
	MAUTOSTRIP(CSolid_CSG_VertexExists, false);
	int nV = m_lVerticesD.Len();
	const CVec3Dfp64* pV = m_lVerticesD.GetBasePtr();
	for(int v = 0; v < nV; v++)
		if (pV[v].DistanceSqr(_v) < Sqr(_Epsilon))
			return true;
	return false;
}

void CSolid::CSG_CopyAttributes(const CSolid& _Solid)
{
	MAUTOSTRIP(CSolid_CSG_CopyAttributes, MAUTOSTRIP_VOID);
	m_Flags = _Solid.m_Flags;
	m_ModelMask = _Solid.m_ModelMask;
	m_Medium = _Solid.m_Medium;
}

int CSolid::CSG_CompareMediums(const CSolid& _Solid) const
{
	MAUTOSTRIP(CSolid_CSG_CompareMediums, 0);
	// Compares "hardness".
	return m_Medium.CompareMediums(_Solid.m_Medium);

/*	int bs1 = BitScanFwd32(m_Medium.m_MediumFlags & XW_MEDIUM_TYPEMASK);
	int bs2 = BitScanFwd32(_Solid.m_Medium.m_MediumFlags & XW_MEDIUM_TYPEMASK);
	if (bs1 < bs2) 
		return 1;
	else if (bs1 > bs2)
		return -1;
	else 
		return 0;*/
}

static int GetSurfaceIndex(TArray<spCXW_Surface> _lspSurfaces, const char* _pSurfName)
{
	MAUTOSTRIP(GetSurfaceIndex, 0);
	int nS = _lspSurfaces.Len();
	for(int iS = 0; iS < nS; iS++)
		if (!_lspSurfaces[iS]->m_Name.CompareNoCase(_pSurfName)) return iS;
	return -1;
}

void CSolid::CSG_GetMediumFromSurfaces()
{
	MAUTOSTRIP(CSolid_CSG_GetMediumFromSurfaces, MAUTOSTRIP_VOID);
	// Visible surface has priority over invisible surface
	// Surfaces with the same visibility and different mediums is
	// considered a minor error and is reported in the log.

	if (!m_lspPlanes.Len()) return;
	CXR_MediumDesc Medium;
	CXW_Surface* pMediumSurf = NULL;
	Medium.SetAir();
	for(int iP = 0; iP < m_lspPlanes.Len(); iP++)
	{
//		int iSurf = GetSurfaceIndex(_lspSurfaces, m_lspPlanes[iP]->m_spMapping->m_SurfName);

		CXR_MediumDesc SurfMedium;
		CXW_Surface* pS = GetSurface(iP);;
		if (pS)
		{
			CXW_SurfaceKeyFrame* pSKF = pS->GetBaseFrame();
			SurfMedium = pSKF->m_Medium;
			//LogFile(CStrF("    Plane %d medium %.8x, Name %s", iP, SurfMedium.m_MediumFlags, (char*) pS->m_Name));
		}
		else
			LogFile(CStrF("WARNING: CSG_GetMediumFromSurfaces run on solid without initialized surfaces.  BoundBox (%s, %s), BoundPos %s", m_BoundMin.GetString().Str(), m_BoundMax.GetString().Str(), m_BoundPos.GetString().Str()));

		if (!iP || !pMediumSurf)
		{
			Medium = SurfMedium;
			pMediumSurf = pS;
		}
		else if (pS != NULL)
		{
			if (Medium.m_CSGPriority > SurfMedium.m_CSGPriority)
			{
				// Current has higher prioriy, ignore
			}
			else if (Medium.m_CSGPriority < SurfMedium.m_CSGPriority)
			{
				// Current has lower priority, replace
				Medium = SurfMedium;
				pMediumSurf = pS;
			}
			else if ((pMediumSurf->m_Flags & XW_SURFFLAGS_INVISIBLE) &&
				!(pS->m_Flags & XW_SURFFLAGS_INVISIBLE))
			{
				// Current is invisible and new is visible, replace
				Medium = SurfMedium;
				pMediumSurf = pS;
			}
			else if (!(pMediumSurf->m_Flags & XW_SURFFLAGS_INVISIBLE) &&
				(pS->m_Flags & XW_SURFFLAGS_INVISIBLE))
			{
				// Current is visible and new is invisible, ignore
			}
			else if (!Medium.Equal(SurfMedium))
			{
				LogFile(CStrF("WARNING: Inconsistent mediums defined by brush surfaces, %s (Medium %.8x) is in conflict with %s (Medium %.8x) BoundBox (%s, %s), BoundPos %s", 
					(char*)pS->m_Name, SurfMedium.m_MediumFlags, (char*)pMediumSurf->m_Name, Medium.m_MediumFlags, m_BoundMin.GetString().Str(), m_BoundMax.GetString().Str(), m_BoundPos.GetString().Str()));
			}
		}
//LogFile(CStrF("%.8x, ", Medium.m_MediumFlags) + m_lspPlanes[0]->m_spMapping->m_SurfName);
	}

	if (Medium.m_MediumFlags & XW_MEDIUM_FOG)
	{
		Medium.m_FogPlane.CreateNV(CVec3Dfp32(0,0,1), m_BoundMin);
		Medium.m_FogAttenuation = 1.0f / (m_BoundMax[2] - m_BoundMin[2]);
	}

	m_Medium = Medium;
}

int CSolid::CSG_EqualMediums(const CSolid& _Solid) const
{
	MAUTOSTRIP(CSolid_CSG_EqualMediums, 0);
	// Compares all attributes.
	return m_Medium.Equal(_Solid.m_Medium);
}

bool CSolid::CSG_CanClip(const CSolid& _Solid, bool _bHeedStructure) const
{
	MAUTOSTRIP(CSolid_CSG_CanClip, false);

	if (m_Medium.m_iPhysGroup != _Solid.m_Medium.m_iPhysGroup)
		return false;

	if (!_bHeedStructure)
	{
		return CSG_CompareMediums(_Solid) >= 0;
	}
	else
	{
		if (m_Flags & CSOLID_FLAGS_STRUCTURE)
		{
			if (_Solid.m_Flags & CSOLID_FLAGS_STRUCTURE)
				return CSG_CompareMediums(_Solid) >= 0;
			else
				return true;
		}
		else
		{
			if (_Solid.m_Flags & CSOLID_FLAGS_STRUCTURE)
				return false;
			else
				return CSG_CompareMediums(_Solid) >= 0;
		}
	}
}

spCSolid CSolid::CSG_CutBack(const CSolid_Plane* _pPlane) const
{
	MAUTOSTRIP(CSolid_CSG_CutBack, NULL);
	int Mask = CSG_GetCutMask(_pPlane->m_Plane);
	if ((Mask & 3) == 1)
	{
		return NULL;
	}
	else if ((Mask & 3) == 2)
	{
		spCSolid spS = MNew(CSolid);
		if (!spS) MemError("CSG_CutBack");
		*spS = *this;
		return spS;
	}
	else
	{
		spCSolid spDest = MNew(CSolid);
		if (!spDest) MemError("CSG_CutBack");
		spDest->m_lspPlanes.SetGrow(m_lspPlanes.Len() + 1);

		for(int iP = 0; iP < m_lspPlanes.Len(); iP++)
			spDest->m_lspPlanes.Add(m_lspPlanes[iP]->Duplicate());
		spDest->m_lspPlanes.Add(_pPlane->Duplicate());
		spDest->CSG_CopyAttributes(*this);
		spDest->UpdateMesh();
		if (spDest->GetNumPlanes() < 4) 
			spDest = NULL;
		return spDest;
	}
}

spCSolid CSolid::CSG_CutFront(const CSolid_Plane* _pPlane) const
{
	MAUTOSTRIP(CSolid_CSG_CutFront, NULL);
	int Mask = CSG_GetCutMask(_pPlane->m_Plane);
	if ((Mask & 3) == 2)
	{
		return NULL;
	}
	else if ((Mask & 3) == 1)
	{
		return Duplicate();
	}
	else
	{
		spCSolid spDest = MNew(CSolid);
		if (!spDest) MemError("CSG_CutFront");

		spDest->m_lspPlanes.SetGrow(m_lspPlanes.Len() + 1);

		for(int iP = 0; iP < m_lspPlanes.Len(); iP++)
			spDest->m_lspPlanes.Add(m_lspPlanes[iP]->Duplicate());
		int iPlane = spDest->m_lspPlanes.Add(_pPlane->Duplicate());
		spDest->m_lspPlanes[iPlane]->m_Plane.Inverse();
		spDest->CSG_CopyAttributes(*this);
		spDest->UpdateMesh();
		if (spDest->GetNumPlanes() < 4) 
			spDest = NULL;
		return spDest;
	}
}

spCSolid CSolid::CSG_And(const CSolid& _Oper) const
{
	MAUTOSTRIP(CSolid_CSG_And, NULL);
	// Does solid2 intersect any of solid1's planes?
	int iP;
	for(iP = 0; iP < m_lspPlanes.Len(); iP++)
		if (!(_Oper.CSG_GetCutMask(m_lspPlanes[iP]->m_Plane) & 2)) return NULL;

	// Does solid1 intersect any of solid2's planes?
	for(iP = 0; iP < _Oper.m_lspPlanes.Len(); iP++)
		if (!(CSG_GetCutMask(_Oper.m_lspPlanes[iP]->m_Plane) & 2)) return NULL;

	spCSolid spAnd = MNew(CSolid);
	if (!spAnd) MemError("Merge");

	for(int ip1 = 0; ip1 < m_lspPlanes.Len(); ip1++)
		spAnd->AddPlane(m_lspPlanes[ip1]->Duplicate());
	for(int ip2 = 0; ip2 < _Oper.m_lspPlanes.Len(); ip2++)
		spAnd->AddPlane(_Oper.m_lspPlanes[ip2]->Duplicate());

/*	for(int ip1 = 0; ip1 < m_lspPlanes.Len(); ip1++)
		spAnd->m_lspPlanes.Add(m_lspPlanes[ip1]->Duplicate());
	for(int ip2 = 0; ip2 < _Oper.m_lspPlanes.Len(); ip2++)
		spAnd->m_lspPlanes.Add(_Oper.m_lspPlanes[ip2]->Duplicate());*/

	spAnd->CSG_CopyAttributes(*this);
//LogFile(CStrF("(CSG_And) (2), %d + %d = %d planes", m_lspPlanes.Len(), _Oper.m_lspPlanes.Len(), spAnd->m_lspPlanes.Len() ));
	spAnd->UpdateMesh();
//LogFile(CStrF("(CSG_And) (3), %d planes", spAnd->m_lspPlanes.Len() ));
	if (spAnd->m_lspPlanes.Len() < 4)
		spAnd = NULL;


	// Check bounding
	{
		CBox3Dfp32 Box(m_BoundMin, m_BoundMax);
		Box.Expand(_Oper.m_BoundMin);
		Box.Expand(_Oper.m_BoundMax);
		Box.Grow(1.0);

		CSolid* pOut = spAnd;
		for(; pOut; pOut = pOut->m_spNext)
		{
			for(int i = 0; i < 3; i++)
			{	
				if (pOut->m_BoundMin[i] < Box.m_Min[i] ||
					pOut->m_BoundMax[i] > Box.m_Max[i])
				{
					LogFile("WARNING: CSG_And failure detected!!!");
					return Duplicate();	// Return this only.
				}
			}
		}
	}

	return spAnd;
}

spCSolid CSolid::CSG_AndNotClip(const TArray<spCSolid_Plane>& _lspPlanes, int _ClipPlaneFlags, bool _bOptimize) const
{
	MAUTOSTRIP(CSolid_CSG_AndNotClip, NULL);
	spCSolid spOutSolids1;
	spCSolid spOutSolids2;
	spCSolid spS = Duplicate();
	int iP;
	for(iP = 0; iP < _lspPlanes.Len(); iP++)
	{
		if (!spS) break;

		if ((spS->CSG_GetCutMask(_lspPlanes[iP]->m_Plane) & 2) == 2)
		{
			spCSolid_Plane spP = _lspPlanes[iP]->Duplicate();

			CPlane3Dfp64 PInv;
			PInv.CreateInverse(_lspPlanes[iP]->m_Plane);

			int iPlane = CSG_GetPlaneIndex(_lspPlanes[iP]->m_Plane, 0.1);
			if (iPlane >= 0)
			{
//				LogFile("Found! (1)");
				if (m_lspPlanes[iPlane]->m_spMapping != NULL)
					spP->m_spMapping = m_lspPlanes[iPlane]->m_spMapping->Duplicate();
			}
			else if ((iPlane = CSG_GetPlaneIndex(PInv, 0.1)) >= 0)
			{
//				LogFile("Found! (1 Inv)");
				if (m_lspPlanes[iPlane]->m_spMapping != NULL)
					spP->m_spMapping = m_lspPlanes[iPlane]->m_spMapping->Duplicate();
			}
			else
			{
//				LogFile("Not found! (1)");
				spP->m_Flags |= _ClipPlaneFlags;
			}

			spCSolid spBack = spS->CSG_CutBack(spP);
			spCSolid spFront = spS->CSG_CutFront(spP);

			if (spFront)
			{
				spFront->m_spNext = spOutSolids1;
				if (spOutSolids1) spOutSolids1->m_pPrev = spFront;
				spOutSolids1 = spFront;
			}
			spS = spBack;
		}
	}

	if (_bOptimize)
	{
		if (!spS) spS = MNew(CSolid);
		if (!spS) MemError("CSG_AndNotClip");
		*spS = *this;
		for(iP = _lspPlanes.Len()-1; iP >= 0; iP--)
		{
			if (!spS) break;

			if ((spS->CSG_GetCutMask(_lspPlanes[iP]->m_Plane) & 2) == 2)
			{
				spCSolid_Plane spP = _lspPlanes[iP]->Duplicate();
//				spP->m_Flags |= _ClipPlaneFlags;

				CPlane3Dfp64 PInv;
				PInv.CreateInverse(_lspPlanes[iP]->m_Plane);

				int iPlane = CSG_GetPlaneIndex(_lspPlanes[iP]->m_Plane, 0.1);
				if (iPlane >= 0)
				{
//					LogFile("Found! (2)");
					if (m_lspPlanes[iPlane]->m_spMapping != NULL)
						spP->m_spMapping = m_lspPlanes[iPlane]->m_spMapping->Duplicate();
				}
				else if ((iPlane = CSG_GetPlaneIndex(PInv, 0.1)) >= 0)
				{
//					LogFile("Found! (2 Inv)");
					if (m_lspPlanes[iPlane]->m_spMapping != NULL)
						spP->m_spMapping = m_lspPlanes[iPlane]->m_spMapping->Duplicate();
				}
				else
				{
//					LogFile("Not found! (2)");
					spP->m_Flags |= _ClipPlaneFlags;
				}

				spCSolid spBack = spS->CSG_CutBack(spP);
				spCSolid spFront = spS->CSG_CutFront(spP);

				if (spFront)
				{
					spFront->m_spNext = spOutSolids2;
					if (spOutSolids2) spOutSolids2->m_pPrev = spFront;
					spOutSolids2 = spFront;
				}
				spS = spBack;
			}
		}

		if (spOutSolids1 && spOutSolids2)
			if (spOutSolids2->GetChainLen() < spOutSolids1->GetChainLen()) spOutSolids1 = spOutSolids2;
	}

	return spOutSolids1;
}

spCSolid CSolid::CSG_AndNot(const CSolid& _Solid, int _ClipPlaneFlags, bool _bOptimize) const
{
	MAUTOSTRIP(CSolid_CSG_AndNot, NULL);
	// First, a simple bounding check to avoid the heavy work.
	if (!CSG_Intersection(_Solid)) return Duplicate();

	// Cut!
	spCSolid spRes = CSG_AndNotClip(_Solid.m_lspPlanes, _ClipPlaneFlags, _bOptimize);

	// Optimize result.
	spCSolid spMergeRes =  CSG_MergeChain(spRes);

	// Check bounding
	{
		CBox3Dfp32 Box(m_BoundMin, m_BoundMax);
		Box.Expand(_Solid.m_BoundMin);
		Box.Expand(_Solid.m_BoundMax);
		Box.Grow(1.0);

		CSolid* pOut = spMergeRes;
		for(; pOut; pOut = pOut->m_spNext)
		{
			for(int i = 0; i < 3; i++)
			{	
				if (pOut->m_BoundMin[i] < Box.m_Min[i] ||
					pOut->m_BoundMax[i] > Box.m_Max[i])
				{
					LogFile("WARNING: CSG_AndNot failure detected!!!");
					return Duplicate();	// Return this only.
				}
			}
		}
	}

	return spMergeRes;
}

spCSolid CSolid::CSG_Or(const CSolid& _Solid)
{
	MAUTOSTRIP(CSolid_CSG_Or, NULL);
	// NOTE: Returns a chain of non-intersecting solids. At least one is returned.

	// Is that wholy inside this?
	if (CSG_Inside(_Solid))
	{
		return Duplicate();
	}

	// Is this wholy inside that?
/*	if (_Solid.CSG_Inside(*this))
	{
		spCSolid spS = DNew(CSolid) CSolid;
		*spS = _Solid;
		return spS;
	}*/

	if (!CSG_Intersection(_Solid))
	{
		// No intersection, return both brushes in a chain
		spCSolid spS1 = Duplicate();
		spCSolid spS2 = _Solid.Duplicate();
		spS1->m_spNext = spS2;
		spS2->m_pPrev = spS1;
		return spS1;
	}

	const CSolid* pS1 = this;
	const CSolid* pS2 = &_Solid;

	// nS1 is the number of planes from solid1 that intersects solid2
/*	int nS1 = 0;
	for(int iP = 0; iP < pS1->m_lspPlanes.Len(); iP++)
		if ((pS2->CSG_GetCutMask(pS1->m_lspPlanes[iP]->m_Plane) & 3) == 3) nS1++;

	// nS2 is the number of planes from solid2 that intersects solid1
	int nS2 = 0;
	for(iP = 0; iP < pS2->m_lspPlanes.Len(); iP++)
		if ((pS1->CSG_GetCutMask(pS2->m_lspPlanes[iP]->m_Plane) & 3) == 3) nS2++;

	if (!nS1 || !nS2) Error("Or", "No split-planes found.");
*/
	// Put all planes that intersects on a separate list.
	TArray<spCSolid_Plane> lspPlanes;
	for(int iP = 0; iP < pS1->m_lspPlanes.Len(); iP++)
		if ((pS2->CSG_GetCutMask(pS1->m_lspPlanes[iP]->m_Plane) & 2) == 2)
#ifdef CSOLID_PARANOIAPLANECHECK
			if (!pS2->CSG_PlaneExists(pS1->m_lspPlanes[iP]->m_Plane, 0.01) &&
				!pS2->CSG_InversePlaneExists(pS1->m_lspPlanes[iP]->m_Plane, 0.01))
#endif
				lspPlanes.Add(pS1->m_lspPlanes[iP]);

	spCSolid spOutSolids = pS2->CSG_AndNotClip(lspPlanes, CSOLID_PLANEFLAGS_CSGFACE, true);

	// Put cut-brush on result-list
	spCSolid spSOrg = pS1->Duplicate();
	spSOrg->m_spNext = spOutSolids;
	if (spOutSolids) spOutSolids->m_pPrev = spSOrg;
	spOutSolids = spSOrg;

	// Check bounding
	{
		CBox3Dfp32 Box(m_BoundMin, m_BoundMax);
		Box.Expand(_Solid.m_BoundMin);
		Box.Expand(_Solid.m_BoundMax);
		Box.Grow(1.0);

		CSolid* pOut = spOutSolids;
		for(; pOut; pOut = pOut->m_spNext)
		{
			for(int i = 0; i < 3; i++)
			{	
				if (pOut->m_BoundMin[i] < Box.m_Min[i] ||
					pOut->m_BoundMax[i] > Box.m_Max[i])
				{
					LogFile(CStrF("WARNING: CSG_Or failure detected!!! (OutBox (%s - %s > InBox %s)", pOut->m_BoundMin.GetString().Str(), pOut->m_BoundMax.GetString().Str(), Box.GetString().Str()));
					return Duplicate();	// Return this only.
				}
			}
		}
	}

	return spOutSolids;
}

spCSolid CSolid::CSG_Merge(const CSolid& _Oper, bool _bIgnoreTexture) const
{
	MAUTOSTRIP(CSolid_CSG_Merge, NULL);
	// First, a simple bounding check to avoid the heavy work.
	for(int i = 0; i < 3; i++)
	{
		if (m_BoundMin.k[i] - _Oper.m_BoundMax.k[i] > CSOLID_EPSILON*100000.0f) return NULL;
		if (_Oper.m_BoundMin.k[i] - m_BoundMax.k[i] > CSOLID_EPSILON*100000.0f) return NULL;
	}

	if (!CSG_EqualMediums(_Oper)) return NULL;
	bool s1IsNormal = (m_Flags & (CSOLID_FLAGS_STRUCTURE | CSOLID_FLAGS_NAVIGATION)) == 0;
	bool s2IsNormal = (_Oper.m_Flags & (CSOLID_FLAGS_STRUCTURE | CSOLID_FLAGS_NAVIGATION)) == 0;
	if (s1IsNormal ^ s2IsNormal) return NULL;

	// Check if this medium allow merging
	if (s1IsNormal && (m_Medium.m_MediumFlags & XW_MEDIUM_NOMERGE)) return NULL;
	if (!s1IsNormal && (m_Medium.m_MediumFlags & XW_MEDIUM_NOSTRUCTUREMERGE)) return NULL;

//	if ((m_Flags & CSOLID_FLAGS_STRUCTURE) ^ (_Oper.m_Flags & CSOLID_FLAGS_STRUCTURE)) return NULL;
		
	int iP1, iP2;
	for(iP1 = 0; iP1 < m_lspPlanes.Len(); iP1++)
		for(iP2 = 0; iP2 < _Oper.m_lspPlanes.Len(); iP2++)
		{
			// Check if the planes are coplanar.
			if ((M_Fabs(m_lspPlanes[iP1]->m_Plane.n * _Oper.m_lspPlanes[iP2]->m_Plane.n + 1.0) < 0.0001) &&
				(M_Fabs(m_lspPlanes[iP1]->m_Plane.d + _Oper.m_lspPlanes[iP2]->m_Plane.d) < 0.0001))
			{
				// Check that all vertices of plane1 are on the surface of _Oper
				const CSolid_Face* pF1 = &GetFace(iP1);
				const uint16* piV1 = GetFaceVertexIndices(*pF1);
				int v;
				for(v = 0; v < pF1->m_nV; v++)
					if (!_Oper.CSG_VertexExists(m_lVerticesD[piV1[v]], CSOLID_EPSILON*100.0f)) return NULL;

				// Check that all vertices of plane2 are on the surface of this
				const CSolid_Face* pF2 = &_Oper.GetFace(iP2);
				const uint16* piV2 = _Oper.GetFaceVertexIndices(*pF2);
				for(v = 0; v < pF2->m_nV; v++)
					if (!CSG_VertexExists(_Oper.m_lVerticesD[piV2[v]], CSOLID_EPSILON*100.0f)) return NULL;

				spCSolid spMerge = MNew(CSolid);
				if (!spMerge) MemError("Merge");
				spMerge->m_lspPlanes.SetGrow(m_lspPlanes.Len() + _Oper.m_lspPlanes.Len());

				// Add planes from this
				for(int ip1 = 0; ip1 < m_lspPlanes.Len(); ip1++)
					if (ip1 != iP1) spMerge->m_lspPlanes.Add(m_lspPlanes[ip1]->Duplicate());

				// Add planes from _Oper
				for(int ip2 = 0; ip2 < _Oper.m_lspPlanes.Len(); ip2++)
					if (ip2 != iP2)
					{
						// Check for indentical plane with different mapping.
						if (!_bIgnoreTexture)
						{
							int iPM = -1;
							if((iPM = spMerge->CSG_GetPlaneIndex(_Oper.m_lspPlanes[ip2]->m_Plane, CSOLID_EPSILON*100.0)) >= 0)
							{
								if (!spMerge->m_lspPlanes[iPM]->m_spMapping || !_Oper.m_lspPlanes[ip2]->m_spMapping)
									continue;

								if ((spMerge->m_lspPlanes[iPM]->m_Flags != _Oper.m_lspPlanes[ip2]->m_Flags) ||
									!spMerge->m_lspPlanes[iPM]->m_spMapping->IsEqual(_Oper.m_lspPlanes[ip2]->m_spMapping))
									return NULL;
								else
									continue;
							}
						}
						else
							if(spMerge->CSG_GetPlaneIndex(_Oper.m_lspPlanes[ip2]->m_Plane, CSOLID_EPSILON*100.0) >= 0) continue;

						// Plane was ok, so add it.
						spMerge->m_lspPlanes.Add(_Oper.m_lspPlanes[ip2]->Duplicate());
					}

				spMerge->CSG_CopyAttributes(*this);
				spMerge->UpdateMesh();
				if (m_lspPlanes.Len() < 4) return NULL;
				if (!spMerge->CSG_Inside(*this)) return NULL;
				if (!spMerge->CSG_Inside(_Oper)) return NULL;

				// Check bounding
				{
					CBox3Dfp32 Box(m_BoundMin, m_BoundMax);
					Box.Expand(_Oper.m_BoundMin);
					Box.Expand(_Oper.m_BoundMax);
					Box.Grow(1.0);
					
					for(int i = 0; i < 3; i++)
					{	
						if (spMerge->m_BoundMin[i] < Box.m_Min[i] ||
							spMerge->m_BoundMax[i] > Box.m_Max[i])
						{
							LogFile("WARNING: CSG_Merge failure detected and avoided.");
							return NULL;
						}
					}
				}

				// TJOHO!
//				LogFile("Merge!");
				return spMerge;
			}
		}
	return NULL;
}

// -------------------------------------------------------------------
//  Static CSG functions
// -------------------------------------------------------------------
spCSolid CSolid::CSG_CutBackChain(spCSolid _spSrc, const CSolid_Plane* _pPlane)
{
	spCSolid spRes;
	CSolid* pTail = NULL;

	CSolid* pS = _spSrc;
	while(pS)
	{
		spCSolid spBack = pS->CSG_CutBack(_pPlane);
		if (spBack != NULL)
		{
			if (spRes != NULL)
				spBack->LinkAfter(pTail);
			else
				spRes = spBack;
			pTail = spBack;
		}
		pS = pS->m_spNext;
	}

	return spRes;
}

spCSolid CSolid::CSG_CutFrontChain(spCSolid _spSrc, const CSolid_Plane* _pPlane)
{
	spCSolid spRes;
	CSolid* pTail = NULL;

	CSolid* pS = _spSrc;
	while(pS)
	{
		spCSolid spFront = pS->CSG_CutFront(_pPlane);
		if (spFront != NULL)
		{
			if (spRes != NULL)
				spFront->LinkAfter(pTail);
			else
				spRes = spFront;
			pTail = spFront;
		}
		pS = pS->m_spNext;
	}

	return spRes;
}


#define CSOLID_NEW_CSG_MERGECHAIN
#ifdef CSOLID_NEW_CSG_MERGECHAIN

spCSolid CSolid::CSG_MergeChain(spCSolid _spSrc, bool _bIgnoreTexture, bool _bLog, IProgress* _pProgress)
{
	MAUTOSTRIP(CSolid_CSG_MergeChain, NULL);
	// NOTE: Operands are modified.
	int nOrgSolids = _spSrc->GetChainLen();
	int nIDs = nOrgSolids;
	int nSrcSolids = nOrgSolids;
	int nDoneSolids = 0;
	int nTotSolids = nOrgSolids;

	TPtr<CSolid_SpaceEnum> spSolidEnum = MNew(CSolid_SpaceEnum);
	if (!spSolidEnum) Error_static("CSG_MergeChain", "Out of memory.");
	spSolidEnum->Create(nIDs);

	TArray<int32> liEnum;
	TArray<spCSolid> lspSolids;
	liEnum.SetLen(nIDs);
	lspSolids.SetLen(nIDs);
	int iSolid = 0;
	{
		spCSolid spS = _spSrc;
		while(spS)
		{
			spCSolid spNext = spS->m_spNext;
			spSolidEnum->Insert(iSolid, spS);
			lspSolids[iSolid] = spS;
			spS->Unlink();
			spS = spNext;
			iSolid++;
		}
	}

//	spSolidEnum->Log();

	int nTested = 0;
	int nTestEnum = 0;
	int nMaxEnum = 0;

	int iMergePass = 1;
	bool bMerged = true;
	while(bMerged)
	{
		if (_bLog) LogFile(CStrF("    CSG Merge pass %d", iMergePass++) );
		bMerged = false;
		for(int iS = 0; iS < iSolid; iS++)
		{
			if (_pProgress) _pProgress->SetProgress(fp32(iS) / fp32(iSolid));

			spCSolid spS = lspSolids[iS];
			if (spS)
			{
				nTested++;
				int nEnum = spSolidEnum->EnumerateBox(spS, liEnum.GetBasePtr(), liEnum.Len());
				nTestEnum += nEnum;
				nMaxEnum += nSrcSolids;
				int bIntersect = false;
				int bDeniedIntersect = false;
				for(int i = 0; i < nEnum; i++)
				{
					int iSTest = liEnum[i];
					spCSolid spS2 = lspSolids[iSTest];
					if (spS2 != spS)
					{
						spCSolid spMerge = spS->CSG_Merge(*spS2, _bIgnoreTexture);
						if (spMerge)
						{
							nSrcSolids--;
							spSolidEnum->Remove(iSTest);
							lspSolids[iSTest] = NULL;
							lspSolids[iS] = spMerge;
							spSolidEnum->Insert(iS, spMerge);
							bMerged = true;
							iS--;			// Try this solid next time also.
							break;
						}
					}
				}
			}
		}
	}

	liEnum.Clear();

	spCSolid spHead;
	for(int iS = 0; iS < iSolid; iS++)
	{
		spCSolid spS = lspSolids[iS];
		if (spS)
		{
			if (spHead) spS->LinkBefore(spHead);
			spHead = spS;
		}
	}

	spSolidEnum = NULL;
	lspSolids.Clear();

	return spHead;
}

#else
spCSolid CSolid::CSG_MergeChain(spCSolid _spSrc, bool _bIgnoreTexture, bool _bLog, IProgress* _pProgress)
{
	MAUTOSTRIP(CSolid_CSG_MergeChain_2, NULL);
	// NOTE: Operands are modified.

	spCSolid spHead = _spSrc;

	bool bMerged = true;
	while(bMerged)
	{
		bMerged = false;
		spCSolid spS1 = spHead;
		while(spS1)
		{
			spCSolid spS2 = spS1->m_spNext;
			while(spS2)
			{
				spCSolid spMerge = spS1->CSG_Merge(*spS2, _bIgnoreTexture);
				if (spMerge)
				{
					spS2->Unlink();

					spCSolid spS1Next = spS1->m_spNext;
					if (spHead == spS1) { spHead = spS1->m_spNext; }
					spS1->Unlink();
					spS1 = spS1Next;

					spMerge->LinkAfter(spHead);

					bMerged = true;
					break;
				}
				spS2 = spS2->m_spNext;
			}

			if (spS1) spS1 = spS1->m_spNext;
		}
	}

	return spHead;
}
#endif

#define CSOLID_NEW_CSG_ORCHAIN

#ifdef CSOLID_NEW_CSG_ORCHAIN
spCSolid CSolid::CSG_OrChain(spCSolid _spSrc, bool _bHeedStructure, bool _bLog, IProgress* _pProgress, bool _bIgnoreTexture, int _MaxClipResult, bool _bPostOrMerge)
{
	MAUTOSTRIP(CSolid_CSG_OrChain, NULL);
	// NOTE: Operands are modified.

	if (!_spSrc) return NULL;
	spCSolid spDone;

	int nPreMerge = _spSrc->GetChainLen();
	_spSrc = CSG_MergeChain(_spSrc, _bIgnoreTexture, _bLog, _pProgress);
	if(_bLog) LogFile(CStrF("    CSG Merge (%d -> %d solids)", nPreMerge, _spSrc->GetChainLen()));

	int nOrgSolids = _spSrc->GetChainLen();
	int nIDs = (nOrgSolids*10+10000) & ~31;
	int nSrcSolids = nOrgSolids;
	int nDoneSolids = 0;
	int nTotSolids = nOrgSolids;

	TPtr<CSolid_SpaceEnum> spSolidEnum = MNew(CSolid_SpaceEnum);
	if (!spSolidEnum) Error_static("CSG_OrChain", "Out of memory.");
	spSolidEnum->Create(nIDs);

	TArray<int32> liEnum;
	TArray<spCSolid> lspSolids;
	liEnum.SetLen(nIDs);
	lspSolids.SetLen(nIDs);

	CIDHeap SolidHeap;
	SolidHeap.Create(nIDs);

	int iSolidMax = 0;
	{
		spCSolid spS = _spSrc;
		while(spS)
		{
			spCSolid spNext = spS->m_spNext;
			int iSolid = SolidHeap.AllocID();
			iSolidMax = Max(iSolid, iSolidMax);
			spSolidEnum->Insert(iSolid, spS);
			lspSolids[iSolid] = spS;
			spS->Unlink();
			spS = spNext;
		}
	}

//spSolidEnum->Log();


	int nTested = 0;
	int nTestEnum = 0;
	int nMaxEnum = 0;
	bool bFinished = false;
	for(int MaxResult = 1; (!bFinished) && (MaxResult <= _MaxClipResult); MaxResult++)
	{
//		if (_bLog) LogFile(CStrF("    CSG pass %d (%d -> %d solids, %d done)", MaxResult, nPrevMerge, nPostMerge, spDone->GetChainLen() ));
		if (_bLog) LogFile(CStrF("    CSG pass %d (%d/%d solids, %d done)", MaxResult, nSrcSolids, iSolidMax, nDoneSolids ));

		bFinished = true;
		for(int iS = 0; iS <= iSolidMax; iS++)
		{
			if (_pProgress) _pProgress->SetProgress(fp32(iS) / fp32(iSolidMax));

			spCSolid spS = lspSolids[iS];
			if (spS)
			{
				nTested++;
				int nEnum = spSolidEnum->EnumerateBox(spS, liEnum.GetBasePtr(), liEnum.Len());
				nTestEnum += nEnum;
				nMaxEnum += nSrcSolids;
				int bIntersect = false;
				int bDeniedIntersect = false;
				for(int i = 0; i < nEnum; i++)
				{
					int iSTest = liEnum[i];
					spCSolid pSI = lspSolids[iSTest];
					if (!pSI) continue;
					if ((pSI != spS) && (spS->CSG_Intersection(*pSI)))
					{
						spCSolid spAnd = spS->CSG_And(*pSI);
						if (!spAnd || (spAnd->GetVolume() < 0.1))
						{
//							LogFile("Skipped OR.");
							continue;
						}

						spCSolid spRes;
						if (MaxResult == 1)
						{
							if (spS->CSG_CanClip(*pSI, _bHeedStructure) && spS->CSG_Inside(*pSI))
								spRes = spS;
							else if (pSI->CSG_CanClip(*spS, _bHeedStructure) && pSI->CSG_Inside(*spS))
								spRes = pSI;
						}
						else
		/*				if (MaxResult == 1)
						{
							if (pSI->CSG_CanClip(*spS, _bHeedStructure) && spS->CSG_Inside(*pSI))
								spRes = spS;
							else if (spS->CSG_CanClip(*pSI, _bHeedStructure) && pSI->CSG_Inside(*spS))
								spRes = pSI;
						}
						else*/
						{
							spCSolid spRes2;
							if (spS->CSG_CanClip(*pSI, _bHeedStructure))
								spRes = spS->CSG_Or(*pSI);
							if (pSI->CSG_CanClip(*spS, _bHeedStructure))
								spRes2 = pSI->CSG_Or(*spS);

							if (spRes && spRes2 && spRes2->GetChainLen() < spRes->GetChainLen())
								spRes = spRes2;
							if (!spRes) spRes = spRes2;

							if (spRes != NULL)
							{
								spCSolid pPrev = spRes;
								spCSolid pTest = pPrev->m_spNext;
								while(pTest)
								{
									if (pTest->GetVolume() < 0.001f)
									{
//										LogFile(CStrF("OrChain - Intersection post-or, deleting solid with volume %f, box %s - %s", pTest->GetVolume(), pTest->m_BoundMin.GetString().Str(), pTest->m_BoundMax.GetString().Str() ));
										spCSolid spNext = pTest->m_spNext;
										pPrev->m_spNext = spNext;
										pTest = spNext;
									}
									else
										pTest = pTest->m_spNext;
								}

								if (pPrev && pPrev->GetVolume() < 0.001f)
									pPrev = pPrev->m_spNext;

								spRes = pPrev;

								pTest = spRes;
								for(; pTest; pTest = pTest->m_spNext)
								{
									spCSolid pPrev = pTest;
									spCSolid pTest2 = pTest->m_spNext;
									for(; pTest2; pTest2 = pTest2->m_spNext)
									{
										if (pTest->CSG_Intersection(*pTest2))
										{
//											LogFile(CStrF("OrChain - Intersection  , deleting solid with volume %f, box %s - %s", pTest2->GetVolume(), pTest2->m_BoundMin.GetString().Str(), pTest2->m_BoundMax.GetString().Str() ));
											pPrev->m_spNext = pTest2->m_spNext;
										}
										pPrev = pTest2;
									}
								}
							}
						}

						if (!spRes || (spRes->GetChainLen() > MaxResult))
						{
							bFinished = false;
							bDeniedIntersect = 1;
							bIntersect = 1;
							continue;
						}

//						if (spRes->GetChainLen() == 1 && (spRes->m_lspPlanes.Len() == pSI->m_lspPlanes.Len()))
/*						if (spRes != NULL)
						{
							CSolid* pR = spRes;
							for(; pR; pR = pR->m_spNext)
							{
								if (spS->CSG_Intersection(*pR) && pR->CSG_Intersection(*spS))
								{
									LogFile("WARNING: CSG_OrChain failure detected.");
									break;
								}
							}

							if (pR)
							{
								bFinished = false;
								bDeniedIntersect = 1;
								bIntersect = 1;
								continue;
							}
						}*/


						if (spRes)
						{
							// Unlink intersection-brush.
							spSolidEnum->Remove(iSTest);
							lspSolids[iSTest] = NULL;
							SolidHeap.FreeID(iSTest);
							nSrcSolids--; nTotSolids--;
							// Unlink source-brush
							spSolidEnum->Remove(iS);
							lspSolids[iS] = NULL;
							SolidHeap.FreeID(iS);
							nSrcSolids--; nTotSolids--;

							spCSolid spR = spRes;
							while(spR)
							{
								int iSolid = SolidHeap.AllocID();
								if (iSolid < 0) 
								{
									LogFile(CStrF("OrChain - Cannot add more or-result solids. (%d/%d)", iSolid, nIDs));
									MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
									if (pSys)
									{
										LogFile(CStrF("Attempted to add result:") + pSys->m_ExePath + "ResultSolids.xmp");
										WriteXMP(spRes, pSys->m_ExePath + "ResultSolids.xmp");

										LogFile(CStrF("Writing hashed solids to: ") + pSys->m_ExePath + "HashedSolids.xmp");
										WriteXMP(lspSolids, pSys->m_ExePath + "HashedSolids.xmp");
									}

									Error_static("OrChain", "Cannot add more or-result solids.");
								}
								spCSolid spNext = spR->m_spNext;
								spSolidEnum->Insert(iSolid, spR);
								iSolidMax = Max(iSolidMax, iSolid);
								lspSolids[iSolid] = spR;
								spR->Unlink();
								spR = spNext;
								nSrcSolids++; nTotSolids++;
							}

							bFinished = false;
							bIntersect = 1;
							bDeniedIntersect = 0;
							break;
						}
						else
							/*if (_bLog)*/ LogFile("No brushes from brush-OR.");
					}
				}

				// Didn't intersect anything, move it to done-list.
				if (!bIntersect)
				{
					spSolidEnum->Remove(iS);
					SolidHeap.FreeID(iS);
					lspSolids[iS] = NULL;
					if (spDone) spS->LinkBefore(spDone);
					spDone = spS;
					nSrcSolids--; nDoneSolids++;
				}

				if (bDeniedIntersect)
				{
					bFinished = false;
				}
			}
		}
	}

	// Add solids that were not allowed to be ORed. (if _MaxClipResult is not "infinite")
	{
		for(int i = 0; i < lspSolids.Len(); i++)
		{
			if (lspSolids[i] != NULL)
			{
				spCSolid spS = lspSolids[i];
				if (spDone) 
					spS->LinkBefore(spDone);
				spDone = spS;
				nDoneSolids++;
			}
		}
	}

if(_bLog) LogFile(CStrF("    CSG nTested %d, Average enum %.2f / %.2f", nTested, fp32(nTestEnum) / fp32(nTested), fp32(nMaxEnum) / fp32(nTested)));

	spSolidEnum = NULL;
	lspSolids.Clear();

	// Merge done-list
	if (_bPostOrMerge)
	{
		int nPrevMerge = spDone->GetChainLen();
		spDone = CSG_MergeChain(spDone, _bIgnoreTexture, _bLog, _pProgress);
		int nPostMerge = spDone->GetChainLen();
		if (_bLog) LogFile(CStrF("    CSG Merge (%d -> %d solids)", nPrevMerge, nPostMerge));
	}

//	return CSolid::CSG_MergeChain(spDone);
	return spDone;
}

#else

spCSolid CSolid::CSG_OrChain(spCSolid _spSrc, bool _bHeedStructure, bool _bLog, IProgress* _pProgress)
{
	MAUTOSTRIP(CSolid_CSG_OrChain_2, NULL);
	// NOTE: Operands are modified.

	if (!_spSrc) return NULL;
	spCSolid spDone;
	spCSolid spS = _spSrc;

	int nOrgSolids = spS->GetChainLen();

	int nSrcSolids = nOrgSolids;
	int nDoneSolids = 0;
	int nTotSolids = nOrgSolids;

	spCSolid spNewSrc = spS;
	for(int MaxResult = 1; spNewSrc; MaxResult++)
	{
		int nPrevMerge = spNewSrc->GetChainLen();
		spNewSrc = CSG_MergeChain(spNewSrc);
		spS = spNewSrc;
		int nPostMerge = spNewSrc->GetChainLen();
		nTotSolids -= nPrevMerge - nPostMerge;
		nSrcSolids -= nPrevMerge - nPostMerge;
		if (_bLog) LogFile(CStrF("    CSG pass %d (%d -> %d solids, %d done)", MaxResult, nPrevMerge, nPostMerge, spDone->GetChainLen() ));

		while(spS)
		{
			if (_pProgress) _pProgress->SetProgress(fp32(nDoneSolids) / fp32(nTotSolids));

			bool bDeniedIntersect = 0;
			bool bIntersect = 0;
			spCSolid pSI = spNewSrc;
			while(pSI)
			{
				if ((pSI != spS) && (spS->CSG_Intersection(*pSI)))
				{
					spCSolid spRes;
					if (MaxResult == 1)
					{
						if (spS->CSG_CanClip(*pSI, _bHeedStructure) && spS->CSG_Inside(*pSI))
							spRes = spS;
						else if (pSI->CSG_CanClip(*spS, _bHeedStructure) && pSI->CSG_Inside(*spS))
							spRes = pSI;
					}
					else
					{
						spCSolid spRes2;
						if (spS->CSG_CanClip(*pSI, _bHeedStructure))
							spRes = spS->CSG_Or(*pSI);
						if (pSI->CSG_CanClip(*spS, _bHeedStructure))
							spRes2 = pSI->CSG_Or(*spS);

						if (spRes && spRes2 && spRes2->GetChainLen() < spRes->GetChainLen())
							spRes = spRes2;
						if (!spRes) spRes = spRes2;
					}

					if (!spRes || (spRes->GetChainLen() > MaxResult))
					{
						bDeniedIntersect = 1;
						bIntersect = 1;
						pSI = pSI->m_spNext;
						continue;
					}

					if (spRes)
					{
						// Unlink intersection-brush.
						if (spNewSrc == pSI) spNewSrc = (pSI->m_pPrev) ? pSI->m_pPrev : pSI->m_spNext;
						pSI->Unlink();
						pSI = NULL;
						nSrcSolids--; nTotSolids--;

						// Unlink source-brush
						spCSolid spNext = spS->m_spNext;
						if (spNewSrc == spS) spNewSrc = (spS->m_pPrev) ? spS->m_pPrev : spS->m_spNext;
						spS->Unlink();
						nSrcSolids--; nTotSolids--;

						// Add result brush-chain to source.
						if (!spNext)
							spNext =spRes;
						else
						{
							spCSolid spR = spRes;
							while(spR)
							{
								spCSolid spN = spR->m_spNext;
								spR->Unlink();
								spR->LinkAfter(spNext);
								spR = spN;
								nSrcSolids++; nTotSolids++;
							}
						}

						spS = spNext;
						bIntersect = 1;
						bDeniedIntersect = 0;
						break;
					}
					else
						if (_bLog) LogFile("No brushes from brush-OR.");
				}
				pSI = pSI->m_spNext;
			}

			// Didn't intersect anything, move it to done-list.
			if (!bIntersect)
			{
				spS->m_Color = 0xffffffff;
				spCSolid spNext = spS->m_spNext;
				if (spNewSrc == spS) spNewSrc = (spS->m_pPrev) ? spS->m_pPrev : spS->m_spNext;

				// Unlink brush from source and add it to done-list.
				spS->Unlink();
				if (spDone) spS->LinkBefore(spDone);
				spDone = spS;

				spS = spNext;
				nSrcSolids--; nDoneSolids++;
			}

			if (bDeniedIntersect)
			{
				spCSolid spNext = spS->m_spNext;
				spS = spNext;
			}
		}
	}

	// Merge done-list
	{
		int nPrevMerge = spDone->GetChainLen();
		spDone = CSG_MergeChain(spDone);
		int nPostMerge = spDone->GetChainLen();
		if (_bLog) LogFile(CStrF("    CSG done, merged %d -> %d solids", nPrevMerge, nPostMerge));
	}

	return CSolid::CSG_MergeChain(spDone);
//	return spDone;
}
#endif

spCSolid CSolid::CSG_AndNotChain(spCSolid _spSrc, spCSolid _Oper, bool _bOptimize, bool _bLog, IProgress* _pProgress)
{
	MAUTOSTRIP(CSolid_CSG_AndNotChain, NULL);
	// NOTE:	_spSrc is modified,
	//			_Oper is NOT modified.
	//			Returns the result of _spSrc andnot'ed (Subtracted) with _Oper.


	spCSolid spDone;
	spCSolid spSrc = _spSrc;

	for(int MaxResult = (_bOptimize) ? 1 : 10000; spSrc; MaxResult++)
	{
		spCSolid spS = spSrc;

		while(spS)
		{
//			if (_pProgress) _pProgress->SetProgress(fp32(nDoneSolids) / fp32(nTotSolids));

			bool bDeniedIntersect = 0;
			bool bIntersect = 0;
			spCSolid pSI = _Oper;
			while(pSI)
			{
				if (spS->CSG_Intersection(*pSI))
				{
					spCSolid spAnd = spS->CSG_And(*pSI);
					if ((spAnd == NULL) || (spAnd->GetVolume() < 1.0f))
					{
						// Pretend they didn't intersect
						pSI = pSI->m_spNext;
						continue;
					}

					spCSolid spRes = spS->CSG_AndNot(*pSI, 0, _bOptimize);

					int nRes = spRes->GetChainLen();
/*					if ((nRes == 1) && (spRes->m_lspPlanes.Len() == spS->m_lspPlanes.Len()))
					{
						if (spRes->CSG_Intersection(*spS))
//						if (spRes->CSG_Inside(*spS))
						{
							LogFile("WARNING: CSG_AndNotChain failure detected.");
							pSI = pSI->m_spNext;
							continue;
						}
					}*/

					if (nRes > MaxResult)
					{
						bDeniedIntersect = 1;
						bIntersect = 1;
						pSI = pSI->m_spNext;
						continue;
					}

					// Unlink source-brush
					spCSolid spNext = spS->m_spNext;
					if (spSrc == spS) spSrc = spS->m_spNext;
					spS->Unlink();
//					nSrcSolids--; nTotSolids--;

					if (spRes)
					{
						// Add result brush-chain to source.
						if (!spSrc)
							spSrc =spRes;
						else
							spRes->LinkChainAfter(spSrc->GetTail());
					}

					spS = spNext;
					if (!spNext) spS = spSrc;
					
					bIntersect = 1;
					bDeniedIntersect = 0;
					break;
				}
				pSI = pSI->m_spNext;
			}

			// Didn't intersect anything, move it to done-list.
			if (!bIntersect)
			{
				spCSolid spNext = spS->m_spNext;

if (!spS->MRTC_ReferenceCount() || !spSrc)
	int apan = 1;

				if (spSrc == spS) spSrc = (spS->m_pPrev) ? (spCSolid)spS->m_pPrev : spS->m_spNext;

				// Unlink brush from source and add it to done-list.
				spS->Unlink();
				if (spDone) spS->LinkBefore(spDone);
				spDone = spS;

				spS = spNext;
//				nSrcSolids--; nDoneSolids++;
			}

			if (bDeniedIntersect)
			{
				spCSolid spNext = spS->m_spNext;
				spS = spNext;
			}
		}
	}

	return (_bOptimize) ? CSolid::CSG_MergeChain(spDone) : spDone;
}

spCSolid CSolid::CSG_AndChain(spCSolid _spSrc, spCSolid _Oper, bool _bLog, IProgress* _pProgress)
{
	MAUTOSTRIP(CSolid_CSG_AndChain, NULL);
	// NOTE:	_spSrc is modified,
	//			_Oper is NOT modified.
	//			Returns the result of _spSrc and'ed with _Oper.


	spCSolid spDone;
	spCSolid spS = _spSrc;

	while(spS)
	{
		spCSolid spAnd = _Oper;
		spCSolid spCut = spS;
		while(spAnd && spCut)
		{
			if (!spCut->CSG_Inside(*spAnd))
				spCut = spCut->CSG_And(*spAnd);
			spAnd = spAnd->m_spNext;
		}

		if (spCut)
		{
			// Add to done-list.
			if (spDone)
				spCut->LinkAfter(spDone);
			else
				spDone = spCut;
		}

		spS = spS->m_spNext;
	}

	return spDone;
}

// -------------------------------------------------------------------
//CVec3Dfp32 g_lVertices[512];

void CSolid::RenderEdges(CWireContainer* _pWC, const CMat4Dfp32* _pMatrix, const CPixel32& _Col) const
{
	MAUTOSTRIP(CSolid_RenderEdges2, MAUTOSTRIP_VOID);

	CPixel32 Col = _Col;

	int iInvalid = 0;
	int nFaces = GetNumFaces();
	for(int f = 0; f < nFaces; f++)
	{
		const CSolid_Face& Face = GetFace(f);
		if(Face.m_iInvalid > iInvalid)
		{
			Col = _Col * Face.m_Color;
			iInvalid = Face.m_iInvalid;
		}
	}

	for(int iEdge = 0; iEdge < m_lEdges.Len(); iEdge++)
	{
		CVec3Dfp32 p0 = m_lVerticesD[m_lEdges[iEdge].m_iV[0]].Getfp32() * *_pMatrix;
		CVec3Dfp32 p1 = m_lVerticesD[m_lEdges[iEdge].m_iV[1]].Getfp32() * *_pMatrix;
		_pWC->RenderWire(p0, p1, _Col, 0.0f, false);
	}
}

void CSolid::RenderEdges(CXR_VBManager* _pVBM, const CMat4Dfp32* _pMatrix, const CPixel32& _Col) const
{
	MAUTOSTRIP(CSolid_RenderEdges2, MAUTOSTRIP_VOID);

	CPixel32 Col = _Col;

	int iInvalid = 0;
	int nFaces = GetNumFaces();
	for(int f = 0; f < nFaces; f++)
	{
		const CSolid_Face& Face = GetFace(f);
		if(Face.m_iInvalid > iInvalid)
		{
			Col = _Col * Face.m_Color;
			iInvalid = Face.m_iInvalid;
		}
	}
/*
	CXR_VertexBuffer* pVB = _pVBM->Alloc_VB(CXR_VB_ATTRIB);
	if(!pVB)
		return;
	if(!pVB->SetVBChain(_pVBM, false))
		return;
	CXR_VBChain* pChain = pVB->GetVBChain();
	pVB->m_Color	= Col;
	pChain->m_pV	= _pVBM->Alloc_V3(m_lVertices.Len());
	pChain->m_piPrim	= (uint16*)_pVBM->Alloc(sizeof(uint16) * m_lEdges.Len() * 2);
	if(!pChain->m_pV || !pChain->m_piPrim)
		return;
	pChain->m_nV	= m_lVertices.Len();
	pChain->m_nPrim	= m_lEdges.Len() * 2;
	pChain->m_PrimType	= CRC_RIP_WIRES;
	memcpy(pChain->m_pV, m_lVertices.GetBasePtr(), m_lVertices.ListSize());
	memcpy(pChain->m_piPrim, m_lEdges.GetBasePtr()->m_iV, m_lEdges.Len() * 2 * sizeof(uint16));
	pVB->m_Priority	= 10000000.0f;
	_pVBM->AddVB(pVB);
*/
	int nV = m_lVerticesD.Len();
	CVec3Dfp32* pV = _pVBM->Alloc_V3(nV);
	if (!pV)
		return;

	const CVec3Dfp64* pV8 = m_lVerticesD.GetBasePtr();
	for(int v = 0; v < nV; v++)
		pV8[v].Assignfp32(pV[v]);

	_pVBM->RenderWires(*_pMatrix, pV, m_lEdges.GetBasePtr()->m_iV, m_lEdges.Len() * 2, Col, false);
}


void CSolid::RenderEdges(CRenderContext* _pRC, const CMat4Dfp32* _pMatrix, const CPixel32& _Col) const
{
	MAUTOSTRIP(CSolid_RenderEdges, MAUTOSTRIP_VOID);

	CPixel32 Col = _Col;

	int iInvalid = 0;
	int nFaces = GetNumFaces();
	for(int f = 0; f < nFaces; f++)
	{
		const CSolid_Face& Face = GetFace(f);
		if(Face.m_iInvalid > iInvalid)
		{
			Col = _Col * Face.m_Color;
			iInvalid = Face.m_iInvalid;
		}
	}

	CVec3Dfp32 lTemp[CSOLID_MAXVERTICES];
	int nV = m_lVerticesD.Len();
	 const CVec3Dfp64* pV = m_lVerticesD.GetBasePtr();
	for(int v = 0; v < nV; v++)
	{
		pV[v].Assignfp32(lTemp[v]);
		lTemp[v] *= *_pMatrix;
	}

	_pRC->Attrib_TextureID(0, 0);
	_pRC->Geometry_Clear();
	_pRC->Geometry_VertexArray(lTemp, nV, true);
//	_pRC->Geometry_VertexArray(g_lVertices, m_lVertices.Len());
//	_pRC->Geometry_ColorArray(lColors);
//	_pRC->Geometry_Color(0xffffffff);
	_pRC->Geometry_Color(Col);

	_pRC->Render_IndexedWires(const_cast<uint16*>(m_lEdges.GetBasePtr()->m_iV), m_lEdges.Len()*2);

	_pRC->Geometry_VertexArray(NULL, 0, 0);
	_pRC->Geometry_ColorArray(NULL);


/*	const CVec3Dfp32* pV = m_lVertices.GetBasePtr();
	const CSolid_Edge* pE = m_lEdges.GetBasePtr();
	for(int i = 0; i < m_lEdges.Len(); i++)
	{
		CPixel32 Col = _Col;
		if (pE[i].m_iFaces[0] != -1 && m_lspFaces[pE[i].m_iFaces[0]]->m_Color != 0xffffffff)
			Col *= m_lspFaces[pE[i].m_iFaces[0]]->m_Color;
		if (pE[i].m_iFaces[1] != -1 && m_lspFaces[pE[i].m_iFaces[1]]->m_Color != 0xffffffff)
			Col *= m_lspFaces[pE[i].m_iFaces[1]]->m_Color;

		_pRC->Render_Wire(pV[pE[i].m_iV[0]], pV[pE[i].m_iV[1]], Col);
	}*/
}

void CSolid::RenderWire(CRenderContext* _pRC, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CPixel32& _Col)
{
	MAUTOSTRIP(CSolid_RenderWire, MAUTOSTRIP_VOID);

//	Error("RenderWire", "No longer supported.");

	CMat4Dfp32 m_CurL2VMat;
	_WMat.Multiply(_VMat, m_CurL2VMat);

	CVec3Dfp32 Pos = m_BoundPos;
	Pos *= m_CurL2VMat;
	if (!_pRC->Viewport_Get()->SphereInView(Pos, m_BoundRadius))
		return;

	//_pRC->Matrix_PushMultiply(m_CurL2VMat);

//	CVec3Dfp32::MultiplyMatrix(m_lVertices.GetBasePtr(), g_lVertices, m_CurL2VMat, m_lVertices.Len());

/*	int v = m_lVertices.Len();
	CVec3Dfp32* pV = m_lVertices.GetBasePtr();
	for(int v = 0; v < nv; v++)*/


	RenderEdges(_pRC, &m_CurL2VMat, _Col);
//	for(int f = 0; f < m_lspFaces.Len(); f++)
//		_pRC->Render_WireLoop(m_lVertices.GetBasePtr(), m_lspFaces[f]->m_liVertices.GetBasePtr(), m_lspFaces[f]->m_liVertices.Len(), _Col);
	//_pRC->Matrix_Pop();
}

void CSolid::RenderWire(CWireContainer* _pWC, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CPixel32& _Col)
{
	MAUTOSTRIP(CSolid_RenderWire, MAUTOSTRIP_VOID);
	CMat4Dfp32 m_CurL2VMat;
	_WMat.Multiply(_VMat, m_CurL2VMat);

	RenderEdges(_pWC, &m_CurL2VMat, _Col);
}

void CSolid::RenderWire(CXR_VBManager* _pVBM, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CPixel32& _Col)
{
	MAUTOSTRIP(CSolid_RenderWire, MAUTOSTRIP_VOID);
	CMat4Dfp32 m_CurL2VMat;
	_WMat.Multiply(_VMat, m_CurL2VMat);

	CVec3Dfp32 Pos = m_BoundPos;
	Pos *= m_CurL2VMat;
	if (!_pVBM->Viewport_Get()->SphereInView(Pos, m_BoundRadius))
		return;

	RenderEdges(_pVBM, &m_CurL2VMat, _Col);
}

void CSolid::RenderSolidNoCull(CRenderContext* _pRC, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CPixel32& _Col, int _iType, CMTime _AnimTime)
{
	MAUTOSTRIP(CSolid_RenderSolidNoCull, MAUTOSTRIP_VOID);
	CMat4Dfp32 m_CurL2VMat;
	_WMat.Multiply(_VMat, m_CurL2VMat);

	CMat4Dfp32 VMatInv;
	m_CurL2VMat.InverseOrthogonal(VMatInv);
	CVec3Dfp32 LocalVP;
	LocalVP = CVec3Dfp32::GetRow(VMatInv, 3);
	CVec3Dfp64 LocalVP8(LocalVP.k[0], LocalVP.k[1], LocalVP.k[2]);

	CVec3Dfp32 vLight(-0.4f, -0.6f, 1.0f);
	vLight.Normalize();

	_pRC->Attrib_Push();
	_pRC->Matrix_PushMultiply(m_CurL2VMat);
	_pRC->Attrib_Disable(CRC_FLAGS_SMOOTH);
	int nF = GetNumFaces();
	for(int f = 0; f < nF; f++)
	{
		if((_iType & OGIER_RENDER_CULL) && m_lspPlanes[f]->m_Plane.Distance(LocalVP8) < -0.0f)
			continue;

		const CSolid_Face* pF = &GetFace(f);
		CSolid_Plane *pP = m_lspPlanes[f];
		if(pP->m_spMapping && pP->m_spMapping->m_pSurface)
		{
			if(pP->m_spMapping->m_pSurface->m_Flags & XW_SURFFLAGS_OGRINVISIBLE || pP->m_spMapping->m_pSurface->m_Flags & XW_SURFFLAGS_INVISIBLE)
				continue;

			int TxtID = 0;
			if (pP->m_spMapping && pP->m_spMapping->m_pSurface && (_iType & OGIER_RENDER_TEXTURES))
			{
				CXW_SurfaceKeyFrame* pSKF = pP->m_spMapping->m_pSurface->GetBaseFrame();
				TxtID = pSKF->m_lTextures[0].m_TextureID;

				if(pSKF->m_lTextures[0].m_Flags & XW_LAYERFLAGS_ALPHACOMPARE)
					_pRC->Attrib_AlphaCompare(CRC_COMPARE_GREATEREQUAL, pSKF->m_lTextures[0].m_AlphaRef);
			}

			_pRC->Attrib_TextureID(0, TxtID);
		}
		else
			_pRC->Attrib_TextureID(0, 0);

		// Cheap shading..
		CPixel32 Col = _Col * pF->m_Color;
		if(_iType & OGIER_RENDER_SHADE)
		{
			CVec3Dfp32 n = pP->m_Plane.n.Getfp32();
			n.MultiplyMatrix3x3(_WMat);
			fp32 dotp = vLight * n;
			fp32 i = 0.60f + dotp*0.40f;
			Col.R() *= i;
			Col.G() *= i;
			Col.B() *= i;
		}

		int nv = pF->m_nV;

		Error("RenderSolidNoCull", "FixMe, using legacy rendering API.");

//		_pRC->Render_Polygon(nv, m_lVertices.GetBasePtr(), pF->m_lTVertices.GetBasePtr(),
//			pF->m_liVertices.GetBasePtr(), g_IndexRamp32, Col);
	}

/*	for(int f = 0; f < m_lspFaces.Len(); f++)
	{
		CSolid_Face *pF = m_lspFaces[f];
		CSolid_Plane *pP = m_lspPlanes[f];
		
		

	CXR_Util::Render_Surface(0, pP->m_spMapping->m_pSurface, pP->m_spMapping->m_pSurface->GetBaseFrame(), NULL, pVBM, NULL, NULL, &m_CurL2VMat, &VB, 
		0, 1.0f, NULL, NULL, 0);
	}*/

	_pRC->Matrix_Pop();
	_pRC->Attrib_Pop();
}

void CSolid::RenderSolidNoCull(CRenderContext* _pRC, CXR_VBManager *_pVBM, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CPixel32& _Col, int _iType, CMTime _AnimTime, COgrRenderBuffer &_RenderBuffer)
{
	MAUTOSTRIP(CSolid_RenderSolidNoCull_2, MAUTOSTRIP_VOID);
	CMat4Dfp32 L2V;
	_WMat.Multiply(_VMat, L2V);
	if( _iType & OGIER_RENDER_NOTRANSFORM ) L2V.Unit();

	CMat4Dfp32 VMatInv;
	L2V.InverseOrthogonal(VMatInv);
	CVec3Dfp32 LocalVP;
	LocalVP = CVec3Dfp32::GetRow(VMatInv, 3);
	CVec3Dfp64 LocalVP8(LocalVP.k[0], LocalVP.k[1], LocalVP.k[2]);

	CVec3Dfp32 vLight(-0.4f, -0.6f, 1.0f);
	vLight.Normalize();

	// Max 64 surfaces
	int8 iVB[CSOLID_MAXPLANES];
	int8 bBack[CSOLID_MAXPLANES];
	int nVB = 0;
	int nVertices[64];
	CXR_VertexBuffer *lpVB[64];
	CXW_Surface *pSurf[64];
	int nPolys[64];
	bool bTrans[64];

	int f;
	int nF = GetNumFaces();
	for(f = 0; f < nF; f++)
	{
		bBack[f] = false;
		if(m_lspPlanes[f]->m_Plane.Distance(LocalVP8) < -0.0f)
		{
			if(_iType & OGIER_RENDER_CULL)
			{
				iVB[f] = -1;
				continue;
			}
			else
				bBack[f] = true;
		}

		CXW_Surface *pS = NULL;
		const CSolid_Face* pF = &GetFace(f);
		CSolid_Plane *pP = m_lspPlanes[f];
		if(pP->m_spMapping && pP->m_spMapping->m_pSurface)
		{
			int Flags = pP->m_spMapping->m_pSurface->m_Flags;
			if(Flags & XW_SURFFLAGS_OGRINVISIBLE || ((_iType & OGIER_RENDER_IGNOREINVISIBLE) && Flags & XW_SURFFLAGS_INVISIBLE))
			{
				iVB[f] = -1;
				continue;
			}

			if (_iType & OGIER_RENDER_TEXTURES)
				pS = pP->m_spMapping->m_pSurface;
		}

		int i;
		for(i = 0; i < nVB; i++)
			if(pSurf[i] == pS)
				break;

		if(i == nVB)
		{
			nVB++;
			lpVB[i] = _pVBM->Alloc_VB(CXR_VB_ATTRIB);
			if(!lpVB[i])
				return;

			if(pS)
				bTrans[i] = (pS->m_Flags & XW_SURFFLAGS_TRANSPARENT) != 0 || (pF->m_Color.GetA() != 255) || (_Col.GetA() != 255);
			else
			{
				bTrans[i] = (pF->m_Color.GetA() != 255) || (_Col.GetA() != 255);
				if(bTrans[i]) // No surface and transparent requires manually set rastermode
				{
					lpVB[i]->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
					lpVB[i]->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
				}
			}


			nVertices[i] = pF->m_nV;
			nPolys[i] = 1;
			pSurf[i] = pS;
		}
		else
		{
			nVertices[i] += pF->m_nV;
			nPolys[i]++;
		}

		iVB[f] = i;
	}

	int i;
	for(i = 0; i < nVB; i++)
	{
		CPixel32 *pC = _pVBM->Alloc_CPixel32(nVertices[i]);
		if(!pC)
			return;

		if (!lpVB[i]->AllocVBChain(_pVBM, false))
			return;
		lpVB[i]->Geometry_ColorArray(pC);
		
		CVec3Dfp32 *pV = _pVBM->Alloc_V3(nVertices[i]);
		if(!pV)
			return;
		lpVB[i]->Geometry_VertexArray(pV, nVertices[i], true);
		
		if(pSurf[i])
		{
			CVec2Dfp32 *pTV = _pVBM->Alloc_V2(nVertices[i]);
			if(!pTV)
				return;
			lpVB[i]->Geometry_TVertexArray(pTV, 0);

//			if(pSurf[i]->m_Flags & XW_SURFFLAGS_NEEDNORMALS)
			{				
				CVec3Dfp32 *pN = _pVBM->Alloc_V3(nVertices[i]);
				if(!pN)
					return;
				lpVB[i]->Geometry_Normal(pN);
			}
		}

		if (_iType & OGIER_RENDER_PERPIXEL)
		{
			CVec3Dfp32* pTangU = _pVBM->Alloc_V3(nVertices[i]);
			CVec3Dfp32* pTangV = _pVBM->Alloc_V3(nVertices[i]);

			lpVB[i]->Geometry_TVertexArray(pTangU, 1);	// Dummy
			lpVB[i]->Geometry_TVertexArray(pTangU, 2);
			lpVB[i]->Geometry_TVertexArray(pTangV, 3);
		}
		
		int StreamLen = 2 * nPolys[i] + nVertices[i] + 1;
		uint16 *pStream = _pVBM->Alloc_Int16(StreamLen);
		if(!pStream)
			return;
		lpVB[i]->Render_IndexedPrimitives(pStream, StreamLen);

		nVertices[i] = 0;
		nPolys[i] = 0;
	}

	int iPass = 0;
	if(_iType & OGIER_RENDER_CULL)
		iPass = 1;

	for(int p = iPass; p < 2; p++)
	{
		for(f = 0; f < nF; f++)
		{
			if(iVB[f] == -1)
				continue;

			if(p == 0 && !bBack[f])
				continue;
			if(p == 1 && bBack[f])
				continue;
			
			CSolid_Face *pF = &GetFaceNonConst(f);
			CSolid_Plane *pP = m_lspPlanes[f];

			CXR_VertexBuffer *pVB = lpVB[iVB[f]];
			
			CPixel32 Col = _Col * pF->m_Color;
			if((int(Col) & 0xfcfcfcfc) == 0xfcfcfcfc)
				Col = 0xffffffff;
			if(Col != -1 && _iType & OGIER_RENDER_PERPIXEL)
				pVB->m_Flags |= CXR_VBFLAGS_OGR_SELECTED;
			if(_iType & OGIER_RENDER_OVERBRIGHT && !pSurf[iVB[f]])
				Col *= CPixel32(0xff808080);
			
			// Cheap shading..
			if(COgrRenderBuffer::ShouldShade(pSurf[iVB[f]], _iType))
			{
				CVec3Dfp32 n = pP->m_Plane.n.Getfp32();
				n.MultiplyMatrix3x3(_WMat);
				fp32 dotp = vLight * n;
				fp32 i = 0.60f + dotp*0.40f;
				Col.R() *= i;
				Col.G() *= i;
				Col.B() *= i;
			}

			int nVert = pF->m_nV;

			int iS = nPolys[iVB[f]];
			if (!pVB->AllocVBChain(_pVBM, false))
				return;
			CXR_VBChain *pChain = pVB->GetVBChain();
			uint16 *pStream = pChain->m_piPrim;
			pStream[iS++] = CRC_RIP_TRIFAN + ((nVert + 2) << 8);
			pStream[iS++] = nVert;

			const uint16* piV = GetFaceVertexIndices(*pF);
			const CVec2Dfp32* pFaceTV = GetFaceTVertices(*pF);

			int iV = nVertices[iVB[f]];
			for(int i = 0; i < nVert; i++)
			{
				pChain->m_pV[iV] = m_lVerticesD[piV[i]].Getfp32() * L2V;
				pChain->m_pCol[iV] = Col;
				pStream[iS++] = iV++;
			}

			if(pChain->m_pTV[0])
			{
				int iV = nVertices[iVB[f]];
				CVec2Dfp32* pTV = (CVec2Dfp32*) pChain->m_pTV[0];
				for(int i = 0; i < nVert; i++)
					pTV[iV++] = pFaceTV[i];
			}

			if(pChain->m_pN)
			{
				int iV = nVertices[iVB[f]];
				for(int i = 0; i < nVert; i++)
				{
					pChain->m_pN[iV] = pF->m_spPlane->m_Plane.n.Getfp32();
					pChain->m_pN[iV++].MultiplyMatrix3x3(L2V);
				}
			}

			if(pChain->m_pTV[2] && pChain->m_pTV[3] && pF->m_spPlane->m_spMapping)
			{
				int iV = nVertices[iVB[f]];

				CVec3Dfp32* pTangU = (CVec3Dfp32*) pChain->m_pTV[2];
				CVec3Dfp32* pTangV = (CVec3Dfp32*) pChain->m_pTV[3];
				pTangU += iV;
				pTangV += iV;
				
				CVec3Dfp32 N = pF->m_spPlane->m_Plane.n.Getfp32();
				CXR_PlaneMapping Mapping = pF->m_spPlane->m_spMapping->GetMappingPlane(pF->m_spPlane->m_Plane);
				
				fp32 ULengthRecp = 1.0f / Mapping.m_U.Length();
				fp32 VLengthRecp = 1.0f / Mapping.m_V.Length();
				
				{
					CVec3Dfp32 TangU;
					Mapping.m_U.Scale(ULengthRecp, TangU);
					TangU.Combine(N, -(N * TangU), TangU);
					TangU.Normalize();
					for(int v = 0; v < nVert; v++)
					{
						*pTangU = TangU;
						pTangU->MultiplyMatrix3x3(L2V);
						pTangU++;
					}
				}
				
				{
					CVec3Dfp32 TangV;
					Mapping.m_V.Scale(VLengthRecp, TangV);
					TangV.Combine(N, -(N * TangV), TangV);
					TangV.Normalize();
					for(int v = 0; v < nVert; v++)
					{
						*pTangV = TangV;
						pTangV->MultiplyMatrix3x3(L2V);
						pTangV++;
					}
				}
			}
			
			nPolys[iVB[f]] += 2 + nVert;
			nVertices[iVB[f]] += nVert;
		}
	}	
	
	for(i = 0; i < nVB; i++)
	{
		lpVB[i]->GetVBChain()->m_piPrim[nPolys[i]] = CRC_RIP_END + (1 << 8);	

		_RenderBuffer.OgrAddVB(_pRC, _pVBM, lpVB[i], bTrans[i], m_BoundPos, LocalVP, _AnimTime, pSurf[i]);
	}
}

void CSolid::RenderSolid(CRenderContext* _pRC, CXR_VBManager *_pVBM, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CPixel32& _Col, int _iType, CMTime _AnimTime, COgrRenderBuffer &_RenderBuffer)
{
	MAUTOSTRIP(CSolid_RenderSolid, MAUTOSTRIP_VOID);
	CMat4Dfp32 m_CurL2VMat;
	_WMat.Multiply(_VMat, m_CurL2VMat);
	CVec3Dfp32 Pos = m_BoundPos;
	Pos *= m_CurL2VMat;
	if ( !(_iType & OGIER_RENDER_NOCULL) && !_pRC->Viewport_Get()->SphereInView(Pos, m_BoundRadius)) return;
	RenderSolidNoCull(_pRC, _pVBM, _WMat, _VMat, _Col, _iType, _AnimTime, _RenderBuffer);
}

void CSolid::Render(CRenderContext* _pRC, CXR_VBManager *_pVBM, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CPixel32& _Col, int _iType, CMTime _AnimTime, COgrRenderBuffer &_RenderBuffer)
{
	MAUTOSTRIP(CSolid_Render, MAUTOSTRIP_VOID);
	MSCOPE(CSolid::Render, OGIER);
	if(_iType & OGIER_RENDER_WIRE)
		RenderWire(_pRC, _WMat, _VMat, _Col);
	else
		RenderSolid(_pRC, _pVBM, _WMat, _VMat, _Col, _iType, _AnimTime, _RenderBuffer);
}


#define MAT_EPSILON 0.001f

static bool IsNiceMatrix(const CMat4Dfp32 &_Mat)
{
	MAUTOSTRIP(IsNiceMatrix, false);
	CVec3Dfp32 V0 = CVec3Dfp32::GetMatrixRow(_Mat, 0);
	CVec3Dfp32 V1 = CVec3Dfp32::GetMatrixRow(_Mat, 1);
	CVec3Dfp32 V2 = CVec3Dfp32::GetMatrixRow(_Mat, 2);
	V0.Normalize();
	V1.Normalize();
	V2.Normalize();

	if (M_Fabs(V0 * V1) > MAT_EPSILON) return false;
	if (M_Fabs(V0 * V2) > MAT_EPSILON) return false;
	if (M_Fabs(V1 * V2) > MAT_EPSILON) return false;

	static CVec3Dfp32 X(1,0,0);
	static CVec3Dfp32 Y(0,1,0);
	static CVec3Dfp32 Z(0,0,1);

	fp32 dotp = M_Fabs(V0 * X);
	if ((dotp > MAT_EPSILON) && (dotp < (1.0f-MAT_EPSILON))) return false;
	dotp = M_Fabs(V0 * Y);
	if ((dotp > MAT_EPSILON) && (dotp < (1.0f-MAT_EPSILON))) return false;
	dotp = M_Fabs(V0 * Z);
	if ((dotp > MAT_EPSILON) && (dotp < (1.0f-MAT_EPSILON))) return false;

	dotp = M_Fabs(V1 * X);
	if ((dotp > MAT_EPSILON) && (dotp < (1.0f-MAT_EPSILON))) return false;
	dotp = M_Fabs(V1 * Y);
	if ((dotp > MAT_EPSILON) && (dotp < (1.0f-MAT_EPSILON))) return false;
	dotp = M_Fabs(V1 * Z);
	if ((dotp > MAT_EPSILON) && (dotp < (1.0f-MAT_EPSILON))) return false;

	dotp = M_Fabs(V2 * X);
	if ((dotp > MAT_EPSILON) && (dotp < (1.0f-MAT_EPSILON))) return false;
	dotp = M_Fabs(V2 * Y);
	if ((dotp > MAT_EPSILON) && (dotp < (1.0f-MAT_EPSILON))) return false;
	dotp = M_Fabs(V2 * Z);
	if ((dotp > MAT_EPSILON) && (dotp < (1.0f-MAT_EPSILON))) return false;

	return true;
}

// -------------------------------------------------------------------
void CSolid::Apply(const CMat4Dfp32 &_Mat,bool _bSnap, int _TextureLock)
{
	MAUTOSTRIP(CSolid_Apply, MAUTOSTRIP_VOID);
//	if (_bSnap) _bSnap = IsNiceMatrix(_Mat);

	if(_TextureLock & 2)
		SetMappingType(-1, OGIER_MAPPINGTYPE_PLANEMAPPED);

	CMat4Dfp64 Mat8;
	_Mat.Assignfp64(Mat8);

	int nVert = m_lVerticesD.Len();
	int i;
	for(i = 0; i < nVert; i++)
	{
//		m_lVertices[i] = m_lVertices[i] * _Mat;
		m_lVerticesD[i] = m_lVerticesD[i] * Mat8;
/*		if(_bSnap)
		{
			m_lVertices[i].Snap(CSOLID_SNAPGRID, CSOLID_SNAPTRESH);
			m_lVerticesD[i].Snap(CSOLID_SNAPGRID, CSOLID_SNAPTRESH);
		}*/
	}

	int nPlanes = m_lspPlanes.Len();
	for(i = 0; i < nPlanes; i++)
	{
		const uint16* piV = GetFaceVertexIndices(i);

		m_lspPlanes[i]->m_v0 = m_lVerticesD[piV[0]];
		m_lspPlanes[i]->m_v1 = m_lVerticesD[piV[1]];
		m_lspPlanes[i]->m_v2 = m_lVerticesD[piV[2]];
		m_lspPlanes[i]->m_Plane.Create(m_lspPlanes[i]->m_v0, m_lspPlanes[i]->m_v2, m_lspPlanes[i]->m_v1);
	}

	UpdateBound();

	CMat4Dfp32 IMat;
	if(_TextureLock)
		_Mat.Inverse(IMat);
	int nFaces = GetNumFaces();
	for(i = 0; i < nFaces; i++)
	{
		CSolid_Face *pFace = &GetFaceNonConst(i);
		if(_TextureLock)
		{
			// Convert to planemapping before transforming texture
			MoveTexture(i, _Mat);
/*			CMat4Dfp32 Mat2 = _Mat;
			Mat2.GetRow(3) = 0;
			MoveTexture(i, Mat2);
			Mat2.Unit();
			_Mat.GetRow(3).SetRow(Mat2, 3);
			MoveTexture(i, Mat2);*/
		}

		UpdateSurface(i);
	}

//	if(_bSnap)
//		SetFaceColors();
}

void CSolid::MoveVertex(int _iIndex, const CVec3Dfp32 &_V, bool _bSnap)
{
	MAUTOSTRIP(CSolid_MoveVertex, MAUTOSTRIP_VOID);
	//Note: CreateFromMesh is appropriate after this
	m_lVerticesD[_iIndex] += _V.Getfp64();
//	m_lVertices[_iIndex] += _V;
	if(_bSnap)
	{
//		m_lVertices[_iIndex].Snap(CSOLID_SNAPGRID, CSOLID_SNAPTRESH);
		m_lVerticesD[_iIndex].Snap(CSOLID_SNAPGRID, CSOLID_SNAPTRESH);
	}
}

void CSolid::MoveTexture(int _iIndex, const CVec2Dfp32 &_O, const CVec2Dfp32 &_S, float _Rot, const CVec3Dfp32 *_Ref)
{
	MAUTOSTRIP(CSolid_MoveTexture, MAUTOSTRIP_VOID);
	if(_iIndex < 0 || _iIndex >= GetNumFaces())
		return;
	if (!m_lspPlanes[_iIndex]->m_spMapping)
		return;

	m_lspPlanes[_iIndex]->m_spMapping->MoveTexture(m_lspPlanes[_iIndex]->m_Plane, _O, _S, _Rot, _Ref);

	UpdateSurface(_iIndex);
}

void CSolid::MoveTexture(int _iIndex, const CMat4Dfp32& _Mat)
{
	MAUTOSTRIP(CSolid_MoveTexture_2, MAUTOSTRIP_VOID);
	if(_iIndex < 0 || _iIndex >= GetNumFaces())
		return;
	if (!m_lspPlanes[_iIndex]->m_spMapping)
		return;

	CSolid_Face* pF = &GetFaceNonConst(_iIndex);

	CXR_PlaneMapping Mapping = m_lspPlanes[_iIndex]->m_spMapping->GetMappingPlane(m_lspPlanes[_iIndex]->m_Plane);

	CMat4Dfp32 Mat;
	_Mat.Inverse(Mat);
	Mat.Transpose3x3();
	CVec3Dfp32 UVec = Mapping.m_U;
	CVec3Dfp32 VVec = Mapping.m_V;
	UVec *= 1.0f / (UVec * UVec);
	VVec *= 1.0f / (VVec * VVec);
	UVec.MultiplyMatrix3x3(Mat);
	VVec.MultiplyMatrix3x3(Mat);
	UVec *= 1.0f / (UVec * UVec);
	VVec *= 1.0f / (VVec * VVec);

	fp32 UVecLenSqrInv = 1.0f/(UVec*UVec);
	fp32 VVecLenSqrInv = 1.0f/(VVec*VVec);

	fp32 dU = (UVec * CVec3Dfp32::GetMatrixRow(_Mat, 3)) * UVecLenSqrInv;
	fp32 dV = (VVec * CVec3Dfp32::GetMatrixRow(_Mat, 3)) * VVecLenSqrInv;

	Mapping.m_UOffset -= dU;
	Mapping.m_VOffset -= dV;
	Mapping.m_U = UVec;
	Mapping.m_V = VVec;
	m_lspPlanes[_iIndex]->m_spMapping->SetMappingPlane(Mapping, m_lspPlanes[_iIndex]->m_Plane);

//	MoveTexture(_iIndex, CVec2Dfp32(dU, dV), CVec2Dfp32(1,1), 0, NULL);
}

void CSolid::FitTexture(int _iFace, int _iBase)
{
	MAUTOSTRIP(CSolid_FitTexture, MAUTOSTRIP_VOID);

	if(_iFace < 0 || _iFace >= GetNumFaces())
		return;
	if (!m_lspPlanes[_iFace]->m_spMapping)
		return;

	CXW_Surface *pSurf = GetSurface(_iFace);
	if(!pSurf)
		return;

	CVec3Dfp32 ReferenceDim = pSurf->GetTextureMappingReferenceDimensions();
	fp32 TxtWInv = 1.0f / ReferenceDim[0];
	fp32 TxtHInv = 1.0f / ReferenceDim[1];

	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if(!pTC)
		return;

	if(!pSurf->GetBaseFrame() || pSurf->GetBaseFrame()->m_lTextures.Len() == 0)
		return;
	CXW_SurfaceLayer *pLayer = &pSurf->GetBaseFrame()->m_lTextures[0];

	CSolid_Face* pF = &GetFaceNonConst(_iFace);

	SetMappingType(_iFace, OGIER_MAPPINGTYPE_PLANEMAPPED);
	int nVert = pF->m_nV;
	const uint16* piV = GetFaceVertexIndices(*pF);
	CVec3Dfp32 V0 = m_lVerticesD[piV[(_iBase + 0) % nVert]].Getfp32();
	CVec3Dfp32 V1 = m_lVerticesD[piV[(_iBase + 1) % nVert]].Getfp32();
	CVec3Dfp32 V2 = m_lVerticesD[piV[(_iBase + 2) % nVert]].Getfp32();
	
	CImage Desc;
	int nMip;
	pTC->GetTextureDesc(pLayer->m_TextureID, &Desc, nMip);

	int Width = Desc.GetWidth();
	int Height = Desc.GetHeight();

	CXR_PlaneMapping Mapping;
	CVec3Dfp32 E0 = V1 - V0;
	CVec3Dfp32 E1 = V2 - V1;
	Mapping.m_U = (E1 / m_lspPlanes[_iFace]->m_Plane.n.Getfp32()).Normalize();
	Mapping.m_V = (E0 / m_lspPlanes[_iFace]->m_Plane.n.Getfp32()).Normalize();

	Mapping.m_U.SetLength((E0 * Mapping.m_U) * TxtWInv);
	Mapping.m_V.SetLength((E1 * Mapping.m_V) * TxtHInv);
		
	CVec3Dfp32 U = Mapping.m_U;
	CVec3Dfp32 V = Mapping.m_V;
	U.SetLength(Mapping.m_U.LengthInv());
	V.SetLength(Mapping.m_V.LengthInv());

	CPlane3Dfp32 PlaneU(U, V0);
	Mapping.m_UOffset = PlaneU.d;
	CPlane3Dfp32 PlaneV(V, V0);
	Mapping.m_VOffset = PlaneV.d;

	m_lspPlanes[_iFace]->m_spMapping->SetMappingPlane(Mapping, m_lspPlanes[_iFace]->m_Plane);
	UpdateSurface(_iFace);
}

static void CreateMapping(const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, const CVec3Dfp32& _v2, const CVec2Dfp32& _tv0, const CVec2Dfp32& _tv1, const CVec2Dfp32& _tv2, CXR_PlaneMapping& _Mapping)
{
	CMat4Dfp32 U2W;
	U2W.Unit();
	CVec3Dfp32 e0, e1, en;
	_v1.Sub(_v0, e0);
	_v2.Sub(_v0, e1);
	e0.CrossProd(e1, en);
	e0.SetRow(U2W, 0);
	e1.SetRow(U2W, 1);
	en.SetRow(U2W, 2);
	_v0.SetRow(U2W, 3);


	CMat4Dfp32 U2T;
	U2T.Unit();
	CVec3Dfp32 tv0(_tv0[0], _tv0[1], 0);
	CVec3Dfp32 tv1(_tv1[0], _tv1[1], 0);
	CVec3Dfp32 tv2(_tv2[0], _tv2[1], 0);

	CVec3Dfp32 te0, te1, ten;
	tv1.Sub(tv0, te0);
	tv2.Sub(tv0, te1);
	te0.CrossProd(te1, ten);
	te0.SetRow(U2T, 0);
	te1.SetRow(U2T, 1);
	ten.SetRow(U2T, 2);
	tv0.SetRow(U2T, 3);

	CMat4Dfp32 W2U;
	U2W.Inverse(W2U);

	CMat4Dfp32 T2U;
	U2T.Inverse(T2U);

	CMat4Dfp32 W2T;
	CMat4Dfp32 T2W;
	//	U2T.Multiply(W2U, W2T);
	T2U.Multiply(U2W, T2W);
	//	T2U.Multiply(U2W, W2T);
	W2U.Multiply(U2T, W2T);

	const CMat4Dfp32& T(W2T);
	CVec3Dfp32 U(T.k[0][0], T.k[1][0], T.k[2][0]);
	CVec3Dfp32 V(T.k[0][1], T.k[1][1], T.k[2][1]);

	fp32 ULen = U.Length();
	fp32 VLen = V.Length();
	U.Scale(1.0f / Sqr(ULen), U);
	V.Scale(1.0f / Sqr(VLen), V);

	ULen = U.Length();
	VLen = V.Length();
	//	LogFile(CStrF("              ULen %f, VLen %f", ULen, VLen));

	_Mapping.m_U = U;
	_Mapping.m_V = V;
	_Mapping.m_UOffset = _tv0[0] - (U * _v0) / Sqr(ULen);
	_Mapping.m_VOffset = _tv0[1] - (V * _v0) / Sqr(VLen);

	/*	CVec2Dfp32 mtv0 = EvalUV(_Mapping, _v0);
	CVec2Dfp32 mtv1 = EvalUV(_Mapping, _v1);
	CVec2Dfp32 mtv2 = EvalUV(_Mapping, _v2);

	LogFile(CStrF("                 %s, %s, %s", _tv0.GetString().Str(), mtv0.GetString().Str(), (mtv0 - _tv0).GetString().Str() ));
	LogFile(CStrF("                 %s, %s, %s", _tv1.GetString().Str(), mtv1.GetString().Str(), (mtv1 - _tv1).GetString().Str() ));
	LogFile(CStrF("                 %s, %s, %s", _tv2.GetString().Str(), mtv2.GetString().Str(), (mtv2 - _tv2).GetString().Str() ));*/
}

void CSolid::TileTexture(int _iFace, TArray<CVec3Dfp32> &_lVert, TArray<CVec2Dfp32> &_lTV)
{
	if(_iFace < 0 || _iFace >= GetNumFaces())
		return;
	if (!m_lspPlanes[_iFace]->m_spMapping)
		return;
	if(_lVert.Len() < 3)
		return;
	CXW_Surface *pSurf = GetSurface(_iFace);
	if(!pSurf)
		return;

	CVec3Dfp32 ReferenceDim = pSurf->GetTextureMappingReferenceDimensions();
	fp32 TxtWInv = 1.0f / ReferenceDim[0];
	fp32 TxtHInv = 1.0f / ReferenceDim[1];

	if(!pSurf->GetBaseFrame() || pSurf->GetBaseFrame()->m_lTextures.Len() == 0)
		return;
	CXW_SurfaceLayer *pLayer = &pSurf->GetBaseFrame()->m_lTextures[0];

	CSolid_Face* pF = &GetFaceNonConst(_iFace);
	const uint16* piV = GetFaceVertexIndices(*pF);

	int nMatch = 0;
	CVec3Dfp32 lVert[2];
	CVec2Dfp32 lTV[2];
	int liHit[2];
	for(int j = 0; j < _lVert.Len(); j++)
	{
		int i;
		for(i = 0; i < pF->m_nV; i++)
			if((m_lVerticesD[piV[i]].Getfp32() - _lVert[j]).LengthSqr() < 0.001f)
			{
				liHit[nMatch] = j;
				lVert[nMatch] = _lVert[j];
				lTV[nMatch++] = _lTV[j];
				break;
			}

		if(nMatch == 2)
			break;
	}

	if(nMatch == 1)
	{
		for(int j = 0; j < _lVert.Len(); j++)
		{
			CVec3Dfp32 VV1 = _lVert[j];
			CVec3Dfp32 V1 = VV1 - lVert[0];
			fp32 V1Len = V1.Length();
			if(V1Len == 0)
				continue;

			int i;
			for(i = 0; i < pF->m_nV; i++)
			{
				CVec3Dfp32 V = m_lVerticesD[piV[i]].Getfp32();
				CVec3Dfp32 V2 = V - lVert[0];
				fp32 V2Len = V2.Length();
				if(V2Len == 0)
					continue;

				fp32 Dot = V1.Normalize() * V2.Normalize();
				if(Dot > 0.9999f)
				{
					liHit[nMatch] = j;
					lVert[nMatch] = m_lVerticesD[piV[i]].Getfp32();
					lTV[nMatch++] = lTV[0] + (_lTV[j] - lTV[0]) * (V2Len / V1Len);
					break;
				}
			}
			if(nMatch == 2)
				break;
		}
	}

	if(nMatch != 2)
		return;

	int iFree = -1;
	for(int j = 0; j < _lVert.Len(); j++)
	{
		if(liHit[0] != j && liHit[1] != j)
		{
			iFree = j;
			break;
		}
	}

	CVec2Dfp32 lTVOrg[3];
	for(int i = 0; i < 3; i++)
	{
		lTVOrg[i] = _lTV[i];
		lTVOrg[i][0] *= ReferenceDim[0];
		lTVOrg[i][1] *= ReferenceDim[1];
	}

	CXR_PlaneMapping Source;
	CreateMapping(_lVert[0], _lVert[1], _lVert[2], lTVOrg[0], lTVOrg[1], lTVOrg[2], Source);
	CXR_PlaneMapping Mapping = m_lspPlanes[_iFace]->m_spMapping->GetMappingPlane(m_lspPlanes[_iFace]->m_Plane);

	CVec3Dfp32 E = lVert[0] - lVert[1];
	CMat4Dfp32 Org, IOrg;
	Org.UnitNot3x3();
	E.SetRow(Org, 2);
	CVec3Dfp32 OrgN = (Source.m_V / Source.m_U).Normalize();
	OrgN.SetRow(Org, 0);
	Org.RecreateMatrix(2, 0);
	lVert[0].SetRow(Org, 3);
	Org.InverseOrthogonal(IOrg);

	CMat4Dfp32 Cur, ICur;
	Cur.UnitNot3x3();
	E.SetRow(Cur, 2);
	CVec3Dfp32 CurN = (Mapping.m_V / Mapping.m_U).Normalize();
	CurN.SetRow(Cur, 0);
	Cur.RecreateMatrix(2, 0);
	Cur.InverseOrthogonal(ICur);
	lVert[0].SetRow(Cur, 3);

	CMat4Dfp32 Rot;
	IOrg.Multiply(Cur, Rot);
	CVec3Dfp32 Ref = _lVert[iFree] * Rot;

	lTV[0][0] *= ReferenceDim[0];
	lTV[0][1] *= ReferenceDim[1];
	lTV[1][0] *= ReferenceDim[0];
	lTV[1][1] *= ReferenceDim[1];
	CVec2Dfp32 TV2 = _lTV[iFree];
	TV2[0] *= ReferenceDim[0];
	TV2[1] *= ReferenceDim[1];

	CreateMapping(lVert[0], lVert[1], Ref, lTV[0], lTV[1], TV2, Mapping);
	m_lspPlanes[_iFace]->m_spMapping->SetMappingPlane(Mapping, m_lspPlanes[_iFace]->m_Plane);
	UpdateSurface(_iFace);
}

void CSolid::SetMapping(int _iFace, CXR_PlaneMapping *_pMapping)
{
	SetMappingType(_iFace, OGIER_MAPPINGTYPE_PLANEMAPPED);
	m_lspPlanes[_iFace]->m_spMapping->SetMappingPlane(*_pMapping, m_lspPlanes[_iFace]->m_Plane);
	UpdateSurface(_iFace);
}

// -------------------------------------------------------------------
void CSolid::ReadGeometry(CCFile* _pF)
{
	MAUTOSTRIP(CSolid_ReadGeometry, MAUTOSTRIP_VOID);
	int16 Ver = 0;
	_pF->ReadLE(Ver);

	switch(Ver)
	{
	case 0x0100 :
		{
			int16 nP;
			_pF->ReadLE(nP);
			_pF->ReadLE(m_iNode);

			m_lspPlanes.SetLen(nP);
			for(int i = 0; i < nP; i++)
			{
				CPlane3Dfp32 P;
				CPlane3Dfp64 P8;
				P.n.Read(_pF);
				_pF->ReadLE(P.d);
				P8.n = P.n.Getfp64();
				P8.d = P.d;
				m_lspPlanes[i] = MNew1(CSolid_Plane,P8);
				if (!m_lspPlanes[i]) MemError("ReadGeometry");
			}
		}
		break;

	default :
		Error("ReadGeometry", CStrF("Unsupported version (%.4x > %.4x)", Ver, XWSOLID_GEOMETRY_VERSION));
	}

//	UpdateMesh();
}

void CSolid::ReadGeometry2(CCFile* _pF)
{
	MAUTOSTRIP(CSolid_ReadGeometry, MAUTOSTRIP_VOID);
	int16 Ver = 0;
	_pF->ReadLE(Ver);

	switch(Ver)
	{
	case 0x0100 :
		{
			int16 nP;
			_pF->ReadLE(nP);
			_pF->ReadLE(m_iNode);

			m_lspPlanes.SetLen(nP);
			for(int i = 0; i < nP; i++)
			{
				m_lspPlanes[i] = MNew(CSolid_Plane);
				if (!m_lspPlanes[i]) MemError("ReadGeometry");
				m_lspPlanes[i]->m_Plane.Read(_pF);
			}
		}
		break;

	default :
		Error("ReadGeometry", CStrF("Unsupported version (%.4x > %.4x)", Ver, XWSOLID_GEOMETRY_VERSION));
	}

	//	UpdateMesh();
}

void CSolid::WriteGeometry(CCFile* _pF)
{
	MAUTOSTRIP(CSolid_WriteGeometry, MAUTOSTRIP_VOID);
	int16 Ver = XWSOLID_GEOMETRY_VERSION;
	_pF->WriteLE(Ver);
	int16 nP = m_lspPlanes.Len();
	_pF->WriteLE(nP);
	_pF->WriteLE(m_iNode);
	for(int i = 0; i < nP; i++)
	{
		CPlane3Dfp32 P;
		P.n = m_lspPlanes[i]->m_Plane.n.Getfp32();
		P.d = m_lspPlanes[i]->m_Plane.d;
		P.n.Write(_pF);
		_pF->WriteLE(P.d);
	}
}

void CSolid::WriteGeometry2(CCFile* _pF)
{
	MAUTOSTRIP(CSolid_WriteGeometry, MAUTOSTRIP_VOID);
	int16 Ver = XWSOLID_GEOMETRY_VERSION;
	_pF->WriteLE(Ver);
	int16 nP = m_lspPlanes.Len();
	_pF->WriteLE(nP);
	_pF->WriteLE(m_iNode);
	for(int i = 0; i < nP; i++)
	{
		m_lspPlanes[i]->m_Plane.Write(_pF);
	}
}

void CSolid::Read(CCFile* _pF)
{
	_pF->ReadLE(m_Flags);
	m_Medium.Read(_pF);
	_pF->ReadLE(m_TempSurfaceID);
	ReadGeometry2(_pF);
}

void CSolid::Write(CCFile* _pF)
{
	_pF->WriteLE(m_Flags);
	m_Medium.Write(_pF);
	_pF->WriteLE(m_TempSurfaceID);
	WriteGeometry2(_pF);
}

void CSolid::WriteXMP(TArray<spCSolid> _lspSolids, CStr _FileName)
{
	CKeyContainerNode XMP;
	XMP.GetKeys()->AddKey("CLASSNAME", "WORLDSPAWN");

	for(int i = 0; i < _lspSolids.Len(); i++)
	{
		CSolid* pSrc = _lspSolids[i];
		for(; pSrc; pSrc = pSrc->m_spNext)
		{
			spCSolid pS = pSrc->Duplicate();
			pS->UpdateMesh();

			spCKeyContainerNode spBrushNode = MNew(CKeyContainerNode);
			XMP.AddChild(spBrushNode);

			spBrushNode->GetKeys()->AddKey("CLASSNAME", "BRUSH");
			for(int p = 0; p < pS->m_lspPlanes.Len(); p++)
			{
				CSolid_Plane* pP = pS->m_lspPlanes[p];
				if (!pP->m_spMapping)
				{
					TPtr<CSolid_MappingMAP> spM = MNew(CSolid_MappingMAP);
					spM->m_MapMapping.CreateUnit();
					pP->m_spMapping = spM;
				}

				CStr Key, Val;
				pS->GetPlaneKey(p, Key, Val);
				spBrushNode->GetKeys()->AddKey(Key, Val);
			}							
		}
	}
	XMP.WriteToScript(_FileName);
}

void CSolid::WriteXMP(CSolid* _pSolids, CStr _FileName)
{
	CKeyContainerNode XMP;
	XMP.GetKeys()->AddKey("CLASSNAME", "WORLDSPAWN");

	CSolid* pSrc = _pSolids;
	for(; pSrc; pSrc = pSrc->m_spNext)
	{
		spCSolid pS = pSrc->Duplicate();
		pS->UpdateMesh();

		spCKeyContainerNode spBrushNode = MNew(CKeyContainerNode);
		XMP.AddChild(spBrushNode);

		spBrushNode->GetKeys()->AddKey("CLASSNAME", "BRUSH");
		for(int p = 0; p < pS->m_lspPlanes.Len(); p++)
		{
			CSolid_Plane* pP = pS->m_lspPlanes[p];
			if (!pP->m_spMapping)
			{
				TPtr<CSolid_MappingMAP> spM = MNew(CSolid_MappingMAP);
				spM->m_MapMapping.CreateUnit();
				pP->m_spMapping = spM;
			}

			CStr Key, Val;
			pS->GetPlaneKey(p, Key, Val);
			spBrushNode->GetKeys()->AddKey(Key, Val);
		}							
	}

	XMP.WriteToScript(_FileName);
}

// -------------------------------------------------------------------
void CSolid::OptimizeMemory()
{
	MAUTOSTRIP(CSolid_OptimizeMemory, MAUTOSTRIP_VOID);
	m_lspPlanes.OptimizeMemory();
	m_lVerticesD.OptimizeMemory();
	m_lEdges.OptimizeMemory();
	m_lFaces.OptimizeMemory();
}

int CSolid::GetMemorySize()
{
	MAUTOSTRIP(CSolid_GetMemorySize, 0);
	int FacesSize = 0;
	int nF = GetNumFaces();
	for(int f = 0; f < nF; f++)
		FacesSize += GetFaceNonConst(f).GetMemorySize();

	int PlanesSize = 0;
	for(int p = 0; p < m_lspPlanes.Len(); p++)
		PlanesSize += m_lspPlanes[p]->GetMemorySize();

	return
		sizeof(*this) +
		m_lspPlanes.ListSize() + 
		m_lVerticesD.ListSize() + 
		m_lEdges.ListSize() + 
		m_lFaces.ListSize() + 
		FacesSize +
		PlanesSize;
}

int CSolid::GetMemoryUse()
{
	MAUTOSTRIP(CSolid_GetMemoryUse, 0);
	return 0;
}


// -------------------------------------------------------------------
//  CSolidNode
// -------------------------------------------------------------------
CSolidNode::CSolidNode()
{
	MAUTOSTRIP(CSolidNode_ctor, MAUTOSTRIP_VOID);
	m_TreeOper = CSOLID_OPERATOR_OR;
}

void CSolidNode::PurgeRedundantNodes_r()
{
	MAUTOSTRIP(CSolidNode_PurgeRedundantNodes_r, MAUTOSTRIP_VOID);
	for(int iCh = m_lspChildren.Len()-1; iCh >= 0; iCh--)
	{
		m_lspChildren[iCh]->PurgeRedundantNodes_r();
		if (!m_lspChildren[iCh]->m_spSolid)
		{
			// The child does not have a solid, add all it's children to this node's children and zap the node.
			CSolidNode* pC = m_lspChildren[iCh];
			m_lspChildren.Add(&pC->m_lspChildren);
			m_lspChildren.Del(iCh);
			LogFile("Purge.");
		}
	}
}

spCSolid CSolidNode::GetAll_r()
{
	MAUTOSTRIP(CSolidNode_GetAll_r, NULL);

	if (m_TreeOper == -1)
	{
		spCSolid spS;
		CSolid* pS = m_spTreeResult;
		for(; pS; pS = pS->m_spNext)
		{
			if (!spS)
				spS = pS->Duplicate();
			else
				pS->Duplicate()->LinkAfter(spS);
		}
		
		return spS;
	}


	spCSolid spS;
	if (m_spSolid) spS = m_spSolid->Duplicate();
//if(spS) LogFile("solid");
	spCSolid spTail = spS;
	int nCh = 0;
	for(int iCh = 0; iCh < m_lspChildren.Len(); iCh++)
	{
		spCSolid spSC = m_lspChildren[iCh]->GetAll_r();
		if (spSC)
		{
			nCh += spSC->GetChainLen();
			if (spS)
			{
//int nC = spSC->GetChainLen();
//int nTot = spS->GetChainLen();
				spSC->LinkChainAfter(spTail);
//int nAfter = spS->GetChainLen();
//LogFile(CStrF("%d + %d = %d (%d)", nC, nTot, nC+nTot, nAfter));
			}
			else
			{
				spS = spSC;
				spTail = spS;
			}
		}

		if(spTail) while(spTail->m_spNext) spTail = spTail->m_spNext;
	}
//	LogFile(CStrF("   total %d, children %d", (spS) ? spS->GetChainLen() : 0, nCh));

	return spS;
}

int CSolidNode::GetSolidCount_r()
{
	MAUTOSTRIP(CSolidNode_GetSolidCount_r, 0);

	if (m_TreeOper == -1)
		return m_spTreeResult->GetChainLen();

	int nS = 0;
	if (m_spSolid) nS++;

	for(int iCh = 0; iCh < m_lspChildren.Len(); iCh++)
		nS += m_lspChildren[iCh]->GetSolidCount_r();

	return nS;
}

void CSolidNode::CSG_FreeTreeResult_r()
{
	MAUTOSTRIP(CSolidNode_CSG_FreeTreeResult_r, MAUTOSTRIP_VOID);

	if (m_TreeOper == -1)
		return;

	m_spTreeResult = NULL;
	for(int iCh = 0; iCh < m_lspChildren.Len(); m_lspChildren[iCh++]->CSG_FreeTreeResult_r())
	{
	}
}

void CSolidNode::CSG_Evaluate_r(int _FlagsAndAnyOf, int _FlagsAnd, int _FlagsEqual, bool _bNoCSGOr, int _ModelMask, int _MediumMask)
{
	MAUTOSTRIP(CSolidNode_CSG_Evaluate_r, MAUTOSTRIP_VOID);

	if (m_TreeOper == -1)
		return;

	m_spTreeResult = NULL;
	int iCh;
	for(iCh = 0; iCh < m_lspChildren.Len(); iCh++)
		m_lspChildren[iCh]->CSG_Evaluate_r(_FlagsAndAnyOf, _FlagsAnd, _FlagsEqual, _bNoCSGOr, _ModelMask, _MediumMask);

	for(iCh = 0; iCh < m_lspChildren.Len(); iCh++)
		if (m_lspChildren[iCh]->m_TreeOper == CSOLID_OPERATOR_OR)
		{
			if (m_lspChildren[iCh]->m_spTreeResult)
			{
				if (m_spTreeResult)
					m_lspChildren[iCh]->m_spTreeResult->LinkChainAfter(m_spTreeResult);
				else
					m_spTreeResult = m_lspChildren[iCh]->m_spTreeResult;
			}

			m_lspChildren[iCh]->CSG_FreeTreeResult_r();
		}

	for(iCh = 0; iCh < m_lspChildren.Len(); iCh++)
		if (m_lspChildren[iCh]->m_TreeOper == CSOLID_OPERATOR_ANDNOT)
		{
			if(m_lspChildren[iCh]->m_spTreeResult)
				m_spTreeResult = CSolid::CSG_AndNotChain(m_spTreeResult, m_lspChildren[iCh]->m_spTreeResult, true);
			m_lspChildren[iCh]->CSG_FreeTreeResult_r();
		}

	for(iCh = 0; iCh < m_lspChildren.Len(); iCh++)
		if (m_lspChildren[iCh]->m_TreeOper == CSOLID_OPERATOR_AND)
		{
			if(m_lspChildren[iCh]->m_spTreeResult)
				m_spTreeResult = CSolid::CSG_AndChain(m_spTreeResult, m_lspChildren[iCh]->m_spTreeResult);
			m_lspChildren[iCh]->CSG_FreeTreeResult_r();
		}

//	if (m_spSolid && ((m_spSolid->m_Flags & _FlagsAnd) == _FlagsEqual))
	if (m_spSolid)
		if ((m_spSolid->m_Medium.m_MediumFlags & _MediumMask) &&
			(m_spSolid->m_ModelMask & _ModelMask) &&
			(
				((m_spSolid->m_Flags & _FlagsAnd) == _FlagsEqual) ||
				(m_spSolid->m_Flags & _FlagsAndAnyOf)
			))
			{
				if (m_spTreeResult)
					m_spSolid->Duplicate()->LinkAfter(m_spTreeResult);
				else
					m_spTreeResult = m_spSolid->Duplicate();
			}

//	m_TreeOper = (m_spSolid) ? m_spSolid->m_Operator : CSOLID_OPERATOR_OR;
}

void CSolidNode::CSG_GetMediumFromSurfaces_r()
{
	MAUTOSTRIP(CSolidNode_CSG_GetMediumFromSurfaces_r, MAUTOSTRIP_VOID);
	for(int iCh = 0; iCh < m_lspChildren.Len(); iCh++)
	{
		m_lspChildren[iCh]->CSG_GetMediumFromSurfaces_r();
	}
	if (m_spSolid) 
		m_spSolid->CSG_GetMediumFromSurfaces();
}

spCSolidNode CSolidNode::Duplicate()
{
	spCSolidNode spNode = MNew(CSolidNode);
	if(!spNode) MemError("CSolidNode::Duplicate");

	if(m_spSolid)
		spNode->m_spSolid	= m_spSolid->Duplicate();
	if(m_spTreeResult)
		spNode->m_spTreeResult	= m_spTreeResult->Duplicate();
	int nChildren = m_lspChildren.Len();
	spNode->m_lspChildren.SetLen(nChildren);
	for(int i = 0; i < nChildren; i++)
		spNode->m_lspChildren[i] = m_lspChildren[i]->Duplicate();
	spNode->m_TreeOper	= m_TreeOper;

	return spNode;
}

void CSolidNode::Apply_r(const CMat4Dfp32& _Mat, bool _bSnap, int _TextureLock)
{
	if(m_spSolid)
	{
		// Convert to Plane mapping (box mapping is broken when moving)
		if(_TextureLock & 2)
			m_spSolid->SetMappingType(-1, OGIER_MAPPINGTYPE_PLANEMAPPED);
		m_spSolid->Apply(_Mat, _bSnap, _TextureLock);
		m_spSolid->UpdateMesh();
	}

	for(int i = 0; i < m_lspChildren.Len(); i++)
		m_lspChildren[i]->Apply_r(_Mat, _bSnap, _TextureLock);
}
#endif
