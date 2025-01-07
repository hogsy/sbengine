
#include "PCH.h"
#include "MRenderPS3_Context.h"
#include "MRenderPS3_FragmentProgram.h"
#ifdef PLATFORM_PS3
  #include "cg/cgc.h"
#endif

#define	PS3_FPCACHE_VERSION	1

namespace NRenderPS3GCM
{

void DumpSource(const char* _pSource)
{
	char aBuf[256];
	while(*_pSource)
	{
		int l = 0;
		while(_pSource[l] && l < 255)
		{
			aBuf[l] = _pSource[l];
			l++;
		}
		_pSource += l;
		aBuf[l++] = 0;
		M_TRACEALWAYS("%s", aBuf);
	}
}



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

static int Translate_Int(const char* _pVal, const char** _pStrings)
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

static void RenameSamplers(CFStr& _Line, const char** _ppOriginal, const char** _ppSub)
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

static CStr GenerateHeader(uint32 _SamplerMask, uint8* _pSamplerType, uint32 _MaxUsedConstant, CFStr* _lpAttribNames, CFStr* _lpOutNames)
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

//	Header += CStrF("void main(out float4 %s : COLOR", _OutName.GetStr());
	Header += CStrF("void main(");
	for(uint i = 0; i < 4; i++)
	{
		if(_lpOutNames[i].Len() > 0)
		{
			Header += CStrF("%s out float4 %s : COLOR%d", (i!=0)?",":"", _lpOutNames[i].GetStr(), i);
		}
	}

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

static CStr GenerateTemps(TArray<CFStr> _lTemps)
{
	CStr Temps;
	for(int i = 0; i < _lTemps.Len(); i++)
	{
		Temps += CStrF("\tfloat4 %s;\n", _lTemps[i].GetStr());
	}

	return Temps;
}

static CStr GenerateParams(TArray<CFStr> _lNames, TArray<CFStr> _lValues)
{
	CStr Temps;
	for(int i = 0; i < _lNames.Len(); i++)
	{
		Temps += CStrF("\tfloat4 %s = %s;\n", _lNames[i].GetStr(), _lValues[i].GetStr());
	}

	return Temps;
}

static int GetNextNonWhite(const char* _pFP, int _iCurrentPos, int _nLen)
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

static bool FindChar(const char _c, const char* _pSep)
{
	while(*_pSep)
	{
		if(*_pSep == _c)
			return true;
		_pSep++;
	}

	return false;
}

static int GetNextSep(const char* _pFP, int _iCurrentPos, int _nLen, const char* _pSep)
{
	while(_iCurrentPos < _nLen)
	{
		if(FindChar(_pFP[_iCurrentPos], _pSep))
			break;
		_iCurrentPos++;
	}

	return _iCurrentPos;
}

static int SkipToNextLine(const char* _pFP, int _iCurrentPos, int _nLen)
{
	while(_iCurrentPos < _nLen)
	{
		if(_pFP[_iCurrentPos++] == '\n')
			break;
	}

	return _iCurrentPos;
}

static int GetNextSymbol(const char* _pFP, int _iCurrentPos, int _nLen, int& _iSymbolStart, int& _iSymbolEnd)
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

static CStr ConvertFPToCG(CStr _FP, const char* _pShaderName)
{
	CStr CG;
	CFStr OutputVars[4];
	int iCurrentPos = 0;
	int nLen = _FP.Len();
	TArray<CFStr> lTemps;
	TArray<CFStr> lConstantName;
	TArray<CFStr> lConstantValue;
	CFStr lAttribNames[9];
	uint8 SamplerTypes[16] = {0, 0, 0, 0, 0, 0, 0, 0};
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
					int iNameStart, iNameStop;
					iCurrentPos = GetNextSymbol(pFP, iCurrentPos, nLen, iNameStart, iNameStop);
					if(iNameStart == -1)
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
					CFStr Name;
					Name.Capture(pFP + iNameStart, iNameStop - iNameStart);

					if(!CStrBase::strnicmp(pFP + iVarStart, "result.color[", 13))
					{
						uint iMRT = pFP[iVarStart + 13] - '0';
						if(iMRT >= CRC_MAXMRT)
						{
							LogFile(CStrF("Syntax error in fragment program '%s'", _pShaderName));
							break;
						}
						Name.MakeLowerCase();
						OutputVars[iMRT] = Name;
						OutputVars[iMRT].MakeLowerCase();
					}
					else
					{
						if(CStrBase::strnicmp(pFP + iVarStart, "result.color", 12))
						{
							LogFile(CStrF("Syntax error in fragment program '%s'", _pShaderName));
							break;
						}
						// Not MRT
						OutputVars[0] = Name;
						OutputVars[0].MakeLowerCase();
					}
//					OutputVar.Capture(pFP + iTempStart, iTempStop - iTempStart);
//					OutputVar.MakeLowerCase();
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
						uint iAttrib = pFP[iTexStart + 18] - '0';
						if(iAttrib >= 8)
						{
							LogFile(CStrF("Syntax error in fragment program '%s'", _pShaderName));
							break;
						}
						Name.MakeLowerCase();
						lAttribNames[iAttrib] = Name;
					}
				}
				else if(!CStrBase::strnicmp(pFP + iSymStart, "TEMP", 4))
				{
					do 
					{
						int iVarStart, iVarStop;
						iCurrentPos = GetNextSymbol(pFP, iCurrentPos, nLen, iVarStart, iVarStop);
						CFStr Temp;
						Temp.Capture(pFP + iVarStart, iVarStop - iVarStart);
						Temp.MakeLowerCase();
						lTemps.Add(Temp);

						iCurrentPos = GetNextNonWhite(pFP, iCurrentPos, nLen);
						if(pFP[iCurrentPos] != ',')
							break;
						iCurrentPos++;
					} while(1);
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

	CStr CGHeader = GenerateHeader(UsedSamplers, SamplerTypes, MaxUsedConstant, lAttribNames, OutputVars);
	CStr CGParams = GenerateParams(lConstantName, lConstantValue);
	CStr CGTemps = GenerateTemps(lTemps);

	return CGHeader + CGParams + CGTemps + CG + "}\n";
}

void CContext_FragmentProgram::Bind( CRC_ExtAttributes_FragmentProgram20* _pExtAttr )
{
#ifdef PLATFORM_PS3
	M_LOCK(m_Lock);
	int Hash = _pExtAttr->m_ProgramNameHash;
	if(Hash == 0) Hash = StringToHash(_pExtAttr->m_pProgramName);
	CRenderPS3Resource_FragmentProgram_Local* pProg = m_ProgramTree.FindEqual(Hash);
	if(!pProg)
	{
		CFStr CacheFilename = CFStrF("System/PS3/Cache/%s.cgf", _pExtAttr->m_pProgramName);
		if(!CDiskUtil::FileExists(CacheFilename))
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

			{
				char* bin;
				static const char* Options[] = {"-strict", NULL};
				int ret = compile_program_from_string(CG.Str(), "sce_fp_rsx", NULL, Options, &bin);

				if(0);
				{
					TArray<uint8> lData;
					lData.SetLen(CG.Len());
					memcpy(lData.GetBasePtr(), CG.Str(), lData.Len());
					CDiskUtil::WriteFileFromArray(CFStrF("System/PS3/%s/%s.fp", bin?"Cache":"Broken", _pExtAttr->m_pProgramName), CFILE_WRITE, lData);
				}

				if(bin)
				{
					LoadBinary((uint8*)bin, cellGcmCgGetTotalBinarySize((CGprogram)bin), Hash);

					if(0);
					{
						TArray<uint8> lData;
						lData.SetLen(cellGcmCgGetTotalBinarySize((CGprogram)bin));
						memcpy(lData.GetBasePtr(), bin, lData.Len());
						CDiskUtil::WriteFileFromArray(CFStrF("System/PS3/Cache/%s.fpb", _pExtAttr->m_pProgramName), CFILE_WRITE, lData);
					}
					free_compiled_program(bin);
				}
				else
				{
					M_TRACEALWAYS("Failed to compile fragment program '%s'\r\n", _pExtAttr->m_pProgramName);
					DumpSource(CG.Str());
					M_BREAKPOINT;
				}
			}
		}
		else
			LoadBinaryFromFile(CacheFilename, Hash);
		m_bDirtyCache = 1;
		pProg = m_ProgramTree.FindEqual(Hash);
		pProg->m_Name = _pExtAttr->m_pProgramName;

		M_ASSERT(pProg, "!");
	}

	pProg->Touch();
	if(pProg != m_pCurrentProgram)
	{
		gcmSetFragmentProgram(pProg->m_Program, pProg->GetOffset());

		m_pCurrentProgram = pProg;
		CRCPS3GCM::ms_This.m_ContextStatistics.m_nStateFP++;
	}
#ifndef GCM_USEFPPT
	if(_pExtAttr->m_nParams > 0)
	{
		if(pProg->m_nParamCount)
		{
			uint nParams = _pExtAttr->m_nParams;
			if(nParams > pProg->m_nParamCount)
			{
				if(pProg->m_bInvalidNumberOfParams == false)
				{
					LogFile(CStrF("Fragment program passed invalid number of constants!! (passed %d, shader has %d", _pExtAttr->m_nParams, pProg->m_nParamCount));
				}
				nParams = pProg->m_nParamCount;
				pProg->m_bInvalidNumberOfParams = true;
			}

			CRCPS3GCM::ms_This.m_ContextStatistics.m_nStateFPParam++;
			CRCPS3GCM::ms_This.m_ContextStatistics.m_nStateFPParamCount += nParams;
			for(uint i = 0; i < nParams; i++)
			{
				CGparameter param = cellGcmCgGetNamedParameter(pProg->m_Program, CFStrF("c[%d]", i).Str());
				if(param)
				{
//					if ( gCellGcmCurrentContext->current + 128 > gCellGcmCurrentContext->end )
//					   gCellGcmCurrentContext->callback( gCellGcmCurrentContext, 128 );
					gcmSetFragmentProgramParameter(pProg->m_Program, param, _pExtAttr->m_pParams[i].k, pProg->GetOffset());
				}
			}

			gcmSetUpdateFragmentProgramParameter(pProg->GetOffset());
		}
	}
#endif
#endif
}

void CContext_FragmentProgram::Disable()
{
	// Should mount a dummy program here?

	m_pCurrentProgram = NULL;
	CRCPS3GCM::ms_This.m_ContextStatistics.m_nStateFP++;
}

uint CContext_FragmentProgram::LoadBinaryFromFile(const char* _pProgram, uint32 _Hash)
{
	TArray<uint8> lData;
	lData = CDiskUtil::ReadFileToArray(_pProgram, CFILE_READ);
	return LoadBinary(lData.GetBasePtr(), lData.Len(), _Hash);
}

uint CContext_FragmentProgram::LoadBinary(const uint8* _pData, uint32 _Size, uint32 _Hash)
{
	void* pFP = M_ALLOC(_Size);
	memcpy(pFP, _pData, _Size);

	CRenderPS3Resource_FragmentProgram_Local* pProg = DNew(CRenderPS3Resource_FragmentProgram_Local) CRenderPS3Resource_FragmentProgram_Local;
	pProg->Create(pFP, _Size);
	pProg->m_Hash = _Hash;
	while(cellGcmCgGetNamedParameter(pProg->m_Program, CFStrF("c[%d]", pProg->m_nParamCount).Str()))
		pProg->m_nParamCount++;
	m_ProgramTree.f_Insert(pProg);

	return pProg->m_Program != 0;
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

static const char* pFPCacheFile = "System/PS3/Cache/FPCache.dat"; 
void CContext_FragmentProgram::LoadCache()
{
	M_LOCK(m_Lock);
	CCFile CacheFile;
	if(!CDiskUtil::FileExists(pFPCacheFile))
		return;

	if(0)
	{
		M_BREAKPOINT;

		CRC_ExtAttributes_FragmentProgram20 ExtAttr;
		ExtAttr.Clear();
		ExtAttr.m_pProgramName = "WClientMod_OW1_1";

		Bind(&ExtAttr);
	}

	CacheFile.Open(pFPCacheFile, CFILE_READ | CFILE_BINARY);	
	bool bDeleteCache = true;
	do {

		FPCacheHeader Header;
		CacheFile.Read(&Header, sizeof(Header));
		if(Header.m_Version != PS3_FPCACHE_VERSION)
		{
			LogFile("FPCache: Invalid version");
			break;
		}
		TThinArray<uint8> lObject;
		lObject.SetLen(Header.m_LargestProgram);
		for(uint32 i = 0; i < Header.m_Entries; i++)
		{
			FPCacheEntry Entry;
			CacheFile.Read(&Entry, sizeof(Entry));
			CacheFile.Read(lObject.GetBasePtr(), Entry.m_Size);
			LoadBinary(lObject.GetBasePtr(), lObject.Len(), Entry.m_Hash);
		}
		bDeleteCache = false;
		LogFile(CStrF("Loaded %d FP-cache entries", Header.m_Entries));
	} while(0);

	CacheFile.Close();
	if(bDeleteCache)
	{
		LogFile("FPCache: Cleaning");
		CDiskUtil::DelFile(pFPCacheFile);

		CDirectoryNode Dir;
		Dir.ReadDirectory("System/PS3/Cache/*.fp");
		for(uint i = 0; i < Dir.GetFileCount(); i++)
			CDiskUtil::DelFile(CStr("System/PS3/Cache/") + Dir.GetFileName(i));

		Dir.ReadDirectory("System/PS3/Cache/*.cgf");
		for(uint i = 0; i < Dir.GetFileCount(); i++)
			CDiskUtil::DelFile(CStr("System/PS3/Cache/") + Dir.GetFileName(i));
	}
}

void CContext_FragmentProgram::SaveCache()
{
	M_LOCK(m_Lock);
	FPCacheHeader Header;
	Header.m_Padding	= 0;
	Header.m_Version	= PS3_FPCACHE_VERSION;
	
	uint32 nPrograms = 0;
	uint32 LargestProgram = 0;

	{
		DIdsTreeAVLAligned_Iterator(CRenderPS3Resource_FragmentProgram_Local, m_Link, const uint32, CRenderPS3Resource_FragmentProgram_Local::CTreeCompare) Iterator = m_ProgramTree;

		while(Iterator)
		{
			nPrograms++;
			LargestProgram = Max(LargestProgram, Iterator->m_MemSize);

			++Iterator;
		}
	}

	Header.m_LargestProgram	= LargestProgram;
	Header.m_Entries	= nPrograms;

	{
		CCFile CacheFile;
		CacheFile.Open(pFPCacheFile, CFILE_WRITE | CFILE_BINARY);	
		CacheFile.Write(&Header, sizeof(Header));
		DIdsTreeAVLAligned_Iterator(CRenderPS3Resource_FragmentProgram_Local, m_Link, const uint32, CRenderPS3Resource_FragmentProgram_Local::CTreeCompare) Iterator = m_ProgramTree;

		while(Iterator)
		{
			FPCacheEntry Entry;
			Entry.m_Size = Iterator->m_MemSize;
			Entry.m_Hash = Iterator->m_Hash;
			CacheFile.Write(&Entry, sizeof(Entry));
			CacheFile.Write((const void*&)Iterator->m_Program, Iterator->m_MemSize);
			++Iterator;
		}
		CacheFile.Close();
	}

	m_bDirtyCache = 0;
}

void CContext_FragmentProgram::PrepareFrame()
{
	m_pCurrentProgram = NULL;
}

void CContext_FragmentProgram::DeleteAllObjects()
{
	M_LOCK(m_Lock);
	Disable();

	CRenderPS3Resource_FragmentProgram_Local* pProg;
	while((pProg = m_ProgramTree.GetRoot()))
	{
		m_ProgramTree.f_Remove(pProg);
		//cgDestroyProgram(pProg->m_Program);
		delete pProg;
	}
}

void CContext_FragmentProgram::Create()
{
}

void CContext_FragmentProgram::PostDefrag()
{
	M_LOCK(m_Lock);
}

};	// namespace NRenderPS3
