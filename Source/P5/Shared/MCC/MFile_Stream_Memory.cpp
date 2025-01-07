
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

#include "MFile_Stream_Memory.h"


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CStream_Memory
|__________________________________________________________________________________________________
\*************************************************************************************************/
CStream_Memory::CStream_Memory()
{
	m_Mode = 0;
	m_MinGrow = 32768;
	m_Pos = 0;
	m_Len = 0;
	m_pFileMem = NULL;
	m_FileMaxLen = 0;
}

CStream_Memory::CStream_Memory(TArray<uint8> _lFileMem, int _MinGrow)
{
	Open(_lFileMem, CFILE_READ | CFILE_WRITE, _MinGrow);
}

CStream_Memory::CStream_Memory(uint8* _pMem, int _CurrentLen, int _MaxLen)
{
	m_Pos = 0;
	m_Len = _CurrentLen;
	m_pFileMem = _pMem;
	m_FileMaxLen = _MaxLen;
	m_Mode = CFILE_WRITE | CFILE_READ | CFILE_BINARY;
}

CStream_Memory::~CStream_Memory()
{
}

uint8* CStream_Memory::GetWritePtr(int _Pos, int _Count)
{
	if (!_Count) return NULL;

	if (m_pFileMem)
	{
		if (_Pos + _Count > m_FileMaxLen)
			Error("GetWritePtr", "Memory-file full.");

		return &m_pFileMem[_Pos];
	}
	else
	{
		if (_Pos + _Count > m_lFileMem.Len())
			m_lFileMem.SetLen(MaxMT(_Pos + _Count, MaxMT((aint)m_MinGrow, m_lFileMem.Len() * 2)));

		return &m_lFileMem[_Pos];
	}
}

bool CStream_Memory::FileExists(CStr _name, int _mode)
{
	return false;
}

void CStream_Memory::Open(const CStr _name, int _mode)
{
	m_Mode = _mode;

	if (_mode & CFILE_APPEND)
	{
		m_Pos=m_Len;
	}
	else
	{
		m_Pos = 0;
	}
}

void CStream_Memory::Open(uint8* _pMem, int _CurrentLen, int _MaxLen, int _Mode)
{
	m_Pos = 0;
	m_Len = _CurrentLen;
	m_pFileMem = _pMem;
	m_FileMaxLen = _MaxLen;
	m_Mode = _Mode;
}

void CStream_Memory::Open(TArray<uint8> _lFileMem, int _Mode, int _MinGrow)
{
	m_Mode = _Mode;
	m_MinGrow = _MinGrow;
	m_Pos = 0;
	m_Len = _lFileMem.Len();
	m_pFileMem = NULL;
	m_FileMaxLen = 0;
	m_lFileMem = _lFileMem;
}

void CStream_Memory::Close()
{
	m_lFileMem.SetLen(m_Len);
}

void CStream_Memory::Read(void* dest, mint _Size)
{
	if (!EndOfFile())
	{
		uint8* pFile = GetWritePtr(0, m_Len);

		memcpy(dest, &pFile[m_Pos], Min(m_Len - m_Pos, _Size));
		m_Pos += Min(m_Len - m_Pos, _Size);
	}
}

void CStream_Memory::Write(const void* src, mint size)
{
	uint8* pFile = GetWritePtr(m_Pos, size);
	memcpy(pFile, src, size);
	m_Pos += size;
	m_Len = Max(m_Pos, m_Len);
}


bool CStream_Memory::Read(CAsyncRequest *_pRequest) 
{
	uint8* pFile = GetWritePtr(0, m_Len);

	fint CopyPos = _pRequest->m_StartPos + _pRequest->m_nBytesCopied;
	fint ToCopy = Min((fint)m_Len - CopyPos, (fint)_pRequest->m_nBytes - (fint)_pRequest->m_nBytesCopied);

	memcpy(_pRequest->m_pBuffer, pFile + CopyPos, ToCopy);

	_pRequest->m_nBytesCopied += ToCopy;

	return _pRequest->Done();
}

bool CStream_Memory::Write(CAsyncRequest *_pRequest)
{
	int NewLen = _pRequest->m_StartPos + _pRequest->m_nBytes;
	if (NewLen > (aint)m_Len)
		m_Len = NewLen;

	uint8* pFile = GetWritePtr(0, m_Len);

	fint CopyPos = _pRequest->m_StartPos + _pRequest->m_nBytesCopied;
	fint ToCopy = Min((fint)m_Len - (fint)CopyPos, (fint)_pRequest->m_nBytes - (fint)_pRequest->m_nBytesCopied);

	memcpy(pFile + CopyPos, _pRequest->m_pBuffer, ToCopy);

	_pRequest->m_nBytesCopied += ToCopy;

	return _pRequest->Done();
}


void CStream_Memory::Seek(fint pos)
{
	if (pos < 0 || pos > (fint)m_Len)
	{
		GetWritePtr(pos, 1);
		m_Len = pos;
		m_Pos = pos;

//		FileError("Seek", CStrF("Attempted to seek outside the file. (%d -> %d > %d)", m_Pos, pos, m_Len), 0);
	}
	else
		m_Pos = pos;
}

void CStream_Memory::SeekToEnd()
{
	m_Pos = m_Len;
}

void CStream_Memory::RelSeek(fint _dPos)
{
	int32 wantedpos = m_Pos + _dPos;
	Seek(wantedpos);
}

fint CStream_Memory::Pos()
{
	if (m_Mode & CFILE_HIDEPOS)
		Error("Pos", "Invalid operation.");
	return m_Pos;
}

fint CStream_Memory::Length()
{
	return m_Len;
}

bool CStream_Memory::EndOfFile() 
{
	return (m_Pos == m_Len);
}
