
#ifndef _INC_MARCHIVE
#define _INC_MARCHIVE

#include "MDA.h"
#include "MFile.h"

// -------------------------------------------------------------------
//  CCFileArchive
// -------------------------------------------------------------------
//
// CLASS AUTHOR:	Daniel Hansson
// STARTED	   :	970310
// LAST UPDATED:	970311
// COMMENTS	   :	Contains information about a certain file archive.
//

struct CCArchiveEntry {
	CStr FileName;
	uint32 Offset;
	uint32 Length;

	bool operator < (const CCArchiveEntry& ae) const
		{return strcmp(FileName,ae.FileName)<0;};
	bool operator == (const CCArchiveEntry& ae) const
		{return strcmp(FileName,ae.FileName)==0;};
};

enum eArchiveOpenModes {ARCHIVE_NO_CREATE, ARCHIVE_CREATE};

class MCCDLLEXPORT CCFileArchive : public CReferenceCount
{

	CStr ArchiveFileName;

	TSearchTree<CCArchiveEntry,0> ArchiveList;

	uint16 MajorVersion,MinorVersion;
	uint16 NrFiles;

	bool IsOpen;

	CCArchiveEntry LastEntry;

public:
	
	DECLARE_OPERATOR_NEW
	

	CCFileArchive() { IsOpen=FALSE; LastEntry.FileName=CStr(""); };
	virtual ~CCFileArchive() { CloseArchive(); };

	bool OpenArchive(const CStr& ArcName, eArchiveOpenModes mode=ARCHIVE_NO_CREATE);
	void CloseArchive();

	CStr GetArchiveFileName();

	CCArchiveEntry* GetFileInfo(const CStr& FileName);
	uint32 GetFileOffset(const CStr& FileName);
	uint32 GetFileLength(const CStr& FileName);

	void* GetFile(const CStr& FileName,void *Data);

	bool AddFile(const CStr& FileName,void *Data,uint32 Len, ECompressTypes type=NO_COMPRESSION, ESettings set=NORMAL_COMPRESSION);
	bool AddFile(const CStr& FileName, ECompressTypes type=NO_COMPRESSION, ESettings set=NORMAL_COMPRESSION);

	bool RemoveFile(const CStr& FileName);
};


// -------------------------------------------------------------------
//  CCArchiveContainer
// -------------------------------------------------------------------
//
// CLASS AUTHOR:	Daniel Hansson
// STARTED	   :	970310
// LAST UPDATED:	970311
// COMMENTS	   :	A container holding archives.
//

class MCCDLLEXPORT CCArchiveContainer : public CReferenceCount {

	TList_Linked<CCFileArchive*> FileArchiveList;

public:
	virtual ~CCArchiveContainer()
	{
		for (int i=0; i<FileArchiveList.Len(); i++) delete FileArchiveList[i];
	}

	CCFileArchive* GetFileOwner(const CStr& FileName);

	void RegisterArchive(const CStr& ArcName);
	void UnregisterArchive(const CStr& ArcName);
};

#endif // _INC_MARCHIVE




