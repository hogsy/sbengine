
#include "PCH.h"
#include "MRenderPS3_Context.h"

#define	PS3_FPCACHE_VERSION	1

const char* aPreparseDefines[] =
{
	"platform_ps3",
	"playstation3",
	"support_normalize",
	NULL
};

const char* aSubstituteNames[] =
{
	"texture0", "texture1", "texture2", "texture3", "texture4", "texture5", "texture6", "texture7", "texture8", "texture9", "texture10", "texture11", "texture12", "texture13", "texture14", "texture15", NULL
};

const char* aOriginalNames[] =
{
	"texture[0]", "texture[1]", "texture[2]", "texture[3]", "texture[4]", "texture[5]", "texture[6]", "texture[7]", "texture[8]", "texture[9]", "texture[10]", "texture[11]", "texture[12]", "texture[13]", "texture[14]", "texture[15]", NULL
};

const char* aTexOps[] =
{
	"TEX", "TXP", NULL
};

int Translate_Int(const char* _pVal, const char** _pStrings)
{
	MAUTOSTRIP(Translate_Int, 0);
//	int Flags = 0;
	CStr Val(_pVal);
	while(Val != "")
	{
		CStr s = Val.GetStrMSep(" ,+");
		s.Trim();
		for(int i = 0; _pStrings[i]; i++)
			if (s.CompareNoCase(_pStrings[i]) == 0)
				return i;
	}
	return -1;
}

void RenameSamplers(CFStr& _Line, const char** _ppOriginal, const char** _ppSub)
{
	CFStr Copy;

	for(int i = 0; aOriginalNames[i]; i++)
	{
		int iPos;
		while((iPos = _Line.Find(aOriginalNames[i])) >= 0)
		{
			Copy = _Line.Copy(0, iPos);
			Copy += aSubstituteNames[i];
			Copy += _Line.GetStr() + iPos + strlen(aOriginalNames[i]);
			_Line = Copy;
		}
	}
}

uint32 ScanForStrings(CFStr _String, const char** _ppStrings);

CStr FPIncludeHeader;

const char* aSamplerTypes[] = {"2D", "3D", "CUBE", NULL};
CStr GenerateHeader(uint32 _SamplerMask, uint8* _pSamplerType, uint32 _MaxUsedConstant, CFStr* _lpAttribNames, const CStr& _OutName)
{
	if(FPIncludeHeader.Len() == 0)
	{
		FPIncludeHeader.ReadFromFile("System/PS3/FPInclude_PS3.xrg");
	}
//	CStr Header = "#include \"FPInclude_PS3.xrg\"\n";
	CStr Header = FPIncludeHeader;

	for(int iSampler = 0; iSampler < 32; iSampler++)
	{
		if(_SamplerMask & (1 << iSampler))
			Header += CStrF("uniform sampler%s texture%d : TEXUNIT%d;\n", aSamplerTypes[_pSamplerType[iSampler]], iSampler, iSampler);
	}

	if(_MaxUsedConstant > 0)
		Header += CStrF("uniform float4 c[%d];\n", _MaxUsedConstant);

	Header += CStrF("void main(out float4 %s : COLOR", _OutName.GetStr());

	for(int i = 0; i < 8; i++)
	{
		if(_lpAttribNames[i].Len() > 0)
		{
			_lpAttribNames[i].MakeLowerCase();
			Header += CStrF(", in float4 %s : TEXCOORD%d", _lpAttribNames[i].GetStr(), i);
		}
	}

	if(_lpAttribNames[8].Len() > 0)
	{
		_lpAttribNames[8].MakeLowerCase();
		Header += CStrF(", in float4 %s : COLOR", _lpAttribNames[8].GetStr());
	}

	Header += ")\n{\n";

	return Header;
}

CStr GenerateTemps(TArray<CFStr> _lTemps)
{
	CStr Temps;
	for(int i = 0; i < _lTemps.Len(); i++)
	{
		Temps += CStrF("\tfloat4 %s;\n", _lTemps[i].GetStr());
	}

	return Temps;
}

CStr GenerateParams(TArray<CFStr> _lNames, TArray<CFStr> _lValues)
{
	CStr Temps;
	for(int i = 0; i < _lNames.Len(); i++)
	{
		Temps += CStrF("\tfloat4 %s = %s;\n", _lNames[i].GetStr(), _lValues[i].GetStr());
	}

	return Temps;
}

int GetNextNonWhite(const char* _pFP, int _iCurrentPos, int _nLen)
{
	while(_iCurrentPos < _nLen)
	{
		if(CStrBase::IsWhiteSpace(_pFP[_iCurrentPos]))
			_iCurrentPos++;
		else
			return _iCurrentPos;
	}

	return -1;
}

bool FindChar(const char _c, const char* _pSep)
{
	while(*_pSep)
	{
		if(*_pSep == _c)
			return true;
		_pSep++;
	}

	return false;
}

int GetNextSep(const char* _pFP, int _iCurrentPos, int _nLen, const char* _pSep)
{
	while(_iCurrentPos < _nLen)
	{
		if(FindChar(_pFP[_iCurrentPos], _pSep))
			break;
		_iCurrentPos++;
	}

	return _iCurrentPos;
}

int SkipToNextLine(const char* _pFP, int _iCurrentPos, int _nLen)
{
	while(_iCurrentPos < _nLen)
	{
		if(_pFP[_iCurrentPos++] == '\n')
			break;
	}

	return _iCurrentPos;
}

int GetNextSymbol(const char* _pFP, int _iCurrentPos, int _nLen, int& _iSymbolStart, int& _iSymbolEnd)
{
	_iSymbolStart = -1;
	_iSymbolEnd = -1;

	int iSymbolStart = GetNextNonWhite(_pFP, _iCurrentPos, _nLen);
	if(iSymbolStart >= 0)
	{
		_iSymbolStart = iSymbolStart;
		_iSymbolEnd = GetNextSep(_pFP, iSymbolStart + 1, _nLen, " \t\r\n,;");

		_iCurrentPos = _iSymbolEnd;
	}

	return _iCurrentPos;
}

CStr ConvertFPToCG(CStr _FP, const char* _pShaderName)
{
	CStr CG;
	CFStr OutputVar;
	int iCurrentPos = 0;
	int nLen = _FP.Len();
	TArray<CFStr> lTemps;
	TArray<CFStr> lConstantName;
	TArray<CFStr> lConstantValue;
	CFStr lAttribNames[9];
	uint8 SamplerTypes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	lTemps.SetGrow(32);
	lConstantName.SetGrow(32);
	lConstantValue.SetGrow(32);
	const char* pFP = _FP.GetStr();

	uint32 UsedSamplers = 0;
	uint32 MaxUsedConstant = 0;

	while(iCurrentPos < nLen)
	{
		int iSymStart, iSymEnd;
		iCurrentPos = GetNextSymbol(pFP, iCurrentPos, nLen, iSymStart, iSymEnd);
		if(iSymStart >= 0)
		{
			if((pFP[iSymStart] == '#') || (pFP[iSymStart] == '!'))
			{
				// Comment, skip to end of line
				iCurrentPos = SkipToNextLine(pFP, iSymStart, nLen);
			}
			else if(!CStrBase::strnicmp(pFP + iSymStart, "OPTION", 6))
			{
				iCurrentPos = SkipToNextLine(pFP, iCurrentPos, nLen);
			}
			else
			{
				if(!CStrBase::strnicmp(pFP + iSymStart, "END", 3))
					break;
				else if(!CStrBase::strnicmp(pFP + iSymStart, "OUTPUT", 6))
				{
					int iTempStart, iTempStop;
					iCurrentPos = GetNextSymbol(pFP, iCurrentPos, nLen, iTempStart, iTempStop);
					if(iTempStart == -1)
					{
						LogFile(CStrF("Syntax error in fragment program '%s'", _pShaderName));
						break;
					}
					int iEqStart, iEqStop;
					iCurrentPos = GetNextSymbol(pFP, iCurrentPos, nLen, iEqStart, iEqStop);
					if(iEqStart == -1)
					{
						LogFile(CStrF("Syntax error in fragment program '%s'", _pShaderName));
						break;
					}

					int iVarStart, iVarStop;
					iCurrentPos = GetNextSymbol(pFP, iCurrentPos, nLen, iVarStart, iVarStop);
					if(iVarStart == -1)
					{
						LogFile(CStrF("Syntax error in fragment program '%s'", _pShaderName));
						break;
					}

					OutputVar.Capture(pFP + iTempStart, iTempStop - iTempStart);
					OutputVar.MakeLowerCase();
				}
				else if(!CStrBase::strnicmp(pFP + iSymStart, "PARAM", 5))
				{
					int iNameStart, iNameStop;
					iCurrentPos = GetNextSymbol(pFP, iCurrentPos, nLen, iNameStart, iNameStop);
					if(iNameStart == -1)
					{
						LogFile(CStrF("Syntax error in fragment program '%s'", _pShaderName));
						break;
					}
					CFStr Name;
					Name.Capture(pFP + iNameStart, iNameStop - iNameStart);

					int iEqStart, iEqStop;
					iCurrentPos = GetNextSymbol(pFP, iCurrentPos, nLen, iEqStart, iEqStop);
					if(iEqStart == -1)
					{
						LogFile(CStrF("Syntax error in fragment program '%s'", _pShaderName));
						break;
					}

					int iParamStart = GetNextNonWhite(pFP, iCurrentPos, nLen);
					int iParamEnd = GetNextSep(pFP, iCurrentPos, nLen, ";");

					CFStr Value;
					if(pFP[iParamStart] == '{')
					{
						// program constant
						Value.Capture(pFP + iParamStart, iParamEnd - iParamStart);
					}
					else
					{
						int iSizeStart = GetNextSep(pFP, iParamStart, nLen, "[");
						int iSizeStop = GetNextSep(pFP, iParamStart, nLen, "]");

						CFStr ConstIndex;
						ConstIndex.Capture(pFP + iSizeStart + 1, iSizeStop - iSizeStart - 1);
						MaxUsedConstant = Max(MaxUsedConstant, (uint32)ConstIndex.Val_int() + 1);

						CFStr Temp;
						Temp.Capture(pFP + iSizeStart, iSizeStop - iSizeStart + 1);
						Value = CFStrF("c%s", Temp.GetStr());
					}

					Name.MakeLowerCase();
					Value.MakeLowerCase();
					lConstantName.Add(Name);
					lConstantValue.Add(Value);
					iCurrentPos = iParamEnd;
				}
				else if(!CStrBase::strnicmp(pFP + iSymStart, "ATTRIB", 6))
				{
					int iNameStart, iNameStop;
					iCurrentPos = GetNextSymbol(pFP, iCurrentPos, nLen, iNameStart, iNameStop);
					if(iNameStart == -1)
					{
						LogFile(CStrF("Syntax error in fragment program '%s'", _pShaderName));
						break;
					}
					CFStr Name;
					Name.Capture(pFP + iNameStart, iNameStop - iNameStart);

					int iEqStart, iEqStop;
					iCurrentPos = GetNextSymbol(pFP, iCurrentPos, nLen, iEqStart, iEqStop);
					if(iEqStart == -1)
					{
						LogFile(CStrF("Syntax error in fragment program '%s'", _pShaderName));
						break;
					}

					int iTexStart, iTexStop;
					iCurrentPos = GetNextSymbol(pFP, iCurrentPos, nLen, iTexStart, iTexStop);
					if(iTexStart == -1)
					{
						LogFile(CStrF("Syntax error in fragment program '%s'", _pShaderName));
						break;
					}
					if(!CStrBase::strnicmp(pFP + iTexStart, "fragment.color", 14))
					{
						lAttribNames[8] = Name;
					}
					else
					{
						if(CStrBase::strnicmp(pFP + iTexStart, "fragment.texcoord[", 18))
						{
							LogFile(CStrF("Syntax error in fragment program '%s'", _pShaderName));
							break;
						}
						int iAttrib = pFP[iTexStart + 18] - '0';
						Name.MakeLowerCase();
						lAttribNames[iAttrib] = Name;
					}
				}
				else if(!CStrBase::strnicmp(pFP + iSymStart, "TEMP", 4))
				{
					int iVarStart, iVarStop;
					iCurrentPos = GetNextSymbol(pFP, iCurrentPos, nLen, iVarStart, iVarStop);
					CFStr Temp;
					Temp.Capture(pFP + iVarStart, iVarStop - iVarStart);
					Temp.MakeLowerCase();
					lTemps.Add(Temp);
				}
				else
				{
					// Actual opcode (or something else that's broken)
					int iEOL = GetNextSep(pFP, iSymStart, nLen, ";");
					CFStr Line;
					Line.Capture(pFP + iSymStart, iEOL - iSymStart);
					Line.Trim();

					CFStr Opcode = Line.GetStrMSep(" \t");
					Opcode.MakeUpperCase();

					if(ScanForStrings(Opcode, aTexOps))
					{
						RenameSamplers(Line, aOriginalNames, aSubstituteNames);
						int iSampler = ScanForStrings(Line, aSubstituteNames);
						UsedSamplers |= iSampler;

						int iType = Line.FindReverse(",");
						CFStr Type;
						Type.Capture(Line.GetStr() + iType + 1);
						Type.Trim();
						Line.GetStr()[iType] = 0;
						Opcode += Type;

						SamplerTypes[Log2(iSampler)] = Translate_Int(Type.GetStr(), aSamplerTypes);
					}
					else if(Opcode == "SWZ")
					{
						// Swizzle is special
					}

					Line.MakeLowerCase();
					CFStr Dest = Line.GetStrSep(",");
					Dest.Trim();
					CFStr DestMask;
					int iDestMask = Dest.FindReverse(".");
					if(iDestMask >= 0)
					{
						DestMask.Capture(Dest.Str() + iDestMask, Dest.Len() - iDestMask);
					}

					Line.Trim();
					if(Opcode == "SWZ")
					{
						// Swizzle is special
						CFStr Source = Line.GetStrSep(",");
						CFStr x = Line.GetStrSep(",");
						CFStr y = Line.GetStrSep(",");
						CFStr z = Line.GetStrSep(",");
						CFStr w = Line.GetStrSep(",");
						Source.Trim();
						x.Trim();
						y.Trim();
						z.Trim();
						w.Trim();
						bool bXNeg = x.GetStr()[0] == '-';
						bool bYNeg = y.GetStr()[0] == '-';
						bool bZNeg = z.GetStr()[0] == '-';
						bool bWNeg = w.GetStr()[0] == '-';
						if(!x.IsNumeric())
							x = CFStrF("%s%s.%s", bXNeg?"-":"", Source.GetStr(), x.GetStr());
						if(!y.IsNumeric())
							y = CFStrF("%s%s.%s", bYNeg?"-":"", Source.GetStr(), y.GetStr());
						if(!z.IsNumeric())
							z = CFStrF("%s%s.%s", bZNeg?"-":"", Source.GetStr(), z.GetStr());
						if(!w.IsNumeric())
							w = CFStrF("%s%s.%s", bWNeg?"-":"", Source.GetStr(), w.GetStr());

						Opcode = "float4";
						Line = CFStrF("%s, %s, %s, %s", x.Str(), y.Str(), z.Str(), w.Str());
					}

					CFStr CGLine = "\t" + Dest + " = " + Opcode + "(" + Line + ")" + DestMask + ";\n";

					CG += CGLine;
					iCurrentPos = iEOL;
				}

				iCurrentPos = GetNextNonWhite(pFP, iCurrentPos, nLen);
				if(pFP[iCurrentPos] != ';')
				{
					LogFile(CStrF("Syntax error in fragment program '%s'", _pShaderName));
					break;
				}
				iCurrentPos++;
			}
		}
		else
			break;
	}

	CStr CGHeader = GenerateHeader(UsedSamplers, SamplerTypes, MaxUsedConstant, lAttribNames, OutputVar);
	CStr CGParams = GenerateParams(lConstantName, lConstantValue);
	CStr CGTemps = GenerateTemps(lTemps);

	return CGHeader + CGParams + CGTemps + CG + "}\n";
}

void CRenderContextPS3::FP_Bind( const char* _pProgramName )
{
}

void CRenderContextPS3::FP_Bind( CRC_ExtAttributes_FragmentProgram20* _pExtAttr )
{
#ifndef PLATFORM_PS3
	return;
#endif
	int Hash = _pExtAttr->m_ProgramNameHash;
	if(Hash == 0) Hash = StringToHash(_pExtAttr->m_pProgramName);
	CCGFPProgram* pProg = m_FPProgramTree.FindEqual(Hash);
	if(!pProg)
	{
/*
		CFStr CacheFilename = CFStrF("System/PS3/Cache/%s.cgf", _pExtAttr->m_pProgramName);
		CFStr CGFilename = CFStrF("System/PS3/%s.fp", _pExtAttr->m_pProgramName);

		if(!CDiskUtil::FileExists(CacheFilename))
		{
			CStr FP;
			CFStr OriginalShader = CFStrF("System/GL/ARB_Fragment_Program/%s.fp", _pExtAttr->m_pProgramName);
			FP.ReadFromFile(OriginalShader);
			FPPreParser_Inplace(FP.GetStr(), aPreparseDefines);
			CStr CG = ConvertFPToCG(FP, _pExtAttr->m_pProgramName);
			{
				TArray<uint8> lData;
				lData.SetLen(CG.Len());
				memcpy(lData.GetBasePtr(), CG.Str(), lData.Len());
				CDiskUtil::WriteFileFromArray(CGFilename, CFILE_WRITE, lData);
			}
			M_TRACEALWAYS(CFStrF("Please compile the CG fragment program '%s'...\n", CGFilename.Str()));
			while(!CDiskUtil::FileExists(CacheFilename))
			{
				MRTC_SystemInfo::OS_Sleep(1000);
			}
		}
		FP_Load(CacheFilename, Hash);
		pProg = m_FPProgramTree.FindEqual(Hash);
		pProg->m_Name = _pExtAttr->m_pProgramName;
*/
		{
			CStr CG;
			if(CDiskUtil::FileExists(CFStrF("System/PS3/%s.fp", _pExtAttr->m_pProgramName)))
			{
				// PS3 overloaded version, use that instead
				CG.ReadFromFile(CFStrF("System/PS3/%s.fp", _pExtAttr->m_pProgramName));
			}
			else
			{
				CStr FP;
				CFStr OriginalShader = CFStrF("System/GL/ARB_Fragment_Program/%s.fp", _pExtAttr->m_pProgramName);
				FP.ReadFromFile(OriginalShader);
				FPPreParser_Inplace(FP.GetStr(), aPreparseDefines);
				CG = ConvertFPToCG(FP, _pExtAttr->m_pProgramName);
			}
			FP_Load(CG.GetStr(), Hash);
			pProg = m_FPProgramTree.FindEqual(Hash);
			pProg->m_Name = _pExtAttr->m_pProgramName;
		}

		M_ASSERT(pProg, "!");
	}

	if(pProg != m_pCurrentFPProgram)
	{
		cgGLBindProgram(pProg->m_Program);
		GLErr("FP_Bind");

		cgGLEnableProfile(CG_PROFILE_SCE_FP_RSX);
		GLErr("FP_Bind");

		m_pCurrentFPProgram = pProg;
		m_nStateFP++;
	}
	if(_pExtAttr->m_nParams > 0)
	{
		int ConstSize = -1;
		if(pProg->m_ConstParam)
		{
			ConstSize = cgGetArraySize(pProg->m_ConstParam, 0);
			int nParam = Min(_pExtAttr->m_nParams, ConstSize);
			cgGLSetParameterArray4f(pProg->m_ConstParam, 0, nParam, _pExtAttr->m_pParams->k);
		}

		if(ConstSize != _pExtAttr->m_nParams && !pProg->m_bWrongNumberOfParameters)
		{
			LogFile(CStrF("Fragment shaders '%s' uses wrong number of constants %d vs. %d", pProg->m_Name.Str(), ConstSize, _pExtAttr->m_nParams));
			pProg->m_bWrongNumberOfParameters = true;
		}
	}
}

void CRenderContextPS3::FP_Disable()
{
	cgGLDisableProfile(CG_PROFILE_SCE_FP_RSX);
	GLErr("FP_Disable");

	cgGLUnbindProgram(CG_PROFILE_SCE_FP_RSX);
	GLErr("FP_Disable");

	m_pCurrentFPProgram = NULL;
	m_nStateFP++;
}

void CRenderContextPS3::FP_Init()
{
}

void CRenderContextPS3::FP_Load( const char* _pProgram, uint32 _Hash )
{
//	TArray<uint8> lData;
//	lData = CDiskUtil::ReadFileToArray(_pProgram, CFILE_READ);
//	CGprogram cgprog = cgCreateProgram(m_CGContext, CG_BINARY, (const char*)lData.GetBasePtr(), CG_PROFILE_SCE_FP_RSX, NULL, NULL);
//	GLErr("FP_Load (cgCreateProgram)");
	CGprogram cgprog = cgCreateProgram(m_CGContext, CG_SOURCE, _pProgram, CG_PROFILE_SCE_FP_RSX, NULL, NULL);
	GLErr("FP_Load (cgCreateProgram)");
	CGparameter constants = cgGetNamedParameter(cgprog, "c");
	GLErr("FP_Load (cgGetNamedParameter)");

	CCGFPProgram* pProg = DNew(CCGFPProgram) CCGFPProgram;
	pProg->m_Hash = _Hash;
	pProg->m_Program = cgprog;
	pProg->m_ConstParam = constants;
	m_FPProgramTree.f_Insert(pProg);

	m_bDirtyFPCache = 1;

	GLErr("FP_Load (Post)");
}

int CRenderContextPS3::FP_LoadFile(CStr _Name, int _ID)
{
	return 0;
}


struct FPCacheHeader
{
	uint32	m_Version;
	uint32	m_Entries;
	uint32	m_LargestProgram;
	uint32	m_Padding;
};

struct FPCacheEntry
{
	uint32	m_Size;
	uint32	m_Hash;
	// Followed by binary for program
};

static const char* pCacheFile = "System/PS3/Cache/FPCache.dat"; 
void CRenderContextPS3::FP_LoadCache()
{
	CCFile CacheFile;
	if(!CDiskUtil::FileExists(pCacheFile))
		return;

	CacheFile.Open(pCacheFile, CFILE_READ | CFILE_BINARY);	
	bool bDeleteCache = true;
	do {

		FPCacheHeader Header;
		CacheFile.Read(&Header, sizeof(Header));
		if(Header.m_Version != PS3_FPCACHE_VERSION)
		{
			LogFile("FPCache has invalid version");
		}
		TThinArray<uint8> lObject;
		lObject.SetLen(Header.m_LargestProgram);
		for(uint32 i = 0; i < Header.m_Entries; i++)
		{
			FPCacheEntry Entry;
			CacheFile.Read(&Entry, sizeof(Entry));
			CacheFile.Read(lObject.GetBasePtr(), Entry.m_Size);
			FP_LoadBinary(lObject.GetBasePtr(), Entry.m_Hash);
		}
		bDeleteCache = false;
		LogFile(CStrF("Loaded %d FP-cache entries", Header.m_Entries));
	} while(0);

	CacheFile.Close();
	if(bDeleteCache)
		CDiskUtil::DelFile(pCacheFile);
}

void CRenderContextPS3::FP_LoadBinary(const uint8* _pProgram, uint32 _Hash)
{
	CGprogram cgprog = cgCreateProgram(m_CGContext, CG_BINARY, (const char*)_pProgram, CG_PROFILE_SCE_FP_RSX, NULL, NULL);
	GLErr("FP_LoadBinary (cgCreateProgram)");
	CGparameter constants = cgGetNamedParameter(cgprog, "c");
	GLErr("FP_LoadBinary (cgGetNamedParameter)");

	CCGFPProgram* pProg = DNew(CCGFPProgram) CCGFPProgram;
	pProg->m_Hash = _Hash;
	pProg->m_Program = cgprog;
	pProg->m_ConstParam = constants;
	m_FPProgramTree.f_Insert(pProg);

	GLErr("FP_LoadBinary (Post)");
}

void CRenderContextPS3::FP_SaveCache()
{
	FPCacheHeader Header;
	Header.m_Padding	= 0;
	Header.m_Version	= PS3_FPCACHE_VERSION;
	
	uint32 nPrograms = 0;
	uint32 LargestProgram = 0;

	DIdsTreeAVLAligned_Iterator(CCGFPProgram, m_Link, const uint32, CCGFPProgram::CTreeCompare) Iterator = m_FPProgramTree;

	while(Iterator)
	{
		nPrograms++;

		++Iterator;
	}

	Header.m_LargestProgram	= LargestProgram;
	Header.m_Entries	= nPrograms;

	m_bDirtyFPCache = 0;
}
