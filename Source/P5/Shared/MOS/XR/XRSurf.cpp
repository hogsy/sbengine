#include "PCH.h"

#include "XRSurf.h"
#include "XRSurfaceContext.h"
//#include "XRShader.h"

#include "../MOS.h"
#include "../MSystem/Raster/MTextureContainers.h"
#include "MFloat.h"


	MRTC_IMPLEMENT_DYNAMIC(CXW_SurfaceKeyFrame, CReferenceCount);
	IMPLEMENT_OPERATOR_NEW(CXW_SurfaceKeyFrame);



	MRTC_IMPLEMENT_DYNAMIC(CXW_SurfaceSequence, CReferenceCount);
	IMPLEMENT_OPERATOR_NEW(CXW_SurfaceSequence);



	MRTC_IMPLEMENT_DYNAMIC(CXW_Surface, CReferenceCount);
	IMPLEMENT_OPERATOR_NEW(CXW_Surface);


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXW_LayerOperation
|__________________________________________________________________________________________________
\*************************************************************************************************/
CXW_LayerOperation::CXW_LayerOperation()
{
	MAUTOSTRIP(CXW_LayerOperation_ctor, MAUTOSTRIP_VOID);
	m_OpCode = XW_LAYEROPCODE_OPERATOR;
	m_iOperatorClass = 0;
	m_Components = 0;
	m_OperatorClass[0] = 0;
}

void CXW_LayerOperation::SetClass(const char* _pStr)
{
	MAUTOSTRIP(CXW_LayerOperation_SetClass, MAUTOSTRIP_VOID);
	int Len = MinMT( int(sizeof(m_OperatorClass))-1, CStr::StrLen(_pStr) );
	memcpy(m_OperatorClass, _pStr, Len);
	m_OperatorClass[Len] = 0;
}

const char* CXW_LayerOperation::GetClass() const
{
	MAUTOSTRIP(CXW_LayerOperation_GetClass, NULL);
	return (const char*)m_OperatorClass;
}


bool CXW_LayerOperation::ParseKey(CStr _Value)
{
	MAUTOSTRIP(CXW_LayerOperation_ParseKey, false);
	const char* g_ComponentMasks[] = 
	{
		"x", "y", "z", 
		"nx", "ny", "nz", 
		"u", "v", "w", 
		"b", "g", "r", "a",
		"attrib",
//	"sb", "sg", "sr",
		(char*)NULL
	};


	// "command" "gen_tvertex wave, u+v, amp=12, period=
	// "command" "gen_env uv"
	// "command" "rotate uv, 2.0, 0.5, 0.5"
	// "command" "wave uv, 16, 64, 2"
	// "command" "rotate u+v, 2.0, 0.5, 0.5"
	// "command" "rotate u+v, 2.0, 0.5, 0.5"
	// "command" "rotate nx+ny+nz, 2.0, 0.5, 0.5"

	// sr,sg,sb,sa
	// nx,ny,nz
	// r,g,b,a
	// x,y,z
	// u,v,w

	CFStr Val(_Value);
	Val.Trim();

	// Parse class
	CFStr Class = Val.GetStrMSep(" ,");
	Class.Trim();
	SetClass(Class);

	// Parse component mask
	m_Components = 0;
	CFStr Components = Val.GetStrMSep(" ,");
	Components.Trim();
	while(Components != "")
	{
		CFStr Comp = Components.GetStrMSep("_+");
		Comp.Trim();
		m_Components |= Comp.TranslateFlags(g_ComponentMasks);
	}

	// Parse params
	int nP = 0;
	fp4 Params[256];

	Val.Trim();
	while(Val != "")
	{
		if (nP >= 256) Error_static("ParseKey", "Too many parameters.");
		fp4 v = (Val.GetStrMSep(" ,")).Val_fp8();
		Params[nP++] = v;
		Val.Trim();
	}

	m_lParams.SetLen(nP);
	memcpy(m_lParams.GetBasePtr(), Params, sizeof(fp4) * nP);

	return true;
}

void CXW_LayerOperation::Read(CCFile* _pFile)
{
	MAUTOSTRIP(CXW_LayerOperation_Read, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXW_LayerOperation::Read);
	_pFile->Read(&m_OpCode, 4 + sizeof(m_OperatorClass));
	SwapLE(m_OpCode);
	SwapLE(m_iOperatorClass);
	SwapLE(m_Components);

/*	_pFile->ReadLE(m_OpCode);
	_pFile->ReadLE(m_iOperatorClass);
	_pFile->ReadLE(m_Components);
	_pFile->Read(m_OperatorClass, sizeof(m_OperatorClass));*/

	uint32 Len = 0;
	_pFile->ReadLE(Len);
	m_lParams.SetLen(Len);
	for(int i = 0; i < Len; i++) _pFile->ReadLE(m_lParams[i]);
}

void CXW_LayerOperation::Write(CCFile* _pFile)  const
{
	MAUTOSTRIP(CXW_LayerOperation_Write, MAUTOSTRIP_VOID);
	_pFile->WriteLE(m_OpCode);
	uint8 Foo = 0;
	_pFile->WriteLE(Foo);
	_pFile->WriteLE(m_Components);
	_pFile->Write(m_OperatorClass, sizeof(m_OperatorClass));

	uint32 Len = m_lParams.Len();
	_pFile->WriteLE(Len);
	for(int i = 0; i < Len; i++) _pFile->WriteLE(m_lParams[i]);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXW_SurfaceLayer
|__________________________________________________________________________________________________
\*************************************************************************************************/

// Only used for converting legacy surfaces to operators
#define XW_LAYERMAPPING_PLANE			0
#define XW_LAYERMAPPING_ENVIRONMENT		2
#define XW_LAYERMAPPING_WARP			3
#define XW_LAYERMAPPING_SKY				4
#define XW_LAYERMAPPING_CAUSTICS		5
#define XW_LAYERMAPPING_ENVIRONMENT2	6
#define XW_LAYERMAPPING_PROJECTION		7


CXW_SurfaceLayer::CXW_SurfaceLayer()
{
	MAUTOSTRIP(CXW_SurfaceLayer_ctor, MAUTOSTRIP_VOID);
	m_Flags = 0;
	m_Type = 0;
	m_RasterMode = 0;
	m_HDRColor.SetOne();
	m_TexCoordNr = 0;
	m_TexChannel = 0;
	m_TexEnvMode = CRC_TEXENVMODE_MULTIPLY;

	m_ZFunc = CRC_COMPARE_LESSEQUAL;
	m_AlphaFunc = CRC_COMPARE_GREATEREQUAL;
	m_AlphaRef = 128;
	m_SpecularFresnelBias = 0;
	m_SpecularFresnelScale = 0;

#ifdef USE_HASHED_TEXTURENAME
	m_TextureNameID = 0;
#else
	m_TextureName[0] = 0;
#endif
	m_TextureID = 0;
}

void CXW_SurfaceLayer::SetTextureName(const char* _pName)
{
	MAUTOSTRIP(CXW_SurfaceLayer_SetTextureName, MAUTOSTRIP_VOID);
	int Len = CStr::StrLen(_pName);
	if (Len >= XW_SURFACE_MAXTEXNAMELEN)
		Error_static("CXW_SurfaceLayer::SetTextureName", CStrF("Too many characters: %s (%d/%d)", _pName, Len, XW_SURFACE_MAXTEXNAMELEN-1));

#ifdef USE_HASHED_TEXTURENAME
	m_TextureNameID = StringToHash(_pName);
#else
	memcpy(m_TextureName, _pName, Len);
	m_TextureName[Len] = 0;
#endif
}


void CXW_SurfaceLayer::Init()
{
	MAUTOSTRIP(CXW_SurfaceLayer_Init, MAUTOSTRIP_VOID);
}

void CXW_SurfaceLayer::ParseContainer(CKeyContainer& _Keys)
{
	MAUTOSTRIP(CXW_SurfaceLayer_ParseContainer, MAUTOSTRIP_VOID);
	const char* g_SurfLayerFlagsTranslate[] = 
	{
		"$$$", "lighting", "alphacompare", "nozwrite", 
		"nocolorwrite", "noalphawrite", "mappingreference", "customtexenv",
		"stencilmask", "$$$", "$$$", "$$$", 
		"$$$", "transparent", "group", "invisible", 
		"nofog", "forcefogblack", "forcefogcolor", "forcefogalpha",
		"usercolor", 
		(char*)NULL
	};

	const char* g_SurfLayerTypeTranslate[] = 
	{
		"rendersurface", "diffuse", "specular2", "specular", "normal", "height", "transmission", "environment", "attribute", "anisotropicdirection", (char*)NULL
	};

	const char* g_SurfLayerRastermodeTranslate[] = 
	{
		"replace", "alphablend", "lightmapblend", "mul", "add", "alphaadd", "alphamul", "mul2", "muladd", "one_alpha", "one_invalpha", "destalphablend", (char*)NULL
	};

	const char* g_SurfLayerTexEnvTranslate[] = 
	{
		"mul", "blend", "decal", "add", "sub", (char*)NULL
	};

	const char* g_SurfLayerCompareTranslate[] = 
	{
		"$$$", "never", "less", "equal", "lessequal", "greater", "notequal", "greaterequal", "always", (char*)NULL
	};

	const char* g_SurfLayerMappingTranslate[] = 
	{
		"texcoord", "$$$", "env", "warp", "$$$", "$$$", "env2", "proj", (char*)NULL
	};

	TArray<CXW_LayerOperation> lOpCodes;

	bool bAlphaRef = false;
	fp4 AlphaRefFloat = 1;
	int Mapping = 0; 

	CVec2Dfp4 Offset(0);
	CVec2Dfp4 Scale(1);
	fp4 Params[4] = { 0,0,0,128 };

	int nKeys = _Keys.GetnKeys();
	for(int iKey = 0; iKey < nKeys; iKey++)
	{
		CStr Key = _Keys.GetKeyName(iKey).UpperCase();
		CStr Value = _Keys.GetKeyValue(iKey);
		
		if (Key == "FLAGS")
		{
			m_Flags = Value.TranslateFlags(g_SurfLayerFlagsTranslate);
		}
		else if (Key == "TYPE")
		{
			m_Type = Value.TranslateInt(g_SurfLayerTypeTranslate);
		}
		else if (Key == "RASTERMODE")
		{
			m_RasterMode = Value.TranslateInt(g_SurfLayerRastermodeTranslate);
		}
		else if (Key == "MAPPING")
		{
			Mapping = Value.TranslateInt(g_SurfLayerMappingTranslate);
		}
		else if (Key == "COLOR")
		{
//			CPixel32 Col;
//			Col.Parse(Value);
//			m_Color = Col;
			m_HDRColor.ParseColor(Value);
		}
		else if (Key == "TEXENV")
		{
			m_TexEnvMode = Value.TranslateInt(g_SurfLayerTexEnvTranslate);
		}
		else if (Key == "TEXCOORD")
		{
			m_TexCoordNr = Max(0, Value.Val_int());
		}
		else if (Key == "ZFUNC")
		{
			m_ZFunc = Value.TranslateInt(g_SurfLayerCompareTranslate);
		}
		else if (Key == "ALPHAFUNC")
		{
			m_AlphaFunc = Value.TranslateInt(g_SurfLayerCompareTranslate);
		}
		else if (Key == "ALPHAREF" || Key == "SPECULARPOWER")
		{
			bAlphaRef = true;
			m_AlphaRef = Value.Val_int();
			AlphaRefFloat = Value.Val_fp8();
		}
		else if (Key == "USERCOLOR")
		{
			m_iUserColor = Value.Val_int();
		}
		else if (Key == "SPECULARFRESNELBIAS")
		{
			m_SpecularFresnelBias = Value.Val_int();
		}
		else if (Key == "SPECULARFRESNELSCALE")
		{
			m_SpecularFresnelScale = Value.Val_int();
		}
		else if (Key == "OFFSET")
		{
			Offset.k[0] = (Value.GetStrSep(",")).Val_fp8();
			Offset.k[1] = (Value.GetStrSep(",")).Val_fp8();
		}
		else if (Key == "SCALE")
		{
			Scale.k[0] = (Value.GetStrSep(",")).Val_fp8();
			Scale.k[1] = (Value.GetStrSep(",")).Val_fp8();
		}
		else if (Key == "PARAM0")
			Params[0] = Value.Val_fp8();
		else if (Key == "PARAM1")
			Params[1] = Value.Val_fp8();
		else if (Key == "PARAM2")
			Params[2] = Value.Val_fp8();
		else if (Key == "PARAM3")
			Params[3] = Value.Val_fp8();
		else if (Key == "PARAMS")
		{
			for(int i = 0; i < 4; i++)
				Params[i] = (Value.GetStrSep(",")).Val_fp8();
		}
		else if (Key == "TEXTURE")
		{
			int Len = Value.Len();
			Value = Value.UpperCase();
			if (Len+1 > sizeof(m_TextureName)) Error_static("CXW_SurfaceLayer::ParseContainer", "Too long texture name. (" + Value + ")");
			SetTextureName((const char*)Value);
		}
		else if (Key.CompareSubStr("OPERATOR") == 0)
		{
			CXW_LayerOperation Op;
			if (Op.ParseKey(Value))
				lOpCodes.Add(Op);
		}
	}

	// -------------------------------------------------------------------
	//  Port legacy layer effects to VB operators
	// -------------------------------------------------------------------
	{
		switch(Mapping)
		{
		case XW_LAYERMAPPING_ENVIRONMENT : 
			{
				CXW_LayerOperation Op;
				if (Op.ParseKey(CStr("genenv u+v")))
					lOpCodes.Add(Op);
			}
			break;

		case XW_LAYERMAPPING_ENVIRONMENT2 : 
			{
				CXW_LayerOperation Op;
				if (Op.ParseKey(CStr("genenv2 u+v")))
					lOpCodes.Add(Op);
			}
			break;

		default : ;
		}

		if (Scale.k[0] != 1.0f ||
			Scale.k[1] != 1.0f)
		{
			CXW_LayerOperation Op;
			if (Op.ParseKey(CStrF("Scale u+v,%f,%f", Scale.k[0], Scale.k[1]) ))
				lOpCodes.Add(Op);
		}

		if (Offset.k[0] != 0 ||
			Offset.k[1] != 0)
		{
			CXW_LayerOperation Op;
			if (Op.ParseKey(CStrF("Offset u+v,%f,%f", Offset.k[0], Offset.k[1]) ))
				lOpCodes.Add(Op);
		}
	}

	// -------------------------------------------------------------------
	m_lOper.SetLen(lOpCodes.Len());
	for(int i = 0; i < lOpCodes.Len(); i++)
		m_lOper[i] = lOpCodes[i];

	if(!bAlphaRef)
		m_AlphaRef = (uint8)Params[3];

#ifdef USE_HASHED_TEXTURENAME
	if (m_TextureNameID == 0)
		Error_static("CXW_SurfaceLayer::ParseContainer_Layer", "No valid texture-name.");
#else
	if (!CStr::StrLen(GetTextureName()))
		Error_static("CXW_SurfaceLayer::ParseContainer_Layer", "No valid texture-name.");
#endif	

	Init();
}

void CXW_SurfaceLayer::Read(CCFile* _pFile)
{
	MAUTOSTRIP(CXW_SurfaceLayer_Read, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXW_SurfaceLayer::Read);
	uint32 Ver = 0;
	_pFile->ReadLE(Ver);

	CStr TexName;
	switch(Ver)
	{
/*	case 0x0100 :
		{
			uint16 Flags; _pFile->ReadLE(Flags); m_Flags = Flags;
			_pFile->ReadLE(m_Mapping);
			_pFile->ReadLE(m_RasterMode);
			_pFile->ReadLE(m_Color);
			CVec2Dfp4 Dummy;
			Dummy.Read(_pFile);
			Dummy.Read(_pFile);
			{ for(int i = 0; i < 4; i++) _pFile->ReadLE(Dummy.k[0]); }

			m_lOper.Clear();

			TexName.Read(_pFile);
			if (TexName.Len() >= XW_SURFACE_MAXTEXNAMELEN)
				Error_static("CXW_SurfaceLayer::Read", CStrF("Too long texture-name (%d/%d)", TexName.Len(), XW_SURFACE_MAXTEXNAMELEN-1) );
		}
		break;

	case 0x0101 :
		{
			uint16 Flags; _pFile->ReadLE(Flags); m_Flags = Flags;
			_pFile->ReadLE(m_Mapping);
			_pFile->ReadLE(m_RasterMode);
			_pFile->ReadLE(m_Color);
			_pFile->ReadLE(m_TexCoordNr);
			_pFile->ReadLE(m_TexChannel);
			_pFile->ReadLE(m_TexEnvMode);
			_pFile->ReadLE(m_ZFunc);
			_pFile->ReadLE(m_AlphaFunc);
			_pFile->ReadLE(m_AlphaRef);
			CVec2Dfp4 Dummy;
			Dummy.Read(_pFile);
			Dummy.Read(_pFile);
			{ for(int i = 0; i < 4; i++) _pFile->ReadLE(Dummy.k[0]); }

			m_lOper.Clear();

			TexName.Read(_pFile);
			if (TexName.Len() >= XW_SURFACE_MAXTEXNAMELEN)
				Error_static("CXW_SurfaceLayer::Read", CStrF("Too long texture-name (%d/%d)", TexName.Len(), XW_SURFACE_MAXTEXNAMELEN-1) );
		}
		break;

	case 0x0102 :
		{
			_pFile->ReadLE(m_Flags);
			_pFile->ReadLE(m_Mapping);
			_pFile->ReadLE(m_RasterMode);
			_pFile->ReadLE(m_Color);
			_pFile->ReadLE(m_TexCoordNr);
			_pFile->ReadLE(m_TexChannel);
			_pFile->ReadLE(m_TexEnvMode);
			_pFile->ReadLE(m_ZFunc);
			_pFile->ReadLE(m_AlphaFunc);
			_pFile->ReadLE(m_AlphaRef);
			CVec2Dfp4 Dummy;
			Dummy.Read(_pFile);
			Dummy.Read(_pFile);
			{ for(int i = 0; i < 4; i++) _pFile->ReadLE(Dummy.k[0]); }

			uint16 nOps = 0;
			_pFile->ReadLE(nOps);
			m_lOper.SetLen(nOps);
			{ for(int i = 0; i < nOps; i++) m_lOper[i].Read(_pFile); };

			TexName.Read(_pFile);
			if (TexName.Len() >= XW_SURFACE_MAXTEXNAMELEN)
				Error_static("CXW_SurfaceLayer::Read", CStrF("Too long texture-name (%d/%d)", TexName.Len(), XW_SURFACE_MAXTEXNAMELEN-1) );
		}
		break;

	case 0x0103 :
		{
			uint32 Flags;
			uint8 Mapping, RasterMode, TexCoordNr, TexChannel, TexEnvMode, ZFunc, AlphaFunc, AlphaRef;

			_pFile->ReadLE(Flags);
			_pFile->ReadLE(Mapping);
			_pFile->ReadLE(RasterMode);
			_pFile->ReadLE(m_Color);
			_pFile->ReadLE(TexCoordNr);
			_pFile->ReadLE(TexChannel);
			_pFile->ReadLE(TexEnvMode);
			_pFile->ReadLE(ZFunc);
			_pFile->ReadLE(AlphaFunc);
			_pFile->ReadLE(AlphaRef);

			m_Flags = Flags;
			m_Type = 0;
//			m_Mapping = Mapping;
			m_RasterMode = RasterMode;
			m_TexCoordNr = TexCoordNr;
			m_TexChannel = TexChannel;
			m_TexEnvMode = TexEnvMode;
			m_ZFunc = ZFunc;
			m_AlphaFunc = AlphaFunc;
			m_AlphaRef = AlphaRef;
			m_SpecularFresnelBias = 0;
			m_SpecularFresnelScale = 0;

			uint16 nOps = 0;
			_pFile->ReadLE(nOps);
			m_lOper.SetLen(nOps);
			{ for(int i = 0; i < nOps; i++) m_lOper[i].Read(_pFile); };

			TexName.Read(_pFile);
			if (TexName.Len() >= XW_SURFACE_MAXTEXNAMELEN)
				Error_static("CXW_SurfaceLayer::Read", CStrF("Too long texture-name (%d/%d)", TexName.Len(), XW_SURFACE_MAXTEXNAMELEN-1) );
		}
		break;
*/
	case 0x0104 :
		{
			uint32 BF0, BF1;
			_pFile->ReadLE(BF0);
			_pFile->ReadLE(BF1);
			uint32 Color32;
			_pFile->ReadLE(Color32);
			CPixel32 Color = Color32;
			m_HDRColor.k[0] = fp4(Color.GetR()) / 255.0f;
			m_HDRColor.k[1] = fp4(Color.GetG()) / 255.0f;
			m_HDRColor.k[2] = fp4(Color.GetB()) / 255.0f;
			m_HDRColor.k[3] = fp4(Color.GetA()) / 255.0f;
			_pFile->ReadLE(m_SpecularFresnelBias);
			_pFile->ReadLE(m_SpecularFresnelScale);

			m_Flags = BF0;
			m_Type = BF0 >> 28;

			m_RasterMode = BF1 >> 0;
			m_TexCoordNr = BF1 >> 4;
			m_TexChannel = BF1 >> 8;
			m_TexEnvMode = BF1 >> 12;
			m_ZFunc = BF1 >> 16;
			m_AlphaFunc = BF1 >> 20;
			m_AlphaRef = BF1 >> 24;

			uint16 nOps = 0;
			_pFile->ReadLE(nOps);
			m_lOper.SetLen(nOps);
			{ for(int i = 0; i < nOps; i++) m_lOper[i].Read(_pFile); };

			TexName.Read(_pFile);
			if (TexName.Len() >= XW_SURFACE_MAXTEXNAMELEN)
				Error_static("CXW_SurfaceLayer::Read", CStrF("Too long texture-name (%d/%d)", TexName.Len(), XW_SURFACE_MAXTEXNAMELEN-1) );
		}
		break;

	case 0x0105 :
		{
			uint32 BF0, BF1;
			_pFile->ReadLE(BF0);
			_pFile->ReadLE(BF1);
			m_HDRColor.Read(_pFile);
			_pFile->ReadLE(m_SpecularFresnelBias);
			_pFile->ReadLE(m_SpecularFresnelScale);

			m_Flags = BF0;
			m_Type = BF0 >> 28;

			m_RasterMode = BF1 >> 0;
			m_TexCoordNr = BF1 >> 4;
			m_TexChannel = BF1 >> 8;
			m_TexEnvMode = BF1 >> 12;
			m_ZFunc = BF1 >> 16;
			m_AlphaFunc = BF1 >> 20;
			m_AlphaRef = BF1 >> 24;

			uint16 nOps = 0;
			_pFile->ReadLE(nOps);
			m_lOper.SetLen(nOps);
			{ for(int i = 0; i < nOps; i++) m_lOper[i].Read(_pFile); };

			TexName.Read(_pFile);
			if (TexName.Len() >= XW_SURFACE_MAXTEXNAMELEN)
				Error_static("CXW_SurfaceLayer::Read", CStrF("Too long texture-name (%d/%d)", TexName.Len(), XW_SURFACE_MAXTEXNAMELEN-1) );
		}
		break;

	default :
		Error_static("CXW_SurfaceLayer::Read", CStrF("Unsupported layer-version (%.4x)", Ver));
	}

	SetTextureName((const char*) TexName);
	m_TextureID = 0;

	Init();
}

void CXW_SurfaceLayer::Write(CCFile* _pFile) const
{
#ifdef USE_HASHED_TEXTURENAME
	Error_static("CXW_SurfaceLayer::Write", "Cannot write when texturenames are disabled");
#else
	MAUTOSTRIP(CXW_SurfaceLayer_Write, MAUTOSTRIP_VOID);
	uint32 Ver = XW_SURFLAYER_VERSION;
	_pFile->WriteLE(Ver);
	
	uint32 BF0 = m_Flags + (m_Type << 28);
	uint32 BF1 = m_RasterMode + (m_TexCoordNr << 4) + (m_TexChannel << 8) + (m_TexEnvMode << 12) + 
		(m_ZFunc << 16) + (m_AlphaFunc << 20) + (m_AlphaRef << 24);

	_pFile->WriteLE(BF0);
	_pFile->WriteLE(BF1);
	m_HDRColor.Write(_pFile);
	_pFile->WriteLE(m_SpecularFresnelBias);
	_pFile->WriteLE(m_SpecularFresnelScale);

	uint16 nOps = m_lOper.Len();
	_pFile->WriteLE(nOps);
	{ for(int i = 0; i < nOps; i++) m_lOper[i].Write(_pFile); };

	CStr TexName = m_TextureName;
	TexName.Write(_pFile);
//	_pFile->Write(&m_TextureName[0], sizeof(m_TextureName));
#endif
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXW_SurfaceKeyFrame
|__________________________________________________________________________________________________
\*************************************************************************************************/
CXW_SurfaceKeyFrame::CXW_SurfaceKeyFrame()
{
	MAUTOSTRIP(CXW_SurfaceKeyFrame_ctor, MAUTOSTRIP_VOID);
	m_Duration = 0.1f;
	m_DurationRecp = 1.0f / m_Duration;
#ifdef XW_FOGMASK_ENABLE	
	m_FogMaskTextureID = 0;
#endif	
	m_PolygonOffsetScale = 1.0f;
	m_PolygonOffsetUnits = 0;

	m_Friction = 0.5f;
	m_Elasticy = 0.0f;
	m_MaterialType = 0;
	FillChar(m_User, sizeof(m_User), 0);
	FillChar(m_fUser, sizeof(m_fUser), 0);
	m_Medium.SetSolid();

}

void CXW_SurfaceKeyFrame::operator=(const CXW_SurfaceKeyFrame& _CopyFrom)
{
	MAUTOSTRIP(CXW_SurfaceKeyFrame_operator_assign, MAUTOSTRIP_VOID);
	m_lTextures.SetLen(_CopyFrom.m_lTextures.Len());
	for(int i = 0; i < m_lTextures.Len(); i++)
		m_lTextures[i] = _CopyFrom.m_lTextures[i];
	m_Duration = _CopyFrom.m_Duration;
	m_DurationRecp = _CopyFrom.m_DurationRecp;
#ifdef XW_FOGMASK_ENABLE	
	m_FogMaskName = _CopyFrom.m_FogMaskName;
	m_FogMaskTextureID = _CopyFrom.m_FogMaskTextureID;
#endif	
	m_PolygonOffsetScale = _CopyFrom.m_PolygonOffsetScale;
	m_PolygonOffsetUnits = _CopyFrom.m_PolygonOffsetUnits;
	m_Friction = _CopyFrom.m_Friction;
	m_Elasticy = _CopyFrom.m_Elasticy;
	m_MaterialType = _CopyFrom.m_MaterialType;
	memcpy(m_User, _CopyFrom.m_User, sizeof(m_User));
	memcpy(m_fUser, _CopyFrom.m_fUser, sizeof(m_fUser));
	m_Medium = _CopyFrom.m_Medium;
}

spCXW_SurfaceKeyFrame CXW_SurfaceKeyFrame::Duplicate() const
{
	MAUTOSTRIP(CXW_SurfaceKeyFrame_Duplicate, NULL);
	spCXW_SurfaceKeyFrame spKey = MNew(CXW_SurfaceKeyFrame);
	if (!spKey) MemError("Duplicate");

	*spKey = *this;
	return spKey;
}

#define LERPI(c0, c1, t) ((c0) + ((((c1)-(c0))*(t)) >> 8))
/*
static CPixel32 AlphaBlend(CPixel32 c0, CPixel32 c1, int alpha)
{
	MAUTOSTRIP(AlphaBlend, CPixel32());
	int r = LERPI(c0.GetR(), c1.GetR(), alpha);
	int g = LERPI(c0.GetG(), c1.GetG(), alpha);
	int b = LERPI(c0.GetB(), c1.GetB(), alpha);
	int a = LERPI(c0.GetA(), c1.GetA(), alpha);
	return (a << 24) + (r << 16) + (g << 8) + b;
}
*/

void CXW_SurfaceLayer::Interpolate(const CXW_SurfaceLayer& _Frm, fp4 t, CXW_SurfaceLayer& _Dst) const
{
	MAUTOSTRIP(CXW_SurfaceLayer_Interpolate, MAUTOSTRIP_VOID);
	_Dst.m_Flags = m_Flags;
	_Dst.m_Type = m_Type;
	_Dst.m_RasterMode = m_RasterMode;

	int TInt = RoundToInt(t*255.0f);
//	_Dst.m_Color = AlphaBlend(m_Color, _Frm.m_Color, TInt);
	m_HDRColor.Lerp(_Frm.m_HDRColor, t, _Dst.m_HDRColor);

	_Dst.m_TexCoordNr = m_TexCoordNr;
	_Dst.m_TexChannel = m_TexChannel;
	_Dst.m_TexEnvMode = m_TexEnvMode;
	_Dst.m_ZFunc = m_ZFunc;
	_Dst.m_AlphaFunc = m_AlphaFunc;
	_Dst.m_AlphaRef = LERPI(m_AlphaRef, _Frm.m_AlphaRef, TInt);
	_Dst.m_SpecularFresnelBias = LERPI(m_SpecularFresnelBias, _Frm.m_SpecularFresnelBias, TInt);
	_Dst.m_SpecularFresnelScale = LERPI(m_SpecularFresnelScale, _Frm.m_SpecularFresnelScale, TInt);

//	_Dst.m_Color = CPixel32(_Frm.m_Color).AlphaBlendRGBA(m_Color, RoundToInt(t*256.0f));
	_Dst.m_TextureID = m_TextureID;
	
	if (_Frm.m_lOper.Len() == m_lOper.Len())
	{
//		ConOutL(CStrF("(CXW_SurfaceLayer::Interpolate) %d != %d, %d", _Frm.m_lOper.Len(), m_lOper.Len(), _Dst.m_lOper.Len()));

		_Dst.m_lOper.QuickSetLen(m_lOper.Len());
		for(int iOper = 0; iOper < _Dst.m_lOper.Len(); iOper++)
		{
			const CXW_LayerOperation& OperSrc = m_lOper[iOper];
			const CXW_LayerOperation& OperFrm = _Frm.m_lOper[iOper];
			CXW_LayerOperation& OperDst = _Dst.m_lOper[iOper];

			OperDst.m_OpCode = OperSrc.m_OpCode;
			OperDst.m_iOperatorClass = OperSrc.m_iOperatorClass;
			OperDst.m_Components = OperSrc.m_Components;

			if (OperSrc.m_lParams.Len() == OperFrm.m_lParams.Len())
			{
				// Lerp params
				int nP = OperSrc.m_lParams.Len();
				OperDst.m_lParams.SetLen(nP);
				const fp4* pParamSrc = OperSrc.m_lParams.GetBasePtr();
				const fp4* pParamFrm = OperFrm.m_lParams.GetBasePtr();
				fp4* pParamDst = OperDst.m_lParams.GetBasePtr();
				for(int iP = 0; iP < nP; iP++)
					pParamDst[iP] = pParamSrc[iP] + t*(pParamFrm[iP] - pParamSrc[iP]);
			}
			else
				// Copy params
				OperDst.m_lParams = OperSrc.m_lParams;
		}
	}
	else
	{
		_Dst.m_lOper = m_lOper;
	}
}

CXW_SurfaceLayer* CXW_SurfaceKeyFrame::GetBumpMap()
{
	MAUTOSTRIP(CXW_SurfaceKeyFrame_GetBumpMap, NULL);
	CXW_SurfaceLayer* pLayers = m_lTextures.GetBasePtr();
	int nLayers = m_lTextures.Len();

	for(int i = 0; i < nLayers; i++)
		if (pLayers[i].m_Type == XW_LAYERTYPE_NORMAL)
			return &pLayers[i];

	return NULL;
}

void CXW_SurfaceKeyFrame::Interpolate(const CXW_SurfaceKeyFrame& _Frm, fp4 t, CXW_SurfaceKeyFrame& _Dst) const
{
	MAUTOSTRIP(CXW_SurfaceKeyFrame_Interpolate, MAUTOSTRIP_VOID);
	int i;
	int nTextures = m_lTextures.Len();
	_Dst.m_lTextures.QuickSetLen(nTextures);
	for(i = 0; i < nTextures; i++)
		m_lTextures[i].Interpolate(_Frm.m_lTextures[i], t, _Dst.m_lTextures[i]);

#ifdef XW_FOGMASK_ENABLE	
	_Dst.m_FogMaskTextureID = m_FogMaskTextureID;
#endif
	const fp4 invt = 1.0f - t;
	_Dst.m_PolygonOffsetScale = m_PolygonOffsetScale * invt + _Frm.m_PolygonOffsetScale * t;
	_Dst.m_PolygonOffsetUnits = m_PolygonOffsetUnits * invt + _Frm.m_PolygonOffsetUnits * t;
	_Dst.m_Friction = m_Friction * invt + _Frm.m_Friction * t;
	_Dst.m_Elasticy = m_Elasticy * invt + _Frm.m_Elasticy * t;
	_Dst.m_MaterialType = m_MaterialType;
	for(i = 0; i < 4; i++)
	{
		_Dst.m_User[i] = m_User[i];
		_Dst.m_fUser[i] = m_fUser[i] * invt + _Frm.m_fUser[i] * t;
	}
}

void CXW_SurfaceKeyFrame::ParseContainer(CKeyContainer& _Keys)
{
	MAUTOSTRIP(CXW_SurfaceKeyFrame_ParseContainer, MAUTOSTRIP_VOID);
	for(int i = 0; i < _Keys.GetnKeys(); i++)
	{
		CStr Key = _Keys.GetKeyName(i);
		CStr Value = _Keys.GetKeyValue(i);
		if (!m_Medium.ParseKey(Key, Value))
		{
			if (Key == "DURATION")
			{
				m_Duration = Value.Val_fp8();
				m_DurationRecp = 1.0f / m_Duration;
			}
#ifdef XW_FOGMASK_ENABLE	
			else if (Key == "FOGMASK")
				m_FogMaskName = Value;
#endif				
			else if (Key == "POLYGONOFFSET")
			{
				m_PolygonOffsetScale = Value.Getfp8Sep(",");
				m_PolygonOffsetUnits = Value.Getfp8Sep(",");
			}
			else if (Key == "FRICTION")
				m_Friction = Value.Val_fp8();
			else if (Key == "ELASTICY")
				m_Elasticy = Value.Val_fp8();
			else if (Key == "MATERIALTYPE")
				m_MaterialType = Value.Val_int();
			else if (Key == "USER0")
				m_User[0] = Value.Val_int();
			else if (Key == "USER1")
				m_User[1] = Value.Val_int();
			else if (Key == "USER2")
				m_User[2] = Value.Val_int();
			else if (Key == "USER3")
				m_User[3] = Value.Val_int();
			else if (Key == "USER0F")
				m_fUser[0] = Value.Val_fp8();
			else if (Key == "USER1F")
				m_fUser[1] = Value.Val_fp8();
			else if (Key == "USER2F")
				m_fUser[2] = Value.Val_fp8();
			else if (Key == "USER3F")
				m_fUser[3] = Value.Val_fp8();
		}
	}
}

void CXW_SurfaceKeyFrame::ParseNode(CKeyContainerNode* _pNode)
{
	MAUTOSTRIP(CXW_SurfaceKeyFrame_ParseNode, MAUTOSTRIP_VOID);
	ParseContainer(*_pNode->GetKeys());

	int iTxtLayer = 0;
//	int iBumpLayer = 0;
	for(int i = 0; i < _pNode->GetNumChildren(); i++)
	{
		CKeyContainerNode* pN = _pNode->GetChild(i);
		CKeyContainer* pKC = pN->GetKeys();
		if (pKC->GetKeyIndex("CLASSNAME") >= 0)
		{
			int iKey = pKC->GetKeyIndex("CLASSNAME");
			CStr Val = pKC->GetKeyValue(iKey).UpperCase();

			if (Val == "BLAYER")
			{
/*				if (m_lBumpMaps.Len() >= XW_SURFACE_MAXBUMPMAPS)
					Error_static("ParseNode", "Too many bumpmap layers.");

				if (iBumpLayer >= m_lBumpMaps.Len())
					m_lBumpMaps.Add(CXW_SurfaceLayer());
				m_lBumpMaps[iBumpLayer].ParseContainer(*pKC);
				iBumpLayer++;*/
			}
			else if (Val == "TLAYER")
			{
//				if (m_lTextures.Len() >= XW_SURFACE_MAXTEXTURES)
//					Error_static("ParseNode", "Too many texture layers.");

				if (iTxtLayer >= m_lTextures.Len())
					m_lTextures.SetLen(iTxtLayer+1);
				m_lTextures[iTxtLayer].ParseContainer(*pKC);
				iTxtLayer++;
			}
		}
	}

}
void CXW_SurfaceKeyFrame::InitTextures(CTextureContext* pTC, bool _bReportFailure)
{
	MAUTOSTRIP(CXW_SurfaceKeyFrame_InitTextures, MAUTOSTRIP_VOID);
	int iTxt;
	// Textures
	for(iTxt = 0; iTxt < m_lTextures.Len(); iTxt++)
	{
		CXW_SurfaceLayer& Layer = m_lTextures[iTxt];
#ifdef USE_HASHED_TEXTURENAME
		if (Layer.m_TextureNameID == StringToHash("$REFLECTIONMAP"))
			Layer.m_TextureID = XW_SURFTEX_REFLECTIONMAP;
		else if (Layer.m_TextureNameID == StringToHash("$REFRACTIONMAP"))
			Layer.m_TextureID = XW_SURFTEX_REFRACTIONMAP;
		else if (Layer.m_TextureNameID == StringToHash("$LIGHTMAP"))
			Layer.m_TextureID = XW_SURFTEX_LIGHTMAP;
		else if (Layer.m_TextureNameID == StringToHash("$TEXTUREMAP0"))
			Layer.m_TextureID = XW_SURFTEX_TEXTUREMAP0;
		else if (Layer.m_TextureNameID == StringToHash("$TEXTUREMAP1"))
			Layer.m_TextureID = XW_SURFTEX_TEXTUREMAP1;
		else if (Layer.m_TextureNameID == StringToHash("$TEXTUREMAP2"))
			Layer.m_TextureID = XW_SURFTEX_TEXTUREMAP2;
		else if (Layer.m_TextureNameID == StringToHash("$TEXTUREMAP3"))
			Layer.m_TextureID = XW_SURFTEX_TEXTUREMAP3;
		else if (Layer.m_TextureNameID)
			Layer.m_TextureID = pTC->GetTextureID(Layer.m_TextureNameID);
#else		
		const char* pTextureName = Layer.GetTextureName();
		if (pTextureName && pTextureName[0] == '$')
		{
			if(strncmp(pTextureName, "$REFLECTIONMAP", 14) == 0)
				Layer.m_TextureID = XW_SURFTEX_REFLECTIONMAP;
			else if(strncmp(pTextureName, "$REFRACTIONMAP", 14) == 0)
				Layer.m_TextureID = XW_SURFTEX_REFRACTIONMAP;
			else if(strncmp(pTextureName, "$LIGHTMAP", 9) == 0)
				Layer.m_TextureID = XW_SURFTEX_LIGHTMAP;
			else if(strncmp(pTextureName, "$TEXTUREMAP0", 12) == 0)
				Layer.m_TextureID = XW_SURFTEX_TEXTUREMAP0;
			else if(strncmp(pTextureName, "$TEXTUREMAP1", 12) == 0)
				Layer.m_TextureID = XW_SURFTEX_TEXTUREMAP1;
			else if(strncmp(pTextureName, "$TEXTUREMAP2", 12) == 0)
				Layer.m_TextureID = XW_SURFTEX_TEXTUREMAP2;
			else if(strncmp(pTextureName, "$TEXTUREMAP3", 12) == 0)
				Layer.m_TextureID = XW_SURFTEX_TEXTUREMAP3;
			else
				Layer.m_TextureID = 0;
		}
		else if (pTextureName)
			Layer.m_TextureID = pTC->GetTextureID(pTextureName);
#endif			

		// Throw an exception if _bReportFailure says so.
		if (!Layer.m_TextureID)
		{
			if (_bReportFailure)
			{
#ifdef USE_HASHED_TEXTURENAME
				Error("InitTextures", CStrF("Undefined texture: '%08X'", Layer.m_TextureNameID));
#else
				Error("InitTextures", CStrF("Undefined texture: %s", pTextureName ? pTextureName : "(null)"));
#endif		
			}
			else
				Layer.m_TextureID = pTC->GetTextureID("SPECIAL_DEFAULT");
		}
	}

#ifdef XW_FOGMASK_ENABLE	
	// Fogmask
	if (m_FogMaskName != "")
	{
		m_FogMaskTextureID = pTC->GetTextureID(m_FogMaskName);
		if (!m_FogMaskTextureID)
			if (_bReportFailure)
				Error("InitTextures", "Undefined fog-texture: " + m_FogMaskName)
			else
				m_FogMaskTextureID = pTC->GetTextureID("SPECIAL_DEFAULT");

//		LogFile("FOGMASK " + m_FogMaskName + CStrF(", %d", m_FogMaskTextureID));
	}
#endif	
}

/*
void CXW_SurfaceKeyFrame::SetTextureFlags(CTextureContext* pTC, int _Flags)
{
	MAUTOSTRIP(CXW_SurfaceKeyFrame_SetTextureFlags, MAUTOSTRIP_VOID);
	CXR_SurfaceContext* pSC = NULL;

	for(int iTxt = 0; iTxt < m_lTextures.Len(); iTxt++)
	{
		const CXW_SurfaceLayer& Layer = m_lTextures[iTxt];

		bool bOperTextureID = false;

		for(int iOper = 0; iOper < Layer.m_lOper.Len(); iOper++)
		{
			const CXW_LayerOperation& Oper = Layer.m_lOper[iOper];
			if (Oper.m_Components & XW_LAYEROP_ATTRIB)
			{
				if (!pSC)
				{
					MACRO_GetRegisterObject(CXR_SurfaceContext, pSCTemp, "SYSTEM.SURFACECONTEXT");
					if (!pSCTemp) Error("SetTextureFlags", "No surface-context available.");
					pSC = pSCTemp;
				}

				CXR_VBOperator* pVBOper = pSC->VBOperator_Get(Oper.m_iOperatorClass);
				if (pVBOper)
				{
					int nTxt = pVBOper->OnGetTextureCount(&Layer, Oper);
					for(int i = 0; i < nTxt; i++)
						pTC->SetTextureFlags(pVBOper->OnEnumTextureID(&Layer, Oper, i), _Flags);

					bOperTextureID = true;
				}
			}
		}

		if (!bOperTextureID)
		{
			if (m_lTextures[iTxt].m_TextureID < XW_SURFTEX_SPECIALBASE)
				pTC->SetTextureFlags(m_lTextures[iTxt].m_TextureID, _Flags);
		}
	}

	if (m_FogMaskTextureID)
		pTC->SetTextureFlags(m_FogMaskTextureID, _Flags);
}
*/

void CXW_SurfaceKeyFrame::SetTextureParam(CTextureContext* pTC, int _Param, int _Value)
{
	MAUTOSTRIP(CXW_SurfaceKeyFrame_SetTextureParam, MAUTOSTRIP_VOID);
	CXR_SurfaceContext* pSC = NULL;

	for(int iTxt = 0; iTxt < m_lTextures.Len(); iTxt++)
	{
		const CXW_SurfaceLayer& Layer = m_lTextures[iTxt];

//		bool bOperTextureID = false;

		for(int iOper = 0; iOper < Layer.m_lOper.Len(); iOper++)
		{
			const CXW_LayerOperation& Oper = Layer.m_lOper[iOper];
//			if (Oper.m_Components & XW_LAYEROP_ATTRIB)
			{
				if (!pSC)
				{
					MACRO_GetRegisterObject(CXR_SurfaceContext, pSCTemp, "SYSTEM.SURFACECONTEXT");
					if (!pSCTemp) Error("SetTextureFlags", "No surface-context available.");
					pSC = pSCTemp;
				}

				CXR_VBOperator* pVBOper = pSC->VBOperator_Get(Oper.m_iOperatorClass);
				if (pVBOper)
				{
//					M_TRACEALWAYS("VBOper %s\n", Oper.m_OperatorClass);

					int nTxt = pVBOper->OnGetTextureCount(NULL, &Layer, Oper);
					for(int i = 0; i < nTxt; i++)
					{
						int TxtID = pVBOper->OnEnumTextureID(NULL, &Layer, Oper, i);
						if (TxtID < XW_SURFTEX_SPECIALBASE)
							pTC->SetTextureParam(TxtID, _Param, _Value);
					}

//					bOperTextureID = true;
				}
			}
		}

//		if (!bOperTextureID)
		{
			if (m_lTextures[iTxt].m_TextureID < XW_SURFTEX_SPECIALBASE)
				pTC->SetTextureParam(m_lTextures[iTxt].m_TextureID, _Param, _Value);
		}
	}

#ifdef XW_FOGMASK_ENABLE	
	if (m_FogMaskTextureID)
		pTC->SetTextureParam(m_FogMaskTextureID, _Param, _Value);
#endif		
}

void CXW_SurfaceKeyFrame::SetTextureParamfv(CTextureContext* pTC, int _Param, const fp4* _pValues)
{
	MAUTOSTRIP(CXW_SurfaceKeyFrame_SetTextureParamfv, MAUTOSTRIP_VOID);
	CXR_SurfaceContext* pSC = NULL;

	for(int iTxt = 0; iTxt < m_lTextures.Len(); iTxt++)
	{
		const CXW_SurfaceLayer& Layer = m_lTextures[iTxt];

//		bool bOperTextureID = false;

		for(int iOper = 0; iOper < Layer.m_lOper.Len(); iOper++)
		{
			const CXW_LayerOperation& Oper = Layer.m_lOper[iOper];
//			if (Oper.m_Components & XW_LAYEROP_ATTRIB)
			{
				if (!pSC)
				{
					MACRO_GetRegisterObject(CXR_SurfaceContext, pSCTemp, "SYSTEM.SURFACECONTEXT");
					if (!pSCTemp) Error("SetTextureFlags", "No surface-context available.");
					pSC = pSCTemp;
				}

				CXR_VBOperator* pVBOper = pSC->VBOperator_Get(Oper.m_iOperatorClass);
				if (pVBOper)
				{
					int nTxt = pVBOper->OnGetTextureCount(NULL, &Layer, Oper);
					for(int i = 0; i < nTxt; i++)
						pTC->SetTextureParamfv(pVBOper->OnEnumTextureID(NULL, &Layer, Oper, i), _Param, _pValues);

//					bOperTextureID = true;
				}
			}
		}

//		if (!bOperTextureID)
		{
			if (m_lTextures[iTxt].m_TextureID < XW_SURFTEX_SPECIALBASE)
				pTC->SetTextureParamfv(m_lTextures[iTxt].m_TextureID, _Param, _pValues);
		}
	}

#ifdef XW_FOGMASK_ENABLE	
	if (m_FogMaskTextureID)
		pTC->SetTextureParamfv(m_FogMaskTextureID, _Param, _pValues);
#endif		
}

#ifndef PLATFORM_CONSOLE
bool CXW_SurfaceKeyFrame::IsTransparent(CTextureContext* _pTC, CVec2Dfp4 _uv)
{
	MAUTOSTRIP(CXW_SurfaceKeyFrame_IsTransparent, false);
	if(m_lTextures.Len() != 0)
	{
		
		// Much to do if we are going to complete this function.. -JA
#ifdef USE_HASHED_TEXTURENAME
		int textureID = _pTC->GetTextureID(m_lTextures[0].m_TextureNameID);
#else
		int textureID = _pTC->GetTextureID(m_lTextures[0].GetTextureName());
#endif		
		if(textureID < XW_SURFTEX_SPECIALBASE)
		{
			int _iMipMap = 0;
			CImage *pImage = _pTC->GetTexture(textureID, 0, CTC_TEXTUREVERSION_ANY);
			if(!pImage) // Compressed 
			{
				int iL = _pTC->GetLocal(textureID);
				
				CTextureContainer_Plain* con = (CTextureContainer_Plain*)_pTC->GetTextureContainer(textureID);
				
				CTexture* pT = con->GetTextureMap(iL, CTC_TEXTUREVERSION_ANY);
				pT->Decompress(false); 
				
				//int nMip = pT->m_nMipmaps;
				//if ((_iMipMap < 0) || (_iMipMap >= nMip)) Error("GetTexture", CStrF("Invalid MIP-map number. (%d/%d, %s)", _iMipMap, nMip, (char*) pT->m_LargestMap.IDString() ));
				
				pImage =  &pT->m_LargestMap;



			}
			
			if(pImage)
			{
				int x = RoundToInt(_uv[0] * pImage->GetWidth()) & (pImage->GetWidth() - 1);
				int y = RoundToInt(_uv[1] * pImage->GetHeight()) & (pImage->GetHeight() - 1);
				/*spCImage pImageUC;
				if(pImage->IsCompressed())
				{
					pImageUC = DNew(CImage) CImage();
					pImage->Uncompress(pImageUC);
					pImage = pImageUC;
				}*/
				
				CPixel32 col = pImage->GetPixel(pImage->GetClipRect(), CPnt(x, y));
				if(col.GetA() == 0)
					return true;
			}
		}
		
	}
	return false;
}
#endif

// -------------------------------------------------------------------
void CXW_SurfaceKeyFrame::Read(CCFile* _pFile)
{
	MAUTOSTRIP(CXW_SurfaceKeyFrame_Read, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXW_SurfaceKeyFrame::Read);
	
	uint16 Version = 0;
	_pFile->ReadLE(Version);

	switch(Version)
	{
	case 0x0103 :
		{
			_pFile->ReadLE(m_PolygonOffsetScale);
			_pFile->ReadLE(m_PolygonOffsetUnits);
		}

	case 0x0102 :
		{
			_pFile->ReadLE(m_Duration);
			if (m_Duration > 0.0)
				m_DurationRecp = 1.0f / m_Duration;
			else
				m_DurationRecp = 0.0;
#ifdef XW_FOGMASK_ENABLE	
			m_FogMaskName.Read(_pFile);
#else
			{
				CStr FogMaskName;
				FogMaskName.Read(_pFile);
			}
#endif			
			_pFile->ReadLE(m_Friction);
			_pFile->ReadLE(m_Elasticy);
			_pFile->ReadLE(m_MaterialType);
			int i;
			for(i = 0; i < 4; i++)
				_pFile->ReadLE(m_User[i]);
			for(i = 0; i < 4; i++)
				_pFile->ReadLE(m_fUser[i]);

			m_Medium.Read(_pFile);

			int32 nTxt;
			_pFile->ReadLE(nTxt);
			m_lTextures.SetLen(nTxt);

			for(i = 0; i < nTxt; i++)
				m_lTextures[i].Read(_pFile);
		}
		break;

	case 0x0101 :
		{
			_pFile->ReadLE(m_Duration);
			if (m_Duration > 0.0)
				m_DurationRecp = 1.0f / m_Duration;
			else
				m_DurationRecp = 0.0;
		}
	case 0x0100 :
		{
			int i;
#ifdef XW_FOGMASK_ENABLE	
			m_FogMaskName.Read(_pFile);
#else
			{
				CStr FogMaskName;
				FogMaskName.Read(_pFile);
			}
#endif			
			_pFile->ReadLE(m_Friction);
			_pFile->ReadLE(m_Elasticy);
			_pFile->ReadLE(m_MaterialType);
			for(i = 0; i < 4; i++)
				_pFile->ReadLE(m_User[i]);
			for(i = 0; i < 4; i++)
				_pFile->ReadLE(m_fUser[i]);

			m_Medium.Read(_pFile);

			int32 nTxt, nBump;
			_pFile->ReadLE(nTxt);
			_pFile->ReadLE(nBump);
			m_lTextures.SetLen(nTxt);
//			m_lBumpMaps.SetLen(nBump);

			for(i = 0; i < nTxt; i++)
				m_lTextures[i].Read(_pFile);
			CXW_SurfaceLayer Dummy;
			for(i = 0; i < nBump; i++)
				Dummy.Read(_pFile);
		}
		break;
	default :
		Error("Read", CStrF("Unsupported surface-keyframe version %.4x", Version));
	}
}

void CXW_SurfaceKeyFrame::Write(CCFile* _pFile) const
{
	MAUTOSTRIP(CXW_SurfaceKeyFrame_Write, MAUTOSTRIP_VOID);
	int i;
	uint16 Version = XW_SURFKEYFRAME_VERSION;

	_pFile->WriteLE(Version);

	// 0x0103:
	_pFile->WriteLE(m_PolygonOffsetScale);
	_pFile->WriteLE(m_PolygonOffsetUnits);

	// 0x0102:
	_pFile->WriteLE(m_Duration);
#ifdef XW_FOGMASK_ENABLE	
	m_FogMaskName.Write(_pFile);
#else
	CStr FogMaskName;
	FogMaskName.Write(_pFile);
#endif	
	_pFile->WriteLE(m_Friction);
	_pFile->WriteLE(m_Elasticy);
	_pFile->WriteLE(m_MaterialType);
	for(i = 0; i < 4; i++)
		_pFile->WriteLE(m_User[i]);
	for(i = 0; i < 4; i++)
		_pFile->WriteLE(m_fUser[i]);

	m_Medium.Write(_pFile);

	int32 nTxt = m_lTextures.Len();
	_pFile->WriteLE(nTxt);

	for(i = 0; i < nTxt; i++)
		m_lTextures[i].Write(_pFile);
}

void CXW_SurfaceKeyFrame::MulSpecPower()
{
	uint32 i;
	for( i = 0;i < m_lTextures.Len();i++ )
	{
		CXW_SurfaceLayer &Tex = m_lTextures[i];

		// If alpharef was set on a specular2 layer we assume someone wanted to set specular power with it
		if( Tex.m_Type == XW_LAYERTYPE_DIFFUSE2 && Tex.m_AlphaRef != 128 )
		{
			Tex.m_HDRColor.k[3] = fp4(Tex.m_HDRColor.k[3]) * fp4(Tex.m_AlphaRef);
		}
	}
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXW_SurfaceSequence
|__________________________________________________________________________________________________
\*************************************************************************************************/
int CXW_SurfaceSequence::GetNumKeys() const
{
	MAUTOSTRIP(CXW_SurfaceSequence_GetNumKeys, 0);
#ifdef XW_SURF_THIN
	return m_lKeyFrames.Len();
#else
	if (D_MXDFCREATE && D_MPLATFORM != 0)
		return m_lKeyFramesThin.Len();
	return m_lspKeyFrames.Len();
#endif
}

CXW_SurfaceKeyFrame* CXW_SurfaceSequence::GetKey(int _iKey)
{
	MAUTOSTRIP(CXW_SurfaceSequence_GetKey, NULL);
#ifdef XW_SURF_THIN
	return &m_lKeyFrames[_iKey];
#else
	if (D_MXDFCREATE && D_MPLATFORM != 0)
		return &m_lKeyFramesThin[_iKey];
	
	return m_lspKeyFrames[_iKey];
#endif
}

const CXW_SurfaceKeyFrame* CXW_SurfaceSequence::GetKey(int _iKey) const
{
	MAUTOSTRIP(CXW_SurfaceSequence_GetKey_2, NULL);
#ifdef XW_SURF_THIN
	return &m_lKeyFrames[_iKey];
#else
	if (D_MXDFCREATE && D_MPLATFORM != 0)
		return &m_lKeyFramesThin[_iKey];
	return m_lspKeyFrames[_iKey];
#endif
}

CXW_SurfaceSequence::CXW_SurfaceSequence()
{
	MAUTOSTRIP(CXW_SurfaceSequence_ctor, MAUTOSTRIP_VOID);
	m_iRepSFrame = 0;
	m_iRepEFrame = 0;
	m_RepSTime = 0;
	m_RepETime = 0;
}

spCXW_SurfaceSequence CXW_SurfaceSequence::Duplicate() const
{
	MAUTOSTRIP(CXW_SurfaceSequence_Duplicate, NULL);
	MSCOPESHORT(CXW_SurfaceSequence::Duplicate);
	int i;
	spCXW_SurfaceSequence spSeq = MNew(CXW_SurfaceSequence);
	if (!spSeq) MemError("Duplicate");
	spSeq->m_iRepSFrame = m_iRepSFrame;
	spSeq->m_iRepEFrame = m_iRepEFrame;
	spSeq->m_RepSTime = m_RepSTime;
	spSeq->m_RepETime = m_RepETime;
	int nKeys = GetNumKeys();
#ifdef XW_SURF_THIN
	spSeq->m_lKeyFrames.SetLen(nKeys);
	for(i = 0; i < nKeys; i++)
		spSeq->m_lKeyFrames[i] = m_lKeyFrames[i];
#else
	if (D_MXDFCREATE && D_MPLATFORM != 0)
	{
		spSeq->m_lKeyFramesThin.SetLen(nKeys);
		for(i = 0; i < nKeys; i++)
			spSeq->m_lKeyFramesThin[i] = m_lKeyFramesThin[i];
	}
	else
	{
		spSeq->m_lspKeyFrames.SetLen(nKeys);
		for(i = 0; i < nKeys; i++)
			spSeq->m_lspKeyFrames[i] = m_lspKeyFrames[i]->Duplicate();
	}
#endif
	return spSeq;
}

CXW_SurfaceKeyFrame* CXW_SurfaceSequence::GetBaseFrame(CXW_Surface* _pSurf)
{
	MAUTOSTRIP(CXW_SurfaceSequence_GetBaseFrame, NULL);
#ifdef XW_SURF_THIN
	if (!m_lKeyFrames.Len())
		Error("GetBaseFrame", "Surface sequence have no key-frames (" + _pSurf->m_Name + ")");
	return &m_lKeyFrames[0];
#else
	if (D_MXDFCREATE && D_MPLATFORM != 0)
	{
		if (!m_lKeyFramesThin.Len())
			Error("GetBaseFrame", "Surface sequence have no key-frames (" + _pSurf->m_Name + ")");
		return &m_lKeyFramesThin[0];
	}
	else
	{
		if (!m_lspKeyFrames.Len())
			Error("GetBaseFrame", "Surface sequence have no key-frames (" + _pSurf->m_Name + ")");
		return m_lspKeyFrames[0];
	}
#endif
}

const CXW_SurfaceKeyFrame* CXW_SurfaceSequence::GetBaseFrame(const CXW_Surface* _pSurf) const
{
	MAUTOSTRIP(CXW_SurfaceSequence_GetBaseFrame2, NULL);
#ifdef XW_SURF_THIN
	if (!m_lKeyFrames.Len())
		Error("GetBaseFrame", "Surface sequence have no key-frames (" + _pSurf->m_Name + ")");
	return &m_lKeyFrames[0];
#else
	if (D_MXDFCREATE && D_MPLATFORM != 0)
	{
		if (!m_lKeyFramesThin.Len())
			Error("GetBaseFrame", "Surface sequence have no key-frames (" + _pSurf->m_Name + ")");
		return &m_lKeyFramesThin[0];
	}
	else
	{
		if (!m_lspKeyFrames.Len())
			Error("GetBaseFrame", "Surface sequence have no key-frames (" + _pSurf->m_Name + ")");
		return m_lspKeyFrames[0];
	}
#endif
}

void CXW_SurfaceSequence::GetFrameAndTimeFraction(const CMTime& _Time, int& _Frame, fp4& _tFrac, CMTime& _tWrapped) const
{
	MAUTOSTRIP(CXW_SurfaceSequence_GetFrameAndTimeFraction, MAUTOSTRIP_VOID);
	_Frame = 0;
	_tFrac = 0;
	_tWrapped = _Time; // Make sure the time isn't too large

	int nKeys = GetNumKeys();
	if (!nKeys) return;

	fp4 t = 0;
	int Frame = 0;
	if (m_iRepSFrame || m_iRepEFrame)
	{
//	LogFile(CStrF("Before loop, t %f -> %f, Frame %d", _Time, t, Frame));
		if (_Time.Compare(m_RepSTime) > 0.0f)
		{
			fp4 Mod = m_RepETime - m_RepSTime;
			if(Mod > 0.0001f)
				t = (_Time - CMTime::CreateFromSeconds(m_RepSTime)).GetTimeModulus(Mod);
//			t = M_FMod(_Time - m_RepSTime, m_RepETime - m_RepSTime);
			Frame = m_iRepSFrame;
//	LogFile(CStrF("In loop, t %f -> %f, Frame %d", _Time, t, Frame));
		}
		else
			t = _Time.GetTime();
	}
	else if (_Time.Compare(m_TotTime) >= 0) 
	{
		_Frame = nKeys-1;
		return;
	}
	else
		t = _Time.GetTime();


	_tWrapped = CMTime::CreateFromSeconds(t + m_RepSTime);

	while(t > GetKey(Frame)->m_Duration)
	{
		t -= GetKey(Frame)->m_Duration;
		if (Frame+1 >= nKeys) break;
		Frame++;
	}

	_Frame = Frame;
	_tFrac = t;
}

CXW_SurfaceKeyFrame* CXW_SurfaceSequence::GetFrame(int _iFrm)
{
	MAUTOSTRIP(CXW_SurfaceSequence_GetFrame, NULL);
	if (_iFrm < 0)
		return GetKey(0);
	else if (_iFrm >= GetNumKeys())
		return GetKey(GetNumKeys()-1);
	else
		return GetKey(_iFrm);
}

CXW_SurfaceKeyFrame* CXW_SurfaceSequence::GetFrame(const CMTime& _Time, CXW_SurfaceKeyFrame& _TmpStorage)
{
	MAUTOSTRIP(CXW_SurfaceSequence_GetFrame_2, NULL);
	MSCOPESHORT(CXW_SurfaceSequence::GetFrame);
	if (GetNumKeys() < 2)
	{
		CXW_SurfaceKeyFrame* pSKF = GetKey(0);
		pSKF->m_AnimTime = _Time;
		pSKF->m_AnimTimeWrapped = _Time;
		return pSKF;
	}

	fp4 tFrac;
	CMTime tWrapped;
	int Frame;
	GetFrameAndTimeFraction(_Time, Frame, tFrac, tWrapped);

	if (tFrac == 0.0f)
	{
//ConOut(CStrF("SurfSeq on frame %f, %d", _Time, Frame));
//LogFile(CStrF("SurfSeq on frame %f, %d", _Time, Frame));
		CXW_SurfaceKeyFrame* pSKF = GetKey(Frame);
		pSKF->m_AnimTime = _Time;
		pSKF->m_AnimTimeWrapped = tWrapped;
		return pSKF;
	}
	else
	{
		tFrac *= GetKey(Frame)->m_DurationRecp;
		int Frame2 = Min(Frame+1, GetNumKeys()-1);
		GetKey(Frame)->Interpolate(*GetKey(Frame2), tFrac, _TmpStorage);

		_TmpStorage.m_AnimTime = _Time;
		_TmpStorage.m_AnimTimeWrapped = tWrapped;

//ConOut(CStrF("SurfSeq %f, %d, %f, %.8x", _Time, Frame, tFrac, _TmpStorage.m_lTextures[0].m_Color));
//LogFile(CStrF("SurfSeq %f, %d, %f, %.8x", _Time, Frame, tFrac, _TmpStorage.m_lTextures[0].m_Color));
		return &_TmpStorage;
	}
}

void CXW_SurfaceSequence::SetNumKeyFrames(int _nKeys)
{
	MAUTOSTRIP(CXW_SurfaceSequence_SetNumKeyFrames, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXW_SurfaceSequence::SetNumKeyFrames);
#ifdef XW_SURF_THIN
	m_lKeyFrames.SetLen(_nKeys);
#else
	if (D_MXDFCREATE && D_MPLATFORM != 0)
	{
		m_lKeyFramesThin.SetLen(_nKeys);
	}
	else
	{
		m_lspKeyFrames.SetLen(_nKeys);
		for(int i = 0; i < _nKeys; i++)
			if (!m_lspKeyFrames[i])
			{
				m_lspKeyFrames[i] = MNew(CXW_SurfaceKeyFrame);
				if (!m_lspKeyFrames[i]) MemError("SetNumKeyFrames");
			}
	}
#endif

}

void CXW_SurfaceSequence::DeleteExcessSequences(CKeyContainer* _pKC)
{
	MAUTOSTRIP(CXW_SurfaceSequence_DeleteExcessSequences, MAUTOSTRIP_VOID);
	for(int i = 0; i < _pKC->GetnKeys(); i++)
	{
		if (_pKC->GetKeyValue(i).Find("#") >= 0)
			_pKC->SetKeyValue(i, _pKC->GetKeyValue(i).GetStrSep("#"));
	}
}

void CXW_SurfaceSequence::DeleteExcessSequences_r(CKeyContainerNode* _pNode)
{
	MAUTOSTRIP(CXW_SurfaceSequence_DeleteExcessSequences_r, MAUTOSTRIP_VOID);
	DeleteExcessSequences(_pNode->GetKeys());
	for(int i = 0; i < _pNode->GetNumChildren(); i++)
		DeleteExcessSequences_r(_pNode->GetChild(i));
}

void CXW_SurfaceSequence::DeleteExcessFrames(CKeyContainer* _pKC)
{
	MAUTOSTRIP(CXW_SurfaceSequence_DeleteExcessFrames, MAUTOSTRIP_VOID);
	for(int i = 0; i < _pKC->GetnKeys(); i++)
	{
		if (_pKC->GetKeyValue(i).Find(";") >= 0)
			_pKC->SetKeyValue(i, _pKC->GetKeyValue(i).GetStrSep(";"));
	}
}

void CXW_SurfaceSequence::DeleteExcessFrames_r(CKeyContainerNode* _pNode)
{
	MAUTOSTRIP(CXW_SurfaceSequence_DeleteExcessFrames_r, MAUTOSTRIP_VOID);
	DeleteExcessFrames(_pNode->GetKeys());
	for(int i = 0; i < _pNode->GetNumChildren(); i++)
		DeleteExcessFrames_r(_pNode->GetChild(i));
}

void CXW_SurfaceSequence::DeleteFrame(CKeyContainer* _pKC)
{
	MAUTOSTRIP(CXW_SurfaceSequence_DeleteFrame, MAUTOSTRIP_VOID);
	for(int i = 0; i < _pKC->GetnKeys(); i++)
	{
		int PosFrm = _pKC->GetKeyValue(i).Find(";");
		if (PosFrm >= 0)
		{
			_pKC->SetKeyValue(i, _pKC->GetKeyValue(i).Copy(PosFrm+1, 100000));
		}
	}
}

void CXW_SurfaceSequence::DeleteFrame_r(CKeyContainerNode* _pNode)
{
	MAUTOSTRIP(CXW_SurfaceSequence_DeleteFrame_r, MAUTOSTRIP_VOID);
	DeleteFrame(_pNode->GetKeys());
	for(int i = 0; i < _pNode->GetNumChildren(); i++)
		DeleteFrame_r(_pNode->GetChild(i));
}

void CXW_SurfaceSequence::ParseContainer(CKeyContainer& _Keys)
{
	MAUTOSTRIP(CXW_SurfaceSequence_ParseContainer, MAUTOSTRIP_VOID);
	int nKeys = _Keys.GetnKeys();
	for(int iKey = 0; iKey < nKeys; iKey++)
	{
		CStr Key = _Keys.GetKeyName(iKey).UpperCase();
		CStr Value = _Keys.GetKeyValue(iKey);

		if (Key == "REPEAT")
		{
			m_iRepSFrame = Value.GetStrSep(",").Val_int();
			m_iRepEFrame = Value.GetStrSep(",").Val_int();
		}
	}
}

void CXW_SurfaceSequence::ParseNode(CKeyContainerNode* _pNode)
{
	MAUTOSTRIP(CXW_SurfaceSequence_ParseNode, MAUTOSTRIP_VOID);
	if (CXW_Surface::IsAnimated(_pNode))
	{
		spCKeyContainerNode spNode = _pNode->Duplicate();
		CXW_SurfaceSequence::DeleteExcessSequences_r(spNode);
		ParseContainer(*spNode->GetKeys());
		
		int nFrames = CXW_Surface::GetNumKeyFrames_r(_pNode);
//LogFile(CStrF("    Frames %d", nFrames));
#ifdef	PLATFORM_CONSOLE
		SetNumKeyFrames(nFrames);
#else
		SetNumKeyFrames(1);
#endif	// PLATFORM_CONSOLE

		int i;
		for(i = 0; i < nFrames; i++)
		{
			if (i)
			{
#ifndef	PLATFORM_CONSOLE
				SetNumKeyFrames(i+1);
#endif	// PLATFORM_CONSOLE
				*GetKey(i) = *GetKey(i-1);
			}
//				m_lspKeyFrames.Add(m_lspKeyFrames[i-1]->Duplicate());
			spCKeyContainerNode spFrm = spNode->Duplicate();
			DeleteExcessFrames_r(spFrm);
			GetKey(i)->ParseNode(spFrm);
			DeleteFrame_r(spNode);
		}

		//We need to do this last
		for(i = 0;i < nFrames;i++)
		{
			GetKey(i)->MulSpecPower();
		}
	}
	else
	{
		ParseContainer(*_pNode->GetKeys());
		SetNumKeyFrames(1);
		GetKey(0)->ParseNode(_pNode);
		GetKey(0)->MulSpecPower();
	}
}

void CXW_SurfaceSequence::Initialize()
{
	MAUTOSTRIP(CXW_SurfaceSequence_Initialize, MAUTOSTRIP_VOID);
	m_TotTime = 0;
	m_RepSTime = 0;
	m_RepETime = 0;
	int nkeys = GetNumKeys();
	if (m_iRepSFrame < 0) m_iRepSFrame = 0;
	if (m_iRepEFrame < 0) m_iRepEFrame = 0;
/*	if (m_Flags & ANIM_SEQFLAGS_LOOP)
	{
		if (m_iRepSFrame == m_iRepEFrame)
		{
			m_Flags &= ~ANIM_SEQFLAGS_LOOP;
			m_iRepSFrame = 0;
			m_iRepEFrame = 0;
		}
	}*/

	if (!nkeys) return;
	if (m_iRepSFrame >= nkeys) m_iRepSFrame = nkeys-1;
	if (m_iRepEFrame >= nkeys) m_iRepEFrame = nkeys-1;

	if (m_iRepSFrame == m_iRepEFrame)
	{
		m_iRepSFrame = 0;
		m_iRepEFrame = 0;
	}

	for(int iFrm = 0; iFrm < nkeys; iFrm++)
	{
		CXW_SurfaceKeyFrame* pK = GetFrame(iFrm);
		if (iFrm < m_iRepSFrame) m_RepSTime += pK->m_Duration;
		if (iFrm < m_iRepEFrame) m_RepETime += pK->m_Duration;
		m_TotTime += pK->m_Duration;
	}
}

void CXW_SurfaceSequence::InitTextures(CTextureContext* pTC, bool _bReportFailure)
{
	MAUTOSTRIP(CXW_SurfaceSequence_InitTextures, MAUTOSTRIP_VOID);
	int nKeys = GetNumKeys();
	for(int i = 0; i < nKeys; i++)
		GetKey(i)->InitTextures(pTC, _bReportFailure);
}

/*void CXW_SurfaceSequence::SetTextureFlags(CTextureContext* pTC, int _Flags)
{
	MAUTOSTRIP(CXW_SurfaceSequence_SetTextureFlags, NULL);
	int nKeys = m_lspKeyFrames.Len();
	for(int i = 0; i < nKeys; i++)
		m_lspKeyFrames[i]->SetTextureFlags(pTC, _Flags);
}*/

void CXW_SurfaceSequence::SetTextureParam(CTextureContext* pTC, int _Param, int _Value)
{
	MAUTOSTRIP(CXW_SurfaceSequence_SetTextureParam, MAUTOSTRIP_VOID);
	int nKeys = GetNumKeys();
	for(int i = 0; i < nKeys; i++)
		GetKey(i)->SetTextureParam(pTC, _Param, _Value);
}

void CXW_SurfaceSequence::SetTextureParamfv(CTextureContext* pTC, int _Param, const fp4* _pValues)
{
	MAUTOSTRIP(CXW_SurfaceSequence_SetTextureParamfv, MAUTOSTRIP_VOID);
	int nKeys = GetNumKeys();
	for(int i = 0; i < nKeys; i++)
		GetKey(i)->SetTextureParamfv(pTC, _Param, _pValues);
}

// -------------------------------------------------------------------
void CXW_SurfaceSequence::Read(CCFile* _pFile)
{
	MAUTOSTRIP(CXW_SurfaceSequence_Read, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXW_SurfaceSequence::Read);
	uint16 Version = 0;
	_pFile->ReadLE(Version);

	switch(Version)
	{
	case XW_SURFSEQUENCE_VERSION :
		{
			_pFile->ReadLE(m_iRepSFrame);
			_pFile->ReadLE(m_iRepEFrame);
			_pFile->ReadLE(m_RepSTime);
			_pFile->ReadLE(m_RepETime);
			int32 nKeys = 0;
			_pFile->ReadLE(nKeys);
			SetNumKeyFrames(nKeys);

			for(int i = 0; i < nKeys; i++)
				GetKey(i)->Read(_pFile);
		}
		break;
	default :
		Error("Read", CStrF("Unsupported surface-sequence version %.4x", Version));
	}
	Initialize();
}

void CXW_SurfaceSequence::Write(CCFile* _pFile) const
{
	MAUTOSTRIP(CXW_SurfaceSequence_Write, MAUTOSTRIP_VOID);
	uint16 Version = XW_SURFSEQUENCE_VERSION;
	_pFile->WriteLE(Version);
	_pFile->WriteLE(m_iRepSFrame);
	_pFile->WriteLE(m_iRepEFrame);
	_pFile->WriteLE(m_RepSTime);
	_pFile->WriteLE(m_RepETime);
	int32 nKeys = GetNumKeys();
	_pFile->WriteLE(nKeys);
	for(int i = 0; i < nKeys; i++)
		GetKey(i)->Write(_pFile);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXW_Surface
|__________________________________________________________________________________________________
\*************************************************************************************************/
CXW_Surface::CXW_Surface()
{
	MAUTOSTRIP(CXW_Surface_ctor, MAUTOSTRIP_VOID);
	m_Flags = 0;
	m_iController = 0;
	m_iGroup = 0;

	m_OcclusionMask = 0;
	m_PriorityOffset = 0;
	m_Options = 0;
	m_Requirements = 0;
#ifndef XW_SURF_THIN
	m_SmoothGroup = 0;
	m_SmoothAngle = 30.0;
#endif

/*	m_nTextures = 0;
	m_nBumpMaps = 0;
	m_FogMaskTextureID = 0;
	m_Friction = 0.5f;
	m_Density = 1.0f;
	m_Elasticy = 0.0f;
	m_Priority = 0.0f;
	m_DamageType = 0.0f;
	m_Medium.SetSolid();*/
}

void CXW_Surface::Inherit(const CXW_Surface& _Surf)
{
	MAUTOSTRIP(CXW_Surface_Inherit, MAUTOSTRIP_VOID);
	m_Flags = _Surf.m_Flags;
	m_iController = _Surf.m_iController;
	m_iGroup = _Surf.m_iGroup;
	m_OcclusionMask = _Surf.m_OcclusionMask;
	m_PriorityOffset = _Surf.m_PriorityOffset;
	m_Options  = _Surf.m_Options;
	m_Requirements = _Surf.m_Requirements;
#ifndef XW_SURF_THIN
	m_SmoothGroup = _Surf.m_SmoothGroup;
	m_SmoothAngle = _Surf.m_SmoothAngle;
#endif

	m_lspSequences.SetLen(_Surf.m_lspSequences.Len());
	for(int i = 0; i < _Surf.m_lspSequences.Len(); i++)
		m_lspSequences[i] = _Surf.m_lspSequences[i]->Duplicate();


//	m_nTextures = _Surf.m_nTextures;
//	m_nBumpMaps = _Surf.m_nBumpMaps;
//	{ for(int i = 0; i  < m_nTextures; i++) m_lTextures[i] = _Surf.m_lTextures[i]; }
//	m_FogMaskName = _Surf.m_FogMaskName;
//	m_FogMaskTextureID = _Surf.m_FogMaskTextureID;
//	m_Friction = _Surf.m_Friction;
//	m_Density = _Surf.m_Density;
//	m_Elasticy = _Surf.m_Elasticy;
//	m_Priority = _Surf.m_Priority;
//	m_DamageType = _Surf.m_DamageType;
//	m_Medium = _Surf.m_Medium;

#ifdef XW_SURF_THIN
	m_Keys = _Surf.m_Keys;
#else
	if (D_MXDFCREATE && D_MPLATFORM != 0)
	{
		m_KeysThin = _Surf.m_KeysThin;
	}
	else
	{
		if (_Surf.m_spKeys != NULL)
		{
			m_spKeys = MNew(CKeyContainer);
			if (!m_spKeys) MemError("Inherit");
			*m_spKeys = *(const CKeyContainer*)_Surf.m_spKeys;
		}
		else
			m_spKeys = NULL;
	}
#endif
}

CXW_Surface* CXW_Surface::GetSurface(int _Options, int _Caps)
{
	MAUTOSTRIP(CXW_Surface_GetSurface, NULL);
	CXW_Surface* pBestSurf = NULL;
	int BestDiff = 32;
	CXW_Surface* pS = this;
	while(pS)
	{
		if ((pS->m_Requirements & _Caps) == pS->m_Requirements)
		{
			// Surface can be rendered
			int dOptions = (pS->m_Options ^ _Options)/* & (~(pS->m_Options | _Options))*/;
			if (!dOptions)
			{
				// Perfect match, don't check any more surfaces
				return pS;
			}

			int Diff = 0;
			for(int i = 0; i < 8; i++)
				if (dOptions & (1 << i)) Diff++;

			if (Diff < BestDiff)
			{
				pBestSurf = pS;
				BestDiff = Diff;
			}
		}
		pS = pS->m_spNextSurf;
	}

	if (!pBestSurf) 
		return this;

	return pBestSurf;
}

#ifndef XW_SURF_THIN
void CXW_Surface::AddSurfaceVersion(spCXW_Surface _spSurf)
{
	MAUTOSTRIP(CXW_Surface_AddSurfaceVersion, NULL);
	CXW_Surface* pSurf = this;
	while(pSurf->m_spNextSurf != NULL)
		pSurf = pSurf->m_spNextSurf;

	pSurf->m_spNextSurf = _spSurf;
}
#endif

CXW_SurfaceKeyFrame* CXW_Surface::GetBaseFrame()
{
	MAUTOSTRIP(CXW_Surface_GetBaseFrame, NULL);
	if (!m_lspSequences.Len()) Error("GetBaseFrame", "Surface have no sequences (" + m_Name + ")");
	return m_lspSequences[0]->GetBaseFrame(this);
}

const CXW_SurfaceKeyFrame* CXW_Surface::GetBaseFrame() const
{
	MAUTOSTRIP(CXW_Surface_GetBaseFrame2, NULL);
	if (!m_lspSequences.Len()) Error("GetBaseFrame", "Surface have no sequences (" + m_Name + ")");
	return m_lspSequences[0]->GetBaseFrame(this);
}

CXW_SurfaceKeyFrame* CXW_Surface::GetFrame(int _iSeq, const CMTime& _Time, CXW_SurfaceKeyFrame& _TmpStorage)
{
	MAUTOSTRIP(CXW_Surface_GetFrame, NULL);
	if (_iSeq <  0) _iSeq = 0;
	if (_iSeq >= m_lspSequences.Len()) _iSeq = m_lspSequences.Len()-1;

	return m_lspSequences[_iSeq]->GetFrame(_Time, _TmpStorage);
}

spCXW_Surface CXW_Surface::Duplicate() const
{
	MAUTOSTRIP(CXW_Surface_Duplicate, NULL);
	MSCOPESHORT(Duplicate);
	spCXW_Surface spSurf = MNew(CXW_Surface);
	if (!spSurf) MemError("Duplicate");

	spSurf->Inherit(*this);

	spSurf->m_Name = m_Name;

	if (m_spNextSurf != NULL)
		spSurf->m_spNextSurf = m_spNextSurf->Duplicate();

	return spSurf;
}

void CXW_Surface::Read(CCFile* _pFile)
{
	MAUTOSTRIP(CXW_Surface_Read, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXW_Surface::Read); //AR-SCOPE

	uint16 Version;
	_pFile->ReadLE(Version);

	switch(Version)
	{
	case 0x0102 :
		{
			_pFile->ReadLE(m_Flags);
			m_Name.Read(_pFile);
#ifdef XW_SURF_THIN
			CStr NodeName;
			NodeName.Read(_pFile);
#else
			if (D_MXDFCREATE && D_MPLATFORM != 0)
			{
				CStr NodeName;
				NodeName.Read(_pFile);
			}
			else
			{
				m_NodeName.Read(_pFile);
			}
#endif
			int16 nTextures, nBumpMaps;
			_pFile->ReadLE(nTextures);
			_pFile->ReadLE(nBumpMaps);

			SetNumSequences(1);
			m_lspSequences[0]->SetNumKeyFrames(1);
			CXW_SurfaceKeyFrame* pSKF = GetBaseFrame();

			pSKF->m_lTextures.SetLen(nTextures);
//			pSKF->m_lBumpMaps.SetLen(nBumpMaps);

			{ for(int i = 0; i < nTextures; i++) pSKF->m_lTextures[i].Read(_pFile); }
			CXW_SurfaceLayer Dummy;
			{ for(int i = 0; i < nBumpMaps; i++) Dummy.Read(_pFile); }

			fp4 Foo;
			int32 Foo32;
			_pFile->ReadLE(pSKF->m_Friction);
			_pFile->ReadLE(Foo);			// Was m_Density
			_pFile->ReadLE(pSKF->m_Elasticy);
			_pFile->ReadLE(Foo32);			// Was m_Priority
			_pFile->ReadLE(Foo32);			// m_DamageType
			pSKF->m_Medium.Read(_pFile);

#ifdef XW_SURF_THIN
			m_Keys.Read(_pFile);
#else
			if (D_MXDFCREATE && D_MPLATFORM != 0)
			{
				m_KeysThin.Read(_pFile);
			}
			else
			{
				m_spKeys = MNew(CKeyContainer);
				if (!m_spKeys) MemError("Read");
				m_spKeys->Read(_pFile);
			}
#endif
		}
		break;
	case 0x0200:
		{
			_pFile->ReadLE(m_Flags);
			m_Name.Read(_pFile);
#ifdef XW_SURF_THIN
			CStr NodeName;
			NodeName.Read(_pFile);
#else
			if (D_MXDFCREATE && D_MPLATFORM != 0)
			{
				CStr NodeName;
				NodeName.Read(_pFile);
			}
			else
			{
				m_NodeName.Read(_pFile);
			}
#endif
			uint32 BF0;
			_pFile->ReadLE(BF0);
			m_iController = BF0 & 1;
			m_iGroup = (BF0 >> 1) & 3;

			int32 nSeq = 0;
			_pFile->ReadLE(nSeq);
			m_lspSequences.SetLen(nSeq);
			for(int iSeq = 0; iSeq < m_lspSequences.Len(); iSeq++)
			{
				m_lspSequences[iSeq] = MNew(CXW_SurfaceSequence);
				if (!m_lspSequences[iSeq]) MemError("Read");
				m_lspSequences[iSeq]->Read(_pFile);
			}

		}
		break;

	case 0x0201:
		{
			_pFile->ReadLE(m_Flags);
			m_Name.Read(_pFile);
#ifdef XW_SURF_THIN
			CStr NodeName;
			NodeName.Read(_pFile);
#else
			if (D_MXDFCREATE && D_MPLATFORM != 0)
			{
				CStr NodeName;
				NodeName.Read(_pFile);
			}
			else
			{
				m_NodeName.Read(_pFile);
			}
#endif
			uint32 BF0;
			_pFile->ReadLE(BF0);
			m_iController = BF0 & 1;
			m_iGroup = (BF0 >> 1) & 3;

			int32 nSeq = 0;
			_pFile->ReadLE(nSeq);
			m_lspSequences.SetLen(nSeq);
			for(int iSeq = 0; iSeq < m_lspSequences.Len(); iSeq++)
			{
				m_lspSequences[iSeq] = MNew(CXW_SurfaceSequence);
				if (!m_lspSequences[iSeq]) MemError("Read");
				m_lspSequences[iSeq]->Read(_pFile);
			}

#ifdef XW_SURF_THIN
			m_Keys.Read(_pFile);
#else
			if (D_MXDFCREATE && D_MPLATFORM != 0)
			{
				m_KeysThin.Read(_pFile);
			}
			else
			{
				m_spKeys = MNew(CKeyContainer);
				if (!m_spKeys) MemError("Read");
				m_spKeys->Read(_pFile);
			}
#endif
		}
		break;

	case XW_SURFACE_VERSION :
		{
			_pFile->ReadLE(m_Flags);
			m_Name.Read(_pFile);
#ifdef XW_SURF_THIN
			CStr NodeName;
			NodeName.Read(_pFile);
#else
			if (D_MXDFCREATE && D_MPLATFORM != 0)
			{
				CStr NodeName;
				NodeName.Read(_pFile);
			}
			else
			{
				m_NodeName.Read(_pFile);
			}
#endif
			uint32 BF0;
			_pFile->ReadLE(BF0);
			m_iController = BF0 & 1;
			m_iGroup = (BF0 >> 1) & 3;

			_pFile->ReadLE(m_OcclusionMask);
			_pFile->ReadLE(m_PriorityOffset);
			_pFile->ReadLE(m_Options);
			_pFile->ReadLE(m_Requirements);
#ifdef XW_SURF_THIN
			uint64 Temp;
			_pFile->ReadLE(Temp);
#else
			if (D_MXDFCREATE && D_MPLATFORM != 0)
			{
				uint64 Temp;
				_pFile->ReadLE(Temp);
			}
			else
			{
				_pFile->ReadLE(m_SmoothGroup);
				_pFile->ReadLE(m_SmoothAngle);
			}
#endif

			int32 nSeq = 0;
			_pFile->ReadLE(nSeq);
			m_lspSequences.SetLen(nSeq);
			for(int iSeq = 0; iSeq < m_lspSequences.Len(); iSeq++)
			{
				MSCOPESHORT(Sequence);
				m_lspSequences[iSeq] = MNew(CXW_SurfaceSequence);
				if (!m_lspSequences[iSeq]) MemError("Read");
				m_lspSequences[iSeq]->Read(_pFile);
			}

			if (m_Flags & XW_SURFFLAGS_HASKEYS)
			{
				MSCOPESHORT(KeyContainer);
#ifdef XW_SURF_THIN
				m_Keys.Read(_pFile);
#else
				if (D_MXDFCREATE && D_MPLATFORM != 0)
				{
					m_KeysThin.Read(_pFile);
				}
				else
				{
					m_spKeys = MNew(CKeyContainer);
					if (!m_spKeys) MemError("Read");
					m_spKeys->Read(_pFile);
				}
#endif
			}

			if (m_Flags & XW_SURFFLAGS_HASNEXTSURF)
			{
				MSCOPESHORT(Surface);
				m_spNextSurf = MNew(CXW_Surface);
				if (!m_spNextSurf) MemError("Read");
				m_spNextSurf->Read(_pFile);
			}
		}

		break;

	default:
		Error("Read", CStrF("Unsupported surface-version %d.%d", Version >> 8, Version & 0xff));

	}

	Init();
//	for(int i = 0; i < m_Keys.GetnKeys(); i++)
//		LogFile(m_Keys.GetKeyName(i) + " = " + m_Keys.GetKeyValue(i));
}

void CXW_Surface::Write(CCFile* _pFile)
{
	MAUTOSTRIP(CXW_Surface_Write, MAUTOSTRIP_VOID);
#ifdef XW_SURF_THIN
	Error("Write", "Write is not avail.");

#else
	if (D_MXDFCREATE && D_MPLATFORM != 0)
	{
		Error("Write", "Write is not avail.");
	}
	if (m_Flags & XW_SURFFLAGS_GENERATED)
	{
		if (m_spNextSurf != NULL)
		{
			m_spNextSurf->Write(_pFile);
			return;
		}
		else
			Error("Write", CStrF("A generated surface cannot be last in surface chain. (Surface: %s)", m_Name.Str()));
	}


	uint16 Version = XW_SURFACE_VERSION;
	_pFile->WriteLE(Version);

	if (!m_spNextSurf)
		m_Flags &= ~XW_SURFFLAGS_HASNEXTSURF;
	else
		m_Flags |= XW_SURFFLAGS_HASNEXTSURF;

	if (m_spKeys != NULL && m_spKeys->GetnKeys())
		m_Flags |= XW_SURFFLAGS_HASKEYS;
	else
		m_Flags &= ~XW_SURFFLAGS_HASKEYS;

	_pFile->WriteLE(m_Flags);
	m_Name.Write(_pFile);
	m_NodeName.Write(_pFile);

	uint32 BF0 = (m_iController) | (m_iGroup << 1);
	_pFile->WriteLE(BF0);

	_pFile->WriteLE(m_OcclusionMask);
	_pFile->WriteLE(m_PriorityOffset);
	_pFile->WriteLE(m_Options);
	_pFile->WriteLE(m_Requirements);
	_pFile->WriteLE(m_SmoothGroup);
	_pFile->WriteLE(m_SmoothAngle);

	int32 nSeq = m_lspSequences.Len();
	_pFile->WriteLE(nSeq);
	for(int iSeq = 0; iSeq < m_lspSequences.Len(); iSeq++)
		m_lspSequences[iSeq]->Write(_pFile);

	if (m_Flags & XW_SURFFLAGS_HASKEYS)
		m_spKeys->Write(_pFile);

	if (m_spNextSurf != NULL)
		m_spNextSurf->Write(_pFile);


/*	Version 0x0102

	_pFile->WriteLE(m_Flags);
	m_Name.Write(_pFile);
	m_NodeName.Write(_pFile);
	_pFile->WriteLE(m_nTextures);
	_pFile->WriteLE(m_nBumpMaps);

	{ for(int i = 0; i < m_nTextures; i++) m_lTextures[i].Write(_pFile); }
	{ for(int i = 0; i < m_nBumpMaps; i++) m_lBumpMaps[i].Write(_pFile); }

	_pFile->WriteLE(m_Friction);
	_pFile->WriteLE(m_Density);
	_pFile->WriteLE(m_Elasticy);
	_pFile->WriteLE(m_Priority);
	_pFile->WriteLE(m_DamageType);
	m_Medium.Write(_pFile);

	m_Keys.Write(_pFile);*/
#endif
}

void CXW_Surface::Init()
{
	MAUTOSTRIP(CXW_Surface_Init, MAUTOSTRIP_VOID);
	
#ifdef XW_SURF_THIN
	for (int i=0; i<m_Keys.GetnKeys(); i++)
	{
		const char* pKeyName = m_Keys.GetKeyName(i);
		if (CStrBase::CompareNoCase(pKeyName, "OCCLUSIONMASK") == 0)
		{
			m_OcclusionMask = strtoul(m_Keys.GetKeyValue(i), NULL, 0);
		}
		else if (CStrBase::CompareNoCase(pKeyName, "DRAWORDER") == 0)
		{
			m_PriorityOffset = strtod(m_Keys.GetKeyValue(i), NULL);
		}
	}
#else
	if (D_MXDFCREATE && D_MPLATFORM != 0)
	{
		for (int i=0; i<m_KeysThin.GetnKeys(); i++)
		{
			const char* pKeyName = m_KeysThin.GetKeyName(i);
			if (CStrBase::CompareNoCase(pKeyName, "OCCLUSIONMASK") == 0)
			{
				m_OcclusionMask = strtoul(m_KeysThin.GetKeyValue(i), NULL, 0);
			}
			else if (CStrBase::CompareNoCase(pKeyName, "DRAWORDER") == 0)
			{
				m_PriorityOffset = strtod(m_KeysThin.GetKeyValue(i), NULL);
			}
		}
	}
	else
	{
		if (m_spKeys != NULL)
		{
			for(int i = 0; i < m_spKeys->GetnKeys(); i++)
			{
				CStr s = m_spKeys->GetKeyName(i);
		/*		if (s.CompareNoCase("FOGMASK") == 0)
				{
					pSKF->m_FogMaskName = m_Keys.GetKeyValue(i);
					pSKF->m_FogMaskName.MakeUpperCase();
				}
				else*/ if (s.CompareNoCase("OCCLUSIONMASK") == 0)
				{
					m_OcclusionMask = m_spKeys->GetKeyValue(i).Val_int();
				}
				else if (s.CompareNoCase("DRAWORDER") == 0)
				{
					m_PriorityOffset = m_spKeys->GetKeyValue(i).Val_fp8();
				}
			}
		}
	}
#endif	

	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	if (!pSC) Error("Init", "No surface-context available.");

	m_Flags &= ~XW_SURFFLAGS_SHADER;
	m_Flags |= XW_SURFFLAGS_SHADERONLY;
	m_Requirements = 0;
	for(int iSeq = 0; iSeq < m_lspSequences.Len(); iSeq++)
	{
		CXW_SurfaceSequence* pSeq = m_lspSequences[iSeq];
		pSeq->Initialize();
		for(int iKey = 0; iKey < pSeq->GetNumKeys(); iKey++)
		{
			CXW_SurfaceKeyFrame* pKey = pSeq->GetKey(iKey);

			// Check polygonoffset
			if (pKey->m_PolygonOffsetScale != 1.0f ||
				pKey->m_PolygonOffsetUnits != 0.0f)
				m_Flags |= XW_SURFFLAGS_POLYGONOFFSET;

			for(int iTxt = 0; iTxt < pKey->m_lTextures.Len(); iTxt++)
			{
				CXW_SurfaceLayer* pLayer = &pKey->m_lTextures[iTxt];

				// Check for lighting
				if (pLayer->m_Flags & XW_LAYERFLAGS_LIGHTING)
				{
					m_Flags |= XW_SURFFLAGS_LIGHTING;
					m_Flags |= XW_SURFFLAGS_NEEDDIFFUSE;
				}

				// Shader?
				if (pLayer->m_Type > XW_LAYERTYPE_RENDERSURFACE)
				{
					m_Flags |= XW_SURFFLAGS_SHADER;
				}
				else
					m_Flags &= ~XW_SURFFLAGS_SHADERONLY;

				// Check for bumpmap
				if (pLayer->m_Type > XW_LAYERTYPE_DIFFUSE)
				{
					pLayer->m_Flags |= XW_LAYERFLAGS_INVISIBLE;
					if (pLayer->m_Type == XW_LAYERTYPE_NORMAL)
						m_Flags |= XW_SURFFLAGS_HASBUMPMAP;
//					m_Flags |= XW_SURFFLAGS_NEEDTANGENTS;	// ?
				}

				// Check for reflection-map
#ifdef USE_HASHED_TEXTURENAME
				if (pLayer->m_TextureNameID == StringToHash("$REFLECTIONMAP"))
					m_Flags |= XW_SURFFLAGS_NEEDREFLECTIONMAP;
#else
				if (pLayer->GetTextureName() && strncmp(pLayer->GetTextureName(), "$REFLECTIONMAP", 14) == 0)
					m_Flags |= XW_SURFFLAGS_NEEDREFLECTIONMAP;
#endif				

				// Check for refraction-map
#ifdef USE_HASHED_TEXTURENAME
				if (pLayer->m_TextureNameID == StringToHash("$REFRACTIONMAP"))
					m_Flags |= XW_SURFFLAGS_NEEDREFRACTIONMAP;
#else
				if (pLayer->GetTextureName() && strncmp(pLayer->GetTextureName(), "$REFRACTIONMAP", 14) == 0)
					m_Flags |= XW_SURFFLAGS_NEEDREFRACTIONMAP;
#endif					

				// Init VB-Operator class IDs.
				for(int iOp = 0; iOp < pLayer->m_lOper.Len(); iOp++)
				{
					CXW_LayerOperation& Oper = pLayer->m_lOper[iOp];
					Oper.m_iOperatorClass = pSC->VBOperator_GetOperatorID(Oper.GetClass());

					CXR_VBOperator* pVBOper = pSC->VBOperator_Get(Oper.m_iOperatorClass);
					if (pVBOper)
						pVBOper->OnInitSurface(this, Oper);
					else
						m_Requirements |= XW_SURFREQ_OPERATOR_NA;
				}
			}

			// Init grouping channel nr
			{
				int iChannel = 0;
				for(int iTxt = pKey->m_lTextures.Len()-1; iTxt >= 0; iTxt--)
				{
					CXW_SurfaceLayer* pLayer = &pKey->m_lTextures[iTxt];
					if (pLayer->m_Flags & XW_LAYERFLAGS_GROUP)
						iChannel++;
					else
						iChannel = 0;

					pLayer->m_TexChannel = iChannel;
					if (iChannel >= 1) m_Requirements |= XW_SURFREQ_MULTITEX2;
					if (iChannel >= 2) m_Requirements |= XW_SURFREQ_MULTITEX3;
					if (iChannel >= 3) m_Requirements |= XW_SURFREQ_MULTITEX4;
					if (iChannel >= 4) m_Requirements |= XW_SURFREQ_MULTITEX8;
				}
			}
		}
	}

	if (m_Flags & XW_SURFFLAGS_SHADER)
		m_Options |= XW_SURFOPTION_SHADER;
	else
		m_Flags &= ~XW_SURFFLAGS_SHADERONLY;

	if (m_spNextSurf != NULL)
		m_spNextSurf->Init();
}

void CXW_Surface::InitTextures(bool _bReportFailure)
{
	MAUTOSTRIP(CXW_Surface_InitTextures, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("InitTextures", "No texture-context available.");

	InitTextures(pTC, _bReportFailure);
}

void CXW_Surface::InitTextures(CTextureContext* _pTC, bool _bReportFailure)
{
	MAUTOSTRIP(CXW_Surface_InitTextures_2, MAUTOSTRIP_VOID);
	int nSeq = m_lspSequences.Len();
	for(int i = 0; i < nSeq; i++)
		m_lspSequences[i]->InitTextures(_pTC, _bReportFailure);

	if (m_spNextSurf != NULL)
		m_spNextSurf->InitTextures(_pTC, _bReportFailure);
}

CVec3Dfp4 CXW_Surface::GetTextureMappingReferenceDimensions()
{
	MAUTOSTRIP(CXW_Surface_GetTextureMappingReferenceDimensions, CVec3Dfp4());
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("GetTextureMappingReferenceDimensions", "No texture-context available.");

	const CXW_SurfaceKeyFrame* pSKF = GetBaseFrame();

	int nTextures = pSKF->m_lTextures.Len();
	// Check for an explicitly definied reference layer.
	{
		for(int i = 0; i < nTextures; i++)
		{
			if (pSKF->m_lTextures[i].m_Flags & XW_LAYERFLAGS_MAPPINGREFERENCE)
			{
				int TxtID = pSKF->m_lTextures[i].m_TextureID;
				if (TxtID >= XW_SURFTEX_SPECIALBASE)
				{
					ConOutL(CStrF("§cf80WARNING: (CXW_Surface::GetTextureMappingReferenceDimensions) Layer with SPECIALBASE texture with MAPPINGREFERENCE property. (Surface: %s)", m_Name.Str()));
					break;
				}
				if (!TxtID)
				{
//					ConOutL(CStrF("§cf80WARNING: (CXW_Surface::GetTextureMappingReferenceDimensions) Layer with invalid texture with MAPPINGREFERENCE property. (Surface: %s)", m_Name.Str()));
					break;
				}

				CImage Desc; int nMipMaps;
				pTC->GetTextureDesc(TxtID, &Desc, nMipMaps);
				return CVec3Dfp4(Desc.GetWidth(), Desc.GetHeight(), 256);
			}
		}
	}

	// No layer was set, get an appropriate layer automatically.
	{
		CVec3Dfp4 Dim(256.0f);
		for(int i = 0; i < nTextures; i++)
		{
			int TxtID = pSKF->m_lTextures[i].m_TextureID;

			// "special" textures do not make good references
			if (TxtID >= XW_SURFTEX_SPECIALBASE)
				continue;

			if (TxtID)
			{
				if (pTC)
				{
					CImage Desc; int nMipMaps;
					CTC_TextureProperties Properies;
					pTC->GetTextureDesc(TxtID, &Desc, nMipMaps);
					pTC->GetTextureProperties(TxtID, Properies);

					// Cubemaps do not make good references.
					if (Properies.m_Flags & (CTC_TEXTUREFLAGS_CUBEMAP | CTC_TEXTUREFLAGS_CUBEMAPCHAIN))
						continue;

					Dim[0] = Desc.GetWidth();
					Dim[1] = Desc.GetHeight();
					break;
				}
			}		
		}

		return Dim;
	}
}

/*void CXW_Surface::SetTextureFlags(int _Flags)
{
	MAUTOSTRIP(CXW_Surface_SetTextureFlags, NULL);
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("InitTextures", "No texture-context available.");

	int nSeq = m_lspSequences.Len();
	for(int i = 0; i < nSeq; i++)
		m_lspSequences[i]->SetTextureFlags(pTC, _Flags);

	if (m_spNextSurf != NULL)
		m_spNextSurf->SetTextureFlags(_Flags);
}*/

void CXW_Surface::SetTextureParam(int _Param, int _Value)
{
	MAUTOSTRIP(CXW_Surface_SetTextureParam, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("InitTextures", "No texture-context available.");

	int nSeq = m_lspSequences.Len();
	for(int i = 0; i < nSeq; i++)
		m_lspSequences[i]->SetTextureParam(pTC, _Param, _Value);

	if (m_spNextSurf != NULL)
		m_spNextSurf->SetTextureParam(_Param, _Value);
}

void CXW_Surface::SetTextureParamfv(int _Param, const fp4* _pValues)
{
	MAUTOSTRIP(CXW_Surface_SetTextureParamfv, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("InitTextures", "No texture-context available.");

	int nSeq = m_lspSequences.Len();
	for(int i = 0; i < nSeq; i++)
		m_lspSequences[i]->SetTextureParamfv(pTC, _Param, _pValues);

	if (m_spNextSurf != NULL)
		m_spNextSurf->SetTextureParamfv(_Param, _pValues);
}

// -------------------------------------------------------------------
bool CXW_Surface::IsAnimated(CKeyContainer& _Keys)
{
	MAUTOSTRIP(CXW_Surface_IsAnimated, false);
	int nk = _Keys.GetnKeys();
	for(int i = 0; i < nk; i++)
	{
		CStr Value = _Keys.GetKeyValue(i);
		if ((Value.Find(";") >= 0) || (Value.Find("#") >= 0)) return true;
	}

	return false;
}

int CXW_Surface::GetNumSequences(CKeyContainer& _Keys)
{
	MAUTOSTRIP(CXW_Surface_GetNumSequences, 0);
	int nKeys = _Keys.GetnKeys();
	int nSeq = 1;
	for(int i = 0; i < nKeys; i++)
		nSeq = Max(nSeq, _Keys.GetKeyValue(i).GetNumMatches('#') + 1);
	return nSeq;
}

int CXW_Surface::GetNumSequences_r(CKeyContainerNode* _pNode)
{
	MAUTOSTRIP(CXW_Surface_GetNumSequences_r, 0);
	int nSeq = 1;
	nSeq = Max(nSeq, GetNumSequences(*_pNode->GetKeys()));

	for(int i = 0; i < _pNode->GetNumChildren(); i++)
	{
		CKeyContainerNode* pN = _pNode->GetChild(i);
		if (IsSurfaceChild(pN))
			nSeq = Max(nSeq, GetNumSequences_r(pN));
	}

	return nSeq;
}

int CXW_Surface::GetNumKeyFrames(CKeyContainer& _Keys)
{
	MAUTOSTRIP(CXW_Surface_GetNumKeyFrames, 0);
	int nKeys = _Keys.GetnKeys();
	int nKeyFrames = 1;
	for(int i = 0; i < nKeys; i++)
		nKeyFrames = Max(nKeyFrames, _Keys.GetKeyValue(i).GetNumMatches(';') + 1);
	return nKeyFrames;
}

int CXW_Surface::GetNumKeyFrames_r(CKeyContainerNode* _pNode)
{
	MAUTOSTRIP(CXW_Surface_GetNumKeyFrames_r, 0);
	int nKeyFrames = 1;
	nKeyFrames = Max(nKeyFrames, GetNumKeyFrames(*_pNode->GetKeys()));

	for(int i = 0; i < _pNode->GetNumChildren(); i++)
	{
		CKeyContainerNode* pN = _pNode->GetChild(i);
		if (IsSurfaceChild(pN))
			nKeyFrames = Max(nKeyFrames, GetNumKeyFrames_r(pN));
	}

	return nKeyFrames;
}

bool CXW_Surface::IsSurface(CKeyContainerNode* _pNode)
{
	MAUTOSTRIP(CXW_Surface_IsSurface, false);
	CKeyContainer* pKC = _pNode->GetKeys();
	if (pKC->GetKeyIndex("CLASSNAME") >= 0)
	{
		int iKey = pKC->GetKeyIndex("CLASSNAME");
		CStr Val = pKC->GetKeyValue(iKey).UpperCase();
		if (Val == "SURFACE")
			return true;
		else if (Val == "BLAYER")
			return true;
		else if (Val == "TLAYER")
			return true;
	}
	return false;
}

bool CXW_Surface::IsAnimated(CKeyContainerNode* _pNode)
{
	MAUTOSTRIP(CXW_Surface_IsAnimated_2, false);
	if (!IsSurface(_pNode)) return false;

	if (GetNumSequences(*_pNode->GetKeys()) > 1) return true;
	if (GetNumKeyFrames(*_pNode->GetKeys()) > 1) return true;

	for(int i = 0; i < _pNode->GetNumChildren(); i++)
	{
		CKeyContainerNode* pN = _pNode->GetChild(i);
		if (CXW_Surface::IsAnimated(pN)) return true;
	}
	return false;
}

void CXW_Surface::SetNumSequences(int _nSeq)
{
	MAUTOSTRIP(CXW_Surface_SetNumSequences, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXW_Surface::SetNumSequences);
	m_lspSequences.SetLen(_nSeq);
	for(int i = 0; i < _nSeq; i++)
		if (!m_lspSequences[i])
		{
			m_lspSequences[i] = MNew(CXW_SurfaceSequence);
			if (!m_lspSequences[i]) MemError("SetNumSequences");
		}
}

void CXW_Surface::DeleteSequence(CKeyContainer* _pKC)
{
	MAUTOSTRIP(CXW_Surface_DeleteSequence, MAUTOSTRIP_VOID);
	for(int i = _pKC->GetnKeys()-1; i >= 0; i--)
	{
		if(_pKC->GetKeyName(i) != "CLASSNAME")
		{
			int PosSeq = _pKC->GetKeyValue(i).Find("#");
			if (PosSeq >= 0) 
				_pKC->SetKeyValue(i, _pKC->GetKeyValue(i).Copy(PosSeq+1, 10000));
			else
				_pKC->DeleteKey(i);
		}
	}
}

void CXW_Surface::DeleteSequence_r(CKeyContainerNode* _pNode)
{
	MAUTOSTRIP(CXW_Surface_DeleteSequence_r, MAUTOSTRIP_VOID);
	DeleteSequence(_pNode->GetKeys());
	for(int i = 0; i < _pNode->GetNumChildren(); i++)
		DeleteSequence_r(_pNode->GetChild(i));
}

bool CXW_Surface::IsSurfaceChild(CKeyContainerNode* _pNode)
{
	MAUTOSTRIP(CXW_Surface_IsSurfaceChild, false);
	CKeyContainer* pKC = _pNode->GetKeys();
	if (pKC->GetKeyIndex("CLASSNAME") >= 0)
	{
		int iKey = pKC->GetKeyIndex("CLASSNAME");
		CStr Val = pKC->GetKeyValue(iKey).UpperCase();
		if (Val == "BLAYER")
			return true;
		else if (Val == "TLAYER")
			return true;
	}
	return false;
}

void CXW_Surface::DeleteNoSurfaceChildren(CKeyContainerNode* _pNode)
{
	MAUTOSTRIP(CXW_Surface_DeleteNoSurfaceChildren, MAUTOSTRIP_VOID);
	for(int i = _pNode->GetNumChildren()-1; i >= 0; i--)
	{
		CKeyContainerNode* pN = _pNode->GetChild(i);
		if (!IsSurfaceChild(pN))
			_pNode->DeleteChild(i);
	}
}

void CXW_Surface::ParseContainer(CKeyContainer& _Keys)
{
	MAUTOSTRIP(CXW_Surface_ParseContainer, MAUTOSTRIP_VOID);
	const char* g_SurfFlagsTranslate[] = 
	{
		"lighting", "transparent", "invisible", "reflection", 
		"delete", "needdiffuse", "needspecular", "neednormals", 
		"nocull", "sky", "nodynamiclight", "nowallmark", 
		"nolightmap", "$$$", "$$$", "$$$", 
		"needreflectionmap", "needrefractionmap", "nofog", "subdivision", 
		"needtangents", "$$$", "$$$", "keepallversions",
		
		(char*)NULL
	};

	const char* g_SurfOptionsTranslate[] = 
	{
		"simple0", "simple1", "simple2", "simple3", "hq0", "hq1", "hq2", "hq3", (char*)NULL
	};

	const char* g_SurfReqTranslate[] = 
	{
		"dualtex", "trippletex", "quadtex", (char*)NULL
	};

	static CXR_MediumDesc DummyMedium;

	int nKeys = _Keys.GetnKeys();
	for(int iKey = 0; iKey < nKeys; iKey++)
	{
		CStr Key = _Keys.GetKeyName(iKey).UpperCase();
		CStr Value = _Keys.GetKeyValue(iKey);
		{
			if (Key == "FLAGS")
			{
				if (Value[0] >= '0' && Value[0] <= '9')
					m_Flags = Value.Val_int();
				else
					m_Flags = Value.TranslateFlags(g_SurfFlagsTranslate);
			}
			else if (Key == "CONTROLLER")
				m_iController = Value.Val_int();
			else if (Key == "GROUPINDEX")
				m_iGroup = Value.Val_int();
			else if (Key == "NAME")
				m_Name = Value.UpperCase();
#ifndef XW_SURF_THIN
			else if (Key == "NODENAME")
				m_NodeName = Value.UpperCase();
#endif
			else if (Key == "OCCLUSIONMASK")
				m_OcclusionMask = Value.Val_int();
			else if (Key == "DRAWORDER")
				m_PriorityOffset = Value.Val_fp8();
			else if (Key == "OPTIONS")
			{
				if (Value[0] >= '0' && Value[0] <= '9')
					m_Options = Value.Val_int();
				else
					m_Options = Value.TranslateFlags(g_SurfOptionsTranslate);
			}
			else if (Key == "REQUIREMENTS")
			{
				if (Value[0] >= '0' && Value[0] <= '9')
					m_Requirements = Value.Val_int();
				else
					m_Requirements = Value.TranslateFlags(g_SurfReqTranslate);
			}
#ifndef XW_SURF_THIN
			else if (Key == "SMOOTHGROUP")
				m_SmoothGroup = Value.Val_int();
			else if (Key == "SMOOTHANGLE")
				m_SmoothAngle = Value.Val_fp8();
#endif
			else if (Key == "TEXTURE")
			{
				Error("ParseContainer_Surface", "TEXTURE key not supported in surface anymore.");
			}
			else if (
				Key == "CLASSNAME" ||
				Key == "REPEAT" ||
				Key == "DURATION" ||
				Key == "POLYGONOFFSET" ||
				Key == "XR_PARSE" ||
				Key == "MATERIALTYPE"
				)
			{
			}
			else if (DummyMedium.ParseKey(Key, Value))
			{
			}
	/*		else if (Key.CompareSubStr("OGR_") == 0)
			{
				// Ignore Ogier key
			}*/
			else
			{
				if (Key.Find("OGR_") != 0)
					LogFile("(CXW_Surface::ParseContainer) AddKey " + Key + " = " + Value);

#ifdef XW_SURF_THIN
				m_Keys.AddKey(Key, Value);
#else
				if (D_MXDFCREATE && D_MPLATFORM != 0)
				{
					m_KeysThin.AddKey(Key, Value);
				}
				else
				{
					if (!m_spKeys)
					{
						m_spKeys = MNew(CKeyContainer);
						if (!m_spKeys) MemError("ParseContainer");
					}
					m_spKeys->AddKey(Key, Value);
				}
#endif
			}
		}
	}
}

void CXW_Surface::ParseNode_r(CKeyContainerNode* _pNode, const CXW_Surface& _Surf, TList_Vector<spCXW_Surface>& _lspSurfaces)
{
	MAUTOSTRIP(CXW_Surface_ParseNode_r, MAUTOSTRIP_VOID);
	MSCOPESHORT(ParseNode_r);
	spCXW_Surface spSurf = MNew(CXW_Surface);
	if (!spSurf) Error_static("ParseNode_r", "Out of memory.");
	spSurf->Inherit(_Surf);

	spSurf->ParseContainer(*_pNode->GetKeys());

	if (CXW_Surface::IsAnimated(_pNode))
	{
//LogFile("Found animated surface." + spSurf->m_Name);
		int nSeq = CXW_Surface::GetNumSequences_r(_pNode);
//LogFile(CStrF("    Sequences %d", nSeq));
		spSurf->SetNumSequences(1);

		spCKeyContainerNode spNode = _pNode->Duplicate();
		DeleteNoSurfaceChildren(spNode);
		for(int i = 0; i < nSeq; i++)
		{
			if (i)
				spSurf->m_lspSequences.Add(spSurf->m_lspSequences[i-1]->Duplicate());
			spSurf->m_lspSequences[i]->ParseNode(spNode);
			CXW_Surface::DeleteSequence_r(spNode);
		}
	}
	else
	{
		spSurf->SetNumSequences(1);
		spSurf->m_lspSequences[0]->ParseNode(_pNode);
	}

//	spSurf->ParseContainer_Surface(*_pNode->GetKeys());
	bool IsSurf = true;

	{
		CKeyContainer* pKC = _pNode->GetKeys();
		if (pKC->GetKeyIndex("CLASSNAME") >= 0)
		{
			int iKey = pKC->GetKeyIndex("CLASSNAME");
			CStr Val = pKC->GetKeyValue(iKey).UpperCase();
			if (!Val.CompareNoCase("NODE") || !Val.CompareNoCase("SURFACES")) IsSurf = false;
		}
		else
			IsSurf = true;	// Fallback thingy
	}

	M_TRY
	{
		if (_pNode->GetNumChildren())
		{
			for(int i = 0; i < _pNode->GetNumChildren(); i++)
			{
				CKeyContainerNode* pN = _pNode->GetChild(i);
				CKeyContainer* pKC = pN->GetKeys();
				if (pKC->GetKeyIndex("CLASSNAME") >= 0)
				{
					int iKey = pKC->GetKeyIndex("CLASSNAME");
					CStr Val = pKC->GetKeyValue(iKey).UpperCase();

					if (Val == "BLAYER")
					{
/*						if (Surf.m_nBumpMaps >= XW_SURFACE_MAXBUMPMAPS)
							Error_static("ParseContainer_Surface", "Too many bumpmap layers.");

						Surf.ParseContainer_Layer(*pKC, Surf.m_lBumpMaps[Surf.m_nBumpMaps]);
						Surf.m_nBumpMaps++;*/
					}
					else if (Val == "TLAYER")
					{
/*						if (Surf.m_nTextures >= XW_SURFACE_MAXTEXTURES)
							Error_static("ParseContainer_Surface", "Too many texture layers.");

						Surf.ParseContainer_Layer(*pKC, Surf.m_lTextures[Surf.m_nTextures]);
						Surf.m_nTextures++;*/
					}
					else
						ParseNode_r(_pNode->GetChild(i), *spSurf, _lspSurfaces);
				}
				else
					ParseNode_r(_pNode->GetChild(i), *spSurf, _lspSurfaces);
			}
		}

		if (IsSurf)
		{
			_lspSurfaces.Add(spSurf);
//			LogFile(Surf.m_Name);
		}
	}
	M_CATCH(
	catch(CCException)
	{
		LogFile(CStrF("Error parsing tree '%s'", (char*) spSurf->m_Name));
		throw;
	}
	)
}

TList_Vector<spCXW_Surface> CXW_Surface::CombineSurfaces(TList_Vector<spCXW_Surface> _lspSurf)
{
	MAUTOSTRIP(CXW_Surface_CombineSurfaces, TList_Vector<spCXW_Surface>());
	TList_Vector<spCXW_Surface> lspSurf2;
	lspSurf2.SetGrow(256);

	int i = 0;
	while(i < _lspSurf.Len())
	{
		CStr Name = _lspSurf[i]->m_Name;
		spCXW_Surface spS = _lspSurf[i];
		lspSurf2.Add(spS);
		i++;
		while(i < _lspSurf.Len() && _lspSurf[i]->m_Name.CompareNoCase(Name) == 0)
		{
			spS->m_spNextSurf = _lspSurf[i];
			spS = _lspSurf[i];
//			LogFile("  Adding subsurface " + Name);
			i++;
		}
	}

	lspSurf2.OptimizeMemory();
	for(i = 0; i < lspSurf2.Len(); i++)
		lspSurf2[i]->Init();

	return lspSurf2;
}

void CXW_Surface::AddTextureName(CXW_Surface* _pS, CStr _dName)
{
#ifdef USE_HASHED_TEXTURENAME
	Error_static("CXW_Surface::AddTextureName", "Not yet implemented for TextureNameIDs..");
#else
	MAUTOSTRIP(CXW_Surface_AddTextureName, MAUTOSTRIP_VOID);
	// Append _dName to all texture names in all layers in all keyframes in all sequences.

	for(int iSeq = 0; iSeq < _pS->m_lspSequences.Len(); iSeq++)
	{
		CXW_SurfaceSequence* pSeq = _pS->m_lspSequences[iSeq];
		for(int iFrm = 0; iFrm < pSeq->GetNumKeys(); iFrm++)
		{
			CXW_SurfaceKeyFrame* pFrame = pSeq->GetKey(iFrm);
			for(int iL = 0; iL < pFrame->m_lTextures.Len(); iL++)
			{
				CXW_SurfaceLayer* pL = &pFrame->m_lTextures[iL];
				pL->SetTextureName(CStrF("%s%s", pL->GetTextureName(), (char*)_dName));
			}
		}
	}
#endif
}

TList_Vector<spCXW_Surface> CXW_Surface::ExpandLODSurfaces(TList_Vector<spCXW_Surface> _lspSurf)
{
	MAUTOSTRIP(CXW_Surface_ExpandLODSurfaces, TList_Vector<spCXW_Surface>());
	MSCOPESHORT(ExpandLODSurfaces);
	TList_Vector<spCXW_Surface> lspSurf2;
	lspSurf2.SetGrow(256);

	int i = 0;
	for(; i < _lspSurf.Len(); i++)
	{
		CXW_Surface* pS = _lspSurf[i];
#ifdef XW_SURF_THIN
		int iKey = pS->m_Keys.GetKeyIndex("GENLOD");
#else
		int iKey;
		if (D_MXDFCREATE && D_MPLATFORM != 0)
		{
			iKey = pS->m_KeysThin.GetKeyIndex("GENLOD");
		}
		else
		{
			iKey = (pS->m_spKeys != NULL) ? pS->m_spKeys->GetKeyIndex("GENLOD") : -1;
		}
#endif

		if (iKey >= 0)
		{
#ifdef XW_SURF_THIN 
			int nLOD = strtoul(pS->m_Keys.GetKeyValue(iKey), NULL, 0);
#else
			int nLOD;
			if (D_MXDFCREATE && D_MPLATFORM != 0)
				nLOD = strtoul(pS->m_KeysThin.GetKeyValue(iKey), NULL, 0);
			else
				nLOD = pS->m_spKeys->GetKeyValue(iKey).Val_int();
#endif
			for(int i = 0; i < nLOD; i++)
			{
				spCXW_Surface spLOD = pS->Duplicate();
				spLOD->m_Name = pS->m_Name + CStrF("L%d", i);
#ifdef XW_SURF_THIN
				spLOD->m_Keys.DeleteKey("GENLOD");
#else
				if (D_MXDFCREATE && D_MPLATFORM != 0)
					spLOD->m_KeysThin.DeleteKey("GENLOD");
				else
					spLOD->m_spKeys->DeleteKey("GENLOD");
#endif
				AddTextureName(spLOD, CStrF("L%d", i));
				lspSurf2.Add(spLOD);
			}
		}
		else
			lspSurf2.Add(_lspSurf[i]);
	}

	lspSurf2.OptimizeMemory();
	for(i = 0; i < lspSurf2.Len(); i++)
		lspSurf2[i]->Init();

	return lspSurf2;
}

TList_Vector<spCXW_Surface> CXW_Surface::ReadScript(TArray<uint8> _lData)
{
	CCFile MemFile;
	MemFile.Open(_lData, CFILE_READ);

	CKeyContainerNode Node;
	Node.ReadFromScript(&MemFile);

	TList_Vector<spCXW_Surface> lspSurfaces;
	lspSurfaces.SetGrow(256);

	CXW_Surface Surf;
	ParseNode_r(&Node, Surf, lspSurfaces);

	TList_Vector<spCXW_Surface> lspCombined = CombineSurfaces(lspSurfaces);
	TList_Vector<spCXW_Surface> lspExpanded = ExpandLODSurfaces(lspCombined);
	return lspExpanded;
}

TList_Vector<spCXW_Surface> CXW_Surface::ReadScript(CStr _Filename)
{
	MAUTOSTRIP(CXW_Surface_ReadScript, TList_Vector<spCXW_Surface>());
	// Mirror any changes here to CXR_SurfaceContext::AddSurfaces in XRSurface.cpp

	CKeyContainerNode Node;
	Node.ReadFromScript(_Filename);

	TList_Vector<spCXW_Surface> lspSurfaces;
	lspSurfaces.SetGrow(256);

	CXW_Surface Surf;
	ParseNode_r(&Node, Surf, lspSurfaces);

	TList_Vector<spCXW_Surface> lspCombined = CombineSurfaces(lspSurfaces);
	TList_Vector<spCXW_Surface> lspExpanded = ExpandLODSurfaces(lspCombined);
	return lspExpanded;
}

void CXW_Surface::Write(CCFile* _pFile, TList_Vector<spCXW_Surface>& _lspSurfaces) 
{
	MAUTOSTRIP(CXW_Surface_Write_2, MAUTOSTRIP_VOID);
	for(int iSurf = 0; iSurf < _lspSurfaces.Len(); iSurf++)
		_lspSurfaces[iSurf]->Write(_pFile);
}

void CXW_Surface::Read(CCFile* _pFile, TThinArray<spCXW_Surface>& _lspSurfaces, int _nSurf)
{
	MAUTOSTRIP(CXW_Surface_Read_2, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXW_Surface::Read(CCFile,TThinArray,int));

	_lspSurfaces.SetLen(_nSurf);
	for(int iSurf = 0; iSurf < _lspSurfaces.Len(); iSurf++)
	{
		_lspSurfaces[iSurf] = MNew(CXW_Surface);
		if (!_lspSurfaces[iSurf]) Error_static("CXW_Surface::Read", "Out of memory.");
		_lspSurfaces[iSurf]->Read(_pFile);

		spCXW_Surface spS = _lspSurfaces[iSurf];
		spCXW_Surface spRoot = NULL;
		spCXW_Surface spPrev = NULL;

		while(spS != NULL)
		{
			spCXW_Surface spSOpt = spS->CreateOptimizedSurfaces();
			if (spSOpt != NULL)
			{
				if (spRoot != NULL)
				{
					spPrev->m_spNextSurf = spSOpt;
					spPrev = spSOpt;
				}
				else
				{
					spRoot = spSOpt;
					spPrev = spSOpt;
				}
			}

			if (spRoot != NULL)
			{
				spPrev->m_spNextSurf = spS;
				spPrev = spS;
			}
			else
			{
				spRoot = spS;
				spPrev = spS;
			}

			spS = spS->m_spNextSurf;
		}

		_lspSurfaces[iSurf] = spRoot;
	}
}

void CXW_Surface::Read(CCFile* _pFile, TList_Vector<spCXW_Surface>& _lspSurfaces, int _nSurf)
{
	MAUTOSTRIP(CXW_Surface_Read_2, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXW_Surface::Read(CCFile,TList_Vector,int));

	_lspSurfaces.SetLen(_nSurf);
	for(int iSurf = 0; iSurf < _lspSurfaces.Len(); iSurf++)
	{
		_lspSurfaces[iSurf] = MNew(CXW_Surface);
		if (!_lspSurfaces[iSurf]) Error_static("CXW_Surface::Read", "Out of memory.");
		_lspSurfaces[iSurf]->Read(_pFile);

		spCXW_Surface spS = _lspSurfaces[iSurf];
		spCXW_Surface spRoot = NULL;
		spCXW_Surface spPrev = NULL;

		while(spS != NULL)
		{
			spCXW_Surface spSOpt = spS->CreateOptimizedSurfaces();
			if (spSOpt != NULL)
			{
				if (spRoot != NULL)
				{
					spPrev->m_spNextSurf = spSOpt;
					spPrev = spSOpt;
				}
				else
				{
					spRoot = spSOpt;
					spPrev = spSOpt;
				}
			}

			if (spRoot != NULL)
			{
				spPrev->m_spNextSurf = spS;
				spPrev = spS;
			}
			else
			{
				spRoot = spS;
				spPrev = spS;
			}

			spS = spS->m_spNextSurf;
		}

		_lspSurfaces[iSurf] = spRoot;
	}
}

int CXW_Surface::GetSurfaceIndex(TList_Vector<spCXW_Surface> _lspSurfaces, const char* _pSurfName)
{
	MAUTOSTRIP(CXW_Surface_GetSurfaceIndex, 0);
	int nS = _lspSurfaces.Len();
	for(int iS = 0; iS < nS; iS++)
		if (!_lspSurfaces[iS]->m_Name.CompareNoCase(_pSurfName)) return iS;
	return -1;
}


spCXW_Surface CXW_Surface::CreateOptimizedSurfaces()
{
	MAUTOSTRIP(CXW_Surface_CreateOptimizedSurfaces, NULL);

#ifdef NEVER
#if !( defined(PLATFORM_PS2) || defined( PLATFORM_DOLPHIN ) )
//#ifndef	PLATFORM_CONSOLE
	if (m_Requirements & (XW_SURFREQ_NV20 | XW_SURFREQ_NV10 | XW_SURFREQ_MULTITEX2 | XW_SURFREQ_MULTITEX3 | XW_SURFREQ_MULTITEX4 | XW_SURFREQ_MULTITEX8))
		return NULL;

	if (m_lspSequences.Len() != 1)
		return NULL;

	const CXW_SurfaceSequence& Seq = *m_lspSequences[0];

	if (Seq.GetNumKeys() != 1)
		return NULL;

	const CXW_SurfaceKeyFrame& Key = *(const CXW_SurfaceKeyFrame*)Seq.GetKey(0);

	if (Key.m_lTextures.Len() >= 2)
	{
		const CXW_SurfaceLayer* pL = Key.m_lTextures.GetBasePtr();
		if (pL[0].m_Type > XW_LAYERTYPE_RENDERSURFACE ||
			pL[1].m_Type > XW_LAYERTYPE_RENDERSURFACE)
			return NULL;
		if (pL[0].m_Flags & (XW_LAYERFLAGS_INVISIBLE) ||
			pL[1].m_Flags & (XW_LAYERFLAGS_INVISIBLE))
			return NULL;
		if (pL[0].m_RasterMode != CRC_RASTERMODE_NONE)
			return NULL;
		if (pL[1].m_Flags & XW_LAYERFLAGS_ALPHACOMPARE && 
			pL[1].m_AlphaFunc != CRC_COMPARE_ALWAYS)
			return NULL;
		if (pL[0].m_Flags & XW_LAYERFLAGS_ALPHACOMPARE && 
			pL[0].m_AlphaFunc != CRC_COMPARE_ALWAYS && 
			pL[1].m_ZFunc != CRC_COMPARE_EQUAL)
			return NULL;

		int RasterMode = pL[1].m_RasterMode;
		switch(RasterMode)
		{
		case CRC_RASTERMODE_NONE :
			{
			}
			break;
		case CRC_RASTERMODE_ALPHABLEND :
			{
			};
			break;

		case CRC_RASTERMODE_MULADD :
		case CRC_RASTERMODE_MULTIPLY2 :
			{
				if (pL[1].m_Flags & XW_LAYERFLAGS_LIGHTING)
					return NULL;
			}
			break;

//		case CRC_RASTERMODE_ONE_INVALPHA :

		default :
			return NULL;
		}

		spCXW_Surface spNewSurf = Duplicate();
		spNewSurf->m_Flags |= XW_SURFFLAGS_GENERATED;
		spNewSurf->m_Requirements |= XW_SURFREQ_GENERATED;

		CXW_SurfaceKeyFrame& Key = *spNewSurf->m_lspSequences[0]->GetKey(0);
		CXW_SurfaceLayer* pNewL = Key.m_lTextures.GetBasePtr();

		pNewL[0].m_Flags |= XW_LAYERFLAGS_GROUP;
		pNewL[0].m_Flags &= ~XW_LAYERFLAGS_ALPHACOMPARE;
		pNewL[1].m_Flags |= XW_LAYERFLAGS_CUSTOMTEXENV;
		pNewL[1].m_RasterMode = CRC_RASTERMODE_NONE;
		pNewL[0].m_AlphaFunc = CRC_COMPARE_ALWAYS;
		pNewL[1].m_AlphaFunc = pL[0].m_AlphaFunc;
		pNewL[1].m_AlphaRef = pL[0].m_AlphaRef;
		pNewL[1].m_ZFunc = pL[0].m_ZFunc;
		pNewL[0].m_ZFunc = CRC_COMPARE_ALWAYS;

		if (pL[0].m_Flags & XW_LAYERFLAGS_ALPHACOMPARE)
			pNewL[1].m_Flags |= XW_LAYERFLAGS_ALPHACOMPARE;

		if (pL[0].m_Flags & XW_LAYERFLAGS_LIGHTING)
			pNewL[1].m_Flags |= XW_LAYERFLAGS_LIGHTING;

		if (RasterMode == CRC_RASTERMODE_MULADD)
			if (pL[0].m_Flags & XW_LAYERFLAGS_LIGHTING)
				pNewL[1].m_Flags |= XW_LAYERFLAGS_LIGHTING;

		CXW_LayerOperation Oper;
		Oper.SetClass("MultiTxt");
		Oper.m_lParams.SetLen(1);
		Oper.m_lParams[0] = pL[1].m_RasterMode;
		pNewL[1].m_lOper.SetLen(pNewL[1].m_lOper.Len() + 1);
		pNewL[1].m_lOper[pNewL[1].m_lOper.Len()-1] = Oper;

		spNewSurf->Init();
		return spNewSurf;
	}
	else
#endif
#endif
		return NULL;
}

#ifndef USE_HASHED_TEXTURENAME
class CSurfaceTranslate
{
public:
	CSurfaceTranslate() {}
	CSurfaceTranslate( CStr _Original, CStr _Replacement )
	{
		m_Original = _Original;
		m_Replacement = _Replacement;
	}
	CStr m_Original;
	CStr m_Replacement;
};

spCXW_Surface CXW_Surface::CreateShaderFallbackSurface()
{
	MAUTOSTRIP(CXW_Surface_CreateShaderFallbackSurface, NULL);

	if (!(m_Flags & XW_SURFFLAGS_SHADER))
		Error("CreateShaderFallbackSurface", "Not a shader surface.");

	spCXW_Surface spS = Duplicate();
	if (!spS)
		MemError("CreateShaderFallbackSurface");
	if (spS->m_lspSequences.Len() < 1)
	{
		LogFile(CStrF("WARNING: No sequences in surface %s.", m_Name.Str()));
		return NULL;
	}

	int nSequences = m_lspSequences.Len();
	for( int iSeq = 0; iSeq < nSequences; iSeq++ )
	{
//		spS->m_lspSequences.SetLen(1);
		spS->m_Options = XW_SURFOPTION_LIGHTMAP;

		spCXW_SurfaceSequence spSeq = spS->m_lspSequences[iSeq];

		#ifdef XW_SURF_THIN
		int nKeyFrames = spSeq->m_lKeyFrames.Len();
		#else
		int nKeyFrames;
		if (D_MXDFCREATE && D_MPLATFORM != 0)
			nKeyFrames = spSeq->m_lKeyFramesThin.Len();
		else
			nKeyFrames = spSeq->m_lspKeyFrames.Len();

		#endif
		for( int iKey = 0; iKey < nKeyFrames; iKey++ )
		{
//TODO: Need to check so neither Diffuse, Normal or Specular are being animated (since we can't do that). All other layers may animate though
		#ifdef XW_SURF_THIN
//			spSeq->m_lKeyFrames.SetLen(1);
			CXW_SurfaceKeyFrame& Key = spSeq->m_lKeyFrames[iKey];
		#else
//			spSeq->m_lspKeyFrames.SetLen(1);
			CXW_SurfaceKeyFrame* pKey;
			if (D_MXDFCREATE && D_MPLATFORM != 0)
				pKey = &(spSeq->m_lKeyFramesThin[iKey]);
			else
				pKey = spSeq->m_lspKeyFrames[iKey];
			CXW_SurfaceKeyFrame& Key = *pKey;
		#endif

			Key.m_lTextures.SetLen(0);

			const CXW_SurfaceLayer* pL = GetBaseFrame()->m_lTextures.GetBasePtr();
			int nL = GetBaseFrame()->m_lTextures.Len();
			int nLNew = 0;

			const CXW_SurfaceLayer* pLDiff = NULL;
			const CXW_SurfaceLayer* pLSpec = NULL;

			for(int i = 0; i < nL; i++)
			{
				if (pL[i].m_Type == XW_LAYERTYPE_DIFFUSE)
					pLDiff = &pL[i];
				else if (pL[i].m_Type == XW_LAYERTYPE_SPECULAR)
					pLSpec = &pL[i];
			}

			bool bHasAddLayers = false;
			bool bHasNonAddLayers = false;

			TArray<CSurfaceTranslate> lSurfaceTranslation;

			for(int i = 0; i < nL; i++)
			{
				if (pL[i].m_Type == XW_LAYERTYPE_NORMAL)
				{
					CXW_SurfaceLayer Layer;

					const CXW_SurfaceLayer* pLNorm = &pL[i];
					CStr NormName = (char*)pLNorm->m_TextureName;
					NormName.MakeUpperCase();
					if (NormName.Right(2).Compare("_N") == 0)
					{
						CStr BaseName = NormName.Left(NormName.Len() - 2);
						BaseName.MakeUpperCase();
						int iRMap = 0;

						if (pLDiff)
						{
							CStr DiffName = (char*)pLDiff->m_TextureName;
							DiffName.MakeUpperCase();

							if (pLDiff->m_Flags & XW_LAYERFLAGS_ALPHACOMPARE)
							{
								Layer.m_AlphaFunc = pLDiff->m_AlphaFunc;
								Layer.m_AlphaRef = pLDiff->m_AlphaRef;
								Layer.m_Flags |= XW_LAYERFLAGS_ALPHACOMPARE;
							}

							iRMap = -1;

							if (DiffName.CompareSubStr(BaseName) == 0)
							{
								if (CStrF("%s_C", BaseName.GetStr()).Compare(DiffName) == 0)
								{
									iRMap = 0;
								}
								else
								{
									CStr DiffNameEnd = DiffName.Del(0, BaseName.Len());

									for(int iDMap = 1; iDMap < 64; iDMap++)
									{
										CStr DMapName1 = CStrF("_C%.2d", iDMap);
										CStr DMapName2 = CStrF("_C%d", iDMap);
										CStr DMapName3 = CStrF("_%.2d_C", iDMap);
										CStr DMapName4 = CStrF("_%d_C", iDMap);

										if (DMapName1 == DiffNameEnd ||
											DMapName2 == DiffNameEnd ||
											DMapName3 == DiffNameEnd ||
											DMapName4 == DiffNameEnd)
										{
											iRMap = iDMap;
											break;
										}
									}
								}
							}

							if (iRMap == -1)
							{
								LogFile(CStrF("WARNING: Diffuse map name does not follow naming conventions. (Surface %s, Normalmap %s, Diffusemap %s)", m_Name.Str(), NormName.Str(), DiffName.Str() ));
								return NULL;
 							}
						}

						CStr RenderMapName = (iRMap) ? CStrF("%s_R%.2d", BaseName.Str(), iRMap) : CStrF("%s_R", BaseName.Str());

						if( pLNorm ) lSurfaceTranslation.Add( CSurfaceTranslate( pLNorm->m_TextureName, RenderMapName ) );
						if( pLSpec ) lSurfaceTranslation.Add( CSurfaceTranslate( pLSpec->m_TextureName, RenderMapName ) );
						if( pLDiff ) lSurfaceTranslation.Add( CSurfaceTranslate( pLDiff->m_TextureName, RenderMapName ) );

						Layer.SetTextureName(RenderMapName);
						Layer.m_Flags |= XW_LAYERFLAGS_LIGHTING;
						// Carry mapping reference over to new layer (but remember that the Normalmap is the texture that decides what is used
						if( pLDiff ) Layer.m_Flags |= ( pLDiff->m_Flags & XW_LAYERFLAGS_MAPPINGREFERENCE );
						if( pLNorm ) Layer.m_Flags |= ( pLNorm->m_Flags & XW_LAYERFLAGS_MAPPINGREFERENCE );
						if( pLSpec ) Layer.m_Flags |= ( pLSpec->m_Flags & XW_LAYERFLAGS_MAPPINGREFERENCE );

						Key.m_lTextures.SetLen(nLNew + 1);
						Key.m_lTextures[nLNew] = Layer;
						nLNew++;
					}
					else
					{
						LogFile(CStrF("WARNING: Normal map name does not follow naming conventions. (Surface %s, Normalmap %s)", m_Name.Str(), NormName.Str()));
						return NULL;
					}
				}
				else if (pL[i].m_Type == XW_LAYERTYPE_RENDERSURFACE)
				{
					int i2;
					for( i2 = 0; i2 < lSurfaceTranslation.Len(); i2++ )
					{
						if( lSurfaceTranslation[i2].m_Original.Compare( pL[i].m_TextureName ) == 0 )
						{
							// This texture should be translated

							CXW_SurfaceLayer Layer;

							Layer = pL[i];
							strncpy( Layer.m_TextureName, lSurfaceTranslation[i2].m_Replacement.Str(), sizeof( Layer.m_TextureName ) );
							Key.m_lTextures.SetLen( nLNew + 1 );
							Key.m_lTextures[nLNew] = Layer;
							nLNew++;
							break;
						}
					}
					if( i2 == lSurfaceTranslation.Len() )
					{
						Key.m_lTextures.SetLen(nLNew + 1);
						Key.m_lTextures[nLNew] = pL[i];
						nLNew++;
					}

					switch( Key.m_lTextures[nLNew-1].m_RasterMode )
					{
					default:
						{
							bHasNonAddLayers = true;
							break;
						}

					case CRC_RASTERMODE_ADD:
					case CRC_RASTERMODE_ALPHAADD:
					case CRC_RASTERMODE_MULADD:
					case CRC_RASTERMODE_DESTADD:
						{
							bHasAddLayers = true;
							break;
						}
					}
				}
			}

			if( bHasAddLayers )
			{
				if( bHasNonAddLayers )
				{
					if( D_MPLATFORM == 2 )
						LogFile( CStrF( "Surface '%s' might not render correctly on PS2", spS->m_Name.Str() ) );
				}
			}
		}
	}

	spS->Init();
	return spS;
}
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CThinKeyContainer
|
| This is a replacement for the CKeyContainer used in CXW_Surface..
|
| Optimization note:
|  If the container only contains one key, m_lKeys will be empty 
|  and m_StrData will contain the two strings for that key.
|__________________________________________________________________________________________________
\*************************************************************************************************/
//#ifdef XW_SURF_THIN

CThinKeyContainer::CThinKeyContainer()
{
	MAUTOSTRIP( CThinKeyContainer_ctor, MAUTOSTRIP_VOID );
}


CThinKeyContainer::~CThinKeyContainer()
{
	MAUTOSTRIP( CThinKeyContainer_dtor, MAUTOSTRIP_VOID );
}


void CThinKeyContainer::Create(const TArray<CStr>& _lKeyNames, const TArray<CStr>& _lKeyValues)
{
	MAUTOSTRIP( CThinKeyContainer_Create, MAUTOSTRIP_VOID );
	int nKeys = _lKeyNames.Len();
	if (!nKeys)
		return;
	M_ASSERT(_lKeyValues.Len() == nKeys, "!");

	// calculate size needed.
	int nStrSize = 0;	
	int i;
	for (i=0; i<nKeys; i++)
	{
		nStrSize += _lKeyNames[i].Len()+1;
		nStrSize += _lKeyValues[i].Len()+1;
	}
	M_ASSERT(nStrSize < 65536, "Too large for 16-bit indices!");

	bool bMultipleKeys = (nKeys > 1);
	if (bMultipleKeys)
		m_lKeys.SetLen(nKeys);
	m_StrData.SetLen(nStrSize);

	// copy data
	int iStrIndex = 0;
	char* pStrData = (char*)m_StrData.GetBasePtr();
	for (i=0; i<nKeys; i++)
	{
		int nStrLen = _lKeyNames[i].Len();
		if (nStrLen > 0)
			strcpy(pStrData+iStrIndex, _lKeyNames[i]);
		else
			pStrData[iStrIndex] = 0;

		if (bMultipleKeys)
			m_lKeys[i].m_iName = iStrIndex;
		iStrIndex += nStrLen+1;

		nStrLen = _lKeyValues[i].Len();
		if (nStrLen > 0)
			strcpy(pStrData+iStrIndex, _lKeyValues[i]);
		else
			pStrData[iStrIndex] = 0;

		if (bMultipleKeys)
			m_lKeys[i].m_iValue = iStrIndex;
		iStrIndex += nStrLen+1;
	}
	M_ASSERT(iStrIndex == m_StrData.Len(), "!");
}


void CThinKeyContainer::operator= (const CThinKeyContainer& _KC)
{
	MAUTOSTRIP( CThinKeyContainer_operator_equal, MAUTOSTRIP_VOID );
	m_StrData = _KC.GetStrData();
	m_lKeys   = _KC.GetlKeys();
}


int CThinKeyContainer::AddKey(const CStr _KeyName, const CStr _KeyValue)
{
	MAUTOSTRIP( CThinKeyContainer_AddKey, 0 );
	int i = GetKeyIndex(_KeyName);
	if (i >= 0)
	{
		SetKeyValue(i, _KeyValue);
		return i;
	}

	if (HasOneSingleKey())
	{
		m_lKeys.SetLen(1);
		m_lKeys[0].m_iName  = 0;
		m_lKeys[0].m_iValue = (uint16)strlen((const char*)m_StrData.GetBasePtr());
	}

	int nStrSize = _KeyName.Len()+1 + _KeyValue.Len()+1;
	int iStrLast = m_StrData.Len();
	int iLast    = m_lKeys.Len();

	M_ASSERT(iStrLast+nStrSize < 65536, "Too large for 16-bit indices!");

	bool bMultipleKeys = (iLast > 0);
	if (bMultipleKeys)
		m_lKeys.SetLen(iLast+1);
	m_StrData.SetLen(iStrLast+nStrSize);

	// copy name
	char* pStrData = (char*)m_StrData.GetBasePtr();
	if (_KeyName.Len() > 0)
		strcpy(pStrData+iStrLast, _KeyName);
	else
		pStrData[iStrLast] = 0;

	if (bMultipleKeys)
		m_lKeys[iLast].m_iName = iStrLast;
	iStrLast += _KeyName.Len()+1;

	// copy value
	if (_KeyValue.Len() > 0)
		strcpy(pStrData+iStrLast, _KeyValue);
	else
		pStrData[iStrLast] = 0;

	if (bMultipleKeys)
		m_lKeys[iLast].m_iValue = iStrLast;

	return iLast;
}


void CThinKeyContainer::SetKeyValue(int _iIndex, CStr _Value)
{
	MAUTOSTRIP( CThinKeyContainer_SetKeyValue, MAUTOSTRIP_VOID );
	M_ASSERT(false, "Not supported!");
	Error_static("CThinKeyContainer::SetKeyValue", "Not supported!");
	
//	Key& k = m_lKeys[_iIndex];
//	int iStrOffset = 
	

/*
	M_ASSERT(_iIndex < m_lKeys.Len(), "Index out of range!");

	Key& k = m_lKeys[_iIndex];
	int nOldValSize = strlen(m_StrData.GetBasePtr()+k.m_iValue);
	int nNewValSize = _Value.Len();
	int nNewStrSize = m_StrData.Len() - nOldValSize + nNewValSize;
	M_ASSERT(nNewStrSize < 65536, "Too large for 16-bit indices!");

	if (nOldValSize == nNewValSize)
	{
		// we don't need to move anything. (yeah, like this ever happens..)
		strcpy(m_StrData.GetBasePtr()+k.m_iValue, _Value);
		return;
	}
	else if (nOldValSize < nNewValSize)
	{
		// we need to move everything forward
		int nOldStrSize = m_StrData.Len();
		m_StrData.SetLen(nNewStrSize);
		int nOffset = nNewStrSize - nOldStrSize;
		int iStart = m_lKeys[_iIndex+1].m_iName;
		memmove(m_StrData.GetBasePtr()+iStart+nOffset, m_StrData.GetBasePtr()+iStart, nOldStrSize-iStart);
		for (int 

	int nKeys = m_lKeys.Len();
	int iStrOffset = k.m_iValue;
	for (int i=_iIndex; i<nKeys; i++)
		

	// remove the old value
	
	int iStrData = 
	m_lKeys
*/
}


void CThinKeyContainer::DeleteKey(const CStr _KeyName)
{
	MAUTOSTRIP( CThinKeyContainer_DeleteKey_1, MAUTOSTRIP_VOID );
	M_ASSERT(false, "Not supported!");
	Error_static("CThinKeyContainer::DeleteKey", "Not supported!");
}


void CThinKeyContainer::DeleteKey(int _iIndex)
{
	MAUTOSTRIP( CThinKeyContainer_DeleteKey_2, MAUTOSTRIP_VOID );
	M_ASSERT(false, "Not supported!");
	Error_static("CThinKeyContainer::DeleteKey", "Not supported!");
}


int CThinKeyContainer::GetnKeys() const
{
	MAUTOSTRIP( CThinKeyContainer_GetnKeys, 0 );
	if (m_StrData.Len() && !m_lKeys.Len())
		return 1;
	else
		return m_lKeys.Len();
}


int CThinKeyContainer::GetKeyIndex(CStr _Key) const
{
	MAUTOSTRIP(CThinKeyContainer_GetKeyIndex, -1);

	if (HasOneSingleKey())
	{
		if (_Key.CompareNoCase((const char*)m_StrData.GetBasePtr()) == 0)
			return 0;
	}
	else
	{
		int nKeys = m_lKeys.Len();
		for (int i=0; i<nKeys; i++)
		{
			const Key& k = m_lKeys[i];
		 	const char* pCurr = (const char*)m_StrData.GetBasePtr()+k.m_iName;
			if (_Key.CompareNoCase(pCurr) == 0)
				return i;
		}
	}
	return -1;
}


const char* CThinKeyContainer::GetKeyName(int _iKey) const
{
	MAUTOSTRIP( CThinKeyContainer_GetKeyName, 0 );
	if (HasOneSingleKey())
	{
		M_ASSERT(_iKey == 0, "Index out of range!");
		return (const char*)m_StrData.GetBasePtr();
	}
	else
	{
		M_ASSERT(_iKey < m_lKeys.Len(), "Index out of range!");
		return (const char*)m_StrData.GetBasePtr() + m_lKeys[_iKey].m_iName;
	}
}


const char* CThinKeyContainer::GetKeyValue(int _iKey) const
{
	MAUTOSTRIP( CThinKeyContainer_GetKeyValue, 0 );
	if (HasOneSingleKey())
	{
		M_ASSERT(_iKey == 0, "Index out of range!");
		int nNameLen = (int)strlen((const char*)m_StrData.GetBasePtr());
		return (const char*)m_StrData.GetBasePtr() + nNameLen;
	}
	else
	{
		M_ASSERT(_iKey < m_lKeys.Len(), "Index out of range!");
		return (const char*)m_StrData.GetBasePtr() + m_lKeys[_iKey].m_iValue;
	}
}


void CThinKeyContainer::Read(CCFile* _pFile)
{
	MAUTOSTRIP( CThinKeyContainer_Read, MAUTOSTRIP_VOID );
	int32 nKeys = 0;
	_pFile->ReadLE(nKeys);
	bool bMultipleKeys = (nKeys > 1);
	if (bMultipleKeys)
		m_lKeys.SetLen(nKeys);

	// calculate size needed.
/*	int32 FilePos = _pFile->Pos();
	int nStrSize = 0;
	int i;
	for (i=0; i<nKeys; i++)
	{
		uint32 lk = 0;
		_pFile->ReadLE(lk);
		nStrSize += lk+1;
		_pFile->RelSeek(lk+1);

		uint32 lv = 0;
		_pFile->ReadLE(lv);
		nStrSize += lv+1;
		if (lv > 0)
			_pFile->RelSeek(lv+1);
	}
	_pFile->Seek(FilePos);*/

	// read data
	int MaxStrLen = 128*1024;
	m_StrData.SetLen(MaxStrLen);
	char* pStrData = (char*)m_StrData.GetBasePtr();
	int iStrOffset = 0;
	for (int i=0; i<nKeys; i++)
	{
		uint32 lk = 0;
		_pFile->ReadLE(lk);
		if (iStrOffset + lk+1 > MaxStrLen)
			Error_static(M_FUNCTION, "Out of space");

		_pFile->Read(pStrData+iStrOffset, lk+1);
		if (bMultipleKeys)
			m_lKeys[i].m_iName = iStrOffset;
		iStrOffset += lk+1;

		uint32 lv = 0;
		_pFile->ReadLE(lv);
		if (iStrOffset + lv+1 > MaxStrLen)
			Error_static(M_FUNCTION, "Out of space");
		if (lv > 0)
			_pFile->Read(pStrData+iStrOffset, lv+1);
		else
			pStrData[iStrOffset] = 0;
		if (bMultipleKeys)
			m_lKeys[i].m_iValue = iStrOffset;
		iStrOffset += lv+1;
	}
	m_StrData.SetLen(iStrOffset);
}

//#endif
