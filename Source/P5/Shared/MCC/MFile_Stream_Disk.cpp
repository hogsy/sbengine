
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

#include "MFile_Stream_Disk.h"

#ifdef PLATFORM_PS2
#include <errno.h>
#endif // COMPILER_GNU

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CStream_Disk
|__________________________________________________________________________________________________
\*************************************************************************************************/
CStream_Disk::CStream_Disk()
{
	m_Mode = 0;
};

CStream_Disk::~CStream_Disk()
{
};

bool CStream_Disk::FileExists(CStr _Name, int _mode)
{
	return MRTC_SystemInfo::OS_FileExists(_Name.Str());
//	if (m_Stream.Create(_Name, EByteStream_Read, 0))
//		return true;
//	else
//		return false;
};

bool CStream_Disk::DirectoryExists(CStr _Name, int _mode)
{
	return MRTC_SystemInfo::OS_DirectoryExists(_Name.Str());
}

void CStream_Disk::Open(const CStr _Name, int _Mode)
{
	OpenExt(_Name, _Mode);
};

void CStream_Disk::SetPriority(fp4 _Priority)
{
	m_Stream.SetPriority(_Priority);
}

void CStream_Disk::OpenExt(const CStr _Name, int _Mode, float _Priority, aint _NumCacheLines, aint _CacheLineSize)
{
	m_Mode = _Mode;

	int OpenMode = 0;

	if ((_Mode & CFILE_READ) != 0) 
		OpenMode |= EByteStream_Read;

	if ((_Mode & CFILE_NOLOG) != 0) 
		OpenMode |= EByteStream_NoLog;

	if ((_Mode & CFILE_NODEFERCLOSE) != 0)
		OpenMode |= EByteStream_NoDeferClose;
	
#if !defined PLATFORM_DREAMCAST
	if (((_Mode & CFILE_READ) == 0) && ((_Mode & CFILE_WRITE) != 0)) 
		OpenMode |= EByteStream_Create;
#endif
	
	if ((_Mode & CFILE_WRITE) != 0) 
		OpenMode |= EByteStream_Write;

	if ((_Mode & CFILE_APPEND) && !(_Mode & CFILE_TRUNC))
	{
	}
	else
	{
		if ((_Mode & CFILE_TRUNC) || (((_Mode & CFILE_READ) == 0) && ((_Mode & CFILE_WRITE) != 0)))
			OpenMode |= EByteStream_Truncate;
	}
		
	if (!m_Stream.Create(_Name, OpenMode, _Priority, _NumCacheLines, _CacheLineSize))
	{	
		FileError("Open", _Name, errno);
	}

	if (_Mode & CFILE_APPEND)
		m_Stream.SeekEnd(0);

	m_FileName = _Name;
}


void CStream_Disk::Close()
{
	if (!m_Stream.Close())
	{
#ifndef PLATFORM_XBOX
		FileError("Close", m_FileName, errno);
#endif
	}
}

void CStream_Disk::Rename(const CStr& _Name)
{
	Error("Rename", "Not implemented.");
}

void CStream_Disk::Remove()
{
	Error("Remove", "Not implemented.");
}

void CStream_Disk::Read(void* _pDest, mint _Size)
{
#ifndef M_RTM
	if (!m_Stream.Good()) 
		FileError("Read", m_FileName, errno);
#endif

	m_Stream.Read((char*) _pDest, _Size);

#ifndef M_RTM
	if (!m_Stream.Good()) 
		FileError("Read", m_FileName, errno);
#endif
};

void CStream_Disk::Write(const void* _pSrc, mint _Size)
{
#ifndef M_RTM
	if (!m_Stream.Good()) FileError("Write", m_FileName, errno);
#endif
	m_Stream.Write((char*) _pSrc, _Size);
#ifndef M_RTM
	if (!m_Stream.Good()) FileError("Write", m_FileName, errno);
#endif
};

void CStream_Disk::Seek(fint _Pos)
{
	if (EndOfFile()) 
		m_Stream.ClearFlags();

	m_Stream.SeekBeg(_Pos);

#ifndef M_RTM
	if (!m_Stream.Good()) 
		FileError(CStrF("Seek (%d)", _Pos), m_FileName, errno);
#endif
};

void CStream_Disk::SeekToEnd()
{
	m_Stream.SeekEnd(0);
	
#ifndef M_RTM
	if (!m_Stream.Good()) 
		FileError("Seek", m_FileName, errno);
#endif
};

void CStream_Disk::RelSeek(fint _Pos)
{
	if (EndOfFile()) 
		m_Stream.ClearFlags();

	m_Stream.SeekCur(_Pos);
	
#ifndef M_RTM
	if (!m_Stream.Good()) FileError("Seek", m_FileName, errno);
#endif
};

fint CStream_Disk::Pos()
{
	uint32 pos = m_Stream.Pos();

#ifndef M_RTM
	if (!m_Stream.Good()) 
		FileError("Seek", m_FileName, errno);
#endif

	return pos;
};

fint CStream_Disk::Length()
{
	return m_Stream.Len();
};

bool CStream_Disk::EndOfFile()
{
	return m_Stream.EndOfFile();
};
