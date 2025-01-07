#include "PCH.h"
#include "WObjCore.h"
#include "Server/WServer.h"
#include "WMapData.h"
#include "MFloat.h"
#include "WDynamicsEngine.h"
#include "WDynamicsEngine/WDynamicsEngine2.h"

#include "WObjects/WObj_PhysCluster.h"

#ifdef _DEBUG
	#include "MFloat.h"
	#define DEBUG_CHECK_ROW(v, r)\
		M_ASSERT(!FloatIsInvalid((v).k[r][0]) && \
		         !FloatIsInvalid((v).k[r][1]) && \
		         !FloatIsInvalid((v).k[r][2]) && \
		         !FloatIsInvalid((v).k[r][3]) && \
		         (M_Fabs((v).k[r][0]) + M_Fabs((v).k[r][1]) + M_Fabs((v).k[r][2]) + M_Fabs((v).k[r][3]) < 1000000.0f), "Invalid vector!");
	#define DEBUG_CHECK_MATRIX(m)\
		DEBUG_CHECK_ROW(m, 0)\
		DEBUG_CHECK_ROW(m, 1)\
		DEBUG_CHECK_ROW(m, 2)\
		DEBUG_CHECK_ROW(m, 3)
	#define DEBUG_CHECK_VECTOR(v) \
		M_ASSERT(!FloatIsInvalid((v).k[0]) && \
		         !FloatIsInvalid((v).k[1]) && \
		         !FloatIsInvalid((v).k[2]) && \
		         (M_Fabs((v).k[0]) + M_Fabs((v).k[1]) + M_Fabs((v).k[2]) < 1000000.0f), "Invalid vector!");
	#define DEBUG_CHECK_AXISANGLE(q) \
		M_ASSERT(!FloatIsInvalid((q).m_Axis.k[0]) && \
		         !FloatIsInvalid((q).m_Axis.k[1]) && \
		         !FloatIsInvalid((q).m_Axis.k[2]) && \
		         !FloatIsInvalid((q).m_Angle) && \
		         (M_Fabs((q).m_Axis.k[0]) + M_Fabs((q).m_Axis.k[0]) + M_Fabs((q).m_Axis.k[0]) + M_Fabs((q).m_Angle) <= 1000000.0f), "Invalid axis-angle!");
#else
	#define DEBUG_CHECK_ROW(v, r)
	#define DEBUG_CHECK_MATRIX(m)
	#define DEBUG_CHECK_VECTOR(v)
	#define DEBUG_CHECK_AXISANGLE(q)
#endif

//#pragma optimize("",off)
//#pragma inline_depth(0)


// -------------------------------------------------------------------
int TranslateInt(const char* _pVal, const char** _pStrings);
int TranslateInt(const char* _pVal, const char** _pStrings)
{
	MAUTOSTRIP(TranslateInt, 0);
//	int Flags = 0;
	CStr Val(_pVal);
	while(Val != "")
	{
		CStr s = Val.GetStrMSep(" ,+");
		s.Trim();
		for(int i = 0; _pStrings[i]; i++)
			if (s.CompareNoCase(_pStrings[i]) == 0)
				return i;
	}
	return -1;
}

int TranslateFlags(const char* _pVal, const char** _pStrings);
int TranslateFlags(const char* _pVal, const char** _pStrings)
{
	MAUTOSTRIP(TranslateFlags, 0);
	int Flags = 0;
	CStr Val(_pVal);
	while(Val != "")
	{
		CStr s = Val.GetStrMSep(" ,+");
		s.Trim();
		for(int i = 0; _pStrings[i]; i++)
			if (s.CompareNoCase(_pStrings[i]) == 0)
				Flags |= (1 << i);
	}
	return Flags;
}

// -------------------------------------------------------------------

CWO_PhysicsPrim::CWO_PhysicsPrim(int _PrimType, int _iPhysModel, const CVec3Dfp32& _Dim, const CVec3Dfp32& _Offset, uint _PhysModelMask, uint _ObjectFlagsMask)
{
	MAUTOSTRIP(CWO_PhysicsPrim_ctor, MAUTOSTRIP_VOID);
	m_PrimType = _PrimType;
	m_iPhysModel = _iPhysModel;
	m_PhysModelMask = _PhysModelMask;
	m_ObjectFlagsMask = _ObjectFlagsMask;
#ifndef M_RTM
	if (_Dim[0] > CWO_PHYSICSPRIM_MAXDIM_XY || _Dim[1] > CWO_PHYSICSPRIM_MAXDIM_XY ||_Dim[2] > CWO_PHYSICSPRIM_MAXDIM_Z ||
		Abs(_Offset[0]) > CWO_PHYSICSPRIM_MAXOFS || Abs(_Offset[1]) > CWO_PHYSICSPRIM_MAXOFS || Abs(_Offset[2]) > CWO_PHYSICSPRIM_MAXOFS)
		ConOut(CStrF("§cf80WARNING (CWO_PhysicsPrim::-): Invalid dim/offset %s, %s", _Dim.GetString().Str(), _Offset.GetString().Str()));
#endif
	SetDim(_Dim);
	SetOffset(_Offset);
//	m_Dimensions = _Dim;
//	m_Offset = _Offset;
//	m_Mass = _Mass;
}

void CWO_PhysicsPrim::Create(int _PrimType, int _iPhysModel, const CVec3Dfp32& _Dim, const CVec3Dfp32& _Offset, uint _PhysModelMask, uint _ObjectFlagsMask)
{
	MAUTOSTRIP(CWO_PhysicsPrim_Create, MAUTOSTRIP_VOID);
	m_PrimType = _PrimType;
	m_iPhysModel = _iPhysModel;
	m_PhysModelMask = _PhysModelMask;
	m_ObjectFlagsMask = _ObjectFlagsMask;
#ifndef M_RTM
	if (_Dim[0] > CWO_PHYSICSPRIM_MAXDIM_XY || _Dim[1] > CWO_PHYSICSPRIM_MAXDIM_XY ||_Dim[2] > CWO_PHYSICSPRIM_MAXDIM_Z ||
		Abs(_Offset[0]) > CWO_PHYSICSPRIM_MAXOFS || Abs(_Offset[1]) > CWO_PHYSICSPRIM_MAXOFS || Abs(_Offset[2]) > CWO_PHYSICSPRIM_MAXOFS)
		ConOut(CStrF("§cf80WARNING (CWO_PhysicsPrim::-): Invalid dim/offset %s, %s", _Dim.GetString().Str(), _Offset.GetString().Str()));
#endif
	SetDim(_Dim);
	SetOffset(_Offset);
//	m_Dimensions = _Dim;
//	m_Offset = _Offset;
//	m_Mass = _Mass;
}

void CWO_PhysicsPrim::OnLoad(CCFile* _pFile, CMapData* _pWData)
{
	MAUTOSTRIP(CWO_PhysicsPrim_OnLoad, MAUTOSTRIP_VOID);
	CFStr PhysModel;
	PhysModel.Read(_pFile);
	int iPhysModel = _pWData->GetResourceIndex(PhysModel);
	if (iPhysModel > CWO_PHYSICSPRIM_MAXRESOURCEINDEX)
		Error_static("OnLoad", CStrF("Invalid resource index. (%d)", iPhysModel));
	m_iPhysModel = iPhysModel;

	uint8 PrimType;
	_pFile->ReadLE(PrimType);
	m_PrimType = PrimType;

	_pFile->ReadLE(m_PhysModelMask);
	_pFile->ReadLE(m_ObjectFlagsMask);

	uint16 Dim[3];
	_pFile->ReadLE(Dim, 3);
	m_DimX = Dim[0];
	m_DimY = Dim[1];
	m_DimZ = Dim[2];

	_pFile->ReadLE(&m_Offset[0].value, 3);
}

void CWO_PhysicsPrim::OnSave(CCFile* _pFile, CMapData* _pWData) const
{
	MAUTOSTRIP(CWO_PhysicsPrim_OnSave, MAUTOSTRIP_VOID);
	CStr PhysModel = _pWData->GetResourceName(m_iPhysModel);
	PhysModel.Write(_pFile);
	uint8 PrimType = m_PrimType;
	_pFile->WriteLE(PrimType);
	_pFile->WriteLE(m_PhysModelMask);
	_pFile->WriteLE(m_ObjectFlagsMask);

	uint16 Dim[3] = { m_DimX, m_DimY, m_DimZ };
	_pFile->WriteLE(Dim, 3);
	_pFile->WriteLE(&m_Offset[0].value, 3);
}

void CWO_PhysicsPrim::Read(CCFile* _pFile)
{
	MAUTOSTRIP(CWO_PhysicsPrim_Read, MAUTOSTRIP_VOID);
	uint16 iPhysModel;
	uint8 PrimType;
	_pFile->ReadLE(iPhysModel);
	_pFile->ReadLE(PrimType);
	m_iPhysModel = iPhysModel;
	m_PrimType = PrimType;
	_pFile->ReadLE(m_PhysModelMask);
	_pFile->ReadLE(m_ObjectFlagsMask);

	uint16 Dim[3];
	_pFile->ReadLE(Dim, 3);
	m_DimX = Dim[0];
	m_DimY = Dim[1];
	m_DimZ = Dim[2];

	_pFile->ReadLE(&m_Offset[0].value, 3);
}

void CWO_PhysicsPrim::Write(CCFile* _pFile) const
{
	MAUTOSTRIP(CWO_PhysicsPrim_Write, MAUTOSTRIP_VOID);
	uint16 iPhysModel = m_iPhysModel;
	uint8 PrimType = m_PrimType;
	_pFile->WriteLE(iPhysModel);
	_pFile->WriteLE(PrimType);
	_pFile->WriteLE(m_PhysModelMask);
	_pFile->WriteLE(m_ObjectFlagsMask);

	uint16 Dim[3] = { m_DimX, m_DimY, m_DimZ };
	_pFile->WriteLE(Dim, 3);
	_pFile->WriteLE(&m_Offset[0].value, 3);
}

int CWO_PhysicsPrim::Compare(const CWO_PhysicsPrim& _Prim) const
{
	MAUTOSTRIP(CWO_PhysicsPrim_Compare, 0);
	if (m_PrimType != _Prim.m_PrimType) return 1;
	if (m_iPhysModel != _Prim.m_iPhysModel) return 1;
	if (m_PhysModelMask != _Prim.m_PhysModelMask) return 1;
	if (m_DimX != _Prim.m_DimX) return 1;
	if (m_DimY != _Prim.m_DimY) return 1;
	if (m_DimZ != _Prim.m_DimZ) return 1;
	if (m_Offset[0].value != _Prim.m_Offset[0].value) return 1;
	if (m_Offset[1].value != _Prim.m_Offset[1].value) return 1;
	if (m_Offset[2].value != _Prim.m_Offset[2].value) return 1;
	return 0;
}

uint8* CWO_PhysicsPrim::Write(uint8* _pData) const
{
	MAUTOSTRIP(CWO_PhysicsPrim_Write_2, NULL);
	PTR_PUTUINT8(_pData, m_PrimType);
	PTR_PUTUINT16(_pData, m_iPhysModel);
	PTR_PUTUINT16(_pData, m_PhysModelMask);
	PTR_PUTUINT16(_pData, m_ObjectFlagsMask);
	PTR_PUTUINT16(_pData, m_DimX);
	PTR_PUTUINT16(_pData, m_DimY);
	PTR_PUTUINT16(_pData, m_DimZ);
	PTR_PUTINT16(_pData, m_Offset[0].value);
	PTR_PUTINT16(_pData, m_Offset[1].value);
	PTR_PUTINT16(_pData, m_Offset[2].value);
	return _pData;
}

const uint8* CWO_PhysicsPrim::Read(const uint8* _pData)
{
	MAUTOSTRIP(CWO_PhysicsPrim_Read_2, NULL);
	PTR_GETUINT8(_pData, m_PrimType);
	PTR_GETUINT16(_pData, m_iPhysModel);
	PTR_GETUINT16(_pData, m_PhysModelMask);
	PTR_GETUINT16(_pData, m_ObjectFlagsMask);
	PTR_GETUINT16(_pData, m_DimX);
	PTR_GETUINT16(_pData, m_DimY);
	PTR_GETUINT16(_pData, m_DimZ);
	PTR_GETINT16(_pData, m_Offset[0].value);
	PTR_GETINT16(_pData, m_Offset[1].value);
	PTR_GETINT16(_pData, m_Offset[2].value);
	return _pData;
}

CStr CWO_PhysicsPrim::Dump(int _DumpFlags) const
{
	MAUTOSTRIP(CWO_PhysicsPrim_Dump, CStr());
	return CStrF("PT %d, iModel %d, PMask %.4x, OMask %.4x, Dim %d,%d,%d, Ofs (%.3f,%.3f,%.3f)", 
		m_PrimType, m_iPhysModel, m_PhysModelMask, m_ObjectFlagsMask,  
		m_DimX, m_DimY, m_DimZ, 
		fp32(m_Offset[0]), fp32(m_Offset[1]), fp32(m_Offset[2]));
}

// -------------------------------------------------------------------
void CWO_PhysicsState::operator= (const CWO_PhysicsState& _Phys)
{
	MAUTOSTRIP(CWO_PhysicsState_operator_assign, MAUTOSTRIP_VOID);
	m_nPrim = _Phys.m_nPrim;
	for(int i = 0; i < m_nPrim; i++)
		m_Prim[i] = _Phys.m_Prim[i];

	m_NavGridFillValue = _Phys.m_NavGridFillValue;
	m_PhysFlags = _Phys.m_PhysFlags;
	m_MediumFlags = _Phys.m_MediumFlags;
	m_iExclude = _Phys.m_iExclude;
	m_ObjectFlags = _Phys.m_ObjectFlags;
	m_ObjectIntersectFlags = _Phys.m_ObjectIntersectFlags;
	m_MoveSubdivisionSize = _Phys.m_MoveSubdivisionSize;

	m_pModelInstance = _Phys.m_pModelInstance;
}

CWO_PhysicsState::CWO_PhysicsState(const CWO_PhysicsState& _Phys)
{
	MAUTOSTRIP(CWO_PhysicsState_ctor, MAUTOSTRIP_VOID);
	*this = _Phys;
}

void CWO_PhysicsState::Clear()
{
	MAUTOSTRIP(CWO_PhysicsState_Clear, MAUTOSTRIP_VOID);
//	m_Prim.Create(OBJECT_PRIMTYPE_NONE, -1, 0, 0);
	m_nPrim = 0;

//	m_PrimType = OBJECT_PRIMTYPE_NONE;
	m_NavGridFillValue = 0;
	m_PhysFlags = 0;
//	m_iPhysModel = -1;
	m_MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS/* | XW_MEDIUM_DUALGLASS*/;
	m_iExclude = ~0;
	m_ObjectFlags = OBJECT_FLAGS_STATIC;
	m_ObjectIntersectFlags = 0;
//	m_Dimensions = 0;
//	m_Offset = 0;
	m_MoveSubdivisionSize = GetMoveSubdivisionSize();
	m_RigidBodyID = 0xFFFF; // Set dummy value for BoxEnumCheck to pass
	m_pModelInstance = NULL;
}

CWO_PhysicsState::CWO_PhysicsState()
{
	MAUTOSTRIP(CWO_PhysicsState_ctor_2, MAUTOSTRIP_VOID);
	Clear();
}

CWO_PhysicsState::CWO_PhysicsState(int _PrimType, int _PhysFlags, int _MediumFlags, const CVec3Dfp32& _Dimensions, int _ObjFlags, int _ObjIntersectFlags, int _iPhysModel)
{
	MAUTOSTRIP(CWO_PhysicsState_ctor_3, MAUTOSTRIP_VOID);
	m_Prim[0].Create(_PrimType, _iPhysModel, _Dimensions, 0, 1);
	m_nPrim	= 1;

//	m_PrimType = _PrimType;
	m_NavGridFillValue = 0;
	m_PhysFlags = _PhysFlags;
//	m_iPhysModel = _iPhysModel;
	m_MediumFlags = _MediumFlags;
//	m_Dimensions = _Dimensions;
	m_ObjectFlags = _ObjFlags;
	m_ObjectIntersectFlags = _ObjIntersectFlags;
	m_iExclude = ~0;
//	m_Offset = 0;
	m_MoveSubdivisionSize = GetMoveSubdivisionSize();

	m_pModelInstance = NULL;
}

void CWO_PhysicsState::AddPhysicsPrim(int _iPrim, const CWO_PhysicsPrim& _Prim)
{
	MAUTOSTRIP(CWO_PhysicsState_AddPhysicsPrim, MAUTOSTRIP_VOID);
	if (m_nPrim >= CWO_MAXPHYSPRIM) Error_static("CWO_PhysicsState::AddPhysicsPrim", "Too many primitives.");
	m_Prim[m_nPrim++] = _Prim;
	m_MoveSubdivisionSize = GetMoveSubdivisionSize();
}

fp32 CWO_PhysicsState::GetMoveSubdivisionSize() const
{
	MAUTOSTRIP(CWO_PhysicsState_GetMoveSubdivisionSize, 0.0f);
	fp32 SubDiv = 32.0f;
	for(int i = 0; i < m_nPrim; i++)
		switch(m_Prim[i].m_PrimType)
		{
		case OBJECT_PRIMTYPE_SPHERE :
			SubDiv = Min(SubDiv, Max(1.0f, m_Prim[i].GetRadius()));
			break;
//	case OBJECT_PRIMTYPE_CYLINDER_Z:
//		SubDiv = Min(SubDiv, Max(1.0f, fp32(Min(m_Prim[i].m_Dimensions[0], m_Prim[i].m_Dimensions[1]))));
//		break;
		case OBJECT_PRIMTYPE_BOX :
			SubDiv = Min(SubDiv, Max(1.0f, fp32(Min3(m_Prim[i].m_DimX, m_Prim[i].m_DimY, m_Prim[i].m_DimZ))));
			break;

		case OBJECT_PRIMTYPE_POINT :
			SubDiv = 10000.0;
			break;

		default :
			SubDiv = Min(SubDiv, 8.0f);
		}
	return SubDiv;
}

void CWO_PhysicsState::OnLoad(CCFile* _pFile, CMapData* _pWData)
{
	MAUTOSTRIP(CWO_PhysicsState_OnLoad, MAUTOSTRIP_VOID);
	_pFile->ReadLE(m_nPrim);
	for(int i = 0; i < m_nPrim; i++) m_Prim[i].OnLoad(_pFile, _pWData);

	_pFile->ReadLE(m_NavGridFillValue);
	_pFile->ReadLE(m_PhysFlags);
	_pFile->ReadLE(m_MediumFlags);
	_pFile->ReadLE(m_iExclude);
	_pFile->ReadLE(m_ObjectFlags);
	_pFile->ReadLE(m_ObjectIntersectFlags);

	_pFile->ReadLE(m_MoveSubdivisionSize);
}

void CWO_PhysicsState::OnSave(CCFile* _pFile, CMapData* _pWData) const
{
	MAUTOSTRIP(CWO_PhysicsState_OnSave, MAUTOSTRIP_VOID);
	_pFile->WriteLE(m_nPrim);
	for(int i = 0; i < m_nPrim; i++) m_Prim[i].OnSave(_pFile, _pWData);

	_pFile->WriteLE(m_NavGridFillValue);
	_pFile->WriteLE(m_PhysFlags);
	_pFile->WriteLE(m_MediumFlags);
	_pFile->WriteLE(m_iExclude);
	_pFile->WriteLE(m_ObjectFlags);
	_pFile->WriteLE(m_ObjectIntersectFlags);

	_pFile->WriteLE(m_MoveSubdivisionSize);
}

void CWO_PhysicsState::Read(CCFile* _pFile)
{
	MAUTOSTRIP(CWO_PhysicsState_Read, MAUTOSTRIP_VOID);
	_pFile->ReadLE(m_nPrim);
	for(int i = 0; i < m_nPrim; i++) m_Prim[i].Read(_pFile);

	_pFile->ReadLE(m_NavGridFillValue);
	_pFile->ReadLE(m_PhysFlags);
	_pFile->ReadLE(m_MediumFlags);
	_pFile->ReadLE(m_iExclude);
	_pFile->ReadLE(m_ObjectFlags);
	_pFile->ReadLE(m_ObjectIntersectFlags);

	_pFile->ReadLE(m_MoveSubdivisionSize);
}

void CWO_PhysicsState::Write(CCFile* _pFile) const
{
	MAUTOSTRIP(CWO_PhysicsState_Write, MAUTOSTRIP_VOID);
	_pFile->WriteLE(m_nPrim);
	for(int i = 0; i < m_nPrim; i++) m_Prim[i].Write(_pFile);

	_pFile->WriteLE(m_NavGridFillValue);
	_pFile->WriteLE(m_PhysFlags);
	_pFile->WriteLE(m_MediumFlags);
	_pFile->WriteLE(m_iExclude);
	_pFile->WriteLE(m_ObjectFlags);
	_pFile->WriteLE(m_ObjectIntersectFlags);

	_pFile->WriteLE(m_MoveSubdivisionSize);
}

int CWO_PhysicsState::CompareCoreData(const CWO_PhysicsState& _Prim) const
{
	MAUTOSTRIP(CWO_PhysicsState_CompareCoreData, 0);
	if (m_nPrim != _Prim.m_nPrim) return 1;
	if (m_NavGridFillValue != _Prim.m_NavGridFillValue) return 1;
	if (m_PhysFlags != _Prim.m_PhysFlags) return 1;
	if (m_MediumFlags != _Prim.m_MediumFlags) return 1;
	if (m_iExclude != _Prim.m_iExclude) return 1;
	if (m_ObjectFlags != _Prim.m_ObjectFlags) return 1;
	if (m_ObjectIntersectFlags != _Prim.m_ObjectIntersectFlags) return 1;
	return 0;
}

bool CWO_PhysicsState::CanIntersect(const CWO_PhysicsState& _State) const
{
	if ((m_ObjectIntersectFlags & _State.m_ObjectFlags) ||
		(m_ObjectFlags & _State.m_ObjectIntersectFlags))
		return true;
	else
		return false;
}

uint8* CWO_PhysicsState::Write(uint8* _pData) const
{
	MAUTOSTRIP(CWO_PhysicsState_Write_2, NULL);
	PTR_PUTUINT8(_pData, m_nPrim);
	PTR_PUTUINT8(_pData, m_NavGridFillValue);
	PTR_PUTUINT16(_pData, m_PhysFlags);
	PTR_PUTUINT16(_pData, m_MediumFlags);
	PTR_PUTUINT16(_pData, m_iExclude);
	PTR_PUTUINT16(_pData, m_ObjectFlags);
	PTR_PUTUINT16(_pData, m_ObjectIntersectFlags);
	return _pData;
}

const uint8* CWO_PhysicsState::Read(const uint8* _pData)
{
	MAUTOSTRIP(CWO_PhysicsState_Read_2, NULL);
	PTR_GETUINT8(_pData, m_nPrim);
	PTR_GETUINT8(_pData, m_NavGridFillValue);
	PTR_GETUINT16(_pData, m_PhysFlags);
	PTR_GETUINT16(_pData, m_MediumFlags);
	PTR_GETUINT16(_pData, m_iExclude);
	PTR_GETUINT16(_pData, m_ObjectFlags);
	PTR_GETUINT16(_pData, m_ObjectIntersectFlags);
	return _pData;
}

CStr CWO_PhysicsState::Dump(int _DumpFlags) const
{
	MAUTOSTRIP(CWO_PhysicsState_Dump, CStr());
	CStr s = CStrF("Flg %.4x, NF %.2x, MFlg %.8x, Excl %d, ObjFlg %.4x, %.4x, MSD %f", 
		m_PhysFlags, m_NavGridFillValue, m_MediumFlags, m_iExclude, m_ObjectFlags, m_ObjectIntersectFlags, m_MoveSubdivisionSize);

	for(int i = 0; i < m_nPrim; i++)
		s += ", PRIM: " + m_Prim[i].Dump(_DumpFlags);

	return s;
}

// -------------------------------------------------------------------
//  CWO_PhysicsAttrib
// -------------------------------------------------------------------
void CWO_PhysicsAttrib::Clear()
{
	MAUTOSTRIP(CWO_PhysicsAttrib_Clear, MAUTOSTRIP_VOID);
	m_Mass = 10.0f;
	m_Friction = 0.4f;
	m_Elasticy = 0.0f;
	m_StepSize = 0.0f;
}

CWO_PhysicsAttrib::CWO_PhysicsAttrib()
{
	MAUTOSTRIP(CWO_PhysicsAttrib_ctor, MAUTOSTRIP_VOID);
	Clear();
}

void CWO_PhysicsAttrib::Read(CCFile* _pFile)
{
	MAUTOSTRIP(CWO_PhysicsAttrib_Read, MAUTOSTRIP_VOID);
//	_pFile->ReadLE(m_Mass);
	_pFile->ReadLE(m_Friction);
	_pFile->ReadLE(m_Elasticy);
	_pFile->ReadLE(m_StepSize);
}

void CWO_PhysicsAttrib::Write(CCFile* _pFile) const
{
	MAUTOSTRIP(CWO_PhysicsAttrib_Write, MAUTOSTRIP_VOID);
//	_pFile->WriteLE(m_Mass);
	_pFile->WriteLE(m_Friction);
	_pFile->WriteLE(m_Elasticy);
	_pFile->WriteLE(m_StepSize);
}

int CWO_PhysicsAttrib::Compare(const CWO_PhysicsAttrib& _Attr) const
{
	MAUTOSTRIP(CWO_PhysicsAttrib_Compare, 0);
	if (m_Friction != _Attr.m_Friction) return 1;
	if (m_Elasticy != _Attr.m_Elasticy) return 1;
	if (m_StepSize != _Attr.m_StepSize) return 1;
	return 0;
}

uint8* CWO_PhysicsAttrib::Write(uint8* _pData) const
{
	MAUTOSTRIP(CWO_PhysicsAttrib_Write_2, NULL);
	PTR_PUTFP32(_pData, m_Friction);
	PTR_PUTFP32(_pData, m_Elasticy);
	PTR_PUTFP32(_pData, m_StepSize);
	return _pData;
}

const uint8* CWO_PhysicsAttrib::Read(const uint8* _pData)
{
	MAUTOSTRIP(CWO_PhysicsAttrib_Read_2, NULL);
	PTR_GETFP32(_pData, m_Friction);
	PTR_GETFP32(_pData, m_Elasticy);
	PTR_GETFP32(_pData, m_StepSize);
	return _pData;
}

// -------------------------------------------------------------------
//  CWObject_Message
// -------------------------------------------------------------------
CWObject_Message::CWObject_Message()
{
	MAUTOSTRIP(CWObject_Message_ctor, MAUTOSTRIP_VOID);
	m_Msg = 0;
	m_Param0 = 0;
	m_Param1 = 0;
	m_Reason = 0; // remove
	m_iSender = -1;
	m_VecParam0 = 0; // remove
	m_VecParam1 = 0; // remove
	m_pData = NULL;
	m_DataSize = 0; // remove (?)
}

CWObject_Message::CWObject_Message(int _Msg, aint _Param0, aint _Param1, int16 _iSender, int16 _Reason, const CVec3Dfp32& _VecParam0, const CVec3Dfp32& _VecParam1, void* _pData, int _DataSize)
{
	MAUTOSTRIP(CWObject_Message_ctor_2, MAUTOSTRIP_VOID);
	m_Msg = _Msg;
	m_Param0 = _Param0;
	m_Param1 = _Param1;
	m_iSender = _iSender;
	m_Reason = _Reason;
	m_VecParam0 = _VecParam0;
	m_VecParam1 = _VecParam1;
	m_pData = _pData;
	m_DataSize = _DataSize;
}

// -------------------------------------------------------------------
//  CWObject_CoreData
// -------------------------------------------------------------------
const char* CWObject_CoreData::ms_PrimTranslate[] = 
{
	"$$$", "MODEL", "SPHERE", "BOX", "POINT", (char*)NULL
};

const char* CWObject_CoreData::ms_ObjectFlagsTranslate[] = 
{
	"WORLD", "PHYSMODEL", "PHYSOBJECT", "PICKUP", "PLAYER", "CHARACTER", "PROJECTILE", "STATIC", "LIGHT", "SOUND", "TRIGGER", "PLAYERPHYSMODEL", "NAVIGATION", "WORLDTELEPORT", "ANIMPHYS", "OBJECT", (char*)NULL
};

const char* CWObject_CoreData::ms_ClientFlagsTranslate[] = 
{
	"INVISIBLE", "LINKINFINITE", "NOUPDATE", "NOREFRESH", "PREDICT", "HIGHPRECISION", "HASHDIRTY", "VELOCITY", (char*)NULL
};

const char* CWObject_CoreData::ms_PhysFlagsTranslate[] = 
{
	"ROTATION", "PUSHER", "PUSHABLE", "SLIDEABLE", "FRICTION", "PHYSMOVEMENT", "ROTFRICTION_SLIDE", "ROTFRICTION_ROT", "OFFSET", "ROTREFLECT", "ROTVELREFLECT", "APPLYROTVEL", "PHYSICSCONTROLLED", "MEDIUMQUERY", (char*)NULL
};

const char* CWObject_CoreData::ms_PhysExtensionsTranslate[] =
{
	"EXCLUDEOWNER", (char*)NULL
};

void CWObject_CoreData::operator= (const CWObject_CoreData& _Obj)
{
	MAUTOSTRIP(CWObject_CoreData_operator_assign, MAUTOSTRIP_VOID);
	DEBUG_CHECK_MATRIX(_Obj.m_Pos);
	DEBUG_CHECK_MATRIX(_Obj.m_LastPos);
	DEBUG_CHECK_MATRIX(_Obj.m_LocalPos);

	m_pRTC = _Obj.m_pRTC;
	m_PhysState = _Obj.m_PhysState;
	m_PhysAbsBoundBox = _Obj.m_PhysAbsBoundBox;
	m_PhysVelocity = _Obj.m_PhysVelocity;

	m_iObjectParent = _Obj.m_iObjectParent;
	m_iObjectChild = _Obj.m_iObjectChild;
	m_iObjectChildPrev = _Obj.m_iObjectChildPrev;
	m_iObjectChildNext = _Obj.m_iObjectChildNext;
	m_iParentAttach = _Obj.m_iParentAttach;

	m_VisBoxMin = _Obj.m_VisBoxMin;
	m_VisBoxMax = _Obj.m_VisBoxMax;
	m_LocalPos = _Obj.m_LocalPos;
	m_Pos = _Obj.m_Pos;
	m_LastPos = _Obj.m_LastPos;

	m_ClientFlags = _Obj.m_ClientFlags;
	m_iObject = _Obj.m_iObject;
	m_iClass = _Obj.m_iClass;
	m_CreationGameTick = _Obj.m_CreationGameTick;
	m_CreationGameTickFraction = _Obj.m_CreationGameTickFraction;
	m_iAnim0 = _Obj.m_iAnim0;
	m_iAnim1 = _Obj.m_iAnim1;
	m_iAnim2 = _Obj.m_iAnim2;

	memcpy(m_iModel, _Obj.m_iModel, sizeof(m_iModel));
	memcpy(m_iSound, _Obj.m_iSound, sizeof(m_iSound));
	memcpy(m_Data, _Obj.m_Data, sizeof(m_Data));
//	memcpy(m_PerClientData, _Obj.m_PerClientData, sizeof(m_PerClientData));
	
/*	*m_iModel = *_Obj.m_iModel;
	*m_iSound = *_Obj.m_iSound;
	*m_Data = *_Obj.m_Data;*/

	m_PhysAttrib = _Obj.m_PhysAttrib;

//	for(int i = 0; i < CWO_NUMCLIENTOBJ; i++)
//		m_lspClientObj[i] = NULL;
}

void CWObject_CoreData::Clear()
{
	MAUTOSTRIP(CWObject_CoreData_Clear, MAUTOSTRIP_VOID);
	m_VisBoxMin = 0;
	m_VisBoxMax = 0;
	m_PhysAbsBoundBox.m_Min = 0;
	m_PhysAbsBoundBox.m_Max = 0;

	m_ClientFlags = 0;
	m_iObject = 0;
	m_iClass = 0;
	FillChar(&m_iModel, sizeof(m_iModel), 0);
	m_CreationGameTick = 0;
	m_CreationGameTickFraction = 0;
	m_iAnim0 = 0;
	m_iAnim1 = 0;
	m_iAnim2 = 0;
	FillChar(&m_iSound, sizeof(m_iSound), 0);
	m_PhysVelocity.Unit();

	m_iObjectParent = 0;
	m_iObjectChild = 0;
	m_iObjectChildPrev = 0;
	m_iObjectChildNext = 0;
	m_iParentAttach = -1;

	m_LocalPos.Unit();
	m_Pos.Unit();
	m_LastPos.Unit();

	m_PhysState.Clear();
	m_PhysAttrib.Clear();

	FillChar(m_Data, sizeof(m_Data), 0);
//	FillChar(m_PerClientData, sizeof(m_PerClientData), 0);

	// Delete client-objects.
	{
		for(int i = 0; i < CWO_NUMCLIENTOBJ; i++)
			m_lspClientObj[i] = NULL;
	}

	/*
		Dynamics
	*/
	m_pRigidBody2 = NULL;
	m_pPhysCluster = NULL;
	m_liConnectedToConstraint.SetLen(0);
}

CWObject_CoreData::CWObject_CoreData()
{
	MAUTOSTRIP(CWObject_CoreData_ctor, MAUTOSTRIP_VOID);
	m_pRTC = NULL;
#ifndef M_RTM
	m_bIsInWOHash = 0;
#endif
	Clear();
}

void CWObject_CoreData::CleanPhysCluster()
{
	if( !m_pPhysCluster ) return;

	CWPhys_ClusterObject * pPO = m_pPhysCluster->m_lObjects.GetBasePtr();
	for(int i = 0;i < m_pPhysCluster->m_lObjects.Len();i++)
	{
		if( pPO[i].m_pRB )
			delete pPO[i].m_pRB;
	}
	delete m_pPhysCluster;
	m_pPhysCluster = NULL;
}

CWObject_CoreData::~CWObject_CoreData()
{
	/*if (m_pRigidBody) 
	{
		ConOutL(CStr("§cff0WARNING SOMEONE: Rigidbody exist in object core data. Where should this be removed!!"));
		delete m_pRigidBody;
		m_pRigidBody = NULL;
	}*/

	if (m_pRigidBody2)
	{
		M_TRACE("WARNING SOMEONE: Rigidbody exist in object core data. Where should this be removed!!\n");
		delete m_pRigidBody2;
		m_pRigidBody2 = NULL;
	}

	if (m_pPhysCluster)
	{
		M_TRACE("WARNING SOMEONE: Physcluster in coredata? find someplace else to do this. *Please*.\n");
		CleanPhysCluster();
	}

#ifndef M_RTM
	if (m_bIsInWOHash > 0)
	{
		ConOutL(CStrF("WARNING: Object %i core data is still in the hash during destruction!", m_iObject));
//		M_ASSERT(0, "!");
//		Error("~CWObject_CoreData", "The object core data is still in the hash");
	}
#endif
}

void CWObject_CoreData::GetVisBoundBox(CBox3Dfp32& _Box) const	
{
	MAUTOSTRIP(CWObject_CoreData_GetVisBoundBox, MAUTOSTRIP_VOID);
	_Box.m_Min[0] = m_VisBoxMin[0] * CWO_VISBOXSCALE;
	_Box.m_Min[1] = m_VisBoxMin[1] * CWO_VISBOXSCALE;
	_Box.m_Min[2] = m_VisBoxMin[2] * CWO_VISBOXSCALE;
	_Box.m_Max[0] = m_VisBoxMax[0] * CWO_VISBOXSCALE;
	_Box.m_Max[1] = m_VisBoxMax[1] * CWO_VISBOXSCALE;
	_Box.m_Max[2] = m_VisBoxMax[2] * CWO_VISBOXSCALE;
}

void CWObject_CoreData::SetVisBoundBox(const CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CWObject_CoreData_SetVisBoundBox, MAUTOSTRIP_VOID);
	m_VisBoxMin[0] = Max(-128, RoundToInt(_Box.m_Min[0] * CWO_VISBOXSCALERECP - 0.5f));
	m_VisBoxMin[1] = Max(-128, RoundToInt(_Box.m_Min[1] * CWO_VISBOXSCALERECP - 0.5f));
	m_VisBoxMin[2] = Max(-128, RoundToInt(_Box.m_Min[2] * CWO_VISBOXSCALERECP - 0.5f));
	m_VisBoxMax[0] = Min(127, RoundToInt(_Box.m_Max[0] * CWO_VISBOXSCALERECP + 0.5f));
	m_VisBoxMax[1] = Min(127, RoundToInt(_Box.m_Max[1] * CWO_VISBOXSCALERECP + 0.5f));
	m_VisBoxMax[2] = Min(127, RoundToInt(_Box.m_Max[2] * CWO_VISBOXSCALERECP + 0.5f));
}

void CWObject_CoreData::GetAbsVisBoundBox(CBox3Dfp32& _Box) const
{
	MAUTOSTRIP(CWObject_CoreData_GetAbsVisBoundBox, MAUTOSTRIP_VOID);
	if (m_ClientFlags & CWO_CLIENTFLAGS_AXISALIGNEDVISBOX)
	{
		GetVisBoundBox(_Box);
		_Box.m_Min += GetPosition();
		_Box.m_Max += GetPosition();
	}
	else
	{
		CBox3Dfp32 Tmp;
		GetVisBoundBox(Tmp);
		Tmp.Transform(GetPositionMatrix(), _Box);
	}
}

void CWObject_CoreData::GetVelocityMatrix(CMat4Dfp32& _Dest) const
{
	MAUTOSTRIP(CWObject_CoreData_GetVelocityMatrix, MAUTOSTRIP_VOID);
	if (M_Fabs(m_PhysVelocity.m_Rot.m_Angle) > 0.0001f)
		m_PhysVelocity.m_Rot.CreateMatrix3x3(_Dest);
	else
		_Dest.Unit3x3();

	m_PhysVelocity.m_Move.SetRow(_Dest, 3);

	_Dest.k[0][3] = 0;
	_Dest.k[1][3] = 0;
	_Dest.k[2][3] = 0;
	_Dest.k[3][3] = 1;
}

void CWObject_CoreData::GetVelocityMatrix(fp32 _Scale, CMat4Dfp32& _Dest) const
{
	MAUTOSTRIP(CWObject_CoreData_GetVelocityMatrix_2, MAUTOSTRIP_VOID);
	if (_Scale == 1.0f)
	{
		GetVelocityMatrix(_Dest);
	}
	else
	{
		CAxisRotfp32 Rot;
		Rot.m_Axis = m_PhysVelocity.m_Rot.m_Axis;
		Rot.m_Angle = m_PhysVelocity.m_Rot.m_Angle * _Scale;
		Rot.CreateMatrix3x3(_Dest);
		m_PhysVelocity.m_Move.Scale(_Scale, CVec3Dfp32::GetMatrixRow(_Dest, 3));
		_Dest.k[0][3] = 0;
		_Dest.k[1][3] = 0;
		_Dest.k[2][3] = 0;
		_Dest.k[3][3] = 1;
	}

	DEBUG_CHECK_MATRIX(_Dest);
}

void CWObject_CoreData::SetAnimTick(CWorld_Server* _pWServer, int32 _AnimTick, fp32 _TickFraction)
{
	MAUTOSTRIP(CWObject_CoreData_SetAnimTick, MAUTOSTRIP_VOID);
	int WholeFraction = TruncToInt(_TickFraction);
	m_CreationGameTick = _pWServer->GetGameTick() - (_AnimTick + WholeFraction);
	m_CreationGameTickFraction = _TickFraction - WholeFraction;
}

int32 CWObject_CoreData::GetAnimTick(CWorld_PhysState* _pWPhysState) const
{
	return (_pWPhysState->GetGameTick() - m_CreationGameTick);
}

fp32 CWObject_CoreData::GetAnimTickFraction() const
{
	return m_CreationGameTickFraction;
}



// -------------------------------------------------------------------
// URGENTFIXME:
//     Make all data-transfers endian-safe.

/*
	Object update, 2 bytes
		uint16
			bit 0..10:	iObject[0..2047]
			bit 11:		0 = Clear, 1 = Update
			bit 12:		Delta Pos & dPos update.
			bit 13:		Rotation update
			bit 14:		iAnim0 update
			bit 15:		Update bits 2 exist

	[Update bits 2], 1 byte
		uint8
			bit 0: Update AnimTime
			bit 1: Update iAnim1
			bit 2: Update iAnim2
			bit 3: Update iModel1
			bit 4: Update iClass
			bit 5: Update Pos & dPos absolute
			bit 6: Clear object first
			bit 7: Update bits 3 exist

	[Update bits 3], 1 byte
			bit 0: Update ClientFlags
			bit 1: Update iModel2
			bit 2: Update iModel3
			bit 3: Update AutoVar
			bit 4: Update iSound1
			bit 5: Update iSound2
			bit 6: Update Data0
			bit 7: Update bits 4 exist

	[Update bits 4], 1 byte
			bit 0: Update Data1
			bit 1: Update Data2
			bit 2: Update Data3
			bit 3: Update Data4
			bit 4: Update Data5
			bit 5: Update Data6
			bit 6: Update Data7
			bit 7: Update bits 5 exist

	[Update bits 5], 1 byte
			bit 0: Update Physstate
			bit 1: Update PhysPrim0
			bit 2: Update PhysPrim1
			bit 3: Update PhysPrim2
			bit 4: Update PhysPrim3
			bit 5: Update PhysAttr
			bit 6: Update Hierarchy
			bit 7: -

	[Delta Pos], 6 bytes
		LogDeltaVector, 4 bytes, 10bits per komp.
	[Rotation], 8 bytes
		Quaternion16
	[iAnim0], 2 bytes
		int16

	[AnimTime], 4 bytes
		fp32
	[iAnim1], 2 bytes
		int16
	[iAnim2], 2 bytes
		int16
	[iModel], 2 bytes
		int16
	[iClass], 2 bytes
		int16
	[Abs Pos], 12 bytes
		CVec3Dfp32

	[ClientFlags]
		int32, 4 bytes
	[iModel2]
		int16, 2 bytes
	[iModel3]
		int16, 2 bytes
	[iModel4]
		int16, 2 bytes
	[iSound1]
		int16, 2 bytes
	[iSound2]
		int16, 2 bytes
	[Data0]
		int32, 4 bytes
	
	[Data1]
		int32, 4 bytes
	[Data2]
		int32, 4 bytes
	[Data3]
		int32, 4 bytes
	[Data4]
		int32, 4 bytes
	[Data5]
		int32, 4 bytes
	[Data6]
		int32, 4 bytes
	[Data7]
		int32, 4 bytes

	To be continued...
*/

#define OBJUPD_NOBJAND			DBitRange(0, 11)		// max 4096 objects

#define OBJUPD_MAXDELTAX		126.0f
#define OBJUPD_MAXDELTAY		126.0f
#define OBJUPD_MAXDELTAZ		126.0f

#define OBJUPD_FLAGS0_UPDATE_ELSE_CLEAR	M_Bit(12)
#define OBJUPD_FLAGS0_DELTAPOS			M_Bit(13)
#define OBJUPD_FLAGS0_ROTATION			M_Bit(14)
#define OBJUPD_FLAGS0_FLAGS1			M_Bit(15)

#define OBJUPD_FLAGS1_HIGHPRECISION	M_Bit(0)
#define OBJUPD_FLAGS1_MOVEVELOCITY	M_Bit(1)
#define OBJUPD_FLAGS1_ROTVELOCITY	M_Bit(2)
#define OBJUPD_FLAGS1_ICLASS		M_Bit(3)
#define OBJUPD_FLAGS1_FLAGS2		M_Bit(7)

#define OBJUPD_FLAGS2_ANIMTIME		M_Bit(0)
#define OBJUPD_FLAGS2_IANIM0		M_Bit(1)
#define OBJUPD_FLAGS2_IANIM1		M_Bit(2)
#define OBJUPD_FLAGS2_IANIM2		M_Bit(3)
#define OBJUPD_FLAGS2_IMODEL1		M_Bit(4)
#define OBJUPD_FLAGS2_POS			M_Bit(5)
#define OBJUPD_FLAGS2_CLEAR			M_Bit(6)
#define OBJUPD_FLAGS2_FLAGS3		M_Bit(7)

#define OBJUPD_FLAGS3_CLFLAGS		M_Bit(0)
#define OBJUPD_FLAGS3_IMODEL23		M_Bit(1)
#define OBJUPD_FLAGS3_AUTOVAR		M_Bit(2)
#define OBJUPD_FLAGS3_EXTRADATA		M_Bit(3)
#define OBJUPD_FLAGS3_ISOUND1		M_Bit(4)
#define OBJUPD_FLAGS3_ISOUND2		M_Bit(5)
#define OBJUPD_FLAGS3_DATA0			M_Bit(6)
#define OBJUPD_FLAGS3_FLAGS4		M_Bit(7)

#define OBJUPD_FLAGS4_DATA1			M_Bit(0)
#define OBJUPD_FLAGS4_DATA2			M_Bit(1)
#define OBJUPD_FLAGS4_DATA3			M_Bit(2)
#define OBJUPD_FLAGS4_DATA4			M_Bit(3)
#define OBJUPD_FLAGS4_DATA5			M_Bit(4)
#define OBJUPD_FLAGS4_DATA6			M_Bit(5)
#define OBJUPD_FLAGS4_DATA7			M_Bit(6)
#define OBJUPD_FLAGS4_FLAGS5		M_Bit(7)

#define OBJUPD_FLAGS5_PHYSSTATE		M_Bit(0)
#define OBJUPD_FLAGS5_PHYSPRIM0		M_Bit(1)
#define OBJUPD_FLAGS5_PHYSPRIM1		M_Bit(2)
#define OBJUPD_FLAGS5_PHYSPRIM2		M_Bit(3)
#define OBJUPD_FLAGS5_VISBOX		M_Bit(4)
#define OBJUPD_FLAGS5_PHYSATTR		M_Bit(5)
#define OBJUPD_FLAGS5_HIERARCHY		M_Bit(6)

// Debug options:
//#define OBJUPD_DEBUG_ABSPOS
#define OBJUPD_DEBUG_FLOATPOS
#define OBJUPD_DEBUG_FLOATQUAT


int CWObject::OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const
{
	MAUTOSTRIP(CWObject_OnCreateClientUpdate, 0);
	int ClientFlags = _pClObjInfo->m_ClientFlags | m_ClientFlags;

#ifdef OBJUPD_DEBUG_FLOATPOS
	int bHighPrecision = 1;
#else
	int bHighPrecision = ClientFlags & CWO_CLIENTFLAGS_HIGHPRECISION;
#endif

//	int nData = 0;
	uint8* pD = _pData;
	
	uint16 Flags0 = m_iObject & OBJUPD_NOBJAND;
	uint8 Flags1 = 0;
	uint8 Flags2 = 0;
	uint8 Flags3 = 0;
	uint8 Flags4 = 0;
	uint8 Flags5 = 0;

	if (!_pOld->m_iClass)
	{
		if (!m_iClass) return 0;

		Flags2 |= OBJUPD_FLAGS2_CLEAR;
	}
	else if (_pOld->m_iClass != m_iClass)
	{
		Flags2 |= OBJUPD_FLAGS2_CLEAR;
	}

//	CVec3Dint16 BMin(m_BoxMin.k[0], m_BoxMin.k[1], m_BoxMin.k[2]);
//	CVec3Dint16 BMax(m_BoxMax.k[0], m_BoxMax.k[1], m_BoxMax.k[2]);
	if (!m_iClass)
	{
		Flags2 = 0;
	}
	else
	{
		Flags0 |= OBJUPD_FLAGS0_UPDATE_ELSE_CLEAR;

		const CMat4Dfp32& MNew_ = GetLocalPositionMatrix();
		const CMat4Dfp32& MOld = _pOld->GetLocalPositionMatrix();

		const CVec3Dfp32* pVNew = &CVec3Dfp32::GetMatrixRow(MNew_, 3);
		const CVec3Dfp32* pVOld = &CVec3Dfp32::GetMatrixRow(MOld, 3);

		if (memcmp(&MNew_, &MOld, sizeof(MNew_)) != 0)
		{
			{
				int i, j;
				for(i = 0; i < 3; i++)
				{
					for(j = 0; j < 3; j++)
						if (Abs(MNew_.k[i][j] - MOld.k[i][j]) > 0.0001f) break;
					if (j != 3) break;
				}
				if (!((i == 3) && (j == 3)))
				{
					if (bHighPrecision)
						Flags1 |= OBJUPD_FLAGS1_HIGHPRECISION;

					Flags0 |= OBJUPD_FLAGS0_ROTATION;

					if (ClientFlags & CWO_CLIENTFLAGS_ROTVELOCITY)
						Flags1 |= OBJUPD_FLAGS1_ROTVELOCITY;
				}

				for(j = 0; j < 3; j++)
					if (MNew_.k[3][j] != MOld.k[3][j]) break;

				if (j != 3)
				{
					if (bHighPrecision)
						Flags1 |= OBJUPD_FLAGS1_HIGHPRECISION;

					if (ClientFlags & CWO_CLIENTFLAGS_MOVEVELOCITY)
						Flags1 |= OBJUPD_FLAGS1_MOVEVELOCITY;

	#ifdef OBJUPD_DEBUG_ABSPOS
					Flags2 |= OBJUPD_FLAGS2_POS;
	#else
					if (bHighPrecision ||
						(Abs(pVNew->k[0] - pVOld->k[0]) > OBJUPD_MAXDELTAX) ||
						(Abs(pVNew->k[1] - pVOld->k[1]) > OBJUPD_MAXDELTAY) ||
						(Abs(pVNew->k[2] - pVOld->k[2]) > OBJUPD_MAXDELTAZ))
						Flags2 |= OBJUPD_FLAGS2_POS;
					else
						Flags0 |= OBJUPD_FLAGS0_DELTAPOS;
	#endif
				}
			}
		}

		if (m_iClass != _pOld->m_iClass) Flags1 |= OBJUPD_FLAGS1_ICLASS;

		if (m_CreationGameTick != _pOld->m_CreationGameTick) Flags2 |= OBJUPD_FLAGS2_ANIMTIME;
		if (m_CreationGameTickFraction != _pOld->m_CreationGameTickFraction) Flags2 |= OBJUPD_FLAGS2_ANIMTIME;
		if (m_iAnim0 != _pOld->m_iAnim0) Flags2 |= OBJUPD_FLAGS2_IANIM0;
		if (m_iAnim1 != _pOld->m_iAnim1) Flags2 |= OBJUPD_FLAGS2_IANIM1;
		if (m_iAnim2 != _pOld->m_iAnim2) Flags2 |= OBJUPD_FLAGS2_IANIM2;
		if (m_iModel[0] != _pOld->m_iModel[0]) Flags2 |= OBJUPD_FLAGS2_IMODEL1;

/*		CVec3Dint16 OBMin(_pOld->m_BoxMin.k[0], _pOld->m_BoxMin.k[1], _pOld->m_BoxMin.k[2]);
		CVec3Dint16 OBMax(_pOld->m_BoxMax.k[0], _pOld->m_BoxMax.k[1], _pOld->m_BoxMax.k[2]);

		if ((BMin != OBMin) || (BMax != OBMax)) Flags2 |= OBJUPD_FLAGS2_BOXMINMAX;*/

		// Flags3
		if (ClientFlags != _pOld->m_ClientFlags) Flags3 |= OBJUPD_FLAGS3_CLFLAGS;
		if (m_iModel[1] != _pOld->m_iModel[1]) Flags3 |= OBJUPD_FLAGS3_IMODEL23;
		if (m_iModel[2] != _pOld->m_iModel[2]) Flags3 |= OBJUPD_FLAGS3_IMODEL23;
//		if (m_iModel[3] != _pOld->m_iModel[3]) Flags3 |= OBJUPD_FLAGS3_IMODEL4;
		if(_Flags & CWO_CLIENTUPDATE_AUTOVAR) Flags3 |= OBJUPD_FLAGS3_AUTOVAR;
		if(_Flags & CWO_CLIENTUPDATE_EXTRADATA) Flags3 |= OBJUPD_FLAGS3_EXTRADATA;
		if (m_iSound[0] != _pOld->m_iSound[0]) Flags3 |= OBJUPD_FLAGS3_ISOUND1;
		if (m_iSound[1] != _pOld->m_iSound[1]) Flags3 |= OBJUPD_FLAGS3_ISOUND2;

		// Flags4
		if(m_Data[0] != _pOld->m_Data[0]) Flags3 |= OBJUPD_FLAGS3_DATA0;
		if(m_Data[1] != _pOld->m_Data[1]) Flags4 |= OBJUPD_FLAGS4_DATA1;
		if(m_Data[2] != _pOld->m_Data[2]) Flags4 |= OBJUPD_FLAGS4_DATA2;
		if(m_Data[3] != _pOld->m_Data[3]) Flags4 |= OBJUPD_FLAGS4_DATA3;
		if(m_Data[4] != _pOld->m_Data[4]) Flags4 |= OBJUPD_FLAGS4_DATA4;
		if(m_Data[5] != _pOld->m_Data[5]) Flags4 |= OBJUPD_FLAGS4_DATA5;
		if(m_Data[6] != _pOld->m_Data[6]) Flags4 |= OBJUPD_FLAGS4_DATA6;
		if(m_Data[7] != _pOld->m_Data[7]) Flags4 |= OBJUPD_FLAGS4_DATA7;

		const CWO_PhysicsState& PNew = GetPhysState();
		const CWO_PhysicsState& POld = _pOld->GetPhysState();

		if(PNew.CompareCoreData(POld))					Flags5 |= OBJUPD_FLAGS5_PHYSSTATE;
		if(PNew.m_Prim[0].Compare(POld.m_Prim[0]))		Flags5 |= OBJUPD_FLAGS5_PHYSPRIM0;
		if(PNew.m_Prim[1].Compare(POld.m_Prim[1]))		Flags5 |= OBJUPD_FLAGS5_PHYSPRIM1;
		if(PNew.m_Prim[2].Compare(POld.m_Prim[2]))		Flags5 |= OBJUPD_FLAGS5_PHYSPRIM2;
/*		if(PNew.m_Prim[3].Compare(POld.m_Prim[3]))		Flags5 |= OBJUPD_FLAGS5_PHYSPRIM3;*/

		const CVec3Dint8& VisBoxMin = GetVisBoundBox_RawMin();
		const CVec3Dint8& VisBoxMax = GetVisBoundBox_RawMax();
		const CVec3Dint8& VisBoxMinOld = _pOld->GetVisBoundBox_RawMin();
		const CVec3Dint8& VisBoxMaxOld = _pOld->GetVisBoundBox_RawMax();

		if (VisBoxMin[0] != VisBoxMinOld[0] ||
			VisBoxMin[1] != VisBoxMinOld[1] ||
			VisBoxMin[2] != VisBoxMinOld[2] ||
			VisBoxMax[0] != VisBoxMaxOld[0] ||
			VisBoxMax[1] != VisBoxMaxOld[1] ||
			VisBoxMax[2] != VisBoxMaxOld[2])
			Flags5 |= OBJUPD_FLAGS5_VISBOX;

		if(m_PhysAttrib.Compare(_pOld->m_PhysAttrib))	Flags5 |= OBJUPD_FLAGS5_PHYSATTR;

		if(GetParent() != _pOld->GetParent())			Flags5 |= OBJUPD_FLAGS5_HIERARCHY;
		else if(GetFirstChild() != _pOld->GetFirstChild())	Flags5 |= OBJUPD_FLAGS5_HIERARCHY;
		else if(GetPrevChild() != _pOld->GetPrevChild())	Flags5 |= OBJUPD_FLAGS5_HIERARCHY;
		else if(GetNextChild() != _pOld->GetNextChild())	Flags5 |= OBJUPD_FLAGS5_HIERARCHY;

	}
	if (Flags5) Flags4 |= OBJUPD_FLAGS4_FLAGS5;
	if (Flags4) Flags3 |= OBJUPD_FLAGS3_FLAGS4;
	if (Flags3) Flags2 |= OBJUPD_FLAGS2_FLAGS3;
	if (Flags2) Flags1 |= OBJUPD_FLAGS1_FLAGS2;
	if (Flags1) Flags0 |= OBJUPD_FLAGS0_FLAGS1;

	PTR_PUTINT16(pD, Flags0);
	if (Flags0 & OBJUPD_FLAGS0_FLAGS1)
		PTR_PUTINT8(pD, Flags1);
	if (Flags1 & OBJUPD_FLAGS1_FLAGS2)
		PTR_PUTINT8(pD, Flags2);
	if (Flags2 & OBJUPD_FLAGS2_FLAGS3)
		PTR_PUTINT8(pD, Flags3);
	if (Flags3 & OBJUPD_FLAGS3_FLAGS4)
		PTR_PUTINT8(pD, Flags4);
	if (Flags4 & OBJUPD_FLAGS4_FLAGS5)
		PTR_PUTINT8(pD, Flags5);

//LogFile(CStrF("     Upd iObj %d,  %.2x, %.2x, %.2x, %.2x, t %f, %f, (%.8x, %.8x, %.8x, %.8x, %.8x, %.8x, %.8x, %.8x)", iObj, Flags1, Flags2, Flags3, Flags4, 
//		m_AnimTime, _pOld->m_AnimTime,
//		m_Data[0], m_Data[1], m_Data[2], m_Data[3], m_Data[4], m_Data[5], m_Data[6], m_Data[7]));

	if (Flags1 & OBJUPD_FLAGS1_ICLASS)
		PTR_PUTINT16(pD, m_iClass);

	if (Flags0 & OBJUPD_FLAGS0_DELTAPOS)
	{
		const CMat4Dfp32& MNew_ = GetLocalPositionMatrix();
		const CMat4Dfp32& MOld = _pOld->GetLocalPositionMatrix();

		const CVec3Dfp32* pVNew = &CVec3Dfp32::GetMatrixRow(MNew_, 3);
		const CVec3Dfp32* pVOld = &CVec3Dfp32::GetMatrixRow(MOld, 3);

/*		if (bHighPrecision)
		{
			int16 dm[3];
			dm[0] = (pVNew->k[0] - pVOld->k[0]) * 256.0f;
			dm[1] = (pVNew->k[1] - pVOld->k[1]) * 256.0f;
			dm[2] = (pVNew->k[2] - pVOld->k[2]) * 256.0f;
			PTR_PUTINT16(pD, dm[0]);
			PTR_PUTINT16(pD, dm[1]);
			PTR_PUTINT16(pD, dm[2]);
		}
		else*/
		{
			int8 dm[3];
			dm[0] = RoundToInt(pVNew->k[0] - pVOld->k[0]);
			dm[1] = RoundToInt(pVNew->k[1] - pVOld->k[1]);
			dm[2] = RoundToInt(pVNew->k[2] - pVOld->k[2]);
			PTR_PUTINT8(pD, dm[0]);
			PTR_PUTINT8(pD, dm[1]);
			PTR_PUTINT8(pD, dm[2]);
		}
	}

	if (Flags0 & OBJUPD_FLAGS0_ROTATION)
	{
		if (bHighPrecision)
		{
			CQuatfp32 Q;
			Q.GetMatrix(m_LocalPos);
#ifdef OBJUPD_DEBUG_FLOATQUAT
			PTR_PUTFP32(pD, Q.k[0]);
			PTR_PUTFP32(pD, Q.k[1]);
			PTR_PUTFP32(pD, Q.k[2]);
			PTR_PUTFP32(pD, Q.k[3]);

#else
			uint16 qi[4];
			qi[0] = (Q.k[0] + 1.0f) * 0.5f * 65535.0f;
			qi[1] = (Q.k[1] + 1.0f) * 0.5f * 65535.0f;
			qi[2] = (Q.k[2] + 1.0f) * 0.5f * 65535.0f;
			qi[3] = (Q.k[3] + 1.0f) * 0.5f * 65535.0f;

			PTR_PUTINT16(pD, qi[0]);
			PTR_PUTINT16(pD, qi[1]);
			PTR_PUTINT16(pD, qi[2]);
			PTR_PUTINT16(pD, qi[3]);
#endif
		}
		else
		{
			CVec3Dfp32 v;
			v.CreateAnglesFromMatrix(0, m_LocalPos);
			uint8 vi[3];
			vi[0] = RoundToInt(v[0] * 256.0f);
			vi[1] = RoundToInt(v[1] * 256.0f);
			vi[2] = RoundToInt(v[2] * 256.0f);
			PTR_PUTINT8(pD, vi[0]);
			PTR_PUTINT8(pD, vi[1]);
			PTR_PUTINT8(pD, vi[2]);
		}
	}

	if (Flags1)
	{
		if (Flags1 & OBJUPD_FLAGS1_MOVEVELOCITY)
		{
			const CVec3Dfp32& Vel = m_PhysVelocity.m_Move;
			if (bHighPrecision)
			{
				PTR_PUTFP32(pD, Vel.k[0]);
				PTR_PUTFP32(pD, Vel.k[1]);
				PTR_PUTFP32(pD, Vel.k[2]);
			}
			else
			{
				CVec3Dint8 Pos_i = CVec3Dint8(int8(Vel[0]), int8(Vel[1]), int8(Vel[2]));
				PTR_PUTINT8(pD, Pos_i.k[0]);
				PTR_PUTINT8(pD, Pos_i.k[1]);
				PTR_PUTINT8(pD, Pos_i.k[2]);
			}
		}
		
		if (Flags1 & OBJUPD_FLAGS1_ROTVELOCITY)
		{
/*			if (bHighPrecision)
			{*/
				PTR_PUTFP32(pD, m_PhysVelocity.m_Rot.m_Axis.k[0]);
				PTR_PUTFP32(pD, m_PhysVelocity.m_Rot.m_Axis.k[1]);
				PTR_PUTFP32(pD, m_PhysVelocity.m_Rot.m_Axis.k[2]);
				PTR_PUTFP32(pD, m_PhysVelocity.m_Rot.m_Angle);
/*			}
			else
			{
				// Something else..
			}*/
		}
	}

	if (Flags2)
	{
		if (Flags2 & OBJUPD_FLAGS2_ANIMTIME)
		{
			PTR_PUTINT32(pD, m_CreationGameTick);
			PTR_PUTFP32(pD, m_CreationGameTickFraction);
		}
		if (Flags2 & OBJUPD_FLAGS2_IANIM0)
			PTR_PUTINT16(pD, m_iAnim0);
		if (Flags2 & OBJUPD_FLAGS2_IANIM1)
			PTR_PUTINT16(pD, m_iAnim1);
		if (Flags2 & OBJUPD_FLAGS2_IANIM2)
			PTR_PUTINT16(pD, m_iAnim2);
		if (Flags2 & OBJUPD_FLAGS2_IMODEL1)
			PTR_PUTINT16(pD, m_iModel[0]);

		if (Flags2 & OBJUPD_FLAGS2_POS)
		{
			if (bHighPrecision)
			{
				PTR_PUTFP32(pD, m_LocalPos.k[3][0]);
				PTR_PUTFP32(pD, m_LocalPos.k[3][1]);
				PTR_PUTFP32(pD, m_LocalPos.k[3][2]);
			}
			else
			{
				CVec3Dint16 Pos_i(int16(m_LocalPos.k[3][0]), int16(m_LocalPos.k[3][1]), int16(m_LocalPos.k[3][2]));
				PTR_PUTINT16(pD, Pos_i.k[0]);
				PTR_PUTINT16(pD, Pos_i.k[1]);
				PTR_PUTINT16(pD, Pos_i.k[2]);
			}
		}
/*		if (Flags2 & OBJUPD_FLAGS2_BOXMINMAX)
		{
			*(CVec3Dint16*) pD = BMin;	IncPtr(&pD, sizeof(CVec3Dint16));
			*(CVec3Dint16*) pD = BMax;	IncPtr(&pD, sizeof(CVec3Dint16));
		}*/
	}

	if (Flags3)
	{
		if (Flags3 & OBJUPD_FLAGS3_CLFLAGS)		PTR_PUTINT32(pD, ClientFlags);
		if (Flags3 & OBJUPD_FLAGS3_IMODEL23)	PTR_PUTINT16(pD, m_iModel[1]);
		if (Flags3 & OBJUPD_FLAGS3_IMODEL23)	PTR_PUTINT16(pD, m_iModel[2]);
//		if (Flags3 & OBJUPD_FLAGS3_IMODEL4)		{	*(int16*) pD = m_iModel[3];	IncPtr(&pD, sizeof(m_iModel[3])); }
		if (Flags3 & OBJUPD_FLAGS3_ISOUND1)		PTR_PUTINT16(pD, m_iSound[0]);
		if (Flags3 & OBJUPD_FLAGS3_ISOUND2)		PTR_PUTINT16(pD, m_iSound[1]);
		if (Flags3 & OBJUPD_FLAGS3_DATA0)		PTR_PUTINT32(pD, m_Data[0]);
	}

	if (Flags4)
	{
		if (Flags4 & OBJUPD_FLAGS4_DATA1)		PTR_PUTINT32(pD, m_Data[1]);
		if (Flags4 & OBJUPD_FLAGS4_DATA2)		PTR_PUTINT32(pD, m_Data[2]);
		if (Flags4 & OBJUPD_FLAGS4_DATA3)		PTR_PUTINT32(pD, m_Data[3]);
		if (Flags4 & OBJUPD_FLAGS4_DATA4)		PTR_PUTINT32(pD, m_Data[4]);
		if (Flags4 & OBJUPD_FLAGS4_DATA5)		PTR_PUTINT32(pD, m_Data[5]);
		if (Flags4 & OBJUPD_FLAGS4_DATA6)		PTR_PUTINT32(pD, m_Data[6]);
		if (Flags4 & OBJUPD_FLAGS4_DATA7)		PTR_PUTINT32(pD, m_Data[7]);
	}

	if (Flags5)
	{
		if (Flags5 & OBJUPD_FLAGS5_PHYSSTATE) pD = m_PhysState.Write(pD);
		if (Flags5 & OBJUPD_FLAGS5_PHYSPRIM0) pD = m_PhysState.m_Prim[0].Write(pD);
		if (Flags5 & OBJUPD_FLAGS5_PHYSPRIM1) pD = m_PhysState.m_Prim[1].Write(pD);
		if (Flags5 & OBJUPD_FLAGS5_PHYSPRIM2) pD = m_PhysState.m_Prim[2].Write(pD);
/*		if (Flags5 & OBJUPD_FLAGS5_PHYSPRIM3) pD = m_PhysState.m_Prim[3].Write(pD);*/
		if (Flags5 & OBJUPD_FLAGS5_VISBOX)
		{
			*(pD++) = m_VisBoxMin[0];
			*(pD++) = m_VisBoxMin[1];
			*(pD++) = m_VisBoxMin[2];
			*(pD++) = m_VisBoxMax[0];
			*(pD++) = m_VisBoxMax[1];
			*(pD++) = m_VisBoxMax[2];
		}
		if (Flags5 & OBJUPD_FLAGS5_PHYSATTR) pD = m_PhysAttrib.Write(pD);
		if (Flags5 & OBJUPD_FLAGS5_HIERARCHY)
		{
			PTR_PUTINT16(pD, m_iObjectParent);
			PTR_PUTINT16(pD, m_iObjectChild);
			PTR_PUTINT16(pD, m_iObjectChildNext);
			PTR_PUTINT16(pD, m_iObjectChildPrev);
			PTR_PUTINT8(pD, m_iParentAttach);
		}
	}

	// Check if nothing changed.
	if (!(Flags0 & ~(OBJUPD_NOBJAND | OBJUPD_FLAGS0_UPDATE_ELSE_CLEAR)))
		if (Flags0 & OBJUPD_FLAGS0_UPDATE_ELSE_CLEAR)
			return 0;


	return (uint8*)pD - _pData;
}

int CWObject_CoreData::GetUpdatedClass(int _iCurrentClass, const uint8* _pData)
{
	MAUTOSTRIP(CWObject_CoreData_GetUpdatedClass, 0);
	const uint8* pD = _pData;

	uint16 Flags0 = 0;
	PTR_GETINT16(pD, Flags0);

	if (Flags0 & OBJUPD_FLAGS0_UPDATE_ELSE_CLEAR)
	{
		uint8 Flags1 = 0;
		uint8 Flags2 = 0;
		uint8 Flags3 = 0;
		uint8 Flags4 = 0;
		uint8 Flags5 = 0;

		// Get Flags1?
		if (Flags0 & OBJUPD_FLAGS0_FLAGS1)
			PTR_GETINT8(pD, Flags1);

		// Class not changed?
		if (!(Flags1 & OBJUPD_FLAGS1_ICLASS))
			return _iCurrentClass;

		// Get Flags2?
		if (Flags1 & OBJUPD_FLAGS1_FLAGS2)
			PTR_GETINT8(pD, Flags2);

		// Get Flags3?
		if (Flags2 & OBJUPD_FLAGS2_FLAGS3)
			PTR_GETINT8(pD, Flags3);

		// Get Flags4?
		if (Flags3 & OBJUPD_FLAGS3_FLAGS4)
			PTR_GETINT8(pD, Flags4);

		// Get Flags5?
		if (Flags4 & OBJUPD_FLAGS4_FLAGS5)
			PTR_GETINT8(pD, Flags5);

		int16 iClass; 
		PTR_GETINT16(pD, iClass);
		return iClass;
	}
	else
		return 0;
}

int CWObject_CoreData::GetObjectNr(const uint8* _pData)
{
	MAUTOSTRIP(CWObject_CoreData_GetObjectNr, 0);
	return (_pData[0] + (_pData[1] << 8)) & OBJUPD_NOBJAND;
}

CStr CWObject_CoreData::Dump(CMapData* _pWData, int _DumpFlags)
{
	MAUTOSTRIP(CWObject_CoreData_Dump, CStr());
	MRTC_CRuntimeClass_WObject* pRTC = _pWData->GetResource_Class(m_iClass);
//	CStr s = CStrF("%.16s", (pRTC) ? (char*)CStr(pRTC->m_ClassName) : "");
	CBox3Dfp32 VisBox;
	GetAbsVisBoundBox(VisBox);
	
	const char *pName = (pRTC) ? pRTC->m_ClassName : "";

	return CStrF("(%-24s), Obj %3.d, Cls %4.d, Clf %.8x, OPar %.3d, OChi %.3d, OPNCh %.3d, %.3d, Mdl %4.d, Snd %4.d, At %d + %4.f, iA(%5.d, %5.d, %5.d)", 
		pName,
		m_iObject, m_iClass, m_ClientFlags, m_iObjectParent, m_iObjectChild, m_iObjectChildPrev, m_iObjectChildNext, m_iModel[0], m_iSound[0], m_CreationGameTick, m_CreationGameTickFraction, m_iAnim0, m_iAnim1, m_iAnim2) + 
		m_LocalPos.GetString() + GetPositionMatrix().GetString()/* + GetAbsBoundBox()->GetString() + VisBox.GetString()*/;
}

// -------------------------------------------------------------------
void CWObject_CoreData::Read(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_CoreData_Read, MAUTOSTRIP_VOID);
	m_PhysState.Read(_pFile);
	m_PhysAbsBoundBox.Read(_pFile);
	m_PhysVelocity.Read(_pFile);

	m_VisBoxMin.Read(_pFile);
	m_VisBoxMax.Read(_pFile);
	m_LocalPos.Read(_pFile);
	m_Pos.Read(_pFile);
	m_LastPos.Read(_pFile);

	_pFile->ReadLE(m_iObjectParent);
	_pFile->ReadLE(m_iObjectChild);
	_pFile->ReadLE(m_iObjectChildPrev);
	_pFile->ReadLE(m_iObjectChildNext);

	int8 iParentAttachTmp;
	_pFile->ReadLE(iParentAttachTmp);
	m_iParentAttach = iParentAttachTmp;

	_pFile->ReadLE(m_ClientFlags);
	_pFile->ReadLE(m_iObject);
	_pFile->ReadLE(m_iClass);
	{ for(int i = 0; i < CWO_NUMMODELINDICES; i++) _pFile->ReadLE(m_iModel[i]); }
	_pFile->ReadLE(m_CreationGameTick);
	_pFile->ReadLE(m_CreationGameTickFraction);
	_pFile->ReadLE(m_iAnim0);
	_pFile->ReadLE(m_iAnim1);
	_pFile->ReadLE(m_iAnim2);
	{ for(int i = 0; i < CWO_NUMSOUNDINDICES; i++) _pFile->ReadLE(m_iSound[i]); }

	{ for(int i = 0; i < CWO_NUMDATA; i++) _pFile->ReadLE(m_Data[i]); }
}

void CWObject_CoreData::Write(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_CoreData_Write, MAUTOSTRIP_VOID);
	m_PhysState.Write(_pFile);
	m_PhysAbsBoundBox.Write(_pFile);
	m_PhysVelocity.Write(_pFile);

	m_VisBoxMin.Write(_pFile);
	m_VisBoxMax.Write(_pFile);
	m_LocalPos.Write(_pFile);
	m_Pos.Write(_pFile);
	m_LastPos.Write(_pFile);

	_pFile->WriteLE(m_iObjectParent);
	_pFile->WriteLE(m_iObjectChild);
	_pFile->WriteLE(m_iObjectChildPrev);
	_pFile->WriteLE(m_iObjectChildNext);
	_pFile->WriteLE(int8(m_iParentAttach));

	_pFile->WriteLE(m_ClientFlags);
	_pFile->WriteLE(m_iObject);
	_pFile->WriteLE(m_iClass);
	{ for(int i = 0; i < CWO_NUMMODELINDICES; i++) _pFile->WriteLE(m_iModel[i]); }
	_pFile->WriteLE(m_CreationGameTick);
	_pFile->WriteLE(m_CreationGameTickFraction);
	_pFile->WriteLE(m_iAnim0);
	_pFile->WriteLE(m_iAnim1);
	_pFile->WriteLE(m_iAnim2);
	{ for(int i = 0; i < CWO_NUMSOUNDINDICES; i++) _pFile->WriteLE(m_iSound[i]); }

	{ for(int i = 0; i < CWO_NUMDATA; i++) _pFile->WriteLE(m_Data[i]); }
}

// -------------------------------------------------------------------
//  CWObject_Client
// -------------------------------------------------------------------
void CWObject_Client::operator= (const CWObject_Client& _Obj)
{
	MAUTOSTRIP(CWObject_Client_operator_assign, MAUTOSTRIP_VOID);
	CWObject_CoreData::operator=(_Obj);

/*	*m_hVoice = *_Obj.m_hVoice;
	*m_iSoundPlaying = *_Obj.m_iSoundPlaying;*/
	memcpy(m_hVoice, _Obj.m_hVoice, sizeof(m_hVoice));
	memcpy(m_iSoundPlaying, _Obj.m_iSoundPlaying, sizeof(m_iSoundPlaying));
	memcpy(m_ClientData, _Obj.m_ClientData, sizeof(m_ClientData));
	m_iActiveClientCopy = _Obj.m_iActiveClientCopy;
//	Unlink();
//	m_spNextCopy = NULL;
//	m_spNextCopy = _Obj.m_spNextCopy;
}

void CWObject_Client::Clear()
{
	MAUTOSTRIP(CWObject_Client_Clear, MAUTOSTRIP_VOID);
	CWObject_CoreData::Clear();

	MemSet(m_ClientData, 0, sizeof(m_ClientData));
	
	for(int i = 0; i < CWO_NUMMODELINDICES; i++)
		m_lModelInstances[i] = NULL;
	
	m_iActiveClientCopy = 0;
	Unlink();
//	m_spNextCopy = NULL;
}

MRTC_IMPLEMENT(CWObject_Client, CWObject_CoreData);

CWObject_Client::CWObject_Client()
{
	MAUTOSTRIP(CWObject_Client_ctor, MAUTOSTRIP_VOID);
	Clear();

	for(int i = 0; i < CWO_NUMSOUNDINDICES; i++)
	{
		m_hVoice[i] = -1;
		m_iSoundPlaying[i] = 0;
	}
}

void CWObject_Client::UpdateModelInstance(CWObject_Client* _pObj, CWorld_Client* _pWClient, int _iModel, TPtr<CXR_ModelInstance> &_spInstance)
{
	MAUTOSTRIP(CWObject_Client_UpdateModelInstance, MAUTOSTRIP_VOID);
	MSCOPE(CWObject_Client::UpdateModelInstance, WORLD_CLIENT);
	
	// It is possible to crash here when entities with unprecached resources have been created. -JA
	CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_iModel);
	if(pModel)
	{
		_spInstance = pModel->CreateModelInstance();
		if (_spInstance != NULL)
			_spInstance->Create(pModel, CXR_ModelInstanceContext(_pWClient->GetGameTick(), _pWClient->GetGameTickTime(), _pObj, _pWClient));
	}
	else
		_spInstance = NULL;
}

int CWObject_Client::AddDeltaUpdate(CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	MAUTOSTRIP(CWObject_Client_AddDeltaUpdate, 0);
	MSCOPE(CWObject_Client::AddDeltaUpdate, WORLD_CLIENT);
	const uint8* pD = _pData;

	uint16 Flags0 = 0; //*(uint16*) pD; IncPtr(&pD, sizeof(Flags0));
	PTR_GETINT16(pD, Flags0);

	bool bHierarchy = false;
	m_bAutoVarDirty = false;

	if (Flags0 & OBJUPD_FLAGS0_UPDATE_ELSE_CLEAR)
	{
		uint8 Flags1 = 0;
		uint8 Flags2 = 0;
		uint8 Flags3 = 0;
		uint8 Flags4 = 0;
		uint8 Flags5 = 0;

		// Get Flags1?
		if (Flags0 & OBJUPD_FLAGS0_FLAGS1)
			PTR_GETINT8(pD, Flags1);

		// Get Flags2?
		if (Flags1 & OBJUPD_FLAGS1_FLAGS2)
			PTR_GETINT8(pD, Flags2);

		// Get Flags3?
		if (Flags2 & OBJUPD_FLAGS2_FLAGS3)
			PTR_GETINT8(pD, Flags3);

		// Get Flags4?
		if (Flags3 & OBJUPD_FLAGS3_FLAGS4)
			PTR_GETINT8(pD, Flags4);

		// Get Flags5?
		if (Flags4 & OBJUPD_FLAGS4_FLAGS5)
			PTR_GETINT8(pD, Flags5);

		int bHighPrecision = (Flags1 & OBJUPD_FLAGS1_HIGHPRECISION);

		if (Flags2 & OBJUPD_FLAGS2_CLEAR)
		{
			int iObj = m_iObject;
			int iClass = m_iClass;
			Clear();
			m_iObject = iObj;
			m_iClass = iClass;
		}

		if (Flags1 & OBJUPD_FLAGS1_ICLASS)
		{	
			PTR_GETINT16(pD, m_iClass);
			m_pRTC = _pWClient->GetMapData()->GetResource_Class(m_iClass);
		}

		// Run OnInitClientObjects when object is cleared and/or when class changes
		if (m_pRTC && ((Flags2 & OBJUPD_FLAGS2_CLEAR) || (Flags1 & OBJUPD_FLAGS1_ICLASS)))
			m_pRTC->m_pfnOnInitClientObjects(this, _pWClient);

		if (Flags0 & OBJUPD_FLAGS0_DELTAPOS)
		{
			_Flags |= CWO_ONCLIENTUPDATEFLAGS_DOLINK;
			int8 dm[3];
//			memcpy(&dm, pD, 6); IncPtr(&pD, 6);
			PTR_GETINT8(pD, dm[0]);
			PTR_GETINT8(pD, dm[1]);
			PTR_GETINT8(pD, dm[2]);

			m_LocalPos.k[3][0] += fp32(dm[0]);
			m_LocalPos.k[3][1] += fp32(dm[1]);
			m_LocalPos.k[3][2] += fp32(dm[2]);

//	ConOutL(CStrF("MOVE UPDATE %s", (char*) CVec3Dfp32::GetMatrixRow(m_PhysVelocity.m_Move, 3).GetString() ));
//			if (Flags2 & OBJUPD_FLAGS2_CLEAR) m_dPos = m_Pos;
		}

		if (Flags0 & OBJUPD_FLAGS0_ROTATION)
		{
			_Flags |= CWO_ONCLIENTUPDATEFLAGS_DOLINK;
			CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(m_LocalPos, 3);

			if (bHighPrecision)
			{
				// Use quaternion with 16 bit precision per element
				CQuatfp32 Q, dQ;
#ifdef OBJUPD_DEBUG_FLOATQUAT
				PTR_GETFP32(pD, Q.k[0]);
				PTR_GETFP32(pD, Q.k[1]);
				PTR_GETFP32(pD, Q.k[2]);
				PTR_GETFP32(pD, Q.k[3]);
#else
				uint16 qi[4];
				PTR_GETINT16(pD, qi[0]);
				PTR_GETINT16(pD, qi[1]);
				PTR_GETINT16(pD, qi[2]);
				PTR_GETINT16(pD, qi[3]);
				Q.k[0] = fp32(qi[0])/(0.5f*65535.0f) - 1.0f;
				Q.k[1] = fp32(qi[1])/(0.5f*65535.0f) - 1.0f;
				Q.k[2] = fp32(qi[2])/(0.5f*65535.0f) - 1.0f;
				Q.k[3] = fp32(qi[3])/(0.5f*65535.0f) - 1.0f;
#endif
				Q.SetMatrix(m_LocalPos);
			}
			else
			{
				// Use euler angles with 8 bit precision per angle
				uint8 vi[3];
				PTR_GETINT8(pD, vi[0]);
				PTR_GETINT8(pD, vi[1]);
				PTR_GETINT8(pD, vi[2]);
				CVec3Dfp32 v;
				v[0] = fp32(vi[0]) / 256.0f;
				v[1] = fp32(vi[1]) / 256.0f;
				v[2] = fp32(vi[2]) / 256.0f;
				v.CreateMatrixFromAngles(0, m_LocalPos);
			}

			CVec3Dfp32::GetMatrixRow(m_LocalPos, 3) = Pos;
		}

		if (Flags1)
		{
			if (Flags1 & OBJUPD_FLAGS1_MOVEVELOCITY)
			{
				if (bHighPrecision)
				{
					PTR_GETFP32(pD, m_PhysVelocity.m_Move.k[0]);
					PTR_GETFP32(pD, m_PhysVelocity.m_Move.k[1]);
					PTR_GETFP32(pD, m_PhysVelocity.m_Move.k[2]);
				}
				else
				{
					CVec3Dint8 vi;
					PTR_GETINT8(pD, vi.k[0]);
					PTR_GETINT8(pD, vi.k[1]);
					PTR_GETINT8(pD, vi.k[2]);
					CVec3Dfp32 Vel(vi[0], vi[1], vi[2]);
					m_PhysVelocity.m_Move = Vel;
				}
			}

			if (Flags1 & OBJUPD_FLAGS1_ROTVELOCITY)
			{
				PTR_GETFP32(pD, m_PhysVelocity.m_Rot.m_Axis.k[0]);
				PTR_GETFP32(pD, m_PhysVelocity.m_Rot.m_Axis.k[1]);
				PTR_GETFP32(pD, m_PhysVelocity.m_Rot.m_Axis.k[2]);
				PTR_GETFP32(pD, m_PhysVelocity.m_Rot.m_Angle);
			}
		}

		if (Flags2)
		{
			if (Flags2 & OBJUPD_FLAGS2_ANIMTIME)
			{
				PTR_GETINT32(pD, m_CreationGameTick);
				PTR_GETFP32(pD, m_CreationGameTickFraction);
			}
			if (Flags2 & OBJUPD_FLAGS2_IANIM0)
				PTR_GETINT16(pD, m_iAnim0);
			if (Flags2 & OBJUPD_FLAGS2_IANIM1)
				PTR_GETINT16(pD, m_iAnim1);
			if (Flags2 & OBJUPD_FLAGS2_IANIM2)
				PTR_GETINT16(pD, m_iAnim2);
			if (Flags2 & OBJUPD_FLAGS2_IMODEL1)
			{
				PTR_GETINT16(pD, m_iModel[0]);
				UpdateModelInstance(this, _pWClient, m_iModel[0], m_lModelInstances[0]);
			}

			if (Flags2 & OBJUPD_FLAGS2_POS)
			{
				_Flags |= CWO_ONCLIENTUPDATEFLAGS_DOLINK;
				if (bHighPrecision)
				{
					CVec3Dfp32& Row = CVec3Dfp32::GetMatrixRow(m_LocalPos, 3);
					PTR_GETFP32(pD, Row.k[0]);
					PTR_GETFP32(pD, Row.k[1]);
					PTR_GETFP32(pD, Row.k[2]);
				}
				else
				{
					CVec3Dint16 Pos_i;
					PTR_GETINT16(pD, Pos_i.k[0]);
					PTR_GETINT16(pD, Pos_i.k[1]);
					PTR_GETINT16(pD, Pos_i.k[2]);
					CVec3Dfp32 Pos(Pos_i.k[0], Pos_i.k[1], Pos_i.k[2]);
					CVec3Dfp32::GetMatrixRow(m_LocalPos, 3) = Pos;
				}
			}
		}

		if (Flags3)
		{
			if (Flags3 & OBJUPD_FLAGS3_CLFLAGS)
			{
				int32 OldClientFlags = m_ClientFlags;
				PTR_GETINT32(pD, m_ClientFlags);
				int32 LinkChange = (m_ClientFlags ^ OldClientFlags) & (CWO_CLIENTFLAGS_INVISIBLE | CWO_CLIENTFLAGS_LINKINFINITE | CWO_CLIENTFLAGS_VISIBILITY | CWO_CLIENTFLAGS_NOROTINHERITANCE | CWO_CLIENTFLAGS_AXISALIGNEDVISBOX | CWO_CLIENTFLAGS_SHADOWCASTER);
				if (LinkChange)
					_Flags |= CWO_ONCLIENTUPDATEFLAGS_DOLINK;
			}
			if (Flags3 & OBJUPD_FLAGS3_IMODEL23)
			{
				PTR_GETINT16(pD, m_iModel[1]);
				UpdateModelInstance(this, _pWClient, m_iModel[1], m_lModelInstances[1]);
			}
			if (Flags3 & OBJUPD_FLAGS3_IMODEL23)
			{
				PTR_GETINT16(pD, m_iModel[2]);
				UpdateModelInstance(this, _pWClient, m_iModel[2], m_lModelInstances[2]);
			}
//			if (Flags3 & OBJUPD_FLAGS3_IMODEL4)		{	m_iModel[3] = *(int16*) pD; IncPtr(&pD, sizeof(m_iModel[3])); }
			if (Flags3 & OBJUPD_FLAGS3_AUTOVAR)		{	m_bAutoVarDirty = true; }
			if (Flags3 & OBJUPD_FLAGS3_ISOUND1)		PTR_GETINT16(pD, m_iSound[0]);
			if (Flags3 & OBJUPD_FLAGS3_ISOUND2)		PTR_GETINT16(pD, m_iSound[1]);
			if (Flags3 & OBJUPD_FLAGS3_DATA0)		PTR_GETINT32(pD, m_Data[0]);
		}

		if(Flags4)
		{
			if (Flags4 & OBJUPD_FLAGS4_DATA1)	PTR_GETINT32(pD, m_Data[1]);
			if (Flags4 & OBJUPD_FLAGS4_DATA2)	PTR_GETINT32(pD, m_Data[2]);
			if (Flags4 & OBJUPD_FLAGS4_DATA3)	PTR_GETINT32(pD, m_Data[3]);
			if (Flags4 & OBJUPD_FLAGS4_DATA4)	PTR_GETINT32(pD, m_Data[4]);
			if (Flags4 & OBJUPD_FLAGS4_DATA5)	PTR_GETINT32(pD, m_Data[5]);
			if (Flags4 & OBJUPD_FLAGS4_DATA6)	PTR_GETINT32(pD, m_Data[6]);
			if (Flags4 & OBJUPD_FLAGS4_DATA7)	PTR_GETINT32(pD, m_Data[7]);
		}

		if (Flags5)
		{
			if (Flags5 & OBJUPD_FLAGS5_PHYSSTATE)
			{
				_Flags |= CWO_ONCLIENTUPDATEFLAGS_DOLINK;
				pD = m_PhysState.Read(pD);
			}
			if (Flags5 & OBJUPD_FLAGS5_PHYSPRIM0)
			{
				_Flags |= CWO_ONCLIENTUPDATEFLAGS_DOLINK;
				pD = m_PhysState.m_Prim[0].Read(pD);
			}
			if (Flags5 & OBJUPD_FLAGS5_PHYSPRIM1)
			{
				_Flags |= CWO_ONCLIENTUPDATEFLAGS_DOLINK;
				pD = m_PhysState.m_Prim[1].Read(pD);
			}
			if (Flags5 & OBJUPD_FLAGS5_PHYSPRIM2) 
			{
				_Flags |= CWO_ONCLIENTUPDATEFLAGS_DOLINK;
				pD = m_PhysState.m_Prim[2].Read(pD);
			}
/*			if (Flags5 & OBJUPD_FLAGS5_PHYSPRIM3) pD = m_PhysState.m_Prim[3].Read(pD);*/

			if (Flags5 & OBJUPD_FLAGS5_VISBOX)
			{
				_Flags |= CWO_ONCLIENTUPDATEFLAGS_DOLINK;
				m_VisBoxMin[0] = *(pD++);
				m_VisBoxMin[1] = *(pD++);
				m_VisBoxMin[2] = *(pD++);
				m_VisBoxMax[0] = *(pD++);
				m_VisBoxMax[1] = *(pD++);
				m_VisBoxMax[2] = *(pD++);
			}

			if (Flags5 & OBJUPD_FLAGS5_PHYSATTR)
				pD = m_PhysAttrib.Read(pD);
			if (Flags5 & OBJUPD_FLAGS5_HIERARCHY)
			{
				_Flags |= CWO_ONCLIENTUPDATEFLAGS_DOLINK;
				PTR_GETINT16(pD, m_iObjectParent);
				PTR_GETINT16(pD, m_iObjectChild);
				PTR_GETINT16(pD, m_iObjectChildNext);
				PTR_GETINT16(pD, m_iObjectChildPrev);
				PTR_GETINT8(pD, m_iParentAttach);
				bHierarchy = true;
			}
		}
	}
	else
	{
//		LogFile(CStrF("Clearing object %d, %d", Flags0 & OBJUPD_NOBJAND, iObj));
		m_iClass = 0;
	}
//	m_iObject = iObj;

	if (bHierarchy)
	{
		CWObject_CoreData* pParent = this;
		while(1)
		{
			CWObject_CoreData* pObj = _pWClient->Object_GetCD(pParent->GetParent());
			if (!pObj)
				break;
			pParent = pObj;
		}

		if (pParent->m_iObject != m_iObject)
		{
			// LogFile(CStrF("Object %d, Relinking from parent %d", m_iObject, pParent->m_iObject));
			_pWClient->Phys_InsertPosition(pParent->m_iObject, pParent);
		}
	}

	return (uint8*)pD - _pData;
}

CXR_AnimState CWObject_Client::GetDefaultAnimState(CWorld_Client* _pWClient, int iClientData)
{
	MAUTOSTRIP(CWObject_Client_GetDefaultAnimState, CXR_AnimState());
	return CXR_AnimState(CMTime::CreateFromTicks(GetAnimTick(_pWClient), _pWClient->GetGameTickTime(), _pWClient->GetRenderTickFrac()),
						 CMTime::CreateFromTicks(m_iAnim2, _pWClient->GetGameTickTime()),
						 m_iAnim0, m_iAnim1,
						 m_lModelInstances[iClientData], m_iObject);
}

// -------------------------------------------------------------------
//  CWObject_ClientExecute
// -------------------------------------------------------------------
void CWObject_ClientExecute::operator= (const CWObject_ClientExecute& _Obj)
{
	MAUTOSTRIP(CWObject_ClientExecute_operator_assign, MAUTOSTRIP_VOID);
	CWObject_Client::operator=(_Obj);

}

void CWObject_ClientExecute::Clear()
{
	MAUTOSTRIP(CWObject_ClientExecute_Clear, MAUTOSTRIP_VOID);
	CWObject_Client::Clear();
}

CWObject_ClientExecute::CWObject_ClientExecute()
{
	MAUTOSTRIP(CWObject_ClientExecute_ctor, MAUTOSTRIP_VOID);
	Clear();
}

void CWObject_ClientExecute::Read(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_ClientExecute_Read, MAUTOSTRIP_VOID);
	CWObject_CoreData::Read(_pFile);
}

void CWObject_ClientExecute::Write(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_ClientExecute_Write, MAUTOSTRIP_VOID);
	CWObject_CoreData::Write(_pFile);
}

// -------------------------------------------------------------------
//  CWObject
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject, CReferenceCount, 0x0100);

/*
const CWO_PhysicsState& CWObject::GetPhysState(int _iPhysState) const
{ 
	MAUTOSTRIP(CWObject_GetPhysState, *((void*)NULL));
	if ((_iPhysState < 0) || (_iPhysState >= m_nPhysStates)) 
		Error("GetPhysState", "Index out of range.");
	return m_PhysStates[_iPhysState]; 
};*/

CWObject::CWObject()
{
	MAUTOSTRIP(CWObject_ctor, MAUTOSTRIP_VOID);
	m_pWServer = NULL;
	m_GUID = 0;
	m_IntersectNotifyFlags = 0;
	m_iOwner = 0;
	m_NextRefresh = 0;
	m_LastClientUpdate = 0;
	m_DirtyMask = 0;
	m_Param = 0;

	m_pNameNode = NULL;
	m_ParentNameHash = 0;

	m_bOriginallySpawned = false;
	m_bNoSave = false;

//	m_iTZObject = 0;
}

#define MAXTMPPHYSSTATERECURS 8
static CWO_PhysicsState ms_TmpPhysState[MAXTMPPHYSSTATERECURS];
static uint8 ms_nTmpPhysState_RecursLevels = 0;
static int ms_TmpPhysState_iObject[MAXTMPPHYSSTATERECURS];

void CWObject::ReserveTempPhysState()
{
	MAUTOSTRIP(CWObject_OnInitTempPhysState, MAUTOSTRIP_VOID);

	if(ms_nTmpPhysState_RecursLevels == 0 || ms_TmpPhysState_iObject[ms_nTmpPhysState_RecursLevels - 1] != m_iObject)
	{
		if(ms_nTmpPhysState_RecursLevels == MAXTMPPHYSSTATERECURS - 1)
			Error("OnInitTempPhysState", "Out of temporary physstates.");

		ms_TmpPhysState[ms_nTmpPhysState_RecursLevels].Clear();
		ms_TmpPhysState[ms_nTmpPhysState_RecursLevels].m_ObjectFlags = OBJECT_FLAGS_PHYSOBJECT;
		ms_TmpPhysState_iObject[ms_nTmpPhysState_RecursLevels] = m_iObject;
		ms_nTmpPhysState_RecursLevels++;
	}
}

void CWObject::DiscardTempPhysState()
{
	MAUTOSTRIP(CWObject_OnDiscardTempPhysState, MAUTOSTRIP_VOID);
	ms_nTmpPhysState_RecursLevels--;
}

CWO_PhysicsState *CWObject::GetTempPhysState()
{
	if(ms_nTmpPhysState_RecursLevels != 0 && ms_TmpPhysState_iObject[ms_nTmpPhysState_RecursLevels - 1] == m_iObject)
		return &ms_TmpPhysState[ms_nTmpPhysState_RecursLevels - 1];

	return NULL;
}

int CWObject::Phys_TranslateObjectFlags(const char* _pStr)
{
	MAUTOSTRIP(CWObject_Phys_TranslateObjectFlags, 0);
	return CStrBase::TranslateFlags(_pStr, ms_ObjectFlagsTranslate);
}

int CWObject::Phys_TranslatePhysFlags(const char* _pStr)
{
	MAUTOSTRIP(CWObject_Phys_TranslatePhysFlags, 0);
	return CStrBase::TranslateFlags(_pStr, ms_PhysFlagsTranslate);
}

void CWObject::Phys_AddPrimitive(const char* _pPrim, CWO_PhysicsState* _pTarget)
{
	MAUTOSTRIP(CWObject_Phys_AddPrimitive, MAUTOSTRIP_VOID);
	CWO_PhysicsState* pPhys = _pTarget;
	if (pPhys->m_nPrim >= CWO_MAXPHYSPRIM) Error("Phys_AddPrimitive", "Too many physics primitives.");

	CFStr OrgPrim = _pPrim;
	CFStr Prim = _pPrim;
	CWO_PhysicsPrim* pPrim = &pPhys->m_Prim[pPhys->m_nPrim];
	pPrim->m_PrimType = Prim.GetStrSep(",").TranslateInt(ms_PrimTranslate);
	if (pPrim->m_PrimType <= OBJECT_PRIMTYPE_NONE)
		Error("Phys_AddPrimitive", CStrF("Invalid primtive: %s", (char*) OrgPrim));

	pPrim->m_DimX = uint16(Prim.Getfp64Sep(","));
	pPrim->m_DimY = uint16(Prim.Getfp64Sep(","));
	pPrim->m_DimZ = uint16(Prim.Getfp64Sep(","));
	pPrim->m_Offset[0] = Prim.Getfp64Sep(",");
	pPrim->m_Offset[1] = Prim.Getfp64Sep(",");
	pPrim->m_Offset[2] = Prim.Getfp64Sep(",");

	if (pPrim->m_PrimType == OBJECT_PRIMTYPE_PHYSMODEL)
	{
		pPrim->m_iPhysModel = m_pWServer->GetMapData()->GetResourceIndex_Model(Prim.GetStrSep(","));
		CXR_Model* pM = m_pWServer->GetMapData()->GetResource_Model(pPrim->m_iPhysModel);
		if (pM)
		{
			CXR_PhysicsModel* pPhysModel = pM->Phys_GetInterface();
			if (pPhysModel)
			{
				// Model ok.
				pPhys->m_nPrim++;
			}
			else
				ConOutL("§cf80WARNING (CWObject_Model::Phys_AddPrimitive): Model was not a physics-model.");
		}
		else
			ConOutL("§cf80WARNING (CWObject_Model::Phys_AddPrimitive): Invalid model-index.");
	}
	else
		pPhys->m_nPrim++;

	if (Prim != "")
	{
		pPhys->m_PhysFlags = Prim.GetStrSep(",").TranslateFlags(ms_PhysFlagsTranslate);
		pPhys->m_ObjectFlags = Prim.GetStrSep(",").TranslateFlags(ms_ObjectFlagsTranslate);
		pPhys->m_ObjectIntersectFlags = Prim.GetStrSep(",").TranslateFlags(ms_ObjectFlagsTranslate);
	}

	while (Prim != "")
	{
		int i = Prim.GetStrSep(",").TranslateFlags(ms_PhysExtensionsTranslate);
		switch(i)
		{
		case 1:
			pPhys->m_iExclude = m_iOwner;
			break;
		}
	}
}

void CWObject::SetNextRefresh(int _Tick)
{
	MAUTOSTRIP(CWObject_SetNextRefresh, MAUTOSTRIP_VOID);
	m_NextRefresh = _Tick;
}

void CWObject::SetTemplateName(CStr _TemplateName)
{
	MAUTOSTRIP(CWObject_SetTemplateName, MAUTOSTRIP_VOID);
	m_TemplateName = _TemplateName;
}

uint32 CWObject::GetNameHash() const
{
	if (m_pNameNode)
		return m_pNameNode->m_NameHash;
	return 0;
}


void CWObject::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();


	if (KeyName[0] == '$' ||
		_KeyHash == MHASH3('SERV','ERFL','AGS') ||
		_KeyHash == MHASH3('BRUS','HFL','AGS') || 
		_KeyHash == MHASH4('LIGH','T_MI','NLEV','EL') ||
		_KeyHash == MHASH3('LIGH','T_FL','AGS') ||
		_KeyHash == MHASH5('LIGH','T_SH','ADOW','MODE','L') ||
		_KeyHash == MHASH1('NAME') ||
		_KeyHash == MHASH2('COMM','ENT') ||
		(KeyName.CompareSubStr("BRUSH_PLANE_") == 0) ||
		(KeyName.CompareSubStr("OGR_") == 0))
	{
		// Nothing, just to get rid of the warnings in the logfile.
		return; 
	}

	switch(_KeyHash)
	{
	case MHASH3('CLIE','NTFL','AGS') : // "CLIENTFLAGS"
		{
			m_ClientFlags = _pKey->GetThisValue().TranslateFlags(ms_ClientFlagsTranslate);
		}
		break;

	case MHASH3('PHYS','PRIM','0') : // "PHYSPRIM0"
		{
			ReserveTempPhysState();
			Phys_AddPrimitive(_pKey->GetThisValue(), GetTempPhysState());
		}
		break;

	case MHASH3('PHYS','PRIM','1') : // "PHYSPRIM1"
		{
			ReserveTempPhysState();
			Phys_AddPrimitive(_pKey->GetThisValue(), GetTempPhysState());
		}
		break;

	case MHASH3('PHYS','PRIM','2') : // "PHYSPRIM2"
		{
			ReserveTempPhysState();
			Phys_AddPrimitive(_pKey->GetThisValue(), GetTempPhysState());
		}
		break;

	case MHASH2('ORI','GIN') : // "ORIGIN"
		{
			CVec3Dfp32 v; 
			_pKey->GetThisValueaf(3, v.k);

			if (!m_pWServer->Object_SetPosition(m_iObject, v))
				LogFile("§cf80WARNING: Failed setting ORIGIN, Entity: " + Dump(m_pWServer->GetMapData(), 0) );
			m_LastPos = m_Pos;
		}
		break;
	
	case MHASH2('ANG','LE') : // "ANGLE"
		{ 
			CMat4Dfp32 Mat(GetLocalPositionMatrix());
			Mat.SetZRotation3x3(_pKey->GetThisValuef() * (1.0f/360.0f));
			if (!m_pWServer->Object_SetPosition(m_iObject, Mat))
				LogFile("§cf80WARNING: Failed setting ANGLE, Entity: " + Dump(m_pWServer->GetMapData(), 0) );
			m_LastPos = m_Pos;
		}
		break;

	case MHASH2('ANG','LES') : // "ANGLES"
	case MHASH2('ROTA','TION') : // "ROTATION"
		{ 
			CMat4Dfp32 Mat;
			CVec3Dfp32 v; v.ParseString(_pKey->GetThisValue());
			v *= (1.0f/360.0f);
			v.CreateMatrixFromAngles(0, Mat);
			CVec3Dfp32::GetMatrixRow(Mat, 3) = GetLocalPosition();
			if (!m_pWServer->Object_SetPosition(m_iObject, Mat))
				LogFile("§cf80WARNING: Failed setting ANGLES, Entity: " + Dump(m_pWServer->GetMapData(), 0) );
			m_LastPos = m_Pos;
		}
		break;

	case MHASH2('ANIM','TIME') : // "ANIMTIME"
		{
			int KeyValuei = _pKey->GetThisValuei();
			fp32 KeyValuef = _pKey->GetThisValuef();
			SetAnimTick(m_pWServer, KeyValuei, KeyValuef - KeyValuei);
		}
		break;

	case MHASH2('ANIM','0') : // "ANIM0"
		{
			m_iAnim0 = _pKey->GetThisValuei();
		}
		break;

	case MHASH2('ANIM','1') : // "ANIM1"
		{
			m_iAnim1 = _pKey->GetThisValuei();
		}
		break;

	case MHASH2('ANIM','2') : // "ANIM2"
		{
			m_iAnim2 = _pKey->GetThisValuei();
		}
		break;

	case MHASH3('TARG','ETNA','ME') : // "TARGETNAME"
		{
			m_pWServer->Object_SetName(m_iObject, m_pWServer->World_MangleTargetName(_pKey->GetThisValue()));
		}
		break;

	case MHASH2('PARE','NT') : // "PARENT"
		{
			CStr KeyValue = _pKey->GetThisValue();
			CStr ParentName = m_pWServer->World_MangleTargetName(KeyValue.GetStrSep(":"));
			m_ParentNameHash = StringToHash(ParentName.Str());
#ifndef M_RTMCONSOLE
			m_ParentName = ParentName;
#endif
			if (KeyValue.Len())
			{ // Set attachpoint
				int iAttach = KeyValue.Val_int();
				m_iParentAttach = iAttach;
			}
		}
		break;

	case MHASH4('INTE','RSEC','TNOT','IFY') : // "INTERSECTNOTIFY"
		{
			m_IntersectNotifyFlags = _pKey->GetThisValue().TranslateFlags(ms_ObjectFlagsTranslate);
		}
		break;

	case MHASH3('PHYS','FLAG','S') : // "PHYSFLAGS"
		{
			int PhysFlags = _pKey->GetThisValue().TranslateFlags(ms_PhysFlagsTranslate);
			CWO_PhysicsState NewPhysState = m_PhysState;
			NewPhysState.m_PhysFlags |= PhysFlags;
			m_pWServer->Object_SetPhysics(m_iObject, NewPhysState);
		}
		break;

	default :
		{
			//		ConOutL("(CWObject::OnEvalKey) Key not evaluated: " + KeyName + " = " + KeyValue + ", Entity: " + Dump(m_pWServer->GetMapData(), 0) );
		}
	}
}

void CWObject::OnCreate()
{
	MAUTOSTRIP(CWObject_OnCreate, MAUTOSTRIP_VOID);
	SetAnimTick(m_pWServer, 0, 0);
	m_NextRefresh = m_pWServer->GetGameTick()+1;
}

void CWObject::OnFinishEvalKeys()
{
	MAUTOSTRIP(CWObject_OnFinishEvalKeys, MAUTOSTRIP_VOID);
	CWO_PhysicsState *pTempPS = GetTempPhysState();
	if (pTempPS != NULL)
	{
		if(pTempPS->m_nPrim && !m_pWServer->Object_SetPhysics(m_iObject, *pTempPS))
		{
			ConOutL("§cf80WARNING: Unable to set temporary physics state.");
			LogFile("PHYSSTATE: " + pTempPS->Dump(-1));
		}

		DiscardTempPhysState();
	}
}

void CWObject::OnInitInstance(const aint* _pParam, int _nParam)
{
	MAUTOSTRIP(CWObject_OnInitInstance, MAUTOSTRIP_VOID);
}


void CWObject::OnTransform(const CMat4Dfp32& _Mat)
{
	MAUTOSTRIP(CWObject_OnTransform, MAUTOSTRIP_VOID);

}

void CWObject::OnRegistryChange(const CRegistry* _pReg)
{
	MAUTOSTRIP(CWObject_OnRegistryChange, MAUTOSTRIP_VOID);

}

void CWObject::OnSpawnWorld()
{
	MAUTOSTRIP(CWObject_OnSpawnWorld, MAUTOSTRIP_VOID);
	if (m_ParentNameHash)
	{
		int iParent = m_pWServer->Selection_GetSingleTarget(m_ParentNameHash);
		if (iParent > 0)
			m_pWServer->Object_AddChild(iParent, m_iObject);
		else
		{
#ifndef M_RTMCONSOLE
			ConOutL(CStrF("(CWObject::OnSpawnWorld) Parent object '%s' not found.", (char*) m_ParentName));
#endif
		}
		m_ParentNameHash = 0;	// only used to get hold parent name until Object_AddChild() has been called
		m_ParentName.Clear();	// only used to give debug message if invalid parent was set
	}
}

void CWObject::OnSpawnWorld2()
{
	MAUTOSTRIP(CWObject_OnSpawnWorld2, MAUTOSTRIP_VOID);
}

void CWObject::OnLoad(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_OnLoad, MAUTOSTRIP_VOID);
	m_PhysState.OnLoad(_pFile, m_pWServer->GetMapData());
	m_PhysAbsBoundBox.Read(_pFile);
	m_PhysVelocity.Read(_pFile);

	m_VisBoxMin.Read(_pFile);
	m_VisBoxMax.Read(_pFile);
	m_LocalPos.Read(_pFile);
	m_Pos.Read(_pFile);
	m_LastPos.Read(_pFile);

	_pFile->ReadLE(m_iObjectParent);
	_pFile->ReadLE(m_iObjectChild);
	_pFile->ReadLE(m_iObjectChildPrev);
	_pFile->ReadLE(m_iObjectChildNext);

	int8 iParentAttachTmp;
	_pFile->ReadLE(iParentAttachTmp);
	m_iParentAttach = iParentAttachTmp;

	_pFile->ReadLE(m_ClientFlags);
	//LogFile("(CWObject_CoreData::OnLoad) 2");
	{ 
		for(int i = 0; i < CWO_NUMMODELINDICES; i++)
		{
			CStr s; s.Read(_pFile);
			//LogFile("(CWObject_CoreData::OnLoad) Model " + s);
			m_iModel[i] = m_pWServer->GetMapData()->GetResourceIndex(s);
		}
	}
	//LogFile("(CWObject_CoreData::OnLoad) 3");
	_pFile->ReadLE(m_CreationGameTick);
	_pFile->ReadLE(m_CreationGameTickFraction);
	_pFile->ReadLE(m_iAnim0);
	_pFile->ReadLE(m_iAnim1);
	_pFile->ReadLE(m_iAnim2);
	{ 
		for(int i = 0; i < CWO_NUMSOUNDINDICES; i++)
		{
			CStr s; s.Read(_pFile);
			LogFile("(CWObject_CoreData::OnLoad) Sound " + s);
			m_iSound[i] = m_pWServer->GetMapData()->GetResourceIndex(s);
		}
	}
	_pFile->Read((char *)m_Data, sizeof(m_Data));
	//	_pFile->ReadLE(m_Data, CWO_NUMDATA);
	//LogFile("(CWObject_CoreData::OnLoad) 4");

	{
		CStr s; s.Read(_pFile);
		m_pWServer->Object_SetName(m_iObject, s);
	}
/*	{
		CStr s; s.Read(_pFile);
		m_pWServer->Object_SetTarget(m_iObject, s);
	}*/

	_pFile->ReadLE(m_GUID);
	_pFile->ReadLE(m_IntersectNotifyFlags);
	uint16 iOwner = m_iOwner;
	_pFile->ReadLE(iOwner);

	m_PhysAttrib.Read(_pFile);
}

void CWObject::OnSave(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_OnSave, MAUTOSTRIP_VOID);
	m_PhysState.OnSave(_pFile, m_pWServer->GetMapData());
	m_PhysAbsBoundBox.Write(_pFile);
	m_PhysVelocity.Write(_pFile);

	m_VisBoxMin.Write(_pFile);
	m_VisBoxMax.Write(_pFile);
	m_LocalPos.Write(_pFile);
	m_Pos.Write(_pFile);
	m_LastPos.Write(_pFile);

	_pFile->WriteLE(m_iObjectParent);
	_pFile->WriteLE(m_iObjectChild);
	_pFile->WriteLE(m_iObjectChildPrev);
	_pFile->WriteLE(m_iObjectChildNext);
	_pFile->WriteLE(int8(m_iParentAttach));

	_pFile->WriteLE(m_ClientFlags);
	{ for(int i = 0; i < CWO_NUMMODELINDICES; i++) m_pWServer->GetMapData()->GetResourceName(m_iModel[i]).Write(_pFile); }
	_pFile->WriteLE(m_CreationGameTick);
	_pFile->WriteLE(m_CreationGameTickFraction);
	_pFile->WriteLE(m_iAnim0);
	_pFile->WriteLE(m_iAnim1);
	_pFile->WriteLE(m_iAnim2);
	{ for(int i = 0; i < CWO_NUMSOUNDINDICES; i++) m_pWServer->GetMapData()->GetResourceName(m_iSound[i]).Write(_pFile); }
	_pFile->Write((char *)m_Data, sizeof(m_Data));

	CStr a(GetName());
	a.Write(_pFile);

	_pFile->WriteLE(m_GUID);
	_pFile->WriteLE(m_IntersectNotifyFlags);
	uint16 iOwner = m_iOwner;
	_pFile->WriteLE(iOwner);

	m_PhysAttrib.Write(_pFile);
}

void CWObject::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	MAUTOSTRIP(CWObject_OnDeltaLoad, MAUTOSTRIP_VOID);
	_pFile->ReadLE(m_Param);
}

void CWObject::OnDeltaSave(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_OnDeltaSave, MAUTOSTRIP_VOID);
	_pFile->WriteLE(m_Param);
}

void CWObject::OnFinishDeltaLoad()
{
}

void CWObject::OnDestroy()
{
	MAUTOSTRIP(CWObject_OnDestroy, MAUTOSTRIP_VOID);
}

aint CWObject::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_OnMessage, 0);
	switch(_Msg.m_Msg)
	{
	case OBJSYSMSG_GETCAMERA :
		{
			if (_Msg.m_DataSize != sizeof(CMat4Dfp32)) return 0;
			CMat4Dfp32& Camera = *(CMat4Dfp32*)_Msg.m_pData;
			Camera = GetPositionMatrix();
			return 1;
		}
	case OBJSYSMSG_DESTROY:
		{
			m_pWServer->Object_Destroy(m_iObject);
			return 1;
		}
	case OBJSYSMSG_TELEPORT:
		{
			if(!_Msg.m_pData)
				return 0;

			CMat4Dfp32 Mat;
			if(_Msg.m_Param0 == 0)
			{
				CFStr ObjName = (const char *)_Msg.m_pData;
				CWObject* pObj = NULL;
				if (ObjName.CompareNoCase("$activator") == 0)
					pObj = m_pWServer->Object_Get(_Msg.m_Param1);
				else
					pObj = m_pWServer->Object_Get(m_pWServer->Selection_GetSingleTarget(ObjName));

				if (!pObj)
					return 0;

				Mat = pObj->GetPositionMatrix();
			}
			else if(_Msg.m_Param0 == 1)
			{
				Mat.Unit();
				CVec3Dfp32::GetRow(Mat, 3).ParseString((char *)_Msg.m_pData);
			}
			else if(_Msg.m_Param0 == 2)
			{
				Mat.Unit();
				CVec3Dfp32::GetRow(Mat, 3) = _Msg.m_VecParam0;
			}
			else if(_Msg.m_Param0 == 3)
			{
				Mat = *(CMat4Dfp32 *)_Msg.m_pData;
			}
			else if(_Msg.m_Param0 == 4)
			{
				CWObject *pObj = m_pWServer->Object_Get(m_pWServer->Selection_GetSingleTarget((char *)_Msg.m_pData));
				if (!pObj)
					return 0;

				Mat = GetPositionMatrix();
				CVec3Dfp32::GetRow(Mat, 3) = pObj->GetPosition();
			}
			else if (_Msg.m_Param0 == 5)
			{
				// Teleport to target but ignore XY-rotation
				CWObject* pObj = m_pWServer->Object_Get( m_pWServer->Selection_GetSingleTarget((char *)_Msg.m_pData) );
				if (!pObj)
					return 0;

				CVec3Dfp32 Angles1, Angles2;
				Angles1.CreateAnglesFromMatrix(0, GetPositionMatrix());
				Angles2.CreateAnglesFromMatrix(0, pObj->GetPositionMatrix());
				Angles1.k[2] = Angles2.k[2];
				Angles1.CreateMatrixFromAngles(0, Mat);
				Mat.GetRow(3) = pObj->GetPosition();
			}
			else if (_Msg.m_Param0 == 8)
			{ // Delta movement
				CVec3Dfp32 Offset;
				Offset.ParseString((const char*)_Msg.m_pData);

				Mat = GetPositionMatrix();
				Mat.GetRow(3) += Offset;
			}

			return m_pWServer->Object_SetPosition_World(m_iObject, Mat);
		}
	case OBJSYSMSG_PLAYSOUND:
		{
			if (_Msg.m_pData) // Format:  "<SOUND>/[<SYNCOBJ>]"
			{
				CFStr SyncObj = (const char *)_Msg.m_pData;
				CFStr Sound = SyncObj.GetStrSep("/");

				int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(Sound);
				if (iSound > 0)
				{
					uint32 GroupID = SyncObj.StrHash(); 

					///m_pWServer->Sound_At(GetPosition(), iSound, 0);
					if (!_Msg.m_Param0)
						m_pWServer->Sound_On(m_iObject, iSound, 0, 0, 1.0f, -1, CVec3Dfp32(0.0f), CVec3Dfp32(0.0f), GroupID);
					else
						m_pWServer->Sound_Off(m_iObject, iSound, 0);
				}
			}
			return 1;
		}
	case OBJSYSMSG_SETSOUND:
		{
			uint iSlot     = Clamp(_Msg.m_Param0 % 100, 0, CWO_NUMSOUNDINDICES-1);
			bool bInfinite = (_Msg.m_Param0 / 100) != 0;
			const char* pSound = (const char *)_Msg.m_pData;

			if (!pSound || (*pSound == 0))
			{
				iSound(iSlot) = 0;
			}
			else
			{
				int16 iRc = m_pWServer->GetMapData()->GetResourceIndex_Sound(pSound);
				if (iRc && bInfinite)
					iRc |= 0x8000;
				iSound(iSlot) = iRc;
			}
			return 1;
		}
	case OBJSYSMSG_SETMODEL:
		{
			char *pModel = (char *)_Msg.m_pData;
			if(!pModel || (*pModel == 0))
				iModel(_Msg.m_Param0) = 0;
			else
				iModel(_Msg.m_Param0) = m_pWServer->GetMapData()->GetResourceIndex_Model(pModel);

			UpdateVisibility();
			return 1;
		}
	case OBJSYSMSG_PRECACHEMESSAGE:
		{
			if(_Msg.m_DataSize == sizeof(CWObject_Message))
			{
				CWObject_Message *pMsg = (CWObject_Message *)_Msg.m_pData;
				switch(pMsg->m_Msg)
				{
					case OBJSYSMSG_PLAYSOUND:
					case OBJSYSMSG_SETSOUND:
						{
							if (pMsg->m_pData)
							{
								CFStr Sound = CFStr((const char*)pMsg->m_pData).GetStrSep("/");
								if (!Sound.IsEmpty())
									return m_pWServer->GetMapData()->GetResourceIndex_Sound(Sound) > 0;
							}
						}
						break;
					case OBJSYSMSG_SETMODEL:
						{
							char *pModel = (char *)pMsg->m_pData;
							if(pModel && (*pModel != 0))
								return m_pWServer->GetMapData()->GetResourceIndex_Model(pModel) > 0;
						}
						break;
				}
				
				return 1;
			}
		}
	
	case OBJSYSMSG_PARAM_SET:
		m_Param = _Msg.m_Param0;
		return m_Param;

	case OBJSYSMSG_PARAM_GET:
		return m_Param;

	case OBJSYSMSG_PARAM_ADD:
		m_Param += _Msg.m_Param0;
		return m_Param;

	case OBJSYSMSG_PARAM_FLAG_SET:
		m_Param |= _Msg.m_Param0;
		return m_Param;

	case OBJSYSMSG_PARAM_FLAG_CLEAR:
		m_Param &= ~_Msg.m_Param0;
		return m_Param;

	case OBJSYSMSG_PARAM_FLAG_ISSET:
		return (m_Param & _Msg.m_Param0) ? 1 : 0;

	case OBJSYSMSG_GETDISTANCE:
		{
			CWObject *pObj = m_pWServer->Object_Get(_Msg.m_iSender);
			if(!pObj)
				return 0;

			return aint((pObj->GetPosition() - GetPosition()).Length());
		}

	case OBJSYSMSG_GETRANDOM:
		if(_Msg.m_Param0 <= 0)
			return 0;
		return MRTC_RAND() % _Msg.m_Param0;

	case OBJSYSMSG_PARAM_RANDOMIZE:
		if(_Msg.m_Param0 <= 0)
			m_Param = 0;
		else
			m_Param = MRTC_RAND() % _Msg.m_Param0;
		return m_Param;
	
	case OBJSYSMSG_PARAM_COPYFROM:
		{
			if(_Msg.m_pData)
			{
				int iObj = m_pWServer->Selection_GetSingleTarget((const char *)_Msg.m_pData);
				CWObject *pObj = m_pWServer->Object_Get(iObj);
				if(pObj)
				{
					m_Param = pObj->m_Param;
					return 1;
				}
			}
			return 0;
		}

	case OBJSYSMSG_SETPARENT:
		{
			CFStr KeyValue = (const char*)_Msg.m_pData;
			CFStr TargetName = KeyValue.GetStrSep(":");	 
			if (KeyValue.Len())
			{   // Set attach joint. Note that this is done before check for correct target name as it was before. 
				// I guess you should be able to change joint for current parent without specifying parent with a ":<joint>" param... 
				int iAttach = KeyValue.GetStrSep(",").Val_int();
				m_iParentAttach = iAttach;
			}

			// Fast hack, recognize $this and $activator. This should be 
			// generalized so that all special names are recognized (and 
			// the functionality to do so should be available for all CWObjects)
			int iParent = 0;
			if (TargetName == "$this")
			{
				iParent = _Msg.m_iSender;
			}
			else if (TargetName == "$activator")
			{
				iParent = _Msg.m_Param1;
			}
			else
			{
				CFStr ParentName = m_pWServer->World_MangleTargetName(TargetName);
				iParent = m_pWServer->Selection_GetSingleTarget(ParentName);
				if ((iParent <= 0) && !ParentName.IsEmpty())
				{
					ConOutL(CStrF("WARNING: Parent object '%s' not found", ParentName.Str()));
					return 0;
				}

			}
			m_pWServer->Object_AddChild(iParent, m_iObject);

			{ // Set velocity
				const CVec3Dfp32& Pos0 = GetLastPosition();
				const CVec3Dfp32& Pos1 = GetPosition();
				m_pWServer->Object_SetVelocity(m_iObject, Pos1 - Pos0);
			}

			return 1;
		}


#ifndef M_RTM
	case OBJSYSMSG_GETDEBUGSTRING:
		{
			if(_Msg.m_DataSize == sizeof(CStr *))
			{
				CStr *pSt = (CStr *)_Msg.m_pData;
				MRTC_CRuntimeClass_WObject* pRTC = m_pWServer->GetMapData()->GetResource_Class(m_iClass);
				CFStr ClassName = pRTC ? CFStr(pRTC->m_ClassName).Copy(9, 1024).GetStr() : "";
				*pSt = CStrF("%i|%s|%s|%s,%s,%s|%i|", m_iObject, GetTemplateName() ? GetTemplateName() : ClassName.Str(), CFStr(GetName()).LowerCase().Str(), 
					CFStr::GetFilteredString(GetPosition()[0], 0).Str(), 
					CFStr::GetFilteredString(GetPosition()[1], 0).Str(),
					CFStr::GetFilteredString(GetPosition()[2], 0).Str(), GetParent());
				return 1;
			}
		}
#endif
	}
	return 0;
}

void CWObject::UpdateVisibility(int *_lpExtraModels, int _nExtraModels)
{
	CBox3Dfp32 VisBox;
	VisBox.m_Min = 0;
	VisBox.m_Max = 0;
	bool bShadowCaster = false;
	int nModels = CWO_NUMMODELINDICES + _nExtraModels;

	for(int i = 0; i < nModels; i++)
	{
		CXR_Model* pM;
		if(i < CWO_NUMMODELINDICES)
			pM = m_pWServer->GetMapData()->GetResource_Model(m_iModel[i]);
		else
			pM = m_pWServer->GetMapData()->GetResource_Model(_lpExtraModels[i - CWO_NUMMODELINDICES]);

		if (pM)
		{
			// Update vis box.
			CBox3Dfp32 Box;
			if (m_PhysState.m_nPrim && m_PhysState.m_Prim[0].m_ObjectFlagsMask != 0xffff)
				pM->GetBound_Box(Box, m_PhysState.m_Prim[0].m_ObjectFlagsMask);
			else
				pM->GetBound_Box(Box);

			VisBox.Expand(Box);

			if (pM->GetParam(CXR_MODEL_PARAM_ISSHADOWCASTER))
				bShadowCaster = true;
		}
	}

	if(bShadowCaster && !(m_ClientFlags & CWO_CLIENTFLAGS_SHADOWCASTER))
		ClientFlags() |= CWO_CLIENTFLAGS_SHADOWCASTER;
	else if(!bShadowCaster && (m_ClientFlags & CWO_CLIENTFLAGS_SHADOWCASTER))
		ClientFlags() &= ~CWO_CLIENTFLAGS_SHADOWCASTER;

	CBox3Dfp32 CurBox;
	GetVisBoundBox(CurBox);
	if(CurBox.m_Max != VisBox.m_Max || CurBox.m_Min != VisBox.m_Min)
		m_pWServer->Object_SetVisBox(m_iObject, VisBox.m_Min, VisBox.m_Max);
}

void CWObject::OnRefresh()
{
	MAUTOSTRIP(CWObject_OnRefresh, MAUTOSTRIP_VOID);
}

void CWObject::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CWObject_OnIncludeClass, MAUTOSTRIP_VOID);
}

void CWObject::OnIncludeTemplate(CRegistry *, CMapData* _pWData, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CWObject_OnIncludeTemplate, MAUTOSTRIP_VOID);
}

void CWObject::OnClientCreate(CWObject_ClientExecute* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_OnClientCreate, MAUTOSTRIP_VOID);
	_pObj->m_CreationGameTick = _pWClient->GetGameTick();
	_pObj->m_CreationGameTickFraction = 0.0f;

}

void CWObject::OnClientExecute(CWObject_ClientExecute* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_OnClientExecute, MAUTOSTRIP_VOID);
}

void CWObject::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_OnClientRefresh, MAUTOSTRIP_VOID);
}

void CWObject::OnClientPredict(CWObject_Client* _pObj, CWorld_Client* _pWClient, int, int, int)
{
	MAUTOSTRIP(CWObject_OnClientPredict, MAUTOSTRIP_VOID);
}

void CWObject::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_OnClientRender, MAUTOSTRIP_VOID);
}

void CWObject::OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_OnClientRenderVis, MAUTOSTRIP_VOID);
}

aint CWObject::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_OnClientMessage, 0);
	return 0;
}

void CWObject::OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg)
{
	MAUTOSTRIP(CWObject_OnClientNetMsg, MAUTOSTRIP_VOID);
}

int CWObject::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	MAUTOSTRIP(CWObject_OnClientUpdate, 0);
	MSCOPESHORT(CWObject::OnClientUpdate);
	return _pObj->AddDeltaUpdate(_pWClient, _pData, _Flags);
}

void CWObject::OnClientPrecache(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine)
{
	MAUTOSTRIP(CWObject_OnClientPrecache, MAUTOSTRIP_VOID);
}

void CWObject::OnClientPrecacheClass(CWorld_Client* _pWClient, CXR_Engine* _pEngine)
{
	MAUTOSTRIP(CWObject_OnClientPrecacheClass, MAUTOSTRIP_VOID);
}

void CWObject::OnClientPostPrecache(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine)
{
	MAUTOSTRIP(CWObject_OnClientPrecache, MAUTOSTRIP_VOID);
}

void CWObject::OnClientLoad(CWObject_Client* _pObj, CWorld_Client* _pWClient, CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	MAUTOSTRIP(CWObject_OnClientLoad, MAUTOSTRIP_VOID);
	_pObj->CWObject_CoreData::Read(_pFile);
}

void CWObject::OnClientSave(CWObject_Client* _pObj, CWorld_Client* _pWClient, CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	MAUTOSTRIP(CWObject_OnClientSave, MAUTOSTRIP_VOID);
	_pObj->CWObject_CoreData::Write(_pFile);
}


bool CWObject::OnIntersectLine(CWO_OnIntersectLineContext& _Context, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CWObject_OnIntersectLine, false);
	CWObject_CoreData* pObj = _Context.m_pObj;
	if (!pObj) 
		return false;

	// This test is done in the hash enumerator
	//	if (!((pObj->GetPhysState().m_ObjectFlags | pObj->GetPhysState().m_ObjectIntersectFlags) & _ObjectFlags)) return false;
	//	if (!(pObj->GetPhysState().m_ObjectFlags & _ObjectFlags)) return false;

	CWorld_PhysState* pWPhysState = _Context.m_pPhysState;

	const CWO_PhysicsState& PhysState = pObj->GetPhysState();

	bool bImpact = false;
	for(int iPrim = 0; iPrim < PhysState.m_nPrim; iPrim++)
	{
		const CWO_PhysicsPrim& PhysPrim = PhysState.m_Prim[iPrim];
		if (!(((PhysState.m_ObjectFlags & PhysPrim.m_ObjectFlagsMask) & _Context.m_ObjectIntersectionFlags) ||
			(PhysState.m_ObjectIntersectFlags & _Context.m_ObjectFlags)))
			continue;

		CXR_PhysicsModel* pPhysModel = NULL;

		const CMat4Dfp32* pPos = &pObj->GetPositionMatrix();
		CMat4Dfp32 Pos;
		if (pObj->GetPhysState().m_PhysFlags & OBJECT_PHYSFLAGS_OFFSET)
		{
			Pos = *pPos;
			CVec3Dfp32 Ofs = pObj->GetPhysState().m_Prim[iPrim].GetOffset();
			if (pObj->GetPhysState().m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION)
				Ofs.MultiplyMatrix3x3(*pPos);
			CVec3Dfp32::GetMatrixRow(Pos, 3) += Ofs;
			pPos = &Pos;
		}

		CXR_PhysicsContext PhysContext;
		switch(PhysPrim.m_PrimType)
		{
		case OBJECT_PRIMTYPE_PHYSMODEL :
			{
				CXR_Model* pModel = pWPhysState->GetMapData()->GetResource_Model(PhysPrim.m_iPhysModel);
				if (!pModel) continue;
				pPhysModel = pModel->Phys_GetInterface();

				CXR_AnimState Anim(CMTime::CreateFromTicks(pWPhysState->GetGameTick() - pObj->m_CreationGameTick, pWPhysState->GetGameTickTime(), -pObj->m_CreationGameTickFraction), CMTime(), pObj->m_iAnim0, pObj->m_iAnim1, NULL, 0);
				PhysContext = CXR_PhysicsContext(*pPos, &Anim);
				PhysContext.m_PhysGroupMaskThis = PhysPrim.m_PhysModelMask;
				pPhysModel->Phys_Init(&PhysContext);
			}
			break;

		case OBJECT_PRIMTYPE_SPHERE :
			{
				pWPhysState->m_PhysModel_Sphere.Phys_SetDimensions(PhysPrim.GetRadius());
				PhysContext = CXR_PhysicsContext(*pPos);
				pWPhysState->m_PhysModel_Sphere.Phys_Init(&PhysContext);
				pPhysModel = &pWPhysState->m_PhysModel_Sphere;
			}
			break;

		case OBJECT_PRIMTYPE_BOX :
			{
				pWPhysState->m_PhysModel_Box.Phys_SetDimensions(PhysPrim.GetDim());
				PhysContext = CXR_PhysicsContext(*pPos);
				pWPhysState->m_PhysModel_Box.Phys_Init(&PhysContext);
				pPhysModel = &pWPhysState->m_PhysModel_Box;
			}
		default :
			break;
		}

		if (pPhysModel)
		{
			if(_pCollisionInfo)
			{
				CCollisionInfo CollInfo;
				CollInfo.CopyParams(*_pCollisionInfo);
				if (pPhysModel->Phys_IntersectLine(&PhysContext, _Context.m_p0, _Context.m_p1, _Context.m_MediumFlags, &CollInfo))
				{
					CollInfo.m_iObject = pObj->m_iObject;
					_pCollisionInfo->Improve(CollInfo);
					if (_pCollisionInfo->IsComplete())
						return true;

	#ifdef _DEBUG
					if (_pCollisionInfo->m_bIsValid && (_pCollisionInfo->m_Time < -0.01f || _pCollisionInfo->m_Time > 1.01f))
					{
						ConOut(CStrF("(CWorld_PhysState::Object_IntersectLine) Entity %d,  Phys_IntersectLine returned T = %f", pObj->m_iObject, _pCollisionInfo->m_Time));
	//					pPhysModel->Phys_IntersectLine(_p0, _p1, _MediumFlags, (_pCollisionInfo) ? &CInfo : NULL);
					}
	#endif
					bImpact = true;
				}
			}
			else
			{
				if(pPhysModel->Phys_IntersectLine(&PhysContext, _Context.m_p0, _Context.m_p1, _Context.m_MediumFlags, NULL))
					return true;
			}
		}
	}

	return bImpact;

}

int CWObject::OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CWObject_OnPhysicsEvent, 0);
	switch(_Event)
	{
	case CWO_PHYSEVENT_GETACCELERATION :
		{
			if (_pMat) _pMat->Unit();
			return SERVER_PHYS_DEFAULTHANDLER;
		}
		break;

	default :
		return SERVER_PHYS_DEFAULTHANDLER;
	}
	return SERVER_PHYS_DEFAULTHANDLER;
}


void CWObject::OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
//	M_TRACE("[%d: %d] OnInitClientObjects (%s)\n", _pObj->m_iClass, _pObj->m_iObject, _pWPhysState->IsClient() ? "client" : "server");
}


uint CWObject::OnGetAttachMatrices(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, uint* _piAttachMatrices, uint _nAttachMatrices, CMat4Dfp32* _pRet)
{
	return 0;
}



CStr CWObject::Dump(CMapData* _pWData, int _DumpFlags)
{
	MAUTOSTRIP(CWObject_Dump, CStr());
//#ifndef M_RTMCONSOLE
	return CStrF("Name %-23s, TName %-32s, %s, GUID %.4d, Owner %.4d, (Phys %s), (PhysBox %s), Vel %s, RotVel %s", 
		m_Name.Str(), 
		(char*)m_TemplateName,
		(char*)CWObject_CoreData::Dump(_pWData, _DumpFlags), 
		m_GUID,
		m_iOwner,
		(char*)m_PhysState.Dump(_DumpFlags),
		(char*)m_PhysAbsBoundBox.GetString(),
		(char*)GetMoveVelocity().GetString(),
		(char*)m_PhysVelocity.m_Rot.GetString()
		);
/*#else
	return CStr();
#endif*/
/*
	return CStrF("%s, GUID %.4d, TName %s, Name %s, Tgt %s, Owner %.4d, (Phys %s), (PhysBox %s), Vel %s, RotVel %s", 
		(char*)CWObject_CoreData::Dump(_pWData, _DumpFlags), 
		m_GUID,
		(char*)m_TemplateName,
		(char*)m_Name.m_Name, 
		(char*)m_Target, 
		m_iOwner,
		(char*)m_PhysState.Dump(_DumpFlags),
		(char*)m_PhysAbsBoundBox.GetString(),
		(char*)GetMoveVelocity().GetString(),
		(char*)m_PhysVelocity.m_Rot.GetString()
		);
*/
}

void CWObject::IncludeModelFromKey(const CStr Key, CRegistry *pReg, CMapData *_pMapData)
{
	MAUTOSTRIP(CWObject_IncludeModelFromKey, MAUTOSTRIP_VOID);
	if(pReg)
	{
		CRegistry *pChild = pReg->FindChild(Key);
		if(pChild)
			_pMapData->GetResourceIndex_Model(pChild->GetThisValue());
	}
}

void CWObject::IncludeSoundFromKey(const CStr Key, CRegistry *pReg, CMapData *_pMapData)
{
	MAUTOSTRIP(CWObject_IncludeSoundFromKey, MAUTOSTRIP_VOID);
	if(pReg)
	{
		CRegistry *pChild = pReg->FindChild(Key);
		if(pChild)
			_pMapData->GetResourceIndex_Sound(pChild->GetThisValue());
	}
}

void CWObject::IncludeClassFromKey(const CStr Key, CRegistry *pReg, CMapData *_pMapData)
{
	MAUTOSTRIP(CWObject_IncludeClassFromKey, MAUTOSTRIP_VOID);
	if(pReg)
	{
		CRegistry *pChild = pReg->FindChild(Key);
		if(pChild)
			_pMapData->GetResourceIndex_Class(pChild->GetThisValue());
	}
}

void CWObject::IncludeSurfaceFromKey(const CStr Key, CRegistry *pReg, CMapData *_pMapData)
{
	MAUTOSTRIP(CWObject_IncludeSurfaceFromKey, MAUTOSTRIP_VOID);
	if(pReg)
	{
		CRegistry *pChild = pReg->FindChild(Key);
		if(pChild)
			_pMapData->GetResourceIndex_Surface(pChild->GetThisValue());
	}
}

void CWObject::IncludeAnimFromKey(const CStr Key, CRegistry *pReg, CMapData *_pMapData)
{
	MAUTOSTRIP(CWObject_IncludeAnimFromKey, MAUTOSTRIP_VOID);
	if(pReg)
	{
		CRegistry *pChild = pReg->FindChild(Key);
		if(pChild)
		{
			CFStr st = pChild->GetThisValue();
			_pMapData->GetResourceIndex_Anim(st.GetStrSep(":"));
		}
	}
}

void CWObject::IncludeModel(const CStr _Model, CMapData *_pMapData)
{
	MAUTOSTRIP(CWObject_IncludeModel, MAUTOSTRIP_VOID);
	_pMapData->GetResourceIndex_Model(_Model);
}

void CWObject::IncludeSound(const CStr _Sound, CMapData *_pMapData)
{
	MAUTOSTRIP(CWObject_IncludeSound, MAUTOSTRIP_VOID);
	_pMapData->GetResourceIndex_Sound(_Sound);
}

void CWObject::IncludeClass(const CStr _Class, CMapData *_pMapData)
{
	MAUTOSTRIP(CWObject_IncludeClass, MAUTOSTRIP_VOID);
	_pMapData->GetResourceIndex_Class(_Class);
}

void CWObject::IncludeSurface(const CStr _Surface, CMapData *_pMapData)
{
	MAUTOSTRIP(CWObject_IncludeSurface, MAUTOSTRIP_VOID);
	_pMapData->GetResourceIndex_Surface(_Surface);
}

void CWObject::IncludeAnim(const CStr _Anim, CMapData *_pMapData)
{
	MAUTOSTRIP(CWObject_IncludeAnim, MAUTOSTRIP_VOID);
	CFStr st = _Anim;
	_pMapData->GetResourceIndex_Anim(st.GetStrSep(":"));
}

// -------------------------------------------------------------------
int UnpackDeltaRegistry(CRegistry* _pReg, const uint8* _pData, int _Len)
{
	MAUTOSTRIP(UnpackDeltaRegistry, 0);
/*
Format:
	int8		RegNr
	int8		nStrings
	int16		nTotalStrings

	nStrings * 
	{
		int16	iString, 0x8000 == Key value, else Key name.
		int8	StringLen
		int8	StringData[StringLen]
	}

*/
	const uint8* pD = _pData;
	int nStr;
	int nTotal;
	PTR_GETINT8(pD, nStr);
	PTR_GETINT16(pD, nTotal);
//ConOutL(CStrF("    (Unpack) Reg %.8x", _pReg));
//ConOutL(CStrF("    (Unpack) nTotal %d, nStr %d", nTotal, nStr));
	_pReg->SetNumChildren(nTotal);

	for(int i = 0; i < nStr; i++)
	{
		int iStr;
		int Len;
		PTR_GETINT16(pD, iStr);
		PTR_GETINT8(pD, Len);
//ConOutL(CStrF("       (Unpack) iStr %d, Len %d", iStr, Len));
		CStr s;
		s.Capture((char*)pD, Len);
		pD += Len;
		int iS = iStr & 0x7fff;
		if (iS < 0 || iS >= nTotal)
		{
			ConOut(CStrF("(::UnpackDeltaRegistry) String index out of range. (%s/%s)", iS, nTotal));
			return 0;
		}
//ConOutL(CStrF("       (Unpack) Str: %s", (char*) s));
		if (iStr & 0x8000)
			_pReg->SetValue(iS, s);
		else
			_pReg->RenameKey(iS, s);
	}

	return pD - _pData;
}

int PackDeltaRegistry(int _RegNr, const CRegistry* _pReg, const CRegistry* _pOldReg, uint8* _pData, int _RecommendLen, int _MaxLen)
{
	MAUTOSTRIP(PackDeltaRegistry, 0);
/*
MaxLen is the buffer size.

RecommendLen is the desired maximum update size, but an update might be 
	larger if one string doesn't fit in _RecommendLen.

Format:
	int8		RegNr
	int8		nStrings
	int16		nTotalStrings

	nStrings * 
	{
		int16	iString, 0x8000 == Key value, else Key name.
		int8	StringLen
		int8	StringData[StringLen]
	}

*/

	int nKeys = _pReg->GetNumChildren();
	int nOldKeys = _pOldReg->GetNumChildren();
	if (nOldKeys > nKeys) nOldKeys = nKeys;

	uint8* pD = &_pData[2];

	_pData[0] = _RegNr;
	_pData[1] = 0;
	PTR_PUTINT16(pD, nKeys);
// ConOutL(CStrF("(Pack) Reg %.8x, OldReg %.8x", _pReg, _pOldReg));
// ConOutL(CStrF("(Pack) nTotal %d, nOldTotal %d, iReg %d", nKeys, nOldKeys, _RegNr));

	int i = 0;
	for(; i < nOldKeys; i++)
	{
		if (_pData[1] && ((pD-_pData) > _RecommendLen)) break;
		if (_MaxLen-(pD-_pData) < 250) break;
		if (_pData[1] > 240) break;

		{
			if (_pReg->GetValue(i) != _pOldReg->GetValue(i))
			{
// ConOutL(CStrF("Value New %s, Old %s", (char*) _pReg->GetValue(i), (char*) _pOldReg->GetValue(i)));
				_pData[1]++;
				int len = _pReg->GetValue(i).Len();
				if (len > 240) break;
				const char* pS = _pReg->GetValue(i);
				PTR_PUTINT16(pD, i + 0x8000);
				PTR_PUTINT8(pD, len);
				memcpy(pD, pS, len);
				pD += len;
			}
		}

		if (_pData[1] && ((pD-_pData) > _RecommendLen)) break;
		if (_MaxLen-(pD-_pData) < 250) break;
		{
			if (_pReg->GetName(i) != _pOldReg->GetName(i))
			{
// ConOutL(CStrF("Name New %s, Old %s", (char*) _pReg->GetName(i), (char*) _pOldReg->GetName(i)));
				_pData[1]++;
				int len = _pReg->GetName(i).Len();
				if (len > 240) break;
				const char* pS = _pReg->GetName(i);
				PTR_PUTINT16(pD, i);
				PTR_PUTINT8(pD, len);
				memcpy(pD, pS, len);
				pD += len;
			}
		}
	}

	for(;i < nKeys; i++)
	{
		if (_pData[1] && ((pD-_pData) > _RecommendLen)) break;
		if (_MaxLen-(pD-_pData) < 250) break;
		if (_pData[1] > 250) break;		// Check if we've updated 250 string.

		{
// ConOutL(CStrF("Value New %s", (char*) _pReg->GetValue(i)));
			_pData[1]++;
			int len = _pReg->GetValue(i).Len();
			const char* pS = _pReg->GetValue(i);
			PTR_PUTINT16(pD, i + 0x8000);
			PTR_PUTINT8(pD, len);
			memcpy(pD, pS, len);
			pD += len;
		}

		if (_pData[1] && ((pD-_pData) > _RecommendLen)) break;
		if (_MaxLen-(pD-_pData) < 250) break;

		{
// ConOutL(CStrF("Name New %s", (char*) _pReg->GetName(i)));
			_pData[1]++;
			int len = _pReg->GetName(i).Len();
//			const char* pS = _pReg->GetName(i);
			PTR_PUTINT16(pD, i);
			PTR_PUTINT8(pD, len);
			pD += len;
		}
	}

	if (pD != &_pData[4])
	{
// ConOutL(CStrF("(Pack) nStr %d", _pData[1]));
		return pD - _pData; //_pDeltaBuffer->AddDeltaUpdate(DeltaBuffer, pD - DeltaBuffer);
	}
	return 0;
}


//-----------------------------------------------------------------------------
//Static stuff

static CBox3Dfp32 s_WorldBound;

void CWObject::InitStatic() const
{
	CWObject* pWorldSpawn = m_pWServer->Object_Get( m_pWServer->Object_GetWorldspawnIndex() );
	if (pWorldSpawn)
	{
		s_WorldBound = *pWorldSpawn->GetAbsBoundBox();
		s_WorldBound.Grow(8.0f);
	}
}


// Hack to remove objects that have managed to get totally out of control
bool CWObject::CheckForOOB() const
{
	if (!GetAbsBoundBox()->IsInside(s_WorldBound))
	{
		m_pWServer->Object_Destroy(m_iObject);
		return true;
	}

	return false;
}
