
#ifndef _INC_MRENDERVPGEN
#define _INC_MRENDERVPGEN

#include "../../MOS.h"

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Vertex program generator
					
	Author:			Magnus Högdahl
					
	Copyright:		Starbreeze Studios AB 2001
					
	History:		
		010831:		Created File
\*____________________________________________________________________________________________*/


// #define CRC_QUATMATRIXPALETTE
// #define CRC_SUPPORTCUBEVP

enum EMPFlags
{
	 EMPFlags_Normal = 0
	,EMPFlags_SpecialCubeVec = M_Bit(0)
	,EMPFlags_SpecialCubeTex = M_Bit(1)
	,EMPFlags_SpecialCubeTexScaleZ = M_Bit(2)
	,EMPFlags_SpecialCubeTexScale2 = M_Bit(3)
};

#ifdef PLATFORM_XENON
#define DEF_CRC_HARDCODEMULTITEXTURE
#endif

#ifdef PLATFORM_PS3
#define DEF_CRC_HARDCODEMULTITEXTURE
#endif

#if DEF_CRC_MAXTEXTURES <= 4
#define DEF_CRC_HARDCODEMULTITEXTURE
#endif

/*#ifdef DEF_CRC_HARDCODEMULTITEXTURE
#define DEF_CRC_VPFormat_MaxTextures DEF_CRC_MAXTEXCOORDS
#else
	#define DEF_CRC_VPFormat_MaxTextures m_ProgramGenFormat.m_MultiTexture
#endif*/

class CRC_VPFormat
{
public:
	enum
	{
		FMTBF0_TEXCOORDINPUTSCALE_SHIFT =	 0,		// "INPUTSCALE" is scaling and translation for compressed vertex attribute formats.
		FMTBF0_TEXCOORDINPUTSCALE_AND =	  0xff,
		FMTBF0_TEXCOORDINPUTSCALE_ANDSHIFTED = (FMTBF0_TEXCOORDINPUTSCALE_AND << FMTBF0_TEXCOORDINPUTSCALE_SHIFT),
		FMTBF0_TEXTUREMATRIX_SHIFT = 		 8,
		FMTBF0_TEXTUREMATRIX_AND =		  0xff,
		FMTBF0_TEXTUREMATRIX_ANDSHIFTED = (FMTBF0_TEXTUREMATRIX_AND << FMTBF0_TEXTUREMATRIX_SHIFT),
		FMTBF0_MWCOMP_SHIFT =			    16,
		FMTBF0_MWCOMP_AND =				  0x0f,
		FMTBF0_MWCOMP_ANDSHIFTED = (FMTBF0_MWCOMP_AND << FMTBF0_MWCOMP_SHIFT),
		FMTBF0_NONORMAL =			0x00100000,
		FMTBF0_USENORMAL =			0x00200000,
		FMTBF0_USETANGENTS =		0x00400000,
		FMTBF0_NORMALIZENORMAL =	0x00800000,
		FMTBF0_NOVERTEXCOLOR =		0x01000000,
		FMTBF0_NOCOLOROUTPUT =		0x02000000,
		FMTBF0_VERTEXINPUTSCALE =	0x04000000,
		FMTBF0_CLIPPLANES =			0x08000000,
		FMTBF0_DEPTHFOG =			0x10000000,
		FMTBF0_VERTEXLIGHTING =		0x20000000,
	};

	class CProgramFormat
	{
	public:

		enum
		{
			BITSIZE_LIGHTTYPES = 2,
			BITSIZE_TEXGENMODE = 5,
			BITSIZE_TEXINPUTRENAME = DNumBits(CRC_MAXTEXCOORDS - 1),

			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// 
			// WARNING
			// WARNING
			// WARNING
			// WARNING
			// WARNING
			// WARNING
			// WARNING
			// WARNING
			// WARNING
			// WARNING
			// WARNING
			// WARNING
			// WARNING
			// 
			// When you change the bitfields and members of this structure make sure that you change the version or xenon program
			// cache will break
			// 
			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			FORMAT_VERSION = 0x0104,
		};

		// Current size:
		//		PC		6*32 = 192bit, 24 bytes
		//		Xenon	5*32 = 160bit, 20 bytes
		//		PS3		4*32 = 128bit, 16 bytes


		uint32 m_TexInputRename : CRC_MAXTEXCOORDS * BITSIZE_TEXINPUTRENAME;					// 32, Xenon, PC, needs to be killed for PS3
		uint32 m_FmtBF0;																		// 32

		uint8 m_lTexGenMode[CRC_MAXTEXCOORDS];													// 64

#if defined PLATFORM_XENON || !defined PLATFORM_CONSOLE
		uint32 m_ActiveVertices;																// 32, PC, Xenon
#endif
#ifdef CRC_SUPPORTVERTEXLIGHTING
		uint32 m_LightTypes : CRC_MAXLIGHTS * BITSIZE_LIGHTTYPES;								// 16, PC
#endif
#ifdef CRC_SUPPORTVERTEXLIGHTING
		uint32 m_nLights : 4;																	// 4, PC
#endif
#ifdef CRC_SUPPORTCUBEVP
		uint32 m_MPFlags : 4;																	// 4, None
#endif

		uint32 GetTexCoordInputScale(uint32 _iTxt) const
		{
			return ((m_FmtBF0 >> FMTBF0_TEXCOORDINPUTSCALE_SHIFT) >> (_iTxt)) & 0x1;
		}
		uint32 GetRename(uint32 _iTxt) const
		{
			return ((m_TexInputRename >> (_iTxt * BITSIZE_TEXINPUTRENAME)) & ((1<<BITSIZE_TEXINPUTRENAME)-1));
		}
		uint32 GetTexGen(uint32 _iTxt) const
		{
			return m_lTexGenMode[_iTxt];
		}

		uint32 GetTexMatrixMask() const
		{
			return (m_FmtBF0 >> FMTBF0_TEXTUREMATRIX_SHIFT) & FMTBF0_TEXTUREMATRIX_AND;
		}

		uint32 GetTexCoordInputScaleMask() const
		{
			return (m_FmtBF0 >> FMTBF0_TEXCOORDINPUTSCALE_SHIFT) & FMTBF0_TEXCOORDINPUTSCALE_AND;
		}

		uint32 GetMWComp() const
		{
			return (m_FmtBF0 >> FMTBF0_MWCOMP_SHIFT) & FMTBF0_MWCOMP_AND;
		}

		void Clear()
		{
#ifdef CRC_SUPPORTVERTEXLIGHTING
			m_LightTypes = 0;
#endif
			m_TexInputRename = 0;
			m_FmtBF0 = 0;
#ifdef CRC_SUPPORTCUBEVP
			m_MPFlags = 0;
#endif
#ifdef PLATFORM_XENON
			m_ActiveVertices = 0;
#endif
			for(int i = 0; i < CRC_MAXTEXCOORDS; i++)
				m_lTexGenMode[i] = 0;
		}

		CFStr GetString() const
		{
			CFStr VPName;
			char* pD = VPName.GetStr();
			pD[0] = 'V';
			pD[1] = 'P';
			CStrBase::HexStr(pD + 2, this, sizeof(CProgramFormat));
			pD[2 + sizeof(CProgramFormat) * 2] = 0;
			return VPName;
		}
	};

	CProgramFormat m_ProgramGenFormat;

	uint16 m_nTexGenParams;
	int16 m_iConstant_Base;
#ifndef PLATFORM_XENON
	uint8 m_iConstant_ClipPlanes;
#endif
	uint8 m_iConstant_FogDepth;
#ifdef CRC_SUPPORTVERTEXLIGHTING
	uint8 m_iConstant_Lights;
#endif
	uint8 m_iConstant_PosTransform;
	uint8 m_iConstant_MP;
	uint8 m_iConstant_ConstantColor;
	uint8 m_iConstant_TexGenParam[CRC_MAXTEXCOORDS][4];
	uint8 m_iConstant_TexMatrix[CRC_MAXTEXCOORDS];
	uint8 m_iConstant_TexTransform[CRC_MAXTEXCOORDS];

	static uint8 ms_lnTexGenParams[CRC_TEXGENMODE_MAX];
	static uint32 ms_lTexGenFmtFlags[CRC_TEXGENMODE_MAX];

	static void InitRegsNeeded()
	{
		memset(&ms_lnTexGenParams, 0, sizeof(ms_lnTexGenParams));
		memset(&ms_lTexGenFmtFlags, 0, sizeof(ms_lTexGenFmtFlags));
		ms_lnTexGenParams[CRC_TEXGENMODE_LINEAR] = 4;
		ms_lnTexGenParams[CRC_TEXGENMODE_LINEARNHF] = 3;
		ms_lnTexGenParams[CRC_TEXGENMODE_BOXNHF] = 10;
		ms_lnTexGenParams[CRC_TEXGENMODE_SHADOWVOLUME2] = 1;
		ms_lnTexGenParams[CRC_TEXGENMODE_CONSTANT] = 1;
		ms_lnTexGenParams[CRC_TEXGENMODE_SHADOWVOLUME] = 1;
		ms_lnTexGenParams[CRC_TEXGENMODE_BUMPCUBEENV] = 8;
		ms_lnTexGenParams[CRC_TEXGENMODE_TSREFLECTION] = 1;
		ms_lnTexGenParams[CRC_TEXGENMODE_LIGHTING] = 2;
		ms_lnTexGenParams[CRC_TEXGENMODE_LIGHTING_NONORMAL] = 2;
		ms_lnTexGenParams[CRC_TEXGENMODE_LIGHTFIELD] = 6;
		ms_lnTexGenParams[CRC_TEXGENMODE_DECALTSTRANSFORM] = 4;
		ms_lnTexGenParams[CRC_TEXGENMODE_TSLV] = 1;
		ms_lnTexGenParams[CRC_TEXGENMODE_PIXELINFO] = 3;
		ms_lnTexGenParams[CRC_TEXGENMODE_DEPTHOFFSET] = 2;
		{
			for(int i = CRC_TEXGENMODE_FIRSTUSENORMAL; i < CRC_TEXGENMODE_MAX; i++)
				ms_lTexGenFmtFlags[i] |= FMTBF0_USENORMAL;
		}
		{
			for(int i = CRC_TEXGENMODE_FIRSTUSETANGENT; i < CRC_TEXGENMODE_MAX; i++)
				ms_lTexGenFmtFlags[i] |= FMTBF0_USETANGENTS;
		}
	}
	
/*	void Clear()
	{
		memset(&m_ProgramGenFormat, 0, sizeof(m_ProgramGenFormat));

#ifdef PLATFORM_XENON
		m_ProgramGenFormat.m_ActiveVertices = 0xffffffff;
#endif
	}*/
	CRC_VPFormat()
	{
		Create();
	}

	void Create(uint _iConstant_Base = 0)
	{
		FillChar(this, sizeof(*this), 0);
		//		Clear();
		m_ProgramGenFormat.m_TexInputRename = 0;
		for(uint32 l = 0; l < CRC_MAXTEXCOORDS; l++)
			m_ProgramGenFormat.m_TexInputRename |= (l) << (l * CProgramFormat::BITSIZE_TEXINPUTRENAME);

		m_iConstant_Base = _iConstant_Base;
		m_iConstant_MP = EMatrixPalette;
		m_iConstant_ConstantColor = EConstantColor;
//#ifndef PLATFORM_XENON
		m_iConstant_FogDepth = EAttrib;
//#endif
#ifdef CRC_SUPPORTVERTEXLIGHTING
		m_iConstant_Lights = ELights;
#endif
		/*
		m_nMWComp = 0;
		m_TexTransform = 0;
		m_Lighting = 0;
		m_nLights = 0;

		for(uint32 l = 0; l < CRC_MAXLIGHTS; l++)
		m_LightTypes[l] = CRC_LIGHTTYPE_POINT;

		for(uint32 i = 0; i < CRC_MAXTEXCOORDS; i++)
		{
		m_TexGenMode[i] = 0;
		m_iConstant_TexGenParam[i] = 0;
		m_iConstant_TexMatrix[i] = 0;
		}

		m_iConstant_MP = 0;
		m_iConstant_Lights = 0;
		*/
	}

#if defined(PLATFORM_XENON) || defined(PLATFORM_PS3)
	enum EMaxSizes
	{
		// All sizes has to be aligned on 4 registers
		ESizeInit = 2,
		ESizeConstantColor = 2,
		ESizeClipPlanes = 6,
		// These are unsupported at the moment
		ESizeLights = 0,
		ESizeAttrib = 0,
		ESizeTransform = 4 * 2 + 2,
		ESizeTexGenMatrix = CRC_MAXTEXCOORDS * 4 + CRC_MAXTEXCOORDS * 4,
		// 148 left for matrix palette
	};
#else
	enum EMaxSizes
	{
		ESizeInit = 2,								
		ESizeConstantColor = 1,
		ESizeLights = CRC_MAXLIGHTS * 2,
		ESizeAttrib = 1,
		ESizeTransform = 5 * 2,
		ESizeClipPlanes = 6,
		ESizeTexGenMatrix = CRC_MAXTEXCOORDS * 4 + CRC_MAXTEXCOORDS * 4,
		// 148 left for matrix palette
	};
#endif

	enum ERegisterStart
	{
		EStart = 8,
		EInit = EStart,								
		EConstantColor = EInit + ESizeInit,					
		ETexGenMatrix = ((EConstantColor + ESizeConstantColor) + 3) &(~3),
		EAttrib = ((ETexGenMatrix + ESizeTexGenMatrix) + 3) &(~3),		
		ETransform = ((EAttrib + ESizeAttrib) + 3) &(~3),				
		ELights = ((ETransform + ESizeTransform) + 3) &(~3),			
		EClipPlanes = ((ELights + ESizeLights) + 3) &(~3), // Align on 4 registers
		EMatrixPalette = ((EClipPlanes + ESizeClipPlanes) + 3) &(~3), 
	};
#if 0
	M_INLINE static uint32 GetUsedInputTextures(const CRC_Attributes* _pAttrib)
	{
		uint32 RetUsed = 0;
		for(uint32 iTxt = 0; iTxt < CRC_MAXTEXCOORDS; iTxt++)
		{
			uint32 TexGenMode = _pAttrib->m_lTexGenMode[iTxt];
			if (TexGenMode >= CRC_TEXGENMODE_FIRSTUSENORMAL)
				RetUsed |= 1 << CRC_MAXTEXCOORDS;

			if (TexGenMode >= CRC_TEXGENMODE_FIRSTUSETANGENT)
				RetUsed |= (1 << _pAttrib->m_iTexCoordSet[2] | 1 << _pAttrib->m_iTexCoordSet[3]);

			switch(TexGenMode)
			{
			case CRC_TEXGENMODE_TEXCOORD:
				if (_pAttrib->m_TextureID[iTxt])
					RetUsed |= 1 << _pAttrib->m_iTexCoordSet[iTxt];
				break;
			case CRC_TEXGENMODE_ENV:
			case CRC_TEXGENMODE_SHADOWVOLUME2:
				RetUsed |= 1 << _pAttrib->m_iTexCoordSet[iTxt];
				break;
			case CRC_TEXGENMODE_BUMPCUBEENV :
			case CRC_TEXGENMODE_TSREFLECTION :
			case CRC_TEXGENMODE_TSLV :
			case CRC_TEXGENMODE_PIXELINFO :
				RetUsed |= 1 << _pAttrib->m_iTexCoordSet[iTxt];
				break;

			case CRC_TEXGENMODE_DEPTHOFFSET :
				RetUsed |= 1 << _pAttrib->m_iTexCoordSet[iTxt];
				break;

/*			case CRC_TEXGENMODE_LINEAR:
			case CRC_TEXGENMODE_LINEARNHF:
			case CRC_TEXGENMODE_SHADOWVOLUME:
			case CRC_TEXGENMODE_SHADOWVOLUME2:
			case CRC_TEXGENMODE_BOXNHF:
			case CRC_TEXGENMODE_NORMALMAP:
			case CRC_TEXGENMODE_REFLECTION:
			case CRC_TEXGENMODE_TSLV:
			case CRC_TEXGENMODE_BUMPCUBEENV:
			case CRC_TEXGENMODE_VOID:
			case CRC_TEXGENMODE_TSREFLECTION:
			case CRC_TEXGENMODE_TSEV:
			case CRC_TEXGENMODE_ENV2:
			case CRC_TEXGENMODE_SCREEN:
			case CRC_TEXGENMODE_MSPOS:
				break;*/
			}
		}

		return RetUsed;

	}
#endif 

	void SetAttrib(const CRC_Attributes* _pAttrib, const CMat4Dfp32** _pMatrix, uint32 _EnabledTextures = 0xffffffff)
	{
		uint32 nRegsNeeded = 0;
		uint32 iTextMask = 1;
#if CRC_MAXTEXCOORDS == 8
		const static uint32 ActiveTexturesMask[CRC_MAXTEXCOORDS] = {(1 << (2 + 0)), (1 << (2 + 1)), (1 << (2 + 2)), (1 << (2 + 3)), (1 << (2 + 4)), (1 << (2 + 5)), (1 << (2 + 6)), (1 << (2 + 7))};
#else
		uint32 ActiveTexturesMask[CRC_MAXTEXCOORDS];
		for(uint32 iTxt = 0; iTxt < CRC_MAXTEXCOORDS; iTxt++)
		{
			ActiveTexturesMask[iTxt] = (1 << (2 + iTxt));
		}
#endif

		uint32 FmtBF0 = m_ProgramGenFormat.m_FmtBF0;
//		uint32 bUseNormal = 0;
//		uint32 bUseTangents = 0;
		for(uint32 iTxt = 0; iTxt < CRC_MAXTEXCOORDS; iTxt++, iTextMask = iTextMask << 1)
		{
//			M_ASSERT(_pAttrib->m_lTexGenMode[iTxt] <= 255, "No more than 8 bits allowed, increase to x bit percomponent");
//			m_ProgramGenFormat.m_TexGenMode |= (_pAttrib->m_lTexGenMode[iTxt] << (iTxt * CProgramFormat::BITSIZE_TEXGENMODE));

			const CMat4Dfp32* pMatrix = _pMatrix[iTxt];
			if (pMatrix)
				nRegsNeeded += 4;

			uint32 TexGenMode = _pAttrib->m_lTexGenMode[iTxt];
#ifdef PLATFORM_XENON
/*			if (!TexGenMode && !(m_ProgramGenFormat.m_ActiveVertices & ActiveTexturesMask[_pAttrib->m_iTexCoordSet[iTxt]])
				 && (!(_EnabledTextures & (iTextMask))))
				TexGenMode = CRC_TEXGENMODE_VOID;*/
			if (!TexGenMode && (!(_EnabledTextures & (iTextMask))))
				TexGenMode = CRC_TEXGENMODE_VOID;
#endif

			m_ProgramGenFormat.m_lTexGenMode[iTxt] = TexGenMode;

/*			if (TexGenMode >= CRC_TEXGENMODE_FIRSTUSENORMAL)
				bUseNormal = 1;
			if (TexGenMode >= CRC_TEXGENMODE_FIRSTUSETANGENT)
				bUseTangents = 1;
*/
			M_ASSERT(TexGenMode < CRC_TEXGENMODE_MAX, "!");

			FmtBF0 |= ms_lTexGenFmtFlags[TexGenMode];
			nRegsNeeded += ms_lnTexGenParams[TexGenMode];
		}

//		m_ProgramGenFormat.m_bUseNormal = bUseNormal;
//		m_ProgramGenFormat.m_bUseTangents = bUseTangents;

		m_nTexGenParams = nRegsNeeded;

#ifndef PLATFORM_XENON
		if (_pAttrib->m_Flags & CRC_FLAGS_FOG)
			FmtBF0 |= FMTBF0_DEPTHFOG;
//			m_ProgramGenFormat.m_FogDepth = 1;
#endif
		m_ProgramGenFormat.m_FmtBF0 = FmtBF0;
	}

	/*
	Not used any more, the has to be set through set attrib
	M_INLINE void SetTexGenOff(uint32 _iTxt)
	{
		m_ProgramGenFormat.m_TexGenMode = (m_ProgramGenFormat.m_TexGenMode & (~(15 << (_iTxt * 4)))) | (CRC_TEXGENMODE_VOID << (_iTxt * 4));
	}

	M_INLINE void SetTexGenOn(uint32 _iTxt, const CRC_Attributes* _pAttrib)
	{
		M_ASSERT(_pAttrib->m_lTexGenMode[iTxt] <= 15, "No more than 4 bits allowed, increase to 8 bit percomponent");
		m_ProgramGenFormat.m_TexGenMode = (m_ProgramGenFormat.m_TexGenMode & (~(15 << (_iTxt * 4)))) | (_pAttrib->m_lTexGenMode[_iTxt] << (_iTxt * 4));
	}
	*/

#ifdef CRC_SUPPORTVERTEXLIGHTING
	M_INLINE void SetLights(const CRC_Light* _pLights, uint32 _nLights)
	{
		m_ProgramGenFormat.m_nLights = _nLights;
		m_ProgramGenFormat.m_LightTypes = 0;
		m_ProgramGenFormat.m_FmtBF0 |= FMTBF0_VERTEXLIGHTING | FMTBF0_USENORMAL | FMTBF0_NORMALIZENORMAL;

		for(uint32 i = 0; i < _nLights; i++)
			m_ProgramGenFormat.m_LightTypes |= _pLights[i].m_Type << (i*CProgramFormat::BITSIZE_LIGHTTYPES);
	}

#endif

	M_INLINE void SetVertexTransform()
	{
		m_ProgramGenFormat.m_FmtBF0 |= FMTBF0_VERTEXINPUTSCALE;
	}

	M_INLINE void SetTexCoordInputScale(uint32 _iTxt)
	{
		m_ProgramGenFormat.m_FmtBF0 |= (1 << (_iTxt + FMTBF0_TEXCOORDINPUTSCALE_SHIFT));
	}

	M_INLINE void SetTextureTransformFull(uint32 _Transform)
	{
		M_ASSERT((m_ProgramGenFormat.m_FmtBF0 & FMTBF0_TEXCOORDINPUTSCALE_AND) == 0, "!");
		m_ProgramGenFormat.m_FmtBF0 |= (m_ProgramGenFormat.m_FmtBF0 & ~FMTBF0_TEXCOORDINPUTSCALE_ANDSHIFTED) + (_Transform << FMTBF0_TEXCOORDINPUTSCALE_SHIFT);
	}

	M_INLINE void SetTransformFull()
	{
		m_ProgramGenFormat.m_FmtBF0 |= FMTBF0_VERTEXINPUTSCALE | FMTBF0_TEXCOORDINPUTSCALE_ANDSHIFTED;
	}

	M_INLINE void SetVertexColorComponent(bool _bColorVertex)
	{
		m_ProgramGenFormat.m_FmtBF0 = (m_ProgramGenFormat.m_FmtBF0 & ~FMTBF0_NOVERTEXCOLOR) | ((_bColorVertex) ? 0 : FMTBF0_NOVERTEXCOLOR);
	}

	M_INLINE void SetColorOutput(bool _bColor)
	{
		m_ProgramGenFormat.m_FmtBF0 = (m_ProgramGenFormat.m_FmtBF0 & ~FMTBF0_NOCOLOROUTPUT) | ((_bColor) ? 0 : FMTBF0_NOCOLOROUTPUT);
	}

#if defined PLATFORM_XENON || !defined PLATFORM_CONSOLE
	M_INLINE void SetActiveVertices(uint32 _ActivecVertices)
	{
		m_ProgramGenFormat.m_ActiveVertices = _ActivecVertices;
	}
#endif

	M_INLINE static uint32 UpdateSetTexRename(uint32 _Start, uint32 _iTxt, uint32 _iRenameTo)
	{
		return (_Start & (~(((1<<CProgramFormat::BITSIZE_TEXINPUTRENAME)-1) << (_iTxt * CProgramFormat::BITSIZE_TEXINPUTRENAME)))) | _iRenameTo << (_iTxt * CProgramFormat::BITSIZE_TEXINPUTRENAME);
	}

	M_INLINE void SetTexRenameFull(uint32 _Rename)
	{
		m_ProgramGenFormat.m_TexInputRename = _Rename;
	}

	M_INLINE void SetTexRename(uint32 _iTxt, uint32 _iRenameTo)
	{
//		m_ProgramGenFormat.m_TexInputRename = (m_ProgramGenFormat.m_TexInputRename & (~(3 << (_iTxt * 2)))) | _iRenameTo << (_iTxt * 2); 
		m_ProgramGenFormat.m_TexInputRename = (m_ProgramGenFormat.m_TexInputRename & (~(((1<<CProgramFormat::BITSIZE_TEXINPUTRENAME)-1) << (_iTxt * CProgramFormat::BITSIZE_TEXINPUTRENAME)))) | _iRenameTo << (_iTxt * CProgramFormat::BITSIZE_TEXINPUTRENAME); 
	}

	M_INLINE void SetMWComp(uint32 _nMWComp)
	{
		m_ProgramGenFormat.m_FmtBF0 = (m_ProgramGenFormat.m_FmtBF0 & ~FMTBF0_MWCOMP_ANDSHIFTED) | (_nMWComp << FMTBF0_MWCOMP_SHIFT);
	}

#ifdef CRC_SUPPORTCUBEVP
	M_INLINE void SetMPFlags(uint32 _Flags)
	{
		m_ProgramGenFormat.m_MPFlags = _Flags;
	}
#endif

#ifndef PLATFORM_XENON
	M_INLINE void SetClipPlanes(uint32 _bEnabled)
	{
		m_ProgramGenFormat.m_FmtBF0 = (m_ProgramGenFormat.m_FmtBF0 & ~FMTBF0_CLIPPLANES) | ((_bEnabled) ? FMTBF0_CLIPPLANES : 0);
	}
#endif

	M_INLINE void SetTexMatrix(uint32 _iTxt)
	{
		m_ProgramGenFormat.m_FmtBF0 |= (1 << FMTBF0_TEXTUREMATRIX_SHIFT) << _iTxt;
	}

	M_INLINE uint32 UpdateSetTexMatrix(uint32 _Value, uint32 _iTxt)
	{
		return _Value | 1 << _iTxt;
	}

	M_INLINE void SetTexMatrixFull(uint32 _Value)
	{
		m_ProgramGenFormat.m_FmtBF0 = (m_ProgramGenFormat.m_FmtBF0 & ~FMTBF0_TEXTUREMATRIX_ANDSHIFTED) | (_Value << FMTBF0_TEXTUREMATRIX_SHIFT);
	}

	uint32 SetRegisters_Init(CVec4Dfp32* _pRegisters, uint32 _iReg);

	M_INLINE uint32 SetRegisters_ConstantColor(CVec4Dfp32* _pRegisters, uint32 _iReg, uint32 _Color)
	{
		m_iConstant_ConstantColor = _iReg;
		vec128 col = M_VMul(M_VLd_Pixel32_f32(&_Color), M_VScalar(1.0f / 255.0f));
		_pRegisters[0].v = col;

/*		(*_pRegisters)[0] = fp32(_Color.GetR()) / 255.0f;
		(*_pRegisters)[1] = fp32(_Color.GetG()) / 255.0f;
		(*_pRegisters)[2] = fp32(_Color.GetB()) / 255.0f;
		(*_pRegisters)[3] = fp32(_Color.GetA()) / 255.0f;*/
		return 1;
	}


	M_INLINE uint32 SetRegisters_Attrib(CVec4Dfp32* _pRegisters, uint32 _iReg, const CRC_Attributes* _pAttrib, class CRC_Core* _pRC)
	{
		uint32 iReg = _iReg;
//#ifndef PLATFORM_XENON
		if (m_ProgramGenFormat.m_FmtBF0 & FMTBF0_DEPTHFOG)
		{
		// compute fog factor f = (fog_end - dist)*(1/(fog_end-fog_start))
		// this is == to: (fog_end/(fog_end-fog_start) - dist/(fog_end-fog_start)
			(*_pRegisters)[0] = _pAttrib->m_FogEnd / (_pAttrib->m_FogEnd - _pAttrib->m_FogStart);
			(*_pRegisters)[1] = 1.0 / (_pAttrib->m_FogEnd - _pAttrib->m_FogStart);
			(*_pRegisters)[2] = 0;
			(*_pRegisters)[3] = 0;
			m_iConstant_FogDepth = iReg;
			iReg++;
		}
//#endif
		return iReg - _iReg;
	}

	static uint32 GetFirstFreeRegister() { return 8; };

	template <class t_CRegisterBatchGetter>
	uint32 SetRegisters_ClipPlanes(CVec4Dfp32* _pRegisters, uint32 _iReg, CPlane3Dfp32* _pClipPlanes, uint32 _nPlanes)
	{
		m_iConstant_ClipPlanes = _iReg;

		uint32 nRegs = 6;
		CVec4Dfp32 *pReg = _pRegisters;
		uint32 nLeft = t_CRegisterBatchGetter::NewRegisterBatch(pReg, nRegs);
		M_ASSERT(nLeft >= nRegs, "Only supported");
		M_ASSERT(_nPlanes <= nRegs, "!");

		for(uint32 i = 0; i < _nPlanes; i++)
		{
			const CPlane3Dfp32& Plane = _pClipPlanes[i];
			pReg[i][0] = Plane.n[0];
			pReg[i][1] = Plane.n[1];
			pReg[i][2] = Plane.n[2];
			pReg[i][3] = Plane.d;
		}

		return nRegs;
	}

	template <class t_CRegisterBatchGetter>
	uint32 SetRegisters_VertexInputScale(CVec4Dfp32* &_pRegisters, uint32 _iReg, const CRC_VRegTransform *_pTransform)
	{
		uint32 nRegsNeeded = 0;
		if (m_ProgramGenFormat.m_FmtBF0 & FMTBF0_VERTEXINPUTSCALE)
			++nRegsNeeded;

		uint32 TexCoordScaleMask = m_ProgramGenFormat.GetTexCoordInputScaleMask();
		nRegsNeeded += (TexCoordScaleMask >> 0) & 1;
		nRegsNeeded += (TexCoordScaleMask >> 1) & 1;
		nRegsNeeded += (TexCoordScaleMask >> 2) & 1;
		nRegsNeeded += (TexCoordScaleMask >> 3) & 1;
		nRegsNeeded += (TexCoordScaleMask >> 4) & 1;
		nRegsNeeded += (TexCoordScaleMask >> 5) & 1;
		nRegsNeeded += (TexCoordScaleMask >> 6) & 1;
		nRegsNeeded += (TexCoordScaleMask >> 7) & 1;

		if (!nRegsNeeded)
			return 0;
		nRegsNeeded = nRegsNeeded << 1;

		uint32 nRegs = nRegsNeeded;
		
		uint32 iReg = _iReg;
		CVec4Dfp32 *pReg = _pRegisters;
		uint32 nLeft = t_CRegisterBatchGetter::NewRegisterBatch(pReg, nRegsNeeded);
		M_ASSERT(nLeft >= nRegsNeeded, "Only supported");

		const CRC_VRegTransform *pTransform = _pTransform;


		if (m_ProgramGenFormat.m_FmtBF0 & FMTBF0_VERTEXINPUTSCALE)
		{
			m_iConstant_PosTransform = iReg; iReg += 2;
			pReg[0] = pTransform->m_Scale;
			pReg[1] = pTransform->m_Offset;
			++pTransform;
			pReg += 2;
		}

		uint Mask = 1;
		for (uint32 i = 0; i < CRC_MAXTEXCOORDS; ++i)
		{
			if (TexCoordScaleMask & Mask)
			{
				pReg[0] = pTransform->m_Scale;
				pReg[1] = pTransform->m_Offset;
				for (uint32 j = 0; j < CRC_MAXTEXCOORDS; ++j)
				{
					uint32 OriginalCoord = m_ProgramGenFormat.GetRename(i);
					if (OriginalCoord == i)
						m_iConstant_TexTransform[j] = iReg;
				}
				iReg += 2;
				pReg += 2;
				++pTransform;
			}

			Mask += Mask;
		}

		_pRegisters = pReg;
		return nRegs;
	}

	uint32 SetRegisters_TextureTransformFull(CVec4Dfp32* _pRegisters, uint _iReg, uint _Rename, const CVec4Dfp32* _pScalers)
	{
		// Enables all transforms with proper renaming
		uint32 nRegsNeeded = 2 + CRC_MAXTEXCOORDS * 2;
		m_iConstant_PosTransform = _iReg;
		_iReg += 2;
		m_iConstant_TexTransform[0] = _iReg + (((_Rename >> 0) & 7) << 1);
		m_iConstant_TexTransform[1] = _iReg + (((_Rename >> 3) & 7) << 1);
		m_iConstant_TexTransform[2] = _iReg + (((_Rename >> 6) & 7) << 1);
		m_iConstant_TexTransform[3] = _iReg + (((_Rename >> 9) & 7) << 1);
		m_iConstant_TexTransform[4] = _iReg + (((_Rename >> 12) & 7) << 1);
		m_iConstant_TexTransform[5] = _iReg + (((_Rename >> 15) & 7) << 1);
		m_iConstant_TexTransform[6] = _iReg + (((_Rename >> 18) & 7) << 1);
		m_iConstant_TexTransform[7] = _iReg + (((_Rename >> 21) & 7) << 1);

		memcpy(_pRegisters, _pScalers, sizeof(CVec4Dfp32) * 18);

		_iReg += CRC_MAXTEXCOORDS * 2;
		return nRegsNeeded;
	}

	template <class t_CRegisterBatchGetter>
	uint32 SetRegisters_TexGenMatrix(CVec4Dfp32* &_pRegisters, uint32 _iReg, const CMat4Dfp32** _pMatrix, const CRC_Attributes* _pAttrib)
	{
		uint32 nRegsNeeded = m_nTexGenParams;

		if (!nRegsNeeded)
			return 0;

		uint32 iReg = _iReg;
		CVec4Dfp32 *pReg = _pRegisters;
		uint32 nLeft = t_CRegisterBatchGetter::NewRegisterBatch(pReg, nRegsNeeded);
		M_ASSERT(nLeft >= nRegsNeeded, "Only supported");

		fp32* pAttr = _pAttrib->m_pTexGenAttr;

		for(uint32 iTxt = 0; iTxt < CRC_MAXTEXCOORDS; iTxt++)
		{
			const CMat4Dfp32* pMatrix = _pMatrix[iTxt];

			if (pMatrix)
			{
				m_iConstant_TexMatrix[iTxt] = iReg;
				(*pReg)[0] = pMatrix->k[0][0];
				(*pReg)[1] = pMatrix->k[1][0];
				(*pReg)[2] = pMatrix->k[2][0];
				(*pReg)[3] = pMatrix->k[3][0];
				++pReg;
				(*pReg)[0] = pMatrix->k[0][1];
				(*pReg)[1] = pMatrix->k[1][1];
				(*pReg)[2] = pMatrix->k[2][1];
				(*pReg)[3] = pMatrix->k[3][1];
				++pReg;
				(*pReg)[0] = pMatrix->k[0][2];
				(*pReg)[1] = pMatrix->k[1][2];
				(*pReg)[2] = pMatrix->k[2][2];
				(*pReg)[3] = pMatrix->k[3][2];
				++pReg;
				(*pReg)[0] = pMatrix->k[0][3];
				(*pReg)[1] = pMatrix->k[1][3];
				(*pReg)[2] = pMatrix->k[2][3];
				(*pReg)[3] = pMatrix->k[3][3];
				++pReg;
				iReg += 4;
			}

			uint32 TexGenMode = _pAttrib->m_lTexGenMode[iTxt];
#ifdef PLATFORM_XENON
			// 7 is texcoord0 on xenon?
			if (!TexGenMode && !(m_ProgramGenFormat.m_ActiveVertices & (1 << (7 + m_ProgramGenFormat.GetRename(iTxt)))))
				TexGenMode = CRC_TEXGENMODE_VOID;
#endif

			switch(TexGenMode)
			{
//			case CRC_TEXGENMODE_TEXCOORD :
//				break;

			case CRC_TEXGENMODE_LINEAR :
				{
					uint32 Comp = _pAttrib->GetTexGenComp(iTxt);

					{
						if (pAttr)
						{
							if ((Comp & CRC_TEXGENCOMP_U))
							{	(*pReg) = *(CVec4Dfp32*)pAttr; pAttr += 4; }
							else
								(*pReg) = CVec4Dfp32(0,0,0,0);++pReg;
							if ((Comp & CRC_TEXGENCOMP_V))
							{	(*pReg) = *(CVec4Dfp32*)pAttr; pAttr += 4; }
							else
								(*pReg) = CVec4Dfp32(0,0,0,0);++pReg;
							if ((Comp & CRC_TEXGENCOMP_W))
							{	(*pReg) = *(CVec4Dfp32*)pAttr; pAttr += 4; }
							else
								(*pReg) = CVec4Dfp32(0,0,0,0);++pReg;
							if ((Comp & CRC_TEXGENCOMP_Q))
							{	(*pReg) = *(CVec4Dfp32*)pAttr; pAttr += 4; }
							else
								(*pReg) = CVec4Dfp32(0,0,0,1);++pReg;
						}
						else
						{
							(*pReg) = CVec4Dfp32(0,0,0,0); ++pReg;
							(*pReg) = CVec4Dfp32(0,0,0,0); ++pReg;
							(*pReg) = CVec4Dfp32(0,0,0,0); ++pReg;
							(*pReg) = CVec4Dfp32(0,0,0,1); ++pReg;
						}
					}

					m_iConstant_TexGenParam[iTxt][0] = iReg;
					iReg += 4;
					break;
				}

			case CRC_TEXGENMODE_LINEARNHF :
				{
					M_ASSERT(pAttr, "!");

					if (pAttr)
					{
						memcpy(pReg, pAttr, sizeof(CVec4Dfp32) * 3);
						pReg += 3;
						pAttr += 4 * 3;
					}
					else
					{
						memset(pReg, 0, sizeof(CVec4Dfp32) * 3);
						pReg += 3;
					}

					m_iConstant_TexGenParam[iTxt][0] = iReg;
					iReg += 3;
					break;
				}

			case CRC_TEXGENMODE_BOXNHF :
				{
					M_ASSERT(pAttr, "!");

					if (pAttr)
					{
						memcpy(pReg, pAttr, sizeof(CVec4Dfp32) * 10);
						pReg += 10;
						pAttr += 10 * 4;
					}
					else
					{
						memset(pReg, 0, sizeof(CVec4Dfp32) * 10);
						pReg += 10;
					}
					m_iConstant_TexGenParam[iTxt][0] = iReg;
					iReg += 10;
				}
				break;

			case CRC_TEXGENMODE_LIGHTING :
			case CRC_TEXGENMODE_LIGHTING_NONORMAL :
				{
					if (pAttr)
					{
						*pReg = *((CVec4Dfp32*)pAttr); pReg++; pAttr += 4;
						*pReg = *((CVec4Dfp32*)pAttr); pReg++; pAttr += 4;
					}
					else 
					{
						*pReg = CVec4Dfp32(0,0,0,1.0f/256.0f); pReg++;
						*pReg = CVec4Dfp32(1,1,1,0); pReg++;
					}
					m_iConstant_TexGenParam[iTxt][0] = iReg;
					iReg += 2;
					break;
				}

			case CRC_TEXGENMODE_LIGHTFIELD :
				{
					if (pAttr)
					{
						memcpy(pReg, pAttr, sizeof(CVec4Dfp32) * 6);
						pReg += 6;
						pAttr += 6 * 4;
					}
					else
					{
						memset(pReg,0,sizeof(CVec4Dfp32) * 6);
						pReg += 6;
					}
					m_iConstant_TexGenParam[iTxt][0] = iReg;
					iReg += 6;
					break;
				}

			case CRC_TEXGENMODE_TSLV :
				{
					if (pAttr)
					{
						*pReg = *((CVec4Dfp32*)pAttr);
						pAttr += 4;
					}
					else 
					{
						*pReg = CVec4Dfp32(0,0,0,1);
					}
					++pReg;
					m_iConstant_TexGenParam[iTxt][0] = iReg;
					iReg += 1;
					break;
				}

			case CRC_TEXGENMODE_PIXELINFO :
				{
					if (pAttr)
					{
						pReg[0] = ((CVec4Dfp32*)pAttr)[0];
						pReg[1] = ((CVec4Dfp32*)pAttr)[1];
						pReg[2] = ((CVec4Dfp32*)pAttr)[2];
						pAttr += 12;
					}
					else 
					{
						pReg[0] = CVec4Dfp32(1,0,0,0);
						pReg[1] = CVec4Dfp32(0,1,0,0);
						pReg[2] = CVec4Dfp32(0,0,1,0);
					}
					m_iConstant_TexGenParam[iTxt][0] = iReg;
					pReg += 3;
					iReg += 3;
					break;
				}

			case CRC_TEXGENMODE_BUMPCUBEENV :
				{
					M_ASSERT(pAttr, "!");
					if (pAttr)
					{
						memcpy(pReg, pAttr, sizeof(CVec4Dfp32) * 8);
						pReg += 8;
						pAttr += 4 * 8;
					}
					else
					{
						memset(pReg, 0, sizeof(CVec4Dfp32) * 8);
						pReg += 8;
					}

					m_iConstant_TexGenParam[iTxt][0] = iReg;
					iReg += 8;
					break;
				}
			case CRC_TEXGENMODE_TSREFLECTION :
				{
					M_ASSERT(pAttr, "!");
					if (pAttr)
					{
						memcpy(pReg, pAttr, sizeof(CVec4Dfp32) * 2);
						pReg += 2;
						pAttr += 4 * 2;
					}
					else
					{
						memset(pReg, 0, sizeof(CVec4Dfp32) * 2);
						pReg += 2;
					}

					m_iConstant_TexGenParam[iTxt][0] = iReg;
					iReg += 2;
					break;
				}

			case CRC_TEXGENMODE_SHADOWVOLUME :
			case CRC_TEXGENMODE_SHADOWVOLUME2 :
				{
					if (pAttr)
					{
						memcpy(pReg, pAttr, sizeof(CVec4Dfp32)); pAttr += 4;
					}
					else 
					{
						*pReg = CVec4Dfp32(0,0,0,0);
					}
					++pReg;

					m_iConstant_TexGenParam[iTxt][0] = iReg;
					iReg += 1;
					break;
				}

			case CRC_TEXGENMODE_CONSTANT :
				{
					if (pAttr)
					{
						pReg[0] = *((CVec4Dfp32*)pAttr);
						pAttr += 4;
					}
					else 
					{
						pReg[0] = CVec4Dfp32(0,0,0,0);
					}
					m_iConstant_TexGenParam[iTxt][0] = iReg;
					pReg += 1;
					iReg += 1;
					break;
				}

			case CRC_TEXGENMODE_DEPTHOFFSET :
				{
					if (pAttr)
					{
						pReg[0] = ((CVec4Dfp32*)pAttr)[0];
						pReg[1] = ((CVec4Dfp32*)pAttr)[1];
						pAttr += 8;
					}
					else
					{
						pReg[0] = CVec4Dfp32(0,0,0,0);
						pReg[1] = CVec4Dfp32(1,1,0,0);
					}
					m_iConstant_TexGenParam[iTxt][0] = iReg;
					iReg += 2;
					pReg += 2;
					break;
				}

//			case CRC_TEXGENMODE_MSPOS :
//					break;

//			case CRC_TEXGENMODE_NORMALMAP :
//				break;

//			case CRC_TEXGENMODE_REFLECTION :
//				break;

//			case CRC_TEXGENMODE_ENV :
//				break;


//			case CRC_TEXGENMODE_VOID :
//				break;

			default :
				break;
			}

		}
		_pRegisters = pReg;
		return iReg - _iReg;
	}

#ifdef CRC_SUPPORTVERTEXLIGHTING
	template <class t_CRegisterBatchGetter>
	uint32 SetRegisters_Lights(CVec4Dfp32* &_pRegisters, uint32 _iReg, const CRC_Light* _pLights, uint32 _nLights)
	{

		uint32 nRegsNeeded = _nLights*2;

		CVec4Dfp32 *pReg = _pRegisters;
		uint32 nLeft = t_CRegisterBatchGetter::NewRegisterBatch(pReg, nRegsNeeded);
		M_ASSERT(nLeft >= nRegsNeeded, "Only supported");

		m_iConstant_Lights = _iReg;
	#ifdef CRC_LOGLIGHTS
		ConOut(CStrF("(CRC_VPFormat::SetRegisters_Lights) pL %.8x, nL %d", _pLights, _nLights));
	#endif

		for(uint32 i = 0; i < _nLights; i++)
		{
			const CRC_Light& L = _pLights[i];
			switch(L.m_Type)
			{
			case 3 :
				{
					(*pReg)[0] = 0;
					(*pReg)[1] = 0;
					(*pReg)[2] = 0;
					(*pReg)[3] = 0;
					++pReg;
					(*pReg)[0] = fp32(L.m_Color.GetR()) / 255.0f;
					(*pReg)[1] = fp32(L.m_Color.GetG()) / 255.0f;
					(*pReg)[2] = fp32(L.m_Color.GetB()) / 255.0f;
					(*pReg)[3] = fp32(L.m_Color.GetA()) / 255.0f;
					++pReg;

	#ifdef CRC_LOGLIGHTS
					CVec4Dfp32 Amb(
						fp32(L.m_Color.GetR()) / 255.0f,
						fp32(L.m_Color.GetG()) / 255.0f,
						fp32(L.m_Color.GetB()) / 255.0f,
						1.0f);
					ConOut(CStrF("    Light %d, Ambient, Dir %s, Pos %s, Col %s", i, L.m_Direction.GetString().Str(), L.m_Pos.GetString().Str(), Amb.GetString().Str() ));
	#endif
				}
				break;
				
			case CRC_LIGHTTYPE_POINT :
				{
					(*pReg)[0] = L.m_Pos[0];
					(*pReg)[1] = L.m_Pos[1];
					(*pReg)[2] = L.m_Pos[2];
					(*pReg)[3] = L.m_Attenuation[1];
					++pReg;
					(*pReg)[0] = fp32(L.m_Color.GetR()) / 255.0f;
					(*pReg)[1] = fp32(L.m_Color.GetG()) / 255.0f;
					(*pReg)[2] = fp32(L.m_Color.GetB()) / 255.0f;
					(*pReg)[3] = fp32(L.m_Color.GetA()) / 255.0f;
					++pReg;

	#ifdef CRC_LOGLIGHTS
					CVec4Dfp32 Diff(
						fp32(L.m_Color.GetR()) / 255.0f,
						fp32(L.m_Color.GetG()) / 255.0f,
						fp32(L.m_Color.GetB()) / 255.0f,
						1.0f);
					ConOut(CStrF("    Light %d, Point, Dir %s, Pos %s, Col %s", i, L.m_Direction.GetString().Str(), L.m_Pos.GetString().Str(), Diff.GetString().Str() ));
	#endif
				}
				break;

			case CRC_LIGHTTYPE_PARALLELL :
				{
					(*pReg)[0] = L.m_Direction[0];
					(*pReg)[1] = L.m_Direction[1];
					(*pReg)[2] = L.m_Direction[2];
					(*pReg)[3] = 0;
					++pReg;
					(*pReg)[0] = fp32(L.m_Color.GetR()) / 255.0f;
					(*pReg)[1] = fp32(L.m_Color.GetG()) / 255.0f;
					(*pReg)[2] = fp32(L.m_Color.GetB()) / 255.0f;
					(*pReg)[3] = fp32(L.m_Color.GetA()) / 255.0f;
					++pReg;

	#ifdef CRC_LOGLIGHTS
					CVec4Dfp32 Diff(
						fp32(L.m_Color.GetR()) / 255.0f,
						fp32(L.m_Color.GetG()) / 255.0f,
						fp32(L.m_Color.GetB()) / 255.0f,
						1.0f);
					ConOut(CStrF("    Light %d, Parallell, Dir %s, Pos %s, Col %s", i, L.m_Direction.GetString().Str(), L.m_Pos.GetString().Str(), Diff.GetString().Str() ));
	#endif
				}
				break;
			}
		}

		_pRegisters = pReg;
		return _nLights*2;
	}
#endif

#ifdef CRC_SUPPORTCUBEVP
	template <class t_CRegisterBatchGetter>
	uint32 SetRegisters_MatrixPaletteSpecialCube(CVec4Dfp32* &_pRegisters, uint32 _iReg, uint32 _nMaxReg, const CRC_MatrixPalette *_pMatrixPalette)
	{
		m_iConstant_MP = _iReg;
		
		uint32 iReg = _iReg;

		uint32 MaxMat = (_nMaxReg) / 2;
		uint32 nMat = Min((uint32)MaxMat, _pMatrixPalette->m_nMatrices);

		uint32 nRegsNeeded = nMat * 2;

		if (!nRegsNeeded)
			return 0;

		CVec4Dfp32 *pReg = _pRegisters;
		uint32 nLeft = t_CRegisterBatchGetter::NewRegisterBatch(pReg, nRegsNeeded);
		M_ASSERT(nLeft >= nRegsNeeded, "Only supported");

		// Transpose 4x4 and store as 4x3.
		const CVec4Dfp32* pMatArray = (const CVec4Dfp32*)_pMatrixPalette->m_pMatrices;
		for(uint32 i = 0; i < nMat; i++)
		{
			const CVec4Dfp32 *pMat = pMatArray + i*2;

			*pReg = *pMat;
			++pMat;	++pReg;
			*pReg = *pMat;
			++pMat;	++pReg;
		}

		_pRegisters = pReg;

		return nMat*2;
	}
#endif

	template <class t_CRegisterBatchGetter>
	uint32 SetRegisters_MatrixPalette(CVec4Dfp32* &_pRegisters, uint32 _iReg, uint32 _nMaxReg, const CRC_MatrixPalette *_pMatrixPalette)
	{
		m_iConstant_MP = _iReg;
		
		uint32 iReg = _iReg;

		uint32 MaxMat = (_nMaxReg) / 3;
		uint32 nMat = Min((uint32)MaxMat, _pMatrixPalette->m_nMatrices);

		uint32 nRegsNeeded = nMat * 3;

		if (!nRegsNeeded)
			return 0;

		CVec4Dfp32 *pReg = _pRegisters;
		uint32 nLeft = t_CRegisterBatchGetter::NewRegisterBatch(pReg, nRegsNeeded);
		M_ASSERT(nLeft >= nRegsNeeded, "Only supported");

		if (_pMatrixPalette->m_piMatrices)
		{
			// Transpose 4x4 and store as 4x3.
			const CMat4Dfp32* pMatArray = (const CMat4Dfp32* )_pMatrixPalette->m_pMatrices;
			const uint16 *pIndexArray = _pMatrixPalette->m_piMatrices;
			for(uint32 i = 0; i < nMat; i++)
			{
				const CMat4Dfp32 &Mat = pMatArray[pIndexArray[i]];
//				CMat4Dfp32 Temp;Temp.Unit();const CMat4Dfp32 &Mat = Temp;

				(*pReg)[0] = Mat.k[0][0];
				(*pReg)[1] = Mat.k[1][0];
				(*pReg)[2] = Mat.k[2][0];
				(*pReg)[3] = Mat.k[3][0];
				++pReg;
				(*pReg)[0] = Mat.k[0][1];
				(*pReg)[1] = Mat.k[1][1];
				(*pReg)[2] = Mat.k[2][1];
				(*pReg)[3] = Mat.k[3][1];
				++pReg;
				(*pReg)[0] = Mat.k[0][2];
				(*pReg)[1] = Mat.k[1][2];
				(*pReg)[2] = Mat.k[2][2];
				(*pReg)[3] = Mat.k[3][2];
				++pReg;
			}
		}
		else
		{
			// Transpose 4x4 and store as 4x3.
			const CMat4Dfp32* pMatArray = (const CMat4Dfp32*)_pMatrixPalette->m_pMatrices;
			for(uint32 i = 0; i < nMat; i++)
			{
				const CMat4Dfp32 &Mat = pMatArray[i];
//				CMat4Dfp32 Temp; Temp.Unit(); const CMat4Dfp32 &Mat = Temp;

				(*pReg)[0] = Mat.k[0][0];
				(*pReg)[1] = Mat.k[1][0];
				(*pReg)[2] = Mat.k[2][0];
				(*pReg)[3] = Mat.k[3][0];
				++pReg;
				(*pReg)[0] = Mat.k[0][1];
				(*pReg)[1] = Mat.k[1][1];
				(*pReg)[2] = Mat.k[2][1];
				(*pReg)[3] = Mat.k[3][1];
				++pReg;
				(*pReg)[0] = Mat.k[0][2];
				(*pReg)[1] = Mat.k[1][2];
				(*pReg)[2] = Mat.k[2][2];
				(*pReg)[3] = Mat.k[3][2];
				++pReg;
			}
		}

		_pRegisters = pReg;

		return nMat*3;
	}

	template <class t_CRegisterBatchGetter>
	uint32 SetRegisters_MatrixPalette_Quat(CVec4Dfp32* &_pRegisters, uint32 _iReg, uint32 _nMaxReg, const CRC_MatrixPalette *_pMatrixPalette)
	{
		m_iConstant_MP = _iReg;
		
		uint32 iReg = _iReg;

		uint32 MaxMat = (_nMaxReg) / 2;
		uint32 nMat = Min((uint32)MaxMat, _pMatrixPalette->m_nMatrices);

		uint32 nRegsNeeded = nMat * 2;

		if (!nRegsNeeded)
			return 0;

		CVec4Dfp32 *pReg = _pRegisters;
		uint32 nLeft = t_CRegisterBatchGetter::NewRegisterBatch(pReg, nRegsNeeded);
		M_ASSERT(nLeft >= nRegsNeeded, "Only supported");

		if (_pMatrixPalette->m_piMatrices)
		{
			// Transpose 4x4 and store as 4x3.
			const CMat4Dfp32* pMatArray = _pMatrixPalette->m_pMatrices;
			const uint16 *pIndexArray = _pMatrixPalette->m_piMatrices;
			for(uint32 i = 0; i < nMat; i++)
			{
				const CMat4Dfp32 Mat = pMatArray[pIndexArray[i]];

				CQuatfp32 Q;
				CMat4Dfp32 Tmp(Mat);
				Tmp.Transpose3x3();
				Q.Create(Tmp);

				(*pReg)[0] = Q.k[0];
				(*pReg)[1] = Q.k[1];
				(*pReg)[2] = Q.k[2];
				(*pReg)[3] = Q.k[3];
				++pReg;
				(*pReg)[0] = Mat.k[3][0];
				(*pReg)[1] = Mat.k[3][1];
				(*pReg)[2] = Mat.k[3][2];
				(*pReg)[3] = Mat.k[3][3];
				++pReg;
			}
		}
		else
		{
			// Transpose 4x4 and store as 4x3.
			const CMat4Dfp32* pMatArray = _pMatrixPalette->m_pMatrices;
			for(uint32 i = 0; i < nMat; i++)
			{
				const CMat4Dfp32 Mat = pMatArray[i];
				CQuatfp32 Q;
				CMat4Dfp32 Tmp(Mat);
				Tmp.Transpose3x3();
				Q.Create(Tmp);

				(*pReg)[0] = Q.k[0];
				(*pReg)[1] = Q.k[1];
				(*pReg)[2] = Q.k[2];
				(*pReg)[3] = Q.k[3];
				++pReg;
				(*pReg)[0] = Mat.k[3][0];
				(*pReg)[1] = Mat.k[3][1];
				(*pReg)[2] = Mat.k[3][2];
				(*pReg)[3] = Mat.k[3][3];
				++pReg;
			}
		}

		_pRegisters = pReg;

		return nMat*2;
	}

//	uint32 SetRegisters_MatrixPalette_Quat(CVec4Dfp32* _pRegisters, uint32 _iReg, uint32 _iMaxReg, const uint8 *_pValidRegMap, const CMat4Dfp32* _pMatrices, uint32 _nMatrices);
};


class CRC_VPGenerator : public CReferenceCount
{
protected:
	spCRegistry m_spVPSource;
	uint32 m_MaxTexCoords;

	char m_TextureSubst[CRC_MAXTEXCOORDS][32]; 

	int GetKeyList_r(CRegistry* _pSrc, const char** _lpKeys, int _MaxKeys, const char** _lpDefines, int _nDefines);
	int SubstituteConstants(char* _pVPDst, int _MaxDstSize, const char* _pVPSrc, const CRC_VPFormat& _Format);

public:
	void Create(spCRegistry _spReg);
	void Create(CStr _FileName, CStr _FileNameDef, bool _bLowerCase, int _MaxTextures);

	CStr CreateVP(const CRC_VPFormat& _Format);
};

void FPPreParser_Inplace(char* _pSource, const char* _lpDefines[]);

#endif // _INC_MRENDERVPGEN
