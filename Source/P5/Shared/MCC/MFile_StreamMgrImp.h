
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/


#include "MMemMgrHeap.h"
#include "MFile_Misc.h"

static class CByteStreamManager *GetStreamManager();

#define D_DefaultCacheSize 65536
#define D_DefaultNumCaches 4

class MCCDLLEXPORT CByteStreamAsyncInstance
{
public:
	void *m_pInstance;
	mint m_BytesToProcess;
	int m_bIsDone;

	CByteStreamAsyncInstance();
	~CByteStreamAsyncInstance();
	bool Done(bool _bThrows, bool &_bWantedToThrow);
	void CloseInstance();
};

/*class MCCDLLEXPORT CByteStreamDriveSortList
{
public:
	int Compare(void *_pFirst, void *_pSecond);
};*/


class MCCDLLEXPORT CCacheMem : public CReferenceCount
{
public:
	uint8 *m_pMemory;
	volatile bool m_bUsed;
	CCacheMem();
	~CCacheMem();
	void SetCacheSize(aint _CacheSize, aint _Granularity);
};

//public CDA_Referable, 
class MCCDLLEXPORT CByteStreamCacheLine
{
public:
	MRTC_CriticalSection m_Lock;
	void *m_pAlignedCacheLine;
	class CByteStreamData *m_pStream;
	class CByteStreamDrive *m_pDrive;

	void Create(class CByteStreamData *_pStream, aint _CacheLineSize, aint _Granulatiry, void *_pCacheLine);

	~CByteStreamCacheLine()
	{

		Destroy();
	}

	void Destroy();

	DLinkDS_Link(CByteStreamCacheLine, m_CacheLineLink);
	DLinkDS_Link(CByteStreamCacheLine, m_DriveLink);
	DLinkDS_Link(CByteStreamCacheLine, m_PendingLink);

	DLinkDS_Link(CByteStreamCacheLine, m_TempLink);

	CByteStreamAsyncInstance m_PendingOperation;
	CCacheMem *m_pCacheMemUsed;

	fint m_DataOffest;
	mint m_NumBytesUsed;
	mint m_NumBytes;
	fp32 m_Prio;
	bool m_bDirty;
	bool m_bWrite;
	bool m_bPending;
	bool m_bOperating;
	bool m_bOwnCacheLine;
	bool m_bPrecache;

	M_INLINE void *GetCacheLineData()
	{
		return m_pAlignedCacheLine;
	}

	M_INLINE void SetPending(bool _bPending)
	{
		M_LOCK(m_Lock);
		M_ASSERT(m_bPending != _bPending,"");
		m_bPending = _bPending;
	}

	M_INLINE bool GetPending()
	{
		M_LOCK(m_Lock);
		return m_bPending;
	}

	M_INLINE void SetOperating(bool _bOperating)
	{
		M_LOCK(m_Lock);
		M_ASSERT(m_bOperating != _bOperating,"");
		m_bOperating = _bOperating;
	}

	M_INLINE bool GetOperating()
	{
		M_LOCK(m_Lock);
		return m_bOperating;
	}

	bool Done(bool _bThrows, bool &_bWantedToThrow);

	M_INLINE void PerformOperation()
	{
		if (m_bWrite)
			Write();
		else
			Read();

		M_LOCK(m_Lock);
		m_bPending = false;
	}

	void Read();
	void Write();

	void BlockUntilDone();

};

class MCCDLLEXPORT CByteStreamDrive
{
public:
	MRTC_CriticalSection m_Lock;

	CStr m_DriveName;
	DLinkDS_Link(CByteStreamDrive, m_Link);

	class CCompare
	{
	public:
		static aint Compare(CByteStreamDrive *_pFirst, CByteStreamDrive *_pSecond, void *_pContext)
		{
			return _pFirst->m_DriveName.CompareNoCase(_pSecond->m_DriveName);
		}

		static aint Compare(CByteStreamDrive *_pFirst, const ch8 *_Key, void *_pContext)
		{
			return _pFirst->m_DriveName.CompareNoCase(_Key);
		}
	};

	DIdsTreeAVLAligned_Link(CByteStreamDrive, m_TreeLink, const ch8 *, CByteStreamDrive::CCompare);

	DLinkDS_List(CByteStreamCacheLine, m_PendingLink) m_PendingOpr;
	DLinkDS_List(CByteStreamCacheLine, m_PendingLink) m_PendingCacheLinesForDelete;
	DLinkDS_List(CByteStreamCacheLine, m_DriveLink) m_RequestsSorted;

	aint m_CacheSize;
	aint m_NumCaches;
	aint m_Granulartity;

	aint m_CacheSizeUsed;

	typedef TPtr<CCacheMem> spCCacheMem;

	TThinArray<spCCacheMem> m_lCacheMem;

	CCacheMem *GetCacheMem(aint _CacheLineSize);
	void ReturnCacheMem(CCacheMem *_pCacheMem);
	void Create(const char *_pDriveName);
	void SetCacheSize(aint _Value);
	void SetCacheNum(aint _Value);
	aint GetCacheSize();
	aint GetCacheNum();
	void AddRequest(CByteStreamCacheLine *_pCacheLine);
	bool Service(class CByteStreamData *_pTrowOnStream);
};


class CByteStreamManager;

class MCCDLLEXPORT CByteStreamData
{
public:
	CByteStreamManager *m_pStreamManager;
	CByteStreamDrive *m_pDrive;
	CStr m_FileName;
	float m_Priority;
	void *m_pFile;
	int m_OpenFlags;	
	fint m_FilePos;
#ifndef M_RTM
	fint m_LastReadFilePosXDF;
	fint m_LastReadFilePos;
	CMTime m_TimeOpen;
#endif
	fint m_BytesRead;
	fint m_FileSize;

	DLinkDS_Link(CByteStreamData, m_Link);

	DLinkDS_List(CByteStreamCacheLine, m_CacheLineLink) m_CacheLines; // A list of chache lines available for completion
	DLinkDS_List(CByteStreamCacheLine, m_CacheLineLink) m_PendingCacheLines; // A list of requests that has been send out
	CByteStreamCacheLine *m_pLastCacheLine;
	aint m_nCacheLines;
	aint m_CacheLine_Size;
	aint m_CacheLine_Num;
	bool m_bGood;
	bool m_bAllowPrecache;


	CByteStreamData();
	~CByteStreamData();
	bool Create(const CStr &_FileName, int _Flags, float _Priority, aint _NumCacheLines, aint _CacheLineSize);
	void SetPriority(fp32 _Priority);
	void Close();
	void CreateCachelines();
	bool Good_Get();
	void Good_Clear();
	bool EndOfFile();
	void SeekEnd(fint _BytesFromEnd);
	void SeekBeg(fint _BytesFromBeg);
	void SeekCur(fint _BytesFromCur);
	fint Len();
	fint Pos();
	void ServicePrecache();

	void Service();
	void WriteCacheLine(CByteStreamCacheLine *_pCacheLine);
	bool PrepareCacheline(CByteStreamCacheLine *_pCacheLine, fint _Start, mint _NumBytes, bool _bBlock);
	CByteStreamCacheLine *PrepareCacheline(fint _Start, mint _NumBytes, bool _bBlock);
	void SetLastCacheLine(CByteStreamCacheLine *_pLastCacheLine);
	CByteStreamCacheLine *GetCacheLine(fint _Start, mint _NumBytes, bool _bBlock = true);
	CByteStreamCacheLine *GetCacheLine_NotINL(fint _Start, mint _NumBytes, bool _bBlock);
	void Read(void *_pData, mint _nBytes);
	void Write(const void *_pData, mint _nBytes);
	bool Read(CAsyncRequest *_pRequest);
	bool Write(CAsyncRequest *_pRequest);
	bool AsyncFlush(bool _bBlock);

#ifndef M_RTM
	void XDFRecordClose();
	void XDFRecordCreate(CStr _FileName);
	void XDFRecordSeek();
	void XDFRecordRead();
#endif



	
};


class MCCDLLEXPORT CByteStreamManager
{
public:

	MRTC_CriticalSection m_Lock;

	DLinkDS_List(CByteStreamData, m_Link) m_OpenStreams;
	DLinkDS_List(CByteStreamDrive, m_Link) m_Drives;

	DIdsTreeAVLAligned_Tree(CByteStreamDrive, m_TreeLink, const ch8 *, CByteStreamDrive::CCompare) m_DriveTree;

	mint m_WorkSize;
	mint m_DefaultCacheSize;
	aint m_DefaultNumCaches;
	int m_Corrupt;
	int m_DisableCaching;

	TCPool<CByteStreamData, 128, NThread::CMutualAggregate> m_PoolByteStreamData;
	TCPool<CByteStreamCacheLine, 128, NThread::CMutualAggregate> m_PoolCacheLines;

	void *m_pXDFThread;
	class CXDF *m_pXDFRecord;
	class CXDF *m_pXDFUse;
	NThread::CMutual m_XDFLock;
	NThread::CMutual m_XDFUseLock;

	TPtr<CLogFile> m_spLogFile;

	TArray<CStr> m_SearchPaths;
	CStr m_SearchPathBase;
	bool m_bLogData;

	void Service();
	CByteStreamDrive *GetDrive(CStr &_Drive);
	CByteStreamDrive *CreateDrive(CStr &_Drive);

	CByteStreamManager();
	~CByteStreamManager();
	void AddCorrupt(int _Corrupt);
	void RemoveCorrupt(int _Corrupt);
	int GetCorrupt();
	void SetDefaultCacheSize(aint _Value);
	void SetDefaultCacheNum(aint _Value);
	mint GetDefaultCacheSize();
	aint GetDefaultCacheNum();
	void SetDriveCacheSize(const char *_pDrive, aint _Value);
	void SetDriveCacheNum(const char *_pDrive,aint _Value);
	void XDF_Record(const char* _pName, const char* _pBasePath);
	void XDF_Use(const char* _pName, const char* _pBasePath);
	void XDF_Stop();
	void XDF_Pause();
	void XDF_Resume();
	bint XDF_IsPasued();
	CStr XDF_GetCurrent();
	fint XDF_GetPosition();

	CXDF *XDF_GetUse();
	CXDF *XDF_GetRecord();
	void XDF_Destruct();

	void DisableCache();
	void EnableCache();
	int GetCacheEnable();

	bool ServiceDrives(bool _bThrows);

	void SearchPath_Add(CStr _Path);
	void SearchPath_SetBase(CStr _Path);
	CStr SearchPath_GetBase();
	int SearchPath_GetNumPaths();
	CStr SearchPath_GetPath(int _iPath);
};

CByteStreamManager *CreateStreamManager();
M_INLINE static CByteStreamManager *GetStreamManager()
{
	if (!MRTC_GetObjectManager()->m_pByteStreamManager)
	{
		CreateStreamManager();
	}

	return MRTC_GetObjectManager()->m_pByteStreamManager;
}

class CMDisableDiskCache
{
public:
	CMDisableDiskCache()
	{
		GetStreamManager()->DisableCache();
	}
	~CMDisableDiskCache()
	{
		GetStreamManager()->EnableCache();
	}

};



