
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
		051013:		Added ZLIB enums // Anders Ekermo
\*_____________________________________________________________________________________________*/


#ifndef _INC_MFILEDEF
#define _INC_MFILEDEF

enum ECompressTypes {NO_COMPRESSION, RLE, HUFFMAN, LZW, LZW_GIF, LZW_HUFFMAN, LZSS, LZSS_HUFFMAN, ZLIB};
enum ESettings {LOW_COMPRESSION, NORMAL_COMPRESSION, HIGH_COMPRESSION};

// Remember to change this if they are changed in zlib.h!
enum EZLIBLevels {ZLIB_NO_COMPRESSION = 0,ZLIB_LOW_COMPRESSION = 1,ZLIB_STANDARD_COMPRESSION = -1,
					ZLIB_HIGH_COMPRESSION = 9};
enum EZLIBStrategies {ZLIB_STRATEGY_DEFAULT = 0,ZLIB_STRATEGY_FILTERED = 1,
					ZLIB_STRATEGY_HUFFMAN_ONLY = 2,ZLIB_STRATEGY_RLE = 3};

#include "MFile_Stream.h"
/*
class IFile : public CReferenceCount
{
public:
	virtual void Open(TArray<uint8> _lStream, int _Mode, int _MinGrow = 32768) pure;
	virtual void Open(CStream* _pStream, int _Mode, bool _bOwnStream = false) pure;
	virtual void Open(CStr _Name, int _Mode, ECompressTypes _eType = NO_COMPRESSION, ESettings _eSet = NORMAL_COMPRESSION) pure;
	
	virtual void OpenExt(CStream* _pStream, int _Mode, bool _bOwnStream = false, float _Priority = 0) pure;
	virtual void OpenExt(CStr _Name, int _Mode, ECompressTypes _eType = NO_COMPRESSION, ESettings _eSet = NORMAL_COMPRESSION, float _Priority = 0) pure;
	
	virtual void Close() pure;
	virtual CStr GetFileName() pure;

	virtual void Read(void* _pDest, uint32 _Size) pure;
	virtual void Write(const void* _pSrc, uint32 _Size) pure;
	virtual CStr Readln() pure;
	virtual void Writeln(CStr _s) pure;

	virtual void Rename(const CStr& _Name) pure;
	virtual void Remove() pure;

	virtual int32 Pos() pure;
	virtual int32 Length() pure;
	virtual void Seek(int32 _Pos) pure;
	virtual void SeekToEnd() pure;
	virtual void RelSeek(int32 _Pos) pure;
	virtual bool EndOfFile() pure;

	virtual void Read(uint8& _Value) pure;
	virtual void Write(uint8 _Value) pure;

	// Little endian --------------------------------
	virtual void ReadLE(uint8& _Value) pure;
	virtual void ReadLE(int16& _Value) pure;
	virtual void ReadLE(uint16& _Value) pure;
	virtual void ReadLE(int32& _Value) pure;
	virtual void ReadLE(uint32& _Value) pure;
	virtual void ReadLE(int64& _Value) pure;
	virtual void ReadLE(uint64& _Value) pure;
	virtual void ReadLE(fp32& _Value) pure;
	virtual void ReadLE(fp64& _Value) pure;

	virtual void WriteLE(const uint8& _Value) pure;
	virtual void WriteLE(const int16& _Value) pure;
	virtual void WriteLE(const uint16& _Value) pure;
	virtual void WriteLE(const int32& _Value) pure;
	virtual void WriteLE(const uint32& _Value) pure;
	virtual void WriteLE(const int64& _Value) pure;
	virtual void WriteLE(const uint64& _Value) pure;
	virtual void WriteLE(const fp32& _Value) pure;
	virtual void WriteLE(const fp64& _Value) pure;

	virtual void ReadLE(uint8* _pData, int _Count) pure;
	virtual void ReadLE(uint16* _pData, int _Count) pure;
	virtual void ReadLE(uint32* _pData, int _Count) pure;
	virtual void ReadLE(int8* _pData, int _Count) pure;
	virtual void ReadLE(int16* _pData, int _Count) pure;
	virtual void ReadLE(int32* _pData, int _Count) pure;

	virtual void WriteLE(uint8* _pData, int _Count) pure;
	virtual void WriteLE(uint16* _pData, int _Count) pure;
	virtual void WriteLE(uint32* _pData, int _Count) pure;
	virtual void WriteLE(int8* _pData, int _Count) pure;
	virtual void WriteLE(int16* _pData, int _Count) pure;
	virtual void WriteLE(int32* _pData, int _Count) pure;

	// Big endian --------------------------------
	virtual void ReadBE(uint8& _Value) pure;
	virtual void ReadBE(int16& _Value) pure;
	virtual void ReadBE(uint16& _Value) pure;
	virtual void ReadBE(int32& _Value) pure;
	virtual void ReadBE(uint32& _Value) pure;
	virtual void ReadBE(int64& _Value) pure;
	virtual void ReadBE(uint64& _Value) pure;
	virtual void ReadBE(fp32& _Value) pure;
	virtual void ReadBE(fp64& _Value) pure;

	virtual void WriteBE(const uint8& _Value) pure;
	virtual void WriteBE(const int16& _Value) pure;
	virtual void WriteBE(const uint16& _Value) pure;
	virtual void WriteBE(const int32& _Value) pure;
	virtual void WriteBE(const uint32& _Value) pure;
	virtual void WriteBE(const int64& _Value) pure;
	virtual void WriteBE(const uint64& _Value) pure;
	virtual void WriteBE(const fp32& _Value) pure;
	virtual void WriteBE(const fp64& _Value) pure;
};
*/
// -------------------------------------------------------------------
//  CLogFile
// -------------------------------------------------------------------
class ILogFile : public CReferenceCount
{
	MRTC_DECLARE;
public:
	virtual void Create(CStr _FileName, bool _bAppend = false) pure;
	virtual void Log(const CStr& _s) pure;
	virtual void Log(const char* _pStr) pure;
	virtual void SetFileName(const CStr& _s, bool _bAppend = false) pure;
};

// -------------------------------------------------------------------
//  IProgress
// -------------------------------------------------------------------
class IProgress : public CReferenceCount
{
	MRTC_DECLARE;
public:
	IProgress* m_pNextProgress;

	virtual void Push(const char* _pLevelName = NULL) pure;
	virtual void Pop() pure;

	virtual void SetProgress(fp32 _p, const char* _pTaskName = NULL) pure;
	virtual void InitProgressCount(int _Count, const char* _pTaskName = NULL) pure;
	virtual void SetProgressText(const char* _pTaskName = NULL) pure;
	virtual void IncProgress(const char* _pTaskName = NULL) pure;
};

#endif // _INC_MFILEDEF
