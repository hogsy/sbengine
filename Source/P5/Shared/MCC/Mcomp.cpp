
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments

		051011:		Added ZLIB compression option //Anders Ekermo
\*_____________________________________________________________________________________________*/


#include "PCH.h"
#include "MComp.h"
#include "MFloat.h"

////////////////////////////
// TPriorityQueue //
////////////////////////////
template <class T>
void TPriorityQueue<T>::Clear()
{
	m_pRightMostChild=NULL;
	TSearchTree<TQueueElem<T>,0>::Clear();
}

template <class T>
void TPriorityQueue<T>::Put(const T& _Element, uint32 _Priority)
{
	TQueueElem<T> qe;
	qe.m_Priority = _Priority;
	qe.m_Element = _Element;

	TTNode<TQueueElem<T>,2>* pNewChild = &Insert(qe);

	// Has the new child become the rightmost child?
	if (!m_pRightMostChild)
		m_pRightMostChild = pNewChild;
	else
		if (_Priority > m_pRightMostChild->GetElement().m_Priority)
			m_pRightMostChild = pNewChild;

}

template <class T>
T TPriorityQueue<T>::Get()
{
	if (TPriorityQueue<T>::Tree==NULL) Error("Get","Queue is empty.");

	TTNode<TQueueElem<T>,2>* pChild = m_pRightMostChild;

	T Element = pChild->GetElement().m_Element;

	// Update the rightmost child pointer.
	if (pChild->GetChild(0)) 
	{
		if (pChild->GetParent()) 
		{
			(pChild->GetParent())->SetChild(1,pChild->GetChild(0));
			(pChild->GetChild(0))->SetParent(pChild->GetParent());

			m_pRightMostChild = pChild->GetChild(0);
		}
		else 
		{
			TPriorityQueue<T>::Tree = pChild->GetChild(0);
			TPriorityQueue<T>::Tree->SetParent(NULL);
			m_pRightMostChild = TPriorityQueue<T>::Tree;
		}

		while (m_pRightMostChild->GetChild(1))
			m_pRightMostChild = m_pRightMostChild->GetChild(1);

		pChild->SetChild(0,NULL);	// Make sure we don't delete its left children.
	}
	else 
	{
		if (pChild->GetParent() != NULL) 
		{
			m_pRightMostChild = pChild->GetParent();
			(pChild->GetParent())->SetChild(1,NULL);
		}
		else 
		{
			m_pRightMostChild=NULL;
			TPriorityQueue<T>::Tree=NULL;
		}
	}

	delete pChild;
	return Element;
}

//////////////////
// CHuffmanTree //
//////////////////

void CHuffmanTree::CalculateFrequencies(void *_pSource,uint32 _Len, uint8 _InputSize)
{
	if (m_pFrequencies==NULL) Error("CalculateFrequencies","There's no frequency table.");

	uint8* pSrc=(uint8*)_pSource;

	if (m_InputSize == 8) 
	{ 
		memset(m_pFrequencies,0,sizeof(uint32)*256);

		for (uint32 i = 0; i < _Len; i++)
			m_pFrequencies[*(pSrc++)]++;
	}
	else 
	{
		memset(m_pFrequencies,0,sizeof(uint32)*((mint)1 << _InputSize));

		CInputBits<uint32> Input(pSrc);

		uint32 Length=(uint32)M_Ceil((float)(_Len << 3)/(float)_InputSize);
		for (uint32 i = 0; i < Length; i++)
			m_pFrequencies[Input.Get(_InputSize)]++;
	}
}

// Used by the Huffman encoder.
// Memory requirements: 28*(2^(InputSize+1) + 2^InputSize - 1) + 46*2^InputSize.
// For normal input size (8-bit) this becomes appr. 32.5kb
void CHuffmanTree::Build(void *_pSource, uint32 _Len, uint8 _InputSize)
{
	if (m_pCodes!=NULL) delete[] m_pCodes;

	m_InputSize = _InputSize;

	TTNode<CHMBuildElem,2>* pNewNode=NULL;

	TPriorityQueue<TTNode<CHMBuildElem,2>*> pq;

	uint8* pCode=NULL;

	M_TRY
	{
		if (m_pFrequencies) 
			delete[] m_pFrequencies;

		m_pFrequencies = DNew(uint32) uint32[(mint)1 << _InputSize];

		if (!m_pFrequencies) 
			MemError("Build");

		CalculateFrequencies(_pSource,_Len,_InputSize);

		if (m_pLeafs!=NULL) 
			delete[] m_pLeafs;

		{
			typedef TTNode<CHMBuildElem,2>* CTemp;
			m_pLeafs = DNew(CTemp) CTemp[(mint)1 << _InputSize];
		}

		if (!m_pLeafs) MemError("Build");

		m_NrLeafs = 1 << _InputSize;

		typedef TTNode<CHMBuildElem,2> CTemp;

		// Create the leafs of the Huffman tree
		// and put them in the priority queue.
		CHMBuildElem he;
		he.m_Character = 0;
		he.m_Frequency = 0;
		m_NrCharacters = 0;
		int32 i;
		for (i=0; i<(int32)m_NrLeafs; i++)
		{
			if (m_pFrequencies[i]>0) 
			{
				he.m_Character = i;
				he.m_Frequency = m_pFrequencies[i];
				{
					m_pLeafs[i]=DNew(CTemp) CTemp(he,NULL);
				}
				if (m_pLeafs[i]==NULL) MemError("Build");
				pq.Put(m_pLeafs[i],MAX_FREQUENCY-he.m_Frequency);
				m_NrCharacters++;
			}
			else
				m_pLeafs[i]=NULL;
		}

		// Now, build the tree from bottom and up.
		// This will result in each character getting
		// an unique Huffman code.
		TTNode<CHMBuildElem,2> *pLeft,*pRight;

		he.m_Character=0;
		for (i=0; i<(int32)m_NrCharacters-1; i++) 
		{
			pLeft=pq.Get();
			pRight=pq.Get();

			he.m_Frequency = pLeft->GetElement().m_Frequency + pRight->GetElement().m_Frequency;

			pNewNode = DNew(CTemp) CTemp(he,NULL);
			if (!pNewNode) 
				MemError("Build");

			pq.Put(pNewNode,MAX_FREQUENCY-he.m_Frequency);

			pLeft->SetParent(pNewNode);
			pRight->SetParent(pNewNode);

			pNewNode->SetChild(0,pLeft);
			pNewNode->SetChild(1,pRight);
		}

		if (m_NrCharacters == 1) 
		{
			pRight = pq.Get();

			pNewNode = DNew(CTemp) CTemp(he,NULL);
			if (!pNewNode)
				MemError("Build");

			pRight->SetParent(pNewNode);

			pNewNode->SetChild(1,pRight);
		}

		// Create a lookup table for the characters and
		// their codes.
		// Calculate the encoded size at the same time.
		if (m_pCodes) 
			delete[] m_pCodes;

		m_pCodes = DNew(CHMCode) CHMCode[(mint)1 << _InputSize];
		if (m_pCodes==NULL) 
			MemError("Build");

		pCode = DNew(uint8) uint8[(mint)1 << _InputSize];

		if (pCode == NULL) 
			MemError("Build");

		TTNode<CHMBuildElem,2> *pParent;
		TTNode<CHMBuildElem,2> *pChild;

		uint32 NrTotalBits=0;
		int32 j,k;
		for (i=0; i<(int32)(1 << _InputSize); i++) 
		{
			if (m_pLeafs[i]!=NULL) 
			{
				j=0;
				pChild = m_pLeafs[i];
				while ((pParent = pChild->GetParent())!=NULL) 
				{
					if (pParent->GetChild(0) == pChild)
						pCode[j++]=0;
					else
						pCode[j++]=1;

					pChild = pParent;
				}

				if (j > 256) 
					Error("Build","Huffman code too big.");

				m_pCodes[i].m_NrBits = j-1;	// Number of bits - 1 (as we need 256 in an uint8)

				NrTotalBits += j * m_pFrequencies[i];

				if (j-1<16) 
					NrTotalBits += 5+j; 
				else 
					NrTotalBits += 9+j; // For output table.

				// Output the code bit by bit.
				COutputBits<uint32> Output(m_pCodes[i].m_Code,32);
				for (k=j-1; k>=0; k--)
				{
					if (pCode[k]==0)
						Output.Add(0,1);
					else
						Output.Add(1,1);
				}
			}			
		}

		if (m_NrLeafs == m_NrCharacters) 
			NrTotalBits += 9; 
		else 
			NrTotalBits += 9 + m_NrLeafs;

		m_EncodedSize = (NrTotalBits + 7) >> 3;

		delete[] m_pFrequencies;
		m_pFrequencies = NULL;

		delete[] pCode;
		pCode = NULL;

		pq.Clear();

		// Free the tree.
		if (pNewNode != NULL) 
			delete pNewNode;		// Deletes the leafs as well.
	}
	M_CATCH(
	catch (...) 
	{
		// Free the tree.
		while (pq.GetRoot())
			delete pq.Get();

		if (m_pFrequencies!=NULL) delete[] m_pFrequencies;
		if (m_pCodes!=NULL) delete[] m_pCodes;
		if (m_pLeafs!=NULL) delete[] m_pLeafs;
		if (pCode!=NULL) delete[] pCode;

		throw;
	}
	)

}

// Tests if the encoded size is less than Size.
// This calculation is done before the
// encoding has taken place.
uint32 CHuffmanTree::GetEncodedSize()
{
	return m_EncodedSize;
}

// Used by the Huffman decoder.
// Memory requirements: 24*(2^(Original_InputSize+1)-1) + 2^Original_InputSize.
// For normal input size (8-bit) this becomes appr. 12.3 kb
uint8 CHuffmanTree::Rebuild(CInputBits<uint32>& Input)
{
	m_pRoot = NULL;
	uint8* pUsedFlags = NULL;

	uint8 InputSize;

	M_TRY
	{
		CHMRebuildElem he;
		he.m_Character=-1;

		typedef TTNode<CHMRebuildElem,2> CTemp;
		m_pRoot=DNew(CTemp) CTemp(he,NULL);

		uint32 Size;

		// Get input size used when the data was compressed.
		Size = (1 << (InputSize = Input.Get(8)));

		pUsedFlags = DNew(uint8) uint8[Size];
		if (!pUsedFlags) MemError("Rebuild");

		// Is there a table of used-flags?
		uint32 i;
		if (Input.Get(1)==1) {
			for (i=0; i<Size; i++)
				pUsedFlags[i]=Input.Get(1);
		}
		else
			memset(pUsedFlags,1,Size);

		// Now, rebuild the binary tree from the
		// codes of the characters.
		uint8 NrBits;
		uint8 Child;

		TTNode<CHMRebuildElem,2>* pNode;
		TTNode<CHMRebuildElem,2>* pNewNode;

		for (i=0; i<Size; i++) {
			if (pUsedFlags[i]==1) {
				if (Input.Get(1)==0)
					NrBits=Input.Get(4)+1;
				else
					NrBits=Input.Get(8)+1;

				pNode = m_pRoot;
				// Insert code in the tree.
				while (NrBits>0) {
					Child=Input.Get(1);
					pNewNode=pNode->GetChild(Child);
					if (pNewNode==NULL) {
						if (NrBits==1)
							he.m_Character=i;
						else
							he.m_Character=-1;

						pNewNode=DNew(CTemp) CTemp(he,pNode);
						if (pNewNode==NULL) MemError("Rebuild");

						pNode->SetChild(Child,pNewNode);
					}

					pNode = pNewNode;
					NrBits--;
				}

			}
		}

		delete[] pUsedFlags;
	}
	M_CATCH(
	catch (...) 
	{
		if (m_pRoot!=NULL) delete m_pRoot;
		if (pUsedFlags!=NULL) delete[] pUsedFlags;

		throw;
	}
	)

	return InputSize;
}

void CHuffmanTree::OutputCodeTable(COutputBits<uint32>& Output) {

	if (m_pCodes==NULL || m_pLeafs==NULL) Error("OutputCodeTable","There are no codes.");

	uint32 i;

	// Output the input size.
	Output.Add(m_InputSize,8);

	// Do we need a table of used-flags?
	if (m_NrLeafs == m_NrCharacters)
		Output.Add(0,1);		// No flags needed.
	else {
		Output.Add(1,1);		// Flags needed.

		// Output the used-flags.
		for (i=0; i<m_NrLeafs; i++)
			if (m_pLeafs[i]==NULL)
				Output.Add(0,1);
			else
				Output.Add(1,1);
	}

	// Output the codes of all USED characters.
	for (i=0; i<m_NrLeafs; i++)
		if (m_pLeafs[i]!=NULL) {
			int16 NrBits=m_pCodes[i].m_NrBits;

			if (NrBits<16) {
				Output.Add(0,1);		// Code is < 16
				Output.Add(NrBits,4);
			}
			else {
				Output.Add(1,1);		// Code is >=16
				Output.Add(NrBits,8);
			}

			NrBits++;
			if (NrBits<24)
				Output.Add(*(uint32*)(m_pCodes[i].m_Code),NrBits);
			else {
				uint16 Offs=0;
				while (NrBits>0) {
					if (NrBits>=16) {
						Output.Add(*(uint16*)(&m_pCodes[i].m_Code[Offs]),16);
						Offs+=2;
						NrBits-=16;
					}
					else {
						Output.Add(*(uint16*)(&m_pCodes[i].m_Code[Offs]),NrBits);
						NrBits=0;
					}
				}
			}

		}

}

//////////////
// CHuffman //
//////////////

void CHuffman::SetCompress(CCompressSettings& Settings)
{
	if (Settings.GetCompressType()==HUFFMAN)
		m_InputSize=((CHuffmanSettings*)&Settings)->GetInputSize();
	else
		Error("SetCompress","Incompatible Huffman encoding settings.");
}

// Memory requirements: 2*Len + 28*(2^(InputSize+1) + 2^InputSize - 1) + 46*2^InputSize.
// For normal input size (8-bit) this becomes appr. 2*Len + 32.5kb
void* CHuffman::Compress(void *Source, void *Destination, mint Len)
{
	uint8* Src=(uint8*)Source;
	uint8* Dst=(uint8*)Destination;

	uint32 Size=Len;

	if (Dst==NULL) Dst=DNew(uint8) uint8[Size+5+4];
	if (Dst==NULL) MemError("Compress");

	*((uint32*)Dst)=Len;

	// Build the Huffman code tree.
	CHuffmanTree ht;
	ht.Build(Src,Len,m_InputSize);

	// What's best: To compress or to just copy the data?
	// We sub. the code table size from Size as it's
	// not included in the GetEncodedSize method.
	if (ht.GetEncodedSize()<Size) 
	{

		// Output COMPRESSED flag.
		Dst[4]=COMPRESSED;

		COutputBits<uint32> Output((uint8*)Dst+5,Size);

		// Output the Huffman code table.
		ht.OutputCodeTable(Output);

		// Encode the data.
		CHMCode* pCodes=ht.GetCodes();

		if (m_InputSize==8) 
		{

			// Input is 8-bit sized.
			int32 NrBits;
			for (uint32 i=0; i<Len; i++) 
			{

				NrBits=pCodes[*Src].m_NrBits+1;
				if (NrBits<=24)
					Output.Add(*(uint32*)(pCodes[*(Src++)].m_Code),NrBits);
				else 
				{
					uint16 Offs=0;
					while (NrBits>0) 
					{
						if (NrBits>=16) 
						{
							Output.Add(*(uint16*)(&pCodes[*Src].m_Code[Offs]),16);
							Offs+=2;
							NrBits-=16;
						}
						else 
						{
							Output.Add(*(uint16*)(&pCodes[*Src].m_Code[Offs]),NrBits);
							NrBits=0;
						}
					}
					Src++;
				}

			}
		}
		else 
		{
			// Input is not 8-bit sized.
			// We'll have to use a more 
			// general (and slower) approach.
			CInputBits<uint32> Input(Src);

			uint32 Number;

			int32 NrBits;
			uint32 Length=(uint32)M_Ceil((float)(Len << 3)/(float)m_InputSize);
			for (uint32 i=0; i<Length; i++) {
				Number=Input.Get(m_InputSize);

				NrBits=pCodes[Number].m_NrBits+1;
				if (NrBits<=24)
					Output.Add(*(uint32*)(pCodes[Number].m_Code),NrBits);
				else {
					uint16 Offs=0;
					while (NrBits>0) {
						if (NrBits>=16) {
							Output.Add(*(uint16*)(&pCodes[Number].m_Code[Offs]),16);
							Offs+=2;
							NrBits-=16;
						}
						else {
							Output.Add(*(uint16*)(&pCodes[Number].m_Code[Offs]),NrBits);
							NrBits=0;
						}
					}
				}

			}
		}

		m_CompressedLength=Output.GetLength()+5;
	}
	else {
		// Just copy the data. (Huffman encoding failed)
		Dst[4]=NOT_COMPRESSED;

		memcpy(Dst+5,Src,Len);

		m_CompressedLength=Len+5;
	}

	return (void*)Dst;
}

// Memory requirements: encoded_len + decoded_len +
// 24*(2^(Original_InputSize+1)-1) + 2^Original_InputSize.
// For normal input size (8-bit) this becomes appr. encoded_len + decoded_len + 12.3 kb
void* CHuffman::Decompress(void *Source, void *Destination)
{
	uint8* Src=(uint8*)Source;
	uint8* Dst=(uint8*)Destination;

	uint32 Length=GetUncompressedLength(Source);

	if (Dst==NULL) Dst=DNew(uint8) uint8[Length+4];
	if (Dst==NULL) MemError("Decompress");

	uint8* DstCopy=Dst;

	// Is the source really Huffman encoded ur just
	// copied?
	if (Src[4]==COMPRESSED) {

		CInputBits<uint32> Input((uint8*)Src+5);

		// Rebuild the Huffman code tree.
		CHuffmanTree ht;
		uint8 InputSize=ht.Rebuild(Input);

		// Decode the data.
		if (InputSize==8) {
			// 8-bit dependent approach.
			for (uint32 i=0; i<Length; i++)
				*(Dst++)=ht.GetCharacter(Input);
		}
		else {
			// The more general (and slower) approach.
			COutputBits<uint32> Output(Dst,Length+4);

			Length=(uint32)M_Ceil((float)(Length << 3)/(float)InputSize);
			for (uint32 i=0; i<Length; i++)
				Output.Add(ht.GetCharacter(Input),InputSize);
		}

	}
	else {
		// Just copy the data.
		memcpy(Dst,Src+5,Length);
	}

	return (void*)DstCopy;
}

//////////
// CLZW //
//////////

void CLZW::SetCompress(CCompressSettings& Settings)
{
	if (Settings.GetCompressType()==LZW || Settings.GetCompressType()==LZW_GIF) 
	{

		m_MinBits=((CLZWSettings*)&Settings)->GetMinimumBits();
		if (m_MinBits<9 && m_MinBits!=0)
			Error("SetCompress","Minimum code size may not be less than 9.");

		m_MaxBits=((CLZWSettings*)&Settings)->GetMaximumBits();
		if (m_MaxBits<9) Error("SetCompress","Maximum code size may not be less than 9.");
		if (m_MaxBits>15) Error("SetCompress","Maximum code size may not exceed 15.");

		if (m_MinBits>m_MaxBits)
			Error("SetCompress","Minimum code size may not be greater than the maximum code size.");

		m_bUseControlCodes=((CLZWSettings*)&Settings)->GetControlCodesFlag();
	}
	else
		Error("SetCompress","Incompatible LZW-compression settings.");
}

// Memory requirements: 2*Len + 8*2^m_MaxBits
// With m_MaxBits=12 this becomes appr. 2*Len + 32kb
void* CLZW::Compress(void *Source, void *Destination, mint Len)
{
	if (m_bUseControlCodes)
		return CompressWithCC(Source,Destination,Len);

	uint8* Src=(uint8*)Source;
	uint8* Dst=(uint8*)Destination;

	uint8* SrcStart=Src;

	uint32 Size=Len;

	if (Dst==NULL) Dst=DNew(uint8) uint8[Size+6+4];

	uint8** HashTable=DNew(uint8 *) uint8*[((mint)1 << m_MaxBits)];
	uint8* BucketSizes=DNew(uint8) uint8[((mint)1 << m_MaxBits)];

	if (Dst==NULL || HashTable==NULL || BucketSizes==NULL) {
		if (Dst!=NULL && Destination==NULL) delete[] Dst;
		if (HashTable!=NULL) delete[] HashTable;
		if (BucketSizes!=NULL) delete[] BucketSizes;
		MemError("Compress");
	}

	memset(HashTable,0,((mint)1 << m_MaxBits)*sizeof(uint8*));

	// Output uncompressed length.
	*((uint32*)Dst)=Len;

	// Set compressed flag.
	Dst[4]=COMPRESSED;

	// Write maximum code size.
	Dst[5]=m_MaxBits;

	// Initiate some variables.
	uint32 NextCode=(1 << CHARACTER_SIZE);
	uint32 CodeBits;
	if (m_MinBits==0)
		CodeBits=CHARACTER_SIZE+1;
	else
		CodeBits=m_MinBits;
	uint32 Limit=1 << CodeBits;
	uint32 MaxLimit=(1 << m_MaxBits)-1;

	COutputBits<uint32> Output(Dst+6,Size);

	uint32 j;

	uint32 len;
	uint8 Character;
	uint32 PrefixCode=*(Src++);

	uint8* BucketList;

	bool Skip=FALSE;

	int64 NrBitsLeft=Size << 3;

	uint32 i=Len-1;
	while (i>0 && NrBitsLeft>0) {

		// Loop until we have a string that's not
		// in the hash-table.
		do {
			Character=*(Src++);
			i--;
			if ((BucketList=HashTable[PrefixCode])==NULL) break;
			len=BucketSizes[PrefixCode]+1;
			for (j=0; j<len; j++) {
				if (BucketList[j*3]==Character)
					break;
			}
			if (j>=len) break;
			PrefixCode=*(uint16*)(BucketList+j*3+1);
		} while (i>0);

		// Output the prefix code.
		if ((NrBitsLeft-=CodeBits)>0)
			Output.Add(PrefixCode,CodeBits);

		// Time to increase code size?
		// Do that if the last dictionary
		// addition was element (2^CodeBits)-1.
		if (NextCode-1>=Limit-1) {
			Limit<<=1;
			CodeBits++;

			// Time to destroy the dictionary?
			if (CodeBits>m_MaxBits) {

				// Clear hash-table.
				for (j=0; j<=MaxLimit; j++) {
					if (HashTable[j]!=NULL) {
						delete[] HashTable[j];
						HashTable[j]=NULL;
					}
				}

				NextCode=(1 << CHARACTER_SIZE);
				if (m_MinBits==0)
					CodeBits=CHARACTER_SIZE+1;
				else
					CodeBits=m_MinBits;
				Limit=1 << CodeBits;

				Skip=TRUE;
			}
		}

		if (!Skip) {
			// Add a new phrase to the dictionary.
			if (HashTable[PrefixCode]==NULL) {
				BucketSizes[PrefixCode]=0;
				if ((HashTable[PrefixCode]=DNew(uint8) uint8[3])==NULL) MemError("Compress");
				*(HashTable[PrefixCode])=Character;
				*(uint16*)(HashTable[PrefixCode]+1)=NextCode++;
			}
			else {
				BucketList=HashTable[PrefixCode];
				if ((HashTable[PrefixCode]=DNew(uint8) uint8[(BucketSizes[PrefixCode]+2)*3])==NULL)
					MemError("Compress");
				memcpy(HashTable[PrefixCode]+3,BucketList,(BucketSizes[PrefixCode]+1)*3);
				delete[] BucketList;
				*(HashTable[PrefixCode])=Character;
				*(uint16*)(HashTable[PrefixCode]+1)=NextCode++;
				BucketSizes[PrefixCode]++;
			}
		}
		else
			Skip=FALSE;

		PrefixCode=Character;
	}

	if ((NrBitsLeft-=CodeBits)>0)
		Output.Add(PrefixCode,CodeBits);

	delete[] HashTable;
	delete[] BucketSizes;

	// Better to just copy?
	if (NrBitsLeft<0) {
		Dst[4]=NOT_COMPRESSED;

		memcpy(Dst+5,SrcStart,Len);

		m_CompressedLength=Len+5;
	}
	else
		m_CompressedLength=Output.GetLength()+6;

	return (void*)Dst;
}

// Memory requirements: 3*Len + 8*2^m_MaxBits
// With m_MaxBits=12 this becomes appr. 3*Len + 32kb
void* CLZW::CompressWithCC(void *Source, void *Destination, mint Len)
{
	uint8* Src=(uint8*)Source;
	uint8* Dst=(uint8*)Destination;

	uint32 Size=Len*2;	// Enough!?!

	if (Dst==NULL) Dst=DNew(uint8) uint8[Size+4+4];

	uint8** HashTable=DNew(uint8 *) uint8*[((mint)1 << m_MaxBits)];
	uint8* BucketSizes=DNew(uint8) uint8[((mint)1 << m_MaxBits)];

	if (Dst==NULL || HashTable==NULL || BucketSizes==NULL) {
		if (Dst!=NULL && Destination==NULL) delete[] Dst;
		if (HashTable!=NULL) delete[] HashTable;
		if (BucketSizes!=NULL) delete[] BucketSizes;
		MemError("Compress");
	}

	memset(HashTable,0,((mint)1 << m_MaxBits)*sizeof(uint8*));

	// Output uncompressed length.
	*((uint32*)Dst)=Len;

	// Initiate some variables.
	uint32 NextCode=(1 << CHARACTER_SIZE);
	uint32 CodeBits;
	if (m_MinBits==0)
		CodeBits=CHARACTER_SIZE+1;
	else
		CodeBits=m_MinBits;
	uint32 Limit=1 << CodeBits;
	uint32 nMaxLimit=(1 << m_MaxBits)-1;

	uint32 CC_Init, CC_EOI;
	CC_Init=NextCode++;
	CC_EOI=NextCode++;

	COutputBits<uint32> Output((uint32*)Dst+1,Size);

	// Insert the first CC_Init.
	Output.Add(CC_Init,CodeBits);

	uint32 j;

	uint32 len;
	uint8 Character;
	uint32 PrefixCode=*(Src++);

	uint8* BucketList;

	bool Skip=FALSE;

	int64 NrBitsLeft=Size << 3;

	uint32 i=Len-1;
	while (i>0 && NrBitsLeft>0) {

		// Loop until we have a string that's not
		// in the hash-table.
		do {
			Character=*(Src++);
			i--;
			if ((BucketList=HashTable[PrefixCode])==NULL) break;
			len=BucketSizes[PrefixCode]+1;
			for (j=0; j<len; j++) {
				if (BucketList[j*3]==Character)
					break;
			}
			if (j>=len) break;
			PrefixCode=*(uint16*)(BucketList+j*3+1);
		} while (i>0);

		// Output the prefix code.
		if ((NrBitsLeft-=CodeBits)>0)
			Output.Add(PrefixCode,CodeBits);

		// Time to increase code size?
		// Do that if the last dictionary
		// addition was element (2^CodeBits)-1.
		if (NextCode-1>=Limit-1) {
			Limit<<=1;
			CodeBits++;

			// Time to destroy the dictionary?
			if (CodeBits>m_MaxBits) {
				Output.Add(CC_Init,CodeBits-1);

				// Clear hash-table.
				for (j=0; j<=nMaxLimit; j++) {
					if (HashTable[j]!=NULL) {
						delete[] HashTable[j];
						HashTable[j]=NULL;
					}
				}

				NextCode=(1 << CHARACTER_SIZE);
				if (m_MinBits==0)
					CodeBits=CHARACTER_SIZE+1;
				else
					CodeBits=m_MinBits;
				Limit=1 << CodeBits;
				NextCode+=2;

				Skip=TRUE;
			}
		}

		if (!Skip) {
			// Add a new phrase to the dictionary.
			if (HashTable[PrefixCode]==NULL) {
				BucketSizes[PrefixCode]=0;
				if ((HashTable[PrefixCode]=DNew(uint8) uint8[3])==NULL) MemError("Compress");
				*(HashTable[PrefixCode])=Character;
				*(uint16*)(HashTable[PrefixCode]+1)=NextCode++;
			}
			else {
				BucketList=HashTable[PrefixCode];
				if ((HashTable[PrefixCode]=DNew(uint8) uint8[(BucketSizes[PrefixCode]+2)*3])==NULL)
					MemError("Compress");
				memcpy(HashTable[PrefixCode]+3,BucketList,(BucketSizes[PrefixCode]+1)*3);
				delete[] BucketList;
				*(HashTable[PrefixCode])=Character;
				*(uint16*)(HashTable[PrefixCode]+1)=NextCode++;
				BucketSizes[PrefixCode]++;
			}
		}
		else
			Skip=FALSE;

		PrefixCode=Character;
	}

	if ((NrBitsLeft-=CodeBits)>0)
		Output.Add(PrefixCode,CodeBits);

	if ((NrBitsLeft-=CodeBits)>0)
		Output.Add(CC_EOI,CodeBits);

	if (NrBitsLeft<0) Error("CompressWithCC","Compression failed.");

	m_CompressedLength=Output.GetLength()+4;

	delete[] HashTable;
	delete[] BucketSizes;

	return (void*)Dst;
}

/*
void* CLZW::Compress(void *Source, void *Destination, uint32 Len)
{

	uint8* Src=(uint8*)Source;
	uint8* Dst=(uint8*)Destination;

	uint32 Size=Len*2;	// What's sufficient?

	if (Dst==NULL) Dst=DNew(uint8) uint8[Size+4+4];

	TArray<StringEntryComp>* StringTable=
		DNew(TArray<StringEntryComp>) TArray<StringEntryComp>[1 << m_MaxBits];

	if (Dst==NULL || StringTable==NULL) {
		if (Dst!=NULL && Destination==NULL) delete[] Dst;	
		if (StringTable!=NULL) delete[] StringTable;
		MemError("Compress");
	}

	// Output uncompressed length.
	*((uint32*)Dst)=Len;

	// Initiate hash-table.
	uint16 ch;
	StringEntryComp sec;
	for (ch=0; ch<(1 << CHARACTER_SIZE); ch++) {
		sec.Character=ch;
		sec.m_Code=ch;
		sec.PrefixCode=0xffff;	// 0xffff means no prefix.
		(StringTable[ch]).Add(sec);
	}

	// Initiate some variables.
	uint16 NextCode=(1 << CHARACTER_SIZE);
	uint8 CodeBits;
	if (m_MinBits==0)
		CodeBits=CHARACTER_SIZE+1;
	else
		CodeBits=m_MinBits;
	uint16 Limit=1 << CodeBits;

	uint16 MaxLimit=(1 << m_MaxBits)-1;

	uint16 CC_Init, CC_EOI;
	if (m_bUseControlCodes) {
		CC_Init=NextCode++;
		CC_EOI=NextCode++;
	}

	uint16 Character;

	uint16 PrefixCode=0xffff;
	uint32 StringSum=0;

	COutputBits<uint32> Output((uint32*)Dst+1,Size);

	// Insert the first CC_Init (if we're using control codes).
	if (m_bUseControlCodes)
		Output.Add(CC_Init,CodeBits);

	uint32 i;
	for (i=0; i<Len; i++) {
		Character=*(Src++);

		StringSum+=Character;	// StringSum is later used to hash the string.

		bool Exists=FALSE;
		uint16 j;
		for (j=0; j<(StringTable[StringSum & MaxLimit]).m_Len(); j++) {
			if ((StringTable[StringSum & MaxLimit])[j].PrefixCode==PrefixCode &&
				(StringTable[StringSum & MaxLimit])[j].Character==Character) {
				Exists=TRUE;
				break;
			}
		}

		// Does this code exist?
		if (!Exists) {

			// Output the code.
			Output.Add(PrefixCode,CodeBits);

			// Time to increase code size?
			// Do that if the last dictionary
			// addition was element (2^n)-1.
			if (NextCode-1>=Limit-1) {
				Limit<<=1;
				CodeBits++;
			}

			// Time to destroy the dictionary?
			if (CodeBits>m_MaxBits) {
				if (m_bUseControlCodes)
					Output.Add(CC_Init,CodeBits-1);

				for (ch=0; ch<(1 << CHARACTER_SIZE); ch++) {
					sec.Character=ch;
					sec.m_Code=ch;
					sec.PrefixCode=0xffff;
					(StringTable[ch]).Clear();
					(StringTable[ch]).Add(sec);
				}
				for (ch=(1 << CHARACTER_SIZE); ch<=MaxLimit; ch++)
					(StringTable[ch]).Clear();

				NextCode=(1 << CHARACTER_SIZE);
				if (m_MinBits==0)
					CodeBits=CHARACTER_SIZE+1;
				else
					CodeBits=m_MinBits;
				Limit=1 << CodeBits;
				if (m_bUseControlCodes)
					NextCode+=2;
			}
			else {
				// Add a DNew phrase to the dictionary.
				StringEntryComp sec;
				sec.Character=Character;
				sec.PrefixCode=PrefixCode;
				sec.m_Code=NextCode++;
				(StringTable[StringSum & MaxLimit]).Add(sec);
			}

			PrefixCode=Character;
			StringSum=Character;
		}
		else
			PrefixCode=(StringTable[StringSum & MaxLimit])[j].m_Code;

	}

	Output.Add(PrefixCode,CodeBits);

	if (m_bUseControlCodes)
		Output.Add(CC_EOI,CodeBits);

	m_CompressedLength=(uint32)(Output.GetPointer())-(uint32)(Dst);

	delete[] StringTable;

	return (void*)Dst;

}
*/

// Memory requirements: comp_len + decomp_len + 256 + 8*2^m_MaxBits.
// With m_MaxBits=12 this becomes appr. comp_len + decomp_len + 32.25kb
void* CLZW::Decompress(void *Source, void *Destination)
{
	if (m_bUseControlCodes)
		return DecompressWithCC(Source,Destination);

	int64 Length=GetUncompressedLength(Source);

	uint8* Src=(uint8*)Source;
	uint8* Dst=(uint8*)Destination;

	if (Dst==NULL) Dst=DNew(uint8) uint8[Length];
	if (Dst==NULL) MemError("Decompress");

	uint8* DstStart=Dst;

	// Uncompress or just copy?
	if (Src[4]==COMPRESSED) {

		// Write maximum code size.
		uint8 nMaxBits=Src[5];

		uint8* BasicCharacters=DNew(uint8) uint8[1 << CHARACTER_SIZE];
		StringEntryDecomp* StringTable=DNew(StringEntryDecomp) StringEntryDecomp[(mint)1 << nMaxBits];

		if (BasicCharacters==NULL || StringTable==NULL) {
			if (Dst!=NULL && Destination==NULL) delete[] Dst;	
			if (BasicCharacters!=NULL) delete[] BasicCharacters;	
			if (StringTable!=NULL) delete[] StringTable;
			MemError("Decompress");
		}

		// Init string table.
		uint16 ch;
		for (ch=0; ch<(1 << CHARACTER_SIZE); ch++) 
		{
			StringTable[ch].m_pStr=BasicCharacters+ch;
			*(StringTable[ch].m_pStr)=(uint8)ch;
			StringTable[ch].m_Len=1;
		}

		// Initiate some variables.
		uint16 NextCode=(1 << CHARACTER_SIZE);
		uint8 CodeBits;
		if (m_MinBits==0)
			CodeBits=CHARACTER_SIZE+1;
		else
			CodeBits=m_MinBits;
		uint16 Limit=1 << CodeBits;

		uint16 Code, OldCode;

		CInputBits<uint32> Input(Src+6);

		// Get the fist code.
		OldCode=Input.Get(CodeBits);
		*(Dst++)=OldCode;
		Length--;

		while (Length>0) {
			Code=Input.Get(CodeBits);

			// Does this code exist?
			if (Code<NextCode) {
				memcpy(Dst,StringTable[Code].m_pStr,StringTable[Code].m_Len);

				StringTable[NextCode].m_pStr=Dst-StringTable[OldCode].m_Len;
				StringTable[NextCode].m_Len=StringTable[OldCode].m_Len+1;

				Dst+=StringTable[Code].m_Len;
				Length-=StringTable[Code].m_Len;

				NextCode++;

				OldCode=Code;
			}
			else {
				memcpy(Dst,StringTable[OldCode].m_pStr,StringTable[OldCode].m_Len);
				*(Dst+StringTable[OldCode].m_Len)=*(StringTable[OldCode].m_pStr);

				StringTable[NextCode].m_pStr=Dst;
				StringTable[NextCode].m_Len=StringTable[OldCode].m_Len+1;

				Dst+=StringTable[OldCode].m_Len+1;
				Length-=(StringTable[OldCode].m_Len+1);

				NextCode++;

				OldCode=NextCode-1;
			}

			// Time to increase code size?
			if (NextCode-1>=Limit-1 && Length>0) {
				Limit<<=1;
				CodeBits++;

				// Time to destroy dictionary?
				if (CodeBits>nMaxBits) {
					NextCode=(1 << CHARACTER_SIZE);
					if (m_MinBits==0)
						CodeBits=CHARACTER_SIZE+1;
					else
						CodeBits=m_MinBits;
					Limit=1 << CodeBits;

					OldCode=Input.Get(CodeBits);
					*(Dst++)=OldCode;
					Length--;
				}

			}

		}

		delete[] BasicCharacters;
		delete[] StringTable;
	}
	else {
		// Just copy the data.
		memcpy(Dst,Src+5,Length);
	}

	return (void*)DstStart;
}

// This decompressor uses GIF control codes.
//
// Memory requirements: comp_len + decomp_len + 256 + 8*2^m_MaxBits.
// With m_MaxBits=12 this becomes appr. comp_len + decomp_len + 32.25kb
void* CLZW::DecompressWithCC(void *Source, void *Destination)
{
	uint8* Src=(uint8*)Source;
	uint8* Dst=(uint8*)Destination;

	int64 Length=GetUncompressedLength(Source);

	if (Dst==NULL) Dst=DNew(uint8) uint8[Length];
	uint8* BasicCharacters=DNew(uint8) uint8[1 << CHARACTER_SIZE];
	StringEntryDecomp* StringTable=DNew(StringEntryDecomp) StringEntryDecomp[(mint)1 << m_MaxBits];

	if (Dst==NULL || BasicCharacters==NULL || StringTable==NULL) {
		if (Dst!=NULL && Destination==NULL) delete[] Dst;	
		if (BasicCharacters!=NULL) delete[] BasicCharacters;	
		if (StringTable!=NULL) delete[] StringTable;
		MemError("DecompressWithCC");
	}

	uint8* DstStart=Dst;

	// Init string table.
	uint16 ch;
	for (ch=0; ch<(1 << CHARACTER_SIZE); ch++) {
		StringTable[ch].m_pStr=BasicCharacters+ch;
		*(StringTable[ch].m_pStr)=(uint8)ch;
		StringTable[ch].m_Len=1;
	}

	// Initiate some variables.
	uint16 NextCode=(1 << CHARACTER_SIZE);
	uint8 CodeBits;
	if (m_MinBits==0)
		CodeBits=CHARACTER_SIZE+1;
	else
		CodeBits=m_MinBits;
	uint16 Limit=1 << CodeBits;

	uint16 CC_Init, CC_EOI;
	CC_Init=NextCode++;
	CC_EOI=NextCode++;

	uint16 Code, OldCode;

	CInputBits<uint32> Input((uint32*)Src+1);

	// Get the first CC_Init
	Input.Get(CodeBits);

	// Get the fist code.
	OldCode=Input.Get(CodeBits);
	*(Dst++)=OldCode;
	Length--;

	while (Length>0) {
		Code=Input.Get(CodeBits);

		// Time to destroy dictionary?
		if (Code==CC_Init) {
			NextCode=(1 << CHARACTER_SIZE)+2;
			if (m_MinBits==0)
				CodeBits=CHARACTER_SIZE+1;
			else
				CodeBits=m_MinBits;
			Limit=1 << CodeBits;

			OldCode=Input.Get(CodeBits);
			*(Dst++)=OldCode;
			Length--;
		}

		// Does this code exist?
		else if (Code<NextCode) {
			memcpy(Dst,StringTable[Code].m_pStr,StringTable[Code].m_Len);

			StringTable[NextCode].m_pStr=Dst-StringTable[OldCode].m_Len;
			StringTable[NextCode].m_Len=StringTable[OldCode].m_Len+1;

			Dst+=StringTable[Code].m_Len;
			Length-=StringTable[Code].m_Len;

			NextCode++;

			OldCode=Code;
		}
		else {
			memcpy(Dst,StringTable[OldCode].m_pStr,StringTable[OldCode].m_Len);
			*(Dst+StringTable[OldCode].m_Len)=*(StringTable[OldCode].m_pStr);

			StringTable[NextCode].m_pStr=Dst;
			StringTable[NextCode].m_Len=StringTable[OldCode].m_Len+1;

			Dst+=StringTable[OldCode].m_Len+1;
			Length-=(StringTable[OldCode].m_Len+1);

			NextCode++;

			OldCode=NextCode-1;
		}

		// Time to increase code size?
		if (NextCode-1>=Limit-1 && Length>0) {
			Limit<<=1;
			if (CodeBits<m_MaxBits) CodeBits++;
		}

	}

	delete[] BasicCharacters;
	delete[] StringTable;

	return (void*)DstStart;
}

//////////////
// CLZWFast //
//////////////
/*
void CLZWFast::SetCompress(CCompressSettings& Settings)
{
	if (Settings.GetCompressType()==LZW || Settings.GetCompressType()==LZW_GIF) {

		m_MinBits=((CLZWSettings*)&Settings)->GetMinimumBits();
		if (m_MinBits<9 && m_MinBits!=0)
			Error("SetCompress","Minimum code size may not be less than 9.");

		m_MaxBits=((CLZWSettings*)&Settings)->GetMaximumBits();
		if (m_MaxBits<9) Error("SetCompress","Maximum code size may not be less than 9.");
		if (m_MaxBits>15) Error("SetCompress","Maximum code size may not exceed 15.");

		m_bUseControlCodes=((CLZWSettings*)&Settings)->GetControlCodesFlag();

	}
	else
		Error("SetCompress","Incompatible LZW-compression settings.");
}

void* CLZWFast::Compress(void *Source, void *Destination, uint32 Len)
{
	uint8* Src=(uint8*)Source;
	uint8* Dst=(uint8*)Destination;

	uint32 Size=Len*2;	// What's sufficient?

	if (Dst==NULL) Dst=DNew(uint8) uint8[Size+4+4];
	LZWNode* LZWTree=DNew(LZWNode) LZWNode[1 << m_MaxBits];
	if (Dst==NULL || LZWTree==NULL) {
		if (Dst!=NULL && Destination==NULL) delete[] Dst;	
		if (LZWTree!=NULL) delete[] LZWTree;
		MemError("Compress");
	}

	// Output uncompressed length.
	*((uint32*)Dst)=Len;

	// Initiate the tree.
	memset(LZWTree,0xff,(1 << m_MaxBits)*sizeof(LZWNode));

	// Initiate some variables.
	uint32 NextCode=(1 << CHARACTER_SIZE);
	uint8 CodeBits;
	if (m_MinBits==0)
		CodeBits=CHARACTER_SIZE+1;
	else
		CodeBits=m_MinBits;
	uint32 Limit=1 << CodeBits;

	uint32 CC_Init, CC_EOI;
	if (m_bUseControlCodes) {
		CC_Init=NextCode++;
		CC_EOI=NextCode++;
	}

	uint32 Character;
	COutputBits<uint32> Output((uint32*)Dst+1,Size);

	// Insert the first CC_Init (if we're using control codes).
	if (m_bUseControlCodes)
		Output.Add(CC_Init,CodeBits);

	uint32 PrefixNode=*(Src++);

	bool Skip=FALSE;

	uint32 i=Len-1;
	while (i>0) {

		// Loop until we find an unknown string.
		Character=*(Src++);
		i--;
		while (i>0 && LZWTree[PrefixNode].m_pChildren[Character]!=0xffff) {
			PrefixNode=LZWTree[PrefixNode].m_pChildren[Character];
			Character=*(Src++);
			i--;
		}

		// Output the prefix code.
		Output.Add(PrefixNode,CodeBits);

		// Time to increase code size?
		if (i>0 && NextCode-1>=Limit-1) {
			Limit<<=1;
			CodeBits++;

			// Time to destroy the dictionary?
			if (CodeBits>m_MaxBits) {
				if (m_bUseControlCodes)
					Output.Add(CC_Init,CodeBits-1);

				memset(LZWTree,0xff,(1 << m_MaxBits)*sizeof(LZWNode));

				NextCode=(1 << CHARACTER_SIZE);
				CodeBits=CHARACTER_SIZE+1;
				Limit=1 << CodeBits;
				if (m_bUseControlCodes)
					NextCode+=2;

				Skip=TRUE;
			}
		}

		if (!Skip)
			// Add the DNew phrase to the dictionary.
			LZWTree[PrefixNode].m_pChildren[Character]=NextCode++;
		else
			Skip=FALSE;

		PrefixNode=Character;
	}

	Output.Add(PrefixNode,CodeBits);

	if (m_bUseControlCodes)
		Output.Add(CC_EOI,CodeBits);

	m_CompressedLength=(uint32)(Output.GetPointer())-(uint32)(Dst);

	delete[] LZWTree;

	return (void*)Dst;

}

void* CLZWFast::Decompress(void *Source, void *Destination)
{
	if (m_bUseControlCodes)
		return DecompressWithCC(Source,Destination);

	uint8* Src=(uint8*)Source;
	uint8* Dst=(uint8*)Destination;

	int64 Length=GetUncompressedLength(Source);

	if (Dst==NULL) Dst=DNew(uint8) uint8[Length+4];
	uint8* BasicCharacters=DNew(uint8) uint8[1 << CHARACTER_SIZE];
	StringEntryDecomp* StringTable=DNew(StringEntryDecomp) StringEntryDecomp[1 << m_MaxBits];

	if (Dst==NULL || BasicCharacters==NULL || StringTable==NULL) {
		if (Dst!=NULL && Destination==NULL) delete[] Dst;	
		if (BasicCharacters!=NULL) delete[] BasicCharacters;	
		if (StringTable!=NULL) delete[] StringTable;
		MemError("Decompress");
	}

	uint8* DstStart=Dst;

	// Init string table.
	uint16 ch;
	for (ch=0; ch<(1 << CHARACTER_SIZE); ch++) {
		StringTable[ch].m_pStr=BasicCharacters+ch;
		*(StringTable[ch].m_pStr)=(uint8)ch;
		StringTable[ch].m_Len=1;
	}

	// Initiate some variables.
	uint16 NextCode=(1 << CHARACTER_SIZE);
	uint8 CodeBits;
	if (m_MinBits==0)
		CodeBits=CHARACTER_SIZE+1;
	else
		CodeBits=m_MinBits;
	uint16 Limit=1 << CodeBits;

	uint16 Code, OldCode;

	CInputBits<uint32> Input((uint32*)Src+1);

	// Get the fist code.
	OldCode=Input.Get(CodeBits);
	*(Dst++)=OldCode;
	Length--;

	while (Length>0) {
		Code=Input.Get(CodeBits);

		// Does this code exist?
		if (Code<NextCode) {
			memcpy(Dst,StringTable[Code].m_pStr,StringTable[Code].m_Len);

			StringTable[NextCode].m_pStr=Dst-StringTable[OldCode].m_Len;
			StringTable[NextCode].m_Len=StringTable[OldCode].m_Len+1;

			Dst+=StringTable[Code].m_Len;
			Length-=StringTable[Code].m_Len;

			NextCode++;

			OldCode=Code;
		}
		else {
			memcpy(Dst,StringTable[OldCode].m_pStr,StringTable[OldCode].m_Len);
			*(Dst+StringTable[OldCode].m_Len)=*(StringTable[OldCode].m_pStr);

			StringTable[NextCode].m_pStr=Dst;
			StringTable[NextCode].m_Len=StringTable[OldCode].m_Len+1;

			Dst+=StringTable[OldCode].m_Len+1;
			Length-=(StringTable[OldCode].m_Len+1);

			NextCode++;

			OldCode=NextCode-1;
		}

		// Time to increase code size?
		if (NextCode-1>=Limit-1 && Length>0) {
			Limit<<=1;
			CodeBits++;

			// Time to destroy dictionary?
			if (CodeBits>m_MaxBits) {
				NextCode=(1 << CHARACTER_SIZE);
				if (m_MinBits==0)
					CodeBits=CHARACTER_SIZE+1;
				else
					CodeBits=m_MinBits;
				Limit=1 << CodeBits;

				OldCode=Input.Get(CodeBits);
				*(Dst++)=OldCode;
				Length--;
			}

		}

	}

	delete[] BasicCharacters;
	delete[] StringTable;

	return (void*)DstStart;
}

// This decompressor uses GIF control codes.
void* CLZWFast::DecompressWithCC(void *Source, void *Destination)
{
	uint8* Src=(uint8*)Source;
	uint8* Dst=(uint8*)Destination;

	int64 Length=GetUncompressedLength(Source);

	if (Dst==NULL) Dst=DNew(uint8) uint8[Length+4];
	uint8* BasicCharacters=DNew(uint8) uint8[1 << CHARACTER_SIZE];
	StringEntryDecomp* StringTable=DNew(StringEntryDecomp) StringEntryDecomp[1 << m_MaxBits];

	if (Dst==NULL || BasicCharacters==NULL || StringTable==NULL) {
		if (Dst!=NULL && Destination==NULL) delete[] Dst;	
		if (BasicCharacters!=NULL) delete[] BasicCharacters;	
		if (StringTable!=NULL) delete[] StringTable;
		MemError("DecompressWithCC");
	}

	uint8* DstStart=Dst;

	// Init string table.
	uint16 ch;
	for (ch=0; ch<(1 << CHARACTER_SIZE); ch++) {
		StringTable[ch].m_pStr=BasicCharacters+ch;
		*(StringTable[ch].m_pStr)=(uint8)ch;
		StringTable[ch].m_Len=1;
	}

	// Initiate some variables.
	uint16 NextCode=(1 << CHARACTER_SIZE);
	uint8 CodeBits;
	if (m_MinBits==0)
		CodeBits=CHARACTER_SIZE+1;
	else
		CodeBits=m_MinBits;
	uint16 Limit=1 << CodeBits;

	uint16 CC_Init, CC_EOI;
	CC_Init=NextCode++;
	CC_EOI=NextCode++;

	uint16 Code, OldCode;

	CInputBits<uint32> Input((uint32*)Src+1);

	// Get the first CC_Init
	Input.Get(CodeBits);

	// Get the fist code.
	OldCode=Input.Get(CodeBits);
	*(Dst++)=OldCode;
	Length--;

	while (Length>0) {
		Code=Input.Get(CodeBits);

		// Time to destroy dictionary?
		if (Code==CC_Init) {
			NextCode=(1 << CHARACTER_SIZE)+2;
			if (m_MinBits==0)
				CodeBits=CHARACTER_SIZE+1;
			else
				CodeBits=m_MinBits;
			Limit=1 << CodeBits;

			OldCode=Input.Get(CodeBits);
			*(Dst++)=OldCode;
			Length--;
		}

		// Does this code exist?
		else if (Code<NextCode) {
			memcpy(Dst,StringTable[Code].m_pStr,StringTable[Code].m_Len);

			StringTable[NextCode].m_pStr=Dst-StringTable[OldCode].m_Len;
			StringTable[NextCode].m_Len=StringTable[OldCode].m_Len+1;

			Dst+=StringTable[Code].m_Len;
			Length-=StringTable[Code].m_Len;

			NextCode++;

			OldCode=Code;
		}
		else {
			memcpy(Dst,StringTable[OldCode].m_pStr,StringTable[OldCode].m_Len);
			*(Dst+StringTable[OldCode].m_Len)=*(StringTable[OldCode].m_pStr);

			StringTable[NextCode].m_pStr=Dst;
			StringTable[NextCode].m_Len=StringTable[OldCode].m_Len+1;

			Dst+=StringTable[OldCode].m_Len+1;
			Length-=(StringTable[OldCode].m_Len+1);

			NextCode++;

			OldCode=NextCode-1;
		}

		// Time to increase code size?
		if (NextCode-1>=Limit-1 && Length>0) {
			Limit<<=1;
			if (CodeBits<m_MaxBits) CodeBits++;
		}

	}

	delete[] BasicCharacters;
	delete[] StringTable;

	return (void*)DstStart;
}
*/
///////////
// CLZSS //
///////////
uint8 CLZSS::GetSrc(uint8* pSrc, int _Pos)
{
	M_ASSERT( (uint32)_Pos >= 0 && (uint32)_Pos < m_WindowSize, "_Pos is outside LZSS window" );
	int Ofs = (pSrc - m_pSrcCopy) + _Pos;
	if (Ofs < 0)
		return 0;
	else if ((uint32)Ofs >= m_SrcLen)
		return 0;
	else
		return m_pSrcCopy[Ofs];
}

void CLZSS::InsertNewStrings()
{
	for (uint32 j=0; j<m_WindowMove; j++) 
	{
		int i=(m_ChildPos+j) & (m_WindowSize-1);

		m_pChildren[i].m_Left=EMPTY;
		m_pChildren[i].m_Right=EMPTY;
		m_pChildren[i].m_pStringRef=m_pSrc+j;

		m_Tree=(GetSrc(m_pSrc, j) << 8)+GetSrc(m_pSrc, j+1);

		m_MaxLen = Min(m_WindowMove-j,m_MaxMatchLength);

		// Insert new string.
		if (m_pRoots[m_Tree]==EMPTY) {
			m_pRoots[m_Tree]=i;
			m_pChildren[i].m_Parent=ROOT;
		}
		else {
			m_Node = m_pRoots[m_Tree];

			while (TRUE) {
				m_pSrc2=m_pChildren[m_Node].m_pStringRef;
				int len=2;
				m_Diff = 0;
//				while (len<m_MaxMatchLength && (m_Diff=GetSrc(m_pSrc, j+len)-m_pSrc2[len])==0)
//					len++;
				while (len<(int)m_MaxLen && (m_Diff=GetSrc(m_pSrc, j+len)-m_pSrc2[len])==0)
					len++;

				if (m_Diff<0) {
					if (m_pChildren[m_Node].m_Left==EMPTY) {
						m_pChildren[i].m_Parent=m_Node;
						m_pChildren[m_Node].m_Left=i;
						break;
					}
					else
						m_Node=m_pChildren[m_Node].m_Left;
				}
				else if (m_Diff>0) {
					if (m_pChildren[m_Node].m_Right==EMPTY) {
						m_pChildren[i].m_Parent=m_Node;
						m_pChildren[m_Node].m_Right=i;
						break;
					}
					else
						m_Node=m_pChildren[m_Node].m_Right;
				}
				else break;

			}

		}

	}
}

void CLZSS::DeleteOldStrings()
{
	for (int j=m_WindowMove; j>0; j--) {
		int i=(m_ChildPos+j-1) & (m_WindowSize-1);

		// Remove old string.
		if (m_pChildren[i].m_Parent!=EMPTY) {
			if (m_pChildren[i].m_Left==EMPTY)
				m_ReplaceWith=m_pChildren[i].m_Right;
			else if (m_pChildren[i].m_Right==EMPTY)
				m_ReplaceWith=m_pChildren[i].m_Left;
			else if (m_pChildren[m_pChildren[i].m_Left].m_Right==EMPTY) {
				m_ReplaceWith=m_pChildren[i].m_Left;
				if ((m_pChildren[m_ReplaceWith].m_Right=m_pChildren[i].m_Right)!=EMPTY)
					m_pChildren[m_pChildren[m_ReplaceWith].m_Right].m_Parent=m_ReplaceWith;
			}
			else if (m_pChildren[m_pChildren[i].m_Right].m_Left==EMPTY) {
				m_ReplaceWith=m_pChildren[i].m_Right;
				if ((m_pChildren[m_ReplaceWith].m_Left=m_pChildren[i].m_Left)!=EMPTY)
					m_pChildren[m_pChildren[m_ReplaceWith].m_Left].m_Parent=m_ReplaceWith;
			}
			else {
				// Find the smallest element in the right subtree
				// and replace node i with it.
				m_ReplaceWith=m_pChildren[i].m_Right;
				while (m_pChildren[m_ReplaceWith].m_Left!=EMPTY)
					m_ReplaceWith=m_pChildren[m_ReplaceWith].m_Left;

				m_pChildren[m_pChildren[m_ReplaceWith].m_Parent].m_Left=
					m_pChildren[m_ReplaceWith].m_Right;

				if (m_pChildren[m_ReplaceWith].m_Right!=EMPTY)
					m_pChildren[m_pChildren[m_ReplaceWith].m_Right].m_Parent=
						m_pChildren[m_ReplaceWith].m_Parent;

				m_pChildren[m_ReplaceWith].m_Left=m_pChildren[i].m_Left;
				if (m_pChildren[i].m_Left!=EMPTY)
					m_pChildren[m_pChildren[i].m_Left].m_Parent=m_ReplaceWith;

				m_pChildren[m_ReplaceWith].m_Right=m_pChildren[i].m_Right;
				if (m_pChildren[i].m_Right!=EMPTY)
					m_pChildren[m_pChildren[i].m_Right].m_Parent=m_ReplaceWith;
			}

			if (m_pChildren[i].m_Parent==ROOT) {
				m_pSrc2=m_pChildren[i].m_pStringRef;
				m_Tree=(m_pSrc2[0] << 8)+m_pSrc2[1];
				m_pRoots[m_Tree]=m_ReplaceWith;
			}
			else {
				if (m_pChildren[m_pChildren[i].m_Parent].m_Left==i)
					m_pChildren[m_pChildren[i].m_Parent].m_Left=m_ReplaceWith;
				else
					m_pChildren[m_pChildren[i].m_Parent].m_Right=m_ReplaceWith;
			}

			if (m_ReplaceWith!=EMPTY) m_pChildren[m_ReplaceWith].m_Parent=m_pChildren[i].m_Parent;
			m_pChildren[i].m_Parent=EMPTY;
		}

	}
}

void CLZSS::FindMaxMatch(uint32 Len)
{
	m_Tree=(GetSrc(m_pSrc, 0) << 8)+GetSrc(m_pSrc, 1);

	m_Node=m_pRoots[m_Tree];

	while (m_Node!=EMPTY) {
		m_pSrc2=m_pChildren[m_Node].m_pStringRef;

		m_MaxLen=MinMT((mint)m_pSrc-(mint)m_pSrc2,
				MinMT(Len,m_MaxMatchLength));

		if (m_MaxLen>2) {
			int len=2;
			while (len<(int)m_MaxLen && (m_Diff=GetSrc(m_pSrc, len)-m_pSrc2[len])==0)
				len++;

			if (len>(int)m_BestMatchLength) {
				m_BestMatchLength=len;
				m_BestMatchPos=(mint)m_pChildren[m_Node].m_pStringRef;
				if (m_BestMatchLength==m_MaxMatchLength) break;
			}
		}
		else {
			int len=2;
			while (len<(int)m_MaxLen && (m_Diff=GetSrc(m_pSrc, len)-m_pSrc2[len])==0)
				len++;
		}

		if (m_Diff<0)
			m_Node=m_pChildren[m_Node].m_Left;
		else
			m_Node=m_pChildren[m_Node].m_Right;
	}

}

void CLZSS::SetCompress(CCompressSettings& Settings)
{
	if (Settings.GetCompressType()==LZSS) {
		m_WindowSize=((CLZSSSettings*)&Settings)->GetWindowSize();
		m_MaxMatchLength=((CLZSSSettings*)&Settings)->GetMaxMatchLength();
		m_bUseHuffman=((CLZSSSettings*)&Settings)->GetHuffman();

		uint8 NrBits=(uint8)(logf(fp32(m_WindowSize))/logf(fp32(2))+0.5f);
		if ((m_WindowSize-(1 << NrBits))!=0)
			Error("SetCompress","m_pWindow size must be a power of 2.");

		m_MinMatchLength=0;
		while (m_MinMatchLength*9<=NrBits+2+M_Floor(logf(fp32(m_MaxMatchLength-m_MinMatchLength))/logf(fp32(2))))
			m_MinMatchLength++;

		m_OffsetSize=NrBits;
		m_LengthSize=1+(uint32)M_Floor(logf(fp32(m_MaxMatchLength-m_MinMatchLength))/logf(fp32(2)));
	}
	else
		Error("SetCompress","Incompatible LZSS-compression settings.");
}

// Memory requirements: 2*Len + 128kb + m_WindowSize*12 
// With m_WindowSize=4096 this becomes appr. 2*Len + 176kb
void* CLZSS::Compress(void *Source, void *Destination, mint Len)
{
	if (m_bUseHuffman)
		return CompressWithHuffman(Source,Destination,Len);

	m_pSrc=(uint8*)Source;
	uint8* Dst=(uint8*)Destination;

	uint32 Size=Len;

	if (Dst==NULL) Dst=(uint8*)DNew(uint8) uint8[Size+4+5+1+4];
	m_pRoots=DNew(uint16) uint16[256*256];
	m_pChildren=DNew(LZSSNode) LZSSNode[m_WindowSize];
	if (Dst==NULL || m_pRoots==NULL || m_pChildren==NULL) {
		if (Dst!=NULL && Destination==NULL) delete[] Dst;
		if (m_pRoots!=NULL) delete[] m_pRoots;
		if (m_pChildren!=NULL) delete[] m_pChildren;
		MemError("Compress");
	}

	memset(m_pRoots,0xff,sizeof(uint16)*256*256);
	memset(m_pChildren,0xff,sizeof(LZSSNode)*m_WindowSize);

	m_pSrcCopy=m_pSrc;
	m_SrcLen = Len;

	// Output uncompressed length.
	*((uint32*)Dst)=Len;

	*(Dst+4)=COMPRESSED;

	// Output m_bUseHuffman boolean.
	*(Dst+5)=FALSE;

	COutputBits<uint32> Output((uint8*)Dst+6,Size);

	Output.Add(m_OffsetSize,8);
	Output.Add(m_LengthSize,8);
	Output.Add(m_MinMatchLength,8);
	Output.Add(m_MaxMatchLength,8);

	uint32 WindowLength=0;
	m_ChildPos=0;

	uint32 Length=Len;
	while (Len>0 && ((mint)Output.GetPointer()-(mint)Dst)<Size) {

		m_pWindow=m_pSrc-WindowLength;

		// Find the maximum match in the window.
		m_BestMatchLength=m_MinMatchLength-1;

		FindMaxMatch(Len);

		m_BestMatchPos-=(mint)m_pWindow;

		if (m_BestMatchLength>=m_MinMatchLength) {
			// Output a LZSS (pos,len) pair.
			Output.Add(1,1);
			Output.Add(m_BestMatchPos,m_OffsetSize);
			Output.Add(m_BestMatchLength-m_MinMatchLength,m_LengthSize);

			WindowLength=Min(WindowLength+m_BestMatchLength,m_WindowSize);
			Len-=m_BestMatchLength;
			m_WindowMove=m_BestMatchLength;
		}
		else {
			// Output a single byte.
			Output.Add(0,1);
			Output.Add(*m_pSrc,8);

			WindowLength=Min(WindowLength+1,m_WindowSize);
			Len--;
			m_WindowMove=1;
		}

		// Update the trees as the window has moved.
		DeleteOldStrings();
		InsertNewStrings();

		m_ChildPos=(m_ChildPos+m_WindowMove) & (m_WindowSize-1);

		m_pSrc+=m_WindowMove;
	}

	delete[] m_pChildren;
	delete[] m_pRoots;

	// Better to copy than to compress?
	m_CompressedLength=(mint)Output.GetPointer()-(mint)Dst;
	if (m_CompressedLength>=Size) {

		*(Dst+4)=NOT_COMPRESSED;

		memcpy(Dst+5,m_pSrcCopy,Length);

		m_CompressedLength=Length+5;
	}

	return (void*)Dst;
}

// Memory requirements: encoded_len + decoded_len + 0kb
// In other words it uses no additional memory.
void* CLZSS::Decompress(void *Source, void *Destination)
{
	// Decompress or just copy?
	if (*((uint8*)Source+4)==NOT_COMPRESSED) {
		uint8* Src=(uint8*)Source;
		uint8* Dst=(uint8*)Destination;

		uint32 Length=GetUncompressedLength(Source);

		if (Dst==NULL) Dst=DNew(uint8) uint8[Length+4];
		if (Dst==NULL) MemError("Decompress");

		memcpy(Dst,Src+5,Length);

		return (void*) Dst;
	}

	// Use Huffman?
	if (*((uint8*)Source+5))
		return DecompressWithHuffman(Source,Destination);

	uint8* Src=(uint8*)Source;
	uint8* Dst=(uint8*)Destination;

	uint32 Length=GetUncompressedLength(Source);

	if (Dst==NULL) Dst=DNew(uint8) uint8[Length];
	if (Dst==NULL) MemError("Decompress");

	uint8* DstCopy=Dst;

	CInputBits<uint32> Input((uint8*)Src+6);

	uint32 OffsetSize=Input.Get(8);
	uint32 LengthSize=Input.Get(8);
	uint32 MinMatchLength=Input.Get(8);
	uint32 OldMML=m_MaxMatchLength;
	m_MaxMatchLength=Input.Get(8);

	uint32 WindowSize=1 << OffsetSize;
	uint32 WindowLength=0;

	uint32 len;
	uint8* Str;
	while (Length>0) {

		if (Input.Get(1)==0)
		{
			*(Dst++)=Input.Get(8);
			Length--;
			WindowLength=Min(WindowLength+1,WindowSize);
		}
		else {
			Str=Dst-WindowLength+Input.Get(OffsetSize);
			memcpy(Dst,Str,len=Input.Get(LengthSize)+MinMatchLength);
			Dst+=len;
			Length-=len;
			WindowLength=Min(WindowLength+len,WindowSize);
		}

	}

	m_MaxMatchLength=OldMML;

	return (void*)DstCopy;
}

// Memory requirements: Len*(3.125f + (log2(m_WindowSize)
// +1+floor(log2(m_MaxMatchLength-m_MinMatchLength)))/(8*m_MinMatchLength))
// + 128kb + m_WindowSize*12
// With default values this becomes appr. 3.917f*Len + 176kb
// CONCLUSION: This one eats memory (but has a nice compression ratio).
void* CLZSS::CompressWithHuffman(void *Source, void *Destination, mint Len)
{
	m_pSrc=(uint8*)Source;
	uint8* Dst=(uint8*)Destination;

	uint32 Size=Len;

	CHuffman Huff;

	if (Dst==NULL) Dst=(uint8*)DNew(uint8) uint8[Size+4+10+4*5+5*Huff.GetCompDestExtra()];
	if (Dst==NULL) MemError("CompressWithHuffman");

	uint32 OffsetLowSize=m_OffsetSize >> 1;
	uint32 OffsetHighSize=m_OffsetSize >> 1;
	if ((m_OffsetSize & 1)==1) OffsetHighSize++;

	// Calculate the "worst-case" amount of memory needed.
	uint32 Worst0=(Size >> 3)+1;
	uint32 Worst1=(((Size*OffsetLowSize) >> 3)/m_MinMatchLength)+1;
	uint32 Worst2=(((Size*OffsetHighSize) >> 3)/m_MinMatchLength)+1;
	uint32 Worst3=(((Size*m_LengthSize) >> 3)/m_MinMatchLength)+1;
	uint32 Worst4=Size;

	uint8* Dst0=(uint8*)DNew(uint8) uint8[Worst0+Worst1+Worst2+Worst3+Worst4+4];
	uint8* Dst1=Dst0+Worst0;
	uint8* Dst2=Dst1+Worst1;
	uint8* Dst3=Dst2+Worst2;
	uint8* Dst4=Dst3+Worst3;

	m_pRoots=DNew(uint16) uint16[256*256];

	m_pChildren=DNew(LZSSNode) LZSSNode[m_WindowSize];

	if (Dst0==NULL || m_pRoots==NULL || m_pChildren==NULL) {
		if (Dst0!=NULL) delete[] Dst0;
		if (m_pRoots!=NULL) delete[] m_pRoots;
		if (m_pChildren!=NULL) delete[] m_pChildren;
		MemError("CompressWithHuffman");
	}

	memset(m_pRoots,0xff,sizeof(uint16)*256*256);
	memset(m_pChildren,0xff,sizeof(LZSSNode)*m_WindowSize);

	m_pSrcCopy=m_pSrc;
	m_SrcLen = Len;

	// Output uncompressed length.
	*((uint32*)Dst)=Len;

	*((uint8*)Dst+4)=COMPRESSED;

	// Output m_bUseHuffman boolean.
	*((uint8*)Dst+5)=TRUE;

	*((uint8*)Dst+6+4*5)=m_OffsetSize;
	*((uint8*)Dst+7+4*5)=m_LengthSize;
	*((uint8*)Dst+8+4*5)=m_MinMatchLength;
	*((uint8*)Dst+9+4*5)=m_MaxMatchLength;
	
	COutputBits<uint32> Output(Dst0,Worst0);
	COutputBits<uint32> Output1(Dst1,Worst1);
	COutputBits<uint32> Output2(Dst2,Worst2);
	COutputBits<uint32> Output3(Dst3,Worst3);

	uint8* Dst4Copy=Dst4;
	uint8* SrcCopy=m_pSrc;

	uint32 WindowLength=0;
	m_ChildPos=0;

	uint32 Length=Len;

	///////////////////////////////////////////
	///////////////////////////////////////////
	// This is a quick fix for a case when the
	// encoder hard-locks. Just fail compression
	// when this happens - JA
/*	int MagicByte = Len & 0xfc;
	if(MagicByte == Len)
		MagicByte -= 4;
	if(Len <= 4 || (Len < 28 && m_pSrc[MagicByte] == 0))
	{
		LogFile("(CLZSS::CompressWithHuffman) Error during encoding. Using workaround");
		*(Dst+4)=NOT_COMPRESSED;
		
		memcpy(Dst+5,SrcCopy,Length);
		
		m_CompressedLength=Length+5;
		delete[] Dst0;
		return (void*)Dst;
	}*/
	///////////////////////////////////////////
	///////////////////////////////////////////
	
//	int FailSafe = 0;
	while (Len>0) {

		m_pWindow=m_pSrc-WindowLength;

		// Find the maximum match in the window.
		m_BestMatchPos=0;
		m_BestMatchLength=m_MinMatchLength-1;

		FindMaxMatch(Len);

		m_BestMatchPos-=(mint)m_pWindow;

		if (m_BestMatchLength>=m_MinMatchLength) {
			// Output a LZSS (pos,len) pair.
			Output.Add(1,1);
			Output1.Add(m_BestMatchPos,OffsetLowSize);
			Output2.Add(m_BestMatchPos >> OffsetLowSize,OffsetHighSize);
			Output3.Add(m_BestMatchLength-m_MinMatchLength,m_LengthSize);

			WindowLength=Min(WindowLength+m_BestMatchLength,m_WindowSize);
			Len-=m_BestMatchLength;
			m_WindowMove=m_BestMatchLength;
		}
		else {
			// Output a single byte.
			Output.Add(0,1);
			*(Dst4++)=*m_pSrc;

			WindowLength=Min(WindowLength+1,m_WindowSize);
			Len--;
			m_WindowMove=1;
		}

		// Update the trees as the window has moved.
		DeleteOldStrings();
		InsertNewStrings();

		m_ChildPos=(m_ChildPos+m_WindowMove) & (m_WindowSize-1);

		m_pSrc+=m_WindowMove;
	}

	delete[] m_pChildren;
	delete[] m_pRoots;

	uint32 TotalLength=(mint)Output.GetPointer()-(mint)Dst0+
		(mint)Output1.GetPointer()-(mint)Dst1+
		(mint)Output2.GetPointer()-(mint)Dst2+
		(mint)Output3.GetPointer()-(mint)Dst3+
		(mint)Dst4-(mint)Dst4Copy;

	// Better to copy than to compress?
	if (TotalLength>=Size) {

		*(Dst+4)=NOT_COMPRESSED;

		memcpy(Dst+5,SrcCopy,Length);

		m_CompressedLength=Length+5;
	}
	else {
		// Now Huffman encode the five different groups seperately.
		CHuffmanSettings hset;

		m_CompressedLength=10+4*5;
		uint8* Ptr=Dst+m_CompressedLength;

		*((uint32*)((uint8*)Dst+6))=(mint)Ptr-(mint)Dst;
		hset.SetInputSize(8);
		Huff.SetCompress(hset);
		Huff.Compress(Dst0,Ptr,(mint)Output.GetPointer()-(mint)Dst0);
		m_CompressedLength+=Huff.GetCompressedLength();
		Ptr+=Huff.GetCompressedLength();

		*((uint32*)((uint8*)Dst+6)+1)=(mint)Ptr-(mint)Dst;
		hset.SetInputSize(OffsetLowSize);
		Huff.SetCompress(hset);
		Huff.Compress(Dst1,Ptr,(mint)Output1.GetPointer()-(mint)Dst1);
		m_CompressedLength+=Huff.GetCompressedLength();
		Ptr+=Huff.GetCompressedLength();

		*((uint32*)((uint8*)Dst+6)+2)=(mint)Ptr-(mint)Dst;
		hset.SetInputSize(OffsetHighSize);
		Huff.SetCompress(hset);
		Huff.Compress(Dst2,Ptr,(mint)Output2.GetPointer()-(mint)Dst2);
		m_CompressedLength+=Huff.GetCompressedLength();
		Ptr+=Huff.GetCompressedLength();

		*((uint32*)((uint8*)Dst+6)+3)=(mint)Ptr-(mint)Dst;
		hset.SetInputSize(m_LengthSize);
		Huff.SetCompress(hset);
		Huff.Compress(Dst3,Ptr,(mint)Output3.GetPointer()-(mint)Dst3);
		m_CompressedLength+=Huff.GetCompressedLength();
		Ptr+=Huff.GetCompressedLength();

		*((uint32*)((uint8*)Dst+6)+4)=(mint)Ptr-(mint)Dst;
		hset.SetInputSize(8);
		Huff.SetCompress(hset);
		Huff.Compress(Dst4Copy,Ptr,(mint)Dst4-(mint)Dst4Copy);
		m_CompressedLength+=Huff.GetCompressedLength();

		// Better to copy than to compress?
		if (m_CompressedLength>=Size) {

			*(Dst+4)=NOT_COMPRESSED;

			memcpy(Dst+5,SrcCopy,Length);

			m_CompressedLength=Length+5;
		}
	}

	delete[] Dst0;

	return (void*)Dst;
}

// Memory requirements: Absolute worst case is encoded_len + 2*decoded_len
void* CLZSS::DecompressWithHuffman(void *Source, void *Destination)
{
	uint8* Src=(uint8*)Source;
	uint8* Dst=(uint8*)Destination;

	uint32 Length=GetUncompressedLength(Source);

	// Get the pointers to the five different Huffman encoded
	// groups.
#ifdef	CPU_ALIGNED_MEMORY_ACCESS
	uint8* Ptr0 = Src + MEM_UNALIGNED_READUINT32LE( Src + 6 );
	uint8* Ptr1 = Src + MEM_UNALIGNED_READUINT32LE( Src + 6 + 4 );
	uint8* Ptr2 = Src + MEM_UNALIGNED_READUINT32LE( Src + 6 + 8 );
	uint8* Ptr3 = Src + MEM_UNALIGNED_READUINT32LE( Src + 6 + 12 );
	uint8* Ptr4 = Src + MEM_UNALIGNED_READUINT32LE( Src + 6 + 16 );
#else
	uint8* Ptr0=Src+*((uint32*)((uint8*)Src+6));
	uint8* Ptr1=Src+*((uint32*)((uint8*)Src+6)+1);
	uint8* Ptr2=Src+*((uint32*)((uint8*)Src+6)+2);
	uint8* Ptr3=Src+*((uint32*)((uint8*)Src+6)+3);
	uint8* Ptr4=Src+*((uint32*)((uint8*)Src+6)+4);
#endif
	// Allocate memory for the decoded versions of the
	// groups and then decode them.
	CHuffman* Huff=MNew(CHuffman);

	uint32 TotalLength=Huff->GetUncompressedLength(Ptr0)+Huff->GetUncompressedLength(Ptr1)
		+Huff->GetUncompressedLength(Ptr2)+Huff->GetUncompressedLength(Ptr3)
		+Huff->GetUncompressedLength(Ptr4);

	uint8* Group0=DNew(uint8) uint8[TotalLength+4];
	if (Group0==NULL) MemError("DecompressWithHuffman");
	uint8* Group1=Group0+Huff->GetUncompressedLength(Ptr0);
	uint8* Group2=Group1+Huff->GetUncompressedLength(Ptr1);
	uint8* Group3=Group2+Huff->GetUncompressedLength(Ptr2);
	uint8* Group4=Group3+Huff->GetUncompressedLength(Ptr3);

	Huff->Decompress(Ptr0,Group0);
	Huff->Decompress(Ptr1,Group1);
	Huff->Decompress(Ptr2,Group2);
	Huff->Decompress(Ptr3,Group3);
	Huff->Decompress(Ptr4,Group4);

	delete Huff;	// Free memory used by the Huffman decoder.

	// Allocate memory for destination if necessary.
	if (Dst==NULL) Dst=DNew(uint8) uint8[Length];
	if (Dst==NULL) MemError("Decompress");

	uint8* DstCopy=Dst;

	// Start the LZSS uncompression.
	CInputBits<uint32> Input(Group0);
	CInputBits<uint32> Input1(Group1);
	CInputBits<uint32> Input2(Group2);
	CInputBits<uint32> Input3(Group3);

	Src+=6+4*5;
	uint32 OffsetSize=*(Src++);
	uint32 LengthSize=*(Src++);
	uint32 MinMatchLength=*(Src++);
	uint32 OldMML=m_MaxMatchLength;
	m_MaxMatchLength=*(Src++);

	uint32 OffsetLowSize=OffsetSize >> 1;
	uint32 OffsetHighSize=OffsetSize >> 1;
	if ((OffsetSize & 1)==1) OffsetHighSize++;

	uint32 WindowSize=1 << OffsetSize;
	uint32 WindowLength=0;

	uint32 len;
	uint8* Str;
	while (Length>0) {

		if (Input.Get(1)==0)
		{
			*(Dst++)=*(Group4++);
			Length--;
			WindowLength=Min(WindowLength+1,WindowSize);
		}
		else {
			Str=Dst-WindowLength+Input1.Get(OffsetLowSize)+
				(Input2.Get(OffsetHighSize) << OffsetLowSize);
			memcpy(Dst,Str,len=Input3.Get(LengthSize)+MinMatchLength);
			Dst+=len;
			Length-=len;
			WindowLength=Min(WindowLength+len,WindowSize);
		}

	}

	delete[] Group0;

	m_MaxMatchLength=OldMML;

	return (void*)DstCopy;
}


//////////
// ZLIB //
//////////

#include "../../Sdk/ZLib/zlib.h"

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:  Sets the ZLIB compression settings

Parameters:			
_Settings:		Settings to use (must be CZLIBSettings)

Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
void CZLIB::SetCompress(CCompressSettings & _Settings)
{
	if (_Settings.GetCompressType() != ZLIB)
	{
		Error("SetCompress","Incompatible ZLIB-compression settings.");
		return;
	}

	CZLIBSettings *Settings = (CZLIBSettings*)&_Settings;

	m_Level = Settings->GetLevel();
	m_Strategy = Settings->GetStrategy();
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:		Compresses data

Parameters:			
_pSource:			The data to be compressed
_pDestination:	Where the compressed data should be put. If this 
				parameter is NULL the Compress function will allocate
				the memory for you
_Len:			The length of the uncompressed data

Returns:		The destination buffer supplied to the function, or if
				NULL was passed the allocated destination buffer is 
				returned

Comments:		See base class for additional details

\*_____________________________________________________________________________*/
void* CZLIB::Compress(void *_pSource, void *_pDestination, mint _Len)
{
//	uint8* pSrc=(uint8*)_pSource;
	uint8* pDst=(uint8*)_pDestination;

	uint32 Size=compressBound(_Len);

	bool bHasBuffer = (_pDestination != NULL);

	if( !bHasBuffer )
	{
		pDst=DNew(uint8) uint8[Size+8];
		if (pDst==NULL) 
		{
			MemError("Compress");
			return NULL;
		}
	}

	*((uint32*)pDst)=_Len;

//	int Error = compress2(pDst+8,&Size,(const Bytef*)_pSource,_Len,m_Level);
	//*
	int Error;

	z_stream ZStream; 

	ZStream.zalloc = NULL;
	ZStream.zfree = NULL;
	ZStream.opaque = NULL;

	Error = deflateInit(&ZStream, m_Level);
	if( Error == Z_OK )
	{
		deflateParams(&ZStream, m_Level, m_Strategy);

		ZStream.next_in  = (Bytef*)_pSource;
		ZStream.next_out = pDst + 8;
		ZStream.avail_in = _Len;
		ZStream.avail_out = Size;

		Error = deflate(&ZStream, Z_FINISH);
		if( Error == Z_STREAM_END )
		{
			Error = deflateEnd(&ZStream);

			Size = ZStream.total_out;
		}
	}
	
	if( Error != Z_OK )
	{
		if( !bHasBuffer ) delete [] pDst;
		CStr ErrorCode = CStrF("ZLIB compression failed, code: %s",zError(Error));
		Error("Compress",ErrorCode.Str());
		return NULL;
	}
	//*/

	//Enter compressed datasize for fast access when decompressing
	*((uint32*)(pDst+4))= Size;
	m_CompressedLength = Size+8;

	return pDst;
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:		Decompresses data

Parameters:			
_pSource:			The data to be uncompressed
_pDestination:	Where the uncompressed data should go. If this parameter
				is NULL the Decompress function will allocate the memory
				for you

Returns:		The pointer supplied to Destination, or if NULL was 
				passed the allocated destination buffer is returned.

Comments:		See base class for additional details
\*_____________________________________________________________________________*/
void* CZLIB::Decompress(void * _pSource,void * _pDestination)
{
	uint8* pSrc=(uint8*)_pSource;
	uint8* pDst=(uint8*)_pDestination;

	z_uLongf Length=GetUncompressedLength(_pSource);
	mint CompressedLength=GetZLIBCompressedLength(_pSource);

	bool bHasBuffer = (_pDestination != NULL);
	if( !bHasBuffer )
	{
		pDst=DNew(uint8) uint8[Length];
		if (pDst==NULL) 
		{
			MemError("Decompress");
			return NULL;
		}
	}

	int Error = uncompress(pDst,&Length,pSrc+8,CompressedLength);

	if( Error != Z_OK )
	{
		if( !bHasBuffer ) delete [] pDst;
		CStr ErrorCode = CStrF("ZLIB decompression failed, code: %s",zError(Error));
		Error("Decompress",ErrorCode.Str());
		return NULL;
	}

	return pDst;
}

namespace NIds
{
	namespace NDataProcessing
	{

		ch8 g_Base64EncodingTable[65] = 
		{
			'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
			'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
			'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
			'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/',
			'=' // Padding
		};

		int8 g_Base64EncodingTableReverse[256] = 
		{
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
			52,53,54,55,56,57,58,59,60,61,-1,-1,-1,64,-1,-1,
			-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
			15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
			-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
			41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
		};

		CBinaryStream_Base64::CBinaryStream_Base64()
		{
			m_pStream = DNP;
			m_OpenFlags = 0;
			m_CurrentLoaded = -1;
			m_FilePos = 0;
			m_bCurrentDirty = false;
			m_FileLen = 0;
		}

		CBinaryStream_Base64::~CBinaryStream_Base64()
		{
			Close();
		}

		void CBinaryStream_Base64::Close()
		{
			if (m_bCurrentDirty)
				WriteDirty();
			m_OpenFlags = 0;
		}

		void CBinaryStream_Base64::Open(CCFile *_pStream, int _OpenFlags)
		{
			m_pStream = _pStream;
			m_OpenFlags = _OpenFlags;
			m_pStream->Seek(0);
			m_bCurrentDirty = false;

			if (m_OpenFlags == CFILE_WRITE)
			{
				m_FilePos = 0;
			}
			else if (m_OpenFlags == CFILE_READ)
			{
				m_FilePos = 0;
				fint FLen = m_pStream->Length();
				m_pStream->SeekToEnd();
				m_pStream->RelSeek(-2);
				uint8 Bytes[2];
				m_pStream->Read(Bytes, 2);
				if (g_Base64EncodingTableReverse[Bytes[0]] == 64)
					m_FileLen = ((FLen / 4) * 3) - 2;
				else if (g_Base64EncodingTableReverse[Bytes[1]] == 64)
					m_FileLen = ((FLen / 4) * 3) - 1;
				else
					m_FileLen = ((FLen / 4) * 3);
				m_pStream->Seek(0);
			}
			else
			{
				Error_static(M_FUNCTION,"You must open the file either with read or write access not both at the same time");
			}
		}

		void CBinaryStream_Base64::WriteDirty()
		{
			m_pStream->Seek((m_CurrentLoaded/3)*4);				
			int8 Temp[CBinaryStream_Base64::EChunkSizeBase64];
            
			// 111111110000000011111111
			// 111111000000111111000000
			Temp[0] = g_Base64EncodingTable[m_DecryptedData[0]>>2];
			Temp[1] = g_Base64EncodingTable[((m_DecryptedData[0]<<4) | (m_DecryptedData[1]>>4)) & 0x3f];
			Temp[2] = g_Base64EncodingTable[((m_DecryptedData[1]<<2) | (m_DecryptedData[2]>>6)) & 0x3f];
			Temp[3] = g_Base64EncodingTable[m_DecryptedData[2] & 0x3f];

			if ((m_FileLen - m_CurrentLoaded) <= 2)
				Temp[3] = g_Base64EncodingTable[64];
			if ((m_FileLen - m_CurrentLoaded) <= 1)
				Temp[2] = g_Base64EncodingTable[64];

			m_pStream->Write(Temp, CBinaryStream_Base64::EChunkSizeBase64);
			m_bCurrentDirty = false;
		}

		aint CBinaryStream_Base64::PrepareBlock(fint _Pos, bint _bWrite)
		{
			if (m_CurrentLoaded >= 0 && _Pos >= m_CurrentLoaded && _Pos < m_CurrentLoaded + CBinaryStream_Base64::EChunkSizeData)
			{
				return _Pos - m_CurrentLoaded;
			}

			if (m_bCurrentDirty)
			{
				WriteDirty();
			}

			m_CurrentLoaded = (_Pos / 3)*3;
			fint BlockPos = (_Pos / 3) * 4;

			if ((BlockPos+4) > m_pStream->Length())
			{
				if (_bWrite)
					memset(m_DecryptedData, 0, sizeof(m_DecryptedData));
				else
					Error_static(M_FUNCTION,"Read past end of file");
			}
			else
			{
				m_pStream->Seek(BlockPos);					
				uint8 Temp[CBinaryStream_Base64::EChunkSizeBase64];
				m_pStream->Read(Temp, CBinaryStream_Base64::EChunkSizeBase64);
				int Table0 = g_Base64EncodingTableReverse[Temp[0]];
				int Table1 = g_Base64EncodingTableReverse[Temp[1]];
				int Table2 = g_Base64EncodingTableReverse[Temp[2]];
				int Table3 = g_Base64EncodingTableReverse[Temp[3]];



				if (Table0 < 0 || Table1 < 0 || Table2 < 0 || Table3 < 0)
					Error_static(M_FUNCTION,"Invalid Base64 character");

				m_DecryptedData[0] = Table0 << 2 | Table1 >> 4;
				if (Table2 != 64)
				{
					m_DecryptedData[1] = (Table1 & 0xf) << 4 | (Table2 >> 2);
					if (Table3 != 64)
					{
						m_DecryptedData[2] = (Table2 & 0x3) << 6 | Table3;
					}
				}
			}

			return _Pos - m_CurrentLoaded;
		}

		void CBinaryStream_Base64::FeedBytes(const void *_pMem, mint _nBytes)
		{
			if (!(m_OpenFlags & CFILE_WRITE))
				Error_static(M_FUNCTION,"File was not opened for write.");

			const uint8 *pMem = (const uint8 *)_pMem;
			while (_nBytes)
			{
				aint Pos = PrepareBlock(m_FilePos, true);
				aint ThisTime = MinMT(_nBytes, (mint)EChunkSizeData - Pos);
				memcpy(m_DecryptedData + Pos, pMem, ThisTime);
				m_bCurrentDirty = true;

				m_FilePos += ThisTime;
				pMem += ThisTime;
				_nBytes -= ThisTime;
				if (m_FilePos > m_FileLen)
					m_FileLen = m_FilePos;
			}
		}

		void CBinaryStream_Base64::ConsumeBytes(void *_pMem, mint _nBytes)
		{
			if (!(m_OpenFlags & CFILE_READ))
				Error_static(M_FUNCTION,"File was not opened for read.");

			uint8 *pMem = (uint8 *)_pMem;
			while (_nBytes)
			{
				aint Pos = PrepareBlock(m_FilePos, false);
				aint ThisTime = MinMT(_nBytes, (mint)EChunkSizeData - Pos);
				memcpy(pMem, m_DecryptedData + Pos, ThisTime);

				m_FilePos += ThisTime;
				pMem += ThisTime;
				_nBytes -= ThisTime;
			}
		}

		bint CBinaryStream_Base64::IsValid()
		{
			return true;
		}

		bint CBinaryStream_Base64::IsAtEndOfStream()
		{
			return m_pStream->EndOfFile();
		}

		fint CBinaryStream_Base64::GetPosition()
		{
			return m_FilePos;
		}

		void CBinaryStream_Base64::SetPosition(fint _Pos)
		{
			m_FilePos = _Pos;
		}

		void CBinaryStream_Base64::SetPositionFromEnd(fint _Pos)
		{
			m_FilePos = (m_pStream->Length()/4)*3 - _Pos;
		}

		void CBinaryStream_Base64::AddPosition(fint _Pos)
		{
			m_FilePos += _Pos;
		}

		fint CBinaryStream_Base64::GetLength()
		{
			if (m_OpenFlags == CFILE_WRITE)
				return Max((m_pStream->Length()/4)*3, Max(m_CurrentLoaded, m_FilePos));
			else
				return m_FileLen;
		}

		CStr MCCDLLEXPORT Base64EncodeData(const void *_pData, mint _Len)
		{
			CCFile Stream;
			int FLen = ((_Len+2) / 3) * 4;
			CStr Ret;
			CStream_Memory MemStream((uint8 *)Ret.GetBuffer(FLen+1), 0, FLen);
			Stream.Open(&MemStream, CFILE_READ | CFILE_WRITE);
			{
				CBinaryStream_Base64 Base64;
				Base64.Open(&Stream, CFILE_WRITE);
				Base64.FeedBytes(_pData, _Len);
			}
			Ret.SetChar(FLen, 0);
			return Ret;
		}

		TArray<uint8> MCCDLLEXPORT Base64DecodeData(CStr _String)
		{
			CCFile Stream;
			CStream_Memory MemStream((uint8*)_String.Str(), _String.Len(), _String.Len());
			Stream.Open(&MemStream, CFILE_READ);
			TArray<uint8> Ret;
			{
				CBinaryStream_Base64 Base64;
				Base64.Open(&Stream, CFILE_READ);
				mint FLen = Base64.GetLength();
				Ret.SetLen(FLen);
				Base64.ConsumeBytes(Ret.GetBasePtr(), FLen);
			}
			return Ret;
		}

		CStr MCCDLLEXPORT Base64EncodeStr(CStr _String)
		{
			CCFile Stream;
			int FLen = ((_String.Len()+2) / 3) * 4;
			CStr Ret;
			CStream_Memory MemStream((uint8 *)Ret.GetBuffer(FLen+1), 0, FLen);
			Stream.Open(&MemStream, CFILE_READ | CFILE_WRITE);
			{
				CBinaryStream_Base64 Base64;
				Base64.Open(&Stream, CFILE_WRITE);
				Base64.FeedBytes(_String.Str(), _String.Len());
			}
			Ret.SetChar(FLen, 0);
			return Ret;
		}

		CStr MCCDLLEXPORT Base64DecodeStr(CStr _String)
		{
			CCFile Stream;
			CStream_Memory MemStream((uint8*)_String.Str(), _String.Len(), _String.Len());
			Stream.Open(&MemStream, CFILE_READ);
			CStr Ret;
			{
				CBinaryStream_Base64 Base64;
				Base64.Open(&Stream, CFILE_READ);
				mint FLen = Base64.GetLength();
				Base64.ConsumeBytes(Ret.GetBuffer(FLen+1), FLen);
				Ret.SetChar(FLen, 0);
			}
			return Ret;
		}

	}
}
