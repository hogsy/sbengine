/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			cfstream emulation for the PS2 platform
					
	Author:			Jim Kjellin
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		A_list_of_classes_functions_etc_defined_in_file
					
	Comments:		Longer_description_not_mandatory
					
	History:		
		030506:		Added comments
\*_____________________________________________________________________________________________*/

#if !defined __INC_MFILE_PS2 && defined PLATFORM_PS2
#define __INC_MFILE_PS2

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| PLATFORM_PS2, fstream
|__________________________________________________________________________________________________
\*************************************************************************************************/

// Waaah.. fixedpoint troubles :(
#ifndef CPU_SUPPORT_FP64
#undef	double
#endif

#include <fstream.h>
#include <errno.h>
#include "MRTC.h"

#ifdef CPU_SUPPORT_FP64
#define double double_definition
#endif

#define fstream CfstreamEmulator_PS2

class CfstreamEmulator_PS2 : public CReferenceCount
{
private:
	uint32 m_bIsOpen : 1;
	uint32 m_ErrorFlags;
	uint32 m_CurrentPosition;
	uint32 m_FileLen;

	int	m_FileHandle;

public:
	CfstreamEmulator_PS2();
	~CfstreamEmulator_PS2();
	void open(const char *FileName, uint32 OpenMode);
	void close();
	bool CheckFileOpen();
	void read(void *Buffer, int Size);
	void write(void *Buffer, int Size);
	void seekp(int32 off, ios::seekdir dir = ios::beg);
	streampos tellp();
	bool good();
	bool eof();
	bool bad();
	bool is_open();
	void clear( int nState = 0 );
	void getline( const char* puch, int nCount, unsigned char delim = '\n' );
	void read(const char *Buffer, int Size);
	void write(const char *Buffer, int Size);

	// converts '\' to '/'	
	static char* Slashify(const char *pStr);
};
	
#endif // __INC_MFILE_PS2

