
#include "PCH.h"

#include "WDataCore.h"
#include "WDataRes_Core.h"
#include "MFloat.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWorldDataLoader
|__________________________________________________________________________________________________
\*************************************************************************************************/

void StrFixFilename(const char* _pStr, char* _pDest, uint _nMax)
{
	uint i = 0, j = 0;
	if (_pStr)
	{
		while (_pStr[i] && (j < _nMax-1))
		{
			char c = _pStr[i++];
			if (c == '\\' || c == '/')
			{
				c = '/';
				while (_pStr[i] == '\\' || _pStr[i] == '/') i++;
			}
			_pDest[j++] = c;
		}
	}
	_pDest[j] = 0;
}

// #define WRESOURCE_ASYNC_IO

CWorldDataLoader::CWorldDataLoader()
{
	MAUTOSTRIP(CWorldDataLoader_ctor, MAUTOSTRIP_VOID);
	m_Status = 0;
}

void CWorldDataLoader::Create()
{
	MAUTOSTRIP(CWorldDataLoader_Create, MAUTOSTRIP_VOID);
#ifdef WRESOURCE_ASYNC_IO	
	m_lpLoadQueue.SetGrow(128);
	Thread_Create(NULL, 32768);
#endif
}

const char* CWorldDataLoader::Thread_GetName() const
{
	return "World data loader";
}

int CWorldDataLoader::Thread_Main()
{
	MAUTOSTRIP(CWorldDataLoader_Thread_Main, 0);
#ifndef WRESOURCE_ASYNC_IO
	M_ASSERT(0, "!");
#endif

	ConOutL("(CWorldDataLoader::Thread_Main) Started.");

	M_TRY
	{
		while(!(m_Thread_State & MRTC_THREAD_TERMINATING))
		{
			spCWResource spRc;

			// Find a resource to load
			{
				M_LOCK(*(MRTC_GOM()->m_pGlobalLock));

				if (m_lpLoadQueue.Len())
				{
					// Grab first in queue until we've found one that is not loaded.
					M_ASSERT(m_lpLoadQueue[0]->MRTC_ReferenceCount() > 0, "Queued object has no references.");
					spRc = m_lpLoadQueue[0];
					while(spRc != NULL && spRc->IsLoaded())
					{
						m_lpLoadQueue.Del(0);
						spRc = m_lpLoadQueue.Len() ? m_lpLoadQueue[0] : NULL;
						if (spRc != NULL)
							M_ASSERT(spRc->MRTC_ReferenceCount() > 1, "Queued object has no references.");
					}

					if (spRc != NULL)
						spRc->m_Flags |= WRESOURCE_ASYNCLOADING;
				}
			}

			// Found any?
			if (spRc != NULL)
			{
				// Load it
				{
					MSCOPESHORT(CWResource::OnLoad);
					RESOURCE_MEMSCOPE;

					M_TRY
					{
						spRc->OnLoad();
					}
					M_CATCH(
					catch(CCExceptionFile)
					{
						m_Status = DISKUTIL_STATUS_CORRUPTFILE;
						spRc->m_Flags |= WRESOURCE_CORRUPT;
					}
					)
#ifdef M_SUPPORTSTATUSCORRUPT
					M_CATCH(
					catch(CCException)
					{
						m_Status = DISKUTIL_STATUS_CORRUPT;
						spRc->m_Flags |= WRESOURCE_CORRUPT;
					}
					)
#endif
					spRc->m_MemUsed += RESOURCE_MEMDELTA;
				}

				// Done, remove it from queue and set some flags
				{
					M_LOCK(*(MRTC_GOM()->m_pGlobalLock));
					spRc->m_Flags |= WRESOURCE_LOADED;
					spRc->m_Flags &= ~WRESOURCE_ASYNCLOADING;
					m_lpLoadQueue.Del(0);
				}
			}
			else
				MRTC_SystemInfo::OS_Sleep(5);
		}
	}
	M_CATCH(
	catch(CCException)
	{
		ConOutL("(CWorldDataLoader::Thread_Main) Terminating due to unhandled exception.");
		throw;
	}
	)

	return 0;
}

void CWorldDataLoader::AddToQueue(CWResource* _pRc, bool _bUrgent)
{
	MAUTOSTRIP(CWorldDataLoader_AddToQueue, MAUTOSTRIP_VOID);
#ifndef WRESOURCE_ASYNC_IO
	if (_pRc->IsLoaded())
		return;

	_pRc->m_Flags |= WRESOURCE_ASYNCLOADING;
	
	{
		RESOURCE_MEMSCOPE;
		static const char ContextName[] = "CWResource::OnLoad";
		MRTC_Context _Context_(ContextName,_pRc->MRTC_ClassName());

		M_TRY
		{
			_pRc->OnLoad();
		}
		M_CATCH(
		catch(CCExceptionFile)
		{
			m_Status = DISKUTIL_STATUS_CORRUPTFILE;
			_pRc->m_Flags |= WRESOURCE_CORRUPT;
		}
		)
#ifdef M_SUPPORTSTATUSCORRUPT
		M_CATCH(
		catch(CCException)
		{
			m_Status = DISKUTIL_STATUS_CORRUPT;
			_pRc->m_Flags |= WRESOURCE_CORRUPT;
		}
		)
#endif
		_pRc->m_MemUsed += RESOURCE_MEMDELTA;
	}

	_pRc->m_Flags |= WRESOURCE_LOADED;
	_pRc->m_Flags &= ~WRESOURCE_ASYNCLOADING;

#else
	M_LOCK(*(MRTC_GOM()->m_pGlobalLock));

	if (!_pRc)
	{
		ConOutL("(CWorldDataLoader::AddToQueue) NULL object.");
		return;
	}

	if (_pRc->IsLoaded())
		return;

	for(int i = 0; i < m_lpLoadQueue.Len(); i++)
	{
		if(m_lpLoadQueue[i] == _pRc)
		{
			if (i > 1)
				m_lpLoadQueue.Del(i);
			m_lpLoadQueue.Insert(1, _pRc);
			return;
		}
	}

	if (_bUrgent && m_lpLoadQueue.Len())
		m_lpLoadQueue.Insert(1, _pRc);
	else
		m_lpLoadQueue.Add(_pRc);

#endif
//	ConOutL(CStrF("(CWorldDataLoader::AddToQueue) %d resources in pipeline.", m_lpLoadQueue.Len()));
}

void CWorldDataLoader::RemoveFromQueue(CWResource* _pRc)
{
	MAUTOSTRIP(CWorldDataLoader_RemoveFromQueue, MAUTOSTRIP_VOID);
#ifdef WRESOURCE_ASYNC_IO

	M_LOCK(*(MRTC_GOM()->m_pGlobalLock));

	if (m_lpLoadQueue.Len() && m_lpLoadQueue[0] == _pRc)
	{
		while(!_pRc->IsLoaded())
		{
			M_UNLOCK(*(MRTC_GOM()->m_pGlobalLock));
			MRTC_SystemInfo::OS_Sleep(5);
		}

		if(!_pRc->IsLoaded())
			Error("RemoveFromQueue", "Internal error.");
	}
	else
	{
		for(int i = 1; i < m_lpLoadQueue.Len(); i++)
		{
			if(m_lpLoadQueue[i] == _pRc)
			{
				m_lpLoadQueue.Del(i);
				return;
			}
		}
	}

#endif
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWorldDataCore
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_DYNAMIC(CWorldDataCore, CWorldData);

#include "../../MSystem/Raster/MTextureContainerXTC2.h"

spCWaveContainer_Plain CWorldDataCore::ReadWaveContainer(CStr _Name)
{
	MAUTOSTRIP(CWorldDataCore_ReadWaveContainer, NULL);
	MSCOPE(CWorldDataCore::ReadWaveContainer, WORLDDATA_WAVECONTAINERS);
	
	//	ConOutL("        " + _Name);
	spCWaveContainer_Plain spWC = MNew(CWaveContainer_Plain);
	if (!spWC) MemError("ReadWaveContainer");
	spWC->AddXWC(_Name, true);
	return spWC;
}

void CWorldDataCore::ReadTextureContainer(CStr _Name)
{
	MAUTOSTRIP(CWorldDataCore_ReadTextureContainer, MAUTOSTRIP_VOID);
	MSCOPE(CWorldDataCore::ReadTextureContainer, WORLDDATA_TEXTURECONTAINERS);
	
//	ConOutL("        " + _Name);

	spCCFile spFile = MNew(CCFile);
	spFile->OpenExt(_Name, CFILE_READ | CFILE_BINARY, NO_COMPRESSION, NORMAL_COMPRESSION, 0, 1, 16384);

	CDataFile DataFile;
	DataFile.Open(spFile, 0);
	DataFile.PushPosition();

	if (DataFile.GetNext("IMAGEDIRECTORY5"))
	{
		DataFile.PopPosition();
		spCTextureContainer_VirtualXTC2 spTC = MNew(CTextureContainer_VirtualXTC2);
		if (!spTC) MemError("ReadTextureContainer");
		spTC->Create(&DataFile);
		if (m_CachePath != "")
			spTC->SetCacheFile(m_CachePath + _Name);

		DataFile.Close();
		spFile->Close();

		spTC->PostCreate();

		m_lspTC.Add((CTextureContainer*)spTC);
	}
	else
	{
		DataFile.PopPosition();
		spCTextureContainer_VirtualXTC spTC = MNew(CTextureContainer_VirtualXTC);
		if (!spTC) 
			MemError("ReadTextureContainer");
		spTC->Create(&DataFile);
//		if (m_CachePath != "")
//			spTC->SetCacheFile(m_CachePath + _Name);

		m_lspTC.Add((CTextureContainer*)spTC);
	}
}

void CWorldDataCore::ScanWaveContainers_r(int _iContentDirectory, CStr _Path, TArray<CContainerPath> &_lContainers)
{
	MAUTOSTRIP(CWorldDataCore_ScanWaveContainers_r, MAUTOSTRIP_VOID);

	CDirectoryNode Dir;
	Dir.ReadDirectory(m_lWorldPathes[_iContentDirectory] + _Path + "*");

	int nFiles = Dir.GetFileCount();
	for(int i = 0; i < nFiles; i++)
	{
		CDir_FileRec* pRec  = Dir.GetFileRec(i);
		if (pRec->IsDirectory())
		{
			if (pRec->m_Name.Copy(0,1) != ".")
				ScanWaveContainers_r(_iContentDirectory, _Path + pRec->m_Name + "\\", _lContainers);
		}
		else
		{
			if (pRec->m_Ext.CompareNoCase("XWC") == 0)
			{
				// Make sure that:
				// 1. Containers will be scanned even if not having a version in the original content directory
				// 2. A full matching path is needed to override a file, not just the same file-name

				CStr Path = _Path + pRec->m_Name;
				int f = 0;
				for(; f < _lContainers.Len(); f++)
					if(_lContainers[f].m_Path.CompareNoCase(Path) == 0)
					{
						_lContainers[f].m_iContentDirectory = _iContentDirectory;
						break;
					}

				if(f == _lContainers.Len())
				{
					CContainerPath ConPath;
					ConPath.m_Path = Path;
					ConPath.m_iContentDirectory = _iContentDirectory;
					_lContainers.Add(ConPath);
				}
			}
		}
	}
}
		
void CWorldDataCore::ScanWaveContainers(CStr _Path)
{
	MAUTOSTRIP(CWorldDataCore_ScanWaveContainers, MAUTOSTRIP_VOID);
	MSCOPE(CWorldDataCore::ScanWaveContainers, WORLDDATA_WAVECONTAINERS);

	TArray<CContainerPath> lContainerPath;
	lContainerPath.SetGrow(50);
	for(int32 p = 0; p < m_lWorldPathes.Len(); p++)
		ScanWaveContainers_r(p, _Path, lContainerPath);

	for(int i = 0; i < lContainerPath.Len(); i++)
	{
		CStr FileName = m_lWorldPathes[lContainerPath[i].m_iContentDirectory] + lContainerPath[i].m_Path;
//		M_TRACEALWAYS("ScanWave: %s\n", FileName.Str());
		M_TRY 
		{ 
			m_lspWC.Add(ReadWaveContainer(FileName)); 
		}
		M_CATCH(
		catch(CCExceptionFile)
		{
			CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
			ConOutL("§cf80WARNING: Failure reading wavecontainer: " + FileName);
		}
		)
#ifdef M_SUPPORTSTATUSCORRUPT
		M_CATCH(
		catch(CCException)
		{
			CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
			ConOutL("§cf80WARNING: Failure reading wavecontainer: " + FileName);
		}
		)
#endif
	}
}

uint64 CWorldDataCore::FindTextureContainers_r(uint _iWorldPath, CStr _Path, TArray<CStr>& _lFileNames) const
{
	CStr BasePath = m_lWorldPathes[_iWorldPath];

	CDirectoryNode Dir;
	Dir.ReadDirectory(BasePath + _Path + "*");

	uint nFiles = Dir.GetFileCount();
	uint64 CheckSum = 5381;

	// Scan for files
	for (uint i = 0; i < nFiles; i++)
	{
		CDir_FileRec* pRec = Dir.GetFileRec(i);
		if (pRec->IsArchive())
		{
			if ((pRec->m_Ext.CompareNoCase("XTC") == 0) && 
				(pRec->m_Name.Copy(0, 2).CompareNoCase("T_") != 0))
			{
				CheckSum = CheckSum*33 + pRec->m_WriteTime;
				_lFileNames.Add(BasePath + _Path + pRec->m_Name);
			}
		}
	}

	// Scan for directories
	for (uint i = 0; i < nFiles; i++)
	{
		CDir_FileRec* pRec = Dir.GetFileRec(i);
		if (pRec->IsDirectory())
		{
			if (pRec->m_Name.Copy(0,1) != ".")
			{
				uint64 x = FindTextureContainers_r(_iWorldPath, _Path + pRec->m_Name + "\\", _lFileNames);
				CheckSum = CheckSum*33 + x;
			}
		}
	}
	return CheckSum;
}

void CWorldDataCore::ReadTextureContainers(TAP<const CStr> _lFileNames)
{
	for (uint i = 0; i < _lFileNames.Len(); i++)
	{
		const CStr& FileName = _lFileNames[i];

		M_TRY 
		{ 
			ReadTextureContainer(FileName); 
		}
		M_CATCH(
		catch(CCExceptionFile)
		{
			CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
			ConOutL("§cf80WARNING: Failure reading texture-container: " + FileName);
		}
		)
#ifdef M_SUPPORTSTATUSCORRUPT
		M_CATCH(
		catch(CCException)
		{ 
			CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
			ConOutL("§cf80WARNING: Failure reading texture-container: " + FileName);
		}
		)
#endif
	}
}

void CWorldDataCore::ScanTextureContainers(CStr _Path)
{
	MAUTOSTRIP(CWorldDataCore_ScanTextureContainers, MAUTOSTRIP_VOID);
	MSCOPE(CWorldDataCore::ScanTextureContainers, WORLDDATA_TEXTURECONTAINERS);

	for (uint iPath = 0; iPath < m_lWorldPathes.Len(); iPath++)
	{
		// Scan file tree to get filenames and checksum
		TArray<CStr> lFileNames;
		lFileNames.SetGrow(50);
		uint64 CheckSum = FindTextureContainers_r(iPath, _Path, lFileNames);

		CStr CacheFile = m_lWorldPathes[iPath] + _Path + "_TextureCache.XTSC";
		CTextureCache c;
		if ((c.ReadCheckSum(CacheFile) == CheckSum) && c.ReadCache(CacheFile))
		{
			ConOutL(CStrF("        Used texture-scan cache: %s", CacheFile.Str()));
			M_TRACE("        Used texture-scan cache: %s\n", CacheFile.Str());
			m_lspTC.Add(&c.m_lspTC);
		}
		else
		{ // Scan as usual
			uint iFirstNew = m_lspTC.Len();
			ReadTextureContainers(lFileNames);
#ifndef PLATFORM_CONSOLE
			uint nNew = m_lspTC.Len() - iFirstNew;
			if (nNew && !D_MXDFCREATE)
			{ // Write cache file
				c.m_CheckSum = CheckSum;
				c.m_lspTC.SetLen(nNew);
				for (uint iTC = 0; iTC < nNew; iTC++)
					c.m_lspTC[iTC] = m_lspTC[iFirstNew + iTC];

				ConOutL(CStrF("        Creating texture-scan cache: %s", CacheFile.Str()));
				M_TRACE("        Creating texture-scan cache: %s\n", CacheFile.Str());
				c.WriteCache(CacheFile);
			}
#endif
		}
	}
}

void CWorldDataCore::ScanVideos(CStr _Path, CTextureContainer_Video* _pTCVideo, CStr _Ext)
{
	MAUTOSTRIP(CWorldDataCore_ScanVideos, MAUTOSTRIP_VOID);
	CDirectoryNode Dir;

	for(int32 d = 0; d < m_lWorldPathes.Len(); d++)
	{
		Dir.ReadDirectory(m_lWorldPathes[d] + _Path + "*");

		for(int i = 0; i < Dir.GetFileCount(); i++)
		{
			CDir_FileRec* pRec  = Dir.GetFileRec(i);
			if (pRec->IsDirectory())
			{
				if (pRec->m_Name.Copy(0,1) != ".")
					ScanVideos(_Path + pRec->m_Name + "\\", _pTCVideo, _Ext);
			}
			else
			{
	#if defined(PLATFORM_PS2)
				if (pRec->m_Ext.CompareNoCase("PSS") == 0)
	#elif defined(PLATFORM_XBOX1)
				if (pRec->m_Ext.CompareNoCase("XMV") == 0)
	#elif defined(PLATFORM_PS3)
				if (pRec->m_Ext.CompareNoCase("M2V") == 0 || pRec->m_Ext.CompareNoCase("AVC") == 0)
//	#elif defined(PLATFORM_XENON)
//				if (false) // OGG is temporarily disabled on xbox2
	#else
//				if (pRec->m_Ext.CompareNoCase("BIK") == 0)
				if (pRec->m_Ext.CompareNoCase(_Ext) == 0)
	#endif
				{
					CStr FileName = ResolveFileName(_Path + pRec->m_Name);
					M_TRY 
					{ 					
						_pTCVideo->AddVideo(FileName); 
					}
					M_CATCH(
					catch(CCExceptionFile)
					{
						CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
						ConOutL("§cf80WARNING: Failure reading video " + FileName);
					}
					)
	#ifdef M_SUPPORTSTATUSCORRUPT
					M_CATCH(
					catch(CCException)
					{
						CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
						ConOutL("§cf80WARNING: Failure reading video " + FileName);
					}
					)
	#endif
				}
			}		
		}
	}
}


// -------------------------------------------------------------------
CWorldDataCore::CWorldDataCore()
{
	MAUTOSTRIP(CWorldDataCore_ctor, MAUTOSTRIP_VOID);
	m_AsyncDisable = 0;
	m_bCreated = 0;
	//m_Status = 0;
	m_iCreateFlagsStack = 0;
	m_lCreateFlagsStack[m_iCreateFlagsStack] = WRESOURCE_PRECACHE;

	m_XDFSize_Precache = 0;
	m_XDFSize_Server = 0;
	m_XDFSize_MaxPos = 0;

#ifdef PLATFORM_CONSOLE
	m_lspTC.SetGrow(16);
	m_lspWC.SetGrow(16);
#else
	m_lspTC.SetGrow(2048);
	m_lspWC.SetGrow(2048);
#endif

//	m_Hash.Create(0, false);
}

CWorldDataCore::~CWorldDataCore()
{
	MAUTOSTRIP(CWorldDataCore_dtor, MAUTOSTRIP_VOID);
	MSCOPE(CWorldDataCore::~CWorldDataCore, WORLDDATA);
	
	m_lspResources.Destroy();
	m_spGameReg = NULL;
	
	RemoveFromConsole();

#ifdef NEVER

	try
	{
		ConOut("Destroying resource-manager.");


		{
			LogFile("----------------------------------------------------------------------------------");
			LogFile(" WORLDDATA RESOURCES");
			LogFile("----------------------------------------------------------------------------------");
			for(int i = 0; i < m_lspResources.Len(); i++)
			{
				CWResource* pR = GetResource(i);
				if (pR)
				{
					LogFile(CStrF("%4.d, %4.d, %9.d, %s", i, pR->GetClass(), pR->m_MemUsed, (char*) pR->GetName()));
				}
				else
					LogFile(CStrF("%4.d,  -", i));
			}
			LogFile("----------------------------------------------------------------------------------");
		}

		LogFile("(CWorldDataCore::~CWorldDataCore)");
		m_lspResources.Clear();
		LogFile("(CWorldDataCore::~CWorldDataCore) Progress 1");
		m_lspTC.Clear();
		LogFile("(CWorldDataCore::~CWorldDataCore) Progress 2");
		m_lspWC.Clear();
	//	LogFile("(CWorldDataCore::~CWorldDataCore) Progress 3");
	//	m_lspFonts.Clear();
	//	m_lFontNames.Clear();
		LogFile("(CWorldDataCore::~CWorldDataCore) Done.");
	}
	catch(CCException)
	{
	}

#endif
}


//
//
//
bool CWorldDataCore::AddWorldPath(CStr _Path)
{
	m_lWorldPathes.Add(_Path);
	return true;
}

//
//
//
bool CWorldDataCore::RemoveWorldPath(CStr _Path)
{
	for(int32 i = 0; i < m_lWorldPathes.Len(); i++)
		if(m_lWorldPathes[i] == _Path)
		{
			m_lWorldPathes.Del(i);
			return true;
		}
	return false;
}


void CWorldDataCore::Create(spCRegistry _spGameReg, int _Flags)
{
	MAUTOSTRIP(CWorldDataCore_Create, MAUTOSTRIP_VOID);
	MSCOPE(CWorldDataCore::Create, WORLDDATA);

//	m_TimeLastRefresh = CMTime::GetCPU();
	m_TimeLastRefresh.Snapshot();
	m_TimeLastHibernate = m_TimeLastRefresh;
	m_TouchTime = m_TimeLastRefresh;
	m_iLastHibernate = 0;

	// We can have pretty aggressive grow since it's just pointers in the array.
	{
		M_LOCK(m_WDataLock);
		m_lspResources.SetGrow(2048);
		m_lspResources.Add(spCWResource(NULL));		// Reserve index 0
	}

	m_Loader.Create();

	if (m_bCreated & 4)
		Error("Create", "Already created.");
	m_bCreated |= 4;

	AddToConsole();

	m_spGameReg = _spGameReg;

	// Add all "native" resource classes. These must be added in
	// the same order as the WRESOURCE_CLASS_xxxx enums in WDataRes_Core.h
	// or the resource type-checking in WMapData will break down.
	// (I could check the prefix instead, but I can always claim
	// it's faster to check the class-index instead. ;) )
	m_lClasses.Clear();
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_DLL", "DLL"));
#ifndef MDISABLE_CLIENT_SERVER_RES
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_Class", "CLS"));
#endif
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_Model_XW", "BSP"));
//#if defined(PLATFORM_WIN_PC) || defined(PLATFORM_DOLPHIN) || defined(PLATFORM_PS2)
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_Model_XMD", "XMD"));
//#else
//	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_Model_XMD", "XMD", 10.0f, 0.5f));		// 10s hibernate, 0.5s unload timeouts
//#endif
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_Model_Custom", "CTM"));
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_Model_Custom_File", "CMF"));
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_Anim", "ANM"));
	m_lClasses.Add(CWD_ResourceClassDesc("__CWRes_XTC (Dont use)", "XTC"));
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_Sound", "SND"));
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_Wave", "WAV"));
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_XWIndex", "XWI"));
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_Surface", "SUR"));
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_Registry", "XRG"));
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_XFC", "XFC"));
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_XWResource", "XWD"));
#ifndef MDISABLE_CLIENT_SERVER_RES
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_Template", "TPL"));
#endif
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_XWNavGrid", "XWG"));
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_Model_XW2", "XW2"));
#ifndef M_DISABLE_TODELETE
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_AnimList", "ANL"));
#endif
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_Dialogue", "DLG"));
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_XWNavGraph", "XNG"));
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_ObjectAttribs", "ATR"));
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_FacialSetup", "FAC"));
	
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_AnimGraph2", "XAH"));

	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_Model_XW3", "XW3"));
	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_Model_XW4", "XW4"));

	m_lClasses.Add(CWD_ResourceClassDesc("CWRes_Model_Glass", "XGL"));

	if (m_lClasses.Len() != WRESOURCE_NUMPREFIXES)
		Error("Create", "Internal error.");

	// Get world-path from registry
	{
		CRegistry* pR = _spGameReg->Find("WORLDPATH");
		if (!pR) Error("Create", "No WORLDPATH key in registry.");

		m_lWorldPathes.Clear();
		CStr Pathes = pR->GetThisValue();
		while(Pathes != "")
		{
			CStr Path = Pathes.GetStrSep(";");
			ConOutL("(CWorldDataCore::Create) WorldPath: " + Path);
			AddWorldPath(Path);
		}

		if (!m_lWorldPathes.Len())
			Error("Create", "WORLDPATH did not contain a path.");
	}

/*	m_CachePath = _spGameReg->GetValue("CACHEPATH");
	if (m_CachePath != "")
	{
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if (!pSys) Error("Create", "No system.");
		if (m_CachePath.GetDevice() == "")
			m_CachePath = pSys->m_ExePath + m_CachePath;
		ConOutL("(CWorldDataCore::Create) WorldPath: " + m_CachePath);
		AddWorldPath(m_CachePath);
	}*/

	ConOutL("§C484Content data path priority:");
	for(int i = 0; i < m_lWorldPathes.Len(); i++)
		ConOutL("§C484" + m_lWorldPathes[i]);

	// Scan stuff recursively.
	if(_Flags & FLAGS_SCANTEXTURES)
	{
		MSCOPE(ScanTextures, WORLDDATA_TEXTURECONTAINERS);
		ConOutL("(CWorldDataCore::Create) ScanTextureContainers...");
		M_TRACE("(CWorldDataCore::Create) ScanTextureContainers...\n");

		CMTime tm; 
		TStart(tm);
		ScanTextureContainers(CStr());
//		for(int i = m_lWorldPathes.Len() - 1; i >= 0; i--)
//			ScanTextureContainers("");
		ConOutL(CStrF("        %d texture containers loaded.", m_lspTC.Len()));
		M_TRACE("        %d texture containers loaded.\n", m_lspTC.Len());
		
		// Figure out how many textures we scanned in a pretty ugly way.
		MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
		if (pTC)
		{
			int ID = pTC->AllocID(0,0,(const char *)NULL);
			if (ID > 0)
			{
				TStop(tm);
				ConOutL(CStrF("        %d textures scanned (%.1f seconds).", ID, tm.GetTime()));
				M_TRACE("        %d textures scanned (%.1f seconds).\n", ID, tm.GetTime());
				pTC->FreeID(ID);
			}
		}
	}

	if(_Flags & FLAGS_SCANWAVES)
	{
		MSCOPE(ScanWaves, WORLDDATA_WAVECONTAINERS);

		ConOutL("(CWorldDataCore::Create) ScanWaveContainers...");
		M_TRACE("(CWorldDataCore::Create) ScanWaveContainers...\n");

		CMTime tm; 
		TStart(tm);
		CStr WavesDir = "Waves";
#ifdef PLATFORM_WIN_PC

		switch (D_MPLATFORM)
		{
		case 0:
			WavesDir = "Waves";
			break;
		case 1:
			WavesDir = "Waves_XBox";
			break;
		case 2:
			WavesDir = "Waves_PS2";
			break;
		case 3:
			WavesDir = "Waves_GC";
			break;
		case 4:
			WavesDir = "Waves_Xenon";
			break;
		case 5:
			WavesDir = "Waves_PS3";
			break;
		}
#else
	#if 0
	#elif defined(PLATFORM_XENON)
			WavesDir = "Waves_Xenon";
	#elif defined(PLATFORM_XBOX1)
			WavesDir = "Waves_XBox";
	#elif defined( PLATFORM_PS2 )
			WavesDir = "Waves_PS2";
	#elif defined( PLATFORM_PS3 )
			WavesDir = "Waves_PS3";
	#else
			WavesDir = "Waves";
	#endif
#endif

		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if (pSys && pSys->GetEnvironment())
			WavesDir = pSys->GetEnvironment()->GetValue("SND_SCANDIR", WavesDir);

		ScanWaveContainers(WavesDir + "\\");

//		for(int i = m_lWorldPathes.Len() - 1; i >= 0; i--)
//			ScanWaveContainers("");
		ConOutL(CStrF("        %d wave containers loaded.", m_lspWC.Len()));
		M_TRACE("        %d wave containers loaded.\n", m_lspWC.Len());

		// Figure out how many waves we scanned in a pretty ugly way.
		MACRO_GetRegisterObject(CWaveContext, pWC, "SYSTEM.WAVECONTEXT");
		if (pWC)
		{
			int ID = pWC->AllocID(0,0);
			if (ID > 0)
			{
				TStop(tm);
				ConOutL(CStrF("        %d waves scanned (%.1f seconds).", ID, tm.GetTime()));
				M_TRACE("        %d waves scanned (%.1f seconds).\n", ID, tm.GetTime());
				pWC->FreeID(ID);
			}
		}
	}
	if(_Flags & FLAGS_SCANVIDEOS)
	{
		MSCOPE(ScanVideos, WORLDDATA_VIDEOS);

		ConOutL("(CWorldDataCore::Create) ScanVideos...");
		M_TRACE("(CWorldDataCore::Create) ScanVideos...\n");

		CMTime tm; 
		TStart(tm);

#if defined(PLATFORM_PS2)
		spCReferenceCount spObj = MRTC_GOM()->GetClassRegistry()->CreateObject("CTextureContainer_Video_Mpeg");
		m_spTCVideo = safe_cast<CTextureContainer_Video>((CReferenceCount*)spObj);
		if (!m_spTCVideo) ConOutL("§cf80WARNING: (CRC_ConsoleRender::-) Could not create CTextureContainer_Video_Mpeg");
#elif defined(PLATFORM_XBOX1)
		spCReferenceCount spObj = MRTC_GOM()->GetClassRegistry()->CreateObject("CTextureContainer_Video_XMV");
		m_spTCVideo = safe_cast<CTextureContainer_Video>((CReferenceCount*)spObj);
		if (!m_spTCVideo) ConOutL("§cf80WARNING: (CRC_ConsoleRender::-) Could not create CTextureContainer_Video_XMV");
#elif defined(PLATFORM_PS3)
		spCReferenceCount spObj = MRTC_GOM()->GetClassRegistry()->CreateObject("CTextureContainer_Video_PS3");
		m_spTCVideo = safe_cast<CTextureContainer_Video>((CReferenceCount*)spObj);
		if (!m_spTCVideo) ConOutL("§cf80WARNING: (CRC_ConsoleRender::-) Could not create CTextureContainer_Video_PS3");
#else
		spCReferenceCount spObj = MRTC_GOM()->GetClassRegistry()->CreateObject("CTextureContainer_Video_Theora");
		m_spTCVideo = safe_cast<CTextureContainer_Video>((CReferenceCount*)spObj);
		if (!m_spTCVideo) ConOutL("§cf80WARNING: (CRC_ConsoleRender::-) Could not create CTextureContainer_Video_Theora");
//		spCReferenceCount spObj = MRTC_GOM()->GetClassRegistry()->CreateObject("CTextureContainer_Video_Bink");
//		m_spTCVideo = safe_cast<CTextureContainer_Video>((CReferenceCount*)spObj);
//		if (!m_spTCVideo) ConOutL("§cf80WARNING: (CRC_ConsoleRender::-) Could not create CTextureContainer_Video_Bink");

#if defined(PLATFORM_XENON)
		spObj = MRTC_GOM()->GetClassRegistry()->CreateObject("CTextureContainer_Video_WMV");
		m_spTCSecVideo = safe_cast<CTextureContainer_Video>((CReferenceCount*)spObj);
		if(!m_spTCSecVideo) ConOutL("§cf80WARNING: (CRC_ConsoleRender::-) Could not create CTextureContainer_Video_WMV");
#endif

#endif

		if (m_spTCVideo != NULL)
		{
			M_TRY
			{
				// NOTE: No path resolving is performed, only first game path is scanned.
				m_spTCVideo->Create(NULL);
				ScanVideos(CStr("Videos\\"), m_spTCVideo,"OGG");	// Extension argument only used for Xenon/Windows
				uint32 nLocal =  m_spTCVideo->GetNumLocal();

#if defined(PLATFORM_XENON)
				if( m_spTCSecVideo )
				{
					m_spTCSecVideo->Create(NULL);
					ScanVideos(CStr("Videos\\"), m_spTCSecVideo,"WMV");
					nLocal += m_spTCSecVideo->GetNumLocal();
				}
#endif

				TStop(tm);
				ConOutL(CStrF("        %d videos scanned (%.1f seconds).", nLocal, tm.GetTime()));
				M_TRACE("        %d videos scanned (%.1f seconds).\n", nLocal, tm.GetTime());
			}
			M_CATCH(
			catch(CCExceptionFile)
			{
				CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
				m_spTCVideo = NULL;
				m_spTCSecVideo = NULL;
			}
			)
#ifdef M_SUPPORTSTATUSCORRUPT
			M_CATCH(
			catch(CCException)
			{
				CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
				m_spTCVideo = NULL;
				m_spTCSecVideo = NULL;
			}
			)
#endif

		}
	}

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys)
		Error("Execute", "No system");
	
	CRegistry* pEnv = pSys->GetEnvironment();
	if(!pEnv)
		Error("Execute", "No env");

	if(!pEnv->GetValuei("XWC_DONTSCANSURFACES", 0, 0) && _Flags & FLAGS_SCANSURFACES)
	{
		MSCOPE(ScanSurfaces, WORLDDATA_SURFACES);
		// NOTE: Surfaces should be scanned last so that texture IDs can be properly initialized.
		CMTime tm;
		TStart(tm);

		ConOutL("(CWorldDataCore::Create) ScanSurfaces...");
		M_TRACE("(CWorldDataCore::Create) ScanSurfaces...\n");
		MACRO_GetRegisterObject(CXR_SurfaceContext, pSurfContext, "SYSTEM.SURFACECONTEXT");
		
		if (pSurfContext)
		{
			// Clear all loaded surfaces first.
			pSurfContext->Create();

			for(int i = m_lWorldPathes.Len() - 1; i >= 0; i--)
				pSurfContext->AddDirectory(m_lWorldPathes[i]+"Surfaces\\");

			TStop(tm);
			ConOutL(CStrF("        %d surfaces loaded (%.1f seconds).", pSurfContext->GetNumSurfaces(), tm.GetTime()));
			M_TRACE("        %d surfaces loaded (%.1f seconds).\n", pSurfContext->GetNumSurfaces(), tm.GetTime());
		}
	}

	// Any resources to be force-loaded?
	{
//		MSCOPE(ForceLoadResources, WORLDDATA);

		CRegistry* pR = _spGameReg->Find("LOADRESOURCES");
		if (pR)
		{
			CStr Rc = pR->GetThisValue();
			while(Rc != "")
			{
				int iRc = GetResourceIndex(Rc.GetStrSep(";"), NULL);
				spCWResource spRc = GetResourceRef(iRc);
				if (spRc)
					m_lspResourceForceIncludeRef.Add(spRc);
			}
		}
	}
}

void CWorldDataCore::OnRefresh()
{
	MAUTOSTRIP(CWorldDataCore_OnRefresh, MAUTOSTRIP_VOID);
	MSCOPE(CWorldDataCore::OnRefresh, WORLDDATA);

//	m_TimeLastRefresh = CMTime::GetCPU();
	m_TimeLastRefresh.Snapshot();
	m_TouchTime = m_TimeLastRefresh;

	//Resource_AutoHibernate();
#ifdef PLATFORM_XBOX1
	if (!m_AsyncDisable)
	{
		Resource_AsyncCacheRefresh();
	}
#endif
}
/*
int CWorldDataCore::GetStatus()
{
	MAUTOSTRIP(CWorldDataCore_GetStatus, 0);
	return m_Status;
};

void CWorldDataCore::SetStatus(int _Status)
{
	MAUTOSTRIP(CWorldDataCore_SetStatus, MAUTOSTRIP_VOID);
	m_Status |= _Status;
}

void CWorldDataCore::ClearStatus(int _Status)
{
	MAUTOSTRIP(CWorldDataCore_ClearStatus, MAUTOSTRIP_VOID);
	m_Status &= ~_Status;
}
*/
class CAsyncCopy : public CReferenceCount, public MRTC_Thread
{
private:
	CFileAsyncCopy m_FileCopy;
//	int m_CurrentCategory;
	volatile int m_CurrentMode;
//	int m_nCategories;
//	int m_CurrentFile;
//	int m_CurrentNumFiles;
	class CCategory
	{
	public:
		CCategory()
		{
			m_spFileHash = MNew(CStringHashConst);
		}
		~CCategory()
		{
			m_spFileHash->Clear();
		}

		CCategory &operator = (const CCategory &_Src)
		{
			m_spFileHash= _Src.m_spFileHash;
			m_lFiles = _Src.m_lFiles;
			return *this;
		}

		spCStringHashConst m_spFileHash;
		TArray<CStr> m_lFiles;
	};

	TArray<CCategory> m_lCategories;
	volatile int m_iCurrentCategory;
	volatile int m_iCurrentCategoryPrio;
	int m_iCurrentFile;

//	CRegistry *m_pCurrentFile;
//	CRegistry *m_pCurrentDir;
//	CRegistry *m_pRegistry;
	CFileInfo m_InfSrc;
	CStr m_CurrentDestFile;
	CStr m_BaseDestPath;
	bool m_bPending;
	volatile bool m_bDone;
	bool m_bError;

	int m_iDisableCount;

	CStr m_ExePath;

	CStr m_PrioCopy;

	class CPrioCopyData
	{
	public:
		CStr sFileName;
		CStr Src;
		CStr File;
		CFileInfo FileInfo;
		int Mode;
		CPrioCopyData()
		{
			Mode = 0;
			pCopy = NULL;
		}

		~CPrioCopyData()
		{
			if (pCopy)
				delete pCopy;
		}

		int iPrio;
		CFileAsyncCopy *pCopy;
	};

	CPrioCopyData m_PrioCopyData;

	const char* Thread_GetName() const
	{
		return "WDataCore Async copy";
	}

	MRTC_CriticalSection m_Lock;
	int Thread_Main()
	{
		while (!Thread_IsTerminating())
		{
			M_TRY
			{
				CStr PrioCopy;

				{
					M_LOCK(m_Lock);
					PrioCopy.Capture(m_PrioCopy.Str());
				}

				if (PrioCopy.Len())
				{
					if (Refresh(m_PrioCopy))
					{
						M_LOCK(m_Lock);
						m_PrioCopy.Clear();
					}
				}
				else
				{
					Refresh(CStr());
				}

				if (m_bDone)
					return 0;

				int Mode;
				{
					M_LOCK(m_Lock);
					Mode = m_CurrentMode;
				}

				if (Mode < 1)
					MRTC_SystemInfo::OS_Sleep(10);
				else
					MRTC_SystemInfo::OS_Sleep(100);
			}
			M_CATCH(
			catch(CCExceptionFile)
			{
				m_bDone = true;
				CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
				ConOutL("§cf80WARNING: Failure reading wavecontainer");
				return 0;
			}
			)
#ifdef M_SUPPORTSTATUSCORRUPT
			M_CATCH(
			catch(CCException)
			{
				m_bDone = true;
				CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
				ConOutL("§cf80WARNING: Failure reading wavecontainer");
				return 0;
			}
			)
#endif
		}

		return 0;
	}

	bool Refresh(CStr _PrioCopy)
	{
		M_TRY
		{
			for (int i = 0; i < 2;++i)
			{			
				// Everything is already copied
				{
					M_LOCK(m_Lock);
					if (this->m_iCurrentCategory >= this->m_lCategories.Len())
					{
						if (this->m_CurrentMode > 0)
						{
							if (!this->m_bPending)
							{
								if (m_PrioCopyData.sFileName.Len())
								{
									m_PrioCopyData.sFileName.Clear();
									m_PrioCopyData.Src.Clear();
									m_PrioCopyData.File.Clear();
									if (m_PrioCopyData.pCopy)
									{
										this->m_FileCopy.Resume();
										delete m_PrioCopyData.pCopy;
										m_PrioCopyData.pCopy = NULL;
									}
								}
								m_bDone = true;
								return true;
							}
						}
						else
						{
							this->m_iCurrentCategory = 0;
							++this->m_CurrentMode;
							CDiskUtil::SearchPath_Add(this->m_BaseDestPath);
						}
					}
				}

				if (m_PrioCopyData.sFileName.Len() || _PrioCopy.Len() && this->m_CurrentMode > 0)
				{

					if (!m_PrioCopyData.sFileName.Len())
					{
						m_PrioCopyData.sFileName = _PrioCopy;
						m_PrioCopyData.Src = m_ExePath + _PrioCopy;
						m_PrioCopyData.File = this->m_BaseDestPath + _PrioCopy;
						m_PrioCopyData.Mode = 0;
						m_PrioCopyData.iPrio = this->m_lCategories[this->m_iCurrentCategoryPrio].m_spFileHash->GetIndex(_PrioCopy);
					}

					if (m_PrioCopyData.Mode == 0)
					{
						if (m_PrioCopyData.iPrio < 0)
						{
							m_PrioCopyData.sFileName.Clear();
							m_PrioCopyData.Src.Clear();
							m_PrioCopyData.File.Clear();
							return true;
						}

						if (m_PrioCopyData.iPrio < this->m_iCurrentFile)
						{
							m_PrioCopyData.sFileName.Clear();
							m_PrioCopyData.Src.Clear();
							m_PrioCopyData.File.Clear();
							return true;
						}

						if (m_PrioCopyData.iPrio != this->m_iCurrentFile)
						{
							if (!CDiskUtil::FileExists(m_PrioCopyData.Src))
							{
								CDiskUtil::DelFile(m_PrioCopyData.File);
								{
									m_PrioCopyData.sFileName.Clear();
									m_PrioCopyData.Src.Clear();
									m_PrioCopyData.File.Clear();
									return true;
								}
							}

							m_PrioCopyData.FileInfo = CDiskUtil::FileTimeGet(m_PrioCopyData.Src);
							if (CDiskUtil::FileExists(m_PrioCopyData.File))
							{
								CFileInfo InfDest = CDiskUtil::FileTimeGet(m_PrioCopyData.File);

								if (!m_PrioCopyData.FileInfo.AlmostEqual(InfDest)) 
									CDiskUtil::DelFile(m_PrioCopyData.File);
								else
								{
									m_PrioCopyData.sFileName.Clear();
									m_PrioCopyData.Src.Clear();
									m_PrioCopyData.File.Clear();
									return true;
								}
							}

							this->m_FileCopy.Pause();
							m_PrioCopyData.Mode = 1;

							CDiskUtil::CreatePath(m_PrioCopyData.File.GetPath());

							{
								if (m_PrioCopyData.pCopy)
									delete m_PrioCopyData.pCopy;
								m_PrioCopyData.pCopy = DNew(CFileAsyncCopy) CFileAsyncCopy;
								m_PrioCopyData.pCopy->CpyFile(m_PrioCopyData.Src, m_PrioCopyData.File + ".tmp", 128*1024);
							}
						}
					}
					else
					{
						if (m_PrioCopyData.pCopy->Done())
						{
							CDiskUtil::RenameFile(m_PrioCopyData.File + ".tmp", m_PrioCopyData.File);					

							M_TRY
							{
								CDiskUtil::FileTimeSet(m_PrioCopyData.File, m_PrioCopyData.FileInfo);
							}
							M_CATCH(
							catch (CCException)
							{
							}
							)
		                    
							m_PrioCopyData.sFileName.Clear();
							m_PrioCopyData.Src.Clear();
							m_PrioCopyData.File.Clear();
							this->m_FileCopy.Resume();
							delete m_PrioCopyData.pCopy;
							m_PrioCopyData.pCopy = NULL;
							return true;
						}

						return false;
					}
				}

				// Check if current is done
				if (this->m_bPending)
				{
					if (this->m_FileCopy.Done())
					{				
						CDiskUtil::RenameFile(this->m_CurrentDestFile + ".tmp", this->m_CurrentDestFile);					

						M_TRY
						{
							CDiskUtil::FileTimeSet(this->m_CurrentDestFile, this->m_InfSrc);
						}
						M_CATCH(
						catch (CCException)
						{
						}
						)
						
						this->m_bPending = false;
						continue;
					}
					else
						return false;
				}


				if (this->m_lCategories[this->m_iCurrentCategory].m_lFiles.Len() == 0)
				{
					{
						M_LOCK(m_Lock);
						++this->m_iCurrentCategory;
					}
					continue;
				}

				CStr File = this->m_lCategories[this->m_iCurrentCategory].m_lFiles[this->m_iCurrentFile];
				this->m_CurrentDestFile = this->m_BaseDestPath + File;
				
				CStr Src = m_ExePath + File;
				
				bool CopyFile = true;
				
				if (this->m_CurrentMode < 1)
				{
					CopyFile = false;
					if (CDiskUtil::FileExists(this->m_CurrentDestFile))
					{
						if (CDiskUtil::FileExists(Src))
						{
							this->m_InfSrc = CDiskUtil::FileTimeGet(Src);
							CFileInfo InfDest = CDiskUtil::FileTimeGet(this->m_CurrentDestFile);

							/*
	#ifdef PLATFORM_XBOX
							SYSTEMTIME Time1;
							SYSTEMTIME Time2;
							
							FileTimeToSystemTime((FILETIME *)&this->m_InfSrc.m_TimeWrite, &Time1);
							FileTimeToSystemTime((FILETIME *)&InfDest.m_TimeWrite, &Time2);
	#endif*/
							
							// Already on disk
							if (!this->m_InfSrc.AlmostEqual(InfDest))  
							{
								CDiskUtil::DelFile(this->m_CurrentDestFile);
								M_TRACEALWAYS("Deleted file that does not have same date %s\n", this->m_CurrentDestFile.Str());
							}
						}
						else
						{
							CDiskUtil::DelFile(this->m_CurrentDestFile);
							M_TRACEALWAYS("Deleted file that does does not exist in source %s\n", this->m_CurrentDestFile.Str());
						}
					}
				}
				else
				{					
					if (CDiskUtil::FileExists(this->m_CurrentDestFile) || !CDiskUtil::FileExists(Src))
					{
						CopyFile = false;
					}
				}
				
				if (CopyFile)
				{
					//M_TRACEALWAYS("Copying file %s\n", this->m_CurrentDestFile.Str());
					this->m_bPending = true;
					this->m_InfSrc = CDiskUtil::FileTimeGet(Src);
					
					CDiskUtil::CreatePath(this->m_CurrentDestFile.GetPath());
					
					this->m_FileCopy.CpyFile(Src, this->m_CurrentDestFile + ".tmp", 128*1024);
				}
				
				++this->m_iCurrentFile;
				if (this->m_iCurrentFile >= this->m_lCategories[this->m_iCurrentCategory].m_lFiles.Len())
				{
					++this->m_iCurrentCategory;
					this->m_iCurrentFile = 0;
				}
			}
		}
		M_CATCH(
		catch (CCExceptionFile)
		{
			m_bDone = true;
			CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
		}
		)
#ifdef M_SUPPORTSTATUSCORRUPT
		M_CATCH(
		catch (CCException)
		{
			m_bDone = true;
			CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);		
		}
		)
#endif
        return false;
	}
public:
	CAsyncCopy()
	{
		m_bDone = false;
		m_iCurrentCategory = 0;
		m_iCurrentCategoryPrio = 0;
		m_iCurrentFile = 0;
		m_iDisableCount = 0;

		m_CurrentMode = 0;
		m_bPending = false;
//		m_pRegistry = NULL;
//		m_pCurrentDir = NULL;
	}
	~CAsyncCopy()
	{
		Thread_Destroy();
	}

	void Init(CRegistry *_pRec)
	{
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");		

		this->m_ExePath = pSys->m_ExePath;
		
		CRegistry *pReg = _pRec;
		this->m_lCategories.SetLen(pReg->GetValuei("NumCategories"));
		this->m_BaseDestPath = pReg->GetValue("Destination");


		for (int i = 0; i < this->m_lCategories.Len(); ++i)
		{
			CRegistry *pFReg = pReg->FindChild(CFStrF("Files_%d", i));

			if (pFReg)
			{
				for (int f = 0; f < pFReg->GetNumChildren(); ++f)
				{
					CDirectoryNode Dir;

					CStr Search = pSys->m_ExePath + pFReg->GetChild(f)->GetThisValue();
					Dir.ReadDirectory(Search);

					for (int j = 0; j < Dir.GetFileCount(); ++j)
					{
						if (!Dir.IsDirectory(j))
						{
							CStr FileName = Search.GetPath() + Dir.GetFileName(j);
							this->m_lCategories[i].m_lFiles.Add(FileName.Right(FileName.Len() - pSys->m_ExePath.Len()));
						}
					}

				}

				int nFiles = this->m_lCategories[i].m_lFiles.Len();
				this->m_lCategories[i].m_spFileHash->Create(nFiles, false);
				for (int j = 0; j < nFiles; ++j)
				{
					this->m_lCategories[i].m_spFileHash->Insert(j, this->m_lCategories[i].m_lFiles[j]);
				}
				
			}
		}
		
		CDiskUtil::SearchPath_SetBase(pSys->m_ExePath);

		Thread_Create();
	}

	bool CategoryDone(int _iCategory)
	{
		M_LOCK(m_Lock);

		if (m_bDone)
			return true;
		if (m_CurrentMode < 1)
			return false;

		if (m_iCurrentCategory == (_iCategory + 1) && m_iCurrentFile == 0)
		{
			return !m_bPending;
		}
		else
			return m_iCurrentCategory > _iCategory;
	}

	void Pause(bool _bForce)
	{
		if (_bForce)
		{
			m_FileCopy.Pause();
		}
		else
		{
			if ((++m_iDisableCount) == 1)
			{
				m_FileCopy.Pause();
			}
		}
	}

	void Resume(bool _bForce)
	{
		if (_bForce)
		{
			m_FileCopy.Resume();
		}
		else
		{
			if ((--m_iDisableCount) == 0)
			{
				m_FileCopy.Resume();
			}
		}
	}

	void SetPrio(int _iCategory, CStr _Prio)
	{
		M_LOCK(m_Lock);

		if (m_bDone)
			return;

		while (m_CurrentMode < 1)
		{
			M_UNLOCK(m_Lock);
		}

		while (m_iCurrentCategory < _iCategory)
		{
			M_UNLOCK(m_Lock);
		}

		if (m_iCurrentCategory == (_iCategory + 1) && m_iCurrentFile == 0)
		{
			if (!m_bPending)
				return;
		}
		else if (m_iCurrentCategory > _iCategory)
			return;

		m_iCurrentCategoryPrio = _iCategory;

		m_PrioCopy.Capture(_Prio.Str());
	}

	bool PrioDone()
	{
		M_LOCK(m_Lock);
		return !m_PrioCopy.Len();
	}

	bool IsDone()
	{
		M_LOCK(m_Lock);
		return m_bDone;
	}

};

uint32 CWorldDataCore::Resource_AsyncCacheGetEnabled()
{
	return m_AsyncDisable;
}

void CWorldDataCore::Resource_AsyncCacheEnable(uint32 _Mask)
{
	MAUTOSTRIP(CWorldDataCore_Resource_AsyncCacheEnable, MAUTOSTRIP_VOID);

	if (_Mask == 0x70000000 && m_spAsyncCopyContext)
		((CAsyncCopy *)((CReferenceCount *)m_spAsyncCopyContext))->Resume(true);
	else
	{
		m_AsyncDisable &= ~_Mask;

		if (m_spAsyncCopyContext)
		{
			((CAsyncCopy *)((CReferenceCount *)m_spAsyncCopyContext))->Resume(false);
		}
	}
}

void CWorldDataCore::Resource_AsyncCacheDisable(uint32 _Mask)
{
	MAUTOSTRIP(CWorldDataCore_Resource_AsyncCacheDisable, MAUTOSTRIP_VOID);
	if (_Mask == 0x70000000 && m_spAsyncCopyContext)
		((CAsyncCopy *)((CReferenceCount *)m_spAsyncCopyContext))->Pause(true);
	else
	{
		m_AsyncDisable |= _Mask;

		if (m_spAsyncCopyContext)
		{
			((CAsyncCopy *)((CReferenceCount *)m_spAsyncCopyContext))->Pause(false);
		}
	}
}

bool CWorldDataCore::Resource_AsyncCacheRefresh(CStr _PrioCopy)
{
	MAUTOSTRIP(CWorldDataCore_Resource_AsyncCacheRefresh, true);
	if (m_AsyncDisable & 1)
		return true;

	CRegistry *pReg = m_spGameReg->FindChild("AsyncCache");
	if (pReg)
	{
		
		CAsyncCopy *pAsyncCopy = (CAsyncCopy *)(CReferenceCount *)m_spAsyncCopyContext;
		if (!pAsyncCopy)
		{
			m_spAsyncCopyContext = MNew(CAsyncCopy);
			CAsyncCopy *pAsyncCopy = (CAsyncCopy *)(CReferenceCount *)m_spAsyncCopyContext;
			pAsyncCopy->Init(pReg);
		}	
		else
		{
			if (pAsyncCopy->IsDone())
			{
				m_AsyncDisable |= 1;
				m_spAsyncCopyContext = NULL;
			}

		}
	}
	else
		m_AsyncDisable |= 1;

	return true;

}

void CWorldDataCore::Resource_AsyncCacheBlockOnFile(int _Category, CStr _FileName)
{
#ifdef PLATFORM_XBOX1
	Resource_AsyncCacheBlockOnCategory(_Category - 1);

	Resource_AsyncCacheRefresh();

	if (m_AsyncDisable & 1)
		return;

	CAsyncCopy *pAsyncCopy = (CAsyncCopy *)((CReferenceCount *)m_spAsyncCopyContext);

	pAsyncCopy->SetPrio(_Category, _FileName);

	while (pAsyncCopy->PrioDone()) 
		MRTC_SystemInfo::OS_Sleep(10);

#else
#endif
}

void CWorldDataCore::Resource_AsyncCacheBlockOnCategory(int _Category)
{
	MAUTOSTRIP(CWorldDataCore_Resource_AsyncCacheBlockOnCategory, MAUTOSTRIP_VOID);
#ifdef PLATFORM_XBOX1
	Resource_AsyncCacheRefresh();

	if (m_AsyncDisable & 1)
		return;

	CAsyncCopy *pAsyncCopy = (CAsyncCopy *)((CReferenceCount *)m_spAsyncCopyContext);
	while (!pAsyncCopy->CategoryDone(_Category))
	{
		MRTC_SystemInfo::OS_Sleep(1);
	}
/*	while (pAsyncCopy->m_CurrentMode < 1 || pAsyncCopy->m_iCurrentCategory <= (_Category))
	{
		Resource_AsyncCacheRefresh();
		MRTC_SystemInfo::OS_Sleep(1);
	}*/
#else
#endif
}

bool CWorldDataCore::Resource_AsyncCacheCategoryDone(int _Category)
{
	MAUTOSTRIP(CWorldDataCore_Resource_AsyncCacheCategoryDone, false);
#ifdef PLATFORM_XBOX1
	Resource_AsyncCacheRefresh();

	if (m_AsyncDisable & 1)
		return true;

	CAsyncCopy *pAsyncCopy = (CAsyncCopy *)((CReferenceCount *)m_spAsyncCopyContext);
	return pAsyncCopy->CategoryDone(_Category);
#else 
	return true;
#endif
}

bool CWorldDataCore::Resource_AsyncCacheFileDone(int _Category, CStr _FileName)
{
#ifdef PLATFORM_XBOX1

	if (_Category >= 0)
		if (!Resource_AsyncCacheCategoryDone(_Category-1))
			return false;

	Resource_AsyncCacheRefresh();

	if (m_AsyncDisable & 1)
		return true;

	CAsyncCopy *pAsyncCopy = (CAsyncCopy *)((CReferenceCount *)m_spAsyncCopyContext);

	if (_Category >= 0)
	{
		pAsyncCopy->SetPrio(_Category, _FileName);
		return pAsyncCopy->PrioDone();
	}
	else
		return pAsyncCopy->PrioDone();
#else 
	return true;
#endif
}

CStr CWorldData::ResolveFileName(CStr *_pPaths, int _nPaths, CStr _FileName)
{
	CStr TestPath;
	for(int i = _nPaths-1; i >= 0; i--)
	{
		TestPath = _pPaths[i] + _FileName;
		if(CDiskUtil::FileExists(TestPath))
			return TestPath;
	}

	return TestPath;	// Didn't find a file. Return the lowest priority path.

}

CStr CWorldData::ResolvePath(CStr *_pPaths, int _nPaths, CStr _FileName)
{
	return ResolveFileName(_pPaths, _nPaths, _FileName).GetPath();
}

// -------------------------------------------------------------------
CStr CWorldDataCore::ResolveFileName(CStr _FileName, bool _bAllowCache)
{
	MAUTOSTRIP(CWorldDataCore_ResolveFileName, CStr());

	int nPathes = m_lWorldPathes.Len();
	if (!m_CachePath.Len())
		_bAllowCache = false;
	else if (!_bAllowCache)
		nPathes--;

	CStr Path = CWorldData::ResolveFileName(m_lWorldPathes.GetBasePtr(), nPathes, _FileName);

	if (_bAllowCache && CDiskUtil::FileExists(Path))
	{
		if (m_lWorldPathes[nPathes-1] + _FileName == Path)
		{
			M_TRY
			{
				CCFile File;
				File.Open(Path, CFILE_BINARY | CFILE_READ);
				if (File.Length() == 0)
				{
					// Broken file it seems, delete it and continue looking for the file.
					File.Close();
					ConOutL("Invalid cache copy " + Path);
					CDiskUtil::DelFile(Path);
					Path = CWorldData::ResolveFileName(m_lWorldPathes.GetBasePtr(), nPathes, _FileName);
					goto CopyFiles;
				}
			}
			M_CATCH(
			catch(CCException)
			{
				ConOutL("Failed to validate cache copy " + Path);
				Path = CWorldData::ResolveFileName(m_lWorldPathes.GetBasePtr(), nPathes, _FileName);
				goto CopyFiles;
			}
			)			
		}
		else
		{
CopyFiles:
			CStr Dest = m_CachePath + _FileName;
			CStr DestPath = Dest.GetPath();

			ConOutL(CStrF("(CWorldDataCore) Caching file %s at %s", (char*)Path, (char*)(m_CachePath + _FileName)));

			if (DestPath != "")
				CDiskUtil::CreatePath(DestPath);

			CDiskUtil::CpyFile(Path, m_CachePath + _FileName, 1024 * 512);
			return m_CachePath + _FileName;
		}
	}

	return Path;
}

CStr CWorldDataCore::ResolvePath(CStr _FileName)
{
	MAUTOSTRIP(CWorldDataCore_ResolvePath, CStr());
	return ResolveFileName(_FileName).GetPath();
}

TArray_SimpleSortable<CStr> CWorldDataCore::ResolveDirectory(CStr _Path, bool _bDirectories)
{
	TArray_SimpleSortable<CStr> lFiles;
	MAUTOSTRIP(CWorldDataCore_ResolveDirectory, lFiles);
	

	lFiles.SetGrow(256);

	for(int i = m_lWorldPathes.Len() - 1; i >= 0; i--)
	{
		TPtr<CDirectoryNode> spNode = MNew(CDirectoryNode);
		if (!spNode) return lFiles;

		spNode->ReadDirectory(m_lWorldPathes[i] + _Path);

		for(int f = 0; f < spNode->GetFileCount(); f++)
		{
			CDir_FileRec* pRec = spNode->GetFileRec(f);
			if ((pRec->IsArchive() && !_bDirectories) ||
				(!pRec->IsArchive() && _bDirectories))
			{
				// Jepp, this one should be added...
				CStr FileName = pRec->m_Name;
				if (i > 0)
				{
					// Check if file already exist and in that case overwrite it.
					int k = 0;
					for(; k < lFiles.Len(); k++)
						if (lFiles[k] == FileName)
						{
							lFiles[k] = FileName;
							break;
						}
					if (k < lFiles.Len()) continue;
				}
				lFiles.Add(FileName);
			}
		}
	}

	return lFiles;
}

CStr CWorldDataCore::GetBasePath()
{
	MAUTOSTRIP(CWorldDataCore_GetBasePath, CStr());
	return m_lWorldPathes[0];
}

// -------------------------------------------------------------------
const char* CWorldDataCore::GetResourceClassName(int _iRcClass)
{
	MAUTOSTRIP(CWorldDataCore_GetResourceClassName, NULL);
	if (!m_lClasses.ValidPos(_iRcClass))
		Error("GetResourceClassName", "Invalid resource class index.");

	return m_lClasses[_iRcClass].m_ClassName;
}

const char* CWorldDataCore::GetResourceClassPrefix(int _iRcClass)
{
	MAUTOSTRIP(CWorldDataCore_GetResourceClassPrefix, NULL);
	if (!m_lClasses.ValidPos(_iRcClass))
		Error("GetResourceClassPrefix", "Invalid resource class index.");

	return m_lClasses[_iRcClass].m_Prefix;
}

int CWorldDataCore::ResourceExists(const char* _pRcName)
{
	MAUTOSTRIP(CWorldDataCore_ResourceExists, 0);
	M_LOCK(m_WDataLock);
	if (!m_spHash) return 0;

	char TmpName[300];
	StrFixFilename((char*)_pRcName, TmpName, sizeof(TmpName));

	int iRC = m_spHash->GetIndex(TmpName);
	return (iRC >= 0) ? iRC : 0;
}

int CWorldDataCore::ResourceExistsPartial(const char* _pRcName)
{
	// Ok, go through resources, with same types (anm for example) compare names between ":"
	// if they match up return that resource

	MAUTOSTRIP(CWorldDataCore_ResourceExistsPartial, 0);
	M_LOCK(m_WDataLock);
	if (!m_spPartialNameHash) return 0;

	char TmpName[300];
	StrFixFilename((char*)_pRcName, TmpName, sizeof(TmpName));

	int iRC = m_spPartialNameHash->GetIndex(TmpName);
	return (iRC >= 0) ? iRC : 0;

	/*int32 Len = CStr::StrLen(_pRcName);
	if (Len < 4) return 0;

	// Find class type
	int32 iClassType = -1;
	Len  = m_lClasses.Len();
	for(int i = 0; i < Len; i++)
	{
		if (strnicmp(m_lClasses[i].m_Prefix, _pRcName, 3) == 0)
		{
			iClassType = i;
			break;
		}
	}
	if (iClassType == -1)
		return 0;

	// Ok got the classtype, find all resources with the same 
	Len = m_lspResources.Len();
	for (int32 i = 0; i < Len; i++)
	{
		if (m_lspResources[i] && m_lspResources[i]->GetClass() == iClassType)
		{
			// Found class of same type, extract name and see if they are the same
			CStr ResourceName = m_lspResources[i]->GetName();
			int32 StrLen = ResourceName.Len();
			// Remove everything after the second ":" from name
			const char* pStr = ResourceName.Str();
			// Prefix + : is 4
			for (int32 j = StrLen-1; j > 3; j--)
			{
				if (pStr[j] == ':')
				{
					ResourceName = ResourceName.DelFrom(j);
					break;
				}
			}
			if (ResourceName.Find(_pRcName) != -1)
				return i;
		}
	}

	return 0;*/
}

int CWorldDataCore::AddResource(spCWResource _spRes)
{
	MAUTOSTRIP(CWorldDataCore_AddResource, 0);
	MSCOPE(CWorldDataCore::AddResource, WORLDDATA);

	M_LOCK(m_WDataLock);

	TAP<spCWResource> pspResources = m_lspResources;
	for(int i = 1; i < pspResources.Len(); i++)
		if (pspResources[i] == NULL)
		{
			_spRes->m_iRc = i;
			pspResources[i] = _spRes;
			Hash_Insert(i);
			return i;
		}

	int iRc;
	{
		MSCOPESHORT(ListAdd);
		iRc = m_lspResources.Add(_spRes);
	}
	_spRes->m_iRc = iRc;
	Hash_Insert(iRc);
	return iRc;
}

void CWorldDataCore::Hash_Update()
{
	MAUTOSTRIP(CWorldDataCore_Hash_Update, MAUTOSTRIP_VOID);
	M_LOCK(m_WDataLock);
	m_spHash = NULL;
	m_spPartialNameHash = NULL;
	Hash_Insert(0);
}

void CWorldDataCore::Hash_Clear()
{
	MAUTOSTRIP(CWorldDataCore_Hash_Clear, MAUTOSTRIP_VOID);
	M_LOCK(m_WDataLock);
	m_spHash = NULL;
	m_spPartialNameHash = NULL;
}

bool GetPartialName(CStr& _Name)
{
	int32 StrLen = _Name.Len();
	// Remove everything after the second ":" from name
	const char* pStr = _Name.Str();
	// Prefix + : is 4
	for (int32 j = StrLen-1; j > 3; j--)
	{
		if (pStr[j] == ':')
		{
			_Name = _Name.DelFrom(j);
			return true;
		}
	}
	return false;
}

void CWorldDataCore::Hash_Insert(int _iRc)
{
	MAUTOSTRIP(CWorldDataCore_Hash_Insert, MAUTOSTRIP_VOID);
	MSCOPE(CWorldDataCore::Hash_Insert, WORLDDATA);

	M_LOCK(m_WDataLock);

	if (!m_lspResources.ValidPos(_iRc)) return;

	if (!m_spHash)
	{
		m_spHash = MNew(CStringHash);
		if (!m_spHash) MemError("UpdateHashTable");
	}

	if (_iRc < m_spHash->GetMaxIDs())
	{
		if (m_lspResources[_iRc] != NULL)
			m_spHash->Insert(_iRc, m_lspResources[_iRc]->GetName());
		else
			m_spHash->Remove(_iRc);
	}
	else
	{
		if (m_spHash->GetMaxIDs() < m_lspResources.Len())
		{
//			m_spHash = DNew(CStringHash) CStringHash;
			m_spHash->Create(Max(512, m_lspResources.Len()*2), false);
		}

		for(int iRc = 0; iRc < m_lspResources.Len(); iRc++)
			if (m_lspResources[iRc] != NULL)
				m_spHash->Insert(iRc, m_lspResources[iRc]->GetName());
	}

	// Remake partial hash if it doesn't match with main hash
	if (!m_spPartialNameHash)
	{
		m_spPartialNameHash = MNew(CStringHash);
		if (!m_spPartialNameHash) MemError("UpdateHashTable");
	}

	if (m_spHash->GetMaxIDs() != m_spPartialNameHash->GetMaxIDs())
	{
		m_spPartialNameHash->Create(m_spHash->GetMaxIDs(),false);
		int32 iResourceClass = -1;
		for(int i = 0; i < m_lClasses.Len(); i++)
			if (CStrBase::strnicmp(m_lClasses[i].m_Prefix, "ANM", 3) == 0)
			{
				iResourceClass = i;
				break;
			}

		// Don't insert all resources, only anm for now
		for(int iRc = 0; iRc < m_lspResources.Len(); iRc++)
			if (m_lspResources[iRc] != NULL && m_lspResources[iRc]->GetClass() == iResourceClass)
			{
				CStr ResourceName = m_lspResources[iRc]->GetName();
				GetPartialName(ResourceName);
				m_spPartialNameHash->Insert(iRc, ResourceName);
			}
	}

	// Add to secondary hash for items that have multiple ":" (anims only?)
	// First find out if we need to insert it into this hash
	// Found class of same type, extract name and see if they are the same
	if (!m_lspResources[_iRc])
		return;

	CStr ResourceName = m_lspResources[_iRc]->GetName();
	if (CStrBase::strnicmp("ANM", ResourceName.GetStr(), 3) != 0)
		return;

	GetPartialName(ResourceName);
	if (m_lspResources[_iRc] != NULL)
		m_spPartialNameHash->Insert(_iRc, ResourceName);
	else
		m_spPartialNameHash->Remove(_iRc);
}


int CWorldDataCore::GetResourceIndex(const char* _pRcName, int _RcClass, CMapData* _pMapData)
{
	MAUTOSTRIP(CWorldDataCore_GetResourceIndex, 0);
//	MSCOPE(CWorldDataCore::GetResourceIndex_C, WORLDDATA);

	char TmpName[300];
	StrFixFilename((char*)_pRcName, TmpName, sizeof(TmpName));

	// Check if we have already have created this resource
	int iRC = ResourceExists(TmpName);
	if (iRC > 0) return iRC;
	
//	M_TRACEALWAYS("Creating new resource %s:%s\n", m_lClasses[_RcClass].m_ClassName.Str(), _pRcName);
	// Check class index
	if (!m_lClasses.ValidPos(_RcClass))
		Error("GetResourceIndex", CStrF("Invalid resource-class. (%s, class %d)", TmpName, _RcClass));

	MPUSH(GetResourceIndex);
	RESOURCE_MEMSCOPE;

	// Instance resource object
	TPtr<CReferenceCount> spObj = (CReferenceCount*) MRTC_GetObjectManager()->GetClassRegistry()->CreateObject(m_lClasses[_RcClass].m_ClassName);
	if (!spObj) Error("GetResourceIndex", "Invalid resource class: " + m_lClasses[_RcClass].m_ClassName);

	// Make sure it really is a resource object.
	spCWResource spRc;
	spRc = safe_cast<CWResource>((CReferenceCount*) spObj);
	if (!spRc) Error("GetResourceIndex", CStrF("Resource class %s is not a subclass of CWResource.", (char*)m_lClasses[_RcClass].m_ClassName));

	// Create it
	M_TRY
	{
		spRc->m_Flags = m_lCreateFlagsStack[m_iCreateFlagsStack] & ~(WRESOURCE_LOADED | WRESOURCE_ASYNCLOADING | WRESOURCE_ASYNCLOADHIGHPRIOR);
		if (!spRc->Create(this, TmpName, _pMapData, _RcClass))
		{
			//	LogFile("(CWorldDataCore::GetResourceIndex) Failed creating resource " + CStr(_pRcName) + ".");
			return 0;
		}
	}
	M_CATCH(
	catch(CCExceptionFile)
	{
		LogFile("(CWorldDataCore::GetResourceIndex) File exception creating resource " + CStr(TmpName) + ".");
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
		return 0;
	}
	)
#ifdef M_SUPPORTSTATUSCORRUPT
	M_CATCH(
	catch(CCException)
	{
		LogFile("(CWorldDataCore::GetResourceIndex) Exception creating resource " + CStr(TmpName) + ".");
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
		return 0;
	}
	)
#endif

	spRc->m_MemUsed += RESOURCE_MEMDELTA;

	// Make sure the resource thinks it is the correct class.
	if (spRc->GetClass() != _RcClass)
		Error("GetResourceIndex", CStrF("Internal error, Resource: %s", TmpName));

	int iRc = AddResource(spRc);
	if (!iRc) Error("GetResourceIndex", CStrF("Internal error. (Rc %d)", iRc));

	if (spRc->IsPrecache())
		Resource_Load(iRc);

	return iRc;
	MPOP;
}

int CWorldDataCore::GetResourceIndex(const char* _pRcName, CMapData* _pMapData)
{
	MAUTOSTRIP(CWorldDataCore_GetResourceIndex_2, 0);
//	MSCOPE(CWorldDataCore::GetResourceIndex, WORLDDATA);

	char TmpName[300];
	StrFixFilename((char*)_pRcName, TmpName, sizeof(TmpName));

	int l = CStr::StrLen(TmpName);
	if (l < 4) return 0;
	if (_pRcName[3] != ':') Error("GetResourceIndex", CStrF("Invalid resource name: %s", TmpName));

	for(int i = 0; i < m_lClasses.Len(); i++)
		if (CStrBase::strnicmp(m_lClasses[i].m_Prefix, TmpName, 3) == 0)
			return GetResourceIndex(_pRcName, i, _pMapData);
	Error("GetResourceIndex", CStrF("Resource class not identified. (%s)", TmpName));
	return 0;
}

CWResource* CWorldDataCore::GetResource(const char* _RcName, int _RcClass, CMapData* _pMapData)
{
	MAUTOSTRIP(CWorldDataCore_GetResource, NULL);
	int iRc = GetResourceIndex(_RcName, _RcClass, _pMapData);
	return GetResource(iRc);
}

CWResource* CWorldDataCore::GetResource(int _iRc)
{
	MAUTOSTRIP(CWorldDataCore_GetResource_2, NULL);
	M_LOCK(m_WDataLock);
	if (_iRc < 0 || _iRc >= m_lspResources.Len()) return NULL;
	return m_lspResources.GetBasePtr()[_iRc];
}

spCWResource CWorldDataCore::GetResourceRef(int _iRc)
{
	MAUTOSTRIP(CWorldDataCore_GetResourceRef, NULL);
	M_LOCK(m_WDataLock);
	if (_iRc < 0 || _iRc >= m_lspResources.Len()) return NULL;
	return m_lspResources.GetBasePtr()[_iRc];
}

// -------------------------------------------------------------------
int CWorldDataCore::Resource_GetCreateFlags()
{
	MAUTOSTRIP(CWorldDataCore_Resource_GetCreateFlags, 0);
	return m_lCreateFlagsStack[m_iCreateFlagsStack];
}

void CWorldDataCore::Resource_PushCreateFlags(int _Flags)
{
	MAUTOSTRIP(CWorldDataCore_Resource_PushCreateFlags, MAUTOSTRIP_VOID);
	M_LOCK(m_WDataLock);
	m_lCreateFlagsStack[m_iCreateFlagsStack+1] = _Flags;
	m_iCreateFlagsStack++;
	if (m_iCreateFlagsStack >= WRESOURCE_CREATEFLAGSSTACKDEPTH)
		Error("Resource_PopCreateFlags", "Stack overflow.");
}

void CWorldDataCore::Resource_PopCreateFlags()
{
	MAUTOSTRIP(CWorldDataCore_Resource_PopCreateFlags, MAUTOSTRIP_VOID);
	M_LOCK(m_WDataLock);
	m_iCreateFlagsStack--;
	if (m_iCreateFlagsStack < 0)
		Error("Resource_PopCreateFlags", "Stack underflow.");
}

// -------------------------------------------------------------------
int CWorldDataCore::Resource_GetMaxIndex()
{
	MAUTOSTRIP(CWorldDataCore_Resource_GetMaxIndex, 0);
	return m_lspResources.Len()-1;
}

int CWorldDataCore::Resource_GetFlags(int _iRc)
{
	MAUTOSTRIP(CWorldDataCore_Resource_GetFlags, 0);
	CWResource* pRc = GetResource(_iRc);
	if (!pRc) return 0;
	return pRc->m_Flags;
}

bool CWorldDataCore::Resource_IsValid(int _iRc)
{
	MAUTOSTRIP(CWorldDataCore_Resource_IsValid, false);
	M_LOCK(m_WDataLock);
	if (!m_lspResources.ValidPos(_iRc)) return false;
	if (m_lspResources[_iRc] == NULL) return false;
	return true;
}

int CWorldDataCore::Resource_GetClass(int _iRc)
{
	MAUTOSTRIP(CWorldDataCore_Resource_GetClass, 0);
	CWResource* pRc = GetResource(_iRc);
	if (!pRc) return -1;
	return pRc->GetClass();
}

const char* CWorldDataCore::Resource_GetName(int _iRc)
{
	MAUTOSTRIP(CWorldDataCore_Resource_GetName, NULL);
	CWResource* pRc = GetResource(_iRc);
	if (!pRc) return NULL;
	return pRc->GetName();
}

const char* CWorldDataCore::Resource_GetClassPrefix(int _iRc)
{
	MAUTOSTRIP(CWorldDataCore_Resource_GetClassPrefix, NULL);
	CWResource* pRc = GetResource(_iRc);
	if (!pRc) return NULL;
	return GetResourceClassPrefix(pRc->GetClass());
}

void CWorldDataCore::Resource_Delete(int _iRc)
{
	MAUTOSTRIP(CWorldDataCore_Resource_Delete, MAUTOSTRIP_VOID);
//	MSCOPE(Resource_Delete, WORLDDATA);
	M_LOCK(m_WDataLock);

	if (!Resource_IsValid(_iRc)) return;

	m_Loader.RemoveFromQueue(m_lspResources[_iRc]);

//	int iClass = m_lspResources[_iRc]->GetClass();
	m_lspResources[_iRc] = NULL;

	if (m_spHash != NULL) 
		m_spHash->Remove(_iRc);

	if (m_spPartialNameHash != NULL && _iRc < m_spPartialNameHash->GetMaxIDs())
		m_spPartialNameHash->Remove(_iRc);
}

void CWorldDataCore::Resource_Unload(int _iRc)
{
	MAUTOSTRIP(CWorldDataCore_Resource_Unload, MAUTOSTRIP_VOID);
	MSCOPESHORT(Resource_Unload);
	RESOURCE_MEMSCOPE;
	M_LOCK(m_WDataLock);
	CWResource* pRc = GetResource(_iRc);
	if (!pRc) return;
	m_Loader.RemoveFromQueue(pRc);
	pRc->OnUnload();
	pRc->m_Flags &= ~WRESOURCE_LOADED;
	pRc->m_MemUsed += RESOURCE_MEMDELTA;
}

void CWorldDataCore::Resource_Load(int _iRc)
{
	MAUTOSTRIP(CWorldDataCore_Resource_Load, MAUTOSTRIP_VOID);
	MSCOPESHORT(Resource_load);
	CWResource* pRc = GetResource(_iRc);
	if (!pRc) return;
	
	if (CDiskUtil::GetCorrupt() & DISKUTIL_STATUS_CORRUPTANY)
		return;

	m_Loader.AddToQueue(pRc, 0);
	if (m_Loader.m_Status)
		CDiskUtil::AddCorrupt(m_Loader.m_Status);
}

void CWorldDataCore::Resource_LoadSync(int _iRc)
{
	MAUTOSTRIP(CWorldDataCore_Resource_LoadSync, MAUTOSTRIP_VOID);
//	MSCOPE(Resource_load, WORLDDATA);
	CWResource* pRc = GetResource(_iRc);
	if (!pRc) return;
	if (pRc->IsLoaded()) return;
	if (CDiskUtil::GetCorrupt() & DISKUTIL_STATUS_CORRUPTANY)
		return;
//	ConOutL(CStrF("(CWorldDataCore::Resource_LoadSync) Block on resource: %s", pRc->m_Name.Str()));
	m_Loader.AddToQueue(pRc, 1);
	while(!pRc->IsLoaded())
	{
		if (m_Loader.m_Status)
			CDiskUtil::AddCorrupt(m_Loader.m_Status);
		if (CDiskUtil::GetCorrupt() & DISKUTIL_STATUS_CORRUPTANY)
			break;
		MRTC_SystemInfo::OS_Sleep(5);
	}

	if (m_Loader.m_Status)
		CDiskUtil::AddCorrupt(m_Loader.m_Status);
}

void CWorldDataCore::Resource_Precache(int _iRc, class CXR_Engine* _pEngine)
{
	MAUTOSTRIP(CWorldDataCore_Resource_Precache, MAUTOSTRIP_VOID);
//	MSCOPE(Resource_Precache, WORLDDATA);
	CWResource* pRc = GetResource(_iRc);
	if (!pRc) return;
	if (CDiskUtil::GetCorrupt() & DISKUTIL_STATUS_CORRUPTANY)
		return;
	if (!pRc->IsLoaded())
		Resource_LoadSync(_iRc);
	if (CDiskUtil::GetCorrupt() & DISKUTIL_STATUS_CORRUPTANY)
		return;
	pRc->OnPrecache(_pEngine);
}

void CWorldDataCore::Resource_PostPrecache(int _iRc, class CXR_Engine* _pEngine)
{
	MAUTOSTRIP(CWorldDataCore_Resource_PostPrecache, MAUTOSTRIP_VOID);
//	MSCOPE(Resource_PostPrecache, WORLDDATA);
	CWResource* pRc = GetResource(_iRc);
	if (!pRc) return;
	if (CDiskUtil::GetCorrupt() & DISKUTIL_STATUS_CORRUPTANY)
		return;
	if (!pRc->IsLoaded())	// If not loaded (erh.. would be weird)... then return
		return;
	if (CDiskUtil::GetCorrupt() & DISKUTIL_STATUS_CORRUPTANY)
		return;
	pRc->OnPostPrecache(_pEngine);
}

void CWorldDataCore::Resource_Touch(int _iRc)
{
	MAUTOSTRIP(CWorldDataCore_Resource_Touch, MAUTOSTRIP_VOID);
	CWResource* pRc = GetResource(_iRc);
	if (!pRc) return;

	pRc->m_TouchTime = m_TouchTime;
}
int CWorldDataCore::XDF_GetSize()
{
	return m_XDFSize_Server + m_XDFSize_Precache;
}

int CWorldDataCore::XDF_GetPosition()
{
	CStr File = CDiskUtil::XDF_GetCurrent().UpperCase();
	int Pos = CDiskUtil::XDF_GetPosition();

	if (File.Find("PRECACHE.XDF") != -1)
		Pos += m_XDFSize_Server;

	if (Pos > m_XDFSize_MaxPos)
		m_XDFSize_MaxPos = Pos;

	return m_XDFSize_MaxPos;
}

void CWorldDataCore::Resource_WorldOpen(CStr _WorldName)
{
	CFStr WorldName = _WorldName;

	M_TRY
	{
		m_DialogContainer.Create(ResolveFileName("Dialogues\\All.xcd"), true);
#ifndef PLATFORM_CONSOLE
		m_DialogContainer.CloseFile();
#endif
	}
	M_CATCH(
	catch (CCException)
	{
	}
	)

	if (WorldName != m_WorldName)
	{
		m_WorldName = WorldName;

		CStr XDFPath0 = ResolveFileName("XDF\\" + _WorldName.GetFilenameNoExt() + ".wtc");
		CStr XDFPath1 = ResolveFileName("XDF\\" + _WorldName.GetFilenameNoExt() + ".wwc");
		CStr XDFPath2 = ResolveFileName("XDF\\" + _WorldName.GetFilenameNoExt() + ".swwc");
		if (CDiskUtil::FileExists(XDFPath0))
		{
			m_spTextureContainer = NULL;
			m_spTextureContainer = MNew(CTextureContainer_VirtualXTC2);
			((CTextureContainer_VirtualXTC2*)(CTextureContainer *)m_spTextureContainer)->Create(XDFPath0);
		}

		if (CDiskUtil::FileExists(XDFPath1))
		{
			if (m_spWaveContainer)
				RemoveWC((CWaveContainer_Plain *)(CWaveContainer *)m_spWaveContainer);
			m_spWaveContainer = NULL;
			m_spWaveContainer = MNew(CWaveContainer_Plain);
			((CWaveContainer_Plain*)(CWaveContainer *)m_spWaveContainer)->AddXWC(XDFPath1, true);
			AddWC((CWaveContainer_Plain*)(CWaveContainer *)m_spWaveContainer);
		}

		if (CDiskUtil::FileExists(XDFPath2))
		{
			if (m_spWaveContainerStreamed)
				RemoveWC((CWaveContainer_Plain *)(CWaveContainer *)m_spWaveContainerStreamed);
			m_spWaveContainerStreamed = NULL;
			m_spWaveContainerStreamed = MNew(CWaveContainer_Plain);
			((CWaveContainer_Plain*)(CWaveContainer *)m_spWaveContainerStreamed)->AddXWC(XDFPath2, true);
			AddWC((CWaveContainer_Plain*)(CWaveContainer *)m_spWaveContainerStreamed);
		}
	}

}


void CWorldDataCore::Resource_WorldClose()
{
	if (!m_WorldNameClient.Len() && !m_WorldNameServer.Len())
	{
		m_WorldName.Clear();
		m_spTextureContainer = NULL;

		if (m_spWaveContainer)
			RemoveWC((CWaveContainer_Plain *)(CWaveContainer *)m_spWaveContainer);
		m_spWaveContainer = NULL;

		if (m_spWaveContainerStreamed)
			RemoveWC((CWaveContainer_Plain *)(CWaveContainer *)m_spWaveContainerStreamed);
		m_spWaveContainerStreamed = NULL;
		m_DialogContainer.Clear();
	}

}

#ifdef PLATFORM_XENON
// Hack
void gf_RenderXenonFlushExternalResources();
#endif

int CWorldDataCore::Resource_WorldOpenServer(CStr _WorldName)
{
	m_XDFSize_Server = 0;
	m_XDFSize_Precache = 0;
	m_XDFSize_MaxPos = 0;

#ifdef PLATFORM_XENON
// Hack
	gf_RenderXenonFlushExternalResources();
#endif

	m_WorldNameServer = _WorldName;
	Resource_WorldOpen(_WorldName);

	return 0;
}

int CWorldDataCore::Resource_WorldNeedXDF()
{
	if (CDiskUtil::XDF_GetCurrent() != "")
		return 0;

	CStr XDFPath = ResolveFileName("XDF\\" + m_WorldNameClient + "_Precache.XDF");

	bool bXDFEnable = true;

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) Error("Resource_WorldOpenClient", "No system.");

	M_TRY
	{
		if (CDiskUtil::FileExists(XDFPath))
			CDiskUtil::XDF_Use(XDFPath, ResolvePath(""));
		else
			bXDFEnable = false;
	}
	M_CATCH(
	catch(CCExceptionFile)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
	}
	)
#ifdef M_SUPPORTSTATUSCORRUPT
	M_CATCH(
	catch(CCException)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
	}
	)
#endif
	return 0;
}

int CWorldDataCore::Resource_WorldOpenClient(CStr _WorldName)
{
	m_WorldNameClient = _WorldName;
	Resource_WorldOpen(_WorldName);

	return 0;
}

int CWorldDataCore::Resource_WorldCloseServer()
{
	int nRc = Resource_GetMaxIndex() + 1;
	for(int i = 0; i < nRc; i++)
	{
		if (Resource_IsValid(i))
		{
			const char* pPrefix = Resource_GetClassPrefix(i);
			if (pPrefix)
			{
				if (strcmp(pPrefix, "TPL") == 0)
				{
					LogFile((char*)CStrF("(CGameContext::FreeResources) Deleting %s", Resource_GetName(i)));
					Resource_Delete(i);
				}
			}
		}
	}
	m_WorldNameServer.Clear();
	Resource_WorldClose();

	return 0;
}
int CWorldDataCore::Resource_WorldCloseClient()
{
	m_WorldNameClient.Clear();

	Resource_WorldClose();

	return 0;
}


void CWorldDataCore::Resource_AutoHibernate()
{
	MAUTOSTRIP(CWorldDataCore_Resource_AutoHibernate, MAUTOSTRIP_VOID);
	if (CDiskUtil::GetCorrupt() & DISKUTIL_STATUS_CORRUPTANY)
		return;

//	MSCOPE(Resource_AutoHibernate, WORLDDATA);

	fp32 dTime = Clamp01((m_TimeLastRefresh - m_TimeLastHibernate).GetTime());
	m_TimeLastHibernate = m_TimeLastRefresh;

	int nRc = m_lspResources.Len();
	int nRefresh = Max(10, Min(RoundToInt(0.5f + fp32(nRc) * dTime), nRc));

	if (m_iLastHibernate >= nRc)
		m_iLastHibernate = 0;

	int iRc = m_iLastHibernate;

	for(int i = 0; i < nRefresh; i++)
	{
		CWResource* pRc = GetResource(iRc);
		if (pRc && pRc->IsLoaded())
		{
			CWD_ResourceClassDesc& RcClass = m_lClasses[pRc->GetClass()];
			const fp32 Delta = (m_TouchTime - pRc->m_TouchTime).GetTime();
			if (!pRc->IsPrecache() &&
				RcClass.m_AutoUnloadTimeout != 0 && 
				Delta > RcClass.m_AutoUnloadTimeout)
			{
				MPUSH(Resource_AutoUnload);
				Resource_Unload(pRc->m_iRc);
				MPOP;
			}
			else if (RcClass.m_AutoHibernateTimeout != 0)
			{
				if (Delta > RcClass.m_AutoHibernateTimeout)
				{
					MPUSH(Resource_AutoHibernate);
					RESOURCE_MEMSCOPE;
					pRc->OnHibernate();
					pRc->m_TouchTime = m_TouchTime;
					pRc->m_MemUsed += RESOURCE_MEMDELTA;
					MPOP;
				}
				else
				{
					MPUSH(Resource_AutoHibernate);
					RESOURCE_MEMSCOPE;
					pRc->OnRefresh();
					pRc->m_MemUsed += RESOURCE_MEMDELTA;
					MPOP;
				}
			}
		}

		iRc++;
		if (iRc >= nRc)
			iRc = 0;
	}

	m_iLastHibernate = iRc;
}

void CWorldDataCore::Resource_DeleteUnreferenced(CXR_VBManager* _pVBM, int _Mask)
{
	MAUTOSTRIP(CWorldDataCore_Resource_DeleteUnreferenced, MAUTOSTRIP_VOID);
//	MSCOPE(Resource_DeleteUnreferenced, WORLDDATA);

	int nDel;
	do
	{
		nDel = 0;
		for(int iRc = 0; iRc < m_lspResources.Len(); iRc++)
		{
			CWResource* pRc = GetResource(iRc);
			if(pRc && _Mask & (1 << pRc->m_iRcClass))
			{
				if (pRc->MRTC_ReferenceCount() == 1)
				{
					Resource_Delete(iRc);
					nDel++;
				}
			}
		}
//		LogFile(CStrF("(CWorldDataCore::Resource_DeleteUnreferenced) %d resources deleted.", nDel));
	} while(nDel);
}

// -------------------------------------------------------------------
#ifndef MDISABLE_CLIENT_SERVER_RES
void CWorldDataCore::ClearClassStatistics()
{
	MAUTOSTRIP(CWorldDataCore_ClearClassStatistics, MAUTOSTRIP_VOID);
	for(int i = 0; i < m_lspResources.Len(); i++)
	{
		CWResource* pRc = GetResource(i);
		if (pRc && (pRc->GetClass() == WRESOURCE_CLASS_WOBJECTCLASS))
		{
			CWRes_Class* pC = safe_cast<CWRes_Class>(pRc);
			if (pC) pC->ClearStats();
		}
	}
}
#endif

void CWorldDataCore::UpdateResourceClass(const char* _pPrefix)
{
	MAUTOSTRIP(CWorldDataCore_UpdateResourceClass, MAUTOSTRIP_VOID);
//	MSCOPE(CWorldDataCore::UpdateResourceClass, WORLDDATA);
	
	// This function must be used with caution since all
	// resource classes cannot be reloaded arbitrarily.
	// (For example BSP-models)

	for(int i = 0; i < m_lspResources.Len(); i++)
	{
		const char* pRCPrefix = Resource_GetClassPrefix(i);
		if (pRCPrefix && strcmp(pRCPrefix, _pPrefix) == 0)
		{
			Resource_Unload(i);
			Resource_Load(i);
		}
	}

	// Notify all subsystems
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(pSys) pSys->System_BroadcastMessage(CSS_Msg(CSS_MSG_PRECACHEHINT));

}

void CWorldDataCore::Con_RS_UpdateSurfaces()
{
	MAUTOSTRIP(CWorldDataCore_Con_RS_UpdateSurfaces, MAUTOSTRIP_VOID);
//	MSCOPE(CWorldDataCore::Con_RS_UpdateSurfaces, WORLDDATA);
	
	MACRO_GetRegisterObject(CXR_SurfaceContext, pSurfContext, "SYSTEM.SURFACECONTEXT");
	if (pSurfContext)
	{
		pSurfContext->UpdateSurfaces(m_lWorldPathes[0]+"Surfaces\\");
		ConOutL(CStrF("        %d surfaces.", pSurfContext->GetNumSurfaces()));
	}
}

void CWorldDataCore::Con_RS_UpdateAnims()
{
	MAUTOSTRIP(CWorldDataCore_Con_RS_UpdateAnims, MAUTOSTRIP_VOID);
//	MSCOPE(CWorldDataCore::Con_RS_UpdateAnims, WORLDDATA);
	
	UpdateResourceClass("ANM");
	UpdateResourceClass("AMP");
}

void CWorldDataCore::Con_RS_UpdateXMD()
{
	MAUTOSTRIP(CWorldDataCore_Con_RS_UpdateXMD, MAUTOSTRIP_VOID);
//	MSCOPE(CWorldDataCore::Con_RS_UpdateXMD, WORLDDATA);
	
	UpdateResourceClass("XMD");
	UpdateResourceClass("CTM");
	UpdateResourceClass("CMF");
}

void CWorldDataCore::Con_RS_UpdateRegistry()
{
	UpdateResourceClass("XRG");
	UpdateResourceClass("TPL");
}

void CWorldDataCore::Con_RS_Classes()
{
	MAUTOSTRIP(CWorldDataCore_Con_RS_Classes, MAUTOSTRIP_VOID);
	{
		for(int i = 0; i < m_lClasses.Len(); i++)
		{
			m_lClasses[i].m_MemUsed = 0;
			m_lClasses[i].m_nResources = 0;
		}
	}

	{
		for(int i = 0; i < m_lspResources.Len(); i++)
		{
			CWResource* pR = GetResource(i);
			if (pR)
			{
				m_lClasses[pR->GetClass()].m_MemUsed += pR->m_MemUsed;
				m_lClasses[pR->GetClass()].m_nResources++;
			}
		}
	}

	{
		int AllMemUsed = 0;
		int nResources = 0;

		for(int i = 0; i < m_lClasses.Len(); i++)
		{
			const CWD_ResourceClassDesc& Cls = m_lClasses[i];
			AllMemUsed += Cls.m_MemUsed;
			nResources += Cls.m_nResources;
			ConOutL(CStrF("%4.d, %4.d %s, TimeOut %.2f, MemUsed %9.d, %s", i, Cls.m_nResources, Cls.m_Prefix, Cls.m_AutoHibernateTimeout, Cls.m_MemUsed, Cls.m_ClassName.Str() ));
		}

		ConOutL(CStrF(" ALL, %4.d                    MemUsed %9.d", nResources, AllMemUsed ));
	}
}

void CWorldDataCore::Con_RS_Resources()
{
	MAUTOSTRIP(CWorldDataCore_Con_RS_Resources, MAUTOSTRIP_VOID);
	for(int i = 0; i < m_lspResources.Len(); i++)
	{
		CWResource* pR = GetResource(i);
		if (pR)
		{
			ConOutL(CStrF("%4.d, %.4x, %4.d, %9.d, %s", i, pR->GetClass(), pR->m_Flags, pR->m_MemUsed, (char*) pR->GetName()));
		}
		else
			ConOutL(CStrF("%4.d,  -", i));
	}
}

class CWRes_SortEntry
{
public:
	CWResource* m_pRc;

	CWRes_SortEntry()
	{
		m_pRc = NULL;
	};

	CWRes_SortEntry(CWResource* _pRc)
	{
		m_pRc = _pRc;
	}

	int Compare(const CWRes_SortEntry& _Entry) const
	{
		if (m_pRc->m_MemUsed < _Entry.m_pRc->m_MemUsed)
			return -1;
		else if (m_pRc->m_MemUsed > _Entry.m_pRc->m_MemUsed)
			return 1;
		else
			return 0;
	}
};

void CWorldDataCore::Con_RS_ResourcesByClass(int _iClass)
{
	MAUTOSTRIP(CWorldDataCore_Con_RS_ResourcesByClass, MAUTOSTRIP_VOID);
	TArray_Sortable<CWRes_SortEntry> lSort;
	lSort.SetGrow(m_lspResources.Len());

	{
		for(int i = 0; i < m_lspResources.Len(); i++)
		{
			CWResource* pR = GetResource(i);
			if (pR && pR->m_iRcClass == _iClass)
				lSort.Add(CWRes_SortEntry(pR));
		}
	}

	lSort.Sort();

	{
		for(int i = 0; i < lSort.Len(); i++)
		{
			CWResource* pR = lSort[i].m_pRc;;
			ConOutL(CStrF("%4.d, %.4x, %4.d, %9.d, %s", pR->m_iRc, pR->GetClass(), pR->m_Flags, pR->m_MemUsed, (char*) pR->GetName()));
		}
	}
}

void CWorldDataCore::Con_RS_ResourcesSorted()
{
	MAUTOSTRIP(CWorldDataCore_Con_RS_ResourcesSorted, MAUTOSTRIP_VOID);
	for(int i = 0; i < m_lClasses.Len(); i++)
	{
		LogFile("----------------------------------------------------------------------------------");
		Con_RS_ResourcesByClass(i);
	}
	LogFile("----------------------------------------------------------------------------------");
}

void CWorldDataCore::Register(CScriptRegisterContext & _RegContext)
{
	MAUTOSTRIP(CWorldDataCore_Register, MAUTOSTRIP_VOID);
	_RegContext.RegFunction("rs_updatesurfaces", this, &CWorldDataCore::Con_RS_UpdateSurfaces);
	_RegContext.RegFunction("rs_updateanims", this, &CWorldDataCore::Con_RS_UpdateAnims);
	_RegContext.RegFunction("rs_updatemodels", this, &CWorldDataCore::Con_RS_UpdateXMD);
	_RegContext.RegFunction("rs_updateregistry", this, &CWorldDataCore::Con_RS_UpdateRegistry);
	_RegContext.RegFunction("rs_classes", this, &CWorldDataCore::Con_RS_Classes);
	_RegContext.RegFunction("rs_resources", this, &CWorldDataCore::Con_RS_Resources);
	_RegContext.RegFunction("rs_resourcesbyclass", this, &CWorldDataCore::Con_RS_ResourcesByClass);
	_RegContext.RegFunction("rs_resourcessorted", this, &CWorldDataCore::Con_RS_ResourcesSorted);
	_RegContext.RegFunction("rs_asynccacheblock", this, &CWorldDataCore::Resource_AsyncCacheBlockOnCategory);
	
}

