/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			WModel_EffectSystem.h

Author:			Patrik Willbo

Copyright:		2006 Starbreeze Studios AB

Contents:		CFXSysUtil					Helper
				CSpline_Beam				Create render beams on splines
				CSpline_BeamStrip			Create a beamstrip from spline
				CEffectSystemRenderChain	Render storage
				CEffectSystemRenderData		Used in render chain
				CFXLayer					Overloading particle systems using layers
				CFXLayerTwirl				Twirl particles
				CFXLayerBoneBind			Spawn particles on bones
				CFXLayerBoxSpawn			Spawn particles in a collection of boxes
				CFXLayerNoise				Apply noise on particles
				CFXLayerPath				Follow path
				CFXDataCollect				Collect setup/init data
				CFXDataObject				Base fx object
				CFXDataShader				Shader object
				CFXDataRenderTarget			Render target object
				CFXDataBeam					Beam object
				CFXDataLight				Light object
				CFXDataWallmark				Wallmark object
				CFXDataModel				Model object
				CFXDataQuad					Quad object
				CFXDataCone					Cone object
				CFXDataParticlesOpt			ParticlesOpt wrapped
				CFXDataBeamStrip			Beam strip object
				CFXDataParticleHook			Hook into ParticlesOpt class for layer rendering
				CFXDataFXParticle			FXParticle containing hook and layers
				CEffectSystemHistory		History object
				CXR_Model_EffectSystem		Main model taking care of all data objects

Comments:

History:
\*____________________________________________________________________________________________*/
#ifndef __WModel_EffectSystem_h__
#define __WModel_EffectSystem_h__


#include "CSurfaceKey.h"
#include "ModelsMisc.h"
#include "WModel_ParticlesOpt.h"
#include "CDynamicVB.h"
#include "../../../../Shared/MOS/XR/XRVBUtil.h"


#define FXSIZE_TYPE_BEAMSTRIP		sizeof(CEffectSystemHistory::CFXDataBeamStripHistory)
#define FXSIZE_TYPE_BEAM			sizeof(CEffectSystemHistory::CFXDataBeamHistory)
#define FXSIZE_TYPE_CONE			sizeof(CEffectSystemHistory::CFXDataConeHistory)
#define FXSIZE_TYPE_QUAD			sizeof(CEffectSystemHistory::CFXDataQuadHistory)
#define FXRAND_KEYS					4


class CFXDataShader;
class CFXDataRenderTarget;


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Enums
|__________________________________________________________________________________________________
\*************************************************************************************************/
enum
{
	FXDATA_TYPE_BEAM						= 0,
	FXDATA_TYPE_MODEL,
	FXDATA_TYPE_QUAD,
	FXDATA_TYPE_PARTICLESOPT,
	FXDATA_TYPE_BEAMSTRIP,
	FXDATA_TYPE_FXPARTICLE,
	FXDATA_TYPE_CONE,
	FXDATA_TYPE_SHADER,
	
	FXDATA_TYPE_LIGHT,						// Requires CWObject_EffectSystem to function properly
	FXDATA_TYPE_WALLMARK,					// Requires CWObject_EffectSystem to function properly

	// Pre defined shader parameters that's always passed in
//	FXSHADER_TEXGEN_PARAMS					= 4,
//	FXSHADER_FRAGMENT_PARAMS				= 5,
//	FXSHADER_PREDEFINED_PARAMS				= FXSHADER_TEXGEN_PARAMS+FXSHADER_FRAGMENT_PARAMS,

	FXSHADER_TEXGEN_NONE					= 0,
	FXSHADER_TEXGEN_SCREENCOORD				= 1,
	FXSHADER_TEXGEN_EMITPOSITION			= 2,

	FXSHADER_MAP_FRAMEBUFFER				= -1,					// If this is set, We will parse in the framebuffer as texture map
	FXSHADER_MAP_DEPTHBUFFER				= -2,					// Uses the depth buffer as texture map
	FXSHADER_MAP_DEPTHSTENCILBUFFER			= -3,					// Uses the depth stencil buffer as texture map
	FXSHADER_MAP_DEFERREDDIFFUSE			= -4,					// Use the deferred diffuse texture as texture map
	FXSHADER_MAP_DEFERREDNORMAL				= -5,					// Use the deferred normal texture as texture map
	FXSHADER_MAP_DEFERREDSPECULAR			= -6,					// Use the deferred specular texture as texture map
	FXSHADER_MAP_LASTSCREEN					= -7,					// Uses the last screen as texture map
	FXSHADER_MAP_SHADOWMASK					= -8,					// Uses the shadow mask texture as texture map
	FXSHADER_MAP_POSTPROCESS				= -9,					// Uses the post process texture as texture map
	FXSHADER_MAP_RENDERTARGET				= -100,					// Reserved for information linking, tells us we want to use a render target.

	FXANIM_DATA_FLAGS						= 0,					// AnimState data field for flags
	FXANIM_DATA_DATA1						= 1,					// AnimState data field for custom data
	FXANIM_DATA_DATA2						= 2,					// AnimState data field for custom data

	// FXANIM_DATA_FLAGS
	FXANIM_FLAGS_USESKELETON				= M_Bit(0),				// Data1 & Data2 is skeleton data
	FXANIM_FLAGS_USEBOXES					= M_Bit(1),				// Data1 is box data and Data2 is num of boxes
	FXANIM_FLAGS_ANIMTIME0					= M_Bit(2),				// Use animtime1 as animation time in shader
	FXANIM_FLAGS_RESOLVE					= M_Bit(3),				// Resolves screen area around effect

	// FXANIM_FLAGS_USESKELETON
	FXANIM_DATA_SKELETONTYPE				= FXANIM_DATA_DATA1,	// Skeleton instance data
	FXANIM_DATA_SKELETON					= FXANIM_DATA_DATA2,	// Skeleton data
	
	// FXANIM_DATA_USEBOXES
	FXANIM_DATA_NUMBOXES					= FXANIM_DATA_DATA1,	// Number of boxes
	FXANIM_DATA_BOXES						= FXANIM_DATA_DATA2,	// Boxes data

	FXDATA_REG_TYPE							= 0,
	FXDATA_REG_FLAGS						= 1,
	FXDATA_REG_SURFACE						= 2,
	FXDATA_REG_DURATION						= 3,
	FXDATA_REG_LENGTH						= 4,
	FXDATA_REG_WIDTH						= 5,

	FXFLAGS_TIMEMODE_CONTINUOUS				= M_Bit(0),		// Continuous system, keeps rendering forever and ever
	FXFLAGS_TIMEMODE_CONTROLLED				= M_Bit(1),		// Controlled system, only renderes when "triggered"
	FXFLAGS_TIMEMODE_CONTINUOUSCONTROLLED	= M_Bit(2),		// Controlled continuous system
	FXFLAGS_HASHISTORY						= M_Bit(3),		// Objects in system allows history
	FXFLAGS_ISALIVE							= M_Bit(4),		// Is system alive
	FXFLAGS_HASCOLLISION					= M_Bit(5),		// Objects in system allows for world collision
	FXFLAGS_HASSHADER						= M_Bit(6),		// Do we need to calculate shader parameters?
	FXFLAGS_RENDERVIS						= M_Bit(7),		// Does effect system object need to run OnClientRenderVis to function properly ?
	FXFLAGS_RENDERCLIENT					= M_Bit(8),		// Does effect system object required to run OnClientRender to function properly ?
	
	FXFLAGS_INT_HASCALCEDBOUNDBOX			= M_Bit(0),		// Is this bounding box pre calculated already?
	FXFLAGS_INT_ANIMATED					= M_Bit(1),		// Is this effect animated?

	// Entity flags
	FXFLAG_BEAM_EDGEFADE					= M_Bit(0),
	FXFLAG_BEAM_ALLOWHISTORY				= M_Bit(1),
	FXFLAG_BEAM_PLANE_X						= M_Bit(2),
	FXFLAG_BEAM_PLANE_Y						= M_Bit(3),
	FXFLAG_BEAM_PLANE_Z						= M_Bit(4),
	FXFLAG_BEAM_PLANE_ALL					= M_Bit(5),
	FXFLAG_BEAM_MULTI						= M_Bit(6),

	FXFLAG_QUAD_BILLBOARD					= M_Bit(0),
	FXFLAG_QUAD_PLANE_XY					= M_Bit(1),
	FXFLAG_QUAD_PLANE_XZ					= M_Bit(2),
	FXFLAG_QUAD_PLANE_YZ					= M_Bit(3),
	FXFLAG_QUAD_HEIGHT						= M_Bit(4),
	FXFLAG_QUAD_EDGEFADE					= M_Bit(5),
	FXFLAG_QUAD_ALLOWHISTORY				= M_Bit(6),
	FXFLAG_QUAD_WIDTH2						= M_Bit(7),
	//FXFLAG_QUAD_THIS_IS_A_NO_NO			= M_Bit(8),

	FXFLAG_BEAMSTRIP_ALLOWHISTORY			= M_Bit(0),
	FXFLAG_BEAMSTRIP_PLANE_XY				= M_Bit(1),
	FXFLAG_BEAMSTRIP_PLANE_XZ				= M_Bit(2),
	FXFLAG_BEAMSTRIP_PLANE_YZ				= M_Bit(3),

	FXFLAG_FXPARTICLE_ALLOWHISTORY			= M_Bit(0),
	FXFLAG_FXPARTICLE_WORLDCOLLIDE			= M_Bit(1),

	FXFLAG_CONE_PLANE_XY					= M_Bit(0),
	FXFLAG_CONE_PLANE_XZ					= M_Bit(1),
	FXFLAG_CONE_PLANE_YZ					= M_Bit(2),
	FXFLAG_CONE_SPHEROID					= M_Bit(3),
	FXFLAG_CONE_ALLOWHISTORY				= M_Bit(4),

	FXFLAG_LIGHT_NOSHADOWS					= M_Bit(0),
	FXFLAG_LIGHT_NODIFFUSE					= M_Bit(1),
	FXFLAG_LIGHT_NOSPECULAR					= M_Bit(2),
	FXFLAG_LIGHT_ANIMTIME					= M_Bit(3),
	FXFLAG_LIGHT_EXCLUDEOWNER				= M_Bit(4),

	FXPRIO_NA								= 0,
	FXPRIO_POSTPROCESS						= 1,
	FXPRIO_TRANSPARENT						= 2,
	FXPRIO_OPAQUE							= 3,
	FXPRIO_SURFACE_TRANSPARENT				= 4,
	FXPRIO_SURFACE_OPAQUE					= 5,

	//FXFLAG_SHADER_PRIO_LIGHT				= M_Bit(0),
	FXFLAG_SHADER_WRITEZFIRST				= M_Bit(0),
	FXFLAG_SHADER_NOANIMATE					= M_Bit(1),
//	FXFLAG_SHADER_PRIO_POSTPROCESS			= M_Bit(2),
	FXFLAG_SHADER_ADVANCEDSURFACE			= M_Bit(2),
//	FXFLAG_SHADER_PRIO_TRANSPARENT			= M_Bit(4),
//	FXFLAG_SHADER_PRIO_OPAQUE				= M_Bit(5),
//	FXFLAG_SHADER_PRIO_SURFACE_TRANSPARENT	= M_Bit(6),
//	FXFLAG_SHADER_PRIO_SURFACE_OPAQUE		= M_Bit(7),

	// Randomizing flags
	FXRAND_QUAD_SURFACE						= M_Bit(0),
	FXRAND_QUAD_WIDTH						= M_Bit(1),
	FXRAND_QUAD_ROTATEANGLE					= M_Bit(2),
	FXRAND_QUAD_COLOR						= M_Bit(3),
	FXRAND_QUAD_BASEROTATEANGLE				= M_Bit(4),

	FXRAND_BEAMSTRIP_SURFACE				= M_Bit(0),
	FXRAND_BEAMSTRIP_WIDTH					= M_Bit(1),
	FXRAND_BEAMSTRIP_SEGMENTS				= M_Bit(2),
	FXRAND_BEAMSTRIP_STRIPHEIGHT			= M_Bit(3),
	FXRAND_BEAMSTRIP_COLOR					= M_Bit(4),
	FXRAND_BEAMSTRIP_WIDTHNOISE				= M_Bit(5),
	FXRAND_BEAMSTRIP_POSNOISE				= M_Bit(6),
	FXRAND_BEAMSTRIP_VELOCITY				= M_Bit(7),
	FXRAND_BEAMSTRIP_TEXTURESCROLL			= M_Bit(8),
	FXRAND_BEAMSTRIP_VELOCITYNOISE			= M_Bit(9),
	FXRAND_BEAMSTRIP_SEED					= M_Bit(10),

	FXRAND_BEAM_SURFACE						= M_Bit(0),
	FXRAND_BEAM_WIDTH						= M_Bit(1),
	FXRAND_BEAM_LENGTH						= M_Bit(2),
	FXRAND_BEAM_COLOR						= M_Bit(3),
	FXRAND_BEAM_DIRECTION					= M_Bit(4),

	FXRAND_CONE_RADIUS						= M_Bit(0),
	FXRAND_CONE_SURFACE						= M_Bit(1),
	FXRAND_CONE_BASEROTATEANGLE				= M_Bit(2),

	// Render flags
	FXFLAG_RENDERQUAD_X2					= M_Bit(0),
	FXFLAG_RENDERQUAD_X4					= M_Bit(1),

	BEAMSPLINE_FLAGS_LIGHTING				= M_Bit(0),
};


template <int _tSize>
M_INLINE int TFXAlign(int _Size)
{
	return (_Size + _tSize - 1) & ~(_tSize - 1);
}


template <class _Type, int _tSize>
M_INLINE _Type* TFXAlignGet(uint8*& _pVBMem)
{
	_Type* pRet = (_Type*)_pVBMem;
	_pVBMem += TFXAlign<_tSize>(sizeof(_Type));
	return pRet;
}


template <class _Type, int _tSize>
M_INLINE _Type* TFXAlignGetArray(uint8*& _pVBMem, int _Len)
{
	_Type* pRet = (_Type*)_pVBMem;
	_pVBMem += TFXAlign<_tSize>(sizeof(_Type) * _Len);
	return pRet;
}


template <class _Type>
M_INLINE _Type* TFXAlignGetMem(uint8*& _pVBMem, int _IncPtr)
{
	if (_IncPtr)
	{
		_Type* pRet = (_Type*)_pVBMem;
		_pVBMem += _IncPtr;
		return pRet;
	}
	return NULL;
}


#define FXAlign4	TFXAlign<4>
#define FXAlign16	TFXAlign<16>


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXVBMAllocUtil
|__________________________________________________________________________________________________
\*************************************************************************************************/
//#define FXVBM_USE_INCORRECT
//#define FXVBMALLOCUTIL_DEBUG


#if defined(FXVBMALLOCUTIL_DEBUG)
	#define FXVBMASSERT0(f, Mess)	M_ASSERT(f, Mess)
	#define FXVBMASSERT1(f, Mess)	M_ASSERT(f, Mess)
#else
	#define FXVBMASSERT0(f, Mess)
	#define FXVBMASSERT1(f, Mess)
#endif


// Throw this, (it was once used for debug purposes but is no longer needed.)
enum
{
#ifdef FXVBM_USE_INCORRECT

	CFXVBM_ATTRIB		= 1,
	CFXVBM_VERTICES		= 2,
	CFXVBM_TVERTICES0	= 4,
	CFXVBM_TVERTICES1	= 8,
	CFXVBM_TVERTICES2	= 16,
	CFXVBM_TVERTICES3	= 32,
	CFXVBM_COLORS		= 64,
	CFXVBM_DIFFUSE		= 128,
	CFXVBM_SPECULAR		= 256,
//	CFXVBM_FOG			= 512,

	CFXVBM_TVERTICESALL	= 4+8+16+32,

#else

	CFXVBM_ATTRIB		= CXR_VB_ATTRIB,
	CFXVBM_VERTICES		= CXR_VB_VERTICES,
	CFXVBM_COLORS		= CXR_VB_COLORS,
	CFXVBM_DIFFUSE		= CXR_VB_DIFFUSE,
	CFXVBM_SPECULAR		= CXR_VB_SPECULAR,
	CFXVBM_TVERTICES0	= CXR_VB_TVERTICES0,
	CFXVBM_TVERTICES1	= CXR_VB_TVERTICES1,
	CFXVBM_TVERTICES2	= CXR_VB_TVERTICES2,
	CFXVBM_TVERTICES3	= CXR_VB_TVERTICES3,
//	CFXVBM_FOG			= CXR_VB_FOG,

	CFXVBM_TVERTICESALL	= CXR_VB_TVERTICESALL,

#endif
};


#define CFXVBMALLOCUTIL_NRM(_Type, _AllocName) public: \
	M_INLINE void Alloc_##_AllocName() { m_AllocSize += (sizeof(_Type) + 15) & ~15; FXVBMASSERT0(((m_AllocSize + 15) & ~15) == m_AllocSize, "Alloc_" #_AllocName "(): Is not aligned!\n"); } \
	M_INLINE _Type* Get_##_AllocName() { _Type* pRet = (_Type*)m_pAllocMem; m_pAllocMem += (sizeof(_Type) + 15) & ~15;  \
										 FXVBMASSERT1((m_pAllocMem - m_Debug_StartMem) <= m_Debug_AllocedSize, "CFXVBMAllocUtil::Get_" #_AllocName "(): Out of bound!\n"); return pRet; } \


#define CFXVBMALLOCUTIL_FUNC(_Type, _AllocName, _Func) public: \
	M_INLINE void Alloc_Num##_AllocName(int _nV)	{ m_AllocSize += ((sizeof(_Type) + 15) & ~15) * _nV; FXVBMASSERT0(((m_AllocSize + 15) & ~15) * _nV == m_AllocSize, "Alloc_Num" #_AllocName "(): Is not aligned!\n"); } \
	M_INLINE void Alloc_##_AllocName()				{ m_AllocSize += (sizeof(_Type) + 15) & ~15; FXVBMASSERT0(((m_AllocSize + 15) & ~15) == m_AllocSize, "Alloc_" #_AllocName "(): Is not aligned!\n"); } \
	M_INLINE _Type* Get_##_AllocName()				{ _Type* pRet = (_Type*)m_pAllocMem; m_pAllocMem += (sizeof(_Type) + 15) & ~15; pRet->_Func; \
													  FXVBMASSERT1((m_pAllocMem - m_Debug_StartMem) <= m_Debug_AllocedSize, "CFXVBMAllocUtil::Get_" #_AllocName "(): Out of bound!\n"); return pRet; }

#define CFXVBMALLOCUTIL_SET(_Type, _AllocName) public: \
	M_INLINE void Alloc_##_AllocName() { m_AllocSize += (sizeof(_Type) + 15) & ~15; FXVBMASSERT0(((m_AllocSize + 15) & ~15) == m_AllocSize, "Alloc_" #_AllocName "(): Is not aligned!\n"); } \
	M_INLINE _Type* Get_##_AllocName() { _Type* pRet = (_Type*)m_pAllocMem; m_pAllocMem += (sizeof(_Type) + 15) & ~15;  \
										 FXVBMASSERT1((m_pAllocMem - m_Debug_StartMem) <= m_Debug_AllocedSize, "CFXVBMAllocUtil::Get_" #_AllocName "(): Out of bound!\n"); return pRet; } \
	M_INLINE _Type* Get_##_AllocName(const _Type& _##_AllocName) { _Type* pRet = (_Type*)m_pAllocMem; m_pAllocMem += (sizeof(_Type) + 15) & ~15; *pRet = _##_AllocName; \
																   FXVBMASSERT1((m_pAllocMem - m_Debug_StartMem) <= m_Debug_AllocedSize, "CFXVBMAllocUtil::Get_" #_AllocName "(" #_Type "): Out of bound!\n"); return pRet; }

#define CFXVBMALLOCUTIL_ARRAY_(_Type, _AllocName, _Alloc, _Get) public: \
	M_INLINE void _Alloc##_AllocName(int _nV, int _nSets = 1) { m_AllocSize += (((sizeof(_Type) * _nV) + 15) & ~15) * _nSets; FXVBMASSERT0((((m_AllocSize + 15) & ~15) * _nSets) == m_AllocSize, #_Alloc #_AllocName "(_nV): Is not aligned!\n"); } \
	M_INLINE _Type* _Get##_AllocName(int _nV) { _Type* pRet = (_Type*)m_pAllocMem; m_pAllocMem += ((sizeof(_Type) * _nV) + 15) & ~15; \
												FXVBMASSERT1((m_pAllocMem - m_Debug_StartMem) <= m_Debug_AllocedSize, "CFXVBMAllocUtil::" #_Get "" #_AllocName "(_nV): Out of bound!\n"); return pRet; }
#define CFXVBMALLOCUTIL_ARRAY(_Type, _AllocName) CFXVBMALLOCUTIL_ARRAY_(_Type, _AllocName, Alloc_, Get_)


class CFXVBMAllocUtil
{
private:
	int		m_AllocSize;
	int		m_StoredAlloc;
	uint8*	m_pAllocMem;
	CXR_VBManager* m_pVBM;

#if defined(FXVBMALLOCUTIL_DEBUG)
	int		m_Debug_AllocedSize;
	uint8*	m_Debug_StartMem;
#endif

	CFXVBMALLOCUTIL_FUNC(CXR_VertexBuffer,	VB,			Construct());
	CFXVBMALLOCUTIL_FUNC(CXR_VBChain,		VBChain,	Clear());
	CFXVBMALLOCUTIL_FUNC(CRC_Attributes,	Attrib,		SetDefault());
	CFXVBMALLOCUTIL_SET(CMat4Dfp32,			M4);
	CFXVBMALLOCUTIL_SET(CMat4Dfp32,			M43);
	CFXVBMALLOCUTIL_ARRAY(CVec4Dfp32,		V4);
	CFXVBMALLOCUTIL_ARRAY(CVec3Dfp32,		V3);
	CFXVBMALLOCUTIL_ARRAY(CVec2Dfp32,		V2);
	CFXVBMALLOCUTIL_ARRAY(CPixel32,			Pixel32);
	CFXVBMALLOCUTIL_ARRAY(fp32,				Fp32);
	CFXVBMALLOCUTIL_ARRAY(int32,			Int32);
	CFXVBMALLOCUTIL_ARRAY(int16,			Int16);
	
	CFXVBMALLOCUTIL_NRM(CXW_SurfaceKeyFrame, SurfaceKeyFrame);
	CFXVBMALLOCUTIL_FUNC(CRC_ExtAttributes_FragmentProgram20, FragmentProgram20, Clear());

public:
	M_INLINE void Store_Alloc(int _nVB, int _nAttrib, int _nVBChain, int _nV, int _nTV0, int _nCol)
	{
		m_StoredAlloc = (((sizeof(CXR_VertexBuffer) + 15) & ~15) * _nVB) +
						(((sizeof(CRC_Attributes) + 15) & ~15) * _nAttrib) +
						(((sizeof(CXR_VBChain) + 15) & ~15) * _nVBChain) +
						(((sizeof(CVec3Dfp32) * _nV) + 15) & ~15) +
						(((sizeof(CVec2Dfp32) * _nTV0) + 15) & ~15) +
						(((sizeof(CPixel32) * _nCol) + 15) & ~15);
	}

	M_INLINE void Alloc_Store(int _nStores = 1)
	{
		m_AllocSize += (m_StoredAlloc * _nStores);
		
		FXVBMASSERT1(((m_AllocSize + 15) & ~15) == m_AllocSize, "Alloc_Store(_nStores): Is not aligned!\n"); 
	}

	M_INLINE void Alloc_Any(int _Size)
	{
		m_AllocSize += (_Size + 15) & ~15;
		FXVBMASSERT1(((m_AllocSize + 15) & ~15) == m_AllocSize, "Alloc_Any(_Size): Is not aligned!\n"); 
	}

	M_INLINE void* Get_Any(int _Size)
	{
		void* pRet = (void*)m_pAllocMem;
		m_pAllocMem += (_Size + 15) & ~15;
		FXVBMASSERT1((m_pAllocMem - m_Debug_StartMem) <= m_Debug_AllocedSize, "CFXVBMAllocUtil::Get_VBChain(_Contents, _nV): Out of bound!\n");
		return pRet;
	}

	M_INLINE void Alloc_VB(int _Contents, int _nV = 0)
	{
		Alloc_VB();
		
		if (_Contents & CXR_VB_ATTRIB)
			Alloc_Attrib();

		if (_Contents & (CXR_VB_VERTICES | CXR_VB_COLORS | CXR_VB_SPECULAR | CXR_VB_TVERTICESALL))
			Alloc_VBChain(_Contents, _nV);
	}

	M_INLINE void Alloc_FragmentProgram20(int _nParams)
	{
		Alloc_FragmentProgram20();
		Alloc_V4(_nParams);
	}

	M_INLINE void Alloc_NumFragmentProgram20(int _nV, int _nParams)
	{
		Alloc_NumFragmentProgram20(_nV);
		Alloc_V4(_nParams, _nV);
	}

	M_INLINE CRC_ExtAttributes_FragmentProgram20* Get_FragmentProgram20(int _nParams)
	{
		CRC_ExtAttributes_FragmentProgram20* pFP = Get_FragmentProgram20();
		pFP->SetParameters(Get_V4(_nParams), _nParams);
		return pFP;
	}

	M_INLINE CRC_ExtAttributes_FragmentProgram20* Get_FragmentProgram20(int _nParams, const char* _pProgramName, uint32 _ProgramNameHash)
	{
		CRC_ExtAttributes_FragmentProgram20* pFP = Get_FragmentProgram20(_nParams);
		pFP->SetProgram(_pProgramName, _ProgramNameHash);
		return pFP;
	}

	M_INLINE void Alloc_CopyToTexture(int _nCopy)
	{
		Alloc_Any(_nCopy * (
			TFXAlign<16>(sizeof(CXR_VertexBuffer)) +
			TFXAlign<16>(sizeof(CXR_VertexBuffer_PreRender)) +
			TFXAlign<16>(sizeof(CXR_PreRenderData_RenderTarget_CopyToTexture1))));
	}

	M_INLINE void Alloc_VBCopyToTexture(int _nCopy = 1)
	{
		Alloc_Any(_nCopy * (
			TFXAlign<16>(sizeof(CXR_VertexBuffer_PreRender)) +
			TFXAlign<16>(sizeof(CXR_PreRenderData_RenderTarget_CopyToTexture1))));
	}

	M_INLINE void AddCopyToTexture(fp32 _Priority, CRct& _SrcRect, CPnt& _Dst, uint16 _TextureID, bint _bContinueTiling, uint16 _Slice = 0, uint32 _iMRT = 0)
	{
		// Vertex buffer
		CXR_VertexBuffer* pVB = Get_VB();
		pVB->m_Priority = _Priority;
		
		// Setup vb
		AddCopyToTexture(pVB, _SrcRect, _Dst, _TextureID, _bContinueTiling, _Slice, _iMRT);

		m_pVBM->AddVB(pVB);
	}

	M_INLINE void AddCopyToTexture(CXR_VertexBuffer* _pVB, CRct& _SrcRect, CPnt& _Dst, uint16 _TextureID, bint _bContinueTiling, uint16 _Slice = 0, uint32 _iMRT = 0)
	{
		// Context data
		CXR_PreRenderData_RenderTarget_CopyToTexture1* pData = (CXR_PreRenderData_RenderTarget_CopyToTexture1*)Get_Any(sizeof(CXR_PreRenderData_RenderTarget_CopyToTexture1));

		pData->m_SrcRect = _SrcRect;
		pData->m_Dst = _Dst;
		pData->m_bContinueTiling = _bContinueTiling;
		pData->m_TextureID = _TextureID;
		pData->m_Slice = _Slice;
		pData->m_iMRT = _iMRT;

		// Pre-render buffer
		CXR_VertexBuffer_PreRender* pPreRender = (CXR_VertexBuffer_PreRender*)Get_Any(sizeof(CXR_VertexBuffer_PreRender));
		PFN_VERTEXBUFFER_PRERENDER pfnPreRender = CXR_PreRenderData_RenderTarget_CopyToTexture1::RenderTarget_CopyToTexture;
		pPreRender->Create((void*)pData, pfnPreRender);

		_pVB->m_Flags |= CXR_VBFLAGS_PRERENDER;
		_pVB->m_pPreRender = pPreRender;
		_pVB->SetVBEColor(0xffffff00);
	}

	void Alloc_VBChain(int _Contents, int _nV)
	{
		Alloc_VBChain();

		if (_Contents & CXR_VB_VERTICES)
			Alloc_V3(_nV);

		if (_Contents & CXR_VB_COLORS)
			Alloc_Pixel32(_nV);

		if (_Contents & CXR_VB_SPECULAR)
			Alloc_Pixel32(_nV);

		if (_Contents & CXR_VB_TVERTICESALL)
		{
			if ((_Contents & CXR_VB_TVERTICESALL) == CXR_VB_TVERTICESALL)
			{
				Alloc_Any((((sizeof(CVec2Dfp32) * _nV) + 15) & ~15) * CRC_MAXTEXCOORDS);
			}
			else
			{
				int Flags = CXR_VB_TVERTICES0;
				for (int i = 0; i < CRC_MAXTEXCOORDS; i++)
				{
					if (_Contents & Flags)
						Alloc_V2(_nV);

					Flags = Flags << 1;
				}
			}
		}
	}


	CXR_VertexBuffer* Get_VB(int _Contents, int _nV = 0)
	{
		CXR_VertexBuffer* pVB = Get_VB();
		
		if (_Contents & CXR_VB_ATTRIB)
			pVB->m_pAttrib = Get_Attrib();

		if (_Contents & (CXR_VB_VERTICES | CXR_VB_COLORS | CXR_VB_SPECULAR | CXR_VB_TVERTICESALL))
		{
			pVB->m_pVBChain = Get_VBChain(_Contents, _nV);
			pVB->m_Flags |= CXR_VBFLAGS_VBCHAIN;
		}
		
		FXVBMASSERT0((m_pAllocMem - m_Debug_StartMem) <= m_Debug_AllocedSize, "CFXVBMAllocUtil::Get_VB(_Contents, _nV): Out of bound!\n");
		return pVB;
	}

	CXR_VBChain* Get_VBChain(int _Contents, int _nV)
	{
		CXR_VBChain* pVBChain = Get_VBChain();
		
		if (_Contents & CXR_VB_VERTICES)
		{
			pVBChain->m_pV = Get_V3(_nV);
			pVBChain->m_nV = _nV;
		}
		
		if (_Contents & CXR_VB_COLORS)
			pVBChain->m_pCol =  Get_Pixel32(_nV);

		if (_Contents & CXR_VB_SPECULAR)
			pVBChain->m_pSpec = Get_Pixel32(_nV);

		if (_Contents & CXR_VB_TVERTICESALL)
		{
			if ((_Contents & CXR_VB_TVERTICESALL) == CXR_VB_TVERTICESALL)
			{
				for (int i = 0; i < CRC_MAXTEXCOORDS; i++)
				{
					pVBChain->m_pTV[i] = (fp32*)Get_V2(_nV);
					pVBChain->m_nTVComp[i] = 2;
				}
			}
			else
			{
				int Flags = CXR_VB_TVERTICES0;
				for (int i = 0; i < CRC_MAXTEXCOORDS; i++)
				{
					if (_Contents & Flags)
					{
						pVBChain->m_pTV[i] = (fp32*)Get_V2(_nV);
						pVBChain->m_nTVComp[i] = 2;
					}

					Flags = Flags << 1;
				}
			}
		}

		FXVBMASSERT0((m_pAllocMem - m_Debug_StartMem) <= m_Debug_AllocedSize, "CFXVBMAllocUtil::Get_VBChain(_Contents, _nV): Out of bound!\n");

		return pVBChain;
	}

public:
	CFXVBMAllocUtil()
	{
		Clear();
	}

	void Clear()
	{
		m_StoredAlloc = 0;
		m_AllocSize = 0;
		m_pAllocMem = NULL;
		m_pVBM = NULL;
	}

	M_INLINE bool IsAlloced()
	{
		return (m_AllocSize == 0 || m_pAllocMem);
	}

	// Allocate memory, make sure only one alloc call to vb manager is being done
	M_INLINE bool Alloc(CXR_VBManager* _pVBM)
	{
		if (m_AllocSize)
			m_pAllocMem = (uint8*)_pVBM->Alloc(m_AllocSize);

		FXVBMASSERT0(((m_AllocSize + 15) & ~15) == m_AllocSize, "CFXVBMAllocUtil::Alloc(CXR_VBManager): Allocation is not aligned!\n");

		#if defined(FXVBMALLOCUTIL_DEBUG)
		{
			m_Debug_AllocedSize = m_AllocSize;
			m_Debug_StartMem = m_pAllocMem;
		}
		#endif

		m_pVBM = _pVBM;
		return (m_pAllocMem != NULL);
	}
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXSysUtil
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CFXSysUtil
{
private:
	static bool					ms_bInit;

	// Parsing helpers
#ifndef M_RTM
	template <class T> static M_INLINE uint32 _Debug_ValueValidation(TThinArray<T>& _lList, CStr _Value, uint32 _nKeys)
	{
		uint32 nFoundKeys = 0;
		while(_Value != "")
		{
			_Value.GetStrSep(";");
			nFoundKeys++;
		}

		if(_lList.Len() != 0)
			_nKeys = _lList.Len();

		if(nFoundKeys < _nKeys)
		{
			ConOutL(CStrF("CXR_Model_EffectSystem::EvalRegisterObject: Too few values in key. Found %d, expected %d key(s)\n", nFoundKeys, _nKeys));
			M_TRACEALWAYS("CXR_Model_EffectSystem::EvalRegisterObject: Too few values in key. Found %d, expected %d key(s)\n", nFoundKeys, _nKeys);
		}
		else if(nFoundKeys > _nKeys)
		{
			ConOutL(CStrF("CXR_Model_EffectSystem::EvalRegisterObject: Too many values in key. Found %d, expected %d key(s)\n", nFoundKeys, _nKeys));
			M_TRACEALWAYS("CXR_Model_EffectSystem::EvalRegisterObject: Too many values in key. Found %d, expected %d key(s)\n", nFoundKeys, _nKeys);
		}

		return MaxMT(_nKeys, nFoundKeys);
	}
#endif

	template <class T> static M_INLINE uint32 _GetNumKeys(TThinArray<T>& _lList, CStr& _Keys, uint _nKeys)
	{
		#ifndef M_RTM
			return _Debug_ValueValidation<T>(_lList, _Keys, _nKeys);
		#else
			return (_lList.Len() > 0 ? _lList.Len() : _nKeys);
		#endif
	}

	template <class T> static M_INLINE T* _GetAllocList(TThinArray<T>& _lList, uint _nKeys)
	{
		// Not preallocated for random data,
		if(_lList.Len() == 0)
			_lList.SetLen(_nKeys);

		return _lList.GetBasePtr();
	}

	template <class T> static M_INLINE void _ParseAllocListFpT(TThinArray<T>& _lList, CStr& _Keys, uint _nKeys, const char* _pSep = ";")
	{
		uint32 nKeys = CFXSysUtil::_GetNumKeys<T>(_lList, _Keys, _nKeys);
		T* pList = CFXSysUtil::_GetAllocList<T>(_lList, nKeys);
		TAP_RCD<T> lList = _lList;
		uint32 iKey = 0;

		while(_Keys != "")
		{
			if(lList.Len() <= iKey)
				break;

			lList[iKey++] = (T)_Keys.GetStrSep(_pSep).Val_fp64();
		}
	}

	template <class T> static M_INLINE void _ParseAllocListIntT(TThinArray<T>& _lList, CStr& _Keys, uint _nKeys, const char* _pSep = ";")
	{
		uint32 nKeys = CFXSysUtil::_GetNumKeys<T>(_lList, _Keys, _nKeys);
		T* pList = CFXSysUtil::_GetAllocList<T>(_lList, nKeys);
		TAP_RCD<T> lList = _lList;
		uint32 iKey = 0;

		while(_Keys != "")
		{
			if(lList.Len() <= iKey)
				break;

			lList[iKey++] = (T)_Keys.GetStrSep(_pSep).Val_int();
		}
	}

	template <class T> static M_INLINE void _ParseAllocListParseStrT(TThinArray<T>& _lList, CStr& _Keys, uint _nKeys, const char* _pSep = ";")
	{
		uint32 nKeys = CFXSysUtil::_GetNumKeys<T>(_lList, _Keys, _nKeys);
		T* pList = CFXSysUtil::_GetAllocList<T>(_lList, nKeys);
		TAP_RCD<T> lList = _lList;
		uint32 iKey = 0;

		while(_Keys != "")
		{
			if(lList.Len() <= iKey)
				break;

			lList[iKey++].ParseString(_Keys.GetStrSep(_pSep));
		}
	}

	template <class T> static M_INLINE void _ParseAllocListParseT(TThinArray<T>& _lList, CStr& _Keys, uint _nKeys, const char* _pSep = ";")
	{
		uint32 nKeys = CFXSysUtil::_GetNumKeys<T>(_lList, _Keys, _nKeys);
		T* pList = CFXSysUtil::_GetAllocList<T>(_lList, nKeys);
		TAP_RCD<T> lList = _lList;
		uint32 iKey = 0;

		while(_Keys != "")
		{
			if(lList.Len() <= iKey)
				break;

			lList[iKey++].Parse(_Keys.GetStrSep(_pSep));
		}
	}

	template <class T> static M_INLINE void _ParseAllocListColT(TThinArray<T>& _lList, CStr& _Keys, uint _nKeys, const char* _pSep = ";")
	{
		uint32 nKeys = CFXSysUtil::_GetNumKeys<T>(_lList, _Keys, _nKeys);
		T* pList = CFXSysUtil::_GetAllocList<T>(_lList, nKeys);
		TAP_RCD<T> lList = _lList;
		uint32 iKey = 0;
		CPixel32 Color;

		while(_Keys != "")
		{
			if(lList.Len() <= iKey)
				break;

			Color.Parse(_Keys.GetStrSep(_pSep));
			lList[iKey++] = Color;
		}
	}

	template <class T> static M_INLINE void _ParseAllocListSurfaceT(TThinArray<T>& _lList, CStr& _Keys, uint _nKeys, const char* _pSep = ";")
	{
		uint nKeys = CFXSysUtil::_GetNumKeys<T>(_lList, _Keys, _nKeys);
		T* pList = CFXSysUtil::_GetAllocList<T>(_lList, nKeys);
		TAP_RCD<T> lList = _lList;
		uint32 iKey = 0;

		MACRO_GetRegisterObject(CXR_SurfaceContext, pSContext, "SYSTEM.SURFACECONTEXT");
		if(pSContext)
		{
			while(_Keys != "")
			{
				// break out if we are trying to read to many
				if(lList.Len() <= iKey)
					break;

				lList[iKey++] = (T)pSContext->GetSurfaceID(_Keys.GetStrSep(_pSep).GetStr());
			}
		}
		else
		{
			ConOutL("§cf00CXR_Model_EffectSystem(CFXSysUtil::ParseAllocListSurface): No Surface context!");
		}
	}

public:
	// Initializer
	static M_INLINE void Init()
	{
		if(!ms_bInit)
		{
			MACRO_GetRegisterObject(CXR_SurfaceContext, pSContext, "SYSTEM.SURFACECONTEXT");
			ms_bInit = true;
		}
	}


	// Allocations
	template <class T> static M_INLINE T* GetAllocList(TThinArray<T>& _lList, CStr& _Keys, uint _nKeys, const char* _pSep = ";")
	{
		uint32 nKeys = CFXSysUtil::_GetNumKeys<T>(_lList, _Keys, _nKeys);
		return CFXSysUtil::_GetAllocList<T>(_lList, _nKeys);
	}


	// Misc helpers
	static M_INLINE CVec4Dfp32 CreateVec4Dfp32(CVec3Dfp32& _Vec3Dfp32)
	{
		return CVec4Dfp32(_Vec3Dfp32.k[0], _Vec3Dfp32.k[1], _Vec3Dfp32.k[2], 0.0f);
	}


	// Interpolation
	static M_INLINE CPixel32 LerpColor(const CPixel32& _Src, const CPixel32& _Dst, fp32 _Time)
	{
		return CPixel32::LinearRGBA(_Src, _Dst, TruncToInt(_Time*256));
	}

	static M_INLINE CPixel32 LerpColor(uint _Src, uint _Dst, fp32 _Time)
	{
		return CPixel32::LinearRGBA(CPixel32(_Src), CPixel32(_Dst), TruncToInt(_Time*256));
	}

	static M_INLINE CPixel32 LerpRandColor(const uint32* _pArray, uint32& _Rand, fp32 _Time)
	{
		CPixel32 Result = CFXSysUtil::LerpColor(LerpColor(_pArray[0], _pArray[1], MFloat_GetRand(_Rand)), LerpColor(_pArray[2], _pArray[3], MFloat_GetRand(_Rand+1)), _Time);
		_Rand	+= 2;
		return Result;
	}

	static M_INLINE CPixel32 LerpColorMix(bool _bRand, const uint32* _pColor, uint32& _Rand, uint32 _iCurKey, uint32 _iNextKey, fp32 _TimeRand, fp32 _TimeAnim)
	{
		return (_bRand) ? LerpRandColor(_pColor, _Rand, _TimeRand) : LerpColor(_pColor[_iCurKey], _pColor[_iNextKey], _TimeAnim);
	}

	template <class T> static M_INLINE int LerpInt(const T _Src, const T _Dst, fp32 _Time)
	{
		return (T)TruncToInt(_Src * (1.0f - _Time) + (_Dst * _Time));
	}

	template <class T> static M_INLINE T LerpRandInt(const T* _pArray, uint32& _Rand, fp32 _Time)
	{
		T Result = LerpInt(LerpInt(_pArray[0], _pArray[1], MFloat_GetRand(_Rand)), LerpInt(_pArray[2], _pArray[3], MFloat_GetRand(_Rand + 1)), _Time);
		_Rand += 2;
		return Result;
	}

	template <class T> static M_INLINE T LerpIntMix(bool _bRand, const T* _pArray, uint32& _Rand, uint32 _iCurKey, uint32 _iNextKey, fp32 _TimeRand, fp32 _TimeAnim)
	{
		return (_bRand) ? LerpRandInt(_pArray, _Rand, _TimeRand) : LerpInt(_pArray[_iCurKey], _pArray[_iNextKey], _TimeAnim);
	}

	template <class T> static M_INLINE T LerpMT(const T _Src, const T _Dst, fp32 _Time)
	{
		return (T)(_Src * (1.0f - _Time) + (_Dst * _Time));
	}

	template <class T> static M_INLINE T LerpRandMT(const T* _pArray, uint32& _Rand, fp32 _Time)
	{
		T Result = LerpMT(LerpMT(_pArray[0], _pArray[1], MFloat_GetRand(_Rand)), LerpMT(_pArray[2], _pArray[3], MFloat_GetRand(_Rand + 1)), _Time);
		_Rand += 2;
		return Result;
	}

	template <class T> static M_INLINE T LerpMixMT(bool _bRand, const T* _pArray, uint32& _Rand, uint32 _iCurKey, uint32 _iNextKey, fp32 _TimeRand, fp32 _TimeAnim)
	{
		return (_bRand) ? LerpRandMT(_pArray, _Rand, _TimeRand) : LerpMT(_pArray[_iCurKey], _pArray[_iNextKey], _TimeAnim);
	}

	static M_INLINE CVec3Dfp32 RandPointInBox(const CBox3Dfp32& _Box, int _Seed)
	{
		return CVec3Dfp32(LerpMT(_Box.m_Min.k[0], _Box.m_Max.k[0], MFloat_GetRand(_Seed)),
						  LerpMT(_Box.m_Min.k[1], _Box.m_Max.k[1], MFloat_GetRand(_Seed+1)),
						  LerpMT(_Box.m_Min.k[2], _Box.m_Max.k[2], MFloat_GetRand(_Seed+2)));
	}


	// Parsers
	static M_INLINE void ParseAllocListFp(TThinArray<fp32>& _lList, CStr& _Keys, uint _nKeys, const char* _pSep = ";")			{ CFXSysUtil::_ParseAllocListFpT<fp32>(_lList, _Keys, _nKeys, _pSep); }
	static M_INLINE void ParseAllocListFp(TThinArray<fp64>& _lList, CStr& _Keys, uint _nKeys, const char* _pSep = ";")			{ CFXSysUtil::_ParseAllocListFpT<fp64>(_lList, _Keys, _nKeys, _pSep); }
	static M_INLINE void ParseAllocListInt(TThinArray<uint32>& _lList, CStr& _Keys, uint _nKeys, const char* _pSep = ";")		{ CFXSysUtil::_ParseAllocListIntT<uint32>(_lList, _Keys, _nKeys, _pSep); }
	static M_INLINE void ParseAllocListInt(TThinArray<int32>& _lList, CStr& _Keys, uint _nKeys, const char* _pSep = ";")		{ CFXSysUtil::_ParseAllocListIntT<int32>(_lList, _Keys, _nKeys, _pSep); }
	static M_INLINE void ParseAllocListInt(TThinArray<uint16>& _lList, CStr& _Keys, uint _nKeys, const char* _pSep = ";")		{ CFXSysUtil::_ParseAllocListIntT<uint16>(_lList, _Keys, _nKeys, _pSep); }
	static M_INLINE void ParseAllocListInt(TThinArray<int16>& _lList, CStr& _Keys, uint _nKeys, const char* _pSep = ";")		{ CFXSysUtil::_ParseAllocListIntT<int16>(_lList, _Keys, _nKeys, _pSep); }
	static M_INLINE void ParseAllocListInt(TThinArray<uint8>& _lList, CStr& _Keys, uint _nKeys, const char* _pSep = ";")		{ CFXSysUtil::_ParseAllocListIntT<uint8>(_lList, _Keys, _nKeys, _pSep); }
	static M_INLINE void ParseAllocListInt(TThinArray<int8>& _lList, CStr& _Keys, uint _nKeys, const char* _pSep = ";")			{ CFXSysUtil::_ParseAllocListIntT<int8>(_lList, _Keys, _nKeys, _pSep); }
	static M_INLINE void ParseAllocListVec(TThinArray<CVec2Dfp32>& _lList, CStr& _Keys, uint _nKeys, const char* _pSep = ";")	{ CFXSysUtil::_ParseAllocListParseStrT<CVec2Dfp32>(_lList, _Keys, _nKeys, _pSep); }
	static M_INLINE void ParseAllocListVec(TThinArray<CVec3Dfp32>& _lList, CStr& _Keys, uint _nKeys, const char* _pSep = ";")	{ CFXSysUtil::_ParseAllocListParseStrT<CVec3Dfp32>(_lList, _Keys, _nKeys, _pSep); }
	static M_INLINE void ParseAllocListVec(TThinArray<CVec4Dfp32>& _lList, CStr& _Keys, uint _nKeys, const char* _pSep = ";")	{ CFXSysUtil::_ParseAllocListParseStrT<CVec4Dfp32>(_lList, _Keys, _nKeys, _pSep); }
	static M_INLINE void ParseAllocListCol(TThinArray<uint32>& _lList, CStr& _Keys, uint _nKeys, const char* _pSep = ";")		{ CFXSysUtil::_ParseAllocListColT<uint32>(_lList, _Keys, _nKeys, _pSep); }
	static M_INLINE void ParseAllocListSurface(TThinArray<int>& _lList, CStr& _Keys, uint _nKeys, const char* _pSep = ";")		{ CFXSysUtil::_ParseAllocListSurfaceT<int>(_lList, _Keys, _nKeys, _pSep); }

	static M_INLINE uint32 RandSeed(int32 _nRand, uint32 _Seed)
	{
		if (_nRand <= 1)
			return 0;

		CRand_MersenneTwister* pRand = MRTC_GetRand();
		pRand->InitRand(_Seed);
		return (pRand->GenRand32() % _nRand);
	}


	static M_INLINE uint32 RandExclude(int32 _nRand, int32 _Exclude, uint32 _Rand)
	{
		if (_nRand <= 1)
			return 0;
		int32 Rand = _Rand % (_nRand);
		if (Rand == _Exclude)
			return (Rand) ? Rand - 1 : MinMT(Rand + 1, _nRand - 1);
		else
			return Rand;
	}

	static M_INLINE uint32 RandExcludeSeed(int32 _nRand, int32 _Exclude, uint32 _Seed)
	{
		CRand_MersenneTwister* pRand = MRTC_GetRand();
		pRand->InitRand(_Seed);
		return RandExclude(_nRand, _Exclude, pRand->GenRand32());
	}

	static void CalcBoxScissor(const CXR_Engine* _pEngine, CRC_Viewport* _pVP, const CMat4Dfp32* _pW2V, const CBox3Dfp32& _Box, CScissorRect& _Scissor);
	static void TransformAABB(const CMat4Dfp32& _Mat, const CBox3Dfp32& _Box, CBox3Dfp32& _DestBox);
};


class CXR_Model_EffectSystem;
class CEffectSystemHistory;
class CEffectSystemRenderData;


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CSpline_Beam
|	Modified CSpline_Vec3Dfp32 that will build a vertex buffer to render, lightweight and doesn't
|	take as many parameters and customizations as CSpline_BeamStrip
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CSpline_Beam
{
private:
	enum
	{
		MaxCachePoints	= 8,		// Max cache points to use
	};

	// Spline point
	struct Point
	{
		CVec4Dfp32 m_Pos;			// Start value
		CVec4Dfp32 m_TangentOut;	// Out tangent
		CVec4Dfp32 m_TangentIn;
	};

	struct Segment
	{
		TThinArray<fp32>	m_lProperties;
		TThinArray<uint8>	m_liPoints;
		uint16				m_nPoints;
		uint16				m_SurfaceID;
		CSurfaceKey			m_SK;
		CXR_VertexBuffer*	m_pVB;
		CXW_SurfaceKeyFrame* m_pSurfaceKeyFrame;
	};

	TThinArray<Point>	m_lPoints;
	TThinArray<Segment>	m_lSegments;
	uint32				m_nCachePoints : 16;		// Number of segments to smooth spline over
	uint32				m_nPoints : 16;				// Number of points
	uint8				m_bCalcedVBMem : 1;			// Has vb memory been pre calculated ?
	uint8				m_bFade : 1;
	fp32				m_UOffset;

	vec128				CalcPosVec128(const int _iSeg, const int _iLocal, vec128 _Time);
	bool				Alloc_VBMem(CFXVBMAllocUtil* _pVBMHelp, CXR_VBManager* _pVBM, const CMat4Dfp32& _W2V, const int _nCachePoints);

public:
	CSpline_Beam(const uint16* _pSurfaceID, const int _nSurfaceID, const int _nPoints);

	// Spline creation
	void SetSpline(const fp32 _Width, const fp32 _Length, const fp32 _UOffset, const CVec3Dfp32& _Pos1, const CVec3Dfp32& _Pos2, const fp32 _LengthRcp, const CVec3Dfp32& _Tang1, const CVec3Dfp32& _Tang2, const int _iSeg);
	bool Finalize(CFXVBMAllocUtil* _pVBMHelp, CXR_VBManager* _pVBM, const CMat4Dfp32& _W2V, bool _bUVRot = false, const int _nCachePoints = MaxCachePoints);
	void Render(CXR_SurfaceContext* _pSC, CXR_Engine* _pEngine, CXR_VBManager* _pVBM, CXR_RenderInfo* _pRenderInfo, const CMTime& _AnimTime, uint _Flags, CXR_RenderSurfExtParam* _pParam);

	void	CalcVBMem(CFXVBMAllocUtil* _pVBMHelp, const int _nCachePoints = MaxCachePoints);
	bool	IsEmpty() const;

	M_INLINE void SetFade(bool _bFade)				{ m_bFade = (_bFade) ? 1 : 0; }
	M_INLINE bool GetFade()							{ return (m_bFade) ? true : false; }
	M_INLINE void SetUOffset(const fp32 _UOffset)	{ m_UOffset = _UOffset; }

	// Debug
#ifndef M_RTM
	void Debug_Render();
	void Debug_RenderVertexSize(CWireContainer* _pWC, const CVec3Dfp32& _Pos, const CPixel32& _Color = 0xffffffff, const fp32 _Size = 2.0f, const fp32 _Duration = 0.0f, const bool _bFade = false);
#endif
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CSpline_BeamStrip
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CSpline_BeamStripData
{
public:
	CVec3Dfp32 m_Pos;
	CVec3Dfp32 m_Tng;
	uint32 m_Color;
	fp32 m_Width;
	fp32 m_UOffset;

	CSpline_BeamStripData() {}
	CSpline_BeamStripData(CVec3Dfp32 _Pos, CVec3Dfp32 _Tng, uint32 _Color, fp32 _Width, fp32 _UOffset)
		: m_Pos(_Pos)
		, m_Tng(_Tng)
		, m_Color(_Color)
		, m_Width(_Width)
		, m_UOffset(_UOffset)
	{
	}
};


class CSpline_BeamStrip
{
	enum
	{
		MaxCachePoints	= 8,
	};

	class CStrip
	{
	public:
		TStaticArray<CSpline_BeamStripData, 8> m_lBeamData;
		CXR_VertexBuffer* m_pVB;
		CXW_SurfaceKeyFrame* m_pSurfaceKeyFrame;
		uint16 m_SurfaceID;
		uint8 m_TexCoordRotate;

		CStrip()
			: m_pVB(NULL)
			, m_pSurfaceKeyFrame(NULL)
			, m_SurfaceID(0)
			, m_TexCoordRotate(0)
		{
		}
	};

	TStaticArray<CStrip, 8> m_lStrips;	// Strip information data
	uint8 m_bVBMem : 1;					// Indicate if vb memory has been calculated or not

	vec128 CalcPosVec128(const CVec4Dfp32& _P0, const CVec4Dfp32& _P1, const CVec4Dfp32& _T0, const CVec4Dfp32& _T1, vec128 _Time);
	bool VBMem_Collect(CFXVBMAllocUtil* _pVBMHelp, const CMat4Dfp32& _W2V, const uint _nCachePoints = MaxCachePoints);
public:
	CSpline_BeamStrip(uint _nStrips);

	void VBMem_Calculate(CFXVBMAllocUtil* _pVBMHelp, const uint _nCachePoints = MaxCachePoints);

	void AddBeamData(CSpline_BeamStripData& _BeamData, uint _iStrip);
	void AddBeamData(const CVec3Dfp32& _Pos, const CVec3Dfp32& _Tng, CPixel32 _Color, fp32 _Width, fp32 _UOffset, uint _iStrip);
	bool Finalize(CFXVBMAllocUtil* _pVBMHelp, const CMat4Dfp32& _W2V, const uint _nCachePoints = MaxCachePoints);

	void SetSurface(uint _iStrip, uint16 _SurfaceID);
	void RotateTexCoord(uint _iStrip, bool _bRotate);

	void Render(CXR_SurfaceContext* _pSC, CXR_Engine* _pEngine, CXR_VBManager* _pVBM, CXR_RenderInfo* _pRenderInfo, const CMTime& _AnimTime, uint _Flags, CXR_RenderSurfExtParam* _pParam);

#ifndef M_RTM
	void Debug_Render(CWireContainer* _pWC);
#endif
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CEffectSystemRenderChain
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CEffectSystemRenderChain
{
public:
	// Setups a queue for rendering and validates it on success
	CEffectSystemRenderChain()
	{
		m_Valid = false;
		m_VB.Clear();
		m_SurfaceKey.m_ColorAlpha = 0;
		m_SurfaceKey.m_ColorRGB = 0;
		m_SurfaceKey.m_pKeyFrame = 0;
		m_SurfaceKey.m_pSurface = 0;
	}

	CEffectSystemRenderChain(int _Seq, const CXR_RenderInfo& _RenderInfo, const CEffectSystemRenderData* _pRD, const CFXDataShader* _pShader, CXR_SurfaceContext* _pSurfaceContext, CXR_Engine* _pEngine, const CMTime& _AnimTime, CXR_VBChain* _pChain, CXR_VBManager* _pVBM, uint _SurfaceID, const CMat4Dfp32& _VMat, const fp32* _pPriority);

	// Render if valid
	void Render(CXR_VBManager* _pVBM, CXR_Engine* _pEngine)
	{
		MSCOPESHORT(CEffectSystemRenderChain::Render);

		if(m_Valid)
		{
			{
//				m_SurfaceKey.m_pKeyFrame->m_AnimTime = m_AnimTime;
//				m_SurfaceKey.m_pKeyFrame->m_AnimTimeWrapped = m_AnimTimeWrapped;
				m_SurfaceKey.Render(&m_VB, _pVBM, _pEngine);
			}
		}
	}

private:
//	CMTime				m_AnimTime;
//	CMTime				m_AnimTimeWrapped;
	CXR_VertexBuffer	m_VB;
	CSurfaceKey			m_SurfaceKey;
	uint8				m_Priority;
	fp32					m_PriorityOffset;
	bool				m_Valid;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CEffectSystemRenderData
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CEffectSystemRenderData
{
public:
	// Public access data
	CVec3Dfp32				m_CameraFwd, m_CameraLeft, m_CameraUp, m_CameraPos;
	CMTime					m_AnimTime0, m_AnimTime1;
	uint32					m_Anim0, m_Anim1;
	uint32					m_AnimAttr0;
	fp32						m_Time0, m_Time1;
	uint32					m_Seeding;
	uint32					m_Flags;
	CEffectSystemHistory*	m_pHistory;
	CXR_SurfaceContext*		m_pSurfaceContext;
	CMat4Dfp32				m_WMat;
	CMat4Dfp32				m_VMat;
	CXR_VBChain*			m_pChain;	// Current vertex buffer chain
	CXR_Engine*				m_pEngine;	// Current engine
	CXR_Model_EffectSystem*	m_pFXSystem;
	
	CVec4Dfp32				m_ShaderParam[5];
	//CVec4Dfp32				m_ShaderParam[FXSHADER_FRAGMENT_PARAMS];	// Predefined parameters
	//TThinArray<CVec4Dfp32>	m_lShaderParam;
	
	//CVec4Dfp32				m_TexGenParam[0];

	CXR_VBManager*			m_pVBM;		// Vertex buffer manager
	CXR_SkeletonInstance*	m_pSkeletonInst;
	CXR_Skeleton*			m_pSkeleton;
	uint32					m_iSkeletonType;
	
	CXR_AnimState*			m_pAnimState;
private:
	TThinArray<CEffectSystemRenderChain>
							m_lChain;	// Vertex buffers rendering chain

	// Is this really needed??
	uint32					m_nV;		// Current number of vertices
	uint32					m_nQueues;

	bool ProcessAdvancedSurface(int _iChannel, CXR_VertexBuffer* _pVB, const CXW_SurfaceLayer& _Layer, const CXW_SurfaceKeyFrame* _pSurfaceKey);

public:
	// Helper functions
	void InitVBChain(CXR_VBManager* _pVBM)	{ m_pVBM = _pVBM; m_pChain = 0; m_nV = 0; }
	bool CreateNewVBChain(uint _nV, uint _Contents = CXR_VB_VERTICES | CXR_VB_COLORS);
	
	// Setup helpers
	void SetupBeamStripIndices(uint _nTris = 0)			{ m_pChain->Render_IndexedTriangles(CXR_Util::m_lBeamStripTriangles, (_nTris > 0) ? _nTris : (m_nV-2)); }
	void SetupBeamIndices(uint _nTris = 0)				{ m_pChain->Render_IndexedTriangles(CXR_Util::m_lBeamTriangles, (_nTris > 0) ? _nTris : m_nV/2); }
	void SetupQuadIndices(uint _nTris = 0)				{ m_pChain->Render_IndexedTriangles(CXR_Util::m_lQuadParticleTriangles, (_nTris > 0) ? _nTris : m_nV/2); }
	void SetupTriIndices(uint _nTris = 0)				{ m_pChain->Render_IndexedTriangles(CXR_Util::m_lTriParticleTriangles, (_nTris > 0) ? _nTris : m_nV/3); }
	void SetupIndices(uint _nTris, uint16* _pTris)		{ m_pChain->Render_IndexedTriangles(_pTris, _nTris); }

	// Allocation helpers
	uint16* AllocateIndices(uint _nT)					{ return m_pVBM->Alloc_Int16(_nT*3); }
	CVec2Dfp32* AllocateTVertices(uint _nV = 0)			{ return m_pVBM->Alloc_V2((_nV > 0) ? _nV : m_nV); }
	CVec4Dfp32* AllocShaderParams(const CXR_VertexBuffer* _pVB, uint _nParams, uint32 _ShaderFlags, uint _nTexGenParams, uint _nPredefinedParams, uint _iTexGen);

	bool CreateFragmentProgram(CXR_VertexBuffer* _pVB, const CFXDataShader* _pShader, CSurfaceKey& _SurfaceKey);

	// Render queue
	//void Render_Queue(uint _SurfaceID, const CMat4Dfp32& _VMat, const CMTime* _pTime = NULL);
	void Render_Queue(uint _SurfaceID, const CMat4Dfp32& _VMat, const CFXDataShader* _pShader, const CMTime* _pTime = NULL, const fp32* _pPriority = NULL);
	void Render();

	void InitRenderChain(uint _nElements)				{ m_lChain.SetLen(_nElements); m_nQueues = 0; }
	void ClearRenderChain()								{ m_lChain.Clear(); m_nQueues = 0; }
};


// Extended particle layers
#ifdef TESTNEWPARTICLES
	typedef CXR_Model_Particles	CFXLayerModelParticles;
#else
	typedef CXR_Model_ParticlesOpt CFXLayerModelParticles;
#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXLayer / CFXLayerData
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CFXLayer
{
public:
	typedef CFXLayerModelParticles::PerRenderVariables FXPerRenderVariables;
	typedef CFXLayerModelParticles::CBatch FXBatch;

	class CFXLayerData : public CReferenceCount
	{
	public:
		CFXLayerData() {}
		~CFXLayerData() {}

		virtual void OnEvalKey(CStr& _Key) {}
		virtual void GenerateRender(CFXLayer::FXPerRenderVariables* M_RESTRICT _pPR, CFXLayer::FXBatch* M_RESTRICT _pBatch, CXR_Model_Custom_RenderParams* _pRenderParams, int _GenerateFlags) {}
		virtual void GenerateDistribution(uint _MaxNumParticles, CFXLayer::FXPerRenderVariables* M_RESTRICT _pPR, CFXLayer::FXBatch* M_RESTRICT _pBatch, int& _GenerateFlags) {}
		
		virtual void EvaluateBeamStrip(CXR_BeamStrip* _pBeams, uint _nBeams, fp32 _AnimTime, const CMat4Dfp32& _WMat) {}
	};

	TThinArray<TPtr<CFXLayerData> >	m_lpExtLayers;	// Our layers

	CFXLayer();
	~CFXLayer();

	CFXLayer& operator = (const CFXLayer& _Layer);

	template <class T> void OnEvalKey(CStr& _Key)
	{
		T* pLayer = MNew(T);
		if(pLayer)
		{
			pLayer->OnEvalKey(_Key);

			uint nLayers = m_lpExtLayers.Len();
			m_lpExtLayers.SetLen(nLayers + 1);
			m_lpExtLayers[nLayers] = pLayer;
		}
	}

	void EvaluateBeamStrip(CXR_BeamStrip* _pBeams, uint _nBeams, fp32 _AnimTime, const CMat4Dfp32& _WMat)
	{
		uint nLayers = m_lpExtLayers.Len();
		TPtr<CFXLayerData>* spLayerData = m_lpExtLayers.GetBasePtr();
		for (uint i = 0; i < nLayers; i++)
			spLayerData[i]->EvaluateBeamStrip(_pBeams, _nBeams, _AnimTime, _WMat);
	}

	virtual void GenerateRender(FXPerRenderVariables* M_RESTRICT _pPR, CFXLayerModelParticles::CBatch* M_RESTRICT _pBatch, CXR_Model_Custom_RenderParams* _pRenderParams, int _GenerateFlags);
	virtual void GenerateDistribution(uint _MaxNumParticles, FXPerRenderVariables* M_RESTRICT _pPR, CFXLayerModelParticles::CBatch* M_RESTRICT _pBatch, int& _GenerateFlags);
};

typedef CFXLayer::CFXLayerData	CFXLayerData;


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXLayerTwirl
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CFXLayerTwirl : public CFXLayerData
{
public:
	fp32	m_Offset;
	fp32	m_Speed;
	fp32	m_Radius;
	fp32	m_Loops;
	uint8	m_iComponents;

	CFXLayerTwirl() : m_Offset(0), m_Speed(1), m_Radius(2), m_Loops(2), m_iComponents(1<<2) {}

	virtual void OnEvalKey(CStr& _Key);
	virtual void GenerateRender(CFXLayer::FXPerRenderVariables* M_RESTRICT _pPR, CFXLayer::FXBatch* M_RESTRICT _pBatch, CXR_Model_Custom_RenderParams* _pRenderParams, int _GenerateFlags);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXLayerBoneBind
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CFXLayerBoneBind : public CFXLayerData
{
public:
	CFXLayerBoneBind() {}

	virtual void GenerateDistribution(uint _MaxNumParticles, CFXLayer::FXPerRenderVariables* M_RESTRICT _pPR, CFXLayer::FXBatch* M_RESTRICT _pBatch, int& _GenerateFlags);
	void GetDistributedPoint_InSkeleton(uint _SkeletonType, CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInst, CVec3Dfp32& _point, uint32 _iRandSeed);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXLayerBoxSpawn
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CFXLayerBoxSpawn : public CFXLayerData
{
public:
	CFXLayerBoxSpawn() {}

	virtual void GenerateDistribution(uint _MaxNumParticles, CFXLayer::FXPerRenderVariables* M_RESTRICT _pPR, CFXLayer::FXBatch* M_RESTRICT _pBatch, int& _GenerateFlags);
	void GetDistributedPoint_InBounding(CBox3Dfp32* _pBoxes, uint32 _nBoxes, CVec3Dfp32& _Point, uint32 _iRandSeed);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXLayerNoise
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CFXLayerNoise : public CFXLayerData
{
public:
	bool		m_bAnimated;
	fp32		m_Timescale;
	CVec3Dfp32	m_Noise;

	CFXLayerNoise() : m_bAnimated(0), m_Timescale(1), m_Noise(0) { }
	virtual void OnEvalKey(CStr& _Key);

	virtual void EvaluateBeamStrip(CXR_BeamStrip* _pBeams, uint _nBeams, fp32 _AnimTime, const CMat4Dfp32& _WMat);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXLayerPath
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CFXLayerPath : public CFXLayerData
{
public:
	TThinArray<CVec3Dfp32>	m_lPath;

	CFXLayerPath() {}
	virtual void OnEvalKey(CStr& _Key);
	
	virtual void EvaluateBeamStrip(CXR_BeamStrip* _pBeams, uint _nBeams, fp32 _AnimTime, const CMat4Dfp32& _WMat);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataCollect
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CFXDataCollect
{
	public:
		CFXDataCollect();
		~CFXDataCollect()
		{
			m_lDurationReciprocal.Clear();
			m_lSurfaceID.Clear();
			m_lDuration.Clear();
			m_lLength.Clear();
			m_lWidth.Clear();
			m_lWidth2.Clear();
			m_lHeight.Clear();
			m_lRotationAngle.Clear();
			m_lColor.Clear();
			m_lSegments.Clear();
			m_lRings.Clear();
			m_lStripHeight.Clear();
			m_lWidthNoise.Clear();
			m_lInnerRadius.Clear();
			m_lOuterRadius.Clear();
			m_lSphereRadius.Clear();
			m_lTolerance.Clear();
			m_lIntensity.Clear();
			m_lPosNoise.Clear();
			m_lPosOffset.Clear();
			m_lVelocity.Clear();
			m_lVelocityNoise.Clear();
			m_lTextureScroll.Clear();
			m_lDirection.Clear();
			m_lRange.Clear();
			m_lShaderParams.Clear();
			m_lShaderMaps.Clear();
		}
	
		void ParticleKey(const char* _pName, const char* _pValue)
		{
			if(m_ParticleKeys != "")
				m_ParticleKeys += ",";

			m_ParticleKeys += _pName;
			m_ParticleKeys += "=";
			m_ParticleKeys += _pValue;
		}

		void ParticleKeyString(const char* _pValue)
		{
			m_ParticleKeys = _pValue;
		}

		void PreAllocateKey(CStr& _KeyName, CStr& _Value)
		{
			uint8 nKeys = 0;
			while(_Value != "")
			{
				nKeys++;
				_Value.GetStrSep(";");
			}
			if(nKeys == 0)
				return;

			if(_KeyName == "SURFACE")
				m_lSurfaceID.SetLen(nKeys);
			else if(_KeyName == "WIDTH")
				m_lWidth.SetLen(nKeys);
			//else if(_KeyName == "WIDTH2")
			//	m_lWidth2.SetLen(nKeys);
			else if(_KeyName == "ROTATEANGLE")
				m_lRotationAngle.SetLen(nKeys);
			else if(_KeyName == "COLOR")
				m_lColor.SetLen(nKeys);
			//else if(_KeyName == "RADIUS")
			//	m_lRadius.SetLen(nKeys);
			else if(_KeyName == "SEGMENTS")
				m_lSegments.SetLen(nKeys);
			else if(_KeyName == "STRIPHEIGHT")
				m_lStripHeight.SetLen(nKeys);
			else if(_KeyName == "WIDTHNOISE")
				m_lWidthNoise.SetLen(nKeys);
			else if(_KeyName == "POSNOISE")
				m_lPosNoise.SetLen(nKeys);
			else if(_KeyName == "VELOCITY")
				m_lVelocity.SetLen(nKeys);
			else if(_KeyName == "TEXTURESCROLL")
				m_lTextureScroll.SetLen(nKeys);
			else if(_KeyName == "VELOCITYNOISE")
				m_lVelocityNoise.SetLen(nKeys);
			else if(_KeyName == "LENGTH")
				m_lLength.SetLen(nKeys);
		}

		CStr					m_Flags;
		fp32						m_TotalDuration;
		TThinArray<fp32>			m_lDurationReciprocal;
		TThinArray<int>			m_lSurfaceID;
		TThinArray<fp32>			m_lDuration;
		TThinArray<fp32>			m_lLength;
		TThinArray<fp32>			m_lWidth;
		TThinArray<fp32>			m_lWidth2;
		TThinArray<fp32>			m_lHeight;
		TThinArray<fp32>			m_lRotationAngle;
		TThinArray<uint32>		m_lColor;
		TThinArray<uint8>		m_lSegments;
		TThinArray<uint8>		m_lRings;
		TThinArray<fp32>			m_lStripHeight;
		TThinArray<fp32>			m_lWidthNoise;
		TThinArray<fp32>			m_lInnerRadius;
		TThinArray<fp32>			m_lOuterRadius;
		TThinArray<fp32>			m_lSphereRadius;
		TThinArray<fp32>			m_lTolerance;
		TThinArray<CVec3Dfp32>	m_lIntensity;
		TThinArray<CVec3Dfp32>	m_lPosNoise;
		TThinArray<CVec3Dfp32>	m_lPosOffset;
		TThinArray<CVec3Dfp32>	m_lVelocity;
		TThinArray<CVec3Dfp32>	m_lVelocityNoise;
		TThinArray<CVec2Dfp32>	m_lTextureScroll;
		TThinArray<CVec3Dfp32>	m_lDirection;
		TThinArray<fp32>			m_lRange;
		CStr					m_ParticleKeys;
		CStr					m_RandomFlags;
		uint32					m_Seed;
		uint8					m_IntFlags;
		uint8					m_nMulti;
		fp32						m_HistoryTime;
		fp32						m_HistorySpawn;

		// Shader controls
		CFXDataShader*			m_pShader;
		//fp32						m_Priority;
		uint8					m_Priority;
		fp32						m_PriorityOffset;
		TThinArray<CVec4Dfp32>	m_lShaderParams;
		TThinArray<int>			m_lShaderMaps;
		CStr					m_ProgramName;
		uint32					m_Enable;
		uint32					m_Disable;
		uint8					m_iTexGen;
		uint8					m_RasterMode;
		uint8					m_ZCompare;
		uint8					m_AlphaCompare;
		uint16					m_AlphaRef;
		uint8					m_iShader;
		uint8					m_iRenderTarget;
		CStr					m_RenderTarget;
		CStr					m_RenderType;

		// Extended layer postprocessing data
		CFXLayer				m_Layer;
		//TThinArray<CFXLayer*>	m_lpExtLayers;
		/*
		TThinArray<CFXLayerTwirl>		m_lTwirlLayer;
		TThinArray<CFXLayerBoneBind>	m_lBoneBindLayer;
		TThinArray<CFXLayerNoise>		m_lNoiseLayer;
		TThinArray<CFXLayerPath>		m_lPathLayer;
		*/

		// Array control
		uint8					m_nKeys;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataObject
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CFXDataObject
{
	public:
		CFXDataObject();
		CFXDataObject(const CFXDataCollect& _Data);

		//virtual void Render(CXR_Model_EffectSystem* _pFXSystem, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, uint32 _Flags);
		virtual void Render(CEffectSystemRenderData* _pRD, const CMat4Dfp32& _WMat);
		virtual void RenderVis(CWorld_Client* _pWClient, CXR_SceneGraphInstance* _pSGI, uint16& _LightGUID, int _iOwner, fp32 _AnimTime, const CMat4Dfp32& _WMat, uint32 _Flags) {}
		virtual void RenderClient(const CXR_SurfaceContext* _pSurfaceContext, CWorld_Client* _pWClient, const CMTime& _AnimTime, const CMat4Dfp32& _WMat, uint32 _Flags) {}
		virtual void OnCollisionRefresh(CWorld_Client& _WClient, class CEffectSystemHistory& _History, int _iExclude = 0) {}

		virtual void CreateConeScreenQuad_Render(CEffectSystemRenderData* _pRD, const CMat4Dfp32& _WMat) {}

		//fp32 LinearInterpolateFloat(fp32 _Begin, fp32 _End, fp32 _Time, fp32 _Reciprocal)
		fp32 LinearInterpolateFloat(fp32 _Begin, fp32 _End, fp32 _Time)
		{
			//return (_Begin + ((_End - _Begin) * (_Time * _Reciprocal)));
			return (_Begin + ((_End - _Begin) * _Time));
		}

		//int LinearInterpolateInt(int _Begin, int _End, fp32 _Time, fp32 _Reciprocal)
		int LinearInterpolateInt(int _Begin, int _End, fp32 _Time)
		{
			//return static_cast<int>(LinearInterpolateFloat(static_cast<fp32>(_Begin), static_cast<fp32>(_End), _Time, _Reciprocal));
			return static_cast<int>(LinearInterpolateFloat(static_cast<fp32>(_Begin), static_cast<fp32>(_End), _Time));
		}

		template<class T> M_INLINE T LinearInterpolateMT(const T& _Begin, const T& _End, fp32 _Time)
		{
			return(_Begin + static_cast<T>(((_End - _Begin) * _Time)));
		}

		// Sets this and next key and return interpolation time between them
		#define GETKEY(_Return, _Time, _iKey, _iNextKey, _Flags)	uint _iKey, _iNextKey; const fp32 _Return = GetKey(_Time, _iKey, _iNextKey, _Flags)
		fp32 GetKey(fp32 _Time, uint& _iKey, uint& _iNextKey, uint32 _Flags = 0)
		{
			if(!(m_IntFlags & FXFLAGS_INT_ANIMATED))
			{
				_iKey = _iNextKey = 0;
				return 0;
			}

			int nDuration = m_lDuration.Len();
			if (_Flags & FXFLAGS_TIMEMODE_CONTINUOUS)
			{
				const fp32* pDuration = m_lDuration.GetBasePtr();
//				const fp32 StartTime = m_TotalDuration * TruncToInt(_Time / m_TotalDuration);
				fp32 Duration = 0.0f;
				for(int i = 0; i < nDuration; i++)
				{
					Duration += pDuration[i];
					if(_Time <= Duration)
					{
						_iKey = (uint8)i++;
						_iNextKey = (i >= nDuration) ? 0 : (uint8)i;
						return (_Time - (Duration - pDuration[i])) * m_lDurationReciprocal.GetBasePtr()[i];
					}
				}

				M_ASSERT(true, "This should actually never happen!!, why did it happened?");

                _iNextKey = _iKey = (uint8)(nDuration - 1);
				return 0;
			}
			
			if(_Time > m_TotalDuration)
			{
				_iKey = _iNextKey = (uint8)(nDuration - 1);
				return 0;
			}

			const fp32* pDuration = m_lDuration.GetBasePtr();
			fp32 Duration = 0.0f;
			for(int i = 0; i < nDuration; i++)
			{
				Duration += pDuration[i];
				if(_Time <= Duration)
				{
					_iKey = (uint8)i;
					_iNextKey = (uint8)MinMT(i+1, nDuration-1);
					return (_Time - (Duration - pDuration[i])) * m_lDurationReciprocal.GetBasePtr()[i];
				}
			}

			// Time larger. Return last key
			_iNextKey = _iKey = (uint8)(nDuration - 1);
			return 0;
		}

		uint8 GetKeyFromTime(fp32 _Time, fp32* pKeyTime = NULL, uint32 _Flags = 0)
		{
			if(_Flags & FXFLAGS_TIMEMODE_CONTINUOUS)
			{
//				if(pKeyTime)
//				{
					const fp32 StartTime = (static_cast<fp32>(static_cast<uint32>(_Time / m_TotalDuration)) * m_TotalDuration);
					const uint Length = m_lDuration.Len();
					const fp32* pDuration = m_lDuration.GetBasePtr();
					const fp32 Time = _Time - StartTime;
					fp32 DurationSum = 0.0f;
					for(uint i = 0; i < Length; i++)
					{
						if(pKeyTime) *pKeyTime = StartTime + DurationSum;
						DurationSum += pDuration[i];
						if(Time < DurationSum) return i;
					}

					if(pKeyTime) *pKeyTime = StartTime + DurationSum;
					return(Length - 1);
					//return(_Time - (static_cast<fp32>(static_cast<uint32>(_Time / m_TotalDuration)) * m_TotalDuration));
//				}
			}
			else
			{
				if(_Time > m_TotalDuration)
					return (m_lDuration.Len()-1);
				else
				{
					const uint Length = m_lDuration.Len();
					const fp32* pDuration = m_lDuration.GetBasePtr();
					fp32 DurationSum = 0.0f;
					for(uint i = 0; i < Length; i++)
					{
						if(pKeyTime) *pKeyTime = DurationSum;
						DurationSum += pDuration[i];
						if(_Time < DurationSum) return i;
					}

					if(pKeyTime) *pKeyTime = DurationSum;
					return (Length - 1);
				}
			}
		}

//		fp32 GetRandomFp32(const uint32& _Rand)							{ return MFloat_GetRand(_Rand); }
//		fp32 GetAnimatedFp32(const CRegistry* _pReg, fp32 _Time)	{ return 0.0f; }
//		fp32 GetAnimatedFp32(const TArray<fp32>& lFp32, fp32 _Time)	{ const uint8& CurrKey = GetKeyFromTime(_Time); return LinearInterpolateFloat(lFp32[CurrKey], lFp32[MinMT(CurrKey+1,lFp32.Len())]); }
        
		virtual void CalcMaxBound_Box() { m_MaxBound = 0.0f; }
		virtual void CalcBound_Box(const CXR_AnimState* _pAnimState) { m_BoundBox = CBox3Dfp32(0,0); }
		virtual fp32 CalcBound_Sphere(const CXR_AnimState* _pAnimState) { return 16.0f; }

		virtual void RenderQuad(CXR_Model_EffectSystem* _pFXSystem, CXR_VBManager* _pVBM, int _SurfaceID, const CMTime& _AnimTime, const CVec3Dfp32* _pV, const uint32* _pC = NULL, const uint16* _pI = NULL, const CVec2Dfp32* _pTV = NULL, uint32 _Flags = FXFLAG_RENDERQUAD_X2);

		virtual bool Render_BeamStrip(CEffectSystemRenderData* _pRD, uint _SurfaceID, const CXR_BeamStrip* _pBeams, int _nBeams, int _Flags, const CMat4Dfp32* _pWMat = NULL, const CMat4Dfp32* _pVMat = NULL);
		virtual bool Render_Quad(CEffectSystemRenderData* _pRD, uint _SurfaceID, const CVec3Dfp32& _Pos, fp32 _Width, fp32 _Width2, fp32 _Height, fp32 _SphereRadius, fp32 _Angle, uint32 _Color, uint32 _Flags, const CMat4Dfp32* _pWMat = NULL, const CMat4Dfp32* _pVMat = NULL, const CMTime* _pTime = NULL);
		virtual bool Render_Quad_Skeleton(CEffectSystemRenderData* _pRD, uint _SurfaceID, fp32 _Width, fp32 _Width2, fp32 _SphereRadius, fp32 _Angle, uint32 _Color, uint32 _Flags, const CMat4Dfp32* _pWMat, const CMat4Dfp32* _pVMat, const CMTime* _pTime);
		virtual bool Render_ScreenQuad(CEffectSystemRenderData* _pRD, const CRect2Duint16& _VPRect16, int _SurfaceID, uint32 _Color, const CMTime* _pTime = NULL, CVec2Dfp32* _pTVTexGen = NULL);

		static void Render_Quad(CXR_VBManager* _pVBM, CXR_Engine* _pEngine, int _TextureID, const CVec3Dfp32& _LocalPos, fp32 _Size, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, bool _bNoZ = true, fp32 _Priority = 0.0f);

		fp32 GetPriority(const CXR_RenderInfo& _RenderInfo) const;

		// Clipping util
		void ClipHomogenousLine(CVec4Dfp32& _v0, CVec4Dfp32& _v1);

		// Base
		uint8						m_IntFlags;					// Internal flags
		uint8						m_Flags;
		uint16						m_RandFlags;
		TThinArray<fp32>			m_lDuration;
		TThinArray<fp32>			m_lDurationReciprocal;
		//TThinArray<fp32>			m_lTimeScale;			// Timescale effect,
		fp32						m_TotalDuration;
		//fp32						m_Priority;
		uint8						m_Priority;
		fp32						m_PriorityOffset;
		CBox3Dfp32					m_BoundBox;
		fp32						m_MaxBound;
		uint8						m_iShader;
		uint8						m_iRenderTarget;
		
		// History control
		uint16						m_UseHistoryObject;
		fp32						m_HistoryTime;
		fp32						m_HistorySpawn;
		//uint32					m_Enable;
		//uint32					m_Disable;
		//uint8						m_RasterMode;
		
		// Shader used during rendering
		CFXDataShader*				m_pShader;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataShader
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CFXDataShader : public CFXDataObject
{
	public:
		CFXDataShader();
		CFXDataShader(const CFXDataCollect& _Data);

		uint32					m_Enable;
		uint32					m_Disable;
		uint16					m_AlphaRef;
		uint8					m_RasterMode;
		uint8					m_ZCompare;
		uint8					m_AlphaCompare;
		
		uint8					m_iTexGen;
		uint8					m_nTexGenParams;
		uint8					m_nPredefinedParams;

		TThinArray<CVec4Dfp32>	m_lShaderParams;
		TThinArray<int>			m_lShaderMaps;

		CStr					m_Program;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataRenderTarget
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CFXDataRenderTarget : public CFXDataObject
{
	public:
		CFXDataRenderTarget();
		CFXDataRenderTarget(const CFXDataCollect& _Data);

		CStr							m_RenderTarget;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataBeam
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CFXDataBeam : public CFXDataObject
{
	public:
		CFXDataBeam();// {}
		CFXDataBeam(const CFXDataCollect& _Data);

		//virtual void Render(CXR_Model_EffectSystem* _pFXSystem, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, uint32 _Flags);
		virtual void Render(CEffectSystemRenderData* _pRD, const CMat4Dfp32& _WMat);
		virtual void CalcBound_Box(const CXR_AnimState* _pAnimState);

		TThinArray<int>			m_lSurfaceID;
		TThinArray<uint32>		m_lColor;
		TThinArray<fp32>		m_lWidth;
		TThinArray<fp32>		m_lLength;
		TThinArray<CVec3Dfp32>	m_lPosOffset;
		TThinArray<CVec3Dfp32>	m_lDirection;
		TThinArray<uint8>		m_lSegments;
		uint32					m_Seed;
		uint8					m_nMulti;

		// Supported layers for beam object
		//TThinArray<CFXLayerNoise>	m_lNoises;
		//TThinArray<CFXLayer*>	m_lpExtLayers;
		CFXLayer				m_Layer;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataLight
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CFXDataLight : public CFXDataObject
{
	public:
		CFXDataLight();
		CFXDataLight(const CFXDataCollect& _Data);

		//virtual void Render(CXR_Model_EffectSystem* _pFXSystem, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, uint32 _Flags);
		virtual void Render(CEffectSystemRenderData* _pRD, const CMat4Dfp32& _WMat){}
		virtual void RenderVis(CWorld_Client* _pWClient, CXR_SceneGraphInstance* _pSGI, uint16& _LightGUID, int _iOwner, fp32 _AnimTime, const CMat4Dfp32& _WMat, uint32 _Flags);

		TThinArray<CVec3Dfp32>	m_lIntensity;
		TThinArray<fp32>			m_lRange;
		TThinArray<CVec3Dfp32>	m_lPosOffset;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataWallmark
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CFXDataWallmark : public CFXDataObject
{
	public:
		CFXDataWallmark();
		CFXDataWallmark(const CFXDataCollect& _Data);

		virtual void RenderClient(const CXR_SurfaceContext* _pSurfaceContext, CWorld_Client* _pWClient, const CMTime& _AnimTime, const CMat4Dfp32& _WMat, uint32 _Flags);

		TThinArray<fp32>			m_lWidth;
		TThinArray<fp32>			m_lTolerance;
		TThinArray<int>			m_lSurfaceID;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataModel
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CFXDataModel : public CFXDataObject
{
	public:
		CFXDataModel() {}
		CFXDataModel(const CFXDataCollect& _Data) : CFXDataObject(_Data) {}

		//virtual void Render(CXR_Model_EffectSystem* _pFXSystem, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, uint32 _Flags);
		virtual void Render(CEffectSystemRenderData* _pRD, const CMat4Dfp32& _WMat);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataQuad
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CFXDataQuad : public CFXDataObject
{
	public:
		CFXDataQuad();
		CFXDataQuad(const CFXDataCollect& _Data);

		//virtual void Render(CXR_Model_EffectSystem* _pFXSystem, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, uint32 _Flags);
		virtual void Render(CEffectSystemRenderData* _pRD, const CMat4Dfp32& _WMat);
		virtual void CalcMaxBound_Box();
		virtual void CalcBound_Box(const CXR_AnimState* _pAnimState);
		virtual fp32 CalcBound_Sphere(const CXR_AnimState* _pAnimState);

		virtual void CreateConeScreenQuad_Render(CEffectSystemRenderData* _pRD, const CMat4Dfp32& _WMat);

		TThinArray<fp32>			m_lWidth;
		TThinArray<fp32>			m_lWidth2;
		TThinArray<int>			m_lSurfaceID;
		TThinArray<fp32>			m_lRotationAngle;
		TThinArray<CVec3Dfp32>	m_lPositionNoise;
		TThinArray<CVec3Dfp32>	m_lPosOffset;
		TThinArray<uint32>		m_lColor;
		TThinArray<fp32>			m_lHeight;
		TThinArray<fp32>			m_lSphereRadius;
		uint32					m_Seed;
		uint8					m_iRenderType;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataCone
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CFXDataCone : public CFXDataObject
{
	public:
		CFXDataCone();
		CFXDataCone(const CFXDataCollect& _Data);

		//virtual void Render(CXR_Model_EffectSystem* _pFXSystem, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, uint32 _Flags);
		virtual void Render(CEffectSystemRenderData* _pRD, const CMat4Dfp32& _WMat);
		virtual void CalcBound_Box(const CXR_AnimState* _pAnimState);

		TThinArray<fp32>		m_lOuterRadius;
		TThinArray<fp32>		m_lInnerRadius;
		TThinArray<int>		m_lSurfaceID;
		TThinArray<fp32>		m_lRotationAngle;
		TThinArray<uint32>	m_lColor;
		TThinArray<fp32>		m_lHeight;
		TThinArray<uint8>	m_lSegments;
		TThinArray<uint8>	m_lRings;		// Valid on spheroidic cones
		uint32				m_Seed;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataParticlesOpt
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CFXDataParticlesOpt : public CFXDataObject
{
	public:
		CFXDataParticlesOpt();// {}
		CFXDataParticlesOpt(const CFXDataCollect& _Data);// : CFXDataObject(_Data);// {}

		//virtual void Render(CXR_Model_EffectSystem* _pFXSystem, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, uint32 _Flags);
		virtual void Render(CEffectSystemRenderData* _pRD, const CMat4Dfp32& _WMat);

		TPtr<ParticlesOptClassName>	m_spParticleSystem;
		//ParticlesOptClassName*	m_pParticleSystem;
		//CBox3Dfp32					m_BoundBox;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataBeamStrip
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CFXDataBeamStrip : public CFXDataObject
{
	public:
		CFXDataBeamStrip();
		CFXDataBeamStrip(const CFXDataCollect& _Data);

		//virtual void Render(CXR_Model_EffectSystem* _pFXSystem, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, uint32 _Flags);
		virtual void Render(CEffectSystemRenderData* _pRD, const CMat4Dfp32& _WMat);
		virtual void CalcBound_Box(const CXR_AnimState* _pAnimState);

		TThinArray<fp32>			m_lWidth;
		TThinArray<int>			m_lSurfaceID;
		TThinArray<uint8>		m_lSegments;
		TThinArray<fp32>			m_lStripHeight;
		//TThinArray<fp32>		m_lRotationAngle;
		TThinArray<fp32>			m_lWidthNoise;
		TThinArray<CVec3Dfp32>	m_lPosNoise;
		TThinArray<CVec3Dfp32>	m_lPosOffset;
		TThinArray<CVec3Dfp32>	m_lVelocity;
		TThinArray<CVec3Dfp32>	m_lVelocityNoise;
		TThinArray<uint32>		m_lColor;
		TThinArray<CVec2Dfp32>	m_lTextureScroll;
		TThinArray<CVec3Dfp32>	m_lDirection;
		uint32					m_Seed;

		// Supported layers for beam strips
		CFXLayer				m_Layer;
		//TThinArray<CFXLayer*>	m_lpExtLayers;
		//TThinArray<CFXLayerTwirl>	m_lTwirls;
		//TThinArray<CFXLayerNoise>	m_lNoises;
		//TThinArray<CFXLayerPath>	m_lPaths;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataParticleHook
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CFXDataFXParticleHook : 
#ifdef TESTNEWPARTICLES
	public CXR_Model_Particles
#else
	public CXR_Model_ParticlesOpt
#endif
{
	friend class CXR_Model_EffectSystem;
public:
	CFXDataFXParticleHook();
	virtual bool GenerateParticles(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, CXR_Model_Custom_RenderParams* _pRenderParams);
	virtual bool Generate_Render(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, CXR_Model_Custom_RenderParams* _pRenderParams, int _GenerateFlags);
	virtual void Generate_Distribution_Point(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, int& _GenerateFlags);
	virtual void Generate_Distribution(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, int& _GenerateFlags);
	virtual void RenderPerRenderVariables(const CXR_AnimState* _pAnimState, PerRenderVariables* M_RESTRICT _pPR);
	//virtual void GetDistributedPoint_InSkeleton(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, CVec3Dfp32& _point, uint32& _iRandseed) const;
	//virtual void GetDistributedPoint_InBounding(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, CVec3Dfp32& _Point, uint32& _iRandSeed) const;
	
	//M_INLINE virtual void SetLocalToWorld(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const CMat4Dfp32& _WMat) { if(_pPR->m_pSkeletonInst && _pPR->m_pSkeleton && m_lBoneBind.Len() > 0) _pBatch->m_LocalToWorld.Unit(); else ParticlesOptClassName::SetLocalToWorld(_pPR, _pBatch, _WMat); }
	
	virtual TPtr<CXR_ModelInstance> CreateModelInstance();

	virtual void CreateLayers(const CFXDataCollect& _Data);

	// Supported layers for fx particle systems
	CFXLayer m_Layer;
	//TThinArray<CFXLayer*> m_lpExtLayers;
	//TThinArray<CFXLayerTwirl>		m_lTwirl;
	//TThinArray<CFXLayerBoneBind>	m_lBoneBind;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataFXParticle
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CFXDataFXParticle : public CFXDataObject
{
	public:
		CFXDataFXParticle();
		CFXDataFXParticle(const CFXDataCollect& _Data);

		//virtual void Render(CXR_Model_EffectSystem* _pFXSystem, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, uint32 _Flags);
		virtual void Render(CEffectSystemRenderData* _pRD, const CMat4Dfp32& _WMat);
//		virtual void OnCollisionRefresh(CWorld_Client& _WClient, CEffectSystemHistory& _History, int _iExclude = 0);
//		virtual void CalcBound_Box(const CXR_AnimState* _pAnimState);
//		virtual fp32 CalcBound_Sphere(const CXR_AnimState* _pAnimState);

		TPtr<CFXDataFXParticleHook>	m_spParticleSystem;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CEffectSystemHistory
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CEffectSystemHistory : public CXR_ModelInstance
{
	friend class CXR_Model_EffectSystem;

public:
	struct CFXDataBeamStripHistory
	{
		CVec3Dfp32	m_Position;
		fp32			m_Time;
		uint32		m_Seed;
		uint8		m_Cut;
	};

	//struct CFXDataFXParticleHistory
	//{
		//uint32		m_Seed;
		//CVec3Dfp32	m_Position;
		//CVec3Dfp32	m_Velocity;
		//fp32			m_Time;
		//fp32			m_CollTime;
	//};

	struct CFXDataBeamHistory
	{
		fp32			m_Time;
		CVec3Dfp32	m_Position;
		CVec3Dfp32	m_Direction;
	};

	struct CFXDataConeHistory
	{
		CVec3Dfp32	m_Position;
	};

	struct CFXDataQuadHistory
	{
		fp32			m_Time;
		CVec3Dfp32	m_Position;
		uint32		m_Seed;
	};

	class CEffectSystemHistoryObject
	{
		friend class CEffectSystemHistory;

		public:
			CEffectSystemHistoryObject() : m_CurrentPos(0), m_SizeEntry(0) /*, m_nEntries(0)*/ { m_lEntries.SetLen(0); m_lEntries.SetGrow(0); }
			CEffectSystemHistoryObject(int _ElemSize, int _Type = -1) : m_CurrentPos(0) //, m_nEntries(0)
			{
				switch(_Type)
				{
					case FXDATA_TYPE_BEAMSTRIP:
						m_SizeEntry = sizeof(CEffectSystemHistory::CFXDataBeamStripHistory);//FXSIZE_TYPE_BEAMSTRIP;
						break;
					//case FXDATA_TYPE_FXPARTICLE:
					//	m_SizeEntry = sizeof(CEffectSystemHistory::CFXDataFXParticleHistory);//FXSIZE_TYPE_FXPARTICLE;
					//	break;
					case FXDATA_TYPE_BEAM:
						m_SizeEntry = sizeof(CEffectSystemHistory::CFXDataBeamHistory);//FXSIZE_TYPE_BEAM;
						break;
					case FXDATA_TYPE_CONE:
						m_SizeEntry = sizeof(CEffectSystemHistory::CFXDataConeHistory);//FXSIZE_TYPE_CONE;
						break;
					case FXDATA_TYPE_QUAD:
						m_SizeEntry = sizeof(CEffectSystemHistory::CFXDataQuadHistory);//FXSIZE_TYPE_QUAD;
						break;
					//case FXDATA_TYPE_PARTICLESOPT:
					//	m_SizeEntry = sizeof(CEffectSystemHistory::CFXDataFXParticleHistory);
					//	break;
					default:
						m_SizeEntry = _ElemSize;
						break;
				}
				
				m_lEntries.SetLen(0);
				m_lEntries.SetGrow(32*m_SizeEntry);
			}

			const uint8* GetDataBasePtr() const		{ return m_lEntries.GetBasePtr(); }
			      uint8* GetDataBasePtr()			{ return m_lEntries.GetBasePtr(); }

			uint8* GetDataBasePtrNumEntries(uint16& _nEntries)
			{ 
				_nEntries = m_lEntries.Len() / m_SizeEntry;
				return m_lEntries.GetBasePtr();
			}
			const uint8* GetDataBasePtrNumEntries(uint16& _nEntries) const
			{ 
				_nEntries = m_lEntries.Len() / m_SizeEntry;
				return m_lEntries.GetBasePtr();
			}

			uint8 GetDataSize()						{ return m_SizeEntry; }
			uint8 GetDataSize() const				{ return m_SizeEntry; }

			uint16 GetNumEntries()					{ return m_lEntries.Len() / m_SizeEntry; }
			uint16 GetNumEntries() const			{ return m_lEntries.Len() / m_SizeEntry; }

			#define GETBEAMSTRIPENTRY(pDataChunk)	*(CEffectSystemHistory::CFXDataBeamStripHistory*)pDataChunk; pDataChunk += sizeof(CEffectSystemHistory::CFXDataBeamStripHistory)//FXSIZE_TYPE_BEAMSTRIP
			//#define GETFXPARTICLEENTRY(pDataChunk)	*(CEffectSystemHistory::CFXDataFXParticleHistory*)pDataChunk; pDataChunk += sizeof(CEffectSystemHistory::CFXDataFXParticleHistory)//FXSIZE_TYPE_FXPARTICLE
			#define GETBEAMENTRY(pDataChunk)		*(CEffectSystemHistory::CFXDataBeamHistory*)pDataChunk; pDataChunk += sizeof(CEffectSystemHistory::CFXDataBeamHistory)//FXSIZE_TYPE_BEAM
			#define GETQUADENTRY(pDataChunk)		*(CEffectSystemHistory::CFXDataQuadHistory*)pDataChunk; pDataChunk += sizeof(CEffectSystemHistory::CFXDataQuadHistory)//FXSIZE_TYPE_QUAD

			#define GETBEAMENTRYSIMPLE(pDataChunk)	*(CEffectSystemHistory::CFXDataBeamHistory*)pDataChunk
			#define GETBEAMENTRYPREV(pDataChunk)	*(CEffectSystemHistory::CFXDataBeamHistory*)(pDataChunk - sizeof(CEffectSystemHistory::CFXDataBeamHistory)//FXSIZE_TYPE_BEAM

			      CFXDataBeamStripHistory*	GetBeamStripEntry(uint _i)				{ return (CFXDataBeamStripHistory*)(m_lEntries.GetBasePtr() + (m_SizeEntry * _i)); }
			const CFXDataBeamStripHistory*	GetBeamStripEntry(uint _i) const		{ return (const CFXDataBeamStripHistory*)(m_lEntries.GetBasePtr() + (m_SizeEntry * _i)); }
			//CFXDataFXParticleHistory*	GetFXParticleEntry(uint _i)					{ return (CFXDataFXParticleHistory*)(m_lEntries.GetBasePtr() + (m_SizeEntry * _i)); }
			//CFXDataFXParticleHistory*	GetFXParticleEntry(uint _i) const			{ return (CFXDataFXParticleHistory*)(m_lEntries.GetBasePtr() + (m_SizeEntry * _i)); }
			      CFXDataBeamHistory*		GetBeamEntry(uint _i)					{ return (CFXDataBeamHistory*)(m_lEntries.GetBasePtr() + (m_SizeEntry * _i)); }
			const CFXDataBeamHistory*		GetBeamEntry(uint _i) const				{ return (const CFXDataBeamHistory*)(m_lEntries.GetBasePtr() + (m_SizeEntry * _i)); }
			      CFXDataConeHistory*		GetConeEntry(uint _i)					{ return (CFXDataConeHistory*)(m_lEntries.GetBasePtr() + (m_SizeEntry * _i)); }
			const CFXDataConeHistory*		GetConeEntry(uint _i) const				{ return (const CFXDataConeHistory*)(m_lEntries.GetBasePtr() + (m_SizeEntry * _i)); }
			      CFXDataQuadHistory*		GetQuadEntry(uint _i)					{ return (CFXDataQuadHistory*)(m_lEntries.GetBasePtr() + (m_SizeEntry * _i)); }
			const CFXDataQuadHistory*		GetQuadEntry(uint _i) const				{ return (const CFXDataQuadHistory*)(m_lEntries.GetBasePtr() + (m_SizeEntry * _i)); }

			uint8* RemoveUpdate(int _Element)				{ m_lEntries.Delx((_Element * m_SizeEntry), m_SizeEntry); return m_lEntries.GetBasePtr(); }
			void RemoveEntry(int _Element)					{ m_lEntries.Delx((_Element * m_SizeEntry), m_SizeEntry); }
			//void AddEntry(CFXDataFXParticleHistory* _pEntry)		{ m_lEntries.Insertx(m_lEntries.Len(), (uint8*)_pEntry, FXSIZE_TYPE_FXPARTICLE); }
			void AddEntry(CFXDataBeamStripHistory* _pEntry)			{ m_lEntries.Insertx(m_lEntries.Len(), (uint8*)_pEntry, FXSIZE_TYPE_BEAMSTRIP); }
			void AddEntry(CFXDataBeamHistory* _pEntry)				{ m_lEntries.Insertx(m_lEntries.Len(), (uint8*)_pEntry, FXSIZE_TYPE_BEAM);}
			void AddEntry(CFXDataConeHistory* _pEntry)				{ m_lEntries.Insertx(m_lEntries.Len(), (uint8*)_pEntry, FXSIZE_TYPE_CONE);}
			void AddEntry(CFXDataQuadHistory* _pEntry)				{ m_lEntries.Insertx(m_lEntries.Len(), (uint8*)_pEntry, FXSIZE_TYPE_QUAD); }
			//void AddEntries(CFXDataFXParticleHistory* _pEntries, int _nEntries)	{ m_lEntries.Insertx(m_lEntries.Len(), (uint8*)_pEntries, FXSIZE_TYPE_FXPARTICLE * _nEntries); }
			void AddEntries(CFXDataBeamStripHistory* _pEntries, int _nEntries)	{ m_lEntries.Insertx(m_lEntries.Len(), (uint8*)_pEntries, FXSIZE_TYPE_BEAMSTRIP * _nEntries); }

			TArray<uint8>							m_lEntries;		// History data storage
			CVec3Dfp32								m_CurrentPos;	// Current Position		(Remove?)

		private:
			uint8									m_SizeEntry;	// Size per entry object
	};

	class CCreationParam
	{
	public:
		CCreationParam(int _Param0 = 0) : m_Param0(0) {}
		int m_Param0;
	};

	CEffectSystemHistory(uint _nObjects, uint _nModelInstances, const CCreationParam* _pParams = NULL);
	CEffectSystemHistory(const CEffectSystemHistory* pObject);

	// pure virtual overrides
	virtual void Create(class CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context);
	virtual void OnRefresh(const CXR_ModelInstanceContext& _Context, const CMat4Dfp32* _pMat = NULL, int _nMat = 0, int _Flags = 0);
	virtual bool NeedRefresh(class CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context);

	virtual TPtr<CXR_ModelInstance> Duplicate() const;
	virtual void operator = (const CXR_ModelInstance& _Instance);
	void operator = (const CEffectSystemHistory& _Instance);

	CEffectSystemHistoryObject* GetHistory(uint _nObject);
	CEffectSystemHistoryObject* GetHistory(uint _nObject) const;
	TPtr<CXR_ModelInstance>		GetModelInstance(uint _iObject);
	TPtr<CXR_ModelInstance>		GetModelInstance(uint _iObject) const;
	uint32						GetGameTick() { return m_GameTick; }
	uint16						GetNumHistoryObjects() { return m_Objects; }
	uint32						GetNumHistoryEntries()
	{
		uint32 nEntries = 0;
		CEffectSystemHistoryObject* pObjects = m_lObjectHistory.GetBasePtr();
		for(int i = 0; i < m_Objects; i++)
			nEntries += pObjects[i].GetNumEntries();
		return nEntries;
	}

private:

	TArray<TPtr<CXR_ModelInstance> >	m_lspModelInstances;
	TArray<CEffectSystemHistoryObject>	m_lObjectHistory;
	uint16								m_Objects;
	uint8								m_ModelInstances;
	uint32								m_GameTick;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_EffectSystem
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Model_EffectSystem;
class CXR_Model_EffectSystem : public CXR_Model_Custom
{
	friend class CEffectSystemData;
	MRTC_DECLARE;

public:
	CXR_Model_EffectSystem();

	virtual void Create(const char* _pParam);
	virtual void OnPrecache(CXR_Engine* _pEngine, int _iVariation);
	virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat);
	virtual void PreRender(CXR_Engine* _pEngine, CXR_ViewClipInterface* _pViewClip, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0);
	virtual fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState = 0);
	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState = 0);
	virtual aint GetParam(int _Param);
	virtual void SetParam(int _Param, aint _Value);
	static void OnClientRenderVis(CWorld_Client* _pWClient, const CXR_Model_EffectSystem* _pFXSystem, CXR_SceneGraphInstance* _pSGI, fp32 _AnimTime, const CMat4Dfp32& _WMat, int _iOwner);
	static void OnClientRender(const CXR_Model_EffectSystem* _pFXSystem, CWorld_Client* _pWClient, const CMTime& _AnimTime, const CMat4Dfp32& _WMat);

	// This is the function that actually sets the whole effect up.
	void CreateEffect(CRegistry *_pRegistry);

	CXR_RenderInfo& GetRenderInfo(CXR_Model_Custom_RenderParams* _pRenderParams) { return _pRenderParams->m_RenderInfo; }

	bool NeedCollisionUpdate();
	bool NeedOnRenderVis();
	bool NeedOnRenderClient();

	virtual TPtr<CXR_ModelInstance> CreateModelInstance();

	// Called from effect system object (GameObject, an effect system with collision info has to exist as an game object!)
	static void OnCollisionRefresh(class CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Model_EffectSystem& _FXModel, CEffectSystemHistory& _History, int _iOwner = 0);

	CEffectSystemRenderData* GetRenderData() { return m_pRD; }
	uint32 GetNumFXParticles() { return m_lFXParticles.Len(); }
	CFXDataFXParticle* GetFXParticles() { return m_lFXParticles.GetBasePtr(); }
	
	static CEffectSystemHistory::CEffectSystemHistoryObject* GetFXParticleHistory(CXR_Model_EffectSystem* _pFXSystem, const CFXDataFXParticle& FXParticle)			{ return _pFXSystem->m_pRD->m_pHistory->GetHistory(FXParticle.m_UseHistoryObject); }
	//static CEffectSystemHistory::CEffectSystemHistoryObject* GetFXParticleHistory(CXR_Model_EffectSystem* _pFXSystem, const CFXDataFXParticle& FXParticle) const	{ return _pFXSystem->m_pRD->m_pHistory->GetHistory(FXParticle.m_UseHistoryObject); }

	CStr GetShader(uint _iShader);
	CFXDataShader* GetShaderObject(uint _iShader);
	
	uint8 m_RenderingObject;

private:
	static void	Init();
	CFXDataObject* GetDataObject(uint _iObject, const uint8* _pObjectsType, const uint8* _piObjects);
	void EvalRegisterObject(const CRegistry* _pReg, CFXDataCollect& _DataStorage);
	void RetriveNumKeys(const CRegistry* _pReg, CFXDataCollect& _DataStorage);
	void RetriveShaderSettings(const CRegistry* _pReg, CFXDataCollect& _DataStorage);
	void LinkShaders();

	void RetriveRenderTargetSettings(const CRegistry* _pReg, CFXDataCollect& _DataStorage);
	void AddRenderTarget(CFXDataCollect& _DataStorage);
	void OptimizeRenderTargets();

	// TODO:	Yuck, this code really isn't very nice. It should be reformed to work much more intelligent.
	TArray<uint8>				m_lObjectsType;
	TArray<uint8>				m_liObjects;
	TArray<CFXDataBeam>			m_lBeams;
	TArray<CFXDataQuad>			m_lQuads;
	TArray<CFXDataParticlesOpt>	m_lParticlesOpt;
	TArray<CFXDataLight>		m_lLights;
	TArray<CFXDataBeamStrip>	m_lBeamStrips;
	TArray<CFXDataFXParticle>	m_lFXParticles;
	TArray<CFXDataShader>		m_lShaders;
	TArray<CFXDataRenderTarget>	m_lRenderTargets;
	TArray<CFXDataCone>			m_lCones;
	TArray<CFXDataWallmark>		m_lWallmarks;
	uint8						m_nObjects;
	uint32						m_Flags;
	CEffectSystemRenderData		m_RD;
	CEffectSystemRenderData*	m_pRD;
	fp32							m_Duration;

	static bool					m_bStaticsInitialized;
	CRegistry*					m_pRegistry;

	#ifndef M_RTM
		CStr m_EffectSystemName;
		uint8 Debug_ValueValidation(const CRegistry* _pReg, uint8 _nKeys);
	#endif

public:
	CXR_Model_Custom_RenderParams*	m_pRenderParams;
	static uint16					m_lQuadCrossTriangles[12];
	static fp32						m_lQuadCrossTVertices[5][2];
	static uint16					m_lBoxTriangles[36];
};


#endif

