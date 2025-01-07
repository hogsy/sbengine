

#include "PCH.h"

#ifndef IMAGE_IO_NOVORBIS

#include "mrtc.h"

#include "../../MSystem.h"

#include "../MSound_Core.h"


//#pragma include_alias( <..\..\..\..\SDK\Vorbis\ogg\include\ogg\os_types.h>, <ogg/os_types.h> )
//#pragma include_alias( <..\..\..\ogg\include\ogg\ogg.h>, <ogg/ogg.h> )

#include "ogg/ogg.h"
#include "vorbis/codec.h"
#include "vorbis/vorbisenc.h"

#include <vorbis/vorbisfile.h>



class CMSound_Codec_VORB : public CSCC_Codec
{
	MRTC_DECLARE;

public:

#ifndef SOUND_IO_NOCOMPRESS
	class CEncoder
	{
	public:
		ogg_stream_state OggStreamState;	// take physical pages, weld into a logical stream of packets
		ogg_page         OggPage;			// one Ogg bitstream page.  Vorbis packets are inside
		ogg_packet       OggPacket;			// one raw packet of data for decode

		vorbis_info      VorbisInfo;		// struct that stores all the static vorbis bitstream settings
		vorbis_dsp_state VorbisDSPState;	// central working state for the packet->PCM decoder
		vorbis_block     VorbisBlock;		// local working space for packet->PCM decode

		bool m_bEOS;

	};
#endif
	
	CMSound_Codec_VORB()
	{		
		MSCOPE(CMSound_Codec_VORB::CMSound_Codec_VORB, VORBIS);
#ifndef SOUND_IO_NOCOMPRESS
		m_pEncoder = NULL;

#endif
		m_pDecoder = NULL;
	}
	
	~CMSound_Codec_VORB()
	{		
		MSCOPE(CMSound_Codec_VORB::CMSound_Codec_VORB, VORBIS);
		Close();
	}

	virtual void Close()
	{
		MSCOPE(CMSound_Codec_VORB::Close, VORBIS);
#ifndef SOUND_IO_NOCOMPRESS
		if (m_pEncoder)
		{
			// Finish off the encode and flush all data

			// end of file.  this can be done implicitly in the mainline,
			//  but it's easier to see here in non-clever fashion.
			// Tell the library we're at end of stream so that it can handle
			// the last frame and mark end of stream in the output properly 
			vorbis_analysis_wrote(&m_pEncoder->VorbisDSPState,0);

			EncodeSubmittedData();

			// clean up and exit.  vorbis_info_clear() must be called last 
			
			ogg_stream_clear(&m_pEncoder->OggStreamState);
			vorbis_block_clear(&m_pEncoder->VorbisBlock);
			vorbis_dsp_clear(&m_pEncoder->VorbisDSPState);
			vorbis_info_clear(&m_pEncoder->VorbisInfo);
			
			// ogg_page and ogg_packet structs always point to storage in
			// libvorbis.  They're never freed or manipulated directly

			delete m_pEncoder;
			m_pEncoder = NULL;
		}
#endif


		if (m_pDecoder)
		{
			ov_clear(&m_pDecoder->VorbisFile);
    
			delete m_pDecoder;
			m_pDecoder = NULL;
		}
	}

	/*************************************************************************************************\
	|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
	| Encoder
	|__________________________________________________________________________________________________
	\*************************************************************************************************/

#ifndef SOUND_IO_NOCOMPRESS
	CEncoder *m_pEncoder;
#endif

#ifndef SOUND_IO_NOCOMPRESS
	void EncodeSubmittedData()
	{
		MSCOPE(CMSound_Codec_VORB::EncodeSubmittedData, VORBIS);
		// vorbis does some data preanalysis, then divvies up blocks for
		// more involved (potentially parallel) processing.  Get a single
		// block for encoding now 
		while(vorbis_analysis_blockout(&m_pEncoder->VorbisDSPState, &m_pEncoder->VorbisBlock)==1)
		{
			
			// analysis 
			vorbis_analysis(&m_pEncoder->VorbisBlock,&m_pEncoder->OggPacket);
			
			// weld the packet into the bitstream 
			ogg_stream_packetin(&m_pEncoder->OggStreamState,&m_pEncoder->OggPacket);
			
			// write out pages (if any) 
			while(!m_pEncoder->m_bEOS)
			{
				int result=ogg_stream_pageout(&m_pEncoder->OggStreamState,&m_pEncoder->OggPage);
				
				if (result==0)
					break;
				
				m_pFile->Write(m_pEncoder->OggPage.header,m_pEncoder->OggPage.header_len);
				m_pFile->Write(m_pEncoder->OggPage.body,m_pEncoder->OggPage.body_len);
				
				// this could be set above, but for illustrative purposes, I do 
				//  it here (to show that vorbis does know where the stream ends) 
				
				if(ogg_page_eos(&m_pEncoder->OggPage))
					m_pEncoder->m_bEOS=true;
			}
		}		
	}
#endif

	virtual bool CreateEncoder(float _Quality)
	{
		MSCOPE(CMSound_Codec_VORB::CreateEncoder, VORBIS);
#ifndef SOUND_IO_NOCOMPRESS
		if (m_pEncoder)
			delete m_pEncoder;

		m_pEncoder = DNew(CEncoder) CEncoder;

		vorbis_info_init(&m_pEncoder->VorbisInfo);

		float Quality = _Quality;

		if (m_Format.m_Data.GetSampleRate() > 0)
		{
//			float BitRate = (m_Format.m_SampleRate / 44100.0) * (m_Format.m_nChannels / 2.0) * 128000.0;
//
//			if (BitRate < 192000.0)
//				BitRate = 192000.0;

			float BitRate = Quality * 1000.0f;

			vorbis_encode_init_vbr(&m_pEncoder->VorbisInfo, m_Format.m_Data.GetChannels(), m_Format.m_Data.GetSampleRate(), Quality * 0.75f);
//			vorbis_encode_init(&m_pEncoder->VorbisInfo, m_Format.m_nChannels, m_Format.m_SampleRate, -1, BitRate, -1);
		}
		else
		{
//			float BitRate = Quality * (m_Format.m_nChannels / 2.0) * 128000;
//
//			if (BitRate < 192000.0)
//				BitRate = 192000.0;

//			BitRate *= Quality;
			float BitRate = Quality * 1000.0f;

			vorbis_encode_init_vbr(&m_pEncoder->VorbisInfo, m_Format.m_Data.GetChannels(), 44100, Quality * 0.75f);
//			vorbis_encode_init(&m_pEncoder->VorbisInfo,m_Format.m_nChannels, 44100, -1, BitRate, -1);
		}

		vorbis_comment   VorbisComment;		// struct that stores all the user comments

		m_pEncoder->m_bEOS = false;

		// add a comment
		vorbis_comment_init(&VorbisComment);
		vorbis_comment_add_tag(&VorbisComment,"ENCODER","XWC");
//		vorbis_comment_add(&VorbisComment,"");

		vorbis_analysis_init(&m_pEncoder->VorbisDSPState,&m_pEncoder->VorbisInfo);
		vorbis_block_init(&m_pEncoder->VorbisDSPState,&m_pEncoder->VorbisBlock);

		// set up our packet->stream encoder
		// pick a random serial number; that way we can more likely build
		// chained streams just by concatenation
		MRTC_GetRand()->InitRand((uint32)(((int64)CMTime::GetCPU().GetCycles())&0xFFFFFFFF));
		ogg_stream_init(&m_pEncoder->OggStreamState,MRTC_RAND());

		// set up the analysis state and auxiliary encoding storage

		// Vorbis streams begin with three headers; the initial header (with
		// most of the codec setup parameters) which is mandated by the Ogg
		// bitstream spec.  The second header holds any comment fields.  The
		// third header holds the bitstream codebook.  We merely need to
		// make the headers, then pass them to libvorbis one at a time;
		// libvorbis handles the additional Ogg bitstream constraints
		
		{
			ogg_packet header;
			ogg_packet header_comm;
			ogg_packet header_code;
			
			vorbis_analysis_headerout(&m_pEncoder->VorbisDSPState,&VorbisComment,&header,&header_comm,&header_code);
			ogg_stream_packetin(&m_pEncoder->OggStreamState,&header); // automatically placed in its own page
			ogg_stream_packetin(&m_pEncoder->OggStreamState,&header_comm);
			ogg_stream_packetin(&m_pEncoder->OggStreamState,&header_code);
			
			// We don't have to write out here, but doing so makes streaming
			// much easier, so we do, flushing ALL pages. This ensures the actual
			// audio data will start on a new page
			while(!m_pEncoder->m_bEOS)
			{
				int result = ogg_stream_flush(&m_pEncoder->OggStreamState,&m_pEncoder->OggPage);
				
				if (result == 0)
					break;
				
				m_pFile->Write(m_pEncoder->OggPage.header,m_pEncoder->OggPage.header_len);
				m_pFile->Write(m_pEncoder->OggPage.body,m_pEncoder->OggPage.body_len);
			}			
		}
		
		vorbis_comment_clear(&VorbisComment);
		
		return true;
#else
		return false;
#endif
	}

	virtual bool AddData(void *&_pData, int &_nBytes)
	{
		MSCOPE(CMSound_Codec_VORB::AddData, VORBIS);
		
#ifndef SOUND_IO_NOCOMPRESS
		while (1)
		{
			int SubmittedSamples = _nBytes / (m_Format.m_Data.GetSampleSize() * m_Format.m_Data.GetChannels());
			long Samples = Min(1024, SubmittedSamples);
			
			if (!Samples)
				return true;
			
			// data to encode
			
			// expose the buffer to submit data 
			float **buffer = vorbis_analysis_buffer(&m_pEncoder->VorbisDSPState,Samples);
			
			// uninterleave samples
			switch (m_Format.m_Data.GetSampleSize())
			{
			case 1:
				{						
					for(int s = 0; s < Samples; s++)
					{
						for(int i = 0; i < m_Format.m_Data.GetChannels(); i++)
						{
							buffer[i][s] = (*((uint8 *)_pData) / 128.0) - 1.0;
							*((uint8**)&_pData) += 1;
							_nBytes -= 1;
						}
					}
				}
				break;
			case 2:
				{						
					for(int s = 0; s < Samples; s++)
					{
						for(int i = 0; i < m_Format.m_Data.GetChannels(); i++)
						{
							buffer[i][s] = *((int16 *)_pData) / 32768.0;
							*((uint8**)&_pData) += 2;
							_nBytes -= 2;
						}
					}
				}
				break;
			case 3:
				{	
					Error("Encode","I have no idea how 24 data is saved. Implement");
					return false;
					/*
					for(int s = 0; s < Samples; s++)
					{
						for(i = 0; i < Channels; i++)
						{
							buffer[i][s] = *((int16 *)WavePtr) / 32768.0;
							WavePtr += BitDepth;
						}
					}*/
				}
				break;
			case 4:
				{						
					Error("Encode","I have no idea how 32 data is saved. Implement");
					return false;
					/*
					for(int s = 0; s < Samples; s++)
					{
						for(i = 0; i < Channels; i++)
						{
							buffer[i][s] = *((int16 *)WavePtr) / 32768.0;
							WavePtr += BitDepth;
						}
					}
					*/
				}
				break;
			default:
				Error("Compress", "Sampleformat unsupported");
				return false;
				break;
			}
			// tell the library how much we actually submitted 
			vorbis_analysis_wrote(&m_pEncoder->VorbisDSPState,Samples);
			
			EncodeSubmittedData();

		}

		return true;
#else
		return false;
#endif

	}

	/*************************************************************************************************\
	|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
	| Decoder
	|__________________________________________________________________________________________________
	\*************************************************************************************************/
	
	class CDecoder
	{
	public:
		OggVorbis_File VorbisFile;
		int Section;
		int m_FileOffset;

		CDecoder()
		{
			memset(this, 0, sizeof(CDecoder));
		}
	};
	
	CDecoder *m_pDecoder;

	static size_t Callback_Read(void *ptr, size_t size, size_t nmemb, CMSound_Codec_VORB *This)
	{
		MSCOPE(CMSound_Codec_VORB::Close, VORBIS);
		int ReadBytes = -This->m_pFile->Pos();

		This->m_pFile->Read(ptr, size * nmemb);

		ReadBytes += This->m_pFile->Pos();

		return ReadBytes;
	}

	static int Callback_Seek(CMSound_Codec_VORB *This, ogg_int64_t offset, int whence)
	{
		MSCOPE(CMSound_Codec_VORB::Close, VORBIS);
		switch (whence)
		{
		case SEEK_SET:
			This->m_pFile->Seek(This->m_pDecoder->m_FileOffset + offset);
			break;
		case SEEK_END:
			This->m_pFile->SeekToEnd();
			M_ASSERT(offset<=0, "Maybe offset sould be reversed ??");
			This->m_pFile->RelSeek(offset);
			break;
		case SEEK_CUR:
			This->m_pFile->RelSeek(offset);
			break;
		}
		return This->m_pFile->Pos() - This->m_pDecoder->m_FileOffset;
	}

	static int Callback_Close(CMSound_Codec_VORB *This)
	{
		MSCOPE(CMSound_Codec_VORB::Close, VORBIS);
		// We don't care about closing the stream... that is not our job
		//M_ASSERT(0, "Why would it want to close the file, bastards :)");
		return false;
	}

	static long Callback_Tell(CMSound_Codec_VORB *This)
	{
		MSCOPE(CMSound_Codec_VORB::Close, VORBIS);
		return This->m_pFile->Pos() - This->m_pDecoder->m_FileOffset;
	}
	
	virtual bool CreateDecoder(int _Flags)
	{
		MSCOPE(CMSound_Codec_VORB::CreateDecoder, VORBIS);
		if (m_pDecoder)
			delete m_pDecoder;

		m_pDecoder = DNew(CDecoder) CDecoder;

		m_pDecoder->m_FileOffset = m_pFile->Pos();

		ov_callbacks CallBacks;

		CallBacks.read_func = (size_t (*) (void *ptr, size_t size, size_t nmemb, void *datasource))Callback_Read;
		CallBacks.seek_func = (int (*) (void *datasource, ogg_int64_t offset, int whence))Callback_Seek;
		CallBacks.close_func = (int (*) (void *datasource))Callback_Close;
		CallBacks.tell_func = (long (*) (void *datasource))Callback_Tell; 

		m_pDecoder->Section = 0;

		if(ov_open_callbacks(this, &m_pDecoder->VorbisFile, NULL, 0, CallBacks) < 0)
			return false;

		// Get info for stream
	    vorbis_info *VorbisInfo = ov_info(&m_pDecoder->VorbisFile,-1);

		m_Format.m_Data.SetSampleRate(VorbisInfo->rate);
		m_Format.m_Data.SetChannels(VorbisInfo->channels);

		//VorbisInfo->channels
		
		return true;
	}

	virtual bool SeekData(int _SampleOffset)
	{
		MSCOPE(CMSound_Codec_VORB::SeekData, VORBIS);
		if (ov_pcm_seek(&m_pDecoder->VorbisFile, _SampleOffset))
			Error("Decode", "Seekerror in stream");

		return true;
	}

	virtual mint GetData(fp32 *_pDest, mint _nMaxSamples, bint _bLooping)
	{
		enum
		{
#ifdef CPU_BIGENDIAN
			BigEndian = 1
#else
			BigEndian = 0
#endif
		};
		MSCOPE(CMSound_Codec_VORB::GetData, VORBIS);
		if (_bLooping)
		{
			int notehu= 0;
		}
		int NumTimesNoData = 0;
		mint nSamples = _nMaxSamples;
		uint32 nChannels = m_Format.m_Data.GetChannels();
		fp32 *pDest = _pDest;
		while(nSamples)
		{
			long nDecoded;
			float **ppData;
			nDecoded = ov_read_float(&m_pDecoder->VorbisFile, &ppData, nSamples, &m_pDecoder->Section);
	
			if (nDecoded == 0)
			{
				if (_bLooping)
				{
					if (ov_pcm_seek(&m_pDecoder->VorbisFile, 0))
						Error("Decode", "Seekerror in stream");
				}
				else
					return _nMaxSamples - nSamples;
			} 
			else if (nDecoded < 0)
			{
				Error("Decode", "Streaming error");
				return _nMaxSamples - nSamples;
			} 
			else
			{
				// Do the interleave
				// TODO: Optimize
				for (uint32 i = 0; i < nDecoded; ++i)
				{
					for (uint32 j = 0; j < nChannels; ++j)
					{
						pDest[j] = ppData[j][i];
					}
					pDest += nChannels;
				}
				nSamples -= nDecoded;
			}
		}
		return _nMaxSamples - nSamples;
	}

	virtual bool GetData(void *&_pData, mint &_nBytes, bool _bLooping, bool &_bReadSomething)
	{
		enum
		{
#ifdef CPU_BIGENDIAN
			BigEndian = 1
#else
			BigEndian = 0
#endif
		};
		_bReadSomething = true;
		MSCOPE(CMSound_Codec_VORB::GetData, VORBIS);
		int NumTimesNoData = 0;
		while(_nBytes)
		{
			
			long ret;

//			int StartOffset = ov_pcm_tell(&m_pDecoder->VorbisFile);
//
//			if (StartOffset<0)
//				Error("Decode","Offset error");
			
			// uninterleave samples
			switch (m_Format.m_Data.GetSampleSize())
			{
			case 1:
				{						
					ret = ov_read(&m_pDecoder->VorbisFile,(char *)_pData,_nBytes,BigEndian,1,0,&m_pDecoder->Section);
				}
				break;
			case 2:
				{						
					ret = ov_read(&m_pDecoder->VorbisFile,(char *)_pData,_nBytes,BigEndian,2,1,&m_pDecoder->Section);
				}
				break;
			case 3:
				{	
					Error("Decode","24 bit format not supported under vorbisfile");
					return false;
				}
				break;
			case 4:
				{						
					Error("Decode","32 bit format not supported under vorbisfile");
					return false;
				}
				break;
			default:
				Error("Decode", "Sampleformat unsupported");
				return false;
				break;
			}

//			int EndOffset = ov_pcm_tell(&m_pDecoder->VorbisFile);
//			if (EndOffset<0)
//				Error("Decode","Offset error");

//			if (EndOffset == StartOffset)
//				++NumTimesNoData;
//			else
//				NumTimesNoData = 0;

//			int BytesCopied = (EndOffset - StartOffset) * m_Format.m_SampleSize * m_Format.m_nChannels;

//			_nBytes -= BytesCopied;
//			*((uint8**)&_pData) += BytesCopied;
	
//			if (NumTimesNoData > 2) 
//			{
//				if (m_pDecoder->m_bLoop)
//				{
///					if (ov_pcm_seek(&m_pDecoder->VorbisFile, 0))
///						Error("Decode", "Seekerror in stream");
//				}
//				return true;
//			} 
//			else if (ret < 0) 
//			{
//				return false;
//			} 

			if (ret == 0)
			{
				if (_bLooping)
				{
					if (ov_pcm_seek(&m_pDecoder->VorbisFile, 0))
						Error("Decode", "Seekerror in stream");

					return true;
				}
				return false;
			} 
			else if (ret < 0)
			{
				Error("Decode", "Streaming error");
				return false;
			} 
			else
			{
				_nBytes -= ret;
				*((uint8**)&_pData) += ret;
			}
		}

		return true;
	}
};

MRTC_IMPLEMENT_DYNAMIC(CMSound_Codec_VORB, CSCC_Codec);

#endif
