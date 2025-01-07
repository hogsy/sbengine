
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

// -------------------------------------------------------------------
//  CStream_RAM and CRAMDisk
// -------------------------------------------------------------------
//
// CLASS AUTHOR:	Daniel Hansson
// STARTED	   :	970314
// LAST UPDATED:	970315
// COMMENTS	   :	Makes it possible to use the memory as a disk.
//


CRAMEntry::CRAMEntry() 
{ 
	Location=NULL; 
};

bool CRAMEntry::operator < (const CRAMEntry& re) const 
{
	return strcmp(FileName,re.FileName)<0;
}

bool CRAMEntry::operator == (const CRAMEntry& re) const
{
	return strcmp(FileName,re.FileName)==0;
};


void CRAMDisk::FreeAll(STANDARD_NODE(CRAMEntry,2)* Node)
{
	if (Node==NULL) return;
	
	FreeAll(Node->GetChild(0));
	FreeAll(Node->GetChild(1));
	
	if (Node->GetElement().Location!=NULL)
		delete[] Node->GetElement().Location;
}

CRAMDisk::CRAMDisk() 
{ LastNode=NULL; 
}

CRAMDisk::~CRAMDisk()
{
	// Free all files.
	FreeAll(Files.GetRoot());
}

mint CRAMDisk::CreateFile(const CStr& FileName, mint Size)
{
/*
// Make sure the name isn't already in use.
CRAMEntry re;
re.FileName=FileName;
if (Files.Find(re)!=NULL)
Error("CreateFile","File already exists.");
	*/
	STANDARD_NODE(CRAMEntry,2)* Node;
	
	// Delete old file with the same name if there is any.
	CRAMEntry re;
	re.FileName=FileName;
	if ((Node=Files.Find(re))!=NULL) {
		CRAMEntry* pre=&Node->GetElement();
		
		if (pre->Location!=NULL) delete[] pre->Location;
		pre->Length=Size;
		if (Size>0) {
			pre->Location=DNew(uint8) uint8[Size];
			if (pre->Location==NULL) MemError("CreateFile");
		}
		else
			pre->Location=NULL;
	}
	else {
		// Create the new file.
		re.Length=Size;
		if (Size>0) {
			re.Location=DNew(uint8) uint8[Size];
			if (re.Location==NULL) MemError("CreateFile");
		}
		else
			re.Location=NULL;
		
		Node=&Files.Insert(re);
	}
	
	return (mint)(Node);
}

mint CRAMDisk::OpenFile(const CStr& FileName)
{
	if (LastNode!=NULL && LastNode->GetElement().FileName.Len()>0
		&& strcmp(LastNode->GetElement().FileName,FileName)==0)
		return (mint)LastNode;
	
	CRAMEntry re;
	re.FileName=FileName;
	STANDARD_NODE(CRAMEntry,2)* Node=Files.Find(re);
	if (Node==NULL) return 0;
	
	LastNode=Node;
	
	return (mint)(Node);
}

void CRAMDisk::ExtendFileSpace(mint handle, mint Size)
{
	CRAMEntry* re=&(((STANDARD_NODE(CRAMEntry,2)*)handle)->GetElement());
	
	uint8* NewLocation=DNew(uint8) uint8[re->Length+Size];
	if (NewLocation==NULL) MemError("ExtendFileSpace");
	
	if (re->Location!=NULL) {
		memcpy(NewLocation,re->Location,re->Length);
		delete[] re->Location;
	}
	
	re->Length+=Size;
	re->Location=NewLocation;
}

void CRAMDisk::RemoveFile(mint handle)
{
	STANDARD_NODE(CRAMEntry,2)* Node=(STANDARD_NODE(CRAMEntry,2)*)handle;
	
	if (Node->GetElement().Location!=NULL) delete[] Node->GetElement().Location;
	
	Files.DeleteNode(*Node);
}

CRAMEntry& CRAMDisk::GetFileInfo(mint handle)
{
	STANDARD_NODE(CRAMEntry,2)* Node=(STANDARD_NODE(CRAMEntry,2)*)handle;
	return Node->GetElement();
}

CRAMDisk g_RAMDiskHandler;
