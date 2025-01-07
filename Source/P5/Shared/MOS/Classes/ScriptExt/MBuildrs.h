// XWCScrpt

#ifndef _INC_MBUILDRS
#define _INC_MBUILDRS

#include "../../MOS.h"
#include "../ASE/MASE.h"
#include "../Render/MRenderUtil.h"

// -------------------------------------------------------------------
//  CDataFileBuilder
// -------------------------------------------------------------------
class CDataFileBuilder : public CConsoleClient
{
	bint m_bIsAdded;
	TArray<spCDataFile> m_lspDataFiles;
	CStr m_ScriptPath;

public:
	static TMRTC_ThreadLocal<CDataFileBuilder *> m_pThis;
	CDataFileBuilder();
	CDataFileBuilder(CStr _ScriptPath);
	~CDataFileBuilder();

	static CDataFile* GetDataFile(int _hDFile);
	static CStr GetCurrentScriptPath();

	static int DFile_Create(CStr _Filename);		// ret. _hDFile
	static void DFile_Close(int _hDFile);

	static void DFile_BeginEntry(int _hDFile, CStr _Name);
	static void DFile_EndEntry(int _hDFile, int _UserData);
	static void DFile_BeginSubDir(int _hDFile);
	static void DFile_EndSubDir(int _hDFile);

	static void Log(CStr _Str);

	void Register(CScriptRegisterContext &_RegContext);
};

typedef TPtr<CDataFileBuilder> spCDataFileBuilder;

// -------------------------------------------------------------------
//  CBuilder_XTC
// -------------------------------------------------------------------
class CBuilder_XTC : public CConsoleClient
{
	CDataFileBuilder* m_pDFB;
	spCTextureContainer_Plain m_spTC;
	bint m_bIsAdded;

public:
	static TMRTC_ThreadLocal<CBuilder_XTC *> m_pThis;
	CBuilder_XTC();
	CBuilder_XTC(CDataFileBuilder* _pDFB);
	~CBuilder_XTC();

	static void XTC_Begin();
	static void XTC_End();
	static void XTC_Finalize(int _Flags);

	static void XTC_AddTextureFlags(CStr _Name, CStr _Filename, int _Flags);
	static void XTC_AddTexture(CStr _Name, CStr _Filename);

	static void XTC_WriteImageList(int _hDFile, int _Flags);
	static void XTC_WriteWAD(CStr _Filename);

	static void XTC_Quantize(int _iLocal);
	static int XTC_GetNumTextures();

	void Register(CScriptRegisterContext &_RegContext);
};

// -------------------------------------------------------------------
//  CBuilder_XWC
// -------------------------------------------------------------------
class CBuilder_XWC : public CConsoleClient
{
	bint m_bIsAdded;
	CDataFileBuilder* m_pDFB;
	spCWaveContainer_Plain m_lspWC[16];		// Do 1 for each enabled platform
//	TArray<CSC_SFXDesc> m_lSFXDesc;
	CStr m_DestPath;

	CStr m_DestFileName;
	CStr m_lDestFilePath[16];
	int64 m_lDestFileTimes[16]; // Last write times for the dest WCs.

	bool m_bUseDestWaves;

	// Hold the destination wave containers if they exist and bUseDestWaves is true.
	// Actually act as sources of data, despite their name.
	spCWaveContainer_Plain m_lspDestWC[16];

public:
	static TMRTC_ThreadLocal<CBuilder_XWC *> m_pThis;
	CBuilder_XWC();
	CBuilder_XWC(CDataFileBuilder* _pDFB);
	CBuilder_XWC(CDataFileBuilder* _pDFB, bool _bUseDestWaves);
	~CBuilder_XWC();

	// Helper functions
	static TArray<CSC_SFXDesc*> CreateSFXDesc(CStr _Name, int32 _Mode, int _Priority, fp32 _Pitch, 
				fp32 _PitchRndAmp, fp32 _Volume, fp32 _VolumeRndAmp, fp32 _AttnMaxDist, fp32 _AttnMinDist, int _VolumeCategory);

	static void AddWavesToThinList(int _iPlatform, TThinArray<int16> &_List, CStr _WaveName);



	static void XWC_Begin(CStr _FileName);
	static void XWC_End();
//	void XWC_Finalize(int _Flags);

	static void XWC_AddWave4(CStr _Name, CStr _Filename, int32 _Flags, fp32 _Quality);
	static void XWC_AddWaveStreamLipSync(CStr _Name, CStr _Filename);
	static void XWC_AddWaveLipSync(CStr _Name, CStr _Filename);
	static void XWC_AddWaveStream(CStr _Name, CStr _Filename);
	static void XWC_AddWave(CStr _Name, CStr _Filename);

	static void XWC_AddWaveStreamLipSyncQuality(CStr _Name, CStr _Filename, fp32 _Quality);
	static void XWC_AddWaveLipSyncQuality(CStr _Name, CStr _Filename, fp32 _Quality);
	static void XWC_AddWaveStreamQuality(CStr _Name, CStr _Filename, fp32 _Quality);
	static void XWC_AddWaveQuality(CStr _Name, CStr _Filename, fp32 _Quality);

	static void XWC_AddWaveStreamLipSyncDir(CStr _Directory, int _bRecursive);
	static void XWC_AddWaveLipSyncDir(CStr _Directory, int _bRecursive);
	static void XWC_AddWaveDir(CStr _Directory, int _bRecursive);
	static void XWC_AddWaveStreamDir(CStr _Directory, int _bRecursive);

	static void XWC_AddWaveStreamLipSyncDirQuality(CStr _Directory, int _bRecursive, fp32 _Quality);
	static void XWC_AddWaveLipSyncDirQuality(CStr _Directory, int _bRecursive, fp32 _Quality);
	static void XWC_AddWaveDirQuality(CStr _Directory, int _bRecursive, fp32 _Quality);
	static void XWC_AddWaveStreamDirQuality(CStr _Directory, int _bRecursive, fp32 _Quality);


	static void XWC_AddSFXDesc(CStr _Name, CStr _WaveName, int _Priority, fp32 _Pitch, fp32 _PitchRndAmp, 
		fp32 _Volume, fp32 _VolumeRndAmp, fp32 _AttnMaxDist, fp32 _AttnMinDist);
	static void XWC_AddSFXDescDir(CStr _Path, int _Priority, fp32 _Pitch, fp32 _PitchRndAmp, 
		fp32 _Volume, fp32 _VolumeRndAmp, fp32 _AttnMaxDist, fp32 _AttnMinDist);

	static void XWC_AddSFXDescCat(CStr _Name, CStr _WaveName, int _Priority, fp32 _Pitch, fp32 _PitchRndAmp, 
		fp32 _Volume, fp32 _VolumeRndAmp, fp32 _AttnMaxDist, fp32 _AttnMinDist, int _VolumeCategory);
	static void XWC_AddSFXDescDirCat(CStr _Path, int _Priority, fp32 _Pitch, fp32 _PitchRndAmp, 
		fp32 _Volume, fp32 _VolumeRndAmp, fp32 _AttnMaxDist, fp32 _AttnMinDist, int _VolumeCategory);


	/*
	void XWC_AddSFXDesc2(CStr _Name, CStr _WaveName, int _Priority, fp32 _Pitch, fp32 _PitchRndAmp, 
		fp32 _Volume, fp32 _VolumeRndAmp, fp32 _AttnMaxDist, fp32 _AttnMinDist, 
		fp32 _PanBiasMin, fp32 _PanBiasMax, fp32 _PanBiasMinDist, fp32 _PanBiasMaxDist);
		*/

	static void XWC_AddMaterialSFXDesc(CStr _Name, int _Priority, fp32 _Pitch, fp32 _PitchRndAmp, 
		fp32 _Volume, fp32 _VolumeRndAmp, fp32 _AttnMaxDist, fp32 _AttnMinDist);

	static void XWC_AddMaterialSFXDescCat(CStr _Name, int _Priority, fp32 _Pitch, fp32 _PitchRndAmp, 
		fp32 _Volume, fp32 _VolumeRndAmp, fp32 _AttnMaxDist, fp32 _AttnMinDist, int _VolumeCategory);


	static void XWC_AddWavesToMaterialSFXDesc(CStr _Name, int32 _iMaterial, CStr _WaveName);

//	void XWC_WriteWaveList(int _hDFile, int _Flags);
//	void XWC_WriteSFXDesc(int _hDFile, int _Flags);

	static void XWC_CheckVariableBounds(const char *_pName, const char *_pVariableName, float _Value, float _Lower, float _Upper);

	static int XWC_GetNumWaves();
	static int XWC_GetNumSFXDesc();

	void Register(CScriptRegisterContext &_RegContext);
};

// -------------------------------------------------------------------
//  CBuilder_XFC, Font-container
// -------------------------------------------------------------------
class CBuilder_XFC : public CConsoleClient
{
	bint m_bIsAdded;
	CDataFileBuilder* m_pDFB;
	spCRC_Font m_spFont;

public:
	static TMRTC_ThreadLocal<CBuilder_XFC *> m_pThis;
	CBuilder_XFC();
	CBuilder_XFC(CDataFileBuilder* _pDFB);
	~CBuilder_XFC();

	static void XFC_ReadFontScript(CStr _Filename);
#ifdef PLATFORM_WIN_PC
	static void XFC_ReadFontScriptWin32(CStr _Filename);
#endif
	static void XFC_WriteFont(int _hDFile, int _Flags);
	static void XFC_Compile(CStr _Config, CStr _Output);

	void Register(CScriptRegisterContext &_RegContext);
};

// -------------------------------------------------------------------
//  CBuilder_ASE script extension
// -------------------------------------------------------------------
#ifdef XWC

class CTriangleMesh;
typedef TPtr<CTriangleMesh> spCTriangleMesh;

class CBuilder_ASE : public CConsoleClient
{
	bint m_bIsAdded;
	CDataFileBuilder* m_pDFB;
	TArray<spCParamNode> m_lspRoots;
	TArray<spCTriangleMesh> m_lspTriMeshes;

public:
	static TMRTC_ThreadLocal<CBuilder_ASE *> m_pThis;
	CBuilder_ASE();
	CBuilder_ASE(CDataFileBuilder* _pDFB);
	~CBuilder_ASE();

	static int ASE_Read(CStr _Filename);
	static void ASE_Close(int _hASE);
	static void ASE_LogDump(int _hASE, int _Level);
	static CParamNode* ASE_Get(int _hTM);

	static int ASE_TriMesh_Create();
	static void ASE_TriMesh_Close(int _hTM);
	static CTriangleMesh* ASE_TriMesh_Get(int _hTM);

	static void ASE_TriMesh_Parse(int _hTM, int _hASE, int _Flags);
	static void ASE_TriMesh_Write(int _hTM, CStr _Filename);
	static int ASE_TriMesh_AddTexture(int _hTM, CStr _Filename, CStr _Name, int _Flags);
	static void ASE_TriMesh_MapAll(int _hTM, int _iMap, int _iTxt);

	void Register(CScriptRegisterContext &_RegContext);
};

#endif

#endif // _INC_MBUILDRS
