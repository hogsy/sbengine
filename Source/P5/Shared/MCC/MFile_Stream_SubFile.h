
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/


#ifndef __INC_MFile_Stream_SubFile
#define __INC_MFile_Stream_SubFile

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Substream based stream for use with CCFile
						
	Comments:			Supply an file and a maxleng and let this 
						stream be a placeholder for a subfile inside a 
						file.
\*____________________________________________________________________*/

class MCCDLLEXPORT CStream_SubFile : public CStream
{
	spCCFile m_spFile;
	fint m_StartPos;
	fint m_SubLen;

public:

	
	DECLARE_OPERATOR_NEW
	

	CStream_SubFile();
	CStream_SubFile(spCCFile _pSubFile, int _StartPos, int _SubLen);
	~CStream_SubFile();

	uint8* GetWritePtr(int _Pos, int _Count);

	bool FileExists(CStr _name, int _mode);
	void Open(const CStr _name, int _mode);
	void OpenExt(const CStr _name, int _mode, float _Priority = 0, aint _NumCacheLines = -1, aint _CacheLineSize = -1);
	void SetPriority(fp4 _Priority);
	CStr GetFileName();
	
	bool Read(CAsyncRequest *_pRequest);
	bool Write(CAsyncRequest *_pRequest);
	void Open(spCCFile _pSubFile, int _StartPos, int _SubLen);
	void Close();
	void Read(void* dest, mint _Size);
	void Write(const void* src, mint size);
	void Seek(fint pos);
	void SeekToEnd();
	void RelSeek(fint _dPos);
	fint Pos();
	fint Length();
	bool EndOfFile();
};

#endif // __INC_MFile_Stream_SubFile
