
#include "PCH.h"
#include "MRenderVPGen.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CRC_VPFormat
|__________________________________________________________________________________________________
\*************************************************************************************************/

int CRC_VPFormat::SetRegisters_Init(CVec4Dfp4* _pRegisters, int _Base, int _iReg)
{
	m_iConstant_Base = _Base;

	(*_pRegisters).k[0] = 0;
	(*_pRegisters).k[1] = 1.0;
	(*_pRegisters).k[2] = 0.5;
/*
#ifdef PLATFORM_XENON
	if (m_bUseQuatMatrixPallette)
		(*_pRegisters).k[3] = 2.0f;	// 0.0001 added to avoid precision problems when floor()ing
	else
		(*_pRegisters).k[3] = 3.0f;	// 0.0001 added to avoid precision problems when floor()ing
#else*/
	if (m_bUseQuatMatrixPallette)
		(*_pRegisters).k[3] = 2.0f*255.0001f;	// 0.0001 added to avoid precision problems when floor()ing
	else
		(*_pRegisters).k[3] = 3.0f*255.0001f;	// 0.0001 added to avoid precision problems when floor()ing
//#endif

	++_pRegisters;

	(*_pRegisters).k[0] = 2.0f*255.0001f; // MP cube special multiply
	(*_pRegisters).k[1] = 255.0001f; // MP cube special scale 2
	(*_pRegisters).k[2] = 1/20.0f; // Cube scale
	(*_pRegisters).k[3] = 2.0f/512.0f; // Free

//	_pRegisters[_iReg].k[3] = 2.0f*255.0001f;	// 0.0001 added to avoid precision problems when floor()ing
//	_pRegisters[_iReg].k[3] = -2.0f*254.9999f;	// 0.0001 added to avoid precision problems when floor()ing

	return 2;
}


/*
int CRC_VPFormat::SetRegisters_MatrixPalette_Quat(CVec4Dfp4* _pRegisters, int _iReg, int _iMaxReg, const uint8 *_pValidRegMap, const CMat4Dfp4* _pMatrices, int _nMatrices)
{
	m_iConstant_MP = _iReg;

	// Store 4x3 matrix as quaternion and translation.
	int MaxMat = (_iMaxReg) / 2;

	CVec4Dfp4* pReg = _pRegisters;
	const CMat4Dfp4* pMat = _pMatrices;
	int nMat = Min(MaxMat, _nMatrices);
	for(int i = 0; i < nMat; i++)
	{
		CQuatfp4 Q;
		CMat4Dfp4 Tmp(pMat[i]);
		Tmp.Transpose3x3();
		Q.Create(Tmp);

		int iReg0 = (_pValidRegMap) ? _pValidRegMap[i*2] : _iReg + i*2;
		int iReg1 = (_pValidRegMap) ? _pValidRegMap[i*2+1] : _iReg + i*2+1;
		pReg[iReg0][0] = Q.k[0];
		pReg[iReg0][1] = Q.k[1];
		pReg[iReg0][2] = Q.k[2];
		pReg[iReg0][3] = Q.k[3];
		pReg[iReg1][0] = pMat[i].k[3][0];
		pReg[iReg1][1] = pMat[i].k[3][1];
		pReg[iReg1][2] = pMat[i].k[3][2];
		pReg[iReg1][3] = pMat[i].k[3][3];
	}

	return nMat*2;
}
*/

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CRC_VPGenerator
|__________________________________________________________________________________________________
\*************************************************************************************************/
int CRC_VPGenerator::GetKeyList_r(CRegistry* _pSrc, const char** _lpKeys, int _MaxKeys, const char** _lpDefines, int _nDefines)
{
	int nCh = _pSrc->GetNumChildren();
	int nKeys = 0;
	for(int iCh = 0; iCh < nCh; iCh++)
	{
		CRegistry* pChild = _pSrc->GetChild(iCh);
		CStr KeyName = pChild->GetThisName();

		if (KeyName.Len() >= 3 && CStrBase::strnicmp(KeyName.Str(), "IF_", 3) == 0)
		{
			// if_
			int iDef = 0;
			for(; iDef < _nDefines; iDef++)
				if (CStrBase::stricmp(&KeyName.Str()[3], _lpDefines[iDef]) == 0)
					break;

			if (iDef == _nDefines)
				continue;
		}
		else if (KeyName.Len() >= 6 && CStrBase::strnicmp(KeyName.Str(), "IFNOT_", 6) == 0)
		{
			// ifnot_
			int iDef = 0;
			for(; iDef < _nDefines; iDef++)
				if (CStrBase::stricmp(&KeyName.Str()[6], _lpDefines[iDef]) == 0)
					break;

			if (iDef != _nDefines)
				continue;
		}

		if (pChild->GetNumChildren())
		{
			nKeys += GetKeyList_r(pChild, &_lpKeys[nKeys], _MaxKeys - nKeys, _lpDefines, _nDefines);
		}
		else
		{
			if (nKeys >= _MaxKeys)
				Error("GetKeyList_r", "Too many keys.");

			if (pChild->GetThisValue() != "")
				_lpKeys[nKeys++] = pChild->GetThisValue().Str();
		}
	}

	return nKeys;
}

static bool IsNumeric(const char _Char)
{
	return (_Char >= '0' && _Char <= '9');
}
static bool SubstConstant(const char*& _pVPSrc, const char* _pConst, CFStr& _Dst, int _ConstValue, bool &Neg)
{
	int ConstLen = CStr::StrLen(_pConst);
	if (CStrBase::strnicmp(_pVPSrc, _pConst, ConstLen) == 0)
	{
		_pVPSrc += ConstLen;
		while(CStr::IsWhiteSpace(*_pVPSrc)) _pVPSrc++;
		if (*_pVPSrc == '+')
		{
			int Value = 0;
			_pVPSrc++;
			while(CStr::IsWhiteSpace(*_pVPSrc)) _pVPSrc++;

			if (IsNumeric(*_pVPSrc))
				_pVPSrc += CStrBase::Val_int(_pVPSrc, Value);
			else
				--_pVPSrc;
				
			Value += _ConstValue;

			if (Value < 0)
				Neg = true;

			_Dst.CaptureFormated("%d", Value);
		}
		else if (*_pVPSrc == '-')
		{
			int Value = 0;

			_pVPSrc++;
			while(CStr::IsWhiteSpace(*_pVPSrc)) _pVPSrc++;

			_pVPSrc += CStrBase::Val_int(_pVPSrc, Value);
			Value = -Value;
			Value += _ConstValue;

			if (Value < 0)
				Neg = true;

			_Dst.CaptureFormated("%d", Value);
		}
		else
		{
			if (_ConstValue < 0)
				Neg = true;
			_Dst.CaptureFormated("%d", _ConstValue);
		}

		return true;
	}
	else
		return false;
}

static bool SubstVertex(const char*& _pVPSrc, CFStr& _Dst, uint32 ActiveVertex)
{
	if (_pVPSrc[0] == 'v')
	{
		_pVPSrc += 1;
		int Value = 0;
		_pVPSrc += CStrBase::Val_int(_pVPSrc, Value);
		if (ActiveVertex & (1 << Value))
		{
			_Dst.CaptureFormated("v%d", Value);
			if ((*_pVPSrc) == '.')
			{
				_Dst += ".";
				while (*_pVPSrc)
				{
					++_pVPSrc;					
					switch ((*_pVPSrc))
					{
					case 'x':
					case 'y':
					case 'z':
					case 'w':
						CFStr Temp;
						Temp.Capture(_pVPSrc, 1);
						_Dst += Temp;
						continue;
					}
					break;
				};
			}
		}
		else
		{
			int NumToReplace = 0;
			if ((*_pVPSrc) == '.')
			{
				while (*_pVPSrc)
				{
					++_pVPSrc;					
					switch ((*_pVPSrc))
					{
					case 'x':
					case 'y':
					case 'z':
					case 'w':
						++NumToReplace;
						continue;
					}
					break;
				};
			}

			_Dst = "c[8].";

			if (!NumToReplace)
				NumToReplace = 4;
			while (NumToReplace--)
				_Dst = _Dst + "y";

		}

		return true;
	}
	else
		return false;
}

static bool SubstTexture(const char*& _pVPSrc, const char* _pConst, CFStr& _Dst, const char * _ConstValue)
{
	int ConstLen = CStr::StrLen(_pConst);
	if (CStrBase::strnicmp(_pVPSrc, _pConst, ConstLen) == 0)
	{
		_pVPSrc += ConstLen;
		while(CStr::IsWhiteSpace(*_pVPSrc)) _pVPSrc++;

		_Dst.Capture(_ConstValue);

		return true;
	}
	else
		return false;
}

#ifndef PLATFORM_XBOX1
#define ActiveVertices _Format.m_ProgramGenFormat.m_ActiveVertices
#else
#define ActiveVertices 0xffffffff
#endif

int CRC_VPGenerator::SubstituteConstants(char* _pVPDst, int _MaxDstSize, const char* _pVPSrc, const CRC_VPFormat& _Format)
{
	char* pVPDstOrg = _pVPDst;

//	int Len = CStr::StrLen(_pVPSrc);

	const char* pFind = 0;
	while(pFind = strstr(_pVPSrc, "$"))
	{
		int nCopy = pFind - _pVPSrc;
		memcpy(_pVPDst, _pVPSrc, nCopy);
		_pVPDst += nCopy;
		_pVPSrc += nCopy+1;
		_MaxDstSize -= nCopy;

		bool Neg=false;
		CFStr Subst;
		do
		{
			if (SubstVertex(_pVPSrc, Subst, ActiveVertices)) break;
			if (SubstConstant(_pVPSrc, "MP", Subst, _Format.m_iConstant_Base + _Format.m_iConstant_MP, Neg)) break;
			if (SubstConstant(_pVPSrc, "BASE", Subst, _Format.m_iConstant_Base, Neg)) break;
			if (SubstConstant(_pVPSrc, "CONSTANTCOLOR", Subst, _Format.m_iConstant_Base + _Format.m_iConstant_ConstantColor, Neg)) break;
			if (SubstConstant(_pVPSrc, "POSTRANS", Subst, _Format.m_iConstant_Base + _Format.m_iConstant_PosTransform, Neg)) break;
#ifndef PLATFORM_XENON
			if (SubstConstant(_pVPSrc, "LIGHT", Subst, _Format.m_iConstant_Base + _Format.m_iConstant_Lights, Neg)) break;
			if (SubstConstant(_pVPSrc, "FOGDEPTH", Subst, _Format.m_iConstant_Base + _Format.m_iConstant_FogDepth, Neg)) break;
			if (SubstConstant(_pVPSrc, "CLIPPLANES", Subst, _Format.m_iConstant_Base + _Format.m_iConstant_ClipPlanes, Neg)) break;
#endif
				
			bool bBreak = false;
			for (int i = 0; i < CRC_MAXTEXCOORDS; ++i)
			{
				for (int j = 3; j >= 0; --j)
				{
					if (!j)
					{
						if (SubstConstant(_pVPSrc, CFStrF("TEXPARAM%d", i), Subst, _Format.m_iConstant_Base + _Format.m_iConstant_TexGenParam[i][j], Neg))
						{bBreak = true; break;}
					}
					else
					{
						if (SubstConstant(_pVPSrc, CFStrF("TEXPARAM%d_%d", i, j), Subst, _Format.m_iConstant_Base + _Format.m_iConstant_TexGenParam[i][j], Neg))
						{bBreak = true; break;}
					}
				}
				if (bBreak) break;
			}
			if (bBreak) break;

			bBreak = false;
			for (int i = 0; i < CRC_MAXTEXCOORDS; ++i)
			{
				if (SubstConstant(_pVPSrc, CFStrF("TEXMATRIX%d", i), Subst, _Format.m_iConstant_Base + _Format.m_iConstant_TexMatrix[i], Neg))
				{bBreak = true; break;}
			}
			if (bBreak) break;

			bBreak = false;
			for (int i = 0; i < CRC_MAXTEXCOORDS; ++i)
			{
				if (SubstConstant(_pVPSrc, CFStrF("TEXTURETRANS%d", i), Subst, _Format.m_iConstant_Base + _Format.m_iConstant_TexTransform[i], Neg))
				{bBreak = true; break;}
			}
			if (bBreak) break;

			bBreak = false;
			for (int i = 0; i < CRC_MAXTEXCOORDS; ++i)
			{
				if (SubstTexture(_pVPSrc, CFStrF("TEXINPUT%d", i), Subst, m_TextureSubst[_Format.m_ProgramGenFormat.GetRename(i)]))
				{bBreak = true; break;}
			}
			if (bBreak) break;

			{
				CFStr Source;
				Source.Capture(_pVPSrc, 20);
				Error("SubstituteConstants", CStrF("Invalid constant: %s", Source.Str()));
			}
		}
		while (0);

		CFStr TempStr;
		TempStr = Subst;
		const char *pTempStr = TempStr;		
		if ((*pTempStr) == '$')
		{
			++pTempStr;
			if (*_pVPSrc == '.')
			{
				CFStr TT;
				TT.Capture(_pVPSrc, 1);
				TempStr += TT;
				++_pVPSrc;
				while (*_pVPSrc == 'x' || *_pVPSrc == 'y' || *_pVPSrc == 'z' || *_pVPSrc == 'w')
				{
					CFStr TT;
					TT.Capture(_pVPSrc, 1);
					TempStr += TT;
					++_pVPSrc;
				}
			}
			SubstVertex(pTempStr, Subst, ActiveVertices);
		}

		if (Neg && *(_pVPDst-1) == '+')
			--_pVPDst;

		memcpy(_pVPDst, Subst.Str(), Subst.Len());
		_pVPDst += Subst.Len();
		_MaxDstSize -= Subst.Len();
	}

	int nCopy = CStr::StrLen(_pVPSrc);
	memcpy(_pVPDst, _pVPSrc, nCopy);
	_pVPDst += nCopy;
	_pVPSrc += nCopy;
	_MaxDstSize -= nCopy;
	
	if (_MaxDstSize < 0)
		Error("SubstituteConstants", "Internal error.");
	
	return _pVPDst - pVPDstOrg;
}

void CRC_VPGenerator::Create(spCRegistry _spReg)
{
	m_spVPSource = _spReg;
}


void CRC_VPGenerator::Create(CStr _FileName, CStr _FileNameDef, bool _bLowerCase, int _MaxTextures)
{
	CCFile File;
	File.Open(_FileName, CFILE_BINARY | CFILE_READ);

	m_MaxTextures = _MaxTextures;

	CStr FileStrIn = CStr(' ', File.Length());
	
	File.Read(FileStrIn.GetStr(), File.Length());

	CStr FileStr;
	if (_bLowerCase)
		FileStr = FileStrIn.LowerCase();
	else
		FileStr = FileStrIn;

	spCRegistry spDef = REGISTRY_CREATE;
	if (!spDef)
		MemError("Create");
	spDef->XRG_Read(_FileNameDef);

	spDef = spDef->GetChild(0);
	
	
	{
		for (int i = 0; i < _MaxTextures; ++i)
		{
			CFStr Def = CFStrF("V_TEX%d",i);
			
			int iCh = 0;
			for(; iCh < spDef->GetNumChildren(); iCh++)
			{
				if (spDef->GetName(iCh).CompareNoCase(Def) == 0)
				{
					CStr Str = spDef->GetValue(iCh);
					strcpy(m_TextureSubst[i],Str);
					break;
				}
			}
			
			if (iCh == spDef->GetNumChildren())
				Error("Create", CStrF("Cant find texture define %d", i));
		}
	}
	
	
	TArray<char> lFileOut;
	lFileOut.SetGrow(File.Length());
	
	const char* pIn = FileStr.Str();
	int i = 0;
	int nLen = FileStr.Len();
	while(i < nLen)
	{
		if (pIn[i] == '@')
		{
			i++;
			int iStart = i;
			while((pIn[i] >= 'a' && pIn[i] <= 'z') ||
				(pIn[i] >= 'A' && pIn[i] <= 'Z') ||
				(pIn[i] >= '0' && pIn[i] <= '9') ||
				(pIn[i] == '_')) i++;
			
			CFStr Def;
			Def.Capture(&pIn[iStart], i - iStart);
			
			int iCh = 0;
			for(; iCh < spDef->GetNumChildren(); iCh++)
				if (spDef->GetName(iCh).CompareNoCase(Def) == 0)
				{
					CStr Str = spDef->GetValue(iCh);
					for(int j = 0; j < Str.Len(); j++)
						lFileOut.Add(Str.GetStr()[j]);
					
					break;
				}
				
				if (iCh == spDef->GetNumChildren())
					Error("Create", CStrF("Invalid define %s", Def.Str()));

		}
		else
			lFileOut.Add(pIn[i++]);
	}

	spCRegistry spVPSource = REGISTRY_CREATE;
	if (!spVPSource)
		MemError("Create");
	CRegistry_ParseContext Context;
	Context.m_ThisFileName = _FileName;
	spVPSource->XRG_Parse(lFileOut.GetBasePtr(), lFileOut.GetBasePtr(), lFileOut.Len(), Context);

	Create(spVPSource);
}

CStr CRC_VPGenerator::CreateVP(const CRC_VPFormat& _Format)
{
	const char* lpKeys[256];
	const char* lpDefines[128];
	int nDefines = 0;

	// ----------------------------------------------------------------
	// Create defines
	CFStr DefMWComp = CFStrF("MWCOMP%d", _Format.m_ProgramGenFormat.m_nMWComp);
	lpDefines[nDefines++] = DefMWComp.Str();

	CFStr TexCoordIn[CRC_MAXTEXCOORDS];
	CFStr TexCoordOut[CRC_MAXTEXCOORDS];

	int MaxTexcoord;
#if DEF_CRC_MAXTEXTURES > 4
	MaxTexcoord = _Format.m_ProgramGenFormat.m_MultiTexture?_Format.m_ProgramGenFormat.m_MultiTexture:m_MaxTextures;
#else
	MaxTexcoord = CRC_MAXTEXCOORDS;
#endif
	for( int i = 0; i < CRC_MAXTEXCOORDS; i++ )
	{
		TexCoordIn[i].CaptureFormated( "TEXCOORDIN%d", i );
		lpDefines[nDefines++] = TexCoordIn[i].Str();
	}
	for( int i = 0; i < MaxTexcoord; i++ )
	{
		TexCoordOut[i].CaptureFormated( "TEXCOORDOUT%d", i );
		lpDefines[nDefines++] = TexCoordOut[i].Str();
	}

	if (_Format.m_bUseQuatMatrixPallette)
		lpDefines[nDefines++] = "MPQuat";
		

	// Lights
#ifndef PLATFORM_XENON
	if (_Format.m_ProgramGenFormat.m_Lighting)
		lpDefines[nDefines++] = "LIGHTING";
#endif

	// Colorvertex
	if (!_Format.m_ProgramGenFormat.m_bNoVertexColorComponent)
		lpDefines[nDefines++] = "COLORVERTEX";

	// Colorvertex
	if (!_Format.m_ProgramGenFormat.m_bNoColorOutput)
		lpDefines[nDefines++] = "COLOROUTPUT";	

#ifndef PLATFORM_XBOX1
	if (_Format.m_ProgramGenFormat.m_bNoNormal)
		lpDefines[nDefines++] = "NONORMAL";
#endif
	
	// UseNormal
	if (_Format.m_ProgramGenFormat.m_bUseNormal)
		lpDefines[nDefines++] = "USENORMAL";

	// UseTangents
	if (_Format.m_ProgramGenFormat.m_bUseTangents)
		lpDefines[nDefines++] = "USETANGENTS";

	// Normalize normal
	if (_Format.m_ProgramGenFormat.m_bNormalizeNormal)
		lpDefines[nDefines++] = "NORMALIZENORMAL";
#ifndef PLATFORM_XENON
	// DepthFog
	if (_Format.m_ProgramGenFormat.m_FogDepth)
		lpDefines[nDefines++] = "FOGDEPTH";
#endif

	// VertexTransform
	if (_Format.m_ProgramGenFormat.m_VertexTransform)
		lpDefines[nDefines++] = "POSTRANS";
#ifndef PLATFORM_XENON

	// ClipPlanes
	if (_Format.m_ProgramGenFormat.m_bClipPlanes)
		lpDefines[nDefines++] = "CLIPPLANES";
#endif

	if (_Format.m_ProgramGenFormat.m_MPFlags & EMPFlags_SpecialCubeVec)
		lpDefines[nDefines++] = "CubeVec";

	if (_Format.m_ProgramGenFormat.m_MPFlags & EMPFlags_SpecialCubeTex)
		lpDefines[nDefines++] = "CubeTex";

	if (_Format.m_ProgramGenFormat.m_MPFlags & EMPFlags_SpecialCubeTexScaleZ)
		lpDefines[nDefines++] = "CubeTexSclZ";

	if (_Format.m_ProgramGenFormat.m_MPFlags & EMPFlags_SpecialCubeTexScale2)
		lpDefines[nDefines++] = "CubeTexScl2";

	
#ifndef PLATFORM_XENON

	CFStr LightTypes[CRC_MAXLIGHTS];
	{
		for(int i = 0; i < _Format.m_ProgramGenFormat.m_nLights; i++)
		{
			int Type = (_Format.m_ProgramGenFormat.m_LightTypes >> (i*2)) & 3;
			switch(Type)
			{
			case 3 :
				LightTypes[i].CaptureFormated("LIGHT%d_AMBIENT", i);
				break;
			case CRC_LIGHTTYPE_POINT :
				LightTypes[i].CaptureFormated("LIGHT%d_POINT", i);
				break;
			case CRC_LIGHTTYPE_PARALLELL :
				LightTypes[i].CaptureFormated("LIGHT%d_PARALLELL", i);
				break;

			default :
				Error("CreateVP", CStrF("Unsupported light type %d", Type));
			}
			lpDefines[nDefines++] = LightTypes[i];
		}
	}
#endif

	// TexGen, TexTransform
	CFStr TexGenMode[CRC_MAXTEXCOORDS];
	CFStr TexMatrix[CRC_MAXTEXCOORDS];
	CFStr TexTransform[CRC_MAXTEXCOORDS];
	CFStr TexGenComp[CRC_MAXTEXCOORDS][4];
	{
		for(int i = 0; i < MaxTexcoord; i++)
		{
			uint32 Components = _Format.m_ProgramGenFormat.GetTexGenComponents(i);
			for (int j = 0; j < 4;++j)
			{
				if (Components & (1 << j))
				{
					TexGenComp[i][j].CaptureFormated("TEXGEN%d_COMP%d", i, j);
					lpDefines[nDefines++] = TexGenComp[i][j];
				}
			}
			switch(_Format.m_ProgramGenFormat.GetTexGen(i))
			{
			case CRC_TEXGENMODE_TEXCOORD :
				TexGenMode[i].CaptureFormated("TEXGEN%d_TEXCOORD", i);
				break;

			case CRC_TEXGENMODE_LINEAR :
				TexGenMode[i].CaptureFormated("TEXGEN%d_LINEAR", i);
				break;

			case CRC_TEXGENMODE_LINEARNHF :
				TexGenMode[i].CaptureFormated("TEXGEN%d_LINEARNHF", i);
				break;

			case CRC_TEXGENMODE_BOXNHF :
				TexGenMode[i].CaptureFormated("TEXGEN%d_BOXNHF", i);
				break;

			case CRC_TEXGENMODE_NORMALMAP :
				TexGenMode[i].CaptureFormated("TEXGEN%d_NORMALMAP", i);
				break;

			case CRC_TEXGENMODE_REFLECTION :
				TexGenMode[i].CaptureFormated("TEXGEN%d_REFLECTION", i);
				break;

			case CRC_TEXGENMODE_ENV :
				TexGenMode[i].CaptureFormated("TEXGEN%d_ENV", i);
				break;

			case CRC_TEXGENMODE_LIGHTING :
				TexGenMode[i].CaptureFormated("TEXGEN%d_LIGHTING", i);
				break;

			case CRC_TEXGENMODE_TSLV :
				TexGenMode[i].CaptureFormated("TEXGEN%d_TSLV", i);
				break;

			case CRC_TEXGENMODE_PIXELINFO :
				TexGenMode[i].CaptureFormated("TEXGEN%d_PIXELINFO", i);
				break;

			case CRC_TEXGENMODE_TANG_U :
				TexGenMode[i].CaptureFormated("TEXGEN%d_TANG_U", i);
				break;

			case CRC_TEXGENMODE_TANG_V :
				TexGenMode[i].CaptureFormated("TEXGEN%d_TANG_V", i);
				break;

			case CRC_TEXGENMODE_BUMPCUBEENV :
				TexGenMode[i].CaptureFormated("TEXGEN%d_BUMPCUBEENV", i);
				break;

			case CRC_TEXGENMODE_VOID :
				TexGenMode[i].CaptureFormated("TEXGEN%d_VOID", i);
				break;

			case CRC_TEXGENMODE_NULL :
				TexGenMode[i].CaptureFormated("TEXGEN%d_NULL", i);
				break;

			case CRC_TEXGENMODE_TSREFLECTION :
				TexGenMode[i].CaptureFormated("TEXGEN%d_TSREFLECTION", i);
				break;

			case CRC_TEXGENMODE_SHADOWVOLUME :
				TexGenMode[i].CaptureFormated("TEXGEN%d_SHADOWVOLUME", i);
				break;

			case CRC_TEXGENMODE_SHADOWVOLUME2 :
				TexGenMode[i].CaptureFormated("TEXGEN%d_SHADOWVOLUME2", i);
				break;

			case CRC_TEXGENMODE_CONSTANT :
				TexGenMode[i].CaptureFormated("TEXGEN%d_CONSTANT", i);
				break;				

			case CRC_TEXGENMODE_MSPOS :
				TexGenMode[i].CaptureFormated("TEXGEN%d_MSPOS", i);
				break;

			case CRC_TEXGENMODE_SCREEN :
				TexGenMode[i].CaptureFormated("TEXGEN%d_SCREEN", i);
				break;			

			case CRC_TEXGENMODE_DECALTSTRANSFORM:
				TexGenMode[i].CaptureFormated("TEXGEN%d_DECALTSTRANSFORM",i);
				break;

			case CRC_TEXGENMODE_DEPTHOFFSET :
				TexGenMode[i].CaptureFormated("TEXGEN%d_DEPTHOFFSET", i);
				break;

			default :
				ConOut(CStrF("(CRC_VPGenerator::CreateVP) Unsupported texgen mode %d", _Format.m_ProgramGenFormat.GetTexGen(i)));
				TexGenMode[i].CaptureFormated("TEXGEN%d_NONE", i);
				break;
			}
			lpDefines[nDefines++] = TexGenMode[i];

			if (_Format.m_ProgramGenFormat.m_TexMatrix & (1 << i))
			{
				TexMatrix[i].CaptureFormated("TEXMATRIX%d", i);
				lpDefines[nDefines++] = TexMatrix[i];
			}

			if (_Format.m_ProgramGenFormat.m_TextureTransform & (1 << _Format.m_ProgramGenFormat.GetRename(i)))
			{
				TexTransform[i].CaptureFormated("TEXTURETRANS%d", i);
				lpDefines[nDefines++] = TexTransform[i];
			}
		}
#ifndef PLATFORM_CONSOLE
		for( int iTexCoord = _Format.m_ProgramGenFormat.m_MultiTexture; iTexCoord < CRC_MAXTEXCOORDS; iTexCoord++ )
		{
			TexGenMode[iTexCoord].CaptureFormated("TEXGEN%d_VOID", iTexCoord );
			lpDefines[nDefines++]	= TexGenMode[iTexCoord];
		}
#endif
	}

	if (nDefines > 128)
		Error("CreateVP", "Internal error. (1)");

	// ----------------------------------------------------------------
	// Eval VP source
	int nKeys = GetKeyList_r(m_spVPSource, lpKeys, 256, lpDefines, nDefines);
	if (nKeys > 256)
		Error("CreateVP", "Internal error. (2)");

	// Calc program size
	int ProgramSize = 0;
	{
		for(int i = 0; i < nKeys; i++)
			ProgramSize += CStr::StrLen(lpKeys[i]);
	}
	ProgramSize++;

	CStr VP = CStr(' ', ProgramSize);

	{
		int Pos = 0;
		for(int i = 0; i < nKeys; i++)
		{
			Pos += SubstituteConstants(&VP.GetStr()[Pos], ProgramSize - Pos, lpKeys[i], _Format);
			if (Pos > ProgramSize)
				Error("CreateVP", "Internal error. (3)");
		}

		VP.GetStr()[Pos] = 0;
		Pos++;

		// Convert all tabs to spaces to make error indicator work.
		{
			char *pVP = VP.GetStr();
			for(int i = 0; i < Pos; i++)
				if (pVP[i] == char(9))
					pVP[i] = ' ';
		}


		if (Pos > ProgramSize)
			Error("CreateVP", "Internal error. (4)");
	}

	return VP;
}

//
//
// @if support_normalize
// NRM result.xyz, source.xyz;
// @else
// DP3 result.w, result.xyz, result.xyz;
// RSQ result.w, result.w;
// MUL result.xyz, result.xyz, result.w;
// @endif
//
// @if platform_xenon || platform_ps3
// @elifnot platform_pc
// @endif
//

static void CopyLine(char* &_pSrc, char* &_pDst)
{
	char* pStart = _pSrc;
	while(*_pSrc)
	{
		char c = *_pSrc++;
		if(c == 10)
			break;
	}


	if(pStart != _pSrc)
	{
		uint32 nCount = (uint32)(_pSrc - pStart);
		memcpy(_pDst, pStart, nCount);
		_pDst	+= nCount;
	}
}

static void SkipLine(char*& _pStr)
{
	while(*_pStr)
	{
		char c = *_pStr++;
		if(c == 10)
			break;
	}
}

static char* SkipToEndOfToken(char* _pStr)
{
	while(*_pStr)
	{
		if(CStrBase::IsWhiteSpace(*_pStr))
			break;
		_pStr++;
	}

	return _pStr;
}

static bool ScanDefines(CFStr _Statement, const char* _lpDefines[])
{
	bool aStack[2] = {false, false};
	int nStack = 0;
	int nOperator = 0;
	_Statement.Trim();
	while(_Statement.Len() > 0)
	{
		bool bFound = false;
		CFStr Word = _Statement.GetStrMSep(" \t");
		if(Word == "||")
			nOperator = 1;
		else if(Word == "&&")
			nOperator = 2;
		else
		{
			for(int i = 0; _lpDefines[i]; i++)
			{
				if(Word == _lpDefines[i])
				{
					bFound = true;
					break;
				}
			}

			aStack[nStack++]	= bFound;

			if(nStack == 2)
			{
				M_ASSERT(nOperator == 1 || nOperator == 2, "No operator between defines");

				bool bValue1 = aStack[0];
				bool bValue2 = aStack[1];
				nStack = 0;
				aStack[nStack++] = (nOperator==1)?(bValue1 || bValue2):(bValue1 && bValue2);
			}
		}
	}

	return aStack[0];
}

static bool EvalStatement(bool& _bPreviousStatement, CFStr _Statement, const char* _lpDefines[])
{
	bool bReturn = false;
	CFStr Keyword = _Statement.GetStrMSep(" \t\r\n");
	Keyword.MakeLowerCase();
	uint32 KeyHash = StringToHash(Keyword.GetStr());
	if(KeyHash == MHASH1('@if') || KeyHash == MHASH2('@ifn', 'ot'))
	{
		bReturn = ScanDefines(_Statement, _lpDefines);
		if(KeyHash == MHASH2('@ifn', 'ot'))
			bReturn = !bReturn;

		_bPreviousStatement = bReturn;
	}
	else if(KeyHash == MHASH2('@eli', 'f') || KeyHash == MHASH2('@eli', 'fnot'))
	{
		if(_bPreviousStatement == false)
		{
			// These statements are only taken if previous statement hasn't been
			bReturn = ScanDefines(_Statement, _lpDefines);
			if(KeyHash == MHASH2('@eli', 'fnot'))
				bReturn = !bReturn;

			_bPreviousStatement = bReturn;
		}
	}
	else if(KeyHash == MHASH2('@els', 'e'))
	{
		if(_bPreviousStatement == false)
		{
			bReturn = true;
			_bPreviousStatement = true;
		}
	}
	else if(KeyHash == MHASH2('@end', 'if'))
	{
		// End of statement
		_bPreviousStatement = false;
		bReturn = true;
	}
	else
	{
		Error_static("EvalStatement", CStrF("%s is not a valid preparser command", Keyword.Str()));
	}

	return bReturn;
}

void FPPreParser_Inplace(char* _pSource, const char* _lpDefines[])
{
	bool bCopy = true;
	char* pSrc = _pSource;
	char* pDst = _pSource;
	bool bPreviousStatement = false;

	while(*pSrc)
	{
		char c = *pSrc;
		if(CStrBase::IsWhiteSpace(c))
		{
			if(bCopy)
				*pDst++	= *pSrc++;
			else if(c == 10)
			{
				*pDst++	= '\n';
				pSrc++;
			}
			else
				pSrc++;
		}
		else
		{
			// If line does not start with a @ character then it's not a preparser line so just skip it
			if(c != '@')
			{
				if(bCopy)
					CopyLine(pSrc, pDst);
				else
				{
					*pDst++	= '\n';
					SkipLine(pSrc);
				}
			}
			else
			{
				// Preparser thingy
				const char* pToken = pSrc;
				SkipLine(pSrc);
				CFStr Statement;
				Statement.Capture(pToken, pSrc - pToken);
				if(EvalStatement(bPreviousStatement, Statement, _lpDefines))
				{
					// This statement should be used
					*pDst++ = '\n';
					bCopy = true;
				}
				else
				{
					// 
					*pDst++ = '\n';
					bCopy = false;
				}
			}
		}
	}
	*pDst = 0;
}
