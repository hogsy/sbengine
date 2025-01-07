
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/


#include "PCH.h"

#include "MCC.h"

#include "MFile_XDF.h"

#ifdef PLATFORM_XBOX

	#include <xtl.h>

#endif


//JK-NOTE: Had to move the constructor out from the class since it requires the definition of CByteStreamData class
CByteStreamManager::CByteStreamManager()
{
	MRTC_GetObjectManager()->m_pByteStreamManager = this;
	m_spLogFile = MNew(CLogFile);

	M_OFFSET(CByteStreamDrive, m_DriveName, Offset);
	m_DefaultCacheSize = 65536;
	m_DefaultNumCaches = 4;
	m_pXDFThread = 0;

	m_bLogData = false;
	m_pXDFRecord = NULL;
	m_pXDFUse = NULL;
	m_Corrupt = 0;
	m_DisableCaching = 0;
#if defined(PLATFORM_XBOX1) && !defined(M_DEMO_XBOX)

	if (CDiskUtil::FileExists("Z:\\FormatZ"))
	{
		CDiskUtil::DelFile("Z:\\FormatZ");
		XFormatUtilityDrive();
	}
#endif
	
#if defined(M_DEMO_XBOX) && defined(PLATFORM_XBOX1)
//	XMountUtilityDrive(TRUE);
#endif
}


void CByteStreamManager::DisableCache()
{
	M_LOCK(m_Lock);
	++m_DisableCaching;
}

void CByteStreamManager::EnableCache()
{
	M_LOCK(m_Lock);
	--m_DisableCaching;
}

int CByteStreamManager::GetCacheEnable()
{
	M_LOCK(m_Lock);
	return m_DisableCaching;
}


void CByteStreamCacheLine::Read()
{
	if(!m_pStream->m_pFile)
		return;

	M_LOCK(m_Lock);

//	m_PendingOperation = DNew(CByteStreamAsyncInstance) CByteStreamAsyncInstance();		
	m_PendingOperation.m_bIsDone = false;
	m_PendingOperation.m_pInstance = MRTC_SystemInfo::OS_FileAsyncRead(m_pStream->m_pFile, GetCacheLineData(), m_NumBytesUsed, m_DataOffest);
	
	fint Len = m_pStream->Len();
	mint ProcessBytes = m_NumBytesUsed;
	if (ProcessBytes + m_DataOffest > Len)
		ProcessBytes = Len - m_DataOffest;
	m_PendingOperation.m_BytesToProcess = ProcessBytes;

/*	{
		M_LOCK(m_pDrive->m_Lock);
		m_DriveList.LinkSortedStartTail(&m_pDrive->m_Requests, this);
	}*/

}

void CByteStreamCacheLine::Write()
{
	if(!m_pStream->m_pFile)
		return;

	M_LOCK(m_Lock);
	m_PendingOperation.m_bIsDone = false;
//	m_PendingOperation = DNew(CByteStreamAsyncInstance) CByteStreamAsyncInstance();		
	m_PendingOperation.m_pInstance = MRTC_SystemInfo::OS_FileAsyncWrite(m_pStream->m_pFile, GetCacheLineData(), m_NumBytesUsed, m_DataOffest);
	m_PendingOperation.m_BytesToProcess = m_NumBytesUsed;

/*	{
		M_LOCK(m_pDrive->m_Lock);
		m_DriveList.LinkSortedStartTail(&m_pDrive->m_Requests, this);
	}*/
}

void CByteStreamCacheLine::BlockUntilDone()
{
	m_pStream->Service();
	bool bWantedToThrow = false;
	if (!Done(true, bWantedToThrow))
	{
		MSCOPESHORT(CByteStreamCacheLine::BlockUntilDone);
		while (!Done(true, bWantedToThrow))
		{
			MRTC_SystemInfo::OS_Sleep(0);
			m_pStream->Service();
		}
	}
	m_pStream->Service();
}



void CByteStreamDrive::Create(const char *_pDriveName)
{
	m_DriveName = _pDriveName;
	m_Granulartity = MRTC_SystemInfo::OS_FileOperationGranularity(m_DriveName);
	
	m_CacheSize = -1;
	m_NumCaches = -1;
	
	m_lCacheMem.Clear();
#ifndef PLATFORM_CONSOLE
	m_lCacheMem.SetLen(GetStreamManager()->m_DefaultNumCaches);
	m_CacheSizeUsed = GetStreamManager()->m_DefaultCacheSize;

	for (int i = 0; i < m_lCacheMem.Len(); ++i)
	{
		m_lCacheMem[i] = MNew(CCacheMem);
		m_lCacheMem[i]->SetCacheSize(GetStreamManager()->m_DefaultCacheSize, m_Granulartity);
	}
#endif
}

void CByteStreamDrive::SetCacheSize(aint _Value)
{
	M_LOCK(m_Lock);
	m_CacheSize = _Value;
	
	if (m_CacheSize > 0)
		m_CacheSizeUsed = m_CacheSize;
	else
		m_CacheSizeUsed = GetStreamManager()->m_DefaultCacheSize;

	for (int i = 0; i < m_lCacheMem.Len(); ++i)
	{
		if (m_lCacheMem[i]->m_bUsed)
			Error_static("CByteStreamDrive::SetCacheSize", "Cannot do this function while a file is open");
		m_lCacheMem[i]->SetCacheSize(m_CacheSizeUsed, m_Granulartity);
	}
}

void CByteStreamDrive::SetCacheNum(aint _Value)
{
	M_LOCK(m_Lock);
	m_NumCaches = _Value;
	
	int i;
	for (i = 0; i < m_lCacheMem.Len(); ++i)
	{
		if (m_lCacheMem[i]->m_bUsed)
			Error_static("CByteStreamDrive::SetCacheSize", "Cannot do this function while a file is open");
	}
	
	m_lCacheMem.Clear();
	
	if (m_NumCaches > 0)
		m_lCacheMem.SetLen(m_NumCaches);
	else
		m_lCacheMem.SetLen(GetStreamManager()->m_DefaultNumCaches);
	
	for (i = 0; i < m_lCacheMem.Len(); ++i)
	{
		m_lCacheMem[i] = MNew(CCacheMem);
	}
	
	if (m_CacheSize > 0)
		m_CacheSizeUsed = m_CacheSize;
	else
		m_CacheSizeUsed = GetStreamManager()->m_DefaultCacheSize;
	
	for (i = 0; i < m_lCacheMem.Len(); ++i)
	{
		m_lCacheMem[i]->SetCacheSize(m_CacheSizeUsed, m_Granulartity);
	}
}



void CByteStreamCacheLine::Create(CByteStreamData *_pStream, aint _CacheLineSize, aint _Granulatiry, void *_pCacheLine)
{
	m_pStream = _pStream;
	m_pDrive = m_pStream->m_pDrive;

	m_pCacheMemUsed = NULL;
	m_bOwnCacheLine = false;
	m_bPrecache = false;

	if (_pCacheLine)
	{
		mint Aligned = (((_CacheLineSize - 1) / _Granulatiry) + 1) * _Granulatiry;
		M_ASSERT(_CacheLineSize == Aligned, "Must be aligned right");
		m_pCacheMemUsed = NULL;
		m_pAlignedCacheLine = _pCacheLine;
	}
	else
	{
		_CacheLineSize = (((_CacheLineSize - 1) / _Granulatiry) + 1) * _Granulatiry;
		m_pCacheMemUsed = m_pDrive->GetCacheMem(_CacheLineSize);
		
		if (m_pCacheMemUsed)
		{
			m_pAlignedCacheLine = m_pCacheMemUsed->m_pMemory;
		}
		else
		{
			m_bOwnCacheLine = true;


	#ifdef PLATFORM_DOLPHIN
			m_pAlignedCacheLine = MRTC_GetMemoryManager()->AllocAlign(_CacheLineSize, 32);
	#elif defined( PLATFORM_PS2)
			m_pAlignedCacheLine = MRTC_GetMemoryManager()->AllocAlign(_CacheLineSize, 64);
	#else
			m_pAlignedCacheLine = M_ALLOC(_CacheLineSize);
	#endif
		}
		
	}

	if (!m_pAlignedCacheLine)
		M_BREAKPOINT;

	m_DataOffest = 0;
	m_NumBytes = _CacheLineSize;
	m_NumBytesUsed = 0;
	m_bDirty = false;
	m_bWrite = false;
	m_bPending = false;
	m_bOperating = false;
}

void CByteStreamCacheLine::Destroy()
{
	M_ASSERT(!m_bPending,"");
	M_ASSERT(!m_bOperating,"");
	M_ASSERT(!m_bDirty,"");
	bool bWantThrow = false;
	M_ASSERT(!m_PendingOperation.m_pInstance || m_PendingOperation.Done(false, bWantThrow), "Must not be doing this there");

	{
		M_LOCK(m_pDrive->m_Lock);
		m_CacheLineLink.Unlink();
		m_DriveLink.Unlink();
		m_PendingLink.Unlink();
	}

	if (m_PendingOperation.m_pInstance)
		m_PendingOperation.CloseInstance();

	if (m_pCacheMemUsed)
		m_pDrive->ReturnCacheMem(m_pCacheMemUsed);
	else if (m_bOwnCacheLine)
		MRTC_GetMemoryManager()->Free(m_pAlignedCacheLine);

}



//#################################################################################
//#################################################################################
//#################################################################################

void CByteStreamManager::XDF_Record(const char* _pName, const char* _pBasePath)
{
	M_LOCK(m_XDFLock);
	if (m_pXDFThread)
	{
		if (m_pXDFThread != MRTC_SystemInfo::OS_GetThreadID())
			return;	
	}
	m_pXDFThread = MRTC_SystemInfo::OS_GetThreadID();
	
	if (m_pXDFRecord)
		delete m_pXDFRecord;

	m_pXDFRecord = NULL;
	
	CXDF * pNewRecord = DNew(CXDF) CXDF;
	
	pNewRecord->Record_Start(_pName, _pBasePath);
	
	m_pXDFRecord = pNewRecord;
}

void CByteStreamManager::XDF_Use(const char* _pName, const char* _pBasePath)
{
	M_LOCK(m_XDFLock);
	if (m_pXDFThread)
	{
		if (m_pXDFThread != MRTC_SystemInfo::OS_GetThreadID())
			return;	
	}
	m_pXDFThread = MRTC_SystemInfo::OS_GetThreadID();
	
	M_TRACEALWAYS("--- XDFUse (name='%s' basepath='%s') ---\n", _pName, _pBasePath);

	CXDF * pUse = m_pXDFUse;

	{
		DLock(m_XDFUseLock);
		m_pXDFUse = NULL;
	}
	if (pUse)
		delete m_pXDFUse;
	
	CXDF *pUseXDF = DNew(CXDF) CXDF;
	
	M_TRY
	{
		pUseXDF->Open(_pName, _pBasePath);
	}
	M_CATCH(
	catch (CCException)
	{
		delete pUseXDF;
		throw;
	}
	)
	
	{
		DLock(m_XDFUseLock);
		m_pXDFUse = pUseXDF;
	}
}

CStr CByteStreamManager::XDF_GetCurrent()
{
	DLock(m_XDFUseLock);
	if (m_pXDFUse)
	{
		return m_pXDFUse->m_File;
	}
	else
		return "";
}

fint CByteStreamManager::XDF_GetPosition()
{
	if (m_pXDFUse)
	{
		return m_pXDFUse->m_FileStream.Pos();
	}
	else
		return 0;
}


void CByteStreamManager::XDF_Stop()
{
	M_LOCK(m_XDFLock);
	if (m_pXDFThread != MRTC_SystemInfo::OS_GetThreadID())
		return;	

	M_TRACEALWAYS("--- XDFStop ---\n");
	m_pXDFThread = NULL;
	
	if (m_pXDFRecord)
		delete m_pXDFRecord;
	m_pXDFRecord = NULL;
	
	CXDF * pXDF = m_pXDFUse;
	{
		DLock(m_XDFUseLock);
		m_pXDFUse = NULL;
	}
	if (pXDF)
		delete pXDF;
}

void CByteStreamManager::XDF_Pause()
{
	if (m_pXDFThread != MRTC_SystemInfo::OS_GetThreadID())
		return;	
	
	if (m_pXDFRecord)
	{
		return m_pXDFRecord->Pause();
	}
	else if (m_pXDFUse)
	{
		return m_pXDFUse->Pause();
	}
}

void CByteStreamManager::XDF_Resume()
{
	if (m_pXDFThread != MRTC_SystemInfo::OS_GetThreadID())
		return;	
	
	if (m_pXDFRecord)
	{
		return m_pXDFRecord->Resume();
	}
	else if (m_pXDFUse)
	{
		return m_pXDFUse->Resume();
	}
}

bint CByteStreamManager::XDF_IsPasued()
{
	if (m_pXDFThread != MRTC_SystemInfo::OS_GetThreadID())
		return true;	
	
	if (m_pXDFRecord)
	{
		return m_pXDFRecord->m_Pause != 0;
	}
	else if (m_pXDFUse)
	{
		return m_pXDFUse->m_Pause != 0;
	}

	return true;
}

CXDF *CByteStreamManager::XDF_GetUse()
{

	if (m_pXDFThread != MRTC_SystemInfo::OS_GetThreadID())
		return NULL;
	
	return m_pXDFUse;
}

CXDF *CByteStreamManager::XDF_GetRecord()
{
	if (m_pXDFThread != MRTC_SystemInfo::OS_GetThreadID())
		return NULL;
	
	return m_pXDFRecord;
}


void CByteStreamManager::XDF_Destruct()
{
	M_LOCK(m_XDFLock);
	if (m_pXDFRecord)
		delete m_pXDFRecord;
	m_pXDFRecord = NULL;
	CXDF *pUse = m_pXDFUse;
	{
		DLock(m_XDFUseLock);
		m_pXDFUse = NULL;
	}
	if (pUse)
		delete pUse;
}

#ifndef M_RTM

void CByteStreamData::XDFRecordCreate(CStr _FileName)
{
	CXDF *pXDF = m_pStreamManager->XDF_GetRecord();
	if (pXDF)
		pXDF->Record_OpenFile(_FileName);
}

void CByteStreamData::XDFRecordClose()
{
	CXDF *pXDF = m_pStreamManager->XDF_GetRecord();
	if (pXDF)
		pXDF->Record_CloseFile(m_FileName);
}

void CByteStreamData::XDFRecordSeek()
{
	CXDF *pXDF = m_pStreamManager->XDF_GetRecord();
	if (!CDiskUtil::XDF_IsPaused() && pXDF && m_LastReadFilePosXDF != m_FilePos && !(m_OpenFlags & EByteStream_NoLog))
	{
		pXDF->Record_Seek(m_FilePos);
	}
}

void CByteStreamData::XDFRecordRead()
{
	CXDF *pXDF = m_pStreamManager->XDF_GetRecord();
	if (pXDF)
		pXDF->Record_Read(m_FilePos);
}
#endif

//#################################################################################
//#################################################################################
//#################################################################################
CByteStreamManager *CreateStreamManager()
{
	M_LOCK(*(MRTC_GetObjectManager()->m_pGlobalLock));
	if (!MRTC_GetObjectManager()->m_pByteStreamManager)
	{
		MRTC_GetObjectManager()->m_pByteStreamManager = DNew(CByteStreamManager) CByteStreamManager;
		if(MRTC_GetObjectManager()->m_pByteStreamManager->m_bLogData)
		{
#ifdef PLATFORM_XBOX
#ifndef M_RTM
			MRTC_GetObjectManager()->m_pByteStreamManager->m_spLogFile->Create("Z:\\SBZFileUsage");
#endif
#elif defined PLATFORM_CONSOLE

#elif defined(PLATFORM_WIN_PC) && !defined(M_RTM)
			MRTC_GetObjectManager()->m_pByteStreamManager->m_spLogFile->Create("C:\\SBZFileUsage");
#endif
		}
	}

	return MRTC_GetObjectManager()->m_pByteStreamManager;
}

CByteStreamDrive *CByteStreamManager::CreateDrive(CStr &_Drive)
{
	CByteStreamDrive *pDrive;

	pDrive = DNew(CByteStreamDrive) CByteStreamDrive;
	pDrive->Create(_Drive);
	m_DriveTree.f_Insert(pDrive);
	m_Drives.Insert(pDrive);
	return pDrive;
}

bool CByteStreamManager::ServiceDrives(bool _bThrows)
{
	bool bThrew = 0;
	M_LOCK(m_Lock);
	DLinkDS_Iter(CByteStreamDrive, m_Link) Iter = m_Drives;

	while (Iter)
	{
		CByteStreamDrive *pDrive = Iter;
		++Iter;
		{
			M_UNLOCK(m_Lock);
			if (pDrive->Service(NULL))
				bThrew = true;
		}
	}

	return bThrew;
}


CByteStreamManager::~CByteStreamManager()
{
	XDF_Destruct();

	m_spLogFile = NULL;
	{
		DLinkDS_Iter(CByteStreamData, m_Link) Iter = m_OpenStreams;
		while (Iter)
		{
			CByteStreamData* pData = (CByteStreamData*)Iter;
			++Iter;
			M_TRACEALWAYS("Autodeleting stream for file %s\r\n", pData->m_FileName.GetStr());
			delete pData;
		}
	}

	m_DriveTree.RemoveAll();
	m_Drives.DeleteAll();
}


void CByteStreamManager::SearchPath_Add(CStr _Path)
{	
	M_LOCK(m_Lock);
	m_SearchPaths.Add(_Path);
}

void CByteStreamManager::SearchPath_SetBase(CStr _Path)
{
	M_LOCK(m_Lock);
	m_SearchPathBase = _Path;
	m_SearchPathBase.MakeLowerCase();
}

void CByteStreamManager::SetDriveCacheSize(const char *_pDrive, aint _Value)
{
	CStr Tmp = _pDrive;
	GetDrive(Tmp)->SetCacheSize(_Value);

}

void CByteStreamManager::SetDriveCacheNum(const char *_pDrive, aint _Value)
{
	CStr Tmp = _pDrive;
	GetDrive(Tmp)->SetCacheNum(_Value);
}

void CByteStreamData::SetPriority(fp32 _Priority)
{
	m_Priority = _Priority;
}

bool CByteStreamData::Create(const CStr &_FileName, int _Flags, float _Priority, aint _NumCacheLines, aint _CacheLineSize)
{
#ifndef M_RTM
	if (m_pStreamManager->m_bLogData && !(_Flags & EByteStream_NoLog))
	{
		M_LOCK(m_pStreamManager->m_Lock);
		m_pStreamManager->m_spLogFile->Log(CFStrF("%s", _FileName.Str()).Str());
		m_TimeOpen.Start();
	}
	
	if (!CDiskUtil::XDF_IsPaused() && !(_Flags & EByteStream_NoLog))
	{
		XDFRecordCreate(_FileName);
	}
#endif

//		MSCOPESHORT(CByteStreamData::Create);

//		M_CALLGRAPH;

	Close();

	m_FileName = _FileName;
	m_Priority = _Priority;
	m_CacheLine_Size = _CacheLineSize;
	m_CacheLine_Num = _NumCacheLines;

	m_FilePos = 0;
#ifndef M_RTM
	m_LastReadFilePos = -1;
	m_LastReadFilePosXDF = 0;
#endif
	m_BytesRead = 0;

	CStr DriveName;
	{
//			MSCOPE(GetDriveName, IGNORE);
		char TempChar[256];
		MRTC_SystemInfo::OS_FileGetDrive(m_FileName, TempChar);

		DriveName = TempChar;
	}

	{
//			MSCOPE(GetDrive, IGNORE);
		m_pDrive = m_pStreamManager->GetDrive(DriveName);
	}

	{
		M_LOCK(m_pStreamManager->m_Lock);
		m_pStreamManager->m_OpenStreams.Insert(this);
	}

	{
		m_OpenFlags = _Flags;
//			MSCOPE(MRTC_SystemInfo::OS_FileOpen, IGNORE);
//		m_pFile = MRTC_SystemInfo::OS_FileOpen(m_FileName, (_Flags & EByteStream_Read) != 0, 0, 0, 0);
		m_pFile = MRTC_SystemInfo::OS_FileOpen(m_FileName, (_Flags & (EByteStream_Read | EByteStream_Write)) != 0, (_Flags & EByteStream_Write) != 0, (_Flags & EByteStream_Create) != 0, (_Flags & EByteStream_Truncate) != 0, (_Flags & EByteStream_NoDeferClose) == 0);
	}

	if (!m_pFile)
		return false;

	{
//			MSCOPE(MRTC_SystemInfo::OS_FileSize, IGNORE);
		if (_Flags & EByteStream_DoNotCacheFileSize)
			m_FileSize = -1;		
		else
			m_FileSize = MRTC_SystemInfo::OS_FileSize(m_pFile);
	}

	return true;
}

void CByteStreamData::Close()
{	
#ifndef M_RTM
	if (m_pStreamManager->m_bLogData && m_FileName.Len())
	{
		if (!(m_OpenFlags & EByteStream_NoLog))
		{
			m_TimeOpen.Stop();
			fp32 Time = m_TimeOpen.GetTime();

			M_LOCK(m_pStreamManager->m_Lock);
			m_pStreamManager->m_spLogFile->Log(CFStrF("X %9d (%.3f MB/Sec, %.1f ms) Closed File %s", (int)m_BytesRead, (m_BytesRead / Time) / (1024.0f * 1024.0f), Time*1000.0f, m_FileName.Str()).Str());
		}
	}
	
	if (!CDiskUtil::XDF_IsPaused() && !(m_OpenFlags & EByteStream_NoLog) && m_FileName.Len())
	{
		XDFRecordClose();
	}
#endif
	bool bFailedDelete = false;
//		MSCOPESHORT(CByteStreamData::Close);
	m_pLastCacheLine = NULL;
	{			

#ifdef PLATFORM_XBOX
		// Remove those that are not writing
		if (!(m_OpenFlags & (EByteStream_Write | EByteStream_NoDeferClose)))
		{
			DLinkDS_Iter(CByteStreamCacheLine, m_CacheLineLink) Iter = m_PendingCacheLines;
			if (Iter)
			{
				MSCOPESHORT(CByteStreamCacheLine::BlockOnPending);
				while (Iter)
				{
			
					CByteStreamCacheLine *_pToDelete = Iter;
					++Iter;
				
					if (!_pToDelete->m_bWrite)
					{
						{
							M_LOCK(m_pDrive->m_Lock);

							_pToDelete->m_CacheLineLink.Unlink();
							_pToDelete->m_DriveLink.Unlink();
							_pToDelete->m_PendingLink.Unlink();
						}

						_pToDelete->m_bPending = false;
						_pToDelete->m_bOperating = false;

						bool bWantedToThrow = false;

						if (_pToDelete->m_PendingOperation.m_pInstance && !_pToDelete->Done(false, bWantedToThrow))
						{
							M_LOCK(m_pDrive->m_Lock);
							m_pDrive->m_PendingCacheLinesForDelete.Insert(_pToDelete);
						}
						else
						{
							M_LOCK(m_pDrive->m_Lock);
							m_pStreamManager->m_PoolCacheLines.Delete(_pToDelete);
						}						
					}
				}
			}
		}
#endif
		
		M_TRY
		{
			Service();

			{
				CByteStreamCacheLine *pFirst = m_PendingCacheLines.GetFirst();
				if (pFirst)
				{
					MSCOPESHORT(CByteStreamCacheLine::BlockOnPending);
					while (pFirst)
					{
						Service();
		/*				CByteStreamCacheLine *_pToDelete = Iter;
						_pToDelete->BlockUntilDone();
						
						delete _pToDelete;*/
						MRTC_SystemInfo::OS_Sleep(0);
						pFirst = m_PendingCacheLines.GetFirst();
					}
					Service();
				}
			}
		}
		M_CATCH(
		catch (CCExceptionFile)
		{
			bFailedDelete = true;
		}
		)

		CByteStreamCacheLine *pFirst = m_CacheLines.GetFirst();
		while (pFirst)
		{
			CByteStreamCacheLine *_pToDelete = pFirst;


			if (_pToDelete->m_bDirty)
			{
				M_ASSERT(!_pToDelete->m_bWrite, "HoHo");
				M_TRY
				{
					WriteCacheLine(_pToDelete);
					
					Service();
					{
						//						MSCOPESHORT(CByteStreamCacheLine::BlockOnPending2);
						bool bWantedToThrow = false;

						while (!_pToDelete->Done(true, bWantedToThrow))
						{
							MRTC_SystemInfo::OS_Sleep(0);
							Service();
						}
					}
					Service();
				}
				M_CATCH(
				catch (CCExceptionFile)
				{
					_pToDelete->m_bDirty = false;
					_pToDelete->m_bPending = false;
					bFailedDelete = true;
				}
				)
			}

			
			m_pStreamManager->m_PoolCacheLines.Delete(_pToDelete);
			pFirst = m_CacheLines.GetFirst();
		}
		m_nCacheLines = 0;
	}

	if (m_pFile)
	{
		M_TRY
		{
			MRTC_SystemInfo::OS_FileClose(m_pFile);
		}
		M_CATCH(
		catch (CCExceptionFile)
		{
			bFailedDelete = true;
		}
		)
		
		m_pFile = NULL;

		M_TRY
		{
			if (m_FileName.Len() && (m_OpenFlags & EByteStream_Write))
				MRTC_SystemInfo::OS_FileSetFileSize(m_FileName, m_FileSize);
		}
		M_CATCH(
		catch (CCExceptionFile)
		{
			bFailedDelete = true;
		}
		)
		
	}

	m_FileName.Clear();

	if (bFailedDelete)
		FileError_static("CByteStreamAsyncInstance::Close", "Failed closing file", 0);						

	{
		M_LOCK(m_pStreamManager->m_Lock);
		m_pStreamManager->m_OpenStreams.Remove(this);
	}
}


void CByteStreamData::CreateCachelines()
{
//		MSCOPESHORT(CByteStreamData::CreateCachelines);

	if (m_nCacheLines)
		return;

	aint NumCacheLines = m_CacheLine_Num;
	aint CacheLineSize = m_CacheLine_Size;

	if (CacheLineSize < 0)
		CacheLineSize = m_pDrive->GetCacheSize();
	if (NumCacheLines < 0)
		NumCacheLines = m_pDrive->GetCacheNum();

	if (NumCacheLines < 0)
		NumCacheLines = m_pStreamManager->GetDefaultCacheNum();

	if (m_OpenFlags & EByteStream_Write)
		NumCacheLines = 1;

	if (CacheLineSize < 0)
		CacheLineSize = m_pStreamManager->GetDefaultCacheSize();

	m_nCacheLines = NumCacheLines;
	m_CacheLine_Size = CacheLineSize;

	{
//			MSCOPE(NewCacheLines, IGNORE);
		while (NumCacheLines)
		{
			CByteStreamCacheLine *pNewCacheLine = m_pStreamManager->m_PoolCacheLines.New();
			pNewCacheLine->Create(this, CacheLineSize, m_pDrive->m_Granulartity, NULL);

			m_CacheLines.Insert(pNewCacheLine);

			--NumCacheLines;
		}
	}
}

void CByteStreamData::SeekEnd(fint _BytesFromEnd)
{
	fint FileSize = Len();

	M_ASSERT(_BytesFromEnd >= 0, "Cannot be negative");

	m_FilePos = FileSize - _BytesFromEnd;		

	if (m_FilePos < 0)
	{
		m_FilePos = 0;
		m_bGood = false;
	}

}

void CByteStreamData::SeekBeg(fint _BytesFromBeg)
{
	fint FileSize = Len();

	M_ASSERT(_BytesFromBeg >= 0, "Cannot be negative");

	m_FilePos = _BytesFromBeg;

	if (m_FilePos > FileSize && !(m_OpenFlags & EByteStream_Write))
	{
		m_FilePos = FileSize - 1;
		m_bGood = false;
	}
}

void CByteStreamData::SeekCur(fint _BytesFromCur)
{
	fint FileSize = Len();

	m_FilePos += _BytesFromCur;

	if (m_FilePos > FileSize && !(m_OpenFlags & EByteStream_Write))
	{
		m_FilePos = FileSize - 1;
		m_bGood = false;
	}
	else if (m_FilePos < 0)
	{
		m_FilePos = 0;
		m_bGood = false;
	}
}

class CCompare_ServicePrecache
{
public:
	static int Compare(void *_pContext, CByteStreamCacheLine *pFirst, CByteStreamCacheLine *pSecond)
	{
		if (pFirst->m_DataOffest > pSecond->m_DataOffest)
			return 1;
		else if (pFirst->m_DataOffest < pSecond->m_DataOffest)
			return -1;
		return 0;
	}
};



void CByteStreamData::ServicePrecache()
{
	if (!m_bAllowPrecache)
		return;

	if (m_pLastCacheLine)
	{
		MSCOPESHORT(CByteStreamData::ServicePrecache);

		// Add sorted to new temporary list
		DLinkDS_List(CByteStreamCacheLine, m_TempLink) TempList;
		DLinkDS_List(CByteStreamCacheLine, m_TempLink) TempList2;
		DLinkDS_Iter(CByteStreamCacheLine, m_CacheLineLink) IterSrc = m_PendingCacheLines;
		{
			while (IterSrc)
			{
				TempList.InsertSorted<CCompare_ServicePrecache>(IterSrc);
				++IterSrc;
			}
			
			IterSrc = m_CacheLines;
			while (IterSrc)
			{
				TempList.InsertSorted<CCompare_ServicePrecache>(IterSrc);
				++IterSrc;
			}
		}
		
		fint LastOffsetGotten = m_pLastCacheLine->m_DataOffest + m_pLastCacheLine->m_NumBytesUsed;
		fint MaxOffset = LastOffsetGotten + (m_nCacheLines - 1) * m_pLastCacheLine->m_NumBytes;
		if (MaxOffset > Len())
			MaxOffset = Len();
		{
			DLinkDS_Iter(CByteStreamCacheLine, m_TempLink) Iter = TempList;
			while (Iter)
			{
				CByteStreamCacheLine *pIter = Iter;
				++Iter;
				if (pIter == m_pLastCacheLine)
				{
				}
				else if (pIter->GetOperating())
				{
					
				}
				else if (pIter->m_bDirty)
				{
				}
				else 
				{
					if (pIter != m_pLastCacheLine && 
						(pIter->m_DataOffest < LastOffsetGotten
						||
						pIter->m_DataOffest + pIter->m_NumBytesUsed > MaxOffset
						|| 
						((pIter->m_DataOffest - LastOffsetGotten) % m_pLastCacheLine->m_NumBytes) != 0
						|| pIter->m_NumBytesUsed != m_pLastCacheLine->m_NumBytes
						))
					{
						TempList2.Insert(pIter);
					}				
				}
				
			}
		}
		
		// Rebuild list
		fint CurrentOffset = LastOffsetGotten;
		{
			while (CurrentOffset < MaxOffset)
			{
				bool Found = false;
				DLinkDS_Iter(CByteStreamCacheLine, m_TempLink) Iter = TempList;
				while (Iter)
				{
					if (Iter->m_DataOffest == CurrentOffset)
					{
						Found = true;
						break;
					}
					++Iter;
				}
				if (!Found)
				{
					CByteStreamCacheLine *pIter2 = TempList2.GetFirst();
					// failing misarably
					if (!pIter2)
						break;
					pIter2->m_TempLink.Unlink();
					PrepareCacheline(pIter2, CurrentOffset, m_pLastCacheLine->m_NumBytes, true);
					pIter2->m_bPrecache = true;
					m_bAllowPrecache = false;
				}
				
				CurrentOffset += m_pLastCacheLine->m_NumBytes;
			}
		}
	}
}

void CByteStreamData::Service()
{
	if (m_pDrive)
		m_pDrive->Service(this);
	DLinkDS_Iter(CByteStreamCacheLine, m_CacheLineLink) Iter = m_PendingCacheLines;

	while (Iter)
	{
		CByteStreamCacheLine *pToTest = Iter;
		++Iter;

		bool bWantedToThrow = false;
		if (pToTest->Done(true, bWantedToThrow))
		{
			if (pToTest->m_PendingOperation.m_pInstance)
				pToTest->m_PendingOperation.CloseInstance();
			
			pToTest->m_bWrite = false;
			pToTest->SetOperating(false);

			m_CacheLines.Insert(pToTest);
		}
	}

#ifdef PLATFORM_DOLPHIN
	GlobalCheckForReset();
#endif
}

void CByteStreamData::WriteCacheLine(CByteStreamCacheLine *_pCacheLine)
{
	CByteStreamCacheLine *pCacheLine = _pCacheLine;

	pCacheLine->m_bWrite = true;
	pCacheLine->m_bDirty = false;

	m_pDrive->AddRequest(pCacheLine);
	pCacheLine->SetOperating(true);
	m_PendingCacheLines.Insert(pCacheLine);

}

bool CByteStreamData::PrepareCacheline(CByteStreamCacheLine *_pCacheLine, fint _Start, mint _NumBytes, bool _bBlock)
{
//		MSCOPESHORT(CByteStream::PrepareCacheline2);

	CByteStreamCacheLine *pCacheLine = _pCacheLine;
	// Remove from list so noone else can screw us up
	bool bWantedToThrow = false;
	if (_bBlock)
	{
		pCacheLine->BlockUntilDone();
	}
	else if (pCacheLine->Done(true, bWantedToThrow))
	{
		Service();
	}
	else
		return false;

	M_ASSERT(!pCacheLine->GetOperating(), "");

	if (pCacheLine == m_pLastCacheLine)
		m_pLastCacheLine = NULL;

	if (pCacheLine->m_bDirty)
	{
		M_ASSERT(!pCacheLine->m_bWrite, "HoHo");
		WriteCacheLine(pCacheLine);

		if (_bBlock)
			pCacheLine->BlockUntilDone();
		else if (pCacheLine->Done(true, bWantedToThrow))
		{
			Service();
		}
		else
			return false;
	}

	pCacheLine->m_CacheLineLink.Unlink();

	pCacheLine->m_bDirty = false;
	pCacheLine->m_bPending = false;
	pCacheLine->m_bWrite = false;

	pCacheLine->m_DataOffest = (_Start / m_pDrive->m_Granulartity) * m_pDrive->m_Granulartity;
	pCacheLine->m_NumBytesUsed = (_Start - pCacheLine->m_DataOffest) + _NumBytes;
//		pCacheLine->m_NumBytesUsed = pCacheLine->m_NumBytes;
	if (pCacheLine->m_NumBytesUsed > pCacheLine->m_NumBytes)
	{
		pCacheLine->m_NumBytesUsed = pCacheLine->m_NumBytes;
	}

	if (pCacheLine->m_DataOffest >= Len() || (m_OpenFlags & CFILE_DISCARDCONTENTS))
	{
		// Off end of file
		pCacheLine->m_NumBytesUsed = pCacheLine->m_NumBytes;
		m_CacheLines.Insert(pCacheLine);
	} 
	else
	{

		if (pCacheLine->m_DataOffest + pCacheLine->m_NumBytesUsed > Len())
		{
			pCacheLine->m_NumBytesUsed = Len() - pCacheLine->m_DataOffest;
		}
		
		pCacheLine->m_NumBytesUsed = (((pCacheLine->m_NumBytesUsed - 1) / m_pDrive->m_Granulartity) + 1) * m_pDrive->m_Granulartity;
		
		pCacheLine->m_bWrite = false;

		pCacheLine->SetOperating(true);
			
		{
			MSCOPESHORT(m_pDrive->AddRequest);

			m_pDrive->AddRequest(pCacheLine);
		}
		
		m_PendingCacheLines.Insert(pCacheLine);
	}

	return true;
}

CByteStreamCacheLine *CByteStreamData::PrepareCacheline(fint _Start, mint _NumBytes, bool _bBlock)
{
//		MSCOPESHORT(CByteStream::PrepareCacheline);

	DLinkDS_Iter(CByteStreamCacheLine, m_CacheLineLink) Iter = m_CacheLines;
	// First try to find an empty cache line
	while (Iter)
	{
		if (Iter->m_NumBytesUsed == 0)
		{
			break;
		}
		++Iter;
	}
	
	while (!Iter)
	{
		if (!_bBlock)
			return NULL;
		{
			//				MSCOPESHORT(CByteStreamCacheLine::m_PendingCacheLines.IsEmpty);
			while (!m_PendingCacheLines.IsEmpty())
			{
				Service();
				// Wait until nothing is pending
				MRTC_SystemInfo::OS_Sleep(0);
			}
		}
		Iter = m_CacheLines;
	}
	
	CByteStreamCacheLine *pCacheLine = Iter;
	pCacheLine->m_bPrecache = false;

	if (PrepareCacheline(pCacheLine, _Start, _NumBytes, _bBlock))
		return pCacheLine;
	else
		return NULL;
}

void CByteStreamData::SetLastCacheLine(CByteStreamCacheLine *_pLastCacheLine)
{
	if (_pLastCacheLine->m_bPrecache)
	{
		_pLastCacheLine->m_bPrecache = false;
		m_bAllowPrecache = true;
	}
	if (m_pLastCacheLine && m_pLastCacheLine != _pLastCacheLine && m_pLastCacheLine->m_bDirty)
	{
		M_ASSERT(!m_pLastCacheLine->m_bWrite, "HoHo");
		WriteCacheLine(m_pLastCacheLine);
	//	m_pLastCacheLine->BlockUntilDone();
	}

	m_pLastCacheLine = _pLastCacheLine;

	if (m_BytesRead > 512)
		ServicePrecache();
}


void CByteStreamData::Read(void *_pData, mint _nBytes)
{
	if (!_nBytes)
		return;

#ifndef M_RTM

	XDFRecordSeek();

	if (m_pStreamManager->m_bLogData && m_LastReadFilePos >= 0 && m_LastReadFilePos != m_FilePos && !(m_OpenFlags & EByteStream_NoLog))
	{
		M_LOCK(m_pStreamManager->m_Lock);
		m_pStreamManager->m_spLogFile->Log(CFStrF("S %9d Seek In File %s", (int)(m_FilePos - m_LastReadFilePos), m_FileName.Str()).Str());
	}
#endif

	MSCOPESHORT(CByteStreamData::Read);
	if (!m_nCacheLines)
		CreateCachelines();

	fint FileLen = Len();
	// Try to find a cache line with the specified data in it
	if ((m_FilePos + _nBytes) > FileLen)
	{
		_nBytes = FileLen - m_FilePos;
		m_bGood = false;
	}

	if (m_FilePos == FileLen)
	{
		m_bGood = false;
	}

	while (_nBytes)
	{			
		// First check if we can find it in the cache
		{
			CByteStreamCacheLine *pCacheLine = GetCacheLine(m_FilePos, _nBytes);

			if (!pCacheLine)
			{
				m_bGood = false;
				return;
			}
			
			
			mint NumBytes = _nBytes;

			fint Offset = (m_FilePos - pCacheLine->m_DataOffest);
			aint CanCopy = pCacheLine->m_NumBytesUsed - Offset;

			if (NumBytes > CanCopy)
				NumBytes = CanCopy;

			memcpy(_pData, ((uint8 *)pCacheLine->GetCacheLineData()) + Offset, NumBytes);

			*((uint8 **)&_pData) += NumBytes;
			m_FilePos += NumBytes;
//#ifndef M_RTM
			m_BytesRead += NumBytes;
//#endif
			_nBytes -= NumBytes;

			// Put last in list so this cache line will be left the longest amount of time
//				pCacheLine->m_CacheLineList.LinkLast(m_CacheLines, (CByteStreamCacheLine *)pCacheLine);
		}

	}

#ifndef M_RTM
	XDFRecordRead();

	m_LastReadFilePos = m_FilePos;
	m_LastReadFilePosXDF = m_FilePos;
#endif
	
}

void CByteStreamData::Write(const void *_pData, mint _nBytes)
{
	if (!_nBytes)
		return;
	
	if (!m_nCacheLines)
		CreateCachelines();
	MSCOPESHORT(CByteStreamData::Write);

	fint FileLen = Len();
//	fint OrgFileLen = FileLen;
	fint NewFileSize = m_FileSize;
	// Try to find a chache line with the specified data in it
	if ((m_FilePos + _nBytes) > FileLen)
	{
		NewFileSize = m_FilePos + _nBytes;
	}

	while (_nBytes)
	{			
		// First check if we can find it in the cache
		{
			CByteStreamCacheLine *pCacheLine = GetCacheLine(m_FilePos, _nBytes);

			
			M_ASSERT(!pCacheLine->GetOperating(), "");
			M_ASSERT(!pCacheLine->GetPending(), "");
			bool bWantThrow = false;
			M_ASSERT(pCacheLine->Done(false, bWantThrow), "");

		/*	if ((m_FilePos + pCacheLine->m_NumBytesUsed) >= OrgFileLen && pCacheLine->m_NumBytesUsed < pCacheLine->m_NumBytes)
			{
				// Expand size of block so we can cache more data in it
				pCacheLine->m_NumBytesUsed = pCacheLine->m_NumBytes;
				pCacheLine->m_NumBytesUsed = (((pCacheLine->m_NumBytesUsed - 1) / m_pDrive->m_Granulartity) + 1) * m_pDrive->m_Granulartity;
			}*/

			if (!pCacheLine)
			{
				m_bGood = false;
				return;
			}

			pCacheLine->m_bDirty = true;				
			
			mint NumBytes = _nBytes;

			fint Offset = (m_FilePos - pCacheLine->m_DataOffest);
			M_ASSERT(Offset >= 0, "");

			aint CanCopy = pCacheLine->m_NumBytesUsed - Offset;

			if (NumBytes > CanCopy)
				NumBytes = CanCopy;

			memcpy(((uint8 *)pCacheLine->GetCacheLineData()) + Offset, _pData, NumBytes);

			*((uint8 **)&_pData) += NumBytes;
			m_FilePos += NumBytes;
			m_BytesRead += NumBytes;
			_nBytes -= NumBytes;

			// Put last in list so this cache line will be left the longest amount of time
//				pCacheLine->m_CacheLineList.LinkLast(m_CacheLines, (CByteStreamCacheLine *)pCacheLine);
		}

	}

	m_FileSize = NewFileSize;
#ifndef M_RTM
	m_LastReadFilePos = m_FilePos;
#endif
	
}


bool CByteStreamData::Read(CAsyncRequest *_pRequest)
{
	if (_pRequest->Done())
		return true;

	if (_pRequest->m_bInMemory)
	{	
		if (!_pRequest->m_pCacheLine)
		{
			_pRequest->m_pCacheLine = m_pStreamManager->m_PoolCacheLines.New();
			_pRequest->m_pCacheLine->m_bOperating = false;
		}

		if (_pRequest->m_pCacheLine->GetOperating())
		{
			M_ASSERT(_pRequest->m_pCacheLine->m_NumBytes == _pRequest->m_nBytes, "The cacheline has to be done here !!!");
			M_ASSERT(_pRequest->m_pCacheLine->m_DataOffest == _pRequest->m_StartPos, "The cacheline has to be done here !!!");
			M_ASSERT(_pRequest->m_pCacheLine->m_pAlignedCacheLine == _pRequest->m_pBuffer, "The cacheline has to be done here !!!");
			m_pDrive->Service(this);
			bool bWantedToThrow = false;
			if (_pRequest->m_pCacheLine->Done(true, bWantedToThrow))
			{
				_pRequest->m_pCacheLine->SetOperating(false);
				_pRequest->m_pCacheLine->Destroy();				
				m_pStreamManager->m_PoolCacheLines.Delete(_pRequest->m_pCacheLine);
				_pRequest->m_pCacheLine = NULL;
				_pRequest->m_nBytesCopied = _pRequest->m_nBytes;
			}
		}
		else
		{
			_pRequest->m_pCacheLine->Create(this, _pRequest->m_nBytes, m_pDrive->m_Granulartity, _pRequest->m_pBuffer);

			_pRequest->m_pCacheLine->m_DataOffest = _pRequest->m_StartPos;
			_pRequest->m_pCacheLine->m_NumBytesUsed = _pRequest->m_nBytes;
			_pRequest->m_pCacheLine->m_NumBytes = _pRequest->m_nBytes;
			_pRequest->m_pCacheLine->m_bDirty = false;
			_pRequest->m_pCacheLine->m_bWrite = false;
			_pRequest->m_pCacheLine->m_bPending = false;
			_pRequest->m_pCacheLine->m_bOperating = true;
			m_pDrive->AddRequest(_pRequest->m_pCacheLine);
			m_pDrive->Service(this);
		}

		return _pRequest->Done();

	}
	else
	{
		MSCOPESHORT(CByteStreamData::Read);

		if (!m_nCacheLines)
			CreateCachelines();

		fint FileLen = Len();
		mint nBytes = _pRequest->m_nBytes - _pRequest->m_nBytesCopied;
		fint FilePos = (_pRequest->m_StartPos + _pRequest->m_nBytesCopied);
		uint8 *pData = (uint8 *)_pRequest->m_pBuffer + _pRequest->m_nBytesCopied;
		// Try to find a cache line with the specified data in it
		if ( (FilePos + nBytes) > FileLen)
		{
			nBytes = FileLen - FilePos;
			m_bGood = false;
		}

		if (m_FilePos == FileLen)
		{
			m_bGood = false;
		}

		while (nBytes)
		{			
			// First check if we can find it in the cache
			{
				mint CacheLineBytes = Max((aint)nBytes, m_CacheLine_Size);
				CByteStreamCacheLine *pCacheLine = GetCacheLine(FilePos, CacheLineBytes, false);

				if (!pCacheLine || !pData)
				{
					return false;
				}
								
				mint NumBytes = nBytes;

				fint Offset = (FilePos - pCacheLine->m_DataOffest);
				aint CanCopy = pCacheLine->m_NumBytesUsed - Offset;

				if (NumBytes > CanCopy)
					NumBytes = CanCopy;

				memcpy(pData, ((uint8 *)pCacheLine->GetCacheLineData()) + Offset, NumBytes);

				pData += NumBytes;
				_pRequest->m_nBytesCopied += NumBytes;
				FilePos += NumBytes;
				nBytes -= NumBytes;
			}

		}

		return _pRequest->Done();
	}

}

bool CByteStreamData::Write(CAsyncRequest *_pRequest)
{
	if (_pRequest->Done())
		return true;
	
	if (_pRequest->m_bInMemory)
	{	
		if (!_pRequest->m_pCacheLine)
		{
			_pRequest->m_pCacheLine = m_pStreamManager->m_PoolCacheLines.New();
			_pRequest->m_pCacheLine->m_bOperating = false;
		}

		if (_pRequest->m_pCacheLine->GetOperating())
		{
			M_ASSERT(_pRequest->m_pCacheLine->m_NumBytes == _pRequest->m_nBytes, "The cacheline has to be done here !!!");
			M_ASSERT(_pRequest->m_pCacheLine->m_DataOffest == _pRequest->m_StartPos, "The cacheline has to be done here !!!");
			M_ASSERT(_pRequest->m_pCacheLine->m_pAlignedCacheLine == _pRequest->m_pBuffer, "The cacheline has to be done here !!!");
			m_pDrive->Service(this);
			bool bWantedToThrow = 0;
			if (_pRequest->m_pCacheLine->Done(true, bWantedToThrow))
			{
				_pRequest->m_pCacheLine->SetOperating(false);
				_pRequest->m_pCacheLine->Destroy();				
				m_pStreamManager->m_PoolCacheLines.Delete(_pRequest->m_pCacheLine);
				_pRequest->m_pCacheLine = NULL;
				_pRequest->m_nBytesCopied = _pRequest->m_nBytes;
			}
		}
		else
		{
			_pRequest->m_pCacheLine->Create(this, _pRequest->m_nBytes, m_pDrive->m_Granulartity, _pRequest->m_pBuffer);

			_pRequest->m_pCacheLine->m_DataOffest = _pRequest->m_StartPos;
			_pRequest->m_pCacheLine->m_NumBytesUsed = _pRequest->m_nBytes;
			_pRequest->m_pCacheLine->m_NumBytes = _pRequest->m_nBytes;
			_pRequest->m_pCacheLine->m_bDirty = false;
			_pRequest->m_pCacheLine->m_bWrite = true;
			_pRequest->m_pCacheLine->m_bPending = false;
			_pRequest->m_pCacheLine->m_bOperating = true;
			m_pDrive->AddRequest(_pRequest->m_pCacheLine);
			m_pDrive->Service(this);
		}

		return _pRequest->Done();

	}
	else
	{
		if (!m_nCacheLines)
			CreateCachelines();
		MSCOPESHORT(CByteStreamData::Write);

		fint FileLen = Len();
//		fint OrgFileLen = FileLen;
		fint NewFileSize = m_FileSize;
		fint FilePos = (_pRequest->m_StartPos + _pRequest->m_nBytesCopied);
		mint nBytes = _pRequest->m_nBytes - _pRequest->m_nBytesCopied;
		uint8 *pData = (uint8 *)_pRequest->m_pBuffer + _pRequest->m_nBytesCopied;
		// Try to find a cache line with the specified data in it
		if ((FilePos + nBytes) > FileLen)
		{
			NewFileSize = FilePos + nBytes;
		}

		while (nBytes)
		{			
			// First check if we can find it in the cache
			{
				CByteStreamCacheLine *pCacheLine = GetCacheLine(FilePos, nBytes, false);

				if (!pCacheLine)
				{
					return false;
				}
				
				M_ASSERT(!pCacheLine->GetOperating(), "");
				M_ASSERT(!pCacheLine->GetPending(), "");
				bool bWantThrow = false;
				M_ASSERT(pCacheLine->Done(false, bWantThrow), "");

				pCacheLine->m_bDirty = true;				
				
				mint NumBytes = nBytes;

				fint Offset = (FilePos - pCacheLine->m_DataOffest);
				M_ASSERT(Offset >= 0, "");

				aint CanCopy = pCacheLine->m_NumBytesUsed - Offset;

				if (NumBytes > CanCopy)
					NumBytes = CanCopy;

				memcpy(((uint8 *)pCacheLine->GetCacheLineData()) + Offset, pData, NumBytes);

				pData += NumBytes;
				_pRequest->m_nBytesCopied += NumBytes;
				FilePos += NumBytes;
				nBytes -= NumBytes;

			}

		}

		m_FileSize = NewFileSize;
	}

	return _pRequest->Done();		
}

bool CByteStreamData::AsyncFlush(bool _bBlock)
{
	m_pLastCacheLine = NULL;
	{			
		DLinkDS_Iter(CByteStreamCacheLine, m_CacheLineLink) Iter = m_CacheLines;
		while (Iter)
		{
			CByteStreamCacheLine *_pToDelete = Iter;
			++Iter;
			if (_pToDelete->m_bDirty && !_pToDelete->m_bOperating)
			{
				M_ASSERT(!_pToDelete->m_bWrite, "HoHo");
#ifndef PLATFORM_CONSOLE
				aint RealLen = (_pToDelete->m_DataOffest + _pToDelete->m_NumBytesUsed) - m_FileSize;
				if (RealLen > 0)
				{
					memset((uint8 *)_pToDelete->m_pAlignedCacheLine + (_pToDelete->m_NumBytes - RealLen), 32, RealLen);

				}
#endif
				WriteCacheLine(_pToDelete);
#ifndef PLATFORM_CONSOLE
				if (RealLen > 0)
				{
					char Temp[2 + sizeof(void *)];
					Temp[0] = 13;
					*((void **)(Temp + 1)) = m_pFile;
					Temp[sizeof(void *) + 1] = 0;
					MRTC_SystemInfo::OS_FileSetFileSize(Temp, m_FileSize);
				}
#endif
			}
		}
	}


	do
	{
		Service();
		if (m_PendingCacheLines.GetFirst())
		{
			if (!_bBlock)
				return false;
			else
				MRTC_SystemInfo::OS_Sleep(1);
		}
		else
			break;

	}
	while (_bBlock);

	return true;
}

void CCacheMem::SetCacheSize(aint _CacheSize, aint _Granularity)
{
	if (m_pMemory)
		MRTC_GetMemoryManager()->Free(m_pMemory);

#ifdef PLATFORM_DOLPHIN
	m_pMemory = (uint8 *)MRTC_GetMemoryManager()->AllocAlign(_CacheSize, 32);
#elif defined( PLATFORM_PS2 )
	m_pMemory = (uint8 *)MRTC_GetMemoryManager()->AllocAlign(_CacheSize, 64);
#else
	m_pMemory = (uint8 *)M_ALLOC(_CacheSize);
#endif
}

CCacheMem *CByteStreamDrive::GetCacheMem(aint _CacheLineSize)
{
	if (!MRTC_GetObjectManager()->InMainThread())
		return NULL;

	M_LOCK(m_Lock);

	if (_CacheLineSize != m_CacheSizeUsed)
		return NULL;

	for (int i = 0; i < m_lCacheMem.Len(); ++i)
	{
		if (!m_lCacheMem[i]->m_bUsed)
		{
			m_lCacheMem[i]->m_bUsed = true;
			return m_lCacheMem[i];
		}
	}		

	return NULL;
}

bool CByteStreamDrive::Service(class CByteStreamData *_pTrowOnStream)
{
	M_LOCK(m_Lock);

	bool bThrew = false;
	int NumFree = 2;
	DLinkDS_Iter(CByteStreamCacheLine, m_PendingLink) Iter = m_PendingOpr;
	while (Iter)
	{
		CByteStreamCacheLine *pCurrentCacheLine = Iter;
		++Iter;
		if (pCurrentCacheLine->Done(pCurrentCacheLine->m_pStream == _pTrowOnStream, bThrew))
		{
			// Remove from pending list
			pCurrentCacheLine->m_PendingLink.Unlink();
		}
		else
			--NumFree;
	}	

	M_ASSERT(NumFree >= 0, "!");

	while (NumFree)
	{
		CByteStreamCacheLine *pFirst = m_RequestsSorted.Pop();

		if (pFirst)
		{	
			CByteStreamCacheLine *pChangeLine = pFirst;
			m_PendingOpr.Insert(pChangeLine);
			{
				MSCOPESHORT(pChangeLine->PerformOperation);
				pChangeLine->PerformOperation();
			}
		}
		--NumFree;
	}

	{
		DLinkDS_Iter(CByteStreamCacheLine, m_PendingLink) Iter = m_PendingCacheLinesForDelete;
		while (Iter)
		{
			CByteStreamCacheLine *pCacheLine = Iter;
			++Iter;

			if (pCacheLine->m_PendingOperation.Done(pCacheLine->m_pStream == _pTrowOnStream, bThrew))
			{
				pCacheLine->m_PendingOperation.CloseInstance();
/*				{
					DLock(pCacheLine->m_pStream->m_Lock);
					pCacheLine->m_CacheLineLink.Unlink();
					pCacheLine->m_DriveLink.Unlink();
					pCacheLine->m_PendingLink.Unlink();
				}*/
				
				GetStreamManager()->m_PoolCacheLines.Delete(pCacheLine);
			}
		}
	}

	return bThrew;
}

CByteStreamCacheLine *CByteStreamData::GetCacheLine_NotINL(fint _Start, mint _NumBytes, bool _bBlock)
{
	while (1)
	{
		bool bWantedToThrow = false;

		if (m_pLastCacheLine)
		{
			M_ASSERT(m_pLastCacheLine->m_CacheLineLink.IsInList(), "nono");
			M_ASSERT(m_pLastCacheLine->Done(false, bWantedToThrow), "");
			M_ASSERT(!m_pLastCacheLine->GetOperating(), "");
			if (_Start >= m_pLastCacheLine->m_DataOffest && _Start < (m_pLastCacheLine->m_DataOffest + m_pLastCacheLine->m_NumBytesUsed))
			{
				return m_pLastCacheLine;
			}
		}

		Service();
		DLinkDS_Iter(CByteStreamCacheLine, m_CacheLineLink) Iter = m_CacheLines;

		while (Iter)
		{
			CByteStreamCacheLine *pIter = Iter;
			++Iter;
			if (_Start >= pIter->m_DataOffest && _Start < (pIter->m_DataOffest + pIter->m_NumBytesUsed))
			{
				SetLastCacheLine(pIter);
				return pIter;
			}
		}

		// Check if its in the pending cache lines, then we need to wait for it
		Iter = m_PendingCacheLines;

		while (Iter)
		{
			CByteStreamCacheLine *pIter = Iter;
			++Iter;
			if (_Start >= pIter->m_DataOffest && _Start < (pIter->m_DataOffest + pIter->m_NumBytesUsed))
			{
				CByteStreamCacheLine *pCacheLine = pIter;

				if (_bBlock)
					pCacheLine->BlockUntilDone();
				else if (pCacheLine->Done(true, bWantedToThrow))
				{
					Service();
				}
				else
					return NULL;

				SetLastCacheLine(pCacheLine);
				return pCacheLine;
			}
			
		}

		m_bAllowPrecache = true;
		CByteStreamCacheLine *pCacheLine = PrepareCacheline(_Start, _NumBytes, _bBlock);

		if (!pCacheLine)
			return NULL;

	}
}

void CByteStreamManager::AddCorrupt(int _Corrupt)
{
	uint32 Corre;
	{
		M_LOCK(m_Lock);
		m_Corrupt |= _Corrupt;
		Corre = m_Corrupt;
	}
#if defined(PLATFORM_XBOX1) && !defined(M_DEMO_XBOX)
	if (Corre) // & DISKUTIL_STATUS_CORRUPTANY
	{
		try 
		{
			if (!CDiskUtil::FileExists("Z:\\FormatZ"))
			{
				static bool bTryOnce = true;
				if (bTryOnce)
				{
					bTryOnce = false;
					CCFile File;
					File.Open("Z:\\FormatZ", CFILE_BINARY | CFILE_WRITE);
//					File.WriteLE((uint8)1);
					File.Close();
				}
			}
		}
		catch (CCException)
		{
		}
	}
#endif
}


#ifdef	NO_INLINE_FILESYSTEM
#undef	M_INLINE
#define	M_INLINE
#include "MFile_StreamMgr.inl"
#endif	// PLATFORM_PS2
