
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/


#ifndef __INC_MFile_RAM
#define __INC_MFile_RAM

struct CRAMEntry 
{
	CStr FileName;
	uint8* Location;
	mint Length;

	CRAMEntry();

	bool operator < (const CRAMEntry& re) const;
	bool operator == (const CRAMEntry& re) const;
};

class CRAMDisk : CReferenceCount 
{

	TSearchTree<CRAMEntry,0> Files;

	STANDARD_NODE(CRAMEntry,2)* LastNode;

	void FreeAll(STANDARD_NODE(CRAMEntry,2)* Node);

public:
	CRAMDisk();
	~CRAMDisk();

	mint CreateFile(const CStr& FileName, mint Size);
	mint OpenFile(const CStr& FileName);
	void ExtendFileSpace(mint handle, mint Size);
	void RemoveFile(mint handle);
	CRAMEntry& GetFileInfo(mint handle);

};

extern CRAMDisk g_RAMDiskHandler;


#endif // __INC_MFile_RAM
