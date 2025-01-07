/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Texture container for XMV (MPEG4) compressed movies
					
	Author:			Magnus Auvinen
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		A_list_of_classes_functions_etc_defined_in_file
					
	Comments:		Longer_description_not_mandatory
					
	History:		
		030506:		Added comments
\*_____________________________________________________________________________________________*/
#ifndef __INC_MTEXTURECONTAINERXMV
#define __INC_MTEXTURECONTAINERXMV

#include "MTextureContainers.h"

#if defined(PLATFORM_XBOX1)


	#include <xtl.h>

#include <xmv.h>


//
//
//
class CTC_XMVTexture : public CReferenceCount
{
public:
	int32 m_LastFrame;
	bool m_bOnLastFrame:1;

	// Decoder and description
	XMVDecoder *m_pDecoder;
	XMVVIDEO_DESC m_VideoDesc;

	CCFile *m_pInFile;
	//TThinArray<uint8> m_Packets[2];
	void *m_pPackets[2];
	CAsyncRequest m_PendingRequest;
	int m_LoadingPacket;
	int m_PendingPacket;
	int m_DecodingPacket;
	int m_DecodingPacketSize;
	int m_MaxPacketSize;

	static HRESULT CALLBACK GetNextPacket(DWORD Context, void **ppPacket, DWORD *pOffsetToNextPacket);
	static HRESULT CALLBACK ReleasePreviousPacket(DWORD Context, LONGLONG NextReadByteOffset, DWORD NextPacketSize);

	// Filename
	CStr m_FileName;

	// Texture
	CStr m_TextureName;
	int32 m_TextureID;

	//
	CMTime m_TimeLastVisible;
	int32 m_CloseTimeoutDelay;
	bool m_bBroken;

	//
	CTC_XMVTexture();
	~CTC_XMVTexture();

	void Create(CStr _FileName);

	void SaveState(); // NOT IMPLEMENTED
	void RestoreState(); // NOT IMPLEMENTED

	bool IsOpen();
	void Open();
	void Close();
	void MakeValid();
};

typedef TPtr<CTC_XMVTexture> spCTC_XMVTexture;

//
//
//
class SYSTEMDLLEXPORT CTextureContainer_Video_XMV : public CTextureContainer_Video
{
	MRTC_DECLARE;

protected:
	TArray<spCTC_XMVTexture> m_lspVideos;
	fp4 m_CloseTimeOut;
	
public:
	CTextureContainer_Video_XMV();
	~CTextureContainer_Video_XMV();
	virtual void Create(void* _pContext);
	virtual int AddVideo(CStr _FileName);
	virtual void CreateFromDirectory(CStr _Path);
	void ValidateLocalID(int _iLocal);
	
	virtual void CloseVideo(int _iLocal);
	virtual void SetVolume(int _iLocal, fp4 fpVol); // NOT SUPPORTED
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

	virtual int GetWidth(int _iLocal);
	virtual int GetHeight(int _iLocal);

	virtual fp4 GetTime(int _iLocal);
};

#endif
#endif // __INC_MTEXTURECONTAINERXMV
