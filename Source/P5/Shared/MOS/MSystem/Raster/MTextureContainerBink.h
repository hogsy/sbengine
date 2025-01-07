/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Texture container for BINK compressed movies
					
	Author:			Jim Kjellin
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		A_list_of_classes_functions_etc_defined_in_file
					
	Comments:		Longer_description_not_mandatory
					
	History:		
		030506:		Added comments
\*_____________________________________________________________________________________________*/

#ifndef __INC_MTEXTURECONTAINERBINK
#define __INC_MTEXTURECONTAINERBINK

#include "MTextureContainers.h"

#if defined(PLATFORM_DOLPHIN)

#ifdef PLATFORM_XBOX
	#include "../../../SDK/BinkSDK_Xbox/Radbase.h"
	#include "../../../SDK/BinkSDK_Xbox/Rad.h"
	#include "../../../SDK/BinkSDK_Xbox/bink.h"
#elif defined(PLATFORM_DOLPHIN)
	#include "../../../SDK/BinkSDK_GameCube/BINK.H"
#else
	#include "../../../SDK/BinkSDK/bink.h"
#endif

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Class used internally by CTextureContainer_Video_Bink						
\*____________________________________________________________________*/
class CTC_BinkTexture : public CReferenceCount
{
public:
	CStr m_FileName;
	CStr m_TextureName;
	HBINK m_hBink;
	int16 m_Width;
	int16 m_Height;
	int16 m_RealWidth;
	int16 m_RealHeight;
	int m_TextureID;
	fp4 m_volume;
	fp4 m_Mainvolume;
//	fp8 m_TimeLastVisible;
	int32 m_TimeLastVisible; //frames
	int m_LastFrame;
	int m_Frames;
	bool m_bValid : 1;
	bool m_bAutoRestart : 1;
	bool m_bPaused : 1;
	bool m_bDoneFrame : 1;
	bool m_bOnLastFrame : 1;
	bool m_bGlobalPaused : 1;

	CTC_BinkTexture();
	~CTC_BinkTexture();

	void Create(CStr _FileName);

	void SaveState();
	void RestoreState();

	bool IsOpen();
	void Open();
	void Close();
	void MakeValid();
	void UpdateVolume();
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
	fp8 m_CloseTimeOut;
	
public:
	CTextureContainer_Video_Bink();
	~CTextureContainer_Video_Bink();
	virtual void Create(void* _pContext);
	virtual int AddVideo(CStr _FileName);
	virtual void CreateFromDirectory(CStr _Path);
	void ValidateLocalID(int _iLocal) const;
	
	virtual void CloseVideo(int _iLocal);
	virtual void SetVolume(int _iLocal, fp4 fpVol);
	virtual void Pause(int _iLocal, bool _Paused = true);
	virtual void AutoRestart(int _iLocal, bool _EnableAutoRestart = true);
	virtual void Rewind(int _iLocal);
	virtual bool IsOnLastFrame(int _iLocal);
	virtual bool MoveToLastFrame(int _iLocal);
	virtual int GetFrame(int _iLocal);
	virtual int GetNumFrames(int _iLocal);
	virtual void SetFrame(int _iLocal, int _Frame);
	virtual fp4 GetTime(int _iLocal);

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

	virtual void GlobalPause();
	virtual void GlobalResume();
};

#endif
#endif // __INC_MTEXTURECONTAINERBINK
