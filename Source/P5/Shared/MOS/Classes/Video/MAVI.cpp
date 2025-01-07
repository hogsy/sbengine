#include "PCH.h"
#ifdef PLATFORM_WIN_PC
#include <windows.h>
#include <vfw.h>
#include "MVideo.h"
#include "../../MOS.h"
#include "../../MSystem/Sound/MSound.h"

#pragma comment(lib,"Vfw32.lib")

class CAviFile : public CVideoFile
{
	fp32 m_FrameRate;
	int m_iFrame;
	int m_FrameSize;
	PAVISTREAM m_CompStream;
	PAVISTREAM m_Stream;
	PAVIFILE m_File;
	CStr m_Filename;
	int m_nFile;
	int m_MaxFrames;

	int m_Width;
	int m_Height;
	int m_bCreated;

	LONG m_nBytesWritten;
	LONG m_nMaxBytesWritten;

public:
	CAviFile()
	{
		Clear();
	}

	void Clear()
	{
		m_CompStream = NULL;
		m_Stream = NULL;
		m_File = NULL;
		m_iFrame = 0;
		m_bCreated = false;
		m_nFile = 0;
		m_MaxFrames = 0;
		m_nBytesWritten = 0;
		m_nMaxBytesWritten = 0;
	}

	virtual ~CAviFile()
	{
		Close();
	}

	virtual bool Create(CStr _FileName, int _Width, int _Height, fp32 _FrameRate, int _KeyFrame = 1)
	{
		return Create2(_FileName, _Width, _Height, _FrameRate, 24*30);
	}

	bool Create2(CStr _FileName, int _Width, int _Height, fp32 _FrameRate, int _MaxFrames)
	{
		ConOutL(CStrF("(CAviFile::Create) %d x %d @ %dhz", _Width, _Height, _FrameRate));
		m_Width = _Width;
		m_Height = _Height;
		m_MaxFrames = _MaxFrames;

		m_FrameSize = _Width * _Height * 3;
		m_FrameRate = _FrameRate;
		if(_FileName.GetFilenameExtenstion().CompareNoCase("avi") != 0)
			_FileName += ".avi";
		m_Filename = _FileName;
		m_nBytesWritten = 0;
		m_nMaxBytesWritten = 0;

		AVIFileInit();

		if(m_nFile > 0)
			_FileName = _FileName.GetPath() + "\\" + _FileName.GetFilenameNoExt() + CStrF("_%.2i", m_nFile) + ".avi";

		HRESULT Res = AVIFileOpen(&m_File, _FileName, OF_WRITE | OF_CREATE, NULL);
		if(Res != AVIERR_OK)
		{
			ConOutL("(CAviFile::Create) AVIFileOpen failed.");
			return false;
		}

		AVISTREAMINFO StrmInfo;
		memset(&StrmInfo, 0, sizeof(StrmInfo));
		StrmInfo.fccType = streamtypeVIDEO;
		StrmInfo.fccHandler = 0;
		StrmInfo.dwScale = 1;
		StrmInfo.dwRate = m_FrameRate;
		StrmInfo.dwSuggestedBufferSize = m_FrameSize;
		StrmInfo.dwQuality = -1;
		StrmInfo.rcFrame.top = 0;
		StrmInfo.rcFrame.left = 0;
		StrmInfo.rcFrame.right = _Width;
		StrmInfo.rcFrame.bottom = _Height;

		Res = AVIFileCreateStream(m_File, &m_Stream, &StrmInfo);
		if(Res != AVIERR_OK)
		{
			ConOutL("(CAviFile::Create) AVIFileCreateStream failed.");
			return false;
		}

		static AVICOMPRESSOPTIONS *g_pOptions = NULL;
		if(!g_pOptions)
		{
			// This will never be deallocated. It is used for remembering settings between sessions
			g_pOptions = DNew(AVICOMPRESSOPTIONS) AVICOMPRESSOPTIONS;
			memset(g_pOptions, 0, sizeof(AVICOMPRESSOPTIONS));
		}

		if(m_nFile == 0)
		{
			if(!AVISaveOptions(NULL, 0, 1, &m_Stream, &g_pOptions))
			{
				ConOutL("(CAviFile::Create) AVISaveOptions failed.");
				return false;
			}
		}

		Res = AVIMakeCompressedStream(&m_CompStream, m_Stream, g_pOptions, NULL);
		if(Res != AVIERR_OK)
		{
			ConOutL("(CAviFile::Create) AVIMakeCompressedStream failed.");
			return false;
		}

		BITMAPINFOHEADER BitmapInfo;
		memset(&BitmapInfo, 0, sizeof(BitmapInfo));
		BitmapInfo.biSize = sizeof(BITMAPINFOHEADER);
		BitmapInfo.biWidth = _Width;
		BitmapInfo.biHeight = _Height;
		BitmapInfo.biPlanes = 1;
		BitmapInfo.biBitCount = 24;
		BitmapInfo.biCompression = BI_RGB;
		BitmapInfo.biSizeImage = m_FrameSize;
		BitmapInfo.biXPelsPerMeter = 0;
		BitmapInfo.biYPelsPerMeter = 0;
		BitmapInfo.biClrUsed = 0;
		BitmapInfo.biClrImportant = 0;
		
		Res = AVIStreamSetFormat(m_CompStream, 0, &BitmapInfo, BitmapInfo.biSize);
		if(Res != AVIERR_OK)
		{
			ConOutL("(CAviFile::Create) AVIStreamSetFormat failed.");
			return false;
		}

		m_bCreated = true;
		return true;
	}

	bool AddFrame(CImage* _pImg)
	{
		if (!m_bCreated)
			Error("AddFrame", "Not initialized.");

		if (_pImg->GetWidth() != m_Width || _pImg->GetHeight() != m_Height)
			Error("AddFrame", "Invalid dimensions.");

		if(m_nBytesWritten + 2*m_nMaxBytesWritten > 2000000000)//m_MaxFrames != 0 && m_iFrame == m_MaxFrames + 1)
		{
			Close();
			int nFile = m_nFile;
			Clear();
			m_nFile = nFile + 1;
			Create(m_Filename, m_Width, m_Height, m_FrameRate);
			m_iFrame = 0;
		}

//		_pImg->Write(CRct(0, 0, 0, 0), CStrF("c:\\test%02i.jpg", m_iFrame));

		// Y-Flip
		{
			uint8* pPixels = (uint8*)_pImg->Lock();
			int Modulo = _pImg->GetModulo();
			uint8 ScanLine[4096*4];
			for(int y = 0; y < m_Height >> 1; y++)
			{
				int yd = m_Height-1-y;
				memcpy(ScanLine, &pPixels[y*Modulo], Modulo);
				memcpy(&pPixels[y*Modulo], &pPixels[yd*Modulo], Modulo);
				memcpy(&pPixels[yd*Modulo], ScanLine, Modulo);
			}
			_pImg->Unlock();
		}

		void* pBuffer = _pImg->Lock();
		if (!pBuffer) Error("AddFrame", "Image could not be locked.");

		LONG nBytes;
		HRESULT Res = AVIStreamWrite(m_CompStream, m_iFrame, 1, pBuffer, m_FrameSize, 0, NULL, &nBytes);
		m_nBytesWritten += nBytes;
		m_nMaxBytesWritten = Max(nBytes,m_nMaxBytesWritten);

		_pImg->Unlock();
		if(Res != AVIERR_OK)
			return false;
		
		m_iFrame++;
		return true;
	}

	bool AddAudio(CWaveform *_pWave)
	{
		void *pData = _pWave->Lock();
		if(!pData)
			return false;

		AVISTREAMINFO StrmInfo;
		memset(&StrmInfo, 0, sizeof(StrmInfo));
		StrmInfo.fccType = streamtypeAUDIO;
		StrmInfo.dwScale = 1;
		StrmInfo.dwRate = m_FrameRate;
		StrmInfo.dwSampleSize = m_FrameSize;
		StrmInfo.dwQuality = -1;

		PAVISTREAM Stream;

		HRESULT hr = AVIFileCreateStream(m_File, &Stream, &StrmInfo);
		if(hr != AVIERR_OK)
			return false;

		WAVEFORMATEX WaveInfo;
		memset(&WaveInfo, 0, sizeof(WaveInfo));
		WaveInfo.wFormatTag = WAVE_FORMAT_PCM;
		WaveInfo.cbSize = 0;
		int Rate = _pWave->WT_GetSampleRate();
		Rate = 24000;
		WaveInfo.nAvgBytesPerSec = Rate * _pWave->GetPixelSize();
		WaveInfo.nBlockAlign = _pWave->GetModulo();
		WaveInfo.nChannels = _pWave->WT_GetChnCount();
		WaveInfo.nSamplesPerSec = Rate;
		WaveInfo.wBitsPerSample = _pWave->WT_GetBitDepth();
		
		hr = AVIStreamSetFormat(Stream, 0, &WaveInfo, sizeof(WAVEFORMATEX));
		if(hr != AVIERR_OK)
			return false;

		hr = AVIStreamWrite(Stream, 0, _pWave->WT_GetSampleCount(), pData, _pWave->WT_GetBufferSize(), 0, NULL, NULL);
		_pWave->Unlock();

		if(hr != AVIERR_OK)
			return false;

		return true;
	}

	void Close()
	{
		ConOutL("(CAviFile::Close)");

		m_bCreated = false;
		if(m_Stream)
		{
			AVIStreamClose(m_Stream);
			m_Stream = NULL;
		}

		if(m_CompStream)
		{
			AVIStreamClose(m_CompStream);
			m_CompStream = NULL;
		}

		if(m_File)
		{
			AVIFileClose(m_File);
			m_File = NULL;
		}

		AVIFileExit();
	}
};

spCVideoFile MCreateVideoFile()
{
	TPtr<CAviFile> spAVI = MNew(CAviFile);
	return (CAviFile*)spAVI;
}
#endif
