#include "PCH.h"

#include "../MSystem.h"
#include "MSound_Core.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CSCC_Codec
|__________________________________________________________________________________________________
\*************************************************************************************************/


MRTC_IMPLEMENT(CSCC_Codec, CReferenceCount);

CSCC_Codec::CSCC_Codec()
{
	m_pFile = NULL;
}

CSCC_Codec::~CSCC_Codec()
{
	
}

// Compressor interface
bool CSCC_Codec::CreateEncoderInternal(CSCC_CodecFormat *_pFormat, CCFile *_pFile)
{
	m_Format = *_pFormat;
	m_pFile = _pFile;
	
	return true;
}

// Decompressor interface
bool CSCC_Codec::CreateDecoderInternal(CCFile *_pFile)
{
	m_pFile = _pFile;
	
	return true;
}

int32 CSCC_Codec::GetDataSize()
{
	return m_Format.m_TotalSize;
}

bool CSCC_Codec::GetDataNonInterleaved(void **_pData, mint &_nBytes, bool _bLooping, bool &_bReadSomething)
{
	M_ASSERT(0, "The codec did not override this function");

	return true;
}

void * CSCC_Codec::GetCodecInfo()
{
//	M_ASSERT(0, "The codec did not override this function");

	return NULL;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CSCC_Codec_RAW
|__________________________________________________________________________________________________
\*************************************************************************************************/


CSCC_Codec_RAW::CSCC_Codec_RAW()
{	
	m_FileStart = 0;
}

CSCC_Codec_RAW::~CSCC_Codec_RAW()
{
	Close();	
}

bool CSCC_Codec_RAW::CreateEncoder(float _Quality)
{
	Error("CSCC_Codec_RAW::CreateEncoder", "This interface is not supported by this codec, you can only use CreateRAW");
	return false;
}

bool CSCC_Codec_RAW::AddData(void *&_pData, int &_nBytes)
{
	Error("CSCC_Codec_RAW::AddData", "This interface is not supported by this codec, you can only use CreateRAW");
	return false;
}

bool CSCC_Codec_RAW::CreateDecoder(int _Flags)
{
	Error("CSCC_Codec_RAW::CreateDecoder", "This interface is not supported by this codec, you can only use CreateRAW");
	return false;
}

bool CSCC_Codec_RAW::CreateRAW(CImage *_pFormat)
{
	m_FileStart = m_pFile->Pos();

	m_Format.m_Data.SetChannels(_pFormat->GetWidth());
	m_Format.m_Data.SetNumSamples(_pFormat->GetHeight());
	m_Format.m_Data.SetSampleRate(22050);
	m_Format.m_Data.SetSampleSize(_pFormat->GetPixelSize());

	return true;
}


bool CSCC_Codec_RAW::GetData(void *&_pData, mint &_nBytes, bool _bLoop, bool &_bReadSomething)
{
	_bReadSomething = true;
	while (_nBytes > 0)
	{
		int Size = m_Format.m_Data.GetNumSamples() * m_Format.m_Data.GetChannels() * m_Format.m_Data.GetSampleSize();
		int Pos = m_pFile->Pos();
		int ToRead = _nBytes;
		if (Pos + ToRead > (Size + m_FileStart))
			ToRead = (Size + m_FileStart) - Pos;

		if (!ToRead)
		{
			if (_bLoop)
			{
				SeekData(0);
				return true;
			}
			else
				return false;
		}
		else
		{
			int ReadBytes = -m_pFile->Pos();
			m_pFile->Read(_pData, ToRead);
			ReadBytes += m_pFile->Pos();

			*((uint8 **)&_pData) += ReadBytes;
			_nBytes -= ReadBytes;
		}
	}	

	return true;
}

bool CSCC_Codec_RAW::SeekData(int _SampleOffset)
{
	int SampleOffset = _SampleOffset % m_Format.m_Data.GetNumSamples();

	m_pFile->Seek(SampleOffset * m_Format.m_Data.GetChannels() * m_Format.m_Data.GetSampleSize());

	return true;
}

void CSCC_Codec_RAW::Close()
{
	
}
