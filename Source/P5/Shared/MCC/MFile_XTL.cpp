#include "PCH.h"

#ifdef PLATFORM_XBOX
#include "MFile_XTL.h"

	#include <xtl.h>


#if 0
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CfstreamEmulator
|__________________________________________________________________________________________________
\*************************************************************************************************/

CfstreamEmulator::CfstreamEmulator()
{
	FileHandle = INVALID_HANDLE_VALUE;
	ErrorFlags = 0;
	CurrentPosition = 0;
}

~CfstreamEmulator()
{
	close();
}

void CfstreamEmulator::open(const char *FileName, DWORD OpenMode)
{
	close();
	clear();

	CurrentPosition = 0;

	DWORD AccessFlags = 0;
	DWORD ShareFlags = FILE_SHARE_READ | FILE_SHARE_WRITE;
	DWORD CreateFlags = 0;
	DWORD OpenFlags = 0;

	if (OpenMode & ios::in)
		AccessFlags |= GENERIC_READ;

	if (OpenMode & ios::out)
		AccessFlags |= GENERIC_WRITE;

	if (OpenMode & ios::nocreate)
		CreateFlags |= OPEN_EXISTING;

	if (OpenMode & ios::nocreate)
		CreateFlags = OPEN_EXISTING;
	else if (OpenMode & ios::noreplace)
		CreateFlags = CREATE_NEW;
	else if (OpenMode & ios::trunc)
		CreateFlags = CREATE_ALWAYS;
	else if (OpenMode & ios::trunc)
		CreateFlags = CREATE_ALWAYS;
	else if (OpenMode & ios::app)
	{
		Error("CfstreamEmulator", "CfstreamEmulator does not support ios::app.");			
	}
	else if (OpenMode & ios::ate)
	{
		CreateFlags = OPEN_ALWAYS;
	}
	else
		CreateFlags = OPEN_ALWAYS;

	if (!(OpenMode & ios::binary))
		Error("CfstreamEmulator", "CfstreamEmulator does not support non binary operations.");			

	FileHandle = CreateFile(FileName, AccessFlags, ShareFlags, NULL, CreateFlags, FILE_FLAG_RANDOM_ACCESS, NULL);


	if (FileHandle == INVALID_HANDLE_VALUE)
		ErrorFlags |= ios::badbit;
	else
	{
		if (OpenMode & ios::ate)
		{
			seekp(0, ios::end);
		}
	}

}

void CfstreamEmulator::close()
{
	if (FileHandle != INVALID_HANDLE_VALUE)
	{	
		if (!CloseHandle(FileHandle))
		{
			ErrorFlags |= ios::badbit;
		}

		FileHandle = INVALID_HANDLE_VALUE;
	}
}

bool CfstreamEmulator::CheckFileOpen()
{
	if (FileHandle == INVALID_HANDLE_VALUE)
	{
		ErrorFlags |= ios::failbit;
		return false;
	}
	else
		return true;

}

void CfstreamEmulator::read(void *Buffer, int Size)
{
	if (!CheckFileOpen())
		return;

	DWORD ReadBytes = 0;

	if (!ReadFile(FileHandle, Buffer, Size, &ReadBytes, NULL))
	{
		ErrorFlags |= ios::badbit;
	}
	else if (!ReadBytes)
	{
		ErrorFlags |= ios::eofbit;
	}

	CurrentPosition += Size;
}


void CfstreamEmulator::write(void *Buffer, int Size)
{
	if (!CheckFileOpen())
		return;

	DWORD WriteBytes = 0;

	if (!WriteFile(FileHandle, Buffer, Size, &WriteBytes, NULL))
	{
		ErrorFlags |= ios::badbit;
	}

	CurrentPosition += Size;
}

void CfstreamEmulator::seekp( streamoff off, ios::seek_dir dir = ios::beg)
{
	if (!CheckFileOpen())
		return;

	DWORD Direction = 0;

	if (dir == ios::beg)
	{
		Direction = FILE_BEGIN;
	}
	else if (dir == ios::cur)
	{
		Direction = FILE_CURRENT;
	}
	else if (dir == ios::end)
		Direction = FILE_END;

	DWORD Pos = SetFilePointer(FileHandle, off, NULL, Direction);

	if (Pos == 0xFFFFFFFF)
	{
		ErrorFlags |= ios::badbit;
	}
	else
	{
		CurrentPosition = Pos;
	}
}

streampos CfstreamEmulator::tellp()
{
	if (!CheckFileOpen())
		return 0;

	return CurrentPosition;
}

bool CfstreamEmulator::good()
{
	return (ErrorFlags & (ios::failbit | ios::badbit)) == 0;
}

bool CfstreamEmulator::eof()
{
	return (ErrorFlags & (ios::eofbit)) != 0;
}

bool CfstreamEmulator::bad()
{
	return (ErrorFlags & (ios::badbit)) != 0;
}

bool CfstreamEmulator::is_open()
{

	return FileHandle != INVALID_HANDLE_VALUE;
}

void CfstreamEmulator::clear( int nState = 0 )
{
	if (!nState)
		ErrorFlags = 0;
	else
	{
		ErrorFlags &= ~nState;
	}
}

void CfstreamEmulator::getline( const char* puch, int nCount, unsigned char delim = '\n' )
{
	if (!CheckFileOpen())
		return;

	M_ASSERT(0, "Not implemented.");

}

void CfstreamEmulator::read(const char *Buffer, int Size)
{
	read((void *)Buffer, Size);
}

void CfstreamEmulator::write(const char *Buffer, int Size)
{
	write((void *)Buffer, Size);
}

#endif // 0

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CfstreamEmulator_CachedAsync::CCachLine
|__________________________________________________________________________________________________
\*************************************************************************************************/

//#define CfstreamEmulator_CachedAsync_NO_BUFFERING

CfstreamEmulator_CachedAsync::CCachLine::CCachLine()
{
	m_Overlapped.Internal = 0;
	m_Overlapped.InternalHigh = 0;
	m_Overlapped.Offset = 0;
	m_Overlapped.OffsetHigh = 0;
	m_Overlapped.hEvent = 0;

	m_bDirtyFlag = false;
	m_bInUse = false;
	m_bPending = false;
	m_bLastOpWrite = false;
}
void CfstreamEmulator_CachedAsync::CCachLine::WaitForComplete(HANDLE hFile)
{
	if (!m_bPending)
		return;

	DWORD nBytesRead = 0;
	while (1)
	{
		double Timer;
		T_Start(Timer);
		if (!GetOverlappedResult(hFile, &m_Overlapped, &nBytesRead, false))
		{
			if (!(GetLastError() == ERROR_IO_INCOMPLETE))
				break;
		}
		else
			break;
		T_Stop(Timer);

		if (Timer/GetCPUFrequency() > 0.01f)
		{
			OutputDebugString(T_String("GetOverlappedResult is taking to long time %d", Timer));
			OutputDebugString("\n");
		}


		Sleep(1);
	}

	M_ASSERT(nBytesRead, "LALA");
//			OutputDebugString(CStrF("GetOverlapped %d\n", nBytesRead));

	m_bPending = false;
}

bool CfstreamEmulator_CachedAsync::CCachLine::IsPending(HANDLE hFile)
{
	if (m_bPending)
	{
		DWORD nBytesRead = 0;
		if (!GetOverlappedResult(hFile, &m_Overlapped, &nBytesRead, false))
		{
			if (GetLastError() == ERROR_IO_INCOMPLETE)
				m_bPending = true;
			else
				m_bPending = false;
		}
		else
			m_bPending = false;

//				OutputDebugString(CStrF("IsPending %d\n", nBytesRead));

		return m_bPending;
	}
	else
		return false;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CfstreamEmulator_CachedAsync
|__________________________________________________________________________________________________
\*************************************************************************************************/
CfstreamEmulator_CachedAsync::CCachLine *CfstreamEmulator_CachedAsync::GetFreeCacheLine(CfstreamEmulator_CachedAsync::CCachLine *_DontTouch)
{
	CCachLine *BestMatch = NULL;
	int NumDirty = 0;

	for (int i = 0; i < ECacheLines; ++i)
	{
		// Dont consider the dont doutch member
		if (&m_CacheLines[i] == _DontTouch)
			continue;

		// If its unsused its optimal
		if (!m_CacheLines[i].m_bInUse)
		{
			// Safe to return
			BestMatch = &m_CacheLines[i];
			break;
		}
		
		if (m_CacheLines[i].m_bDirtyFlag)
			++NumDirty;
		
		if (BestMatch)
		{
			if (BestMatch->m_bDirtyFlag && !m_CacheLines[i].m_bDirtyFlag)
				BestMatch = &m_CacheLines[i];
			else if (BestMatch->IsPending(m_FileHandle) && !m_CacheLines[i].IsPending(m_FileHandle))
				BestMatch = &m_CacheLines[i];
			else if (m_CacheLines[i].m_Priority < BestMatch->m_Priority)
			{
				BestMatch = &m_CacheLines[i];
			}
		}
		else
		{
			BestMatch = &m_CacheLines[i];
		}
	}
	
	while (NumDirty > (ECacheLines/2))
	{
		CCachLine *BestMatch = NULL;
		
		for (int i = 0; i < ECacheLines; ++i)
		{
			// Dont consider the dont doutch member
			if (&m_CacheLines[i] == _DontTouch)
				continue;
			
			if (!m_CacheLines[i].m_bInUse)
				continue;
			
			if (m_CacheLines[i].m_bDirtyFlag)
			{
				++NumDirty;
				
				if (BestMatch)
				{
					if (m_CacheLines[i].m_Priority < BestMatch->m_Priority)
					{
						BestMatch = &m_CacheLines[i];
					}
				}
				else
				{
					BestMatch = &m_CacheLines[i];
				}
			}
		}
		
		if (BestMatch)
			WriteCacheLine(BestMatch);
		else
			break;
	}
	
	if (BestMatch->m_bInUse)
	{
		if (BestMatch->IsPending(m_FileHandle))
			BestMatch->WaitForComplete(m_FileHandle);
		
		if (BestMatch->m_bDirtyFlag)
			WriteCacheLine(BestMatch, false);
	}
	
	BestMatch->m_Overlapped.Internal = 0;
	BestMatch->m_bDirtyFlag = false;
	BestMatch->m_bInUse = true;
	BestMatch->m_bLastOpWrite = false;
	BestMatch->m_bPending = false;
	BestMatch->m_Priority = m_CurrentCacheLinePriority++;

	return BestMatch;
}

CfstreamEmulator_CachedAsync::CCachLine *CfstreamEmulator_CachedAsync::GetCacheLine(int _Offset, bool _ForWrite)
{
	if (m_LastOpCacheLine && ((_Offset - (int)m_LastOpCacheLine->m_Overlapped.Offset) >= 0) && ((_Offset - (int)m_LastOpCacheLine->m_Overlapped.Offset) < ECacheLineSize))
	{
		// Check if we are on same cacheline as last time first
		return m_LastOpCacheLine;
	}

	CCachLine *ToReturn = NULL;
	// Check if already loaded
	for (int i = 0; i < ECacheLines; ++i)
	{
		if (m_CacheLines[i].m_bInUse && ((_Offset - (int)m_CacheLines[i].m_Overlapped.Offset) >= 0) && ((_Offset - (int)m_CacheLines[i].m_Overlapped.Offset) < ECacheLineSize))
		{
			if (m_CacheLines[i].IsPending(m_FileHandle))
			{
				if (m_CacheLines[i].m_bLastOpWrite)
				{
					// Write operation in progress
					if (_ForWrite)
					{
						m_CacheLines[i].m_bDirtyFlag = true;
					}
				}
				else
				{
					// If we are currently reading this, lets finish
					m_CacheLines[i].WaitForComplete(m_FileHandle);
				}
			}
			
			ToReturn = &m_CacheLines[i];
			break;
		}
	}
	
	if (!ToReturn)
	{
		ToReturn = GetFreeCacheLine();
		ToReturn->m_Overlapped.Offset = _Offset & ~((ECacheLineSize - 1));

		if (_ForWrite && ((int)m_CurrentFileSizeInFile - (int)_Offset) <= 0)
		{
		}
		else
		{
			// We have to read this to the end
			ReadCacheLine(ToReturn, false);
		}
	}
	
	m_LastOpCacheLine = ToReturn;
	
#ifdef CfstreamEmulator_CachedAsync_NO_BUFFERING
	if (!_ForWrite)
	{
		// We are not going to use this for write so lets pre read the next block for next read
		
		int OffsetToGet = ToReturn->m_Overlapped.Offset + ECacheLineSize;
		
		CCachLine *ToPreFetch = NULL;
		
		if (((int)m_CurrentFileSizeInFile - (int)OffsetToGet) > 0) // Dont read past end of file
		{				
			for (int i = 0; i < ECacheLines; ++i)
			{
				if (m_CacheLines[i].m_bInUse && m_CacheLines[i].m_Overlapped.Offset == OffsetToGet)
				{
					ToPreFetch = &m_CacheLines[i];
					break;
				}
			}
			if (!ToPreFetch)
			{
				// The next cache line does not exist, lets get it
				
				ToPreFetch = GetFreeCacheLine(ToReturn);
				
				ToPreFetch->m_Overlapped.Offset = OffsetToGet;
				
				ReadCacheLine(ToPreFetch);
			}			
		}
		
	}
#endif
	
	return ToReturn;
	
}

void CfstreamEmulator_CachedAsync::WriteCacheLine(CfstreamEmulator_CachedAsync::CCachLine *_Line, bool _Async)
{
	if (_Line->m_bDirtyFlag)
	{
		if (_Line->IsPending(m_FileHandle))
			_Line->WaitForComplete(m_FileHandle);
		
		if (((int)m_CurrentFileSizeInFile - (int)(_Line->m_Overlapped.Offset + ECacheLineSize)) < 0)
		{
			GrowFileSize((_Line->m_Overlapped.Offset + ECacheLineSize));
		}

		DWORD Written = 0;
		_Line->m_Overlapped.Internal = 0;
		if (!WriteFile(m_FileHandle, _Line->m_Data, ECacheLineSize, &Written, &_Line->m_Overlapped))
		{
			if (GetLastError() == ERROR_IO_PENDING)
				_Line->m_bPending = true;
			else
				_Line->m_bPending = false;

		}
		else
			_Line->m_bPending = false;


		_Line->m_bDirtyFlag = false;

		if (!_Async)
		{
			_Line->WaitForComplete(m_FileHandle);
			_Line->m_bPending = false;
		}
		else
		{
			_Line->m_bLastOpWrite = true;
		}
	}
	else if (_Line->IsPending(m_FileHandle))
	{
		if (!_Async)
		{
			_Line->WaitForComplete(m_FileHandle);
		}
	}
}

void CfstreamEmulator_CachedAsync::ReadCacheLine(CfstreamEmulator_CachedAsync::CCachLine *_Line, bool _Async)
{
	if (_Line->IsPending(m_FileHandle))
	{
		// Already reading
		if (!_Async)
		{
			_Line->WaitForComplete(m_FileHandle);
			_Line->m_bPending = false;
		}
		return;
	}
	
	DWORD Read = (_Line->m_Overlapped.Offset + ECacheLineSize);
	_Line->m_Overlapped.Internal = 0;

	if (((int)m_CurrentFileSizeInFile - (int)Read) < 0)
	{
		Read = (int)m_CurrentFileSizeInFile - (int)_Line->m_Overlapped.Offset;
		Read = (Read + (m_SectorSize - 1)) & (~(m_SectorSize - 1));
	}
	else
	{
		Read = ECacheLineSize;
	}

	if (!ReadFile(m_FileHandle, _Line->m_Data, Read, &Read, &_Line->m_Overlapped))
	{
		if (GetLastError() == ERROR_IO_PENDING)
			_Line->m_bPending = true;
		else
			_Line->m_bPending = false;
		
	}
	else
		_Line->m_bPending = false;
	
	if (!_Async)
	{
		_Line->WaitForComplete(m_FileHandle);
		_Line->m_bPending = false;
	}
	else
	{
		_Line->m_bLastOpWrite = false;
	}
}

void CfstreamEmulator_CachedAsync::Flush()
{
	m_LastOpCacheLine = NULL;

	// Flush all cache.. this is so large operations can be done in sync
	for (int i = 0; i < ECacheLines; ++i)
	{
		if (m_CacheLines[i].m_bDirtyFlag)
		{
			WriteCacheLine(&m_CacheLines[i], false);
		}
		else if (m_CacheLines[i].IsPending(m_FileHandle))
			m_CacheLines[i].WaitForComplete(m_FileHandle);

		m_CacheLines[i].m_bInUse = false;
	}

}

void CfstreamEmulator_CachedAsync::GrowFileSize(int NewSize)
{
	SetFilePointer(m_FileHandle, NewSize, NULL, FILE_BEGIN);
	SetEndOfFile(m_FileHandle);

	m_CurrentFileSizeInFile = NewSize;
}

CfstreamEmulator_CachedAsync::CfstreamEmulator_CachedAsync()
{
	m_FileHandle = INVALID_HANDLE_VALUE;
	m_ErrorFlags = 0;
	m_CurrentPosition = 0;
	m_CurrentCacheLinePriority = 0;
	m_CurrentFileSize = 0;
	m_CurrentFileSizeInFile = 0;

	m_CacheLines = DNew(CCachLine) CCachLine[ECacheLines];
	m_LastOpCacheLine = NULL;
}

CfstreamEmulator_CachedAsync::~CfstreamEmulator_CachedAsync()
{
	close();

	delete [] m_CacheLines;
}

void CfstreamEmulator_CachedAsync::open(const char *FileName, DWORD OpenMode)
{
	close();
	clear();

	m_CurrentPosition = 0;

	DWORD AccessFlags = 0;
	DWORD ShareFlags = FILE_SHARE_READ | FILE_SHARE_WRITE;
	DWORD CreateFlags = 0;
	DWORD OpenFlags = 0;

	if (OpenMode & ios::in)
		AccessFlags |= GENERIC_READ;

	if (OpenMode & ios::out)
		AccessFlags |= GENERIC_WRITE;

	if (OpenMode & ios::nocreate)
		CreateFlags |= OPEN_EXISTING;

	if (OpenMode & ios::nocreate)
		CreateFlags = OPEN_EXISTING;
	else if (OpenMode & ios::noreplace)
		CreateFlags = CREATE_NEW;
	else if (OpenMode & ios::trunc)
		CreateFlags = CREATE_ALWAYS;
	else if (OpenMode & ios::app)
	{
		Error("CfstreamEmulator", "CfstreamEmulator does not support ios::app.");			
	}
	else if (OpenMode & ios::ate)
	{
		CreateFlags = OPEN_ALWAYS;
	}
	else
		CreateFlags = OPEN_ALWAYS;

	if (!(OpenMode & ios::binary))
		Error("CfstreamEmulator", "CfstreamEmulator does not support non binary operations.");			

#ifdef CfstreamEmulator_CachedAsync_NO_BUFFERING
	m_FileHandle = CreateFile(FileName, AccessFlags, ShareFlags, NULL, CreateFlags, FILE_FLAG_OVERLAPPED| FILE_FLAG_NO_BUFFERING, NULL);
#else
	m_FileHandle = CreateFile(FileName, AccessFlags, ShareFlags, NULL, CreateFlags, FILE_FLAG_OVERLAPPED, NULL);
#endif

	if (m_FileHandle == INVALID_HANDLE_VALUE)
		m_ErrorFlags |= ios::badbit;
	else
	{
		m_CurrentFileSizeInFile = m_CurrentFileSize = GetFileSize(m_FileHandle, NULL);
		char Path[4];
		Path[0] = FileName[0];
		Path[1] = FileName[1];
		Path[2] = FileName[2];
		Path[3] = 0;

		m_SectorSize = XGetDiskSectorSize(Path);

		if (OpenMode & ios::ate)
		{
			seekp(0, ios::end);
		}
	}

}

void CfstreamEmulator_CachedAsync::close()
{
	if (m_FileHandle != INVALID_HANDLE_VALUE)
	{	
		Flush();

		if (m_CurrentFileSizeInFile != m_CurrentFileSize)
		{
			// Clip off end of file
//				LARGE_INTEGER NewSize;
//				NewSize.QuadPart = m_CurrentFileSize;
//				DWORD Test = SetFilePointerEx(m_FileHandle, NewSize, NULL, FILE_BEGIN);

			SetFilePointer(m_FileHandle, m_CurrentFileSize, NULL, FILE_BEGIN);
			SetEndOfFile(m_FileHandle);
		}			

		if (!CloseHandle(m_FileHandle))
		{
			m_ErrorFlags |= ios::badbit;
		}

		m_FileHandle = INVALID_HANDLE_VALUE;
	}
}

bool CfstreamEmulator_CachedAsync::CheckFileOpen()
{
	if (m_FileHandle == INVALID_HANDLE_VALUE)
	{
		m_ErrorFlags |= ios::failbit;
		return false;
	}
	else
		return true;

}

void CfstreamEmulator_CachedAsync::read(void *Buffer, int Size)
{
	if (!Size || !CheckFileOpen())
		return;

/*		if (!Buffer)
	{
		m_ErrorFlags |= ios::badbit;
		return;
	}*/

	int BytesToRead = Size;

	if ((m_CurrentPosition + Size) >= m_CurrentFileSize)
	{
		m_ErrorFlags |= ios::eofbit;
		BytesToRead = m_CurrentFileSize - m_CurrentPosition;
	}

	CCachLine *CurrentLine = GetCacheLine(m_CurrentPosition, false);

	if (BytesToRead <= 8 && ((m_CurrentPosition - CurrentLine->m_Overlapped.Offset) < (ECacheLineSize - 8)))
	{
		uint8* PosInCacheLine = CurrentLine->m_Data + (m_CurrentPosition - CurrentLine->m_Overlapped.Offset);
		uint8* OutBuffer = (uint8*)Buffer;
		
		// Spead up read of small values

		if (BytesToRead & 1)
		{
			*OutBuffer = *PosInCacheLine;
			++PosInCacheLine;
			++OutBuffer;
		}

		if (BytesToRead & 2)
		{
			*((int16 *)OutBuffer) = *((int16 *)PosInCacheLine);
			PosInCacheLine += 2;
			OutBuffer += 2;
		}

		if (BytesToRead & 4)
		{
			*((int32 *)OutBuffer) = *((int32 *)PosInCacheLine);
			PosInCacheLine += 4;
			OutBuffer += 4;
		}
		
		if (BytesToRead & 8)
		{
			*((int64 *)OutBuffer) = *((int64 *)PosInCacheLine);
		}

		m_CurrentPosition += BytesToRead;
	}
	else
	{
		uint8* OutBuffer = (uint8*)Buffer;

		while (BytesToRead)
		{
			CCachLine *CurrentLine = GetCacheLine(m_CurrentPosition, false);
			uint8* PosInCacheLine = CurrentLine->m_Data + (m_CurrentPosition - CurrentLine->m_Overlapped.Offset);
			int ReadBytes = ECacheLineSize - (m_CurrentPosition - CurrentLine->m_Overlapped.Offset);
			if (ReadBytes > BytesToRead)
				ReadBytes = BytesToRead;

			memcpy(OutBuffer, PosInCacheLine, ReadBytes);

			OutBuffer += ReadBytes;
			BytesToRead -= ReadBytes;
			m_CurrentPosition += ReadBytes;
		}
	}

}

void CfstreamEmulator_CachedAsync::write(void *Buffer, int Size)
{
	if (!Size || !CheckFileOpen())
		return;

/*		if (!Buffer)
	{
		m_ErrorFlags |= ios::badbit;
		return;
	}*/

	int BytesToWrite = Size;

	if ((m_CurrentPosition + Size) >= m_CurrentFileSize)
	{
		m_CurrentFileSize = (m_CurrentPosition + Size);
	}

	CCachLine *CurrentLine = GetCacheLine(m_CurrentPosition, true);

	if (BytesToWrite <= 8 && ((m_CurrentPosition - CurrentLine->m_Overlapped.Offset) < (ECacheLineSize - 8)))
	{
		uint8* PosInCacheLine = CurrentLine->m_Data + (m_CurrentPosition - CurrentLine->m_Overlapped.Offset);
		uint8* OutBuffer = (uint8*)Buffer;
		CurrentLine->m_bDirtyFlag = true;
		
		// Spead up write of small values

		if (BytesToWrite & 1)
		{
			*PosInCacheLine = *OutBuffer;
			++PosInCacheLine;
			++OutBuffer;
		}

		if (BytesToWrite & 2)
		{
			*((int16 *)PosInCacheLine) = *((int16 *)OutBuffer);
			PosInCacheLine += 2;
			OutBuffer += 2;
		}

		if (BytesToWrite & 4)
		{
			*((int32 *)PosInCacheLine) = *((int32 *)OutBuffer);
			PosInCacheLine += 4;
			OutBuffer += 4;
		}
		
		if (BytesToWrite & 8)
		{
			*((int64 *)PosInCacheLine) = *((int64 *)OutBuffer);
		}

		m_CurrentPosition += BytesToWrite;
	}
	else
	{
		uint8* OutBuffer = (uint8*)Buffer;

		while (BytesToWrite)
		{
			CCachLine *CurrentLine = GetCacheLine(m_CurrentPosition, true);
			CurrentLine->m_bDirtyFlag = true;
			uint8* PosInCacheLine = CurrentLine->m_Data + (m_CurrentPosition - CurrentLine->m_Overlapped.Offset);
			int WriteBytes = ECacheLineSize - (m_CurrentPosition - CurrentLine->m_Overlapped.Offset);
			if (WriteBytes > BytesToWrite)
				WriteBytes = BytesToWrite;

			memcpy(PosInCacheLine, OutBuffer, WriteBytes);

			OutBuffer += WriteBytes;
			BytesToWrite -= WriteBytes;
			m_CurrentPosition += WriteBytes;
		}
	}
}

void CfstreamEmulator_CachedAsync::seekp( streamoff off, ios::seek_dir dir)
{
	if (!CheckFileOpen())
		return;

	if (dir == ios::beg)
	{
		m_CurrentPosition = off;
	}
	else if (dir == ios::cur)
	{
		m_CurrentPosition += off;
	}
	else if (dir == ios::end)
	{
		m_CurrentPosition = m_CurrentFileSize + off;
	}
	if (m_CurrentPosition < 0)
	{
		m_CurrentPosition = 0;
		m_ErrorFlags |= ios::badbit;
	}
	if (m_CurrentPosition > m_CurrentFileSize)
	{
		m_CurrentPosition = m_CurrentFileSize;
		m_ErrorFlags |= ios::badbit;
	}
}

streampos CfstreamEmulator_CachedAsync::tellp()
{
	if (!CheckFileOpen())
		return 0;

	return m_CurrentPosition;
}

bool CfstreamEmulator_CachedAsync::good()
{
	return (m_ErrorFlags & (ios::failbit | ios::badbit)) == 0;
}

bool CfstreamEmulator_CachedAsync::eof()
{
	return (m_ErrorFlags & (ios::eofbit)) != 0;
}

bool CfstreamEmulator_CachedAsync::bad()
{
	return (m_ErrorFlags & (ios::badbit)) != 0;
}

bool CfstreamEmulator_CachedAsync::is_open()
{
	return m_FileHandle != INVALID_HANDLE_VALUE;
}

void CfstreamEmulator_CachedAsync::clear( int nState)
{
	if (!nState)
		m_ErrorFlags = 0;
	else
	{
		m_ErrorFlags &= ~nState;
	}
}

#endif

