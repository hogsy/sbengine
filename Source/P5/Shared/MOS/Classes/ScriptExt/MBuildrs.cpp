#include "PCH.h"

#include "MBuildrs.h"

#ifdef XWC
#include "../../XWC/XWC_Win32.h"
#include <windows.h>
#endif

//#pragma optimize("", off)
//#pragma inline_depth(0)

enum
{
	e_Platform_PC   = 0,
	e_Platform_Xbox = 1,
	e_Platform_PS2  = 2,
	e_Platform_GC   = 3,
	e_Platform_Xenon   = 4,
	e_Platform_PS3	= 5,
};

static const char* lPlatformNames[6] =
{
	"",
	"_XBox",
	"_PS2",
	"_GC",
	"_Xenon",
	"_PS3"
};

// -------------------------------------------------------------------
//  CDataFileBuilder
// -------------------------------------------------------------------

TMRTC_ThreadLocal<CDataFileBuilder *> CDataFileBuilder::m_pThis;

CDataFileBuilder::CDataFileBuilder(CStr _ScriptPath)
{
	m_ScriptPath = _ScriptPath;
	m_bIsAdded = false;
}

CDataFileBuilder::CDataFileBuilder()
{
	m_bIsAdded = true;
	AddToConsole();
}

CDataFileBuilder::~CDataFileBuilder()
{
	if (m_bIsAdded)
		RemoveFromConsole();
}

CDataFile* CDataFileBuilder::GetDataFile(int _hDFile)
{
	CDataFileBuilder *pThis = *m_pThis;
	if (!pThis->m_lspDataFiles[_hDFile]) Error_static("GetDataFile", "File not created.");
	return pThis->m_lspDataFiles[_hDFile];
}

CStr CDataFileBuilder::GetCurrentScriptPath()
{
	CDataFileBuilder *pThis = *m_pThis;
	return pThis->m_ScriptPath;
}

int CDataFileBuilder::DFile_Create(CStr _Filename)
{
	CDataFileBuilder *pThis = *m_pThis;
	spCDataFile spDF = MNew(CDataFile);
	if (!spDF) ThrowError("DFile_Create");
	int hFile = pThis->m_lspDataFiles.Add(spDF);

	if (_Filename.GetDevice() == "") _Filename = GetCurrentScriptPath() + _Filename;
	pThis->m_lspDataFiles[hFile]->Create(_Filename, 2);
	return hFile;
}

void CDataFileBuilder::DFile_Close(int _hDFile)
{
	CDataFileBuilder *pThis = *m_pThis;
	if (!pThis->m_lspDataFiles[_hDFile]) Error_static("DFile_Close", "File not created.");

	pThis->m_lspDataFiles[_hDFile]->Close();
	pThis->m_lspDataFiles[_hDFile] = NULL;
}

void CDataFileBuilder::DFile_BeginEntry(int _hDFile, CStr _Name)
{
	CDataFileBuilder *pThis = *m_pThis;
	if (!pThis->m_lspDataFiles[_hDFile]) Error_static("DFile_Close", "File not created.");
	pThis->m_lspDataFiles[_hDFile]->BeginEntry(_Name);
}

void CDataFileBuilder::DFile_EndEntry(int _hDFile, int _UserData)
{
	CDataFileBuilder *pThis = *m_pThis;
	if (!pThis->m_lspDataFiles[_hDFile]) Error_static("DFile_Close", "File not created.");
	pThis->m_lspDataFiles[_hDFile]->EndEntry(_UserData);
}

void CDataFileBuilder::DFile_BeginSubDir(int _hDFile)
{
	CDataFileBuilder *pThis = *m_pThis;
	if (!pThis->m_lspDataFiles[_hDFile]) Error_static("DFile_Close", "File not created.");
	pThis->m_lspDataFiles[_hDFile]->BeginSubDir();
}

void CDataFileBuilder::DFile_EndSubDir(int _hDFile)
{
	CDataFileBuilder *pThis = *m_pThis;
	if (!pThis->m_lspDataFiles[_hDFile]) Error_static("DFile_Close", "File not created.");
	pThis->m_lspDataFiles[_hDFile]->EndSubDir();
}

void CDataFileBuilder::Log(CStr _Str)
{
	LogFile(_Str);
}

void CDataFileBuilder::Register(CScriptRegisterContext & _RegContext)
{
	_RegContext.RegFunction("DFile_Create", &CDataFileBuilder::DFile_Create);
	_RegContext.RegFunction("DFile_Close", &CDataFileBuilder::DFile_Close);
	_RegContext.RegFunction("DFile_BeginEntry", &CDataFileBuilder::DFile_BeginEntry);
	_RegContext.RegFunction("DFile_EndEntry", &CDataFileBuilder::DFile_EndEntry);
	_RegContext.RegFunction("DFile_BeginSubDir", &CDataFileBuilder::DFile_BeginSubDir);
	_RegContext.RegFunction("DFile_EndSubDir", &CDataFileBuilder::DFile_EndSubDir);
//	_RegContext.RegFunction("Log", Log);
}

// -------------------------------------------------------------------
//  CBuilder_XTC
// -------------------------------------------------------------------
TMRTC_ThreadLocal<CBuilder_XTC *> CBuilder_XTC::m_pThis;

CBuilder_XTC::CBuilder_XTC(CDataFileBuilder* _pDFB)
{
	m_pDFB = _pDFB;
//	g_pOS->m_spCon->AddSubSystem(this);
	m_bIsAdded = false;
}

CBuilder_XTC::CBuilder_XTC()
{
	m_bIsAdded = true;
	AddToConsole();
}

CBuilder_XTC::~CBuilder_XTC()
{
	if (m_bIsAdded)
		RemoveFromConsole();
//	g_pOS->m_spCon->RemoveSubSystem(this);
}

void CBuilder_XTC::XTC_Begin()
{
	CBuilder_XTC *pThis = *m_pThis;
	if (pThis->m_spTC) Error_static("XTC_Begin", "Begin already performed.");

	pThis->m_spTC = MNew(CTextureContainer_Plain);
	if (!pThis->m_spTC) ThrowError("XTC_Begin");
}

void CBuilder_XTC::XTC_End()
{
	CBuilder_XTC *pThis = *m_pThis;
	if (!pThis->m_spTC) Error_static("XTC_End", "No XTC created.");
	pThis->m_spTC = NULL;
}

void CBuilder_XTC::XTC_Finalize(int _Flags)
{
	CBuilder_XTC *pThis = *m_pThis;
	if (!pThis->m_spTC) Error_static("XTC_Finalize", "No XTC created.");
}

void CBuilder_XTC::XTC_AddTextureFlags(CStr _Name, CStr _Filename, int _Flags)
{
	CBuilder_XTC *pThis = *m_pThis;
	if (!pThis->m_spTC) Error_static("XTC_AddTexture", "No XTC created.");

	if (_Filename.GetDevice() == "") _Filename = pThis->m_pDFB->GetCurrentScriptPath() + _Filename;

	CTC_TextureProperties Properties;
	pThis->m_spTC->AddTexture(_Filename, Properties, _Name.UpperCase(), _Flags);

/*	char Snuff[2] = { 34, 0 };
	char* pSnuff = &Snuff[0];
	LogFile("	{");
	LogFile(CStrF("		%sNAME%s		%s%s%s", pSnuff, pSnuff, pSnuff, (char*)_Name, pSnuff));
	LogFile(CStrF("		%sTEXTURE%s	%s%s%s", pSnuff, pSnuff, pSnuff, (char*)_Name, pSnuff));
	LogFile("	}");*/
}

void CBuilder_XTC::XTC_AddTexture(CStr _Name, CStr _Filename)
{
	XTC_AddTextureFlags(_Name, _Filename, 0);
}

void CBuilder_XTC::XTC_WriteImageList(int _hDFile, int _Flags)
{
	CBuilder_XTC *pThis = *m_pThis;
	if (!pThis->m_spTC) Error_static("XTC_WriteImageList", "No XTC created.");
	pThis->m_spTC->WriteImageList(pThis->m_pDFB->GetDataFile(_hDFile));
}

void CBuilder_XTC::XTC_WriteWAD(CStr _Filename)
{
	CBuilder_XTC *pThis = *m_pThis;
	if (!pThis->m_spTC) Error_static("XTC_WriteWAD", "No XTC created.");
	if (_Filename.GetDevice() == "") _Filename = pThis->m_pDFB->GetCurrentScriptPath() + _Filename;
	pThis->m_spTC->WriteWAD(_Filename);
}

void CBuilder_XTC::XTC_Quantize(int _iLocal)
{
	CBuilder_XTC *pThis = *m_pThis;
	if (!pThis->m_spTC) Error_static("XTC_Quantize", "No XTC created.");
	if (_iLocal >= 0)
	{
		CImage TxtDesc;
		int nMipmaps;
		pThis->m_spTC->GetTextureDesc(_iLocal, &TxtDesc, nMipmaps);
		pThis->m_spTC->Quantize(TxtDesc.GetPalette());
	}
	else
		pThis->m_spTC->Quantize();
}

int CBuilder_XTC::XTC_GetNumTextures()
{
	CBuilder_XTC *pThis = *m_pThis;
	if (!pThis->m_spTC) Error_static("XTC_GetNumTextures", "No XTC created.");
	return pThis->m_spTC->GetNumTextures();
}

void CBuilder_XTC::Register(CScriptRegisterContext & _RegContext)
{
	_RegContext.RegFunction("XTC_Begin", &CBuilder_XTC::XTC_Begin);
	_RegContext.RegFunction("XTC_End", &CBuilder_XTC::XTC_End);
	_RegContext.RegFunction("XTC_Finalize", &CBuilder_XTC::XTC_Finalize);
	_RegContext.RegFunction("XTC_AddTexture", &CBuilder_XTC::XTC_AddTexture);
	_RegContext.RegFunction("XTC_AddTextureFlags", &CBuilder_XTC::XTC_AddTextureFlags);
	_RegContext.RegFunction("XTC_WriteImageList", &CBuilder_XTC::XTC_WriteImageList);
	_RegContext.RegFunction("XTC_WriteWAD", &CBuilder_XTC::XTC_WriteWAD);
	_RegContext.RegFunction("XTC_Quantize", &CBuilder_XTC::XTC_Quantize);
	_RegContext.RegFunction("XTC_GetNumTextures", &CBuilder_XTC::XTC_GetNumTextures);
}


// -------------------------------------------------------------------
//  CBuilder_XWC
// -------------------------------------------------------------------

TMRTC_ThreadLocal<CBuilder_XWC *> CBuilder_XWC::m_pThis;

CBuilder_XWC::CBuilder_XWC(CDataFileBuilder* _pDFB) : m_bUseDestWaves(false)
{
	m_pDFB = _pDFB;
	m_bIsAdded = true;
//	g_pOS->m_spCon->AddSubSystem(this);
}
CBuilder_XWC::CBuilder_XWC() : m_bUseDestWaves(false)
{
	m_bIsAdded = true;
	AddToConsole();
}

CBuilder_XWC::CBuilder_XWC(CDataFileBuilder* _pDFB, bool _bUseDestWaves)  : m_bUseDestWaves(_bUseDestWaves) {
	m_pDFB = _pDFB;
	m_bIsAdded = true;
	//	g_pOS->m_spCon->AddSubSystem(this);
}

CBuilder_XWC::~CBuilder_XWC()
{
	if (m_bIsAdded)
		RemoveFromConsole();
//	g_pOS->m_spCon->RemoveSubSystem(this);
}

void CBuilder_XWC::XWC_Begin(CStr _FileName)
{
	CBuilder_XWC *pThis = *m_pThis;
	{
		bool bError = false;
		for(int i = 0; i < 16; i++)
			if (pThis->m_lspWC[i])
			{
				bError = true;
				break;
			}

		if(bError)
			Error_static("XWC_Begin", "Begin already performed.");
	}

	pThis->m_lspWC[e_Platform_PC] = MNew(CWaveContainer_Plain);
	pThis->m_lspWC[e_Platform_PS3] = MNew(CWaveContainer_Plain);
	pThis->m_lspWC[e_Platform_Xenon] = MNew(CWaveContainer_Plain);
	if (!pThis->m_lspWC[e_Platform_PC]) ThrowError("XWC_Begin");
	if (!pThis->m_lspWC[e_Platform_PS3]) ThrowError("XWC_Begin");
	if (!pThis->m_lspWC[e_Platform_Xenon]) ThrowError("XWC_Begin");

	for (int p=0 ; p< 6; p++)
		pThis->m_lspDestWC[p] = NULL;

	{
		CStr BasePath;
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		CStr ExePath = pSys->m_ExePath;

		char *pFile;
		GetFullPathName(ExePath, MAX_PATH, BasePath.GetBuffer(MAX_PATH), &pFile);
		pThis->m_DestPath = BasePath;

		pThis->m_DestFileName = _FileName;

		// Build all destination file paths
		// If bUseDestWaves is true, load each destination file if it exists.

		CStr Logging = "UseDestWaves = ";
		Logging += pThis->m_bUseDestWaves ? "true" : "false";

		pThis->m_pDFB->Log(Logging);

		CFileInfo destWCFileInfo;
		for(int i = 0; i < 6; i++)
		{
			if(pThis->m_lspWC[i])
			{
				pThis->m_lDestFilePath[i] = pThis->m_DestPath + CStrF("\\Content\\Waves%s\\%s", lPlatformNames[i], pThis->m_DestFileName.GetStr());

				if (pThis->m_bUseDestWaves) {
					if (CDiskUtil::FileExists(pThis->m_lDestFilePath[i]))
					{
						pThis->m_lspDestWC[i] = MNew(CWaveContainer_Plain);
						if (pThis->m_lspDestWC[i]) 
						{
							pThis->m_lspDestWC[i]->AddXWC(pThis->m_lDestFilePath[i], false);

							CStr Logging = "Loaded dest sound XWC: ";
							Logging += pThis->m_lDestFilePath[i];
							pThis->m_pDFB->Log(Logging);

							destWCFileInfo = CDiskUtil::FileTimeGet(pThis->m_lDestFilePath[i]);
							pThis->m_lDestFileTimes[i] = destWCFileInfo.m_TimeWrite;
						}
						else
						{
							CStr Logging = "Failed to load dest sound XWC: ";
							Logging += pThis->m_lDestFilePath[i];
							pThis->m_pDFB->Log(Logging);
						}
					}
					else
					{
						CStr Logging = "Dest sound XWC didn't exist: ";
						Logging += pThis->m_lDestFilePath[i];
						pThis->m_pDFB->Log(Logging);
					}
				}
			}
		}

	}
//	pThis->m_lSFXDesc.Clear();
//	pThis->m_lSFXDesc.SetGrow(128);
}

void CBuilder_XWC::XWC_End()
{

	CBuilder_XWC *pThis = *m_pThis;
	{
		bool bError = true;
		for(int i = 0; i < 16; i++)
			if (pThis->m_lspWC[i])
			{
				bError = false;
				break;
			}

		if(bError)
			Error_static("XWC_End", "Begin not performed.");
	}

	for (int p=0 ; p< 6; p++)
		pThis->m_lspDestWC[p] = NULL;

	for(int i = 0; i < 6; i++)
	{
		if(pThis->m_lspWC[i])
		{
			CDiskUtil::CreatePath(pThis->m_DestPath + CStrF("\\Content\\Waves%s", lPlatformNames[i]));
			pThis->m_lspWC[i]->WriteXWC(pThis->m_DestPath + CStrF("\\Content\\Waves%s\\%s", lPlatformNames[i], pThis->m_DestFileName.GetStr()));
		}
		pThis->m_lspWC[i] = NULL;
	}

//	pThis->m_lSFXDesc.Clear();
}

/*
void CBuilder_XWC::XWC_Finalize(int _Flags)
{
	if (!pThis->m_spWC) Error_static("XWC_Finalize", "No XWC created.");
}
*/

CStr GetCodec(int _Platform)
{
	switch (_Platform)
	{
		case e_Platform_PC: // PC
			return "VORB";
		case e_Platform_Xbox: // XBOX
//				if (_Flags)
//					return "VORB";
//				else
				return "XACM";
		case e_Platform_PS2: // PS2
			return "SPU2";
		case e_Platform_GC: // GC
			return "Cube";
		case e_Platform_Xenon: // Xenon
			return "XMA";
		case e_Platform_PS3: // PS3
			return "PS3";

	}

	return "";
}

//
//
//
void CBuilder_XWC::XWC_AddWave4(CStr _Name, CStr _Filename, int32 _Flags, fp32 _Quality)
{
	CBuilder_XWC *pThis = *m_pThis;
	static int32 Count = 0;
	{
		bool bError = true;
		for(int i = 0; i < 16; i++)
			if (pThis->m_lspWC[i])
			{
				bError = false;
				break;
			}

		if(bError)
			Error_static("XWC_AddWave", "Begin not performed.");
	}

	// DEBUG: Was after new block
	if (_Filename.GetDevice() == "") 
	{
		MACRO_GetSystem;
		_Filename = pSys->GetEnvironment()->GetValue("XWC_WaveSourcePath", pThis->m_pDFB->GetCurrentScriptPath()) + _Filename;
	}
	// END DEBUG


	// Only valid if b_UseDestWaves set.
	// If true a wave for the specific platform has been pulled from the destination file,
	// If false, the wave needs to be compressed anew.
	bool lbUsedDestWave[6] = { false, false, false, false, false, false };

	// See if we can just take the wave data from an existing source.
	if (pThis->m_bUseDestWaves)
	{
		CStr Logging = CStrF("AddWave: \"%s\", Checking for existing data: ", (const char *)_Name);
		pThis->m_pDFB->Log(Logging);

//		CFileInfo sourceFileInfo;	// DEBUG COMMENT
//		int64 sourceFileTime;		// Write time. DEBUG

		for(int i = 0; i < 6; i++)
		{
			if(pThis->m_lspWC[i] && pThis->m_lspDestWC[i])
			{
				spCWaveContainer_Plain &destWC = pThis->m_lspDestWC[i];
				spCWaveContainer_Plain &newWC = pThis->m_lspWC[i];

				int16 existingWaveLocalID = destWC->GetLocalWaveID(_Name);

				if (existingWaveLocalID != -1) {
					Logging = CStrF("--------Pre-existing data for platform %i.", i);
					pThis->m_pDFB->Log(Logging);
/*
					if (CDiskUtil::FileExists(_Filename)) 
					{
						sourceFileInfo = CDiskUtil::FileTimeGet(_Filename);
						sourceFileTime = sourceFileInfo.m_TimeWrite;
						if (sourceFileInfo.m_TimeWrite > pThis->m_lDestFileTimes[i]) 
						{
							Logging = CStrF("------------But source is new, so will recompress it.", i);
							pThis->m_pDFB->Log(Logging);

							continue;	// Source file is newer than dest, so recompress it.
						}
					}
*/
					CWaveform *spExistingWaveform = destWC->GetWaveform(existingWaveLocalID);
					if (spExistingWaveform) 
					{
						// Need to duplicate the waveform.
						spCWaveform spExistingWaveformCopy = MNew(CWaveform);
						spExistingWaveform->Duplicate(spExistingWaveformCopy);

						for (int iCA = 0; iCA < 16; iCA++)
						{
							spExistingWaveformCopy->m_SpeakerAssignment[iCA] = spExistingWaveform->m_SpeakerAssignment[iCA];
							spExistingWaveformCopy->m_ChannelAssignments[iCA] = spExistingWaveform->m_ChannelAssignments[iCA];
						}
						spExistingWaveformCopy->m_LoopEnd = spExistingWaveform->m_LoopEnd;
						spExistingWaveformCopy->m_LoopStart = spExistingWaveform->m_LoopStart;
						spExistingWaveformCopy->m_Rate = spExistingWaveform->m_Rate;

						// DEBUG: Compress wave source anyway and compare.
						/*
						int iProperLocalID = pThis->m_lspWC[i]->AddWaveTable(_Filename, _Name, GetCodec(i), 1.0, _Quality, (_Flags&2)/2*20.0f, _Flags&1);

						if (i == e_Platform_PC) 
						{
							CWaveform *pProperWaveform = pThis->m_lspWC[i]->GetWaveform(iProperLocalID);
							
							int ProperW = pProperWaveform->GetWidth();
							int ProperH = pProperWaveform->GetHeight();

							spCWaveform spExistingDC = MNew(CWaveform);
							spCWaveform spProperDC = MNew(CWaveform);

							spExistingWaveformCopy->Decompress(spExistingDC);
							pProperWaveform->Decompress(spProperDC);

							bool bDifferent = false;

							if (pProperWaveform->GetFormat() != spExistingWaveformCopy->GetFormat())
							{
								Logging = CStrF("------------Cached and Proper formats differ! %s", (const char *)_Filename);
								pThis->m_pDFB->Log(Logging);
								bDifferent = true;
							}
							else if (ProperW != spExistingWaveformCopy->GetWidth() || ProperH != spExistingWaveformCopy->GetHeight())
							{
								Logging = CStrF("------------Cached and Proper image sizes differ! %s", (const char *)_Filename);
								pThis->m_pDFB->Log(Logging);
								bDifferent = true;
							}
							else
							{
								CClipRect ClipRect(0, 0, ProperW, ProperH);
								CPnt Pnt;
								for (int y=0;y<ProperH && !bDifferent;y++)
								{
									for (int x=0;x<ProperW && !bDifferent;x++)
									{
										Pnt.x = x;
										Pnt.y = y;
										uint32 ProperPix = spProperDC->GetRAWPixel(ClipRect, Pnt);
										uint32 ExistingPix = spExistingDC->GetRAWPixel(ClipRect, Pnt);

										if (ProperPix != ExistingPix)
										{
											Logging = CStrF("------------Cached and Proper pixel data is different! %s", (const char *)_Filename);
											pThis->m_pDFB->Log(Logging);
											bDifferent = true;
										}
									}

								}

							}

							// Need to check for lipsync data
							int DestLipDataSize = 0;
							int NewLipDataSize = 0;
							bool DestHasLipData, NewHasLipData;
							fp32 NewTimeCode, DestTimeCode;
							void *pDestRawLipData = 0;
							void *pDestLipData = 0;
							void *pNewRawLipData = 0;
							void *pNewLipData = 0;

							DestHasLipData = destWC->GetLSDSize(existingWaveLocalID, DestLipDataSize);
							NewHasLipData = newWC->GetLipDataSet(iProperLocalID, NewTimeCode, *(int32*)&NewLipDataSize, &pNewLipData);
							//NewHasLipData = newWC->GetLSDSize(iProperLocalID, DestLipDataSize);

							if (DestHasLipData != NewHasLipData) 
							{
								if (DestHasLipData)
									Logging = CStrF("------------Cached and Proper - Cached has lipsync data, Proper does not. %s", (const char *)_Filename);
								else
									Logging = CStrF("------------Cached and Proper - Proper has lipsync data, Cached does not. %s", (const char *)_Filename);

								pThis->m_pDFB->Log(Logging);
								bDifferent = true;
							}
							else if (DestHasLipData && NewHasLipData)
							{
								if ((DestLipDataSize - sizeof(fp32)) != NewLipDataSize) //(DestLipDataSize != NewLipDataSize)
								{
									Logging = CStrF("------------Cached (sz: %i) and Proper (sz: %i) lipsync data differs by size! %s", DestLipDataSize, NewLipDataSize, (const char *)_Filename);
									pThis->m_pDFB->Log(Logging);
									bDifferent = true;									
								}
								else 
								{
									pDestRawLipData = DNew(uint8) uint8[DestLipDataSize];
									destWC->GetLSD(existingWaveLocalID, pDestRawLipData, DestLipDataSize);

									DestTimeCode = *(fp32*)pDestRawLipData;
									pDestLipData = ((fp32*)pDestRawLipData)+1;

									if (DestTimeCode != NewTimeCode) 
									{
										Logging = CStrF("------------Cached and Proper lipsync timecodes are different! %s", (const char *)_Filename);
										pThis->m_pDFB->Log(Logging);
										bDifferent = true;									
									}
									else 
									{
										if (memcmp(pNewLipData, pDestLipData, DestLipDataSize))
										{
											Logging = CStrF("------------Cached and Proper lipsync data is different! %s", (const char *)_Filename);
											pThis->m_pDFB->Log(Logging);
											bDifferent = true;									
										}
									}

									delete []pDestRawLipData;
								}
							}

							if (bDifferent == false)
							{
								Logging = CStrF("------------Waves appear to be identical.", (const char *)_Filename);
								pThis->m_pDFB->Log(Logging);							
							}
						}


						lbUsedDestWave[i] = true;

						// END DEBUG
*/
						int iLocalWaveID = newWC->AddWaveTable(spExistingWaveformCopy, _Name);
						lbUsedDestWave[i] = true;

						// Need to check for lipsync data
						int LipDataSize = 0;
						if (destWC->GetLSDSize(existingWaveLocalID, LipDataSize))
						{
							void *pRawLipData = 0;
							void *pLipData = 0;

							pRawLipData = DNew(uint8) uint8[LipDataSize];
							destWC->GetLSD(existingWaveLocalID, pRawLipData, LipDataSize);

							fp32 TimeCode = *(fp32*)pRawLipData;

							pLipData = DNew(uint8) uint8[LipDataSize - sizeof(fp32)];
							memcpy(pLipData, ((char *)pRawLipData) + sizeof(fp32), LipDataSize - sizeof(fp32));

							delete [] pRawLipData;

							newWC->AddLipDataSet(iLocalWaveID, 
												 TimeCode,
												 LipDataSize - sizeof(fp32),
												 pLipData);
						}

						pThis->m_pDFB->Log("------------Used existing data.");

					}
					else
					{
						pThis->m_pDFB->Log("------------Could not access data.");
						lbUsedDestWave[i] = false;
					}
				}
				else 
				{
					Logging = CStrF("--------No pre-existing data for platform %i.", i);
					pThis->m_pDFB->Log(Logging);
					lbUsedDestWave[i] = false;
				}
			}
			else 
			{
				Logging = CStrF("----No pre-existing data for platform %i", i);
				pThis->m_pDFB->Log(Logging);
				lbUsedDestWave[i] = false;
			}

		}		
	}

	CStr Logging = ("Adding wave '%s'", (char*)_Name);
	if (_Flags & 1)
		Logging += " Streamed";
	if (_Flags & 2)
		Logging += " LipSynced";

	Logging += " from " + _Filename;

	pThis->m_pDFB->Log(Logging);
	for(int i = 0; i < 6; i++)
	{
		if(pThis->m_lspWC[i] && ( !pThis->m_bUseDestWaves || (lbUsedDestWave[i] == false)))
		{
			pThis->m_lspWC[i]->AddWaveTable(_Filename, _Name, GetCodec(i), 1.0, _Quality, (_Flags&2)/2*20.0f, _Flags&1);
		}
	}
}

//
void CBuilder_XWC::XWC_AddWaveStreamLipSync(CStr _Name, CStr _Filename)
{ XWC_AddWave4(_Name, _Filename, 3, 1.0); }

void CBuilder_XWC::XWC_AddWaveLipSync(CStr _Name, CStr _Filename)
{ XWC_AddWave4(_Name, _Filename, 2, 1.0); }

void CBuilder_XWC::XWC_AddWaveStream(CStr _Name, CStr _Filename)
{ XWC_AddWave4(_Name, _Filename, 1, 1.0); }

void CBuilder_XWC::XWC_AddWave(CStr _Name, CStr _Filename)
{ XWC_AddWave4(_Name, _Filename, 0, 1.0); }

void CBuilder_XWC::XWC_AddWaveStreamLipSyncQuality(CStr _Name, CStr _Filename, fp32 _Quality)
{ XWC_AddWave4(_Name, _Filename, 3, _Quality); }

void CBuilder_XWC::XWC_AddWaveLipSyncQuality(CStr _Name, CStr _Filename, fp32 _Quality)
{ XWC_AddWave4(_Name, _Filename, 2, _Quality); }

void CBuilder_XWC::XWC_AddWaveStreamQuality(CStr _Name, CStr _Filename, fp32 _Quality)
{ XWC_AddWave4(_Name, _Filename, 1, _Quality); }

void CBuilder_XWC::XWC_AddWaveQuality(CStr _Name, CStr _Filename, fp32 _Quality)
{ XWC_AddWave4(_Name, _Filename, 0, _Quality); }

void GetFileList_r(CStr _Path, TArray<CStr> &_lFiles, bool _bRecursive)
{
	_Path.Trim();

	if (_Path == "")
		return;


	// Read files, include wildcard
	{
		CDirectoryNode Dir;
		Dir.ReadDirectory(_Path);

		int nf = Dir.GetFileCount();
		for(int i = 0; i < nf; i++)
		{
			CDir_FileRec* pF = Dir.GetFileRec(i);
			if (Dir.IsDirectory(i))
			{
			}
			else
			{
				// LogFile("Adding file " + _Path.GetPath() + pF->pThis->m_Name);
				_lFiles.Add(_Path.GetPath() + pF->m_Name);
			}
		}
	}

	// Read files, remove wildcard
	{
		CDirectoryNode Dir;
		Dir.ReadDirectory(_Path.GetPath() + "*.*");

		int nf = Dir.GetFileCount();
		for(int i = 0; i < nf; i++)
		{
			CDir_FileRec* pF = Dir.GetFileRec(i);
			if (Dir.IsDirectory(i))
			{
				if (pF->m_Name == "." || pF->m_Name == "..") continue;
				if (_bRecursive)
					GetFileList_r(_Path.GetPath() + Dir.GetFileName(i) + "\\" + _Path.GetFilename(), _lFiles, _bRecursive);
			}
			else
			{
			}
		}
	}
}

void CBuilder_XWC::XWC_AddWaveStreamLipSyncDirQuality(CStr _Directory, int _bRecursive, fp32 _Quality)
{
	CBuilder_XWC *pThis = *m_pThis;
	TArray<CStr> Files;

	CStr Dir = _Directory;
	if (Dir.GetDevice() == "")
	{
		MACRO_GetSystem;
		Dir = pSys->GetEnvironment()->GetValue("XWC_WaveSourcePath", pThis->m_pDFB->GetCurrentScriptPath().GetPath()) + Dir;
	}


	// Find all files
	GetFileList_r(Dir, Files, _bRecursive != 0);

	for (int i = 0; i < Files.Len(); ++i)
	{
		CStr Ext = Files[i].GetFilenameExtenstion().LowerCase();
		if (Ext == "wav" || Ext == "xrs")
		{
			XWC_AddWave4(Files[i].GetFilenameNoExt(), Files[i], 3, _Quality);
		}
	}	
}

void CBuilder_XWC::XWC_AddWaveLipSyncDirQuality(CStr _Directory, int _bRecursive, fp32 _Quality)
{
	CBuilder_XWC *pThis = *m_pThis;
	TArray<CStr> Files;

	CStr Dir = _Directory;
	if (Dir.GetDevice() == "")
	{
		MACRO_GetSystem;
		Dir = pSys->GetEnvironment()->GetValue("XWC_WaveSourcePath", pThis->m_pDFB->GetCurrentScriptPath().GetPath()) + Dir;
	}

	// Find all files
	GetFileList_r(Dir, Files, _bRecursive != 0);

	for (int i = 0; i < Files.Len(); ++i)
	{
		CStr Ext = Files[i].GetFilenameExtenstion().LowerCase();
		if (Ext == "wav" || Ext == "xrs")
		{
			XWC_AddWave4(Files[i].GetFilenameNoExt(), Files[i], 2, _Quality);
		}
	}	
}

void CBuilder_XWC::XWC_AddWaveDirQuality(CStr _Directory, int _bRecursive, fp32 _Quality)
{
	CBuilder_XWC *pThis = *m_pThis;
	TArray<CStr> Files;

	CStr Dir = _Directory;
	if (Dir.GetDevice() == "")
	{
		MACRO_GetSystem;
		Dir = pSys->GetEnvironment()->GetValue("XWC_WaveSourcePath", pThis->m_pDFB->GetCurrentScriptPath().GetPath()) + Dir;
	}

	// Find all files
	GetFileList_r(Dir, Files, _bRecursive != 0);

	for (int i = 0; i < Files.Len(); ++i)
	{
		CStr Ext = Files[i].GetFilenameExtenstion().LowerCase();
		if (Ext == "wav" || Ext == "xrs")
		{
			XWC_AddWave4(Files[i].GetFilenameNoExt(), Files[i], 0, _Quality);
		}
	}	
}

void CBuilder_XWC::XWC_AddWaveStreamDirQuality(CStr _Directory, int _bRecursive, fp32 _Quality)
{
	CBuilder_XWC *pThis = *m_pThis;
	TArray<CStr> Files;

	CStr Dir = _Directory;
	if (Dir.GetDevice() == "")
	{
		MACRO_GetSystem;
		Dir = pSys->GetEnvironment()->GetValue("XWC_WaveSourcePath", pThis->m_pDFB->GetCurrentScriptPath().GetPath()) + Dir;
	}

	// Find all files
	GetFileList_r(Dir, Files, _bRecursive != 0);

	for (int i = 0; i < Files.Len(); ++i)
	{
		CStr Ext = Files[i].GetFilenameExtenstion().LowerCase();
		if (Ext == "wav" || Ext == "xrs")
		{
			XWC_AddWave4(Files[i].GetFilenameNoExt(), Files[i], 1, _Quality);
		}
	}	
}

void CBuilder_XWC::XWC_AddWaveDir(CStr _Directory, int _bRecursive)
{
	CBuilder_XWC *pThis = *m_pThis;
	TArray<CStr> Files;

	CStr Dir = _Directory;
	if (Dir.GetDevice() == "")
	{
		MACRO_GetSystem;
		Dir = pSys->GetEnvironment()->GetValue("XWC_WaveSourcePath", pThis->m_pDFB->GetCurrentScriptPath().GetPath()) + Dir;
	}

	// Find all files
	GetFileList_r(Dir, Files, _bRecursive != 0);

	for (int i = 0; i < Files.Len(); ++i)
	{
		CStr Ext = Files[i].GetFilenameExtenstion().LowerCase();
		if (Ext == "wav" || Ext == "xrs")
		{
			XWC_AddWave(Files[i].GetFilenameNoExt(), Files[i]);
		}
	}	
}

void CBuilder_XWC::XWC_AddWaveStreamDir(CStr _Directory, int _bRecursive)
{
	CBuilder_XWC *pThis = *m_pThis;
	TArray<CStr> Files;

	CStr Dir = _Directory;
	if (Dir.GetDevice() == "")
	{
		MACRO_GetSystem;
		Dir = pSys->GetEnvironment()->GetValue("XWC_WaveSourcePath", pThis->m_pDFB->GetCurrentScriptPath().GetPath()) + Dir;
	}


	CStr Logging = "AddWaveStreamDir Dir: ";
	Logging += Dir;
	pThis->m_pDFB->Log(Logging);

	// Find all files
	GetFileList_r(Dir, Files, _bRecursive != 0);

	for (int i = 0; i < Files.Len(); ++i)
	{
		CStr Logging = "AddWaveStreamDir: ";
		Logging += Files[i];
		pThis->m_pDFB->Log(Logging);

		CStr Ext = Files[i].GetFilenameExtenstion().LowerCase();
		if (Ext == "wav" || Ext == "xrs")
		{
			XWC_AddWaveStream(Files[i].GetFilenameNoExt(), Files[i]);
		}
	}	
}

void CBuilder_XWC::XWC_AddWaveStreamLipSyncDir(CStr _Directory, int _bRecursive)
{
	CBuilder_XWC *pThis = *m_pThis;
	TArray<CStr> Files;

	CStr Dir = _Directory;
	if (Dir.GetDevice() == "")
	{
		MACRO_GetSystem;
		Dir = pSys->GetEnvironment()->GetValue("XWC_WaveSourcePath", pThis->m_pDFB->GetCurrentScriptPath().GetPath()) + Dir;
	}

	// Find all files
	GetFileList_r(Dir, Files, _bRecursive != 0);

	for (int i = 0; i < Files.Len(); ++i)
	{
		CStr Ext = Files[i].GetFilenameExtenstion().LowerCase();
		if (Ext == "wav" || Ext == "xrs")
		{
			XWC_AddWaveStreamLipSync(Files[i].GetFilenameNoExt(), Files[i]);
		}
	}	
}

void CBuilder_XWC::XWC_AddWaveLipSyncDir(CStr _Directory, int _bRecursive)
{
	CBuilder_XWC *pThis = *m_pThis;
	TArray<CStr> Files;

	CStr Dir = _Directory;
	if (Dir.GetDevice() == "")
	{
		MACRO_GetSystem;
		Dir = pSys->GetEnvironment()->GetValue("XWC_WaveSourcePath", pThis->m_pDFB->GetCurrentScriptPath().GetPath()) + Dir;
	}

	// Find all files
	GetFileList_r(Dir, Files, _bRecursive != 0);

	for (int i = 0; i < Files.Len(); ++i)
	{
		CStr Ext = Files[i].GetFilenameExtenstion().LowerCase();
		if (Ext == "wav" || Ext == "xrs")
		{
			XWC_AddWaveLipSync(Files[i].GetFilenameNoExt(), Files[i]);
		}
	}	
}

void CBuilder_XWC::XWC_CheckVariableBounds(const char *_pName, const char *_pVariableName, float _Value, float _Lower, float _Upper)
{
	CBuilder_XWC *pThis = *m_pThis;
	if (_Value < _Lower)
		pThis->m_pDFB->Log(CStrF("WARNING: %s for sound %s has a value(%f) lower than the allowed value of %f", _pVariableName, _pName, _Value, _Lower));

	if (_Value > _Upper)
		pThis->m_pDFB->Log(CStrF("WARNING: %s for sound %s has a value(%f) larger than the allowed value of %f", _pVariableName, _pName, _Value, _Upper));
}

//
//
//
TArray<CSC_SFXDesc*> CBuilder_XWC::CreateSFXDesc(CStr _Name, int32 _Mode, int _Priority, fp32 _Pitch, 
	fp32 _PitchRndAmp, fp32 _Volume, fp32 _VolumeRndAmp, fp32 _AttnMaxDist, fp32 _AttnMinDist, int _VolumeCategory)
{
	CBuilder_XWC *pThis = *m_pThis;
	// Error_static checks
	{
		{
			bool bError = true;
			for(int i = 0; i < 16; i++)
				if (pThis->m_lspWC[i])
				{
					bError = false;
					break;
				}

			if(bError)
				Error_static("CreateSFXDesc", "No XWC created.");
		}

		if (!_Name.Len())
			Error_static("CreateSFXDesc", "Empty sound-name.");

		#ifndef USE_HASHED_SFXDESC
//			if (_Name.Len() > CSC_SFXDesc::MAXSOUNDLENGTH - 1)
//				Error_static("CreateSFXDesc", "Too long sound-name " + _Name);
		#endif

	}

	CSC_SFXDesc Desc;

	// Copy the name
	#ifdef USE_HASHED_SFXDESC
		Desc.m_SoundIndex = StringToHash(_Name);
	#else
		Desc.m_SoundName = _Name;
	#endif

	// Set the mode of the SFXDesc
	Desc.SetMode(_Mode);

	// Pitch is a little bit special....
	//fp32 Rate = pThis->m_spWC->GetWaveSampleRate(LastIndex);
	if (_Pitch <= 800.0)
	{
		XWC_CheckVariableBounds(_Name, "Pitch", _Pitch, 0.0, 790.0);
		Desc.SetPitch(_Pitch / 100.0);
	}
	//else
	//	Desc.SetPitch(_Pitch / Rate);
		
	if (_PitchRndAmp <= 800.0)
	{
		XWC_CheckVariableBounds(_Name, "PitchRndAmp", _PitchRndAmp, 0.0, 790.0);
		Desc.SetPitchRandAmp(_PitchRndAmp / 100.0);
	}
	//else
	//	Desc.SetPitchRandAmp(_PitchRndAmp / Rate);

	// Check bounds
	XWC_CheckVariableBounds(_Name, "Volume", _Volume, 0.0, 1.0);
	XWC_CheckVariableBounds(_Name, "VolumeRndAmp", _VolumeRndAmp, 0.0, 1.0);
	XWC_CheckVariableBounds(_Name, "AttnMaxDist", _AttnMaxDist, 0.0, (65536.0 * 16.0) - 1.0);
	XWC_CheckVariableBounds(_Name, "AttnMinDist", _AttnMinDist, 0.0, (65536.0 * 16.0) - 1.0);

	// Ser values
	Desc.SetPriority(_Priority);
	Desc.SetVolume(_Volume);
	Desc.SetVolumeRandAmp(_VolumeRndAmp);
	Desc.SetAttnMaxDist(_AttnMaxDist);
	Desc.SetAttnMinDist(_AttnMinDist);
	Desc.SetCategory(_VolumeCategory);


	// distortion check
	if ((_Volume + _VolumeRndAmp) > 1.0)
		pThis->m_pDFB->Log(CStrF("WARNING: %s has a Volume and VolumeRndAmp that adds up to %f, when it should only add up to 1.0", _Name.Str(), _Volume + _VolumeRndAmp));

	TArray<CSC_SFXDesc*> lpSFXDesc;
	lpSFXDesc.SetLen(6);
	for(int i = 0; i < 6; i++)
	{
		// finally add the entry
		if(pThis->m_lspWC[i])
		{
			CSC_SFXDesc DescCopy;
			DescCopy = Desc;
			DescCopy.SetWaveContatiner(pThis->m_lspWC[i]);
			pThis->m_lspWC[i]->AddSFXDesc(DescCopy);
			lpSFXDesc[i] = pThis->m_lspWC[i]->GetSFXDesc(pThis->m_lspWC[i]->GetSFXDescIndex(_Name));
		}
	}
	return lpSFXDesc;
}

//
//
//
void CBuilder_XWC::XWC_AddSFXDescCat(CStr _Name, CStr _WaveName, int _Priority, fp32 _Pitch, 
	fp32 _PitchRndAmp, fp32 _Volume, fp32 _VolumeRndAmp, fp32 _AttnMaxDist, fp32 _AttnMinDist, int _VolCategory)
{
	CBuilder_XWC *pThis = *m_pThis;
	// Error_static checks
	{
		if (!_WaveName.Len())
			Error_static("XWC_AddSFXDesc", "Empty wave-name.");

		for(int i = 0; i < 6; i++)
		{
			if(pThis->m_lspWC[i])
			{
				if(pThis->m_lspWC[i]->GetSFXDesc(pThis->m_lspWC[i]->GetSFXDescIndex(_Name)))
					Error_static("XWC_AddSFXDesc", CStrF("SFXDesc already exists. '%s'", _Name.Str()));
			}
		}
	}

	TArray<CSC_SFXDesc*> lpDesc = CreateSFXDesc(	_Name, CSC_SFXDesc::ENORMAL, _Priority, _Pitch, _PitchRndAmp,
										_Volume, _VolumeRndAmp, _AttnMaxDist, _AttnMinDist, _VolCategory);
	
	for(int i = 0; i < 6; i++)
	{
		if(pThis->m_lspWC[i])
		{
			AddWavesToThinList(i, lpDesc[i]->GetNormalParams()->m_lWaves, _WaveName);
		}
	}
}

void CBuilder_XWC::XWC_AddSFXDesc(CStr _Name, CStr _WaveName, int _Priority, fp32 _Pitch, 
	fp32 _PitchRndAmp, fp32 _Volume, fp32 _VolumeRndAmp, fp32 _AttnMaxDist, fp32 _AttnMinDist)
{
	return XWC_AddSFXDescCat(_Name, _WaveName, _Priority, _Pitch, _PitchRndAmp, _Volume, _VolumeRndAmp, _AttnMaxDist, _AttnMinDist, 0);
}

void CBuilder_XWC::XWC_AddSFXDescDirCat(CStr _Path, int _Priority, fp32 _Pitch, 
								  fp32 _PitchRndAmp, fp32 _Volume, fp32 _VolumeRndAmp, fp32 _AttnMaxDist, fp32 _AttnMinDist, int _Category)
{
	CBuilder_XWC *pThis = *m_pThis;
	TArray<CStr> Files;

	CStr Dir = _Path;
	if (Dir.GetDevice() == "")
	{
		MACRO_GetSystem;
		Dir = pSys->GetEnvironment()->GetValue("XWC_WaveSourcePath", pThis->m_pDFB->GetCurrentScriptPath().GetPath()) + Dir;
	}

	// Find all files
	GetFileList_r(Dir, Files, true);

	for (int i = 0; i < Files.Len(); ++i)
	{
		CStr Ext = Files[i].GetFilenameExtenstion().LowerCase();
		if (Ext == "wav" || Ext == "xrs")
			XWC_AddSFXDescCat(Files[i].GetFilenameNoExt(), Files[i].GetFilenameNoExt(), _Priority, _Pitch, _PitchRndAmp, _Volume, _VolumeRndAmp, _AttnMaxDist, _AttnMinDist, _Category);
	}	
}

void CBuilder_XWC::XWC_AddSFXDescDir(CStr _Path, int _Priority, fp32 _Pitch, 
								  fp32 _PitchRndAmp, fp32 _Volume, fp32 _VolumeRndAmp, fp32 _AttnMaxDist, fp32 _AttnMinDist)
{
	CBuilder_XWC *pThis = *m_pThis;
	TArray<CStr> Files;

	CStr Dir = _Path;
	if (Dir.GetDevice() == "")
	{
		MACRO_GetSystem;
		Dir = pSys->GetEnvironment()->GetValue("XWC_WaveSourcePath", pThis->m_pDFB->GetCurrentScriptPath().GetPath()) + Dir;
	}

	// Find all files
	GetFileList_r(Dir, Files, true);

	for (int i = 0; i < Files.Len(); ++i)
	{
		CStr Ext = Files[i].GetFilenameExtenstion().LowerCase();
		if (Ext == "wav" || Ext == "xrs")
			XWC_AddSFXDesc(Files[i].GetFilenameNoExt(), Files[i].GetFilenameNoExt(), _Priority, _Pitch, _PitchRndAmp, _Volume, _VolumeRndAmp, _AttnMaxDist, _AttnMinDist);
	}	
}

//
//
//
void CBuilder_XWC::XWC_AddMaterialSFXDesc(CStr _Name, int _Priority, fp32 _Pitch, fp32 _PitchRndAmp, 
		fp32 _Volume, fp32 _VolumeRndAmp, fp32 _AttnMaxDist, fp32 _AttnMinDist)
{
	CreateSFXDesc(	_Name, CSC_SFXDesc::EMATERIAL, _Priority, _Pitch, _PitchRndAmp, _Volume, _VolumeRndAmp, _AttnMaxDist, _AttnMinDist, 0);
}

void CBuilder_XWC::XWC_AddMaterialSFXDescCat(CStr _Name, int _Priority, fp32 _Pitch, fp32 _PitchRndAmp, 
		fp32 _Volume, fp32 _VolumeRndAmp, fp32 _AttnMaxDist, fp32 _AttnMinDist, int _Cat)
{
	CreateSFXDesc(	_Name, CSC_SFXDesc::EMATERIAL, _Priority, _Pitch, _PitchRndAmp, _Volume, _VolumeRndAmp, _AttnMaxDist, _AttnMinDist, _Cat);
}

//
//
//
void CBuilder_XWC::AddWavesToThinList(int _iPlatform, TThinArray<int16> &_List, CStr _WaveName)
{
	CBuilder_XWC *pThis = *m_pThis;
	// Size up the list abit
	int32 Count = _List.Len();
	int32 CurrentSize = Count+10;
	_List.SetLen(CurrentSize);

	while(_WaveName.Len())
	{
		CStr SubName = _WaveName.GetStrSep(";");

		// Error_static checks
		if (!SubName.Len())
			Error_static("XWC_AddWavesToMaterialSFXDesc", "Empty subwave-name in " + _WaveName);
		
		#ifndef USE_HASHED_WAVENAME
//			if (SubName.Len() > CSC_SFXDesc::MAXSOUNDLENGTH-1)
//				Error_static("XWC_AddWavesToMaterialSFXDesc", "Too long wave-name " + _WaveName + ", " + SubName);
		#endif

		int32 Index = pThis->m_lspWC[_iPlatform]->GetIndex(SubName);
		
		if (Index < 0)
			pThis->m_pDFB->Log("WARNING: Something references undefined waveform " + SubName + ".");
		else
		{
			// Size up the array if needed
			if(Count == CurrentSize)
			{
				CurrentSize+=10;
				_List.SetLen(CurrentSize);
			}

			// Add the index to the list
			_List[Count++] = Index;
		}
	}

	// Optimize the size of the array
	_List.SetLen(Count);
}

//
//
//
void CBuilder_XWC::XWC_AddWavesToMaterialSFXDesc(CStr _Name, int32 _iMaterial, CStr _WaveName)
{
	CBuilder_XWC *pThis = *m_pThis;
	for(int i = 0; i < 6; i++)
	{
		if(!pThis->m_lspWC[i])
			continue;

		CWaveContainer_Plain* pWC = pThis->m_lspWC[i];
		CSC_SFXDesc *pDesc = pWC->GetSFXDesc(pWC->GetSFXDescIndex(_Name));
		
		if(pDesc->GetMode() != CSC_SFXDesc::EMATERIAL)
			Error_static("XWC_AddWaveToMaterialSFXDesc", "Sound description is not in material mode." + _Name);

		CSC_SFXDesc::CMaterialParams::CMaterialWaveHolder *pHolder = pDesc->GetMaterialParams()->GetOrCreateMaterial(_iMaterial);

		if(!pHolder)
			Error_static("XWC_AddWaveToMaterialSFXDesc", "Sound description has no wave holder." + _Name);

		AddWavesToThinList(i, pHolder->m_lWaves, _WaveName);
	}
}

/*
void CBuilder_XWC::XWC_AddSFXDesc(CStr _Name, CStr _WaveName, int _Priority, fp32 _Pitch, 
	fp32 _PitchRndAmp, fp32 _Volume, fp32 _VolumeRndAmp, fp32 _AttnMaxDist, fp32 _AttnMinDist)
{
	XWC_AddSFXDesc2(_Name, _WaveName, _Priority, _Pitch, _PitchRndAmp, _Volume, _VolumeRndAmp, _AttnMaxDist, _AttnMinDist, 1.0f, 1.0f, 0, 0);
}
*/


/*
void CBuilder_XWC::XWC_WriteWaveList(int _hDFile, int _Flags)
{
	if (!pThis->m_spWC) Error_static("XWC_WriteWaveList", "No XWC created.");

//	pThis->m_spWC->WriteWaveList(pThis->m_pDFB->GetDataFile(_hDFile));
}
*/
/*
void CBuilder_XWC::XWC_WriteSFXDesc(int _hDFile, int _Flags)
{
	if (!pThis->m_spWC) Error_static("XWC_WriteSFXDesc", "No XWC created.");

	CDataFile* pDFile = pThis->m_pDFB->GetDataFile(_hDFile);
	CCFile* pFile = pDFile->GetFile();

	for(int i = 0; i < pThis->m_lSFXDesc.Len(); i++)
		pThis->m_lSFXDesc[i].Write(pFile, pThis->m_spWC);
}
*/

//
//
//
int CBuilder_XWC::XWC_GetNumWaves()
{
	CBuilder_XWC *pThis = *m_pThis;
	int nWaves = ~0;
	for(int i = 0; i < 6; i++)
	{
		if (pThis->m_lspWC[i])
		{
			int nW = pThis->m_lspWC[i]->GetWaveCount();
			if(nWaves == ~0)
				nWaves = nW;
			else if(nWaves != nW)
				Error_static("XWC_GetNumWaves", "Diffrent platforms get diffrent wavecount");
		}
	}

	return nWaves;
}

//
//
//
int CBuilder_XWC::XWC_GetNumSFXDesc()
{
	CBuilder_XWC *pThis = *m_pThis;
	int nSFCDesc = ~0;
	for(int i = 0; i < 6; i++)
	{
		if (pThis->m_lspWC[i])
		{
			int nS = pThis->m_lspWC[i]->GetSFXCount();
			if(nSFCDesc == ~0)
				nSFCDesc = nS;
			else if(nSFCDesc != nS)
				Error_static("XWC_GetNumSFXDesc", "Diffrent platforms get diffrent sfcdesc count");
		}
	}

	return nSFCDesc;
}

//
//
//
void CBuilder_XWC::Register(CScriptRegisterContext & _RegContext)
{
	_RegContext.RegFunction("XWC_Begin", &CBuilder_XWC::XWC_Begin);
	_RegContext.RegFunction("XWC_End", &CBuilder_XWC::XWC_End);
	_RegContext.RegFunction("XWC_AddWave", &CBuilder_XWC::XWC_AddWave);
	_RegContext.RegFunction("XWC_AddWaveQuality", &CBuilder_XWC::XWC_AddWaveQuality);
	_RegContext.RegFunction("XWC_AddWaveDir", &CBuilder_XWC::XWC_AddWaveDir);
	_RegContext.RegFunction("XWC_AddWaveStreamDir", &CBuilder_XWC::XWC_AddWaveStreamDir);

	_RegContext.RegFunction("XWC_AddWaveStreamLipSyncDir", &CBuilder_XWC::XWC_AddWaveStreamLipSyncDir);
	_RegContext.RegFunction("XWC_AddWaveLipSyncDir", &CBuilder_XWC::XWC_AddWaveLipSyncDir);

	_RegContext.RegFunction("XWC_AddWaveStream", &CBuilder_XWC::XWC_AddWaveStream);
	_RegContext.RegFunction("XWC_AddWaveLipSync", &CBuilder_XWC::XWC_AddWaveLipSync);
	_RegContext.RegFunction("XWC_AddWaveStreamLipSync", &CBuilder_XWC::XWC_AddWaveStreamLipSync);

	_RegContext.RegFunction("XWC_AddWaveStreamQuality", &CBuilder_XWC::XWC_AddWaveStreamQuality);
	_RegContext.RegFunction("XWC_AddWaveLipSyncQuality", &CBuilder_XWC::XWC_AddWaveLipSyncQuality);
	_RegContext.RegFunction("XWC_AddWaveStreamLipSyncQuality", &CBuilder_XWC::XWC_AddWaveStreamLipSyncQuality);

	_RegContext.RegFunction("XWC_AddWaveStreamLipSyncDirQuality", &CBuilder_XWC::XWC_AddWaveStreamLipSyncDirQuality);
	_RegContext.RegFunction("XWC_AddWaveLipSyncDirQuality", &CBuilder_XWC::XWC_AddWaveLipSyncDirQuality);
	_RegContext.RegFunction("XWC_AddWaveDirQuality", &CBuilder_XWC::XWC_AddWaveDirQuality);
	_RegContext.RegFunction("XWC_AddWaveStreamDirQuality", &CBuilder_XWC::XWC_AddWaveStreamDirQuality);

	_RegContext.RegFunction("XWC_AddSFXDesc", &CBuilder_XWC::XWC_AddSFXDesc);
	_RegContext.RegFunction("XWC_AddSFXDescDir", &CBuilder_XWC::XWC_AddSFXDescDir);
	_RegContext.RegFunction("XWC_AddSFXDescCat", &CBuilder_XWC::XWC_AddSFXDescCat);
	_RegContext.RegFunction("XWC_AddSFXDescDirCat", &CBuilder_XWC::XWC_AddSFXDescDirCat);
	
	_RegContext.RegFunction("XWC_AddMaterialSFXDesc", &CBuilder_XWC::XWC_AddMaterialSFXDesc);
	_RegContext.RegFunction("XWC_AddMaterialSFXDescCat", &CBuilder_XWC::XWC_AddMaterialSFXDescCat);
	_RegContext.RegFunction("XWC_AddWavesToMaterialSFXDesc", &CBuilder_XWC::XWC_AddWavesToMaterialSFXDesc);
	
	_RegContext.RegFunction("XWC_GetNumWaves", &CBuilder_XWC::XWC_GetNumWaves);
	_RegContext.RegFunction("XWC_GetNumSFXDesc", &CBuilder_XWC::XWC_GetNumSFXDesc);

}

// -------------------------------------------------------------------
//  CBuilder_XFC
// -------------------------------------------------------------------
TMRTC_ThreadLocal<CBuilder_XFC *> CBuilder_XFC::m_pThis;
CBuilder_XFC::CBuilder_XFC(CDataFileBuilder* _pDFB)
{
	m_pDFB = _pDFB;
	m_bIsAdded = true;
//	g_pOS->m_spCon->AddSubSystem(this);
}

CBuilder_XFC::CBuilder_XFC()
{
	m_bIsAdded = true;
	AddToConsole();
}

CBuilder_XFC::~CBuilder_XFC()
{
	if (m_bIsAdded)
		RemoveFromConsole();
//	g_pOS->m_spCon->RemoveSubSystem(this);
}

void CBuilder_XFC::XFC_ReadFontScript(CStr _Filename)
{
	CBuilder_XFC *pThis = *m_pThis;
	if (_Filename.GetDevice() == "") _Filename = pThis->m_pDFB->GetCurrentScriptPath() + _Filename;
	pThis->m_spFont = MNew(CRC_Font);
	pThis->m_spFont->ReadFromScript(_Filename);
}

#ifdef PLATFORM_WIN_PC
void CBuilder_XFC::XFC_ReadFontScriptWin32(CStr _Filename)
{
	CBuilder_XFC *pThis = *m_pThis;
	if (_Filename.GetDevice() == "") _Filename = pThis->m_pDFB->GetCurrentScriptPath() + _Filename;
	pThis->m_spFont = MNew(CRC_Font);
	pThis->m_spFont->ReadFromScriptWin32(_Filename);
}
#endif

void CBuilder_XFC::XFC_WriteFont(int _hDFile, int _Flags)
{
	CBuilder_XFC *pThis = *m_pThis;
	if (!pThis->m_spFont) Error_static("XFC_WriteFont", "No font read.");
	pThis->m_spFont->WriteXFC(pThis->m_pDFB->GetDataFile(_hDFile));
}

void CBuilder_XFC::XFC_Compile(CStr _Config, CStr _Output)
{
	CBuilder_XFC *pThis = *m_pThis;
	XFC_ReadFontScriptWin32(_Config);
	int OutputXTC = pThis->m_pDFB->DFile_Create(_Output);
	XFC_WriteFont(OutputXTC, 0);
	pThis->m_pDFB->DFile_Close(OutputXTC);
}

void CBuilder_XFC::Register(CScriptRegisterContext & _RegContext)
{
	
	_RegContext.RegFunction("XFC_ReadFontScript", &CBuilder_XFC::XFC_ReadFontScript);
#ifdef PLATFORM_WIN_PC
	_RegContext.RegFunction("XFC_ReadFontScriptWin32", &CBuilder_XFC::XFC_ReadFontScriptWin32);
#endif
	_RegContext.RegFunction("XFC_WriteFont", &CBuilder_XFC::XFC_WriteFont);
	_RegContext.RegFunction("XFC_Compile", &CBuilder_XFC::XFC_Compile);

}

// -------------------------------------------------------------------
//  CBuilder_ASE script extension
// -------------------------------------------------------------------
#ifdef XWC
#include "../../XWC/XMDConv.h"

TMRTC_ThreadLocal<CBuilder_ASE *> CBuilder_ASE::m_pThis;
CBuilder_ASE::CBuilder_ASE(CDataFileBuilder* _pDFB)
{
	m_pDFB = _pDFB;
	m_bIsAdded = true;
//	g_pOS->m_spCon->AddSubSystem(this);
}

CBuilder_ASE::CBuilder_ASE()
{
	m_bIsAdded = true;
	AddToConsole();
}

CBuilder_ASE::~CBuilder_ASE()
{
	if (m_bIsAdded)
		RemoveFromConsole();
//	g_pOS->m_spCon->RemoveSubSystem(this);
}

// -------------------------------------------------------------------
int CBuilder_ASE::ASE_Read(CStr _Filename)
{
	CBuilder_ASE *pThis = *m_pThis;
	spCParamNode spRoot = MNew(CParamNode);
	if (!spRoot) ThrowError("ASE_Read");

	if (_Filename.GetDevice() == "") _Filename = pThis->m_pDFB->GetCurrentScriptPath() + _Filename;
	spRoot->ReadASE(_Filename);
	return pThis->m_lspRoots.Add(spRoot);
}

void CBuilder_ASE::ASE_Close(int _hASE)
{
	CBuilder_ASE *pThis = *m_pThis;
	pThis->m_lspRoots[_hASE] = NULL;
}

CParamNode* CBuilder_ASE::ASE_Get(int _hASE)
{
	CBuilder_ASE *pThis = *m_pThis;
	if ((_hASE < 0) || (_hASE >= pThis->m_lspRoots.Len()) || (!pThis->m_lspRoots[_hASE])) Error_static("ASE_Get", "Invalid ASE handle.");
	return pThis->m_lspRoots[_hASE];
}

void CBuilder_ASE::ASE_LogDump(int _hASE, int _Level)
{
	CBuilder_ASE *pThis = *m_pThis;
	ASE_Get(_hASE)->ASE_LogDump(0);
}

// -------------------------------------------------------------------
int CBuilder_ASE::ASE_TriMesh_Create()
{
	CBuilder_ASE *pThis = *m_pThis;
	return pThis->m_lspTriMeshes.Add(MNew(CTriangleMesh));
}

void CBuilder_ASE::ASE_TriMesh_Close(int _hTM)
{
	CBuilder_ASE *pThis = *m_pThis;
	pThis->m_lspTriMeshes[_hTM] = NULL;
}

CTriangleMesh* CBuilder_ASE::ASE_TriMesh_Get(int _hTM)
{
	CBuilder_ASE *pThis = *m_pThis;
	if ((_hTM < 0) || (_hTM >= pThis->m_lspTriMeshes.Len()) || (!pThis->m_lspTriMeshes[_hTM])) 
		Error_static("ASE_TriMesh_Get", "Invalid TriMesh handle.");
	return pThis->m_lspTriMeshes[_hTM];
}

void CBuilder_ASE::ASE_TriMesh_Parse(int _hTM, int _hASE, int _Flags)
{
	ASE_TriMesh_Get(_hTM)->ASE_Parse(ASE_Get(_hASE), _Flags);
}

void CBuilder_ASE::ASE_TriMesh_Write(int _hTM, CStr _Filename)
{
	CBuilder_ASE *pThis = *m_pThis;
	if (_Filename.GetDevice() == "") _Filename = pThis->m_pDFB->GetCurrentScriptPath() + _Filename;
	ASE_TriMesh_Get(_hTM)->WriteXMD(_Filename);
}

int CBuilder_ASE::ASE_TriMesh_AddTexture(int _hTM, CStr _Filename, CStr _Name, int _Flags)
{
	CBuilder_ASE *pThis = *m_pThis;
	if (_Filename.GetDevice() == "") _Filename = pThis->m_pDFB->GetCurrentScriptPath() + _Filename;
	return ASE_TriMesh_Get(_hTM)->AddTexture(_Filename, _Name, _Flags);
}

void CBuilder_ASE::ASE_TriMesh_MapAll(int _hTM, int _iMap, int _iTxt)
{
	CBuilder_ASE *pThis = *m_pThis;
	Error_static("ASE_TriMesh_MapAll", "Not implemented.");
//	ASE_TriMesh_Get(_hTM)->MapAll(_iMap, _iTxt);
}

// -------------------------------------------------------------------
void CBuilder_ASE::Register(CScriptRegisterContext & _RegContext)
{		
	_RegContext.RegFunction("ASE_Read", &CBuilder_ASE::ASE_Read);
	_RegContext.RegFunction("ASE_Close", &CBuilder_ASE::ASE_Close);
	_RegContext.RegFunction("ASE_LogDump", &CBuilder_ASE::ASE_LogDump);
	_RegContext.RegFunction("ASE_TriMesh_Create", &CBuilder_ASE::ASE_TriMesh_Create);
	_RegContext.RegFunction("ASE_TriMesh_Close", &CBuilder_ASE::ASE_TriMesh_Close);
	_RegContext.RegFunction("ASE_TriMesh_Parse", &CBuilder_ASE::ASE_TriMesh_Parse);
	_RegContext.RegFunction("ASE_TriMesh_Write", &CBuilder_ASE::ASE_TriMesh_Write);
	_RegContext.RegFunction("ASE_TriMesh_AddTexture", &CBuilder_ASE::ASE_TriMesh_AddTexture);
	_RegContext.RegFunction("ASE_TriMesh_MapAll", &CBuilder_ASE::ASE_TriMesh_MapAll);
}

#endif
