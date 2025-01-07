#ifndef _INC_MRENDERUTIL
#define _INC_MRENDERUTIL

#include "../../MOS.h"
#include "../../MSystem/Raster/MTextureContainers.h"

class CXR_VBManager;
class CXR_VertexBuffer;

// -------------------------------------------------------------------
//  CRC_ConsoleViewport
// -------------------------------------------------------------------
class SYSTEMDLLEXPORT CRC_ConsoleViewport : public CRC_Viewport, public CConsoleClient
{
	bool m_bRegister;
	fp32 m_TargetFOV;
	CMTime m_LastRefresh;

public:

	DECLARE_OPERATOR_NEW

	CRC_ConsoleViewport();
	~CRC_ConsoleViewport();
	virtual void Create(bool _bRegister = false);

	virtual void Refresh();

	virtual void ReadSettings(CRegistry* _pReg);
	virtual void WriteSettings(CRegistry* _pReg);

	virtual void Con_SetFOV(fp32);
	virtual void Con_Zoom(fp32);
	virtual void Con_SetAspectRatio(fp32);
	virtual void Con_SetFrontPlane(fp32);
	virtual void Con_SetBackPlane(fp32);

	void Register(CScriptRegisterContext &_RegContext);
};

typedef TPtr<CRC_ConsoleViewport> spCRC_ConsoleViewport;

// -------------------------------------------------------------------
//  CRC_Font, RenderContext-font
// -------------------------------------------------------------------
#define CRC_CHARDESC_VERSION	0x0200

class M_ALIGN(8) CRC_FontChar
{
public:
	CVec2Dfp32 m_TVec0;
	CVec2Dfp32 m_TVec1;
	CVec2Dfp32 m_Dimensions;
	int16 m_xOfs;
	int16 m_yOfs;
	int16 m_iLocal;
	int16 m_Spacing;
	wchar m_Char;

	CRC_FontChar();
	void Read_v0000(CCFile* _pFile);
	void Read_v0200(CCFile* _pFile);
	void Write_v0200(CCFile* _pFile);
};

class SYSTEMDLLEXPORT CRC_Font : public CReferenceCount
{
	class CRC_CharHashElem
	{
	public:
		wchar m_Char;

		CRC_CharHashElem()
		{
			m_Char = 0;
		}
	};

	class CRC_CharHash : public THash<int16, CRC_CharHashElem>
	{
	public:
		void Create(int _nChars);
		void Insert(int _Index, wchar _Char);
		int GetIndex(wchar _Char);
	};


	TArray<CRC_FontChar> m_lCharDesc;
	CRC_CharHash m_CharDescHash;

	fp32 m_OriginalSize;
	fp32 m_OriginalSizeRcp;
	CVec2Dfp32 m_TexturePixelUV;
	CStr m_TextureName;
	int m_yOfs;

public:
	spCTextureContainer_Plain m_spTC;
	int m_TextureID;


	DECLARE_OPERATOR_NEW

	CRC_Font();
	~CRC_Font();
	
	void BuildCharDescHash();
	CRC_FontChar& GetCharDesc(wchar _Char);		// Returns '?' on missing chars

	virtual fp32 GetOriginalSize();

	virtual fp32 GetWidth(fp32 _Size, const char* _pStr);
	virtual fp32 GetWidthOfCL(int CharLength, const char* _pStr);
	virtual fp32 GetHeight(fp32 _Size, const char* _pStr);
	virtual int GetFit(fp32 _Size, const char* _pStr, int _Width, bool _bWordWrap);

	virtual fp32 GetWidth(fp32 _Size, const wchar* _pStr);
	virtual fp32 GetWidthOfCL(int CharLength, const wchar* _pStr);
	virtual fp32 GetHeight(fp32 _Size, const wchar* _pStr);
	virtual int GetFit(fp32 _Size, const wchar* _pStr, int _Width, bool _bWordWrap);

	virtual fp32 GetWidth(fp32 _Size, CStr _Str);
	virtual fp32 GetHeight(fp32 _Size, CStr _Str);
	virtual fp32 GetWidthOfCL(int CharLength, CStr _pStr);
	virtual int GetFit(fp32 _Size, CStr _Str, int _Width, bool _bWordWrap);

	static int IsControlCode(const char *_pStr, int _iPos);
	static int IsControlCode(const wchar *_pStr, int _iPos);
	static int IsControlCode(CStr _Str, int _iPos);
	static int GetControlCodes(const char *_pStr, char *_pRes, int _ResLen);
	static int GetControlCodes(const wchar *_pStr, wchar *_pRes, int _ResLen);

	// IO
	virtual void ReadFromFile(CStr _Filename);
	virtual void ReadFromXFC(CDataFile* _pFile);
#ifndef PLATFORM_CONSOLE
	virtual void ReadFromScript(CStr _Filename);
#endif
#ifdef PLATFORM_WIN_PC
	virtual void ReadFromScriptWin32(CStr _Filename);
#endif

#ifndef PLATFORM_CONSOLE
	virtual void WriteXFC(CDataFile* _pFile);
#endif

	// Rendering
	virtual int Write(int _MaxV, CVec3Dfp32* _pV, CVec2Dfp32* _pTV, CPixel32* _pCol, uint16* _piPrim, 
		CPixel32 _Color, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, const wchar* _pStr, 
		const CVec2Dfp32& _Size, const CVec2Dfp32& _MinLimit = 0, const CVec2Dfp32& _MaxLimit = 10000);
	virtual int Write(int _MaxV, CVec3Dfp32* _pV, CVec2Dfp32* _pTV, CPixel32* _pCol, uint16* _piPrim, 
		CPixel32 _Color, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, const char* _pStr, 
		const CVec2Dfp32& _Size, const CVec2Dfp32& _MinLimit = 0, const CVec2Dfp32& _MaxLimit = 10000);

	// wchar, calls Write()
	virtual void Write(CRenderContext* _pRC, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, const wchar* _pStr, 
		const CVec2Dfp32& _Size, CPixel32 _Color, const CVec2Dfp32& _MinLimit = 0, const CVec2Dfp32& _MaxLimit = 10000);
	virtual bool Write(CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, const wchar* _pStr, 
		const CVec2Dfp32& _Size, CPixel32 _Color, const CVec2Dfp32& _MinLimit = 0, const CVec2Dfp32& _MaxLimit = 10000);

	// char, convertes to UNICODE and calls wchar version.
	virtual void Write(CRenderContext* _pRC, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, const char* _pStr, 
		const CVec2Dfp32& _Size, CPixel32 _Color, const CVec2Dfp32& _MinLimit = 0, const CVec2Dfp32& _MaxLimit = 10000);
	virtual bool Write(CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, const char* _pStr, 
		const CVec2Dfp32& _Size, CPixel32 _Color, const CVec2Dfp32& _MinLimit = 0, const CVec2Dfp32& _MaxLimit = 10000);

	// CStr, calls either char or wchar version depending on format.
	virtual void Write(CRenderContext* _pRC, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, const CStr& _Str, 
		const CVec2Dfp32& _Size, CPixel32 _Color, const CVec2Dfp32& _MinLimit = 0, const CVec2Dfp32& _MaxLimit = 10000);
	virtual bool Write(CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, const CStr& _Str, 
		const CVec2Dfp32& _Size, CPixel32 _Color, const CVec2Dfp32& _MinLimit = 0, const CVec2Dfp32& _MaxLimit = 10000);
};

typedef TPtr<CRC_Font> spCRC_Font;

#endif // _INC_MRENDERUTIL
