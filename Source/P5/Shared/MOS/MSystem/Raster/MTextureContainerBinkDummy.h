/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Dummy bink texture container, doesn't actually do anything.
					
	Author:			Jim Kjellin
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		A_list_of_classes_functions_etc_defined_in_file
					
	Comments:		Longer_description_not_mandatory
					
	History:		
		030506:		Added comments
\*_____________________________________________________________________________________________*/

#ifndef __INC_MTEXTURECONTAINERBINKDUMMY
#define __INC_MTEXTURECONTAINERBINKDUMMY

#include "MTextureContainers.h"

#if defined(PLATFORM_PS2) || defined(PLATFORM_DOLPHIN)

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Class used internally by CTextureContainer_Video_Bink						
\*____________________________________________________________________*/
class CTC_BinkTexture : public CReferenceCount
{
public:
	CStr m_FileName;
	CStr m_TextureName;
	uint32 m_hBink;
	int m_Width;
	int m_Height;
	int m_TextureID;
	fp32 m_volume;
	fp32 m_Mainvolume;
	CMTime m_TimeLastVisible;
	int m_LastFrame;
	int m_Frames;
	bool m_bValid;
	bool m_bAutoRestart;
	bool m_bPaused;
	bool m_bDoneFrame;
	bool m_bOnLastFrame;

	CTC_BinkTexture();
	~CTC_BinkTexture();

	void Create(CStr _FileName);

	void SaveState();
	void RestoreState();

	bool IsOpen();
	void Open();
	void Close();
	void MakeValid();
};

typedef TPtr<CTC_BinkTexture> spCTC_BinkTexture;

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Texture container for multiple bink files.
\*____________________________________________________________________*/
class SYSTEMDLLEXPORT CTextureContainer_Video_Bink : public CTextureContainer_Video
{
	MRTC_DECLARE;

protected:
	TArray<spCTC_BinkTexture> m_lspVideos;
	fp32 m_CloseTimeOut;
	
public:
	CTextureContainer_Video_Bink();
	~CTextureContainer_Video_Bink();
	virtual void Create(void* _pContext);
	virtual int AddVideo(CStr _FileName);
	virtual void CreateFromDirectory(CStr _Path);
	void ValidateLocalID(int _iLocal);
	
	virtual void CloseVideo(int _iLocal);
	virtual void SetVolume(int _iLocal, fp32 fpVol);
	virtual void Pause(int _iLocal, bool _Paused = true);
	virtual void AutoRestart(int _iLocal, bool _EnableAutoRestart = true);
	virtual void Rewind(int _iLocal);
	virtual bool IsOnLastFrame(int _iLocal);
	virtual bool MoveToLastFrame(int _iLocal);
	virtual int GetFrame(int _iLocal);
	virtual int GetNumFrames(int _iLocal);

	virtual int GetNumLocal();
	virtual int GetLocal(const char* _pName);
	virtual int GetTextureID(int _iLocal);
	virtual int GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps);
	virtual void GetTextureProperties(int _iLocal, CTC_TextureProperties& _Properties);
	virtual void OnRefresh();
	virtual CStr GetName(int _iLocal);
	virtual void BuildInto(int _iLocal, CImage** _ppImg, int _nMipmaps, int _TextureVersion, int _ConvertType = IMAGE_CONVERT_RGB, int _iStartMip = 0, uint32 _BuildFlags = 0);

	virtual int GetWidth(int _iLocal);
	virtual int GetHeight(int _iLocal);
};

#endif
#endif // __INC_MTEXTURECONTAINERBINKDUMMY