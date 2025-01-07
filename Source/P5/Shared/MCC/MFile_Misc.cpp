
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

#ifdef PLATFORM_WIN
# include <errno.h>
# include <io.h>
extern CFStr ReplaceSlashes(const char* _pFileName);
#endif

#ifdef PLATFORM_DOLPHIN
# include "MFile_Dolphin.h"
#endif

#ifdef PLATFORM_PS2
#include <sifdev.h>
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDirectoryNode
|__________________________________________________________________________________________________
\*************************************************************************************************/

void CDir_FileRec::operator= (const CDir_FileRec& _FileRec)
{
	m_Name = _FileRec.m_Name;
	m_Ext = _FileRec.m_Ext;
	m_Attrib = _FileRec.m_Attrib;
	m_Size = _FileRec.m_Size;
	m_CreationTime = _FileRec.m_CreationTime;
	m_WriteTime = _FileRec.m_WriteTime;
	m_AccessTime = _FileRec.m_AccessTime;

}

CDir_FileRec::CDir_FileRec()
{
	m_Attrib = 0;
	m_Size = 0;
	m_CreationTime = 0;
	m_WriteTime = 0;
	m_AccessTime = 0;
}

CDir_FileRec::CDir_FileRec(CStr _Name, int _Attrib, int _iSubDir)
{
	m_Name = _Name;
	m_Ext = m_Name.GetFilenameExtenstion();
	m_Attrib = _Attrib;
	m_Size = 0;
	m_CreationTime = 0;
	m_WriteTime = 0;
	m_AccessTime = 0;
	FormatName();
}

void CDir_FileRec::FormatName()
{
	char* pStr = m_Name;
	if (!pStr) return;
	int Len = m_Name.Len();

	int i = 0;
	while(i < Len)
	{
		if ((pStr[i] >= 'a') && (pStr[i] <= 'z')) break;
		i++;
	}
	if (i == Len)
	{
		// No lowercase letter found.
		m_Name = CStr(m_Name[0]) + m_Name.Copy(1, Len-1).LowerCase();
	}
}

CDir_FileRec::CDir_FileRec(const CDir_FileRec& _FileRec)
{
	*this = _FileRec;
}

bool CDir_FileRec::IsDirectory() const
{
	return ((m_Attrib & FILEREC_SUBDIR) != 0);
}

bool CDir_FileRec::IsArchive() const
{
	return ((m_Attrib & FILEREC_SUBDIR) == 0);
//	return ((m_Attrib & _A_ARCH) != 0);
}

int CDir_FileRec::Compare(const CDir_FileRec& _FileRec) const
{
	if (IsDirectory())
	{	if (!_FileRec.IsDirectory()) return -1; }
	else
	{	if (_FileRec.IsDirectory()) return 1; }

	int Cmp = m_Ext.CompareNoCase(_FileRec.m_Ext);
	if (!Cmp) Cmp = m_Name.CompareNoCase(_FileRec.m_Name);
	return Cmp;
}

// -------------------------------------------------------------------

IMPLEMENT_OPERATOR_NEW(CDirectoryNode);



CStr CDirectoryNode::GetErrorStr(int _ErrNo)
{
/*	switch(_ErrNo)
	{
#ifdef ENOENT
	case ENOENT :
		return "Directory not found.";
#endif

#ifdef EINVAL
	case EINVAL :
		return "Invalid path.";
#endif
	default :*/
		return CStrF("Unknown error. (%.8x)", _ErrNo);
	//}
}


CDirectoryNode::CDirectoryNode()
{
}

CDirectoryNode::~CDirectoryNode()
{
	m_lFiles.Clear();
}

void CDirectoryNode::ReadDirectory(CStr _Path)
{
	_Path = _Path.Ansi();
#ifdef PLATFORM_DREAMCAST
	MACRO_GetRegisterObject(CRegistry, pFileList, "REGISTRY.FILELIST");

	m_lFiles.Clear();
	m_lFiles.SetGrow(128);

	CRegistry* pTmp = NULL;

	if (_Path == "GDROM:\\")
	{
		pTmp = pFileList;
	} else {
		if (_Path == "\\")
		{
			pTmp = pFileList;
		} else {
			CStr Path = _Path;
			if ((Path.Copy(0, 1) != "\\") && (Path.Copy(0, 7) != "GDROM:\\")) Path = CDiskUtil::GetDir() + Path;
			if (Path.Copy(0, 7) == "GDROM:\\") Path = Path.Copy(7, Path.Len()-7);
			if (Path.Copy(Path.Len()-1, 1) == "\\") Path = Path.Copy(0, Path.Len()-1);
			if (Path.Copy(Path.Len()-4, 4) == "\\*.*") Path = Path.Copy(0, Path.Len()-4);
			pTmp = pFileList->Find(Path);
		}
	}

	if (!pTmp) return;		// Uhrm, how come this happen?

	CDir_FileRec FileRec;
	for (int t=0; t<pTmp->GetNumChildren(); t++)
	{
		FileRec.m_Name = pTmp->GetName(t);
		FileRec.m_Ext = FileRec.m_Name.GetFilenameExtenstion();
		FileRec.m_Size = -1;
		FileRec.m_Attrib = (pTmp->GetValue(t) == "FILE") ? _A_NORMAL : _A_SUBDIR;
		FileRec.FormatName();
		m_lFiles.Add(FileRec);
	}
// -------------------------------------------------------------------
#elif defined PLATFORM_DOLPHIN
	m_lFiles.Clear();
	m_lFiles.SetGrow(128);

	//AR-CHANGES: 
	//  convert backslashes, and remove any trailing "/*.*"
	//  (pattern will be ignored..)
	// TODO: check if patterns need to be supported.
	//       check if this hack will cause trouble with strings like "dir1/dir2" instead of "dir1/dir2/*.*"
	if (strncmp(_Path, "HOST:", 5) != 0)
	{
		CStr Path2 = Slashify(_Path);
		int Pos = Path2.FindReverse("/");
		if (Pos > 0)
			Path2 = Path2.Left(Pos);

	DVDDir DirInfo;

		if (!DVDOpenDir(Path2, &DirInfo))
		Error("ReadDirectory", "DVDOpenDir failed.");

	DVDDirEntry EntryInfo;

	while(DVDReadDir(&DirInfo, &EntryInfo))
	{
		CDir_FileRec FileRec;
		FileRec.m_Name.Capture(EntryInfo.name);
		FileRec.m_Ext = FileRec.m_Name.GetFilenameExtenstion();
		FileRec.m_Attrib = FILEREC_RDONLY | ((EntryInfo.isDir) ? FILEREC_SUBDIR : FILEREC_ARCH);
		FileRec.FormatName();
		m_lFiles.Add(FileRec);
	}

	DVDCloseDir(&DirInfo);
	}

// -------------------------------------------------------------------
#elif defined PLATFORM_PS2
extern int PS2File_FindNext( int, char *, int&, bool& );
extern int PS2File_FindFirst( const char *, char *, int&, bool& );
extern void PS2File_FixPath( char *_String );
extern void PS2File_FindClose( int _handle );

	CStr Path = _Path.Ansi();
	Path.SetChar( Path.Len() - 4, 0 );
	PS2File_FixPath( Path.GetStr() );
	char aDir[1024];
	int size;
	bool Directory;
	int handle = PS2File_FindFirst( Path.GetStr(), aDir, size, Directory );

	m_lFiles.Clear();
	m_lFiles.SetGrow(128);

	if( handle >= 0 )
	{
		int newhandle = handle;
		do
		{
			handle = newhandle;
			CDir_FileRec FileRec;

			FileRec.m_Name.Capture( aDir );
			FileRec.m_Ext = FileRec.m_Name.GetFilenameExtenstion();
			FileRec.m_Size = size;
			FileRec.m_Attrib = FILEREC_RDONLY | (Directory?FILEREC_SUBDIR:FILEREC_ARCH);
			FileRec.FormatName();
			m_lFiles.Add(FileRec);
		}
		while( ( newhandle = PS2File_FindNext( handle, aDir, size, Directory ) ) >= 0 );
		PS2File_FindClose( handle );
	}
// -------------------------------------------------------------------
#elif defined PLATFORM_PS3
extern aint PS3File_FindNext( aint, char *, int&, bool& );
extern aint PS3File_FindFirst( const char *, char *, int&, bool& );
extern void PS3File_FixPath( char *_String );
extern void PS3File_FindClose( aint _handle );

	CStr Path = _Path.Ansi();
//	Path.SetChar( Path.Len() - 4, 0 );
	PS3File_FixPath( Path.GetStr() );
	char aDir[1024];
	int size;
	bool Directory;
	int handle = PS3File_FindFirst( Path.GetStr(), aDir, size, Directory );

	m_lFiles.Clear();
	m_lFiles.SetGrow(128);

	if( handle)
	{
		do
		{
			CDir_FileRec FileRec;

			FileRec.m_Name.Capture( aDir );
			FileRec.m_Ext = FileRec.m_Name.GetFilenameExtenstion();
			FileRec.m_Size = size;
			FileRec.m_Attrib = FILEREC_RDONLY | (Directory?FILEREC_SUBDIR:FILEREC_ARCH);
			FileRec.FormatName();
			m_lFiles.Add(FileRec);
		}
		while( PS3File_FindNext( handle, aDir, size, Directory ));
		PS3File_FindClose( handle );
	}
// -------------------------------------------------------------------
#else
	m_lFiles.Clear();
	m_lFiles.SetGrow(128);
	_finddata_t FileInfo;

	CFStr WinPath = ReplaceSlashes(_Path);
	int hPath = _findfirst(WinPath, &FileInfo);
	if (hPath == -1)
	{
		if (errno == ENOENT) return;
		Error("ReadDirectory", GetErrorStr(errno));
	}
	while(1)
	{
//		AddFile(&FileInfo);
		{
			CDir_FileRec FileRec;
			FileRec.m_Name.Capture(&FileInfo.name[0]);
			FileRec.m_Ext = FileRec.m_Name.GetFilenameExtenstion();
			FileRec.m_Size = FileInfo.size;
			FileRec.m_Attrib = FileInfo.attrib;
			FileRec.m_CreationTime = FileInfo.time_create;
			FileRec.m_WriteTime = FileInfo.time_write;
			FileRec.m_AccessTime = FileInfo.time_access;
			FileRec.FormatName();
			m_lFiles.Add(FileRec);
		}

		int Ret = _findnext(hPath, &FileInfo);
		if (Ret != 0) break;
	}

	_findclose(hPath);
#endif

	CXDF *pXDF = CDiskUtil::XDF_Get();
	if (pXDF)
		pXDF->EnumFiles(m_lFiles, _Path);

	m_lFiles.OptimizeMemory();
	m_lFiles.Sort();
}

int CDirectoryNode::GetFileCount()
{
	return m_lFiles.Len();
}

CDir_FileRec* CDirectoryNode::GetFileRec(int _iFile)
{
	return &m_lFiles[_iFile];
}

CStr CDirectoryNode::GetFileName(int _iFile)
{
	return m_lFiles[_iFile].m_Name;
}

CStr CDirectoryNode::GetDecoratedName(int _iFile)
{
	if (m_lFiles[_iFile].IsDirectory())
		return "<" + m_lFiles[_iFile].m_Name + ">";
	return m_lFiles[_iFile].m_Name;
}

int CDirectoryNode::FindFile(CStr _Name)
{
	CDir_FileRec Rec(_Name);
	int iFind = m_lFiles.Find(Rec);
	return iFind;
}

bool CDirectoryNode::IsDirectory(int _iFile)
{
	return m_lFiles[_iFile].IsDirectory();
}

bool CDirectoryNode::FileExists(CStr _Name, bool _bDir)
{
	CDir_FileRec Rec(_Name, (_bDir) ? FILEREC_SUBDIR : FILEREC_ARCH);
	int iFind = m_lFiles.Find(Rec);
	return (iFind != -1);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CLogFile
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT_DYNAMIC(CLogFile, ILogFile);

CLogFile::CLogFile()
{
	m_bRecursive = false;
	m_bError = false;
}

CLogFile::CLogFile(CStr _FileName, bool _bAppend)
{
	Create(_FileName, _bAppend);
}

CLogFile::~CLogFile()
{
	m_spFile = NULL;
}

#ifdef PLATFORM_WIN_PC
void gf_SetLogFileName(CStr _FileName);
#endif

bool CLogFile::OpenFile(bool _bAppend)
{
	m_bRecursive = true;
	if (m_bError)
	{
		m_bRecursive = false;
		return false;
	}
	if (!m_spFile)
	{
		for (int i = 0; i < 16; ++i)
		{
			m_spFile = MNew(CCFile);

			if (!m_spFile) 
				MemError("-");
			M_TRY
			{
				CStr Path;
				if(i == 0)
					Path = CStrF("%s.log", m_FileName.Str(), i);
				else
					Path = CStrF("%s_%d.log", m_FileName.Str(), i);
				m_spFile->Open(Path, CFILE_WRITE | CFILE_NOLOG | ((_bAppend) ? CFILE_APPEND : 0));
#ifdef PLATFORM_WIN_PC
				gf_SetLogFileName(Path);
#endif
				break;
	//			m_spFile->Close();
			}
			M_CATCH(
			catch (...)
			{
				m_bError = true;
				try
				{
					m_spFile->Close();
				}
				catch (...)
				{
				}
				try
				{
					m_spFile = NULL;
				}
				catch (...)
				{
					try
					{
						m_spFile = NULL;
					}
					catch (...)
					{
						// Error from hell
						m_bRecursive = false;
						return false;
					}
				}
				m_bRecursive = false;
				return false;
			}
			)
		}
	}
	m_bRecursive = false;
	return true;
}

void CLogFile::Create(CStr _FileName, bool _bAppend)
{
#ifndef M_RTM
	m_bRecursive = true;
	m_bError = 0;
	m_FileName = _FileName;
	M_TRY
	{
		m_spFile = NULL;
	}
	M_CATCH(
	catch (...)
	{
		try
		{
			m_spFile = NULL;
		}
		catch (...)
		{
			// Error from hell
			return;
		}
	}
	)
	OpenFile(_bAppend);
	m_bRecursive = false;
#endif
}

static int IsControlCode(const char *_pStr, int _iPos)
{
	int CodeLen = 0;

	char moo = '§';
	if(_pStr[_iPos] == moo)
	{
		CodeLen = 1;
		switch(_pStr[_iPos + 1])
		{
			case 'd':
			case 'D':
				CodeLen = 2;

			case 'a':
			case 'A':
				CodeLen = 3;
				break;

			case 'z':
			case 'Z':
				CodeLen = 4;
				break;

			case 'c':
			case 'C':
			case 'x':
			case 'X':
			case 'y':
			case 'Y':
				CodeLen = 5;
		}
	}

	return CodeLen;
}

void CLogFile::Log(const wchar* _pStr)
{
	if (m_bRecursive)
		return;
	m_bRecursive = true;


#ifndef M_RTM
	if (!OpenFile(true))
		return;

	M_TRY
	{
		m_spFile->Writeln(_pStr);
		m_spFile->AsyncFlush(false);
	}
	M_CATCH(
	catch(...)
	{
		try
		{
			m_spFile->Close();
		}
		catch (...)
		{
		}
		try
		{
			m_spFile = NULL;
		}
		catch (...)
		{
			try
			{
				m_spFile = NULL;
			}
			catch (...)
			{
				// Error from hell
				return;
			}
		}
	}
	)
#endif
	m_bRecursive = false;
}

void CLogFile::Log(const char* _pStr)
{
	wchar Buffer[4096];
	int Len = Min(4095, (int)strlen(_pStr));

	CStrBase::mfsncpy(Buffer, CSTR_FMT_UNICODE, _pStr, CSTR_FMT_ANSI, Len);
	Buffer[Len] = 0;
	Log(Buffer);
}

void CLogFile::Log(const CStr& _s)
{
	CStr st = _s;
	
	if(_s != "" && _s.IsAnsi())
	{
		//Alloc maximum length for st (no alloc function in CStr?)
		st.MakeUnique();
		char *pSt = st;
		int Len = _s.Len();

		int iIndex = 0;
		for(int i = 0; i < Len; i++)
		{
			int CodeLen = IsControlCode(_s, i);
			if(CodeLen > 0)
				i += CodeLen - 1;
			else
				pSt[iIndex++] = _s[i];
		}
		pSt[iIndex++] = 0;
	}

	if(st.IsAnsi())
		Log(st.Str());
	else
		Log(st.StrW());
}

void CLogFile::SetFileName(const CStr& _FileName, bool _bAppend)
{
#ifndef M_RTM
	if (_FileName == m_FileName) return;
	if (m_spFile!=NULL) m_spFile->Close();
	Create(_FileName, _bAppend);
#endif
}


