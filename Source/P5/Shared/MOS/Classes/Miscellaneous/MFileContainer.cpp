#include "PCH.h"
#include "MFileContainer.h"


static void ReplaceChars(char* _pStr, char _Find, char _Replace)
{
	if (_pStr) 
		for (; *_pStr; _pStr++)
			if (*_pStr == _Find)
				*_pStr = _Replace;
}



CMFileContainer::CMFileContainer()
{
	m_bFileOpen = false;
}

void CMFileContainer::Clear()
{
	CloseFile();
	m_Entries.Destroy();
	m_FileName.Clear();
}

void CMFileContainer::Create(const char *_pFile, bool _bLoadEntries)
{
	if (m_FileName.CompareNoCase(_pFile))
	{
		m_FileName = _pFile;

		if (_bLoadEntries)
		{
			if (CDiskUtil::FileExists(m_FileName))
			{
				M_TRY
				{
					OpenFile(false);

					m_spFile->SeekToEnd();
					m_spFile->RelSeek(-4);
					uint32 Offset;
					m_spFile->ReadLE(Offset);
					m_spFile->Seek(Offset);

					uint32 Len;
					m_spFile->ReadLE(Len);

					m_Entries.SetLen(Len);

					for (int i = 0; i < m_Entries.Len(); ++i)
					{
						CEntry& Entry = m_Entries[i];
						m_spFile->ReadLE(Entry.m_FileOffset);
						m_spFile->ReadLE(Entry.m_FileLen);
						Entry.m_Description.Read(m_spFile);
						ReplaceChars(Entry.m_Description.GetStr(), '\\', '/');
					}
				}
				M_CATCH(
				catch (CCException)
				{
					throw;
				}
				)
			}
		}
	}		
}


void CMFileContainer::CloseFile()
{
	if (m_bFileOpen)
	{
		m_spFile = NULL;
		m_bFileOpen = false;
	}

}

void CMFileContainer::OpenFile(bool _bWrite)
{
	if (!m_bFileOpen)
	{
		m_bFileOpen = true;

		m_spFile = MNew(CCFile);

		if (_bWrite)
		{
			m_spFile->Open(m_FileName, CFILE_WRITE|CFILE_TRUNC|CFILE_BINARY);
		}
		else
			m_spFile->Open(m_FileName, CFILE_READ|CFILE_BINARY);
	}
}

void CMFileContainer::AddFile(void *_pFile, fint _FileLen, const char *_pFileName)
{
	OpenFile(true);

	CEntry Entry;

	Entry.m_Description = _pFileName;
	ReplaceChars(Entry.m_Description.GetStr(), '\\', '/');

	Entry.m_FileOffset = m_spFile->Pos();
	Entry.m_FileLen = _FileLen;
	m_Entries.Add(Entry);

	m_spFile->Write(_pFile, _FileLen);
}

void CMFileContainer::Write()
{
	OpenFile(true);
	m_Entries.Sort();
	uint32 Offset = m_spFile->Pos();
	uint32 Len = m_Entries.Len();
	m_spFile->WriteLE(Len);
	for (int i = 0; i < m_Entries.Len(); ++i)
	{
		m_spFile->WriteLE(m_Entries[i].m_FileOffset);
		m_spFile->WriteLE(m_Entries[i].m_FileLen);
		m_Entries[i].m_Description.Write(m_spFile);
	}
	m_spFile->WriteLE(Offset);
}

CMFileContainer::CEntry* CMFileContainer::GetEntry(const char* _pDesc)
{
	CFStr Desc = _pDesc;
	ReplaceChars(Desc.GetStr(), '\\', '/');
	_pDesc = Desc.Str();

    // Do a binary search (list is sorted)
	int Min = 0;
	int Max = m_Entries.Len()-1;
	int CurrentIndex = Max >> 1;

	while (Min <= Max)
	{
		M_ASSERT(CurrentIndex >= 0 && CurrentIndex < m_Entries.Len(), "");
		CEntry* pEntry = &(m_Entries[CurrentIndex]);

		int Compare = CStrBase::stricmp(pEntry->m_Description, _pDesc);
		if (Compare < 0)
		{
			if (Min == CurrentIndex)
				break;
			Min = CurrentIndex;
			CurrentIndex = ((Max + 1 - Min) >> 1) + Min;
		}
		else if (Compare > 0)
		{
			if (Max == CurrentIndex)
				break;
			Max = CurrentIndex;
			CurrentIndex = ((Max - Min) >> 1) + Min;
		}
		else
		{
			return &(m_Entries[CurrentIndex]);
		}
	}
	return NULL;
}

void CMFileContainer::GetFile(CCFile &_File, const char *_pDesc)
{
	OpenFile(false);

	CEntry *pEntry = GetEntry(_pDesc);
	if (!pEntry)
		Error_static("CMFileContainer::GetFile", "File not found");

	CStream_SubFile * NewFile = MNew(CStream_SubFile);

	NewFile->Open(m_spFile, pEntry->m_FileOffset, pEntry->m_FileLen);

	_File.Open(NewFile, CFILE_READ|CFILE_BINARY, true);
}

