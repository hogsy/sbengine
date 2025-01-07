/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File: WMV video player
	Author: Anders Ekermo
	Copyright: Copyright Starbreeze Studios AB 2006
	Contents: <description>
	Comments: Structure copied from Theora player
	History:
	20060705: anek, created file
\*____________________________________________________________________*/

#ifndef __TEXTURECONTAINERWMV_H
#define __TEXTURECONTAINERWMV_H

#if defined(PLATFORM_XENON)

#include "MTextureContainers.h"

#include <xmedia.h>

enum
{
	WMVNumFrames = 3
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:	WMV Texture class
\*____________________________________________________________________*/
class CTC_WMVTexture : public CReferenceCount
{
public:
	int32 m_LastFrame;
	int m_bOnLastFrame;

	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Class: WMV Video decoder
	\*____________________________________________________________________*/
	class CWMV : public MRTC_Thread
	{
	public:

		CCFile m_File;

		bool m_bIsOpen;
		bool m_bEOF;
		int m_iCurrentFrame;

		fp64 m_Time;
		fp64 m_LastAddedTime;

		TArray<uint8>	m_lU;
		TArray<uint8>	m_lV;

		IXMediaXmvPlayer * m_pPlayer;
		XMEDIA_VIDEO_DESCRIPTOR m_VideoDesc;

		/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
			Class: WMV Video frame
		\*____________________________________________________________________*/
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
			int32 m_bFrameShown;
		};

		MRTC_Event m_WorkEvent;
		MRTC_Event m_WorkDoneEvent;
		MRTC_CriticalSection m_Lock;
		CFrame *m_pFreeFrames[WMVNumFrames];
		CFrame *m_pFinishedFrames[WMVNumFrames];
		int m_iFinishedProduce;
		int m_iFinishedConsume;
		fp64 m_FrameTime;
		fp64 m_OriginalFrameTime;
		fp64 m_SoundOffset;
		uint32 m_nFrames;

		CTC_WMVTexture *m_pTexture;

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
		bool Init(CSoundContext *_pSC,D3DDevice * _pDev);
		void Cleanup();

		CWMV();
		~CWMV();
	};
	CWMV *m_pDecoder;
	CWMV::CFrame*	m_pFrame;

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
	bool m_bClosed;
	int32 m_TextureWidth;
	int32 m_TextureHeight;
	fp64 m_FrameTime;

	MRTC_CriticalSection m_TextureLock;

	//
	CTC_WMVTexture();
	~CTC_WMVTexture();

	void Create(CStr _FileName);

	void SaveState(); // NOT IMPLEMENTED
	void RestoreState(); // NOT IMPLEMENTED

	bool IsOpen();
	void Open(D3DDevice * _pDev);
	void Close();
};

typedef TPtr<CTC_WMVTexture> spCTC_WMVTexture;

//
//
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class: WMV Texture container
\*____________________________________________________________________*/
class CTextureContainer_Video_WMV : public CTextureContainer_Video
{
	MRTC_DECLARE;

protected:
	TArray<spCTC_WMVTexture> m_lspVideos;
	fp32 m_CloseTimeOut;

	int m_iSoundChannel;
	void ValidateLocalID(int _iLocal);

	D3DDevice * m_pDevice;
	
public:
	CTextureContainer_Video_WMV();
	~CTextureContainer_Video_WMV();
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

	D3DDevice * GetDevice();
	void ReturnDevice();

	void OpenVideo(CTC_WMVTexture * _pVideo,bool _bAffectRef = true);
	void CloseVideo(CTC_WMVTexture * _pVideo,bool _bAffectRef = true);
};


#endif	//Platform guard
#endif	//Inclusion guard
