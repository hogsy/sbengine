
#if !defined __INC_MFILE_XTL && defined PLATFORM_XBOX
#define __INC_MFILE_XTL


	#include <xtl.h>

#include "MFile.h"

// XBox Standard library fstream seams to be broken lets replace it with a wrapper class

//#define fstream CfstreamEmulator
#define fstream CfstreamEmulator_CachedAsync

class CfstreamEmulator : public CReferenceCount
{
private:
	HANDLE FileHandle;
	DWORD ErrorFlags;
	DWORD CurrentPosition;
public:
	CfstreamEmulator();
	~CfstreamEmulator();
	void open(const char *FileName, DWORD OpenMode);
	void close();
	bool CheckFileOpen();
	void read(void *Buffer, int Size);
	void write(void *Buffer, int Size);
	void seekp( streamoff off, ios::seek_dir dir = ios::beg);
	streampos tellp();
	bool good();
	bool eof();
	bool bad();
	bool is_open();
	void clear( int nState = 0 );
	void getline( const char* puch, int nCount, unsigned char delim = '\n' );
	void read(const char *Buffer, int Size);
	void write(const char *Buffer, int Size);
};

class CfstreamEmulator_CachedAsync : public CReferenceCount
{
private:
	HANDLE m_FileHandle;
	uint32 m_ErrorFlags;
	int32 m_CurrentPosition;
	int32 m_CurrentFileSize;
	int32 m_CurrentFileSizeInFile;
	int32 m_CurrentCacheLinePriority;
	int32 m_SectorSize;
	CStr m_FileName;

	enum
	{
		ECacheLineSize = 2048, // Has to be power of 2
		ECacheLines = 8
	};

	class CCachLine
	{
		public:
		uint8 m_Data[ECacheLineSize];
		OVERLAPPED m_Overlapped;
		bool m_bDirtyFlag:1;
		bool m_bInUse;
		bool m_bPending;
		bool m_bLastOpWrite;
		int32 m_Priority;

		CCachLine();
		void WaitForComplete(HANDLE hFile);
		bool IsPending(HANDLE hFile);
	};

	CCachLine *m_CacheLines;
	CCachLine *m_LastOpCacheLine;

	CCachLine *GetFreeCacheLine(CCachLine *_DontTouch = NULL);
	CCachLine *GetCacheLine(int _Offset, bool _ForWrite);
	void WriteCacheLine(CCachLine *_Line, bool _Async = true);
	void ReadCacheLine(CCachLine *_Line, bool _Async = true);
	void Flush();
	void GrowFileSize(int NewSize);

public:

	CfstreamEmulator_CachedAsync();
	~CfstreamEmulator_CachedAsync();
	void open(const char *FileName, DWORD OpenMode);
	void close();
	bool CheckFileOpen();
	void read(void *Buffer, int Size);
	void write(void *Buffer, int Size);
	void seekp( streamoff off, ios::seek_dir dir = ios::beg);
	streampos tellp();
	bool good();
	bool eof();
	bool bad();
	bool is_open();
	void clear( int nState = 0 );

	M_INLINE void read(const char *Buffer, int Size)
	{
		read((void *)Buffer, Size);
	}

	M_INLINE void write(const char *Buffer, int Size)
	{
		write((void *)Buffer, Size);
	}
};

#endif // __INC_MFILE_XTL

