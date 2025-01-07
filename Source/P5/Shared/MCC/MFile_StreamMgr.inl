
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/

#ifdef PLATFORM_DOLPHIN
extern void GlobalCheckForReset();
#endif

M_INLINE CByteStream::CByteStream()
{
	m_pData = GetStreamManager()->m_PoolByteStreamData.New();
}

M_INLINE CByteStream::~CByteStream()
{
	if (m_pData)
		GetStreamManager()->m_PoolByteStreamData.Delete(m_pData);
}

M_INLINE bool CByteStream::Create(const CStr &_FileName, int _Flags, float _Priority, aint _NumCacheLines, aint _CacheLineSize )
{
	return m_pData->Create(_FileName, _Flags, _Priority, _NumCacheLines, _CacheLineSize);
}

M_INLINE bool CByteStream::Close()
{
	m_pData->Close();
	
	return true;
}

M_INLINE bool CByteStream::Good()
{
	return m_pData->Good_Get();	
}

M_INLINE void CByteStream::ClearFlags()
{
	m_pData->Good_Clear();
}

M_INLINE bool CByteStream::EndOfFile()
{		
	return m_pData->EndOfFile();
}

M_INLINE void CByteStream::SeekEnd(fint _BytesFromEnd)
{
	m_pData->SeekEnd(_BytesFromEnd);
}

M_INLINE void CByteStream::SeekBeg(fint _BytesFromBeg)
{
	m_pData->SeekBeg(_BytesFromBeg);
}

M_INLINE void CByteStream::SeekCur(fint _BytesFromCur)
{
	m_pData->SeekCur(_BytesFromCur);
}

M_INLINE fint CByteStream::Len()
{	
	return m_pData->Len();
}

M_INLINE void CByteStream::SetPriority(fp4 _Priority)
{
	m_pData->SetPriority(_Priority);
}


M_INLINE fint CByteStream::Pos()
{	
	return m_pData->Pos();
}

M_INLINE void CByteStream::Read(void *_pData, mint _nBytes)
{
	m_pData->Read(_pData, _nBytes);
}

M_INLINE void CByteStream::Write(const void *_pData, mint _nBytes)
{
	m_pData->Write(_pData, _nBytes);
}

// Call repeatidly until true is returned
M_INLINE bool CByteStream::Read(CAsyncRequest *_pRequest)
{
	return m_pData->Read(_pRequest);
}

// Call repeatidly until true is returned
M_INLINE bool CByteStream::Write(CAsyncRequest *_pRequest)
{		
	return m_pData->Write(_pRequest);
}

M_INLINE bool CByteStream::AsyncFlush(bool _bBlock)
{		
	return m_pData->AsyncFlush(_bBlock);
}

M_INLINE void CByteStream::SetCacheSize_Dirve(const char *_pDrive, aint _CacheSize)
{
	GetStreamManager()->SetDriveCacheSize(_pDrive, _CacheSize);
}

M_INLINE void CByteStream::SetCacheSize_Default(aint _CacheSize)
{
	GetStreamManager()->SetDefaultCacheSize(_CacheSize);
}

M_INLINE void CByteStream::SetCacheNumber_Drive(const char *_pDrive, aint _nCaches)
{
	GetStreamManager()->SetDriveCacheNum(_pDrive, _nCaches);
}

M_INLINE void CByteStream::SetCacheNumber_Default(aint _nCaches)
{
	GetStreamManager()->SetDefaultCacheNum(_nCaches);
}

M_INLINE void CByteStream::XDF_Record(const char* _pName, const char* _pBasePath)
{
	GetStreamManager()->XDF_Record(_pName, _pBasePath);
}

M_INLINE void CByteStream::XDF_Use(const char* _pName, const char* _pBasePath)
{
	GetStreamManager()->XDF_Use(_pName, _pBasePath);
}

M_INLINE CStr CByteStream::XDF_GetCurrent()
{
	return GetStreamManager()->XDF_GetCurrent();
}

M_INLINE fint CByteStream::XDF_GetPosition()
{
	return GetStreamManager()->XDF_GetPosition();
}

M_INLINE void CByteStream::XDF_Stop()
{
	GetStreamManager()->XDF_Stop();
}

M_INLINE void CByteStream::XDF_Pause()
{
	GetStreamManager()->XDF_Pause();
}

M_INLINE void CByteStream::XDF_Resume()
{
	GetStreamManager()->XDF_Resume();
}

M_INLINE bint CByteStream::XDF_IsPasued()
{
	return GetStreamManager()->XDF_IsPasued();
}

M_INLINE CXDF *CByteStream::XDF_GetUse()
{
	return GetStreamManager()->XDF_GetUse();
}

M_INLINE CXDF *CByteStream::XDF_GetRecord()
{
	return GetStreamManager()->XDF_GetRecord();
}

M_INLINE void CByteStream::SearchPath_Add(CStr _Path)
{	
	GetStreamManager()->SearchPath_Add(_Path);
}

M_INLINE void CByteStream::SearchPath_SetBase(CStr _Path)
{
	GetStreamManager()->SearchPath_SetBase(_Path);
}

M_INLINE CStr CByteStream::SearchPath_GetBase()
{
	return GetStreamManager()->SearchPath_GetBase();
}

M_INLINE int CByteStream::SearchPath_GetNumPaths()
{	
	return GetStreamManager()->SearchPath_GetNumPaths();
}

M_INLINE CStr CByteStream::SearchPath_GetPath(int _iPath)
{	
	return GetStreamManager()->SearchPath_GetPath(_iPath);
}

M_INLINE void CByteStream::AddCorrupt(int _Corrupt)
{
	GetStreamManager()->AddCorrupt(_Corrupt);
}

M_INLINE void CByteStream::RemoveCorrupt(int _Corrupt)
{
	GetStreamManager()->RemoveCorrupt(_Corrupt);
}

M_INLINE int CByteStream::GetCorrupt()
{
	return GetStreamManager()->GetCorrupt();
}

//#################################################################################
//#################################################################################
//#################################################################################

M_INLINE void CByteStreamManager::Service()
{

}

M_INLINE CByteStreamDrive *CByteStreamManager::GetDrive(CStr &_Drive)
{
	M_LOCK(m_Lock);

	CByteStreamDrive *pDrive = m_DriveHash.GetTyped(&_Drive);

	if (!pDrive)
	{
		pDrive = CreateDrive(_Drive);
	}

	return pDrive;
}

M_INLINE void CByteStreamManager::RemoveCorrupt(int _Corrupt)
{
	M_LOCK(m_Lock);
	m_Corrupt &= ~_Corrupt;
}

M_INLINE int CByteStreamManager::GetCorrupt()
{
	M_LOCK(m_Lock);
	return m_Corrupt;
}

M_INLINE void CByteStreamManager::SetDefaultCacheSize(aint _Value)
{
	M_LOCK(m_Lock);
	m_DefaultCacheSize = _Value;
}

M_INLINE void CByteStreamManager::SetDefaultCacheNum(aint _Value)
{
	M_LOCK(m_Lock);
	m_DefaultNumCaches = _Value;
}

M_INLINE mint CByteStreamManager::GetDefaultCacheSize()
{
	M_LOCK(m_Lock);
	return m_DefaultCacheSize;
}

M_INLINE aint CByteStreamManager::GetDefaultCacheNum()
{
	M_LOCK(m_Lock);
	return m_DefaultNumCaches;
}

M_INLINE CStr CByteStreamManager::SearchPath_GetBase()
{
	M_LOCK(m_Lock);
	return m_SearchPathBase;
}

M_INLINE int CByteStreamManager::SearchPath_GetNumPaths()
{	
	M_LOCK(m_Lock);
	return m_SearchPaths.Len();
}

M_INLINE CStr CByteStreamManager::SearchPath_GetPath(int _iPath)
{	
	M_LOCK(m_Lock);
	return m_SearchPaths[_iPath];
}


//#################################################################################
//#################################################################################
//#################################################################################

M_INLINE CByteStreamData::CByteStreamData()
{
	m_pStreamManager = GetStreamManager();
	m_pDrive = NULL;
	m_Priority = 0;
	m_pFile = NULL;
	m_bGood = true;
	m_FileSize = -1;
	m_FilePos = 0;
	m_pLastCacheLine = NULL;
	m_nCacheLines = 0;
	m_OpenFlags = 0;
	m_bAllowPrecache = true;
}

M_INLINE CByteStreamData::~CByteStreamData()
{
	Close();
}


M_INLINE bool CByteStreamData::Good_Get()
{
	return m_bGood;
}

M_INLINE void CByteStreamData::Good_Clear()
{
	m_bGood = true;
}

M_INLINE bool CByteStreamData::EndOfFile()
{
	bool bIsEOF = m_FilePos >= Len();

	return bIsEOF;
}


M_INLINE fint CByteStreamData::Len()
{
	fint FileSize;
	if (m_FileSize >= 0)
		FileSize = m_FileSize;
	else
		FileSize = MRTC_SystemInfo::OS_FileSize(m_pFile);

	return FileSize;
}

M_INLINE fint CByteStreamData::Pos()
{
	fint Pos;

	Pos = m_FilePos;

	return Pos;
}


M_INLINE CByteStreamCacheLine *CByteStreamData::GetCacheLine(fint _Start, mint _NumBytes, bool _bBlock )
{		
	if (m_pLastCacheLine)
	{
		M_ASSERT(m_pLastCacheLine->m_CacheLineList.IsInList(), "nono");
		bool bWantThrow = false;
		M_ASSERT(m_pLastCacheLine->Done(false, bWantThrow), "");
		M_ASSERT(!m_pLastCacheLine->GetOperating(), "");
		if (_Start >= (fint)m_pLastCacheLine->m_DataOffest && _Start < (fint)(m_pLastCacheLine->m_DataOffest + m_pLastCacheLine->m_NumBytesUsed))
		{
			return m_pLastCacheLine;
		}
	}

	return GetCacheLine_NotINL(_Start, _NumBytes, _bBlock);
}


//#################################################################################
//#################################################################################
//#################################################################################

M_INLINE CAsyncRequest::CAsyncRequest()
{
	m_StartPos = 0;
	m_nBytesCopied = 0;
	m_nBytes = 0;
	m_pBuffer = NULL;
	m_pCacheLine = NULL;
}

M_INLINE CAsyncRequest::~CAsyncRequest()
{
	M_ASSERT(!m_pCacheLine, "Memory leak");
}

M_INLINE void CAsyncRequest::SetRequest(void *_pBuffer, fint _StartPos, mint _nBytes, bool _bInMemory)
{
	m_StartPos = _StartPos;
	m_nBytesCopied = 0;
	m_nBytes = _nBytes;
	m_pBuffer = _pBuffer;
	m_bInMemory = _bInMemory;
}

M_INLINE bool CAsyncRequest::Done()
{
	bool bDone = m_nBytes == m_nBytesCopied;
	return bDone;
}

//#################################################################################
//#################################################################################
//#################################################################################

M_INLINE CByteStreamAsyncInstance::CByteStreamAsyncInstance()
{
	m_pInstance = NULL;
	m_bIsDone = false;
	m_BytesToProcess = 0;
}

M_INLINE CByteStreamAsyncInstance::~CByteStreamAsyncInstance()
{
	CloseInstance();
}

M_INLINE bool CByteStreamAsyncInstance::Done(bool _bThrows, bool &_bWantedToThrow)
{
	if (m_bIsDone)
		return true;
	else
	{
		if (m_pInstance)
		{
			M_TRY
			{
				m_bIsDone = MRTC_SystemInfo::OS_FileAsyncIsFinished(m_pInstance);
				if (m_bIsDone)
				{
					mint ToProcess = MRTC_SystemInfo::OS_FileAsyncBytesProcessed(m_pInstance);
					if (ToProcess < m_BytesToProcess)
					{
						FileError_static("CByteStreamAsyncInstance::Done", "Read bytes diffrent from requested", 0);						
					}
				}
				return m_bIsDone != 0;
			}
			M_CATCH(				
			catch (CCExceptionFile)
			{
				_bWantedToThrow = true;
				if (_bThrows)
					throw;

				m_bIsDone = false;
				return m_bIsDone != 0;
			}
			)
		}
		else 
			return true;
	}
}

M_INLINE void CByteStreamAsyncInstance::CloseInstance()
{
	if (m_pInstance)
		MRTC_SystemInfo::OS_FileAsyncClose(m_pInstance);
	m_pInstance = NULL;
}

//#################################################################################
//#################################################################################
//#################################################################################

M_INLINE CCacheMem::CCacheMem()
{
	m_pMemory = NULL;
	m_bUsed = false;
}

M_INLINE CCacheMem::~CCacheMem()
{
	if (m_pMemory)
		MRTC_GetMemoryManager()->Free(m_pMemory);
}

//#################################################################################
//#################################################################################
//#################################################################################


M_INLINE void CByteStreamDrive::ReturnCacheMem(CCacheMem *_pCacheMem)
{
	M_LOCK(m_Lock);
	_pCacheMem->m_bUsed = false;
}

M_INLINE aint CByteStreamDrive::GetCacheSize()
{
	M_LOCK(m_Lock);
	return m_CacheSize;
}

M_INLINE aint CByteStreamDrive::GetCacheNum()
{
	M_LOCK(m_Lock);
	return m_NumCaches;
}

M_INLINE void CByteStreamDrive::AddRequest(CByteStreamCacheLine *_pCacheLine)
{
	{
		M_LOCK(m_Lock);
		_pCacheLine->m_Prio = _pCacheLine->m_pStream->m_Priority;
		_pCacheLine->SetPending(true);
		_pCacheLine->m_DriveList.LinkSortedStartTail(&m_Requests, _pCacheLine);
	}
	Service(_pCacheLine->m_pStream);
}


//#################################################################################
//#################################################################################
//#################################################################################
