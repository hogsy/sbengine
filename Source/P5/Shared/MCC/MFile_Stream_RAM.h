
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/


#ifndef __INC_MFile_Stream_RAM
#define __INC_MFile_Stream_RAM

#include "MFile_RAM.h"
// -------------------------------------------------------------------
//  CStream_RAM
// -------------------------------------------------------------------
class MCCDLLEXPORT CStream_RAM : public CStream
{
	uint32 FileHandle;
	uint32 FilePos;
	CStr mFileName;	// Only used for error reporting

	CRAMEntry re;	// Represents the file.

	bool EOS;

public:
	CStream_RAM();
	~CStream_RAM();

	bool FileExists(CStr _name, int _mode);
	void Open(const CStr _name, int _mode);
	void Close();
	void Read(void* dest, mint size);
	void Write(const void* src, mint size);
	void Seek(fint pos);
	void SeekToEnd();
	void RelSeek(fint pos);
	fint Pos();	
	fint Length();
	bool EndOfFile();
};


#endif // __INC_MFile_Stream_RAM
