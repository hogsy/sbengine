
#include "PCH.h"

#ifdef PLATFORM_WIN
#pragma warning(disable : 4244)
#endif

#if defined(PLATFORM_PS2) && defined(COMPILER_CODEWARRIOR)
// For some reason inlining is bad in this file
#pragma  inline_depth(0)
#endif

#include "XRClass.h"
#include "Phys/WPhysPCS.h"
#include "MFloat.h"


#ifdef PLATFORM_DOLPHIN
CXR_LightGridPoint::CXR_LightGridPoint(const CPixel32 &_Light, const CVec3Dint8 &_LightDir, const uint8 &_LightBias)
{
	uint32 Tmp32 = _Light;
	m_LightR = Tmp32 >> (16+3);
	m_LightG = Tmp32 >> ( 8+3);
	m_LightB = Tmp32 >> ( 0+3);
	m_LightDirX = (_LightDir[0] >> 4);
	m_LightDirY = (_LightDir[1] >> 4);
	m_LightDirZ = (_LightDir[2] >> 4);
	m_LightBias = (_LightBias >> 3);
//LogFile(CStrF("%.8x, %d, %d, %d, %d", m_Light, m_LightDir[0], m_LightDir[1], m_LightDir[2], m_LightBias));
}
#endif
// -------------------------------------------------------------------
//  CXR_LightGridPoint
// -------------------------------------------------------------------
void CXR_LightGridPoint::Read(CCFile* _pF, int _Ver)
{
	MAUTOSTRIP(CXR_LightGridPoint_Read, MAUTOSTRIP_VOID);
	if (_Ver == 0x0100)
	{
#ifdef PLATFORM_DOLPHIN
		uint32 Tmp32;
		_pF->ReadLE(Tmp32);
		m_LightR = Tmp32 >> (16+3);
		m_LightG = Tmp32 >> ( 8+3);
		m_LightB = Tmp32 >> ( 0+3);

		uint8 Tmp8;
		_pF->ReadLE(Tmp8); m_LightDirX = (Tmp8 >> 4);
		_pF->ReadLE(Tmp8); m_LightDirY = (Tmp8 >> 4);
		_pF->ReadLE(Tmp8); m_LightDirZ = (Tmp8 >> 4);
		_pF->ReadLE(Tmp8); m_LightBias = (Tmp8 >> 3);

#else	
		m_Light = 0xff00ff00;
		uint32 Tmp;
		_pF->ReadLE(Tmp);
		m_Light = Tmp;
		_pF->ReadLE((uint8&)m_LightDir[0]);
		_pF->ReadLE((uint8&)m_LightDir[1]);
		_pF->ReadLE((uint8&)m_LightDir[2]);
		_pF->ReadLE(m_LightBias);
#endif		

//LogFile(CStrF("%.8x, %d, %d, %d, %d", m_Light, m_LightDir[0], m_LightDir[1], m_LightDir[2], m_LightBias));
	}
	else
		Error_static("CXR_LightGridPoint::Read", CStrF("Unsupported version %.4x", _Ver));
}

void CXR_LightGridPoint::Write(CCFile* _pF, int _Ver) const
{
	MAUTOSTRIP(CXR_LightGridPoint_Write, MAUTOSTRIP_VOID);
#if defined( PLATFORM_DOLPHIN ) || defined( PLATFORM_PS2 )
	Error_static("CXR_LightGridPoint::Write", "No support for Write()..");
#else	
	if (_Ver == 0x0100)
	{
		uint32 Tmp = m_Light;
		_pF->WriteLE(Tmp);
		_pF->WriteLE((uint8&)m_LightDir[0]);
		_pF->WriteLE((uint8&)m_LightDir[1]);
		_pF->WriteLE((uint8&)m_LightDir[2]);
		_pF->WriteLE(m_LightBias);
	}
	else
		Error_static("CXR_LightGridPoint::Write", CStrF("Unsupported version %.4x", _Ver));
#endif		
}

#ifndef CPU_LITTLEENDIAN
void CXR_LightGridPoint::SwapLE()
{
	MAUTOSTRIP(CXR_LightGridPoint_SwapLE, MAUTOSTRIP_VOID);

#ifdef CPU_BIGENDIAN
	uint32 Tmp = *((uint32*)&m_Light);
	::SwapLE(Tmp);
	m_Light = Tmp;
	m_LightDir.SwapLE();
	::SwapLE(m_LightBias);
#endif
}
#endif

// -------------------------------------------------------------------
//  CXR_LightFieldElement
// -------------------------------------------------------------------
void CXR_LightFieldElement::Read(CCFile* _pF, int _Ver)
{
	switch(_Ver)
	{
	case 0x0102:
		{
			m_Scaler.Read(_pF);
			_pF->ReadLE(&(m_Axis[0][0]), 6 * 3);
			break;
		}
	case 0x0101:
		{
			m_Scaler.m_Half = FP2_ONE;
			_pF->ReadLE(&(m_Axis[0][0]), 6 * 3);
			break;
		};
	case 0x0100:
		{
			m_Scaler.m_Half = FP2_ONE;
			CPixel32 lAxis[6];
			_pF->ReadLE((uint32*)lAxis, 6);
			for(int i = 0; i < 6; i++)
			{
				m_Axis[i][0] = lAxis[i].GetB();
				m_Axis[i][1] = lAxis[i].GetG();
				m_Axis[i][2] = lAxis[i].GetR();
			}
			break;
		};
	}
}

void CXR_LightFieldElement::Write(CCFile* _pF) const
{
	m_Scaler.Write(_pF);
	_pF->WriteLE(&(m_Axis[0][0]), 6 * 3);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_BoxMapping
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*int CXR_PathInterface::Path_GetNeighbourPortal(int _iPL1, int _iPL2)
{
	MAUTOSTRIP(CXR_PathInterface_Path_GetNeighbourPortal, 0);
	Error_static("CXR_PathInterface::Path_GetNeighbourPortal", "Should definitely not be running this code.)");
	return 0;
}
*/

// -------------------------------------------------------------------
//  CXR_BoxMapping
// -------------------------------------------------------------------
void CXR_BoxMapping::CreateUnit()
{
	MAUTOSTRIP(CXR_BoxMapping_CreateUnit, MAUTOSTRIP_VOID);
	m_Offset = 0.0f;	
	m_Scale = 1.0f;
	m_Rot = 0;
}

void CXR_BoxMapping::Create(const class CXR_PlaneMapping& _Plane, fp32 _Epsilon)
{
	MAUTOSTRIP(CXR_BoxMapping_Create, MAUTOSTRIP_VOID);
	const CVec3Dfp32& _U = _Plane.m_U;
	const CVec3Dfp32& _V = _Plane.m_V;
	CVec3Dfp32 n, u0, v0;
	_U.CrossProd(_V, n);

	if (M_Fabs(n.k[2] + _Epsilon) > M_Sqrt(Sqr(n.k[0]) + Sqr(n.k[1])))
	{
		u0 = CVec3Dfp32(1.0f, 0.0f, 0.0f);
		v0 = CVec3Dfp32(0.0f, -1.0f, 0.0f);
	}
	else
	{
		if (M_Fabs(n.k[0]) + _Epsilon > M_Fabs(n.k[1]))
		{
			u0 = CVec3Dfp32(0.0f, 1.0f, 0.0f);
			v0 = CVec3Dfp32(0.0f, 0.0f, -1.0f);
		}
		else
		{
			u0 = CVec3Dfp32(1.0f, 0.0f, 0.0f);
			v0 = CVec3Dfp32(0.0f, 0.0f, -1.0f);
		}
	}
	float ud0 = u0 * _U;
	float ud1 = _U.Length() * u0.Length();

	CVec3Dfp32 RefN;
	u0.CrossProd(v0, RefN);

	float RotU = M_ACos(Max(Min(ud0 / ud1, 1.0f), -1.0f)) / (2.0f * _PI);

	if ((_U / u0) * RefN < 0.0f) RotU = -RotU;

	m_Scale[0] = /*1.0f / */_U.Length();
	m_Scale[1] = /*1.0f / */_V.Length();
	m_Rot = RotU * 360.0f;
	if (n * RefN < 0.0f)
		m_Scale[1] = -m_Scale[1];

	m_Offset[0] = _Plane.m_UOffset;
	m_Offset[1] = _Plane.m_VOffset;
}

bool CXR_BoxMapping::AlmostEqual(const CXR_BoxMapping& _Map, fp32 _Epsilon) const
{
	MAUTOSTRIP(CXR_BoxMapping_AlmostEqual, false);
	if (M_Fabs(m_Rot - _Map.m_Rot) > _Epsilon) return false;
	if ((m_Offset - _Map.m_Offset).LengthSqr() > Sqr(_Epsilon)) return false;
	if ((m_Scale - _Map.m_Scale).LengthSqr() > Sqr(_Epsilon)) return false;
	return true;
}

void CXR_BoxMapping::Read(CCFile* _pFile)
{
	MAUTOSTRIP(CXR_BoxMapping_Read, MAUTOSTRIP_VOID);
	m_Offset.Read(_pFile);
	m_Scale.Read(_pFile);
	_pFile->ReadLE(m_Rot);
}

void CXR_BoxMapping::Write(CCFile* _pFile) const
{
	MAUTOSTRIP(CXR_BoxMapping_Write, MAUTOSTRIP_VOID);
	m_Offset.Write(_pFile);
	m_Scale.Write(_pFile);
	_pFile->WriteLE(m_Rot);
}

// -------------------------------------------------------------------
//  CXR_PlaneMapping
// -------------------------------------------------------------------
void CXR_PlaneMapping::CreateUnit()
{
	MAUTOSTRIP(CXR_PlaneMapping_CreateUnit, MAUTOSTRIP_VOID);
	m_U = CVec3Dfp32(1,0,0);
	m_V = CVec3Dfp32(0,1,0);
	m_UOffset = 0;
	m_VOffset = 0;
}

void CXR_PlaneMapping::Create(const class CXR_BoxMapping& _Mapping, const CPlane3Dfp64& _Plane)
{
	MAUTOSTRIP(CXR_PlaneMapping_Create, MAUTOSTRIP_VOID);
	CVec3Dfp32 u;
	CVec3Dfp32 v;
	if (M_Fabs(_Plane.n.k[2] + 0.001f) > M_Sqrt(Sqr(_Plane.n.k[0]) + Sqr(_Plane.n.k[1])))
	{
		u = CVec3Dfp32(1.0f, 0.0f, 0.0f);
		v = CVec3Dfp32(0.0f, -1.0f, 0.0f);
		RotateElements(u.k[0], u.k[1], fp32(-_Mapping.m_Rot/360.0f));
		RotateElements(v.k[0], v.k[1], fp32(-_Mapping.m_Rot/360.0f));
	}
	else
	{
		if (M_Fabs(_Plane.n.k[0]) + 0.001f > M_Fabs(_Plane.n.k[1]))
		{
			u = CVec3Dfp32(0.0f, 1.0f, 0.0f);
			v = CVec3Dfp32(0.0f, 0.0f, -1.0f);
			RotateElements(u.k[1], u.k[2], fp32(-_Mapping.m_Rot/360.0f));
			RotateElements(v.k[1], v.k[2], fp32(-_Mapping.m_Rot/360.0f));
		}
		else
		{
			u = CVec3Dfp32(1.0f, 0.0f, 0.0f);
			v = CVec3Dfp32(0.0f, 0.0f, -1.0f);
			RotateElements(u.k[0], u.k[2], fp32(-_Mapping.m_Rot/360.0f));
			RotateElements(v.k[0], v.k[2], fp32(-_Mapping.m_Rot/360.0f));
		}
	}
	u *= _Mapping.m_Scale.k[0];
	v *= _Mapping.m_Scale.k[1];

	m_U = u;
	m_V = v;
	m_UOffset = _Mapping.m_Offset.k[0];
	m_VOffset = _Mapping.m_Offset.k[1];
}

void CXR_PlaneMapping::Create(const class CXR_BoxMapping& _Mapping, const CPlane3Dfp32& _Plane)
{
	MAUTOSTRIP(CXR_PlaneMapping_Create_2, MAUTOSTRIP_VOID);
	CPlane3Dfp64 Planefp64(_Plane.n.Getfp64(), _Plane.d);
	Create(_Mapping, Planefp64);
}

int CXR_PlaneMapping::AlmostEqual(const CXR_PlaneMapping& _Map, fp32 _Epsilon) const
{
	MAUTOSTRIP(CXR_PlaneMapping_AlmostEqual, 0);
	fp32 EpsSqr = Sqr(_Epsilon);
	if ((m_U - _Map.m_U).LengthSqr() > EpsSqr) return 0;
	if ((m_V - _Map.m_V).LengthSqr() > EpsSqr) return 0;
	if (M_Fabs(m_UOffset - _Map.m_UOffset) > _Epsilon) return 0;
	if (M_Fabs(m_VOffset - _Map.m_VOffset) > _Epsilon) return 0;
	return 1;
}

void CXR_PlaneMapping::Read(CCFile* _pFile)
{
	MAUTOSTRIP(CXR_PlaneMapping_Read, MAUTOSTRIP_VOID);
	m_U.Read(_pFile);
	_pFile->ReadLE(m_UOffset);
	m_V.Read(_pFile);
	_pFile->ReadLE(m_VOffset);

//	m_ULengthRecp = 1.0f / m_U.Length();
//	m_VLengthRecp = 1.0f / m_V.Length();
}

void CXR_PlaneMapping::Write(CCFile* _pFile)
{
	MAUTOSTRIP(CXR_PlaneMapping_Write, MAUTOSTRIP_VOID);
	m_U.Write(_pFile);
	_pFile->WriteLE(m_UOffset);
	m_V.Write(_pFile);
	_pFile->WriteLE(m_VOffset);
}

void CXR_PlaneMapping::Struct_Read(CCFile* _pFile)
{
	MAUTOSTRIP(CXR_PlaneMapping_Struct_Read, MAUTOSTRIP_VOID);
	_pFile->Read(&m_U, sizeof(m_U)*2 + sizeof(m_UOffset)*2);
}

#ifndef CPU_LITTLEENDIAN
void CXR_PlaneMapping::Struct_SwapLE()
{
	MAUTOSTRIP(CXR_PlaneMapping_Struct_SwapLE, MAUTOSTRIP_VOID);
	m_U.SwapLE();
	SwapLE(m_UOffset);
	m_V.SwapLE();
	SwapLE(m_VOffset);

//	m_ULengthRecp = 1.0f / m_U.Length();
//	m_VLengthRecp = 1.0f / m_V.Length();
}
#endif

// -------------------------------------------------------------------
//  CXR_LightPosition
// -------------------------------------------------------------------
void CXR_LightPosition::SetDirection(const CVec3Dfp32& _Dir)
{
	_Dir.SetRow(m_Pos, 0);
}

const CVec3Dfp32& CXR_LightPosition::GetDirection() const
{
	return CVec3Dfp32::GetRow(m_Pos, 0);
}

void CXR_LightPosition::SetPosition(const CVec3Dfp32& _Pos)
{
	_Pos.SetRow(m_Pos, 3);
}

const CVec3Dfp32& CXR_LightPosition::GetPosition() const
{
	return CVec3Dfp32::GetRow(m_Pos, 3);
}

fp32 CXR_LightPosition::GetDistance(const CVec3Dfp32& _Pos) const
{
	return CVec3Dfp32::GetRow(m_Pos, 3).Distance(_Pos);
}

fp32 CXR_LightPosition::GetDistanceSqr(const CVec3Dfp32& _Pos) const
{
	return CVec3Dfp32::GetRow(m_Pos, 3).DistanceSqr(_Pos);
}

void CXR_LightPosition::Transform(const CMat4Dfp32& _Mat, CXR_LightPosition& _Dest) const
{
	_Dest.m_Flags = m_Flags;
	_Dest.m_Type = m_Type;
	CMat4Dfp32 Temp;
	m_Pos.Multiply(_Mat, _Dest.m_Pos);
	if (m_Flags & CXR_LIGHT_PROJMAPTRANSFORM)
		m_ProjMapTransform.Multiply(_Mat, _Dest.m_ProjMapTransform);
}

// -------------------------------------------------------------------
//  CXR_Light
// -------------------------------------------------------------------

int CXR_Light::ParseFlags(const char* _pStr)
{
	static const char *lpFlagsTranslate[] =
	{
		"NoShadows", "Flare", "LightFieldMap", "Enabled", "FlareDirectional", "ProjMapTransform", "Hint_Additive",
		"Hint_DontAffectModels", "AnimTime", "NoDiffuse", "NoSpecular", "RadiosityLight","FlareOnly", NULL
	};

	return CStr::TranslateFlags(_pStr, lpFlagsTranslate);
}

CXR_Light::CXR_Light()
{
	MAUTOSTRIP(CXR_Light_ctor, MAUTOSTRIP_VOID);
	m_Flags = CXR_LIGHT_ENABLED;
	m_LightGUID = 0;
	m_iLight = ~0;
	m_iLightf = -1.0f;
	m_Type = 0;
	m_Pos.Unit();
	m_Intensity = 1.0f;
//	m_IntensityInt32 = CPixel32(int(m_Intensity.k[0]*255.0f), int(m_Intensity.k[1]*255.0f), int(m_Intensity.k[2]*255.0f));
	m_Range = 512.0f;
	m_RangeInv = 1.0f/m_Range;
	m_SpotWidth = 1.0f;
	m_SpotHeight = 1.0f;
 	m_ProjMapID = 0;
	m_AnimTime = 0;
	m_pNext = NULL;
	m_pPVS = 0;
}

CXR_Light::CXR_Light(const CVec3Dfp32& _Pos, const CVec3Dfp32& _Intensity, fp32 _Range, int _Flags, int _Type)
{
	MAUTOSTRIP(CXR_Light_ctor_2, MAUTOSTRIP_VOID);
	m_Flags = CXR_LIGHT_ENABLED | _Flags;
	m_LightGUID = 0;
	m_iLight = ~0;
	m_iLightf = -1.0f;
	m_Type = _Type;
	m_Pos.Unit();
	_Pos.SetRow(m_Pos, 3);
	m_Intensity[0] = _Intensity[0];
	m_Intensity[1] = _Intensity[1];
	m_Intensity[2] = _Intensity[2];
	m_Intensity[3] = 1.0f;
//	m_IntensityInt32 = CPixel32(int(m_Intensity.k[0]*255.0f), int(m_Intensity.k[1]*255.0f), int(m_Intensity.k[2]*255.0f));
	m_Range = _Range;
	m_RangeInv = 1.0f/m_Range;
	m_SpotWidth = 1.0f;
	m_SpotHeight = 1.0f;
 	m_ProjMapID = 0;
	m_AnimTime = 0;

	m_pNext = NULL;
	m_pPVS = 0;
}

CXR_Light::CXR_Light(const CMat4Dfp32& _Pos, const CVec3Dfp32& _Intensity, fp32 _Range, int _Flags, int _Type)
{
	MAUTOSTRIP(CXR_Light_ctor_2, MAUTOSTRIP_VOID);
	m_Flags = CXR_LIGHT_ENABLED | _Flags;
	m_LightGUID = 0;
	m_iLight = ~0;
	m_iLightf = -1.0f;
	m_Type = _Type;
	m_Pos = _Pos;
	m_Intensity[0] = _Intensity[0];
	m_Intensity[1] = _Intensity[1];
	m_Intensity[2] = _Intensity[2];
	m_Intensity[3] = 1.0f;
//	m_IntensityInt32 = CPixel32(int(m_Intensity.k[0]*255.0f), int(m_Intensity.k[1]*255.0f), int(m_Intensity.k[2]*255.0f));
	m_Range = _Range;
	m_RangeInv = 1.0f/m_Range;
	m_SpotWidth = 1.0f;
	m_SpotHeight = 1.0f;
	m_AnimTime = 0;
 	m_ProjMapID = 0;

	m_pNext = NULL;
	m_pPVS = 0;
}

#ifndef M_RTM
bool CXR_Light::IsStaticMatch(CXR_Light *_pLight)
{
	if(m_Pos.AlmostEqual(_pLight->m_Pos, 0.01f) &&
	   m_Range == _pLight->m_Range &&
	   m_Type == _pLight->m_Type &&
	   m_SpotWidth == _pLight->m_SpotWidth &&
	   m_SpotHeight == _pLight->m_SpotHeight)
	   return true;

	return false;
}
#endif

void CXR_Light::SetProjectionMap(int _TextureID, const CMat4Dfp32* _pTransform)
{
	m_ProjMapID = _TextureID;
	if (m_ProjMapID && _pTransform)
	{
		m_ProjMapTransform = *_pTransform;
		m_Flags |= CXR_LIGHT_PROJMAPTRANSFORM;
	}
	else
		m_Flags &= ~CXR_LIGHT_PROJMAPTRANSFORM;
}

void CXR_Light::SetIntensity(const CVec3Dfp32& _Intensity)
{
	m_Intensity[0] = _Intensity[0];
	m_Intensity[1] = _Intensity[1];
	m_Intensity[2] = _Intensity[2];
	m_Intensity[3] = 1.0f;
//	m_IntensityInt32 = CPixel32(int(m_Intensity.k[0]*255.0f), int(m_Intensity.k[1]*255.0f), int(m_Intensity.k[2]*255.0f));
}

#ifndef PLATFORM_CONSOLE

#pragma  optimize("g", off)
TPtr<class CSolid> CXR_Light::CreateBoundSolid() const
{
	spCSolid spBound = MNew(CSolid);
	if (!spBound)
		Error_static("CXR_Light::CreateBoundSolid", "Out of memory.");

	switch(m_Type)
	{
	case CXR_LIGHTTYPE_POINT : 
		{
			spBound->CreateFromBox(GetPosition().Getfp64() - CVec3Dfp64(m_Range), GetPosition().Getfp64() + CVec3Dfp64(m_Range));
		}
		break;

	case CXR_LIGHTTYPE_SPOT : 
		{
			spBound->AddPlane(CPlane3Dfp64(CVec3Dfp64(1,0,0), -m_Range));
			spBound->AddPlane(CPlane3Dfp64(CVec3Dfp64(-m_SpotWidth, 1.0f, 0.0f).Normalize(), 0));
			spBound->AddPlane(CPlane3Dfp64(CVec3Dfp64(-m_SpotWidth, -1.0f, 0.0f).Normalize(), 0));
			spBound->AddPlane(CPlane3Dfp64(CVec3Dfp64(-m_SpotHeight, 0.0f, 1.0f).Normalize(), 0));
			spBound->AddPlane(CPlane3Dfp64(CVec3Dfp64(-m_SpotHeight, 0.0f, -1.0f).Normalize(), 0));
/*			spBound->AddPlane(CPlane3Dfp64(CVec3Dfp64(-_SQRT2,-_SQRT2,0), 0));
			spBound->AddPlane(CPlane3Dfp64(CVec3Dfp64(-_SQRT2,0,_SQRT2), 0));
			spBound->AddPlane(CPlane3Dfp64(CVec3Dfp64(-_SQRT2,0,-_SQRT2), 0));*/

			spBound->UpdateMesh();
			spBound->Apply(m_Pos, false);

			fp32 r = m_Range;
			spBound->AddPlane(CPlane3Dfp64(CVec3Dfp64(1, 0, 0), CVec3Dfp64(r, 0, 0) + GetPosition().Getfp64()));
			spBound->AddPlane(CPlane3Dfp64(CVec3Dfp64(-1, 0, 0), CVec3Dfp64(-r, 0, 0) + GetPosition().Getfp64()));
			spBound->AddPlane(CPlane3Dfp64(CVec3Dfp64(0, 1, 0), CVec3Dfp64(0, r, 0) + GetPosition().Getfp64()));
			spBound->AddPlane(CPlane3Dfp64(CVec3Dfp64(0, -1, 0), CVec3Dfp64(0, -r, 0) + GetPosition().Getfp64()));
			spBound->AddPlane(CPlane3Dfp64(CVec3Dfp64(0, 0, 1), CVec3Dfp64(0, 0, r) + GetPosition().Getfp64()));
			spBound->AddPlane(CPlane3Dfp64(CVec3Dfp64(0, 0, -1), CVec3Dfp64(0, 0, -r) + GetPosition().Getfp64()));

			spBound->UpdateMesh();

//			Error("CreateBoundSolid", "Spot not implemented yet.");
		}
		break;

	default :
		Error_static("CXR_Light::CreateBoundSolid", CStrF("Unsupported light type: %d", m_Type));
	}

	return spBound;
}
#pragma  optimize("g", on)

void CXR_Light::CalcBoundBox()
{
	spCSolid spBound = CreateBoundSolid();
	m_BoundBox.m_Min = spBound->m_BoundMin;
	m_BoundBox.m_Max = spBound->m_BoundMax;
}
#endif

void CXR_Light::CalcBoundBoxFast()
{
	switch(m_Type)
	{
	case CXR_LIGHTTYPE_SPOT : 
		{
			fp32 HalfRange = m_Range * 0.5f;
			CVec3Dfp32 Pos = GetPosition() + CVec3Dfp32::GetRow( m_Pos, 0 ) * HalfRange;
			m_BoundBox.m_Min[0] = Pos[0] - HalfRange;
			m_BoundBox.m_Min[1] = Pos[1] - HalfRange;
			m_BoundBox.m_Min[2] = Pos[2] - HalfRange;
			m_BoundBox.m_Max[0] = Pos[0] + HalfRange;
			m_BoundBox.m_Max[1] = Pos[1] + HalfRange;
			m_BoundBox.m_Max[2] = Pos[2] + HalfRange;
		}
		break;
		
	case CXR_LIGHTTYPE_POINT :		// FIXME: optimize spot box later
		{
			const CVec3Dfp32& Pos = GetPosition();
			fp32 r = m_Range;
			m_BoundBox.m_Min[0] = Pos[0] - r;
			m_BoundBox.m_Min[1] = Pos[1] - r;
			m_BoundBox.m_Min[2] = Pos[2] - r;
			m_BoundBox.m_Max[0] = Pos[0] + r;
			m_BoundBox.m_Max[1] = Pos[1] + r;
			m_BoundBox.m_Max[2] = Pos[2] + r;
		}
		break;

/*	case CXR_LIGHTTYPE_SPOT : 
		{
//			Error("CreateBoundSolid", "Spot not implemented yet.");
		}
		break;
*/
	default :
		Error_static("CXR_Light::CalcBoundBoxFast", CStrF("Unsupported light type: %d", m_Type));
	}

}

void CXR_Light::Transform(const CMat4Dfp32& _Mat)
{
	CMat4Dfp32 Temp;
	m_Pos.Multiply(_Mat, Temp);
	m_Pos = Temp;
	if (m_Flags & CXR_LIGHT_PROJMAPTRANSFORM)
	{
		m_ProjMapTransform.Multiply(_Mat, Temp);
		m_ProjMapTransform = Temp;
	}
}

void CXR_Light::Read(CCFile* _pFile, int _Version)
{
	CVec3Dfp32 I;
	switch(_Version)
	{
	case 0x0100 : 
		{
			_pFile->ReadLE(m_Flags);
			_pFile->ReadLE(m_Type);
			m_Pos.Read(_pFile);
			I.Read(_pFile);
			_pFile->ReadLE(m_Range);
			_pFile->ReadLE(m_AnimTime);
			_pFile->ReadLE(m_SpotWidth);
			_pFile->ReadLE(m_SpotHeight);
			m_BoundBox.Read(_pFile);
		}
		break;

	default :
		Error_static("CXR_Light::Read", CStrF("Unsupported version %.4x", _Version));
	}

	m_RangeInv = 1.0f / m_Range;
	SetIntensity(I);
//	m_IntensityInt32 = CPixel32::From_fp32(m_Intensity.k[0]*255.0f, m_Intensity.k[1]*255.0f, m_Intensity.k[2]*255.0f);
}

void CXR_Light::Write(CCFile* _pFile) const
{
	_pFile->WriteLE(m_Flags);
	_pFile->WriteLE(m_Type);
	m_Pos.Write(_pFile);
	_pFile->WriteLE(m_Intensity.k, 3);
	_pFile->WriteLE(m_Range);
	_pFile->WriteLE(m_AnimTime);
	_pFile->WriteLE(m_SpotWidth);
	_pFile->WriteLE(m_SpotHeight);
	m_BoundBox.Write(_pFile);
}

#ifndef CPU_LITTLEENDIAN
void CXR_Light::SwapLE()
{
	::SwapLE(m_Flags);
	::SwapLE(m_Type);
	m_Pos.SwapLE();
	m_Intensity.SwapLE();
	::SwapLE(m_Range);
	::SwapLE(m_AnimTime);
	::SwapLE(m_SpotWidth);
	::SwapLE(m_SpotHeight);
	m_BoundBox.SwapLE();

	m_RangeInv = 1.0f / m_Range;
//	m_IntensityInt32 = CPixel32::From_fp32(m_Intensity.k[0]*255.0f, m_Intensity.k[1]*255.0f, m_Intensity.k[2]*255.0f);
}
#endif

// -------------------------------------------------------------------
CXR_LightID::CXR_LightID()
{
	MAUTOSTRIP(CXR_LightID_ctor, MAUTOSTRIP_VOID);
	m_IntensityInt32 = 0xffffff;
}

// -------------------------------------------------------------------
//  CXR_WorldLightState
// -------------------------------------------------------------------

IMPLEMENT_OPERATOR_NEW(CXR_WorldLightState);


CXR_WorldLightState::CXR_WorldLightState()
{
	MAUTOSTRIP(CXR_WorldLightState_ctor, MAUTOSTRIP_VOID);
	m_nDynamic = 0;
	m_nStatic = 0;
}

CXR_WorldLightState::~CXR_WorldLightState()
{
	MAUTOSTRIP(CXR_WorldLightState_dtor, MAUTOSTRIP_VOID);
}

void CXR_WorldLightState::Create(int _nIDs, int _MaxDynamic, int _MaxStatic)
{
	MAUTOSTRIP(CXR_WorldLightState_Create, MAUTOSTRIP_VOID);
	MSCOPE(CXR_WorldLightState::Create, WORLDLIGHTSTATE);
	m_lDynamic.SetLen(_MaxDynamic);
	m_lStatic.SetLen(_MaxStatic);
//	m_lStatic.SetLen(96);
	m_lLightIDs.SetLen(_nIDs);
	m_nDynamic = 0;
	m_nStatic = 0;
}

void CXR_WorldLightState::PrepareFrame()
{
	MAUTOSTRIP(CXR_WorldLightState_PrepareFrame, MAUTOSTRIP_VOID);
	m_nDynamic = 0;
	m_nStatic = 0;
}

void CXR_WorldLightState::Set(int _LightID, const CVec3Dfp32& _Intensity)
{
	MAUTOSTRIP(CXR_WorldLightState_Set, MAUTOSTRIP_VOID);
	if ((_LightID <= 0) || (_LightID >= m_lLightIDs.Len())) return;

//	m_lLightIDs[_LightID].m_Intensity = _Intensity;
	m_lLightIDs[_LightID].m_IntensityInt32 = CPixel32::From_fp32(_Intensity.k[0]*255.0f, _Intensity.k[1]*255.0f, _Intensity.k[2]*255.0f);
}

void CXR_WorldLightState::AddDynamic(const CXR_Light& _Light)
{
	MAUTOSTRIP(CXR_WorldLightState_AddDynamic, MAUTOSTRIP_VOID);
	if ((m_nDynamic < 0) || (m_nDynamic >= m_lDynamic.Len()))
	{
		if (!m_lDynamic.Len())
			Error("AddDynamic", "Object has not been initialized.");
		return;
	}
	m_lDynamic[m_nDynamic] = _Light;
	m_nDynamic++;
}

void CXR_WorldLightState::AddDynamic(const CVec3Dfp32& _Pos, const CVec3Dfp32& _Intensity, fp32 _Range, int _Flags, int _Type)
{
	MAUTOSTRIP(CXR_WorldLightState_AddDynamic_2, MAUTOSTRIP_VOID);
	if ((m_nDynamic < 0) || (m_nDynamic >= m_lDynamic.Len()))
	{
		if (!m_lDynamic.Len())
			Error("AddDynamic", "Object has not been initialized.");
		return;
	}
	m_lDynamic[m_nDynamic] = CXR_Light(_Pos, _Intensity, _Range, _Flags, _Type);
	m_nDynamic++;
}

void CXR_WorldLightState::AddStatic(const CXR_Light& _Light)
{
	MAUTOSTRIP(CXR_WorldLightState_AddStatic, MAUTOSTRIP_VOID);
	if ((m_nStatic < 0) || (m_nStatic >= m_lStatic.Len())) 
	{
		if (!m_lStatic.Len())
			Error("AddStatic", "Object has not been initialized.");
		ConOut(CStrF("§cf80WARNING: Too many static lights. ( > %d)", m_nStatic));
		return; 
	}

	m_lStatic[m_nStatic] = _Light;
	m_nStatic++;
}

void CXR_WorldLightState::AddStatic(int _LightID, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Intensity, fp32 _Range, int _Flags, int _Type)
{
	MAUTOSTRIP(CXR_WorldLightState_AddStatic_2, MAUTOSTRIP_VOID);
	if ((m_nStatic < 0) || (m_nStatic >= m_lStatic.Len())) 
	{
		if (!m_lStatic.Len())
			Error("AddStatic", "Object has not been initialized.");
		ConOut(CStrF("§cf80WARNING: Too many static lights. ( > %d)", m_nStatic));
		return; 
	}
	if ((_LightID < 0) || (_LightID >= m_lLightIDs.Len())) return;

	CVec3Dfp32 Intens(
		(fp32)(m_lLightIDs[_LightID].m_IntensityInt32.GetR())/255.0f * _Intensity.k[0],
		(fp32)(m_lLightIDs[_LightID].m_IntensityInt32.GetG())/255.0f * _Intensity.k[1],
		(fp32)(m_lLightIDs[_LightID].m_IntensityInt32.GetB())/255.0f * _Intensity.k[2]);
	m_lStatic[m_nStatic] = CXR_Light(_Pos, Intens, _Range, _Flags, _Type);
	m_nStatic++;
}

void CXR_WorldLightState::AddLightVolume(CXR_LightVolume* _pLightVolume, const CVec3Dfp32& _ReferencePos)
{
	MAUTOSTRIP(CXR_WorldLightState_AddLightVolume, MAUTOSTRIP_VOID);
	while(_pLightVolume)
	{
		if ((m_nStatic < 0) || (m_nStatic >= m_lStatic.Len())) 
		{
			if (!m_lStatic.Len())
				Error("AddLightVolume", "Object has not been initialized.");
			ConOut(CStrF("§cf80WARNING: Too many static lights. ( > %d)", m_nStatic));
			return; 
		}

		CXR_Light& Light = m_lStatic[m_nStatic];
		m_nStatic++;

		CVec3Dfp32 LightDir;
		CVec3Dfp32 LightI;
		fp32 Bias = _pLightVolume->Light_EvalVertex(m_lLightIDs.GetBasePtr(), _ReferencePos, LightDir, LightI);
		Light.SetDirection(LightDir);
		Light.SetIntensity(LightI);
//		Light.m_IntensityInt32 = CPixel32::From_fp32(Light.m_Intensity.k[0]*255.0f, Light.m_Intensity.k[1]*255.0f, Light.m_Intensity.k[2]*255.0f);
		Light.m_Type = CXR_LIGHTTYPE_LIGHTVOLUME;
		Light.m_Flags = 0;
		Light.m_Range = Bias;
		Light.m_ProjMapID = 0;
		Light.m_pNext = NULL;

//		Light.m_LightVec *= 0.6f;
//		Light.m_Range /= 0.6f;

//ConOut(CStrF("LightVolume %.8x, %f, %s, %s", _pLightVolume, Bias, (char*)Light.m_LightVec.GetString(), (char*)Light.m_Intensity.GetString()));
		_pLightVolume = _pLightVolume->GetNext();
	}
}

void CXR_WorldLightState::InitLinks()
{
	MAUTOSTRIP(CXR_WorldLightState_InitLinks, MAUTOSTRIP_VOID);
	CXR_Light* pLast = NULL;
	int i;
	for(i = 0; i < m_nDynamic; i++)
	{
		if (pLast) pLast->m_pNext = &m_lDynamic[i];
		pLast = &m_lDynamic[i];
	}

	for(i = 0; i < m_nStatic; i++)
	{
		if (pLast) pLast->m_pNext = &m_lStatic[i];
		pLast = &m_lStatic[i];
	}

	if (pLast) pLast->m_pNext = NULL;
}

CXR_Light* CXR_WorldLightState::GetFirst()
{
	MAUTOSTRIP(CXR_WorldLightState_GetFirst, NULL);
	if (m_nDynamic) return &m_lDynamic[0];
	if (m_nStatic) return &m_lStatic[0];
	return NULL;
}

void CXR_WorldLightState::CopyAndCull(const CXR_WorldLightState* _pWLS, CXR_ViewClipInterface* _pView)
{
	MAUTOSTRIP(CXR_WorldLightState_CopyAndCull, MAUTOSTRIP_VOID);
//	if (_pView)
//		ConOut(CStrF("Static %d, Dynamic %d", _pWLS->m_nStatic, _pWLS->m_nDynamic));

	CXR_Light* pLast = NULL;
	m_nDynamic = 0;
	int i;
	for(i = 0; i < _pWLS->m_nDynamic; i++)
	{
		const CXR_Light* pL = &_pWLS->m_lDynamic[i];
		if (!_pView || _pView->View_GetClip_Sphere(pL->GetPosition(), pL->m_Range, -1, -1, NULL, NULL))
		{
			if (m_nDynamic < m_lDynamic.Len())
			{
				m_lDynamic[m_nDynamic] = *pL;
				if (pLast) pLast->m_pNext = &m_lDynamic[m_nDynamic];
				pLast = &m_lDynamic[m_nDynamic];
				m_nDynamic++;
			}
		}
	}

	m_nStatic = 0;
	for(i = 0; i < _pWLS->m_nStatic; i++)
	{
		const CXR_Light* pL = &_pWLS->m_lStatic[i];
		if (!_pView || _pView->View_GetClip_Sphere(pL->GetPosition(), pL->m_Range, -1, -1, NULL, NULL))
		{
			if (m_nStatic < m_lStatic.Len())
			{
				m_lStatic[m_nStatic] = *pL;
				if (pLast) pLast->m_pNext = &m_lStatic[m_nStatic];
				pLast = &m_lStatic[m_nStatic];
				m_nStatic++;
			}
		}
	}

	if (pLast) pLast->m_pNext = NULL;

	m_lLightIDs = _pWLS->m_lLightIDs;	// List-sharing to avoid data copy. This is why you should always have a
										// hard-hit'n list-class up your sleve.  ;-)
}

void CXR_WorldLightState::CopyAndCull(const CXR_WorldLightState* _pWLS, fp32 _BoundR, const CVec3Dfp32 _BoundPos, int _MaxStatic, int _MaxDynamic)
{
	MAUTOSTRIP(CXR_WorldLightState_CopyAndCull_2, MAUTOSTRIP_VOID);
	
	_MaxStatic = Min(m_lStatic.Len(), _MaxStatic);
	_MaxDynamic = Min(m_lDynamic.Len(), _MaxDynamic);

	m_nDynamic = 0;
	int i;
	for(i = 0; i < _pWLS->m_nDynamic; i++)
	{
		const CXR_Light* pL = &_pWLS->m_lDynamic[i];
		CXR_Light* pLights = m_lDynamic.GetBasePtr();
		CVec3Dfp32 lv;
		pL->GetPosition().Sub(_BoundPos, lv);
		fp32 DistSqr = lv.LengthSqr();

		if (DistSqr < Sqr(pL->m_Range + _BoundR))
		{
			fp32 Dist = Min(pL->m_Range, (fp32) M_Sqrt(DistSqr)) * pL->m_RangeInv;
			CVec4Dfp32 LightColor = pL->GetIntensityv();
			fp32 Intens = LightColor[0] + 2.0f * LightColor[1] + LightColor[2];
			fp32 Scale = Intens * (1.0f - Dist);

			if(m_nDynamic == _MaxDynamic)
			{
				for(int i = 0; i < m_nDynamic; i++)
					if (pLights[i].m_SortVal < Scale)
					{
						pLights[i] = *pL;
						pLights[i].m_SortVal = Scale;
						break;
					}
				continue;
			}
			else
			{
				pLights[m_nDynamic] = *pL;
				pLights[m_nDynamic].m_SortVal = Scale;
			}
			m_nDynamic++;
		}
	}



	m_nStatic = 0;
	for(i = 0; i < _pWLS->m_nStatic; i++)
	{
		const CXR_Light* pL = &_pWLS->m_lStatic[i];
		CXR_Light* pLights = m_lStatic.GetBasePtr();
		CVec3Dfp32 lv;
		pL->GetPosition().Sub(_BoundPos, lv);
		fp32 DistSqr = lv.LengthSqr();

		if (DistSqr < Sqr(pL->m_Range + _BoundR))
		{
			fp32 Dist = Min(pL->m_Range, (fp32) M_Sqrt(DistSqr)) * pL->m_RangeInv;
			CVec4Dfp32 LightColor = pL->GetIntensityv();
			fp32 Intens = LightColor[0] + 2.0f * LightColor[1] + LightColor[2];
			fp32 Scale = Intens * (1.0f - Dist);

			if(m_nStatic == _MaxStatic)
			{
				for(int i = 0; i < m_nStatic; i++)
					if (pLights[i].m_SortVal < Scale)
					{
						pLights[i] = *pL;
						pLights[i].m_SortVal = Scale;
						break;
					}
				continue;
			}
			else
			{
				pLights[m_nStatic] = *pL;
				pLights[m_nStatic].m_SortVal = Scale;
			}

			m_nStatic++;
		}
	}

	CXR_Light* pLast = NULL;
	for(i = 0; i < m_nDynamic; i++)
	{
		if (pLast) pLast->m_pNext = &m_lDynamic[i];
		pLast = &m_lDynamic[i];
	}
	for(i = 0; i < m_nStatic; i++)
	{
		if (pLast) pLast->m_pNext = &m_lStatic[i];
		pLast = &m_lStatic[i];
	}

	if (pLast) pLast->m_pNext = NULL;

	m_lLightIDs = _pWLS->m_lLightIDs;
}

void CXR_WorldLightState::Transform(const CMat4Dfp32& _Mat)
{
	InitLinks();

	CXR_Light* pL = GetFirst();
	while(pL)
	{
		CMat4Dfp32 M;
		pL->m_Pos.Multiply(_Mat, M);
		pL->m_Pos = M;

		pL = pL->m_pNext;
	}
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CCollisionInfo
|__________________________________________________________________________________________________
\*************************************************************************************************/

void CCollisionInfo::Clear()
{
	MAUTOSTRIP(CCollisionInfo_Clear, MAUTOSTRIP_VOID);
	m_bIsValid = false;
	m_bIsCollision = false;
	m_IN1N2Flags = 0;
	m_CollisionType = CXR_COLLISIONTYPE_PHYSICS;
	m_ReturnValues = ~0;
	m_iObject = ~0;
	m_pSurface = NULL;
	m_LocalNode = 0;
	m_Time = 0;
	m_Distance = 0;
	m_SurfaceType = 0;
	m_Friction = 0.4f;
}

CCollisionInfo::CCollisionInfo()
{
	MAUTOSTRIP(CCollisionInfo_ctor, MAUTOSTRIP_VOID);
	Clear();
}

void CCollisionInfo::SetReturnValues(int _Mask)
{
	if (_Mask & (CXR_COLLISIONRETURNVALUE_PENETRATIONDEPTH | CXR_COLLISIONRETURNVALUE_SURFACE | CXR_COLLISIONRETURNVALUE_SURFACEVELOCITY | CXR_COLLISIONRETURNVALUE_LOCALPOSITION))
		_Mask |= CXR_COLLISIONRETURNVALUE_POSITION;

	if (_Mask & CXR_COLLISIONRETURNVALUE_POSITION)
		_Mask |= CXR_COLLISIONRETURNVALUE_TIME;

	m_ReturnValues = _Mask;
}

void CCollisionInfo::CopyParams(const CCollisionInfo& _CInfo)
{
	m_ReturnValues = _CInfo.m_ReturnValues;
	m_CollisionType = _CInfo.m_CollisionType;
}


void CCollisionInfo::CopyReturnValues(const CCollisionInfo& _CInfo)
{
	MAUTOSTRIP(CCollisionInfo_CopyReturnValues, MAUTOSTRIP_VOID);
	m_bIsValid = _CInfo.m_bIsValid;
	m_bIsCollision = _CInfo.m_bIsCollision;

	if (m_ReturnValues & CXR_COLLISIONRETURNVALUE_PENETRATIONDEPTH)
	{
		m_Pos = _CInfo.m_Pos;
		m_Plane = _CInfo.m_Plane;
		m_iObject = _CInfo.m_iObject;
		m_Time = _CInfo.m_Time;
		m_Distance = _CInfo.m_Distance;
	}
	else
	{
		if (m_ReturnValues & CXR_COLLISIONRETURNVALUE_TIME)
		{
			m_iObject = _CInfo.m_iObject;
			m_Time = _CInfo.m_Time;
		}
		if (m_ReturnValues & CXR_COLLISIONRETURNVALUE_POSITION)
		{
			m_Pos = _CInfo.m_Pos;
			m_Plane = _CInfo.m_Plane;
		}
	}
	if (m_ReturnValues & CXR_COLLISIONRETURNVALUE_SURFACE)
	{
		m_pSurface = _CInfo.m_pSurface;
		m_SurfaceType = _CInfo.m_SurfaceType;
	}
	if (m_ReturnValues & CXR_COLLISIONRETURNVALUE_SURFACEVELOCITY)
	{
		m_Velocity = _CInfo.m_Velocity;
		m_RotVelocity = _CInfo.m_RotVelocity;
	}
	if (m_ReturnValues & CXR_COLLISIONRETURNVALUE_LOCALPOSITION)
	{
		m_LocalPos = _CInfo.m_LocalPos;
		m_LocalNode = _CInfo.m_LocalNode;
		m_LocalNodePos = _CInfo.m_LocalNodePos;
	}
}


bool CCollisionInfo::Improve(const CCollisionInfo& _CInfo)
{
	MAUTOSTRIP(CCollisionInfo_Improve, false);


	if( IsImprovement( _CInfo ) )
	{
//		*this = _CInfo;
		CopyReturnValues(_CInfo);
		return true;
	}

	return false;
}

bool CCollisionInfo::IsImprovement(const CCollisionInfo& _CInfo) const
{
	MAUTOSTRIP(CCollisionInfo_IsImprovment, false);
	if (!m_bIsCollision)
		return true;
	if (!_CInfo.m_bIsCollision)
		return false;

	if (!_CInfo.m_bIsValid)
		return true;
	if (!m_bIsValid)
		return false;

	if (_CInfo.m_Time < m_Time)
		return true;
	if (_CInfo.m_Time == m_Time &&
		_CInfo.m_Distance < m_Distance)
		return true;

	return false;
}

bool CCollisionInfo::IsImprovement(fp32 _Time, fp32 _Distance) const
{
	if (!m_bIsCollision)
		return true;

	if (!m_bIsValid)
		return false;

	if (_Time < m_Time)
		return true;

	// I don't even remember in what case this made a difference. _Time == m_Time is a rather unlikely situation.
	if (_Time == m_Time &&
		_Distance < m_Distance)
		return true;

	return false;
}

bool CCollisionInfo::IsComplete() const
{
	if (!m_bIsCollision)
		return false;
	if (!m_bIsValid)
		return true;
	if (!m_ReturnValues)
		return true;
	return false;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model
|__________________________________________________________________________________________________
\*************************************************************************************************/
CXR_Model::CXR_Model()
{
	MAUTOSTRIP(CXR_Model_ctor, MAUTOSTRIP_VOID);
	SetThreadSafe(false);
}

fp32 CXR_Model::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{ 
	MAUTOSTRIP(CXR_Model_GetBound_Sphere, 0.0f);
	return 0;
};

void CXR_Model::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_GetBound_Box, MAUTOSTRIP_VOID);
//	M_ASSERT(false, "GetBound_Box not implemented!");
	_Box.m_Min = 0;
	_Box.m_Max = 0;
};

void CXR_Model::GetBound_Box(CBox3Dfp32& _Box, int _Mask, const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_GetBound_Box, MAUTOSTRIP_VOID);
	CXR_Model::GetBound_Box(_Box, _pAnimState);
};

void* CXR_Model::GetInterface(int _Interface)
{
	MAUTOSTRIP(CXR_Model_GetInterface, NULL);
	switch(_Interface)
	{
	case CXR_MODEL_INTERFACE_SCENEGRAPH	:	return SceneGraph_GetInterface();
	case CXR_MODEL_INTERFACE_VIEWCLIP :		return View_GetInterface();
	case CXR_MODEL_INTERFACE_PHYSICS :		return Phys_GetInterface();
	case CXR_MODEL_INTERFACE_PATH :			return Path_GetInterface();
	case CXR_MODEL_INTERFACE_FOG :			return Fog_GetInterface();
	case CXR_MODEL_INTERFACE_WALLMARK :		return Wallmark_GetInterface();
	case CXR_MODEL_INTERFACE_SKY :			return Sky_GetInterface();
	default :								return NULL;
	}
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_VariationProxy
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_VariationProxy, CXR_Model);

void CXR_Model_VariationProxy::Create(spCXR_Model _spModel, int _iVariation)
{
	M_ASSERT(_spModel != NULL, "CXR_Model_VariationProxy: Invalid model!");
	m_spModel = _spModel;
	m_iVariation = _iVariation;
}


void CXR_Model_VariationProxy::Create(const char* _pParam)
{
	m_spModel->Create(_pParam);
}

void CXR_Model_VariationProxy::Create(const char* _pParam, CDataFile* _pDFile, CCFile* _pFile)
{
	m_spModel->Create(_pParam, _pDFile, _pFile);
}

int CXR_Model_VariationProxy::GetModelClass()
{
	return m_spModel->GetModelClass();
}

int CXR_Model_VariationProxy::GetRenderPass(const CXR_AnimState* _pAnimState)
{
	if (!_pAnimState)
		return m_spModel->GetRenderPass(_pAnimState);

	CXR_AnimState Anim(*_pAnimState);
	Anim.m_Variation = m_iVariation;
	return m_spModel->GetRenderPass(&Anim);
}

int CXR_Model_VariationProxy::GetVariationIndex(const char* _pName)
{
	return m_spModel->GetVariationIndex(_pName);
}

int CXR_Model_VariationProxy::GetNumVariations()
{
	return m_spModel->GetNumVariations();
}

#ifndef M_RTMCONSOLE
CStr CXR_Model_VariationProxy::GetVariationName(int _iVariation)
{
	return m_spModel->GetVariationName(_iVariation);
}
#endif

CXR_Model* CXR_Model_VariationProxy::OnResolveVariationProxy(const CXR_AnimState& _AnimState, CXR_AnimState& _DstAnimState)
{
	_DstAnimState = _AnimState;
	_DstAnimState.m_Variation = m_iVariation;
	return m_spModel;
}

CXR_Model* CXR_Model_VariationProxy::GetLOD(const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, CXR_Engine* _pEngine, int *_piLod)
{
	return m_spModel->GetLOD(_WMat, _VMat, _pEngine, _piLod);
}

CXR_Skeleton* CXR_Model_VariationProxy::GetSkeleton()
{
	return m_spModel->GetSkeleton();
}

CXR_Skeleton* CXR_Model_VariationProxy::GetPhysSkeleton()
{
	return m_spModel->GetPhysSkeleton();
}

aint CXR_Model_VariationProxy::GetParam(int _Param)
{
	return m_spModel->GetParam(_Param);
}

void CXR_Model_VariationProxy::SetParam(int _Param, aint _Value)
{
	m_spModel->SetParam(_Param, _Value);
}

int CXR_Model_VariationProxy::GetParamfv(int _Param, fp32* _pRetValues)
{
	return m_spModel->GetParamfv(_Param, _pRetValues);
}

void CXR_Model_VariationProxy::SetParamfv(int _Param, const fp32* _pValues)
{
	return m_spModel->SetParamfv(_Param, _pValues);
}

fp32 CXR_Model_VariationProxy::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{
	if (!_pAnimState)
		return m_spModel->GetBound_Sphere(_pAnimState);

	CXR_AnimState Anim(*_pAnimState);
	Anim.m_Variation = m_iVariation;
	return m_spModel->GetBound_Sphere(&Anim);
}

void CXR_Model_VariationProxy::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	if (!_pAnimState)
	{
		m_spModel->GetBound_Box(_Box);
	}
	else
	{
		CXR_AnimState Anim(*_pAnimState);
		Anim.m_Variation = m_iVariation;
		m_spModel->GetBound_Box(_Box, &Anim);
	}
}

void CXR_Model_VariationProxy::GetBound_Box(CBox3Dfp32& _Box, int _Mask, const CXR_AnimState* _pAnimState)
{
	if (!_pAnimState)
	{
		m_spModel->GetBound_Box(_Box, _Mask);
	}
	else
	{
		CXR_AnimState Anim(*_pAnimState);
		Anim.m_Variation = m_iVariation;
		m_spModel->GetBound_Box(_Box, _Mask, &Anim);
	}
}

void CXR_Model_VariationProxy::PreRender(CXR_Engine* _pEngine, CXR_ViewClipInterface* _pViewClip,
	const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	if (!_pAnimState)
		return;

	CXR_AnimState Anim(*_pAnimState);
	Anim.m_Variation = m_iVariation;
	m_spModel->PreRender(_pEngine, _pViewClip, &Anim, _WMat, _VMat, _Flags);
}

void CXR_Model_VariationProxy::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, 
	CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
	const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	if (!_pAnimState)
		return;

	CXR_AnimState Anim(*_pAnimState);
	Anim.m_Variation = m_iVariation;

	m_spModel->OnRender(_pEngine, _pRender, _pVBM, _pViewClip, _spWLS, &Anim, _WMat, _VMat, _Flags);
}

void CXR_Model_VariationProxy::OnPrecache(CXR_Engine* _pEngine, int _iVariation)
{
	//TODO: Only precache selected variation, if any.. (CXR_Model_TriangleMesh currently precaches all variations)
	return m_spModel->OnPrecache(_pEngine, m_iVariation);
}

void CXR_Model_VariationProxy::OnPostPrecache(CXR_Engine* _pEngine)
{
	return m_spModel->OnPostPrecache(_pEngine);
}

void CXR_Model_VariationProxy::OnRefreshSurfaces()
{
	return m_spModel->OnRefreshSurfaces();
}

void CXR_Model_VariationProxy::OnResourceRefresh(int _Flags)
{
	return m_spModel->OnResourceRefresh(_Flags);
}

void CXR_Model_VariationProxy::OnHibernate(int _Flags)
{
	return m_spModel->OnHibernate(_Flags);
}

void* CXR_Model_VariationProxy::GetInterface(int _Interface)
{
	return m_spModel->GetInterface(_Interface);
}

CXR_SceneGraphInterface* CXR_Model_VariationProxy::SceneGraph_GetInterface()
{
	return m_spModel->SceneGraph_GetInterface();
}

CXR_ViewClipInterface* CXR_Model_VariationProxy::View_GetInterface()
{
	return m_spModel->View_GetInterface();
}

CXR_PhysicsModel* CXR_Model_VariationProxy::Phys_GetInterface()
{
	return m_spModel->Phys_GetInterface();
}

CXR_PathInterface* CXR_Model_VariationProxy::Path_GetInterface()
{
	return m_spModel->Path_GetInterface();
}

CXR_FogInterface* CXR_Model_VariationProxy::Fog_GetInterface()
{
	return m_spModel->Fog_GetInterface();
}

CXR_WallmarkInterface* CXR_Model_VariationProxy::Wallmark_GetInterface()
{
	return m_spModel->Wallmark_GetInterface();
}

CXR_SkyInterface* CXR_Model_VariationProxy::Sky_GetInterface()
{
	return m_spModel->Sky_GetInterface();
}

TPtr<CXR_ModelInstance> CXR_Model_VariationProxy::CreateModelInstance()
{
	return m_spModel->CreateModelInstance();
}

bool CXR_Model_VariationProxy::NeedRefresh()
{
	return m_spModel->NeedRefresh();
}



// -------------------------------------------------------------------
//  These should be removed and made pure virtual.
// -------------------------------------------------------------------
void CXR_ViewClipInterface::View_Reset(int _iView)
{
	MAUTOSTRIP(CXR_ViewClipInterface_View_Reset, MAUTOSTRIP_VOID);
}

void CXR_ViewClipInterface::View_SetState(int _iView, int _State, int _Value)
{
	MAUTOSTRIP(CXR_ViewClipInterface_View_SetState, MAUTOSTRIP_VOID);
}

// -------------------------------------------------------------------
//  CXR_PhysicsModel
// -------------------------------------------------------------------
void CXR_PhysicsModel::Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32* _pV, int _nV, uint32* _pRetMediums)
{
	MAUTOSTRIP(CXR_PhysicsModel_Phys_GetMedium, MAUTOSTRIP_VOID);
	for(int v = 0; v < _nV; v++)
		_pRetMediums[v] = Phys_GetMedium(_pPhysContext, _pV[v]);
}

void CXR_PhysicsModel::Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32* _pV, int _nV, CXR_MediumDesc* _pRetMediums)
{
	MAUTOSTRIP(CXR_PhysicsModel_Phys_GetMedium_2, MAUTOSTRIP_VOID);
	for(int v = 0; v < _nV; v++)
		Phys_GetMedium(_pPhysContext, _pV[v], _pRetMediums[v]);
}

// -------------------------------------------------------------------
//  CXR_PhysicsModel_Sphere
// -------------------------------------------------------------------
void CXR_PhysicsModel_Sphere::Phys_SetDimensions(fp32 _Radius)
{
	MAUTOSTRIP(CXR_PhysicsModel_Sphere_Phys_SetDimensions, MAUTOSTRIP_VOID);
	m_PhysRadius = _Radius;
}

void CXR_PhysicsModel_Sphere::Phys_GetBound_Sphere(const CMat4Dfp32& _Pos, CVec3Dfp32& _RetPos, fp32& _Radius)
{
	MAUTOSTRIP(CXR_PhysicsModel_Sphere_Phys_GetBound_Sphere, MAUTOSTRIP_VOID);
	_RetPos = CVec3Dfp32::GetMatrixRow(_Pos, 3);
	_Radius = m_PhysRadius;
}

void CXR_PhysicsModel_Sphere::Phys_GetBound_Box(const CMat4Dfp32& _Pos, CBox3Dfp32& _RetBox)
{
	MAUTOSTRIP(CXR_PhysicsModel_Sphere_Phys_GetBound_Box, MAUTOSTRIP_VOID);
	fp32 r = m_PhysRadius + 0.0001f;
	_RetBox.m_Min = _Pos.k[3][0] - r;
	_RetBox.m_Min = _Pos.k[3][1] - r;
	_RetBox.m_Min = _Pos.k[3][2] - r;
	_RetBox.m_Max = _Pos.k[3][0] + r;
	_RetBox.m_Max = _Pos.k[3][1] + r;
	_RetBox.m_Max = _Pos.k[3][2] + r;
}

void CXR_PhysicsModel_Sphere::Phys_Init(CXR_PhysicsContext* _pPhysContext)
{
	MAUTOSTRIP(CXR_PhysicsModel_Sphere_Phys_Init, MAUTOSTRIP_VOID);
//	m_PhysWMat = _WMat;
}

int CXR_PhysicsModel_Sphere::Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0)
{
	MAUTOSTRIP(CXR_PhysicsModel_Sphere_Phys_GetMedium, 0);
	return XW_MEDIUM_AIR | XW_MEDIUM_SEETHROUGH;
};

void CXR_PhysicsModel_Sphere::Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, CXR_MediumDesc& _RetMedium)
{
	MAUTOSTRIP(CXR_PhysicsModel_Sphere_Phys_GetMedium_2, MAUTOSTRIP_VOID);
	_RetMedium.SetAir();
};

bool CXR_PhysicsModel_Sphere::Phys_IntersectLine(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_PhysicsModel_Sphere_Phys_IntersectLine, false);

	const CVec3Dfp32& SpherePos = _pPhysContext->m_WMat.GetRow(3);
	CVec3Dfp32 Origin = _v0 - SpherePos;
	CVec3Dfp32 Dir = _v1 - _v0;

	fp32 a = Dir * Dir;
	fp32 b = 2.0f * (Dir * Origin);
	fp32 c = (Origin * Origin) - Sqr(m_PhysRadius);

	if (c < 0.0f) // check if v0 is inside sphere
	{
		if (_pCollisionInfo)
		{
			_pCollisionInfo->m_bIsValid = false;
			_pCollisionInfo->m_bIsCollision = true;
		}
		return true;
	}

	fp32 x = b*b - 4.0f*a*c;
	if (x >= 0.0f)				// Real solution?
	{
 		fp32 Base = -b;
		fp32 Root = M_Sqrt(x);

		fp32 t = _FP32_MAX;
		if (Base > Root)
			t = Base - Root;	// First root is above zero
		else if (-Base < Root)
			t = Base + Root;	// Second root is above zero
		else
			return false;		// Both are below zero

		fp32 den = 2.0f*a;
		if (t > den)
			return false;		// Root is beyond one

		if (_pCollisionInfo)
		{
			_pCollisionInfo->m_bIsValid = true;
			_pCollisionInfo->m_bIsCollision = true;

			if (_pCollisionInfo->m_ReturnValues & CXR_COLLISIONRETURNVALUE_TIME)
			{
				t /= den;
				_pCollisionInfo->m_Time = t;						// return: t [0,1]
				_pCollisionInfo->m_Distance = t * M_Sqrt(a);		// return: distance [0,length(v1-t0)]

				if (_pCollisionInfo->m_ReturnValues & (CXR_COLLISIONRETURNVALUE_LOCALPOSITION | CXR_COLLISIONRETURNVALUE_POSITION))
				{
					CVec3Dfp32 Pos;
					_v0.Combine(Dir, t, Pos);	
					_pCollisionInfo->m_Pos = Pos;					// return: global pos

					CVec3Dfp32 Normal = Pos - SpherePos; 
					Normal.Normalize();
					_pCollisionInfo->m_Plane.CreateNV(Normal, Pos);	// return: plane

					if (_pCollisionInfo->m_ReturnValues & CXR_COLLISIONRETURNVALUE_LOCALPOSITION)
					{
						_pCollisionInfo->m_LocalPos = Pos;
						_pCollisionInfo->m_LocalPos *= _pPhysContext->m_WMatInv;	// return: local pos
					}
				}
				_pCollisionInfo->m_pSurface = NULL;
			}
		}
		return true;
	}
	return false;
}

bool CXR_PhysicsModel_Sphere::Phys_IntersectSphere(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_PhysicsModel_Sphere_Phys_IntersectSphere, false);
	fp32 dsqr = Sqr(_pPhysContext->m_WMat.k[3][0] - _Dest.k[0]) + Sqr(_pPhysContext->m_WMat.k[3][1] - _Dest.k[1]) + Sqr(_pPhysContext->m_WMat.k[3][2] - _Dest.k[2]);
	if (dsqr < Sqr(m_PhysRadius + _Radius))
	{
		if (!_pCollisionInfo)
			return true;

		_pCollisionInfo->m_bIsValid = true;
		_pCollisionInfo->m_bIsCollision = true;

		const CVec3Dfp32& p = _pPhysContext->m_WMat.GetRow(3);
		_pCollisionInfo->m_Distance = M_Sqrt(dsqr) - (m_PhysRadius + _Radius);
		CVec3Dfp32 n = (_Dest - p).Normalize();
		_pCollisionInfo->m_Plane.d = -m_PhysRadius;
		_pCollisionInfo->m_Plane.n = n;
		_pCollisionInfo->m_Pos = n*m_PhysRadius;

	// FIXME: Hay, m_Time is not calculated. -mh
	//	ConOutL("(CXR_PhysicsModel_Sphere::Phys_IntersectSphere) FIXME: Hay, m_Time is not calculated. -mh");
		if (_pCollisionInfo->m_ReturnValues & CXR_COLLISIONRETURNVALUE_TIME)
		{
			// This isn't really correct, but it's probably better than nothing  -ar
			CVec3Dfp32 Dir = (_Dest - _Origin);
			fp32 neg_dt = (Dir * n) * _pCollisionInfo->m_Distance / Dir.LengthSqr();
			_pCollisionInfo->m_Time = Clamp01(1.0f + neg_dt);
		}

		if (_pCollisionInfo->m_ReturnValues & CXR_COLLISIONRETURNVALUE_LOCALPOSITION)
		{
			_pCollisionInfo->m_LocalPos = n*m_PhysRadius;
			_pCollisionInfo->m_LocalPos *= _pPhysContext->m_WMatInv;
		}
		_pCollisionInfo->m_Velocity = 0;
		return true;
	}
	return false;
}


bool CXR_PhysicsModel_Sphere::Phys_IntersectBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _BoxOrigin, const CPhysOBB& _BoxDest, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_PhysicsModel_Sphere_Phys_IntersectBox, false);
	CVec3Dfp32 Pos;
	_BoxDest.TransformToBoxSpace(CVec3Dfp32::GetMatrixRow(_pPhysContext->m_WMat, 3), Pos);

	if (!_pCollisionInfo)
		return _BoxDest.GetMinSqrDistance(Pos) < Sqr(m_PhysRadius);
	else
	{
		if (_BoxDest.GetMinSqrDistance(Pos) < Sqr(m_PhysRadius))
		{
			_pCollisionInfo->m_bIsValid = false;
			_pCollisionInfo->m_bIsCollision = true;
			return true;
		}
	}

	return false;
}


// -------------------------------------------------------------------
//  CXR_PhysicsModel_Box
// -------------------------------------------------------------------
void CXR_PhysicsModel_Box::Phys_SetDimensions(const CVec3Dfp32& _Dim)
{
	MAUTOSTRIP(CXR_PhysicsModel_Box_Phys_SetDimensions, MAUTOSTRIP_VOID);
	m_Box.SetDimensions(_Dim);
}

void CXR_PhysicsModel_Box::Phys_GetBound_Sphere(const CMat4Dfp32& _Pos, CVec3Dfp32& _RetPos, fp32& _Radius)
{
	MAUTOSTRIP(CXR_PhysicsModel_Box_Phys_GetBound_Sphere, MAUTOSTRIP_VOID);
	Error("Phys_GetBound_Sphere", "Not implemented.");
}

void CXR_PhysicsModel_Box::Phys_GetBound_Box(const CMat4Dfp32& _Pos, CBox3Dfp32& _RetBox)
{
	MAUTOSTRIP(CXR_PhysicsModel_Box_Phys_GetBound_Box, MAUTOSTRIP_VOID);
	Error("Phys_GetBound_Box", "Not implemented.");
}

void CXR_PhysicsModel_Box::Phys_Init(CXR_PhysicsContext* _pPhysContext)
{
	MAUTOSTRIP(CXR_PhysicsModel_Box_Phys_Init, MAUTOSTRIP_VOID);
}

int CXR_PhysicsModel_Box::Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0)
{
	MAUTOSTRIP(CXR_PhysicsModel_Box_Phys_GetMedium, 0);
	return XW_MEDIUM_AIR | XW_MEDIUM_SEETHROUGH;
};

void CXR_PhysicsModel_Box::Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, CXR_MediumDesc& _RetMedium)
{
	MAUTOSTRIP(CXR_PhysicsModel_Box_Phys_GetMedium_2, MAUTOSTRIP_VOID);
	_RetMedium.SetAir();
};

bool CXR_PhysicsModel_Box::Phys_IntersectLine(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_PhysicsModel_Box_Phys_IntersectLine, false);
	CPhysOBB TestBox = m_Box;
	TestBox.SetPosition(_pPhysContext->m_WMat);
	CVec3Dfp32 HitPos, v0, v1;
	TestBox.TransformToBoxSpace(_v0, v0);
	TestBox.TransformToBoxSpace(_v1, v1);

	if (TestBox.LocalPointInBox(v0))
	{
		//ConOut("Source Inside MissedBox.");
		if (_pCollisionInfo)
		{
			_pCollisionInfo->m_bIsValid = false;
			_pCollisionInfo->m_bIsCollision = true;
		}
		return true;
	}

	if (_pCollisionInfo)
	{
		if (TestBox.IntersectLine(v0, v1, HitPos))
		{
			_pCollisionInfo->m_bIsValid = true;
			_pCollisionInfo->m_bIsCollision = true;

			if (_pCollisionInfo->m_ReturnValues & CXR_COLLISIONRETURNVALUE_TIME)
			{
				fp32 Dist = v0.Distance(HitPos);
				fp32 LineLength = v0.Distance(v1);
				_pCollisionInfo->m_Distance = Dist;
				if (LineLength > _FP32_EPSILON)
					_pCollisionInfo->m_Time = Dist / LineLength;
				else
					_pCollisionInfo->m_Time = 0.0f;

				if (_pCollisionInfo->m_ReturnValues & (CXR_COLLISIONRETURNVALUE_POSITION | CXR_COLLISIONRETURNVALUE_LOCALPOSITION))
				{
					_pCollisionInfo->m_LocalPos = HitPos;
					TestBox.TransformFromBoxSpace(HitPos, _pCollisionInfo->m_Pos);
					TestBox.GetPlaneFromLocalPoint(_pCollisionInfo->m_LocalPos, _pCollisionInfo->m_Plane);
				}
				_pCollisionInfo->m_pSurface = NULL;
			}
			return true;
		}
		//ConOut("MissedBox.");
		return false;
	}
	else
		return TestBox.IntersectLine(v0, v1, HitPos);
}

bool CXR_PhysicsModel_Box::Phys_IntersectSphere(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_PhysicsModel_Box_Phys_IntersectSphere, false);
	return false;
}

bool CXR_PhysicsModel_Box::Phys_IntersectBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _BoxOrigin, const CPhysOBB& _BoxDest, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_PhysicsModel_Box_Phys_IntersectBox, false);
	CPhysOBB TestBox = m_Box;
	TestBox.SetPosition(_pPhysContext->m_WMat);
	return Phys_Intersect_OBB(TestBox, _BoxOrigin, _BoxDest, _pCollisionInfo);
//	return false;
}

int CXR_PhysicsModel_Box::Phys_CollideBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _Box, int _MediumFlags, CCollisionInfo* _pCollisionInfo, int _nMaxCollisions)
{
	// TODO: Implement this!

	//CPhysOBB Obb;
	//Obb.Set
	//Phys_Collide_OBB()
	return 0;
}

