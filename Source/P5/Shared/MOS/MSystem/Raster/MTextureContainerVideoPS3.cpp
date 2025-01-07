
#include "PCH.h"

#include "../MSystem.h"

#if defined(PLATFORM_PS3)

#include "MTextureContainerVideoPS3.h"

#include <sys/timer.h>
#include <sys/process.h>
#include <sys/sys_time.h>
#include <sys/ppu_thread.h>
#include <sys/spu_initialize.h>
#include <sys/synchronization.h>

#include <cell/codec.h>

class CDecoder : public CReferenceCount
{
	friend class CTextureContainer_Video_PS3::CTC_Texture;
	friend class CInputThread;
protected:
	CellVdecResource m_Resource;
	CellVdecHandle m_Handle;
	uint32 m_bCorrupt:1;
	uint32 m_bFrameAvail:1;
	uint32 m_bEOS:1;

	sys_mutex_t	m_AUMutex;
	sys_cond_t	m_AUCond;
	uint64	m_AUDone;

	sys_mutex_t	m_PicMutex;
	sys_cond_t	m_PicCond;
	uint64	m_PicDone;

	sys_mutex_t m_FBMutex;
	sys_cond_t m_FBCond;

	TThinArrayAlign<uint8, 128> m_lFrameBuffer;
	uint16 m_Width;
	uint16 m_Height;

	spCCFile m_spFile;

	bool Internal_Open(CellVdecCodecType _Type, uint _Profile);
	void Internal_Close();

public:
	static uint32 VideoCallback(CellVdecHandle _Handle, CellVdecMsgType _MsgType, int32 _MsgData, void* _pArg);

	int GetFrame() {return m_PicDone;}
	bool NewFrameAvailable() const
	{
		return m_bFrameAvail;
	}

	CDecoder();
	virtual ~CDecoder();
	virtual bool Open() pure;
	virtual void Close() pure;
};

typedef TPtr<CDecoder> spCDecoder;

class CDecoder_M2V : public CDecoder
{
protected:
	enum
	{
		GROUP_START_CODE		= 0xB8,
		SEQUENCE_END_CODE		= 0xB7,
		PICTURE_START_CODE		= 0x00,
		SEQUENCE_HEADER_CODE	= 0xB3,
	};
	class CInputThread : public MRTC_Thread
	{
	protected:
		virtual const char* Thread_GetName() const
		{
			return "CDecoder::CInputThread";
		}

		uint32 get_access_unit(uint8* _pBuf, uint32 _BuffLength, uint8** _ppAUBuf, uint32* _pAULength);
	public:
		virtual int Thread_Main();
	};

	class CPostThread : public MRTC_Thread
	{
	protected:
		virtual const char* Thread_GetName() const
		{
			return "CDecoder::CPostThread";
		}
	public:
		virtual int Thread_Main();
	};

	CInputThread m_InputThread;
	CPostThread m_PostThread;

public:
	CDecoder_M2V();
	virtual ~CDecoder_M2V();

	virtual bool Open();
	virtual void Close();
};

class CDecoder_AVC : public CDecoder
{
public:
	CDecoder_AVC();
	virtual ~CDecoder_AVC();

	virtual bool Open();
	virtual void Close();
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTC_Texture
|__________________________________________________________________________________________________
\*************************************************************************************************/

CTextureContainer_Video_PS3::CTC_Texture::CTC_Texture()
{
	m_TextureID = 0;
	m_Width = 0;
	m_Height = 0;
}

CTextureContainer_Video_PS3::CTC_Texture::~CTC_Texture()
{
	Close();
}

void CTextureContainer_Video_PS3::CTC_Texture::Create(CStr _Filename)
{
	if (!CDiskUtil::FileExists(_Filename))
		Error("Create", "Not a valid file: " + _Filename);
	m_Filename = _Filename;
	m_TextureName = CStrF("*VIDEO_%s", _Filename.GetFilenameNoExt().UpperCase().Str());
}

void CTextureContainer_Video_PS3::CTC_Texture::Open()
{
	spCCFile spFile = CDiskUtil::CreateCCFile(m_Filename, CFILE_READ | CFILE_BINARY);
	if(!spFile)
		return;
	switch(m_Type)
	{
	case VIDEO_TYPE_M2V:
		{
			m_spDecoder = MNew(CDecoder_M2V);
			break;
		}

	case VIDEO_TYPE_AVC:
		{
			m_spDecoder = MNew(CDecoder_AVC);
			break;
		}
	default:
		return;
	}

	m_spDecoder->m_spFile = spFile;

	if(!m_spDecoder->Open())
	{
		m_spDecoder = NULL;
	}
}

void CTextureContainer_Video_PS3::CTC_Texture::Close()
{
	if(m_spDecoder)
	{
		m_spDecoder->Close();
		m_spDecoder->m_spFile = NULL;
		m_spDecoder = NULL;
	}
}

bool CTextureContainer_Video_PS3::CTC_Texture::IsOpen()
{
	return m_spDecoder != NULL;
}

void CTextureContainer_Video_PS3::CTC_Texture::BuildInto(CImage** _ppImg, uint32 _BuildFlags)
{
	sys_mutex_lock(m_spDecoder->m_FBMutex, 0);
	if(!m_spDecoder->m_bFrameAvail)
	{
		sys_mutex_unlock(m_spDecoder->m_FBMutex);

		// No frame avail
		if (_BuildFlags & ETCBuildFlags_NewTexture)
		{
			uint8 *pDest = (uint8*)(*_ppImg)->Lock();
			memset( pDest, 0, (*_ppImg)->GetModulo() * (*_ppImg)->GetHeight());
			(*_ppImg)->Unlock();
		}

		return;
	}

//	MRTC_SystemInfo::OS_Trace("Copying video texture\n");

	uint8 *pFB = m_spDecoder->m_lFrameBuffer.GetBasePtr();
	uint32 SrcWidth = m_spDecoder->m_Width;
	uint32 SrcHeight = m_spDecoder->m_Height;
//	uint32 SrcModulo = (SrcWidth * 4 + 127) & ~127;
	uint32 SrcModulo = (SrcWidth * 4);
	uint32 DstHeight = (*_ppImg)->GetHeight();
	uint32 DstModulo = (*_ppImg)->GetModulo();

	uint8 *pDest = (uint8*)(*_ppImg)->Lock();

	uint32 CopyHeight = Min(SrcHeight, DstHeight);
	uint32 CopySize = Min(SrcModulo, DstModulo);

	for(uint y = 0; y < CopyHeight; y++)
		memcpy(pDest + y * DstModulo, pFB + y * SrcModulo, CopySize);

	(*_ppImg)->Unlock();

	m_spDecoder->m_bFrameAvail = 0;
	sys_cond_signal(m_spDecoder->m_FBCond);
	sys_mutex_unlock(m_spDecoder->m_FBMutex);
}

int CTextureContainer_Video_PS3::CTC_Texture::GetFrame()
{
	spCDecoder spDecoder = m_spDecoder;
	if(spDecoder)
	{
		return spDecoder->GetFrame();
	}

	return 0;
}

bool CTextureContainer_Video_PS3::CTC_Texture::IsOnLastFrame()
{
	spCDecoder spDecoder = m_spDecoder;
	if(spDecoder)
	{
		return spDecoder->m_bEOS == true;
	}

	return 0;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDecoder
|__________________________________________________________________________________________________
\*************************************************************************************************/

CDecoder::CDecoder()
{
	m_bCorrupt = false;
	m_bFrameAvail = false;
	m_bEOS = false;

	m_AUDone = 0;
	m_PicDone = 0;

	sys_cond_attribute_t cond_attr;
	sys_mutex_attribute_t mutex_attr;

	sys_cond_attribute_initialize( cond_attr );
	sys_mutex_attribute_initialize( mutex_attr );
	if(sys_mutex_create(&m_AUMutex, &mutex_attr) != CELL_OK)
		M_BREAKPOINT;
	if(sys_mutex_create(&m_PicMutex, &mutex_attr) != CELL_OK)
		M_BREAKPOINT;
	if(sys_mutex_create(&m_FBMutex, &mutex_attr) != CELL_OK)
		M_BREAKPOINT;
	if(sys_cond_create(&m_AUCond, m_AUMutex, &cond_attr) != CELL_OK)
		M_BREAKPOINT;
	if(sys_cond_create(&m_PicCond, m_PicMutex, &cond_attr) != CELL_OK)
		M_BREAKPOINT;
	if(sys_cond_create(&m_FBCond, m_FBMutex, &cond_attr) != CELL_OK)
		M_BREAKPOINT;
}

CDecoder::~CDecoder()
{
}

uint32 CDecoder::VideoCallback(CellVdecHandle _Handle, CellVdecMsgType _MsgType, int32 _MsgData, void* _pArg)
{
	CDecoder* pDecoder = (CDecoder*)_pArg;

	switch( _MsgType )
	{
	case CELL_VDEC_MSG_TYPE_AUDONE:
//		MRTC_SystemInfo::OS_Trace("AUDone %d\n", pDecoder->m_AUDone);
		sys_mutex_lock( pDecoder->m_AUMutex, 0 );
		pDecoder->m_AUDone++;
		sys_cond_signal( pDecoder->m_AUCond);
		sys_mutex_unlock( pDecoder->m_AUMutex );

		if( CELL_VDEC_ERROR_AU == _MsgData )
			MRTC_SystemInfo::OS_Trace( "my_cb_lib_msg: the access unit was disregarded.\n" );

		break;

	case CELL_VDEC_MSG_TYPE_PICOUT:
//		MRTC_SystemInfo::OS_Trace("PicOut %d\n", pDecoder->m_PicDone);

		sys_mutex_lock( pDecoder->m_PicMutex, 0 );
		pDecoder->m_PicDone++;
		sys_cond_signal( pDecoder->m_PicCond);
		sys_mutex_unlock( pDecoder->m_PicMutex );

		break;

	case CELL_VDEC_MSG_TYPE_SEQDONE:

		MRTC_SystemInfo::OS_Trace( "my_cb_lib_msg: got sequence done message.\n" );

		break;

	case CELL_VDEC_MSG_TYPE_ERROR:

		MRTC_SystemInfo::OS_Trace( "my_cb_lib_msg: fatal error!! goto exit.\n" );

		pDecoder->m_bCorrupt = 1;
		break;

	default:
		MRTC_SystemInfo::OS_Trace( "my_cb_lib_msg:  error! unknown callback type.\n" );
	}

	return 0;
}

bool CDecoder::Internal_Open(CellVdecCodecType _CodecType, uint _CodecProfile)
{
	CellVdecType dec_type;
	CellVdecAttr dec_attr;

	dec_type.codecType = _CodecType;
	dec_type.profileLevel = _CodecProfile;

	int32 lib_ret = cellVdecQueryAttr( &dec_type, &dec_attr );
	if(lib_ret != CELL_OK)
		M_BREAKPOINT;

	m_Resource.memAddr = M_ALLOCALIGN(dec_attr.memSize, 128);
	m_Resource.memSize = dec_attr.memSize;
	m_Resource.numOfSpus = 1;
	m_Resource.ppuThreadPriority = 450;
	m_Resource.spuThreadPriority = 150;
	m_Resource.ppuThreadStackSize = 8192;

	CellVdecCb Callback;
	Callback.cbArg = (void*)this;
	Callback.cbFunc = CDecoder::VideoCallback;

	lib_ret = cellVdecOpen(&dec_type, &m_Resource, &Callback, &m_Handle);
	if(lib_ret != CELL_OK)
	{
		MRTC_MemFree(m_Resource.memAddr);
		m_Resource.memAddr = NULL;
		m_Resource.memSize = 0;
		return false;
	}

	return true;
}

void CDecoder::Internal_Close()
{
	cellVdecClose(m_Handle);
	MRTC_MemFree(m_Resource.memAddr);
	m_Resource.memAddr = NULL;
	m_Resource.memSize = 0;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDecoder_M2V
|__________________________________________________________________________________________________
\*************************************************************************************************/

CDecoder_M2V::CDecoder_M2V()
{
}

CDecoder_M2V::~CDecoder_M2V()
{
	CDecoder_M2V::Close();
}

bool CDecoder_M2V::Open()
{
	if(!Internal_Open(CELL_VDEC_CODEC_TYPE_MPEG2, CELL_VDEC_MPEG2_MP_HL))
		return false;

	m_InputThread.Thread_Create(this, 8192);
	m_PostThread.Thread_Create(this, 8192);

	return true;
}

void CDecoder_M2V::Close()
{
	m_InputThread.Thread_RequestTermination();
	m_PostThread.Thread_RequestTermination();

	sys_mutex_lock(m_PicMutex, 0);
	sys_cond_signal(m_PicCond);
	sys_mutex_unlock(m_PicMutex);

	sys_mutex_lock(m_FBMutex, 0);
	sys_cond_signal(m_FBCond);
	sys_mutex_unlock(m_FBMutex);

	sys_mutex_lock(m_AUMutex, 0);
	sys_cond_signal(m_AUCond);
	sys_mutex_unlock(m_AUMutex);

	while(!m_InputThread.Thread_IsTerminated() || !m_PostThread.Thread_IsTerminated())
		MRTC_SystemInfo::OS_Sleep(1);

	Internal_Close();
}

uint32 CDecoder_M2V::CInputThread::get_access_unit(uint8* _pBuf, uint32 _BuffLength, uint8** _ppAUBuf, uint32* _pAULength)
{
	bool pic_found;
	uint8 *parse_head;
	uint8 *start_point;
	uint32 read_length;

	read_length = 0;
	pic_found = false;
	start_point = NULL;

	/*E parse loop. */
	while( !Thread_IsTerminating() ){

		if( _BuffLength < (read_length + 4) ){
			/*E end of stream. */
			return ~0;
		}

		parse_head = _pBuf+read_length;
		
		if( parse_head[0] || parse_head[1] || (1 != parse_head[2]) ){
			read_length++;
			continue;
		}
		
		if( CDecoder_M2V::GROUP_START_CODE == parse_head[3] ||
			CDecoder_M2V::SEQUENCE_END_CODE == parse_head[3] ||
			CDecoder_M2V::PICTURE_START_CODE == parse_head[3] ||
			CDecoder_M2V::SEQUENCE_HEADER_CODE == parse_head[3] ){
			
			if( pic_found ){
				*_ppAUBuf = start_point;

				if ( CDecoder_M2V::SEQUENCE_END_CODE == parse_head[3] )
					read_length += 4;

				*_pAULength = read_length - ( start_point - _pBuf );

				return read_length;

			}else{

				if ( CDecoder_M2V::PICTURE_START_CODE == parse_head[3] )
					pic_found = true;
				
				if( NULL == start_point )
					start_point = _pBuf+read_length;
				
			}
		}
		read_length += 4;
	}

	return 0;

}

int CDecoder_M2V::CInputThread::Thread_Main()
{
	CDecoder_M2V* pDecoder = (CDecoder_M2V*)m_Thread_pContext;
	TThinArrayAlign<uint8, 128> lStreamBuffer;
	lStreamBuffer.SetLen(512 * 1024);
	uint32 StreamBuffPos = 0, StreamBuffSize = 0;
	uint64 StreamSize = pDecoder->m_spFile->Length();
	uint64 StreamPos = 0;
	uint8* pStreamBuff = lStreamBuffer.GetBasePtr();

	int32 lib_ret;

	lib_ret = cellVdecStartSeq(pDecoder->m_Handle);
	if(lib_ret != CELL_OK)
	{
		pDecoder->m_bCorrupt = true;
		return 0;
	}

	CellVdecAuInfo au_info;
	CellVdecDecodeMode mode;
	uint64 au_cnt = 0;
	uint32 parse_length;

	uint8* au_buf = NULL;
	uint32 au_length = 0;

	while(!Thread_IsTerminating() && !pDecoder->m_bCorrupt)
	{
		parse_length = get_access_unit(pStreamBuff + StreamBuffPos, StreamBuffSize - StreamBuffPos, &au_buf, &au_length);

		if(parse_length == 0)
		{
			// No more data to read
			pDecoder->m_bEOS = true;
			break;
		}

		// Make sure the previous AU is finished before starting a new one
		sys_mutex_lock(pDecoder->m_AUMutex, 0);
		while((au_cnt > pDecoder->m_AUDone) && !Thread_IsTerminating())
			sys_cond_wait(pDecoder->m_AUCond, 0);
		sys_mutex_unlock(pDecoder->m_AUMutex);
		if(Thread_IsTerminating())
			break;

		if(parse_length == (uint32)~0)
		{
			// Need more data
			memmove(pStreamBuff, pStreamBuff + StreamBuffPos, StreamBuffSize - StreamBuffPos);
			StreamBuffSize -= StreamBuffPos;
			StreamBuffPos = 0;

			int64 ReadSize = Min((uint64)(512*1024 - StreamBuffSize), StreamSize - StreamPos);
			if(ReadSize <= 0)
			{
				// No more data to read
				pDecoder->m_bEOS = true;
				break;
			}
			pDecoder->m_spFile->Read(pStreamBuff + StreamBuffSize, ReadSize);
			StreamBuffSize += ReadSize;
			StreamPos += ReadSize;

			// Retry
			continue;
		}

		StreamBuffPos += parse_length;

		au_info.size = au_length;
		au_info.startAddr = au_buf;
		au_info.userData  = 0;/*E free use. */
		au_info.pts.lower = CELL_VDEC_PTS_INVALID;
		au_info.pts.upper = CELL_VDEC_PTS_INVALID;
		au_info.dts.lower = CELL_VDEC_DTS_INVALID;
		au_info.dts.upper = CELL_VDEC_DTS_INVALID;

		do
		{
			/*E start decode access unit. */
			mode = CELL_VDEC_DEC_MODE_NORMAL;
			lib_ret = cellVdecDecodeAu( pDecoder->m_Handle, mode, &au_info );

			if( CELL_OK != lib_ret )
			{
				if( CELL_VDEC_ERROR_BUSY == lib_ret )
				{
					MRTC_SystemInfo::OS_Trace( "input_thread: cellVdecDecodeAu return BUSY ... waiting output.\n" );
					MRTC_SystemInfo::OS_Sleep(1);
					continue;
				}
				else
				{
					MRTC_SystemInfo::OS_Trace( "input_thread: cellVdecDecodeAu fail ... ret=%x\n", lib_ret );
					pDecoder->m_bCorrupt = true;
				}
			}

		} while(0);

		au_cnt++;
	}

	lib_ret = cellVdecEndSeq(pDecoder->m_Handle);


	return 0;
}

static CMTime GetFrameTime(const CellVdecPicItem *pic_item)
{
	switch( pic_item->codecType ){
	case CELL_VDEC_CODEC_TYPE_MPEG2:
		uint frame_rate = ((CellVdecMpeg2Info*)(pic_item->picInfo))->frame_rate_code;
		switch(frame_rate)
		{
		case CELL_VDEC_MPEG2_FRC_24000DIV1001: return CMTime::CreateFromSeconds(1001.0f / 24000.0f);
		case CELL_VDEC_MPEG2_FRC_24: return CMTime::CreateFromSeconds(1.0f / 24.0f);
		case CELL_VDEC_MPEG2_FRC_25: return CMTime::CreateFromSeconds(1.0f / 25.0f);
		case CELL_VDEC_MPEG2_FRC_30: return CMTime::CreateFromSeconds(1.0f / 30.0f);
		case CELL_VDEC_MPEG2_FRC_30000DIV1001: return CMTime::CreateFromSeconds(1001.0f / 30000.0f);
		case CELL_VDEC_MPEG2_FRC_50: return CMTime::CreateFromSeconds(1.0f / 50.0f);
		case CELL_VDEC_MPEG2_FRC_60: return CMTime::CreateFromSeconds(1.0f / 60.0f);
		case CELL_VDEC_MPEG2_FRC_60000DIV1001: return CMTime::CreateFromSeconds(1001.0f / 60000.0f);
		}
		break;
	default:
		break;
	}

	/*E default interval. */
	return CMTime::CreateFromSeconds(1.0f/60.0f);
}

int CDecoder_M2V::CPostThread::Thread_Main()
{
	CDecoder_M2V* pDecoder = (CDecoder_M2V*)m_Thread_pContext;

	int32 lib_ret;
	uint64 pic_cnt;
	CellVdecMpeg2Info *pic_info;
	CellVdecPicFormat  pic_format;
	const CellVdecPicItem *pic_item;

	CMTime LastFrame = CMTime::GetCPU();

	pic_cnt = 0;
	pic_format.alpha = 0;
	pic_format.formatType = CELL_VDEC_PICFMT_RGBA32_ILV;
	pic_format.colorMatrixType = CELL_VDEC_COLOR_MATRIX_TYPE_BT709;
 
	while(!Thread_IsTerminating() && !pDecoder->m_bCorrupt)
	{
		pic_cnt++;

		sys_mutex_lock(pDecoder->m_PicMutex, 0);
		while((pic_cnt > pDecoder->m_PicDone) && !Thread_IsTerminating())
			sys_cond_wait(pDecoder->m_PicCond, 0);
		sys_mutex_unlock(pDecoder->m_PicMutex);

		if(Thread_IsTerminating())
			break;

		lib_ret = cellVdecGetPicItem(pDecoder->m_Handle, &pic_item);
		if(lib_ret != CELL_OK)
		{
			MRTC_SystemInfo::OS_Trace( "post_thread: cellVdecGetPicItem fail ... %x\n", lib_ret );
			continue;
		}

		if(CELL_VDEC_PICITEM_ATTR_SKIPPED == pic_item->attr)
		{
			// Picture skipped
			cellVdecGetPicture(pDecoder->m_Handle, &pic_format, NULL);

			// Update out counter?
			continue;
		}

		CMTime NextFrame = LastFrame + GetFrameTime(pic_item);
		CMTime Now = CMTime::GetCPU();

		CMTime DeltaTime = NextFrame - Now;

		if(DeltaTime.GetTime() > 0)
			MRTC_SystemInfo::OS_Sleep((int)(DeltaTime.GetTime() * 1000.0f));
		LastFrame = NextFrame;

		// Flag new frame available here
		pic_info = (CellVdecMpeg2Info*)pic_item->picInfo;
		{
			sys_mutex_lock(pDecoder->m_FBMutex, 0);
			if(!Thread_IsTerminating())
			{
				if(pDecoder->m_Width != pic_info->horizontal_size || pDecoder->m_Height != pic_info->vertical_size)
				{
					pDecoder->m_Width = pic_info->horizontal_size;
					pDecoder->m_Height = pic_info->vertical_size;
					pDecoder->m_lFrameBuffer.SetLen((pDecoder->m_Width * pDecoder->m_Height * 4 + 127) & ~127);
				}

				lib_ret = cellVdecGetPicture(pDecoder->m_Handle, &pic_format, pDecoder->m_lFrameBuffer.GetBasePtr());
				if(lib_ret == CELL_OK)
				{
					pDecoder->m_bFrameAvail = true;
				}
				else
				{
					MRTC_SystemInfo::OS_Trace( "post_thread: cellVdecGetPicture fail ... %x\n", lib_ret );
				}
			}

			sys_mutex_unlock(pDecoder->m_FBMutex);
		}

	}

	return 0;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDecoder_AVC
|__________________________________________________________________________________________________
\*************************************************************************************************/

CDecoder_AVC::CDecoder_AVC()
{
}

CDecoder_AVC::~CDecoder_AVC()
{
	CDecoder_AVC::Close();
}

bool CDecoder_AVC::Open()
{
	if(!Internal_Open(CELL_VDEC_CODEC_TYPE_AVC, CELL_VDEC_AVC_LEVEL_4P2))
		return false;
	return true;
}

void CDecoder_AVC::Close()
{
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer_Video_PS3
|__________________________________________________________________________________________________
\*************************************************************************************************/
CTextureContainer_Video_PS3::CTextureContainer_Video_PS3()
{
	CellVdecType dec_type;
	CellVdecAttr dec_attr;

	{
		MRTC_SystemInfo::OS_Trace("MPEG2 Video\r\n");
		uint aProfiles[] = {CELL_VDEC_MPEG2_MP_LL, CELL_VDEC_MPEG2_MP_ML, CELL_VDEC_MPEG2_MP_H14, CELL_VDEC_MPEG2_MP_HL};
		for(uint i = 0; i < sizeof(aProfiles) / sizeof(uint); i++)
		{
			dec_type.codecType = CELL_VDEC_CODEC_TYPE_MPEG2;
			dec_type.profileLevel = aProfiles[i];

			int32 lib_ret = cellVdecQueryAttr( &dec_type, &dec_attr );
			if(lib_ret != CELL_OK)
				continue;

			MRTC_SystemInfo::OS_Trace("-- Profile %d\r\n", aProfiles[i]);
			MRTC_SystemInfo::OS_Trace("-- memSize: %d\r\n", dec_attr.memSize);
			MRTC_SystemInfo::OS_Trace("-- cmdDepth: %d\r\n", dec_attr.cmdDepth);
			MRTC_SystemInfo::OS_Trace("-- decoderVerUpper: %d\r\n", dec_attr.decoderVerUpper);
			MRTC_SystemInfo::OS_Trace("-- decoderVerLower: %d\r\n", dec_attr.decoderVerLower);
		}
	}

	{
		MRTC_SystemInfo::OS_Trace("AVC Video\r\n");
		uint aProfiles[] = {CELL_VDEC_AVC_LEVEL_2P1, CELL_VDEC_AVC_LEVEL_3P0, CELL_VDEC_AVC_LEVEL_4P1, CELL_VDEC_AVC_LEVEL_4P2};
		for(uint i = 0; i < sizeof(aProfiles) / sizeof(uint); i++)
		{
			dec_type.codecType = CELL_VDEC_CODEC_TYPE_AVC;
			dec_type.profileLevel = aProfiles[i];

			int32 lib_ret = cellVdecQueryAttr( &dec_type, &dec_attr );
			if(lib_ret != CELL_OK)
				continue;

			MRTC_SystemInfo::OS_Trace("-- Profile %d\r\n", aProfiles[i]);
			MRTC_SystemInfo::OS_Trace("-- memSize: %d\r\n", dec_attr.memSize);
			MRTC_SystemInfo::OS_Trace("-- cmdDepth: %d\r\n", dec_attr.cmdDepth);
			MRTC_SystemInfo::OS_Trace("-- decoderVerUpper: %d\r\n", dec_attr.decoderVerUpper);
			MRTC_SystemInfo::OS_Trace("-- decoderVerLower: %d\r\n", dec_attr.decoderVerLower);
		}
	}
}

CTextureContainer_Video_PS3::~CTextureContainer_Video_PS3()
{
	for(uint i = 0; i < m_lspVideos.Len(); i++)
	{
		if(m_lspVideos[i])
		{
			m_lspVideos[i]->Close();
			m_pTC->FreeID(m_lspVideos[i]->m_TextureID);
		}
	}
}

void CTextureContainer_Video_PS3::Create(void* _pContext)
{
	MAUTOSTRIP(CTextureContainer_Video_PS3_Create, MAUTOSTRIP_VOID);
	m_pTC->EnableTextureClassRefresh(m_iTextureClass);
	
	if (_pContext)
		AddVideo(CStr((const char*)_pContext));
}

int CTextureContainer_Video_PS3::AddVideo(CStr _FileName)
{
	spCTC_Texture spTex = MNew(CTC_Texture);
	if(_FileName.GetFilenameExtenstion().CompareNoCase("M2V") == 0)
		spTex->m_Type = CTC_Texture::VIDEO_TYPE_M2V;
	else if(_FileName.GetFilenameExtenstion().CompareNoCase("AVC") == 0)
		spTex->m_Type = CTC_Texture::VIDEO_TYPE_AVC;
	else
		spTex->m_Type = CTC_Texture::VIDEO_TYPE_UNDEFINED;

	spTex->Create(_FileName);
	spTex->m_TextureID = m_pTC->AllocID(m_iTextureClass, m_lspVideos.Len(), spTex->m_TextureName.GetStr());
	spTex->m_Width = 720;
	spTex->m_Height = 480;

	return m_lspVideos.Add(spTex);
}

void CTextureContainer_Video_PS3::CreateFromDirectory(CStr _Path)
{
}


void CTextureContainer_Video_PS3::CloseVideo(int _iLocal)
{
	uint iLocal = _iLocal / 3;
	CTC_Texture* pVideo = m_lspVideos[iLocal];
	if(pVideo->IsOpen())
	{
		pVideo->Close();
		if(pVideo->m_TextureID) m_pTC->MakeDirty(pVideo->m_TextureID);
	}
}

void CTextureContainer_Video_PS3::SetVolume(int _iLocal, fp32 fpVol) // NOT SUPPORTED
{
}

void CTextureContainer_Video_PS3::Pause(int _iLocal, bool _Paused)  // NOT SUPPORTED
{
}

void CTextureContainer_Video_PS3::AutoRestart(int _iLocal, bool _EnableAutoRestart) // NOT SUPPORTED
{
}

void CTextureContainer_Video_PS3::Rewind(int _iLocal)
{
	uint iLocal = _iLocal / 3;
	CTC_Texture* pVideo = m_lspVideos[iLocal];
	if(pVideo->IsOpen())
		pVideo->Close();

	pVideo->Open();
	if(pVideo->m_TextureID) m_pTC->MakeDirty(pVideo->m_TextureID);
}

bool CTextureContainer_Video_PS3::IsOnLastFrame(int _iLocal)
{
	return m_lspVideos[_iLocal]->IsOnLastFrame();
}

bool CTextureContainer_Video_PS3::MoveToLastFrame(int _iLocal) // NOT SUPPORTED
{
	return false;
}

int CTextureContainer_Video_PS3::GetFrame(int _iLocal)
{
	return m_lspVideos[_iLocal]->GetFrame();
}

int CTextureContainer_Video_PS3::GetNumFrames(int _iLocal) // NOT SUPPORTED
{
	return ~0;
}


int CTextureContainer_Video_PS3::GetNumLocal()
{
	return m_lspVideos.Len();
}

int CTextureContainer_Video_PS3::GetLocal(const char* _pName)
{
	for(uint i = 0; i < m_lspVideos.Len(); i++)
	{
		if(m_lspVideos[i]->m_Filename.CompareNoCase(_pName) == 0)
			return i;
	}

	return -1;
}

int CTextureContainer_Video_PS3::GetTextureID(int _iLocal)
{
	return m_lspVideos[_iLocal]->m_TextureID;
}

int CTextureContainer_Video_PS3::GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps)
{
	CTC_Texture* pVideo = m_lspVideos[_iLocal];

	if (!pVideo->IsOpen())
	{
		pVideo->Open();
		if(pVideo->m_TextureID) m_pTC->MakeDirty(pVideo->m_TextureID);
	}

	if(!pVideo->IsOpen())
		return 0;

	_pTargetImg->CreateVirtual(pVideo->m_Width, pVideo->m_Height, IMAGE_FORMAT_RGBA8, IMAGE_MEM_IMAGE);
	_Ret_nMipmaps = 1;

	return 0;
}

void CTextureContainer_Video_PS3::GetTextureProperties(int _iLocal, CTC_TextureProperties& _Properties)
{
	CTC_TextureProperties Prop;
	Prop.m_Flags = 
		CTC_TEXTUREFLAGS_CLAMP_U | CTC_TEXTUREFLAGS_CLAMP_V  |
		CTC_TEXTUREFLAGS_NOMIPMAP | CTC_TEXTUREFLAGS_NOPICMIP |
		CTC_TEXTUREFLAGS_HIGHQUALITY | CTC_TEXTUREFLAGS_NOCOMPRESS |
		CTC_TEXTUREFLAGS_PROCEDURAL;
	_Properties = Prop;
}

void CTextureContainer_Video_PS3::OnRefresh()
{
	TAP_RCD<spCTC_Texture> lspVideos = m_lspVideos;
	for(int i = 0; i < lspVideos.Len(); i++)
	{
		CTC_Texture* pVideo = lspVideos[i];
		if(pVideo->IsOpen())
		{
			if(pVideo->m_spDecoder->NewFrameAvailable() && pVideo->m_TextureID)
			{
				m_pTC->MakeDirty(pVideo->m_TextureID);
			}
		}
	}
}

CStr CTextureContainer_Video_PS3::GetName(int _iLocal)
{
	return m_lspVideos[_iLocal]->m_TextureName;
}

void CTextureContainer_Video_PS3::BuildInto(int _iLocal, CImage** _ppImg, int _nMipmaps, int _TextureVersion, int _ConvertType, int _iStartMip, uint32 _BuildFlags)
{
	CTC_Texture* pVideo = m_lspVideos[_iLocal];

	if (!pVideo->IsOpen())
	{
		pVideo->Open();
		if(pVideo->m_TextureID) m_pTC->MakeDirty(pVideo->m_TextureID);
	}

	if(!pVideo->IsOpen())
		return;

	pVideo->BuildInto(_ppImg, _BuildFlags);
}

void CTextureContainer_Video_PS3::SetSoundHandle(int _iLocal, int _hSound)
{
}


int CTextureContainer_Video_PS3::GetWidth(int _iLocal)
{
	CTC_Texture* pVideo = m_lspVideos[_iLocal];

	if (!pVideo->IsOpen())
	{
		pVideo->Open();
		if(pVideo->m_TextureID) m_pTC->MakeDirty(pVideo->m_TextureID);
	}
	if (!pVideo->IsOpen())
		return 0;


	return m_lspVideos[_iLocal]->m_Width;
}

int CTextureContainer_Video_PS3::GetHeight(int _iLocal)
{
	CTC_Texture* pVideo = m_lspVideos[_iLocal];

	if (!pVideo->IsOpen())
	{
		pVideo->Open();
		if(pVideo->m_TextureID) m_pTC->MakeDirty(pVideo->m_TextureID);
	}
	if (!pVideo->IsOpen())
		return 0;

	return m_lspVideos[_iLocal]->m_Height;
}


fp32 CTextureContainer_Video_PS3::GetTime(int _iLocal)
{
	return 0.0f;
}

MRTC_IMPLEMENT_DYNAMIC(CTextureContainer_Video_PS3, CTextureContainer_Video);



#endif

