
#ifndef INC_MFileContainer_h
#define INC_MFileContainer_h
class CMFileContainer
{
public:

	class CEntry
	{
	public:
		CStr m_Description;
        fint m_FileOffset;
        fint m_FileLen;
		
		int Compare(const CEntry &_Other) const
		{
			return CStrBase::stricmp(m_Description,_Other.m_Description);
		}

		bool operator < (const CEntry& _Other) const
		{
			return CStrBase::stricmp(m_Description,_Other.m_Description) < 0;
		}

		bool operator == (const CEntry& _Other) const
		{
			return CStrBase::stricmp(m_Description,_Other.m_Description) == 0;
		}
	};

	TArray_Sortable<CEntry> m_Entries;
	spCCFile m_spFile;
	CStr m_FileName;
	int m_bFileOpen;

	CMFileContainer();
	void Create (const char *_pFile, bool _bLoadEntries = false);
	void Clear();
	void CloseFile();
	void OpenFile(bool _bWrite);
	void AddFile(void *_pFile, fint _FileLen, const char *_pFileName);
	void Write();
	CEntry *GetEntry(const char *_pDesc);
	void GetFile(CCFile &_File, const char *_pDesc);
};
#endif // INC_MFileContainer_h
