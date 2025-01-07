
M_INLINE fint CCFile::Pos() 
{ 
	CFILE_CHECKSTREAM("Pos");
	switch (m_StreamType)
	{
	case ECCFile_Stream_XDF:
		return ((CStream_XDF *)m_pStream)->INL_Pos();
	case ECCFile_Stream_Disk:
		return ((CStream_Disk *)m_pStream)->m_Stream.Pos();
	default:
		return m_pStream->Pos();
	}
};

M_INLINE fint CCFile::Length()
{
	CFILE_CHECKSTREAM("Length");
	switch (m_StreamType)
	{
	case ECCFile_Stream_XDF:
		return ((CStream_XDF *)m_pStream)->INL_Length();
	case ECCFile_Stream_Disk:
		return ((CStream_Disk *)m_pStream)->m_Stream.Len();
	default:
		return m_pStream->Length();
	}
};

M_INLINE void CCFile::Seek(fint pos)
{
	CFILE_CHECKSTREAM("Seek");
	switch (m_StreamType)
	{
	case ECCFile_Stream_XDF:
		((CStream_XDF *)m_pStream)->INL_Seek(pos);
		break;
	case ECCFile_Stream_Disk:
		((CStream_Disk *)m_pStream)->m_Stream.SeekBeg(pos);
		break;
	default:
		m_pStream->Seek(pos);
		break;
	}
};

M_INLINE void CCFile::SeekToEnd()
{
	CFILE_CHECKSTREAM("SeekToEnd");
	switch (m_StreamType)
	{
	case ECCFile_Stream_XDF:
		((CStream_XDF *)m_pStream)->INL_SeekToEnd();
		break;
	case ECCFile_Stream_Disk:
		((CStream_Disk *)m_pStream)->m_Stream.SeekEnd(0);
		break;
	default:
		m_pStream->SeekToEnd();
		break;
	}
};

M_INLINE void CCFile::RelSeek(fint pos)
{
	CFILE_CHECKSTREAM("RelSeek");
	switch (m_StreamType)
	{
	case ECCFile_Stream_XDF:
		((CStream_XDF *)m_pStream)->INL_RelSeek(pos);
		break;
	case ECCFile_Stream_Disk:
		((CStream_Disk *)m_pStream)->m_Stream.SeekCur(pos);
		break;
	default:
		m_pStream->RelSeek(pos);
		break;
	}
};

M_INLINE bool CCFile::EndOfFile()
{
	CFILE_CHECKSTREAM("EndOfFile");
	switch (m_StreamType)
	{
	case ECCFile_Stream_XDF:
		return ((CStream_XDF *)m_pStream)->INL_EndOfFile();
	case ECCFile_Stream_Disk:
		return ((CStream_Disk *)m_pStream)->m_Stream.EndOfFile();
	default:
		return m_pStream->EndOfFile();
	}
};

// --------------------------------
M_INLINE void CCFile::Read(uint8& value)
{ 
	CFILE_CHECKSTREAM("Readuint8");
	switch (m_StreamType)
	{
	case ECCFile_Stream_XDF:
		((CStream_XDF *)m_pStream)->INL_Read(&value, 1);
		break;
	case ECCFile_Stream_Disk:
		((CStream_Disk *)m_pStream)->m_Stream.Read(&value, 1);
		break;
	default:
		m_pStream->Read(&value, 1);
		break;
	}
};

M_INLINE void CCFile::Write(uint8 value)
{
#ifndef PLATFORM_CONSOLE
	if (m_iDisableWrite)
	{					
		m_pStream->RelSeek(1);
		return;
	}											
#endif

	CFILE_CHECKSTREAM("Writeuint8");
	switch (m_StreamType)
	{
	case ECCFile_Stream_XDF:
		((CStream_XDF *)m_pStream)->INL_Write(&value, 1);
		break;
	case ECCFile_Stream_Disk:
		((CStream_Disk *)m_pStream)->m_Stream.Write(&value, 1);
		break;
	default:
		m_pStream->Write(&value, 1);
		break;
	}
};


M_INLINE void CCFile::WriteLE(const uint8* _pData, int _Count)
{
	Write(_pData, _Count);
}

M_INLINE void CCFile::WriteLE(const int8* _pData, int _Count)
{
	Write(_pData, _Count);
}

M_INLINE void CCFile::WriteLE(const uint16* _pData, int _Count)
{
	const int TSize = 2;
	uint16 Buff[256];
	while(_Count)
	{
		int c2 = Min(256, _Count);
		memcpy(Buff, _pData, c2*TSize);
		SwitchArrayLE_uint16(Buff, c2);
		Write(Buff, c2*TSize);
		_pData += c2;
		_Count -= c2;
	}
}

M_INLINE void CCFile::WriteLE(const int16* _pData, int _Count)
{
	const int TSize = 2;
	int16 Buff[256];
	while(_Count)
	{
		int c2 = Min(256, _Count);
		memcpy(Buff, _pData, c2*TSize);
		SwitchArrayLE_int16(Buff, c2);
		Write(Buff, c2*TSize);
		_pData += c2;
		_Count -= c2;
	}
}

M_INLINE void CCFile::WriteLE(const uint32* _pData, int _Count)
{
	const int TSize = 4;
	uint32 Buff[256];
	while(_Count)
	{
		int c2 = Min(256, _Count);
		memcpy(Buff, _pData, c2*TSize);
		SwitchArrayLE_uint32(Buff, c2);
		Write(Buff, c2*TSize);
		_pData += c2;
		_Count -= c2;
	}
}

M_INLINE void CCFile::WriteLE(const int32* _pData, int _Count)
{
	const int TSize = 4;
	int32 Buff[256];
	while(_Count)
	{
		int c2 = Min(256, _Count);
		memcpy(Buff, _pData, c2*TSize);
		SwitchArrayLE_int32(Buff, c2);
		Write(Buff, c2*TSize);
		_pData += c2;
		_Count -= c2;
	}
}

M_INLINE void CCFile::WriteLE(const fp4* _pData, int _Count)
{
	const int TSize = 4;
	fp4 Buff[256];
	while(_Count)
	{
		int c2 = Min(256, _Count);
		memcpy(Buff, _pData, c2*TSize);
		SwitchArrayLE_fp4(Buff, c2);
		Write(Buff, c2*TSize);
		_pData += c2;
		_Count -= c2;
	}
}

M_INLINE void CCFile::WriteLE(const fp8* _pData, int _Count)
{
	const int TSize = 4;
	fp8 Buff[256];
	while(_Count)
	{
		int c2 = Min(256, _Count);
		memcpy(Buff, _pData, c2*TSize);
		SwitchArrayLE_fp8(Buff, c2);
		Write(Buff, c2*TSize);
		_pData += c2;
		_Count -= c2;
	}
}

M_INLINE void CCFile::ReadLE(uint8* _pData, int _Count)
{
	Read(_pData, _Count);
}

M_INLINE void CCFile::ReadLE(int8* _pData, int _Count)
{
	Read(_pData, _Count);
}

M_INLINE void CCFile::ReadLE(uint16* _pData, int _Count)
{
	const int TSize = 2;
	uint16 Buff[256];
	while(_Count)
	{
		int c2 = Min(256, _Count);
		Read(Buff, c2*TSize);
		SwitchArrayLE_uint16(Buff, c2);
		memcpy(_pData, Buff, c2*TSize);
		_pData += c2;
		_Count -= c2;
	}
}

M_INLINE void CCFile::ReadLE(int16* _pData, int _Count)
{
	const int TSize = 2;
	int16 Buff[256];
	while(_Count)
	{
		int c2 = Min(256, _Count);
		Read(Buff, c2*TSize);
		SwitchArrayLE_int16(Buff, c2);
		memcpy(_pData, Buff, c2*TSize);
		_pData += c2;
		_Count -= c2;
	}
}

M_INLINE void CCFile::ReadLE(uint32* _pData, int _Count)
{
	const int TSize = 4;
	uint32 Buff[256];
	while(_Count)
	{
		int c2 = Min(256, _Count);
		Read(Buff, c2*TSize);
		SwitchArrayLE_uint32(Buff, c2);
		memcpy(_pData, Buff, c2*TSize);
		_pData += c2;
		_Count -= c2;
	}
}

M_INLINE void CCFile::ReadLE(int32* _pData, int _Count)
{
	const int TSize = 4;
	int32 Buff[256];
	while(_Count)
	{
		int c2 = Min(256, _Count);
		Read(Buff, c2*TSize);
		SwitchArrayLE_int32(Buff, c2);
		memcpy(_pData, Buff, c2*TSize);
		_pData += c2;
		_Count -= c2;
	}
}

M_INLINE void CCFile::ReadLE(fp4* _pData, int _Count)
{
	const int TSize = 4;
	fp4 Buff[256];
	while(_Count)
	{
		int c2 = Min(256, _Count);
		Read(Buff, c2*TSize);
		SwitchArrayLE_fp4(Buff, c2);
		memcpy(_pData, Buff, c2*TSize);
		_pData += c2;
		_Count -= c2;
	}
}

M_INLINE void CCFile::ReadLE(fp8* _pData, int _Count)
{
	const int TSize = 8;
	fp8 Buff[256];
	while(_Count)
	{
		int c2 = Min(256, _Count);
		Read(Buff, c2*TSize);
		SwitchArrayLE_fp8(Buff, c2);
		memcpy(_pData, Buff, c2*TSize);
		_pData += c2;
		_Count -= c2;
	}
}

M_INLINE void CCFile::Align( int _Alignment )
{
	int Position = Pos();
	int Pad = ( _Alignment - ( Position % _Alignment ) ) % _Alignment;
	const char *aPad = "1234567890123456";

	while( Pad > 0 )
	{
		int Size = (Pad>16)?16:Pad;
		Pad	-= Size;
		Write( aPad, Size );
	}
}


M_INLINE void CCFile::Read(void* dest, mint size)
{
	CFILE_CHECKSTREAM("Read");
	switch (m_StreamType)
	{
	case ECCFile_Stream_XDF:
		((CStream_XDF *)m_pStream)->INL_Read(dest, size);
		break;
	case ECCFile_Stream_Disk:
		((CStream_Disk *)m_pStream)->m_Stream.Read(dest, size);
		break;
	default:
		m_pStream->Read(dest,size);
		break;
	}
};

M_INLINE void CCFile::Write(const void* src, mint size)
{
#ifndef PLATFORM_CONSOLE
	if (m_iDisableWrite)
	{					
		m_pStream->RelSeek(size);
		return;
	}											
#endif
	CFILE_CHECKSTREAM("Write");
	switch (m_StreamType)
	{
	case ECCFile_Stream_XDF:
		((CStream_XDF *)m_pStream)->INL_Write(src,size);
		break;
	case ECCFile_Stream_Disk:
		((CStream_Disk *)m_pStream)->m_Stream.Write(src,size);
		break;
	default:
		m_pStream->Write(src,size);
		break;	
	}
};
