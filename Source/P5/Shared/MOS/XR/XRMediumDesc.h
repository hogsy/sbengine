#ifndef _INC_XR_MediumDesc
#define _INC_XR_MediumDesc

#include "../MOS.h"

// -------------------------------------------------------------------
//  Medium-defines, these oughta' change their prefix..., search/replace anyone?
// -------------------------------------------------------------------
enum
{
	XW_MEDIUM_SOLID				= M_Bit(0),
	XW_MEDIUM_PHYSSOLID			= M_Bit(1),
	XW_MEDIUM_PLAYERSOLID		= M_Bit(2),
	XW_MEDIUM_GLASS				= M_Bit(3),
	XW_MEDIUM_AISOLID			= M_Bit(4),
	XW_MEDIUM_DYNAMICSSOLID		= M_Bit(5),
//	XW_MEDIUM_LAVA				= M_Bit(6),
//	XW_MEDIUM_SLIME				= M_Bit(7),
	XW_MEDIUM_LIQUID			= M_Bit(6),
	XW_MEDIUM_WATER				= XW_MEDIUM_LIQUID,
	XW_MEDIUM_CAMERASOLID		= M_Bit(7),
	XW_MEDIUM_AIR				= M_Bit(8),
	
	XW_MEDIUM_SEETHROUGH		= M_Bit(9),
	XW_MEDIUM_DUALSIDED			= M_Bit(10),
	XW_MEDIUM_FOG				= M_Bit(11),		// Has NHF,
	XW_MEDIUM_LITFOG			= M_Bit(12),		// The NHF has lighting.
	XW_MEDIUM_INVISIBLE			= M_Bit(13),		// Surfaces are invisible
	XW_MEDIUM_CLIPFOG			= M_Bit(14),		// NHF should be clipped. (what that means is very complicated so if you really want to know you have to ask me.  /mh)
	XW_MEDIUM_NAVIGATION		= M_Bit(15),
	XW_MEDIUM_FOGADDITIVE		= M_Bit(16),		// The NHF is drawn with additive blending.
	XW_MEDIUM_FOGNOTESS			= M_Bit(17),		// No auto-tesselation of faces in the fog.
	XW_MEDIUM_SKY				= M_Bit(18),
	XW_MEDIUM_NAVGRIDFLAGS		= M_Bit(19),		// User1 contain nav-grid flags.
	XW_MEDIUM_NOMERGE			= M_Bit(20),		// Brushes with this medium cannot be CSG merged.
	XW_MEDIUM_NOSTRUCTUREMERGE	= M_Bit(21),		// Structure brushes with this medium cannot be CSG merged.
	XW_MEDIUM_DEPTHFOG			= M_Bit(22),		// Override depth-fogging

	XW_MEDIUM_TYPEMASK			= DBitRange(0, 8),
	XW_MEDIUM_ATTRMASK			= DBitRange(9, 31),
};

// -------------------------------------------------------------------
//  CXR_MediumDesc
// -------------------------------------------------------------------
class CXR_MediumDesc
{
public:
	int32 m_MediumFlags;	// Medium type
	fp32 m_CSGPriority;		// CSG carve priority when medium types are equal
	fp32 m_Density;			// Controls whether an object floats or sinks.
	fp32 m_Thickness;		// Controls how much the medium's velocity influence an object's velocity.
	uint32 m_Color;			// Controls colorizing.

	CVec3Dfp32 m_Velocity;	// Units per second
	CVec3Dfp32 m_RotationPivot;
	CVec3Dfp32 m_RotationAxis;
	fp32 m_Rotation;			// Normalized degrees per second.

	CPlane3Dfp32 m_FogPlane;	// Density(v) = FogPlane.Distance(v)*Attenuation + Density;
	fp32 m_FogDensity;
	fp32 m_FogAttenuation;
	uint32 m_FogColor;
	int16 m_FogResolution;

	int16 m_iPhysGroup;

	int32 m_User1;
	int32 m_User2;

	void Clear();
	CXR_MediumDesc();
	CXR_MediumDesc(int _MediumFlags, fp32 _Density, fp32 _Thickness, CVec3Dfp32 _Velocity);
	void SetSolid();
	void SetWater();
	void SetAir();
	static int CompareMediums(int _Medium1, int _Medium2);
	int CompareMediums(const CXR_MediumDesc& _Medium) const;
	int Equal(const CXR_MediumDesc& _Medium) const;

	bool ParseKey(CStr _Key, CStr _Value);
	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile) const;
};

#endif //_INC_XR_MediumDesc
