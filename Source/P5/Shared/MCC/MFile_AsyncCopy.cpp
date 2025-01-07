
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

#ifdef PLATFORM_XBOX

	#include <xtl.h>

#endif

class CFileAsyncCopy_Internal : public MRTC_Thread
{
public:

	CStr m_From;
	CStr m_To;
	int m_BufferSize;
	CCFile m_FileFrom;
	CCFile m_FileTo;
	CAsyncRequest m_ReadRequest;
	CAsyncRequest m_WriteRequest;
	TThinArray<uint8> m_Buffers[2]; // double buffer to be able to read and write at the same time
	int m_iCurrentRead;
	int m_iCurrentWrite;
	int m_FileLength;
	fint m_FromPos;
	fint m_DestPos;
	MRTC_CriticalSection m_Lock;
	volatile int m_bPending;
	volatile int m_bPaused;
	volatile int m_bNew;
	volatile int m_bException;

	CFileAsyncCopy_Internal()
	{
		m_bPending = false;
		m_bPaused = 0;
		m_bNew = false;
		m_bException = false;

		Thread_Create(NULL, 65536, MRTC_THREAD_PRIO_ABOVENORMAL);
	}

	~CFileAsyncCopy_Internal()
	{
		Thread_Destroy();
	}

	int Thread_Main()
	{
		MRTC_SystemInfo::Thread_SetName("Async copy");

		M_TRY
		{
			while (!Thread_IsTerminating())
			{
				if (m_bNew)
				{
					m_bNew = false;
					g_StartTime = CMTime::GetCPU();

					m_FileFrom.OpenExt(m_From, CFILE_READ|CFILE_BINARY|CFILE_NOLOG|CFILE_NODEFERCLOSE, NO_COMPRESSION, NORMAL_COMPRESSION, -1, 1, 4096);
					m_FileTo.OpenExt(m_To, CFILE_WRITE|CFILE_BINARY|CFILE_TRUNC|CFILE_NOLOG|CFILE_DISCARDCONTENTS|CFILE_NODEFERCLOSE, NO_COMPRESSION, NORMAL_COMPRESSION, -1, 1, 4096);
					m_Buffers[0].SetLen(m_BufferSize);
					m_Buffers[1].SetLen(m_BufferSize);
					m_FromPos = 0;
					m_DestPos = 0;
					m_iCurrentRead = -1;
					m_iCurrentWrite = -1;		
					m_FileLength = m_FileFrom.Length();

					if (m_FileLength)
					{
						m_FileTo.Seek(m_FileFrom.Length() - 1);
						m_FileTo.Write((uint8)0);
						m_FileTo.AsyncFlush(true);
					}
					m_FileTo.Seek(0);
				}

				DoWork();

				if (m_bPending)
				{
					if (m_bPaused)
						MRTC_SystemInfo::OS_Sleep(50);
					else
						MRTC_SystemInfo::OS_Sleep(7);
				}
				else
					MRTC_SystemInfo::OS_Sleep(25);
			}

			if (m_bPending)
			{
				while (!m_FileTo.Write(&m_WriteRequest))
					MRTC_SystemInfo::OS_Sleep(10);
				while (!m_FileFrom.Read(&m_ReadRequest))
					MRTC_SystemInfo::OS_Sleep(10);

				m_FileFrom.Close();
				m_FileTo.Close();
			}
		}
#ifdef PLATFORM_XBOX
		M_CATCH(
		catch (CCExceptionFile)
		{

			CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
			m_FileFrom.Close();
			m_FileTo.Close();
			m_bNew = false;
			m_bPending = 0;
			m_bException = true;
		}
		)
#else
		M_CATCH(
		catch (CCExceptionFile)
		{

			m_FileFrom.Close();
			m_FileTo.Close();
			m_bNew = false;
			m_bPending = 0;
			m_bException = true;
		}
		)
#endif

		return 0;
	}

	void Pause()
	{
		M_LOCK(m_Lock);
		++m_bPaused;
	}

	void Resume()
	{
		M_LOCK(m_Lock);
		--m_bPaused;
	}

	static CMTime g_StartTime;
	bool CpyFile(const char *_pFrom, const char *_pTo, int _BufferSize)
	{
		M_ASSERT(!m_bPending, "Must not do this");

		if (!CDiskUtil::FileExists(_pFrom))
			return false;

		M_LOCK(m_Lock);

		m_From = _pFrom;
		m_To = _pTo;
		m_BufferSize = _BufferSize;
		m_bPending = true;
		m_bNew = true;

		return true;
	}

	bool Done()
	{
//		M_LOCK(m_Lock);
		if (m_bException)
		{
			FileError_static("Done", "Async file broken", 0);
		}
		return !m_bPending;
	}

	bool DoWork()
	{
		M_LOCK(m_Lock);

		if (!m_bPending)
			return true;

		if (m_bPaused)
			return false;

		M_TRY
		{
			// Flush
			if (m_DestPos == m_FileLength)
			{
				if (m_FileTo.Write(&m_WriteRequest))
				{
					if (m_FileTo.AsyncFlush(false))
					{
						while (!m_FileFrom.Read(&m_ReadRequest))
							MRTC_SystemInfo::OS_Sleep(1);
//						fp4 FileSize = m_FileLength;
						m_FileTo.Close();
						m_FileFrom.Close();
						m_bPending = false;
						CMTime Delta = CMTime::GetCPU() - g_StartTime;
//						M_TRACEALWAYS("Copied %f s at %f MiB/s\n", Delta.GetTime(), (FileSize/Delta.GetTime()) / (1024.0 * 1024.0));
						return true;
					}
				}
			}
			else
			{
				int bDoMore = 4;
				while (bDoMore)
				{
					if (m_FileTo.Write(&m_WriteRequest))
					{
						m_iCurrentWrite = -1;
					}

					if (m_iCurrentWrite == -1 && m_iCurrentRead == -1)
					{
						m_iCurrentWrite = -1;
						m_iCurrentRead = 0;
						{
							int ToRead = Min((fint)(m_FileLength - m_FromPos), (fint)m_BufferSize);
							if (ToRead == m_BufferSize)
								m_ReadRequest.SetRequest(m_Buffers[m_iCurrentRead].GetBasePtr(), m_FromPos, ToRead, true);
							else
								m_ReadRequest.SetRequest(m_Buffers[m_iCurrentRead].GetBasePtr(), m_FromPos, ToRead, false);
							m_FromPos += ToRead;
							m_FileFrom.Read(&m_ReadRequest);
						}
					}
					else if (m_FileFrom.Read(&m_ReadRequest) && (m_iCurrentWrite == -1))
					{
						m_iCurrentWrite = m_iCurrentRead;
						m_iCurrentRead = 1 - m_iCurrentWrite;

						{
							int ToRead = Min((fint)(m_FileLength - m_DestPos), (fint)(m_BufferSize));
							if (ToRead == m_BufferSize)
								m_WriteRequest.SetRequest(m_Buffers[m_iCurrentWrite].GetBasePtr(), m_DestPos, ToRead, true);
							else
								m_WriteRequest.SetRequest(m_Buffers[m_iCurrentWrite].GetBasePtr(), m_DestPos, ToRead, false);
							m_DestPos += ToRead;
							m_FileTo.Write(&m_WriteRequest);
						}

						{
							int ToRead = Min((fint)(m_FileLength - m_FromPos), (fint)m_BufferSize);
							if (ToRead == m_BufferSize)
								m_ReadRequest.SetRequest(m_Buffers[m_iCurrentRead].GetBasePtr(), m_FromPos, ToRead, true);
							else
								m_ReadRequest.SetRequest(m_Buffers[m_iCurrentRead].GetBasePtr(), m_FromPos, ToRead, false);
							m_FromPos += ToRead;
							m_FileFrom.Read(&m_ReadRequest);
						}

					}
					
					--bDoMore;
				}
			}
		}
		M_CATCH(
		catch (CCExceptionFile)
		{
			try 
			{
				m_bException = true;
				m_bNew = false;
				m_bPending = 0;
				m_FileTo.Close();
				m_FileFrom.Close();
			}
			catch (CCException)
			{
			}
		}
		)


		return false;
	}
};

CMTime CFileAsyncCopy_Internal::g_StartTime;

CFileAsyncCopy::CFileAsyncCopy()
{
	m_pInternal = DNew(CFileAsyncCopy_Internal) CFileAsyncCopy_Internal;

}

CFileAsyncCopy::~CFileAsyncCopy()
{
	delete m_pInternal;
}

void CFileAsyncCopy::Pause()
{
	return m_pInternal->Pause();
}

void CFileAsyncCopy::Resume()
{
	return m_pInternal->Resume();
}

bool CFileAsyncCopy::CpyFile(const char *_pFrom, const char *_pTo, int _BufferSize)
{
	return m_pInternal->CpyFile(_pFrom, _pTo, _BufferSize);

}

bool CFileAsyncCopy::Done()
{
	return m_pInternal->Done();
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class CFileAsyncWrite_Internal : public MRTC_Thread
{
public:


	class CQueueItem
	{
	public:
		int m_bDeleteOperation : 1;
		int m_bOwnChunks : 1;
		CFileAsyncWriteFillDirOptions *m_pFillDirOperation;
		CStr m_FileName;
		int m_ChunkSize;
		int m_FileLength;
		CFileAsyncWriteChunk *m_pChunks;
		int m_nChunks;

		CQueueItem()
		{
			m_bDeleteOperation = 0;
			m_bOwnChunks = 0;
			m_pFillDirOperation = NULL;
			m_ChunkSize = 0;
			m_FileLength = 0;
			m_pChunks = NULL;
			m_nChunks = 0;
		}
	};

	TList_Vector<CQueueItem> m_Queue;

	CFileAsyncWriteChunk *m_pChunks;
	int m_nChunks;
	int m_bOwnChunks;

	int m_ChunkSize;
	int m_iCurrentChunk;
	CCFile m_File;
	int m_FileLength;
	int m_PosCurrentChunk;
	int m_bRequestIsInMemory;
	CAsyncRequest m_WriteRequest;
	fint m_DestPos;
	MRTC_CriticalSection m_Lock;
	MRTC_Event m_WaitEvent;
	volatile int m_bPending;
	volatile int m_bNew;
	volatile int m_bException;

	CFileAsyncWrite_Internal()
	{
		m_bPending = false;
		m_bRequestIsInMemory = false;
		m_bNew = false;
		m_bException = false;
		m_iCurrentChunk = 0;
		m_PosCurrentChunk = 0;
		m_bOwnChunks = false;

		Thread_Create(NULL, 32768, MRTC_THREAD_PRIO_ABOVENORMAL);
	}

	~CFileAsyncWrite_Internal()
	{
		BlockUntilDone();
		Thread_Destroy();
	}

	int Thread_Main()
	{
		MRTC_SystemInfo::Thread_SetName("Async write");

		while (!Thread_IsTerminating())
		{
			M_TRY
			{
				if (!m_bPending && m_bNew)
				{
					CQueueItem Item;
					{
						M_LOCK(m_Lock);
						Item = m_Queue[0];
						m_Queue.Del(0);
					}

					if (Item.m_bDeleteOperation)
					{
						CDirectoryNode Dir;
						Dir.ReadDirectory(Item.m_FileName);
						CStr Path = Item.m_FileName.GetPath();
						int nf = Dir.GetFileCount();
						for(int i = 0; i < nf; i++)
						{
							CDir_FileRec *pF = Dir.GetFileRec(i);
							if(!Dir.IsDirectory(i))
							{
								CDiskUtil::DelFile(Path + pF->m_Name);
							}
						}
					}
					else if (Item.m_pFillDirOperation && Item.m_pFillDirOperation->m_State < 2)
					{
						if (Item.m_pFillDirOperation->m_State == 0)
						{
							const int32 WantedSize = Item.m_pFillDirOperation->m_FillSize;
							CStr Path;
							Path.Capture(Item.m_pFillDirOperation->m_pDirectory);
							int Size = WantedSize;
							int ClusterSize = 1;
							#ifdef PLATFORM_XBOX1
								ClusterSize = XGetDiskClusterSize(Path);
							#endif
							// get current size
							CDirectoryNode Dir;
							CStr CP;
							CP.Capture(Item.m_pFillDirOperation->m_pCalcSizeSearchString);
							Dir.ReadDirectory(CP);
							int nf = Dir.GetFileCount();
							Size -= ClusterSize; // The directory takes 1 block
							for(int i = 0; i < nf; i++)
							{
								CDir_FileRec *pF = Dir.GetFileRec(i);
								if(!Dir.IsDirectory(i))
									Size -= ((pF->m_Size+ClusterSize-1)/ClusterSize) * ClusterSize;
							}

							if (Size >= 0)
								Item.m_pFillDirOperation->m_FillSize = Size;
							else
								Item.m_pFillDirOperation->m_FillSize = 0;
						}

						if (Item.m_pFillDirOperation->m_FillSize > 0)
						{
							CQueueItem NewItem;

							int NewSize = Min(Item.m_pFillDirOperation->m_FillSize, Item.m_pFillDirOperation->m_PerFileSize);

							NewItem.m_ChunkSize = Item.m_pFillDirOperation->m_FillDataSize;
							NewItem.m_nChunks = (NewSize + NewItem.m_ChunkSize - 1) / NewItem.m_ChunkSize;
							NewItem.m_bOwnChunks = true;
							NewItem.m_pFillDirOperation = Item.m_pFillDirOperation;
							NewItem.m_FileName.Capture(CFStrF("%s%s%03d", Item.m_pFillDirOperation->m_pDirectory, Item.m_pFillDirOperation->m_pFillFileName, Item.m_pFillDirOperation->m_iCurrentFile).Str());
							NewItem.m_FileLength = NewSize;
							NewItem.m_pChunks = DNew(CFileAsyncWriteChunk) CFileAsyncWriteChunk[NewItem.m_nChunks];

							Item.m_pFillDirOperation->InitFile(NewSize);

							int Size = NewSize;
							int iChunk = 0;
							while (Size)
							{
								NewItem.m_pChunks[iChunk].m_pMemory = Item.m_pFillDirOperation->m_pFillData;
								int CurrentSize = Min(Size, NewItem.m_ChunkSize);
								NewItem.m_pChunks[iChunk].m_MemorySize = CurrentSize;
								++iChunk;
								Size -= CurrentSize;
							}

							{
								M_LOCK(m_Lock);
								++m_bNew;
								m_Queue.Insert(0, NewItem);
							}

							++Item.m_pFillDirOperation->m_iCurrentFile;
							Item.m_pFillDirOperation->m_FillSize -= NewSize;
							Item.m_pFillDirOperation->m_State = 2;
						}
                        
					}
					else
					{
						if (!CDiskUtil::DirectoryExists(Item.m_FileName.GetPath()))
							CDiskUtil::CreatePath(Item.m_FileName.GetPath());
						m_File.OpenExt(Item.m_FileName, CFILE_WRITE|CFILE_BINARY|CFILE_TRUNC|CFILE_NOLOG|CFILE_DISCARDCONTENTS|CFILE_NODEFERCLOSE, NO_COMPRESSION, NORMAL_COMPRESSION, -1, 1, 4096);
						m_FileLength = Item.m_FileLength;
						m_pChunks = Item.m_pChunks;
						m_nChunks = Item.m_nChunks;
						m_ChunkSize = Item.m_ChunkSize;
						m_bOwnChunks = Item.m_bOwnChunks;

						m_DestPos = 0;
						m_iCurrentChunk = 0;
						m_PosCurrentChunk = 0;
						m_bPending = true;

						m_File.Seek(Max(m_FileLength - 4096, 0));
						m_File.Write((uint8)0);
						m_File.Seek(Max(m_FileLength - 1, 0));
						m_File.Write((uint8)0);
						m_File.AsyncFlush(true);
						m_File.Seek(0);
						if (Item.m_pFillDirOperation)
						{
							Item.m_pFillDirOperation->m_State = 1;
							{
								CQueueItem NewItem;
								NewItem.m_pFillDirOperation = Item.m_pFillDirOperation;
								M_LOCK(m_Lock);
								++m_bNew;
								m_Queue.Insert(0, NewItem);
							}
						}
					}
					{
						M_LOCK(m_Lock);
						--m_bNew;
					}
				}

				DoWork();

				if (m_bPending || m_bNew)
					m_WaitEvent.WaitTimeout(0.010f);
				else
					m_WaitEvent.WaitTimeout(1.0f);
			}
			M_CATCH(
			catch (CCExceptionFile)
			{
				try 
				{
						{
						M_LOCK(m_Lock);
						try
						{
							m_File.Close();
						}
						catch (CCException)
						{
						}
						m_Queue.Clear();
						m_bNew = 0;
						m_bPending = 0;
//						m_bException = true;
					}

				}
				catch (CCException)
				{
				}
			}
			)
		}

		Abort();

		return 0;
	}

	void Abort()
	{
		M_LOCK(m_Lock);

		m_Queue.Clear();
		m_bNew = 0;

		if (m_bPending)
		{
			while (!m_File.Write(&m_WriteRequest))
				MRTC_SystemInfo::OS_Sleep(10);

			m_File.Close();
		}
	}


	bool SaveFile(const char *_pFile, CFileAsyncWriteChunk *_pChunks, int _nChunks, int _ChunkSize)
	{
		/*
		M_ASSERT(!m_bPending, "Must not do this");

		if (m_bPending)
		{
			M_TRACEALWAYS("Blocking on async file write until the last request is done");
			BlockUntilDone();
		}*/

		CQueueItem Item;
		Item.m_bDeleteOperation = false;
		Item.m_FileName = _pFile;
		Item.m_ChunkSize = _ChunkSize;
		Item.m_FileLength = 0;
		for (int i = 0; i < _nChunks; ++i)
		{
			if (_pChunks[i].m_MemorySize & 0x3)
				Error_static("SaveFile","Memory size must be aligned on 4 bytes");
			Item.m_FileLength += _pChunks[i].m_MemorySize;
		}
		Item.m_nChunks = _nChunks;
		Item.m_pChunks = _pChunks;

		{
			M_LOCK(m_Lock);

			m_Queue.Add(Item);
			++m_bNew;
		}
		m_WaitEvent.Signal();

		return true;

	}
	
	bool DelFile(const char *_pFile)
	{
		CQueueItem Item;
		Item.m_bDeleteOperation = true;
		Item.m_FileName = _pFile;

		{
			M_LOCK(m_Lock);

			m_Queue.Add(Item);
			++m_bNew;
		}

		m_WaitEvent.Signal();
		return true;
	}

	bool FillDirecotry(CFileAsyncWriteFillDirOptions *_pOptions)
	{
		CQueueItem Item;
		Item.m_bDeleteOperation = false;
		Item.m_pFillDirOperation = _pOptions;

		M_LOCK(m_Lock);

		{
			m_Queue.Add(Item);
			++m_bNew;
		}
		m_WaitEvent.Signal();

		return true;
	}

	bool Done()
	{
//		M_LOCK(m_Lock);
		if (m_bException)
		{
			FileError_static("Done", "Async file broken", 0);
		}
		bool bDone;
		{
			M_LOCK(m_Lock);
			bDone = !m_bPending && !m_bNew;
		}
		return bDone;
	}

	void BlockUntilDone()
	{
		while (!Done())
		{
			MRTC_SystemInfo::OS_Sleep(10);
		}
	}

	bool DoWork()
	{
		{
			M_LOCK(m_Lock);

			if (!m_bPending)
				return true;
		}


		M_TRY
		{
			// Flush
			if (m_DestPos == m_FileLength)
			{

				if (m_File.Write(&m_WriteRequest))
				{
					if (m_File.AsyncFlush(false))
					{
//						fp4 FileSize = m_FileLength;
						m_File.Close();

						if (m_bOwnChunks && m_pChunks)
						{
							delete [] m_pChunks;
							m_pChunks = NULL;
							m_bOwnChunks = false;
						}
		
						{
							M_LOCK(m_Lock);

							m_bPending = false;
						}

						return true;
					}
				}
			}
			else
			{
				CFileAsyncWriteChunk *pChunk = m_pChunks + m_iCurrentChunk;

				if (m_File.Write(&m_WriteRequest))
				{
					int Offset = m_DestPos - ((m_DestPos / m_ChunkSize) * m_ChunkSize);
					if (!Offset)
						Offset = m_ChunkSize;
					int ToRead = Min((fint)(pChunk->m_MemorySize - m_PosCurrentChunk), (fint)(Offset));

					m_bRequestIsInMemory = true;

					if (ToRead != m_ChunkSize)
						m_bRequestIsInMemory = false;

					if (m_bRequestIsInMemory)
					{
						m_WriteRequest.SetRequest((void *)((uint8 *)(pChunk->m_pMemory) + m_PosCurrentChunk), m_DestPos, ToRead, true);
					}
					else
					{
						m_File.Seek(m_DestPos);
						m_File.Write((void *)(((uint8 *)pChunk->m_pMemory) + m_PosCurrentChunk), ToRead);
						m_File.AsyncFlush(true);
					}
					m_PosCurrentChunk += ToRead;
					m_DestPos += ToRead;

					m_File.Write(&m_WriteRequest);

					if (pChunk->m_MemorySize == m_PosCurrentChunk)
					{
						++m_iCurrentChunk;
						m_PosCurrentChunk = 0;
					}
				}
			}
		}
		M_CATCH(
		catch (CCExceptionFile)
		{
			try 
			{
				try
				{
					m_File.Close();
				}
				catch (CCException)
				{
				}

				{
					M_LOCK(m_Lock);
					m_Queue.Clear();
					m_bNew = 0;
					m_bPending = 0;
//					m_bException = true;
				}

			}
			catch (CCException)
			{
			}
		}
		)


		return false;
	}
};



CFileAsyncWrite::CFileAsyncWrite()
{
	m_pInternal = DNew(CFileAsyncWrite_Internal) CFileAsyncWrite_Internal;

}

CFileAsyncWrite::~CFileAsyncWrite()
{
	delete m_pInternal;
}

bool CFileAsyncWrite::Done()
{
	return m_pInternal->Done();
}

void CFileAsyncWrite::Abort()
{
	m_pInternal->Abort();
}

bool CFileAsyncWrite::SaveFile(const char *_pFile, CFileAsyncWriteChunk *_pChunks, int _nChunks, int _ChunkSize)
{
	return m_pInternal->SaveFile(_pFile, _pChunks, _nChunks, _ChunkSize);
}

bool CFileAsyncWrite::DelFile(const char *_pFile)
{
	return m_pInternal->DelFile(_pFile);
}

bool CFileAsyncWrite::FillDirecotry(CFileAsyncWriteFillDirOptions *_pOptions)
{
	return m_pInternal->FillDirecotry(_pOptions);
}

void CFileAsyncWrite::BlockUntilDone()
{
	m_pInternal->BlockUntilDone();
}
