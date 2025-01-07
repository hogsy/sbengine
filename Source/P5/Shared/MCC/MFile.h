
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/

#ifndef _INC_MFile
#define _INC_MFile

#include "MCCInc.h"

class CCFile;
#include "MDA.h"

#include "MFile_StreamMgr.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CStream
|__________________________________________________________________________________________________
\*************************************************************************************************/
enum
{
	CFILE_READ=1
	,CFILE_WRITE=2
	,CFILE_BINARY=4
	,CFILE_APPEND=8
	,CFILE_TRUNC=16		// Force truncation even if CFILE_READ was specified.
	,CFILE_HIDEPOS=32	// Pos() on the stream or file will generate an exception. Used to force compliance with CDataFile creation v3.0
	,CFILE_UNICODE=64	// Text is written in unicode format if the file is not in binary mode.
	,CFILE_NOLOG=128
	,CFILE_DISCARDCONTENTS=256
	,CFILE_NODEFERCLOSE=512
};

#include "MFile_Stream.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CCFile
|__________________________________________________________________________________________________
\*************************************************************************************************/

#include "MFileDef.h"
#include "MFile_Stream_Disk.h"
#include "MFile_Stream_XDF.h"

#define CFILE_CHECKSTREAM(funcname) \
M_ASSERT(m_pStream, "Stream not open");

enum
{
	ECCFile_Stream_Default
	,ECCFile_Stream_Disk
	,ECCFile_Stream_XDF

};

class MCCDLLEXPORT CCFile : public CReferenceCount
{
	CStr m_FileName;
	CStream* m_pStream;
	int32 m_StreamType;
	int32 m_bStreamAllocated;
#ifndef PLATFORM_CONSOLE
	int32 m_iDisableWrite;
#endif

	void DisconnectStream();
	void ConnectStream(int _mode);
	void ConnectMemoryStream(TArray<uint8> _lStream, int _mode, int _MinGrow);
	void ConnectMemoryStream(void * _pStream, int _CurrentLen, int _MaxLen, int _Mode);

public:
	
	DECLARE_OPERATOR_NEW
	

	CCFile();
	~CCFile();

	CStream *GetStream()
	{
		return m_pStream;
	}
	
	void Open(TArray<uint8> _lStream, int _Mode, int _MinGrow = 32768);
	void Open(void * _pStream, int _CurrentLen, int _MaxLen, int _Mode);
	void Open(CStream* _pStream, int _Mode, bool _bOwnStream = false);
	void Open(CStr _Name, int _Mode, ECompressTypes _eType = NO_COMPRESSION, ESettings _eSet = NORMAL_COMPRESSION);
//	void OpenExt(CStream* _pStream, int _Mode, bool _bOwnStream = false, float _Priority = 0, int _NumCacheLines = -1, int _CacheLineSize = -1);
	void OpenExt(CStr _Name, int _Mode, ECompressTypes _eType = NO_COMPRESSION, ESettings _eSet = NORMAL_COMPRESSION, float _Priority = 0, aint _NumCacheLines = -1, aint _CacheLineSize = -1);
	void SetPriority(fp32 _Priority);
	void Close();
	CStr GetFileName();
	bint IsOpen();

	void Align( int _Alignment );
	void Write(const void* src, mint size);
	void Read(void* dest, mint size);
	
	bool Read(CAsyncRequest *_pRequest);
	bool Write(CAsyncRequest *_pRequest);
	bool AsyncFlush(bool _bBlock) {return m_pStream->AsyncFlush(_bBlock);};

	#ifndef PLATFORM_CONSOLE
	void DisableWrite()
	{
		++m_iDisableWrite;
	}
	void EnableWrite()
	{
		--m_iDisableWrite;
	}
	#endif

	CStr Readln();
	void Writeln(const char* _pStr);
	void Writeln(const wchar* _pStr);
	void Writeln(CStr _s);

	void Rename(const CStr& _Name);
	void Remove();


	void Write(uint8 value);
	void Read(uint8& value);
	bool EndOfFile();
	void RelSeek(fint pos);
	void SeekToEnd();
	void Seek(fint pos);
	fint Length();
	fint Pos() ;

	// --------------------------------
	#ifdef CPU_LITTLEENDIAN
		#define MACRO_CFILE_READ_LE(T, Name, Swapper, TS)	\
		void Name(T& _Value)					\
		{												\
			CFILE_CHECKSTREAM(#Name);				\
			switch (m_StreamType)						\
			{											\
			case ECCFile_Stream_XDF:					\
				((CStream_XDF *)m_pStream)->INL_Read(&_Value, sizeof(T));		\
				break;									\
			case ECCFile_Stream_Disk:					\
				((CStream_Disk *)m_pStream)->m_Stream.Read(&_Value, sizeof(T));		\
				break;									\
			default:									\
				m_pStream->Read(&_Value, sizeof(T));	\
				break;									\
			}											\
		}
	#else
		#define MACRO_CFILE_READ_LE(T, Name, Swapper, TS)	\
		void Name(T& _Value)					\
		{												\
			CFILE_CHECKSTREAM(#Name);				\
			switch (m_StreamType)						\
			{											\
			case ECCFile_Stream_XDF:					\
				((CStream_XDF *)m_pStream)->INL_Read(&_Value, sizeof(T));		\
				break;									\
			case ECCFile_Stream_Disk:					\
				((CStream_Disk *)m_pStream)->m_Stream.Read(&_Value, sizeof(T));		\
				break;									\
			default:									\
				m_pStream->Read(&_Value, sizeof(T));	\
				break;									\
			}											\
			Swapper(*(TS*)&_Value);							\
		}
	#endif

	MACRO_CFILE_READ_LE(int8, ReadLE, Swap_NoSwap, int8);
	MACRO_CFILE_READ_LE(uint8, ReadLE, Swap_NoSwap, uint8);
	MACRO_CFILE_READ_LE(int16, ReadLE, Swap_uint16, uint16);
	MACRO_CFILE_READ_LE(uint16, ReadLE, Swap_uint16, uint16);
	MACRO_CFILE_READ_LE(int32, ReadLE, ByteSwap_uint32, uint32);
	MACRO_CFILE_READ_LE(uint32, ReadLE, ByteSwap_uint32, uint32);
	MACRO_CFILE_READ_LE(int64, ReadLE, ByteSwap_uint64, uint64);
	MACRO_CFILE_READ_LE(uint64, ReadLE, ByteSwap_uint64, uint64);
	MACRO_CFILE_READ_LE(fp32, ReadLE, ByteSwap_uint32, uint32);
	MACRO_CFILE_READ_LE(fp64, ReadLE, ByteSwap_uint64, uint64);
#ifdef	M_WCHARDISTINCTTYPE
	MACRO_CFILE_READ_LE(wchar, ReadLE, Swap_wchar, wchar);
#endif


	// --------------------------------
	#ifdef CPU_LITTLEENDIAN
#ifndef PLATFORM_CONSOLE
		#define MACRO_CFILE_WRITE_LE(T, Name, Swapper, TS)	\
		void Name(const T& _Value)					\
		{												\
			if (m_iDisableWrite)						\
			{											\
				m_pStream->RelSeek(sizeof(T));			\
				return;									\
			}											\
			CFILE_CHECKSTREAM(#Name);				\
			switch (m_StreamType)						\
			{											\
			case ECCFile_Stream_XDF:					\
				((CStream_XDF *)m_pStream)->INL_Write(&_Value, sizeof(T));		\
				break;									\
			case ECCFile_Stream_Disk:					\
				((CStream_Disk *)m_pStream)->m_Stream.Write(&_Value, sizeof(T));		\
				break;									\
			default:									\
				m_pStream->Write(&_Value, sizeof(T));		\
				break;									\
			}											\
		}
#else
		#define MACRO_CFILE_WRITE_LE(T, Name, Swapper, TS)	\
		void Name(const T& _Value)				\
		{												\
			CFILE_CHECKSTREAM(#Name);				\
			switch (m_StreamType)						\
			{											\
			case ECCFile_Stream_XDF:					\
				((CStream_XDF *)m_pStream)->INL_Write(&_Value, sizeof(T));		\
				break;									\
			case ECCFile_Stream_Disk:					\
				((CStream_Disk *)m_pStream)->m_Stream.Write(&_Value, sizeof(T));		\
				break;									\
			default:									\
				m_pStream->Write(&_Value, sizeof(T));		\
				break;									\
			}											\
		}
#endif
	#else
		#define MACRO_CFILE_WRITE_LE(T, Name, Swapper, TS)	\
		void Name(const T& _Value)				\
		{												\
			CFILE_CHECKSTREAM(#Name);				\
			T v(_Value);								\
			Swapper(*(TS*)&v);									\
			switch (m_StreamType)						\
			{											\
			case ECCFile_Stream_XDF:					\
				((CStream_XDF *)m_pStream)->INL_Write(&v, sizeof(T));		\
				break;									\
			case ECCFile_Stream_Disk:					\
				((CStream_Disk *)m_pStream)->m_Stream.Write(&v, sizeof(T));		\
				break;									\
			default:									\
				m_pStream->Write(&v, sizeof(T));		\
				break;									\
			}											\
		}
	#endif

	MACRO_CFILE_WRITE_LE(int8, WriteLE, Swap_NoSwap, int8);
	MACRO_CFILE_WRITE_LE(uint8, WriteLE, Swap_NoSwap, uint8);
	MACRO_CFILE_WRITE_LE(int16, WriteLE, Swap_uint16, uint16);
	MACRO_CFILE_WRITE_LE(uint16, WriteLE, Swap_uint16, uint16);
	MACRO_CFILE_WRITE_LE(int32, WriteLE, ByteSwap_uint32, uint32);
	MACRO_CFILE_WRITE_LE(uint32, WriteLE, ByteSwap_uint32, uint32);
	MACRO_CFILE_WRITE_LE(int64, WriteLE, ByteSwap_uint64, uint64);
	MACRO_CFILE_WRITE_LE(uint64, WriteLE, ByteSwap_uint64, uint64);
	MACRO_CFILE_WRITE_LE(fp32, WriteLE, ByteSwap_uint32, uint32);
	MACRO_CFILE_WRITE_LE(fp64, WriteLE, ByteSwap_uint64, uint64);
#ifdef	M_WCHARDISTINCTTYPE
	MACRO_CFILE_WRITE_LE(wchar, WriteLE, Swap_wchar, wchar);
#endif

	// --------------------------------
	// --------------------------------
	void ReadLE(fp32* _pData, int _Count);
	void ReadLE(fp64* _pData, int _Count);
	void ReadLE(int32* _pData, int _Count);
	void ReadLE(uint32* _pData, int _Count);
	void ReadLE(int16* _pData, int _Count);
	void ReadLE(uint16* _pData, int _Count);
	void ReadLE(int8* _pData, int _Count);
	void ReadLE(uint8* _pData, int _Count);
#ifdef	M_WCHARDISTINCTTYPE
	void ReadLE(wchar* _pData, int _Count);
#endif

	void WriteLE(const fp32* _pData, int _Count);
	void WriteLE(const fp64* _pData, int _Count);
	void WriteLE(const int32* _pData, int _Count);
	void WriteLE(const uint32* _pData, int _Count);
	void WriteLE(const int16* _pData, int _Count);
	void WriteLE(const uint16* _pData, int _Count);
	void WriteLE(const int8* _pData, int _Count);
	void WriteLE(const uint8* _pData, int _Count);
#ifdef	M_WCHARDISTINCTTYPE
	void WriteLE(const wchar* _pData, int _Count);
#endif

	// --------------------------------
	#ifdef CPU_BIGENDIAN

		#define MACRO_CFILE_READ_BE(T, Name, Swapper, TS)	\
		void Name(T& _Value)					\
		{												\
			CFILE_CHECKSTREAM(#Name);				\
			switch (m_StreamType)						\
			{											\
			case ECCFile_Stream_XDF:					\
				((CStream_XDF *)m_pStream)->INL_Read(&_Value, sizeof(T));		\
				break;									\
			case ECCFile_Stream_Disk:					\
				((CStream_Disk *)m_pStream)->m_Stream.Read(&_Value, sizeof(T));		\
				break;									\
			default:									\
				m_pStream->Read(&_Value, sizeof(T));		\
				break;									\
			}											\
		}

	#else

		#define MACRO_CFILE_READ_BE(T, Name, Swapper, TS)	\
		void Name(T& _Value)					\
		{												\
			CFILE_CHECKSTREAM(#Name);				\
			switch (m_StreamType)						\
			{											\
			case ECCFile_Stream_XDF:					\
				((CStream_XDF *)m_pStream)->INL_Read(&_Value, sizeof(T));		\
				break;									\
			case ECCFile_Stream_Disk:					\
				((CStream_Disk *)m_pStream)->m_Stream.Read(&_Value, sizeof(T));		\
				break;									\
			default:									\
				m_pStream->Read(&_Value, sizeof(T));		\
				break;									\
			}											\
			Swapper(*(TS*)&_Value);						\
		}

	#endif


	MACRO_CFILE_READ_BE(int8, ReadBE, Swap_NoSwap, int8);
	MACRO_CFILE_READ_BE(uint8, ReadBE, Swap_NoSwap, uint8);
	MACRO_CFILE_READ_BE(int16, ReadBE, Swap_int16, int16);
	MACRO_CFILE_READ_BE(uint16, ReadBE, Swap_uint16, uint16);
	MACRO_CFILE_READ_BE(int32, ReadBE, ByteSwap_int32, int32);
	MACRO_CFILE_READ_BE(uint32, ReadBE, ByteSwap_uint32, uint32);
	MACRO_CFILE_READ_BE(int64, ReadBE, ByteSwap_int64, int64);
	MACRO_CFILE_READ_BE(uint64, ReadBE, ByteSwap_uint64, uint64);
	MACRO_CFILE_READ_BE(fp32, ReadBE, ByteSwap_int32, int32);
	MACRO_CFILE_READ_BE(fp64, ReadBE, ByteSwap_int64, int64);
#ifdef	M_WCHARDISTINCTTYPE
	MACRO_CFILE_READ_BE(wchar, ReadBE, Swap_wchar, wchar);
#endif

	// --------------------------------
	#ifdef CPU_BIGENDIAN

		#define MACRO_CFILE_WRITE_BE(T, Name, Swapper, TS)	\
		void Name(const T& _Value)				\
		{												\
			CFILE_CHECKSTREAM(#Name);				\
			switch (m_StreamType)						\
			{											\
			case ECCFile_Stream_XDF:					\
				((CStream_XDF *)m_pStream)->INL_Write(&_Value, sizeof(T));		\
				break;									\
			case ECCFile_Stream_Disk:					\
				((CStream_Disk *)m_pStream)->m_Stream.Write(&_Value, sizeof(T));		\
				break;									\
			default:									\
				m_pStream->Write(&_Value, sizeof(T));		\
				break;									\
			}											\
		}

	#else

#ifndef PLATFORM_CONSOLE
		#define MACRO_CFILE_WRITE_BE(T, Name, Swapper, TS)	\
		void Name(const T& _Value)				\
		{												\
			if (m_iDisableWrite)						\
			{											\
				m_pStream->RelSeek(sizeof(T));			\
				return;									\
			}											\
			CFILE_CHECKSTREAM(#Name);				\
			T v(_Value);								\
			Swapper(*(TS*)&v);							\
			switch (m_StreamType)						\
			{											\
			case ECCFile_Stream_XDF:					\
				((CStream_XDF *)m_pStream)->INL_Write(&v, sizeof(T));			\
				break;									\
			case ECCFile_Stream_Disk:					\
				((CStream_Disk *)m_pStream)->m_Stream.Write(&v, sizeof(T));			\
				break;									\
			default:									\
				m_pStream->Write(&v, sizeof(T));			\
				break;									\
			}											\
		}
#else
		#define MACRO_CFILE_WRITE_BE(T, Name, Swapper, TS)	\
		void Name(const T& _Value)				\
		{												\
			CFILE_CHECKSTREAM(#Name);				\
			T v(_Value);								\
			Swapper(*(TS*)&v);							\
			switch (m_StreamType)						\
			{											\
			case ECCFile_Stream_XDF:					\
				((CStream_XDF *)m_pStream)->INL_Write(&v, sizeof(T));			\
				break;									\
			case ECCFile_Stream_Disk:					\
				((CStream_Disk *)m_pStream)->m_Stream.Write(&v, sizeof(T));			\
				break;									\
			default:									\
				m_pStream->Write(&v, sizeof(T));			\
				break;									\
			}											\
		}
#endif

	#endif

	MACRO_CFILE_WRITE_BE(int8, WriteBE, Swap_NoSwap, int8);
	MACRO_CFILE_WRITE_BE(uint8, WriteBE, Swap_NoSwap, uint8);
	MACRO_CFILE_WRITE_BE(int16, WriteBE, Swap_int16, int16);
	MACRO_CFILE_WRITE_BE(uint16, WriteBE, Swap_uint16, uint16);
	MACRO_CFILE_WRITE_BE(int32, WriteBE, ByteSwap_int32, int32);
	MACRO_CFILE_WRITE_BE(uint32, WriteBE, ByteSwap_uint32, uint32);
	MACRO_CFILE_WRITE_BE(int64, WriteBE, ByteSwap_int64, int64);
	MACRO_CFILE_WRITE_BE(uint64, WriteBE, ByteSwap_uint64, uint64);
	MACRO_CFILE_WRITE_BE(fp32, WriteBE, ByteSwap_int32, int32);
	MACRO_CFILE_WRITE_BE(fp64, WriteBE, ByteSwap_int64, int64);
#ifdef	M_WCHARDISTINCTTYPE
	MACRO_CFILE_WRITE_BE(wchar, WriteBE, Swap_wchar, wchar);
#endif

	template<class T>
	void WriteArray(T* _pArray, int _Len)
	{
		for(int i = 0; i < _Len; i++) 
			_pArray[i].Write(this);
	}

	template<class T>
	void ReadArray(T* _pArray, int _Len, int _Version)
	{
		for(int i = 0; i < _Len; i++) 
			_pArray[i].Read(this, _Version);
	}

	template<class T>
	void ReadArray_NoVer(T* _pArray, int _Len)
	{
		for(int i = 0; i < _Len; i++) 
			_pArray[i].Read(this);
	}
};

#ifndef	NO_INLINE_FILESYSTEM
#include "MFile.inl"
#endif

typedef TPtr<CCFile> spCCFile;

#include "MFile_Misc.h"
#include "MFile_Stream_Memory.h"
#include "MFile_Stream_SubFile.h"
#include "MDataFile.h"
#include "MFile_MegaFile.h"

#include "MFile_DiskUtil.h"
#include "MFile_AsyncCopy.h"

#endif // _INC_MFile
