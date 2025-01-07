
#ifndef DInc_MVideoEncoder_h
#define DInc_MVideoEncoder_h

class CMInstallLog
{
public:

	enum
	{
		ELogEntry_File,
		ELogEntry_RegKey,
		ELogEntry_RegValue,
		ELogEntry_DeleteEmptyDir,
		ELogEntry_DeleteEmptyRegKeys,
		ELogEntry_DeleteDirAsk,
	};

	class CLogEntry
	{
	public:
		uint32 m_Type;
		uint32 m_DataInt;
		CStr m_DataStr0;
		CStr m_DataStr1;

		void Write(CCFile &_File)
		{
            _File.WriteLE(m_Type);
            _File.WriteLE(m_DataInt);
			m_DataStr0.Write(&_File);
			m_DataStr1.Write(&_File);
		}

		void Read(CCFile &_File)
		{
            _File.ReadLE(m_Type);
            _File.ReadLE(m_DataInt);
			m_DataStr0.Read(&_File);
			m_DataStr1.Read(&_File);
		}
	};

	bool PerformUninstall(int _iEntry, CStr &_Item)
	{
		CLogEntry &Entry = m_Entries[_iEntry];
        switch (Entry.m_Type)
		{
		case ELogEntry_File:
			{
				_Item = Entry.m_DataStr0;
				try
				{
					if (CDiskUtil::FileExists(Entry.m_DataStr0))
						return CDiskUtil::DelFile(Entry.m_DataStr0);
				}
				catch (CCException)
				{
					return false;
				}
			}
			break;
		case ELogEntry_RegValue:
			{
				_Item = Entry.m_DataStr0;
				return RemoveRegValue(Entry.m_DataInt, Entry.m_DataStr0, Entry.m_DataStr1);
			}
			break;
		case ELogEntry_RegKey:
			{
				_Item = Entry.m_DataStr0;
				return RemoveRegKey(Entry.m_DataInt, Entry.m_DataStr0);
			}
			break;
		case ELogEntry_DeleteEmptyRegKeys:
			{
				_Item = Entry.m_DataStr0;
				return RemoveRegKeyRecursiveEmpty(Entry.m_DataInt, Entry.m_DataStr0);
			}
			break;
		case ELogEntry_DeleteEmptyDir:
			{
				_Item = Entry.m_DataStr0;
				try
				{
					if (CDiskUtil::DirectoryExists(Entry.m_DataStr0))
						return CDiskUtil::DelTreeOnlyEmpty(Entry.m_DataStr0);
				}
				catch (CCException)
				{
					return false;
				}
			}
			break;
		case ELogEntry_DeleteDirAsk:
			{
				_Item = Entry.m_DataStr0;
				try
				{
					if (CDiskUtil::DirectoryExists(Entry.m_DataStr0))
						CDiskUtil::DelTreeOnlyEmpty(Entry.m_DataStr0);
				}
				catch (CCException)
				{
				}

				if (CDiskUtil::DirectoryExists(Entry.m_DataStr0) && AskYesNo(Entry.m_DataStr1))
				{
					try
					{
						return CDiskUtil::DelTree(Entry.m_DataStr0);
					}
					catch (CCException)
					{
						return false;
					}
				}
				else
					return true;
			}
			break;
			
		}
		return true;
	}

	virtual bool RemoveRegValue(int _Root, CStr _Path, CStr _Value) pure;
	virtual bool RemoveRegKey(int _Root, CStr _Path) pure;
	virtual bool RemoveRegKeyRecursiveEmpty(int _Root, CStr _Path) pure;
	virtual bool AskYesNo(CStr _Message) pure;

	TArray<CLogEntry> m_Entries;

	CCFile m_File;

	void AddDeleteEmptyDirs(CStr _Path);
	void AddDeleteDirsAsk(CStr _Path, CStr _Ask);

	void AddFile(CStr _FileName);

	void AddDeleteEmptyRegKeys(int _Root, CStr _Path);
	void AddRegKey(int _Root, CStr _Path);
	void AddRegValue(int _Root, CStr _Path, CStr _Value);


	void CreateRecord(CStr _FileName);
	void CreateUninstall(CStr _FileName);
	void CloseRecord();
};


class CMInstaller : public CReferenceCount
{
public:

	CStr m_RegistryRoot;
	CStr m_InstallerName;
	CStr m_ProgramFilesRoot;
	CStr m_StartMenuRoot;

/*	class CLanguageEntry
	{
	public:
		CStr m_Language;
		CStr m_Text;
		void Write(CCFile &_File);
		void Read(CCFile &_File);
	};

	class CLanguage
	{
	public:
		TArray<CLanguageEntry> m_Entries;
		void Write(CCFile &_File);
		void Read(CCFile &_File);

		CStr GetLocalized(CStr _Language)
		{
			for (int i = 0; i < m_Entries.Len(); ++i)
			{
				if (m_Entries[i].m_Language.CompareNoCase(_Language) == 0)
					return m_Entries[i].m_Text;
			}
			return "";
		}
	};*/

	class CComponent
	{
	public:

		CComponent()
		{
			m_CheckState = 0;
			m_Visable = 0;
			m_NeededSpace = 0;
		}

		uint32 m_CheckState;
		uint32 m_Visable;
		CStr m_FileArchive;
		uint64 m_NeededSpace;

		CStr m_DisplayName;

		class CShortCut
		{
		public:
			CStr m_Target;
			CStr m_Location;
			CStr m_Parameters;

			CStr m_DisplayName;
			int32 m_iIconPos;

//			CStr GetDisplayName(CStr _Language)
//			{
//				return m_DisplayName.GetLocalized(_Language);
//			}

			void Write(CCFile &_File);
			void Read(CCFile &_File);
		};

		TArray<CShortCut> m_ShortCuts;

		enum
		{
			ERegEntryType_Key,
			ERegEntryType_Value
		};
		enum
		{
			ERegEntryValueType_Uint32,
			ERegEntryValueType_Str
		};
		class CRegEntry
		{
		public:
			uint32 m_Type;
			CStr m_Location;
			CStr m_Path;

			CStr m_ValueName;
			uint32 m_ValueType;
			CStr m_ValueStr;
			uint32 m_ValueUint32;

			void Write(CCFile &_File);
			void Read(CCFile &_File);
		};

		TArray<CRegEntry> m_RegEntries;

		class CAskExecute
		{
		public:
			CStr m_Question;
			CStr m_Execute;

			void Write(CCFile &_File);
			void Read(CCFile &_File);
		};

		TArray<CAskExecute> m_AskExecute;

//		CStr GetDisplayName(CStr _Language)
//		{
//			return m_DisplayName.GetLocalized(_Language);
//		}

		void Write(CCFile &_File);
		void Read(CCFile &_File);
	};


	TArray<CComponent> m_Components;

	void CreateInstaller(CStr _SourcePath, CRegistry *_pReg);

	void Write(CCFile &_File);
	void Write(CStr _File);

	void Read(CCFile &_File);
	void Read(CStr _File);

	class CMInstallerWorker *m_pWorker;
	CMInstallLog *m_pInstallLog;

	virtual ~CMInstaller();
	CMInstaller()
	{
		m_pInstallLog = NULL;
		m_pWorker = NULL;
	}

	void StartInstall(CStr _Destination, CStr _SourcePath, CMInstallLog *_pInstallLog);
	void StartUninstall(CMInstallLog *_pInstallLog);
	void CancelInstall();
	fp32 GetInstallProgress();
	CStr GetInstallLastFile();
	int GetInstallDone();

	enum
	{
		EMessageType_NoMessage,
		EMessageType_Error,
		EMessageType_ErrorCancel,
		EMessageType_QuestionYesCancel,
		EMessageType_QuestionYesNo,
		EMessageType_Information,
	};
	int GetLastMessage(CStr &_Message);
	int GetCurrentMedia();
	void MessageAnswer(int _Answer);
	CStr GetInstallLastFail();

	virtual bool CreateShortcut(CStr _Name, CStr _Target, CStr _Location, CStr _Parameters, int _iIconPos)
	{
		return true;
	}

	virtual bool CreateRegistryKey(CStr _Location, CStr _Path)
	{
		return true;
	}

	virtual bool SetRegistryKeyValue(CStr _Location, CStr _Path, CStr _ValueName, uint32 _Value)
	{
		return true;
	}

	virtual bool SetRegistryKeyValue(CStr _Location, CStr _Path, CStr _ValueName, CStr _Value)
	{
		return true;
	}

	virtual bool Execute(CStr _Path)
	{
		return true;
	}

	virtual uint32 GetNamedValue(CStr _Value)
	{
		return 0;
	}

	

};

#endif //DInc_MVideoEncoder_h