
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/


#ifndef __INC_MFile_DiskUtil
#define __INC_MFile_DiskUtil

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDiskUtil
|__________________________________________________________________________________________________
\*************************************************************************************************/
class MCCDLLEXPORT CFileInfo
{
public:
	fint m_Size;
	int64 m_TimeCreate;				// Time of creation
	int64 m_TimeWrite;				// Time of last write
	int64 m_TimeAccess;				// Time of last access

	bool operator == (const CFileInfo&_FileInfo)
	{
		if (_FileInfo.m_Size != m_Size)
			return false;
		if (_FileInfo.m_TimeWrite != m_TimeWrite)
			return false;

		return true;
	}

	bool AlmostEqual(const CFileInfo&_FileInfo)
	{
		if (_FileInfo.m_Size != m_Size)
			return false;
		if (abs((int)(_FileInfo.m_TimeCreate - m_TimeCreate)) > 40000000)
			return false;

		return true;
	}
};

enum
{
	DISKUTIL_STATUS_CORRUPT		= 1,
	DISKUTIL_STATUS_CORRUPTFILE	= 2,
	DISKUTIL_STATUS_CORRUPTWRITE= 4,
	DISKUTIL_STATUS_CORRUPTANY	= DISKUTIL_STATUS_CORRUPT + DISKUTIL_STATUS_CORRUPTFILE
};


class CXDF;

class MCCDLLEXPORT CDiskUtil
{
public:

	static void AddCorrupt(int _Corrupt);
	static void RemoveCorrupt(int _Corrupt);
	static int GetCorrupt();

	static void XDF_Record(const char* _pName, const char* _pBasePath);
	static void XDF_Use(const char* _pName, const char* _pBasePath);

	static void XDF_Pause();
	static void XDF_Resume();
	static bint XDF_IsPaused();

	static void XDF_Stop();
	static CStr XDF_GetCurrent();
	static int XDF_GetPosition();
	static CXDF *XDF_Get();

	static CStr GetDir();
	static CStr GetFullPath(CStr _Path, const char *_pRelative = NULL);
	static CStr GetRelativePath(CStr _Source, const char *_pRelative = NULL);
	static bool ChangeDir(CStr _Dir);
	static bool MakeDir(CStr _At, CStr _Name);
	static bool RemoveDir(CStr _At, CStr _Name);
	static bool RemoveDir(CStr _Dir);
	static bool DelTree(CStr _Dir);
	static bool DelTreeOnlyEmpty(CStr _Dir);
	static bool CreatePath(CStr _Name);
	static bool FileExists(CStr _Name);
	static bool DirectoryExists(CStr _Name);
	static bool DelFile(CStr _Path);
	static bool RenameFile(CStr _Path, CStr _PathTo);

	static void SearchPath_Add(CStr _Path);
	static void SearchPath_SetBase(CStr _Path);

	static CStr GetDrive();
	static bool ChangeDrive(CStr _Drive);
	static lCStr GetDrives();

	static fint GetFileSize(CStr _File);

	static bool CpyFile(CStr _Src, CStr _Dst, int _BufferSize = 32768);

	static CFileInfo FileTimeGet(CStr _FileName);
	static void FileTimeSet(CStr _FileName, const CFileInfo& _FileRec);

	// Useful stuff, unfortunately all winblows defines makes it impossible to have good function names.
	static TArray<uint8> ReadFileToArray(CStr _FileName, int _Mode);
	static void WriteFileFromArray(CStr _FileName, int _Mode, TArray<uint8> _lData);
	static spCCFile CreateCCFile(CStr _Name, int _Mode);
	static spCCFile CreateCCFile(TArray<uint8> _lFile, int _Mode);

	static TArray<uint8> Compress(TArray<uint8> _lData, ECompressTypes _eCompressType = ZLIB, ESettings _eCompressQuality = HIGH_COMPRESSION);
	static TArray<uint8> Decompress(TArray<uint8> _lData, ECompressTypes _eCompressType = ZLIB);
	static spCCFile DecompressToMemoryFile(TArray<uint8> _lData);
	static spCDataFile DecompressDataFile(TArray<uint8> _lData);	// _lData is assumed to contain a compressed data file.
};

class CDiskUtil_NoXDFScope
{
public:
	CDiskUtil_NoXDFScope()
	{
		CDiskUtil::XDF_Pause();
	}
	~CDiskUtil_NoXDFScope()
	{
		CDiskUtil::XDF_Resume();
	}
};

#define D_NOXDF CDiskUtil_NoXDFScope ScopeNoXDF

#endif // __INC_MFile_DiskUtil
