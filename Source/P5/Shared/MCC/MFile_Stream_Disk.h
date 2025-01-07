
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/


#ifndef __INC_MFile_Stream_Disk
#define __INC_MFile_Stream_Disk

#include "MFile_Stream.h"


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CStream_Disk
|__________________________________________________________________________________________________
\*************************************************************************************************/
class MCCDLLEXPORT CStream_Disk : public CStream
{
	CStr m_FileName;

public:
	CByteStream m_Stream;
	CStream_Disk();
	~CStream_Disk();
	bool FileExists(CStr _Name, int _mode);
	bool DirectoryExists(CStr _name, int _mode);
	void Open(const CStr _Name, int _Mode);
	void OpenExt(const CStr _name, int _mode, float _Priority = 0, aint _NumCacheLines = -1, aint _CacheLineSize = -1);
	void SetPriority(fp4 _Priority);

	void Close();
	void Rename(const CStr& _Name);
	void Remove();
	void Read(void* _pDest, mint _Size);
	void Write(const void* _pSrc, mint _Size);
	bool Read(CAsyncRequest *_pRequest) {return m_Stream.Read(_pRequest);};
	bool Write(CAsyncRequest *_pRequest) {return m_Stream.Write(_pRequest);};
	bool AsyncFlush(bool _bBlock) {return m_Stream.AsyncFlush(_bBlock);};
	void Seek(fint _Pos);
	void SeekToEnd();
	void RelSeek(fint _Pos);
	fint Pos();
	fint Length();
	bool EndOfFile();
};

#endif // __INC_MFile_Stream_Disk
