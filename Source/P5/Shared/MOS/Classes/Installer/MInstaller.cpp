
#include "PCH.h"
#include "MInstaller.h"
#include "../Miscellaneous/MMd5.h"
#include "../../../../sdk/ZLib/zlib.h"

class CMInstallerCompiler
{
public:
	CMInstallerCompiler()
	{
		m_MediaSpaceLeft = 0;
		m_iCurrentMedia = 0;
	}
	fint m_MediaSize;
	CStr m_Destination;
	CStr m_SourceRoot;

	fint m_MediaSpaceLeft;
	uint32 m_iCurrentMedia;

	void SetMediaSize(fint _MediaSize)
	{
		m_MediaSpaceLeft = m_MediaSize = _MediaSize;
	}

	class CArchiveWrite
	{
	public:

		CMInstallerCompiler *m_pCompiler;

		CCFile m_File;

		CStr m_FirstFile;
		CStr m_FileName;

		z_stream m_Compressor; /* compression stream */

		uint32 m_NumFiles;
		uint32 m_iFile;

		bint m_bInit;

		TThinArray<uint8> m_InData;
		TThinArray<uint8> m_OutData;

		CArchiveWrite(CMInstallerCompiler *_pCompiler, CStr _File)
		{
			m_pCompiler = _pCompiler;

			m_NumFiles = 0;
			m_FileName = _File;
			m_FirstFile = m_pCompiler->GetDestination() + m_FileName;
			CDiskUtil::CreatePath(m_FirstFile.GetPath());
			m_File.Open(m_FirstFile, CFILE_BINARY | CFILE_WRITE | CFILE_TRUNC);
			m_File.Write("XCAR", 4);
			m_iFile = 0;
			m_File.WriteLE(m_iFile);
			m_File.WriteLE(m_NumFiles);
			m_pCompiler->UsedSpace(12);

			m_OutData.SetLen(32768);
			m_InData.SetLen(32768);
			
			m_Compressor.zalloc = (alloc_func)0;
			m_Compressor.zfree = (free_func)0;
			m_Compressor.opaque = (voidpf)0;
			memset(&m_Compressor, 0, sizeof(m_Compressor));
			int err;
#ifdef _DEBUG
			err = deflateInit(&m_Compressor, Z_BEST_SPEED);
#else
			err = deflateInit(&m_Compressor, Z_BEST_COMPRESSION);
#endif

			if (err != Z_OK)
				Error_static("CArchiveWrite::Create", CStrF("deflateInit returned %d", err));

			m_bInit = true;
		}

		~CArchiveWrite()
		{
			Destroy();
		}

		void Destroy()
		{
			if (m_bInit)
			{
				// Flush stream

				int err = Z_OK;
				while (err != Z_STREAM_END) 
				{
					err = deflate(&m_Compressor, Z_FINISH);
					if (m_Compressor.next_out)
					{
						mint Len = m_OutData.Len() - m_Compressor.avail_out;
						uint8 *pData = m_OutData.GetBasePtr();
						while (Len)
						{
							int MaxSpace = MinMT(SpaceLeft(), Len);
							m_File.Write(pData, MaxSpace);
							pData += MaxSpace;
							Len -= MaxSpace;
							ConsumeSpace(MaxSpace);

							MakeSpace(1);
						}
					}
					m_Compressor.next_out = m_OutData.GetBasePtr();
					m_Compressor.avail_out = m_OutData.Len();
				}
				err = deflateEnd(&m_Compressor);

				if (err != Z_OK)
					LogFile(CStrF("ERROR: deflateEnd returned %d", err));
				m_bInit = false;

				m_File.Close();

				m_File.Open(m_FirstFile, CFILE_BINARY | CFILE_WRITE | CFILE_READ);
				m_File.Seek(8);
				m_File.WriteLE(m_NumFiles);				
				m_File.Close();
			}
			else
			{
				m_File.Close();
			}
		}
		void MakeSpace(int _Space)
		{
			if (m_pCompiler->m_MediaSpaceLeft < _Space)
			{
				m_pCompiler->NextMedia();
				m_File.Close();
				m_pCompiler->WriteSignature();
				CStr FileName = m_pCompiler->GetDestination() + m_FileName;
				CDiskUtil::CreatePath(FileName.GetPath());
				m_File.Open(FileName, CFILE_BINARY | CFILE_WRITE);
				m_File.Write("XCAR", 4);
				++m_iFile;
				m_File.WriteLE(m_iFile);
				m_pCompiler->UsedSpace(8);
			}
		}

		fint SpaceLeft()
		{
			return m_pCompiler->m_MediaSpaceLeft;
		}

		void ConsumeSpace(fint _Space)
		{
			m_pCompiler->UsedSpace(_Space);
		}

		void AddData(const void *_pData, mint _DataLen)
		{
			mint Left = _DataLen;
			m_Compressor.next_in = (Bytef *)_pData;
			m_Compressor.avail_in = Left;

			while (m_Compressor.avail_in)
			{
				if (!m_Compressor.avail_out)
				{
					if (m_Compressor.next_out)
					{
						mint Len = m_OutData.Len();
						uint8 *pData = m_OutData.GetBasePtr();
						while (Len)
						{
							int MaxSpace = MinMT(SpaceLeft(), Len);
							m_File.Write(pData, MaxSpace);
							pData += MaxSpace;
							Len -= MaxSpace;
							ConsumeSpace(MaxSpace);

							MakeSpace(1);
						}
					}
					m_Compressor.next_out = m_OutData.GetBasePtr();
					m_Compressor.avail_out = m_OutData.Len();
				}
				int err = deflate(&m_Compressor, Z_NO_FLUSH);

				if (err != Z_OK && err != Z_BUF_ERROR)
					Error_static("CArchiveWrite::AddData", "deflate returned error");
			}
		}

		void AddFile(CStr _SourceFile, CStr _Name)
		{
			CCFile TempFile;
			CStream_Memory MemoryStream;
			TArray<uint8> Memory;
			MemoryStream.Open(Memory, CFILE_WRITE | CFILE_BINARY);
			TempFile.Open(&MemoryStream, CFILE_WRITE | CFILE_BINARY);

			++m_NumFiles;
			
			uint64 FileSize = CDiskUtil::GetFileSize(_SourceFile);

			m_pCompiler->m_nCurrentBytes += FileSize;

			uint32 HeaderSize = 0;
			CFileInfo Info = CDiskUtil::FileTimeGet(_SourceFile);
			CMD5Digest Digest = GetFileMD5(_SourceFile);
			TempFile.WriteLE(HeaderSize);
			_Name.Write(&TempFile);
			TempFile.WriteLE(FileSize);
			TempFile.Write(Digest.f_GetData(), 16);			
			TempFile.WriteLE(Info.m_TimeWrite);
			

			HeaderSize = TempFile.Pos();
			TempFile.Seek(0);
			TempFile.WriteLE(HeaderSize);
			TempFile.Close();

			AddData(Memory.GetBasePtr(), Memory.Len());
			
			CCFile File;
			File.Open(_SourceFile, CFILE_READ | CFILE_BINARY);

            while (FileSize)
			{
				mint ToRead = MinMT(m_InData.Len(), FileSize);

				File.Read(m_InData.GetBasePtr(), ToRead);

				AddData(m_InData.GetBasePtr(), ToRead);
				FileSize -= ToRead;
			}
		}

	#if 0

		c_stream.next_in = uncompr;
		c_stream.avail_in = (uInt)uncomprLen;
		err = deflate(&c_stream, Z_NO_FLUSH);
		CHECK_ERR(err, "deflate");
		if (c_stream.avail_in != 0) {
			fprintf(stderr, "deflate not greedy\n");
			exit(1);
		}

		/* Feed in already compressed data and switch to no compression: */
		deflateParams(&c_stream, Z_NO_COMPRESSION, Z_DEFAULT_STRATEGY);
		c_stream.next_in = compr;
		c_stream.avail_in = (uInt)comprLen/2;
		err = deflate(&c_stream, Z_NO_FLUSH);
		CHECK_ERR(err, "deflate");

		/* Switch back to compressing mode: */
		deflateParams(&c_stream, Z_BEST_COMPRESSION, Z_FILTERED);
		c_stream.next_in = uncompr;
		c_stream.avail_in = (uInt)uncomprLen;
		err = deflate(&c_stream, Z_NO_FLUSH);
		CHECK_ERR(err, "deflate");

		}
	#endif

	};

	void GetFileList_r(CStr _Path, TArray<CStr> &_lFiles, bool _bRecursive)
	{
		_Path.Trim();

		if (_Path == "")
			return;

		// Read files, include wildcard
		{
			CDirectoryNode Dir;
			Dir.ReadDirectory(_Path);

			int nf = Dir.GetFileCount();
			for(int i = 0; i < nf; i++)
			{
				CDir_FileRec* pF = Dir.GetFileRec(i);
				if (Dir.IsDirectory(i))
				{
				}
				else
				{
					// LogFile("Adding file " + _Path.GetPath() + pF->m_Name);
					_lFiles.Add(_Path.GetPath() + pF->m_Name);
				}
			}
		}

		// Read files, remove wildcard
		{
			CDirectoryNode Dir;
			Dir.ReadDirectory(_Path.GetPath() + "*");

			int nf = Dir.GetFileCount();
			for(int i = 0; i < nf; i++)
			{
				CDir_FileRec* pF = Dir.GetFileRec(i);
				if (Dir.IsDirectory(i))
				{
					if (pF->m_Name == "." || pF->m_Name == "..") continue;
					if (_bRecursive)
						GetFileList_r(_Path.GetPath() + Dir.GetFileName(i) + "\\" + _Path.GetFilename(), _lFiles, _bRecursive);
				}
				else
				{
				}
			}
		}
	}

	void WriteSignature()
	{
		CCFile Temp;
		CStr Name = GetDestination() + "Disk.id";
		CDiskUtil::CreatePath(Name.GetPath());
		Temp.Open(Name, CFILE_WRITE | CFILE_BINARY);
		Temp.WriteLE(m_iCurrentMedia);
		UsedSpace(4);
	}
	void CleanDestination()
	{
		CDiskUtil::DelTree(m_Destination);
	}

	class CSourceDest
	{
	public:
		CStr m_Source;
		CStr m_Dest;
	};

	void GetFiles(CRegistry *_pFiles, TArray<CSourceDest> &_Files)
	{
		for (int i = 0; i < _pFiles->GetNumChildren(); ++i)
		{
			CStr Temp = _pFiles->GetChild(i)->GetThisValue();
			CStr File = Temp.GetStrSep(";");
			int FileDestStrip = Temp.GetStrSep(";").Len() + 1;

			CStr Path = m_SourceRoot + File.CopyFrom(1);
			int SourceLen = m_SourceRoot.Len();
			TArray<CStr> NewFiles;
			GetFileList_r(Path, NewFiles, true);

			if (File[0] == '+')
			{
				for (int j = 0; j < NewFiles.Len(); ++j)
				{
					CStr File = NewFiles[j].CopyFrom(SourceLen);
					bool bFound = false;
					for (int k = 0; k < _Files.Len(); ++k)
					{
						if (_Files[k].m_Source == File)
						{
							bFound = true;
							break;
						}
					}
					if (!bFound)
					{
						CSourceDest New;
						New.m_Source = File;
						New.m_Dest = File.CopyFrom(FileDestStrip);
                        _Files.Add(New);
					}
				}
			}
			else if (File[0] == '-')
			{
				for (int j = 0; j < NewFiles.Len(); ++j)
				{
					CStr File = NewFiles[j].CopyFrom(SourceLen);
					for (int k = 0; k < _Files.Len(); ++k)
					{
						if (_Files[k].m_Source == File)
						{
							_Files.Del(k);
							break;
						}
					}
				}
			}
		}

	}

	CStr GetDestination()
	{
		return m_Destination + CStrF("Media%02d\\", m_iCurrentMedia);
	}

	void UsedSpace(fint _Space)
	{
		m_MediaSpaceLeft -= _Space;

		if (m_MediaSpaceLeft < 0)
		{
			Error_static("UsedSpace", "Exceeded media size");
		}
	}

	void NextMedia()
	{
		++m_iCurrentMedia;
		m_MediaSpaceLeft = m_MediaSize;
	}

	void CopyToMedia(CRegistry *_pFiles)
	{
		TArray<CSourceDest> Files;
		GetFiles(_pFiles, Files);

		for (int i = 0; i < Files.Len(); ++i)
		{
			LogFile(CStrF("Copying: %s to %s", Files[i].m_Source.Str(), Files[i].m_Dest.Str()));
			CStr Source = m_SourceRoot + Files[i].m_Source;
			fint Size = CDiskUtil::GetFileSize(Source);
			if (Size > m_MediaSpaceLeft)
			{
				NextMedia();
				if (Size > m_MediaSpaceLeft)
					Error_static("CopyToMedia", "File does not fit on one media");
			}
			m_MediaSpaceLeft -= Size;
			CStr Dest = GetDestination() + Files[i].m_Dest;
			CDiskUtil::CreatePath(Dest.GetPath());
			CDiskUtil::CpyFile(Source, Dest, 128*1024);
		}
	}

	uint64 m_nCurrentBytes;
	uint64 GenerateArchive(CStr _DestFile, CRegistry *_pFiles)
	{
		TArray<CSourceDest> Files;
		GetFiles(_pFiles, Files);

		m_nCurrentBytes = 0;

		CArchiveWrite Archive(this, _DestFile);

		for (int i = 0; i < Files.Len(); ++i)
		{
			LogFile(CStrF("Adding: %s as %s", Files[i].m_Source.Str(), Files[i].m_Dest.Str()));
			Archive.AddFile(m_SourceRoot + Files[i].m_Source, Files[i].m_Dest);
		}

		LogFile(CStrF("Archive size: %d", m_nCurrentBytes));
		return m_nCurrentBytes;
	}
};

CStr ResolvePath(CStr _Path)
{
	TArray<CStr> Paths;

	while (_Path.Len())
	{
		int iFind = _Path.Find("\\");

		CStr Path;
		if (iFind >= 0)
			Path = _Path.Copy(0, iFind);
		else
			Path = _Path;

		Paths.Add(Path);

		_Path = _Path.CopyFrom(Path.Len() + 1);
	}

	for (int i = 0; i < Paths.Len(); ++i)
	{
		if (Paths[i] == "..")
		{
			Paths.Del(i);
			Paths.Del(i-1);
			--i;
			continue;
		}

	}

	CStr NewPath;

	for (int i = 0; i < Paths.Len(); ++i)
	{
		NewPath += Paths[i] + "\\";
	}

	return NewPath;
}

/*
CMInstaller::CLanguage ParseLanguage(CRegistry *_pReg)
{
	if (!_pReg)
		return CMInstaller::CLanguage();

	CMInstaller::CLanguage Ret;
	
	for (int i = 0; i < _pReg->GetNumChildren(); ++i)
	{
		CMInstaller::CLanguageEntry Temp;
		Temp.m_Language = _pReg->GetChild(i)->GetThisName();
		Temp.m_Text = _pReg->GetChild(i)->GetThisValue();
		Ret.m_Entries.Add(Temp);
	}

	return Ret;
}
*/
void CMInstaller::CreateInstaller(CStr _SourcePath, CRegistry *_pReg)
{

	CMInstallerCompiler Comp;
	Comp.SetMediaSize(_pReg->GetValuef("MediaSize") * 1024 * 1024);
	Comp.m_Destination = ResolvePath(_SourcePath + _pReg->GetValue("Destination"));
	Comp.m_SourceRoot = ResolvePath(_SourcePath + _pReg->GetValue("SourceRoot"));

	m_RegistryRoot = _pReg->GetValue("RegistryRoot");
	m_InstallerName = _pReg->GetValue("InstallerName");
	m_ProgramFilesRoot = _pReg->GetValue("ProgramFilesRoot");
	m_StartMenuRoot = _pReg->GetValue("StartMenuRoot");	

	Comp.CleanDestination();
	Comp.WriteSignature();

	CRegistry *pComponents = _pReg->FindChild("Components");

	if (!pComponents)
		Error("CreateInstaller", "No components found");

	// Generate information
	for (int i = 0; i < pComponents->GetNumChildren(); ++i)
	{
		CRegistry *pComponent = pComponents->GetChild(i);

		if (pComponent->GetValuei("PlaceOnMedia", 0) == 1)
		{
			continue;
		}
		else
		{
			CComponent Comp;
			Comp.m_CheckState = pComponent->GetValuei("CheckState", 0);
			Comp.m_Visable = pComponent->GetValuei("Visable", 1);
			Comp.m_FileArchive = pComponent->GetValue("File");
			Comp.m_DisplayName = pComponent->GetValue("Name");
			
			CRegistry *pRegKeys = pComponent->FindChild("RegKeys");

			if (pRegKeys)
			{
				for (int i = 0; i < pRegKeys->GetNumChildren(); ++i)
				{
					CMInstaller::CComponent::CRegEntry Entry;
					CRegistry *pKey = pRegKeys->GetChild(i);
					if (pKey->GetThisName() == "KEY")
					{
						Entry.m_Type = CMInstaller::CComponent::ERegEntryType_Key;
						Entry.m_Location = pKey->GetValue("Location");
						Entry.m_Path = pKey->GetValue("Key");
						Comp.m_RegEntries.Add(Entry);
					}
					else if (pKey->GetThisName() == "VALUE")
					{
						Entry.m_Type = CMInstaller::CComponent::ERegEntryType_Value;
						Entry.m_Location = pKey->GetValue("Location");
						Entry.m_Path = pKey->GetValue("Key");

						Entry.m_ValueName = pKey->GetValue("ValueName");
						if (pKey->FindChild("ValueStr"))
						{
							Entry.m_ValueType = CMInstaller::CComponent::ERegEntryValueType_Str;
							Entry.m_ValueStr = pKey->GetValue("ValueStr");
							Entry.m_ValueUint32 = 0;
							Comp.m_RegEntries.Add(Entry);
						}
						else if (pKey->FindChild("ValueUint32"))
						{
							Entry.m_ValueType = CMInstaller::CComponent::ERegEntryValueType_Uint32;
							Entry.m_ValueStr;
							Entry.m_ValueUint32 = pKey->GetValuei("ValueUint32");
							Comp.m_RegEntries.Add(Entry);
						}
						else if (pKey->FindChild("ValueUint32Named"))
						{
							Entry.m_ValueType = CMInstaller::CComponent::ERegEntryValueType_Uint32;
							Entry.m_ValueStr = pKey->GetValue("ValueUint32Named");
							Entry.m_ValueUint32 = 0;
							Comp.m_RegEntries.Add(Entry);
						}
						else
							LogFile("ERROR: Parsing reg key failed due to wrong value type");
					}
					else
						LogFile("ERROR: Parsing reg key failed due to wrong key name");

				}
			}

			CRegistry *pAskExecute = pComponent->FindChild("AskExecute");

			if (pAskExecute)
			{
				for (int i = 0; i < pAskExecute->GetNumChildren(); ++i)
				{
					CMInstaller::CComponent::CAskExecute Entry;
					CRegistry *pKey = pAskExecute->GetChild(i);
					Entry.m_Question = pKey->GetValue("Ask");
					Entry.m_Execute = pKey->GetValue("Execute");
					Comp.m_AskExecute.Add(Entry);
				}
			}


			for (int i = 0; i < pComponent->GetNumChildren(); ++i)
			{
				CRegistry *pReg = pComponent->GetChild(i);

				if (pReg->GetThisName() == "SHORTCUT")
				{
					CMInstaller::CComponent::CShortCut Temp;
					Temp.m_DisplayName = pReg->GetValue("Name");
					Temp.m_Location = pReg->GetValue("Location");
					Temp.m_Target = pReg->GetValue("Target");
					Temp.m_Parameters = pReg->GetValue("Parameters");
					Temp.m_iIconPos = pReg->GetValuei("IconPos", -1);
					Comp.m_ShortCuts.Add(Temp);
				}
			}

			m_Components.Add(Comp);
		}        
	}

	CStr DataDest = Comp.GetDestination() + "Data.xin";

	CDiskUtil::CreatePath(DataDest.GetPath());
	Write(DataDest);
	Comp.UsedSpace(CDiskUtil::GetFileSize(DataDest));

	// Place files on media
	int iComp = 0;
	for (int i = 0; i < pComponents->GetNumChildren(); ++i)
	{
		CRegistry *pComponent = pComponents->GetChild(i);

		if (pComponent->GetValuei("PlaceOnMedia", 0) == 1)
		{
			Comp.CopyToMedia(pComponent->FindChild("Files"));
            
			continue;
		}
		else
		{
			CRegistry *pFiles = pComponent->FindChild("Files");
			if (pFiles)
			{
				m_Components[iComp].m_NeededSpace = Comp.GenerateArchive(pComponent->GetValue("File"), pFiles);
			}

			++iComp;
		}        
	}

	Write(DataDest);

	for (int i = 0; i < m_Components.Len(); ++i)
	{
		CStr Temp = m_Components[i].m_DisplayName;
		LogFile(CStrF("%s used %f bytes", Temp.Str(), (fp64)m_Components[i].m_NeededSpace));
	}
	LogFile(CStrF("Used %d medias", Comp.m_iCurrentMedia + 1));
	fp64 UsedBytes = (fp64)(Comp.m_iCurrentMedia * Comp.m_MediaSize) + (fp64)(Comp.m_MediaSize - Comp.m_MediaSpaceLeft);
	LogFile(CStrF("Used %0.0f bytes (%0.2f MiB)", UsedBytes, UsedBytes / (1024.0 * 1024.0)));
}

/*
void CMInstaller::CLanguageEntry::Write(CCFile &_File)
{
	m_Language.Write(&_File);
	m_Text.Write(&_File);
}

void CMInstaller::CLanguageEntry::Read(CCFile &_File)
{
	m_Language.Read(&_File);
	m_Text.Read(&_File);
}

void CMInstaller::CLanguage::Write(CCFile &_File)
{
	uint32 Num = m_Entries.Len();
	_File.WriteLE(Num);

	for (int i = 0; i < Num; ++i)
	{
		m_Entries[i].Write(_File);
	}
}

void CMInstaller::CLanguage::Read(CCFile &_File)
{
	uint32 Num = 0;
	_File.ReadLE(Num);
	m_Entries.SetLen(Num);

	for (int i = 0; i < Num; ++i)
	{
		m_Entries[i].Read(_File);
	}
}*/

void CMInstaller::CComponent::CShortCut::Write(CCFile &_File)
{
	m_Target.Write(&_File);
	m_Location.Write(&_File);
	m_Parameters.Write(&_File);
	m_DisplayName.Write(&_File);
	_File.WriteLE(m_iIconPos);
}

void CMInstaller::CComponent::CShortCut::Read(CCFile &_File)
{
	m_Target.Read(&_File);
	m_Location.Read(&_File);
	m_Parameters.Read(&_File);
	m_DisplayName.Read(&_File);
	_File.ReadLE(m_iIconPos);
}

void CMInstaller::CComponent::CAskExecute::Write(CCFile &_File)
{
	m_Question.Write(&_File);
	m_Execute.Write(&_File);
}

void CMInstaller::CComponent::CAskExecute::Read(CCFile &_File)
{
	m_Question.Read(&_File);
	m_Execute.Read(&_File);
}

void CMInstaller::CComponent::Write(CCFile &_File)
{
	_File.WriteLE(m_CheckState);
	_File.WriteLE(m_Visable);
	_File.WriteLE(m_NeededSpace);
	m_FileArchive.Write(&_File);
	m_DisplayName.Write(&_File);

	{
		uint32 Num = m_ShortCuts.Len();
		_File.WriteLE(Num);

		for (int i = 0; i < Num; ++i)
		{
			m_ShortCuts[i].Write(_File);
		}
	}

	{
		uint32 Num = m_RegEntries.Len();
		_File.WriteLE(Num);

		for (int i = 0; i < Num; ++i)
		{
			m_RegEntries[i].Write(_File);
		}
	}

	{
		uint32 Num = m_AskExecute.Len();
		_File.WriteLE(Num);

		for (int i = 0; i < Num; ++i)
		{
			m_AskExecute[i].Write(_File);
		}
	}

	
}

void CMInstaller::CComponent::Read(CCFile &_File)
{
	_File.ReadLE(m_CheckState);
	_File.ReadLE(m_Visable);
	_File.ReadLE(m_NeededSpace);
	m_FileArchive.Read(&_File);

	m_DisplayName.Read(&_File);

	{
		uint32 Num = 0;

		_File.ReadLE(Num);
		m_ShortCuts.SetLen(Num);

		for (int i = 0; i < Num; ++i)
		{
			m_ShortCuts[i].Read(_File);
		}
	}
	{
		uint32 Num = 0;

		_File.ReadLE(Num);
		m_RegEntries.SetLen(Num);

		for (int i = 0; i < Num; ++i)
		{
			m_RegEntries[i].Read(_File);
		}
	}
	{
		uint32 Num = 0;

		_File.ReadLE(Num);
		m_AskExecute.SetLen(Num);

		for (int i = 0; i < Num; ++i)
		{
			m_AskExecute[i].Read(_File);
		}
	}
}

void CMInstaller::CComponent::CRegEntry::Write(CCFile &_File)
{
	_File.WriteLE(m_Type);
	m_Location.Write(&_File);
	m_Path.Write(&_File);
	m_ValueName.Write(&_File);

	_File.WriteLE(m_ValueType);
	m_ValueStr.Write(&_File);
	_File.WriteLE(m_ValueUint32);
}

void CMInstaller::CComponent::CRegEntry::Read(CCFile &_File)
{
	_File.ReadLE(m_Type);
	m_Location.Read(&_File);
	m_Path.Read(&_File);
	m_ValueName.Read(&_File);

	_File.ReadLE(m_ValueType);
	m_ValueStr.Read(&_File);
	_File.ReadLE(m_ValueUint32);
}

void CMInstaller::Write(CCFile &_File)
{
	m_RegistryRoot.Write(&_File);
	m_InstallerName.Write(&_File);
	m_ProgramFilesRoot.Write(&_File);
	m_StartMenuRoot.Write(&_File);

	uint32 Num = m_Components.Len();
	_File.WriteLE(Num);

	for (int i = 0; i < Num; ++i)
	{
		m_Components[i].Write(_File);
	}

}

void CMInstaller::Read(CCFile &_File)
{
	m_RegistryRoot.Read(&_File);
	m_InstallerName.Read(&_File);
	m_ProgramFilesRoot.Read(&_File);
	m_StartMenuRoot.Read(&_File);

	uint32 Num = 0;
	_File.ReadLE(Num);
	m_Components.SetLen(Num);

	for (int i = 0; i < Num; ++i)
	{
		m_Components[i].Read(_File);
	}
}

void CMInstaller::Write(CStr _File)
{
	CCFile File;
	File.Open(_File, CFILE_WRITE|CFILE_TRUNC|CFILE_BINARY);
	Write(File);
}

void CMInstaller::Read(CStr _File)
{
	CCFile File;
	File.Open(_File, CFILE_READ|CFILE_BINARY);
	Read(File);
}



class CMInstallerWorker : public MRTC_Thread
{
public:

	CMInstaller *m_pInstaller;
	uint64 m_InstallSize;
	CStr m_Destination;
	CStr m_SourcePath;
	CMInstallerWorker(CMInstaller *_pInstaller, CStr _Dest, CStr _SourcePath)
	{
		m_pInstaller = _pInstaller;
		m_InstallSize = 0;
		m_Destination = _Dest;
		m_SourcePath = _SourcePath;
		for (int i = 0; i < m_pInstaller->m_Components.Len(); ++i)
		{
			if (m_pInstaller->m_Components[i].m_CheckState)
			{
				m_InstallSize += m_pInstaller->m_Components[i].m_NeededSpace;
			}
		}
		m_bDone = false;
		m_InstalledBytes = 0;
		m_MessageType = 0;
		m_LastMessageType = 0;
		m_bCancel = false;
		m_MessageAnswer = -2;
		m_iCurrentMedia = 0;
		m_pArchive = NULL;
		m_bUninstall = !_SourcePath.Len() && m_pInstaller->m_pInstallLog;
		m_UninstallProgress = 0;

		Thread_Create();
	}

	~CMInstallerWorker()
	{
		if (m_pArchive)
			delete m_pArchive;
	}

	CStr GetSource()
	{
		int iFind;
		CStr Lower = m_SourcePath.LowerCase(); 
		if ((iFind = Lower.FindReverse("media00")) >= 0)
		{
			return m_SourcePath.Left(iFind) + CStrF("Media%02d\\", m_iCurrentMedia);
		}
		else
		{
			return m_SourcePath;
		}
		return "";
	}

	void UsedSpace(uint64 _Space)
	{
		M_LOCK(m_Lock);
		m_InstalledBytes += _Space;
	}

	// Lock needed
	MRTC_CriticalSection m_Lock;
	uint64 m_InstalledBytes;
	CStr m_LastInstalledFile;
	int m_bDone;
	CStr m_Message;
	CStr m_LastFail;
	int m_MessageType;
	int m_LastMessageType;
	int m_MessageAnswer;
	int m_iCurrentMedia;
	int m_bUninstall;
	int m_bCancel;
	fp64 m_UninstallProgress;

	// Only thread uses
	int m_iCurrentComponent;

	class CArchiveRead
	{
	public:

		CMInstallerWorker *m_pInstaller;

		CCFile m_File;

		CStr m_FileName;

		z_stream m_Compressor; /* compression stream */

		uint32 m_NumFiles;
		uint32 m_WrittenFiles;
		uint32 m_iFile;

		bint m_bInit;

		uint64 m_InFileSize;
		mint m_FillSize;
		mint m_FillPos;

		TThinArray<uint8> m_InData;
		TThinArray<uint8> m_OutData;

		CArchiveRead(CMInstallerWorker *_pCompiler, CStr _File)
		{
			m_pInstaller = _pCompiler;

			m_NumFiles = 0;
			m_FileName = _File;
			m_iFile = 0;
			m_ConsumeMode = 0;
			m_WrittenFiles = 0;
			m_FillSize = 0;
			m_FillPos = 0;
			m_ConsumePos = 0;

			m_OutData.SetLen(32768);
			m_InData.SetLen(32768);

			m_pConsume = NULL;
			m_ConsumeAvail = 0;

			
			m_Compressor.zalloc = (alloc_func)0;
			m_Compressor.zfree = (free_func)0;
			m_Compressor.opaque = (voidpf)0;
			memset(&m_Compressor, 0, sizeof(m_Compressor));
			int err;
			err = inflateInit(&m_Compressor);

			if (err != Z_OK)
				Error_static("CArchiveRead::Create", CStrF("inflateInit returned %d", err));

			m_bInit = true;
			m_bOpened = false;
		}

		enum 
		{
			EUpdate_NoError,
			EUpdate_WrongInputFile,
			EUpdate_CorruptFile,
			EUpdate_CompressionError,
			EUpdate_FileCRCCheckFail,
			EUpdate_Done,
		};

		int m_bOpened;

		void FillInBuffer()
		{
			if (m_Compressor.avail_in || !m_InFileSize)
				return;

//			if (!m_FillSize)
			{
				m_FillSize = MinMT(m_InFileSize, m_InData.Len());
				m_InFileSize -= m_FillSize;
			}

			mint Left = m_FillSize;

			m_File.Read(m_InData.GetBasePtr(), Left);
			m_FillPos += Left;

			m_Compressor.next_in = (Bytef *)m_InData.GetBasePtr();
			m_Compressor.avail_in = m_FillSize;

		}

		int m_ConsumeMode;
		mint m_ConsumePos;
		uint64 m_ConsumeFileSize;
		uint32 m_ConsumeHeaderSize;
		CFileInfo m_ConsumeFileInfo;
		CMD5Digest m_ConsumeFileMD5;
		CMD5 m_ConsumeCurrentMD5;
		CStr m_ConsumeOutputFile;

		uint8 *m_pConsume;
		int m_ConsumeAvail;

		CCFile m_OutFile;
		TArray<uint8> m_TempConsume;
		int ConsumeOutBuffer()
		{
			if (!m_ConsumeAvail)
			{
				m_Compressor.next_out = m_OutData.GetBasePtr();
				m_Compressor.avail_out = m_OutData.Len();

				int err = inflate(&m_Compressor, Z_NO_FLUSH);

				if (err == Z_BUF_ERROR)
				{
	//				err = inflate(&m_Compressor, Z_SYNC_FLUSH);
	//				if (err == Z_BUF_ERROR)
	//				{
	//					if (m_WrittenFiles >= (m_NumFiles - 1) && m_ConsumeMode == 2 && (m_ConsumeFileSize - m_ConsumePos) < 1024*1024)
	//						err = inflate(&m_Compressor, Z_FINISH);
	//
	//					if (err == Z_BUF_ERROR)
	//					{
							if (!m_Compressor.avail_in)
							{
								if (!m_InFileSize)
								{
									m_FillSize = 0;
									m_FillPos = 0;
									m_bOpened = false;
									++m_iFile;
									m_File.Close();
								}
								return EUpdate_NoError;
							}
	//					}
	//				}
				}

				if (err != Z_OK && err != Z_BUF_ERROR && err != Z_STREAM_END)
					return EUpdate_CompressionError;

				m_pConsume = m_OutData.GetBasePtr();
				m_ConsumeAvail = m_OutData.Len() - m_Compressor.avail_out;
			}

			uint8 *pOutput = m_pConsume;
			int Avail = m_ConsumeAvail;

			while (Avail)
			{
				if (m_ConsumeMode == 0)
				{
					mint ToConsume = MinMT(4 - m_ConsumePos, Avail);
					memcpy((uint8 *)&m_ConsumeHeaderSize + m_ConsumePos, pOutput, ToConsume);
					m_ConsumePos += ToConsume;
					Avail -= ToConsume;
					pOutput += ToConsume;
#ifdef CPU_BIGENDIAN
					ByteSwap_uint32(m_ConsumeHeaderSize);
#endif					
					
					if (m_ConsumePos == 4)
					{
						m_ConsumeHeaderSize -= 4;
						if (m_TempConsume.Len() < m_ConsumeHeaderSize)
							m_TempConsume.SetLen(m_ConsumeHeaderSize);
						m_ConsumeMode = 1;
						m_ConsumePos = 0;
					}
				}
				else if (m_ConsumeMode == 1)
				{
					mint ToConsume = MinMT(m_ConsumeHeaderSize - m_ConsumePos, Avail);
					memcpy(m_TempConsume.GetBasePtr() + m_ConsumePos, pOutput, ToConsume);
					m_ConsumePos += ToConsume;
					Avail -= ToConsume;
					pOutput += ToConsume;

					if (m_ConsumePos == m_ConsumeHeaderSize)
					{
						CCFile TempFile;
						CStream_Memory MemoryStream(m_TempConsume.GetBasePtr(), m_ConsumeHeaderSize, m_ConsumeHeaderSize);
						TempFile.Open(&MemoryStream, CFILE_READ | CFILE_BINARY);

						CStr File;
						File.Read(&TempFile);
						{
							M_LOCK(m_pInstaller->m_Lock);
							m_pInstaller->m_LastInstalledFile = File;
						}
						File = m_pInstaller->m_Destination + File;
						TempFile.ReadLE(m_ConsumeFileSize);
						TempFile.Read(m_ConsumeFileMD5.f_GetData(), 16);			
						TempFile.WriteLE(m_ConsumeFileInfo.m_TimeWrite);
						m_ConsumeFileInfo.m_TimeAccess = m_ConsumeFileInfo.m_TimeCreate = m_ConsumeFileInfo.m_TimeWrite;

						m_ConsumePos = 0;
						m_ConsumeMode = 2;
						m_ConsumeCurrentMD5.f_Reset();

						CDiskUtil::CreatePath(File.GetPath());

						m_ConsumeOutputFile = File;
						m_OutFile.Open(m_ConsumeOutputFile, CFILE_WRITE | CFILE_BINARY);

						m_pInstaller->m_pInstaller->m_pInstallLog->AddFile(File);
					}
				}
				else if (m_ConsumeMode == 2)
				{
					mint ToConsume = MinMT(m_ConsumeFileSize - m_ConsumePos, Avail);
					m_OutFile.Write(pOutput, ToConsume);
					m_ConsumeCurrentMD5.f_AddData(pOutput, ToConsume);
					m_ConsumePos += ToConsume;
					Avail -= ToConsume;
					pOutput += ToConsume;
					ConsumeSpace(ToConsume);

					if (m_ConsumePos == m_ConsumeFileSize)
					{
						CMD5Digest Digest(m_ConsumeCurrentMD5);
						m_OutFile.Close();
						++m_WrittenFiles;
						m_ConsumeMode = 0;
						m_ConsumePos = 0;
						if (Digest != m_ConsumeFileMD5)
						{
							m_pConsume = pOutput;
							m_ConsumeAvail = Avail;

							{
								M_LOCK(m_pInstaller->m_Lock);
								m_pInstaller->m_LastFail = m_ConsumeOutputFile;
							}

							return EUpdate_FileCRCCheckFail;
						}
					}
				}
			}

			m_pConsume = pOutput;
			m_ConsumeAvail = Avail;

			return EUpdate_NoError;
		}

		int Open()
		{
			if (!m_bOpened)
			{
				CStr File = m_pInstaller->GetSource() + m_FileName;
				{
					M_LOCK(m_pInstaller->m_Lock);
					m_pInstaller->m_LastFail = File;
				}

				if (!CDiskUtil::FileExists(File))
					return EUpdate_WrongInputFile;

				m_File.Open(File, CFILE_BINARY | CFILE_READ);
				char Temp[5];
				Temp[4] = 0;
				m_File.Read(Temp, 4);
				if (strcmp("XCAR", Temp) != 0)
					return EUpdate_CorruptFile;

				uint32 iFile;
				m_File.ReadLE(iFile);
				if (iFile != m_iFile)
					return EUpdate_WrongInputFile;

				m_InFileSize = m_File.Length();

				if (iFile == 0)
				{
					m_File.ReadLE(m_NumFiles);
					m_InFileSize -= 12;
				}
				else
				{
					m_InFileSize -= 8;
				}

				m_bOpened = true;
			}

			return EUpdate_NoError;
		}

		int Update()
		{
			int Ret = Open();

			if (Ret != EUpdate_NoError)
				return Ret;
			 
			FillInBuffer();

			Ret = ConsumeOutBuffer();
			if (Ret != EUpdate_NoError)
				return Ret;

			if (m_WrittenFiles == m_NumFiles)
				return EUpdate_Done;

			return EUpdate_NoError;
		}

		~CArchiveRead()
		{
			Destroy();
		}

		void Destroy()
		{
			if (m_bInit)
			{
				int err = inflateEnd(&m_Compressor);

				if (err != Z_OK)
					M_TRACE("ERROR: deflateEnd returned %d", err);
				m_bInit = false;
				m_File.Close();
			}
			else
			{
				m_File.Close();
			}
		}

		void ConsumeSpace(fint _Space)
		{
			m_pInstaller->UsedSpace(_Space);
		}

		void AddData(const void *_pData, mint _DataLen)
		{
/*			mint Left = _DataLen;
			m_Compressor.next_in = (Bytef *)_pData;
			m_Compressor.avail_in = Left;

			while (m_Compressor.avail_in)
			{
				if (!m_Compressor.avail_out)
				{
					if (m_Compressor.next_out)
					{
						mint Len = m_OutData.Len();
						uint8 *pData = m_OutData.GetBasePtr();
						while (Len)
						{
							int MaxSpace = MinMT(SpaceLeft(), Len);
							m_File.Write(pData, MaxSpace);
							pData += MaxSpace;
							Len -= MaxSpace;
							ConsumeSpace(MaxSpace);

							MakeSpace(1);
						}
					}
					m_Compressor.next_out = m_OutData.GetBasePtr();
					m_Compressor.avail_out = m_OutData.Len();
				}
				int err = deflate(&m_Compressor, Z_NO_FLUSH);

				if (err != Z_OK && err != Z_BUF_ERROR)
					Error_static("CArchiveRead::AddData", "deflate returned error");
			}*/
		}

		void AddFile(CStr _SourceFile, CStr _Name)
		{
/*			CCFile TempFile;
			CStream_Memory MemoryStream;
			TArray<uint8> Memory;
			MemoryStream.Open(Memory, CFILE_READ | CFILE_BINARY);
			TempFile.Open(&MemoryStream, CFILE_WRITE | CFILE_BINARY);
			
			uint64 FileSize = CDiskUtil::GetFileSize(_SourceFile);

			_Name.Write(&TempFile);
			TempFile.WriteLE(FileSize);
			TempFile.Close();

			AddData(Memory.GetBasePtr(), Memory.Len());
			
			CCFile File;
			File.Open(_SourceFile, CFILE_READ | CFILE_BINARY);

            while (FileSize)
			{
				mint ToRead = MinMT(m_InData.Len(), FileSize);

				File.Read(m_InData.GetBasePtr(), ToRead);

				AddData(m_InData.GetBasePtr(), ToRead);
				FileSize -= ToRead;
			}*/
		}
	};

	CArchiveRead *m_pArchive;

	int m_FailCount;
	int m_bCheckedMedia;
	int m_PerformMode;
	int m_iShortCut;
	int m_iRegEntry;
	int m_iAskExecute;
	int m_iLastAnswer;

	int m_iError;

	int m_iCurrentUninstall;

	void NextMedia()
	{
		m_bCheckedMedia = false;
		++m_iCurrentMedia;
	}

	void Message(CStr _ToAsk, int _Type, bool _bError)
	{
		if (_bError)
			m_iError = true;
		M_LOCK(m_Lock);
		m_LastMessageType = m_MessageType = _Type;
		m_Message = _ToAsk;
		m_MessageAnswer = -1;
	}

	int Thread_Main()
	{
		m_iCurrentComponent = 0;
		m_FailCount = 0;
		m_bCheckedMedia = 0;
		m_PerformMode = 0;
		m_iShortCut = 0;
		m_iRegEntry = 0;
		m_iAskExecute = 0;
		m_iError = 0;
		m_iLastAnswer = -1;
		bool bNoMoreErrors = false;
		if (m_bUninstall)
		{
			m_iCurrentUninstall = m_pInstaller->m_pInstallLog->m_Entries.Len() - 1;
		}

		while (!Thread_IsTerminating())
		{
			bool bSleep = true;

			if (m_bCancel)
			{
				M_LOCK(m_Lock);
				m_bDone = 3;
				return 0;
			}
		
			if (m_MessageAnswer > -2)
			{
				if (m_MessageAnswer != -1)
				{
					// The client thread has answered
					if (m_Message == "UNINSTALLOFITEMFAILED")
					{
						if (!m_MessageAnswer)
							bNoMoreErrors = true;
					}
					else if (m_Message == "ASKFORMEDIA")
					{
						if (m_MessageAnswer)
						{
							m_MessageAnswer = -2;
						}
						else
						{
							M_LOCK(m_Lock);
							m_bDone = 2;
							return 0;
						}
					}
					else
					{
						m_iLastAnswer = m_MessageAnswer;
						if (m_LastMessageType == CMInstaller::EMessageType_QuestionYesCancel)
						{
							if (m_MessageAnswer)
							{
								m_MessageAnswer = -2;
							}
							else
							{
								M_LOCK(m_Lock);
								m_bDone = 2;
								return 0;
							}
						}
						else if (m_LastMessageType == CMInstaller::EMessageType_ErrorCancel)
						{
							if (!m_MessageAnswer)
								bNoMoreErrors = true;
						}

						if (m_Message == "COMPRESSIONERROR" || m_Message == "CRCCHECKFAIL")
						{
							M_LOCK(m_Lock);
							m_bDone = 2;
							return 0;
						}
					}

					m_MessageAnswer = -2;
				}
			}
			else
			{
				if (m_bUninstall)
				{
					if (m_iCurrentUninstall < 0)
					{
						M_LOCK(m_Lock);
						if (m_iError)
							m_bDone = 4;
						else
							m_bDone = 1;
						return 0;
					}
					int iInstall = m_iCurrentUninstall;
					--m_iCurrentUninstall;

					CStr Item;

					if (!m_pInstaller->m_pInstallLog->PerformUninstall(iInstall, Item))
					{
						{
							M_LOCK(m_Lock);
							m_LastFail = Item;
						}
						if (!bNoMoreErrors)
							Message("UNINSTALLOFITEMFAILED", CMInstaller::EMessageType_QuestionYesCancel, true);
						continue;
					}

					{
						M_LOCK(m_Lock);
						m_LastInstalledFile = Item;
						m_UninstallProgress = (m_pInstaller->m_pInstallLog->m_Entries.Len() - m_iCurrentUninstall) / fp64(m_pInstaller->m_pInstallLog->m_Entries.Len());
					}
				}
				else
				{
					if (m_iCurrentComponent >= m_pInstaller->m_Components.Len())
					{
						M_LOCK(m_Lock);
						if (m_iError)
							m_bDone = 4;
						else
							m_bDone = 1;
						return 0;
					}

					CMInstaller::CComponent &Component = m_pInstaller->m_Components[m_iCurrentComponent];

					if (!m_bCheckedMedia)
					{            
						CStr File = GetSource() + "Disk.id";

						if (!CDiskUtil::FileExists(File))
						{
							Message("ASKFORMEDIA", CMInstaller::EMessageType_QuestionYesCancel, false);
							continue;
						}

						CCFile Test; 
						Test.Open(File, CFILE_READ | CFILE_BINARY);
						uint32 iMedia;
						Test.ReadLE(iMedia);

						if (iMedia != m_iCurrentMedia)
						{
							Message("ASKFORMEDIA", CMInstaller::EMessageType_QuestionYesCancel, false);
							continue;
						}

						m_bCheckedMedia = true;
					}

					if (!Component.m_CheckState)
					{
						++m_iCurrentComponent;
						bSleep = 0;
						continue;
					}
					if (m_PerformMode == 0)
					{
						// Files
						if (!m_pArchive)
						{
							if (!Component.m_FileArchive.Len() || !Component.m_CheckState)
							{
								++m_PerformMode;
								bSleep = 0;
								continue;
							}
							m_pArchive = DNew(CArchiveRead) CArchiveRead(this, Component.m_FileArchive);
						}

						int Err = m_pArchive->Update();
						switch (Err)
						{
						case CArchiveRead::EUpdate_NoError:
							{
								bSleep = false;
								m_FailCount = 0;
							}
							break;
						case CArchiveRead::EUpdate_WrongInputFile:
							{
								M_LOCK(m_Lock);
								if (m_FailCount == 0)
									NextMedia();

								if (m_FailCount > 0)
								{
									Message("ASKFORMEDIA", CMInstaller::EMessageType_QuestionYesCancel, false);
								}
								++m_FailCount;
							}
							break;
						case CArchiveRead::EUpdate_CorruptFile:
							{
								Message("CORRUPTFILE", CMInstaller::EMessageType_QuestionYesCancel, true);
							}
							break;

						case CArchiveRead::EUpdate_CompressionError:
							{
								Message("COMPRESSIONERROR", CMInstaller::EMessageType_Error, true);
							}
							break;
						case CArchiveRead::EUpdate_FileCRCCheckFail:
							{
								Message("CRCCHECKFAIL", CMInstaller::EMessageType_Error, true);
							}
							break;
						case CArchiveRead::EUpdate_Done:
							{
								delete m_pArchive;
								m_pArchive = NULL;
								++m_PerformMode;
							}
							break;
						}
					}
					else if (m_PerformMode == 1)
					{
						// Shortcuts
						if (m_iShortCut >= Component.m_ShortCuts.Len())
						{
							++m_PerformMode;
							m_iShortCut = 0;
						}
						else
						{
							CMInstaller::CComponent::CShortCut& Short = Component.m_ShortCuts[m_iShortCut];

							if (!m_pInstaller->CreateShortcut(Short.m_DisplayName, Short.m_Target, Short.m_Location, Short.m_Parameters, Short.m_iIconPos))
							{
								{
									M_LOCK(m_Lock);
									m_LastFail = Short.m_Location + Short.m_DisplayName;
								}

								if (!bNoMoreErrors)
									Message("FAILEDTOCREATESHORTCUT", CMInstaller::EMessageType_ErrorCancel, true);
							}
							bSleep = 0;
							++m_iShortCut;
						}
					}
					else if (m_PerformMode == 2)
					{
						// Shortcuts
						if (m_iRegEntry >= Component.m_RegEntries.Len())
						{
							++m_PerformMode;
							m_iRegEntry = 0;
						}
						else
						{
							CMInstaller::CComponent::CRegEntry& Reg = Component.m_RegEntries[m_iRegEntry];

							switch (Reg.m_Type)
							{
							case CMInstaller::CComponent::ERegEntryType_Key:
								{
									if (!m_pInstaller->CreateRegistryKey(Reg.m_Location, Reg.m_Path))
									{

										{
											M_LOCK(m_Lock);
											m_LastFail = Reg.m_Location + "\\" + Reg.m_Path;
										}
										if (!bNoMoreErrors)
											Message("FAILEDTOCREATEREGISTRYKEY", CMInstaller::EMessageType_ErrorCancel, true);
									}
								}
								break;
							case CMInstaller::CComponent::ERegEntryType_Value:
								{
									switch (Reg.m_ValueType)
									{
									case CMInstaller::CComponent::ERegEntryValueType_Uint32:
										{
											uint32 Value = Reg.m_ValueUint32;
											if (Reg.m_ValueStr.Len())
											{
												Value = m_pInstaller->GetNamedValue(Reg.m_ValueStr);
											}
											if (!m_pInstaller->SetRegistryKeyValue(Reg.m_Location, Reg.m_Path, Reg.m_ValueName, Value))
											{
												{
													M_LOCK(m_Lock);
													m_LastFail = Reg.m_Location + "\\" + Reg.m_Path + "." + Reg.m_ValueName;
												}
												if (!bNoMoreErrors)
													Message("FAILEDTOSETREGISTRYVALUE", CMInstaller::EMessageType_ErrorCancel, true);
											}
										}
										break;
									case CMInstaller::CComponent::ERegEntryValueType_Str:
										{
											if (!m_pInstaller->SetRegistryKeyValue(Reg.m_Location, Reg.m_Path, Reg.m_ValueName, Reg.m_ValueStr))
											{
												{
													M_LOCK(m_Lock);
													m_LastFail = Reg.m_Location + "\\" + Reg.m_Path + "(" + Reg.m_ValueName +")";
												}
												if (!bNoMoreErrors)
													Message("FAILEDTOSETREGISTRYVALUE", CMInstaller::EMessageType_ErrorCancel, true);
											}
										}
										break;
									}
								}
								break;
							}


							bSleep = 0;
							++m_iRegEntry;
						}
					}
					else if (m_PerformMode == 3)
					{
						// Shortcuts
						if (m_iAskExecute >= Component.m_AskExecute.Len())
						{
							++m_PerformMode;
							m_iAskExecute = 0;
						}
						else
						{
							CMInstaller::CComponent::CAskExecute& Entry = Component.m_AskExecute[m_iAskExecute];

							bSleep = 0;
							if (m_iLastAnswer < 0)
								Message(Entry.m_Question, CMInstaller::EMessageType_QuestionYesNo, false);
							else
							{
								if (m_iLastAnswer)
								{
									m_pInstaller->Execute(Entry.m_Execute);
								}
								m_iLastAnswer = -1;
								++m_iAskExecute;
							}							
						}
					}
					else if (m_PerformMode == 4)
					{
						m_PerformMode = 0;
						++m_iCurrentComponent;
					}
				}
			}
			if (bSleep)
				MRTC_SystemInfo::OS_Sleep(10);
		}

		return 0;
	}
};

CMInstaller::~CMInstaller()
{
	if (m_pWorker)
		delete m_pWorker;
}

void CMInstallLog::AddDeleteEmptyDirs(CStr _Path)
{
	CLogEntry Entry;
	Entry.m_Type = CMInstallLog::ELogEntry_DeleteEmptyDir;
	Entry.m_DataInt = 0;
	Entry.m_DataStr0 = _Path;

	Entry.Write(m_File);
}

void CMInstallLog::AddDeleteDirsAsk(CStr _Path, CStr _Ask)
{
	CLogEntry Entry;
	Entry.m_Type = CMInstallLog::ELogEntry_DeleteDirAsk;
	Entry.m_DataInt = 0;
	Entry.m_DataStr0 = _Path;
	Entry.m_DataStr1 = _Ask;	

	Entry.Write(m_File);
}

void CMInstallLog::AddDeleteEmptyRegKeys(int _Root, CStr _Path)
{
	CLogEntry Entry;
	Entry.m_Type = CMInstallLog::ELogEntry_DeleteEmptyRegKeys;
	Entry.m_DataInt = _Root;
	Entry.m_DataStr0 = _Path;

	Entry.Write(m_File);
}

void CMInstallLog::AddFile(CStr _FileName)
{
	CLogEntry Entry;
	Entry.m_Type = CMInstallLog::ELogEntry_File;
	Entry.m_DataInt = 0;
	Entry.m_DataStr0 = _FileName;

	Entry.Write(m_File);
}

void CMInstallLog::AddRegKey(int _Root, CStr _Key)
{
	CLogEntry Entry;
	Entry.m_Type = CMInstallLog::ELogEntry_RegKey;
	Entry.m_DataInt = _Root;
	Entry.m_DataStr0 = _Key;

	Entry.Write(m_File);
}

void CMInstallLog::AddRegValue(int _Root, CStr _Key, CStr _Value)
{
	CLogEntry Entry;
	Entry.m_Type = CMInstallLog::ELogEntry_RegValue;
	Entry.m_DataInt = _Root;
	Entry.m_DataStr0 = _Key;
	Entry.m_DataStr1 = _Value;

	Entry.Write(m_File);
}

void CMInstallLog::CreateRecord(CStr _FileName)
{
	m_Entries.Clear();
	m_File.Open(_FileName, CFILE_WRITE | CFILE_BINARY);    
}
void CMInstallLog::CloseRecord()
{
	m_File.Close();
}


void CMInstallLog::CreateUninstall(CStr _FileName)
{
	m_Entries.Clear();
	m_File.Open(_FileName, CFILE_READ | CFILE_BINARY);    

	while (!m_File.EndOfFile())
	{
		CLogEntry NewEntry;
		NewEntry.Read(m_File);
		m_Entries.Add(NewEntry);
	}

	m_File.Close();
}

void CMInstaller::StartInstall(CStr _Destination, CStr _SourcePath, CMInstallLog *_pInstallLog)
{
	m_pInstallLog = _pInstallLog;

	if (m_pWorker)
		return;

	{
		m_pWorker = DNew(CMInstallerWorker) CMInstallerWorker(this, _Destination, _SourcePath);

	}
}

void CMInstaller::CancelInstall()
{
	if (!m_pWorker)
		return;

	M_LOCK(m_pWorker->m_Lock);
	m_pWorker->m_bCancel = true;
}

void CMInstaller::StartUninstall(CMInstallLog *_pInstallLog)
{
	m_pInstallLog = _pInstallLog;
	if (m_pWorker)
		return;

	{
		m_pWorker = DNew(CMInstallerWorker) CMInstallerWorker(this, "", "");

	}
}

fp32 CMInstaller::GetInstallProgress()
{
	if (m_pWorker)
	{
		M_LOCK(m_pWorker->m_Lock);
		if (m_pWorker->m_bUninstall)
			return m_pWorker->m_UninstallProgress;
		else
			return (fp64)m_pWorker->m_InstalledBytes / (fp64)m_pWorker->m_InstallSize;
	}
	return 0;
}

CStr CMInstaller::GetInstallLastFile()
{
	if (m_pWorker)
	{
		M_LOCK(m_pWorker->m_Lock);
		CStr Temp;
		Temp.Capture(m_pWorker->m_LastInstalledFile.Str());
		return Temp;
	}
	return "";
}

int CMInstaller::GetCurrentMedia()
{
	if (m_pWorker)
	{
		M_LOCK(m_pWorker->m_Lock);
		return m_pWorker->m_iCurrentMedia;
	}
	return 0;
}


int CMInstaller::GetInstallDone()
{
	int bDone = false;
	if (m_pWorker)
	{
		{
			M_LOCK(m_pWorker->m_Lock);
			bDone = m_pWorker->m_bDone;
		}
		if (bDone)
		{
			m_pWorker->Thread_Destroy();
			delete m_pWorker;
			m_pWorker = NULL;
		}
		else
			return false;
	}
	return bDone;
}

int CMInstaller::GetLastMessage(CStr &_Message)
{
	if (!m_pWorker)
		return 0;
	M_LOCK(m_pWorker->m_Lock);
	_Message.Capture(m_pWorker->m_Message.Str());
	return m_pWorker->m_MessageType;
}

void CMInstaller::MessageAnswer(int _Answer)
{
	if (!m_pWorker)
		return;
	M_LOCK(m_pWorker->m_Lock);
	m_pWorker->m_MessageType = EMessageType_NoMessage;
	m_pWorker->m_MessageAnswer = _Answer;
}

CStr CMInstaller::GetInstallLastFail()
{
	CStr Str;
	if (m_pWorker)
	{
		M_LOCK(m_pWorker->m_Lock);
		Str.Capture(m_pWorker->m_LastFail.Str());
	}
	return Str;
}
