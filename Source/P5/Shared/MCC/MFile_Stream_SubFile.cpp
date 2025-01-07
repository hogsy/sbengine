
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

#include "MFile_Stream_SubFile.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CStream_SubFile
|__________________________________________________________________________________________________
\*************************************************************************************************/

IMPLEMENT_OPERATOR_NEW(CStream_SubFile);


CStream_SubFile::CStream_SubFile()
{
	m_StartPos = 0;
	m_SubLen = 0;
}

CStream_SubFile::~CStream_SubFile()
{
	
}

CStream_SubFile::CStream_SubFile(spCCFile _pSubFile, int _StartPos, int _SubLen)
{
	Open(_pSubFile, _StartPos, _SubLen);
}

bool CStream_SubFile::FileExists(CStr _name, int _mode)
{
	return false;
}

void CStream_SubFile::OpenExt(const CStr _name, int _mode, float _Priority, aint _NumCacheLines, aint _CacheLineSize)
{
	Error("CStream_SubFile::Open", "Makes no sense!!");
}

void CStream_SubFile::Open(const CStr _name, int _mode)
{
	Error("CStream_SubFile::Open", "Makes no sense!!");
}

void CStream_SubFile::Open(spCCFile _pSubFile, int _StartPos, int _SubLen)
{
	m_spFile = _pSubFile;
	m_StartPos = _StartPos;
	m_SubLen = _SubLen;
	
	m_spFile->Seek(m_StartPos);
	
	int Len = m_spFile->Length();
	
	if ((Len - m_StartPos) < m_SubLen)
		Error("CStream_SubFile::Open", "The file can not contain the subfile");
}

CStr CStream_SubFile::GetFileName()
{
	return m_spFile->GetFileName();
}

void CStream_SubFile::SetPriority(fp4 _Priority)
{
	m_spFile->SetPriority(_Priority);
}

void CStream_SubFile::Close()
{
	m_spFile = NULL;
}

void CStream_SubFile::Read(void* dest, mint _Size)
{
	if (!m_spFile)
		Error("CStream_SubFile::Read", "Stream is not opon");

	fint PosTemp = Pos();
	
	if ((fint)(PosTemp+_Size) > m_SubLen)
	{
		_Size = m_SubLen - PosTemp;
	}
	
	if (_Size > 0)
	{
		m_spFile->Read(dest, _Size);
	}

	M_ASSERT(Pos() <= m_SubLen, "TNoto");
}

void CStream_SubFile::Write(const void* src, mint _Size)
{
	if (!m_spFile)
		Error("CStream_SubFile::Read", "Stream is not opon");
	
	if (Pos() >= (fint)(m_SubLen + _Size))
	{
		_Size = m_SubLen - Pos();
	}
	
	if (_Size > 0)
	{
		m_spFile->Write(src, _Size);
	}
	
}

bool CStream_SubFile::Read(CAsyncRequest *_pRequest) 
{
	CAsyncRequest Temp;
	Temp = *_pRequest;

	Temp.m_StartPos += m_StartPos;

	bool Ret = m_spFile->Read(&Temp);

	_pRequest->m_nBytesCopied = Temp.m_nBytesCopied;
	_pRequest->m_pCacheLine = Temp.m_pCacheLine;
	Temp.m_pCacheLine = NULL;

	return Ret;
}

bool CStream_SubFile::Write(CAsyncRequest *_pRequest)
{
	CAsyncRequest Temp;
	Temp = *_pRequest;

	Temp.m_StartPos += m_StartPos;

	bool Ret = m_spFile->Write(&Temp);

	_pRequest->m_nBytesCopied = Temp.m_nBytesCopied;
	_pRequest->m_pCacheLine = Temp.m_pCacheLine;
	Temp.m_pCacheLine = NULL;

	return Ret;
}

void CStream_SubFile::Seek(fint _Pos)
{
	if (!m_spFile)
		Error("CStream_SubFile::Read", "Stream is not opon");
	
	m_spFile->Seek(m_StartPos + _Pos);
}

void CStream_SubFile::SeekToEnd()
{
	if (!m_spFile)
		Error("CStream_SubFile::Read", "Stream is not opon");
	
	m_spFile->Seek(m_StartPos + m_SubLen);
}

void CStream_SubFile::RelSeek(fint _dPos)
{
	if (!m_spFile)
		Error("CStream_SubFile::Read", "Stream is not opon");
	
	m_spFile->RelSeek(_dPos);
}

fint CStream_SubFile::Pos()
{
	if (!m_spFile)
		Error("CStream_SubFile::Read", "Stream is not opon");
	
	return m_spFile->Pos() - m_StartPos;
}

fint CStream_SubFile::Length()
{
	if (!m_spFile)
		Error("CStream_SubFile::Read", "Stream is not opon");
	
	return m_SubLen;
}

bool CStream_SubFile::EndOfFile()
{
	if (!m_spFile)
		Error("CStream_SubFile::Read", "Stream is not opon");
	
	return m_spFile->Pos() >= (m_SubLen + m_StartPos);
}
