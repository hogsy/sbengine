#include "PCH.h"

#include "../MSystem.h"
#include "MSound.h"

// -------------------------------------------------------------------
//  CWaveform
// -------------------------------------------------------------------
void CWaveform::WT_Create(int _nSamples, int _nChannels, int _Format)
{
	MAUTOSTRIP(CWaveForm_WT_Create, MAUTOSTRIP_VOID);

	if ((_Format != WAVE_FORMAT_8BIT) &&
		(_Format != WAVE_FORMAT_16BIT)) 
		Error("WT_Create", "Invalid wave format.");

	Create(_nChannels, _nSamples, _Format, IMAGE_MEM_IMAGE | IMAGE_MEM_ALIGN32);

	if (m_Modulo != (_nChannels*m_Pixelsize)) Error("WT_Create", "Invalid modulo.");
}

#define FOURCC(a, b, c, d) ((uint32(a) << 0) + (uint32(b) << 8) + (uint32(c) << 16) + (uint32(d) << 24))


bool CWaveform::WT_CaptureWAV(void* _pData, int _Size)
{
	MAUTOSTRIP( CWaveForm_WT_CaptureWAV, false );

	struct WaveFormatEx
	{
		uint16 wFormatTag;
		uint16 nChannels;
		uint32 nSamplesPerSec;
		uint32 nAvgBytesPerSec;
		uint16 nBlockAlign;
		uint16 wBitsPerSample;
		uint16 cbSize;
	};

	uint32* pD = (uint32*)_pData;

//	uint32 apan1 = FOURCC('R', 'I', 'F', 'F');
//	uint32 apan2 = FOURCC('F', 'F', 'I', 'R');
	if (*pD != FOURCC('R', 'I', 'F', 'F')) return false;
	pD++;
	uint32 Len = *pD++;
	uint32* pEnd = (uint32*)((uint8*)pD + Len);
	uint32 Type = *pD++;
	if (Type != FOURCC('W', 'A', 'V', 'E')) return false;

	uint8* pWave = NULL;
	uint32 WaveLen = 0;

	WaveFormatEx Fmt;
	FillChar(&Fmt, sizeof(Fmt), 0);

	while(pD < pEnd)
	{
		Type = *pD++;
		Len = *pD++;
		switch(Type)
		{
		case FOURCC('f', 'm', 't', ' ') :
			if (Len < sizeof(WaveFormatEx))
			{
                if (Len < 14) return false;
				Fmt = *(WaveFormatEx*) pD;
				if (pWave) pD = pEnd;
			}
			break;
		case FOURCC('d', 'a', 't', 'a') :
			{
				WaveLen = Len;
				pWave = (uint8*) pD;
				if (Fmt.wFormatTag == 1) pD = pEnd;
			}
			break;
		}
		if (pD == pEnd) break;
        pD = (uint32*)((uint8*)pD + ((Len + 1) & ~1));
	}

	if (!(pWave && (Fmt.wFormatTag == 1))) return false;
	if (!Fmt.nChannels) return false;
	int Format = 0;
	if (Fmt.wBitsPerSample == 8) Format = WAVE_FORMAT_8BIT;
	if (Fmt.wBitsPerSample == 16) Format = WAVE_FORMAT_16BIT;
	if (!Format) return false;
	if (Fmt.nBlockAlign != (Fmt.wBitsPerSample / 8)*Fmt.nChannels) return false;
	
	m_Rate = Fmt.nSamplesPerSec;

	int nSmp = WaveLen / Fmt.nBlockAlign;
	Create(Fmt.nChannels, nSmp, Format, IMAGE_MEM_IMAGE | IMAGE_MEM_ALIGN32);

	uint8* pDst = (uint8*) Lock();
	{
		if ((m_Modulo == Fmt.nBlockAlign) && (m_Height >= nSmp))
			memcpy(pDst, pWave, nSmp*Fmt.nBlockAlign);
		else
			for(int i = 0; i < nSmp; i++)
			{
				memcpy(&pDst[m_Modulo*i], pWave, Fmt.nBlockAlign);
				pWave += Fmt.nBlockAlign;
			}
	}
	Unlock();

	// Copy.
/*	for(int i = 0; i < nSmp; i++)
	{
		SetRAWData(CPnt(0, i), Fmt.nBlockAlign, pWave);
		pWave += Fmt.nBlockAlign;
	}*/

	return true;
}	

void CWaveform::WT_ReadWAV(CStr _Filename)
{
	MAUTOSTRIP( CWaveForm_WT_ReadWAV, MAUTOSTRIP_VOID );

	CCFile File;
	File.Open(_Filename, CFILE_READ | CFILE_BINARY);
	int Len = File.Length();
	if (Len <= 0) Error("WT_ReadWAV", "Corrupt file. (" + _Filename + ")");

	uint8* pD = DNew(uint8) uint8[Len];
	if (!pD) MemError("WT_ReadWAV");

	M_TRY
	{
		File.Read(pD, Len);
		if (!WT_CaptureWAV(pD, Len))
			Error("WT_ReadWAV", "Not a valid WAV-file or unsupported WAV-data. (" + _Filename + ")");
	}
	M_CATCH(
	catch(CCException)
	{
		delete[] pD; pD = NULL;
		throw;
	}
	)
	delete[] pD; pD = NULL;
}

void CWaveform::WT_WriteWAV()
{
	MAUTOSTRIP( CWaveForm_WT_WriteWAV, MAUTOSTRIP_VOID );

	Error("WT_WriteWAV", "Not implemented.");
}

void CWaveform::WT_ReadRAW(CCFile* _pFile, int _Size, int _Format, int _nChn, bool _bUnsigned)
{
	MAUTOSTRIP( CWaveForm_WT_ReadRAW__1, MAUTOSTRIP_VOID );

	if (_Size < 0) _Size = (_pFile->Length() - _pFile->Pos());

	int PixelSize = CImage::Format2PixelSize(_Format);
	int nSamples = _Size / (PixelSize*_nChn);
	WT_Create(nSamples, _nChn, _Format);

	void* pData = Lock();
	M_TRY
	{
		_pFile->Read(pData, nSamples*m_Modulo);
		if (_bUnsigned) WT_SignFlip();
	}
	M_CATCH(
	catch(CCException)
	{
		Unlock();
		throw;
	}
	)
	Unlock();
}

void CWaveform::WT_ReadRAW(CStr _Filename, int _Format, int _nChn, bool _bUnsigned)
{
	MAUTOSTRIP( CWaveForm_WT_ReadRAW__2, MAUTOSTRIP_VOID );

	CCFile File;
	File.Open(_Filename, CFILE_READ | CFILE_BINARY);
	WT_ReadRAW(&File, File.Length(), _Format, _nChn, _bUnsigned);
	File.Close();
}

void CWaveform::WT_WriteRAW(CCFile* _pFile)
{
	MAUTOSTRIP( CWaveForm_WT_WriteRAW__1, MAUTOSTRIP_VOID );

	Error("WT_WriteWAV", "Not implemented.");
}

void CWaveform::WT_WriteRAW(CStr _Filename)
{
	MAUTOSTRIP( CWaveForm_WT_WriteRaw__2, MAUTOSTRIP_VOID );

	Error("WT_WriteWAV", "Not implemented.");
}

void CWaveform::WT_Read(CStr _Filename, int _Flags)
{
	MAUTOSTRIP( CWaveForm_WT_Read, MAUTOSTRIP_VOID );

	if (_Flags & 1 && WT_GetChnCount() > 0)
	{
		// We are adding the file to the waveform

		CWaveform Temp;

		CStr Ext = _Filename.GetFilenameExtenstion();
		if (Ext.CompareNoCase("WAV") == 0)
			Temp.WT_ReadWAV(_Filename);
		else if ((Ext.CompareNoCase("RAW") == 0) ||
			(Ext.CompareNoCase("SMP") == 0) ||
			(Ext.CompareNoCase("SND") == 0))
			Temp.WT_ReadRAW(_Filename);
		else
			Error("WT_Read", "Unsupported format (" + _Filename + ")");

		CImage Old;
		Duplicate(&Old);

		if (Old.GetFormat() != Temp.GetFormat())
			Error("WT_Read", "Cannot merge files, different formats");

		if (m_Rate != Temp.m_Rate)
			Error("WT_Read", "Cannot merge files, different sample rates");

		int nOldChannels = Old.GetWidth();
		int nNewChannels = Temp.GetWidth();
		int nChannels = nOldChannels + nNewChannels;
		int nSamples = Max(Old.GetHeight(), Temp.GetHeight());

		Create(nChannels, nSamples, GetFormat(), IMAGE_MEM_IMAGE | IMAGE_MEM_ALIGN32);

		uint8* pDst = (uint8*) Lock();
		uint8* pOld = (uint8*) Old.Lock();
		uint8* pNew = (uint8*) Temp.Lock();
		int OldSize = nOldChannels * Old.GetPixelSize();
		int NewSize = nNewChannels * Old.GetPixelSize();
//		int BlockSize = Old.GetPixelSize() * nChannels;
		int OldModulo = Old.GetModulo();
		int NewModulo = Temp.GetModulo();
		{
			for(int i = 0; i < nSamples; i++)
			{
				memcpy(pDst + m_Modulo*i, pOld + OldModulo*i, OldSize);
				memcpy(pDst + m_Modulo*i + OldSize, pNew + NewModulo*i, NewSize);
			}
		}
		Unlock();
		
	}
	else
	{
		CStr Ext = _Filename.GetFilenameExtenstion();
		if (Ext.CompareNoCase("WAV") == 0)
			WT_ReadWAV(_Filename);
		else if ((Ext.CompareNoCase("RAW") == 0) ||
			(Ext.CompareNoCase("SMP") == 0) ||
			(Ext.CompareNoCase("SND") == 0))
			WT_ReadRAW(_Filename);
		else
			Error("WT_Read", "Unsupported format (" + _Filename + ")");
	}
}

void CWaveform::WT_Add(int _iStart, int _Count, int _Value, bool _bWrap)
{
	MAUTOSTRIP( CWaveForm_WT_Add__1, MAUTOSTRIP_VOID );

	void* pData = Lock();
	M_TRY
	{
		int Fmt = GetFormat();
		switch(Fmt)
		{
		case WAVE_FORMAT_8BIT :
			{
				int8* pD = (int8*) pData;
				for(int s = 0; s < m_Height; s++)
					for(int c = 0; c < m_Width; c++)
						pD[c*m_Pixelsize + s*m_Modulo] += _Value;
			}
			break;
		case WAVE_FORMAT_16BIT :
			{
				int16* pD = (int16*) pData;
				for(int s = 0; s < m_Height; s++)
					for(int c = 0; c < m_Width; c++)
						pD[(c*m_Pixelsize + s*m_Modulo) >> 1] += _Value;
			}
			break;
		default :
			Error("WT_Add", "Format not supported.");
		}
	}
	M_CATCH(
	catch(CCException)
	{
		Unlock();
	}
	)
	Unlock();
}

void CWaveform::WT_Add(int _iStart, int _Count, fp32 _Value, bool _bWrap)
{
	MAUTOSTRIP( CWaveForm_WT_Add__2, MAUTOSTRIP_VOID );

	WT_Add(_iStart, _Count, int(_Value*fp32(1 << WT_GetBitDepth())), _bWrap);
}

void CWaveform::WT_SignFlip()
{
	MAUTOSTRIP( CWaveForm_WT_SignFlip, MAUTOSTRIP_VOID);

	WT_Add(0, WT_GetSampleCount(), 0.5f, true);
}

// -------------------------------------------------------------------

IMPLEMENT_OPERATOR_NEW(CWC_Waveform);


CWC_Waveform::CWC_Waveform()
{
	MAUTOSTRIP( CWC_WaveForm_ctor, MAUTOSTRIP_VOID );

	m_WaveID = -1;
	m_FilePos = 0;
	m_SubLength = 0;
}

void CWC_Waveform::Read(CDataFile* _pDFile, bool _bDynamic)
{
	MAUTOSTRIP( CWC_WaveForm_Read, MAUTOSTRIP_VOID );

/* Reads:
		[NAME]
		[WAVEDATA]
*/
	CStr NodeName;
	while((NodeName = _pDFile->GetNext()) != "")
	{
		CCFile* pFile = _pDFile->GetFile();
		if (NodeName == "NAME")
		{
			CStr Name;
			Name.Read(pFile);
#ifdef USE_HASHED_WAVENAME
			m_NameID = StringToHash(Name);
#else
			m_Name = Name;
#endif
		}
#ifdef USE_HASHED_WAVENAME
		else if (NodeName == "NAMEID")
		{
			pFile->ReadLE(m_NameID);
		}
#endif
		else if (NodeName == "WAVEDATA")
		{
			if (_bDynamic)
			{
				m_FilePos = pFile->Pos();
				m_SubLength = _pDFile->GetEntrySize();
			}
			else
			{
#ifdef PLATFORM_CONSOLE
				Error_static("CWC_Waveform::Read", "shouldn't be used!");
#else
				m_spWave = MNew(CWaveform);
				if (!m_spWave) MemError("Read");
				m_spWave->Read(pFile, IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM, NULL);
#endif
			}
		}
	}
}

void CWC_Waveform::Write(CDataFile* _pDFile)
{
	MAUTOSTRIP( CWC_WaveForm_Write, MAUTOSTRIP_VOID );
#ifdef PLATFORM_CONSOLE
	Error_static("CWC_Waveform::Write", "Not supported!");
#else
	CCFile* pFile = _pDFile->GetFile();
	if (!m_spWave) Error("Write", "No waveform.");

#ifdef USE_HASHED_WAVENAME
	// Write NAME
	_pDFile->BeginEntry("NAMEID");
		pFile->WriteLE(m_NameID);
	_pDFile->EndEntry(sizeof(m_NameID));
#else
	// Write NAME
	if (m_Name != "")
	{
		_pDFile->BeginEntry("NAME");
			m_Name.Write(pFile);
		_pDFile->EndEntry(m_Name.Len()+1);
	}
#endif

	// Write WAVEDATA
	_pDFile->BeginEntry("WAVEDATA");
	m_spWave->Write(pFile);
	_pDFile->EndEntry(0);
#endif
}

void CWC_Waveform::WriteData(CCFile* _pFile)
{
	MAUTOSTRIP( CWC_WaveForm_WriteData, MAUTOSTRIP_VOID );
#ifndef M_Profile
	Error_static("CWC_Waveform::WriteData", "Not supported!");
#else
	CStr ToWrite = m_Name;
	m_FilePos = _pFile->Pos();
	m_spWave->Write(_pFile);
	m_SubLength = _pFile->Pos() - m_FilePos;
#endif
}

void CWC_Waveform::WriteInfo(CCFile* _pFile)
{
	MAUTOSTRIP( CWC_WaveForm_WriteInfo, MAUTOSTRIP_VOID );
#ifdef USE_HASHED_WAVENAME
	Error_static("CWC_Waveform::Write", "Not supported!");
#else
	_pFile->WriteLE(m_FilePos);
	_pFile->WriteLE(m_SubLength);

	CStr ToWrite = m_Name;
	ToWrite.Write(_pFile);
#endif
}

void CWC_Waveform::ReadInfo(CCFile* _pFile, bool _bDynamic)
{
	MAUTOSTRIP( CWC_WaveForm_ReadInfo, MAUTOSTRIP_VOID );

	_pFile->ReadLE(m_FilePos);
	_pFile->ReadLE(m_SubLength);

	CFStr ToRead;
	ToRead.Read(_pFile);
#ifdef USE_HASHED_WAVENAME
	m_NameID = StringToHash(ToRead);
#else	
	m_Name = ToRead;
#endif

	if (!_bDynamic)
	{
		int OldPos = _pFile->Pos();
		ReadData(_pFile);
		_pFile->Seek(OldPos);
	}
}

void CWC_Waveform::ReadData(CCFile* _pFile)
{
	MAUTOSTRIP( CWC_WaveForm_ReadData, MAUTOSTRIP_VOID );
#ifdef PLATFORM_CONSOLE
	Error_static("CWC_Waveform::ReadData", "Not supported!");
#else
	_pFile->Seek(m_FilePos);
	m_spWave = MNew(CWaveform);
	if (!m_spWave) 
		MemError("ReadData");
	m_spWave->Read(_pFile, IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM, NULL);
#endif
}

void CWC_Waveform::ReadDynamic(const CStr& _Filename)
{
	MAUTOSTRIP( CWC_WaveForm_ReadDynamic, MAUTOSTRIP_VOID );
#ifdef PLATFORM_CONSOLE
	Error_static("CWC_Waveform::ReadDynamic", "Not supported!");
#else
	if (m_spWave!=NULL) return;
	if (!m_FilePos) Error("ReadDynamic", "No filepos prepared.");

	CCFile File;
	File.Open(_Filename, CFILE_READ | CFILE_BINARY);
	File.Seek(m_FilePos);
	m_spWave = MNew(CWaveform);
	if (m_spWave==NULL) MemError("Read");
	m_spWave->Read(&File, IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM | IMAGE_MEM_ALIGN32, NULL);

	File.Close();
#endif
}
