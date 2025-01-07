/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Texture container for PSS (MPEG) compressed movies
					
	Author:			Henrik Meijer
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		A_list_of_classes_functions_etc_defined_in_file
					
	Comments:		Longer_description_not_mandatory
					
	History:		
		030506:		Added comments
\*_____________________________________________________________________________________________*/
#ifndef __INC_MTEXTURECONTAINERMPEG
#define __INC_MTEXTURECONTAINERMPEG

#include "MTextureContainers.h"

#if defined(PLATFORM_PS2)


#ifdef _MTEXTURECONTAINERMPEG_INCLUDESUBSYS


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CMpegASyncReader
|
|	Handles multiple streams
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CMpegASyncReaderStreamRequest
{
public:
	 CMpegASyncReaderStreamRequest( void *_pBuffer, int _FilePos, uint32 _Size );
	~CMpegASyncReaderStreamRequest();
	bool Done();
	
	uint32							 m_Size;
	
	CAsyncRequest					 m_Request;
	CMpegASyncReaderStreamRequest	*m_pPrev;
	CMpegASyncReaderStreamRequest	*m_pNext;
};

class CMpegASyncReaderStream
{
public:
	 CMpegASyncReaderStream( CCFile *_pFile, uint32 _FileSize );
	~CMpegASyncReaderStream();

	void DeleteAllRequests();

	void Start();
	void Stop();
	void Rewind();

	void Process();	
	void GetAvailRingBufferData(  uint8 *&_pRingTop, uint32 &_RingSize, uint8 *&_pStart, uint8 *&_pEnd, uint32 &_Size ); 
	void MoveGetPtr( uint32 _Len );


	void AddRequest( void *_pBuffer, int _FilePos, uint32 _Size );
	bool AnyRequests()	{ return( m_pRequests != NULL ); }

	void Dump();
	
	void SetPreCachedData( uint8 *_pData, uint32 _Size );

	CMpegASyncReaderStream			*m_pPrev;
	CMpegASyncReaderStream			*m_pNext;
protected:
	void RemoveRequest( CMpegASyncReaderStreamRequest *_pRequest );
private:
	MRTC_CriticalSection	 		 m_CriticalSection;
	
	CCFile							*m_pFile;
	uint32							 m_FileSize;
	uint32							 m_FilePos;
	uint32							 m_RequestPos;
	uint32							 m_FileStartPos;
	
	uint8							*m_pRingBuffer;
	int32							 m_GetPtr;
	int32							 m_PutPtr;
	int32							 m_SetPtr;
	int32							 m_DataWritten;
	int32							 m_DataRequested;
	
	bool							 m_bStopped;
	
	CMpegASyncReaderStreamRequest	*m_pRequests;
};

class CMpegASyncReader
{
public:
	 CMpegASyncReader();
	~CMpegASyncReader();
	
	void	InsertStream( CMpegASyncReaderStream *_pStream );
	void	RemoveStream( CMpegASyncReaderStream *_pStream );

	bool	AnyStreamsOpen()	{ return( m_pStreams != NULL ); }

	static void ASyncReaderMain( void *_pEntry );
protected:
	void	CreateReaderThread();
	void	DeleteReaderThread();
private:
	MRTC_CriticalSection	 m_CriticalSection;
	CMpegASyncReaderStream	*m_pStreams;

	int						 m_ThreadId;
	uint8					*m_pThreadStack;
	ThreadParam 			 m_TP;
};


class CMpegFilePreCacheData
{
public:
	 CMpegFilePreCacheData( const CStr &_FileName );
	~CMpegFilePreCacheData();
	
	void	 Read( bool _bPreCache );
	bool 	 NameMatch( const CStr &_MatchName );
	
	uint8	*GetData()		{ return( m_pData ); }
	uint32	 GetSize()		{ return( m_Size  ); }
	uint32	 GetFileSize()	{ return( m_FileSize ); }
	
	CMpegFilePreCacheData *m_pNext;
	CMpegFilePreCacheData *m_pPrev;
private:
	CStr	 m_FileName;
	CCFile	*m_pFile;
	uint8	*m_pData;
	uint32	 m_Size;
	uint32	 m_FileSize;
};


class CMpegFilePreCache
{
public:
	 CMpegFilePreCache();
	~CMpegFilePreCache();
	
	bool Empty()	{ return( m_pPreCached == NULL ); }
	
	void Purge();
	void PreCache( const CStr &_FileName, bool _PreCacheData = false );

	CMpegFilePreCacheData *Find( const CStr &_FindName );
protected:	
	void InsertData( CMpegFilePreCacheData *_pData );
	void RemoveData( CMpegFilePreCacheData *_pData );	
private:
	CMpegFilePreCacheData *m_pPreCached;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CMpegAudioDec
|
|	All functionality needed to play PCM audio from an MpegStream
|	(Possibly add functionality for mixing?)
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CMpegAudioDec
{
public:
	class SpuStreamHeader
	{
	public:
		int8	m_Id[4];		// 'S''S''h''d'
		int32	m_Size;			// 24
		int32	m_Type;			// 0 = 16bit big endian, 1 = 16bit little endian, 2 = SPU2-ADPCM(VAG)
		int32	m_Rate;			// sampling rate (must be 48Khz)
		int32	m_Ch;			// number of channels
		int32	m_InterSize;	// interleave size (must be 512)
		int32	m_LoopStart;	// loop start bloxk address
		int32	m_LoopEnd;		// loop end block address
	};

	class SpuStreamBody
	{
	public:
		int8	m_Id[4];		// 'S''S''h''d'
		int32	m_Size;			// size of audio data
	};

	typedef enum
	{
		AU_STATE_INIT,
		AU_STATE_PRESET,
		AU_STATE_PLAY,
		AU_STATE_PAUSE

	} AU_STATE;

	static const uint32 ms_cHdrSize;


	 CMpegAudioDec();
	~CMpegAudioDec();

	void Create
	(
		uint8	*_pBuff,
		int32	 _BuffSize,
		int32	 _IopBuff,
		int32	 _IopBuffSize,
		uint8	*_pZeroBuff,
		int32	 _IopZeroBuff,
		int32	 _ZeroBuffSize
	);
	void Delete();

	void Pause();
	void Resume();

	void Start();
	void Reset();

	void BeginPut( uint8 **_Ptr0, int32 *_Len0, uint8 **_Ptr1, int32 *_Len1 );
	void EndPut( int32 _Size );

	bool IsPreset();										// has enough data

	int32 SendToIop();

	void SetMasterVolume( uint32 _Volume );
	void SetInputVolume( uint32 _Volume );

	void SetCurrentStream( void *_pCurrentStream )
	{
		m_pCurrentStream = _pCurrentStream;
	}

	void *GetCurrentStream()
	{ 
		return( m_pCurrentStream ); 
	}
	
	uint32 NumBytesInBuffer() { return( m_Count ); }

protected:
	void IopGetArea
	( 
		int32	*_pD0, 
		int32	*_D0, 
		int32	*_pD1, 
		int32	*_D1, 
		int32	 _Pos 
	);

	int32 SendToIop2area
	(
		int32	 _pD0, 
		int32	 _D0, 
		int32	 _pD1, 
		int32	 _D1, 
		uint8	*_pS0, 
		int32	 _S0, 
		uint8	*_pS1, 
		int32	 _S1
	);

	int32 SendToIop
	( 
		int32	 _Dst, 
		uint8	*_Src, 
		int32	 _Size 
	);

private:
	int32					 m_State;

	// header of ADS format
	SpuStreamHeader			 m_Sshd;
	SpuStreamBody			 m_Ssbd;
	int32					 m_HdrCount;

	// audio buffer
	uint8					*m_Data;
	int32					 m_Put;
	int32					 m_Count;
	int32					 m_Size;
	int32					 m_TotalBytes;

	// buffer on IOP
	int32					 m_IopBuff;
	int32					 m_IopBuffSize;
	int32					 m_IopLastPos;
	int32					 m_IopPausePos;
	int32					 m_TotalBytesSent;
	int32					 m_IopZero;
	int32					 m_IopZeroSize;

	void					*m_pCurrentStream;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CMpegStream
|
|	All functionality needed to stream and decode a multiplexed Mpeg stream
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CMpegStream
{
public:
	typedef enum						// decoder state
	{
		ePREBUFFER,						// read from start of file until Audio & Video buffers are full
		ePLAY,							// decode until end of stream
		ePAUSE,							// pause at current frame
		eSEEKEND,						// skip B & P frames until end of stream
		eEND							// decoder has reached end of stream

	} eState;

	CMpegStream( const CStr &_FileName, bool _bPlayAudio = false, uint32 _Width = 0, uint32 _Height = 0, uint32 _nFrames = 0, float _FramesPerSecond = 25.f );
	~CMpegStream();

	void SetVolume( float _Volume );				// set volume (0.f - 1.f)
	void SetAutoRewind( bool _bAutoRewind );		// set automatic rewind
	void SetState( CMpegStream::eState _State );	// use this to change state

	void Demux();
	void SendAudio();
	void DecodeNextFrame( uint8 *_pPixels = NULL, uint32 _nBufSize = 0, fp32 _DeltaTime = 0 );

	static int CBVideo( sceMpeg *mp, sceMpegCbDataStr *cbstr, void *data );
	static int CBAudio( sceMpeg *mp, sceMpegCbDataStr *cbstr, void *data );
	static int CBNodata( sceMpeg *mp, sceMpegCbData *cbstr, void *data );
	static int CBBackground( sceMpeg *mp, sceMpegCbData *cbstr, void *data );
	static int CBError( sceMpeg *mp, sceMpegCbData *cbstr, void *data );

public:
	CStr	m_FileName;				// name of stream file
	bool	m_bWHValid;				// m_Width and m_Height fields are valid
	uint32	m_Width;				// width in pixels
	uint32	m_Height;				// height in pixels
	eState	m_State;				// decoder state
	uint32	m_CurrentFrame;			// current frame that has been decoded ( 0 = no frame decoded )
	uint32	m_nFrames;				// number of frames in stream
	bool	m_bNFramesValid;		// nFrames has been counted and m_nFrames is valid (only true if stream has reached end once without using eSEEKEND )
	float	m_FramesPerSecond;		// so that decoder knows when to drop or hold frames
	bool	m_bAutoRewind;			// when decoder enters eEND state it will reopen the stream and go into ePREBUFFER (effectively looping the movie)
	bool	m_bHasAudio;			// if an audio track has been found
	bool	m_bAlpha;
	float	m_Volume;				// volume

	CCFile					*m_pFile;
	CMpegASyncReaderStream	*m_pStream;
	
	sceMpeg		 m_mp;				// Mpeg decoder
	uint8		*m_pMpegBuff;		// work buffer (used when doing motion compensation)
	uint32		 m_mpegBuffSize;	// work buffer size
	sceIpuRGB32	*m_pRgb32;			// final output buffer
	uint8		*m_pDemuxBuff;		// demultiplexed video buffer
	uint32		 m_DemuxBuffSize;	// demultiplexed video buffer size
	uint8		*m_pAudioBuff;		// demultiplexed audio buffer
	int32        m_AudioBuffSize;	// demultiplexed audio buffer size 
	int32		 m_IopBuff;			// iop audio buffer
	uint32		 m_IopBuffSize;		// iop audio buffer size
	uint8		*m_pZeroBuff;		// buffer containing all 0
	int32		 m_IopZeroBuff;		// iop buffer containing all 0
	bool		 m_bAudioBufferFull;// is audio buffer full
	bool		 m_bPlayAudio;		// if decoder should play audio
	bool		 m_bKeepReading;	// should async reader keep reading
	sceIpuDmaEnv m_DmaEnv;
	bool		 m_bDmaStarted;
	uint8		*m_pDemuxBuffPast;	// ring buffer pointer into demultiplexed video buffer
	uint8		*m_pDemuxBuffPresent;//ring buffer pointer into demultiplexed video buffer
	uint8		*m_pDemuxBuffFuture;// ring buffer pointer into demultiplexed video buffer
	CMTime		 m_Time;	
	uint32		 m_ProcBytes;
	bool		 m_bAnyCbCall;

	static CMpegAudioDec	ms_AudioDec;	
	static int32 			ms_ReferenceCount;
};


#endif // _MTEXTURECONTAINERMPEG_INCLUDESUBSYS

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTC_MpegTexture
|
|	Class used internally by CTextureContainer_Video_Bink
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CTC_MpegTexture : public CReferenceCount
{
public:
	CStr				 m_FileName;
	CStr				 m_TextureName;
	class CMpegStream	*m_pMpegStream;
	int					 m_TextureID;
	uint32				 m_Width;
	uint32				 m_Height;
	uint32				 m_nFrames;
	fp32					 m_Mainvolume;
	CMTime					 m_TimeLastVisible;
	bool				 m_bOnLastFrame;
	bool				 m_bPaused;
	bool				 m_bAutoRestart;
	bool				 m_bHasAudio;
	bool				 m_bAlpha;
	bool				 m_bReferenced;
public:
	 CTC_MpegTexture();
	~CTC_MpegTexture();

	void Create( const CStr &_FileName);

	void ReadInfoFile( const CStr &_FileName );
	void DecodeInfo();
	void SaveInfoFile();

	void SaveState();
	void RestoreState();

	bool IsOpen();
	void Open();
	void Close();
	void MakeValid();
};


typedef TPtr<CTC_MpegTexture> spCTC_MpegTexture;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer_Video_Mpeg
|
|	Texture container for multiple Mpeg files
|__________________________________________________________________________________________________
\*************************************************************************************************/
class SYSTEMDLLEXPORT CTextureContainer_Video_Mpeg : public CTextureContainer_Video
{
	MRTC_DECLARE;


protected:
	TArray<spCTC_MpegTexture>		m_lspVideos;
	fp32								m_CloseTimeOut;

public:
	CTextureContainer_Video_Mpeg();
	~CTextureContainer_Video_Mpeg();

	virtual void	Create(void* _pContext);
	virtual int		AddVideo(CStr _FileName);
	virtual void	CreateFromDirectory(CStr _Path);
	void			ValidateLocalID(int _iLocal);
	
	virtual void	CloseVideo(int _iLocal);
	virtual void	SetVolume(int _iLocal, fp32 fpVol);
	virtual void	Pause(int _iLocal, bool _Paused = true);
	virtual void	AutoRestart(int _iLocal, bool _EnableAutoRestart = true);
	virtual void	Rewind(int _iLocal);
	virtual bool	IsOnLastFrame(int _iLocal);
	virtual bool	MoveToLastFrame(int _iLocal);
	virtual int		GetFrame(int _iLocal);
	virtual int		GetNumFrames(int _iLocal);

	virtual int		GetNumLocal();
	virtual int		GetLocal(const char* _pName);
	virtual int		GetTextureID(int _iLocal);
	virtual int		GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps);
	virtual void	GetTextureProperties(int _iLocal, CTC_TextureProperties& _Properties);
	virtual void	OnRefresh();
	virtual CStr	GetName(int _iLocal);
	virtual void	BuildInto(int _iLocal, CImage** _ppImg, int _nMipmaps, int _TextureVersion, int _ConvertType = IMAGE_CONVERT_RGB, int _iStartMip = 0, uint32 _BuildFlags = 0);

	virtual int		GetWidth(int _iLocal);
	virtual int		GetHeight(int _iLocal);
	
	virtual fp32 	GetTime(int _iLocal);
};



#endif // defined( PLATFORM_PS2 )
#endif // __INC_MTEXTURECONTAINERBINK