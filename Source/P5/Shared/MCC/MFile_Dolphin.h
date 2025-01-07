/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			File handling for GameCube

	Author:			Anton Ragnarsson

	Copyright:		Copyright 2002 Starbreeze Studios AB

\*____________________________________________________________________________________________*/

#if !defined __INC_MFILE_DOLPHIN && defined PLATFORM_DOLPHIN
#define __INC_MFILE_DOLPHIN

#include "MRTC.h"


// Utility for converting backslashes into slashes..
const char* Slashify(const char* _pStr);
void        CheckDVDStatus();

// OK, now this is ugly;
extern "C" const char* g_pCurrentDisc;
extern "C" const char* g_pGameName;

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:   	MRTC_FileInfo. 
	         	BaseClass for GameCube-fileinstances.

	Comments:	Inherited by MRTC_DVDFileInfo and MRTC_HostFileInfo
\*____________________________________________________________________*/
class MRTC_FileInfo
{
public:
	virtual ~MRTC_FileInfo() {}

	virtual bool    Open( bool _bRead, bool _bWrite, bool _bCreate, bool _bTruncate) pure;
	virtual void    Close() pure;
	virtual fint GetFileSize() pure;
	virtual void*   AsyncRead(void *_pData, fint _DataSize, fint _FileOffset) pure;
	virtual void*   AsyncWrite(const void *_pData, fint _DataSize, fint _FileOffset) pure;
	virtual fint AsyncBytesProcessed() pure;
	virtual bool    AsyncIsFinished() pure;
	virtual void    AsyncClose() pure;
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:   	MRTC_DVDFileInfo. 

	Comments:	Used for DVD-access.
\*____________________________________________________________________*/
class MRTC_DVDFileInfo : public MRTC_FileInfo
{
private:
	DVDFileInfo	m_FileInfo;
	int32		m_EntryNum;
	bool        m_IsBusy;
	bool		m_Duplicated;

	// this is for retrying a failed read operation
	bool        m_bNeedRestart;
	void*       m_pData;
	fint     m_DataSize;
	fint     m_FileOffset;

private:
	MRTC_DVDFileInfo *Duplicate() const;

	enum Status { e_Busy, e_Done, e_Error };
	Status CheckStatus();

public:
	MRTC_DVDFileInfo(int32 _EntryNum)
		: m_EntryNum(_EntryNum)
		, m_IsBusy(false)
		, m_Duplicated(false)
		, m_bNeedRestart(false)
		, m_pData(NULL)
		, m_DataSize(-1)
		, m_FileOffset(0)
	{ }

	virtual bool    Open(bool _bRead, bool _bWrite, bool _bCreate, bool _bTruncate);
	virtual void    Close();
	virtual fint GetFileSize();
	virtual void*   AsyncRead(void *_pData, fint _DataSize, fint _FileOffset);
	virtual void*   AsyncWrite(const void *_pData, fint _DataSize, fint _FileOffset);
	virtual fint AsyncBytesProcessed();
	virtual bool    AsyncIsFinished();
	virtual void    AsyncClose();
};


#ifdef MRTC_ENABLE_HOSTIO

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:   	MRTC_HostFileInfo. 

	Comments:	Used for HOST I/O
\*____________________________________________________________________*/
class MRTC_HostFileInfo : public MRTC_FileInfo
{
private:
	CFStr  m_Filename;
	FILE*  m_pFile;

public:
	MRTC_HostFileInfo(const char* _pFilename)
		: m_Filename(_pFilename)
		, m_pFile(NULL) 
	{ }

	virtual bool Open(bool _bRead, bool _bWrite, bool _bCreate, bool _bTruncate);
	virtual void Close();
	virtual fint GetFileSize();
	virtual void* AsyncRead(void *_pData, fint _DataSize, fint _FileOffset);
	virtual void* AsyncWrite(const void *_pData, fint _DataSize, fint _FileOffset);
	virtual fint AsyncBytesProcessed();
	virtual bool AsyncIsFinished();
	virtual void AsyncClose();

};

#endif


	
#endif // __INC_MFILE_DOLPHIN

