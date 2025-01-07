
/*------------------------------------------------------------------------------------------------
NAME:		XRClass.cpp/h
PURPOSE:	Extended Reality core classes
CREATION:	9610xx
AUTHOR:		Magnus Högdahl
COPYRIGHT:	(c) Copyright 1998 Starbreeze Studios AB

CONTENTS:
-

MAINTAINANCE LOG:
Lot's of changes...

------------------------------------------------------------------------------------------------*/
#ifndef _INC_XR_Class
#define _INC_XR_Class

// -------------------------------------------------------------------
#include "../MOS.h"
#include "Phys/WPhys.h"
#include "Phys/WPhysPCS.h"
#include "XRSurf.h"
#include "XRVBPrior.h"

// -------------------------------------------------------------------

#define OBJECT_FLAGS_NOLIGHT	1

class CXR_Engine;
class CXW_Surface;
class CXR_VBManager;
class CXR_VBMScope;
class CXR_AnimState;
class CXR_Model;
class CXR_IndexedSolidContainer32;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_BoxMapping
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_BoxMapping
{
public:
	CVec2Dfp32 m_Offset;
	CVec2Dfp32 m_Scale;
	fp32 m_Rot;
	
	CXR_BoxMapping() {}

	void CreateUnit();
	void Create(const class CXR_PlaneMapping&, fp32 _Epsilon = 0.001f);

	bool AlmostEqual(const CXR_BoxMapping& _Map, fp32 _Epsilon) const;

	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile) const;
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_PlaneMapping
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_PlaneMapping
{
public:
	CVec3Dfp32 m_U;
	fp32 m_UOffset;
	CVec3Dfp32 m_V;
	fp32 m_VOffset;

//	fp32 m_ULengthRecp;
//	fp32 m_VLengthRecp;
	
	CXR_PlaneMapping() {}

	void CreateUnit();
	void Create(const class CXR_BoxMapping&, const CPlane3Dfp64& _Plane);
	void Create(const class CXR_BoxMapping&, const CPlane3Dfp32& _Plane);

	int AlmostEqual(const CXR_PlaneMapping& _Map, fp32 _Epsilon) const;

	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile);

	void Struct_Read(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
	void Struct_SwapLE();
#endif
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_RenderInfo
|__________________________________________________________________________________________________
\*************************************************************************************************/
enum
{
	CXR_RENDERINFO_DEPTHFOG = 1,
	CXR_RENDERINFO_VERTEXFOG = 2,
	CXR_RENDERINFO_NHF = 4,
	CXR_RENDERINFO_NEEDLIGHTS = 8,				// Input flag telling the CXR_ViewClipInterface to supply lights in the CXR_RenderInfo structure.
	CXR_RENDERINFO_NOSHADOWVOLUMES = 16,		// The geometry associated with the qeuery does not render any shadow volumes. (No effect unless CXR_RENDERINFO_NEEDLIGHTS is set.)
	CXR_RENDERINFO_INVISIBLE = 32,				// The "body" of the object is invisible
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:	Structure for returning light information from
			the CXR_ViewClipInterface::View_GetClip_xxxx methods.
\*____________________________________________________________________*/
class CXR_LightInfo
{
public:
	CScissorRect m_Scissor;
#ifndef	PLATFORM_PS2
	CScissorRect m_ShadowScissor;
#endif	// PLATFORM_PS2
	const class CXR_Light* m_pLight;
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:	Structure for returning occlusion information from
			the CXR_ViewClipInterface::View_Light_GetOcclusion method.
\*____________________________________________________________________*/
class CXR_LightOcclusionInfo
{
public:
#ifndef	PLATFORM_PS2
	CScissorRect m_ScissorShaded;
	CScissorRect m_ScissorShadow;
#endif	// PLATFORM_PS2
	CScissorRect m_ScissorVisible;
	CScissorRect m_Padding;

	void Clear()
	{
		m_ScissorVisible.SetRect(0xffff, 0x0000);
//		m_ScissorVisible.m_Min[0] = 0xffff;
//		m_ScissorVisible.m_Min[1] = 0xffff;
//		m_ScissorVisible.m_Max[0] = 0;
//		m_ScissorVisible.m_Max[1] = 0;
#ifndef	PLATFORM_PS2
		m_ScissorShaded.SetRect(0xffff, 0x0000);
		m_ScissorShadow.SetRect(0xffff, 0x0000);
//		m_ScissorShaded.m_Min[0] = 0xffff;
//		m_ScissorShaded.m_Min[1] = 0xffff;
//		m_ScissorShaded.m_Max[0] = 0;
//		m_ScissorShaded.m_Max[1] = 0;
//		m_ScissorShadow.m_Min[0] = 0xffff;
//		m_ScissorShadow.m_Min[1] = 0xffff;
//		m_ScissorShadow.m_Max[0] = 0;
//		m_ScissorShadow.m_Max[1] = 0;
#endif	// PLATFORM_PS2
	}

	bool IsVisible() const
	{
		return m_ScissorVisible.IsValid();
//		return ((m_ScissorVisible.m_Min[0] < m_ScissorVisible.m_Max[0]) && (m_ScissorVisible.m_Min[1] < m_ScissorVisible.m_Max[1]));
	}
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:	Structure for returning rendering information from
			the CXR_ViewClipInterface::View_GetClip_xxxx methods.					
\*____________________________________________________________________*/
class CXR_RenderInfo
{
public:
	CXR_Engine *m_pCurrentEngine;

	int m_Flags;								// CXR_RENDERINFO_xxxx flags
	int m_MediumFlags;
	fp32 m_BasePriority_Opaque;
	fp32 m_BasePriority_Transparent;
	int m_iNHFNode;
	CXR_MediumDesc* m_pMedium;
	class CXR_LightVolume* m_pLightVolume;		// Old school light volume, to be removed

//	CRect2Duint16 m_Scissor;					// 2D-visibility for the volume
	CScissorRect m_Scissor;

	CXR_LightInfo* m_pLightInfo;				// This pointer is supplied by the model calling View_GetClip_xxxx
	uint16 m_MaxLights;							// Maximum number of lights that can be supplied in m_pLightInfo, this is supplied by the model calling View_GetClip_xxxx.
	uint16 m_nLights;							// Returned number of lights written into m_pLightInfo

	void Clear(CXR_Engine *_pEngine)
	{
		m_pCurrentEngine = _pEngine;
		m_Flags = 0;
		m_MediumFlags = 0;
		m_BasePriority_Opaque = CXR_VBPRIORITY_MODEL_OPAQUE;
		m_BasePriority_Transparent = CXR_VBPRIORITY_MODEL_TRANSPARENT;
		m_iNHFNode = 0;
		m_pMedium = NULL;
		m_pLightVolume = NULL;
		m_pLightInfo = NULL;
		m_nLights = 0;
		m_MaxLights = 0;
	}

	CXR_RenderInfo(CXR_Engine *_pEngine)
	{
		Clear(_pEngine);		
	}
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Light
|__________________________________________________________________________________________________
\*************************************************************************************************/
enum
{
	CXR_LIGHT_NOSHADOWS =				M_Bit(0),	// This flag is only meaningful on dynamic lights when using unified lighting
	CXR_LIGHT_FLARE =					M_Bit(1),	// This is used in CWObject_Light.
	CXR_LIGHT_LIGHTFIELDMAP =			M_Bit(2),	// Light is not added to engine. It will only be used for light field map rendering.
	CXR_LIGHT_ENABLED =					M_Bit(3),
	CXR_LIGHT_FLAREDIRECTIONAL =		M_Bit(4),	// This is used in CWObject_Light.
	CXR_LIGHT_PROJMAPTRANSFORM =		M_Bit(5),	// m_ProjMapTransform is used for proj map.
	CXR_LIGHT_HINT_ADDITIVE =			M_Bit(6),
	CXR_LIGHT_HINT_DONTAFFECTMODELS =	M_Bit(7),	// Light will only illuminate world.
	CXR_LIGHT_ANIMTIME =				M_Bit(8),	// AnimTime member of CXR_Light is valid.
	CXR_LIGHT_NODIFFUSE =				M_Bit(9), 
	CXR_LIGHT_NOSPECULAR =				M_Bit(10),
	CXR_LIGHT_RADIOSITY =				M_Bit(11),
	CXR_LIGHT_FLAREONLY =				M_Bit(12),

	CXR_LIGHT_LAST =					M_Bit(13),
	// note: 16 bits
};

// -------------------------------------------------------------------
enum
{
	CXR_LIGHTTYPE_POINT	=				0,
	CXR_LIGHTTYPE_FAKESKYRADIOSITY =	1,
	CXR_LIGHTTYPE_PARALLELL =			2,
	CXR_LIGHTTYPE_AMBIENT =				3,
	CXR_LIGHTTYPE_LIGHTVOLUME =			4,
	CXR_LIGHTTYPE_SPOT =				5,

	CXR_LIGHT_VERSION =					0x0100,
};

// -------------------------------------------------------------------
class CXR_LightGridPoint
{
#ifdef	PLATFORM_DOLPHIN
	uint32 m_LightR : 5;
	uint32 m_LightG : 5;
	uint32 m_LightB : 5;
	uint32 m_LightDirX : 4;
	uint32 m_LightDirY : 4;
	uint32 m_LightDirZ : 4;
	uint32 m_LightBias : 5;

public:

	CXR_LightGridPoint() {}
	CXR_LightGridPoint(const CPixel32 &_Light, const CVec3Dint8 &_LightDir, const uint8 &_LightBias);

	const CPixel32 GetLight() const { return CPixel32((m_LightR<<3)|(m_LightR>>2), (m_LightG<<3)|(m_LightG>>2), (m_LightB<<3)|(m_LightB>>2)); }
	const int8 GetLightDirX() const { return int8((m_LightDirX<<4)|(m_LightDirX)); }
	const int8 GetLightDirY() const { return int8((m_LightDirY<<4)|(m_LightDirY)); }
	const int8 GetLightDirZ() const { return int8((m_LightDirZ<<4)|(m_LightDirZ)); }
	const uint8 GetLightBias() const { return uint8((m_LightBias<<3)|(m_LightBias>>2)); }
#else

public:
	CPixel32 m_Light;
	CVec3Dint8 m_LightDir;
	uint8 m_LightBias;

	const CPixel32& GetLight() const { return m_Light; }
	const int8& GetLightDirX() const { return m_LightDir.k[0]; }
	const int8& GetLightDirY() const { return m_LightDir.k[1]; }
	const int8& GetLightDirZ() const { return m_LightDir.k[2]; }
	const uint8& GetLightBias() const { return m_LightBias; }
#endif

	void Read(CCFile* _pF, int _Ver);
	void Write(CCFile* _pF, int _Ver) const;
#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif
};

// The LFE vectors are as follows
// 0 - (-1, 0, 0)
// 1 - ( 1, 0, 0)
// 2 - ( 0,-1, 0)
// 3 - ( 0, 1, 0)
// 4 - ( 0, 0,-1)
// 5 - ( 0, 0, 1)

#if defined(COMPILER_CODEWARRIOR)
#pragma push
#pragma pack(1)
#else
#pragma pack(push, 1)
//#pragma pack(1)
#endif
class CXR_LightFieldElement
{
public:
	fp2 m_Scaler;
	uint8 m_Axis[6][3];

	M_INLINE void Clear()
	{
		m_Scaler.Set(FP2_ONE);
		for(int i = 0; i < 6; i++)
		{
			m_Axis[i][0] = 0;
			m_Axis[i][1] = 0;
			m_Axis[i][2] = 0;
		}
	}
/*
	bool IsWithinBounds(const CXR_LightFieldElement& _Other, int _Delta) const
	{
		return  (Abs(m_Axis[0][0] - _Other.m_Axis[0][0]) <= _Delta) &&
				(Abs(m_Axis[0][1] - _Other.m_Axis[0][1]) <= _Delta) &&
				(Abs(m_Axis[0][2] - _Other.m_Axis[0][2]) <= _Delta) &&
				(Abs(m_Axis[1][0] - _Other.m_Axis[1][0]) <= _Delta) &&
				(Abs(m_Axis[1][1] - _Other.m_Axis[1][1]) <= _Delta) &&
				(Abs(m_Axis[1][2] - _Other.m_Axis[1][2]) <= _Delta) &&
				(Abs(m_Axis[2][0] - _Other.m_Axis[2][0]) <= _Delta) &&
				(Abs(m_Axis[2][1] - _Other.m_Axis[2][1]) <= _Delta) &&
				(Abs(m_Axis[2][2] - _Other.m_Axis[2][2]) <= _Delta) &&
				(Abs(m_Axis[3][0] - _Other.m_Axis[3][0]) <= _Delta) &&
				(Abs(m_Axis[3][1] - _Other.m_Axis[3][1]) <= _Delta) &&
				(Abs(m_Axis[3][2] - _Other.m_Axis[3][2]) <= _Delta) &&
				(Abs(m_Axis[4][0] - _Other.m_Axis[4][0]) <= _Delta) &&
				(Abs(m_Axis[4][1] - _Other.m_Axis[4][1]) <= _Delta) &&
				(Abs(m_Axis[4][2] - _Other.m_Axis[4][2]) <= _Delta) &&
				(Abs(m_Axis[5][0] - _Other.m_Axis[5][0]) <= _Delta) &&
				(Abs(m_Axis[5][1] - _Other.m_Axis[5][1]) <= _Delta) &&
				(Abs(m_Axis[5][2] - _Other.m_Axis[5][2]) <= _Delta);
	}

	M_INLINE bool operator != (const CXR_LightFieldElement& _Other) const
	{
		return  (m_Axis[0][0] != _Other.m_Axis[0][0]) ||
				(m_Axis[0][1] != _Other.m_Axis[0][1]) ||
				(m_Axis[0][2] != _Other.m_Axis[0][2]) ||
				(m_Axis[1][0] != _Other.m_Axis[1][0]) ||
				(m_Axis[1][1] != _Other.m_Axis[1][1]) ||
				(m_Axis[1][2] != _Other.m_Axis[1][2]) ||
				(m_Axis[2][0] != _Other.m_Axis[2][0]) ||
				(m_Axis[2][1] != _Other.m_Axis[2][1]) ||
				(m_Axis[2][2] != _Other.m_Axis[2][2]) ||
				(m_Axis[3][0] != _Other.m_Axis[3][0]) ||
				(m_Axis[3][1] != _Other.m_Axis[3][1]) ||
				(m_Axis[3][2] != _Other.m_Axis[3][2]) ||
				(m_Axis[4][0] != _Other.m_Axis[4][0]) ||
				(m_Axis[4][1] != _Other.m_Axis[4][1]) ||
				(m_Axis[4][2] != _Other.m_Axis[4][2]) ||
				(m_Axis[5][0] != _Other.m_Axis[5][0]) ||
				(m_Axis[5][1] != _Other.m_Axis[5][1]) ||
				(m_Axis[5][2] != _Other.m_Axis[5][2]);
	}

	CStr GetString() const
	{
		return CStrF("(%.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x)",
			m_Axis[0][0], m_Axis[0][1], m_Axis[0][2],
			m_Axis[1][0], m_Axis[1][1], m_Axis[1][2],
			m_Axis[2][0], m_Axis[2][1], m_Axis[2][2],
			m_Axis[3][0], m_Axis[3][1], m_Axis[3][2],
			m_Axis[4][0], m_Axis[4][1], m_Axis[4][2],
			m_Axis[5][0], m_Axis[5][1], m_Axis[5][2]);
	}
*/

	void Read(CCFile* _pF, int _Ver);
	void Write(CCFile* _pF) const;
	void SwapLE()
	{
		m_Scaler.SwapLE();
	}
};
#if defined(COMPILER_CODEWARRIOR)
#pragma pop
#else
#pragma pack(pop)
#endif

enum
{
	XR_LIGHTVOLUME_ADD =				1,
	XR_LIGHTVOLUME_HQ =					2,
};

class CXR_LightVolume
{
public:
	virtual class CXR_LightVolume* GetNext() pure;
	virtual const class CXR_LightVolume* GetNext() const pure;
	virtual fp32 Light_EvalVertex(const class CXR_LightID* _pIDMap, const CVec3Dfp32& _V, CVec3Dfp32& _LDir, CVec3Dfp32& _LColor) pure;
	virtual CPixel32 Light_Vertex(const class CXR_LightID* _pIDMap, const CVec3Dfp32& _V, const CVec3Dfp32* _pN) pure;
	virtual void Light_Array(const class CXR_LightID* _pIDMap, int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN = NULL, const CMat4Dfp32* _pWMat = NULL, int _Flags = 0) pure;
	virtual void Light_EvalPoint(const CVec3Dfp32& _V, CVec4Dfp32* _pAxis) pure;
	virtual void Light_EvalPointArrayMax(const CVec3Dfp32* _pV, int _nV, CVec4Dfp32* _pAxis) pure;
};

class CXR_LightPosition
{
public:
	// Big things first
	CMat4Dfp32 m_Pos;
	CMat4Dfp32 m_ProjMapTransform;					// No IO
	uint16 m_Flags;
	int16 m_Type;

	void SetDirection(const CVec3Dfp32&);
	const CVec3Dfp32& GetDirection() const;

	void SetPosition(const CVec3Dfp32&);
	const CVec3Dfp32& GetPosition() const;

	fp32 GetDistance(const CVec3Dfp32& _Pos) const;
	fp32 GetDistanceSqr(const CVec3Dfp32& _Pos) const;

	void Transform(const CMat4Dfp32&, CXR_LightPosition& _Dest) const;
};

class CXR_Light : public CXR_LightPosition
{
private:
	CVec4Dfp32 m_Intensity;							// Clamped to [0..1] atm.
public:
	fp32 m_Range;
	fp32 m_SpotWidth;
	fp32 m_SpotHeight;

	CBox3Dfp32 m_BoundBox;

	uint32 m_ProjMapID;								// No IO
	fp32 m_AnimTime;									// No IO
	uint16 m_LightGUID;								// No IO, this is used to name dynamic lights.
	uint16 m_iLight;								// No IO, this is used as a rendering order priority among other things. (All passes and shadowvolumes must be rendered together in a particular order and not be mixed up with rendering passes for other lights)
	fp32 m_iLightf;									// m_iLight in float format for use in priority calculations without int->float conversion.
//	CPixel32 m_IntensityInt32;						// No IO
	fp32 m_RangeInv;									// No IO
	CXR_LightVolume* m_pLightVolume;				// No IO

	union 
	{
		uint8* m_pPVS;							// No IO
		fp32 m_SortVal;
	};

	static int ParseFlags(const char* _pStr);

#ifndef M_RTM
	bool IsStaticMatch(CXR_Light *_pLight);
#endif

	void SetProjectionMap(int _TextureID, const CMat4Dfp32* _pTransform = NULL);

	void SetIntensity(const CVec3Dfp32& _Intensity);
	M_FORCEINLINE const CVec3Dfp32& GetIntensity() const { return *((const CVec3Dfp32*)&m_Intensity); }


	void Transform(const CMat4Dfp32&);

#ifndef PLATFORM_CONSOLE
	TPtr<class CSolid> CreateBoundSolid() const;	// Very, very slow!
	void CalcBoundBox();							// Very, very slow! This runs CreateBoundSolid();
#endif
	void CalcBoundBoxFast();

	M_INLINE fp32 GetIntensityf() const
	{
		return ((CVec3Dfp32&)m_Intensity).Length();
	}
	M_INLINE fp32 GetIntensitySqrf() const
	{
		return ((CVec3Dfp32&)m_Intensity).LengthSqr();
	}
	M_INLINE CVec4Dfp32 GetIntensityv() const
	{
		return m_Intensity;
	}

	CXR_Light* m_pNext;

	CXR_Light();
	CXR_Light(const CVec3Dfp32& _Pos, const CVec3Dfp32& _Intensity, fp32 _Range, int _Flags = 0, int _Type = 0);
	CXR_Light(const CMat4Dfp32& _Pos, const CVec3Dfp32& _Intensity, fp32 _Range, int _Flags = 0, int _Type = 0);

	CXR_Light* GetNext() const { return m_pNext; };

	void Read(CCFile* _pFile, int _Version);
	void Write(CCFile* _pFile) const;
#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif
};

class CXR_LightID
{
public:
	CPixel32 m_IntensityInt32;

	CXR_LightID();
};

// -------------------------------------------------------------------
class CXR_ViewClipInterface;

class CXR_WorldLightState : public CReferenceCount
{
public:

	DECLARE_OPERATOR_NEW


	TArray<CXR_LightID> m_lLightIDs;
	TArray<CXR_Light> m_lDynamic;
	TArray<CXR_Light> m_lStatic;

	int m_nDynamic;
	int m_nStatic;

	CXR_WorldLightState();
	~CXR_WorldLightState();

	void Create(int _nIDs = 256, int _MaxDynamic = 16, int _MaxStatic = 32);

	void PrepareFrame();
	void Set(int _LightID, const CVec3Dfp32& _Intensity);

	void AddDynamic(const CXR_Light& _Light);
	void AddDynamic(const CVec3Dfp32& _Pos, const CVec3Dfp32& _Intensity, fp32 _Range = 256.0f, int _Flags = 0, int _Type = 0);
	void AddStatic(const CXR_Light& _Light);
	void AddStatic(int _LightID, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Intensity, fp32 _Range = 512.0f, int _Flags = 0, int _Type = 0);
	void AddLightVolume(CXR_LightVolume* _pLightVolume, const CVec3Dfp32& _ReferencePos);

	void InitLinks();				// Run this...
	CXR_Light* GetFirst();	// ...then use this.

	void CopyAndCull(const CXR_WorldLightState* _pWLS, CXR_ViewClipInterface* _pView);
	void CopyAndCull(const CXR_WorldLightState* _pWLS, fp32 _BoundR, const CVec3Dfp32 _BoundPos, int _MaxStatic, int _MaxDynamic);

	void Optimize(const CVec3Dfp32 _Pos, fp32 _Radius, fp32 _ParallellTresh, const CMat4Dfp32* _pMat = NULL);

	void Transform(const CMat4Dfp32& _Mat);

	static void LightDiffuse(CXR_Light* _pLights, int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN, int _bOmni, CPixel32* pVLight, int _Alpha = 255, fp32 _Scale = 1.0f);
	static void LightSpecular(CXR_Light* _pLights, int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN, int _Power, CPixel32* pVLight, const CVec3Dfp32& _Eye, int _Alpha = 255, fp32 _Scale = 1.0f);
};

typedef TPtr<CXR_WorldLightState> spCXR_WorldLightState;

#include "CollisionInfo.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_ModelInstance
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_ModelInstanceContext
{
public:
	CXR_ModelInstanceContext(uint32 _GameTick, fp32 _TimePerTick, void* _pObj = NULL, void* _pClient = NULL, uint32 _Flags = 0, const CXR_AnimState* _pAnimState = NULL)
		: m_GameTick(_GameTick)
		, m_TimePerTick(_TimePerTick)
		, m_pObj(_pObj)
		, m_pClient(_pClient)
		, m_pAnimState(_pAnimState)
		, m_Flags(_Flags)
	{
		m_GameTime = CMTime::CreateFromTicks(_GameTick, _TimePerTick);
	}

	uint32 m_GameTick;
	fp32 m_TimePerTick;
	CMTime m_GameTime;
	void* m_pObj;
	void* m_pClient;
	const CXR_AnimState* m_pAnimState;
	uint32 m_Flags;
};

class CXR_ModelInstance : public CReferenceCount
{
public:
	virtual void Create(CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context) {}
	virtual void OnRefresh(const CXR_ModelInstanceContext& _Context, const CMat4Dfp32 *_pMat = NULL, int _nMat = 0, int _Flags = 0) {}
	virtual bool NeedRefresh(CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context) { return false; }

	virtual TPtr<CXR_ModelInstance> Duplicate() const = 0;
	virtual void operator= (const CXR_ModelInstance &_Instance) = 0;
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_AnimState
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Anim;

#define ANIMSTATE_NUMDATA	4
#define ANIMSTATE_NUMSURF	4

class CXR_Skeleton;
class CXR_SkeletonInstance;

class CXR_AnimState
{
public:
	CMTime	m_AnimTime0;
	CMTime	m_AnimTime1;
	int16	m_Anim0;
	int16	m_Anim1;

	fp32		m_AnimAttr0;
	fp32		m_AnimAttr1;

	uint32	m_Variation : 8;
	uint32	__m_Padd0 : 24;
	
	aint	m_Data[ANIMSTATE_NUMDATA];
	
	uint16	m_SurfaceOcclusionMask;
	uint16	m_SurfaceShadowOcclusionMask;
	uint16  m_iObject;					// Used for occlusion culling, several models can be rendered using the same iObject (i.e, a character with additional items)
	uint16  m_GUID;						// Used for decals, must be unique
	uint16	m_ExcludeLightGUID;
	uint16	m_NoShadowLightGUID;
	void*	m_pContext;

	CXR_SkeletonInstance*	m_pSkeletonInst;
	CXW_Surface*			m_lpSurfaces[ANIMSTATE_NUMSURF];
	CXR_ModelInstance*		m_pModelInstance;

	void Clear()
	{
		m_AnimTime0.Reset();
		m_AnimTime1.Reset();
		m_Anim0 = 0;
		m_Anim1 = 0;
		m_AnimAttr0 = 0;
		m_AnimAttr1 = 0;
		m_Variation = 0;
		__m_Padd0 = 0;
		m_Data[0] = -1;
		m_Data[1] = -1;
		m_Data[2] = -1;
		m_Data[3] = -1;
		m_SurfaceOcclusionMask = 0;
		m_SurfaceShadowOcclusionMask = 0;
		m_iObject = 0;
		m_GUID = 0;
		m_ExcludeLightGUID = 0;
		m_NoShadowLightGUID = 0;
		m_pContext = NULL;
		m_pSkeletonInst = NULL;
		m_lpSurfaces[0] = NULL;
		m_lpSurfaces[1] = NULL;
		m_lpSurfaces[2] = NULL;
		m_lpSurfaces[3] = NULL;
		m_pModelInstance = NULL;
	}

	M_INLINE uint32 GetColor(int _iColor)
	{
		return m_Data[_iColor] & 0xFFFFffff;
	}

	CXR_AnimState()
	{
		Clear();
	}

	CXR_AnimState(CMTime _Time, CMTime _Time1, int _Anim0, int _Anim1, CXR_ModelInstance *_pInstance, int _iObject)
	{
		m_AnimTime0 = _Time;
		m_AnimTime1 = _Time1;
		m_Anim0 = _Anim0;
		m_Anim1 = _Anim1;
		m_AnimAttr0 = 0;
		m_AnimAttr1 = 0;
		m_Variation = 0;
		__m_Padd0 = 0;
		m_Data[0] = -1;
		m_Data[1] = -1;
		m_Data[2] = -1;
		m_Data[3] = -1;
		m_SurfaceOcclusionMask = 0;
		m_SurfaceShadowOcclusionMask = 0;		
		m_iObject = _iObject;
		m_GUID = 0;
		m_ExcludeLightGUID = 0;
		m_NoShadowLightGUID = 0;
		m_pContext = NULL;
		m_pSkeletonInst = NULL;
		m_lpSurfaces[0] = NULL;
		m_lpSurfaces[1] = NULL;
		m_lpSurfaces[2] = NULL;
		m_lpSurfaces[3] = NULL;
		m_pModelInstance = _pInstance;
	}

	CXR_AnimState(const CXR_AnimState &_Anim)
	{
	
		m_AnimTime0 = _Anim.m_AnimTime0;
		m_AnimTime1 = _Anim.m_AnimTime1;
		m_Anim0 = _Anim.m_Anim0;
		m_Anim1 = _Anim.m_Anim1;
		m_AnimAttr0 = _Anim.m_AnimAttr0;
		m_AnimAttr1 = _Anim.m_AnimAttr1;
		m_Variation = _Anim.m_Variation;
		__m_Padd0 = _Anim.__m_Padd0;
		m_Data[0] = _Anim.m_Data[0];
		m_Data[1] = _Anim.m_Data[1];
		m_Data[2] = _Anim.m_Data[2];
		m_Data[3] = _Anim.m_Data[3];
		m_SurfaceOcclusionMask = _Anim.m_SurfaceOcclusionMask;
		m_SurfaceShadowOcclusionMask = _Anim.m_SurfaceShadowOcclusionMask;
		m_iObject = _Anim.m_iObject;
		m_GUID = _Anim.m_GUID;
		m_ExcludeLightGUID = _Anim.m_ExcludeLightGUID;
		m_NoShadowLightGUID = _Anim.m_NoShadowLightGUID;
		m_pContext = _Anim.m_pContext;
		m_pSkeletonInst = _Anim.m_pSkeletonInst;
		m_lpSurfaces[0] = _Anim.m_lpSurfaces[0];
		m_lpSurfaces[1] = _Anim.m_lpSurfaces[1];
		m_lpSurfaces[2] = _Anim.m_lpSurfaces[2];
		m_lpSurfaces[3] = _Anim.m_lpSurfaces[3];
		m_pModelInstance = _Anim.m_pModelInstance;
	}
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model
|__________________________________________________________________________________________________
\*************************************************************************************************/
enum
{
	CXR_MODEL_CLASS_CUSTOM					= 0,
	CXR_MODEL_CLASS_TRIMESH					= 1,
	CXR_MODEL_CLASS_SOLIDPHYS				= 2,
	CXR_MODEL_CLASS_BSP1					= 0x82,
	CXR_MODEL_CLASS_BSP2					= 0x83,
	CXR_MODEL_CLASS_BSP3					= 0x84,
	CXR_MODEL_CLASS_BSP4					= 0x85,
	CXR_MODEL_CLASS_ANYBSPMASK				= 0x80,

	CXR_MODEL_INTERFACE_SCENEGRAPH			= 1,
	CXR_MODEL_INTERFACE_VIEWCLIP			= 2,
	CXR_MODEL_INTERFACE_PHYSICS				= 3,
	CXR_MODEL_INTERFACE_PATH				= 4,
	CXR_MODEL_INTERFACE_FOG					= 5,
	CXR_MODEL_INTERFACE_WALLMARK			= 6,
	CXR_MODEL_INTERFACE_SKY					= 7,

	CXR_MODEL_PARAM_SKELETON				= 1,
	CXR_MODEL_PARAM_ANIM					= 2,
	CXR_MODEL_PARAM_NUMLODS					= 3,
#ifndef PLATFORM_CONSOLE
	CXR_MODEL_PARAM_NUMTRIANGLES			= 4,
	CXR_MODEL_PARAM_NUMVERTICES				= 5,
#endif
	CXR_MODEL_PARAM_KEYS					= 6,
	CXR_MODEL_PARAM_TEXTUREPARAM			= 7, // Use SetParamfv, _pValues[0] == Param, _pValues[1] == Value
	CXR_MODEL_PARAM_NEEDPATHDATA			= 8, // Added by Mondelore.
	CXR_MODEL_PARAM_SETPICMIPGROUP			= 9,
	CXR_MODEL_PARAM_TIMEMODE				= 10,
//	CXR_MODEL_PARAM_ISALIVE					= 11,
	CXR_MODEL_PARAM_ISSHADOWCASTER			= 12,
	CXR_MODEL_PARAM_THINSOLIDS				= 13,
	CXR_MODEL_PARAM_N_THINSOLIDS			= 14,
	CXR_MODEL_PARAM_LIGHTVOLUME				= 15,
	CXR_MODEL_PARAM_CAPSULES				= 16,
	CXR_MODEL_PARAM_N_CAPSULES				= 17,
	CXR_MODEL_PARAM_REGISTRY				= 18,
	CXR_MODEL_PARAM_GLOBALSCALE				= 19,

	// Time modes
	CXR_MODEL_TIMEMODE_CONTINUOUS			= 0,
	CXR_MODEL_TIMEMODE_CONTROLLED			= 1,

	// Backwards compatible enums
	MODEL_PARAM_SKELETON					= CXR_MODEL_PARAM_SKELETON,
	MODEL_PARAM_ANIM						= CXR_MODEL_PARAM_ANIM,
	MODEL_PARAM_NUMLODS						= CXR_MODEL_PARAM_NUMLODS,
#ifndef PLATFORM_CONSOLE
	MODEL_PARAM_NUMTRIANGLES				= CXR_MODEL_PARAM_NUMTRIANGLES,
	MODEL_PARAM_NUMVERTICES					= CXR_MODEL_PARAM_NUMVERTICES,
#endif
	MODEL_PARAM_KEYS						= CXR_MODEL_PARAM_KEYS,
//	MODEL_PARAM_NEEDPATHDATA				= CXR_MODEL_PARAM_NEEDPATHDATA, // Added by Mondelore.
	
	CXR_MODEL_ONRENDERFLAGS_WORLD			= M_Bit(0),
	CXR_MODEL_ONRENDERFLAGS_CULLED			= M_Bit(1),
	CXR_MODEL_ONRENDERFLAGS_MAXLOD			= M_Bit(2),
	CXR_MODEL_ONRENDERFLAGS_MINLOD			= M_Bit(3),
	CXR_MODEL_ONRENDERFLAGS_NOCULL			= M_Bit(4),
	CXR_MODEL_ONRENDERFLAGS_NOSHADOWS		= M_Bit(5),
	CXR_MODEL_ONRENDERFLAGS_INVISIBLE		= M_Bit(6),
	CXR_MODEL_ONRENDERFLAGS_WIRE			= M_Bit(7),

	CXR_MODEL_ONRENDERFLAGS_SURFMODE_BASE	= 8,
	CXR_MODEL_ONRENDERFLAGS_SURF0_ADD		= M_Bit(8),		// 0: Replace, 1: Add
	CXR_MODEL_ONRENDERFLAGS_SURF1_ADD		= M_Bit(9),		//
	CXR_MODEL_ONRENDERFLAGS_SURF2_ADD		= M_Bit(10),		//   - " - 
	CXR_MODEL_ONRENDERFLAGS_SURF3_ADD		= M_Bit(11),		// 

	CXR_MODEL_ONRENDERFLAGS_SHADOW			= M_Bit(12),		// trimesh internal
	CXR_MODEL_ONRENDERFLAGS_NOSOLID			= M_Bit(13),		// trimesh internal
	CXR_MODEL_ONRENDERFLAGS_ISLOD			= M_Bit(14),		// trimesh internal

	CXR_MODEL_ONRENDERFLAGS_USEWLS			= M_Bit(15),		// Ogier icon rendering - ae

	CXR_MODEL_ONRENDERFLAGS_NODYNAMICLIGHT	= M_Bit(16),		// bsp2-only, used to prevent side scene from receiving light from the scene.
};



class CXR_SceneGraphInterface;
class CXR_ViewClipInterface;
class CXR_PhysicsModel;
class CXR_PathInterface;
class CXR_FogInterface;
class CXR_WallmarkInterface;
class CXR_SkyInterface;

class CXR_Model : public CReferenceCount
{
protected:
	uint32 m_ThreadSafe:1;

public:
	CXR_Model();
	uint32 GetThreadSafe() {return m_ThreadSafe;}
	void SetThreadSafe(bool _bSafe) {m_ThreadSafe = (_bSafe == true);}
	virtual void Create(const char* _pParam) {};														// Any parameter may be NULL
	virtual void Create(const char* _pParam, CDataFile* _pDFile, CCFile* _pFile) { Create(_pParam); };	// Any parameter may be NULL

	virtual int GetModelClass() { return CXR_MODEL_CLASS_CUSTOM; };
	virtual int GetRenderPass(const CXR_AnimState* _pAnimState = NULL) { return 0; };

	virtual int GetVariationIndex(const char* _pName) { return 0; };
	virtual int GetNumVariations() { return 1; };
#ifndef M_RTMCONSOLE
	virtual CStr GetVariationName(int _iVariation) { return CStr(); };
#endif
	virtual CXR_Model* OnResolveVariationProxy(const CXR_AnimState& _AnimState, CXR_AnimState& _DstAnimState)
	{
		_DstAnimState = _AnimState;
		return this;
	}

	virtual CXR_Model* GetLOD(const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, CXR_Engine* _pEngine, int *_piLod = NULL) { return this; };
	virtual CXR_Skeleton* GetSkeleton() { return NULL; };
	virtual CXR_Skeleton* GetPhysSkeleton() { return NULL; };
	virtual CXR_IndexedSolidContainer32 * GetIndexedSolidContainer() { return NULL; }
	virtual void SetIndxedSolidContainer (CXR_IndexedSolidContainer32 *_pSolidContainer) { }

	virtual aint GetParam(int _Param) { return 0; };
	virtual void SetParam(int _Param, aint _Value) { };
	virtual int GetParamfv(int _Param, fp32* _pRetValues) { return 0; };
	virtual void SetParamfv(int _Param, const fp32* _pValues) { };

	// Bounding volumes in model-space
	virtual fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState = NULL);
	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState = NULL);
	virtual void GetBound_Box(CBox3Dfp32& _Box, int _Mask, const CXR_AnimState* _pAnimState = NULL);

	// This function is called, for each view-context, AFTER all models have been enumerated, 
	// and BEFORE rendering of any portals/mirrors.
	virtual void PreRender(CXR_Engine* _pEngine, CXR_ViewClipInterface* _pViewClip,
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0) {};

	// This function is called once during rendering for each view-context.
	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0) pure;
	virtual void OnRender2(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _WVelMat, int _Flags = 0)
	{
		OnRender(_pEngine, _pRender, _pVBM, _pViewClip, _spWLS, _pAnimState, _WMat, _VMat, _Flags);
	}

	virtual void OnPrecache(CXR_Engine* _pEngine, int _iVariation) {};
	virtual void OnPostPrecache(CXR_Engine* _pEngine) {};
	virtual void OnRefreshSurfaces() {};

	// This function is called at regular intervals from the resource manager. This gives
	// the model opportunity to reduce memory consumption automatically.
	virtual void OnResourceRefresh(int _Flags = 0) {};

	// This function is called when the system asks the model to reduce memory consumption.
	virtual void OnHibernate(int _Flags = 0) {};

	// Interface access
	virtual void* GetInterface(int _Interface);
	virtual CXR_SceneGraphInterface* SceneGraph_GetInterface() { return NULL; }
	virtual CXR_ViewClipInterface* View_GetInterface() { return NULL; }
	virtual CXR_PhysicsModel* Phys_GetInterface() { return NULL; }
	virtual CXR_PathInterface* Path_GetInterface() { return NULL; }
	virtual CXR_FogInterface* Fog_GetInterface() { return NULL; }
	virtual CXR_WallmarkInterface* Wallmark_GetInterface() { return NULL; }
	virtual CXR_SkyInterface* Sky_GetInterface() { return NULL; }

	virtual TPtr<CXR_ModelInstance> CreateModelInstance() { return NULL; }
	virtual bool NeedRefresh() { return false; }

	MACRO_OPERATOR_TPTR(CXR_Model);
};

typedef TPtr<CXR_Model> spCXR_Model;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_VariationProxy
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Model_VariationProxy : public CXR_Model
{
	MRTC_DECLARE;

	spCXR_Model m_spModel;
	uint32 m_iVariation : 8;

public:
	virtual void Create(spCXR_Model _spModel, int _iVariation);

  //
  // CXR_Model overrides:
  //
	virtual void Create(const char* _pParam);										// Any parameter may be NULL
	virtual void Create(const char* _pParam, CDataFile* _pDFile, CCFile* _pFile);	// Any parameter may be NULL

	virtual int GetModelClass();
	virtual int GetRenderPass(const CXR_AnimState* _pAnimState = NULL);

	virtual int GetVariationIndex(const char* _pName);
	virtual int GetNumVariations();
#ifndef M_RTMCONSOLE
	virtual CStr GetVariationName(int _iVariation);
#endif
	virtual CXR_Model* OnResolveVariationProxy(const CXR_AnimState& _AnimState, CXR_AnimState& _DstAnimState);

	virtual CXR_Model* GetLOD(const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, CXR_Engine* _pEngine, int *_piLod = NULL);
	virtual CXR_Skeleton* GetSkeleton();
	virtual CXR_Skeleton* GetPhysSkeleton();

	virtual aint GetParam(int _Param);
	virtual void SetParam(int _Param, aint _Value);
	virtual int GetParamfv(int _Param, fp32* _pRetValues);
	virtual void SetParamfv(int _Param, const fp32* _pValues);

	// Bounding volumes in model-space
	virtual fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState = NULL);
	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState = NULL);
	virtual void GetBound_Box(CBox3Dfp32& _Box, int _Mask, const CXR_AnimState* _pAnimState = NULL);

	// This function is called, for each view-context, AFTER all models have been enumerated, 
	// and BEFORE rendering of any portals/mirrors.
	virtual void PreRender(CXR_Engine* _pEngine, CXR_ViewClipInterface* _pViewClip,
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0);

	// This function is called once during rendering for each view-context.
	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0);

	virtual void OnPrecache(CXR_Engine* _pEngine, int _iVariation);
	virtual void OnPostPrecache(CXR_Engine* _pEngine);
	virtual void OnRefreshSurfaces();

	// This function is called at regular intervals from the resource manager. This gives
	// the model opportunity to reduce memory consumption automatically.
	virtual void OnResourceRefresh(int _Flags = 0);

	// This function is called when the system asks the model to reduce memory consumption.
	virtual void OnHibernate(int _Flags = 0);

	// Interface access
	virtual void* GetInterface(int _Interface);
	virtual CXR_SceneGraphInterface* SceneGraph_GetInterface();
	virtual CXR_ViewClipInterface* View_GetInterface();
	virtual CXR_PhysicsModel* Phys_GetInterface();
	virtual CXR_PathInterface* Path_GetInterface();
	virtual CXR_FogInterface* Fog_GetInterface();
	virtual CXR_WallmarkInterface* Wallmark_GetInterface();
	virtual CXR_SkyInterface* Sky_GetInterface();

	virtual TPtr<CXR_ModelInstance> CreateModelInstance();
	virtual bool NeedRefresh();
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_SceneGraphInstance
|__________________________________________________________________________________________________
\*************************************************************************************************/
enum
{
	CXR_SCENEGRAPH_NOPORTALPVSCULL = 1,
	CXR_SCENEGRAPH_SHADOWCASTER = 2,

	CXR_SCENEGRAPH_CREATIONFLAGS_DONTCLEARINTENSITY = 1,
};


class CXR_SceneGraphInstance : public CReferenceCount
{
public:
	virtual void Create(int _ElemHeap, int _LightHeap, int _CreationFlags = 0) pure;

	virtual void SceneGraph_CommitDeferred() {};

	// -------------------------------------------------------------------
	// Object linking

	virtual void SceneGraph_LinkElement(uint16 _Elem, const CBox3Dfp32& _Box, int _Flags) pure;
	virtual void SceneGraph_LinkInfiniteElement(uint16 _Elem, int _Flags) pure;
	virtual void SceneGraph_UnlinkElement(uint16 _Elem) pure;
	virtual int SceneGraph_EnumerateElementNodes(int _Elem, uint32* _piRetNodes, int _MaxRet) pure;

	virtual int SceneGraph_EnumerateNodes(const uint32* _piNodes, int _nNodes, uint16* _pRetElem, int _MaxRet, const uint16* _pMergeElem = NULL, int _nMergeElem = 0) pure;
	virtual int SceneGraph_EnumeratePVS(const uint8* _pPVS, uint16* _pRetElem, int _MaxRet, const uint16* _pMergeElem = NULL, int _nMergeElem = 0) pure;
	virtual int SceneGraph_EnumerateView(int _iView, uint16* _pRetElem, int _MaxRet, const uint16* _pMergeElem = NULL, int _nMergeElem = 0) pure;

	// -------------------------------------------------------------------
	// Lighting

	virtual int SceneGraph_Light_GetMaxIndex() { return 0; };
	virtual void SceneGraph_Light_LinkDynamic(const CXR_Light& _Light) pure;					// NOTE: You MUST set m_LightGUID to a unique value! Otherwise you might overwrite other lights.
	virtual void SceneGraph_Light_AddPrivateDynamic(const CXR_Light& _Light) pure;				// NOTE: You MUST set m_LightGUID to a unique value! Otherwise you might overwrite other lights.
	virtual void SceneGraph_Light_ClearDynamics() pure;											// Remove all dynamic lights from the SGI. Typically only called by system code.
	virtual int SceneGraph_Light_GetIndex(int _LightGUID) pure;									// Convert LightGUID to iLight, if return == 0 _LightGUID doesn't exist.
	virtual int SceneGraph_Light_GetGUID(int _iLight) pure;										// Convert iLight to LightGUID, if return == 0 iLight doesn't exist.
	virtual void SceneGraph_Light_Unlink(int _iLight) pure;

	virtual void SceneGraph_Light_SetIntensity(int _iLight, const CVec3Dfp32& _Intensity, bool _bIsOff) pure;
	virtual void SceneGraph_Light_SetRotation(int _iLight, const CMat4Dfp32& _Rotation) pure;	// Invalid operation on static spot lights
	virtual void SceneGraph_Light_SetPosition(int _iLight, const CMat4Dfp32& _Pos) pure;			// Invalid operation on static lights
	virtual void SceneGraph_Light_SetProjectionMap(int _iLight, int _TextureID, const CMat4Dfp32* _Pos) pure;
	virtual void SceneGraph_Light_Get(int _iLight, CXR_Light& _Light) pure;
	virtual void SceneGraph_Light_GetIntensity(int _iLight, CVec3Dfp32& _Intensity) { _Intensity = 1.0f; };	// Returns (1, 1, 1) if _iLight is invalid
	
	virtual int SceneGraph_Light_Enum(const CBox3Dfp32& _Box, const CXR_Light** _lpLights, int _nMaxLights) pure;	// Returns number of lights written to _lpLights
	virtual int SceneGraph_Light_Enum(const uint16* _piPL, int _nPL, const CBox3Dfp32& _Box, const CXR_Light** _lpLights, int _nMaxLights) pure;	// Returns number of lights written to _lpLights
};

typedef TPtr<CXR_SceneGraphInstance> spCXR_SceneGraphInstance;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_SceneGraphInterface
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_SceneGraphInterface
{
public:
	// PVS
	virtual void SceneGraph_PVSInitCache(int _NumCached) pure;
	virtual int SceneGraph_PVSGetLen() pure;
	virtual bool SceneGraph_PVSGet(int _PVSType, const CVec3Dfp32& _Pos, uint8* _pDst) pure;
	virtual const uint8* SceneGraph_PVSLock(int _PVSType, const CVec3Dfp32& _Pos) pure;
	virtual void SceneGraph_PVSRelease(const uint8* _pPVS) pure;
	virtual int SceneGraph_GetPVSNode(const CVec3Dfp32& _Pos) pure;

	// Scenegraph
	virtual spCXR_SceneGraphInstance SceneGraph_CreateInstance() pure;
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_ViewClipInterface
|__________________________________________________________________________________________________
\*************************************************************************************************/
#define XR_VIEWCLIPSTATE_PORTAL		0

class CXR_ViewClipInterface
{
public:
	// Visibility interface
	virtual void View_Reset(int _iView);
	virtual void View_SetState(int _iView, int _State, int _Value);

	virtual void View_Init(int _iView, CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_ViewClipInterface* _pViewClip,
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0) pure;
	virtual void View_Init(int _iView, CXR_Engine* _pEngine, CRenderContext* _pRender, CVec3Dfp32* _pVPortal, int _nVPortal,
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0) pure;
	virtual void View_SetCurrent(int _iView, CXR_SceneGraphInstance* _pSceneGraphInstance) pure;
	virtual bool View_GetClip_Sphere(CVec3Dfp32 _v0, fp32 _Radius, int _MediumFlags, int _ObjectMask, CRC_ClipVolume* _pClipVolume, CXR_RenderInfo* _pRenderInfo) pure;
	virtual bool View_GetClip_Box(CVec3Dfp32 _min, CVec3Dfp32 _max, int _MediumFlags, int _ObjectMask, CRC_ClipVolume* _pClipVolume, CXR_RenderInfo* _pRenderInfo) pure;
	virtual void View_GetClip(int _Elem, CXR_RenderInfo* _pRenderInfo) {};

	// -------------------------------------------------------------------
	// Light stuff

	virtual const uint16* View_Light_GetVisible(int& _nRetLights) { _nRetLights = 0; return NULL; }		// Return index list of visible lights in the scene
	virtual CXR_LightOcclusionInfo* View_Light_GetOcclusionArray(int& _nLights) { return NULL; _nLights = 0; }	// Return occlusion for a single light
	virtual CXR_LightOcclusionInfo* View_Light_GetOcclusion(int _iLight) { return NULL; }				// Return occlusion for a single light
	virtual void View_Light_ApplyOcclusionArray(int _nLights, const uint16* _piLights, const CXR_LightOcclusionInfo* _pLO) {}
	virtual void View_Light_ApplyOcclusionArray_ShadowShaded(uint _nLights, const uint16* _piLights, const CScissorRect* _pScissors) {}
	virtual int View_Light_GetOcclusionSize() {return 0;}
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_PhysicsModel
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_PhysicsContext
{
public:
	CMat4Dfp32	m_WMat;
	CMat4Dfp32	m_WMatInv;
	class CWireContainer* m_pWC;
	const CXR_AnimState* m_pAnimState;
	uint16 m_PhysGroupMaskThis;
	uint16 m_PhysGroupMaskCollider;

	CXR_PhysicsContext()
	{
		m_WMat.Unit();
		m_WMatInv.Unit();
		m_pWC	= 0;
		m_pAnimState = 0;
		m_PhysGroupMaskThis = ~0;
		m_PhysGroupMaskCollider = ~0;
	}

	CXR_PhysicsContext(const CMat4Dfp32 &_WMat, const class CXR_AnimState* _pAnimState = NULL, class CWireContainer* _pWC = NULL)
	{
		m_WMat	= _WMat;
		m_WMat.InverseOrthogonal(m_WMatInv);
		m_pWC	= _pWC;
		m_pAnimState = _pAnimState;
		m_PhysGroupMaskThis = ~0;
		m_PhysGroupMaskCollider = ~0;
	}

	CXR_PhysicsContext(const CMat4Dfp32 &_WMat, uint16 _PhysGroupMaskThis, uint16 _PhysGroupMaskCollider, const class CXR_AnimState* _pAnimState, class CWireContainer* _pWC)
	{
		m_WMat	= _WMat;
		m_WMat.InverseOrthogonal(m_WMatInv);
		m_pWC	= _pWC;
		m_pAnimState = _pAnimState;
		m_PhysGroupMaskThis = _PhysGroupMaskThis;
		m_PhysGroupMaskCollider = _PhysGroupMaskCollider;
	}

};

class CXR_PhysicsModel : public CXR_Model
{
public:
	// Bounding volumes are in world-space.
	virtual void Phys_GetBound_Sphere(const CMat4Dfp32& _Pos, CVec3Dfp32& _RetPos, fp32& _Radius) pure;
	virtual void Phys_GetBound_Box(const CMat4Dfp32& _Pos, CBox3Dfp32& _RetBox) pure;

	// Collision services. All indata is in world coordinates.
	virtual void Phys_Init(CXR_PhysicsContext* _pPhysContext) pure;

	virtual int Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0) pure;
	virtual void Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, CXR_MediumDesc& _RetMedium) pure;
	
	virtual bool Phys_IntersectLine(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL) pure;
	virtual bool Phys_IntersectSphere(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL) pure;
	virtual bool Phys_IntersectBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _BoxOrigin, const CPhysOBB& _BoxDest, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL) pure;

	/*
	virtual int Phys_CollideBox(const CBox3Dfp32& _Box, const CMat4Dfp32& _Position, int _MediumFlags, CCollisionInfo* _pCollisionInfo, int nMaxCollisions) 
	{
		CXR_PhysicsContext context(_Position);
		bool collide =  Phys_IntersectBox(&context, _Box, _Box, _MediumFlags, _pCollisionInfo);
		return collide ? 1 : 0;
	}*/

	/*
		Förflyttningsriktning på boxen i Phys_CollideBox
	 */

	virtual int Phys_CollideBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _Box, int _MediumFlags, CCollisionInfo* _pCollisionInfo, int _nMaxCollisions) 
	{
		return 0;
	}

	virtual int Phys_CollideBSP2(CXR_PhysicsContext* _pPhysContext, class CXR_IndexedSolidContainer32 *_pSolidContainer, const CMat4Dfp32& _BSP2Transform, const CVec3Dfp32& _Offset, int _MediumFlags, CCollisionInfo* _pCollisionInfo, int _nMaxCollisions) 
	{
		return 0;
	}

	virtual int Phys_CollideCapsule(CXR_PhysicsContext* _pPhysContext, const CPhysCapsule& _Capsule,int _MediumFlags, CCollisionInfo* _pCollisionInfo, int _nMaxCollisions)
	{
		return 0;
	}

	virtual void CollectPCS(CXR_PhysicsContext* _pPhysContext, const uint8 _IN1N2Flags, CPotColSet *_pcs, const int _iObj, const int _MediumFlags ) pure;

	virtual void Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32* _pV, int _nV, uint32* _pRetMediums);
	virtual void Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32* _pV, int _nV, CXR_MediumDesc* _pRetMediums);
	
	virtual int Phys_GetCombinedMediumFlags(CXR_PhysicsContext* _pPhysContext, const CBox3Dfp32& _Box) { return XW_MEDIUM_AIR | XW_MEDIUM_SEETHROUGH; };

	virtual CXR_PhysicsModel* Phys_GetInterface() { return this; };
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Wallmark interface
|__________________________________________________________________________________________________
\*************************************************************************************************/
#define XR_WALLMARK_MAXTEXTUREPARAMS 2


class CXR_WallmarkDesc
{
public:
	CMTime m_SpawnTime;
	int32 m_SurfaceID;
	uint16 m_SurfaceParam_TextureID[XR_WALLMARK_MAXTEXTUREPARAMS];
	uint32 m_Flags : 16;
	uint32 m_iNode : 16;
	uint32 m_GUID;
	fp32 m_Size;

	CXR_WallmarkDesc()
	{
		m_SurfaceID = 0;
		for(int i = 0; i < XR_WALLMARK_MAXTEXTUREPARAMS; i++)
			m_SurfaceParam_TextureID[i] = 0;
		m_Flags = 0;
		m_iNode = 0;
		m_Size = 16.0f;
	}
};

#define XR_WALLMARK_NOFADE	1
#define XR_WALLMARK_ALLOW_PERPENDICULAR 2
#define XR_WALLMARK_TEMPORARY 4
#define XR_WALLMARK_NEVERDESTROY 8		// cant be combined with Temporary
#define XR_WALLMARK_SCANFOROVERLAP 16	// Scan for nearby wallmarks with same surface and render with surf-seq 2 if too close
#define XR_WALLMARK_DESTROYED 32
#define XR_WALLMARK_PRIMARY 64			// Set on the first wallmark created from a wallmark-description
#define XR_WALLMARK_OVERLAPPING 128		// Overlapping wallmark, render with surf-seq 1 instead.
#define XR_WALLMARK_CREATE_TANGENTS 256	// Create tangent vectors for wallmark
#define XR_WALLMARK_NOANIMATE 512		// Use time provided in SpawnTime, but do not animate it


class CXR_WallmarkContextCreateInfo
{
public:
	int m_CapacityDynamic;		// Recycled wallmarks.. blood, bulletholes, etc..
	int m_CapacityStatic;		// Permanent stuff..  grafitti, etc..
	int m_CapacityTemp;			// Decals that only live for one frame
};


class CXR_WallmarkInterface
{
public:
	virtual int Wallmark_CreateContext(const CXR_WallmarkContextCreateInfo& _CreateInfo) pure;
	virtual void Wallmark_DestroyContext(int _hContext) pure;
	virtual int Wallmark_Create(int _hContext, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, int _Material = 0) pure;
	virtual void Wallmark_CreateWithContainer(void* _pContainer, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, int _Material = 0) {};
	virtual bool Wallmark_Destroy(int _GUID) { return false; }
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_SkyInterface
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_SkyInterface
{
public:
	virtual void Sky_PrepareFrame(CXR_Engine* _pEngine) pure;
	virtual void Sky_ClearFrustrum(CXR_Engine* _pEngine) pure;		// Call to clear visible areas.
	virtual void Sky_OpenFrustrum(CXR_Engine* _pEngine) pure;		// Call before Render if the whole screen should be filled with sky
	virtual void Sky_ExpandFrustrum(CXR_Engine* _pEngine, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CVec3Dfp32* _pV, int _nV) pure;
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_PhysicsModel_Sphere
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_PhysicsModel_Sphere : public CXR_PhysicsModel
{
protected:
	fp32 m_PhysRadius;

public:
	virtual void Phys_SetDimensions(fp32 _Radius);

	virtual void Phys_GetBound_Sphere(const CMat4Dfp32& _Pos, CVec3Dfp32& _RetPos, fp32& _Radius);
	virtual void Phys_GetBound_Box(const CMat4Dfp32& _Pos, CBox3Dfp32& _RetBox);

	// Collision services. All indata is in world coordinates.
	virtual void Phys_Init(CXR_PhysicsContext* _pPhysContext);
	virtual int Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0);
	virtual void Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, CXR_MediumDesc& _RetMedium);
	virtual bool Phys_IntersectLine(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);
	virtual bool Phys_IntersectSphere(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);
	virtual bool Phys_IntersectBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _BoxOrigin, const CPhysOBB& _BoxDest, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);

	virtual void CollectPCS(CXR_PhysicsContext* _pPhysContext, const uint8 _IN1N2Flags, CPotColSet *_pcs, const int _iObj, const int _MediumFlags ) {};

	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags) {};
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_PhysicsModel_Box
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_PhysicsModel_Box : public CXR_PhysicsModel
{
protected:
	CPhysOBB m_Box;

public:
	virtual void Phys_SetDimensions(const CVec3Dfp32& _Dim);

	virtual void Phys_GetBound_Sphere(const CMat4Dfp32& _Pos, CVec3Dfp32& _RetPos, fp32& _Radius);
	virtual void Phys_GetBound_Box(const CMat4Dfp32& _Pos, CBox3Dfp32& _RetBox);

	// Collision services. All indata is in world coordinates.
	virtual void Phys_Init(CXR_PhysicsContext* _pPhysContext);
	virtual int Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0);
	virtual void Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, CXR_MediumDesc& _RetMedium);
	virtual bool Phys_IntersectLine(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);
	virtual bool Phys_IntersectSphere(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);
	virtual bool Phys_IntersectBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _BoxOrigin, const CPhysOBB& _BoxDest, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);

	virtual int Phys_CollideBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _Box, int _MediumFlags, CCollisionInfo* _pCollisionInfo, int _nMaxCollisions);

	virtual void CollectPCS(CXR_PhysicsContext* _pPhysContext, const uint8 _IN1N2Flags, CPotColSet *_pcs, const int _iObj, const int _MediumFlags ) {};

	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags) {};
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_PhysicsModel_Box
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_IndexedPortal			// 8 bytes
{
public:
	int32 m_iiVertices;
	uint8 m_nVertices;
	uint8 m_Flags;
	uint16 m_PortalID;		// >0 : User portal

	CXR_IndexedPortal()
	{
		m_iiVertices = 0;
		m_nVertices = 0;
		m_Flags = 0;
		m_PortalID = 0;
	}

	void Read(CCFile* _pF)
	{
		_pF->ReadLE(m_iiVertices);
		_pF->ReadLE(m_nVertices);
		_pF->ReadLE(m_Flags);
		_pF->ReadLE(m_PortalID);
	}

	void Write(CCFile* _pF) const
	{
		_pF->WriteLE(m_iiVertices);
		_pF->WriteLE(m_nVertices);
		_pF->WriteLE(m_Flags);
		_pF->WriteLE(m_PortalID);
	}
};


#endif //_INC_XR_Class


