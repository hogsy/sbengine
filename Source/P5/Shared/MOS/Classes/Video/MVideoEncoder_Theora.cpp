
#include "PCH.h"

#include "theora/theoraenc.h"
#include "stdlib.h"
#include "time.h"

/*
Y  =      (0.257 * R) + (0.504 * G) + (0.098 * B) + 16

Cr = V =  (0.439 * R) - (0.368 * G) - (0.071 * B) + 128

Cb = U = -(0.148 * R) - (0.291 * G) + (0.439 * B) + 128


YUV to RGB Conversion

B = 1.164(Y - 16)                   + 2.018(U - 128)

G = 1.164(Y - 16) - 0.813(V - 128) - 0.391(U - 128)

R = 1.164(Y - 16) + 1.596(V - 128)
*/

#include "MVideoEncoder.h"

class CMVideoEncoder_Theora : public CMVideoEncoder
{
	MRTC_DECLARE;
private:
	int m_VideoX;
	int m_VideoY;

	int m_FrameX;
	int m_FrameY;

	fp32 m_FrameRate;
	fp32 m_PixelAspect;
	fp32 m_Quality;
	fp32 m_Bitrate;

	CCFile *m_pFile;

	class CTheoraParams
	{
	public:
		ogg_stream_state to; // take physical pages, weld into a logical stream of packets
		ogg_page         og; // one Ogg bitstream page.  Vorbis packets are inside
		ogg_packet       op; // one raw packet of data for decode 

		theora_enc_ctx  *td;
//		theora_state     td;
		theora_comment   tc;

		CTheoraParams()
		{
		}

	};

	CTheoraParams m_Theora;

public:

	int GetBitPos(uint32 _Number)
	{
		for (int i = 0; i < 31; ++i)
		{
			if (_Number & (1 << i))
				return i;
		}
		return 0;
	}
	void InitEncode(CCFile *_pFile, int _Width, int _Height, fp32 _FrameRate, int _Keyframe, fp32 _Quality, fp32 _Bitrate, fp32 _PixelAspect, CStr _ExtraInfo)
	{
		m_pFile = _pFile;

		m_FrameX = _Width;
		m_FrameY = _Height;

		m_FrameRate = _FrameRate;
		m_PixelAspect = _PixelAspect;
		m_Quality = _Quality;
		m_Bitrate = _Bitrate;

		// yayness.  Set up Ogg output stream
		MRTC_GetRand()->InitRand(time(NULL));
		ogg_stream_init(&m_Theora.to,MRTC_RAND()); // oops, add one ot the above

		// Theora has a divisible-by-sixteen restriction for the encoded video size
		// scale the frame size up to the nearest /16 and calculate offsets
		m_VideoX = ((m_FrameX + 15) >>4)<<4;
		m_VideoY = ((m_FrameY + 15) >>4)<<4;

		m_YuvFrame.SetLen(m_VideoX*m_VideoY*3/2);

		/* clear initial frame as it may be larger than actual video data */
		/* fill Y plane with 0x10 and UV planes with 0X80, for black data */
		memset(m_YuvFrame.GetBasePtr(),0x10,m_VideoX*m_VideoY);
		memset(m_YuvFrame.GetBasePtr()+m_VideoX*m_VideoY,0x80,m_VideoX*m_VideoY/2);

		memset(&m_TempVideoPage, 0, sizeof(m_TempVideoPage));

		theora_info      ti;
		theora_info_init(&ti);
		ti.pic_width = m_FrameX;
		ti.pic_height = m_FrameY;
		ti.frame_width = m_VideoX;
		ti.frame_height = m_VideoY;
		ti.pic_x = 0;
		ti.pic_y = 0;
		ti.fps_numerator = m_FrameRate * 1000;
		ti.fps_denominator = 1000;
		ti.aspect_numerator = ((fp32)m_FrameX / (fp32)m_FrameY) * m_PixelAspect * 1000;
		ti.aspect_denominator = 1000;
		ti.colorspace = OC_CS_UNSPECIFIED;
		ti.target_bitrate = m_Bitrate;
		ti.keyframe_granule_shift = GetBitPos(_Keyframe);
		ti.quality = 63 * m_Quality;
		m_Theora.td	= theora_encode_alloc(&ti);

		ogg_uint32_t KeyFrams = _Keyframe;
		theora_encode_ctl(m_Theora.td, OC_ENCCTL_SET_KEYFRAME_FREQUENCY_FORCE, &KeyFrams, sizeof(KeyFrams));
		

		theora_info_clear(&ti);

		/* first packet will get its own page automatically */
		theora_comment_init(&m_Theora.tc);

		if(theora_encode_flushheader(m_Theora.td,&m_Theora.tc,&m_Theora.op) <= 0)
		{
			Error_static("CMVideoEncoder_Theora::InitEncode::ogg_stream_pageout", "Internal Ogg library error.\n");
		}


//		theora_encode_header(&m_Theora.td,&m_Theora.op);
		ogg_stream_packetin(&m_Theora.to,&m_Theora.op);

		if(ogg_stream_pageout(&m_Theora.to,&m_Theora.og)!=1)
		{
			Error_static("CMVideoEncoder_Theora::InitEncode::ogg_stream_pageout", "Internal Ogg library error.\n");
		}

		m_pFile->Write(m_Theora.og.header, m_Theora.og.header_len);
		m_pFile->Write(m_Theora.og.body, m_Theora.og.body_len);

		// create the remaining theora headers 

//		char *pComment = (char *)_ExtraInfo.Str();
		m_Theora.tc.comments = 0;
//		m_Theora.tc.user_comments = &pComment;		
//		int Length = _ExtraInfo.Len() + 1;
//		m_Theora.tc.comment_lengths = &Length;
		m_Theora.tc.vendor = "Starbreeze";

		for(;;)
		{
			int ret=theora_encode_flushheader(m_Theora.td,&m_Theora.tc,&m_Theora.op);
			if(ret<0)
			{
				Error_static("CMVideoEncoder_Theora::InitEncode::ogg_stream_pageout", "Internal Ogg library error.\n");
			}
			else if (!ret)
				break;
			ogg_stream_packetin(&m_Theora.to,&m_Theora.op);
		}


		// Flush the rest of our headers. This ensures
		// the actual data in each stream will start
		// on a new page, as per spec. 
		while(1)
		{
			int result = ogg_stream_flush(&m_Theora.to,&m_Theora.og);
			if(result<0)
			{
				/* can't get here */
				Error_static("CMVideoEncoder_TheoraInitEncode::ogg_stream_flush", "Internal Ogg library error.\n");
			}
			if(result==0)
				break;
			m_pFile->Write(m_Theora.og.header, m_Theora.og.header_len);
			m_pFile->Write(m_Theora.og.body, m_Theora.og.body_len);
		}
	}

	void FinishEncode()
	{
		ogg_stream_clear(&m_Theora.to);
		theora_encode_free(m_Theora.td);
//		theora_comment_clear(&m_Theora.tc);
	}

	ogg_page m_TempVideoPage;

	TArray<uint8> m_YuvFrame;

	void AddFrame(CImage &_Image, bool _bLast)
	{
		/* You'll go to Hell for using static variables */
		ogg_packet          op;

		{
			/* Theora is a one-frame-in,one-frame-out system; submit a frame
			for compression and pull out the packet */
			theora_ycbcr_buffer yuv;

//			yuv_buffer          yuv;
			{
				yuv[0].width=m_VideoX;
				yuv[0].height=m_VideoY;
				yuv[0].ystride=m_VideoX;

				yuv[1].width = yuv[2].width = m_VideoX/2;
				yuv[1].height = yuv[2].height = m_VideoY/2;
				yuv[1].ystride = yuv[2].ystride = m_VideoX/2;

				yuv[0].data= (unsigned char *)m_YuvFrame.GetBasePtr();
				yuv[1].data= (unsigned char *)m_YuvFrame.GetBasePtr() + m_VideoX*m_VideoY;
				yuv[2].data= yuv[1].data + ((m_VideoX*m_VideoY)>>2);

				CPixel32 *pImage = (CPixel32 *)_Image.Lock();

				int Modulu = _Image.GetModulo()/4;
				int PixelSize = _Image.GetPixelSize();
				int ModulY = m_VideoX;
				int ModulUV = m_VideoX/2;

				// Convert Y
				{
					CPixel32 *pRow = pImage;
					uint8 *pRowY = (uint8 *)yuv[0].data;
					for (int y = 0; y < m_FrameY; ++y)
					{
						CPixel32 *pCur = pRow + y * Modulu;
						uint8 *pCurY = pRowY + y * ModulY;
						for (int x = 0; x < m_FrameX; ++x)
						{
							int Raw = (0.257f * pCur[x].R()) + (0.504f * pCur[x].G()) + (0.098f * pCur[x].B()) + 16;
							pCurY[x] = Min(Max(Raw, 16), 255);
						}
					}
				}

				// Convert U and V
				{
					CPixel32 *pRow = pImage;
					uint8 *pRowU = (uint8 *)yuv[1].data;
					uint8 *pRowV = (uint8 *)yuv[2].data;
					for (int y = 0; y < m_FrameY/2; ++y)
					{
						CPixel32 *pCur0 = pRow + y*2 * Modulu;
						CPixel32 *pCur1 = pRow + (y*2+1) * Modulu;
						uint8 *pCurU = pRowU + y * ModulUV;
						uint8 *pCurV = pRowV + y * ModulUV;
						for (int x = 0; x < m_FrameX / 2; ++x)
						{
							CPixel32 Average;
							Average.R() = ((int)pCur0[x*2].R() + (int)pCur0[x*2+1].R() + (int)pCur1[x*2].R() + (int)pCur1[x*2+1].R()) / 4;
							Average.G() = ((int)pCur0[x*2].G() + (int)pCur0[x*2+1].G() + (int)pCur1[x*2].G() + (int)pCur1[x*2+1].G()) / 4;
							Average.B() = ((int)pCur0[x*2].B() + (int)pCur0[x*2+1].B() + (int)pCur1[x*2].B() + (int)pCur1[x*2+1].B()) / 4;
							Average.A() = ((int)pCur0[x*2].A() + (int)pCur0[x*2+1].A() + (int)pCur1[x*2].A() + (int)pCur1[x*2+1].A()) / 4;

							int Raw = (0.439 * Average.R()) - (0.368f * Average.G()) - (0.071f * Average.B()) + 128;
							pCurV[x] = Min(Max(Raw, 0), 255);
							Raw = -(0.148 * Average.R()) - (0.291f * Average.G()) + (0.439f * Average.B()) + 128;
							pCurU[x] = Min(Max(Raw, 0), 255);
						}
					}
				}

				// Convert back to image
				{
					CPixel32 *pRow = pImage;
					uint8 *pRowU = (uint8 *)yuv[1].data;
					uint8 *pRowV = (uint8 *)yuv[2].data;
					uint8 *pRowY = (uint8 *)yuv[0].data;
					for (int y = 0; y < m_FrameY; ++y)
					{
						CPixel32 *pCur = pRow + y * Modulu;
						uint8 *pCurY = pRowY + y * ModulY;
						uint8 *pCurU = pRowU + (y >> 1) * ModulUV;
						uint8 *pCurV = pRowV + (y >> 1) * ModulUV;
						for (int x = 0; x < m_FrameX; ++x)
						{
							int Y = pCurY[x];
							int U = pCurU[x>>1];
							int V = pCurV[x>>1];
							int Raw = 1.164f * (Y - 16) + 2.018f * (U - 128);
							pCur[x].B() = Min(Max(Raw, 0), 255);
							Raw = 1.164f * (Y - 16) - 0.813f * (V - 128) - 0.391f * (U - 128);
							pCur[x].G() = Min(Max(Raw, 0), 255);
							Raw = 1.164f * (Y - 16) + 1.596f * (V - 128);
							pCur[x].R() = Min(Max(Raw, 0), 255);
						}
					}
				}

				_Image.Unlock();
			}

			theora_encode_ycbcr_in(m_Theora.td,yuv);

			/* if there's only one frame, it's the last in the stream */
			while(theora_encode_packetout(m_Theora.td,_bLast != 0,&op))
			{
				ogg_stream_packetin(&m_Theora.to,&op);
			}
		}

		while (ogg_stream_pageout(&m_Theora.to,&m_TempVideoPage) > 0) 
		{
			double videotime = theora_granule_time(m_Theora.td,ogg_page_granulepos(&m_TempVideoPage));

			/* flush a video page */
			m_pFile->Write(m_TempVideoPage.header, m_TempVideoPage.header_len);
			m_pFile->Write(m_TempVideoPage.body, m_TempVideoPage.body_len);

			{
				int hundredths=videotime*100-(long)videotime*100;
				int seconds=(long)videotime%60;
				int minutes=((long)videotime/60)%60;
				int hours=(long)videotime/3600;

				LogFile(CStrF("%d:%02d:%02d.%02d video: %f kbps",	hours,minutes,seconds,hundredths,m_pFile->Pos()*8./videotime*.001));
			}
			memset(&m_TempVideoPage, 0, sizeof(m_TempVideoPage));
		}
	}

	void Compile(class CRegistry *_pReg)
	{

	}
};

MRTC_IMPLEMENT_DYNAMIC(CMVideoEncoder_Theora, CMVideoEncoder);

#if 0

void CXVCCompile::Compile(class CRegistry *_pReg)
{


}
#endif