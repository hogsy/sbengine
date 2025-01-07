/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Texture container for XMV (MPEG4) compressed movies
					
	Author:			Magnus Auvinen
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		A_list_of_classes_functions_etc_defined_in_file
					
	Comments:		Longer_description_not_mandatory
					
	History:		
		030506:		Added comments
\*_____________________________________________________________________________________________*/
#ifndef DInc_MTextureContainerTheora_h
#define DInc_MTextureContainerTheora_h

#include "MTextureContainers.h"

#if defined(PLATFORM_WIN_PC) || defined(PLATFORM_XBOX)

#include "theora/theoradec.h"

enum
{
	ENumFrames = 3
};
//
//
//
class CTC_TheoraTexture : public CReferenceCount
{
public:
	int32 m_LastFrame;
	int m_bOnLastFrame;

	// Decoder and description
	class CTheora : public MRTC_Thread
	{
	public:
		ogg_sync_state		m_OggState; // oy
		ogg_page			m_OggPage; // og
		ogg_stream_state	m_OggStreamState; // to
		ogg_packet			m_OggPacket; // op
//		theora_state		m_TheoraState; // td
		theora_dec_ctx		*m_pTheoraDecodeContext;
		theora_comment		m_TheoraComment; // tc
		theora_info			m_TheoraInfo; // ti
		int					m_TheoraPage;
		int64				m_TheoraTime;

		CCFile m_File;

		bool m_bIsOpen;
		bool m_bEOF;
		int m_iCurrentFrame;

		fp64 m_Time;
		fp64 m_LastAddedTime;

		class CFrame
		{
		public:
			CFrame() : m_Time(0), m_iFrame(0), m_bFrameShown(0) 
			{
			}
			fp64 m_Time;
			int m_iFrame;
			CImage m_Y;
			CImage m_UV;
			int m_bFrameShown;
		};

		MRTC_Event m_WorkEvent;
		MRTC_Event m_WorkDoneEvent;
		MRTC_CriticalSection m_Lock;
		CFrame *m_pFreeFrames[ENumFrames];
		CFrame *m_pFinishedFrames[ENumFrames];
		int m_iFinishedProduce;
		int m_iFinishedConsume;
		fp64 m_FrameTime;
		fp64 m_OriginalFrameTime;
		fp64 m_SoundOffset;

		CTC_TheoraTexture *m_pTexture;

		CMTime GetSoundTime();

		// Sound


		class CSoundVoices
		{
		public:
			int m_iVoices[6];
			int m_iVoicesOwn[6];

			CSoundVoices()
			{
				for (int i = 0; i < 6; ++i)
				{
					m_iVoices[i] = -1;
					m_iVoicesOwn[i] = true;
				}
			}

			void Destroy(CSoundContext *_pSC)
			{
				for (int i = 0; i < 6; ++i)
				{
					if (m_iVoicesOwn[i] && m_iVoices[i] >= 0)
					{
						_pSC->Voice_Destroy(m_iVoices[i]);
					}
					m_iVoices[i] = 0;
					m_iVoicesOwn[i] = true;
				}               
			}
		};

		CSoundContext *m_pSC;

		CSoundVoices m_SoundVoices;
		bool m_bPausedSound;

		void PlaySound(int _iChannel);
		void SetSound(int _ihSound);
		void ResumeSound();
		void PauseSound();

		bool DoWork();
		const char* Thread_GetName() const;
		int Thread_Main();
		CFrame *GetFreeFrame();
		CMTime GetNextFrameTime();
		void AddFinisherFrame(CFrame *_pFrame);
		CFrame *GetFrame(bool _bBlock);
		void ReturnFrame(CFrame *_pFrame);
//		void ReturnFrame();
		int BufferData();
		void QueuePage();
		bool Init(CSoundContext *_pSC);
		void Cleanup();

		CTheora();
		~CTheora();
	};
	CTheora *m_pDecoder;
	CTheora::CFrame*	m_pFrame;

	// Filename
	CStr m_FileName;

	// Texture
	CStr m_TextureName[3];
	int32 m_TextureID[3];

	//
	CMTime m_TimeLastVisible;
	CMTime m_VideoTimeStart;
	bool m_bBroken;
	bool m_bWasRendered;
	int32 m_TextureWidth;
	int32 m_TextureHeight;
	fp64 m_FrameTime;

	MRTC_CriticalSection m_TextureLock;

	//
	CTC_TheoraTexture();
	~CTC_TheoraTexture();

	void Create(CStr _FileName);

	void SaveState(); // NOT IMPLEMENTED
	void RestoreState(); // NOT IMPLEMENTED

	bool IsOpen();
	void Open();
	void Close();
	void MakeValid();
};

typedef TPtr<CTC_TheoraTexture> spCTC_TheoraTexture;

//
//
//
class CTextureContainer_Video_Theora : public CTextureContainer_Video
{
	MRTC_DECLARE;

protected:
	TArray<spCTC_TheoraTexture> m_lspVideos;
	fp32 m_CloseTimeOut;

	int m_iSoundChannel;
	void ValidateLocalID(int _iLocal);
	
public:
	CTextureContainer_Video_Theora();
	~CTextureContainer_Video_Theora();
	virtual void Create(void* _pContext);
	virtual int AddVideo(CStr _FileName);
	virtual void CreateFromDirectory(CStr _Path);
	
	virtual void CloseVideo(int _iLocal);
	virtual void SetVolume(int _iLocal, fp32 fpVol); // NOT SUPPORTED
	virtual void Pause(int _iLocal, bool _Paused = true);  // NOT SUPPORTED
	virtual void AutoRestart(int _iLocal, bool _EnableAutoRestart = true); // NOT SUPPORTED
	virtual void Rewind(int _iLocal);
	virtual bool IsOnLastFrame(int _iLocal);
	virtual bool MoveToLastFrame(int _iLocal); // NOT SUPPORTED
	virtual int GetFrame(int _iLocal);
	virtual int GetNumFrames(int _iLocal); // NOT SUPPORTED

	virtual int GetNumLocal();
	virtual int GetLocal(const char* _pName);
	virtual int GetTextureID(int _iLocal);
	virtual int GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps);
	virtual void GetTextureProperties(int _iLocal, CTC_TextureProperties& _Properties);
	virtual void OnRefresh();
	virtual CStr GetName(int _iLocal);
	virtual void BuildInto(int _iLocal, CImage** _ppImg, int _nMipmaps, int _TextureVersion, int _ConvertType = IMAGE_CONVERT_RGB, int _iStartMip = 0, uint32 _BuildFlags = 0);
	virtual void SetSoundHandle(int _iLocal, int _hSound);

	virtual int GetWidth(int _iLocal);
	virtual int GetHeight(int _iLocal);

	virtual fp32 GetTime(int _iLocal);
};

#endif
#endif // DInc_MTextureContainerTheora_h
