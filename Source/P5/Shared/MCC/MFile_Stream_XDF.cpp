
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/

#include "PCH.h"
#include "MFile.h"


#include "MFile_XDF.h"
#include "MFile_Stream_XDF.h"
#include "MFile_Stream_Disk.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CStream_XDF
|__________________________________________________________________________________________________
\*************************************************************************************************/

IMPLEMENT_OPERATOR_NEW(CStream_XDF);


bool CStream_XDF::FileExists(CStr _name, int _mode)
{
	return INL_FileExists(_name, _mode);
}

void CStream_XDF::Open(const CStr _name, int _mode)
{
	INL_OpenExt(_name, _mode, 0.0f);
}

void CStream_XDF::OpenExt(const CStr _name, int _mode, float _Priority, aint _NumCacheLines, aint _CacheLineSize)
{
	INL_OpenExt(_name, _mode, _Priority);
}

void CStream_XDF::Close()
{
	INL_Close();
}

void CStream_XDF::Read(void* dest, mint _Size)
{
	INL_Read(dest, _Size);
}

void CStream_XDF::Write(const void* src, mint _Size)
{
	Error("Write", "Cant write to a XDF stream");
}

void CStream_XDF::Seek(fint _Pos)
{
	INL_Seek(_Pos);
}

void CStream_XDF::SeekToEnd()
{
	INL_SeekToEnd();
}


void CStream_XDF::RelSeek(fint _dPos)
{
	INL_RelSeek(_dPos);
}

fint CStream_XDF::Pos()
{
	return INL_Pos();
}

fint CStream_XDF::Length()
{
	return INL_Length();
}

bool CStream_XDF::EndOfFile()
{
	return INL_EndOfFile();
}

void CStream_XDF::Fallback_Read(void* dest, mint _Size)
{
	INL_OpenFallbackFile();
	m_spFallbackStream->Seek(m_FilePos);
	m_spFallbackStream->Read(dest, _Size);
	m_FilePos += _Size;
}

#ifdef PLATFORM_XENON
extern __declspec(thread) int g_IgnoreFileOpen;
#endif

void CStream_XDF::INL_OpenExt(const CStr _name, int _mode, float _Priority)
{
	CXDF *pXDF = CByteStream::XDF_GetUse();

	m_Priority = _Priority;
	m_OpenMode = _mode;
	m_Mode = _mode;

	m_FilePos = 0;
	CStr NameFixed = _name;
	NameFixed.ReplaceChar('/', '\\');
	NameFixed.MakeLowerCase();
	m_FileName = NameFixed;
	m_iWorkFile = -1;
	if (pXDF)
	{
		if (NameFixed.CompareSubStr(pXDF->m_BasePath) == 0)
		{
			CStr Test = NameFixed.Right(NameFixed.Len() - pXDF->m_BasePath.Len());
			// First try to find the file int the current block;
			CXDF::CXDF_Block *pBlock = pXDF->GetWorkBlock();

			if (pBlock)
			{
				m_iWorkFile = pBlock->m_iFile;
				CXDF::CXDF_File *pFile = pXDF->GetFile(pBlock->m_iFile);
				if (pFile->m_FileDate)
				{
					const char *pFileName = pXDF->GetFileName(pFile->m_iFileName);
					if (Test.CompareNoCase(pFileName) == 0)
					{
						// The right file, we assuma that everything else is peachy until otherwise is porven
						return;
					}
				}

			}

			m_iWorkFile = pXDF->GetFile(Test);

			if (m_iWorkFile >= 0)
			{
				CXDF::CXDF_File *pFile = pXDF->GetFile(m_iWorkFile);
				if (!pFile->m_FileDate)
				{
	#ifdef PLATFORM_XENON
					++g_IgnoreFileOpen;
	#endif
					INL_OpenFallbackFile(); // Fallback because file is older
	#ifdef PLATFORM_XENON
					--g_IgnoreFileOpen;
	#endif
				}
				return;
			}
		}
	}

#ifdef PLATFORM_XENON
//				++g_IgnoreFileOpen;
#endif
	INL_OpenFallbackFile();
#ifdef PLATFORM_XENON
//				--g_IgnoreFileOpen;
#endif
}
void CStream_XDF::ReadImp(void* dest, uint32 _Size)
{

	CXDF *pXDF = CByteStream::XDF_GetUse();
	int iSaveBlock = pXDF->m_iWorkBlock;
	while (_Size)
	{
		bool DoneSomething = false;
		
		if (pXDF)
		{
			// Loop through all blocks
			CXDF::CXDF_Block *pStartBlock = pXDF->GetWorkBlock();
			int iSearch = 0;
			while (1)
			{
				CXDF::CXDF_Block *pBlock = pXDF->GetWorkBlock();
				if (!pBlock)
					break;
				if (m_FilePos >= pBlock->m_FilePos && m_FilePos < (pBlock->m_FilePos + pBlock->m_BlockLen) && m_iWorkFile == pBlock->m_iFile)
				{
					DoneSomething = true;
					fint BlockPos = (m_FilePos - pBlock->m_FilePos);
					fint NewPos = pBlock->m_XDFPos + BlockPos;
#ifdef M_Profile
					fint OldPos = pXDF->m_XDFPos;
					if (iSearch > 0)
						M_TRACEALWAYS("XDF(%s): Searched %d blocks forward to find matching block\n", m_FileName.Str(), iSearch);
					if (OldPos != NewPos)
					{
						M_TRACEALWAYS("XDF(%s): Seeking in stream from 0x%08x to 0x%08x (%d bytes) \n", m_FileName.Str(), OldPos, NewPos, NewPos - OldPos);
						if (pStartBlock)
						{
							int32 Pos = OldPos - pStartBlock->m_XDFPos;
							M_TRACEALWAYS("XDF(%s): Previous block 0x%08x -> 0x%08x (%d bytes)\n", pXDF->GetFileName(pXDF->GetFile(pStartBlock->m_iFile)->m_iFileName), pStartBlock->m_FilePos + Pos, pStartBlock->m_FilePos + pStartBlock->m_BlockLen, pStartBlock->m_BlockLen - Pos);
							
						}
					}
#endif
					
					int MaxCopy = pBlock->m_BlockLen - BlockPos;
					
					int ToCopy = Min(_Size, (uint32)MaxCopy);
					
					if (!pXDF->Read(NewPos, dest, ToCopy))
					{
						Fallback_Read(dest, _Size);
						return;
					}
					iSaveBlock = pXDF->m_iWorkBlock;
					*((int *)(&dest)) += ToCopy;
					_Size -= ToCopy;
					m_FilePos += ToCopy;
					
					// End of block
					if (ToCopy == MaxCopy)
					{
						pXDF->NextBlock();
						iSaveBlock = pXDF->m_iWorkBlock;
					}
					
					break;				
				}
				pXDF->NextBlock();
				++iSearch;
			}
			
			// Now go through all blocks in file to try to find the right block
			/*if (!DoneSomething)
			{
				CXDF::CXDF_File *pFile = pXDF->GetFile(m_iWorkFile);
				int CurrentBlock = pFile->m_iFirstBlock;
				int NumSearched = 0;
				while (CurrentBlock >= 0)
				{
					++NumSearched;
					CXDF::CXDF_Block *pBlock = pXDF->GetBlock(CurrentBlock);
					
					// Found block fitting
					if (m_FilePos >= pBlock->m_FilePos && m_FilePos < (pBlock->m_FilePos + pBlock->m_BlockLen))
					{
						M_TRACEALWAYS("XDF(%s): Restarted search from begining and searched %d blocks to find matching block\n", m_FileName.Str(), NumSearched);
						pXDF->m_iWorkBlock = CurrentBlock;
						DoneSomething = true;
						break;
					}
					
					CurrentBlock = pBlock->m_iNextBlock;
				}
				if (!DoneSomething)
				{
					M_TRACEALWAYS("XDF(%s): Restarted search from begining and searched all blocks(%d) and  found nothing\n", m_FileName.Str(), NumSearched);
				}
			}*/
		}	
		
		if (!DoneSomething)
		{
			M_TRACEALWAYS("Falling back XDF file %s, Pos %08x\n", m_FileName.Str(), m_FilePos);
			pXDF->m_iWorkBlock = iSaveBlock; 
			Fallback_Read(dest, _Size);
			return;
		}
	}
}
