#include "PCH.h"

#include "../Platform/Platform.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| PLATFORM_PS2, fstream
|__________________________________________________________________________________________________
\*************************************************************************************************/

#ifdef PLATFORM_PS2

#include <sifdev.h>
#include "MFile_PS2.h"

CfstreamEmulator_PS2::CfstreamEmulator_PS2()
{
	m_ErrorFlags = 0;
	m_CurrentPosition = 0;
	m_bIsOpen = 0;
	m_FileHandle = -1;
}

CfstreamEmulator_PS2::~CfstreamEmulator_PS2()
{
	close();
}

void CfstreamEmulator_PS2::open(const char *FileName, uint32 OpenMode)
{
//	Error_static( "CfstreamEmulator_PS2::open", "Implement this" );
	m_FileHandle = sceOpen( CStrF( "host0:%s", FileName ).GetStr(), SCE_RDONLY, 0777 );
	if( m_FileHandle < 0 )
		m_ErrorFlags |= ios::badbit;
	else
		m_ErrorFlags = 0;
}

void CfstreamEmulator_PS2::close()
{
	if( m_FileHandle != -1 )
	{
		if( sceClose( m_FileHandle ) )
		{
			m_ErrorFlags |= ios::badbit;
		}

		m_FileHandle = -1;
	}
}

bool CfstreamEmulator_PS2::CheckFileOpen()
{
	if( m_FileHandle == -1 )
	{
		m_ErrorFlags	|= ios::failbit;
		return false;
	}
	else
		return true;
}

void CfstreamEmulator_PS2::read(void *_pBuffer, int _Size)
{
	if( !CheckFileOpen() )
		return;

	int ReadBytes = 0;
	
	if( ( ReadBytes = sceRead( m_FileHandle, _pBuffer, _Size ) ) < 0 )
	{
		m_ErrorFlags |= ios::badbit;
	}
	else if( ReadBytes == 0 )
	{
		m_ErrorFlags |= ios::eofbit;
	}

	m_CurrentPosition += ReadBytes;
}


void CfstreamEmulator_PS2::write(void *Buffer, int Size)
{
	Error("write", "LMAO! Ya gonna write to the DVD? bwhahaha!");
}

void CfstreamEmulator_PS2::seekp(int32 off, ios::seekdir dir)
{
	if (!CheckFileOpen())
		return;

	int Direction = 0;

	if (dir == ios::beg)
	{
		Direction = SEEK_SET;
	}
	else if (dir == ios::cur)
	{
		Direction = SEEK_CUR;
	}
	else if (dir == ios::end)
		Direction = SEEK_END;

	int Pos = sceLseek( m_FileHandle, off, Direction );

	if (Pos == 0xFFFFFFFF)
	{
		m_ErrorFlags |= ios::badbit;
	}
	else
	{
		m_CurrentPosition = Pos;
	}
}

streampos CfstreamEmulator_PS2::tellp()
{
	if( !CheckFileOpen() )
		return 0;

	return sceLseek( m_FileHandle, 0, SEEK_CUR );
}

bool CfstreamEmulator_PS2::good()
{
	return (m_ErrorFlags & (ios::failbit | ios::badbit)) == 0;
}

bool CfstreamEmulator_PS2::eof()
{
	return (m_ErrorFlags & (ios::eofbit)) != 0;
}

bool CfstreamEmulator_PS2::bad()
{
	return (m_ErrorFlags & (ios::badbit)) != 0;
}

bool CfstreamEmulator_PS2::is_open()
{
	return m_FileHandle > 0;
}

void CfstreamEmulator_PS2::clear( int nState )
{
	if (!nState)
		m_ErrorFlags = 0;
	else
	{
		m_ErrorFlags &= ~nState;
	}
}

void CfstreamEmulator_PS2::getline( const char* puch, int nCount, unsigned char delim)
{
	if (!CheckFileOpen())
		return;

	M_ASSERT(0, "Not implemented.");
}

void CfstreamEmulator_PS2::read(const char *Buffer, int Size)
{
	read((void *)Buffer, Size);
}

void CfstreamEmulator_PS2::write(const char *Buffer, int Size)
{
	write((void *)Buffer, Size);
}

#endif

