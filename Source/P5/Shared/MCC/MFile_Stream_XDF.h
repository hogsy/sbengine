
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/



#ifndef __INC_MFile_Stream_XDF
#define __INC_MFile_Stream_XDF


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Substream based stream for use with CCFile
						
	Comments:			Supply an file and a maxleng and let this 
						stream be a placeholder for a subfile inside a 
						file.
\*____________________________________________________________________*/

#include "MFile_XDF.h"

class MCCDLLEXPORT CStream_XDF : public CStream
{
	TPtr<CStream_Disk> m_spFallbackStream;
	CStr m_FileName;
	int64 m_FilePos;
	float m_Priority;
	uint32 m_OpenMode;
	int32 m_iWorkFile;

public:
	
	CStream_XDF()
	{
		m_FilePos = 0;
		m_Priority = 0.0f;
		m_Mode = 0;
	}

	~CStream_XDF()
	{
		
	}

	M_INLINE bool INL_FileExists(CStr _name, int _mode)
	{
		if (_mode & CFILE_WRITE)
			return false;
		
		CXDF *pXDF = CByteStream::XDF_GetUse();

		if (!pXDF)		
			return false;

		CStr NameFixed = _name;
		NameFixed.ReplaceChar('/', '\\');
		NameFixed.MakeLowerCase();

		if (NameFixed.Find(pXDF->m_BasePath) != 0)
			return false;

		CStr Test = NameFixed.Right(NameFixed.Len() - pXDF->m_BasePath.Len());
		
		int iFile = pXDF->GetFile(Test);
		
//		if (iFile < 0)
//			M_TRACEALWAYS("XDF file mismatch %s\n", _name.Str());


		return iFile >= 0;	
	}

	M_INLINE void INL_Open(const CStr _name, int _mode)
	{
		INL_OpenExt(_name, _mode, 0.0f);
	}

	M_INLINE void INL_OpenFallbackFile()
	{
		if  (m_spFallbackStream)
			return;

		m_spFallbackStream = MNew(CStream_Disk);

		m_spFallbackStream->OpenExt(m_FileName, m_OpenMode, m_Priority);
	}

	void INL_OpenExt(const CStr _name, int _mode, float _Priority);

	M_INLINE void INL_Close()
	{
		m_spFallbackStream = NULL;
		m_iWorkFile = 0;
	}

	void ReadImp(void* dest, uint32 _Size);

	M_INLINE void INL_Read(void* dest, uint32 _Size)
	{
		if (m_spFallbackStream)
		{
			Fallback_Read(dest, _Size);
			return;
		}

		CXDF *pXDF = CByteStream::XDF_GetUse();
		if (!pXDF)
			goto LabelFallback;

		while (_Size)
		{
			CXDF::CXDF_Block *pBlock = pXDF->GetWorkBlock();
			if (pBlock && m_FilePos >= pBlock->m_FilePos && (m_FilePos + _Size) <= (pBlock->m_FilePos + pBlock->m_BlockLen) && m_iWorkFile == pBlock->m_iFile)
			{
				fint LocalFilePos = (m_FilePos - pBlock->m_FilePos);
				
				int MaxCopy = pBlock->m_BlockLen - LocalFilePos;
				
				int ToCopy = Min(_Size, (uint32)MaxCopy);
				
				if (!pXDF->Read(pBlock->m_XDFPos + LocalFilePos, dest, ToCopy))
				{
					Fallback_Read(dest, _Size);
					return;
				}
				*((int *)(&dest)) += ToCopy;
				_Size -= ToCopy;
				m_FilePos += ToCopy;
				
				// End of block
				if (ToCopy == MaxCopy)
					pXDF->NextBlock();
			}	
			else
				goto LabelFallback;
		}

		goto LabelEndRead;
LabelFallback:
		ReadImp(dest, _Size);
LabelEndRead:
		return;

	}

	M_INLINE void INL_Write(const void* src, uint32 _Size)
	{
		Error("Write", "Cant write to a XDF stream");
	}

	M_INLINE void INL_Seek(int32 _Pos)
	{
		m_FilePos = _Pos;

		if (m_FilePos > Length())
			Error("Seek", "Seek past end of file");
	}

	M_INLINE void INL_SeekToEnd()
	{
		m_FilePos = Length();
	}

	M_INLINE void INL_RelSeek(int32 _dPos)
	{
		m_FilePos += _dPos;
		if (m_FilePos > Length())
			Error("Seek", "Seek past end of file");
	}

	M_INLINE int32 INL_Pos()
	{
		return m_FilePos;
	}

	M_INLINE int32 INL_Length()
	{
		if (m_spFallbackStream)
			return m_spFallbackStream->Length();

		if (m_iWorkFile >= 0)
		{
			CXDF *pXDF = CByteStream::XDF_GetUse();
			if (pXDF)
			{
				return pXDF->GetFileLen(m_iWorkFile);
			}
		}

		INL_OpenFallbackFile();
		return m_spFallbackStream->Length();
	}

	M_INLINE bool INL_EndOfFile()
	{
		return m_FilePos >= Length();
	}

	
	DECLARE_OPERATOR_NEW
	

	bool FileExists(CStr _name, int _mode);
	void Open(const CStr _name, int _mode);
	void OpenExt(const CStr _name, int _mode, float _Priority = 0, aint _NumCacheLines = -1, aint _CacheLineSize = -1);
	void Fallback_Read(void* dest, mint _Size);
	
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

#endif // __INC_MFile_Stream_XDF
