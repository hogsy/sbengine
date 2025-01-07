#ifndef __MTEXTURECONTAINERVIDEOPS3_H_INCLUDED
#define __MTEXTURECONTAINERVIDEOPS3_H_INCLUDED

#ifdef PLATFORM_PS3

#include "MTextureContainers.h"

class CTextureContainer_Video_PS3 : public CTextureContainer_Video
{
	MRTC_DECLARE;

public:
	class CTC_Texture : public CReferenceCount
	{
		friend class CTextureContainer_Video_PS3;
	protected:
		CStr m_Filename;
		CStr m_TextureName;

		TPtr<class CDecoder> m_spDecoder;
		uint32 m_TextureID:16;
		uint32 m_Type:2;
		uint32 m_Padding:14;
		uint32 m_Width:16;
		uint32 m_Height:16;

	public:

		enum
		{
			VIDEO_TYPE_UNDEFINED = 0,
			VIDEO_TYPE_M2V,
			VIDEO_TYPE_AVC,
		};

		CTC_Texture();
		virtual ~CTC_Texture();

		void Create(CStr _Filename);
		void Open();
		void Close();

		bool IsOpen();
		void BuildInto(CImage** _ppImg, uint32 _BuildFlags);
		int GetFrame();
		bool IsOnLastFrame();
	};


	typedef TPtr<CTC_Texture> spCTC_Texture;

	TArray<spCTC_Texture> m_lspVideos;

	CTextureContainer_Video_PS3();
	~CTextureContainer_Video_PS3();

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

#endif	// __MTEXTURECONTAINERVIDEOPS3_H_INCLUDED
