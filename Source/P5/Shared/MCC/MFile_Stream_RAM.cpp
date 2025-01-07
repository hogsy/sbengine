
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

#include "MFile_Stream_RAM.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CStream_RAM
|__________________________________________________________________________________________________
\*************************************************************************************************/

CStream_RAM::CStream_RAM()
{
	EOS=TRUE;
}

CStream_RAM::~CStream_RAM()
{
}

bool CStream_RAM::FileExists(CStr _name, int _mode)
{
	if( _name.Len() == 0 )
		return false;

	if ((_mode & CFILE_READ)!=0) {
		int i=0;
		if (strncmp(_name,"RAM:",4)==0 || strncmp(_name,"ram:",4)==0) {
			i+=4;
			if (_name[4]=='\\') i++;
		}
		
		return g_RAMDiskHandler.OpenFile(CStr((char*)_name+i))!=0;
	}
	else if ((_mode & CFILE_WRITE)!=0) {
		if (strncmp(_name,"RAM:",4)==0 || strncmp(_name,"ram:",4)==0)
			return TRUE;
		else
			return FALSE;
	}
	else
		return FALSE;
}

void CStream_RAM::Open(const CStr _name, int _mode)
{
	int i=0;
	if (strncmp(_name,"RAM:",4)==0 || strncmp(_name,"ram:",4)==0) {
		i+=4;
		if (_name[4]=='\\') i++;
	}
	
	// Open or create file?
	if ((_mode & CFILE_READ) != 0 || (_mode & CFILE_APPEND) != 0) {
		// Open file.
		if ((FileHandle=g_RAMDiskHandler.OpenFile(CStr((const char*)_name+i)))==0)
			FileError("Open", _name, 2);
	}
	else {
		// Create file.
		FileHandle=g_RAMDiskHandler.CreateFile(CStr((const char*)_name+i),0);
	}
	
	re=g_RAMDiskHandler.GetFileInfo(FileHandle);
	
	if ((_mode & CFILE_APPEND) != 0) {
		FilePos=re.Length;
		EOS=TRUE;
	}
	else {
		FilePos=0;
		if (re.Length>0)
			EOS=FALSE;
		else
			EOS=TRUE;
	}
}

void CStream_RAM::Close()
{
	if (Length()==0)
		g_RAMDiskHandler.RemoveFile(FileHandle);
	
	EOS=TRUE;
}

void CStream_RAM::Read(void* dest, mint size)
{
	if (!EOS) {
		memcpy(dest,re.Location+FilePos, Min((mint)(re.Length-FilePos),size));
		if ((FilePos+=size)>=re.Length)
			EOS=TRUE;
	}
}

void CStream_RAM::Write(const void* src, mint size)
{
	if (FilePos==re.Length) {
		g_RAMDiskHandler.ExtendFileSpace(FileHandle,size);
		re=g_RAMDiskHandler.GetFileInfo(FileHandle);
		memcpy(re.Location+FilePos,src,size);
		FilePos+=size;
	}
	else {
		uint32 len=Min((mint)(re.Length-FilePos),size);
		memcpy(re.Location+FilePos,src,len);
		FilePos+=len;
		if (len<size) {
			g_RAMDiskHandler.ExtendFileSpace(FileHandle,size-len);
			re=g_RAMDiskHandler.GetFileInfo(FileHandle);
			memcpy(re.Location+FilePos,(uint8*)src+len,size-len);
			FilePos+=(size-len);
		}
	}
}

void CStream_RAM::Seek(fint pos)
{
	if (pos<0 || pos>(int32)re.Length) {
		FileError("Seek", mFileName + ", Attempted to seek outside the file.", 0);
	}
	else {
		FilePos=pos;
		if (pos<(int32)re.Length)
			EOS=FALSE;
		else
			EOS=TRUE;
	}
}

void CStream_RAM::SeekToEnd()
{
	FilePos=re.Length;
	EOS=TRUE;
}

void CStream_RAM::RelSeek(fint pos)
{
	fint wantedpos=FilePos+pos;
	if (wantedpos<0 || wantedpos>(fint)re.Length) {
		FileError("RelSeek", mFileName + ", Attempted to seek outside the file.", 0);
	}
	else {
		FilePos=wantedpos;
		if (wantedpos<(fint)re.Length)
			EOS=FALSE;
		else
			EOS=TRUE;
	}
}

fint CStream_RAM::Pos()
{
	return FilePos;
}

fint CStream_RAM::Length()
{
	return re.Length;
}

bool CStream_RAM::EndOfFile()
{
	return EOS;
}
