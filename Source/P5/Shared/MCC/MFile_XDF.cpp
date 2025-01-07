
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

#include "MFile_XDF.h"

CXDF::CXDF()
{
	m_iCurrentBlock = 0;
	m_iCurrentFile = 0;
	m_CurrentPos = 0;
	m_bRecording = false;
	m_Pause = 0;
	m_XDFPos = 0;
	m_pCurrentBuffer = NULL;

	m_NameHash.Create(256, false, 8);
}

CXDF::~CXDF()
{
	if (m_bRecording)
	{
		CCFile TempFile;
		TempFile.Open(&m_FileStream, CFILE_WRITE | CFILE_BINARY);
		Write(&TempFile);

		m_FileStream.Close();
	}

	Thread_Destroy();

	if (m_pCurrentBuffer)
		delete m_pCurrentBuffer;
	m_UnpackFree.DeleteAll();
	m_UnpackDone.DeleteAll();

}

void CXDF::CXDF_File::Write(CCFile *_pFile, CXDF *_pXDF)
{
//	CStr TempStr = &_pXDF->m_lNameHeap[m_iFileName];
//	M_TRACE("file %s\n", TempStr.Str());
//	TempStr.Write(_pFile);

	// Update file date
	CStr FileName = _pXDF->m_BasePath + _pXDF->GetFileName(m_iFileName);
	CFileInfo Info = CDiskUtil::FileTimeGet(FileName); 
	m_FileDate = Info.m_TimeWrite;

	_pFile->WriteLE(m_iFileName);
	_pFile->WriteLE(m_iFirstBlock);
	_pFile->WriteLE(m_iLastBlock);
	_pFile->WriteLE(m_FileLength);	
	_pFile->WriteLE(m_FileDate);
}

void CXDF::CXDF_File::Read(CCFile *_pFile, CXDF *_pXDF)
{
/*	CStr TempStr;
	TempStr.Read(_pFile);

	int OldLen = _pXDF->m_lNameHeap.Len();

	_pXDF->m_lNameHeap.SetLen(_pXDF->m_lNameHeap.Len() + TempStr.Len() + 1);
	char *pFile = _pXDF->m_lNameHeap.GetBasePtr() + OldLen;
	strcpy(pFile, TempStr);

	m_iFileName = OldLen;*/
	_pFile->ReadLE(m_iFileName);
	_pFile->ReadLE(m_iFirstBlock);
	_pFile->ReadLE(m_iLastBlock);
	_pFile->ReadLE(m_FileLength);	
	_pFile->ReadLE(m_FileDate);
}

void CXDF::CXDF_Block::Write(CCFile *_pFile)
{
	_pFile->WriteLE(m_iNextBlock);
	_pFile->WriteLE(m_iFile);
	M_ASSERT(m_BlockLen > 0, "");
	_pFile->WriteLE(m_BlockLen);
	_pFile->WriteLE(m_FilePos);
	_pFile->WriteLE(m_XDFPos);
}

void CXDF::CXDF_Block::Read(CCFile *_pFile)
{
	_pFile->ReadLE(m_iNextBlock);
	_pFile->ReadLE(m_iFile);
	_pFile->ReadLE(m_BlockLen);
	_pFile->ReadLE(m_FilePos);
	_pFile->ReadLE(m_XDFPos);
	M_ASSERT(m_BlockLen > 0, "");
}

void CXDF::Write(CCFile *_pFile)
{
	int32 NumFiles = m_iCurrentFile+1;
	int32 NumBlocks = m_iCurrentBlock+1;
	int32 NameHeapLen = m_lNameHeap.Len();

//	M_TRACEALWAYS("NumFiles %d, NumBlocks %d, NameHeapLen %d\n", NumFiles, NumBlocks, NameHeapLen);
	_pFile->WriteLE(uint32(EXDFVersion));

	_pFile->WriteLE(NameHeapLen);
	_pFile->Write(m_lNameHeap.GetBasePtr(), NameHeapLen);

	_pFile->WriteLE(NumFiles);

	for (int i = 0; i < NumFiles; ++i)
	{
		m_lFiles[i].Write(_pFile, this);
	}

	_pFile->WriteLE(NumBlocks);

	{
		for (int i = 0; i < NumBlocks; ++i)
		{
			m_lBlocks[i].Write(_pFile);
		}	
	}

}

void CXDF::Read(CCFile *_pFile)
{
	int32 NumFiles;
	int32 NumBlocks;
	int32 NameHeapLen;

	uint32 Version;
	_pFile->ReadLE(Version);
	if (Version != uint32(EXDFVersion))
		Error_static(M_FUNCTION, "Old versions not supported");

	_pFile->ReadLE(NameHeapLen);
	m_lNameHeap.SetLen(NameHeapLen);
	_pFile->Read(m_lNameHeap.GetBasePtr(), NameHeapLen);

	_pFile->ReadLE(NumFiles);

	m_lFiles.SetLen(NumFiles);
	for (int i = 0; i < NumFiles; ++i)
	{
		CXDF_File &File = m_lFiles[i];
		File.Read(_pFile, this);
//		M_TRACEALWAYS("XDF Filename '%s', FirstBlock %u, LastBlock %u, Length %d\r\n", GetFileName(File.m_iFileName), File.m_iFirstBlock, File.m_iLastBlock, File.m_FileLength);
		// Disable for X06
#if !defined(PLATFORM_PS3) && defined(M_Profile)
		CStr FileName = m_BasePath + GetFileName(File.m_iFileName);
		if (CDiskUtil::FileExists(FileName))
		{
			CFileInfo Info = CDiskUtil::FileTimeGet(FileName);
			if (Abs((int64)Info.m_TimeWrite - (int64)File.m_FileDate) > 10000000 * 3) // Allow 3 seconds diff (FATX has 2 seconds resolution)
			{
				File.m_FileDate = 0; // Disable use of this file in XDF
			}
		}		
#endif
	}

	_pFile->ReadLE(NumBlocks);
	m_lBlocks.SetLen(NumBlocks);

	{
		for (int i = 0; i < NumBlocks; ++i)
		{
			m_lBlocks[i].Read(_pFile);
		}	
	}

	m_iCurrentFile = NumFiles - 1;
	m_iCurrentBlock = NumBlocks - 1;

	CreateHash();

}

#include "../../sdk/ZLib/zlib.h"

class CCompressedStreamGenerate
{
public:
	bint m_bInit;
	uint8 m_OutBuffer[16384];
	TArray<uint8> m_CompressedStream;

	z_stream m_Compressor; /* compression stream */

	CCompressedStreamGenerate()
	{
		m_bInit = false;
		m_CompressedStream.SetGrow(128*1024*1024);
		memset(&m_Compressor, 0, sizeof(m_Compressor));
		int err;
#ifdef _DEBUG
		err = deflateInit(&m_Compressor, Z_BEST_SPEED);
#else
		err = deflateInit(&m_Compressor, Z_BEST_COMPRESSION);
#endif

		if (err != Z_OK)
			Error_static(M_FUNCTION, CStrF("deflateInit returned %d", err));
		m_bInit = true;
	}

	~CCompressedStreamGenerate()
	{
		if (m_bInit)
			Finish();
	}

	void AddData(const void *_pData, mint _Len)
	{
		mint Left = _Len;
		m_Compressor.next_in = (Bytef *)_pData;
		m_Compressor.avail_in = Left;

		while (m_Compressor.avail_in)
		{
			if (!m_Compressor.avail_out)
			{
				if (m_Compressor.next_out)
				{
					mint Len = 16384;
					m_CompressedStream.Insertx(m_CompressedStream.Len(), m_OutBuffer, Len);
				}
				m_Compressor.next_out = m_OutBuffer;
				m_Compressor.avail_out = 16384;
			}
			int err = deflate(&m_Compressor, Z_NO_FLUSH);

			if (err != Z_OK && err != Z_BUF_ERROR)
				Error_static("CArchiveWrite::AddData", "deflate returned error");
		}
	}

	void Finish()
	{
		int err = Z_OK;
		while (err != Z_STREAM_END) 
		{
			err = deflate(&m_Compressor, Z_FINISH);
			if (m_Compressor.next_out)
			{
				mint Len = 16384 - m_Compressor.avail_out;
				m_CompressedStream.Insertx(m_CompressedStream.Len(), m_OutBuffer, Len);
			}
			m_Compressor.next_out = m_OutBuffer;
			m_Compressor.avail_out = 16384;
		}
		err = deflateEnd(&m_Compressor);
		if (err != Z_OK)
			LogFile(CStrF("ERROR: deflateEnd returned %d", err));

		m_bInit = false;
	}
};

void CXDF::CompileFile(const char *_pFileSource, const char *_pFileDest, const char *_pFilesBasePath)
{
	CCFile File;
	File.Open(_pFileSource, CFILE_READ | CFILE_BINARY);

	Read(&File);

	CCFile Out;
	m_BasePath = _pFilesBasePath;
	m_BasePath.ReplaceChar('/', '\\');
	m_BasePath.MakeLowerCase();
/*
	for (int i = 0; i <= m_iCurrentBlock; ++i)
	{
		M_TRACE("Then m_iNextBlock %d, m_iFile %d, m_BlockLen %d, m_FilePos %d, m_XDFPos %d\n", m_lBlocks[i].m_iNextBlock, m_lBlocks[i].m_iFile, m_lBlocks[i].m_BlockLen, m_lBlocks[i].m_FilePos, m_lBlocks[i].m_XDFPos);
	}
*/
	Out.Open(_pFileDest, CFILE_WRITE | CFILE_BINARY | CFILE_TRUNC);

	// Start by writing once
	Write(&Out);

	{
		uint64 Size = 0;
		for(int iB = 0; iB < m_iCurrentBlock; iB++)
		{
			Size += m_lBlocks[iB].m_BlockLen;
		}
		CStr SizeStr;
		if(Size > 5*1024*1024)
			SizeStr = CStrF("%.3f MiB", (fp64)Size / (1024.0 * 1024.0));
		else if(Size > 5*1024)
			SizeStr = CStrF("%.3f KiB", (fp64)Size / 1024.0);
		else
			SizeStr = CStrF("%lu B", Size);
		LogFile(CStrF("Amount of data read for XDF '%s' is %s", _pFileSource, SizeStr.GetStr()));
	}

	CCompressedStreamGenerate Generator;

	fint CurPos = 0;
	for (int i = 0; i <= m_iCurrentBlock; ++i)
	{
		m_lBlocks[i].m_XDFPos = CurPos;

		CCFile SourceFile;
		SourceFile.Open(CStrF("%s%s", _pFilesBasePath, &m_lNameHeap[m_lFiles[m_lBlocks[i].m_iFile].m_iFileName]), CFILE_READ | CFILE_BINARY);
		SourceFile.Seek(m_lBlocks[i].m_FilePos);

		m_lFiles[m_lBlocks[i].m_iFile].m_FileLength = SourceFile.Length();

		uint8 Buffer[16384];
		int NumBytes = m_lBlocks[i].m_BlockLen;
		while (NumBytes)
		{
			int ToCopy = Min(NumBytes, 16384);
			SourceFile.Read(Buffer, ToCopy);
			Generator.AddData(Buffer, ToCopy);
			CurPos += ToCopy;
			NumBytes -= ToCopy;
		}
	}

	for(int iF = 0; iF < m_lFiles.Len(); iF++)
	{
		if(m_lFiles[iF].m_FileLength == 0)
		{
			m_lFiles[iF].m_FileLength = CDiskUtil::GetFileSize(CStrF("%s%s", _pFilesBasePath, &m_lNameHeap[m_lFiles[iF].m_iFileName]));
		}
	}
	// Rewrite with right xdfposses
	Out.Seek(0);
	Write(&Out);
	Generator.Finish();
	Out.Write(Generator.m_CompressedStream.GetBasePtr(), Generator.m_CompressedStream.Len());
}

int CXDF::Thread_Main()
{
	m_QuitEvent.ReportTo(&m_EventFree);
	m_FileStream.Seek(m_iDataStart);

	{
		M_LOCK(m_UnpackLock);
		for (int i = 0; i < 3; ++i)
		{
			CUnpackBuffer *pBuffer = DNew(CUnpackBuffer) CUnpackBuffer();
			pBuffer->m_Data.SetLen(384*1024);
			m_UnpackFree.Insert(pBuffer);
		}
	}

	z_stream Stream;
	memset(&Stream, 0, sizeof(Stream));
	int err;
	err = inflateInit(&Stream);
	if (err != Z_OK)
		Error_static(M_FUNCTION, "inflateInit failed");

	const int InBufferSize = 128*1024;
	uint8 ReadBuffer[InBufferSize];
	fint FileLen = m_FileStream.Length() - m_iDataStart;

	while (!Thread_IsTerminating())
	{

		CUnpackBuffer *pBuffer;
		{
			M_LOCK(m_UnpackLock);
			pBuffer = m_UnpackFree.Pop();
		}
		int err = 0;

		if (pBuffer)
		{
			pBuffer->m_DataPos = 0;
			pBuffer->m_DataLen = pBuffer->m_Data.Len();
			Stream.next_out = pBuffer->m_Data.GetBasePtr();
			Stream.avail_out = pBuffer->m_DataLen;

			while (1)
			{
				if (FileLen)
					err = inflate(&Stream, Z_NO_FLUSH);
				else
					err = inflate(&Stream, Z_FINISH);

				if (err == Z_BUF_ERROR)
				{
					if (!Stream.avail_in && FileLen)
					{
						Stream.avail_in = MinMT(FileLen, InBufferSize);
						Stream.next_in = ReadBuffer;
						m_FileStream.Read(Stream.next_in, Stream.avail_in);
						FileLen -= Stream.avail_in;
					}
//					else
//						Error_static(M_FUNCTION, "Internal error");
				}

				if (err != Z_OK && err != Z_BUF_ERROR && err != Z_STREAM_END)
					Error_static(M_FUNCTION, "Internal error");

				if (Stream.avail_out == 0)
				{
					M_LOCK(m_UnpackLock);
					m_UnpackDone.Insert(pBuffer);
					m_EventDone.Signal();
					break;
				}
				if (err == Z_STREAM_END)
				{
					pBuffer->m_DataLen -= Stream.avail_out;
					M_LOCK(m_UnpackLock);
					m_UnpackDone.Insert(pBuffer);
					m_EventDone.Signal();
					break; // We are done
				}
			}

			if (err == Z_STREAM_END)
				break;
		}
		else
			m_EventFree.Wait();
	}

	err = inflateEnd(&Stream);

	if (err != Z_OK)
		Error_static(M_FUNCTION, "inflateEnd failed");

	return 0;
}

bint CXDF::Read_NotInline(fint _Pos, void *_pData, mint _Len)
{
	fint ToSeek = _Pos - m_XDFPos;
	if (ToSeek < 0)
	{
		M_TRACEALWAYS("XDF Warning failed negative delete: %d\n", ToSeek);
		return false;
		//Error_static(M_FUNCTION, "Cannot seek backwards in stream");
	}
	else if(ToSeek)
	{
		M_TRACEALWAYS("XDF Warning Seeking: %d\n", ToSeek);
	}

	while (ToSeek)
	{
		CUnpackBuffer *pBuffer = m_pCurrentBuffer;
		while (!pBuffer)
		{
			{
				M_LOCK(m_UnpackLock);
				pBuffer = m_UnpackDone.Pop();
			}
			if (!pBuffer)
				m_EventDone.Wait(); // Wait for buffer to unpack
		}

		mint MaxLen = pBuffer->m_DataLen - pBuffer->m_DataPos;
		mint ToRead = MinMT(MaxLen, ToSeek);

		ToSeek -= ToRead;
		pBuffer->m_DataPos += ToRead;
		m_XDFPos += ToRead;

		if (pBuffer->m_DataPos == pBuffer->m_DataLen)
		{
			M_LOCK(m_UnpackLock);
			m_UnpackFree.Insert(pBuffer);
			m_EventFree.Signal();
			m_pCurrentBuffer = NULL;
		}
		else
			m_pCurrentBuffer = pBuffer;
	}

	uint8 *pData = (uint8 *)_pData;

	while (_Len)
	{
		CUnpackBuffer *pBuffer = m_pCurrentBuffer;
		while (!pBuffer)
		{
			{
				M_LOCK(m_UnpackLock);
				pBuffer = m_UnpackDone.Pop();
			}
			if (!pBuffer)
				m_EventDone.Wait(); // Wait for buffer to unpack
		}

		mint MaxLen = pBuffer->m_DataLen - pBuffer->m_DataPos;
		mint ToRead = Min(MaxLen, _Len);
		memcpy(pData, pBuffer->m_Data.GetBasePtr() + pBuffer->m_DataPos, ToRead);

		pData += ToRead;
		_Len -= ToRead;
		pBuffer->m_DataPos += ToRead;
		m_XDFPos += ToRead;

		if (pBuffer->m_DataPos == pBuffer->m_DataLen)
		{
			M_LOCK(m_UnpackLock);
			m_UnpackFree.Insert(pBuffer);
			m_EventFree.Signal();
			m_pCurrentBuffer = NULL;
		}
		else
		{
			M_ASSERT(_Len == 0, "");
			m_pCurrentBuffer = pBuffer;
		}

	}
	return true;
}


int CXDF::GetFileCreate(const char *_pFile)
{
	int Index = m_NameHash.GetIndex(_pFile);

	if (Index < 0)
	{
		if (m_lFiles.Len() < m_iCurrentFile + 2)
			m_lFiles.SetLen(m_iCurrentFile + 16);

		Index = ++m_iCurrentFile;

		int OldLen = m_lNameHeap.Len();

		m_lNameHeap.SetLen(m_lNameHeap.Len() + CStrBase::StrLen(_pFile) + 1);

		char *pFile = m_lNameHeap.GetBasePtr() + OldLen;

		strcpy(pFile, _pFile);
		m_lFiles[Index].m_iFileName = OldLen;
		m_lFiles[Index].m_iFirstBlock = 0xffFFffFF;
		m_lFiles[Index].m_iLastBlock = 0xffFFffFF;
		m_lFiles[Index].m_FileLength = 0;
		

		CreateHash();

		//m_NameHash.Insert(Index, pFile);
	}

	return Index;
}

void CXDF::CreateHash()
{
	int32 NumFiles = m_iCurrentFile+1;
	m_NameHash.Create(NumFiles, false, 8);

	for (int i = 0; i < NumFiles; ++i)
	{
		m_NameHash.Insert(i, &m_lNameHeap[m_lFiles[i].m_iFileName]);
	}
}
void CXDF::Record_Start(const char *_pFile, const char *_pBasePath)
{
	m_bRecording = true;
//	m_spFile = DNew(CCFile) CCFile();
	m_File = _pFile;
	m_BasePath = _pBasePath;
	m_BasePath.MakeLowerCase();
	m_iCurrentBlock = -1;
	m_iCurrentFile = -1;
	m_iWorkFile = -1;
	m_iWorkBlock = -1;
	m_CurrentPos = 0;
	m_bCreateNewBlock = true;
	M_TRY
	{
		CDiskUtil::CreatePath(m_File.GetPath());
	}
	M_CATCH(
	catch (CCException)
	{
	}
	)
	m_FileStream.Open(m_File, CFILE_WRITE|CFILE_BINARY|CFILE_TRUNC|CFILE_NOLOG);
}


void CXDF::Open(const char *_pFileSource, const char *_pFilesBasePath)
{
	m_File = _pFileSource;
	m_File.ReplaceChar('/', '\\');
	m_File.MakeLowerCase();
	m_BasePath = _pFilesBasePath;
	m_BasePath.ReplaceChar('/', '\\');
	m_BasePath.MakeLowerCase();

//	m_Stream.OpenExt(m_File, CFILE_READ|CFILE_BINARY|CFILE_NOLOG,0, 8, 128*1024);
	m_FileStream.OpenExt(m_File, CFILE_READ|CFILE_BINARY|CFILE_NOLOG);
	CCFile TempFile;
	TempFile.Open(&m_FileStream, 0);
	Read(&TempFile);
	m_iDataStart = TempFile.Pos();
	m_iWorkBlock = 0;
	m_XDFPos = 0;
	MRTC_Thread::Thread_Create(NULL, 256*1024); // Room for read buffer
}

void CXDF::Record_FinishBlock()
{
	if (m_iWorkBlock >= 0)
	{
		m_lBlocks[m_iWorkBlock].m_BlockLen = m_CurrentPos - m_lBlocks[m_iWorkBlock].m_FilePos;
		M_ASSERT(m_lBlocks[m_iWorkBlock].m_BlockLen > 0, "");
		m_iWorkBlock = -1;
	}
}


static bint FindFilesMatchPattern(const char *_pFile, const char *_pPattern)
{
	CFStr Parse;
	Parse.Capture(_pFile);
	Parse.MakeLowerCase();
	CFStr Pattern;
	Pattern.Capture(_pPattern);
	Pattern.MakeLowerCase();

	const char *pParse = Parse.Str();
	const char *pPattern = Pattern.Str();

	while (*pParse && *pPattern)
	{
		if (*pPattern == '*')
		{
			++pPattern;
			while (*pParse && *pParse != *pPattern) // Search until we find pattern charater we don't allow such useless things like "*?.exe"
			{
				++pParse;
			}			
		}
		else if (*pPattern == '?')
		{
			++pPattern;
			++pParse;
		}
		else
		{
			if (*pPattern != *pParse)
				break; // break search failed

			++pPattern;
			++pParse;
		}
	}

	if (*pParse == *pPattern)
		return true;

	return false;
}

void CXDF::EnumFiles(TArray_Sortable<CDir_FileRec> &_Dest, CStr _Path)
{
	CFStr Fixed = _Path;
	StrReplaceChar((char *)Fixed, '/', '\\');
	Fixed.MakeLowerCase();
	// Not inside base path
	if (Fixed.Find(m_BasePath.Str()) != 0)
		return;

	CFStr Directory = Fixed.Right(Fixed.Len() - m_BasePath.Len());

	CFStr FindPart;
	if (Directory == "")
		return;

	int iFind = Directory.FindReverse("\\");
	if (iFind >= 0)
	{
		FindPart.Capture(Directory.Str() + iFind + 1);
		Directory = Directory.Left(iFind + 1);
	}
	else
	{
		FindPart = Directory;
		Directory = "";
	}

	int nFiles = m_lFiles.Len();

	for (int i = 0; i < nFiles; ++i)
	{
		CXDF_File &File = m_lFiles[i];
		CFStr FileName = GetFileName(File.m_iFileName);
		if (FileName.CompareSubStr(Directory) == 0)
		{
			CFStr Right;
			Right = FileName.Right(FileName.Len() - Directory.Len());
			if (Right.Find("\\") < 0)
			{
				// We have a file. We only support files. Create the directory structure
				if (FindFilesMatchPattern(Right.Str(), FindPart))
				{
					int CurLen = _Dest.Len();
					for (int i = 0; i < CurLen; ++i)
					{
						if (_Dest[i].m_Name.CompareNoCase(Right) == 0)
						{
							CurLen = -1;
							break;
						}
					}
					if (CurLen >= 0)
					{
						_Dest.Add(CDir_FileRec(Right));
					}
				}
			}				
		}
	}
}


void CXDF::Record_NewBlock()
{
	if (m_lBlocks.Len() < m_iCurrentBlock + 2)
		m_lBlocks.SetLen(m_iCurrentBlock + 16);

	++m_iCurrentBlock;
	
	m_lBlocks[m_iCurrentBlock].m_FilePos = m_CurrentPos;
	m_lBlocks[m_iCurrentBlock].m_iFile = m_iWorkFile;

	int LastBlock = m_lFiles[m_iWorkFile].m_iLastBlock;
	if (LastBlock >= 0)
		m_lBlocks[LastBlock].m_iNextBlock = m_iCurrentBlock;
	else
		m_lFiles[m_iWorkFile].m_iFirstBlock = m_iCurrentBlock;

	m_lFiles[m_iWorkFile].m_iLastBlock = m_iCurrentBlock;
	m_lBlocks[m_iCurrentBlock].m_iNextBlock = -1;	

	m_iWorkBlock = m_iCurrentBlock;

	m_bCreateNewBlock = false;

//	M_TRACEALWAYS("Record_NewBlock NumFiles %d, NumBlocks %d\n", m_iCurrentFile, m_iCurrentBlock);

}

void CXDF::Record_Seek(int64 _ToByte)
{	
	if (CDiskUtil::XDF_IsPaused())
		return;
	Record_FinishBlock();
	m_CurrentPos = _ToByte; 
	m_bCreateNewBlock = true;

}

void CXDF::Record_Read(int64 _ToByte)
{
	if (CDiskUtil::XDF_IsPaused())
		return;
	if (m_iWorkFile < 0)
	{
//		M_TRACEALWAYS("Read with no workfile\n");
		return;
	}
	
	if (m_bCreateNewBlock)
		Record_NewBlock();

	m_CurrentPos = _ToByte; 
}

void CXDF::Record_OpenFile(const char *_pFile)
{
	CStr File(_pFile);
	File.ReplaceChar('/', '\\');
	File.MakeLowerCase();
	if (File.Find(m_BasePath) != 0)
		return;

	if (m_iWorkFile >= 0)
	{
		LogFile(CStrF("Trying to open: %s. File already open: %s", _pFile, GetFileName(m_iWorkFile)));
		M_TRACEALWAYS("Trying to open: %s. File already open: %s\n", _pFile, GetFileName(m_iWorkFile));
		M_BREAKPOINT;
	}

	File = File.Right(File.Len() - m_BasePath.Len());

	m_iWorkFile = GetFileCreate(File);

	m_CurrentPos = 0; 

	m_bCreateNewBlock = true;
}

void CXDF::Record_CloseFile(const char *_pFile)
{
	Record_FinishBlock();

	m_iWorkFile = -1;
}
