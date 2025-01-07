#ifndef _INC_XW_SURF
#define _INC_XW_SURF

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			CXW_Surface (XR-Component, should be named CXR_Surface)
					
	Author:			Magnus Högdahl
					
	Copyright:		Starbreeze Studios AB 1997, 2000, 2002
					
	Contents:		
					
	Comments:		
					
	History:		
		990201:		Created File
\*____________________________________________________________________________________________*/


#include "../MOS.h"
#include "XRMediumDesc.h"
#include "../../MCC/MMath_fp2.h"

#define XW_SURFACE_VERSION				0x0202
#define XW_SURFLAYER_VERSION			0x0105
#define XW_MEDIUM_VERSION				0x0103

#define XW_SURFSEQUENCE_VERSION			0x0100
#define XW_SURFKEYFRAME_VERSION			0x0103

#define XW_SURFACE_MAXTEXNAMELEN		24

#ifdef PLATFORM_CONSOLE
	#define XW_SURF_THIN
#endif


//#define XW_FOGMASK_ENABLE


// -------------------------------------------------------------------
// #define XW_LAYERFLAGS_MAPPINGSCALE		1	// OBSOLOTE!
// Indicates that (offset != 0) or/and (scale != 1)

enum
{
	XW_LAYERTYPE_RENDERSURFACE		= 0,
	XW_LAYERTYPE_DIFFUSE			= 1,
	XW_LAYERTYPE_DIFFUSE2			= 2,
	XW_LAYERTYPE_SPECULAR			= 3,
	XW_LAYERTYPE_NORMAL				= 4,
	XW_LAYERTYPE_HEIGHT				= 5,
	XW_LAYERTYPE_TRANSMISSION		= 6,
	XW_LAYERTYPE_ENVIRONMENT		= 7,
	XW_LAYERTYPE_ATTRIBUTE			= 8,
	XW_LAYERTYPE_ANISOTROPICDIR		= 9,
};

enum
{
	XW_LAYERFLAGS_LIGHTING			= DBit(1),
	XW_LAYERFLAGS_ALPHACOMPARE		= DBit(2),
	XW_LAYERFLAGS_NOZWRITE			= DBit(3),
	XW_LAYERFLAGS_NOCOLORWRITE		= DBit(4),
	XW_LAYERFLAGS_NOALPHAWRITE		= DBit(5),
	XW_LAYERFLAGS_MAPPINGREFERENCE	= DBit(6),	// This layer's texture is the reference for scale-based texture mapping.
	XW_LAYERFLAGS_CUSTOMTEXENV		= DBit(7),	// This group's texture environment should not handled by Render_Surface.

	XW_LAYERFLAGS_TRANSPARENT		= DBit(13),
	XW_LAYERFLAGS_GROUP				= DBit(14),
	XW_LAYERFLAGS_INVISIBLE			= DBit(15),

	XW_LAYERFLAGS_NOFOG				= DBit(16),	// Disable any and all fogging of a layer
	XW_LAYERFLAGS_FOGBLACK			= DBit(17),	// If layer is fogged, it should be fogged as if the fogcolor was black regardless of the rastermode.
	XW_LAYERFLAGS_FOGCOLOR			= DBit(18),	// If layer is fogged, it should be fogged with the fogcolor regardless of the rastermode.
	XW_LAYERFLAGS_FOGALPHA			= DBit(19),	// If layer is fogged, the alpha channel should be fogged as if the fogcolor was black. (NOT SUPPORTED)
	XW_LAYERFLAGS_USERCOLOR			= DBit(20),	
};

// -------------------------------------------------------------------
enum
{
	XW_SURFFLAGS_LIGHTING			= DBit(0),
	XW_SURFFLAGS_TRANSPARENT		= DBit(1),
	XW_SURFFLAGS_INVISIBLE			= DBit(2),
	XW_SURFFLAGS_REFLECTION			= DBit(3),
	XW_SURFFLAGS_DELETEAFTERBSP		= DBit(4),
	XW_SURFFLAGS_NEEDDIFFUSE		= DBit(5),
	XW_SURFFLAGS_NEEDSPECULAR		= DBit(6),
	XW_SURFFLAGS_NEEDNORMALS		= DBit(7),
	XW_SURFFLAGS_NOCULL				= DBit(8),
	XW_SURFFLAGS_SKYVIS				= DBit(9),
	XW_SURFFLAGS_NODYNAMICLIGHT		= DBit(10),
	XW_SURFFLAGS_NOWALLMARK			= DBit(11),
	XW_SURFFLAGS_NOLIGHTMAP			= DBit(12),
	XW_SURFFLAGS_HASNEXTSURF		= DBit(13),	// This flag is to be used by surface-IO only!
	XW_SURFFLAGS_HASKEYS			= DBit(14),	// This flag is to be used by surface-IO only!
	XW_SURFFLAGS_POLYGONOFFSET		= DBit(15),	// This flag is to be used by surface-IO only!
	XW_SURFFLAGS_NEEDREFLECTIONMAP	= DBit(16),
	XW_SURFFLAGS_NEEDREFRACTIONMAP	= DBit(17),
	XW_SURFFLAGS_NOFOG				= DBit(18),
	XW_SURFFLAGS_SUBDIVISION		= DBit(19),
	XW_SURFFLAGS_NEEDTANGENTS		= DBit(20),
	XW_SURFFLAGS_HASBUMPMAP			= DBit(21),
	XW_SURFFLAGS_GENERATED			= DBit(22),
	XW_SURFFLAGS_KEEPALLVERSIONS	= DBit(23),
	XW_SURFFLAGS_OGRINVISIBLE		= DBit(24),
	XW_SURFFLAGS_SHADER				= DBit(25),	// Surface has shader parameter layers
	XW_SURFFLAGS_SHADERONLY			= DBit(26),	// Surface has only shader parameter layers
};

enum
{
	XW_SURFTEX_SPECIALBASE		= 0xf000,		// This is _not_ a mask, but a base. All texture-ids above or
												// equal to XW_SURFTEX_SPECIALBASE should be treated at special.
	XW_SURFTEX_REFLECTIONMAP	= 0xf000,
	XW_SURFTEX_REFRACTIONMAP	= 0xf001,
	XW_SURFTEX_LIGHTMAP			= 0xf002,

	XW_SURFTEX_TEXTUREMAP0		= 0xf010,
	XW_SURFTEX_TEXTUREMAP1		= 0xf011,
	XW_SURFTEX_TEXTUREMAP2		= 0xf012,
	XW_SURFTEX_TEXTUREMAP3		= 0xf013,
};

enum
{
	XW_SURFOPTION_SIMPLE0	= DBit(0),
	XW_SURFOPTION_SIMPLE1	= DBit(1),
	XW_SURFOPTION_SIMPLE2	= DBit(2),
	XW_SURFOPTION_SIMPLE3	= DBit(3),
	XW_SURFOPTION_HQ0		= DBit(4),
	XW_SURFOPTION_HQ1		= DBit(5),
	XW_SURFOPTION_HQ2		= DBit(6),
	XW_SURFOPTION_HQ3		= DBit(7),

	XW_SURFOPTION_LIGHTMAP	= XW_SURFOPTION_SIMPLE2,
	XW_SURFOPTION_NOGORE	= XW_SURFOPTION_SIMPLE3,
	XW_SURFOPTION_SHADER	= XW_SURFOPTION_HQ3,
};

enum
{
	XW_SURFREQ_MULTITEX2	= DBit(0),
	XW_SURFREQ_MULTITEX3	= DBit(1),
	XW_SURFREQ_MULTITEX4	= DBit(2),
	XW_SURFREQ_MULTITEX8	= DBit(3),
	XW_SURFREQ_CUBEMAP		= DBit(4),
	XW_SURFREQ_NV20			= DBit(5),
	XW_SURFREQ_NV10			= DBit(6),
	XW_SURFREQ_GENERATED	= DBit(7),
	XW_SURFREQ_GAMECUBE		= DBit(8),
	XW_SURFREQ_TEXENVMODE2  = DBit(9),
	XW_SURFREQ_OPERATOR_NA  = DBit(10),
};

// -------------------------------------------------------------------
//  VB operator enums
// -------------------------------------------------------------------

enum
{
	XW_LAYEROPCODE_OPERATOR			= 0,
	XW_LAYEROPCODE_GETLASTLAYER		= 1,
};

enum
{
	XW_LAYEROP_X					= DBit(0),
	XW_LAYEROP_Y					= DBit(1),
	XW_LAYEROP_Z					= DBit(2),

	XW_LAYEROP_NX					= DBit(3),
	XW_LAYEROP_NY					= DBit(4),
	XW_LAYEROP_NZ					= DBit(5),

	XW_LAYEROP_U					= DBit(6),
	XW_LAYEROP_V					= DBit(7),
	XW_LAYEROP_W					= DBit(8),

	XW_LAYEROP_B					= DBit(9),		// Memory layout order. blue byte comes first.
	XW_LAYEROP_G					= DBit(10),
	XW_LAYEROP_R					= DBit(11),
	XW_LAYEROP_A					= DBit(12),

	XW_LAYEROP_ATTRIB				= DBit(13),

/*	XW_LAYEROP_SB					= 8192,
	XW_LAYEROP_SG					= 16384,
	XW_LAYEROP_SR					= 32768,*/

	// Combined masks
	XW_LAYEROP_VERTEX				= XW_LAYEROP_X + XW_LAYEROP_Y + XW_LAYEROP_Z,
	XW_LAYEROP_TVERTEX				= XW_LAYEROP_U + XW_LAYEROP_V + XW_LAYEROP_W,
	XW_LAYEROP_NORMAL				= XW_LAYEROP_NX + XW_LAYEROP_NY + XW_LAYEROP_NZ,
	XW_LAYEROP_COLOR				= XW_LAYEROP_R + XW_LAYEROP_G + XW_LAYEROP_B + XW_LAYEROP_A,
//	XW_LAYEROP_SPECULAR				= XW_LAYEROP_SR + XW_LAYEROP_SG + XW_LAYEROP_SB,
};

class CXW_LayerOperation						// 24 bytes (32 inc memory mgr.)
{
public:
	uint8 m_OpCode;
	uint8 m_iOperatorClass;						// Evaluated at load-time.
	uint16 m_Components;
	char m_OperatorClass[16];					// Layer-operation class names can at most be 15 chars long.

	TThinArray<fp4> m_lParams;

	CXW_LayerOperation();

	void SetClass(const char*);
	const char* GetClass() const;

	bool ParseKey(CStr _Value);

	void Read(class CCFile* _pFile);
	void Write(class CCFile* _pFile) const;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXW_LayerOperList
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXW_LayerOperList : public TThinArray<CXW_LayerOperation>
{
public:
	uint32 m_RealLen;

	CXW_LayerOperList()
	{
		m_RealLen = 0;
	}

	CXW_LayerOperList(const TThinArray<CXW_LayerOperation>& _Array)
	{
		m_pArray = NULL;
		TThinArray<CXW_LayerOperation>::operator= (_Array);
	}

	const TThinArray<CXW_LayerOperation>& operator=(const TThinArray<CXW_LayerOperation>& _Array)
	{
		if (Len() != _Array.Len())
		{
			Clear();
			SetLen(_Array.Len());
		}

		CopyElements(GetBasePtr(), _Array.GetBasePtr(), Len());

		return *this;
	}

	void SetLen(int _Len)
	{
		TThinArray<CXW_LayerOperation>::SetLen(_Len);
		m_RealLen = _Len;
	}

	void QuickSetLen(int _Len)
	{
		if (_Len > TThinArray<CXW_LayerOperation>::Len())
			TThinArray<CXW_LayerOperation>::SetLen(_Len);
		m_RealLen = _Len;
	}

	int Len() const
	{
		return m_RealLen;
	}

	int ListSize() const
	{
		return m_RealLen * sizeof(CXW_LayerOperation);
	}
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXW_SurfaceLayer
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXW_SurfaceLayer					// 28
{
public:
	uint32 m_Flags : 28;				// 4
	uint32 m_Type : 4;

	uint32 m_RasterMode : 4;			// 4
	uint32 m_TexCoordNr : 4;
	uint32 m_TexChannel : 4;
	uint32 m_TexEnvMode : 4;
	uint32 m_ZFunc : 4;
	uint32 m_AlphaFunc : 4;
	uint32 m_AlphaRef : 8;

	CVec4Dfp2 m_HDRColor;				// 8

	union
	{
		uint8 m_SpecularFresnelBias;	// 1
		uint8 m_iUserColor;
	};
	uint8 m_SpecularFresnelScale;		// 1
	uint16 m_TextureID;					// 2

#ifdef USE_HASHED_TEXTURENAME
	uint32 m_TextureNameID;				// 4
#else	
	char m_TextureName[XW_SURFACE_MAXTEXNAMELEN];
#endif	

	CXW_LayerOperList m_lOper;			// 8


	const char* GetTextureName() const { return m_TextureName; };
	void SetTextureName(const char* _pName);

	CXW_SurfaceLayer();
	void Init();

	void Interpolate(const CXW_SurfaceLayer& _Frm, fp4 _t, CXW_SurfaceLayer& _Dst) const;

	void ParseContainer(CKeyContainer& _Keys);

	void Read(class CCFile* _pFile);
	void Write(class CCFile* _pFile) const;
};

/*class CXW_SurfaceLayer
{
public:
#ifdef USE_HASHED_TEXTURENAME
	uint32 m_TextureNameID;
#else	
	char m_TextureName[XW_SURFACE_MAXTEXNAMELEN];
#endif	
	uint32 m_Flags;
	uint8 m_Mapping;
	uint8 m_RasterMode;
	uint32 m_Color;
	uint8 m_TexCoordNr;
	uint8 m_TexChannel;
	uint8 m_TexEnvMode;
	uint8 m_ZFunc;
	uint8 m_AlphaFunc;
	uint8 m_AlphaRef;
	
	int m_TextureID;

	CXW_LayerOperList m_lOper;

#ifndef USE_HASHED_TEXTURENAME
	const char* GetTextureName() const { return m_TextureName; };
#endif

	void SetTextureName(const char* _pName);

	CXW_SurfaceLayer();

	void Init();

	void Interpolate(const CXW_SurfaceLayer& _Frm, fp4 _t, CXW_SurfaceLayer& _Dst) const;

	void ParseContainer(CKeyContainer& _Keys);

	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile) const;
};*/

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXW_LayerList
|__________________________________________________________________________________________________
\*************************************************************************************************/
//#ifdef XW_SURF_THIN

class CXW_SurfaceLayerList : public TThinArray<CXW_SurfaceLayer>
{
public:
	uint32 m_RealLen;

	CXW_SurfaceLayerList()
	{
		m_RealLen = 0;
	}

	CXW_SurfaceLayerList(const TThinArray<CXW_SurfaceLayer>& _Array)
	{
		m_pArray = NULL;
		TThinArray<CXW_SurfaceLayer>::operator= (_Array);
	}

	const TThinArray<CXW_SurfaceLayer>& operator=(const TThinArray<CXW_SurfaceLayer>& _Array)
	{
		if (Len() != _Array.Len())
		{
			Clear();
			SetLen(_Array.Len());
		}

		CopyElements(GetBasePtr(), _Array.GetBasePtr(), Len());

		return *this;
	}

	void SetLen(int _Len)
	{
		TThinArray<CXW_SurfaceLayer>::SetLen(_Len);
		m_RealLen = _Len;
	}

	void QuickSetLen(int _Len)
	{
		if (_Len > TThinArray<CXW_SurfaceLayer>::Len())
			TThinArray<CXW_SurfaceLayer>::SetLen(_Len);
		m_RealLen = _Len;
	}

	int Len() const
	{
		return m_RealLen;
	}

	int ListSize() const
	{
		return m_RealLen * sizeof(CXW_SurfaceLayer);
	}
};

//#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXW_SurfaceKeyFrame
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXW_SurfaceKeyFrame;
typedef TPtr<CXW_SurfaceKeyFrame> spCXW_SurfaceKeyFrame;

class CXW_SurfaceKeyFrame : public CReferenceCount
{
	MRTC_DECLARE;

public:
	
	DECLARE_OPERATOR_NEW
	

#ifdef XW_SURF_THIN
	CXW_SurfaceLayerList m_lTextures;
#else
	TList_Vector<CXW_SurfaceLayer> m_lTextures;
	CXW_SurfaceLayerList m_lTexturesThin;
#endif

	fp4 m_Duration;
	fp4 m_DurationRecp;

#ifdef XW_FOGMASK_ENABLE
	CStr m_FogMaskName;
	uint16 m_FogMaskTextureID;
#endif	

	fp4 m_PolygonOffsetScale;
	fp4 m_PolygonOffsetUnits;

	fp4 m_Friction;
	fp4 m_Elasticy;
	int32 m_MaterialType;			// i.e, is it metal, wood, etc..?
	int32 m_User[4];
	fp4 m_fUser[4];
	CXR_MediumDesc m_Medium;

	// Temporary, used for rendering
	CMTime m_AnimTime;
	CMTime m_AnimTimeWrapped;

	// -------------------------------------------------------------------
	CXW_SurfaceKeyFrame();
	virtual void operator=(const CXW_SurfaceKeyFrame&);
	virtual spCXW_SurfaceKeyFrame Duplicate() const;

	virtual class CXW_SurfaceLayer* GetBumpMap();

	virtual void Interpolate(const CXW_SurfaceKeyFrame& _pFrm, fp4 _t, CXW_SurfaceKeyFrame& _pDst) const;

	virtual void ParseContainer(CKeyContainer& _Keys);
	virtual void ParseNode(class CKeyContainerNode* _pNode);

	virtual void InitTextures(class CTextureContext* pTC, bool _bReportFailure);
//	virtual void SetTextureFlags(class CTextureContext* pTC, int _Flags);
	virtual void SetTextureParam(class CTextureContext* pTC, int _Param, int _Value);
	virtual void SetTextureParamfv(class CTextureContext* pTC, int _Param, const fp4* _pValues);

#ifndef PLATFORM_CONSOLE
	virtual bool IsTransparent(class CTextureContext* _pTC, CVec2Dfp4 _uv);
#endif

	virtual void Read(class CCFile* _pFile);
	virtual void Write(class CCFile* _pFile) const;

	//Hack to fix parsing error
	void MulSpecPower();
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXW_SurfaceSequence
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXW_SurfaceSequence;
typedef TPtr<CXW_SurfaceSequence> spCXW_SurfaceSequence;

class CXW_Surface;

class CXW_SurfaceSequence : public CReferenceCount
{
	MRTC_DECLARE;

public:
	
	DECLARE_OPERATOR_NEW
	

#ifdef XW_SURF_THIN
	TThinArray<CXW_SurfaceKeyFrame> m_lKeyFrames;
#else
	TList_Vector<spCXW_SurfaceKeyFrame> m_lspKeyFrames;
	TThinArray<CXW_SurfaceKeyFrame> m_lKeyFramesThin;
#endif
	int GetNumKeys() const;
	class CXW_SurfaceKeyFrame* GetKey(int _iKey);
	const class CXW_SurfaceKeyFrame* GetKey(int _iKey) const;

	int32 m_iRepSFrame;
	int32 m_iRepEFrame;
	fp4 m_RepSTime;
	fp4 m_RepETime;
	fp4 m_TotTime;

	// -------------------------------------------------------------------
	CXW_SurfaceSequence();
	virtual spCXW_SurfaceSequence Duplicate() const;

	virtual void GetFrameAndTimeFraction(const CMTime& _Time, int& _Frame, fp4& _tFrac, CMTime& _tWrapped) const;
	virtual class CXW_SurfaceKeyFrame* GetBaseFrame(class CXW_Surface* _pSurf);
	virtual const class CXW_SurfaceKeyFrame* GetBaseFrame(const class CXW_Surface* _pSurf) const;
	virtual class CXW_SurfaceKeyFrame* GetFrame(int _iFrm);
	virtual class CXW_SurfaceKeyFrame* GetFrame(const CMTime& _Time, CXW_SurfaceKeyFrame& _TmpStorage);

	virtual void SetNumKeyFrames(int _nKeys);
	static void DeleteExcessSequences(class CKeyContainer* _pKC);
	static void DeleteExcessSequences_r(class CKeyContainerNode* _pNode);
	static void DeleteExcessFrames(class CKeyContainer* _pKC);
	static void DeleteExcessFrames_r(class CKeyContainerNode* _pNode);
	static void DeleteFrame(class CKeyContainer* _pKC);
	static void DeleteFrame_r(class CKeyContainerNode* _pNode);

	virtual void ParseContainer(CKeyContainer& _Keys);
	virtual void ParseNode(class CKeyContainerNode* _pNode);

	virtual void Initialize();
	virtual void InitTextures(class CTextureContext* pTC, bool _bReportFailure);
//	virtual void SetTextureFlags(class CTextureContext* pTC, int _Flags);
	virtual void SetTextureParam(class CTextureContext* pTC, int _Param, int _Value);
	virtual void SetTextureParamfv(class CTextureContext* pTC, int _Param, const fp4* _pValues);

	virtual void Read(class CCFile* _pFile);
	virtual void Write(class CCFile* _pFile) const;
};

// -------------------------------------------------------------------
//  CThinKeyContainer
// -------------------------------------------------------------------
class CThinKeyContainer
{
	struct Key
	{
		uint16 m_iName;
		uint16 m_iValue;
	};

	TThinArray<uint8> m_StrData;
	TThinArray<Key>   m_lKeys;

	bool HasOneSingleKey() const { return (m_StrData.Len() && !m_lKeys.Len()); }

public:
	CThinKeyContainer();
	~CThinKeyContainer();

	void Create(const TList_Vector<CStr>& _lKeyNames, const TList_Vector<CStr>& _lKeyValues);

	void operator= (const CThinKeyContainer& _KC);

	int AddKey(const CStr _KeyName, const CStr _KeyValue);
	void SetKeyValue(int _iIndex, CStr _Value);
	void DeleteKey(const CStr _KeyName);
	void DeleteKey(int _iIndex);

	int GetnKeys() const;
	int GetKeyIndex(CStr _Key) const;
	const char* GetKeyName(int _iKey) const;
	const char* GetKeyValue(int _iKey) const;

	void Read(class CCFile* _pFile);

	const TThinArray<uint8>& GetStrData() const { return m_StrData; }
	const TThinArray<Key>&   GetlKeys() const   { return m_lKeys; }
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXW_Surface
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXW_Surface;
typedef TPtr<CXW_Surface> spCXW_Surface;

class CXW_Surface : public CReferenceCount
{
	MRTC_DECLARE;

public:
	
	DECLARE_OPERATOR_NEW
	

	int32 m_Flags;
	CStr m_Name;
#ifndef XW_SURF_THIN
	CStr m_NodeName;
#endif
	uint32 m_iController : 1;					// Which timer & sequence controller to use.
	uint32 m_iGroup : 2;						// Which surface group it belongs to.
	uint32 __Pad__ : 29;

	int32 m_OcclusionMask;
	fp4 m_PriorityOffset;
	int16 m_Options;
	int16 m_Requirements;
#ifndef XW_SURF_THIN
	int32 m_SmoothGroup;
	fp4 m_SmoothAngle;
#endif

	TList_Vector<spCXW_SurfaceSequence> m_lspSequences;

	spCXW_Surface m_spNextSurf;					// Next surface in a multisurface chain

#ifdef XW_SURF_THIN
	CThinKeyContainer m_Keys;
#else
	spCKeyContainer m_spKeys;
	CThinKeyContainer m_KeysThin;
#endif

	// -------------------------------------------------------------------
	CXW_Surface();

//	void operator= (const CXW_Surface& _Surf);
	virtual void Inherit(const CXW_Surface& _Surf);
	virtual spCXW_Surface Duplicate() const;

	virtual class CXW_Surface* GetSurface(int _Options, int _Caps);
#ifndef XW_SURF_THIN
	void AddSurfaceVersion(spCXW_Surface _spSurf);
#endif

	virtual class CXW_SurfaceKeyFrame* GetBaseFrame();
	virtual const class CXW_SurfaceKeyFrame* GetBaseFrame() const;
	virtual class CXW_SurfaceKeyFrame* GetFrame(int _iSeq, const CMTime& Time, CXW_SurfaceKeyFrame& _TmpStorage);

	virtual void Read(class CCFile* _pFile);
	virtual void Write(class CCFile* _pFile);
	virtual void Init();
	virtual void InitTextures(bool _bReportFailure = true);
	virtual void InitTextures(class CTextureContext* _pTC, bool _bReportFailure = true);
	virtual CVec3Dfp4 GetTextureMappingReferenceDimensions();

//	virtual void SetTextureFlags(int _Flags);
	virtual void SetTextureParam(int _Param, int _Value);
	virtual void SetTextureParamfv(int _Param, const fp4* _pValues);

	static bool IsAnimated(CKeyContainer& _Keys);
	static int GetNumSequences(CKeyContainer& _Keys);
	static int GetNumSequences_r(class CKeyContainerNode* _pNode);
	static int GetNumKeyFrames(CKeyContainer& _Keys);
	static int GetNumKeyFrames_r(class CKeyContainerNode* _pNode);
	static bool IsSurface(class CKeyContainerNode* _pNode);
	static bool IsAnimated(class CKeyContainerNode* _pNode);
	void SetNumSequences(int _nSeq);
	static void DeleteSequence(class CKeyContainer* _pKC);
	static void DeleteSequence_r(class CKeyContainerNode* _pNode);
	static bool IsSurfaceChild(class CKeyContainerNode* _pNode);
	static void DeleteNoSurfaceChildren(class CKeyContainerNode* _pNode);

	virtual void ParseContainer(CKeyContainer& _Keys);
	static void ParseNode_r(class CKeyContainerNode* _pNode, const CXW_Surface& _Surf, TList_Vector<spCXW_Surface>& _lspSurfaces);

	static TList_Vector<spCXW_Surface> CombineSurfaces(TList_Vector<spCXW_Surface> _lspSurf);
	static void AddTextureName(class CXW_Surface* _pS, CStr _dName);
	static TList_Vector<spCXW_Surface> ExpandLODSurfaces(TList_Vector<spCXW_Surface> _lspSurf);

	static TList_Vector<spCXW_Surface> ReadScript(CStr _Filename);
	static TList_Vector<spCXW_Surface> ReadScript(TArray<uint8> _lData);

	static void Write(class CCFile* _pFile, TList_Vector<spCXW_Surface>& _lspSurfaces);
	static void Read(class CCFile* _pFile, TList_Vector<spCXW_Surface>& _lspSurfaces, int _nSurf);
	static void Read(class CCFile* _pFile, TThinArray<spCXW_Surface>& _lspSurfaces, int _nSurf);

	static int GetSurfaceIndex(TList_Vector<spCXW_Surface>, const char* _pSurfName);

	virtual spCXW_Surface CreateOptimizedSurfaces();
#ifndef USE_HASHED_TEXTURENAME
	spCXW_Surface CreateShaderFallbackSurface();
#endif
};


#endif // _INC_XW_SURF
