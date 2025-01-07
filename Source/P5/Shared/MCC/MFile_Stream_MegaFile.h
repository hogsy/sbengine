
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/


#ifndef __INC_MFile_Stream_MegaFile
#define __INC_MFile_Stream_MegaFile


// -------------------------------------------------------------------
//  CStream_Megafile
// -------------------------------------------------------------------
//
// CLASS AUTHOR:	Daniel Hansson
// STARTED	   :	970310
// LAST UPDATED:	970315
// COMMENTS	   :	Handles files thru archives.
//

/*
class MCCDLLEXPORT CStream_Megafile : public CStream
{
	CByteStream m_Stream;

	CCArchiveEntry m_AE;	// Represents the opened file.
	uint32 m_FileEnd;

	CStr m_FileName;	// Only used for error reporting

	bool m_EOS;			// End of stream flag.

public:

	CStream_Megafile();
	~CStream_Megafile();
	bool FileExists(CStr _Name, int _Mode);
	void Open(const CStr _Name, int _Mode);
	void Close();
	void Read(void* _pDest, uint32 _Size);
	void Write(const void* _pSrc, uint32 _Size);
	void Seek(int32 _Pos);
	void SeekToEnd();
	void RelSeek(int32 _Pos);
	int32 Pos();
	int32 Length();
	bool EndOfFile();
};

*/
#endif // __INC_MFile_Stream_MegaFile
