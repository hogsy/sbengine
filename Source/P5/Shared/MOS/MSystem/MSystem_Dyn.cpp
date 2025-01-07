
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Codecs
|__________________________________________________________________________________________________
\*************************************************************************************************/

#include "MRTC.h"

class CRegisterMSystem
{
public:
	CRegisterMSystem()
	{
		MRTC_REFERENCE(CScriptLanguageCore);
		
#ifndef IMAGE_IO_NOVORBIS
		MRTC_REFERENCE(CMSound_Codec_VORB);		
#endif
#if defined(PLATFORM_WIN_PC) || defined(PLATFORM_DOLPHIN)
//		MRTC_REFERENCE(CMSound_Codec_Cube);		
#endif
#if defined(PLATFORM_WIN_PC)
//		MRTC_REFERENCE(CMSound_Codec_MPEG);		
//		MRTC_REFERENCE(CMSound_Codec_SPU2);
//		MRTC_REFERENCE(CMSound_Codec_XACM);
//		MRTC_REFERENCE(CMSound_CodecXDF_XACM);		
		MRTC_REFERENCE(CMSound_CodecXDF_XMA);
		MRTC_REFERENCE(CMSound_CodecXDF_PS3);
		MRTC_REFERENCE(CMSound_Codec_PS3);
		MRTC_REFERENCE(CMSound_Codec_XMA);
#endif
#if defined(PLATFORM_XBOX1)
//		MRTC_REFERENCE(CMSound_Codec_XACM);
#elif defined(PLATFORM_XENON)
		MRTC_REFERENCE(CMSound_Codec_XMA);
#elif defined(PLATFORM_PS3)
		MRTC_REFERENCE(CMSound_Codec_PS3);
#endif
		MRTC_REFERENCE(CRegistry);
//		MRTC_REFERENCE(CRegistry_Const);
		MRTC_REFERENCE(CRegistryCompiled);
		MRTC_REFERENCE(CRegistry_Dynamic);


		MRTC_REFERENCE(CSCC_Codec);

		#ifdef PLATFORM_WIN_PC
			MRTC_REFERENCE(CDI_Device_Keyboard);
			MRTC_REFERENCE(CDI_Device_Mouse);
			MRTC_REFERENCE(CDI_Device_MousePad);
			MRTC_REFERENCE(CDI_Device_Joystick);
			MRTC_REFERENCE(CInputContext_DInput);
//			MRTC_REFERENCE(CSoundContext_DSound);
			MRTC_REFERENCE(CSoundContext_DSound2);
			MRTC_REFERENCE(CSoundContext_ASIO);
//			MRTC_REFERENCE(CSoundContext_GCXDF);
//			MRTC_REFERENCE(CSoundContext_PS2XDF);
//			MRTC_REFERENCE(CSoundContext_Xbox); // For xdf
//			MRTC_REFERENCE(CSoundContext_Xenon); // For xdf
			MRTC_REFERENCE(CSoundContext_Xenon3); // For xdf

/*
			MRTC_REFERENCE(CSoundContext_OpenAL);
*/

		#endif

#ifdef PLATFORM_XENON
			MRTC_REFERENCE(CXenon_Device_Joystick);
			MRTC_REFERENCE(CXenon_Device_KeyBoard);
			MRTC_REFERENCE(CInputContext_Xenon);
//			MRTC_REFERENCE(CSoundContext_Xenon);
			MRTC_REFERENCE(CSoundContext_Xenon3);
#endif
#ifdef PLATFORM_PS3
			MRTC_REFERENCE(CPS3_Device_Joystick);
			MRTC_REFERENCE(CPS3_Device_KeyBoard);
			MRTC_REFERENCE(CInputContext_PS3);
			MRTC_REFERENCE(CSoundContext_PS3);
#endif

		#ifdef PLATFORM_XBOX
			#ifdef PLATFORM_XBOX1
				MRTC_REFERENCE(CInputContext_XTL);
				MRTC_REFERENCE(CSoundContext_Xbox);
				MRTC_REFERENCE(CXTL_Device_Joystick);
				#ifdef M_Profile
					MRTC_REFERENCE(CXTL_Device_DebugKeyboard);
				#endif
			#endif
		#endif

		#ifdef PLATFORM_PS2
			MRTC_REFERENCE(CIPS2_Device_Keyboard);
			MRTC_REFERENCE(CIPS2_Device_Mouse);
			MRTC_REFERENCE(CIPS2_Device_Joystick);
			MRTC_REFERENCE(CSoundContext_PS2)
			MRTC_REFERENCE(CInputContext_PS2);
		#endif

		#ifdef PLATFORM_DOLPHIN
			MRTC_REFERENCE(CSystemDolphin);
			MRTC_REFERENCE(CRenderContextDolphin);
			MRTC_REFERENCE(CDisplayContextDolphin);
			MRTC_REFERENCE(CID_Device_Joystick);
			MRTC_REFERENCE(CInputContext_Dolphin);
		#endif

		#ifdef PLATFORM_XENON
			MRTC_REFERENCE(CSystemXenon);
		#elif defined PLATFORM_WIN
			MRTC_REFERENCE(CSystemWin32);
		#elif defined PLATFORM_PS2
			MRTC_REFERENCE(CSystemPS2);
		#elif defined PLATFORM_PS3
			MRTC_REFERENCE(CSystemPS3);
			MRTC_REFERENCE(CDisplayContextNULL);
		#endif
		MRTC_REFERENCE(CConsole);
		MRTC_REFERENCE(CPerfGraph);
		MRTC_REFERENCE(CTextureContext);
		MRTC_REFERENCE(CTexture);
		MRTC_REFERENCE(CTextureContainer_Plain);
		MRTC_REFERENCE(CTextureContainer_VirtualXTC);
		MRTC_REFERENCE(CTextureContainer_Video);
		MRTC_REFERENCE(CTextureContainer_VirtualXTC2);

		#ifndef PLATFORM_CONSOLE
		MRTC_REFERENCE(CTC_PostFilter_ScaleDown);
		#endif // PLATFORM_CONSOLE

		#if (defined(PLATFORM_WIN_PC) || defined(PLATFORM_XENON)/* && defined(M_RTM)*/)
//			MRTC_REFERENCE(CTextureContainer_Video_Bink);
		#	if defined PLATFORM_XENON
				MRTC_REFERENCE(CTextureContainer_Video_WMV);
		#	endif
			MRTC_REFERENCE(CTextureContainer_Video_Theora);
		#elif defined(PLATFORM_XBOX1)
			MRTC_REFERENCE(CTextureContainer_Video_XMV);
		#elif defined(PLATFORM_PS2)
			MRTC_REFERENCE(CTextureContainer_Video_Mpeg);
		#elif defined(PLATFORM_PS3)
			MRTC_REFERENCE(CTextureContainer_Video_PS3);
		#endif

		MRTC_REFERENCE(CScriptContext);
	}
};

CRegisterMSystem g_MSystemDyn;

#ifdef PLATFORM_WIN_PC
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "wsock32.lib")
#elif defined(PLATFORM_XENON)
#ifdef _DEBUG
#pragma comment(lib, "xnetd.lib")
#pragma comment(lib, "Xaudiod.lib")
#pragma comment(lib, "XMP.lib")
#pragma comment(lib, "x3daudiod.lib")
#else
#pragma comment(lib, "xnet.lib")
#pragma comment(lib, "Xaudio.lib")
#pragma comment(lib, "XMP.lib")
#pragma comment(lib, "x3daudio.lib")
#endif
#endif

/*
#pragma comment(lib, "../../../SDK/OpenAL/Lib/Win32/OpenAL32.lib")
*/

