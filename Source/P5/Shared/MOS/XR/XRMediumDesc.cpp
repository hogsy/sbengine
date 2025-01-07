
#include "PCH.h"

#include "XRMediumDesc.h"

#define XW_MEDIUM_VERSION			0x0103

// -------------------------------------------------------------------
//  CXR_MediumDesc
// -------------------------------------------------------------------
void CXR_MediumDesc::Clear()
{
	m_FogResolution = 128;
	m_MediumFlags = XW_MEDIUM_SOLID;
	m_CSGPriority = 0;
	m_Density = 1.0f;
	m_Thickness = 1.0f;
	m_Color = 0xffffffff;
	m_Velocity.SetScalar(0.0f);
	m_RotationPivot.SetScalar(0.0f);
	m_RotationAxis.SetScalar(0.0f);
	m_Rotation = 0.0f;
	m_FogPlane.n.SetScalar(0.0f);
	m_FogPlane.d = 0.0f;
	m_FogDensity = 1.0f;
	m_FogAttenuation = 0.0f;
	m_FogColor = 0xffffffff;
	m_User1 = 0;
	m_User2 = 0;
	m_iPhysGroup = 0;
}

CXR_MediumDesc::CXR_MediumDesc()
{
	Clear();
}

CXR_MediumDesc::CXR_MediumDesc(int _MediumFlags, fp32 _Density, fp32 _Thickness, CVec3Dfp32 _Velocity)
{
	Clear();
	m_MediumFlags = _MediumFlags;
	m_Density = _Density;
	m_Thickness = _Thickness;
	m_Color = 0xffffffff;
	m_Velocity = _Velocity;
}

void CXR_MediumDesc::SetSolid()
{
	m_MediumFlags = XW_MEDIUM_SOLID;
	m_Density = 1.0f;
	m_Thickness = 1.0f;
	m_Color = 0xffffffff;
	m_Velocity = 0;
	m_RotationPivot = 0;
	m_RotationAxis = 0;
	m_Rotation = 0;
	m_iPhysGroup = 0;
	m_User1 = 0;
	m_User2 = 0;
}

void CXR_MediumDesc::SetWater()
{
	m_MediumFlags = XW_MEDIUM_WATER | XW_MEDIUM_SEETHROUGH | XW_MEDIUM_NAVIGATION;
	m_Density = 1.0f;
	m_Thickness = 0.1f;
	m_Color = 0xffffffff;
	m_Velocity = 0;
	m_RotationPivot = 0;
	m_RotationAxis = 0;
	m_Rotation = 0;
	m_iPhysGroup = 0;
	m_User1 = 0;
	m_User2 = 0;
}

void CXR_MediumDesc::SetAir()
{
	m_MediumFlags = XW_MEDIUM_AIR | XW_MEDIUM_SEETHROUGH | XW_MEDIUM_NAVIGATION;
	m_Density = 0.001f;
	m_Thickness = 0.02f;
	m_Color = 0xffffffff;
	m_Velocity = 0;
	m_RotationPivot = 0;
	m_RotationAxis = 0;
	m_Rotation = 0;
	m_FogPlane.CreateND(CVec3Dfp32(1,0,0), 0);
	m_FogDensity = 1.0f;
	m_FogAttenuation = 0;
	m_FogColor = 0xffffffff;
	m_iPhysGroup = 0;
	m_User1 = 0;
	m_User2 = 0;
}

int CXR_MediumDesc::CompareMediums(int _Medium1, int _Medium2)
{
	int bs1 = BitScanFwd32(_Medium1 & XW_MEDIUM_TYPEMASK);
	int bs2 = BitScanFwd32(_Medium2 & XW_MEDIUM_TYPEMASK);
	if (bs1 < bs2) 
		return 1;
	else if (bs1 > bs2)
		return -1;
	else 
		return 0;
}

int CXR_MediumDesc::CompareMediums(const CXR_MediumDesc& _Medium) const
{
	// The result of this function do NOT affect the medium assigned to a solid.
	// CSolid::CSG_GetMediumFromSurfaces doesn't use this method.

	int bs1 = BitScanFwd32(m_MediumFlags & XW_MEDIUM_TYPEMASK);
	int bs2 = BitScanFwd32(_Medium.m_MediumFlags & XW_MEDIUM_TYPEMASK);

	if (bs1 < bs2) 
		return 1;
	else if (bs1 > bs2)
		return -1;
	else 
	{
		if (m_CSGPriority > _Medium.m_CSGPriority)
			return 1;
		else if (m_CSGPriority < _Medium.m_CSGPriority)
			return -1;
		return 0;
	}
}

int CXR_MediumDesc::Equal(const CXR_MediumDesc& _Medium) const
{
	if ((m_MediumFlags == _Medium.m_MediumFlags) &&
		(m_CSGPriority == _Medium.m_CSGPriority) &&
		(m_Density == _Medium.m_Density) &&
		(m_Thickness == _Medium.m_Thickness) &&
		(m_Color == _Medium.m_Color) &&
		(m_Velocity == _Medium.m_Velocity) &&
		(m_RotationPivot == _Medium.m_RotationPivot) &&
		(m_RotationAxis == _Medium.m_RotationAxis) &&
		(m_Rotation == _Medium.m_Rotation) &&
		(m_FogAttenuation == _Medium.m_FogAttenuation) &&
		(m_FogDensity == _Medium.m_FogDensity) &&
		(m_FogColor == _Medium.m_FogColor) &&
		(m_FogResolution == _Medium.m_FogResolution) &&
		(m_iPhysGroup  == _Medium.m_iPhysGroup) &&
		(m_FogPlane.n.Distance(_Medium.m_FogPlane.n) < 0.001f) &&
		(M_Fabs(m_FogPlane.d - _Medium.m_FogPlane.d) < 0.001f) &&
		(m_User1 == _Medium.m_User1) &&
		(m_User2 == _Medium.m_User2)) return 1;
	return 0;
}

bool CXR_MediumDesc::ParseKey(CStr _Key, CStr _Value)
{
	const char* g_MediumFlagsTranslate[] = 
	{
		"solid", "physsolid", "playersolid", "glass", 
		"aisolid", "dynamicssolid", "liquid", "camerasolid", 
		"air", "vis", "dualsided", "fog", "litfog", 
		"invisible", "clipfog", "navigation", "addfog", 
		"fognotess", "sky", "navgridflags", "nomerge", 
		"nostructuremerge", "depthfog", (char*)NULL
	};

	const fp32 Valuef = _Value.Val_fp64();

	if (!_Key.CompareNoCase("MEDIUM_FLAGS"))
	{
		if (_Value[0] >= '0' && _Value[0] <= '9')
			m_MediumFlags = _Value.Val_int();
		else
			m_MediumFlags = _Value.TranslateFlags(g_MediumFlagsTranslate);
	}
	else if (!_Key.CompareNoCase("MEDIUM_CSGPRIORITY"))
		m_CSGPriority = Valuef;
	else if (!_Key.CompareNoCase("MEDIUM_DENSITY"))
		m_Density = Valuef;
	else if (!_Key.CompareNoCase("MEDIUM_THICKNESS"))
		m_Thickness = Valuef;
	else if (!_Key.CompareNoCase("MEDIUM_COLOR"))
	{
		CPixel32 Col; Col.Parse(_Value);
		m_Color = Col;
	}
	else if (!_Key.CompareNoCase("MEDIUM_VELOCITY"))
		m_Velocity.ParseString(_Value);
	else if (!_Key.CompareNoCase("MEDIUM_ROTPIVOT"))
		m_RotationPivot.ParseString(_Value);
	else if (!_Key.CompareNoCase("MEDIUM_ROTAXIS"))
		m_RotationAxis.ParseString(_Value);
	else if (!_Key.CompareNoCase("MEDIUM_ROTATION"))
		m_Rotation = Valuef;
	else if (!_Key.CompareNoCase("MEDIUM_FOGPLANE"))
	{
		CVec3Dfp32 n, p;
//	LogFile("FogPlane: " + _Key + " = " + _Value);
		n.k[0] = _Value.Getfp64Sep(",");
		n.k[1] = _Value.Getfp64Sep(",");
		n.k[2] = _Value.Getfp64Sep(",");
		p.k[0] = _Value.Getfp64Sep(",");
		p.k[1] = _Value.Getfp64Sep(",");
		p.k[2] = _Value.Getfp64Sep(",");
		m_FogPlane = CPlane3Dfp32(n, p);
//	LogFile("FogPlane: " + m_FogPlane.GetString());
	}
	else if (!_Key.CompareNoCase("MEDIUM_FOGDENSITY"))
		m_FogDensity = Valuef;
	else if (!_Key.CompareNoCase("MEDIUM_FOGATTENUATION"))
	{
		m_FogAttenuation = (Valuef != 0.0f) ? (1.0f / Valuef) : fp32( 0 );
	}
	else if (!_Key.CompareNoCase("MEDIUM_FOGCOLOR"))
	{
		CPixel32 FogColor;
		FogColor.Parse(_Value);
		m_FogColor = FogColor;
	}
	else if (!_Key.CompareNoCase("MEDIUM_FOGRESOLUTION"))
		m_FogResolution = int16(Valuef);
	else if (!_Key.CompareNoCase("MEDIUM_PHYSGROUP"))
		m_iPhysGroup = _Value.Val_int();
	else if (!_Key.CompareNoCase("MEDIUM_USER1"))
		m_User1 = _Value.Val_int();
	else if (!_Key.CompareNoCase("MEDIUM_USER2"))
		m_User2 = _Value.Val_int();
	else if (!_Key.CompareNoCase("MEDIUM_CREATEDYNAMICSSOLID"))
		m_MediumFlags = XW_MEDIUM_SOLID;
	else
		return false;
	return true;
}

void CXR_MediumDesc::Read(CCFile* _pFile)
{
	MSCOPESHORT(CXR_MediumDesc::Read);
	uint32 Ver = 0;
	_pFile->ReadLE(Ver);
	switch(Ver)
	{
	case 0x0100 :
		{
			_pFile->ReadLE(m_MediumFlags);
			_pFile->ReadLE(m_Density);
			_pFile->ReadLE(m_Thickness);
			_pFile->ReadLE(m_Color);

			m_Velocity.Read(_pFile);
			m_RotationPivot.Read(_pFile);
			m_RotationAxis.Read(_pFile);
			_pFile->ReadLE(m_Rotation);

			m_FogPlane.n.Read(_pFile);
			_pFile->ReadLE(m_FogPlane.d);
			_pFile->ReadLE(m_FogDensity);
			_pFile->ReadLE(m_FogAttenuation);
			_pFile->ReadLE(m_FogColor);

			_pFile->ReadLE(m_User1);
			_pFile->ReadLE(m_User2);
		}
		break;
	case XW_MEDIUM_VERSION :
		{
			_pFile->ReadLE(m_CSGPriority);
		}
	case 0x0102 :
		{
			_pFile->ReadLE(m_MediumFlags);
			_pFile->ReadLE(m_Density);
			_pFile->ReadLE(m_Thickness);
			_pFile->ReadLE(m_Color);

			m_Velocity.Read(_pFile);
			m_RotationPivot.Read(_pFile);
			m_RotationAxis.Read(_pFile);
			_pFile->ReadLE(m_Rotation);

			m_FogPlane.n.Read(_pFile);
			_pFile->ReadLE(m_FogPlane.d);
			_pFile->ReadLE(m_FogDensity);
			_pFile->ReadLE(m_FogAttenuation);
			_pFile->ReadLE(m_FogColor);
			_pFile->ReadLE(m_FogResolution);
			_pFile->ReadLE(m_iPhysGroup);
			_pFile->ReadLE(m_User1);
			_pFile->ReadLE(m_User2);
		}
		break;

	default :
		Error_static("CXR_MediumDesc::Read", CStrF("Unsupported medium version (%.4x)", Ver));
	}
}

void CXR_MediumDesc::Write(CCFile* _pFile) const
{
	uint32 Ver = XW_MEDIUM_VERSION;
	_pFile->WriteLE(Ver);

	_pFile->WriteLE(m_CSGPriority);

	_pFile->WriteLE(m_MediumFlags);
	_pFile->WriteLE(m_Density);
	_pFile->WriteLE(m_Thickness);
	_pFile->WriteLE(m_Color);

	m_Velocity.Write(_pFile);
	m_RotationPivot.Write(_pFile);
	m_RotationAxis.Write(_pFile);
	_pFile->WriteLE(m_Rotation);

	m_FogPlane.n.Write(_pFile);
	_pFile->WriteLE(m_FogPlane.d);
	_pFile->WriteLE(m_FogDensity);
	_pFile->WriteLE(m_FogAttenuation);
	_pFile->WriteLE(m_FogColor);
	_pFile->WriteLE(m_FogResolution);
	_pFile->WriteLE(m_iPhysGroup);

	_pFile->WriteLE(m_User1);
	_pFile->WriteLE(m_User2);
}

