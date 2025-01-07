#ifndef _INC_XR_MediumDesc
#define _INC_XR_MediumDesc

#include "../MOS.h"

// -------------------------------------------------------------------
//  Medium-defines, these oughta' change their prefix..., search/replace anyone?
// -------------------------------------------------------------------
enum
{
	XW_MEDIUM_SOLID				= DBit(0),
	XW_MEDIUM_PHYSSOLID			= DBit(1),
	XW_MEDIUM_PLAYERSOLID		= DBit(2),
	XW_MEDIUM_GLASS				= DBit(3),
	XW_MEDIUM_AISOLID			= DBit(4),
	XW_MEDIUM_CAMERASOLID		= DBit(5),
//	XW_MEDIUM_LAVA				= DBit(6),
//	XW_MEDIUM_SLIME				= DBit(7),
	XW_MEDIUM_LIQUID			= DBit(6),
	XW_MEDIUM_WATER				= XW_MEDIUM_LIQUID,
	XW_MEDIUM_RAIN				= DBit(7),
	XW_MEDIUM_AIR				= DBit(8),
	
	XW_MEDIUM_SEETHROUGH		= DBit(9),
	XW_MEDIUM_DUALSIDED			= DBit(10),
	XW_MEDIUM_FOG				= DBit(11),		// Has NHF,
	XW_MEDIUM_LITFOG			= DBit(12),		// The NHF has lighting.
	XW_MEDIUM_INVISIBLE			= DBit(13),		// Surfaces are invisible
	XW_MEDIUM_CLIPFOG			= DBit(14),		// NHF should be clipped. (what that means is very complicated so if you really want to know you have to ask me.  /mh)
	XW_MEDIUM_NAVIGATION		= DBit(15),
	XW_MEDIUM_FOGADDITIVE		= DBit(16),		// The NHF is drawn with additive blending.
	XW_MEDIUM_FOGNOTESS			= DBit(17),		// No auto-tesselation of faces in the fog.
	XW_MEDIUM_SKY				= DBit(18),
	XW_MEDIUM_NAVGRIDFLAGS		= DBit(19),		// User1 contain nav-grid flags.
	XW_MEDIUM_NOMERGE			= DBit(20),		// Brushes with this medium cannot be CSG merged.
	XW_MEDIUM_NOSTRUCTUREMERGE	= DBit(21),		// Structure brushes with this medium cannot be CSG merged.
	XW_MEDIUM_DEPTHFOG			= DBit(22),		// Override depth-fogging

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
	fp4 m_CSGPriority;		// CSG carve priority when medium types are equal
	fp4 m_Density;			// Controls whether an object floats or sinks.
	fp4 m_Thickness;		// Controls how much the medium's velocity influence an object's velocity.
	uint32 m_Color;			// Controls colorizing.

	CVec3Dfp4 m_Velocity;	// Units per second
	CVec3Dfp4 m_RotationPivot;
	CVec3Dfp4 m_RotationAxis;
	fp4 m_Rotation;			// Normalized degrees per second.

	CPlane3Dfp4 m_FogPlane;	// Density(v) = FogPlane.Distance(v)*Attenuation + Density;
	fp4 m_FogDensity;
	fp4 m_FogAttenuation;
	uint32 m_FogColor;
	int16 m_FogResolution;

	int16 m_iPhysGroup;

	int32 m_User1;
	int32 m_User2;

	void Clear();
	CXR_MediumDesc();
	CXR_MediumDesc(int _MediumFlags, fp4 _Density, fp4 _Thickness, CVec3Dfp4 _Velocity);
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
