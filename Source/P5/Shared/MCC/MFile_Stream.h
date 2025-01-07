
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/


#ifndef __INC_MFile_Stream
#define __INC_MFile_Stream

class CAsyncRequest;

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Stream virtual base class.						
\*____________________________________________________________________*/
class MCCDLLEXPORT CStream : public CReferenceCount
{
public:
	int m_Mode;

	
	DECLARE_OPERATOR_NEW
	

	virtual ~CStream() {};

	virtual bool FileExists(CStr _name, int _mode) { return false; };
	virtual bool DirectoryExists(CStr _name, int _mode) { return false; };
	virtual void Open(const CStr _name, int _mode) {};
	virtual void OpenExt(const CStr _name, int _mode, float _Priority = 0, aint _NumCacheLines = -1, aint _CacheLineSize = -1) {Open(_name, _mode);};
	virtual void SetPriority(fp32 _Priority){};	
	virtual void Close() {};

	virtual CStr GetFileName(){return "";}

	virtual void Read(void* dest, mint size) {M_ASSERT(0,"Implement this ?");};
	virtual	void Write(const void* src, mint size) {M_ASSERT(0,"Implement this ?");};
	
	virtual bool Read(CAsyncRequest *_pRequest) {M_ASSERT(0,"Implement this ?"); return true;};
	virtual	bool Write(CAsyncRequest *_pRequest) {M_ASSERT(0,"Implement this ?"); return true;};
	virtual	bool AsyncFlush(bool _bBlock) {M_ASSERT(0,"Implement this ?"); return true;};

	virtual void Rename(const CStr& _Name) {M_ASSERT(0,"Implement this ?");};
	virtual void Remove() {M_ASSERT(0,"Implement this ?");};

	virtual fint Pos() { {M_ASSERT(0,"Implement this ?");} return 0; };
	virtual fint Length() { {M_ASSERT(0,"Implement this ?");}return 0; };
	virtual void Seek(fint pos) {M_ASSERT(0,"Implement this ?");};
	virtual void SeekToEnd() {M_ASSERT(0,"Implement this ?");};
	virtual void RelSeek(fint pos) {M_ASSERT(0,"Implement this ?");};
	virtual bool EndOfFile() { {M_ASSERT(0,"Implement this ?");}return 1; };
};

typedef TPtr<CStream> spCStream;

// -------------------------------------------------------------------
//  Other streams: (in MFile.cpp)
//		CStream_Disk
//		CStream_Megafile
//		CStream_RAM
//		CStream_Compressed
// -------------------------------------------------------------------


#endif // __INC_MFile_Stream
