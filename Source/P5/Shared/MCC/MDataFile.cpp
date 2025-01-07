
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


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDataFileNode
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CDataFileNode::Read(CCFile* _pFile)
{
	_pFile->Read(&m_NodeName, sizeof(m_NodeName));
	_pFile->ReadLE(m_OffsetNext);
	_pFile->ReadLE(m_OffsetSubDir);
	_pFile->ReadLE(m_Size);
	_pFile->ReadLE(m_UserData);
}

void CDataFileNode::Write(CCFile* _pFile)
{
	_pFile->Write(&m_NodeName, sizeof(m_NodeName));
	_pFile->WriteLE(m_OffsetNext);
	_pFile->WriteLE(m_OffsetSubDir);
	_pFile->WriteLE(m_Size);
	_pFile->WriteLE(m_UserData);
}

// -------------------------------------------------------------------
CDataFileNode2::CDataFileNode2()
{
	FillChar(m_NodeName, sizeof(m_NodeName), 0);
	m_OffsetNext = 0;
	m_OffsetSubDir = 0;
	m_OffsetData = 0;
	m_Size = 0;
	m_UserData = 0;
	m_UserData2 = 0;
}

// URGENTFIXME: These IO functions are not used. Beware of unsafe byte-order IO.

void CDataFileNode2::Read(CCFile* _pFile)
{
	M_STATIC_ASSERT(sizeof(*this) == (sizeof(m_NodeName) + sizeof(m_OffsetNext) + sizeof(m_OffsetSubDir) 
	                                + sizeof(m_OffsetData) + sizeof(m_Size) + sizeof(m_UserData) + sizeof(m_UserData2)));

	_pFile->Read(this, sizeof(*this));
	::SwapLE(m_OffsetNext);
	::SwapLE(m_OffsetSubDir);
	::SwapLE(m_OffsetData);
	::SwapLE(m_Size);
	::SwapLE(m_UserData);
	::SwapLE(m_UserData2);
}

void CDataFileNode2::Write(CCFile* _pFile, bool _bPretend)
{
	if (_bPretend)
	{
		_pFile->RelSeek(sizeof(m_NodeName));
		_pFile->RelSeek(sizeof(m_OffsetNext));
		_pFile->RelSeek(sizeof(m_OffsetSubDir));
		_pFile->RelSeek(sizeof(m_OffsetData));
		_pFile->RelSeek(sizeof(m_Size));
		_pFile->RelSeek(sizeof(m_UserData));
		_pFile->RelSeek(sizeof(m_UserData2));
	}
	else
	{
		_pFile->Write(&m_NodeName, sizeof(m_NodeName));
		_pFile->WriteLE(m_OffsetNext);
		_pFile->WriteLE(m_OffsetSubDir);
		_pFile->WriteLE(m_OffsetData);
		_pFile->WriteLE(m_Size);
		_pFile->WriteLE(m_UserData);
		_pFile->WriteLE(m_UserData2);
	}
}

//
//IMPLEMENT_OPERATOR_NEW(CDF_Node);
//

CDF_Node::CDF_Node()
{
	m_iNodePrev = 0;
	m_iNodeParent = 0;
	m_iNodeSubDir = 0;
	m_iNodeNext = 0;
}

CDF_Node::~CDF_Node()
{
	/* Using indices instead of Smartpointers, no need to destroy references

	CDF_Node* pTail = m_spNodeNext;
	if (!pTail) return;

	while(pTail->m_spNodeNext != NULL)
		pTail = pTail->m_spNodeNext;

	while(pTail && (pTail != this))
	{
		CDF_Node* pPrev = pTail->m_pNodePrev;
		if (!pPrev) break;
		pPrev->m_spNodeNext = NULL;
		pTail = pPrev;
	}
	//*/
};

const CDF_Node & CDF_Node::operator = (const CDF_Node & _Node)
{
	memcpy(&m_Node,&_Node.m_Node,sizeof(CDataFileNode2));
	m_FileOffset = _Node.m_FileOffset;
	m_iNodeNext = _Node.m_iNodeNext;
	m_iNodePrev = _Node.m_iNodePrev;
	m_iNodeParent = _Node.m_iNodeParent;
	m_iNodeSubDir = _Node.m_iNodeSubDir;
	m_lData = _Node.m_lData;
	return * this;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDataFile
|__________________________________________________________________________________________________
\*************************************************************************************************/

CDF_Node* CDF_Node::AllocNode()
{
	return MNew(CDF_Node);

/*
	return new(m_NodesPool) CDF_Node;
*/
}


/*
int CDF_Node::GetNodeCount()
{
	int nNodes = 1;
	if (m_iNodeNext != NULL)
		nNodes += m_spNodeNext->GetNodeCount();
	if (m_spNodeSubDir != NULL)
		nNodes += m_spNodeSubDir->GetNodeCount();

	return nNodes; 
}
//*/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Count nodes without allocating space and reading them

Returns:	Amount of nodes found in file

Comments:	-
\*____________________________________________________________________*/ 
uint32 CDataFile::CountFileNodes_r()
{
	CDataFileNode2	Node;
	uint32 Count = 0;

	while(1)
	{
		Node.Read(m_spFile);
		Count++;

		//Recurse...
		if( Node.m_OffsetSubDir )
		{
			m_spFile->Seek(Node.m_OffsetSubDir);
			Count += CountFileNodes_r();
		}

		//Next...
		if( Node.m_OffsetNext )
		{
			m_spFile->Seek(Node.m_OffsetNext);
		}
		else
		{
			break;
		}
	}

	return Count;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Reads the entire helper tree		

Parameters:		
_pFile:		File to read from (leave NULL for default)

Comments:	Added to help reading of sorted and non-sorted files
\*____________________________________________________________________*/ 
void CDataFile::ReadTree(CCFile * _pFile /* = NULL */)
{
	if(!_pFile) _pFile = m_spFile;

	m_RootOffset = _pFile->Pos();

	// Check version. This will crash if we encounter a version 1 datafile with only the header,
	// But we probably won't.
	CDataFileNode2 Node;
	Node.Read(_pFile);

	if (CStrBase::stricmp("MOS DATAFILE1.0", Node.m_NodeName) == 0)
	{
		m_Version = 1;
		_pFile->RelSeek(sizeof(CDataFileNode) - sizeof(CDataFileNode2));
		return;
	}
	else if (CStrBase::stricmp("MOS DATAFILE2.0", Node.m_NodeName) == 0)
	{
		_pFile->Seek(m_RootOffset);
		m_Version = CDATAFILE_VERSION;
		uint32 nNodes = Node.m_UserData;

		if( !nNodes )
		{
			uint32 iStartNode = 0;
			if( m_spFile == _pFile )
			{
				nNodes = CountFileNodes_r();
				_pFile->Seek(m_RootOffset);
				m_lNodes.SetLen(nNodes);
				ReadTree_r(iStartNode,0);
			}
			else
			{
				spCCFile spTemp = m_spFile;
				m_spFile = _pFile;
				nNodes = CountFileNodes_r();
				_pFile->Seek(m_RootOffset);
				m_lNodes.SetLen(nNodes);
				ReadTree_r(iStartNode,0);
				m_spFile = spTemp;
			}
		}
		else
		{
			//Read the entire array into a cache
			TThinArray<uint8>	lData;
			lData.SetLen(nNodes * sizeof(CDataFileNode2));
			_pFile->Read(lData.GetBasePtr(),nNodes * sizeof(CDataFileNode2));
			CCFile DataStream;
			DataStream.Open(lData.GetBasePtr(),lData.Len(),lData.Len(),CFILE_READ | CFILE_BINARY);

			//Read the nodes
			m_lNodes.SetLen(nNodes);
			for(uint32 i = 0;i < nNodes;i++)
			{
				CDF_Node & DFNode = m_lNodes[i];
				DFNode.m_Node.Read(&DataStream);
				DFNode.m_FileOffset = m_RootOffset + i * sizeof(CDataFileNode2);

				DFNode.m_iNodeNext = (DFNode.m_Node.m_OffsetNext - m_RootOffset) / sizeof(CDataFileNode2);
				DFNode.m_iNodeSubDir = (DFNode.m_Node.m_OffsetSubDir - m_RootOffset) / sizeof(CDataFileNode2);
				if(DFNode.m_iNodeNext) 
				{
					m_lNodes[DFNode.m_iNodeNext].m_iNodePrev = i;
					m_lNodes[DFNode.m_iNodeNext].m_iNodeParent = DFNode.m_iNodeParent;
				}
				if(DFNode.m_iNodeSubDir) m_lNodes[DFNode.m_iNodeSubDir].m_iNodeParent = i;
			}

			DataStream.Close();
		}

		_pFile->Seek(m_RootOffset);
	}
	else
	{
		Error("Open", "Not a valid datafile. (" + _pFile->GetFileName() + ")");
	}
}


void CDataFile::ReadTree_r(uint32 & _iNode, uint32 _iParent)
{
	while(1)
	{
		int Pos = m_spFile->Pos();
		CDF_Node & Node = m_lNodes[_iNode];
		Node.m_FileOffset = Pos;
		Node.m_Node.Read(m_spFile);
//		_pNode->m_Name.Capture(_pNode->m_Node.m_NodeName);
		Node.m_iNodeParent = _iParent;

		Node.m_iNodeSubDir = 0;
		Node.m_iNodeNext = 0;

//		M_TRACE("Read %s, Pos %d, Data %d, Next %d, SubDir %d\n", _pNode->m_Node.m_NodeName, Pos, _pNode->m_Node.m_OffsetData, _pNode->m_Node.m_OffsetNext, _pNode->m_Node.m_OffsetSubDir);

		uint32 iThisNode = _iNode;
		if (Node.m_Node.m_OffsetSubDir)
		{
			_iNode++;
			Node.m_iNodeSubDir = _iNode;
			m_spFile->Seek(Node.m_Node.m_OffsetSubDir);
			ReadTree_r(_iNode,iThisNode);
		}

		if (Node.m_Node.m_OffsetNext)
		{
			_iNode++;
			Node.m_iNodeNext = _iNode;
			m_lNodes[_iNode].m_iNodePrev = iThisNode;
			m_spFile->Seek(Node.m_Node.m_OffsetNext);
		}
		else
			break;
	}
}

void CDataFile::WriteTreeHelperPretend_r(uint32 _iNode, int &_MaxPos, CCFile * _pFile)
{
	if( !_pFile ) _pFile = GetFile();
	while (1)
	{
		CDF_Node & Node = m_lNodes[_iNode];
		int Pos = _pFile->Pos();
		
		if (m_Version == 3)
		{
			Node.m_Node.m_OffsetData = m_CreateDataPos;
			
			m_CreateDataPos += Node.m_Node.m_Size;
		}
		
		Node.m_Node.Write(_pFile, true);

		Pos = _pFile->Pos();
		if (Pos > _MaxPos)
			_MaxPos = Pos;
		
		if (Node.m_iNodeSubDir != 0)
		{
			Node.m_Node.m_OffsetSubDir = _pFile->Pos();
			WriteTreeHelperPretend_r(Node.m_iNodeSubDir, _MaxPos,_pFile);
		}

		if (Node.m_iNodeNext != 0)
		{
			Node.m_Node.m_OffsetNext = _pFile->Pos();
			_iNode = Node.m_iNodeNext;
		}
		else
			break;
	}
}

void CDataFile::WriteTreeHelper_r(uint32 _iNode, CCFile *_pFile)
{
	while (1)
	{
		CDF_Node & Node = m_lNodes[_iNode];
		Node.m_Node.Write(_pFile, false);
		
		if (Node.m_iNodeSubDir != 0)
		{
			WriteTreeHelper_r(Node.m_iNodeSubDir, _pFile);
		}
		
		if (Node.m_iNodeNext != 0)
		{
			_iNode = Node.m_iNodeNext;
		}
		else
			break;
	}
}

void CDataFile::WriteTreeHelper(uint32 _iNode, CCFile *_pFile)
{
	if( !_pFile ) _pFile = GetFile();

	int StartPos = _pFile->Pos();
	int MaxPos = StartPos;
	// Pretend to get the positions
	WriteTreeHelperPretend_r(_iNode, MaxPos,_pFile);

	TArray<uint8> lData;
	lData.SetLen(MaxPos - StartPos);

	CCFile TempFile;
	TempFile.Open(lData.GetBasePtr(), 0, lData.Len(), CFILE_WRITE|CFILE_BINARY);

	// Write in memory file for speed
	WriteTreeHelper_r(_iNode, &TempFile);

	// Save to disk
	_pFile->Seek(StartPos);
	_pFile->Write(lData.GetBasePtr(), lData.Len());

	
}

void CDataFile::WriteTree_r(uint32 _iNode)
{
	WriteTreeHelper(_iNode);
	/*
	while (1)
	{
		int Pos = m_spFile->Pos();
		
		if (m_Version == 3)
		{
			_pNode->m_Node.m_OffsetData = m_CreateDataPos;
			
			m_CreateDataPos += _pNode->m_Node.m_Size;
		}
		
		_pNode->m_Node.Write(m_spFile, true);
		
		if (_pNode->m_spNodeSubDir != NULL)
		{
			_pNode->m_Node.m_OffsetSubDir = m_spFile->Pos();
			WriteTree_r(_pNode->m_spNodeSubDir);
		}
		if (_pNode->m_spNodeNext != NULL)
		{
			_pNode->m_Node.m_OffsetNext= m_spFile->Pos();
//			WriteTree_r(_pNode->m_spNodeNext);
		}
		
		int NewPos = m_spFile->Pos();
		m_spFile->Seek(Pos);
		_pNode->m_Node.Write(m_spFile, false);
//		M_TRACE(CStrF("Write %s, Pos %d, Data %d, Next %d, SubDir %d\n", _pNode->m_Node.m_NodeName, Pos, _pNode->m_Node.m_OffsetData, _pNode->m_Node.m_OffsetNext, _pNode->m_Node.m_OffsetSubDir));
		m_spFile->Seek(NewPos);

		if (_pNode->m_spNodeNext != NULL)
		{
			_pNode = _pNode->m_spNodeNext;
//			WriteTree_r(_pNode->m_spNodeNext);
		}
		else
			break;
	}*/
}

void CDataFile::WriteTreeData_r(uint32 _iNode, CCFile * _pFile )
{
	if( !_pFile ) _pFile = m_spFile;

/*
#if 0
	int Pos = m_spFile->Pos();

	if(_pNode->m_Node.m_Size)
	{
		m_spFile->Seek(_pNode->m_Node.m_OffsetData);
		m_spFile->Write(_pNode->m_lData.GetBasePtr(), _pNode->m_Node.m_Size);
	}

	if (_pNode->m_spNodeNext != NULL)
	{
		WriteTreeData_r(_pNode->m_spNodeNext);
	}
	if (_pNode->m_spNodeSubDir != NULL)
	{
		WriteTreeData_r(_pNode->m_spNodeSubDir);
	}
#else
//*/
	//	int Pos = m_spFile->Pos();
	uint32 iCurrent = _iNode;
	CDF_Node *pCurrentNode;

	do
	{
		pCurrentNode = &m_lNodes[iCurrent];

		if(pCurrentNode->m_Node.m_Size)
		{
			_pFile->Seek(pCurrentNode->m_Node.m_OffsetData);
			_pFile->Write(pCurrentNode->m_lData.GetBasePtr(), pCurrentNode->m_Node.m_Size);
		}

		iCurrent = pCurrentNode->m_iNodeNext;
	}
	while( iCurrent );

	iCurrent = _iNode;

	do
	{
		pCurrentNode = &m_lNodes[iCurrent];

		if (pCurrentNode->m_iNodeSubDir != 0)
		{
			WriteTreeData_r(pCurrentNode->m_iNodeSubDir,_pFile);
		}

		iCurrent = pCurrentNode->m_iNodeNext;
	}
	while( iCurrent );
//#endif

}


IMPLEMENT_OPERATOR_NEW(CDataFile)


CDataFile::CDataFile()
{
	m_iStack = 0;
	m_OpenFlags = 0;

	m_Version = CDATAFILE_VERSION;
	m_bWriteMode = false;
	m_bReadMode = false;
	m_RootOffset = 0;

	m_lStack[m_iStack].m_bNextIsThis = true;
	m_lStack[m_iStack].m_EntryOffset = 0;
	m_lStack[m_iStack].m_SubDirLevel = 0;
}

void CDataFile::Open(const char *_FileName)
{
	m_spFile = MNew(CCFile);
	if (m_spFile == NULL) MemError("Open");

	m_spFile->Open(_FileName, CFILE_READ | CFILE_BINARY);

	ReadTree();
	m_spFile->Seek(0);

	m_RootOffset = 0;
	m_bWriteMode = false;
	m_bReadMode = true;
	m_iStack = 0;
	m_lStack[m_iStack].m_EntryOffset = 0;
	m_lStack[m_iStack].m_SubDirLevel = 0;
	m_lStack[m_iStack].m_bNextIsThis = true;
	m_lStack[m_iStack].m_iNode = 0;
}

void CDataFile::Open(spCCFile _spFile, int _Offset)
{
	m_spFile = _spFile;
	m_Version = CDATAFILE_VERSION;
	m_spFile->Seek(_Offset);
	ReadTree();
	m_spFile->Seek(_Offset);

	m_RootOffset = _Offset;
	m_bWriteMode = false;
	m_bReadMode = true;
	m_iStack = 0;
	m_lStack[m_iStack].m_EntryOffset = 0;
	m_lStack[m_iStack].m_SubDirLevel = 0;
	m_lStack[m_iStack].m_bNextIsThis = true;
	m_lStack[m_iStack].m_iNode = 0;
}

void CDataFile::Open(TArray<uint8> _lFile)
{
	spCCFile spFile = MNew(CCFile);
	spFile->Open(_lFile, CFILE_READ | CFILE_BINARY);
	Open(spFile, 0);
}

void CDataFile::Close()
{
	if (m_bWriteMode)
	{
		if (m_Version == 2)
		{
			int TreePos = m_spFile->Pos();
			if (m_lNodes[0].m_iNodeNext != NULL)
			{
				// Write tree
				WriteTree_r(m_lNodes[0].m_iNodeNext);

				// Fill in the tree-position in the root-node in the beginning of the file.
				int NewPos = m_spFile->Pos();
				m_spFile->Seek(m_RootOffset);
				m_lNodes[0].m_Node.m_OffsetNext = TreePos;
				m_lNodes[0].m_Node.Write(m_spFile, false);
				m_spFile->Seek(NewPos);
			}
		}
		else if (m_Version == CDATAFILE_VERSION)
		{
			m_spFile = m_spCreateFile;
			int nNodes = m_lNodes.Len();
			m_CreateDataPos = nNodes * sizeof(CDataFileNode2);
			m_lNodes[0].m_Node.m_UserData = nNodes;				// Put node count in the user data for the header node
			WriteTree_r(0);
			WriteTreeData_r(0);
		}
		else
			Error("Close", "Invalid version.");
	}

	m_lNodes.Clear();

	if (m_spFile ==NULL) return;
//	m_spFile->Close();
	m_spFile = NULL;

	m_bReadMode = false;
	m_bWriteMode = false;
}

bool CDataFile::HasSubDir()
{
	if (!m_bReadMode) Error("HasSubDir", "Not in read-mode.");

	switch(m_Version)
	{
	case 1 :
		{
			CDataFileNode Node;
			int Pos = m_spFile->Pos();
			m_spFile->Seek(m_lStack[m_iStack].m_EntryOffset);
			Node.Read(m_spFile);
			m_spFile->Seek(Pos);
			return Node.m_OffsetSubDir != 0;
		}
		break;
	default :
		{
			return (m_lNodes[m_lStack[m_iStack].m_iNode].m_Node.m_OffsetSubDir) != 0; //Performance warning with only on != 0. ?
		}
	}
	return false;
}

const char *CDataFile::GetNext()
{
	if (!m_bReadMode) Error("GetNext", "Not in read-mode.");

	switch(m_Version)
	{
	case 1 :
		{
			m_spFile->Seek(m_lStack[m_iStack].m_EntryOffset);
			CDataFileNode& Node = m_DF1TempNode;
			Node.Read(m_spFile);
			if (m_lStack[m_iStack].m_bNextIsThis)
			{
				m_lStack[m_iStack].m_LastUserData = Node.m_UserData;
				m_lStack[m_iStack].m_LastUserData2 = 0;
				m_lStack[m_iStack].m_LastEntrySize = Node.m_Size;
				m_lStack[m_iStack].m_bNextIsThis = false;
				return Node.m_NodeName;
			}

			if (Node.m_OffsetNext == 0) return "";
			m_lStack[m_iStack].m_EntryOffset = Node.m_OffsetNext;
			m_spFile->Seek(m_lStack[m_iStack].m_EntryOffset);
			Node.Read(m_spFile);
			m_lStack[m_iStack].m_LastUserData = Node.m_UserData;
			m_lStack[m_iStack].m_LastUserData2 = 0;
			m_lStack[m_iStack].m_LastEntrySize = Node.m_Size;
			return Node.m_NodeName;
		}
		break;
	default :
		{
			if (m_lStack[m_iStack].m_bNextIsThis)
			{
				m_lStack[m_iStack].m_bNextIsThis = false;
				CDF_Node* pN = &m_lNodes[m_lStack[m_iStack].m_iNode];
				m_lStack[m_iStack].m_LastUserData = pN->m_Node.m_UserData;
				m_lStack[m_iStack].m_LastUserData2 = pN->m_Node.m_UserData2;
				m_lStack[m_iStack].m_LastEntrySize = pN->m_Node.m_Size;
// HIRR!
				m_spFile->Seek(pN->m_Node.m_OffsetData);
				return pN->m_Node.m_NodeName;
			}

			if (m_lNodes[m_lStack[m_iStack].m_iNode].m_iNodeNext != 0)
			{
				m_lStack[m_iStack].m_iNode = m_lNodes[m_lStack[m_iStack].m_iNode].m_iNodeNext;
				CDF_Node & Node = m_lNodes[m_lStack[m_iStack].m_iNode];
// HIRR!
				m_spFile->Seek(Node.m_Node.m_OffsetData);
				m_lStack[m_iStack].m_LastUserData = Node.m_Node.m_UserData;
				m_lStack[m_iStack].m_LastUserData2 = Node.m_Node.m_UserData2;
				m_lStack[m_iStack].m_LastEntrySize = Node.m_Node.m_Size;
				return Node.m_Node.m_NodeName;
			}
			else
				return "";
		}
		break;
	}
	return "";
}

bool CDataFile::GetNext(const char * _EntryName)
{
	if (!m_bReadMode) Error("GetNext", "Not in read-mode.");

	const char * Name = "";
	while (CStrBase::stricmp(_EntryName,Name) != 0)
	{
		Name = GetNext();
		if ((*Name) == 0) return false;
	}
	return true;
}

bool CDataFile::GetSubDir()
{
	if (!m_bReadMode) Error("GetSubDir", "Not in read-mode.");

	switch(m_Version)
	{
	case 1 :
		{
			CDataFileNode Node;
			int Pos = m_spFile->Pos();
			m_spFile->Seek(m_lStack[m_iStack].m_EntryOffset);
			Node.Read(m_spFile);
			if (Node.m_OffsetSubDir == 0) 
			{
				m_spFile->Seek(Pos);
				return false;
			}

			m_lStack[m_iStack].m_bNextIsThis = true;
			m_lStack[m_iStack].m_SubDirOffsets[m_lStack[m_iStack].m_SubDirLevel++] = m_lStack[m_iStack].m_EntryOffset;
			m_lStack[m_iStack].m_EntryOffset = Node.m_OffsetSubDir;
			m_spFile->Seek(m_lStack[m_iStack].m_EntryOffset);
			Node.Read(m_spFile);
			m_lStack[m_iStack].m_LastUserData = Node.m_UserData;
			m_lStack[m_iStack].m_LastUserData2 = 0;
			m_lStack[m_iStack].m_LastEntrySize = Node.m_Size;
			return true;
		}
		break;
	default :
		{
			CDF_Node* pN = &m_lNodes[m_lStack[m_iStack].m_iNode];
			if (pN->m_Node.m_OffsetSubDir)
			{

				// Subdir loaded?
				if (!pN->m_iNodeSubDir)
				{
					m_spFile->Seek(pN->m_Node.m_OffsetSubDir);

					//Count how much more space we'll need and create it
					uint32 nNodesNeeded = CountFileNodes_r();
					uint32 iNewNodeID = m_lNodes.Len();
					m_lNodes.SetLen(iNewNodeID + nNodesNeeded);

					//Read new node
					ReadTree_r(iNewNodeID,m_lStack[m_iStack].m_iNode);
					pN->m_iNodeSubDir = iNewNodeID;
				}

				m_lStack[m_iStack].m_iNode = pN->m_iNodeSubDir;
				pN = &m_lNodes[m_lStack[m_iStack].m_iNode];
				m_spFile->Seek(pN->m_Node.m_OffsetData);
				m_lStack[m_iStack].m_LastUserData = pN->m_Node.m_UserData;
				m_lStack[m_iStack].m_LastUserData2 = pN->m_Node.m_UserData2;
				m_lStack[m_iStack].m_LastEntrySize = pN->m_Node.m_Size;
				return true;
			}
			else
				return false;
		}
		break;
	};
	return false;
}

bool CDataFile::GetParent()
{
	if (!m_bReadMode) Error("GetParent", "Not in read-mode.");

	switch(m_Version)
	{
	case 1 :
		{
			if (m_lStack[m_iStack].m_SubDirLevel <= 0) return 0;
			m_lStack[m_iStack].m_EntryOffset = m_lStack[m_iStack].m_SubDirOffsets[--m_lStack[m_iStack].m_SubDirLevel];
			m_spFile->Seek(m_lStack[m_iStack].m_EntryOffset + sizeof(CDataFileNode));
			m_lStack[m_iStack].m_bNextIsThis = false;
			return true;
		}
		break;
	default :
		{
			if (m_lNodes[m_lStack[m_iStack].m_iNode].m_iNodeParent)
			{
				m_lStack[m_iStack].m_iNode = m_lNodes[m_lStack[m_iStack].m_iNode].m_iNodeParent;
				CDF_Node & Node = m_lNodes[m_lStack[m_iStack].m_iNode];
				m_lStack[m_iStack].m_LastUserData = Node.m_Node.m_UserData;
				m_lStack[m_iStack].m_LastUserData2 = Node.m_Node.m_UserData2;
				m_lStack[m_iStack].m_LastEntrySize = Node.m_Node.m_Size;
// HIRR!
				m_spFile->Seek(m_lNodes[m_lStack[m_iStack].m_iNode].m_Node.m_OffsetData);
				return true;
			}
			else
				return false;
		}
		break;
	}
	return false;
}

bool CDataFile::GetDirectory(CStr _Path)
{
	if (!m_bReadMode) Error("GetDirectory", "Not in read-mode.");

	Error("GetDirectory", "Not implemented.");

	int Pos = m_spFile->Pos();
	int OldEntry = m_lStack[m_iStack].m_EntryOffset;
	int OldLevel = m_lStack[m_iStack].m_SubDirLevel;
	bool bNextIsThis = m_lStack[m_iStack].m_bNextIsThis;
	m_lStack[m_iStack].m_bNextIsThis = true;

	if (_Path.Copy(0, 1) == "\\")
	{
		m_lStack[m_iStack].m_EntryOffset = m_RootOffset;
		m_lStack[m_iStack].m_SubDirLevel = 0;
		_Path = _Path.Del(0, 1);
	}

	while (_Path.Len() > 0)
	{
		CStr DirName = _Path.GetStrSep("\\");
		if (DirName == "..")
		{
			if (!GetParent())
			{
				m_spFile->Seek(Pos);
				m_lStack[m_iStack].m_EntryOffset = OldEntry;
				m_lStack[m_iStack].m_SubDirLevel = OldLevel;
				m_lStack[m_iStack].m_bNextIsThis = bNextIsThis;
				return false;
			}
		}
		else
		{
			CFStr ToGet;
			ToGet.Capture(DirName.Str());
			if (!GetNext(ToGet))
			{
				m_spFile->Seek(Pos);
				m_lStack[m_iStack].m_EntryOffset = OldEntry;
				m_lStack[m_iStack].m_SubDirLevel = OldLevel;
				m_lStack[m_iStack].m_bNextIsThis = bNextIsThis;
				return false;
			}
			GetSubDir();
			{
				m_spFile->Seek(Pos);
				m_lStack[m_iStack].m_EntryOffset = OldEntry;
				m_lStack[m_iStack].m_SubDirLevel = OldLevel;
				m_lStack[m_iStack].m_bNextIsThis = bNextIsThis;
				return false;
			}
		}
	}
	return true;
}

spCCFile CDataFile::GetFile()
{
	if (!m_spFile)
		Error("GetFile", "No file.");
	return m_spFile;
}

int32 CDataFile::GetEntryPos()
{
	if (m_bWriteMode)
	{
		if (m_Version == 3)
			return m_spFile->Length();
		else
			return m_spFile->Pos() - m_lNodes[m_lStack[m_iStack].m_iNode].m_Node.m_OffsetData;
	}
	else
		return m_spFile->Pos() - m_lNodes[m_lStack[m_iStack].m_iNode].m_Node.m_OffsetData;
}


uint32 CDataFile::GetEntryDataFileOffset()
{
	return m_lNodes[m_lStack[m_iStack].m_iNode].m_FileOffset;
}

TArray<uint8> CDataFile::ReadEntry()
{
	TArray<uint8> lData;
	lData.SetLen(GetEntrySize());
	GetFile()->Read(lData.GetBasePtr(), lData.Len());
	return lData;
}

void CDataFile::PushPosition()
{
	if (m_iStack >= (CDATAFILE_STACKSIZE-1)) 
		Error("PushPosition", "Stack full.");

	m_lStack[m_iStack+1] = m_lStack[m_iStack];
	m_lStack[m_iStack].m_FilePos = m_spFile->Pos();
	m_iStack++;
}

void CDataFile::PopPosition()
{
	if (m_iStack <= 0)	
		Error("PopPosition", "Stack empty.");

	m_iStack--;
	m_spFile->Seek(m_lStack[m_iStack].m_FilePos);
}

// -------------------------------------------------------------------
void CDataFile::Create(spCCFile _spFile, int _Version, int _Flags)
{
	m_OpenFlags = _Flags;
	m_Version = _Version;

	m_RootOffset = 0;
	m_iStack = 0;
	m_lStack[m_iStack].m_EntryOffset = -1;
	m_lStack[m_iStack].m_SubDirLevel = 0;
	m_lStack[m_iStack].m_iNode = 0;
	m_bWriteMode = true;
	m_bReadMode = false;

	m_lNodes.Clear();
	m_lNodes.SetGrow(256);
	CDF_Node Root;
	strcpy(Root.m_Node.m_NodeName, "MOS DATAFILE2.0");
	m_lNodes.Add(Root);
	
	if (m_Version == 3)
	{
		m_spCreateFile = _spFile;

		m_spFile = MNew(CCFile);
		if (m_spFile == NULL) MemError("Open");
		m_spFile->Open(&m_CreateStream, CFILE_WRITE | CFILE_BINARY | (m_OpenFlags & EDataFileFlag_NoHidePos) ? 0 : CFILE_HIDEPOS);
	}
	else if (m_Version == 2)
	{
		m_spFile = _spFile;
		WriteTree_r(0);
	}
	else
		Error("Create", "Invalid version.");
}

void CDataFile::Create(CStr _FileName, int _Version, ECompressTypes _eType, ESettings _eSet, int _Flags)
{
	spCCFile spFile = CDiskUtil::CreateCCFile(_FileName, CFILE_WRITE | CFILE_BINARY | CFILE_TRUNC);
	Create(spFile, _Version, _Flags);
}

void CDataFile::Create(TArray<uint8> _lFile, int _Version, int _Flags)
{
	spCCFile spFile = MNew(CCFile);
	spFile->Open(_lFile, CFILE_WRITE | CFILE_BINARY | CFILE_TRUNC);
	Create(spFile, _Version, _Flags);
}

void CDataFile::BeginEntry(CStr _EntryName)
{
	if (!m_bWriteMode) Error("BeginEntry", "Not in write-mode.");
	if (_EntryName == "")
		Error("BeginSubDir", "Empty node name not allowed.");

	switch(m_Version)
	{
	case 1 :
		{
			CDataFileNode Node;
			int Pos = m_spFile->Pos();

			if (m_lStack[m_iStack].m_EntryOffset != -1)
			{
				m_spFile->Seek(m_lStack[m_iStack].m_EntryOffset);
				Node.Read(m_spFile);
				Node.m_OffsetNext = Pos;
				m_spFile->Seek(m_lStack[m_iStack].m_EntryOffset);
				Node.Write(m_spFile);
			}

			FillChar(&Node, sizeof(CDataFileNode), 0);
			int l = _EntryName.Len();
			if ((l > 15) || (l <= 0)) Error("BeginEntry", "Invalid entry name: " + _EntryName);
			strcpy(Node.m_NodeName, _EntryName);
			Node.m_OffsetNext = 0;
			Node.m_OffsetSubDir = 0;
			Node.m_Size = 0;
			Node.m_UserData = 0;
			m_spFile->Seek(Pos);
			Node.Write(m_spFile);

			m_lStack[m_iStack].m_EntryOffset = Pos;
		}
		break;

	case 2 :
		{
			uint32 iNewNode = m_lNodes.Len();

			CDF_Node Node;
			
			int l = _EntryName.Len();
			if ((l > 23) || (l <= 0)) Error("BeginEntry", "Invalid entry name: " + _EntryName);
			strcpy(Node.m_Node.m_NodeName, _EntryName);

			int Pos = m_spFile->Pos();
			Node.m_Node.m_OffsetData = Pos;		// <- We calculate offset when we actually write the data to the disk
			m_lNodes[m_lStack[m_iStack].m_iNode].m_iNodeNext = iNewNode;
			Node.m_iNodePrev = m_lStack[m_iStack].m_iNode;
			Node.m_iNodeParent = m_lNodes[m_lStack[m_iStack].m_iNode].m_iNodeParent;
			m_lStack[m_iStack].m_iNode = iNewNode;

			m_lNodes.Add(Node);
			m_lNodes.SetGrow(Max(256, m_lNodes.Len() >> 1));
		}
		break;

	case 3 :
		{
			uint32 iNewNode = m_lNodes.Len();

			CDF_Node Node;

			int l = _EntryName.Len();
			if ((l > 23) || (l <= 0)) Error("BeginEntry", "Invalid entry name: " + _EntryName);
			strcpy(Node.m_Node.m_NodeName, _EntryName);

			Node.m_Node.m_OffsetData = 0;
			m_lNodes[m_lStack[m_iStack].m_iNode].m_iNodeNext = iNewNode;
			Node.m_iNodePrev = m_lStack[m_iStack].m_iNode;
			Node.m_iNodeParent = m_lNodes[m_lStack[m_iStack].m_iNode].m_iNodeParent;
			m_lStack[m_iStack].m_iNode = iNewNode;

			Node.m_lData.Destroy();
			Node.m_lData.Init();
			m_CreateStream.Open(Node.m_lData, CFILE_WRITE | CFILE_BINARY | (m_OpenFlags & EDataFileFlag_NoHidePos) ? 0 : CFILE_HIDEPOS);

			m_lNodes.Add(Node);
			m_lNodes.SetGrow(Max(256, m_lNodes.Len() >> 1));
		}
		break;
	}
}

void CDataFile::EndEntry(int32 _UserData, int32 _UserData2)
{
	if (!m_bWriteMode) Error("EndEntry", "Not in write-mode.");

	switch(m_Version)
	{
	case 1 :
		{
			int Pos = m_spFile->Pos();
			if (m_lStack[m_iStack].m_EntryOffset == -1) Error("EndEntry", "Bad use of CDataFile.");
			CDataFileNode Node;
			m_spFile->Seek(m_lStack[m_iStack].m_EntryOffset);
			Node.Read(m_spFile);
			Node.m_Size = Pos - m_lStack[m_iStack].m_EntryOffset - sizeof(CDataFileNode);
			Node.m_UserData = _UserData;
			m_spFile->Seek(m_lStack[m_iStack].m_EntryOffset);
			Node.Write(m_spFile);

			m_spFile->Seek(Pos);
		}
		break;

	case 2 :
		{
			int Pos = m_spFile->Pos();
			CDF_Node* pN = & m_lNodes[m_lStack[m_iStack].m_iNode];
			pN->m_Node.m_Size = Pos - pN->m_Node.m_OffsetData;
			pN->m_Node.m_UserData = _UserData;
			pN->m_Node.m_UserData2 = _UserData2;
			pN->m_lData.OptimizeMemory();
		}
		break;

	case 3 :
		{
			CDF_Node* pN = & m_lNodes[m_lStack[m_iStack].m_iNode];
			pN->m_Node.m_Size = m_spFile->Length();
			pN->m_Node.m_UserData = _UserData;
			pN->m_Node.m_UserData2 = _UserData2;
			m_CreateStream.Close();
			pN->m_lData.OptimizeMemory();

//			LogFile(CStrF("    Node %d, %d, %s", pN->m_Node.m_Size, pN->m_lData.Len(), &pN->m_Node.m_NodeName[0]));
		}
		break;
	}
}

void CDataFile::BeginSubDir()
{
	if (!m_bWriteMode) Error("BeginSubDir", "Not in write-mode.");
	if (m_lStack[m_iStack].m_SubDirLevel >= CDATAFILE_MAXDEPTH)
		Error("BeginSubDir", "Directory tree is too deep.");

	switch(m_Version)
	{
	case 1 :
		{
			int Pos = m_spFile->Pos();
			CDataFileNode Node;
			m_spFile->Seek(m_lStack[m_iStack].m_EntryOffset);
			Node.Read(m_spFile);
			Node.m_OffsetSubDir = Pos;
			m_spFile->Seek(m_lStack[m_iStack].m_EntryOffset);
			Node.Write(m_spFile);

			m_lStack[m_iStack].m_SubDirOffsets[m_lStack[m_iStack].m_SubDirLevel++] = m_lStack[m_iStack].m_EntryOffset;

			m_spFile->Seek(Pos);
			m_lStack[m_iStack].m_EntryOffset = -1;

			BeginEntry("DIRECTORYHEADER");
			EndEntry(0);
		}
		break;
	default :
		{
			uint32 iNewNode = m_lNodes.Len();
			m_lNodes.SetLen(iNewNode+1);

			CDF_Node & Node = m_lNodes[iNewNode];
			strcpy(Node.m_Node.m_NodeName,"DIRECTORYHEADER");
			Node.m_iNodeParent = m_lStack[m_iStack].m_iNode;
			m_lNodes[m_lStack[m_iStack].m_iNode].m_iNodeSubDir = iNewNode;
			m_lStack[m_iStack].m_iNode = iNewNode;
		}
		break;
	}
}

void CDataFile::EndSubDir()
{
	if (!m_bWriteMode) Error("EndSubDir", "Not in write-mode.");

	switch(m_Version)
	{
	case 1 :
		{
			if (m_lStack[m_iStack].m_SubDirLevel <= 0)
				Error("EndSubDir", "Already at the root level.");
			m_lStack[m_iStack].m_EntryOffset = m_lStack[m_iStack].m_SubDirOffsets[--m_lStack[m_iStack].m_SubDirLevel];
		}
		break;
	default :
		{
			if (!m_lNodes[m_lStack[m_iStack].m_iNode].m_iNodeParent) Error("EndSubDir", "No parent.");
			m_lStack[m_iStack].m_iNode = m_lNodes[m_lStack[m_iStack].m_iNode].m_iNodeParent;
		}
		break;
	}
}

#ifndef M_RTM

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Read all data in the file

Parameters:		
_pNode:		Node to start from

Comments:	-
\*____________________________________________________________________*/ 
void CDataFile::ReadAll_r(uint32 _iNode)
{
	uint32 iItr = _iNode;

	do
	{
		CDF_Node & Node = m_lNodes[iItr];
		if( Node.m_Node.m_Size )
		{
			Node.m_lData.SetLen(Node.m_Node.m_Size);
			m_spFile->Seek(Node.m_Node.m_OffsetData);
			m_spFile->Read(Node.m_lData.GetBasePtr(),Node.m_Node.m_Size);
		}
		iItr = Node.m_iNodeNext;
	}
	while( iItr );

	iItr = _iNode;

	do
	{
		CDF_Node & Node = m_lNodes[iItr];
		if( Node.m_iNodeSubDir )
		{
			ReadAll_r(Node.m_iNodeSubDir);
		}
		iItr = Node.m_iNodeNext;
	}
	while( iItr );
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Creates a new version of the datafile where the header is guaranteed to be first

Parameters:		
_spInFile:	Input file
_spOutFile:	Output file
_InOffset:	Offset in input file stream
_OutOffset: Offset in output file stream

Returns:	true if succeeded, false if the infile was already in correct order

Comments:	when using version 2 files, the header is put last- drastically decreasing performance when loading.
			This function can be called to "fix" this problem
\*____________________________________________________________________*/ 
bool CDataFile::PutHeaderFirst(spCCFile _spInFile, spCCFile _spOutFile, int _InOffset, int _OutOffset)
{
	Open(_spInFile,_InOffset);
	_spOutFile->Seek(_OutOffset);

	int nNodes = m_lNodes.Len();

	// If this was tagged, we already have order
	if( (nNodes > 0) && (m_lNodes[0].m_Node.m_UserData > 0) )
	{
		return false;
	}

	//Get nodecount
	m_lNodes[0].m_Node.m_UserData = nNodes;

	//Read data
	ReadAll_r(0);

	//Setup for data writing
	m_Version = 3;
	m_CreateDataPos = nNodes * sizeof(CDataFileNode2);

	//Write
	WriteTreeHelper(0,_spOutFile);
	WriteTreeData_r(0,_spOutFile);

	return true;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Creates a new version of the datafile where the header is guaranteed to be first

Parameters:		
_pInFile:	Input filename
_pOutFile:	Output filename
_InOffset:	Offset in input file stream
_OutOffset: Offset in output file stream

Returns:	true if succeeded, false if failed.

Comments:	like the other function, this will return false on failure
\*____________________________________________________________________*/ 
bool CDataFile::PutHeaderFirst(const char *_pInFile,const char *_pOutFile, int _InOffset, int _OutOffset)
{
	spCCFile spInFile = MNew(CCFile),
			 spOutFile = MNew(CCFile);
	if ((spInFile == NULL) || (spOutFile == NULL)) MemError("PutHeaderFirst");

	spInFile->Open(_pInFile, CFILE_READ | CFILE_BINARY);
	spOutFile->Open(_pOutFile, CFILE_WRITE | CFILE_BINARY);

	bool bRet = PutHeaderFirst(spInFile,spOutFile,_InOffset,_OutOffset);

	spOutFile->Close();
	spInFile->Close();

	if( !bRet ) CDiskUtil::DelFile(_pOutFile);

	return bRet;
}

#endif
