
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/


#ifndef __INC_MFile_StreamMgr
#define __INC_MFile_StreamMgr

enum
{
	EByteStream_Read				= 0x00000001
	,EByteStream_Write				= 0x00000002
	,EByteStream_Create				= 0x00000004
	,EByteStream_Truncate			= 0x00000008
	,EByteStream_DoNotCacheFileSize	= 0x00000010
	,EByteStream_NoLog				= 0x00000020
	,EByteStream_NoDeferClose		= 0x00000040
};

class CAsyncRequest;
#include "MFile_StreamMgrImp.h"

class MCCDLLEXPORT CAsyncRequest
{
public:
//	MRTC_CriticalSection m_Lock;
	fint m_StartPos;
	mint m_nBytesCopied;
	mint m_nBytes;
	int32 m_bInMemory;
	void *m_pBuffer;
	CByteStreamCacheLine *m_pCacheLine;

	CAsyncRequest();
	~CAsyncRequest();
	void SetRequest(void *_pBuffer, fint _StartPos, mint _nBytes, bool _bInMemory);
	bool Done();
};


class MCCDLLEXPORT CByteStream
{
public:

	class CByteStreamData *m_pData;
	
	
	CByteStream();
	~CByteStream();
	bool Create(const CStr &_FileName, int _Flags, float _Priority, aint _NumCacheLines = -1, aint _CacheLineSize = -1);
	bool Close();

	void SetPriority(fp32 _Priority);

	bool Good();
	void ClearFlags();
	bool EndOfFile();
	void SeekEnd(fint _BytesFromEnd);
	void SeekBeg(fint _BytesFromBeg);
	void SeekCur(fint _BytesFromCur);
	fint Len();
	fint Pos();
	void Read(void *_pData, mint _nBytes);
	void Write(const void *_pData, mint _nBytes);
	// Call repeatidly until true is returned
	bool Read(CAsyncRequest *_pRequest);
	// Call repeatidly until true is returned
	bool Write(CAsyncRequest *_pRequest);
	bool AsyncFlush(bool _bBlock);
	static void SetCacheSize_Dirve(const char *_pDrive, aint _CacheSize);
	static void SetCacheSize_Default(aint _CacheSize);
	static void SetCacheNumber_Drive(const char *_pDrive, aint _nCaches);
	static void SetCacheNumber_Default(aint _nCaches);
	static void XDF_Record(const char* _pName, const char* _pBasePath);
	static void XDF_Use(const char* _pName, const char* _pBasePath);
	static void XDF_Stop();
	static void XDF_Pause();
	static void XDF_Resume();
	static bint XDF_IsPasued();
	static CStr XDF_GetCurrent();
	static fint XDF_GetPosition();

	static CXDF *XDF_GetUse();
	static CXDF *XDF_GetRecord();
	static void SearchPath_Add(CStr _Path);
	static void SearchPath_SetBase(CStr _Path);
	static CStr SearchPath_GetBase();
	static int SearchPath_GetNumPaths();
	static CStr SearchPath_GetPath(int _iPath);
	static void AddCorrupt(int _Corrupt);
	static void RemoveCorrupt(int _Corrupt);
	static int GetCorrupt();
};
	
#ifndef	NO_INLINE_FILESYSTEM	
#include "MFile_StreamMgr.inl"
#endif	// PLATFORM_PS2

#endif // __INC_MFile_StreamMgr
