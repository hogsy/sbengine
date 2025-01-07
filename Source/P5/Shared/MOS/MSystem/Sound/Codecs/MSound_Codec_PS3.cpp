
#include "PCH.h"

#ifndef __INC_MSoundCodec_PS3_cpp__
#define __INC_MSoundCodec_PS3_cpp__

#include "mrtc.h"

#include "../../MSystem.h"

#include "../MSound_Core.h"

#if defined(PLATFORM_WIN_PC)

#ifndef M_RTM

#include "Windows.h"

class CMSound_Codec_PS3 : public CSCC_Codec
{
	MRTC_DECLARE;

public:
	CMSound_Codec_PS3()
	{		
    
	}
	
	~CMSound_Codec_PS3()
	{		
		MSCOPE(CMSound_Codec_PS3::~CMSound_Codec_PS3, GAMECUBE);
		Close();
	}

	virtual void Close()
	{
		MSCOPE(CMSound_Codec_PS3::Close, GAMECUBE);
	}

	/*************************************************************************************************\
	|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
	| Encoder
	|__________________________________________________________________________________________________
	\*************************************************************************************************/
	fp32 m_Quality;

	virtual bool CreateEncoder(float _Quality)
	{
		MSCOPE(CMSound_Codec_PS3::CreateEncoder, GAMECUBE);
		m_Quality = _Quality;

		return true;
	}

	virtual bool AddData(void *&_pData, int &_nBytes)
	{
		return false;
	}

	class CRedirector
	{
	public:
		CRedirector::CRedirector() :
			m_hStdinWrite(NULL),
			m_hStdoutRead(NULL),
			m_hChildProcess(NULL),
			m_hThread(NULL),
			m_hEvtStop(NULL),
			m_dwThreadId(0),
			m_dwWaitTime(100)
		{
		}
		CRedirector::~CRedirector()
		{
			Close();
		}

	private:
		HANDLE m_hThread;		// thread to receive the output of the child process
		HANDLE m_hEvtStop;		// event to notify the redir thread to exit
		uint32 m_dwThreadId;		// id of the redir thread
		uint32 m_dwWaitTime;		// wait time to check the status of the child process

	protected:
		HANDLE m_hStdinWrite;	// write end of child's stdin pipe
		HANDLE m_hStdoutRead;	// read end of child's stdout pipe
		HANDLE m_hChildProcess;

		BOOL LaunchChild(LPCTSTR pszCmdLine,
									  HANDLE hStdOut,
									  HANDLE hStdIn,
									  HANDLE hStdErr)
		{
			PROCESS_INFORMATION pi;
			STARTUPINFO si;

			// Set up the start up info struct.
			::ZeroMemory(&si, sizeof(STARTUPINFO));
			si.cb = sizeof(STARTUPINFO);
			si.hStdOutput = hStdOut;
			si.hStdInput = hStdIn;
			si.hStdError = hStdErr;
			si.wShowWindow = SW_HIDE;
			si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

			// Note that dwFlags must include STARTF_USESHOWWINDOW if we
			// use the wShowWindow flags. This also assumes that the
			// CreateProcess() call will use CREATE_NEW_CONSOLE.

			// Launch the child process.
			if (!::CreateProcess(
				NULL,
				(LPTSTR)pszCmdLine,
				NULL, NULL,
				TRUE,
				CREATE_NEW_CONSOLE,
				NULL, NULL,
				&si,
				&pi))
				return FALSE;

			m_hChildProcess = pi.hProcess;
			// Close any unuseful handles
			::CloseHandle(pi.hThread);
			return TRUE;
		}

		// redirect the child process's stdout:
		// return: 1: no more data, 0: child terminated, -1: os error
		int RedirectStdout()
		{
			for (;;)
			{
				DWORD dwAvail = 0;
				if (!::PeekNamedPipe(m_hStdoutRead, NULL, 0, NULL,
					&dwAvail, NULL))			// error
					break;

				if (!dwAvail)					// not data available
					return 1;

				char szOutput[256];
				DWORD dwRead = 0;
				if (!::ReadFile(m_hStdoutRead, szOutput, min(255, dwAvail),
					&dwRead, NULL) || !dwRead)	// error, the child might ended
					break;

				szOutput[dwRead] = 0;
				WriteStdOut(szOutput);
			}

			DWORD dwError = ::GetLastError();
			if (dwError == ERROR_BROKEN_PIPE ||	// pipe has been ended
				dwError == ERROR_NO_DATA)		// pipe closing in progress
			{
		#ifdef _TEST_REDIR
				WriteStdOut("\r\n<TEST INFO>: Child process ended\r\n");
		#endif
				return 0;	// child process ended
			}

			WriteStdError("Read stdout pipe error\r\n");
			return -1;		// os error
		}

		void DestroyHandle(HANDLE& rhObject)
		{
			if (rhObject != NULL)
			{
				::CloseHandle(rhObject);
				rhObject = NULL;
			}
		}

		public:
		CStr m_StdOut;

		void WriteStdOut(LPCSTR pszOutput)
		{
			m_StdOut += pszOutput;
//			M_TRACEALWAYS("%s", pszOutput);
		}

		void WriteStdError(LPCSTR pszError)
		{
			m_StdOut += pszError;
//			M_TRACEALWAYS("%s", pszError);
		}

		// thread to receive output of the child process
		static DWORD WINAPI OutputThread(LPVOID lpvThreadParam)
		{
			HANDLE aHandles[2];
			int nRet;
			CRedirector* pRedir = (CRedirector*) lpvThreadParam;

			aHandles[0] = pRedir->m_hChildProcess;
			aHandles[1] = pRedir->m_hEvtStop;

			for (;;)
			{
				// redirect stdout till there's no more data.
				nRet = pRedir->RedirectStdout();
				if (nRet <= 0)
					break;

				// check if the child process has terminated.
				DWORD dwRc = ::WaitForMultipleObjects(
					2, aHandles, FALSE, pRedir->m_dwWaitTime);
				if (WAIT_OBJECT_0 == dwRc)		// the child process ended
				{
					nRet = pRedir->RedirectStdout();
					if (nRet > 0)
						nRet = 0;
					break;
				}
				if (WAIT_OBJECT_0+1 == dwRc)	// m_hEvtStop was signalled
				{
					nRet = 1;	// cancelled
					break;
				}
			}

			// close handles
//			pRedir->Close();
			return nRet;
		}


	public:
		BOOL Open(LPCTSTR pszCmdLine)
		{
			HANDLE hStdoutReadTmp;				// parent stdout read handle
			HANDLE hStdoutWrite, hStderrWrite;	// child stdout write handle
			HANDLE hStdinWriteTmp;				// parent stdin write handle
			HANDLE hStdinRead;					// child stdin read handle
			SECURITY_ATTRIBUTES sa;

			Close();
			hStdoutReadTmp = NULL;
			hStdoutWrite = hStderrWrite = NULL;
			hStdinWriteTmp = NULL;
			hStdinRead = NULL;

			// Set up the security attributes struct.
			sa.nLength = sizeof(SECURITY_ATTRIBUTES);
			sa.lpSecurityDescriptor = NULL;
			sa.bInheritHandle = TRUE;

			BOOL bOK = FALSE;
			__try
			{
				// Create a child stdout pipe.
				if (!::CreatePipe(&hStdoutReadTmp, &hStdoutWrite, &sa, 0))
					__leave;

				// Create a duplicate of the stdout write handle for the std
				// error write handle. This is necessary in case the child
				// application closes one of its std output handles.
				if (!::DuplicateHandle(
					::GetCurrentProcess(),
					hStdoutWrite,
					::GetCurrentProcess(),
					&hStderrWrite,
					0, TRUE,
					DUPLICATE_SAME_ACCESS))
					__leave;

				// Create a child stdin pipe.
				if (!::CreatePipe(&hStdinRead, &hStdinWriteTmp, &sa, 0))
					__leave;

				// Create new stdout read handle and the stdin write handle.
				// Set the inheritance properties to FALSE. Otherwise, the child
				// inherits the these handles; resulting in non-closeable
				// handles to the pipes being created.
				if (!::DuplicateHandle(
					::GetCurrentProcess(),
					hStdoutReadTmp,
					::GetCurrentProcess(),
					&m_hStdoutRead,
					0, FALSE,			// make it uninheritable.
					DUPLICATE_SAME_ACCESS))
					__leave;

				if (!::DuplicateHandle(
					::GetCurrentProcess(),
					hStdinWriteTmp,
					::GetCurrentProcess(),
					&m_hStdinWrite,
					0, FALSE,			// make it uninheritable.
					DUPLICATE_SAME_ACCESS))
					__leave;

				// Close inheritable copies of the handles we do not want to
				// be inherited.
				DestroyHandle(hStdoutReadTmp);
				DestroyHandle(hStdinWriteTmp);

				// launch the child process
				if (!LaunchChild(pszCmdLine,
					hStdoutWrite, hStdinRead, hStderrWrite))
					__leave;

				// Child is launched. Close the parents copy of those pipe
				// handles that only the child should have open.
				// Make sure that no handles to the write end of the stdout pipe
				// are maintained in this process or else the pipe will not
				// close when the child process exits and ReadFile will hang.
				DestroyHandle(hStdoutWrite);
				DestroyHandle(hStdinRead);
				DestroyHandle(hStderrWrite);

				// Launch a thread to receive output from the child process.
				m_hEvtStop = ::CreateEvent(NULL, TRUE, FALSE, NULL);
				m_hThread = ::CreateThread(
					NULL, 0,
					OutputThread,
					this,
					0,
					&m_dwThreadId);
				if (!m_hThread)
					__leave;

				bOK = TRUE;
				WaitForSingleObject(m_hChildProcess, INFINITE);
			}
			__finally
			{
				if (!bOK)
				{
					DWORD dwOsErr = ::GetLastError();
					char szMsg[40];
					::sprintf(szMsg, "Redirect console error: %x\r\n", dwOsErr);
					WriteStdError(szMsg);
					DestroyHandle(hStdoutReadTmp);
					DestroyHandle(hStdoutWrite);
					DestroyHandle(hStderrWrite);
					DestroyHandle(hStdinWriteTmp);
					DestroyHandle(hStdinRead);
					Close();
					::SetLastError(dwOsErr);
				}
			}


			return bOK;
		}
		virtual void Close()
		{
			if (m_hThread != NULL)
			{
				// this function might be called from redir thread
				if (::GetCurrentThreadId() != m_dwThreadId)
				{
					::SetEvent(m_hEvtStop);
					//::WaitForSingleObject(m_hThread, INFINITE);
					if (::WaitForSingleObject(m_hThread, 5000) == WAIT_TIMEOUT)
					{
						WriteStdError(("The redir thread is dead\r\n"));
						::TerminateThread(m_hThread, -2);
					}
				}

				DestroyHandle(m_hThread);
			}

			DestroyHandle(m_hEvtStop);
			DestroyHandle(m_hChildProcess);
			DestroyHandle(m_hStdinWrite);
			DestroyHandle(m_hStdoutRead);
			m_dwThreadId = 0;
		}
		BOOL Printf(LPCTSTR pszFormat)
		{
			if (!m_hStdinWrite)
				return FALSE;

			uint32 dwWritten;
			return ::WriteFile(m_hStdinWrite, (LPCTSTR)pszFormat,
				(DWORD)strlen(pszFormat), &dwWritten, NULL);
		}

		void SetWaitTime(DWORD dwWaitTime) { m_dwWaitTime = dwWaitTime; }
	};


	
	virtual bool AddData(CWaveform &_WaveForm)
	{
		MSCOPE(CMSound_Codec_PS3::AddData, GAMECUBE);

		class CEncoderTemp
		{
		public:
			CEncoderTemp()
			{
			}

			~CEncoderTemp()
			{
			}

			class CStream
			{
			public:
				CStream()
				{
					m_CurrentInterleavePos = 0;
				}
				TArray<int16> m_Data;
				int m_nChannels;
				TArray<uint8> m_EncodedHeader;
				TArray<uint8> m_EncodedData;
				uint32 m_CurrentInterleavePos;
			};

			TArray<CStream> m_Streams;
		};

		CEncoderTemp EncTemp;

		int SamplRate = _WaveForm.WT_GetSampleRate();
		int Quality = 2;
		int QualityMatrix[3][3] = {48, 96, 192,   // 256, 512, 1024  
								   96, 192, 192,  // 512, 1024, 1024
								   96, 192, 384,  // 512, 1024, 2048
									};

		if (SamplRate >= 30000)
			Quality = 1;
		else if (SamplRate >= 20000)
			Quality = 2;
		else
			Quality = 2;

		// Frame Sizese
		int FarmeSizeMatrix[3][3] = {256, 512, 1024,  // 256, 512, 1024  
								     512, 1024, 1024, // 512, 1024, 1024
								     512, 1024, 2048, // 512, 1024, 2048
									};
		// Number of frames to interleave
									  
		int InterleaveMatrix[3][3] = {8, 8, 8,   // 256, 512, 1024  
								      4, 4, 4,	// 512, 1024, 1024
								      4, 4, 4,	// 512, 1024, 2048
		};


		int InterleaveSizeMatrix[3][3] = {2048, 4096, 8192,   // 256, 512, 1024  
										  2048, 4096, 4096,	// 512, 1024, 1024
										  2048, 4096, 8192,	// 512, 1024, 2048
		};

		int nChannels = m_Format.m_Data.GetChannels();
		int iChannel = 0;
		int16 *pSrc = (int16 *)_WaveForm.Lock();
		int Modulo = _WaveForm.GetModulo() / 2;
		int nSamp = _WaveForm.GetHeight();
		int nSampReal = nSamp;
		if (nSamp < 6144)
			nSamp = 6144;// Cannot compile with less

		LogFile(CStrF("Sound is %d samples", nSampReal));

		while (nChannels)
		{
			if (nChannels >= 6)
			{
				// Use 6 channel stream
				CEncoderTemp::CStream NewStream;
				NewStream.m_Data.SetLen(nSamp * 6);
				NewStream.m_nChannels = 6;
				TAP<int16> pData = NewStream.m_Data;
				for (int i = 0; i < nSampReal; ++i)
				{
					pData[i*6 + 0] = pSrc[i*Modulo + iChannel + 0];
					pData[i*6 + 1] = pSrc[i*Modulo + iChannel + 1];
					pData[i*6 + 2] = pSrc[i*Modulo + iChannel + 2];
					pData[i*6 + 3] = pSrc[i*Modulo + iChannel + 3];
					pData[i*6 + 4] = pSrc[i*Modulo + iChannel + 4];
					pData[i*6 + 5] = pSrc[i*Modulo + iChannel + 5];
				}
				for (int i = nSampReal; i < nSamp; ++i)
				{
					pData[i*6 + 0] = 0;
					pData[i*6 + 1] = 0;
					pData[i*6 + 2] = 0;
					pData[i*6 + 3] = 0;
					pData[i*6 + 4] = 0;
					pData[i*6 + 5] = 0;
				}

				EncTemp.m_Streams.Add(NewStream);
				nChannels -= 6;
			}
			else if (nChannels >= 2)
			{
				// Use 2 channel stream
				CEncoderTemp::CStream NewStream;
				NewStream.m_Data.SetLen(nSamp * 2);
				NewStream.m_nChannels = 2;
				TAP<int16> pData = NewStream.m_Data;
				for (int i = 0; i < nSampReal; ++i)
				{
					pData[i*2 + 0] = pSrc[i*Modulo + iChannel + 0];
					pData[i*2 + 1] = pSrc[i*Modulo + iChannel + 1];
				}
				for (int i = nSampReal; i < nSamp; ++i)
				{
					pData[i*2 + 0] = 0;
					pData[i*2 + 1] = 0;
				}

				EncTemp.m_Streams.Add(NewStream);
				nChannels -= 2;
			}
			else
			{
				// Use 1 channel stream
				CEncoderTemp::CStream NewStream;
				NewStream.m_Data.SetLen(nSamp * 1);
				NewStream.m_nChannels = 1;
				TAP<int16> pData = NewStream.m_Data;
				for (int i = 0; i < nSampReal; ++i)
				{
					pData[i + 0] = pSrc[i*Modulo + iChannel + 0];
				}
				for (int i = nSampReal; i < nSamp; ++i)
				{
					pData[i + 0] = 0;
				}

				EncTemp.m_Streams.Add(NewStream);
				nChannels -= 1;
			}
		}

		MACRO_GetSystem;
		TAP<CEncoderTemp::CStream> pStreams = EncTemp.m_Streams;
		for (int i = 0; i < pStreams.Len(); ++i)
		{
			uint32 nChannels = pStreams[i].m_nChannels;
			uint32 DataSize = nChannels * nSamp * 2;
			// Write Stream
			CStr Path = pSys->m_ExePath + CStrF("TempEncode-%d.wav", i);
			CCFile OutFile;
			OutFile.Open(Path, CFILE_WRITE);

			// Main chunk
			OutFile.WriteBE(uint32('RIFF')); // File type
			OutFile.WriteLE(uint32(36 + DataSize));
			
			// Format subchunk
			OutFile.WriteBE(uint32('WAVE')); // Format
			OutFile.WriteBE(uint32('fmt ')); // SubChunk: fmt
			OutFile.WriteLE(uint32(16)); // SubChunkSize
			OutFile.WriteLE(uint16(1)); // Format: PCM
			OutFile.WriteLE(uint16(nChannels)); // NChannels
			OutFile.WriteLE(uint32(48000)); // SamplaRate: Always 48000
			OutFile.WriteLE(uint32(nChannels * 48000 * 2)); // ByteRate
			OutFile.WriteLE(uint16(nChannels * 2)); // BlockAlign
			OutFile.WriteLE(uint16(16)); // BitsPerSample

			// Data Chunk
			OutFile.WriteBE(uint32('data')); // SubChunk: data
			OutFile.WriteLE(uint32(DataSize)); // SubChunkSize
			OutFile.Write(pStreams[i].m_Data.GetBasePtr(), DataSize);

//			pStreams[i]
		}

		// Encode
		pStreams = EncTemp.m_Streams;
		for (int i = 0; i < pStreams.Len(); ++i)
		{
			uint32 nChannels = pStreams[i].m_nChannels;
			uint32 iChannel = 0;
			if (nChannels == 6)
				iChannel = 2;
			else if (nChannels == 2)
				iChannel = 1;
			uint32 DataSize = nChannels * nSamp * 2;
			// Write Stream
			CStr SourcePath = pSys->m_ExePath + CStrF("TempEncode-%d.wav", i);
			CStr DestPath = pSys->m_ExePath + CStrF("TempEncode-%d.at3", i);
			CStr ExePath = CStrF("PS3at3tool.exe");
			CStr CommandLine = CStrF("%s -br %d -loop %d %d -e \"%s\" \"%s\"", ExePath.Str(), QualityMatrix[Quality][iChannel], 
				0, nSamp-1,
				SourcePath.Str(), DestPath.Str());
			
#if 0
			PROCESS_INFORMATION ProcessInfo;
			STARTUPINFO StartupInfo;
			{
				CCFile StdOut;
				StdOut.Open(pSys->m_ExePath + "TempStdOut", CFILE_WRITE);
				void *pFileHandle = *((void **)((CStream_Disk *)StdOut.GetStream())->m_Stream.m_pData->m_pFile);
				ZeroMemory(&StartupInfo, sizeof(StartupInfo));
				StartupInfo.cb = sizeof(sizeof(StartupInfo));
				StartupInfo.dwFlags = STARTF_USESTDHANDLES;
				StartupInfo.hStdOutput = pFileHandle;
				ZeroMemory(&ProcessInfo, sizeof(ProcessInfo));
				CreateProcess(NULL, CommandLine, NULL, NULL, true, NORMAL_PRIORITY_CLASS, NULL, NULL, &StartupInfo, &ProcessInfo);
				if (ProcessInfo.hThread)
					CloseHandle(ProcessInfo.hThread);
				if (ProcessInfo.hProcess)
					WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
			}
#endif

			CRedirector Redirector;
			if (Redirector.Open(CommandLine))
			{
				Redirector.Close();
				{
					const ch8 *pParse = Redirector.m_StdOut.Str();
					const ch8 *pParseStart = pParse;
					while (*pParse)
					{
						if (*pParse == '\r' || *pParse == '\n')
						{
							CStr Temp;
							Temp.Capture(pParseStart, pParse - pParseStart);
							LogFile(Temp);
							if (*pParse == '\r')
								++pParse;
							if (*pParse == '\n')
								++pParse;
							pParseStart = pParse;
							continue;
						}

						++pParse;
					}
					if (pParse - pParseStart)
					{
						CStr Temp;
						Temp.Capture(pParseStart, pParse - pParseStart);
						LogFile(Temp);
					}
				}
				// Parse out file
				CCFile OutFile;
				OutFile.Open(DestPath, CFILE_READ);

				if (OutFile.Length() == 0)
					ThrowError("Failed to encode sound");

				uint32 Temp;
				OutFile.ReadBE(Temp); // RIFF
				OutFile.ReadLE(Temp); // SIZE
				OutFile.ReadLE(Temp); // WAVE
				OutFile.ReadBE(Temp); // Chunk
				while (Temp != 'data')
				{
					OutFile.ReadLE(Temp); // 
					OutFile.RelSeek(Temp);
					OutFile.ReadBE(Temp); // Chunk
					if (OutFile.EndOfFile())
						ThrowError("Failed to encode sound");
				}

				M_ASSERT(Temp == 'data', "");
				OutFile.ReadLE(Temp); // DataSize

				uint32 DataPos = OutFile.Pos();
				uint32 FileLen = OutFile.Length();

				pStreams[i].m_EncodedHeader.SetLen(DataPos);
				pStreams[i].m_EncodedData.SetLen(FileLen - DataPos);

				OutFile.Seek(0);
				OutFile.Read(pStreams[i].m_EncodedHeader.GetBasePtr(), DataPos);
				OutFile.Read(pStreams[i].m_EncodedData.GetBasePtr(), FileLen - DataPos);
			}
//			else
//				LogFile(Redirector.m_StdOut);
//			pStreams[i]
		}

		// Write Header
		uint32 HeaderSize = 12;

		fint StartPos = m_pFile->Pos();
		uint32 nStreams = pStreams.Len();
		m_pFile->WriteLE(nStreams);
		m_pFile->WriteLE(uint32(Quality));
		pStreams = EncTemp.m_Streams;
		for (int i = 0; i < pStreams.Len(); ++i)
		{
			HeaderSize += 4 * 3;
			HeaderSize += pStreams[i].m_EncodedHeader.Len();
		}

		m_pFile->WriteLE(HeaderSize);

		for (int i = 0; i < pStreams.Len(); ++i)
		{
			uint32 SubHeaderSize = pStreams[i].m_EncodedHeader.Len();
			m_pFile->WriteLE(SubHeaderSize);
			uint32 DataSize = pStreams[i].m_EncodedData.Len();
			m_pFile->WriteLE(DataSize);

			uint32 nChannels = pStreams[i].m_nChannels;
			uint32 iChannel = 0;
			if (nChannels == 6)
				iChannel = 2;
			else if (nChannels == 2)
				iChannel = 1;
			m_pFile->WriteLE(iChannel); // Channel index

			m_pFile->Write(pStreams[i].m_EncodedHeader.GetBasePtr(), SubHeaderSize);
		}

		fint Pos = m_pFile->Pos();

		if ((Pos + 28) & (2048 - 1))
		{
			// Align

			fint EndPos = (((Pos + 28) + 2047) & ~2047) - 28;
			if (EndPos < Pos)
				EndPos += 2048;

			m_pFile->Align(EndPos);
		}

		// Interleave
		pStreams = EncTemp.m_Streams;
		int bBreak = pStreams.Len();
		while (bBreak)
		{
			for (int i = 0; i < pStreams.Len(); ++i)
			{
				uint32 nChannels = pStreams[i].m_nChannels;
				uint32 iChannel = 0;
				if (nChannels == 6)
					iChannel = 2;
				else if (nChannels == 2)
					iChannel = 1;

				uint32 ToWrite = InterleaveSizeMatrix[Quality][iChannel];

				uint32 EncodedLen = pStreams[i].m_EncodedData.Len();
				uint32 CurrentPos = pStreams[i].m_CurrentInterleavePos;

				if (CurrentPos == EncodedLen)
					M_BREAKPOINT; // Interleave error

				uint32 MaxWrite = Min(ToWrite, EncodedLen - CurrentPos);

				m_pFile->Write(pStreams[i].m_EncodedData.GetBasePtr() + CurrentPos, MaxWrite);

				CurrentPos += MaxWrite;
				pStreams[i].m_CurrentInterleavePos = CurrentPos;

				if (CurrentPos == EncodedLen)
				{
					int Left = ToWrite - MaxWrite;
					while (Left)
					{
						m_pFile->Write((uint8)0);
						--Left;
					}
					--bBreak;
				}
			}
		}

		LogFile(CStrF("Compression Ratio: %.2f", fp32(_WaveForm.GetSize()) / (fp32)(m_pFile->Pos() - StartPos) ));

		// Align the data we are writing to 2048 bytes
/*
		int Left = ((EncTemp.m_EncodedBufferSize + 2047) & ~2047) - EncTemp.m_EncodedBufferSize;
		while (Left)
		{
			m_pFile->Write((uint8)0);
			--Left;
		}
		*/

#if 0
		for (int i = 0; i < nChannels; ++i)
		{
			if (!bDone[i])
			{
				bDone[i] = true;

				int iOther = -1;
				for (int j = 0; j < nChannels; ++j)
				{
					if (_WaveForm.m_ChannelAssignments[i] == _WaveForm.m_ChannelAssignments[j] && i != j)
					{
						bDone[j] = true;
						iOther = j;
						break;
					}
				}


				int iChannel = _WaveForm.m_ChannelAssignments[i];

				int16 *pData = (int16 *)_WaveForm.Lock();
				int Modulo = _WaveForm.GetModulo() / 2;
				int nSamp = _WaveForm.GetHeight();

				int nSampReal = nSamp;

				if (EncTemp.m_Streams.Len() <= iChannel)
					EncTemp.m_Streams.SetLen(iChannel + 1);
				if (EncTemp.m_Data.Len() <= iChannel)
					EncTemp.m_Data.SetLen(iChannel + 1);
				int nStreamChannels = iOther >= 0 ? 2:1;

				PS3ENCODERSTREAM Stream;
				memset(&Stream, 0, sizeof(Stream));
				Stream.Format.Format.cbSize = sizeof(Stream.Format.Format);
				Stream.Format.Format.wFormatTag = WAVE_FORMAT_PCM;
				Stream.Format.Format.nChannels = nStreamChannels;
				Stream.Format.Format.nSamplesPerSec = m_Format.m_Data.GetSampleRate();
				Stream.Format.Format.nBlockAlign = nStreamChannels * 2;
				Stream.Format.Format.nAvgBytesPerSec = Stream.Format.Format.nBlockAlign * Stream.Format.Format.nSamplesPerSec;
				Stream.Format.Format.wBitsPerSample = 16;
				Stream.BufferSize = nSamp * Stream.Format.Format.nBlockAlign;
				Stream.SpeakerAssignment[0] = _WaveForm.m_SpeakerAssignment[i];
				if (iOther >= 0)
					Stream.SpeakerAssignment[1] = _WaveForm.m_SpeakerAssignment[iOther];
				
				if (_WaveForm.m_LoopStart >= 0)
				{
					Stream.LoopStart = _WaveForm.m_LoopStart;
					Stream.LoopLength = _WaveForm.m_LoopEnd - _WaveForm.m_LoopStart;
				}

				if (iOther >= 0)
				{
					// Interleave data
					if (EncTemp.m_Data[iChannel].Len() != nSamp*2)
						EncTemp.m_Data[iChannel].SetLen(nSamp * 2);

					int16 *pDest = EncTemp.m_Data[iChannel].GetBasePtr();

					for (int x = 0; x < nSampReal; ++x)
					{
						pDest[x * 2] = pData[x * Modulo + i];
						pDest[x * 2 + 1] = pData[x * Modulo + iOther];
					}
					for (int x = nSampReal; x < nSamp; ++x)
					{
						pDest[x * 2] = 0;
						pDest[x * 2 + 1] = 0;
					}
				}
				else
				{
					if (EncTemp.m_Data[iChannel].Len() != nSamp)

						EncTemp.m_Data[iChannel].SetLen(nSamp);
					int16 *pDest = EncTemp.m_Data[iChannel].GetBasePtr();

					for (int x = 0; x < nSampReal; ++x)
					{
						pDest[x] = pData[x * Modulo + i];
					}
					for (int x = nSampReal; x < nSamp; ++x)
						pDest[x] = 0;
				}

				Stream.pBuffer = EncTemp.m_Data[iChannel].GetBasePtr();
				EncTemp.m_Streams[iChannel] = Stream;

				_WaveForm.Unlock();

			}
		}

		MRTC_GetObjectManager()->ForgiveDebugNew(1);
		
		HRESULT Ret = PS3InMemoryEncoder(EncTemp.m_Streams.Len(), EncTemp.m_Streams.GetBasePtr(), m_Quality * 25, _WaveForm.m_LoopStart >= 0 ? PS3ENCODER_LOOP:0, &EncTemp.m_pEncodedBuffer, &EncTemp.m_EncodedBufferSize, 
			&EncTemp.m_pEncodedBufferFormat, &EncTemp.m_EncodedBufferFormatSize, &EncTemp.m_pSeekTable, &EncTemp.m_SeekTableSize);
		if (Ret != S_OK)
		{
			TraceLastError(Ret);
			Error_static(M_FUNCTION, "Error encoding data");
		}
		MRTC_GetObjectManager()->ForgiveDebugNew(-1);

		m_pFile->WriteLE(uint32(EncTemp.m_SeekTableSize));
		m_pFile->Write(EncTemp.m_pSeekTable, EncTemp.m_SeekTableSize);
		m_pFile->WriteLE(uint32(EncTemp.m_EncodedBufferFormatSize));
		m_pFile->Write(EncTemp.m_pEncodedBufferFormat, EncTemp.m_EncodedBufferFormatSize);
		m_pFile->WriteLE(uint32(EncTemp.m_EncodedBufferSize));
		fint Pos = m_pFile->Pos();

		if ((Pos + 28) & (2048 - 1))
		{
			// Align

			fint EndPos = (((Pos + 28) + 2047) & ~2047) - 28;
			if (EndPos < Pos)
				EndPos += 2048;

			m_pFile->Align(EndPos);
		}


		m_pFile->Write(EncTemp.m_pEncodedBuffer, EncTemp.m_EncodedBufferSize);

		int nSamp = _WaveForm.GetHeight();
		LogFile(CStrF("Sound is %d samples, Compression Ratio: %.2f", nSamp, fp32(_WaveForm.GetSize()) / (fp32)EncTemp.m_EncodedBufferSize ));

		// Align the data we are writing to 2048 bytes
		int Left = ((EncTemp.m_EncodedBufferSize + 2047) & ~2047) - EncTemp.m_EncodedBufferSize;
		while (Left)
		{
			m_pFile->Write((uint8)0);
			--Left;
		}
#endif

		return true;
	}

	/*************************************************************************************************\
	|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
	| Decoder
	|__________________________________________________________________________________________________
	\*************************************************************************************************/
	
	virtual bool CreateDecoder(int _Flags)
	{
		MSCOPE(CMSound_Codec_PS3::CreateDecoder, GAMECUBE);
		m_Format.m_Data.SetCanDecode(false);

		return true;
	}

	virtual bool SeekData(int _SampleOffset)
	{
		return false;
	}

	virtual bool GetData(void *&_pData, mint &_nBytes, bool _bLooping, bool &_bReadSomething)
	{
		MSCOPE(CMSound_Codec_PS3::AddData, GAMECUBE);

		//
		// Return true
		//
		return true;
	}
};

MRTC_IMPLEMENT_DYNAMIC(CMSound_Codec_PS3, CSCC_Codec);

#endif

#endif

#ifdef PLATFORM_WIN_PC
#define CCodecName CMSound_CodecXDF_PS3
#else
#define CCodecName CMSound_Codec_PS3
#endif

class CCodecName : public CSCC_Codec
{
	MRTC_DECLARE;

public:
	CCodecName()
	{		
		MSCOPE(CCodecName::CCodecName, GAMECUBE);

	}
	
	~CCodecName()
	{		
		MSCOPE(CCodecName::~CCodecName, GAMECUBE);
		Close();
	}

	virtual void Close()
	{
		MSCOPE(CCodecName::Close, GAMECUBE);

	}

	/*************************************************************************************************\
	|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
	| Encoder
	|__________________________________________________________________________________________________
	\*************************************************************************************************/

	virtual bool CreateEncoder(float _Quality)
	{
		Error_static("CCodecName::CreateEncoder","Cannot encode on this platform");
		return true;
	}

	virtual bool AddData(void *&_pData, int &_nBytes)
	{
		MSCOPE(CCodecName::AddData, GAMECUBE);

		Error_static("CCodecName::CreateEncoder","Cannot encode on this platform");

		return true;
	}

	/*************************************************************************************************\
	|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
	| Decoder
	|__________________________________________________________________________________________________
	\*************************************************************************************************/
	
	bool WantDelete()
	{
//		M_ASSERT(0, "We don't support this");
        return true;
	}


	virtual bool CreateDecoder(int _Flags)
	{
//		M_ASSERT(0, "We don't support this");
		return true;
	}

	virtual int32 GetDataSize()
	{
//		M_ASSERT(0, "We don't support this");
		return 0;
	}

	void * GetCodecInfo()
	{
//		M_ASSERT(0, "We don't support this");
		return NULL;
	}



	virtual bool SeekData(int _SampleOffset)
	{
		M_ASSERT(0, "We don't support this");
		return true;
	}

	virtual bool GetData(void *&_pData, mint &_nBytes, bool _bLooping, bool &_bReadSomething)
	{
		M_ASSERT(0, "We don't support this");
		return true;
	}
	
	virtual bool GetDataNonInterleaved(void **_pData, int &_nBytes, bool _bLooping, bool &_bReadSomething)
	{
		M_ASSERT(0, "We don't support this");
		return false;
	}
};

#ifdef PLATFORM_WIN_PC
MRTC_IMPLEMENT_DYNAMIC(CMSound_CodecXDF_PS3, CSCC_Codec);
#else
MRTC_IMPLEMENT_DYNAMIC(CMSound_Codec_PS3, CSCC_Codec);
#endif

#endif // __INC_MSoundCodec_PS3_cpp__
