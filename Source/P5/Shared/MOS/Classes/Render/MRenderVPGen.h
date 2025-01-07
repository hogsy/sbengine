
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


enum EMPFlags
{
	 EMPFlags_Normal = 0
	,EMPFlags_SpecialCubeVec = DBit(0)
	,EMPFlags_SpecialCubeTex = DBit(1)
	,EMPFlags_SpecialCubeTexScaleZ = DBit(2)
	,EMPFlags_SpecialCubeTexScale2 = DBit(3)
};
#if DEF_CRC_MAXTEXTURES > 4
#define DEF_CRC_VPFormat_MaxTextures m_ProgramGenFormat.m_MultiTexture
#else
#define DEF_CRC_VPFormat_MaxTextures DEF_CRC_MAXTEXTURES
#endif

class CRC_VPFormat
{
public:
	class CProgramFormat
	{
	public:

		enum
		{
			BITSIZE_LIGHTTYPES = 2,
#if DEF_CRC_MAXTEXTURES > 4
			BITSIZE_TEXGENMODE = 5,
#else
			BITSIZE_TEXGENMODE = 4,
#endif
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

			FORMAT_VERSION = 0x0101,
		};


		uint32 m_LightTypes : CRC_MAXLIGHTS * BITSIZE_LIGHTTYPES;								// 16 
		uint32 m_TexInputRename : CRC_MAXTEXCOORDS * BITSIZE_TEXINPUTRENAME;					// 8
		uint32 m_TexMatrix : CRC_MAXTEXCOORDS;													// 4

		// bf byte 0
		uint32 m_nMWComp : 4;				// Matrices per vertex, [0..8] is allowed range.	// 4

		// bf byte 1
#ifndef PLATFORM_XBOX1
		uint32 m_bNoNormal : 1;
#endif
		uint32 m_bUseNormal : 1;																// 1
		uint32 m_bUseTangents : 1;																// 1
		uint32 m_bNormalizeNormal : 1;															// 1
		uint32 m_bNoVertexColorComponent : 1;													// 1 
		uint32 m_bNoColorOutput : 1;															// 1 

		// bf byte 2,3
		uint32 m_VertexTransform: 1;															// 1
		uint32 m_TextureTransform: 4; // 6 byte									// 4 // DWORD
#ifndef PLATFORM_XENON
		uint32 m_bClipPlanes : 1;																// 1
		uint32 m_FogDepth : 1;																	// 1
		uint32 m_Lighting : 1;			// Lighting enabled // 7 byte							// 1
		uint32 m_nLights : 4;				// Lights, [0..CRC_MAXLIGHTS] is allowed range		// 4
#endif
		uint32 m_MPFlags : 4;																	// 4
#if DEF_CRC_MAXTEXTURES > 4
		uint32 m_MultiTexture : 4;

		uint16 m_lTexGenModeAndComponents[CRC_MAXTEXCOORDS];										// 32	
#else
		uint8 m_lTexGenModeAndComponents[CRC_MAXTEXCOORDS];										// 32	
#endif
#ifndef PLATFORM_XBOX1
		uint32 m_ActiveVertices;
#endif

		int GetTextureTransform(int _iTxt) const
		{
			return ((m_TextureTransform >> (_iTxt)) & 0x1);
		}
		int GetRename(int _iTxt) const
		{
			return ((m_TexInputRename >> (_iTxt * BITSIZE_TEXINPUTRENAME)) & ((1<<BITSIZE_TEXINPUTRENAME)-1));
		}
		int GetTexGen(int _iTxt) const
		{
			return m_lTexGenModeAndComponents[_iTxt] & ((1 << BITSIZE_TEXGENMODE) - 1);
		}
		int GetTexGenComponents(int _iTxt) const
		{
			return (m_lTexGenModeAndComponents[_iTxt] & (((1 << 4) - 1) << BITSIZE_TEXGENMODE)) >> BITSIZE_TEXGENMODE;
		}
	}; // 12 bytes

	CProgramFormat m_ProgramGenFormat;

	uint16 m_NeededTex:15;
	uint16 m_bUseQuatMatrixPallette:1;
	int16 m_iConstant_Base;
#ifndef PLATFORM_XENON
	uint8 m_iConstant_ClipPlanes;
	uint8 m_iConstant_Lights;
	uint8 m_iConstant_FogDepth;
#endif
	uint8 m_iConstant_PosTransform;
	uint8 m_iConstant_MP;
	uint8 m_iConstant_ConstantColor;
	uint8 m_iConstant_TexGenParam[CRC_MAXTEXCOORDS][4];
	uint8 m_iConstant_TexMatrix[CRC_MAXTEXCOORDS];
	uint8 m_iConstant_TexTransform[CRC_MAXTEXCOORDS];
	
	void Clear()
	{
		memset(&m_ProgramGenFormat, 0, sizeof(m_ProgramGenFormat));

#ifndef PLATFORM_XBOX1
		m_ProgramGenFormat.m_ActiveVertices = 0xffffffff;
#endif
	}
	CRC_VPFormat(bool _bUseQuatMatrixPallette, int _MaxTextures)
	{

		FillChar(this, sizeof(*this), 0);
		Clear();
		m_ProgramGenFormat.m_TexInputRename = 0;
#if DEF_CRC_MAXTEXTURES > 4
		m_ProgramGenFormat.m_MultiTexture = _MaxTextures;
#endif
		for(int l = 0; l < CRC_MAXTEXCOORDS; l++)
			m_ProgramGenFormat.m_TexInputRename |= (l) << (l * CProgramFormat::BITSIZE_TEXINPUTRENAME);

		m_iConstant_Base = -96;
		m_iConstant_MP = EMatrixPalette;
		m_iConstant_ConstantColor = EConstantColor;
#ifndef PLATFORM_XENON
		m_iConstant_FogDepth = EAttrib;
		m_iConstant_Lights = ELights;
#endif
		m_bUseQuatMatrixPallette = _bUseQuatMatrixPallette;
/*
		m_nMWComp = 0;
		m_TexTransform = 0;
		m_Lighting = 0;
		m_nLights = 0;

		for(int l = 0; l < CRC_MAXLIGHTS; l++)
			m_LightTypes[l] = CRC_LIGHTTYPE_POINT;

		for(int i = 0; i < CRC_MAXTEXCOORDS; i++)
		{
			m_TexGenMode[i] = 0;
			m_iConstant_TexGenParam[i] = 0;
			m_iConstant_TexMatrix[i] = 0;
		}

		m_iConstant_MP = 0;
		m_iConstant_Lights = 0;
*/
	}

	void Init(int _MaxTextures)
	{
		Clear();
		m_ProgramGenFormat.m_TexInputRename = 0;
#if DEF_CRC_MAXTEXTURES > 4
		m_ProgramGenFormat.m_MultiTexture = _MaxTextures;
#endif
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

	M_INLINE static int GetUsedInputTextures(const CRC_Attributes* _pAttrib)
	{
		int RetUsed = 0;
		for(int iTxt = 0; iTxt < CRC_MAXTEXCOORDS; iTxt++)
		{
			int TexGenMode = _pAttrib->m_lTexGenMode[iTxt];
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


	M_INLINE void SetAttrib(const CRC_Attributes* _pAttrib, const CMat4Dfp4** _pMatrix, uint32 _EnabledTextures = 0xffffffff)
	{
		int nRegsNeeded = 0;
		for(int iTxt = 0; iTxt < DEF_CRC_VPFormat_MaxTextures; iTxt++)
		{
//			M_ASSERT(_pAttrib->m_lTexGenMode[iTxt] <= 255, "No more than 8 bits allowed, increase to x bit percomponent");
//			m_ProgramGenFormat.m_TexGenMode |= (_pAttrib->m_lTexGenMode[iTxt] << (iTxt * CProgramFormat::BITSIZE_TEXGENMODE));

			const CMat4Dfp4* pMatrix = _pMatrix[iTxt];
			if (pMatrix)
				nRegsNeeded += 4;

			int TexGenMode = _pAttrib->m_lTexGenMode[iTxt];
#ifndef PLATFORM_XBOX1
			if (!TexGenMode && !(m_ProgramGenFormat.m_ActiveVertices & (1 << (2 + _pAttrib->m_iTexCoordSet[iTxt]))))
				TexGenMode = CRC_TEXGENMODE_VOID;
			if (!TexGenMode && (!(_EnabledTextures & (1 << iTxt))))
				TexGenMode = CRC_TEXGENMODE_VOID;
			
#endif
#ifdef PLATFORM_XBOX1
			if (!TexGenMode && ((!((1 << _pAttrib->m_iTexCoordSet[iTxt]) & _EnabledTextures)) || !_pAttrib->m_TextureID[iTxt]))
				TexGenMode = CRC_TEXGENMODE_VOID;
#endif

			m_ProgramGenFormat.m_lTexGenModeAndComponents[iTxt] |= TexGenMode;
#ifdef PLATFORM_XBOX1
			uint32 TexGenComp = _pAttrib->GetTexGenComp(iTxt);
#else
			uint32 TexGenComp = 0xf;
#endif
			m_ProgramGenFormat.m_lTexGenModeAndComponents[iTxt] |= TexGenComp << CProgramFormat::BITSIZE_TEXGENMODE;

			if (TexGenMode >= CRC_TEXGENMODE_FIRSTUSENORMAL)
				m_ProgramGenFormat.m_bUseNormal = 1;

			if (TexGenMode >= CRC_TEXGENMODE_FIRSTUSETANGENT)
				m_ProgramGenFormat.m_bUseTangents = 1;

			switch(TexGenMode)
			{
			case CRC_TEXGENMODE_LINEAR :
				if (TexGenComp & 1)
					nRegsNeeded += 1;
				if (TexGenComp & 2)
					nRegsNeeded += 1;
				if (TexGenComp & 4)
					nRegsNeeded += 1;
				if (TexGenComp & 8)
					nRegsNeeded += 1;
				break;
			case CRC_TEXGENMODE_LINEARNHF :nRegsNeeded += 3;break;
			case CRC_TEXGENMODE_BOXNHF :nRegsNeeded += 10;break;

			case CRC_TEXGENMODE_SHADOWVOLUME2 :
				nRegsNeeded += 1;
				break;
			case CRC_TEXGENMODE_CONSTANT :
				nRegsNeeded += 1;
				break;
			case CRC_TEXGENMODE_SHADOWVOLUME :
//				m_ProgramGenFormat.m_bNormalizeNormal = 1;
				nRegsNeeded += 1;
				break;
			case CRC_TEXGENMODE_BUMPCUBEENV :
				nRegsNeeded += 8;
				break;
			case CRC_TEXGENMODE_TSREFLECTION :
				nRegsNeeded += 1;
				break;
			case CRC_TEXGENMODE_LIGHTING :
				nRegsNeeded += 2;
				break;
			case CRC_TEXGENMODE_TSLV :
				nRegsNeeded += 1;
				break;
			case CRC_TEXGENMODE_PIXELINFO :
				nRegsNeeded += 3;
				break;
			case CRC_TEXGENMODE_DEPTHOFFSET :
				nRegsNeeded += 2;
				break;

			default :;
			}
		}

		m_NeededTex = nRegsNeeded;

#ifndef PLATFORM_XENON
		if (_pAttrib->m_Flags & CRC_FLAGS_FOG)
			m_ProgramGenFormat.m_FogDepth = 1;
#endif
	}

	/*
	Not used any more, the has to be set through set attrib
	M_INLINE void SetTexGenOff(int _iTxt)
	{
		m_ProgramGenFormat.m_TexGenMode = (m_ProgramGenFormat.m_TexGenMode & (~(15 << (_iTxt * 4)))) | (CRC_TEXGENMODE_VOID << (_iTxt * 4));
	}

	M_INLINE void SetTexGenOn(int _iTxt, const CRC_Attributes* _pAttrib)
	{
		M_ASSERT(_pAttrib->m_lTexGenMode[iTxt] <= 15, "No more than 4 bits allowed, increase to 8 bit percomponent");
		m_ProgramGenFormat.m_TexGenMode = (m_ProgramGenFormat.m_TexGenMode & (~(15 << (_iTxt * 4)))) | (_pAttrib->m_lTexGenMode[_iTxt] << (_iTxt * 4));
	}
	*/

#ifndef PLATFORM_XENON
	M_INLINE void SetLights(const CRC_Light* _pLights, int _nLights)
	{
		m_ProgramGenFormat.m_Lighting = 1;
		m_ProgramGenFormat.m_nLights = _nLights;
		m_ProgramGenFormat.m_LightTypes = 0;
		m_ProgramGenFormat.m_bUseNormal = 1;
		m_ProgramGenFormat.m_bNormalizeNormal = 1;

		for(int i = 0; i < _nLights; i++)
			m_ProgramGenFormat.m_LightTypes |= _pLights[i].m_Type << (i*CProgramFormat::BITSIZE_LIGHTTYPES);
	}

#endif

	M_INLINE void SetVertexTransform()
	{
		m_ProgramGenFormat.m_VertexTransform = true;
	}

	M_INLINE void SetTextureTransform(int _iTxt)
	{
		m_ProgramGenFormat.m_TextureTransform |= 1 << _iTxt;
	}

	M_INLINE void SetVertexColorComponent(bool _bColorVertex)
	{
		m_ProgramGenFormat.m_bNoVertexColorComponent = !_bColorVertex; 
	}

	M_INLINE void SetColorOutput(bool _bColor)
	{
		m_ProgramGenFormat.m_bNoColorOutput = !_bColor; 
	}
	#ifndef PLATFORM_XBOX1

	M_INLINE void SetVertexNormalComponent(bool _bNormalVertex)
	{
		m_ProgramGenFormat.m_bNoNormal = !_bNormalVertex;
	}

	M_INLINE void SetActiveVertices(uint32 _ActivecVertices)
	{
		m_ProgramGenFormat.m_ActiveVertices = _ActivecVertices;
	}

	#endif

	M_INLINE void SetTexRename(int _iTxt, int _iRenameTo)
	{
//		m_ProgramGenFormat.m_TexInputRename = (m_ProgramGenFormat.m_TexInputRename & (~(3 << (_iTxt * 2)))) | _iRenameTo << (_iTxt * 2); 
		m_ProgramGenFormat.m_TexInputRename = (m_ProgramGenFormat.m_TexInputRename & (~(((1<<CProgramFormat::BITSIZE_TEXINPUTRENAME)-1) << (_iTxt * CProgramFormat::BITSIZE_TEXINPUTRENAME)))) | _iRenameTo << (_iTxt * CProgramFormat::BITSIZE_TEXINPUTRENAME); 
	}

	M_INLINE void SetMWComp(int _nMWComp)
	{
		m_ProgramGenFormat.m_nMWComp = _nMWComp;
	}

	M_INLINE void SetMPFlags(uint32 _Flags)
	{
		m_ProgramGenFormat.m_MPFlags = _Flags;
	}

#ifndef PLATFORM_XENON
	M_INLINE void SetClipPlanes(uint32 _bEnabled)
	{
		m_ProgramGenFormat.m_bClipPlanes = _bEnabled;
	}
#endif

	M_INLINE void SetTexMatrix(int _iTxt)
	{
		m_ProgramGenFormat.m_TexMatrix |= 1 << _iTxt;
	}

	int SetRegisters_Init(CVec4Dfp4* _pRegisters, int _Base, int _iReg);

	M_INLINE int SetRegisters_ConstantColor(CVec4Dfp4* _pRegisters, int _iReg, CPixel32 _Color)
	{
		m_iConstant_ConstantColor = _iReg;

		(*_pRegisters)[0] = fp4(_Color.GetR()) / 255.0f;
		(*_pRegisters)[1] = fp4(_Color.GetG()) / 255.0f;
		(*_pRegisters)[2] = fp4(_Color.GetB()) / 255.0f;
		(*_pRegisters)[3] = fp4(_Color.GetA()) / 255.0f;
		return 1;
	}


	M_INLINE int SetRegisters_Attrib(CVec4Dfp4* _pRegisters, int _iReg, const CRC_Attributes* _pAttrib, class CRC_Core* _pRC)
	{
		int iReg = _iReg;
#ifndef PLATFORM_XENON
		if (m_ProgramGenFormat.m_FogDepth)
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
#endif
		return iReg - _iReg;
	}

	static int GetFirstFreeRegister() { return 8; };

	template <class t_CRegisterBatchGetter>
	int SetRegisters_ClipPlanes(CVec4Dfp4* _pRegisters, int _iReg, CPlane3Dfp4* _pClipPlanes, int _nPlanes)
	{
		m_iConstant_ClipPlanes = _iReg;

		int nRegs = 6;
		CVec4Dfp4 *pReg = _pRegisters;
		int nLeft = t_CRegisterBatchGetter::NewRegisterBatch(pReg, nRegs);
		M_ASSERT(nLeft >= nRegs, "Only supported");
		M_ASSERT(_nPlanes <= nRegs, "!");

		for(int i = 0; i < _nPlanes; i++)
		{
			const CPlane3Dfp4& Plane = _pClipPlanes[i];
			pReg[i][0] = Plane.n[0];
			pReg[i][1] = Plane.n[1];
			pReg[i][2] = Plane.n[2];
			pReg[i][3] = Plane.d;
		}

		return nRegs;
	}

	template <class t_CRegisterBatchGetter>
	int SetRegisters_Transform(CVec4Dfp4* &_pRegisters, int _iReg, const CRC_VRegTransform *_pTransform)
	{
		int nRegsNeeded = 0;
		if (m_ProgramGenFormat.m_VertexTransform)
			++nRegsNeeded;

		for (int i = 0; i < 4; ++i)
		{
			if (m_ProgramGenFormat.m_TextureTransform & (1 << i))
				++nRegsNeeded;
		}

		if (!nRegsNeeded)
			return 0;
		nRegsNeeded *= 2;

		int nRegs = nRegsNeeded;
		
		int iReg = _iReg;
		CVec4Dfp4 *pReg = _pRegisters;
		int nLeft = t_CRegisterBatchGetter::NewRegisterBatch(pReg, nRegsNeeded);
		M_ASSERT(nLeft >= nRegsNeeded, "Only supported");

		const CRC_VRegTransform *pTransform = _pTransform;


		if (m_ProgramGenFormat.m_VertexTransform)
		{
			m_iConstant_PosTransform = iReg; iReg += 2;
			pReg[0] = pTransform->m_Scale;
			pReg[1] = pTransform->m_Offset;
			++pTransform;
			pReg += 2;
		}

		for (int i = 0; i < 4; ++i)
		{
			if (m_ProgramGenFormat.m_TextureTransform & (1 << i))
			{
				pReg[0] = pTransform->m_Scale;
				pReg[1] = pTransform->m_Offset;
				for (int j = 0; j < DEF_CRC_VPFormat_MaxTextures; ++j)
				{
					int OriginalCoord = m_ProgramGenFormat.GetRename(i);
					if (OriginalCoord == i)
						m_iConstant_TexTransform[j] = iReg;
				}
				iReg += 2;
				pReg += 2;
				++pTransform;
			}
		}

		_pRegisters = pReg;
		return nRegs;
	}

	template <class t_CRegisterBatchGetter>
	int SetRegisters_TexGenMatrix(CVec4Dfp4* &_pRegisters, int _iReg, const CMat4Dfp4** _pMatrix, const CRC_Attributes* _pAttrib)
	{
		int nRegsNeeded = m_NeededTex;
#ifdef _DEBUG
		SetAttrib(_pAttrib, _pMatrix);
		M_ASSERT(m_NeededTex == nRegsNeeded, "SetAttrib has not been called");
#endif

		if (!nRegsNeeded)
			return 0;

		int iReg = _iReg;
		CVec4Dfp4 *pReg = _pRegisters;
		int nLeft = t_CRegisterBatchGetter::NewRegisterBatch(pReg, nRegsNeeded);
		M_ASSERT(nLeft >= nRegsNeeded, "Only supported");

		fp4* pAttr = _pAttrib->m_pTexGenAttr;

		for(int iTxt = 0; iTxt < DEF_CRC_VPFormat_MaxTextures; iTxt++)
		{
			const CMat4Dfp4* pMatrix = _pMatrix[iTxt];

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

			int TexGenMode = _pAttrib->m_lTexGenMode[iTxt];
#ifndef PLATFORM_XBOX1
			if (!TexGenMode && !(m_ProgramGenFormat.m_ActiveVertices & (1 << (7 + m_ProgramGenFormat.GetRename(iTxt)))))
				TexGenMode = CRC_TEXGENMODE_VOID;
#endif

			switch(TexGenMode)
			{
//			case CRC_TEXGENMODE_TEXCOORD :
//				break;

			case CRC_TEXGENMODE_LINEAR :
				{
					int Comp = _pAttrib->GetTexGenComp(iTxt);

					{
						if (pAttr)
						{
							if ((Comp & CRC_TEXGENCOMP_U))
							{	(*pReg) = *(CVec4Dfp4*)pAttr; pAttr += 4; }
							else
								(*pReg) = CVec4Dfp4(0,0,0,0);++pReg;
							if ((Comp & CRC_TEXGENCOMP_V))
							{	(*pReg) = *(CVec4Dfp4*)pAttr; pAttr += 4; }
							else
								(*pReg) = CVec4Dfp4(0,0,0,0);++pReg;
							if ((Comp & CRC_TEXGENCOMP_W))
							{	(*pReg) = *(CVec4Dfp4*)pAttr; pAttr += 4; }
							else
								(*pReg) = CVec4Dfp4(0,0,0,0);++pReg;
							if ((Comp & CRC_TEXGENCOMP_Q))
							{	(*pReg) = *(CVec4Dfp4*)pAttr; pAttr += 4; }
							else
								(*pReg) = CVec4Dfp4(0,0,0,1);++pReg;
						}
						else
						{
							(*pReg) = CVec4Dfp4(0,0,0,0); ++pReg;
							(*pReg) = CVec4Dfp4(0,0,0,0); ++pReg;
							(*pReg) = CVec4Dfp4(0,0,0,0); ++pReg;
							(*pReg) = CVec4Dfp4(0,0,0,1); ++pReg;
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
						memcpy(pReg, pAttr, sizeof(CVec4Dfp4) * 3);
						pReg += 3;
						pAttr += 4 * 3;
					}
					else
					{
						memset(pReg, 0, sizeof(CVec4Dfp4) * 3);
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
						memcpy(pReg, pAttr, sizeof(CVec4Dfp4) * 10);
						pReg += 10;
						pAttr += 10 * 4;
					}
					else
					{
						memset(pReg, 0, sizeof(CVec4Dfp4) * 10);
						pReg += 10;
					}
					m_iConstant_TexGenParam[iTxt][0] = iReg;
					iReg += 10;
				}
				break;

			case CRC_TEXGENMODE_LIGHTING :
				{
					if (pAttr)
					{
						*pReg = *((CVec4Dfp4*)pAttr); pReg++; pAttr += 4;
						*pReg = *((CVec4Dfp4*)pAttr); pReg++; pAttr += 4;
					}
					else 
					{
						*pReg = CVec4Dfp4(0,0,0,1.0f/256.0f); pReg++;
						*pReg = CVec4Dfp4(1,1,1,0); pReg++;
					}
					m_iConstant_TexGenParam[iTxt][0] = iReg;
					iReg += 2;
					break;
				}

			case CRC_TEXGENMODE_TSLV :
				{
					if (pAttr)
					{
						*pReg = *((CVec4Dfp4*)pAttr);
						pAttr += 4;
					}
					else 
					{
						*pReg = CVec4Dfp4(0,0,0,1);
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
						pReg[0] = ((CVec4Dfp4*)pAttr)[0];
						pReg[1] = ((CVec4Dfp4*)pAttr)[1];
						pReg[2] = ((CVec4Dfp4*)pAttr)[2];
						pAttr += 12;
					}
					else 
					{
						pReg[0] = CVec4Dfp4(1,0,0,0);
						pReg[1] = CVec4Dfp4(0,1,0,0);
						pReg[2] = CVec4Dfp4(0,0,1,0);
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
						memcpy(pReg, pAttr, sizeof(CVec4Dfp4) * 8);
						pReg += 8;
						pAttr += 4 * 8;
					}
					else
					{
						memset(pReg, 0, sizeof(CVec4Dfp4) * 8);
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
						memcpy(pReg, pAttr, sizeof(CVec4Dfp4) * 2);
						pReg += 2;
						pAttr += 4 * 2;
					}
					else
					{
						memset(pReg, 0, sizeof(CVec4Dfp4) * 2);
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
						memcpy(pReg, pAttr, sizeof(CVec4Dfp4)); pAttr += 4;
					}
					else 
					{
						*pReg = CVec4Dfp4(0,0,0,0);
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
						pReg[0] = *((CVec4Dfp4*)pAttr);
						pAttr += 4;
					}
					else 
					{
						pReg[0] = CVec4Dfp4(0,0,0,0);
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
						pReg[0] = ((CVec4Dfp4*)pAttr)[0];
						pReg[1] = ((CVec4Dfp4*)pAttr)[1];
						pAttr += 8;
					}
					else
					{
						pReg[0] = CVec4Dfp4(0,0,0,0);
						pReg[1] = CVec4Dfp4(1,1,0,0);
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

	template <class t_CRegisterBatchGetter>
	int SetRegisters_Lights(CVec4Dfp4* &_pRegisters, int _iReg, const CRC_Light* _pLights, int _nLights)
	{

		int nRegsNeeded = _nLights*2;

		CVec4Dfp4 *pReg = _pRegisters;
		int nLeft = t_CRegisterBatchGetter::NewRegisterBatch(pReg, nRegsNeeded);
		M_ASSERT(nLeft >= nRegsNeeded, "Only supported");

		m_iConstant_Lights = _iReg;
	#ifdef CRC_LOGLIGHTS
		ConOut(CStrF("(CRC_VPFormat::SetRegisters_Lights) pL %.8x, nL %d", _pLights, _nLights));
	#endif

		for(int i = 0; i < _nLights; i++)
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
					(*pReg)[0] = fp4(L.m_Color.GetR()) / 255.0f;
					(*pReg)[1] = fp4(L.m_Color.GetG()) / 255.0f;
					(*pReg)[2] = fp4(L.m_Color.GetB()) / 255.0f;
					(*pReg)[3] = fp4(L.m_Color.GetA()) / 255.0f;
					++pReg;

	#ifdef CRC_LOGLIGHTS
					CVec4Dfp4 Amb(
						fp4(L.m_Color.GetR()) / 255.0f,
						fp4(L.m_Color.GetG()) / 255.0f,
						fp4(L.m_Color.GetB()) / 255.0f,
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
					(*pReg)[0] = fp4(L.m_Color.GetR()) / 255.0f;
					(*pReg)[1] = fp4(L.m_Color.GetG()) / 255.0f;
					(*pReg)[2] = fp4(L.m_Color.GetB()) / 255.0f;
					(*pReg)[3] = fp4(L.m_Color.GetA()) / 255.0f;
					++pReg;

	#ifdef CRC_LOGLIGHTS
					CVec4Dfp4 Diff(
						fp4(L.m_Color.GetR()) / 255.0f,
						fp4(L.m_Color.GetG()) / 255.0f,
						fp4(L.m_Color.GetB()) / 255.0f,
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
					(*pReg)[0] = fp4(L.m_Color.GetR()) / 255.0f;
					(*pReg)[1] = fp4(L.m_Color.GetG()) / 255.0f;
					(*pReg)[2] = fp4(L.m_Color.GetB()) / 255.0f;
					(*pReg)[3] = fp4(L.m_Color.GetA()) / 255.0f;
					++pReg;

	#ifdef CRC_LOGLIGHTS
					CVec4Dfp4 Diff(
						fp4(L.m_Color.GetR()) / 255.0f,
						fp4(L.m_Color.GetG()) / 255.0f,
						fp4(L.m_Color.GetB()) / 255.0f,
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

	template <class t_CRegisterBatchGetter>
	int SetRegisters_MatrixPaletteSpecialCube(CVec4Dfp4* &_pRegisters, int _iReg, int _nMaxReg, const CRC_MatrixPalette *_pMatrixPalette)
	{
		m_iConstant_MP = _iReg;
		
		int iReg = _iReg;

		int MaxMat = (_nMaxReg) / 2;
		int nMat = Min((uint32)MaxMat, _pMatrixPalette->m_nMatrices);

		int nRegsNeeded = nMat * 2;

		if (!nRegsNeeded)
			return 0;

		CVec4Dfp4 *pReg = _pRegisters;
		int nLeft = t_CRegisterBatchGetter::NewRegisterBatch(pReg, nRegsNeeded);
		M_ASSERT(nLeft >= nRegsNeeded, "Only supported");

		// Transpose 4x4 and store as 4x3.
		const CVec4Dfp4* pMatArray = (const CVec4Dfp4*)_pMatrixPalette->m_pMatrices;
		for(int i = 0; i < nMat; i++)
		{
			const CVec4Dfp4 *pMat = pMatArray + i*2;

			*pReg = *pMat;
			++pMat;	++pReg;
			*pReg = *pMat;
			++pMat;	++pReg;
		}

		_pRegisters = pReg;

		return nMat*2;
	}

	template <class t_CRegisterBatchGetter>
	int SetRegisters_MatrixPalette(CVec4Dfp4* &_pRegisters, int _iReg, int _nMaxReg, const CRC_MatrixPalette *_pMatrixPalette)
	{
		m_iConstant_MP = _iReg;
		
		int iReg = _iReg;

		int MaxMat = (_nMaxReg) / 3;
		int nMat = Min((uint32)MaxMat, _pMatrixPalette->m_nMatrices);

		int nRegsNeeded = nMat * 3;

		if (!nRegsNeeded)
			return 0;

		CVec4Dfp4 *pReg = _pRegisters;
		int nLeft = t_CRegisterBatchGetter::NewRegisterBatch(pReg, nRegsNeeded);
		M_ASSERT(nLeft >= nRegsNeeded, "Only supported");

		if (_pMatrixPalette->m_piMatrices)
		{
			// Transpose 4x4 and store as 4x3.
			const CMat43fp4_MP* pMatArray = (const CMat43fp4_MP* )_pMatrixPalette->m_pMatrices;
			const uint16 *pIndexArray = _pMatrixPalette->m_piMatrices;
			for(int i = 0; i < nMat; i++)
			{
				const CMat43fp4_MP &Mat = pMatArray[pIndexArray[i]];
//				CMat43fp4_MP Temp;Temp.Unit();const CMat43fp4_MP &Mat = Temp;

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
			const CMat43fp4_MP* pMatArray = (const CMat43fp4_MP*)_pMatrixPalette->m_pMatrices;
			for(int i = 0; i < nMat; i++)
			{
				const CMat43fp4_MP &Mat = pMatArray[i];
//				CMat43fp4_MP Temp; Temp.Unit(); const CMat43fp4_MP &Mat = Temp;

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
	int SetRegisters_MatrixPalette_Quat(CVec4Dfp4* &_pRegisters, int _iReg, int _nMaxReg, const CRC_MatrixPalette *_pMatrixPalette)
	{
		m_iConstant_MP = _iReg;
		
		int iReg = _iReg;

		int MaxMat = (_nMaxReg) / 2;
		int nMat = Min((uint32)MaxMat, _pMatrixPalette->m_nMatrices);

		int nRegsNeeded = nMat * 2;

		if (!nRegsNeeded)
			return 0;

		CVec4Dfp4 *pReg = _pRegisters;
		int nLeft = t_CRegisterBatchGetter::NewRegisterBatch(pReg, nRegsNeeded);
		M_ASSERT(nLeft >= nRegsNeeded, "Only supported");

		if (_pMatrixPalette->m_piMatrices)
		{
			// Transpose 4x4 and store as 4x3.
			const CMat4Dfp4* pMatArray = _pMatrixPalette->m_pMatrices;
			const uint16 *pIndexArray = _pMatrixPalette->m_piMatrices;
			for(int i = 0; i < nMat; i++)
			{
				const CMat4Dfp4 Mat = pMatArray[pIndexArray[i]];

				CQuatfp4 Q;
				CMat4Dfp4 Tmp(Mat);
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
			const CMat4Dfp4* pMatArray = _pMatrixPalette->m_pMatrices;
			for(int i = 0; i < nMat; i++)
			{
				const CMat4Dfp4 Mat = pMatArray[i];
				CQuatfp4 Q;
				CMat4Dfp4 Tmp(Mat);
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

//	int SetRegisters_MatrixPalette_Quat(CVec4Dfp4* _pRegisters, int _iReg, int _iMaxReg, const uint8 *_pValidRegMap, const CMat4Dfp4* _pMatrices, int _nMatrices);
};


class CRC_VPGenerator : public CReferenceCount
{
protected:
	spCRegistry m_spVPSource;
	int m_MaxTextures;

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
