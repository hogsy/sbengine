
#include "PCH.h"
#include <time.h>
#include "MImage.h"
#include "MImageIO.h"
#include "../Sound/MSound_Core.h"

void CImage::Compress_Sound(const char * _pFormat, CImage* _pDestImg, float _Priority, float _Quality, uint32 _Flags)
{
#ifdef	SOUND_IO_NOCOMPRESS
	Error("Compress", "Soundcompression has been disabled in this build.");
#else
	TArray<uint8> lFile;
	CCFile File;
	File.Open(lFile, CFILE_WRITE | CFILE_BINARY);

	char lFormat[5];
	lFormat[0] = 0;

	if (_pFormat)
		strcpy(lFormat, _pFormat);

	// Choose format
	if (strlen(lFormat) == 0)
	{
		// Default codec
		strcpy(lFormat, "MPEG");
	}

	// Make upperstring
	if (lFormat)
		CStr::strupr((char *)lFormat);

	if (strlen(lFormat) > 4)
		Error("CImage::Compress_Sound", "Invalid codec");

	CFStr ClassName;

	ClassName.CaptureFormated("CMSound_Codec_%s", lFormat);

	TPtr<CReferenceCount> _spRefCount = MRTC_GetObjectManager()->CreateObject(ClassName);

	if (!_spRefCount)
		Error("CImage::Compress_Sound", "No such codec");

	CSCC_Codec * _pCodec = TDynamicCast<CSCC_Codec >((CReferenceCount *)_spRefCount);

	if (!_pCodec)
		Error("CImage::Compress_Sound", "Error not a codec");

	CWaveform * WaveForm = TDynamicCast<CWaveform >(this);

	if (!WaveForm)
		Error("CImage::Compress_Sound", "Image is not a waveform")

	File.Write(lFormat, 4);

	int StartPos = File.Pos();

	CSCC_CodecFormat Format;

	Format.m_Data.SetSampleSize(WaveForm->WT_GetBitDepth() / 8);
	Format.m_Data.SetSampleRate(WaveForm->WT_GetSampleRate());
	Format.m_Data.SetChannels(WaveForm->WT_GetChnCount());
	Format.m_Data.SetNumSamples(WaveForm->WT_GetSampleCount());
	if (_Flags)
		Format.m_Data.Set_Flags(Format.m_Data.Get_Flags() | ESC_CWaveDataFlags_Stream);
	
//	Format.m_Priority = _Priority;

	// Write original format
	Format.Save(&File);

//	CCFile File2;
//	File2.Open("Test.mp3", CFILE_WRITE | CFILE_BINARY);

	if (!_pCodec->CreateEncoderInternal(&Format, &File))
		Error("CImage::Compress_Sound", "Error creating codec internal");

	if (!_pCodec->CreateEncoder(_Quality))
		Error("CImage::Compress_Sound", "Error creating codec");

	// Add null data

	int Bytes = WaveForm->WT_GetBufferSize();

	if (!_pCodec->AddData(*WaveForm))
		Error("CImage::Compress_Sound", "Error compressing data")

	// Add null data

	_pCodec->Close();

	// Resave format, incase codec has updated it
	File.Seek(StartPos);
	_pCodec->m_Format.Save(&File);

	_pDestImg->Destroy();
	_pDestImg->m_pBitmap = DNew(uint8) uint8[File.Length()];

	if (!_pDestImg->m_pBitmap) 
		MemError("Compress");

	_pDestImg->m_AllocSize = File.Length();
	_pDestImg->m_Width = GetWidth();
	_pDestImg->m_Height = GetHeight();
	_pDestImg->m_Format = GetFormat();
	_pDestImg->m_Memmodel = GetMemModel() | IMAGE_MEM_COMPRESSED | IMAGE_MEM_COMPRESSTYPE_SOUND;
	_pDestImg->m_Memmodel &= ~IMAGE_MEM_LOCKABLE;
	_pDestImg->UpdateFormat();

	memcpy(_pDestImg->m_pBitmap, lFile.GetBasePtr(), _pDestImg->m_AllocSize);

/*	CCFile File2;
	File2.Open(CStrF("S:\\%d.ogg",(int)MRTC_RAND()), CFILE_WRITE | CFILE_BINARY);
	File2.Write(lFile.GetBasePtr() + StartPos, _pDestImg->m_AllocSize-StartPos);
*/

	if (!CanCompress(IMAGE_MEM_COMPRESSTYPE_SOUND))
		Error("Compress", "Image format can't be compressed as sound.");
#endif	// SOUND_IO_NOCOMPRESS
}

void CImage::Decompress_Sound(CImage* _pDestImg)
{
#ifdef	SOUND_IO_NODECOMPRESS
	Error("Compress", "Soundcompression has been disabled in this build.");
#else
	_pDestImg->Destroy();

	CStream_Memory Stream((uint8*)m_pBitmap, m_AllocSize, m_AllocSize);
	
	CCFile File;
	File.Open(&Stream, CFILE_READ | CFILE_BINARY);
	
	char Format[5];

	// Read codec
	File.Read(Format, 4);
	Format[4] = 0;

//	if (strlen(Format) != 4)
//		Error("CImage::Compress_Sound", "Invalid codec");

	CFStr ClassName;

	ClassName.CaptureFormated("CMSound_Codec_%s", Format);

	TPtr<CReferenceCount> _spRefCount = MRTC_GetObjectManager()->CreateObject(ClassName);

	if (!_spRefCount)
		Error("CImage::Compress_Sound", "No such codec");

	CSCC_CodecFormat OriginalFormat;
	
	// Read the original format
	OriginalFormat.Load(&File);
	
	CSCC_Codec * _pCodec = TDynamicCast<CSCC_Codec >((CReferenceCount *)_spRefCount);

	if (!_pCodec)
		Error("CImage::Compress_Sound", "Error not a codec");

	if (!_pCodec->CreateDecoderInternal(&File))
		Error("CImage::Compress_Sound", "Error creating codec internal")

	if (!_pCodec->CreateDecoder(0))
		Error("CImage::Compress_Sound", "Error creating codec")
		
	
	if (_pCodec->m_Format.m_Data.GetSampleSize() == 0)
		_pCodec->m_Format.m_Data.SetSampleSize(OriginalFormat.m_Data.GetSampleSize());
	
	if (_pCodec->m_Format.m_Data.GetNumSamples() == 0)
		_pCodec->m_Format.m_Data.SetNumSamples(OriginalFormat.m_Data.GetNumSamples());

	if (_pCodec->m_Format.m_Data.GetChannels() == 0)
		_pCodec->m_Format.m_Data.SetChannels(OriginalFormat.m_Data.GetChannels());

	if (_pCodec->m_Format.m_Data.GetSampleRate() == 0)
		_pCodec->m_Format.m_Data.SetSampleRate(OriginalFormat.m_Data.GetSampleRate());
//		Error("CImage::Compress_Sound", "No number of channels, error")

	_pCodec->m_Format.UpdateSize();
	// Decompress it all

	TArray<uint8> lFileOut;
	CCFile FileOut;
	FileOut.Open(lFileOut, CFILE_WRITE | CFILE_BINARY);

	TArray<uint8> TempBuffer;
	TempBuffer.SetLen(_pCodec->m_Format.m_TotalSize);
//	char buffer[4096];

	while (1)
	{
		void *pData = TempBuffer.GetBasePtr();		
		mint BytesDownCount = _pCodec->m_Format.m_TotalSize;
		bool bGotSomething;
		if (!_pCodec->GetData(pData, BytesDownCount, false, bGotSomething))
		{
			FileOut.Write(TempBuffer.GetBasePtr(), _pCodec->m_Format.m_TotalSize - BytesDownCount);
			break;
		}
		FileOut.Write(TempBuffer.GetBasePtr(), _pCodec->m_Format.m_TotalSize - BytesDownCount);
	}

	mint AllocSize = OriginalFormat.m_Data.GetNumSamples() * _pCodec->m_Format.m_Data.GetChannels()*(_pCodec->m_Format.m_Data.GetSampleSize());

	M_ASSERT(FileOut.Length() >= AllocSize, "Something wrong with encode ??");
		//- (SilentAddAmount*(_pCodec->m_Format.m_nChannels)*(_pCodec->m_Format.m_SampleSize));

	FileOut.Close();	// MikeW: Moved to after assert as FileOut.Length() is no longer valid after a close call.

	int ImageFormat = 0;
	switch (_pCodec->m_Format.m_Data.GetSampleSize())
	{
	case 1:
		ImageFormat = IMAGE_FORMAT_I8;
		break;
	case 2:
		ImageFormat = IMAGE_FORMAT_I16;
		break;
	case 3:
		ImageFormat = IMAGE_FORMAT_BGR8;
		break;
	case 4:
		ImageFormat = IMAGE_FORMAT_BGRA8;
		break;		
	}

	_pDestImg->Create(_pCodec->m_Format.m_Data.GetChannels(), AllocSize/ (_pCodec->m_Format.m_Data.GetChannels()*(_pCodec->m_Format.m_Data.GetSampleSize())), ImageFormat, (GetMemModel() | IMAGE_MEM_LOCKABLE) & ~IMAGE_MEM_COMPRESS_ALLFLAGS);
//	_pDestImg->m_Modulo = (_pCodec->m_Format.m_nChannels*(_pCodec->m_Format.m_SampleSize));	

	void *Mem = _pDestImg->Lock();

	memcpy(Mem, lFileOut.GetBasePtr(), AllocSize);// + 32*_pCodec->m_Format.m_nChannels*_pCodec->m_Format.m_SampleSize);

	_pDestImg->Unlock();

	/*
	CCFile File2;
	File2.Open(CStrF("S:\\%d_%d.ref.raw",(int)_pCodec->m_Format.m_SampleSize, (int)_pCodec->m_Format.m_nChannels), CFILE_WRITE | CFILE_BINARY);
	File2.Write(_pDestImg->m_pBitmap, _pDestImg->m_AllocSize);
	*/

//	M_ASSERT(_pDestImg->m_Height == OriginalFormat.m_nSamples, "Something wrong with encode ??");

//	_pDestImg->UpdateFormat();
	
#endif	// SOUND_IO_NOCOMPRESS		
}

