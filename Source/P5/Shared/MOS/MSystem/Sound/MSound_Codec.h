
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030606:		Added Comments
\*_____________________________________________________________________________________________*/

#ifndef __MSound_Codec_h
#define  __MSound_Codec_h

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CODEC interface
|__________________________________________________________________________________________________
\*************************************************************************************************/


class CSCC_CodecFormat
{	
public:
	CSCC_CodecFormat()
	{
		m_Data.SetSampleRate(0);
		m_Data.SetSampleSize(0);
		m_Data.SetChannels(0);
		m_Data.Set_Flags(0);
	}
	
	CWaveData m_Data;

	// Saveable files
//	int32 m_SampleSize;
//	int32 m_nChannels;
//	int32 m_nSamples;
//	float m_SampleRate;
//	float m_Priority;

	void Save(CCFile *_pFile)
	{
		m_Data.Write(_pFile);
	}

	void Load(CCFile *_pFile)
	{
		m_Data.Read(_pFile);
		UpdateSize();
	}

	void UpdateSize()
	{
		m_TotalSize = m_Data.GetSampleSize() * m_Data.GetChannels() * m_Data.GetNumSamples();
	}

	mint m_TotalSize;
};


enum ESCC_CodecFlags
{
	ESCC_CodecFlags_NoAsyncRead = 1
};

class CSCC_Codec : public CReferenceCount
{	
	MRTC_DECLARE;

public:

	CCFile *m_pFile; // File for writing compressed and reading uncompressed data
	CSCC_CodecFormat m_Format;

	CSCC_Codec();
	~CSCC_Codec();

	// Compressor interface
	bool CreateEncoderInternal(CSCC_CodecFormat *_pFormat, CCFile *_pFile);
	virtual bool CreateEncoder(float _Quality) pure;
	virtual bool AddData(void *&_pData, int &_nBytes) pure;

	virtual bool AddData(CWaveform &_WaveForm)
	{
		void *pLock = _WaveForm.Lock();
		int Size = _WaveForm.GetSize();
		while (Size)
		{
			if (!AddData(pLock, Size))
				return false;
		}
		return true;
	}

	// Decompressor interface
	bool CreateDecoderInternal(CCFile *_pFile);
	virtual bool CreateDecoder(int _Flags) pure; // This function must fill the codec format structure
	virtual bool GetData(void *&_pData, mint &_nBytes, bool _bLooping, bool &_bReadSomething) pure; // Return false to signify that stream is over
	virtual bool GetDataNonInterleaved(void **_pData, mint &_nBytes, bool _bLooping, bool &_bReadSomething);
	virtual void * GetCodecInfo();
	virtual bool SeekData(int _SampleOffset) pure;
	virtual int32 GetDataSize();

	virtual mint GetData(fp32 *_pDest, mint _nMaxSamples, bint _bLooping) {return 0;} // returns the number of samples decoded
	
	// Flushes all data and invalidates the stream
	virtual void Close() pure; 

	virtual bool WantDelete() { return true; }
};

typedef TPtr<CSCC_Codec> spCSCC_Codec;


class CSCC_Codec_RAW : public CSCC_Codec
{	
	int m_FileStart;

public:

	CSCC_Codec_RAW();
	~CSCC_Codec_RAW();

	// Compressor interface
	virtual bool CreateEncoder(float _Quality);
	virtual bool AddData(void *&_pData, int &_nBytes);

	// Decompressor interface
	virtual bool CreateRAW(CImage *_pFormat);
	virtual bool CreateDecoder(int _Flags); // This function must fill the codec format structure
	virtual bool GetData(void *&_pData, mint &_nBytes, bool _bLooping, bool &_bReadSomething);
	virtual bool SeekData(int _SampleOffset);
	
	// Flushes all data and invalidates the stream
	virtual void Close(); 
};




/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				A placeholder for a file stream and the codec
\*____________________________________________________________________*/

class CSCC_CodecStream : public CReferenceCount
{
public:
	CSCC_CodecStream()
	{

	}
	~CSCC_CodecStream()
	{
		// Make sure that the codec deletes before the m_StreamFile goes out of scope
		m_spCodec = NULL;

	}
	CCFile m_StreamFile;
	spCSCC_Codec m_spCodec; 
};

M_TPTR(CSCC_CodecStream);

#endif //__MSound_Codec_h
