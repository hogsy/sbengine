
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/


#ifndef __INC_MFile_Stream_Compressed
#define __INC_MFile_Stream_Compressed


// -------------------------------------------------------------------
//  CStream_Compressed
// -------------------------------------------------------------------
//
// CLASS AUTHOR:	Daniel Hansson
// STARTED	   :	970311
// LAST UPDATED:	970315
// COMMENTS	   :	A "fake" stream that operates thru one of the
//					standard streams to handle compressed files as
//					if they we're normal files.
//

#define COMPRESSION_BUFFER_SIZE	65536

class MCCDLLEXPORT CStream_Compressed : public CStream
{
	CStream* Target;
	CCompress* Compressor;

	uint32 BufferPos;
	int32 BlockNr;
	bool BufferNeedsUpdate;
	uint32 FileLength;				// Compressed FileLength - Only used in write mode.
	uint32 Uncompressed_FileLength;

	int32 StartPos;

	bool EOS;

	uint8* Buffer;

public:

	CStream_Compressed();
	~CStream_Compressed();

	void Open(CStream* _Target, CCompress* _Compressor, int _mode);
	void Close();
	void UpdateBuffer();
	void Read(void* dest, mint size);
	void Write(const void* src, mint size);
	void Seek(fint pos);
	void SeekToEnd();
	void RelSeek(fint pos);
	fint Pos();
	fint Length();
	bool EndOfFile();
	
};

#endif // __INC_MFile_Stream_Compressed
