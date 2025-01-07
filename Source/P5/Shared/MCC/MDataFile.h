
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/


#ifndef __INC_MDataFile
#define __INC_MDataFile

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDataFile
|__________________________________________________________________________________________________
\*************************************************************************************************/
class MCCDLLEXPORT CDataFileNode
{
public:
	char m_NodeName[16];
	int32 m_OffsetNext;
	int32 m_OffsetSubDir;
	int32 m_Size;
	int32 m_UserData;

	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile);
};

class MCCDLLEXPORT CDataFileNode2
{
public:
	char m_NodeName[24];
	int32 m_OffsetNext;
	int32 m_OffsetSubDir;
	int32 m_OffsetData;
	int32 m_Size;
	int32 m_UserData;
	int32 m_UserData2;

	CDataFileNode2();
	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile, bool _bPretend);
};

// -------------------------------------------------------------------
class CDF_Node;
typedef TPtr<CDF_Node> spCDF_Node;

//class CDF_Node : public CSCBPoolableCompatibleDeleteCompatibleCRef
class CDF_Node : public CReferenceCount
{
public:
//	
//	DECLARE_OPERATOR_NEW
//	

	static CDF_Node* AllocNode();

	CDF_Node();
	~CDF_Node();

//	CFStr34 m_Name;

	CDataFileNode2 m_Node;
	uint32 m_FileOffset;

	uint32 m_iNodeNext;
	uint32 m_iNodeSubDir;
	uint32 m_iNodePrev;
	uint32 m_iNodeParent;
	
	TArray<uint8> m_lData;

	//Needed for TArray
	const CDF_Node & operator = (const CDF_Node & _Node);

//	int GetNodeCount();
};

// -------------------------------------------------------------------
#define CDATAFILE_MAXDEPTH 32
#define CDATAFILE_STACKSIZE 16
#define CDATAFILE_VERSION 3

enum EDataFileFlag
{
	EDataFileFlag_NoHidePos = DBit(0)
};

class MCCDLLEXPORT CDataFile : public CReferenceCount
{
	class CDF_Position
	{
	public:
		bool m_bNextIsThis;
		int m_HeaderOffset;
		int m_EntryOffset;
		int32 m_SubDirOffsets[CDATAFILE_MAXDEPTH];
		int32 m_SubDirLevel;
		int m_LastUserData;
		int m_LastUserData2;
		int m_LastEntrySize;
		uint32 m_FilePos;		// Only used when push/pop -ing.
		uint32 m_iNode;
	};

	// Global states
	int m_Version;
	int m_RootOffset;
	int m_OpenFlags:8;
	int m_bWriteMode:1;
	int m_bReadMode:1;

	uint32 m_CreateDataPos;
	CStream_Memory m_CreateStream;
	spCCFile m_spCreateFile;
	spCCFile m_spFile;

	CDF_Position m_lStack[CDATAFILE_STACKSIZE];
	int m_iStack;
	
	CDataFileNode m_DF1TempNode;

	// Datafile2
//	CSCBPool m_NodesPool;
//	spCDF_Node m_spRoot;
	TArray<CDF_Node>	m_lNodes;

	uint32 CountFileNodes_r();
	void ReadTree(CCFile * _pFile = NULL);

	void ReadTree_r(uint32 & _iNode, uint32 _iParent);
	void WriteTree_r(uint32 _iNode);
	void WriteTreeData_r(uint32 _iNode, CCFile * _pFile = NULL);

	void WriteTreeHelperPretend_r(uint32 _iNode, int &_MaxPos, CCFile * _pFile = NULL);
	void WriteTreeHelper_r(uint32 _iNode, CCFile *_pFile);
	void WriteTreeHelper(uint32 _iNode, CCFile * _pFile = NULL);

#ifndef M_RTM

	void ReadAll_r(uint32 _iNode);

#endif

public:
	
	DECLARE_OPERATOR_NEW
	

	CDataFile();
	void Open(const char *_FileName);
	void Open(spCCFile _spFile, int _Offset);
	void Open(TArray<uint8> _lFile);
	void Close();

#ifndef M_RTM

	// Put the index in a long list in the beginning of the datafile
	bool PutHeaderFirst(spCCFile _spInFile, spCCFile _spOutFile, int _InOffset = 0, int _OutOffset = 0);
	bool PutHeaderFirst(const char *_pInFile,const char *_pOutFile, int _InOffset = 0, int _OutOffset = 0);

#endif

	// Navigating tools
	bool HasSubDir();
	bool GetNext(const char* _EntryName);
	const char * GetNext();
	bool GetSubDir();
	bool GetParent();
	bool GetDirectory(CStr _Path);
	spCCFile GetFile();
	int32 GetUserData() { return m_lStack[m_iStack].m_LastUserData; };
	int32 GetUserData2() { return m_lStack[m_iStack].m_LastUserData2; };
	int32 GetEntrySize() { return m_lStack[m_iStack].m_LastEntrySize; };
	int32 GetEntryPos();						// Get file position within entry
	uint32 GetEntryDataFileOffset();			// Returns file-offset to the current CDataFileNode, for use with Open(_spFile, _Offset)

	TArray<uint8> ReadEntry();

	void PushPosition();
	void PopPosition();

	// Creation tools
	void Create(spCCFile _spFile, int _Version = CDATAFILE_VERSION, int _Flags = 0);
	void Create(CStr _FileName, int _Version = CDATAFILE_VERSION, ECompressTypes _eType = NO_COMPRESSION, ESettings _eSet = NORMAL_COMPRESSION, int _Flags = 0);
	void Create(TArray<uint8> _lFile, int _Version = CDATAFILE_VERSION, int _Flags = 0);
	void BeginEntry(CStr _EntryName);
	void EndEntry(int32 _UserData, int32 _UserData2 = 0);
	void BeginSubDir();
	void EndSubDir();
	void Write(void* _pBlock, int _Size);

	template<class T>
	void WriteArrayEntry(const char* _EntryName, T* _pArray, int _Len, int _Version)
	{
		BeginEntry(_EntryName);
		for(int i = 0; i < _Len; i++) 
			_pArray[i].Write(this);
		EndEntry(_Len, _Version);
	}

	template<class T>
	void ReadArray_Complex(T* _pArray)
	{
		int Len = GetUserData();
		int Ver = GetUserData2();
		CCFile* pF = GetFile();

		{
			for(int i = 0; i < Len; i++) 
				_pArray[i].Read(pF, Ver);
		}
	}

	template<class T>
	void ReadArray(T* _pArray, int _CurrentVersion)
	{
		int Len = GetUserData();
		int Ver = GetUserData2();
		CCFile* pF = GetFile();

		if ((Len * sizeof(T) == GetEntrySize()) &&
			(Ver == _CurrentVersion))
		{
			pF->Read(_pArray, GetEntrySize());

#ifndef CPU_LITTLEENDIAN
			for(int i = 0; i < Len; i++) 
				_pArray[i].SwapLE();
#endif
		}
		else
		{
			for(int i = 0; i < Len; i++) 
				_pArray[i].Read(pF, Ver);
		}
	}

	template<class T>
	void ReadArray_NoVer(T* _pArray)
	{
		int Len = GetUserData();
		int Ver = GetUserData2();
		CCFile* pF = GetFile();

		if (Len * sizeof(T) == GetEntrySize())
		{
			pF->Read(_pArray, GetEntrySize());

#ifndef CPU_LITTLEENDIAN
			for(int i = 0; i < Len; i++) 
				_pArray[i].SwapLE();
#endif
		}
		else
		{
			for(int i = 0; i < Len; i++) 
				_pArray[i].Read(pF);
		}
	}

	template<class T>
	void WriteArrayEntry(T& _Array, const char* _pEntryName, int _Version)
	{
		WriteArrayEntry(_pEntryName, _Array.GetBasePtr(), _Array.Length(), _Version);
	}

	template <class T>
	bool ReadArrayEntry_Complex(T& _Array, const char* _pEntryName)
	{
		bool bRes = true;
		PushPosition();

		if (GetNext(_pEntryName))
		{
			int Len = GetUserData();
			_Array.SetLen(Len);
			ReadArray_Complex(_Array.GetBasePtr());
		}
		else
			bRes = false;

		PopPosition();
		return bRes;
	}

	template <class T>
	bool ReadArrayEntry(T& _Array, const char* _pEntryName, int _CurrentVersion)
	{
		bool bRes = true;
		PushPosition();

		if (GetNext(_pEntryName))
		{
			int Len = GetUserData();
			_Array.SetLen(Len);
			ReadArray(_Array.GetBasePtr(), _CurrentVersion);
		}
		else
			bRes = false;

		PopPosition();
		return bRes;
	}

	template <class T>
	bool ReadArrayEntry_NoVer(T& _Array, const char* _pEntryName)
	{
		bool bRes = true;
		PushPosition();

		if (GetNext(_pEntryName))
		{
			int Len = GetUserData();
			_Array.SetLen(Len);
			ReadArray_NoVer(_Array.GetBasePtr());
		}
		else
			bRes = false;

		PopPosition();
		return bRes;
	}
};

typedef TPtr<CDataFile> spCDataFile;

#endif // __INC_MDataFile
