
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/


#ifndef __INC_MFile_Stream_Memory
#define __INC_MFile_Stream_Memory


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Memory based stream for use with CCFile
						
	Comments:			Used to stream to and from a given memory
						address or a supplied TArray<uint8>.
						Note that the TArray supplied in Open() will 
						be shared. Therefore no 'Get' function is 
						needed to retrieve the array file data.
\*____________________________________________________________________*/

class MCCDLLEXPORT CStream_Memory : public CStream
{
	mint m_Pos;
	mint m_Len;
	mint m_MinGrow;

	TList_Vector<uint8> m_lFileMem;
	uint8* m_pFileMem;
	int m_FileMaxLen;

	bool EOS;

public:
	CStream_Memory();
	CStream_Memory(TList_Vector<uint8> _lFileMem, int _MinGrow = 32768);
	CStream_Memory(uint8* _pMem, int _CurrentLen, int _MaxLen);
	~CStream_Memory();

	uint8* GetWritePtr(int _Pos, int _Count);

	bool FileExists(CStr _name, int _mode);
	void Open(const CStr _name, int _mode);
	void Open(TList_Vector<uint8> _lFileMem, int _Mode, int _MinGrow = 32768);
	void Open(uint8* _pMem, int _CurrentLen, int _MaxLen, int _Mode);
	void Close();
	void Read(void* dest, mint _Size);
	void Write(const void* src, mint size);
	void Seek(fint pos);
	void SeekToEnd();
	void RelSeek(fint _dPos);
	fint Pos();
	fint Length();
	bool EndOfFile();
	bool Read(CAsyncRequest *_pRequest);
	bool Write(CAsyncRequest *_pRequest);
};

#endif // __INC_MFile_Stream_Memory
