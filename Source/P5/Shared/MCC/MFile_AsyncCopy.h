
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/



class CFileAsyncCopy_Internal;

class MCCDLLEXPORT CFileAsyncCopy
{
	CFileAsyncCopy_Internal	*m_pInternal;

public:

	CFileAsyncCopy();
	~CFileAsyncCopy();

	void Pause();
	void Resume();

	bool CpyFile(const char *_pFrom, const char *_pTo, int _BufferSize);

	bool Done();

};

class CFileAsyncWrite_Internal;

class CFileAsyncWriteChunk
{
public:
	const void *m_pMemory;
	int m_MemorySize;
};

class CFileAsyncWriteFillDirOptions
{
public:
	CFileAsyncWriteFillDirOptions()
	{
		m_pDirectory = NULL;
		m_pCalcSizeSearchString = NULL;
		m_pFillFileName = NULL;
		m_pFillData = NULL;
		m_FillDataSize = 0;
		m_FillSize = 0;
		m_PerFileSize = 0;
		m_State = 0;
		m_iCurrentFile = 0;
	}

	virtual ~CFileAsyncWriteFillDirOptions()
	{
	}

	const char *m_pDirectory;
	const char *m_pCalcSizeSearchString;
	const char *m_pFillFileName;
	void *m_pFillData;
	mint m_FillDataSize;
	mint m_FillSize;
	mint m_PerFileSize;

	mint m_State;
	mint m_iCurrentFile;

	virtual void InitFile(mint _Size) pure;
};

class MCCDLLEXPORT CFileAsyncWrite
{
	CFileAsyncWrite_Internal *m_pInternal;


public:

	CFileAsyncWrite();
	~CFileAsyncWrite();
    
	bool FillDirecotry(CFileAsyncWriteFillDirOptions *_pOptions);
	bool SaveFile(const char *_pFile, CFileAsyncWriteChunk *_pChunks, int _nChunks, int _ChunkSize = 16384);
	bool DelFile(const char *_pFile);

	bool Done();
	void Abort(); // Blocks until abort has finished
	void BlockUntilDone();

};

