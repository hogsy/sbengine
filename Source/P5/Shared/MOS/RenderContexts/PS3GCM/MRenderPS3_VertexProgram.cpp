
#include "PCH.h"
#include "MRenderPS3_Context.h"
#include "MRenderPS3_VertexProgram.h"
#ifdef PLATFORM_PS3
  #include "cg/cgc.h"
#endif

#define	PS3_VPCACHE_VERSION	2


namespace NRenderPS3GCM
{

void DumpSource(const char* _pSource);

const char* aInputNames[] =
{
	"_vpos", "_vnrm", "_vc0", "_vc1", "_vt0", "_vt1", "_vt2", "_vt3", "_vt4", "_vt5", "_vt6", "_vt7",
	"_vmw", "_vmw2", "_vmi", "_vmi2", NULL
};

const char* aOutputNames[] =
{
	"_opos", "_oc0", "_oc1", "_ot0", "_ot1", "_ot2", "_ot3", "_ot4", "_ot5", "_ot6", "_ot7",
	"_ofog", "_oclip0", "_oclip1", "_oclip2", "_oclip3", "_oclip4", "_oclip5", NULL
};

const char* aTempNames[] =
{
	"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", NULL
};

const char* aAddressNames[] =
{
	"a0", NULL
};

const char* aInputType[] =
{
//	"POSITION", "NORMAL", "COLOR0", "TEXCOORD0", "TEXCOORD1", "TEXCOORD2", "TEXCOORD3", "TEXCOORD4", "TEXCOORD5", "TEXCOORD6", "TEXCOORD7",
//	"BLENDWEIGHT", "BLENDINDICES", "ATTR5", "ATTR4", NULL
	"ATTR0", "ATTR1", "ATTR2", "ATTR3", "ATTR8", "ATTR9", "ATTR10", "ATTR11", "ATTR12", "ATTR13", "ATTR14", "ATTR15",
	"ATTR4", "ATTR5", "ATTR6", "ATTR7", NULL
};

const char* aOutputType[] =
{
	"POSITION", "COLOR0", "COLOR1", "TEXCOORD0", "TEXCOORD1", "TEXCOORD2", "TEXCOORD3", "TEXCOORD4", "TEXCOORD5", "TEXCOORD6", "TEXCOORD7",
	"FOG", "CLP0", "CLP1", "CLP2", "CLP3", "CLP4", "CLP5", NULL
};

enum
{
	CG_INPUT_POSITION = M_Bit(0),
	CG_INPUT_NORMAL = M_Bit(1),
	CG_INPUT_COLOR0 = M_Bit(2),
	CG_INPUT_COLOR1 = M_Bit(3),
	CG_INPUT_TEXCOORD0 = M_Bit(4),
	CG_INPUT_TEXCOORD1 = M_Bit(5),
	CG_INPUT_TEXCOORD2 = M_Bit(6),
	CG_INPUT_TEXCOORD3 = M_Bit(7),
	CG_INPUT_TEXCOORD4 = M_Bit(8),
	CG_INPUT_TEXCOORD5 = M_Bit(9),
	CG_INPUT_TEXCOORD6 = M_Bit(10),
	CG_INPUT_TEXCOORD7 = M_Bit(11),
	CG_INPUT_MATRIXWEIGHT = M_Bit(12),
	CG_INPUT_MATRIXWEIGHT2 = M_Bit(13),
	CG_INPUT_MATRIXINDICES = M_Bit(14),
	CG_INPUT_MATRIXINDICES2 = M_Bit(15),

	CG_OUTPUT_POSITION = M_Bit(0),
	CG_OUTPUT_COLOR0 = M_Bit(1),
	CG_OUTPUT_COLOR1 = M_Bit(2),
	CG_OUTPUT_TEXCOORD0 = M_Bit(3),
	CG_OUTPUT_TEXCOORD1 = M_Bit(4),
	CG_OUTPUT_TEXCOORD2 = M_Bit(5),
	CG_OUTPUT_TEXCOORD3 = M_Bit(6),
	CG_OUTPUT_TEXCOORD4 = M_Bit(7),
	CG_OUTPUT_TEXCOORD5 = M_Bit(8),
	CG_OUTPUT_TEXCOORD6 = M_Bit(9),
	CG_OUTPUT_TEXCOORD7 = M_Bit(10),
	CG_OUTPUT_FOG = M_Bit(11),
	CG_OUTPUT_CLIP0 = M_Bit(12),
	CG_OUTPUT_CLIP1 = M_Bit(13),
	CG_OUTPUT_CLIP2 = M_Bit(14),
	CG_OUTPUT_CLIP3 = M_Bit(15),
	CG_OUTPUT_CLIP4 = M_Bit(16),
	CG_OUTPUT_CLIP5 = M_Bit(17),
};

// Valid expression for this is r0.xyz,r11.w, r1;
uint32 ScanForStrings(CFStr _String, const char** _ppStrings)
{
	uint32 UsedMask = 0;
	while(_String.Len() > 0)
	{
		CFStr Part = _String.GetStrMSep(",;");
		_String.Trim();
		if(Part.Find(".") >= 0)
			Part = Part.GetStrSep(".");
		Part.Trim();

		for(int i = 0; _ppStrings[i]; i++)
			if(Part == _ppStrings[i])
				UsedMask |= (1 << i);
	}

	return UsedMask;
}

CStr VPIncludeHeader;

static CStr GenerateHeaderFromMasks(uint32 _UsedInputs, uint32 _UsedOutputs)
{
	if(VPIncludeHeader.Len() == 0)
	{
		VPIncludeHeader.ReadFromFile("System/PS3/VPInclude_PS3.xrg");
	}
	CStr Header = VPIncludeHeader + "\r\nfloat4 c[256] : register(c0);\r\nvoid main(";
//	CStr Header = "#include \"VPInclude_PS3.xrg\"\nfloat4 c[256];\nvoid main(";

	bool bFirst = true;
	for(int i = 0; aInputNames[i]; i++)
	{
		if(_UsedInputs & (1 << i))
		{
			Header += CStrF("%sin float4 %s : %s", bFirst?"":", ", aInputNames[i], aInputType[i]);
			bFirst = false;
		}
	}
	for(int i = 0; aOutputNames[i]; i++)
	{
		if(_UsedOutputs & (1 << i))
		{
			int nSize = 4;

			// Anything equal to or above fog is a float1
			if((1 << i) >= CG_OUTPUT_FOG)
				nSize = 1;
			Header += CStrF(", out float%d %s : %s", nSize, aOutputNames[i], aOutputType[i]);
		}
	}

	Header += ")\r\n";

	return Header;
}

static CStr GenerateTempsFromMask(uint32 _Mask, uint32 _Address)
{
	CStr Temps;
	for(int i = 0; i < 32; i++)
	{
		if(_Mask & (1 << i))
		{
			Temps += CStrF("\tfloat4 r%d;\r\n", i);
		}
	}
	for(int i = 0; i < 32; i++)
	{
		if(_Address & (1 << i))
		{
			Temps += CStrF("\tfloat4 a%d;\r\n", i);
		}
	}

	return Temps;
}

static CStr ConvertVPToCG(CStr _VP)
{
	uint32 UsedInputs = 0;
	uint32 UsedOutputs = 0;
	uint32 UsedTemps = 0;
	uint32 UsedAddress = 0;
	CStr CG;

	int iLast = _VP.Len();
	int iCurrent = 0;
	while(iCurrent < iLast)
	{
		if(CStrBase::IsWhiteSpace(_VP.Str()[iCurrent]))
		{
			iCurrent++;
			continue;
		}
		int iEOL = _VP.FindFrom(iCurrent, ";");
		if(iEOL >= 0)
		{
			CFStr Line;
			Line.Capture(_VP.Str() + iCurrent, iEOL - iCurrent);
			Line.Trim();
			if(Line[0] != '#')
			{
				// Exclude comments
				CFStr Opcode = Line.GetStrMSep(" \t");
				Opcode.MakeUpperCase();
				Line.MakeLowerCase();

				UsedInputs |= ScanForStrings(Line, aInputNames);
				UsedOutputs |= ScanForStrings(Line, aOutputNames);
				UsedTemps |= ScanForStrings(Line, aTempNames);
				UsedAddress |= ScanForStrings(Line, aAddressNames);

				CFStr Dest = Line.GetStrSep(",");
				Dest.Trim();
				CFStr DestMask;
				int iDestMask = Dest.FindReverse(".");
				if(iDestMask >= 0)
				{
					DestMask.Capture(Dest.Str() + iDestMask, Dest.Len() - iDestMask);
				}
				else if((1 << CStrBase::TranslateInt(Dest.GetStr(), aOutputNames)) >= CG_OUTPUT_FOG)
				{
					// float dest so we force a mask
					DestMask = ".x";
				}

				Line.Trim();
				CFStr CGLine = "\t" + Dest + " = " + Opcode + "(" + Line + ")" + DestMask + ";\r\n";

				CG += CGLine;
			}
			iCurrent = iEOL + 1;
		}
		else
			Error_static("ConvertVPToCG", "!");
	}

	CStr CGHeader = GenerateHeaderFromMasks(UsedInputs, UsedOutputs);
	CStr CGTemps = GenerateTempsFromMask(UsedTemps, UsedAddress);


	return CGHeader + "{\r\n" + CGTemps + CG + "}\r\n";
}

uint CContext_VertexProgram::LoadBinaryFromFile(const char* _pProgram, const CRC_VPFormat::CProgramFormat& _Format)
{
	TArray<uint8> lData;
	lData = CDiskUtil::ReadFileToArray(_pProgram, CFILE_READ);
	return LoadBinary(lData.GetBasePtr(), lData.Len(), _Format);
}

uint CContext_VertexProgram::LoadBinary(const uint8* _pProgram, uint32 _Size, const CRC_VPFormat::CProgramFormat& _Format)
{
	void* pVP = M_ALLOC(_Size);
	memcpy(pVP, _pProgram, _Size);

	CRenderPS3Resource_VertexProgram_Main* pProg = DNew(CRenderPS3Resource_VertexProgram_Main) CRenderPS3Resource_VertexProgram_Main;
	pProg->Create(pVP, _Size);
	pProg->m_Format = _Format;
	m_ProgramTree.f_Insert(pProg);

	return pProg->m_Program != 0;
}

void CContext_VertexProgram::Bind(const CRC_VPFormat& _Format)
{
#ifdef PLATFORM_PS3
	M_LOCK(m_Lock);
	const CRC_VPFormat::CProgramFormat& Fmt = _Format.m_ProgramGenFormat;

	CRenderPS3Resource_VertexProgram_Main* pProg = m_ProgramTree.FindEqual(Fmt);
	if(!pProg)
	{
		CMTime T1, T2, T3;
		CFStr VPName = Fmt.GetString();
		CFStr CacheFilename = CFStrF("System/PS3/Cache/%s.cgv", VPName.Str());
		if(!CDiskUtil::FileExists(CacheFilename))
		{
			TStart(T2);
			CStr VP = m_ProgramGenerator.CreateVP(_Format);
			TStop(T2);
			TStart(T3);
			CStr CG = ConvertVPToCG(VP);
			TStop(T3);


			{
				char* bin;
				static const char* Options[] = {"-strict", NULL};
				int ret = compile_program_from_string(CG.Str(), "sce_vp_rsx", NULL, Options, &bin);
				if(0);
				{
					TArray<uint8> lData;
					lData.SetLen(CG.Len());
					memcpy(lData.GetBasePtr(), CG.Str(), lData.Len());
					CDiskUtil::WriteFileFromArray(CFStrF("System/PS3/%s/%s.vp", bin?"Cache":"Broken", VPName.Str()), CFILE_WRITE, lData);
				}
				if(bin)
				{
					LoadBinary((uint8*)bin, cellGcmCgGetTotalBinarySize((CGprogram)bin), Fmt);

					if(0)
					{
						TArray<uint8> lData;
						lData.SetLen(cellGcmCgGetTotalBinarySize((CGprogram)bin));
						memcpy(lData.GetBasePtr(), bin, lData.Len());
						CDiskUtil::WriteFileFromArray(CFStrF("System/PS3/Cache/%s.vpb", VPName.Str()), CFILE_WRITE, lData);
					}
					free_compiled_program(bin);
				}
				else
				{
					M_TRACEALWAYS("Failed to compile vertex program '%s'\r\n", VPName.Str());
					DumpSource(CG.Str());
					M_BREAKPOINT;
				}
			}
		}
		else
			LoadBinaryFromFile(CacheFilename, Fmt);
		m_bDirtyCache = 1;

		pProg = m_ProgramTree.FindEqual(Fmt);
		M_ASSERT(pProg, "!");
	}

	if(pProg == m_pCurrentProgram)
		return;

	pProg->Touch();
	gcmSetVertexProgram(pProg->m_Program, pProg->m_pUCode);

	m_pCurrentProgram = pProg;
	CRCPS3GCM::ms_This.m_ContextStatistics.m_nStateVP++;
#endif
}

void CContext_VertexProgram::Disable()
{
	// Should mount a dummy program here?
	m_pCurrentProgram = NULL;
	CRCPS3GCM::ms_This.m_ContextStatistics.m_nStateVP++;
}

void CContext_VertexProgram::DeleteAllObjects()
{
	M_LOCK(m_Lock);
	Disable();

	CRenderPS3Resource_VertexProgram_Main* pProg;
	while((pProg = m_ProgramTree.GetRoot()))
	{
		m_ProgramTree.f_Remove(pProg);
		//cgDestroyProgram(pProg->m_Program);
		delete pProg;
	}
}

struct VPCacheHeader
{
	uint32	m_Version;
	uint32	m_SizeOfFormat;
	uint32	m_Entries;
	uint32	m_LargestProgram;
};

struct VPCacheEntry
{
	uint32	m_Size;
	CRC_VPFormat::CProgramFormat	m_Fmt;
	// Followed by binary for program
};

static const char* pVPCacheFile = "System/PS3/Cache/VPCache.dat"; 
void CContext_VertexProgram::LoadCache()
{
	M_LOCK(m_Lock);
	CCFile CacheFile;
	if(!CDiskUtil::FileExists(pVPCacheFile))
		return;

	CacheFile.Open(pVPCacheFile, CFILE_READ | CFILE_BINARY);	
	bool bDeleteCache = true;
	do {

		VPCacheHeader Header;
		CacheFile.Read(&Header, sizeof(Header));
		if(Header.m_Version != PS3_VPCACHE_VERSION || Header.m_SizeOfFormat != sizeof(CRC_VPFormat::CProgramFormat))
		{
			LogFile("VPCache: Invalid version");
			break;
		}
		TThinArray<uint8> lObject;
		lObject.SetLen(Header.m_LargestProgram);
		for(uint32 i = 0; i < Header.m_Entries; i++)
		{
			VPCacheEntry Entry;
			CacheFile.Read(&Entry, sizeof(Entry));
			CacheFile.Read(lObject.GetBasePtr(), Entry.m_Size);
			LoadBinary(lObject.GetBasePtr(), lObject.Len(), Entry.m_Fmt);
		}
		bDeleteCache = false;
		LogFile(CStrF("Loaded %d VP-cache entries", Header.m_Entries));
	} while(0);

	CacheFile.Close();
	if(bDeleteCache)
	{
		LogFile("VPCache: Cleaning");

		CDiskUtil::DelFile(pVPCacheFile);
		// Delete all pre-generated programs aswell since they're probably wrong
		CDirectoryNode Dir;

		Dir.ReadDirectory("System/PS3/Cache/*.vp");
		for(uint i = 0; i < Dir.GetFileCount(); i++)
			CDiskUtil::DelFile(CStr("System/PS3/Cache/") + Dir.GetFileName(i));

		Dir.ReadDirectory("System/PS3/Cache/*.cgv");
		for(uint i = 0; i < Dir.GetFileCount(); i++)
			CDiskUtil::DelFile(CStr("System/PS3/Cache/") + Dir.GetFileName(i));
	}
}


void CContext_VertexProgram::SaveCache()
{
	M_LOCK(m_Lock);
	VPCacheHeader Header;
	Header.m_Version	= PS3_VPCACHE_VERSION;
	Header.m_SizeOfFormat	= sizeof(CRC_VPFormat::CProgramFormat);
	
	uint32 nPrograms = 0;
	uint32 LargestProgram = 0;

	{
		DIdsTreeAVLAligned_Iterator(CRenderPS3Resource_VertexProgram_Main, m_Link, const CRC_VPFormat::CProgramFormat, CRenderPS3Resource_VertexProgram_Main::CTreeCompare) Iterator = m_ProgramTree;

		while(Iterator)
		{
			LargestProgram = Max(LargestProgram, Iterator->m_MemSize);
			nPrograms++;

			++Iterator;
		}
	}

	Header.m_LargestProgram	= LargestProgram;
	Header.m_Entries	= nPrograms;

	{
		CCFile CacheFile;
		CacheFile.Open(pVPCacheFile, CFILE_WRITE | CFILE_BINARY);	
		CacheFile.Write(&Header, sizeof(Header));
		DIdsTreeAVLAligned_Iterator(CRenderPS3Resource_VertexProgram_Main, m_Link, const CRC_VPFormat::CProgramFormat, CRenderPS3Resource_VertexProgram_Main::CTreeCompare) Iterator = m_ProgramTree;

		while(Iterator)
		{
			VPCacheEntry Entry;
			Entry.m_Size = Iterator->m_MemSize;
			Entry.m_Fmt = Iterator->m_Format;
			CacheFile.Write(&Entry, sizeof(Entry));
			CacheFile.Write((const void*&)Iterator->m_Program, Iterator->m_MemSize);
			++Iterator;
		}
		CacheFile.Close();
	}

	m_bDirtyCache = 0;
}

void CContext_VertexProgram::SetConstants(uint _iStart, uint _nCount, const CVec4Dfp32* _pData)
{
#ifndef PLATFORM_PS3
	return;
#endif
//	if ( gCellGcmCurrentContext->current + (_nCount * sizeof(CVec4Dfp32)) > gCellGcmCurrentContext->end )
//	   gCellGcmCurrentContext->callback( gCellGcmCurrentContext, _nCount * sizeof(CVec4Dfp32) );
	gcmSetVertexProgramConstants(_iStart, _nCount * 4, (fp32*)_pData);
}

void CContext_VertexProgram::Create()
{
	m_ProgramGenerator.Create("System/GL/VP.xrg", "System/GL/VPDefines_PS3.xrg", false, CRC_MAXTEXCOORDS);
}

void CContext_VertexProgram::PrepareFrame()
{
	m_pCurrentProgram = NULL;
}

void CContext_VertexProgram::PostDefrag()
{
	M_LOCK(m_Lock);
}

};	// namespace NRenderPS3
