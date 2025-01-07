#ifndef __WBSP4GlassCommon_h__
#define __WBSP4GlassCommon_h__

#include "../Model_BSP4/XW4Common.h"
#include "../Model_BSP2/WBSP2Model.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Defines
|__________________________________________________________________________________________________
\*************************************************************************************************/
#define GLASS_USE_BOUNDBOX		0
//#define GLASS_USE_SPHERE		1

#ifndef M_RTM
	// Change to zero to disable all glass debug info
	#define GLASS_DEEP_DEBUG	1
//	#define GLASS_DEEP_TRACE	1
	#define GLASS_DEBUG_SCOPE
	#define GLASS_OPTIMIZE_OFF 0
	//#define GLASS_DEBUG_SCOPEALL
	//#define GLASS_DEBUG_CRUSH
#else
	#define GLASS_OPTIMIZE_OFF 0
	#define GLASS_DEEP_DEBUG 0
	#define GLASS_DEBUG_NOSCOPE
//	#define GLASS_DEEP_TRACE 0
//	#define GLASS_DEBUG_CRUSH
#endif


// Remove all MSCOPE information
#ifdef GLASS_DEBUG_NOSCOPE
#define GLASS_MSCOPE_ALL(_First, _Second)
#define GLASS_MSCOPE(_First, _Second)
#endif


// Keep fat function MSCOPE's
#ifdef GLASS_DEBUG_SCOPE
#define GLASS_MSCOPE_ALL(_First, _Second)
#define GLASS_MSCOPE							MSCOPE
#endif

// Insert MSCOPE EVERYWHERE!
#ifdef GLASS_DEBUG_SCOPEALL
#define GLASS_MSCOPE_ALL						MSCOPE
#define GLASS_MSCOPE							MSCOPE
#endif


#define GLASS_DEBUG				GLASS_DEEP_DEBUG
#define GLASS_MAX_LIGHTS		16					// Maximum lights allowed
#define GLASS_MAX_RENDER		128					// Maximum glasses allowed during rendering
#define GLASS_MAX_PORTALLEAVES	1024

// Wallmark defines
#define GLASS_WM_FADECOLOR				2
#define GLASS_WM_DISTANCE				0.0f
#define GLASS_WM_PERPENDICULAR_TREHOLD	0.1f


#ifndef M_RTM
	// Wire colors								   BGR			  RGB
	#define GLASS_WIRE_WHITE					0xff7f7f7f	// 0xff7f7f7f
	#define GLASS_WIRE_RED						0xff00007f	// 0xff7f0000
	#define GLASS_WIRE_GREEN					0xff007f00	// 0xff007f00
	#define GLASS_WIRE_BLUE						0xff7f0000	// 0xff00007f
	#define GLASS_WIRE_YELLOW					0xff007f7f	// 0xff7f7f00
	#define GLASS_WIRE_PURPLE					0x7f7f007f	// 0x7f7f007f
	#define GLASS_WIRE_DK_WHITE					0x7f3f3f3f	// 0x7f3f3f3f
	#define GLASS_WIRE_DK_RED					0x7f00003f	// 0x7f3f0000
	#define GLASS_WIRE_DK_GREEN					0x7f003f00	// 0x7f003f00
	#define GLASS_WIRE_DK_BLUE					0x7f3f0000	// 0x7f00003f
	#define GLASS_WIRE_DK_YELLOW				0xff003f3f	// 0xff3f3f00
	#define GLASS_WIRE_DK_PURPLE				0x7f3f003f	// 0x7f3f003f

	#define __xrLine__(x)		#x
	#define _xrFileLine(x)		__FILE__ "("__xrLine__(x)") : "
	#define xrMsg(x)			message(_xrFileLine(__LINE__) " Message: " x)
	#define xrWarning(x)		message(_xrFileLine(__LINE__) " Warning: " x)
	#define xrFixMe(x)			message(_xrFileLine(__LINE__) " Fix Me: " x)
#else
	#define __xrLine__(x)		#x
	#define _xrFileLine(x)		__FILE__ "("__xrLine__(x)") : "
	#define xrMsg(x)			message(_xrFileLine(__LINE__) " Message: " x)
	#define xrWarning(x)		message(_xrFileLine(__LINE__) " WARNING : " x)
	#define xrFixMe(x)			message(_xrFileLine(__LINE__) " CRITICAL : " x)
#endif


#define GLASS_CUTFENCE_EPSILON 0.0001f
#define GLASS_CUTFENCE_EPSILONFP64 0.0000001f

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| enum
|__________________________________________________________________________________________________
\*************************************************************************************************/
enum
{
	GLASSNODE_STATUS_NA				= 0,
	GLASSNODE_STATUS_PROCESSED		= M_Bit(0),
	GLASSNODE_STATUS_VISIBLE		= M_Bit(1),

	GLASS_MAX_INSTANCE				= 0xffff, //(uint16)~0,

	GLASS_BATCH_MAX					= 24,
	GLASS_NUM_PALETTEMATRICES		= GLASS_BATCH_MAX,
	GLASS_NUM_COMP_SINGLE			= GLASS_BATCH_MAX * 3,
	GLASS_NUM_COMP_DUAL				= GLASS_NUM_COMP_SINGLE * 2,
	GLASS_NUM_MATRIXINDICES			= GLASS_NUM_COMP_DUAL,
	GLASS_NUM_MATRIXWEIGHTS			= GLASS_NUM_COMP_DUAL * 4,
	GLASS_NUM_INDICES				= GLASS_NUM_COMP_DUAL,
	GLASS_NUM_COLORS				= GLASS_NUM_COMP_DUAL,
	GLASS_NUM_NORMALS				= GLASS_NUM_COMP_DUAL,
	GLASS_NUM_TANGENTS				= GLASS_NUM_COMP_DUAL,

	GLASS_DATA_GAMETICK				= 0,

	GLASS_CRUSHFLAGS_FULLBREAK		= M_Bit(0),						// Overridables when handling crushing

	GLASS_ROT_MAT_MAX				= 32,
	GLASS_ROT_MAT_AND				= (GLASS_ROT_MAT_MAX - 1),
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Random number generator (To avoid locks and extensive tampering with MRTC Merseinne Twister)
|__________________________________________________________________________________________________
\*************************************************************************************************/
#define __GLAS_RAND_XNEW				(__xRandomGen=(18000*(__xRandomGen&0xFFFF)+(__xRandomGen>>16)))
#define __GLAS_RAND_YNEW				(__yRandomGen=(30903*(__yRandomGen&0xFFFF)+(__yRandomGen>>16)))
#define GLASS_RAND_CREATE2(_x, _y)		uint32 __xRandomGen = _x, __yRandomGen = _y
#define GLASS_RAND_CREATE1(_x)			uint32 __xRandomGen = _x; uint32 __yRandomGen = __xRandomGen + 1
#define GLASS_RAND_RESEED(_x, _y)		_xRandomGen = _x; __yRandomGen = _y
#define GLASS_RAND						((__GLAS_RAND_XNEW<<16)+(__GLAS_RAND_YNEW&0xFFFF))


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_LinkContext
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CBSP4Glass_LinkContext : public CBSP2_LinkContext
{
public:
	
	DECLARE_OPERATOR_NEW

	CBSP4Glass_LinkContext() : CBSP2_LinkContext()
	{
	}
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_Index
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CBSP4Glass_IndexLoad;
class CBSP4Glass_Index
{
public:
	// Number of instances and index
	uint16	m_iInstance;
	uint8	m_nInstance;
	uint32	m_NameHash;

	// Bounding information for this node (index)
	CBox3Dfp32	m_BoundingBox;
	CVec3Dfp32	m_Center;
	fp32		m_Radius;

	CBSP4Glass_Index();
	~CBSP4Glass_Index();

	virtual void Read(CCFile* _pFile);
	virtual void Write(CCFile* _pFile);

	CBSP4Glass_Index& operator = (const CBSP4Glass_IndexLoad& _Index);
};


class CBSP4Glass_IndexLoad : public CBSP4Glass_Index
{
public:
	CStr	m_Name;

	CBSP4Glass_IndexLoad();
	~CBSP4Glass_IndexLoad();

	virtual void Read(CCFile* _pFile);
	virtual void Write(CCFile* _pFile);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_Chain
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CBSP4Glass_Chain
{
private:
	CRC_MatrixPalette*	m_pMP;
	CXR_VBChain*		m_pChain;

public:
	CBSP4Glass_Chain()
	{
	}

	~CBSP4Glass_Chain()
	{
	}

	void Clear()
	{
		m_pChain = NULL;
		m_pMP = NULL;
	}

	CXR_VBChain* GetChain()
	{
		return m_pChain;
	}

	CRC_MatrixPalette* GetMatrixPalette()
	{
		return m_pMP;
	}

	bool IsValid_MatrixPalette()
	{
		return (m_pMP != NULL);
	}

	bool IsValid_Chain()
	{
		return (m_pChain != NULL);
	}

	void SetChain(CXR_VBChain* _pChain)
	{
		m_pChain = _pChain;
	}

	void SetMatrixPalette(CRC_MatrixPalette* _pMP)
	{
		m_pMP = _pMP;
	}

	void Set(CXR_VBChain* _pChain, CRC_MatrixPalette* _pMP)
	{
		m_pChain = _pChain;
		m_pMP = _pMP;
	}
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CGlassAttrib
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CGlassAttrib
{
public:
	enum
	{
		// User flags
		ATTRIB_USE_DURABILITY		= M_Bit(0),		// Has durability
		ATTRIB_UNBREAKABLE			= M_Bit(1),		// Unbreakable glass
		ATTRIB_FULLBREAK			= M_Bit(2),		// Invalidate whole grid at destruction
		ATTRIB_IGNOREEDGE			= M_Bit(3),		// Ignores edge creation
		ATTRIB_TIMEBREAKER			= M_Bit(4),		// Loosen shards over time
		ATTRIB_USETHICKNESS			= M_Bit(5),		// Consider thickness when building glass
		ATTRIB_NOSAVE				= M_Bit(6),		// Don't save this window state

		// Flags
		ATTRIB_FLAGS_NOPHYS				= M_Bit(0),
		ATTRIB_FLAGS_NOBASERENDER		= M_Bit(1),		// Don't render base model
		ATTRIB_FLAGS_INACTIVE			= M_Bit(2),		// Inactive, no rendering
	};

	CPlane3Dfp32	m_Plane;
	CVec2Dfp32		m_DimRcp;
	CVec2Dfp32		m_Min;
	CVec3Dfp32		m_MiddlePoint;
	uint8			m_liComp[2];
	fp32			m_TxtWidthInv;
	fp32			m_TxtHeightInv;
	fp32			m_Thickness;
	uint8			m_Flags;
	uint32			m_UserFlags;
	uint32			m_Durability;

	CGlassAttrib();
	~CGlassAttrib();

	void   OnDeltaLoad(CCFile* _pFile);
	uint32 OnDeltaSave(CCFile* _pFile);

	void Attrib_SetPhys(const bool _bHasPhys);
	void Attrib_SetBaseRender(const bool _bBaseRender);
	void Attrib_SetInactive(const bool _bInactive);

	M_INLINE bool Attrib_NoPhys() const			{ return ((m_Flags & ATTRIB_FLAGS_NOPHYS) != 0);		}
	M_INLINE bool Attrib_NoBaseRender() const	{ return ((m_Flags & ATTRIB_FLAGS_NOBASERENDER) != 0);	}
	M_INLINE bool Attrib_Inactive() const		{ return ((m_Flags & ATTRIB_FLAGS_INACTIVE) != 0);		}

	M_INLINE bool Attrib_UseDurability() const	{ return ((m_UserFlags & ATTRIB_USE_DURABILITY) != 0);	}
	M_INLINE bool Attrib_Unbreakable() const	{ return ((m_UserFlags & ATTRIB_UNBREAKABLE) != 0);		}
	M_INLINE bool Attrib_FullBreak() const		{ return ((m_UserFlags & ATTRIB_FULLBREAK) != 0);		}
	M_INLINE bool Attrib_IgnoreEdge() const		{ return ((m_UserFlags & ATTRIB_IGNOREEDGE) != 0);		}
	M_INLINE bool Attrib_TimeBreaker() const	{ return ((m_UserFlags & ATTRIB_TIMEBREAKER) != 0);		}
	M_INLINE bool Attrib_UseThickness() const	{ return ((m_UserFlags & ATTRIB_USETHICKNESS) != 0);	}
	M_INLINE bool Attrib_NoSave() const			{ return ((m_UserFlags & ATTRIB_NOSAVE) != 0);			}
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_CrushData
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CBSP4Glass_CrushData
{
public:
	CVec3Dfp32		m_HitPos;
	fp32				m_ForceScale;
	uint8			m_iComp0;
	uint8			m_iComp1;
	uint8			m_iComp2;

	CBSP4Glass_CrushData() {}
	~CBSP4Glass_CrushData() {}
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_TangentSetup
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CBSP4Glass_TangentSetup
{
private:
	fp32	m_NTangU[2];
	fp32 m_NTangV[2];
	CVec3Dfp32 m_TangU[2];
	CVec3Dfp32 m_TangV[2];

public:
	CBSP4Glass_TangentSetup(const class CBSP4Glass_MappingSetup& _MappingSetup, const CVec3Dfp32& _PlaneN);

	M_INLINE CVec3Dfp32 GetFrontSideTangU() const	{ return m_TangU[0]; }
	M_INLINE CVec3Dfp32 GetFrontSideTangV() const	{ return m_TangV[0]; }
	M_INLINE CVec3Dfp32 GetBackSideTangU() const		{ return m_TangU[1]; }
	M_INLINE CVec3Dfp32 GetBackSideTangV() const		{ return m_TangV[1]; }
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_MappingSetup
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CBSP4Glass_MappingSetup
{
private:
	CVec2Dfp32			m_TProjMin;
	CVec2Dfp32			m_TProjMax;
	fp32				m_UVecLenSqrInv;
	fp32				m_VVecLenSqrInv;
	fp32				m_UVecLenRcp;
	fp32				m_VVecLenRcp;
	fp32				m_UOffset;
	fp32				m_VOffset;
	fp32				m_TxtWidthInv;
	fp32				m_TxtHeightInv;
	CVec3Dfp32			m_UVec;
	CVec3Dfp32			m_VVec;

public:
	CBSP4Glass_MappingSetup(const CXR_PlaneMapping& _Mapping, const CGlassAttrib& _pAttrib);

	M_INLINE CVec2Dfp32 GetMinProj() const	{ return m_TProjMin; }
	M_INLINE CVec2Dfp32 GetMaxProj() const	{ return m_TProjMax; }
	M_INLINE fp32 GetUOffset() const		{ return m_UOffset; }
	M_INLINE fp32 GetVOffset() const		{ return m_VOffset; }
	M_INLINE CVec3Dfp32 GetUVec() const		{ return m_UVec; }
	M_INLINE CVec3Dfp32 GetVVec() const		{ return m_VVec; }
	M_INLINE fp32 GetUVecLenSqrInv() const	{ return m_UVecLenSqrInv; }
	M_INLINE fp32 GetVVecLenSqrInv() const	{ return m_VVecLenSqrInv; }
	M_INLINE fp32 GetUVecLengthRcp() const	{ return m_UVecLenRcp; }
	M_INLINE fp32 GetVVecLengthRcp() const	{ return m_VVecLenRcp; }
	M_INLINE fp32 GetTxtWidthInv() const	{ return m_TxtWidthInv; }
	M_INLINE fp32 GetTxtHeightInv() const	{ return m_TxtWidthInv; }
	
	M_INLINE void ImproveProj(const fp32& _UProj, const fp32& _VProj)
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_MappingSetup::ImproveProj, XR_BSP4GLASS);
		m_TProjMax.k[0] = MaxMT(_UProj, m_TProjMax.k[0]);
		m_TProjMax.k[1] = MaxMT(_VProj, m_TProjMax.k[1]);
		m_TProjMin.k[0] = MinMT(_UProj, m_TProjMin.k[0]);
		m_TProjMin.k[1] = MinMT(_VProj, m_TProjMin.k[1]);
	}

	M_INLINE CVec2Dfp32 GetMid() const
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_MappingSetup::GetMid, XR_BSP4GLASS);
		return CVec2Dfp32(RoundToInt(((m_TProjMin.k[0] + m_TProjMax.k[0]) * 0.5f * m_TxtWidthInv) / 16.0f) * 16.0f,
            			 RoundToInt(((m_TProjMin.k[1] + m_TProjMax.k[1]) * 0.5f * m_TxtHeightInv) / 16.0f) * 16.0f);
	}
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_Wallmark
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CBSP4Glass_Wallmark : public CXR_WallmarkDesc
{
public:
	uint16 m_iV;
	uint16 m_nV;
	CVec3Dfp32 m_Pos;

	CBSP4Glass_Wallmark& operator = (const CXR_WallmarkDesc& _WM);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_WallmarkContext
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CBSP4Glass_WallmarkContext : public CReferenceCount
{
	typedef TQueue<CBSP4Glass_Wallmark>		CBSP4Glass_QWM;
	typedef TPtr<CBSP4Glass_QWM>			spCBSP4Glass_WMQueue;
	typedef TPtr<CBSP4Glass_LinkContext>	spCBSP4Glass_WMLink;
	
public:
	// Geometry data
	TThinArray<CVec3Dfp32>	m_lV;
	TThinArray<CVec3Dfp32>	m_lN;
	TThinArray<CVec2Dfp32>	m_lTV;
	TThinArray<CVec2Dfp32>	m_lTV2;
	TThinArray<CVec3Dfp32>	m_lTangU;
	TThinArray<CVec3Dfp32>	m_lTangV;
	TThinArray<CPixel32>	m_lCol;

	spCBSP4Glass_WMLink		m_spLink;
	spCBSP4Glass_WMQueue	m_spQWM;

	CXR_SurfaceContext*		m_pSC;

	int m_nV;
	int m_iVTail;
	int m_iVHead;
	
	CBSP4Glass_WallmarkContext();
	~CBSP4Glass_WallmarkContext();
	
	void Create(CXR_Model_BSP2* _pModel, int _nV);
	void Clear();
	
	void FreeWallmark();
	int  AddWallmark(const CBSP4Glass_Wallmark& _WM);
	int  AddVertices(int _nV);
	int  MaxPut() const;
};

typedef TPtr<CBSP4Glass_WallmarkContext> spCBSP4Glass_WallmarkContext;


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_Grid_CollisionInfo
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CBSP4Glass_Grid_CollisionInfo
{
public:
	bool	m_bValid;
	int32	m_iInside;
	int32	m_iHitX;
	int32	m_iHitY;
	
	CBSP4Glass_Grid_CollisionInfo()
		: m_bValid(false)
		, m_iInside(-1)
		, m_iHitX(-1)
		, m_iHitY(-1)
	{
	}

	CBSP4Glass_Grid_CollisionInfo(bool _bValid, int32 _iInside, int32 _iHitX, int32 _iHitY)
		: m_bValid(_bValid)
		, m_iInside(_iInside)
		, m_iHitX(_iHitX)
		, m_iHitY(_iHitY)
	{
	}

	CBSP4Glass_Grid_CollisionInfo& operator = (const CBSP4Glass_Grid_CollisionInfo& _From)
	{
		m_bValid = _From.m_bValid;
		m_iInside = _From.m_iInside;
		m_iHitX = _From.m_iHitX;
		m_iHitY = _From.m_iHitY;

		return *this;
	}

	M_INLINE bool IsValid() const	{ return (m_bValid && m_iInside >= 0); }
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_Grid_PhysInfo
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CBSP4Glass_Grid_PhysInfo
{
public:
	CVec2Dint32	m_Start;
	CVec2Dint32	m_End;

	CBSP4Glass_Grid_PhysInfo() {}

	M_INLINE bool IsValid(int32 _nX, int32 _nY) const { return (m_Start.k[0] < m_End.k[0] && m_Start.k[1] < m_End.k[1] && m_Start.k[0] < _nX && m_End.k[0] >= 0 && m_Start.k[1] < _nY && m_End.k[1] >= 0); }
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_Grid
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CBSP4Glass_Grid
{
private:
	typedef CBSP4Glass_Grid_CollisionInfo CInfo;
	class CGridData
	{
	private:
		TThinArray<uint8>	m_lBits;
		TAP<uint8>			m_pBits;
		uint32				m_nElem;
		uint8				m_nBits;

	public:
		CGridData();
		~CGridData();

		void   OnDeltaLoad(CCFile* _pFile);
		uint32 OnDeltaSave(CCFile* _pFile);

		void Clear();
		void SetLen(const uint32 _Len, const uint8 _nBitsPerElement);

		bool Get(const uint32 _iElem, const uint8 _iBit) const;
		uint Get01(const uint32 _iElem, const uint8 _iBit) const;
		uint8* GetArray();
		uint8 GetLastBit();

		void Set(const uint32 _iElem, const uint8 _iBit, const bool _bSet);
		bool Set(const uint32 _iElem, const bool _bSet);
		void Set0(const uint32 _iElem, const uint8 _iBit);
		bool Set0(const uint32 _iElem);
		void Set0All(const uint32 _iElem);
		void Set1(const uint32 _iElem, const uint8 _iBit);
		bool Set1(const uint32 _iElem);
		void Set1All(const uint32 _iElem);
		void SetAll(const bool _bSet);
		void SetAll(const uint8 _iBit, const bool _bSet);
		void SetAllRange(const bool _bSet, const uint32 _Start, const uint32 _Count);

		void operator = (const CGridData& _GridData);
	};

	uint8		m_bCreated : 1;
	uint8		m_bFullBreak : 1;
	uint8		m_bTimeBreak : 1;
	uint8		m_iSound : 2;
	uint32		m_Seed;
	uint32		m_nQuadsX;
	uint32		m_nQuadsY;

	CVec3Dfp32	m_FrontTangU;
	CVec3Dfp32	m_FrontTangV;
	CVec3Dfp32	m_BackTangU;
	CVec3Dfp32	m_BackTangV;

	CVec3Dfp32	m_PlaneN;
	fp32		m_PlaneD;

	TThinArray<CVec3Dfp32>	m_lGridPoints;
	CGridData				m_GridData;
	TArray<uint32>			m_liRefresh;
	TThinArray<int32>		m_lActiveTime;

	aint					m_ShardModel;

public:
	CBSP4Glass_Grid();

	uint32		OnDeltaSave(CCFile* _pFile);
	void		OnDeltaLoad(CCFile* _pFile);

	// Grid functions
	void		CleanGrid(const bool _bKeepCreated);
	void		CreateGrid(const CGlassAttrib& _Attrib, const CVec3Dfp32* _pV, const uint32 _nV, const CMat4Dfp32& _WMat, aint _ShardModel, uint8 _CrushFlags);
	void		InvalidateGrid(const CBSP4Glass_Grid_PhysInfo& _PhysInfo, int32 _GameTick, CVec3Dfp32* _pSrcPos, const CMat4Dfp32& _WMat);
	void		InvalidateGrid(const CBSP4Glass_Grid_CollisionInfo& _CInfo, int32 _GameTick, CVec3Dfp32* _pSrcPos, const CMat4Dfp32& _WMat);
	void		InvalidateGrid(uint32 _nGrids, uint32* _pGrids, int32 _GameTick, CVec3Dfp32* _pSrcPos, const CMat4Dfp32& _WMat);
	
	void		Phys_SetupIntersectGrid(const CVec3Dfp32& _LocalPos, const fp32* _pRadius, const CMat4Dfp32& _WMat, CBSP4Glass_Grid_PhysInfo& _PhysInfo);
	CInfo		Phys_LineIntersectGrid(const CVec3Dfp32& _Position, const CMat4Dfp32& _WMat);
	CInfo		Phys_SphereIntersectGrid(const CPlane3Dfp32& _Plane, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, const fp32 _Radius, CCollisionInfo* _pCollisionInfo, const CMat4Dfp32& _WMat);
	CInfo		Phys_BoxIntersectGrid(const CXR_PhysicsContext* _pPhysContext, const CPlane3Dfp32& _PolyPlane, const CPhysOBB& _BoxStart, const CPhysOBB& _Box, CCollisionInfo* _pCollisionInfo);

	uint32		Phys_EnumBox(const CPhysOBB& _Box, const CMat4Dfp32& _WMat, TThinArray<uint32>& _lReturn);
	uint32		Phys_EnumSphere(const CVec3Dfp32& _LocalPos, fp32 _Radius, const CMat4Dfp32& _WMat, TThinArray<uint32>& _lReturn);

	// Check
	bool		IsValid();
	
	M_INLINE bool  IsFullBreak()	{ return (m_bFullBreak != 0); }
	M_INLINE bool  IsTimeBreak()	{ return (m_bTimeBreak != 0); }
	M_INLINE int32 GetNumRefresh()	{ return m_liRefresh.Len(); }

	void		SetFrontTang(const CVec3Dfp32& _TangU, const CVec3Dfp32& _TangV);
	void		SetBackTang(const CVec3Dfp32& _TangU, const CVec3Dfp32& _TangV);

	// Refresh
	int32		Refresh(CVec4Dfp32* _pVel, const int32 _GameTick, CVec3Dfp32* _pSrcPos, const CMat4Dfp32& _WMat);
	void		CalcNewVelocity(CVec4Dfp32* _pVel, int32 _iXY);
	void		SetNewVelocity(TAP_RCD<CVec4Dfp32>& _lVelocity, const int32 _iHitIndex, const CVec3Dfp32& _Force);
	void		SetNewRefreshVelocity(TAP_RCD<CVec4Dfp32>& _lVelocity, const int32 _nRefresh, const CVec3Dfp32& _Force);
	
	// Get functionality
	uint32		GetQuadsX();
	uint32		GetQuadsY();
	uint32		GetSeed();
	CVec3Dfp32	GetPlaneN();
	CVec3Dfp32	GetFrontTangU();
	CVec3Dfp32	GetFrontTangV();
	CVec3Dfp32	GetBackTangU();
	CVec3Dfp32	GetBackTangV();

	M_INLINE const TThinArray<CVec3Dfp32>& GetGridPoints() { return m_lGridPoints; }
	
	M_INLINE TThinArray<int32>& GetActiveTime() { return m_lActiveTime; }
	M_INLINE aint GetShardModel() { return m_ShardModel; }
	M_INLINE void SetNextSound() { m_iSound += (m_iSound < 2) ? 1 : 0; }
	M_INLINE int8 GetSound() const { return m_iSound; }
	M_INLINE void ResetSound() { m_iSound = 0; }

	void operator = (const CBSP4Glass_Grid& _Grid);

private:
	void	CreateGridMap(const CGlassAttrib& _Attrib, const CVec3Dfp32& _Origin, CVec3Dfp32 _Vec0, CVec3Dfp32 _Vec1, const CMat4Dfp32& _WMat, uint8 _CrushFlags);

	M_INLINE uint32 MakeEntry(uint32 _x, uint32 _y)
	{
		return (((_y & 0xFFFF) << 16) | (_x & 0xFFFF));
	}

	M_INLINE uint32 GetEntryX(uint32 _XYEntry)
	{
		return (_XYEntry & 0xFFFF);
	}

	M_INLINE uint32 GetEntryY(uint32 _XYEntry)
	{
		return ((_XYEntry >> 16) & 0xFFFF);
	}

	// Debugging
	#ifndef M_RTM
	public:
		void	Debug_RenderGrid(CWireContainer* _pWC, CPixel32 _Color, const CMat4Dfp32& _WMat);
		void	Debug_DumpGrid();
	#endif
};


#endif

