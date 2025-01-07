
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/


#ifndef __INC_MFile_Misc
#define __INC_MFile_Misc

#include "MFileDef.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDirectoryNode
|__________________________________________________________________________________________________
\*************************************************************************************************/
enum
{
	FILEREC_NORMAL       = 0x00,    /* Normal file - No read/write restrictions */
	FILEREC_RDONLY       = 0x01,    /* Read only file */
	FILEREC_HIDDEN       = 0x02,    /* Hidden file */
	FILEREC_SYSTEM       = 0x04,    /* System file */
	FILEREC_SUBDIR       = 0x10,    /* Subdirectory */
	FILEREC_ARCH         = 0x20		/* Archive file */
};

class MCCDLLEXPORT CDir_FileRec : public CReferenceCount
{
public:
	CStr m_Name;
	CStr m_Ext;
	fint m_Size;
	int16 m_Attrib;
	int64 m_CreationTime;
	int64 m_WriteTime;
	int64 m_AccessTime;

	void operator= (const CDir_FileRec& _FileRec);
	CDir_FileRec();
	CDir_FileRec(CStr _Name, int _Attrib = 0, int _iSubDir = -1);

	void FormatName();
	CDir_FileRec(const CDir_FileRec& _FileRec);
	bool IsDirectory() const;
	bool IsArchive() const;
	int Compare(const CDir_FileRec& _FileRec) const;
};



/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				File container for a directory.
\*____________________________________________________________________*/
class MCCDLLEXPORT CDirectoryNode : public CReferenceCount
{
	// A list of files contained in directory
	TList_SortableVector<CDir_FileRec> m_lFiles;

	CStr GetErrorStr(int _ErrNo);

public:
	
	DECLARE_OPERATOR_NEW
	

	CDirectoryNode();
	virtual ~CDirectoryNode();
	void ReadDirectory(CStr m_Path);

	int GetFileCount();
	CDir_FileRec* GetFileRec(int _iFile);
	CStr GetFileName(int _iFile);
	CStr GetDecoratedName(int _iFile);
	int FindFile(CStr _Name);

	bool IsDirectory(int _iFile);
	bool FileExists(CStr _Name, bool _bDir);
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Log File
\*____________________________________________________________________*/
class MCCDLLEXPORT CLogFile : public ILogFile
{
	MRTC_DECLARE;

	CStr m_FileName;
	TPtr<CCFile> m_spFile;
	int m_bRecursive;
	int m_bError;

	bool OpenFile(bool _bAppend);

public:
	CLogFile();
	CLogFile(CStr _FileName, bool _bAppend = false);
	~CLogFile();
	void Create(CStr _FileName, bool _bAppend = false);

	void Log(const CStr& _s);
	void Log(const wchar* _pStr);
	void Log(const char* _pStr);
	void SetFileName(const CStr& _s, bool _bAppend = false);
};


#endif // __INC_MFile_Misc
