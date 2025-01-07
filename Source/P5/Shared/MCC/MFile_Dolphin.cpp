#include "PCH.h"

#ifdef PLATFORM_DOLPHIN

#include "MFile_Dolphin.h"
//#include "../MOS/MRndrDolphin/DisplayContext.h"
//#include "../MOS/MRndrDolphin/RenderContext.h"
//#include "../MOS/MRndrDolphin/TevManager.h"
#include "../MOS/MSystem/MSystem_Dolphin.h" // for error infoscreen

const char* g_pCurrentDisc = "";
const char* g_pGameName = "";

extern void MRTC_SystemInfo_CPU_SetClockMark();
extern void MRTC_SystemInfo_CPU_ResetClockToMark();



/************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯| 
| Slashify..
|____________________________________________________________________________________|
\************************************************************************************/
const char* Slashify(const char* _pStr)
{
	static char s_Str[256];
	int j=0;
	for (int i=0; _pStr && _pStr[i] && j<255; i++)
	{
		if (_pStr[i] == '\\')
		{
			s_Str[j++] = '/';

			// skip multiple '\'
			while (_pStr[i+1] && _pStr[i+1] == '\\')
				i++;
		}
		else
		{
			s_Str[j++] = _pStr[i];
		}
	}
	s_Str[j] = 0;
	return s_Str;
}



void CheckDVDStatus()
{
	// FIXME LATER
	const char* pDummyFile = "Environment_GC.cfg";
	void* pFile = MRTC_SystemInfo::OS_FileOpen(pDummyFile, true, false, false, false);
	if (pFile)
	{
		static uint8 buf[64];
		void* pAsync = MRTC_SystemInfo::OS_FileAsyncRead(pFile, (void*)(int(buf+31)&~31), 32, 0);
		if (pAsync)
			MRTC_SystemInfo::OS_FileAsyncClose(pAsync);
		MRTC_SystemInfo::OS_FileClose(pFile);
	}
}


/************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯| 
| MRTC_DVDFileInfo
|____________________________________________________________________________________|
\************************************************************************************/
bool MRTC_DVDFileInfo::Open(bool _bRead, bool _bWrite, bool _bCreate, bool _bTruncate)
{
	if (_bWrite | _bCreate | _bTruncate)
		return false;

	if (DVDFastOpen(m_EntryNum, &m_FileInfo))
		return true;

	return false;
}


void MRTC_DVDFileInfo::Close()
{
	DVDClose(&m_FileInfo);
}


fint MRTC_DVDFileInfo::GetFileSize()
{
	return DVDGetLength(&m_FileInfo);
}


void* MRTC_DVDFileInfo::AsyncRead(void *_pData, fint _DataSize, fint _FileOffset)
{
	M_ASSERT(((int)_pData & 31) == 0, "OS_FileAsyncRead: _pData must be 32-byte aligned");
	M_ASSERT(_DataSize < 0xffffffff, "OS_FileAsyncRead: _DataSize too big");
	M_ASSERT((_DataSize & 31) == 0, "OS_FileAsyncRead: _DataSize should be multiple of 32");
	M_ASSERT((_FileOffset & 3) == 0, "OS_FileAsyncRead: _FileOffset should be multiple of 4");

	if (m_IsBusy)
	{
		MRTC_DVDFileInfo* pCopy = Duplicate();
		if (pCopy)
			return pCopy->AsyncRead(_pData, _DataSize, _FileOffset);
		else
			return NULL;
	}

	m_bNeedRestart = false;
	m_pData = _pData;
	m_DataSize = _DataSize;
	m_FileOffset = _FileOffset;

	if (!DVDReadAsync(&m_FileInfo, _pData, _DataSize, _FileOffset, NULL))
	{
		M_ASSERT(false, "OS_FileAsyncRead: How did this happen!?");
		return NULL;
	}

	m_IsBusy = true;
	return this;
}


void* MRTC_DVDFileInfo::AsyncWrite(const void *_pData, fint _DataSize, fint _FileOffset)
{
	Error_static("MRTC_DVDFileInfo::AsyncWrite", "Cannot write to DVD!");
}


fint MRTC_DVDFileInfo::AsyncBytesProcessed()
{
	Status x = CheckStatus();
	if (x == e_Error)
		return 0;

	return DVDGetTransferredSize(&m_FileInfo);
}


bool MRTC_DVDFileInfo::AsyncIsFinished()
{
	Status x = CheckStatus();
	return (x == e_Done);
}


void MRTC_DVDFileInfo::AsyncClose()
{
	for (;;)
	{
		Status x = CheckStatus();
		if (x == e_Done || x == e_Error)
			break;

		MRTC_SystemInfo::OS_Sleep(1);
	}

	if (m_Duplicated)
		delete this;
	else
		m_IsBusy = false;
}


MRTC_DVDFileInfo* MRTC_DVDFileInfo::Duplicate() const
{
	M_ASSERT(m_EntryNum != -1, "MRTC_DolphinFileInfo::Duplicate, invalid file");

	MRTC_DVDFileInfo* pCopy = DNew(MRTC_DVDFileInfo) MRTC_DVDFileInfo(m_EntryNum);
	pCopy->m_Duplicated = true;
	if (DVDFastOpen(m_EntryNum, &pCopy->m_FileInfo))
		return pCopy;

	delete pCopy;
	return NULL;
}


MRTC_DVDFileInfo::Status MRTC_DVDFileInfo::CheckStatus()
{
	if (m_bNeedRestart)
	{
		DVDReadAsync(&m_FileInfo, m_pData, m_DataSize, m_FileOffset, NULL);
		m_bNeedRestart = false;
	}

	s32 Status = DVDGetFileInfoStatus(&m_FileInfo);
	switch (Status)
	{
	case DVD_STATE_NO_DISK:
	case DVD_STATE_COVER_OPEN:
	case DVD_STATE_WRONG_DISK:
	case DVD_STATE_RETRY:
	case DVD_STATE_FATAL_ERROR:
		break; // error handling below

	case DVD_STATE_END:
	case DVD_STATE_CANCELED:
		return e_Done;

	default:
		return e_Busy;
	}

	MACRO_GetRegisterObject(CSystemDolphin, pSystem, "SYSTEM");
	MACRO_GetRegisterObject(CSoundContext, pSound, "SYSTEM.SOUND");

	if (!MRTC_GOM()->InMainThread())
	{
		DVDCancel(&m_FileInfo.cb);
		m_bNeedRestart = true;

		if (pSystem)
			pSystem->DVDErrorDetectedInOtherThread(true);

		return e_Error;
	}

	// There was an error!
	fp64 t0 = GetCPUTime();
	OSReport("File error, start\n");
	if (pSystem)
		pSystem->System_BroadcastMessage( CSS_Msg(CSS_MSG_GLOBAL_PAUSE, 0,0,0,0, t0) );

	MRTC_SystemInfo_CPU_SetClockMark(); // so that we can restore system time after we let execution continue..
	CInfoScreen_GC& InfoScreen = CInfoScreen_GC::Get();
	InfoScreen.Activate();
	for (;;)
	{
		Status = DVDGetFileInfoStatus(&m_FileInfo);
		char ErrorMsg[200];
		ErrorMsg[0] = 0;

		switch (Status)
		{
		case DVD_STATE_NO_DISK:
			snprintf(ErrorMsg, 200, "Please insert the %s Game Disc%s.", g_pGameName, g_pCurrentDisc);
			break;
		case DVD_STATE_COVER_OPEN:
			snprintf(ErrorMsg, 200, "The Disc Cover is open.\n"
			                        "If you want to continue the game,\n"
			                        "please close the Disc Cover.");
			break;
		case DVD_STATE_WRONG_DISK:
			snprintf(ErrorMsg, 200, "This is not the %s Game Disc%s.\n"
			                        "Please insert the %s Game Disc%s.", g_pGameName, g_pCurrentDisc, g_pGameName, g_pCurrentDisc);
			break;
		case DVD_STATE_RETRY:
			snprintf(ErrorMsg, 200, "The Game Disc could not be read.\n"
			                        "Please read the Nintendo GameCube\n"
			                        "Instruction Booklet for more information.");
			break;
		case DVD_STATE_FATAL_ERROR:
			snprintf(ErrorMsg, 200, "An error has occurred.\n"
			                        "Turn the power off and refer to the\n"
			                        "Nintendo GameCube Instruction Booklet\n"
			                        "for further instructions.");
			break;
		case DVD_STATE_BUSY:
			snprintf(ErrorMsg, 200, "Please wait...");
			break;
		case DVD_STATE_WAITING:
			snprintf(ErrorMsg, 200, "Waiting..."); // shouldn't happen, but..
			break;
		}
		ErrorMsg[sizeof(ErrorMsg)-1] = 0;

		if (ErrorMsg[0] == 0)
			break;

		InfoScreen.OutputText(ErrorMsg);

		if (pSound)
			pSound->Refresh();

		if (pSystem && (Status != DVD_STATE_FATAL_ERROR)) // On "fatal", the reset button should be disabled..
			pSystem->Refresh();
	}
	InfoScreen.Deactivate();
	MRTC_SystemInfo_CPU_ResetClockToMark();

	OSReport("File error, done\n");
	fp64 t1 = GetCPUTime();
	if (pSystem)
		pSystem->System_BroadcastMessage( CSS_Msg(CSS_MSG_GLOBAL_RESUME, 0,0,0,0, t0, t1) );

	return (Status == DVD_STATE_END) ? e_Done : e_Busy;
}



/************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯| 
| MRTC_HostFileInfo
|____________________________________________________________________________________|
\************************************************************************************/
#ifdef MRTC_ENABLE_HOSTIO

bool MRTC_HostFileInfo::Open(bool _bRead, bool _bWrite, bool _bCreate, bool _bTruncate)
{
	FILE* f = fopen(m_Filename, "r");
	if (f)
		fclose(f);
	else if (!_bCreate)
		return false;

	const char* pMode = NULL;

	switch ((int)_bRead + ((int)_bWrite)*2 + ((int)_bTruncate)*4)
	{
	case 1: pMode = "rb"; break; // read
	case 2: pMode = "ab"; break; // write, append
	case 3: pMode = "r+b"; break;// read+write, append
	case 6: pMode = "wb"; break; // write, truncate
	case 7: pMode = "w+b"; break;// read+write, truncate
	default:
		M_ASSERT(0, "MRTC_HostFileInfo::Open(), unsupported openmode!");
	}

	if (!pMode)
		return false;

	m_pFile = fopen(m_Filename, pMode); // +5 <-> "HOST:"
	return (m_pFile != NULL);
}


void MRTC_HostFileInfo::Close()
{
	M_ASSERT(m_pFile, "Cannot close non-existing file!");
	fclose(m_pFile);
	m_pFile = NULL;
}


fint MRTC_HostFileInfo::GetFileSize()
{
	M_ASSERT(m_pFile, "Cannot get filesize of non-existing file!");
	fseek(m_pFile, 0, SEEK_END);
	return ftell(m_pFile);
}


void* MRTC_HostFileInfo::AsyncRead(void *_pData, fint _DataSize, fint _FileOffset)
{
	fseek(m_pFile, _FileOffset, SEEK_SET);
	size_t nBytesRead = fread(_pData, 1, _DataSize, m_pFile);
	if (nBytesRead != _DataSize)
		return NULL;
	return this;
}


void* MRTC_HostFileInfo::AsyncWrite(const void *_pData, fint _DataSize, fint _FileOffset)
{
	fseek(m_pFile, _FileOffset, SEEK_SET);
	size_t nBytesWritten = fwrite(_pData, 1, _DataSize, m_pFile);
	if (nBytesWritten != _DataSize)
		return NULL;
	return this;
}

fint MRTC_HostFileInfo::AsyncBytesProcessed()
{	// HACK, but works with the bytestreammanager
	return 0xffffffff;
}

bool MRTC_HostFileInfo::AsyncIsFinished()
{
	return true;
}

void MRTC_HostFileInfo::AsyncClose()
{
}

#endif



/*
	GetEntryNum().

*/
static inline int32 GetEntryNum(const char *_pPath)
{
	const char* pPath = Slashify(_pPath);
	int32 sEntryNum = DVDConvertPathToEntrynum(pPath);
	return sEntryNum;
}

/*
	IsHostFile().

*/
static inline bool IsHostFile(const char *_pPath)
{
	return (strncmp(_pPath, "HOST:", 5) == 0);
}

/*
	OS_FileExists().

*/
bool MRTC_SystemInfo::OS_FileExists(const char *_pPath)
{
#ifdef MRTC_ENABLE_HOSTIO
	if (IsHostFile(_pPath))
	{
		_pPath += 5;
		while (*_pPath && *_pPath == '\\') _pPath++;
		FILE* f = fopen(_pPath, "r");
		if (!f)
			return false;
		fclose(f);
		return true;
	}
	else
#endif	
	{
		return (GetEntryNum(_pPath) != -1);
	}
}

/*
	OS_FileOpen().

*/
void *MRTC_SystemInfo::OS_FileOpen(const char *_pFileName, bool _bRead, bool _bWrite, bool _bCreate, bool _bTruncate)
{

	MRTC_FileInfo *pFile = NULL;

#ifdef MRTC_ENABLE_HOSTIO
	if (IsHostFile(_pFileName))
	{	//HOST
		pFile = DNew(MRTC_HostFileInfo) MRTC_HostFileInfo(_pFileName+5);
	}
	else
#endif	
	{	// DVD
		M_ASSERT((_bWrite || _bCreate || _bTruncate) == false, "Cannot write to DVD!");

		int32 nEntryNum = GetEntryNum(_pFileName);
		if (nEntryNum == -1)
			return NULL;

		pFile = DNew(MRTC_DVDFileInfo) MRTC_DVDFileInfo(nEntryNum);
	}

	if (pFile->Open(_bRead, _bWrite, _bCreate, _bTruncate))
	{
		return pFile;
	}

	delete pFile;
	return NULL;
}

/*
	OS_FileClose().

*/
void MRTC_SystemInfo::OS_FileClose(void *_pFile)
{
	MRTC_FileInfo* pFile = (MRTC_FileInfo*)_pFile;
	pFile->Close();
	delete pFile;
}

/*
	OS_FileSize().

*/
fint MRTC_SystemInfo::OS_FileSize(void *_pFile)
{
	return ((MRTC_FileInfo*)_pFile)->GetFileSize();
}

/*
	OS_FileAsyncRead().

*/
void *MRTC_SystemInfo::OS_FileAsyncRead(void *_pFile, void *_pData, fint _DataSize, fint _FileOffset)
{
	return ((MRTC_FileInfo*)_pFile)->AsyncRead(_pData, _DataSize, _FileOffset);
}

/*
	OS_FileAsyncWrite().

*/
void *MRTC_SystemInfo::OS_FileAsyncWrite(void *_pFile, const void *_pData, fint _DataSize, fint _FileOffset)
{
	return ((MRTC_FileInfo*)_pFile)->AsyncWrite(_pData, _DataSize, _FileOffset);
}

/*
	OS_FileAsyncBytesProcessed().

*/
fint MRTC_SystemInfo::OS_FileAsyncBytesProcessed(void *_pAsyncInstance)
{
	return ((MRTC_FileInfo*)_pAsyncInstance)->AsyncBytesProcessed();
}

/*
	OS_FileAsyncClose().

*/
void MRTC_SystemInfo::OS_FileAsyncClose(void *_pAsyncInstance)
{
	((MRTC_FileInfo*)_pAsyncInstance)->AsyncClose();
}

/*
	OS_FileAsyncIsFinished().

*/
bool MRTC_SystemInfo::OS_FileAsyncIsFinished(void *_pAsyncInstance)
{
	return ((MRTC_FileInfo*)_pAsyncInstance)->AsyncIsFinished();
}

/*
	OS_FileSetFileSize().

*/
bool MRTC_SystemInfo::OS_FileSetFileSize(const char *_pFilenName, fint _FileSize)
{
	Error_static("MRTC_SystemInfo::OS_FileSetFileSize", "Not supported!");
}

/*
	OS_FileOperationGranularity().

*/
int MRTC_SystemInfo::OS_FileOperationGranularity(const char *_pPath)
{
	return 32;
}

/*
	OS_FileGetTime().

*/
bool MRTC_SystemInfo::OS_FileGetTime(void *_pFile, int64& _TimeCreate, int64& _TimeAccess, int64& _TimeWrite)
{
	Error_static("MRTC_SystemInfo::OS_FileGetTime", "Not supported!");
}

/*
	OS_FileSetTime().

*/
bool MRTC_SystemInfo::OS_FileSetTime(void *_pFile, const int64& _TimeCreate, const int64& _TimeAccess, const int64& _TimeWrite)
{
	Error_static("MRTC_SystemInfo::OS_FileSetTime", "Not supported!");
}

/*
	OS_FileGetDrive().

*/
void MRTC_SystemInfo::OS_FileGetDrive(const char *_pFilenName, char *_pDriveName)
{
	if( IsHostFile(_pFilenName) )
		strcpy( _pDriveName, "HOST:" );
	else
		strcpy( _pDriveName, "DVD:" );
}


#endif
