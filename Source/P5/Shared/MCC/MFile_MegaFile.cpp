
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

#include "MFile_MegaFile.h"

/*

CCArchiveContainer g_Archives;		// Global archive container.

//////////////////
// CCFileArchive //
//////////////////

IMPLEMENT_OPERATOR_NEW(CCFileArchive);



bool CCFileArchive::OpenArchive(const CStr& ArcName, eArchiveOpenModes mode)
{
	CloseArchive();

	fstream f;
	
	// Try to open the archive file.
	f.open((const char*)ArcName
#ifdef PLATFORM_DREAMCAST
	,ios::nocreate | ios::in | ios::binary
#elif defined PLATFORM_DOLPHIN
	,ios::in | ios::binary
#else
	,ios::nocreate | ios::in | ios::binary
#endif
	);

	if (!f.good())
	{
#if defined PLATFORM_DREAMCAST || defined PLATFORM_DOLPHIN
		return FALSE;	// No writing to CD/DVD

#else
		if (mode==ARCHIVE_NO_CREATE) return FALSE;

		f.clear();	// Clear error flag.

		// Try to create an archive file.
		f.open((const char*)ArcName,ios::out | ios::binary);

		if (!f.good())
			return FALSE;

		// Write archive Id.        "
		f.write("MOS FileArchive 1.0",19);

		// Write major and minor version numbers.
		MajorVersion=1;	// Version 1.0
		MinorVersion=0;

#ifdef CPU_BIGENDIAN
	Swap_uint16(MajorVersion);
	Swap_uint16(MinorVersion);
#endif
		f.write((char*)&MajorVersion,2);
		f.write((char*)&MinorVersion,2);

#ifdef CPU_BIGENDIAN
	Swap_uint16(MajorVersion);
	Swap_uint16(MinorVersion);
#endif
		// Write the amount of files contained within the archive.
		NrFiles=0;
		f.write((char*)&NrFiles,2);
#endif
	}
	else {
		// Read the header of the archive file.

		// Get archive Id.
		char ArchiveId[20]="                   ";
		f.read(ArchiveId,19);

		if (strcmp(ArchiveId,"MOS FileArchive 1.0")!=0) {
			f.close();
			return FALSE;
		}

		// Get major and minor version numbers.
		f.read((char*)&MajorVersion,2);
		f.read((char*)&MinorVersion,2);

#ifdef CPU_BIGENDIAN
	Swap_uint16(MajorVersion);
	Swap_uint16(MinorVersion);
#endif
		// Supported archive version? (Only v1.0 at the moment).
		if (MajorVersion!=1 || MinorVersion!=0)
			Error("OpenArchive","Unsupported archive version.");

		// Get the amount of files contained within the archive.
		f.read((char*)&NrFiles,2);
#ifdef CPU_BIGENDIAN
	Swap_uint16(NrFiles);
#endif

		// Read the archive file list and put them in a
		// binary search tree.
		CCArchiveEntry ae;
		uint8 FileName[1025];
		uint16 NameLength;

		for (uint32 i=0; i<NrFiles; i++) {
			f.read((char*)&NameLength,2);
#ifdef CPU_BIGENDIAN
	Swap_uint16(NameLength);
#endif
			if (NameLength<=1024) {
				f.read((char*)FileName,NameLength);
				FileName[NameLength]=0;
				ae.FileName=CStr((char*)FileName);
				f.read((char*)&ae.Length,4);
#ifdef CPU_BIGENDIAN
	ByteSwap_uint32(ae.Length);
#endif
				ae.Offset=(uint32)f.tellp();
				ArchiveList.Insert(ae);
				f.seekp(ae.Length,ios::cur);
			}

			if (!f.good() || NameLength>1024) {
				f.close();
				ArchiveList.Clear();
				return FALSE;
			}

		}

	}

	f.close();

	IsOpen=TRUE;

	ArchiveFileName=ArcName;

	return TRUE;
}

void CCFileArchive::CloseArchive()
{
	if (IsOpen) {

		ArchiveList.Clear();

		LastEntry.FileName=CStr("");

		IsOpen=FALSE;
	}

}

CStr CCFileArchive::GetArchiveFileName()
{
	if (!IsOpen) return CStr("");
	return ArchiveFileName;
}

CCArchiveEntry* CCFileArchive::GetFileInfo(const CStr& FileName)
{
	if (!IsOpen) return NULL;

	if (LastEntry.FileName.Len()>0 && strcmp(LastEntry.FileName,FileName)==0)
		return &LastEntry;

	CCArchiveEntry ae;
	ae.FileName=FileName;
	STANDARD_NODE(CCArchiveEntry,2)* Node=ArchiveList.Find(ae);
	if (Node==NULL)
		return NULL;
	else {
		LastEntry=Node->GetElement();
		return &LastEntry;
	}
}

uint32 CCFileArchive::GetFileOffset(const CStr& FileName)
{
	CCArchiveEntry* ae=GetFileInfo(FileName);
	if (ae==NULL) return 0;
	return ae->Offset;
}

uint32 CCFileArchive::GetFileLength(const CStr& FileName)
{
	CCArchiveEntry* ae=GetFileInfo(FileName);
	if (ae==NULL) return 0;
	return ae->Length;
}

void* CCFileArchive::GetFile(const CStr& FileName,void *Data)
{
	if (!IsOpen) return NULL;

	CCArchiveEntry* ae=GetFileInfo(FileName);
	if (ae==NULL) return NULL;

	if (Data==NULL) Data=DNew(uint8) uint8[ae->Length];
	if (Data==NULL) MemError("GetFile");

	fstream f;
	f.open(ArchiveFileName
#ifdef PLATFORM_DREAMCAST
		,ios::nocreate | ios::in | ios::binary
#elif defined PLATFORM_DOLPHIN
		,ios::in | ios::binary
#else
		,ios::nocreate | ios::in | ios::binary
#endif
	);
	f.seekp(ae->Offset);
	f.read((char*)Data,ae->Length);
	f.close();

	return Data;
}

bool CCFileArchive::AddFile(const CStr& FileName,void *Data,uint32 Len, ECompressTypes type, ESettings set)
{
#ifdef PLATFORM_DREAMCAST
	return FALSE;
#else

	if (!IsOpen) return FALSE;

	// Make sure there isn't any file with this
	// name already.
	CCArchiveEntry ae;
	ae.FileName=FileName;
	if (ArchiveList.Find(ae)!=NULL) return FALSE;
	
	// Open the archive file.
	CCFile dst;
	dst.Open(ArchiveFileName,CFILE_WRITE | CFILE_BINARY | CFILE_APPEND);

	// Write the new file's name and length to the archive.
	uint16 FileNameLength=ae.FileName.Len();
	dst.Write((char*)&FileNameLength,2);
	dst.Write((char*)ae.FileName,FileNameLength);
	uint32 Dummy;
	dst.Write(&Dummy,4);	// Skip file length.

	// Save the file end.
	int32 FileEnd=dst.Pos();

	// Increase the amount of files in the archive.
	NrFiles++;
	dst.Seek(23);
	dst.Write((char*)&NrFiles,2);

	// Close the archive file.
	dst.Close();

	// Reopen the archive file.
	// (We want to be able to compress).
	CCFile f;
	f.Open(ArchiveFileName,CFILE_WRITE | CFILE_BINARY | CFILE_APPEND,type,set);

	// Save the data into the archive.
	f.Write((char*)Data,Len);

	f.Close();

	// Reopen the archive a second time (hmm).
	f.Open(ArchiveFileName,CFILE_WRITE | CFILE_BINARY | CFILE_APPEND);
	int32 FileEnd2=f.Length();
	f.Seek(FileEnd-4);
	uint32 FileLength=FileEnd2-FileEnd;
	f.Write(&FileLength,4);
	f.Close();

	// Add the file to the archive list.
	ae.Offset=FileEnd;
	ae.Length=FileLength;
	ArchiveList.Insert(ae);

	return TRUE;
#endif
}

bool CCFileArchive::AddFile(const CStr& FileName, ECompressTypes type, ESettings set)
{
#ifdef PLATFORM_DREAMCAST
	return FALSE;
#else
	if (!IsOpen) return FALSE;

	// Make sure there isn't any file with this
	// name already.
	CCArchiveEntry ae;
	ae.FileName=FileName;
	if (ArchiveList.Find(ae)!=NULL) return FALSE;

	// Open the source file.
	CCFile src;
	src.Open(FileName,CFILE_READ | CFILE_BINARY);

	// Get source file length.
	uint32 len = src.Length();

	// Open the archive file.
	CCFile dst;
	dst.Open(ArchiveFileName,CFILE_WRITE | CFILE_BINARY | CFILE_APPEND);

	// Write the new file's name and length to the archive.
	uint16 FileNameLength=ae.FileName.Len();
	dst.Write((char*)&FileNameLength,2);
	dst.Write((char*)ae.FileName,FileNameLength);
	uint32 Dummy;
	dst.Write(&Dummy,4);	// Skip file length.

	// Save the file end.
	int32 FileEnd=dst.Pos();

	// Increase the amount of files in the archive.
	NrFiles++;
	dst.Seek(23);
	dst.Write((char*)&NrFiles,2);

	dst.Close();

	// Reopen the archive file.
	// (We want to be able to compress).
	CCFile f;
	f.Open(ArchiveFileName,CFILE_WRITE | CFILE_BINARY | CFILE_APPEND,type,set);

	// Copy the source file into the archive
	// using a 4kb buffer.
	uint8 Buffer[4096];

	uint32 Left=len;
	uint32 Size;
	while (Left>0) {
		Size = Min(Left,(uint32)4096);
		src.Read(Buffer,Size);
		f.Write(Buffer,Size);
		Left-=Size;
	}

	// Close both files.
	src.Close();
	f.Close();

	// Reopen the archive a second time (hmm).
	f.Open(ArchiveFileName,CFILE_WRITE | CFILE_BINARY | CFILE_APPEND);
	int32 FileEnd2=f.Length();
	f.Seek(FileEnd-4);
	uint32 FileLength=FileEnd2-FileEnd;
	f.Write(&FileLength,4);
	f.Close();

	// Add the file to the archive list.
	ae.Offset=FileEnd;
	ae.Length=FileLength;
	ArchiveList.Insert(ae);

	return TRUE;
#endif
}

bool CCFileArchive::RemoveFile(const CStr& FileName)
{
#ifdef PLATFORM_DREAMCAST
	return FALSE;
#else

	if (!IsOpen) return FALSE;

	Error("RemoveFile","Not implemented yet!");

	return TRUE;
#endif
}

///////////////////////
// CCArchiveContainer //
///////////////////////

CCFileArchive* CCArchiveContainer::GetFileOwner(const CStr& FileName)
{
	bool Found=FALSE;
	int i;
	for (i=0; i<FileArchiveList.Len(); i++) {
		if (FileArchiveList[i]->GetFileInfo(FileName)!=NULL) {
			Found=TRUE;
			break;
		}
	}

	if (Found)
		return FileArchiveList[i];
	else
		return NULL;
}

void CCArchiveContainer::RegisterArchive(const CStr& ArcName)
{
	CCFileArchive* pArc=DNew(CCFileArchive) CCFileArchive;
	if (pArc==NULL) MemError("RegisterArchive");

	if (!pArc->OpenArchive(ArcName))
		Error("RegisterArchive","Failed to open file archive.");

	FileArchiveList.Add(pArc);
}

void CCArchiveContainer::UnregisterArchive(const CStr& ArcName)
{
	bool Found=FALSE;
	int i;
	for (i=0; i<FileArchiveList.Len(); i++) {
		if (strcmp(FileArchiveList[i]->GetArchiveFileName(),ArcName)==0) {
			Found=TRUE;
			break;
		}
	}

	if (Found) {
		FileArchiveList[i]->CloseArchive();
		delete FileArchiveList[i];
		FileArchiveList.Del(i);
	}
	else
		Error("UnregisterArchive","Can't unregister an archive that's not been registered.");
}
*/
