
#include "PCH.h"
#include "MRenderPS3_Context.h"

#define	PS3_VPCACHE_VERSION	1

const char* aInputNames[] =
{
	"_vpos", "_vnrm", "_vc0", "_vt0", "_vt1", "_vt2", "_vt3", "_vt4", "_vt5", "_vt6", "_vt7",
	"_vmw", "_vmi", "_vmw2", "_vmi2", NULL
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
	"POSITION", "NORMAL", "COLOR0", "TEXCOORD0", "TEXCOORD1", "TEXCOORD2", "TEXCOORD3", "TEXCOORD4", "TEXCOORD5", "TEXCOORD6", "TEXCOORD7",
	"BLENDWEIGHT", "BLENDINDICES", "ATTR5", "ATTR4", NULL
};

const char* aOutputType[] =
{
	"POSITION", "COLOR0", "COLOR1", "TEXCOORD0", "TEXCOORD1", "TEXCOORD2", "TEXCOORD3", "TEXCOORD4", "TEXCOORD5", "TEXCOORD6", "TEXCOORD7",
	"FOG", "CLP0", "CLP1", "CLP2", "CLP3", "CLP4", "CLP5", NULL
};

enum
{
	CG_INPUT_POSITION = DBit(0),
	CG_INPUT_NORMAL = DBit(1),
	CG_INPUT_COLOR0 = DBit(2),
	CG_INPUT_COLOR1 = DBit(3),
	CG_INPUT_TEXCOORD0 = DBit(4),
	CG_INPUT_TEXCOORD1 = DBit(5),
	CG_INPUT_TEXCOORD2 = DBit(6),
	CG_INPUT_TEXCOORD3 = DBit(7),
	CG_INPUT_TEXCOORD4 = DBit(8),
	CG_INPUT_TEXCOORD5 = DBit(9),
	CG_INPUT_TEXCOORD6 = DBit(10),
	CG_INPUT_TEXCOORD7 = DBit(11),
	CG_INPUT_MATRIXINDICES = DBit(12),
	CG_INPUT_MATRIXWEIGHT = DBit(13),

	CG_OUTPUT_POSITION = DBit(0),
	CG_OUTPUT_COLOR0 = DBit(1),
	CG_OUTPUT_COLOR1 = DBit(2),
	CG_OUTPUT_TEXCOORD0 = DBit(3),
	CG_OUTPUT_TEXCOORD1 = DBit(4),
	CG_OUTPUT_TEXCOORD2 = DBit(5),
	CG_OUTPUT_TEXCOORD3 = DBit(6),
	CG_OUTPUT_TEXCOORD4 = DBit(7),
	CG_OUTPUT_TEXCOORD5 = DBit(8),
	CG_OUTPUT_TEXCOORD6 = DBit(9),
	CG_OUTPUT_TEXCOORD7 = DBit(10),
	CG_OUTPUT_FOG = DBit(11),
	CG_OUTPUT_CLIP0 = DBit(12),
	CG_OUTPUT_CLIP1 = DBit(13),
	CG_OUTPUT_CLIP2 = DBit(14),
	CG_OUTPUT_CLIP3 = DBit(15),
	CG_OUTPUT_CLIP4 = DBit(16),
	CG_OUTPUT_CLIP5 = DBit(17),
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

CStr GenerateHeaderFromMasks(uint32 _UsedInputs, uint32 _UsedOutputs)
{
	if(VPIncludeHeader.Len() == 0)
	{
		VPIncludeHeader.ReadFromFile("System/PS3/VPInclude_PS3.xrg");
	}
	CStr Header = VPIncludeHeader + "\nfloat4 c[256];\nvoid main(";
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

	Header += ")\n";

	return Header;
}

CStr GenerateTempsFromMask(uint32 _Mask, uint32 _Address)
{
	CStr Temps;
	for(int i = 0; i < 32; i++)
	{
		if(_Mask & (1 << i))
		{
			Temps += CStrF("\tfloat4 r%d;\n", i);
		}
	}
	for(int i = 0; i < 32; i++)
	{
		if(_Address & (1 << i))
		{
			Temps += CStrF("\tfloat4 a%d;\n", i);
		}
	}

	return Temps;
}

CStr ConvertVPToCG(CStr _VP)
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
				CFStr CGLine = "\t" + Dest + " = " + Opcode + "(" + Line + ")" + DestMask + ";\n";

				CG += CGLine;
			}
			iCurrent = iEOL + 1;
		}
		else
			Error_static("ConvertVPToCG", "!");
	}

	CStr CGHeader = GenerateHeaderFromMasks(UsedInputs, UsedOutputs);
	CStr CGTemps = GenerateTempsFromMask(UsedTemps, UsedAddress);


	return CGHeader + "{\n" + CGTemps + CG + "}\n";
}

void CRenderContextPS3::VP_Load(const char* _pProgram, const CRC_VPFormat::CProgramFormat& _Format)
{
//	TArray<uint8> lData;
//	lData = CDiskUtil::ReadFileToArray(_pProgram, CFILE_READ);
//	CGprogram cgprog = cgCreateProgram(m_CGContext, CG_BINARY, (const char*)lData.GetBasePtr(), CG_PROFILE_SCE_VP_RSX, NULL, NULL);
//	GLErr("VP_Load (cgCreateProgram)");
	CGprogram cgprog = cgCreateProgram(m_CGContext, CG_SOURCE, _pProgram, CG_PROFILE_SCE_VP_RSX, NULL, NULL);
	GLErr("VP_Load (cgCreateProgram)");
	CGparameter constants = cgGetNamedParameter(cgprog, "c");
	GLErr("VP_Load (cgGetNamedParameter)");

	CCGVPProgram* pProg = DNew(CCGVPProgram) CCGVPProgram;
	pProg->m_Format = _Format;
	pProg->m_Program = cgprog;
	pProg->m_Parameter = constants;
	m_ProgramTree.f_Insert(pProg);

	m_bDirtyVPCache = 1;

	GLErr("VP_Load (Post)");
}

void CRenderContextPS3::VP_Bind(const CRC_VPFormat& _Format)
{
#ifndef PLATFORM_PS3
	return;
#endif
	const CRC_VPFormat::CProgramFormat& Fmt = _Format.m_ProgramGenFormat;

	CCGVPProgram* pProg = m_ProgramTree.FindEqual(Fmt);
	if(!pProg)
	{
/*
		CFStr VPName;
		if( m_bLogVP ) VPName = CFStrF("VP%.6x.%.4x.%.8x%.8x.%.1x.%.1x",
									Fmt.m_nMWComp + (Fmt.m_TexMatrix << 3) + (Fmt.m_Lighting << 11) + (Fmt.m_nLights << 12) + (Fmt.m_FogDepth << 16),
									Fmt.m_LightTypes,
									(Fmt.GetTexGen(0)<<24) | (Fmt.GetTexGen(1)<<16) | (Fmt.GetTexGen(2)<<8) | (Fmt.GetTexGen(3)<<0),
									(Fmt.GetTexGen(4)<<24) | (Fmt.GetTexGen(5)<<16) | (Fmt.GetTexGen(6)<<8) | (Fmt.GetTexGen(7)<<0),
									Fmt.m_MPFlags,
									Fmt.m_bClipPlanes
									);

		CMTime T, T2, T3;
		TStart(T);

		CFStr CacheFilename = CFStrF("System/PS3/Cache/%s.cgv", VPName.Str());
		CFStr CGFilename = CFStrF("System/PS3/%s.vp", VPName.Str());
		if(!CDiskUtil::FileExists(CacheFilename))
		{
			TStart(T2);
			CStr VP = m_VP_ProgramGenerator.CreateVP(_Format);
			TStop(T2);
			TStart(T3);
			CStr CG = ConvertVPToCG(VP);
			TStop(T3);
			{
				TArray<uint8> lData;
				lData.SetLen(CG.Len());
				memcpy(lData.GetBasePtr(), CG.Str(), lData.Len());
				CDiskUtil::WriteFileFromArray(CGFilename, CFILE_WRITE, lData);
			}
			M_TRACEALWAYS(CFStrF("Please compile the CG vertex program '%s'...\n", CGFilename.Str()));
			while(!CDiskUtil::FileExists(CacheFilename))
			{
				MRTC_SystemInfo::OS_Sleep(1000);
			}
		}
		VP_Load(CacheFilename, Fmt);
		TStop(T);
*/
		CMTime T1, T2, T3;
		{
			TStart(T2);
			CStr VP = m_VP_ProgramGenerator.CreateVP(_Format);
			TStop(T2);
			TStart(T3);
			CStr CG = ConvertVPToCG(VP);
			TStop(T3);
			VP_Load(CG.GetStr(), Fmt);
		}

		pProg = m_ProgramTree.FindEqual(Fmt);
		M_ASSERT(pProg, "!");
	}

	if(pProg == m_pCurrentVPProgram)
		return;

	if(m_pCurrentVPProgram)
	{
		// Unbind any parameters
		Internal_VA_Disable();
	}

	cgGLBindProgram(pProg->m_Program);
	GLErr("VP_Bind");

	cgGLEnableProfile(CG_PROFILE_SCE_VP_RSX);
	GLErr("VP_Bind");

	m_pCurrentVPProgram = pProg;
	m_nStateVP++;
}

void CRenderContextPS3::VP_Disable()
{
	cgGLDisableProfile(CG_PROFILE_SCE_VP_RSX);
	GLErr("VP_Disable");

	cgGLUnbindProgram(CG_PROFILE_SCE_VP_RSX);
	GLErr("VP_Disable");

	m_pCurrentVPProgram = NULL;
	m_nStateVP++;
}

void CRenderContextPS3::VP_DeleteAllObjects()
{
	VP_Disable();

	CCGVPProgram* pProg;
	while((pProg = m_ProgramTree.GetRoot()))
	{
		m_ProgramTree.f_Remove(pProg);
		cgDestroyProgram(pProg->m_Program);
		GLErr("VP_DeleteAllObjects");
		delete pProg;
	}
}

void CRenderContextPS3::VP_Init()
{
}

void CRenderContextPS3::VP_Update()
{
	DebugNop();
	cgGLSetParameterArray4f(m_pCurrentVPProgram->m_Parameter, 0, m_nUpdateVPConst, (const float*)m_VPConstRegisters);
	GLErr("VP_Update");
	m_bUpdateVPConst = 0;
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

void CRenderContextPS3::VP_LoadCache()
{
	const char* pVPCacheFile = "System/PS3/Cache/VPCache.dat"; 
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
			LogFile("VPCache has invalid version");
		}
		TThinArray<uint8> lObject;
		lObject.SetLen(Header.m_LargestProgram);
		for(uint32 i = 0; i < Header.m_Entries; i++)
		{
			VPCacheEntry Entry;
			CacheFile.Read(&Entry, sizeof(Entry));
			CacheFile.Read(lObject.GetBasePtr(), Entry.m_Size);
			VP_LoadBinary(lObject.GetBasePtr(), Entry.m_Fmt);
		}
		bDeleteCache = false;
		LogFile(CStrF("Loaded %d VP-cache entries", Header.m_Entries));
	} while(0);

	CacheFile.Close();
	if(bDeleteCache)
		CDiskUtil::DelFile(pVPCacheFile);
}

void CRenderContextPS3::VP_LoadBinary(const uint8* _pProgram, const CRC_VPFormat::CProgramFormat& _Format)
{
	CGprogram cgprog = cgCreateProgram(m_CGContext, CG_BINARY, (const char*)_pProgram, CG_PROFILE_SCE_VP_RSX, NULL, NULL);
	GLErr("VP_LoadBinary (cgCreateProgram)");
	CGparameter constants = cgGetNamedParameter(cgprog, "c");
	GLErr("VP_LoadBinary (cgGetNamedParameter)");

	CCGVPProgram* pProg = DNew(CCGVPProgram) CCGVPProgram;
	pProg->m_Format = _Format;
	pProg->m_Program = cgprog;
	pProg->m_Parameter = constants;
	m_ProgramTree.f_Insert(pProg);

	GLErr("VP_LoadBinary (Post)");
}


void CRenderContextPS3::VP_SaveCache()
{
	VPCacheHeader Header;
	Header.m_Version	= PS3_VPCACHE_VERSION;
	Header.m_SizeOfFormat	= sizeof(CRC_VPFormat::CProgramFormat);
	
	uint32 nPrograms = 0;
	uint32 LargestProgram = 0;

	DIdsTreeAVLAligned_Iterator(CCGVPProgram, m_Link, const CRC_VPFormat::CProgramFormat, CCGVPProgram::CTreeCompare) Iterator = m_ProgramTree;

	while(Iterator)
	{
		nPrograms++;

		++Iterator;
	}

	Header.m_LargestProgram	= LargestProgram;
	Header.m_Entries	= nPrograms;

	m_bDirtyVPCache = 0;
}
