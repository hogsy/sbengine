
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/


#include "MDA_LinkedList.h"
#include "MDA_Reference.h"
#include "MMemMgrHeap.h"
#include "MFile_Misc.h"

static class CByteStreamManager *GetStreamManager();

#define D_DefaultCacheSize 65536
#define D_DefaultNumCaches 4

class MCCDLLEXPORT CByteStreamAsyncInstance : public CDA_Referable
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

//DA_LLSI(CByteStreamAsyncInstance);

class MCCDLLEXPORT CByteStreamDriveSortList : public CDA_LinkedList_Small
{
public:
	int Compare(void *_pFirst, void *_pSecond);
};


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
class MCCDLLEXPORT CByteStreamCacheLine : public CDA_Linkable
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

	CDA_Link m_CacheLineList;
	CDA_Link m_DriveList;
	CDA_Link m_PendingList;
	CByteStreamAsyncInstance m_PendingOperation;
	//TCDA_ReferenceType<CByteStreamAsyncInstance> ;
	CCacheMem *m_pCacheMemUsed;

	fint m_DataOffest;
	mint m_NumBytesUsed;
	mint m_NumBytes;
	fp4 m_Prio;
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


	M_INLINE bool Done(bool _bThrows, bool &_bWantedToThrow)
	{
		M_LOCK(m_Lock);
		if (m_PendingOperation.m_pInstance)
			return m_PendingOperation.Done(_bThrows, _bWantedToThrow);
		else
			return !m_bPending;
	}

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

DA_LLSI(CByteStreamCacheLine);
DA_LLI(CByteStreamCacheLine);

class MCCDLLEXPORT CByteStreamDrive
{
public:
	MRTC_CriticalSection m_Lock;

	CStr m_DriveName;
	CDA_Link m_ListLink;
	CDA_HashLink m_HashLink;

//	TCDA_ReferenceType<CByteStreamCacheLine> m_Pending[2];
	CDA_LinkedList_Small m_PendingOpr;

	CDA_LinkedList_Small m_PendingCacheLinesForDelete;

	CByteStreamDriveSortList m_Requests; // A queue of requests that is handeled in order of importance
//	CDA_LinkedList_Small m_Finished; // A queue of finished requests

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



DA_LLSI(CByteStreamDrive);


class MCCDLLEXPORT CByteStreamDrive_Hash : public TCDA_HashType<CByteStreamDrive, CDA_HashSmall, CStr> 
{
public:
	bool IsSame(void *_pFirst, void *_pSecond)
	{
		return ((CStr *)(_pFirst))->CompareNoCase(*((CStr *)(_pSecond))) == 0;
	}

	mint GetHash(void *_pHashData)
	{
		CFStr Tmp = *((CStr *)(_pHashData));

		CStr::strupr(Tmp);

		CMDA_CRC32Checker Checker;
		Checker.AddStr(Tmp);

		return Checker.m_CurrentCRC;
	}
};



class MCCDLLEXPORT CByteStreamManager
{
public:

	MRTC_CriticalSection m_Lock;

	CDA_LinkedList_Small m_OpenStreams; // Contains streams that are open
	CDA_LinkedList_Small m_Drives; // Contains streams that are open
	CByteStreamDrive_Hash m_DriveHash; // Hash for drives
	mint m_WorkSize;
	mint m_DefaultCacheSize;
	aint m_DefaultNumCaches;
	int m_Corrupt;
	int m_DisableCaching;

	TDA_Pool<CByteStreamData> m_PoolByteStreamData;
	TDA_Pool<CByteStreamCacheLine> m_PoolCacheLines;

	void *m_pXDFThread;
	class CXDF *m_pXDFRecord;
	class CXDF *m_pXDFUse;
	NThread::CMutual m_XDFLock;

	TPtr<CLogFile> m_spLogFile;

	TList_Vector<CStr> m_SearchPaths;
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

	CDA_Link m_ListLink;

	CDA_LinkedList_Small m_CacheLines; // A list of chache lines available for completion
	CDA_LinkedList_Small m_PendingCacheLines; // A list of requests that has been send out
	CByteStreamCacheLine *m_pLastCacheLine;
	aint m_nCacheLines;
	aint m_CacheLine_Size;
	aint m_CacheLine_Num;
	bool m_bGood;
	bool m_bAllowPrecache;


	CByteStreamData();
	~CByteStreamData();
	bool Create(const CStr &_FileName, int _Flags, float _Priority, aint _NumCacheLines, aint _CacheLineSize);
	void SetPriority(fp4 _Priority);
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

	class CSortList : public CDA_LinkedList
	{
	public:
		int Compare(CDA_Linkable *_pFirst, CDA_Linkable *_pSecond)
		{
			return ((CByteStreamCacheLine *)_pFirst)->m_DataOffest - ((CByteStreamCacheLine *)_pSecond)->m_DataOffest;					
		}
	};
	
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

DA_LLSI(CByteStreamData);
