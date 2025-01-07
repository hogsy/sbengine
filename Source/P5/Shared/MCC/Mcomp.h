
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		This is a collection of compression classes.
					All classes are made by Daniel Hansson.
					
					It currently contains:

					(i)		Huffman encoding/decoding
					(ii)		LZW compression/decompression
					(iii)	LZSS compression/decompression
					(With or without Huffman)
					
					(iv)	ZLIB compression
					
	Comments:		
					IMPORTANT!!!

					Memory considerations:
					---------------------
					To be on the safe side use the functions
					GetCompSrcExtra(), GetCompDestExtra(),
					GetDecompSrcExtra() or GetDecompDestExtra()
					AFTER you've done all settings for the
					compression class and they will return
					the correct amount of extra memory needed.

					RLE and LZW_HUFFMAN are not yet implemented.

					I haven't find any way to combine LZW with 
					Huffman as yet. I don't even know if there
					is a way!
					
	History:		
		030505:		Added Comments

		051011:		Added ZLIB compression option //Anders Ekermo
\*_____________________________________________________________________________________________*/


#ifndef _COMPRESSION_CLASSES_
#define _COMPRESSION_CLASSES_

#include "MDA.h"
#include "MFile.h"

#define	MEM_UNALIGNED_READUINT16LE( a )	(((uint32)*((uint8*)(a)+0)<<0) | ((uint32)*((uint8*)(a)+1)<<8))
#define	MEM_UNALIGNED_READUINT32LE( a )	(((uint32)*((uint8*)(a)+0)<<0) | ((uint32)*((uint8*)(a)+1)<<8) | ((uint32)*((uint8*)(a)+2)<<16) | ((uint32)*((uint8*)(a)+3)<<24))
#define	MEM_UNALIGNED_WRITEUINT16LE( dest, a )	{((uint8*)dest)[0] = (uint8)(a & 0xff); ((uint8*)dest)[1] = (uint8)((a>>8) & 0xff);}
#define	MEM_UNALIGNED_WRITEUINT32LE( dest, a )	{((uint8*)dest)[0] = (uint8)(a & 0xff); ((uint8*)dest)[1] = (uint8)((a>>8) & 0xff); ((uint8*)dest)[2] = (uint8)((a>>16) & 0xff); ((uint8*)dest)[3] = (uint8)((a>>24) & 0xff);}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				The abstract compression settings class.
\*_____________________________________________________________________________*/
class MCCDLLEXPORT CCompressSettings 
{
public:
	virtual ~CCompressSettings() {};
	virtual ECompressTypes GetCompressType() pure;
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Settings for the Huffman encoder.
\*_____________________________________________________________________________*/
class MCCDLLEXPORT CHuffmanSettings : public CCompressSettings 
{
	uint8 m_InputSize;
public:
	CHuffmanSettings() 
	{ 
		m_InputSize = 8;
	};

	~CHuffmanSettings() 
	{
	};

	ECompressTypes GetCompressType() 
	{ 
		return HUFFMAN; 
	};

	uint8 GetInputSize() 
	{ 
		return m_InputSize; 
	};

	void SetInputSize(uint8 _Size) 
	{ 
		m_InputSize = _Size; 
	};
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Settings for the LZW compressor.
\*_____________________________________________________________________________*/
class MCCDLLEXPORT CLZWSettings : public CCompressSettings 
{
protected:
	uint8 m_MinBits;
	uint8 m_MaxBits;
public:
	CLZWSettings() 
	{ 
		m_MinBits = 0; 
		m_MaxBits = 13; 
	};

	~CLZWSettings() 
	{
	};

	ECompressTypes GetCompressType() 
	{
		return LZW;
	};

	uint8 GetMaximumBits() 
	{ 
		return m_MaxBits; 
	};
	void SetMaximumBits(uint8 _Max) 
	{ 
		m_MaxBits = _Max; 
	};
	
	uint8 GetMinimumBits() 
	{
		return m_MinBits; 
	};

	void SetMinimumBits(uint8 _Min) 
	{ 
		m_MinBits = _Min; 
	};

	virtual bool GetControlCodesFlag() 
	{ 
		return FALSE; 
	};
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				GIF compatible settings for the LZW compressor.
\*_____________________________________________________________________________*/
class MCCDLLEXPORT CLZW_GIFSettings : public CLZWSettings 
{
public:
	CLZW_GIFSettings() 
	{ 
		m_MinBits = 0; 
		m_MaxBits = 12; 
	};

	ECompressTypes GetCompressType() 
	{ 
		return LZW_GIF; 
	};

	void SetMaximumBits(uint8 _Max) 
	{ 
		m_MaxBits = 12; 
	};

	bool GetControlCodesFlag() 
	{ 
		return TRUE; 
	};
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Settings for the LZSS compressor.
\*_____________________________________________________________________________*/
class MCCDLLEXPORT CLZSSSettings : public CCompressSettings 
{

	uint32 m_WindowSize;
	uint16 m_MaxMatchLength;
	bool m_bUseHuffman;

public:
	CLZSSSettings() 
	{ 
		m_WindowSize = 4096; 
		m_MaxMatchLength = 18; 
		m_bUseHuffman = FALSE; 
	};

	~CLZSSSettings() 
	{
	};

	ECompressTypes GetCompressType() 
	{ 
		return LZSS; 
	};

	uint32 GetWindowSize() 
	{ 
		return m_WindowSize; 
	};

	void SetWindowSize(uint32 _Size) 
	{ 
		m_WindowSize = _Size; 
	};

	uint16 GetMaxMatchLength() 
	{ 
		return m_MaxMatchLength; 
	};

	void SetMaxMatchLength(uint16 _Length) 
	{ 
		m_MaxMatchLength = _Length; 
	};

	bool GetHuffman() 
	{ 
		return m_bUseHuffman; 
	};

	void SetHuffman(bool _bUse)
	{ 
		m_bUseHuffman = _bUse;
		if (m_bUseHuffman)
			m_MaxMatchLength=96;
		else
			m_MaxMatchLength=18;
	}
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Class:				Settings for the ZLIB encoder.
\*_____________________________________________________________________________*/
class MCCDLLEXPORT CZLIBSettings : public CCompressSettings 
{

private:

	EZLIBLevels	m_Level;
	EZLIBStrategies	m_Strategy;

public:
	CZLIBSettings() 
	{ 
		m_Level = ZLIB_HIGH_COMPRESSION;
		m_Strategy = ZLIB_STRATEGY_DEFAULT;
	};

	~CZLIBSettings() 
	{
	};

	ECompressTypes GetCompressType() 
	{ 
		return ZLIB; 
	};

	void SetLevel(EZLIBLevels _Level)
	{
		m_Level = ((_Level < ZLIB_NO_COMPRESSION) || (_Level > ZLIB_HIGH_COMPRESSION)) ? 
			ZLIB_STANDARD_COMPRESSION : _Level;
	};

	EZLIBLevels GetLevel() const
	{
		return m_Level;
	};

	void SetStrategy(EZLIBStrategies _Strategy)
	{
		m_Strategy = _Strategy;
	};

	EZLIBStrategies GetStrategy() const
	{
		return m_Strategy;
	}
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				The abstract compress class.
\*_____________________________________________________________________________*/
class MCCDLLEXPORT CCompress : public CObj 
{
protected:

	mint m_CompressedLength;

public:

	CCompress()
	{
	}


	~CCompress() 
	{
	};

    /*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
    	Function:			Compresses data
    						
    	Parameters:			
    		Source:			The data to be compressed
    		Destination:	Where the compressed data should be put. If this 
							parameter is NULL the Compress function will allocate
							the memory for you
    		Len:			The length of the uncompressed data
    						
    	Returns:			The destination buffer supplied to the function, or if
							NULL was passed the allocated destination buffer is 
							returned
    						
    	Comments:			The Compress function has two obligations:

							(i)		It should fill in the CompressedLength variable
									when done.
							(ii)	It should put the uncompressed length of the source
									in the first uint32 of the output.

    \*_____________________________________________________________________________*/
	virtual void* Compress(void *_pSource, void *_pDestination, mint _Len) pure;

    /*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
    	Function:			Decompresses data
    						
    	Parameters:			
    		Source:			The data to be uncompressed
    		Destination:	Where the uncompressed data should go. If this parameter
							is NULL the Decompress function will allocate the memory
							for you
    						
    	Returns:			The pointer supplied to Destination, or if NULL was 
							passed the allocated destination buffer is returned.
    						
    	Comments:			
    \*_____________________________________________________________________________*/
	virtual void* Decompress(void *_pSource, void *_pDestination) pure;


    /*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
    	Function:			Sets the settings for the compressor
    						
    	Parameters:			
    		Settings:		The settings you want to set in the compressor
    \*_____________________________________________________________________________*/
	virtual void SetCompress(CCompressSettings& _Settings) pure;

#ifdef	CPU_ALIGNED_MEMORY_ACCESS
	mint GetUncompressedLength(void *_pSource) 
	{ 
		return MEM_UNALIGNED_READUINT32LE( _pSource ); 
	};
#else
	mint GetUncompressedLength(void *_pSource) 
	{ 
		return *((uint32*)_pSource); 
	};
#endif
	mint GetCompressedLength() 
	{ 
		return m_CompressedLength; 
	};

	virtual mint GetCompSrcExtra() pure;
	virtual mint GetCompDestExtra() pure;
	virtual mint GetDecompSrcExtra() pure;
	virtual mint GetDecompDestExtra() pure;
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Template:			Output bits
						
	Parameters:			
		T:				Must be an unsigned integer. It must be atleast as big 
						as the the greatest number to be output + 7 bits.
						For ex. if T=uint16 then you can output numbers as big 
						as 2^(16-7) - 1. T=uint64 is not supported at the moment.
						
	Comments:			A little class that's handy for output of numbers
						that aren't a byte,word or dword but rather 7,14
						or 25 bits for example.
\*_____________________________________________________________________________*/
template <class T>
class COutputBits 
{
	uint8* m_pStart;
	uint8* m_pDest;
	uint32 m_BitPos;

	uint32 m_Masks[32];

#ifdef _DEBUG
	uint32 m_Len;
#endif

public:

	COutputBits(void * _pDst, uint32 _MaxLen)
	{
		m_pStart = m_pDest = (uint8*)_pDst;
		m_BitPos = 0;

		for (uint32 i = 0; i<32; i++) 
			m_Masks[i] = (1 << i)-1;

		memset(m_pDest,0,_MaxLen);

#ifdef _DEBUG
		m_Len = _MaxLen;
#endif
	}

	void Add(T _Number, uint8 _NrBits)
	{
#ifdef	CPU_ALIGNED_MEMORY_ACCESS
		if( sizeof( T ) == 4 )
		{
			T value = MEM_UNALIGNED_READUINT32LE( m_pDest );
			value |= ((_Number & m_Masks[_NrBits]) << m_BitPos);
			MEM_UNALIGNED_WRITEUINT32LE( m_pDest, value );
		}
		else if( sizeof( T ) == 2 )
		{
			T value = MEM_UNALIGNED_READUINT16LE( m_pDest );
			value |= ((_Number & m_Masks[_NrBits]) << m_BitPos);
			MEM_UNALIGNED_WRITEUINT16LE( m_pDest, value );
		}
		else if( sizeof( T ) == 1 )
			*((T*)m_pDest) |= ((_Number & m_Masks[_NrBits]) << m_BitPos);

#else
		*((T*)m_pDest) |= ((_Number & m_Masks[_NrBits]) << m_BitPos);
#endif
		if ((m_BitPos += _NrBits) >= 8) 
		{
			m_pDest += (m_BitPos >> 3);
			m_BitPos &= 7;
		}

#ifdef _DEBUG
		if (GetLength() > m_Len)
			Error_static("COutputBits::Add","Out of range.");
#endif
	}

	void *GetPointer() 
	{ 
		if (m_BitPos>0) 
			return (void*)(m_pDest+1); 
		else 
			return (void*)(m_pDest); 
	};

	uint32 GetLength() 
	{ 
		return (uint32)((uint8*)GetPointer()-m_pStart); 
	};

	void SetPointer(void* _pDst)
	{
		m_pDest=(uint8*)_pDst;
		m_BitPos=0;
	}
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Template:			Short_desscription
						
	Parameters:			
		T:				Must be an unsigned integer. It must be atleast as big 
						as the the greatest number to be input + 7 bits.
						For ex. if T=uint16 then you can input numbers as big 
						as 2^(16-7) - 1. T=uint64 is not supported at the moment.
						
	Comments:			A little class that's handy for input of numbers
						that aren't a byte,word or dword.
\*_____________________________________________________________________________*/
template <class T>
class CInputBits 
{

	uint8* m_pStart;
	uint8* m_pSrc;
	uint32 m_BitPos;

	uint32 m_Masks[32];
public:

	CInputBits(void* _pSrc)
	{
		m_pStart = m_pSrc = (uint8*)_pSrc;
		m_BitPos = 0;

		for (uint32 i = 0; i < 32; i++) 
			m_Masks[i] = (1 << i) - 1;
	}

	T Get(uint8 _NrBits)
	{
#ifdef	CPU_ALIGNED_MEMORY_ACCESS
		T Number;

		if( sizeof( T ) == 4 )
			Number = ( MEM_UNALIGNED_READUINT32LE( m_pSrc ) >> m_BitPos ) & m_Masks[_NrBits];
		else if( sizeof( T ) == 2 )
			Number = ( MEM_UNALIGNED_READUINT16LE( m_pSrc ) >> m_BitPos ) & m_Masks[_NrBits];
		else if( sizeof( T ) == 1 )
			Number = (*((T*)m_pSrc) >> m_BitPos) & m_Masks[_NrBits];

#else
		T Number=(*((T*)m_pSrc) >> m_BitPos) & m_Masks[_NrBits];
#endif
		if ((m_BitPos += _NrBits) >= 8) 
		{
			m_pSrc += (m_BitPos >> 3);
			m_BitPos &= 7;
		}

		return Number;
	}

	void *GetPointer() 
	{ 
		if (m_BitPos>0) 
			return (void*)(m_pSrc+1); 
		else 
			return (void*)(m_pSrc); 
	};

	uint32 GetLength() 
	{ 
		return (uint32)((uint8*)GetPointer()-m_pStart); 
	};

	void SetPointer(void* _pSrc)
	{
		m_pSrc=(uint8*)_pSrc;
		m_BitPos=0;
	}
};



/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Template:			Element in a priority queue
						
	Parameters:			
		T:				Element type
\*_____________________________________________________________________________*/
template <class T>
class TQElem 
{
public:
	T m_Element;

	uint32 m_Priority;

	bool operator < (const TQElem& _qe) const
	{
		return m_Priority <= _qe.m_Priority;	// Without = it wouldn't be a queue.
	}

	bool operator == (const TQElem& _qe) const
	{
		return m_Priority == _qe.m_Priority;
	}
};

#define TQueueElem TQElem

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Template:			A binary search-tree based priority queue.
						
	Parameters:			
		T:				The element Type
\*_____________________________________________________________________________*/
template <class T>
class TPriorityQueue : public TSearchTree<TQueueElem<T>,0> 
{

	TTNode<TQueueElem<T>,2>* m_pRightMostChild;

public:

	TPriorityQueue()
	{
		m_pRightMostChild=NULL;
	}

	void Clear();

	void Put(const T& _Element, uint32 _Priority);
	T Get();

//	uint16 GetSize() { return GetOrder(); };
};

#define NOT_COMPRESSED	0
#define COMPRESSED		1

/***************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
|  CHuffmanTree
|___________________________________________________________________________________________________
\***************************************************************************************************/


// Formerly HuffmanBuildElement
struct CHMBE 
{
	uint32 m_Frequency;
	int32 m_Character;
};

// Formerly HuffmanRebuildElement
struct CHMRBE 
{
	int32 m_Character;
};

#define CHMBuildElem CHMBE
#define CHMRebuildElem CHMRBE

// Formerly HuffmanCode
struct CHMCode 
{
	uint8 m_Code[32+4];	// 4 extra bytes because of COutputBits
	uint8 m_NrBits;		// Number of bits - 1
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				The tree used to calculate the Huffman codes.
						
	Comments:			A Huffman code is the optimal code to represent
						a certain character depending on how often it
						occurs in the data stream.
						
						As the tree will be built from bottom and up, the usual tree
						class can't be used. We will use the usual tree node though.


\*_____________________________________________________________________________*/
class MCCDLLEXPORT CHuffmanTree : public CReferenceCount 
{

	// Used by build.
	TTNode<CHMBuildElem,2>** m_pLeafs;
	uint32* m_pFrequencies;
	CHMCode* m_pCodes;

	uint8 m_InputSize;

	uint32 m_NrLeafs;			// Number of leafs.
	uint32 m_NrCharacters;	// Number of used leafs.

	uint32 m_EncodedSize;		// Length of the encoded data.

	// Used by rebuild.
	TTNode<CHMRebuildElem,2>* m_pRoot;

	void CalculateFrequencies(void *_pSource, uint32 _pLen, uint8 _pInputSize = 8);

public:

#define MAX_FREQUENCY	0xffffffff		//((1 << 32)-1)

	CHuffmanTree()
	{
		m_pFrequencies = NULL;
		m_pCodes = NULL;
		m_pLeafs = NULL;

		m_pRoot = NULL;
	}

	~CHuffmanTree()
	{
		if (m_pCodes) 
			delete[] m_pCodes;

		if (m_pLeafs) 
			delete[] m_pLeafs;

		if (m_pRoot) 
			delete m_pRoot;
	}

	void Build(void *_pSource, uint32 _Len, uint8 _InputSize = 8);
	uint8 Rebuild(CInputBits<uint32>& _Input);
	uint32 GetEncodedSize();

	CHMCode* GetCodes()
	{
		if (!m_pCodes)
			Error("GetCodes","There are no codes.");

		return m_pCodes;
	}

	uint32 GetCharacter(CInputBits<uint32> &_Input)
	{
		TTNode<CHMRebuildElem,2>* pNode = m_pRoot;

		// Convert a code to a character.
		while (pNode->GetElement().m_Character==-1)
			pNode = pNode->GetChild(_Input.Get(1));

		return pNode->GetElement().m_Character;
	}

	void OutputCodeTable(COutputBits<uint32> &_Output);
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Huffman encoding and decoding.
\*_____________________________________________________________________________*/
class MCCDLLEXPORT CHuffman : public CCompress 
{

	uint8 m_InputSize;

public:
	CHuffman()
	{
		m_InputSize=8;
	}

	void SetCompress(CCompressSettings& _Settings);

	mint GetCompSrcExtra() 
	{ 
		return (m_InputSize == 8) ? 0 : 4; 
	};

	mint GetCompDestExtra() { return 9;};
	mint GetDecompSrcExtra() { return 4;};
	mint GetDecompDestExtra() { return 4;};

	void* Compress(void *_pSource, void *_pDestination, mint _Len);
	void* Decompress(void *_pSource, void *_pDestination);
};


// For now CHARACTER_SIZE must be 8.
#define CHARACTER_SIZE	8

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Short_desscription
						
	Comments:			Lempel-Ziv Welch compression and decompression.
						Huffman will soon be an option.
						LZW belongs to the LZ78 family.

\*_____________________________________________________________________________*/
class MCCDLLEXPORT CLZW : public CCompress 
{

	uint8 m_MinBits;
	uint8 m_MaxBits;
	bool m_bUseControlCodes;

	void* CompressWithCC(void *_pSource, void *_pDestination, mint _Len);
	void* DecompressWithCC(void *_pSource, void *_pDestination);

public:

	CLZW()
	{
		m_MinBits = 0;	// 0 means not used.
		m_MaxBits = 13;
		m_bUseControlCodes = false;
	}

	void SetCompress(CCompressSettings& _Settings);

	mint GetCompSrcExtra() { return 0; };
	mint GetCompDestExtra() { return 10; };
	mint GetDecompSrcExtra() { return 4; };
	mint GetDecompDestExtra() { return 0; };

/*
	struct StringEntryComp {
		uint16 Code;			// Code of the string itself.
		uint16 PrefixCode;		// Code of its prefix string.
		uint8 Character;		// Its character to be added to its prefix.
	};
*/
	struct StringEntryDecomp 
	{
		uint8* m_pStr;
		uint16 m_Len;
	};

	void* Compress(void *_pSource, void *_pDestination, mint _Len);
	void* Decompress(void *_pSource, void *_pDestination);
};



namespace NIds
{
	namespace NDataProcessing
	{


		extern ch8 g_Base64EncodingTable[65];
		extern int8 g_Base64EncodingTableReverse[256];

		class MCCDLLEXPORT CBinaryStream_Base64
		{
		public:

			CCFile *m_pStream;
			int m_OpenFlags;

			enum 
			{
				EChunkSizeBase64 = 4,
				EChunkSizeData = 3
			};


			fint m_FilePos;
			fint m_CurrentLoaded;
			fint m_FileLen;
			uint8 m_DecryptedData[EChunkSizeData];
			bint m_bCurrentDirty;

		public:
			
			CBinaryStream_Base64();
			~CBinaryStream_Base64();

			void Close();
			void Open(CCFile *_pStream, int _OpenFlags);
			void WriteDirty();
			aint PrepareBlock(fint _Pos, bint _bWrite);
			void FeedBytes(const void *_pMem, mint _nBytes);
			void ConsumeBytes(void *_pMem, mint _nBytes);
			bint IsValid();
			bint IsAtEndOfStream();
			fint GetPosition();
			void SetPosition(fint _Pos);
			void SetPositionFromEnd(fint _Pos);
			void AddPosition(fint _Pos);
			fint GetLength();
		};

		CStr MCCDLLEXPORT Base64EncodeData(const void *_pData, mint _Len);
		TArray<uint8> MCCDLLEXPORT Base64DecodeData(CStr _String);

		CStr MCCDLLEXPORT Base64EncodeStr(CStr _String);
		CStr MCCDLLEXPORT Base64DecodeStr(CStr _String);
	
	}
}

using namespace NIds::NDataProcessing;


#if 0

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Fast Lempel-Ziv Welch compression and decompression.
						
	Comments:			Huffman will soon be an option.

						LZW belongs to the LZ78 family.

						Den här är fan så mycket snabbare än CLZW. Tyvärr slukar 
						den minne som inget annat jag sett!! Kommer du på något 
						mer komprimerat sätt att representera barnen i LZWNode 
						så hojta till! Det görs nämligen 2^maxbits stycken 
						instanser av den (skratta inte!). För att komprimera 
						en GIF med maxbits=12 går det alltså åt exakt 2 Mb.

						Decompressorn däremot kräver inte alls lika mycket minne.
\*_____________________________________________________________________________*/
class CLZWFast : public CCompress 
{
	uint8 m_MinBits;
	uint8 m_MaxBits;
	bool m_bUseControlCodes;

	void* DecompressWithCC(void *_pSource, void *_pDestination);

public:

	CLZWFast()
	{
		m_MinBits=0;	// 0 means not used.
		m_MaxBits=12;
		m_UseControlCodes=FALSE;
	}

	void SetCompress(CCompressSettings& _Settings);

	struct LZWNode 
	{
		uint16 m_Children[256];
	};

	struct StringEntryDecomp 
	{
		uint8* m_pStr;
		uint16 m_Len;
	};

	void* Compress(void *_pSource, void *_pDestination, uint32 _Len);
	void* Decompress(void *_pSource, void *_pDestination);
};
#endif


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Lempel-Ziv Storer-Szymanski compression and decompression.
						
	Comments:			Huffman encoding is optional.
						LZSS belongs to the LZ77 family.
						String matching is done using 65536 binary search trees.
						I'm searching for a faster search-structure, maybe
						red-black trees would be the thing. I tried 4 and 8-way
						trees but got no improvement.
\*_____________________________________________________________________________*/

class MCCDLLEXPORT CLZSS : public CCompress 
{
	struct LZSSNode 
	{	
		// The binary tree node.
		uint16 m_Parent;
		uint16 m_Left;
		uint16 m_Right;
		uint8* m_pStringRef;
	};

	uint32 m_WindowSize;
	uint32 m_MinMatchLength;
	uint32 m_MaxMatchLength;

	bool m_bUseHuffman;

	uint32 m_OffsetSize;
	uint32 m_LengthSize;

	// Temporary variables.
	uint8* m_pSrc;
	uint8* m_pSrcCopy;
	mint m_SrcLen;

	uint8 GetSrc(uint8* _pSrc, int _Pos);

	uint8* m_pWindow;
	uint32 m_WindowMove;
	uint32 m_ChildPos;

	uint16* m_pRoots;
	LZSSNode* m_pChildren;
	mint m_BestMatchPos;
	uint32 m_BestMatchLength;

	uint16 m_Tree;
	uint16 m_Node;
	uint16 m_Parent;
	int32 m_Diff;
	mint m_MaxLen;
//	uint32 m_Len;
	uint8* m_pSrc2;

//	uint32 i,j;
	uint32 m_ReplaceWith;

#define EMPTY	0xffff
#define ROOT	0xfffe

	void InsertNewStrings();
	void DeleteOldStrings();
	void FindMaxMatch(uint32 _Len);

	void* CompressWithHuffman(void *_pSource, void *_pDestination, mint _Len);
	void* DecompressWithHuffman(void *_pSource, void *_pDestination);

public:

	CLZSS()
	{
		m_WindowSize = 4096;
		m_MaxMatchLength = 18;

		m_bUseHuffman = false;

		m_MinMatchLength = 3;
		m_OffsetSize = 12;
		m_LengthSize = 4;
	}

	void SetCompress(CCompressSettings& _Settings);

	mint GetCompSrcExtra() 
	{ 
		return 0; 
	};

	mint GetCompDestExtra() 
	{
		if (m_bUseHuffman) 
		{
			CHuffman Huff;
			return 4+10+4*5+5*Huff.GetCompDestExtra();
		}
		else
			return 14;
	};

	mint GetDecompSrcExtra() { return 4; };
	mint GetDecompDestExtra() { return 0; };

	void* Compress(void *_pSource, void *_pDestination, mint _Len);
	void* Decompress(void *_pSource, void *_pDestination);
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Class:				ZLIB encoding and decoding.
\*_____________________________________________________________________________*/
class MCCDLLEXPORT CZLIB : public CCompress 
{

private:

	EZLIBLevels	m_Level;
	EZLIBStrategies	m_Strategy;

#ifdef	CPU_ALIGNED_MEMORY_ACCESS
	mint GetZLIBCompressedLength(const void *_pSource) const
	{ 
		uint32 * Src = ((uint32*)_pSource) + 1;
		return MEM_UNALIGNED_READUINT32LE( Src ); 
	};
#else
	mint GetZLIBCompressedLength(const void *_pSource) const
	{ 
		return *(((uint32*)_pSource)+1); 
	};
#endif

public:

	CZLIB()
	{
		m_Level = ZLIB_HIGH_COMPRESSION;
		m_Strategy = ZLIB_STRATEGY_DEFAULT;
	}

	CZLIB(EZLIBLevels _Level,EZLIBStrategies _Strategy = ZLIB_STRATEGY_DEFAULT)
	{
		m_Level = _Level;
		m_Strategy = _Strategy;
	}

	void SetCompress(CCompressSettings& _Settings);

	mint GetCompSrcExtra() { return 0;};
	mint GetCompDestExtra() { return 8;};
	mint GetDecompSrcExtra() { return 8;};
	mint GetDecompDestExtra() { return 0;};

	void* Compress(void *_pSource, void *_pDestination, mint _Len);
	void* Decompress(void *_pSource, void *_pDestination);
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				A class that makes the use of the different 
						compressors a bit easier.
\*_____________________________________________________________________________*/
class MCCDLLEXPORT CCompressorInterface : public CObj 
{
public:
	CCompressorInterface()
	{
	}

	CCompress* GetCompressor(ECompressTypes _Type=ZLIB, ESettings _Set = HIGH_COMPRESSION)
	{
		CCompress* pCompressor;

		bool bNoSupport = FALSE;

		switch (_Type) 
		{
		case HUFFMAN: 
			pCompressor = MNew(CHuffman); 
			break;

		case LZW: 
			{

				pCompressor = MNew(CLZW);

				if (pCompressor) 
				{
					CLZWSettings lzset;
					switch (_Set) 
					{
					case LOW_COMPRESSION: lzset.SetMaximumBits(12); break;
					case NORMAL_COMPRESSION: lzset.SetMaximumBits(13); break;
					case HIGH_COMPRESSION: lzset.SetMaximumBits(14); break;
					};
					pCompressor->SetCompress(lzset);
				}
			}
		break;

		case LZSS: 
			{

			pCompressor = MNew(CLZSS);
			if (pCompressor) 
			{
				CLZSSSettings lzset;
				switch (_Set) 
				{
				case LOW_COMPRESSION: 
					{
						lzset.SetWindowSize(4096);
						lzset.SetMaxMatchLength(18);
					}
				break;
				case NORMAL_COMPRESSION: 
					{
						lzset.SetWindowSize(8192);
						lzset.SetMaxMatchLength(18);
					}
				break;
				case HIGH_COMPRESSION: 
					{
						lzset.SetWindowSize(16384);
						lzset.SetMaxMatchLength(18);
					}
				break;
				};

				pCompressor->SetCompress(lzset);
			}
		}
		break;

		case LZSS_HUFFMAN: 
			{
			pCompressor = MNew(CLZSS);
			if (pCompressor != NULL) 
			{
				CLZSSSettings lzset;
				lzset.SetHuffman(TRUE);
				switch (_Set) 
				{
				case LOW_COMPRESSION: 
					{
						lzset.SetWindowSize(1024);
						lzset.SetMaxMatchLength(96);
					}
				break;
				case NORMAL_COMPRESSION: 
					{
						lzset.SetWindowSize(2048);
						lzset.SetMaxMatchLength(96);
					}
				break;
				case HIGH_COMPRESSION: 
					{
						lzset.SetWindowSize(4096);
						lzset.SetMaxMatchLength(96);
					}
				break;
				};
				pCompressor->SetCompress(lzset);
			}
		}
		break;

		case ZLIB:
			{
				pCompressor = MNew(CZLIB);
				if( pCompressor != NULL)
				{
					CZLIBSettings ZLIBSet;
					switch (_Set)
					{

					case LOW_COMPRESSION:
						ZLIBSet.SetLevel(ZLIB_LOW_COMPRESSION);
						break;

					case NORMAL_COMPRESSION:
						ZLIBSet.SetLevel(ZLIB_STANDARD_COMPRESSION);
						break;

					case HIGH_COMPRESSION:
						ZLIBSet.SetLevel(ZLIB_HIGH_COMPRESSION);
						break;
					}
					pCompressor->SetCompress(ZLIBSet);
				}
			}
		break;

		default: pCompressor = NULL; bNoSupport = true; break;

		};

		if (!pCompressor && !bNoSupport) 
			MemError("GetCompressor");

		return pCompressor;
	}
};


#endif




