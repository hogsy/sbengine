
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
#include "MFile.h"
//#include "MArchive.h"
#include "MComp.h"

#include "MFile_RAM.h"
#include "MFile_Misc.h"
#include "MFile_Stream_Disk.h"
#include "MFile_Stream_Memory.h"
#include "MFile_Stream_Megafile.h"
#include "MFile_Stream_Compressed.h"
#include "MFile_Stream_RAM.h"
#include "MFile_Stream_XDF.h"


IMPLEMENT_OPERATOR_NEW(CStream);
MRTC_IMPLEMENT(ILogFile, CReferenceCount);
MRTC_IMPLEMENT(IProgress, CReferenceCount);



// -------------------------------------------------------------------

IMPLEMENT_OPERATOR_NEW(CCFile);


CCFile::CCFile()
{
	m_pStream = NULL;
	m_bStreamAllocated = true;
	m_StreamType = 	ECCFile_Stream_Default;
#ifndef PLATFORM_CONSOLE
	m_iDisableWrite = 0;
#endif
};

CCFile::~CCFile()
{
	DisconnectStream();
};

bool CCFile::Read(CAsyncRequest *_pRequest) 
{
//	try 
//	{
		return m_pStream->Read(_pRequest);
//	}
//	catch (CCExceptionFile)
//	{
//		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
//		return false;
//	}	
};
bool CCFile::Write(CAsyncRequest *_pRequest) 
{	
//	try 
//	{
		return m_pStream->Write(_pRequest);
//	}
//	catch (CCExceptionFile)
//	{
//		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
//		return false;
//	}
};

void CCFile::DisconnectStream()
{
	if (m_pStream && m_bStreamAllocated) delete m_pStream;
	m_pStream = NULL;
};

void CCFile::ConnectStream(int _mode)
{
	MSCOPESHORT(CCFile::ConnectStream); //AR-SCOPE

	DisconnectStream();

	// In write mode FileExists returns wether it's
	// ok to write to a stream.

	m_bStreamAllocated = true;
	m_StreamType = ECCFile_Stream_Default;

	int NumSearchPaths = CByteStream::SearchPath_GetNumPaths();

	CStr StrippedFileName;
	CStr LowerCaseFileName = m_FileName.LowerCase();
	CStr BasePath = CByteStream::SearchPath_GetBase();
	if (LowerCaseFileName.CompareSubStr(BasePath) == 0)
	{
		StrippedFileName = m_FileName.Right(m_FileName.Len() - CByteStream::SearchPath_GetBase().Len());
	}
	else
		NumSearchPaths = 0;

	int NumPathsToSearch = NumSearchPaths+1;
	TPtr<CStream> spStream_XDF = MNew(CStream_XDF);
	TPtr<CStream> spStream_RAM = MNew(CStream_RAM);
	TPtr<CStream> spStream_Disk = MNew(CStream_Disk);
	CStr CurrentFileName = m_FileName;

	// Search for XDF files first
	for (int i = 0; i < NumPathsToSearch; ++i)
	{
		if (i < NumSearchPaths)
		{
			CurrentFileName = CByteStream::SearchPath_GetPath(i) + StrippedFileName;
		}
		else
		{
			CurrentFileName = m_FileName;
		}	

		if (!(_mode & CFILE_NOLOG) && !CDiskUtil::XDF_IsPaused())
		{
			m_pStream = spStream_XDF;
			if (m_pStream == NULL) MemError("ConnectStream");
			if (m_pStream->FileExists(CurrentFileName.LowerCase(), _mode)) 
			{ 
				m_StreamType = ECCFile_Stream_XDF;
				break;
			};
			m_pStream = NULL;
		}
		
	}

	// Search for rest of stream types
	if (!m_pStream)
	{
		for (int i = 0; i < NumPathsToSearch; ++i)
		{
			if (i < NumSearchPaths)
			{
				CurrentFileName = CByteStream::SearchPath_GetPath(i) + StrippedFileName;
			}
			else
			{
				CurrentFileName = m_FileName;
			}	

/*			if (!(_mode & CFILE_NOLOG))
			{
				m_pStream = spStream_XDF;
				if (m_pStream == NULL) MemError("ConnectStream");
				if (m_pStream->FileExists(CurrentFileName.LowerCase(), _mode)) 
				{ 
					m_StreamType = ECCFile_Stream_XDF;
					break;
				};
				m_pStream = NULL;
			}*/

			m_pStream = spStream_RAM;
			if (m_pStream == NULL) MemError("ConnectStream");
			if (m_pStream->FileExists(CurrentFileName, _mode)) 
			{ 
				m_StreamType = ECCFile_Stream_Default;
				break;
			};
			m_pStream = NULL;

			
			// MEGA
			/*
			m_pStream = DNew(CStream_Megafile) CStream_Megafile;
			if (m_pStream == NULL) MemError("ConnectStream");
			if (m_pStream->FileExists(m_FileName,_mode)) { return; };
			delete m_pStream;
			m_pStream = NULL;
			*/

			m_pStream = spStream_Disk;
			if (m_pStream == NULL) MemError("ConnectStream");
			if (m_pStream->FileExists(CurrentFileName, _mode)) 
			{ 
				m_StreamType = ECCFile_Stream_Disk;
				break;
			};
		}
	}
	
	m_pStream->MRTC_AddRef();

	m_FileName = CurrentFileName;

/*	if (!(_mode & CFILE_NOLOG))
	{
		if (m_StreamType == ECCFile_Stream_XDF)
		{
			M_TRACE("Opened file(XDF) %s\n", m_FileName.Str());
		}
		else
		{
			M_TRACE("Opened file %s\n", m_FileName.Str());
		}
	}*/

};

void CCFile::ConnectMemoryStream(TArray<uint8> _lStream, int _mode, int _MinGrow)
{
	DisconnectStream();

	m_bStreamAllocated = true;
	m_StreamType = ECCFile_Stream_Default;

	CStream_Memory* pMS = MNew(CStream_Memory);
	if (pMS == NULL) MemError("ConnectStream");

	m_pStream = pMS;
	pMS->Open(_lStream, _mode, _MinGrow);
}

void CCFile::ConnectMemoryStream(void * _pStream, int _CurrentLen, int _MaxLen, int _Mode)
{
	DisconnectStream();

	m_bStreamAllocated = true;
	m_StreamType = ECCFile_Stream_Default;

	CStream_Memory* pMS = MNew3(CStream_Memory,(uint8 *)_pStream, _CurrentLen, _MaxLen);
	if (pMS == NULL) MemError("ConnectStream");

	m_pStream = pMS;
}



/*void CCFile::OpenExt(CStream* _pStream, int _Mode, bool _bOwnStream, float _Priority, int _NumCacheLines, int _CacheLineSize)
{

}
*/
void CCFile::OpenExt(CStr _name, int _mode, ECompressTypes _eType, ESettings _eSet, float _Priority, aint _NumCacheLines, aint _CacheLineSize)
{
	m_FileName = _name.Ansi();

//OutputDebugString((char*)CFStrF("(CCFile) OPEN %s\n", (const char*) _name));

	ConnectStream(_mode);
	if (m_pStream == NULL) FileError("Open", CFStr(m_FileName) + ", Unable to connect device.", 0);
	m_pStream->OpenExt(m_FileName, _mode, _Priority, _NumCacheLines, _CacheLineSize);

//	fint startpos=m_pStream->Pos();

	// We disable support for compressed files, as it breaks the precaching algo
/*	if ((_mode & CFILE_READ)!=0)
	{
		// Check wether it's a compressed file.
		char Id[5]="    ";
		m_pStream->Read(Id,4);
		if (strcmp(Id,"COMP")==0)
		{
			if (!m_bStreamAllocated)
				Error("Open", "Compression with unallocated stream is not supported.");

			uint32 Len;
			m_pStream->Read(&Len,4);
			if ((int32)Len == m_pStream->Length())
			{
				// It's compressed. Replace the current
				// stream with the "fake" Stream_Compressed.
				CStream* tmp=DNew(CStream_Compressed) CStream_Compressed;
				if (tmp==NULL) MemError("Open");

				uint8 _type;
				m_pStream->Read(&_type,1);

				CCompressorInterface ci;
				CCompress* compressor=ci.GetCompressor((ECompressTypes)_type);
				if (compressor==NULL) Error("Open","Unsupported compression type.");

				m_pStream->Seek(startpos);
				CStream* tmp2=m_pStream;
				m_pStream=tmp;
				((CStream_Compressed*)(m_pStream))->Open(tmp2,compressor,_mode);
			}
			else
				m_pStream->Seek(startpos);
		}
		else
			m_pStream->Seek(startpos);
	}
	else if ((_mode & CFILE_WRITE)!=0 && type!=NO_COMPRESSION)
	{
		if (!m_bStreamAllocated)
			Error("Open", "Compression with unallocated stream is not supported.");

		// "Attach" a compressed stream handler to the current stream.
		CCompressorInterface ci;
		CCompress* compressor=ci.GetCompressor(type,set);
		if (compressor==NULL) Error("Open","Unsupported compression type.");

		// Write Id to file.
		char Id[]="COMP";
		m_pStream->Write(Id,4);
		// Skip FileLength
		uint32 Dummy;
		m_pStream->Write(&Dummy,4);
		// Write compression type.
		uint8 _type=type;
		m_pStream->Write(&_type,1);

		CStream* tmp=DNew(CStream_Compressed) CStream_Compressed;
		if (tmp==NULL) MemError("Open");

		m_pStream->Seek(startpos);
		CStream* tmp2=m_pStream;
		m_pStream=tmp;
		((CStream_Compressed*)(m_pStream))->Open(tmp2,compressor,_mode);
	}*/

	if ((_mode & CFILE_WRITE) != 0 && 
		!((_mode & CFILE_BINARY) != 0) && 
		(_mode & CFILE_UNICODE) != 0 && 
		!(((_mode & CFILE_APPEND) != 0)))
	{
		uint8 UnicodeHeader[2] = { 0xff, 0xfe };
		if (m_pStream->Pos() == 0)
			m_pStream->Write(UnicodeHeader, 2);
	}

	if ((_mode & CFILE_READ) != 0 && 
		!((_mode & CFILE_BINARY) != 0) && 
		!((_mode & CFILE_WRITE) != 0))
	{
		// If we're reading text, check if the file is unicode.
		if(m_pStream->Length() >= 2)
		{
			uint8 UnicodeHeader[2] = { 0x00, 0x00 };
			m_pStream->Read(UnicodeHeader, 2);

			if (UnicodeHeader[0] == 0xff &&
				UnicodeHeader[1] == 0xfe)
				m_pStream->m_Mode |= CFILE_UNICODE;
			else
				m_pStream->RelSeek(-2);
		}
	}
}

void CCFile::SetPriority(fp4 _Priority)
{
	m_pStream->SetPriority(_Priority);
}

void CCFile::Open(TArray<uint8> _lStream, int _Mode, int _MinGrow)
{
	ConnectMemoryStream(_lStream, _Mode, _MinGrow);
	m_FileName.Clear();
}

void CCFile::Open(void * _pStream, int _CurrentLen, int _MaxLen, int _Mode)
{
	ConnectMemoryStream(_pStream, _CurrentLen, _MaxLen, _Mode);
	m_FileName.Clear();
}

void CCFile::Open(CStream* _pStream, int _Mode, bool _bOwnStream)
{
	m_bStreamAllocated = _bOwnStream;
	m_StreamType = ECCFile_Stream_Default;

	m_pStream = _pStream;
	m_FileName.Clear();
}

void CCFile::Open(CStr _name, int _mode, ECompressTypes type, ESettings set)
{
	OpenExt(_name, _mode, type, set);
};

void CCFile::Close()
{
	if (m_pStream)
		m_pStream->Close();
	DisconnectStream();
};

bint CCFile::IsOpen()
{
	return m_pStream != NULL;
}


CStr CCFile::GetFileName()
{
	CStr Str = m_pStream->GetFileName();

	if (Str.Len())
	{
		return Str;
	}
	return m_FileName;
}


/*
CStr CCFile::Readln()
{
	CFILE_CHECKSTREAM("Readln");
	return m_pStream->Readln();
};

void CCFile::Writeln(const CStr &x)
{
	CFILE_CHECKSTREAM("Writeln");
	m_pStream->Writeln(x);
};*/

CStr CCFile::Readln()
{
	if (m_pStream->m_Mode & CFILE_UNICODE)
	{
		wchar Buffer[4096];

		int iBuff = 0;
		while ((iBuff < 4095))
		{
			//Clear position to be overwritten (since read will not always to overwrite it)
			Buffer[iBuff] = 0;

			if (m_pStream->EndOfFile()) 
				break;

			m_pStream->Read(&Buffer[iBuff], sizeof(wchar));
#ifndef CPU_LITTLEENDIAN
			SwapLE((uint16 &)Buffer[iBuff]); //AR-ADD: must convert from UTF-16LE to big endian
#endif

			if (Buffer[iBuff] == wchar(10)) break;

//				FileError("Readln", m_FileName, errno);

			iBuff++;
		}
		Buffer[iBuff] = 0;

		int len = CStr::StrLen(Buffer);
		if (len > 0)
			if (Buffer[len-1] == 0x0d) Buffer[len-1] = 0;

		CStr s;
		s.Capture(Buffer);
		return s;
	}
	else
	{
		char Buffer[4096];

		int iBuff = 0;
		while ((iBuff < 4095))
		{
			//Clear position to be overwritten (since read will not always to overwrite it)
			Buffer[iBuff] = 0;
				
			if (m_pStream->EndOfFile()) 
				break;

			m_pStream->Read(&Buffer[iBuff], 1);

			if (Buffer[iBuff] == char(10)) break;

//				FileError("Readln", m_FileName, errno);

			iBuff++;
		}
		Buffer[iBuff] = 0;

		int len = CStr::StrLen(Buffer);
		if (len > 0)
			if (Buffer[len-1] == 0x0d) Buffer[len-1] = 0;

		CStr s;
		s.Capture(Buffer);
		return s;
	}
};

void CCFile::Writeln(const wchar* _pStr)
{
	if (!(m_pStream->m_Mode & CFILE_UNICODE))
	{
		char Buffer[4096];
		CStrBase::mfscpy(Buffer, CSTR_FMT_ANSI, _pStr, CSTR_FMT_UNICODE);
		Writeln(Buffer);
	}
	else
	{
#if CPU_BIGENDIAN
//# warning "CCFile::Writeln() doesn't not support unicode on big endian machines!"
#endif
		wchar EndLine[2] = { 0x0d, 0x0a };
		m_pStream->Write(_pStr, CStrBase::StrLen(_pStr) * sizeof(wchar));
		m_pStream->Write((char*) &EndLine, 2*sizeof(wchar));
	}
}

void CCFile::Writeln(const char* _pStr)
{
	if (m_pStream->m_Mode & CFILE_UNICODE)
	{
		wchar Buffer[4096];
		CStrBase::mfscpy(Buffer, CSTR_FMT_UNICODE, _pStr, CSTR_FMT_ANSI);
		Writeln(Buffer);
	}
	else
	{
		char EndLine[2] = { 0x0d, 0x0a };
		m_pStream->Write(_pStr, CStrBase::StrLen(_pStr));
		m_pStream->Write((char*) &EndLine, 2);
	}
}

void CCFile::Writeln(CStr _s)
{
	if (_s.IsUnicode())
		Writeln(_s.StrW());
	else
		Writeln(_s.Str());
};


void CCFile::Rename(const CStr& _Name)
{
	CFILE_CHECKSTREAM("Rename");
	CStr Name = _Name.Ansi();
	m_pStream->Rename(Name);
}

void CCFile::Remove()
{
	CFILE_CHECKSTREAM("Remove");
	m_pStream->Remove();
}

#ifdef	NO_INLINE_FILESYSTEM
#undef	M_INLINE
#define	M_INLINE
#include "MFile.inl"
#endif
