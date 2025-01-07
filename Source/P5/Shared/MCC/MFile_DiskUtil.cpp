
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

#include "MFile_RAM.h"
#include "MFile_Misc.h"
#include "MFile_Stream_Disk.h"
#include "MFile_Stream_Memory.h"
#include "MFile_Stream_Megafile.h"
#include "MFile_Stream_Compressed.h"
#include "MFile_Stream_RAM.h"

#ifdef PLATFORM_WIN
# include <io.h>
# include <direct.h>
#endif

#if defined PLATFORM_XBOX

	#include <xtl.h>

#endif

#ifdef PLATFORM_XENON
CFStr ReplaceSlashes(const char* _pFileName);
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDiskUtil
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CDiskUtil::XDF_Record(const char* _pName, const char* _pBasePath)
{
	CByteStream::XDF_Record(_pName, _pBasePath);
}

void CDiskUtil::XDF_Use(const char* _pName, const char* _pBasePath)
{
	CByteStream::XDF_Use(_pName, _pBasePath);
}

void CDiskUtil::XDF_Stop()
{
	CByteStream::XDF_Stop();
}

CXDF *CDiskUtil::XDF_Get()
{
	return CByteStream::XDF_GetUse();
}

void CDiskUtil::XDF_Pause()
{
	CByteStream::XDF_Pause();
}

bint CDiskUtil::XDF_IsPaused()
{
	return CByteStream::XDF_IsPasued();
}

void CDiskUtil::XDF_Resume()
{
	CByteStream::XDF_Resume();
}

CStr CDiskUtil::XDF_GetCurrent()
{
	return CByteStream::XDF_GetCurrent();
}

int CDiskUtil::XDF_GetPosition()
{
	return CByteStream::XDF_GetPosition();
}


CStr CDiskUtil::GetDir()
{
	char Buffer[512];
	return MRTC_SystemInfo::OS_DirectoryGetCurrent(Buffer, 512);
}

CStr CDiskUtil::GetFullPath(CStr _Path, const char *_pRelative)
{
	if(_Path.GetDevice() != "")
		return _Path;
	else
	{
		if(!_pRelative)
		{
			CStr CurDir = GetDir();
			return CurDir + "\\" + _Path;
		}
		else
		{
			CStr Res = _pRelative + _Path;
			while(true)
			{
				int iDot = Res.Find("\\..\\");
				if(iDot == -1)
					break;
				int i;
				for(i = iDot - 1; i >= 0; i--)
				{
					if(Res[i] == '\\')
					{
						Res = Res.Copy(0, i) + Res.Copy(iDot + 3, 1024);
						break;
					}
				}
				if(i == -1)
					break;
			}
			return Res;
		}
	}
}

CStr CDiskUtil::GetRelativePath(CStr _Source, const char *_pRelative)
{
	CStr S = CDiskUtil::GetFullPath(_Source, _pRelative);
	CStr R = CDiskUtil::GetFullPath(_pRelative);
	if(S.GetDevice().CompareNoCase(R.GetDevice()) != 0)
		return S;

	CStr St;
	while(R != "" || S != "")
	{
		CStr R2 = R.GetStrSep("\\");
		CStr S2 = S.GetStrSep("\\");
		if(St == "" && R2 == S2)
			continue;
		if(R2 != "")
		{
			if(St != "")
				St = "..\\" + St;
			else
				St = "..";
		}
		if(S2 != "")
		{
			if(St != "")
				St += "\\";
			St += S2;
		}
	}

	return St;
}

bool CDiskUtil::ChangeDir(CStr _Dir)
{
	_Dir = _Dir.Ansi();

	return MRTC_SystemInfo::OS_DirectoryChange(_Dir);
}

bool CDiskUtil::MakeDir(CStr _At, CStr _Name)
{
	_At = _At.Ansi();
	_Name = _Name.Ansi();

	return MRTC_SystemInfo::OS_DirectoryCreate(_At + MRTC_SystemInfo::OS_DirectorySeparator() + _Name);
}

bool CDiskUtil::RemoveDir(CStr _At, CStr _Name)
{
	_At = _At.Ansi();
	_Name = _Name.Ansi();

	return MRTC_SystemInfo::OS_DirectoryRemove(_At + MRTC_SystemInfo::OS_DirectorySeparator() + _Name);
}

bool CDiskUtil::RemoveDir(CStr _Dir)
{
	_Dir = _Dir.Ansi();

	return MRTC_SystemInfo::OS_DirectoryRemove(_Dir);
}

bool CDiskUtil::DelTree(CStr _Path)
{
	_Path = _Path.Ansi();

#ifdef PLATFORM_XENON
	_Path = ReplaceSlashes(_Path);
#endif

#ifdef PLATFORM_DREAMCAST
	WARNING_UNIMP("CDiskUtil::DelTree(CStr _Path)\n");
	return false;

#elif defined PLATFORM_DOLPHIN
	Error_static("CDiskUtil::DelTree", "Not supported.");

#elif defined PLATFORM_PS2
	Error_static("CDiskUtil::DelTree", "Not supported.");

#else
	CDirectoryNode m_Dir;
	M_TRY
	{ 
		m_Dir.ReadDirectory(_Path + "\\*"); 
	}
	M_CATCH(
	catch(CCException)
	{
		return false;
	}
	)

	int nFiles = m_Dir.GetFileCount();
	for(int i = 0; i < nFiles; i++)
		if (m_Dir.IsDirectory(i))
		{
			CStr Name = m_Dir.GetFileName(i);
			if (Name == ".") continue;
			if (Name == "..") continue;
			if (!DelTree(_Path + "\\" + Name)) return false;
		}
		else
			if (!DelFile(_Path + "\\" + m_Dir.GetFileName(i))) return false;

	return RemoveDir(_Path);
#endif
}

static bool gfs_DelTreeOnlyEmpty_r(CStr _Path)
{
	bool bRet = true;
	CDirectoryNode m_Dir;
	M_TRY 
	{ 
		m_Dir.ReadDirectory(_Path + "\\*"); 
	}
	M_CATCH(
	catch(CCException)
	{
		return false;
	}
	)

	int nFiles = m_Dir.GetFileCount();
	for(int i = 0; i < nFiles; i++)
	{
		if (m_Dir.IsDirectory(i))
		{
			CStr Name = m_Dir.GetFileName(i);
			if (Name == ".") continue;
			if (Name == "..") continue;
			if (!gfs_DelTreeOnlyEmpty_r(_Path + "\\" + Name))
				bRet = false;
		}
	}

	M_TRY
	{ 
		m_Dir.ReadDirectory(_Path + "\\*"); 
	}
	M_CATCH(
	catch(CCException)
	{
		return false;
	}
	)

	bool bFoundFiles = 0;
	nFiles = m_Dir.GetFileCount();
	for(int i = 0; i < nFiles; i++)
	{
		if (m_Dir.IsDirectory(i))
		{
			CStr Name = m_Dir.GetFileName(i);
			if (Name == ".") continue;
			if (Name == "..") continue;			
		}
		bFoundFiles = true;
		break;
	}

	if (!bFoundFiles)
		return CDiskUtil::RemoveDir(_Path);

	return bRet;
}

bool CDiskUtil::DelTreeOnlyEmpty(CStr _Dir)
{
	return gfs_DelTreeOnlyEmpty_r(_Dir);
}


bool CDiskUtil::CreatePath(CStr _Name)
{
	_Name = _Name.Ansi();

#ifdef PLATFORM_DREAMCAST
	WARNING_UNIMP("CDiskUtil::CreatePath(CStr _Name)\n");
	return false;

#elif defined PLATFORM_DOLPHIN
	Error_static("CDiskUtil::CreatePath", "Not supported.");

#elif defined PLATFORM_PS2
	Error_static("CDiskUtil::CreatePath", "Not supported.");
#elif defined(PLATFORM_XBOX) || defined(PLATFORM_PS3)
	M_TRY
	{
		CStr Path;
#ifdef PLATFORM_WIN
		// Device does not exist on same platforms :)
		Path = _Name.GetDevice();
		if(Path == "")
			return false;
#endif

		_Name = _Name.Right(_Name.Len() - Path.Len());
		Path = Path.Left(Path.Len() - 1);

		while(_Name != "")
		{
			CStr Dir = _Name.GetStrSep("\\");
			CStr NewPath = Path + "\\" + Dir;
			if(!DirectoryExists(NewPath))
				MakeDir(Path, Dir);
			Path = NewPath;
		}
	}
	M_CATCH(
	catch(CCException)
	{
		return false;
	}
	)
	
	return true;
#else
	CStr OrgName = _Name;
	CStr OrgDir = GetDir();
	try
	{
//LogFile("Name1: " + _Name);

		CStr Path;
		if (_Name.GetDevice() != "")
		{
			Path = _Name.GetDevice();
			Path = Path.Left(Path.Len()-1);
			_Name = _Name.CopyFrom(Path.Len() +1);

/*			int Pos = _Name.Find(":\\");
			Path = _Name.Copy(0, Pos+1);	// No slash
			_Name = _Name.Del(0, Pos+2);*/
		}
		else
		{
			Path = ".";
		}

//LogFile("Path2: " + Path); LogFile("Name2: " + _Name);

		while(_Name.Len())
		{
//LogFile("Path3: " + Path); LogFile("Name3: " + _Name);
			CStr Dir = _Name.GetStrSep("\\");
			if (ChangeDir(Path + "\\" + Dir))
			{
			}
			else if (!MakeDir(Path, Dir))
			{
//LogFile("Path: " + Path); LogFile("Dir: " + Dir);
				Error_static("CreatePath", "Unable to create path: " + _Name);
			}
			Path += "\\" + Dir;
		}
	}
	catch(CCException)
	{
		return false;
	}

	if (!ChangeDir(OrgDir))
		Error_static("CreatePath", "Unable to switch back to original path.");
		
	return true;
#endif
}

bool CDiskUtil::DirectoryExists(CStr _Name)
{
	_Name = _Name.Ansi();

//	LogFile("(CDiskUtil::FileExists) " + _Name);
	spCStream spStream;

	if (_Name.CompareSubStr("RAM:") == 0)
	{
		_Name = _Name.Del(0, 4);
		spStream = MNew(CStream_RAM);
		if (spStream == NULL) Error_static("CDiskUtil::FileExists", "Out of memory.");
		if (spStream->DirectoryExists(_Name, CFILE_READ))
		{ 
	//		LogFile("(CDiskUtil::FileExists) Yes, RAM stream.");
			return true;
		};
		spStream = NULL;
		return false;
	}

	spStream = MNew(CStream_XDF);
	if (spStream == NULL) Error_static("CDiskUtil::FileExists", "Out of memory.");
	if (spStream->DirectoryExists(_Name, CFILE_READ))
	{ 
//		LogFile("(CDiskUtil::FileExists) Yes, Disk stream.");
		return true;
	};
	spStream = NULL;

	// MEGA
/*	spStream = new CStream_Megafile;
	if (spStream == NULL) Error_static("CDiskUtil::FileExists", "Out of memory.");
	if (spStream->FileExists(_Name, CFILE_READ))
	{ 
//		LogFile("(CDiskUtil::FileExists) Yes, Megafile stream.");
		return true;
	};
	spStream = NULL;
	*/

	spStream = MNew(CStream_Disk);
	if (spStream == NULL) Error_static("CDiskUtil::FileExists", "Out of memory.");
	if (spStream->DirectoryExists(_Name, CFILE_READ))
	{ 
//		LogFile("(CDiskUtil::FileExists) Yes, Disk stream.");
		return true;
	};
	spStream = NULL;

//LogFile("(CDiskUtil::FileExists) No.");
	return false;

	/*
	try
	{
		CCFile File;
		File.Open(_Name, CFILE_READ);
		File.Close();
	}
	catch(CCException)
	{
		return false;
	}
	return true;*/
}


bool CDiskUtil::FileExists(CStr _Name)
{
	_Name = _Name.Ansi();

//	LogFile("(CDiskUtil::FileExists) " + _Name);
	spCStream spStream;

	if (_Name.CompareSubStr("RAM:") == 0)
	{
		_Name = _Name.Del(0, 4);
		spStream = MNew(CStream_RAM);
		if (spStream == NULL) Error_static("CDiskUtil::FileExists", "Out of memory.");
		if (spStream->FileExists(_Name, CFILE_READ))
		{ 
	//		LogFile("(CDiskUtil::FileExists) Yes, RAM stream.");
			return true;
		};
		spStream = NULL;
		return false;
	}

	spStream = MNew(CStream_XDF);
	if (spStream == NULL) Error_static("CDiskUtil::FileExists", "Out of memory.");
	if (spStream->FileExists(_Name, CFILE_READ))
	{ 
//		LogFile("(CDiskUtil::FileExists) Yes, Disk stream.");
		return true;
	};
	spStream = NULL;

	// MEGA
/*	spStream = DNew(CStream_Megafile) CStream_Megafile;
	if (spStream == NULL) Error_static("CDiskUtil::FileExists", "Out of memory.");
	if (spStream->FileExists(_Name, CFILE_READ))
	{ 
//		LogFile("(CDiskUtil::FileExists) Yes, Megafile stream.");
		return true;
	};
	spStream = NULL;
	*/

	spStream = MNew(CStream_Disk);
	if (spStream == NULL) Error_static("CDiskUtil::FileExists", "Out of memory.");
	if (spStream->FileExists(_Name, CFILE_READ))
	{ 
//		LogFile("(CDiskUtil::FileExists) Yes, Disk stream.");
		return true;
	};
	spStream = NULL;

//LogFile("(CDiskUtil::FileExists) No.");
	return false;

	/*
	try
	{
		CCFile File;
		File.Open(_Name, CFILE_READ);
		File.Close();
	}
	catch(CCException)
	{
		return false;
	}
	return true;*/
}
bool CDiskUtil::DelFile(CStr _Path)
{
	_Path = _Path.Ansi();

	if (_Path.CompareSubStr("RAM:") == 0)
	{
		_Path = _Path.Del(0, 4);
		int hFile = g_RAMDiskHandler.OpenFile(_Path);
		if (hFile)
		{
			g_RAMDiskHandler.RemoveFile(hFile);
			return true;
		}
		else
			return false;
	}

	return MRTC_SystemInfo::OS_FileRemove(_Path);
}


bool CDiskUtil::RenameFile(CStr _Path, CStr _PathTo)
{
	_Path = _Path.Ansi();
	_PathTo = _PathTo.Ansi();

	return MRTC_SystemInfo::OS_FileRename(_Path, _PathTo);
}


CStr CDiskUtil::GetDrive()
{
#ifdef PLATFORM_DREAMCAST
	WARNING_UNIMP("CDiskUtil::GetDrive() - Currently only returning 'GDROM'\n");
	return CStr("GDROM");
	
#elif defined PLATFORM_DOLPHIN
	Error_static("CDiskUtil::GetDrive", "Not supported.");
	return "";

#elif defined PLATFORM_XBOX
	Error_static("CDiskUtil::GetDrive", "Not supported.");
	return "";

#elif defined PLATFORM_PS2
	Error_static("CDiskUtil::GetDrive", "Not supported.");
	return "";

#elif defined PLATFORM_PS3
	Error_static("CDiskUtil::GetDrive", "Not supported.");
	return "";

#else
	return char(_getdrive()-1 + 'A');

#endif
}

bool CDiskUtil::ChangeDrive(CStr _Drive)
{
	_Drive = _Drive.Ansi();

#ifdef PLATFORM_DREAMCAST
	WARNING_UNIMP("CDiskUtil::ChangeDrive(CStr _Drive)\n");
	return false;
	
#elif defined PLATFORM_DOLPHIN
	Error_static("CDiskUtil::ChangeDrive", "Not supported.");
	return false;

#elif defined PLATFORM_XBOX
	Error_static("CDiskUtil::ChangeDrive", "Not supported.");
	return false;

#elif defined PLATFORM_PS2
	Error_static("CDiskUtil::ChangeDrive", "Not supported.");
	return false;
#elif defined PLATFORM_PS3
	Error_static("CDiskUtil::ChangeDrive", "Not supported.");
	return false;

#else
	if (_Drive.Len() != 1) return false;
	int iDrive = _Drive.UpperCase()[0] - 'A' + 1;
	return (_chdrive(iDrive) == 0);

#endif
}

lCStr CDiskUtil::GetDrives()
{
#ifdef PLATFORM_DREAMCAST
	WARNING_UNIMP("CDiskUtil::GetDrives() - Currently only returning 'GDROM'\n");
	lCStr lDrives;
	lDrives.Add("GDROM");
	return lDrives;
#else
	CStr CurDrive = GetDrive();
	CStr CurDir = GetDir();

	lCStr lDrives;
	lDrives.Add("A");
	lDrives.Add("B");
	for(char i = 'C'; i <= 'Z'; i++)
		if (ChangeDrive(i)) lDrives.Add(i);

	if (!ChangeDrive(CurDrive)) Error_static("GetDrives", "Failed to switch back to original drive.");
	if (!ChangeDir(CurDir)) Error_static("GetDrives", "Failed to switch back to original directory.");
	return lDrives;
#endif
}

// Tar bort Microsloth's snygga macro som finns i windows.h träsket så vi faktiskt kan använda detta namn.
#ifdef CopyFile
#undef CopyFile
#endif

fint CDiskUtil::GetFileSize(CStr _File)
{
	CCFile File;
	File.Open(_File, CFILE_READ|CFILE_BINARY);
	return File.Length();
}

bool CDiskUtil::CpyFile(CStr _Src, CStr _Dst, int _BufferSize)
{
	M_TRY
	{
		CCFile Src, Dst;
		Src.Open(_Src, CFILE_READ | CFILE_BINARY);
		Dst.Open(_Dst, CFILE_WRITE | CFILE_BINARY);

		TList_Vector<uint8> lBuffer;
		lBuffer.SetLen(_BufferSize);

		int Len = Src.Length();
		int Pos = 0;
		while(!Src.EndOfFile() && (Pos < Len))
		{
			int Chunk = Min(_BufferSize, Len-Pos);
			Src.Read(lBuffer.GetBasePtr(), Chunk);
			Dst.Write(lBuffer.GetBasePtr(), Chunk);
			Pos += Chunk;
		}

		Src.Close();
		Dst.Close();
	}
	M_CATCH(
	catch(CCException)
	{
		return false;
	}
	)
	return true;
}
#ifdef PLATFORM_XENON
extern __declspec(thread) int g_IgnoreFileOpen;
#endif

CFileInfo CDiskUtil::FileTimeGet(CStr _FileName)
{
#ifdef PLATFORM_XENON
	++g_IgnoreFileOpen;
#endif

	void* hFile = MRTC_SystemInfo::OS_FileOpen(_FileName, true, false, false, false, false);
	if (!hFile)
	{
#ifdef PLATFORM_XENON
		--g_IgnoreFileOpen;
#endif
		Error_static("CDiskUtil::GetFileTime", CStrF("Unable to open file %s", _FileName.Str()));
	}

	CFileInfo FileInfo;
	FileInfo.m_Size = MRTC_SystemInfo::OS_FileSize(hFile);
	bool bResult = MRTC_SystemInfo::OS_FileGetTime(hFile, FileInfo.m_TimeCreate, FileInfo.m_TimeAccess, FileInfo.m_TimeWrite);
	MRTC_SystemInfo::OS_FileClose(hFile);

	if (!bResult)
	{
#ifdef PLATFORM_XENON
		--g_IgnoreFileOpen;
#endif
		Error_static("CDiskUtil::GetFileTime", CStrF("Failed to get file time %s", _FileName.Str()));
	}

#ifdef PLATFORM_XENON
	--g_IgnoreFileOpen;
#endif
	return FileInfo;
}

void CDiskUtil::FileTimeSet(CStr _FileName, const CFileInfo& _FileInfo)
{
#ifdef PLATFORM_XENON
	++g_IgnoreFileOpen;
#endif
	void* hFile = MRTC_SystemInfo::OS_FileOpen(_FileName, false, true, false, false, false);
	if (!hFile)
	{
		Error_static("CDiskUtil::SetFileTime", CStrF("Unable to open file %s", _FileName.Str()));
#ifdef PLATFORM_XENON
		--g_IgnoreFileOpen;
#endif
	}

	bool bResult = MRTC_SystemInfo::OS_FileSetTime(hFile, _FileInfo.m_TimeCreate, _FileInfo.m_TimeAccess, _FileInfo.m_TimeWrite);
	MRTC_SystemInfo::OS_FileClose(hFile);

	if (!bResult)
	{
		Error_static("CDiskUtil::SetFileTime", CStrF("Failed to set file time %s", _FileName.Str()));
#ifdef PLATFORM_XENON
		--g_IgnoreFileOpen;
#endif
	}
#ifdef PLATFORM_XENON
	--g_IgnoreFileOpen;
#endif
}


TArray<uint8> CDiskUtil::ReadFileToArray(CStr _FileName, int _Mode)
{
	CCFile File;
	File.Open(_FileName, _Mode);

	TArray<uint8> lFile;
	lFile.SetLen(File.Length());
	File.Read(lFile.GetBasePtr(), lFile.Len());

	return lFile;
}

void CDiskUtil::WriteFileFromArray(CStr _FileName, int _Mode, TArray<uint8> _lData)
{
	CCFile File;
	File.Open(_FileName, _Mode);
	File.Write(_lData.GetBasePtr(), _lData.Len());
	File.Close();
}

spCCFile CDiskUtil::CreateCCFile(CStr _Name, int _Mode)
{
	spCCFile spFile = MNew(CCFile);
	if (!spFile)
		Error_static("CDiskUtil::CreateCCFile", "Out of memory.");
	spFile->Open(_Name, _Mode);
	return spFile;
}

spCCFile CDiskUtil::CreateCCFile(TArray<uint8> _lFile, int _Mode)
{
	spCCFile spFile = MNew(CCFile);
	if (!spFile)
		Error_static("CDiskUtil::CreateCCFile", "Out of memory.");
	spFile->Open(_lFile, _Mode);
	return spFile;
}

TArray<uint8> CDiskUtil::Compress(TArray<uint8> _lData, ECompressTypes _eCompressType, ESettings _eCompressQuality)
{
	CCompressorInterface CI;
	CCompress* pC = CI.GetCompressor(_eCompressType, _eCompressQuality);
	if (!pC) Error_static("CDiskUtil::Compress", "No compressor.");

	TThinArray<uint8> lTmp;
	lTmp.SetLen(pC->GetCompDestExtra() + _lData.Len());

	void* pDataCompr = pC->Compress(_lData.GetBasePtr(), lTmp.GetBasePtr(), _lData.Len());
	if (!pDataCompr) Error_static("CDiskUtil::Compress", "Compress returned NULL.");

	TArray<uint8> lCompressed;
	lCompressed.SetLen(pC->GetCompressedLength());
	memcpy(lCompressed.GetBasePtr(), pDataCompr, lCompressed.Len());
	lTmp.Clear();

	delete pC;

	return lCompressed;
}

TArray<uint8> CDiskUtil::Decompress(TArray<uint8> _lData,ECompressTypes _eCompressType)
{
	CCompressorInterface CI;
	CCompress* pC = CI.GetCompressor(_eCompressType);
	if (!pC) Error_static("CDiskUtil::CreateFromCompressedFile", "No compressor.");

	TArray<uint8> lDecompressed;
	lDecompressed.SetLen(pC->GetUncompressedLength(_lData.GetBasePtr()));
	pC->Decompress(_lData.GetBasePtr(), lDecompressed.GetBasePtr());

	delete pC;

	return lDecompressed;
}

spCCFile CDiskUtil::DecompressToMemoryFile(TArray<uint8> _lData)
{
	return CreateCCFile(Decompress(_lData), CFILE_READ | CFILE_BINARY);
}

spCDataFile CDiskUtil::DecompressDataFile(TArray<uint8> _lData)
{
	spCDataFile spDataFile = MNew(CDataFile);
	if (!spDataFile)
		Error_static("CDiskUtil::DecompressDataFileEntry", "Out of memory.");

	spDataFile->Open(DecompressToMemoryFile(_lData), 0);
	return spDataFile;
}


void CDiskUtil::SearchPath_Add(CStr _Path)
{
	CByteStream::SearchPath_Add(_Path);
}

void CDiskUtil::SearchPath_SetBase(CStr _Path)
{
	CByteStream::SearchPath_SetBase(_Path);
}


void CDiskUtil::AddCorrupt(int _Corrupt)
{
#ifdef PLATFORM_CONSOLE
	CByteStream::AddCorrupt(_Corrupt);
#endif
}

void CDiskUtil::RemoveCorrupt(int _Corrupt)
{
	CByteStream::RemoveCorrupt(_Corrupt);
}

int CDiskUtil::GetCorrupt()
{
	return CByteStream::GetCorrupt();
}
