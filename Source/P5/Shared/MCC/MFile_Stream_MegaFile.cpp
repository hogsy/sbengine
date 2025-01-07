
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

#include "MFile_MegaFile.h"
#include "MFile_Stream_MegaFile.h"

// -------------------------------------------------------------------
//  CStream_Megafile
// -------------------------------------------------------------------
//
// CLASS AUTHOR:	Daniel Hansson
// STARTED	   :	970310
// LAST UPDATED:	970315
// COMMENTS	   :	Handles files thru archives.
//

/*
CStream_Megafile::CStream_Megafile()
{
	m_pStream = DNew(fstream) fstream;
	if (m_pStream == NULL) MemError("-");
	m_EOS = TRUE;
}

CStream_Megafile::~CStream_Megafile()
{
	if (m_pStream->is_open()) m_pStream->close();
	if (m_pStream) { delete m_pStream; m_pStream = NULL; }
}

bool CStream_Megafile::FileExists(CStr _Name, int _Mode)
{
	if ((_Mode & CFILE_READ)!=0) 
	{
		return (g_Archives.GetFileOwner(_Name) != NULL);
	}
	else
		return FALSE;
}

void CStream_Megafile::Open(const CStr _Name, int _Mode)
{
	m_Mode = _Mode;
	
	int iosmode = 0;
#ifdef PLATFORM_DOLPHIN
#else
	if ((_Mode & CFILE_READ) != 0) iosmode |= ios::in;
	if ((_Mode & CFILE_READ) != 0) iosmode |= ios::nocreate | ios::in;
#endif
	//		if ((_Mode & CFILE_WRITE) != 0) iosmode |= ios::out;
	if ((_Mode & CFILE_BINARY) != 0) iosmode |= ios::binary; 
	//		if ((_Mode & CFILE_APPEND) != 0) iosmode |= ios::ate; 
	
	if ((_Mode & CFILE_APPEND) != 0)
		Error("Open","Can't write or append to an archive file (yet).");
	
	CCFileArchive* fa = g_Archives.GetFileOwner(_Name);
	if (!fa) Error("Open","No such file in the registered archives.");
	
	m_AE = *fa->GetFileInfo(_Name);
	
	// Open archive.
	m_pStream->open(fa->GetArchiveFileName()
#if (defined(PLATFORM_DREAMCAST) || defined(PLATFORM_XBOX) || defined(PLATFORM_DOLPHIN))
		, iosmode
#else
		, iosmode, filebuf::sh_read
#endif
		);
	if (!m_pStream->good()) FileError("Open", _Name, errno);
	m_FileName = _Name;
	
	// Seek to start of file within the archive.
	m_pStream->seekp(m_AE.Offset);
	m_FileEnd = m_AE.Offset + m_AE.Length;
	
	if (m_AE.Length > 0)
		m_EOS = FALSE;
	else
		m_EOS = TRUE;
}

void CStream_Megafile::Close()
{
	m_pStream->close();
	if (!m_pStream->good()) FileError("Close", m_FileName, errno);
	m_EOS=TRUE;
}

void CStream_Megafile::Read(void* _pDest, uint32 _Size)
{
	if (!m_EOS) {
		int32 curpos = m_pStream->tellp();
		
		m_pStream->read((char*)_pDest, Min((uint32)_Size, m_FileEnd-curpos));
		if (!m_pStream->good()) FileError("Read", m_FileName, errno);
		
		if ((_Size + (uint32)curpos) >= m_FileEnd)
			m_EOS=TRUE;
	}
}

void CStream_Megafile::Write(const void* _pSrc, uint32 _Size)
{
	Error("Write","Not implemented yet.");
}

void CStream_Megafile::Seek(int32 _Pos)
{
	if ((m_Mode & CFILE_WRITE) != 0)
		Error("Seek","Can't seek in an archive file when in write mode.");
	
	int32 wantedpos = (int32)m_AE.Offset + _Pos;
	if (wantedpos>=(int32)m_AE.Offset && wantedpos <= (int32)m_FileEnd) 
	{
		m_pStream->seekp(wantedpos);
		if (!m_pStream->good()) FileError("Seek", m_FileName, errno);
		if (wantedpos < (int32)m_FileEnd)
			m_EOS=FALSE;
		else
			m_EOS=TRUE;
	}
	else
		Error("Seek","Attempted to seek outside the file.");
}

void CStream_Megafile::SeekToEnd()
{
	if ((m_Mode & CFILE_WRITE) != 0)
		Error("SeekToEnd","Can't seek in an archive file when in write mode.");
	
	m_pStream->seekp(m_FileEnd);
	if (!m_pStream->good()) FileError("SeekToEnd", m_FileName, errno);
	m_EOS=TRUE;
}

void CStream_Megafile::RelSeek(int32 _Pos)
{
	if ((m_Mode & CFILE_WRITE)!=0)
		Error("RelSeek","Can't seek in an archive file when in write mode.");
	
	int32 wantedpos=m_pStream->tellp() + _Pos;
	if (wantedpos >= (int32)m_AE.Offset && wantedpos <= (int32)m_FileEnd) 
	{
		m_pStream->seekp(wantedpos);
		if (!m_pStream->good()) FileError("RelSeek", m_FileName, errno);
		if (wantedpos < (int32)m_FileEnd)
			m_EOS=FALSE;
		else
			m_EOS=TRUE;
	}
	else
		Error("RelSeek","Attempted to seek outside the file.");
}

int32 CStream_Megafile::Pos()
{
	int32 Pos=m_pStream->tellp();
	if (!m_pStream->good()) FileError("Pos", m_FileName, errno);
	return Pos - m_AE.Offset;
}

int32 CStream_Megafile::Length()
{
	return m_AE.Length;
}

bool CStream_Megafile::EndOfFile()
{
	return m_EOS;
}
*/
