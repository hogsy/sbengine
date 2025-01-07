
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/


#ifndef __MFILE_XDF_INCLUDED
#define __MFILE_XDF_INCLUDED

#include "MDA_Hash.h"


class MCCDLLEXPORT CXDF : public MRTC_Thread
{
protected:
	const char* Thread_GetName() const
	{
		return "XDF Unpacker";
	}

public:

	enum
	{
		EXDFVersion = 0x0101
	};

	CStr m_File;
	CStr m_BasePath;
//	CStream_Disk m_Stream;
	CStr m_CurrentFile;
	int32 m_iCurrentBlock;
	int32 m_iCurrentFile;
	int32 m_iWorkFile;
	int32 m_iWorkBlock;
	int32 m_CurrentPos;
	bool m_bCreateNewBlock;
	bool m_bRecording;
	int32 m_Pause;
	int32 m_iDataStart;

	CStream_Disk m_FileStream;

	class CXDF_File
	{
	public:
		uint32 m_iFileName;
		uint32 m_iFirstBlock;
		uint32 m_iLastBlock;
		uint32 m_FileLength;
		uint64 m_FileDate; // 100 nanosecond ticks since 1901

		void Write(CCFile *_pFile, CXDF *_pXDF);
		void Read(CCFile *_pFile, CXDF *_pXDF);
	};

	class CXDF_Block
	{
	public:
		int32 m_iNextBlock;
		int32 m_iFile;
		int32 m_BlockLen;
		int32 m_FilePos;
		int32 m_XDFPos;

		void Write(CCFile *_pFile);
		void Read(CCFile *_pFile);
	};

	void CreateHash();
	int GetFileCreate(const char *_pFile);

	CStringHashConst m_NameHash;
	TThinArray<char> m_lNameHeap;
	TThinArray<CXDF_Block> m_lBlocks;
	TThinArray<CXDF_File> m_lFiles;

	CXDF();
	~CXDF();

	void Pause()
	{
		++m_Pause;
	}
	void Resume()
	{
		--m_Pause;
		M_ASSERT(m_Pause >= 0, "");
	}

	void CompileFile(const char *_pFileSource, const char *_pFileDest, const char *_pFilesBasePath);

	void Read(CCFile *_pFile);
	void Write(CCFile *_pFile);
	
	void Open(const char *_pFileSource, const char *_pFilesBasePath);

	void Record_FinishBlock();
	void Record_NewBlock();
	void Record_Start(const char *_pFile, const char *_pBasePath);

	void Record_Seek(int64 _ToByte);
	void Record_Read(int64 _ToByte);
	void Record_OpenFile(const char *_pFile);
	void Record_CloseFile(const char *_pFile);	

	void EnumFiles(TArray_Sortable<CDir_FileRec> &_Dest, CStr _Path);	
	

	M_INLINE int GetFile(const char *_pFile)
	{
		return m_NameHash.GetIndex(_pFile);
	}

	M_INLINE CXDF::CXDF_Block *GetBlock(int _iBlock)
	{
		return &m_lBlocks[_iBlock];
	}

	M_INLINE CXDF::CXDF_File *GetFile(int _Index)
	{
		return &m_lFiles[_Index];
	}

	M_INLINE int32 GetFileLen(int _iFile)
	{
		return m_lFiles[_iFile].m_FileLength;
	}

	M_INLINE const char *GetFileName(int _Index)
	{
		return &m_lNameHeap[_Index];
	}

	M_INLINE CXDF::CXDF_Block *GetWorkBlock()
	{
		if (m_iWorkBlock < 0 || m_iWorkBlock > m_iCurrentBlock)
			return NULL;

		return &m_lBlocks[m_iWorkBlock];
	}

	M_INLINE void NextBlock()
	{
		++m_iWorkBlock;
	}


	NThread::CMutual m_UnpackLock;
	class CUnpackBuffer
	{
	public:
		TThinArray<uint8> m_Data;
		mint m_DataLen;
		mint m_DataPos;
		DLinkD_Link(CUnpackBuffer, m_Link);
	};

	DLinkD_List(CUnpackBuffer, m_Link) m_UnpackFree;
	DLinkD_List(CUnpackBuffer, m_Link) m_UnpackDone;

	NThread::CEventAutoResetReportable m_EventFree;
	NThread::CEventAutoResetReportable m_EventDone;
	fint m_XDFPos;

	CUnpackBuffer *m_pCurrentBuffer;

	bint Read_NotInline(fint _Pos, void *_pData, mint _Len);
	M_INLINE bint Read(fint _Pos, void *_pData, mint _Len)
	{
		uint8 *pData = (uint8 *)_pData;
		if (m_pCurrentBuffer)
		{
			fint ToSeek = _Pos - m_XDFPos;
			if (!ToSeek)
			{
				CUnpackBuffer *pBuffer = m_pCurrentBuffer;
				mint MaxLen = pBuffer->m_DataLen - pBuffer->m_DataPos;
				mint ToRead = Min(MaxLen, _Len);
				memcpy(pData, pBuffer->m_Data.GetBasePtr() + pBuffer->m_DataPos, ToRead);

				pData += ToRead;
				_Pos += ToRead;
				_Len -= ToRead;
				pBuffer->m_DataPos += ToRead;
				m_XDFPos += ToRead;

				if (pBuffer->m_DataPos == pBuffer->m_DataLen)
				{
					M_LOCK(m_UnpackLock);
					m_UnpackFree.Insert(pBuffer);
					m_EventFree.Signal();
					m_pCurrentBuffer = NULL;
				}
				if (!_Len)
					return true; // Done
			}
		}
		return Read_NotInline(_Pos, pData, _Len);
	}
	int Thread_Main();


};

#endif // __MFILE_XDF_INCLUDED
