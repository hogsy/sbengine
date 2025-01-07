#include "PCH.h"
#include "MRTC_Callgraph.h"
#include "../../Shared/MOS/MMain.h"
#include "../../Shared/MOS/Classes/Render/MRenderCapture.h"
#include "../../Shared/MOS/Classes/Win/MWinGrph.h"
#ifdef PLATFORM_XBOX1
#include "../../Shared/MOS/RndrXbox/MRndrXbox.h"
#endif
#include "../../Shared/MOS/XR/XREngineVar.h"
#include "../../Shared/MOS/XR/XRShader.h"
#include "../../Shared/MOS/XR/XRVBEContext.h"

#include "../../Shared/MOS/MSystem/MSystem_Core.h"

#include "../../Shared/MOS/Classes/GameWorld/WDataRes_Core.h"
#include "../../Shared/MOS/Classes/GameWorld/WDataRes_MiscMedia.h"

#include "../../Shared/MOS/Classes/GameWorld/Server/WServer.h"
#include "../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Game.h"
#include "../../Shared/MOS/Classes/GameWorld/FrontEnd/WFrontEnd.h"

#ifdef PLATFORM_XENON
	#include <pmcpb.h>
	#include <pmcpbsetup.h>
	#pragma comment(lib, "libpmcpb.lib") // Link in the PMC library.
#endif

//#include "../../SDK/MAC/MassiveClientCore.h"

#ifdef PLATFORM_WIN_PC
#include <windows.h>
#include "resource.h"
#endif

#ifdef USE_SN_TUNER
#include "SNTuner.h"
#endif

#include "WGameContextMain.h"
#include "WGameContext_P6.h"
#include "MRTC_CallGraph.h"

#include "MMath_SSE.h"
//#define Floor(x) (((x) < 0.0f) ? fp32(TruncToInt(x)-1) : fp32(TruncToInt(x)))
#ifdef M_Profile
#ifdef PLATFORM_XBOX1
extern int g_bSavedVertexPrograms;
#endif
#endif

#ifdef PLATFORM_CONSOLE
#define FAKEOVERBRIGHT 0
#else
#define FAKEOVERBRIGHT 1
#endif

//#define MiniFont "MINIFONT2"
#define MiniFont "MONOPRO"
//#define Floor(x) (((x) < 0.0f) ? fp32(RoundToInt(x-0.5f)) : fp32(RoundToInt(x-0.5f)))
//#define Floor(x) fp32(RoundToInt(x-0.5f))
//#define Ceil(x) fp32(RoundToInt(x+0.5f))

#if defined(PLATFORM_XENON) || defined(PLATFORM_PS3)
# define STARTUP_MULTITHREADSTATE true
#else
# define STARTUP_MULTITHREADSTATE false
#endif


#ifdef M_Profile
# define LOG_FRAME_INFO
#endif

#ifdef LOG_FRAME_INFO
# define LATENCY_LOG_FRAMES	100
#endif

#include "MMath_gfp2.h"

#ifdef PLATFORM_XENON
#include "tracerecording.h"
#pragma comment( lib, "tracerecording.lib" )
#endif


static fp32 s_DefaultTextureQuality = (1.0/3 * 2);
static int s_EXTRA_PICMIP_WORLDOBJECTS = 0;
static int s_EXTRA_PICMIP_WEAPONS = 0;
static int s_EXTRA_PICMIP_PROJMAPS = 0;


uint32 FloatTestClamp01()
{
	fp32 TestValues[] = {-1234.5678f, -0.5f, 0.5f, 1234.5678f};
	fp32 ResultValues[] = {0.0f, 0.0f, 0.5f, 1.0f};
	uint32 Broken = 0;
	for(int i = 0; i < 4; i++)
		if(Clamp01(TestValues[i]) != ResultValues[i]) Broken |= 1 << i;

	return Broken;
}

uint32 FloatTestClamp()
{
	fp32 TestValues[] = {-1234.5678f, -0.5f, 0.5f, 1234.5678f};
	fp32 ResultValues[] = {-1.0f, -0.5f, 0.5f, 1.5f};
	uint32 Broken = 0;
	for(int i = 0; i < 4; i++)
		if(Clamp(TestValues[i], -1.0f, 1.5f) != ResultValues[i]) Broken |= 1 << i;

	return Broken;
}

uint32 FloatTestTrunc()
{
	fp32 TestValues[] = {-0.5f, -1.5f, 0.5f, 1.5f};
	fp32 ResultValues[] = {0.0f, -1.0f, 0.0f, 1.0f};
	uint32 Broken = 0;
	for(int i = 0; i < 4; i++)
		if(TruncToInt(TestValues[i]) != ResultValues[i]) Broken |= 1 << i;

	return Broken;
}

uint32 FloatTestRound()
{
	fp32 TestValues[] = {-0.5f, -1.0f, -1.5f, -2.0f, 0.5f, 1.0f, 1.5f, 2.0f};
	int ResultValues[] = {-1, -1, -2, -2, 1, 1, 2, 2};
	uint32 Broken = 0;
	for(int i = 0; i < 8; i++)
		if(RoundToInt(TestValues[i]) != ResultValues[i]) Broken |= 1 << i;

	return Broken;
}

uint32 FloatTestFloor()
{
	fp32 TestValues[] = {-1.5f, -0.5f, -0.3f, 0.5f, 1.5f, 2.9f};
	fp32 ResultValues[] = {-2.0f, -1.0f, -1.0f, 0.0f, 1.0f, 2.0f};
	uint32 Broken = 0;
	for(int i = 0; i < 6; i++)
		if(Floor(TestValues[i]) != ResultValues[i]) Broken |= 1 << i;

	return Broken;
}

uint32 FloatTestCeil()
{
	fp32 TestValues[] = {-1.5f, -0.5f, -0.3f, 0.5f, 1.5f, 2.9f};
	fp32 ResultValues[] = {-1.0f, 0.0f, 0.0f, 1.0f, 2.0f, 3.0f};
	uint32 Broken = 0;
	for(int i = 0; i < 6; i++)
		if(Ceil(TestValues[i]) != ResultValues[i]) Broken |= 1 << i;

	return Broken;
}

uint32 FloatTestFraction()
{
	fp32 TestValues[] = {-2.0f, -1.5f, 0.5f, 1.0f, 1.5f};
	fp32 ResultValues[] = {0.0f, -0.5f, 0.5f, 0.0f, 0.5f};
	uint32 Broken = 0;
	for(int i = 0; i < 5; i++)
		if(Fraction(TestValues[i]) != ResultValues[i]) Broken |= 1 << i;

	return Broken;
}
void FloatConformanceTest()
{
	uint32	Clamp01Broken = FloatTestClamp01();
	uint32	ClampBroken = FloatTestClamp();
	uint32	TruncBroken = FloatTestTrunc();
	uint32	RoundBroken = FloatTestRound();
	uint32	FloorBroken = FloatTestFloor();
	uint32	CeilBroken = FloatTestCeil();
	uint32	FractionBroken = FloatTestFraction();

	if( Clamp01Broken )
		M_TRACE( CStrF( "!!!!!!!!MATHERROR: Clamp01 is broken on case 0x%.8X\r\n", Clamp01Broken ) );

	if( ClampBroken )
		M_TRACE( CStrF( "!!!!!!!!MATHERROR: Clamp is broken on case 0x%.8X\r\n", ClampBroken ) );

	if( TruncBroken )
		M_TRACE( CStrF( "!!!!!!!!MATHERROR: TruncToInt is broken on case 0x%.8X\r\n", TruncBroken ) );

	if( RoundBroken )
		M_TRACE( CStrF( "!!!!!!!!MATHERROR: RoundToInt is broken on case 0x%.8X\r\n", RoundBroken ) );

	if( FloorBroken )
		M_TRACE( CStrF( "!!!!!!!!MATHERROR: Floor is broken on case 0x%.8X\r\n", FloorBroken ) );

	if( CeilBroken )
		M_TRACE( CStrF( "!!!!!!!!MATHERROR: Ceil is broken on case 0x%.8X\r\n", CeilBroken ) );

	if( FractionBroken )
		M_TRACE( CStrF( "!!!!!!!!MATHERROR: Fraction is broken on case 0x%.8X\r\n", FractionBroken ) );


/*	gfp2 half;
	half = 1.0f;

	ConOutL(CStrF("1.0f = %d = %f", (int)half.m_Half, (fp32)half));

	fp2 half2;
	half2 = 1.0f;

	ConOutL(CStrF("1.0f = %d = %f", (int)half2.m_Half, (fp32)half2));

	half = 0.5f;
	ConOutL(CStrF("0.5f = %d = %f", (int)half.m_Half, (fp32)half));

	for(int i = 0; i < 20; i++)
	{
		fp32 Rand = (Random - 0.5f) * 100000.000001f;
		half = Rand;
		ConOutL(CStrF("%.20f = %d = %.20f", Rand, (int)half.m_Half, (fp32)half));
	}*/

/*	TObjectPool<uint32> Pool;
	Pool.Create(3);

	TObjectPoolAllocator<uint32> Allocator1(&Pool);
	uint32* pObj = Allocator1.GetObject();
	*pObj = 0;
	TObjectPoolAllocator<uint32> Allocator2(&Pool);
	pObj = Allocator2.GetObject();
	*pObj = 1;
	TObjectPoolAllocator<uint32> Allocator3(&Pool);
	pObj = Allocator3.GetObject();
	*pObj = 2;
	TObjectPoolAllocator<uint32> Allocator4(&Pool);
	pObj = Allocator4.GetObject();
	*pObj = 3;*/
}

// --------------------------------
//  CRC_ConsoleRender
// --------------------------------
#define CRC_CONSOLERENDER_INVISIBLE	0
#define CRC_CONSOLERENDER_OVERLAY	1
#define CRC_CONSOLERENDER_HALF		2
#define CRC_CONSOLERENDER_FULL		3

bool bDumpCallList = false;

class CRC_ConsoleRender : public CConsoleRender
{
public:
	CXR_VBManager* m_pVBM;
	CRC_Util2D m_Util2D;
	CRC_Font *m_pFont; 
	CXR_Shader m_Shader;

	CMTime m_TimeStart;

	int m_Mode;
	int m_LineSpacing;
	fp32 m_FontScale;
	fp32 m_FontSize;

	int m_ConsolePos;
	int m_ConsolePosPrim;
	int m_ConsoleTarget;

	fp32 m_TextTimeOut;
	fp32 m_TextFadeTime;

	CPnt m_EditRelPos;
	int m_bEditGlow;

public:
	CRC_ConsoleRender();
	void SetFont(CRC_Font * _pFont, fp32 _FontScale = 1.0f);
//	void ReadFont(CStr _FileName);
	void SetVBM(CXR_VBManager* _pVBM);
	void SetMode(int _Mode)	{ m_Mode = _Mode; };
	void PrepareFrame(CRenderContext* _pRC, CXR_VBManager* _pVBM);
	int GetMode() { return m_Mode; }
	void EditText(CClipRect cr, CRC_Font* pF, int x, int y, const char* _pText, fp32 _TimeOffs, fp32 _Size);
	virtual void Render(CConsole* pCon, CClipRect cr, CImage* img, CPnt pos);
	virtual void RenderRect(CConsole* pCon, CClipRect cr, CRct pos);
	virtual void RenderRectShaded(CConsole* pCon, CClipRect cr, CRct pos);
};

//----------------------------------------------------------------
CRC_ConsoleRender::CRC_ConsoleRender()
{
	m_pFont = NULL;
	m_Mode = CRC_CONSOLERENDER_OVERLAY;
	m_TextTimeOut = 3.5f;
	m_TextFadeTime = 0.5f;
	m_LineSpacing = 1;
	m_FontScale = 1;
	m_FontSize = 16;
	m_EditRelPos = CPnt(4,0);
	m_bEditGlow = true;

	m_TimeStart.Snapshot();

	m_ConsolePos = 0;
	m_ConsolePosPrim = 0;
	m_ConsoleTarget = 0;

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (pSys) m_FontScale = pSys->GetEnvironment()->GetValuef("CON_FONTSCALE", 0.25f, 0);
}

void CRC_ConsoleRender::SetFont(CRC_Font *_pFont, fp32 _FontScale)
{
	m_pFont = _pFont;
	m_FontScale = _FontScale;
	m_LineSpacing = (int)(fp32(m_pFont->GetOriginalSize()) * m_FontScale);
	m_FontSize = fp32(m_pFont->GetOriginalSize()) * m_FontScale;
}
/*
void CRC_ConsoleRender::ReadFont(CStr _FileName)
{
	m_spFont = MNew(CRC_Font);
	if (!m_spFont) MemError("ReadFont");

	m_spFont->ReadFromFile(_FileName);
	m_LineSpacing = fp32(m_spFont->GetOriginalSize()) * m_FontScale;
	m_FontSize = fp32(m_spFont->GetOriginalSize()) * m_FontScale;
}*/

void CRC_ConsoleRender::SetVBM(CXR_VBManager* _pVBM)
{
	m_pVBM	= _pVBM;
}

void CRC_ConsoleRender::Render(CConsole* pCon, CClipRect cr, CImage* img, CPnt pos)
{
	if (pCon == NULL) return;
	CClipRect imgrect = img->GetClipRect();
	cr += imgrect;

/*	TArray<CStr>* plOutput = pCon->GetOutput();

	int yp = pos.y;
	for (int y = 0; (y < plOutput->Len()); y++)
	{
	f		img->DebugText(cr, CPnt(pos.x, yp), (*plOutput)[y], 0xffffff);
	yp += 8;
	if (!cr.VisibleLine(yp)) break;
	};*/
};

void CRC_ConsoleRender::EditText(CClipRect cr, CRC_Font* pF, int x, int y, const char* _pText, fp32 _TimeOffs, fp32 _Size)
{
//	m_Util2D.Text(cr, pF, x+2, y+2, _pText, 0xff000000);
	m_Util2D.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
//	int Alpha = (M_Sin(_TimeOffs + CMTime::GetCPU().GetTimeModulusScaled(2.0f, _PI2))+1.0f) * 96.0f;
	CMTime TCurrent;
	TCurrent.Snapshot();
	int Alpha = (int)((M_Sin(_TimeOffs + TCurrent.GetTimeModulusScaled(2.0f, _PI2))+1.0f) * 96.0f);

	if (m_bEditGlow)
	{
		m_Util2D.Text(cr, pF, x-1, y-3, _pText, 0x00080810 + (Alpha << 24), _Size);
		m_Util2D.Text(cr, pF, x-1, y+3, _pText, 0x00080810 + (Alpha << 24), _Size);
		m_Util2D.Text(cr, pF, x+1, y-3, _pText, 0x00080810 + (Alpha << 24), _Size);
		m_Util2D.Text(cr, pF, x+1, y+3, _pText, 0x00080810 + (Alpha << 24), _Size);
		m_Util2D.Text(cr, pF, x-6, y, _pText, 0x00080810 + (Alpha << 24), _Size);
		m_Util2D.Text(cr, pF, x+6, y, _pText, 0x00080810 + (Alpha << 24), _Size);

		m_Util2D.Text(cr, pF, x-1, y-2, _pText, 0x00101018 + (Alpha << 24), _Size);
		m_Util2D.Text(cr, pF, x-1, y+2, _pText, 0x00101018 + (Alpha << 24), _Size);
		m_Util2D.Text(cr, pF, x+1, y-2, _pText, 0x00101018 + (Alpha << 24), _Size);
		m_Util2D.Text(cr, pF, x+1, y+2, _pText, 0x00101018 + (Alpha << 24), _Size);
		m_Util2D.Text(cr, pF, x-4, y, _pText, 0x00101018 + (Alpha << 24), _Size);
		m_Util2D.Text(cr, pF, x+4, y, _pText, 0x00101018 + (Alpha << 24), _Size);

		m_Util2D.Text(cr, pF, x, y-1, _pText, 0x00232338 + (Alpha << 24), _Size);
		m_Util2D.Text(cr, pF, x, y+1, _pText, 0x00232338 + (Alpha << 24), _Size);
		m_Util2D.Text(cr, pF, x-2, y, _pText, 0x00232338 + (Alpha << 24), _Size);
		m_Util2D.Text(cr, pF, x+2, y, _pText, 0x00232338 + (Alpha << 24), _Size);
	}
	m_Util2D.Text(cr, pF, x, y, _pText, 0xffffffff, _Size);
}


void CRC_ConsoleRender::RenderRect(CConsole* pCon, CClipRect cr, CRct pos)
{
	if (!m_pFont) return;
	if (pCon == NULL) return;
	int fnth = m_LineSpacing;

	CRC_Viewport* pVP = m_pVBM->Viewport_Get();
	CRct ViewRect = pVP->GetViewRect();

	bool bShowEdit = true;
	bool bTimeout = false;
	bool bShowBG = true;
	switch(m_Mode)
	{
	case CRC_CONSOLERENDER_INVISIBLE :
		return;
	case CRC_CONSOLERENDER_OVERLAY :
		{
			bTimeout = true;
			m_ConsoleTarget = fnth * 4;
//			pos.p1.y = pos.p0.y + fnth * 4;

			bShowEdit = false;
			bShowBG = false;

			// Move instantly
			m_ConsolePos = m_ConsoleTarget;
			m_ConsolePosPrim = 0;
		}
		break;
	case CRC_CONSOLERENDER_HALF :
		{
			m_ConsoleTarget = ViewRect.GetHeight() / 2;
		}
		break;
	case CRC_CONSOLERENDER_FULL :
	default :
		{
			m_ConsoleTarget = ViewRect.GetHeight();
			m_ConsolePos = m_ConsoleTarget;
			m_ConsolePosPrim = 0;
		}
		break;
	}
	Moderate(m_ConsolePos, m_ConsoleTarget, m_ConsolePosPrim, 256);
	pos.p1.y = m_ConsolePos;

//	CMTime Time = CMTime::GetCPU();
	CMTime Time;
	Time.Snapshot();

	m_Util2D.Begin(NULL, pVP, m_pVBM);
	m_Util2D.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
	m_Util2D.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

	CClipRect conrect(pos, pos.p0);
/*		CClipRect imgrect = img->GetClipRect();
	cr += imgrect;
	CClipRect conrect = CClipRect(pos.p0, pos.p1, pos.p0);
	conrect += cr;*/

	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("RenderRect", "No texture-context available.");

	int vish = (pos.p1.y - pos.p0.y);
	int h = fnth+4;
	if (bShowBG)
	{
		int x0(pos.p0.x);
		int y0(pos.p0.y);
		int x1(pos.p1.x);
		int y1(pos.p1.y);

		// Calculate where the ornament should be
		int scrh = ViewRect.GetHeight();
		int x2 = x0 + (x1-x0-256) / 2;
		int y2 = y1 - scrh + (scrh-256) / 2;

		int TxtID = 0;
		TxtID = pTC->GetTextureID("CONSOLE_BG");

		int CS = (TxtID) ? 1 : 0;
		m_Util2D.SetTexture(TxtID);

		// Render bg
		m_Util2D.SetTextureOrigo(conrect, CPnt(x2,y2));
		m_Util2D.Rect3D(conrect, CRct(x0,y0-1, x1, y1-h), 0xffb0b0b0*CS, 0xffffffff*CS, 0xff5f5f5f*CS);

		// Render edit bg
		m_Util2D.SetTexture(pTC->GetTextureID("CONSOLE_DBORDER"));
		m_Util2D.SetTextureOrigo(conrect, CPnt(x0,y1));
		m_Util2D.Rect3D(conrect, CRct(x0,y1-h, x1, y1), 0xffffffff*CS, 0xffffffff*CS, 0xff3f3f3f*CS);
		m_Util2D.Rect3D(conrect, CRct(x0+1,y1-h+1, x1-1, y1-1), 0xffffffff*CS, 0xffffffff*CS, 0xff7f7f7f*CS);

		// Render ornament
		m_Util2D.SetTexture(pTC->GetTextureID("CONSOLE_ORNAMENT"));
		m_Util2D.SetTextureOrigo(conrect, CPnt(x2,y2));
		m_Util2D.Rect(conrect, CRct(x2,y2,x2+256,y2+256), 0xffffffff*CS);

		// Render left border
		m_Util2D.SetTexture(pTC->GetTextureID("CONSOLE_LBORDER"));
		m_Util2D.SetTextureOrigo(cr, CPnt(x0, y1-h));
		m_Util2D.Rect(conrect, CRct(x0,y0, x0+16, y1-h-1), 0xffffffff*CS);

		// Render right border
		m_Util2D.SetTextureOrigo(cr, CPnt(x1-16, y1-h));
		m_Util2D.Rect(conrect, CRct(x1-16, y0, x1, y1-h-1), 0xffffffff*CS);
	}

	int yp = vish-fnth-4;
	if (bShowEdit)
	{
		CFStr cl = CFStrF("> %s", pCon->GetCommandLineEdit()->GetStr().Str());

		CPnt& ERP = m_EditRelPos;
		EditText(conrect, m_pFont, pos.p0.x+ERP.x, yp+ERP.y, cl, 0.0f, m_FontSize);
		EditText(conrect, m_pFont, (int)(pos.p0.x+ERP.x + m_Util2D.TextWidth(m_pFont, (char*)cl.Copy(0, pCon->GetCommandLineEdit()->GetCursorPos()+2)) * m_FontScale), yp+ERP.y, "_", _PI, m_FontSize);
		yp -= fnth+4;
	}

	CClipRect textclip;
	textclip = conrect;
	textclip.clip.p1.x -= 16;
	CConsoleString* pStr = pCon->GetOutputHead();
	while(pStr && yp > -fnth)
	{
		fp32 Age = (Time - pStr->m_Time).GetTime();
		if (!bTimeout || (Age < m_TextTimeOut))
		{
			int Intens = 255;
			if (bShowBG)
			{
//					if (yp < 255) Intens = Max(0, yp);
			}
			else
				if (bTimeout && (Age > m_TextTimeOut-m_TextFadeTime))
					Intens = (int)(255.0f*(m_TextTimeOut - Age) / m_TextFadeTime);

//				if (bShowBG) m_Util2D.Text(textclip, m_spFont, pos.p0.x+16+2, yp+2, pStr->m_Str, CPixel32(0, 0, 0, Intens), -1);

			m_Util2D.Text(textclip, m_pFont, pos.p0.x+16, yp, pStr->m_Str, CPixel32(255, 255, 255, Intens), m_FontSize);
		}
		yp -= fnth;
		pStr = pStr->m_pPrev;
	}

	if(bShowBG)
	{
		m_Util2D.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		m_Util2D.SetTexture(0);
		int yp = 0;
		for(;(yp < 256) && (yp < pos.p1.y-h); yp += 4)
		{
			int Intens = (int)(95.0f + 160.0f * (1.0f - (Sqr(1.0f - (fp32(yp) / 256.0f)))));
//				int Intens = (yp >> 1) + 127;
			m_Util2D.Rect(conrect, CRct(pos.p0.x, yp, pos.p1.x, yp+4), CPixel32(0, 0, 0,255-Intens));
		}
	}

	m_Util2D.End();
};

void CRC_ConsoleRender::PrepareFrame(CRenderContext* _pRC, CXR_VBManager* _pVBM)
{
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("RenderRect", "No texture-context available.");

	m_Shader.Create(pTC, _pVBM);
	m_Shader.PrepareFrame(_pRC, _pVBM);
}

void CRC_ConsoleRender::RenderRectShaded(CConsole* pCon, CClipRect cr, CRct pos)
{
	if (!m_pFont) return;
	if (pCon == NULL) return;
	int fnth = m_LineSpacing;

	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("RenderRect", "No texture-context available.");

	const int nLights = 2;
	CXR_Light lLights[nLights];

	lLights[0] = CXR_Light(CVec3Dfp32(0, -2000, 1800), CVec3Dfp32(0.1,1,2), 5600, 0, 0);
	lLights[0].m_iLight = 1;

	const fp32 WrapTime = 60.0f*10.0f;

	CMTime Time;
	Time.Snapshot();
	Time -= m_TimeStart;
	fp32 TimeWrapped = Time.GetTimeModulus(WrapTime);

	CVec3Dfp32 Angles;
	Angles = TimeWrapped * 0.1f;


	lLights[1] = CXR_Light(CVec3Dfp32(0, -1000, 1800), CVec3Dfp32(1,1,1), 5000, 0, 0);
	lLights[1].m_iLight = 2;

//	ConOut(CStrF("%f", TimeWrapped));

	CMat4Dfp32 ProjTransform;
	ProjTransform.Unit();
	Angles.CreateMatrixFromAngles(0, ProjTransform);

	fp32 Sbz = Clamp01(TimeWrapped - 0.0f) * Clamp01(WrapTime - 0.5*60.0f - 1.0f - TimeWrapped);
	fp32 Proj = Clamp01(TimeWrapped - (WrapTime - 0.5f*60.0f)) * Clamp01(WrapTime-1.0f - TimeWrapped);
	fp32 ProjT = (TimeWrapped - (WrapTime-0.5*60.0f)) / 30.0f;

//	Sbz *= 0.1;
//	Proj *= 0.1;
//	ProjT *= 0.1;

	if (Proj > 0.001f)
	{
		lLights[1].SetPosition(CVec3Dfp32(LERP(3500, -3500, ProjT), -1000, 1800));
		lLights[1].GetPosition().SetRow(ProjTransform, 3);
		lLights[1].SetProjectionMap(pTC->GetTextureID("Console_02_Proj01"), &ProjTransform);
		lLights[1].SetIntensity(CVec3Dfp32(0.1f*Proj, 1.1*Proj, 1.1*Proj));
	}
	else
	{
		lLights[1].GetPosition().SetRow(ProjTransform, 3);
		lLights[1].SetProjectionMap(pTC->GetTextureID("Special_SbzProj256"), &ProjTransform);
		lLights[1].SetIntensity(CVec3Dfp32(2.0f*Sbz,1.0f*Sbz,0.1f*Sbz));
	}


	const CRC_Viewport* pVP = m_pVBM->Viewport_Get();
	const CRct ViewRect = pVP->GetViewRect();

	bool bShowEdit = true;
	bool bTimeout = false;
	bool bShowBG = true;

	switch(m_Mode)
	{
	case CRC_CONSOLERENDER_INVISIBLE :
		return;
	case CRC_CONSOLERENDER_OVERLAY :
		{
			bTimeout = true;
			m_ConsoleTarget = fnth * 4;
//			pos.p1.y = pos.p0.y + fnth * 4;

			bShowEdit = false;
			bShowBG = false;

			// Move instantly
			m_ConsolePos = m_ConsoleTarget;
			m_ConsolePosPrim = 0;
		}
		break;
	case CRC_CONSOLERENDER_HALF :
		{
			m_ConsoleTarget = ViewRect.GetHeight() / 2;
		}
		break;
	case CRC_CONSOLERENDER_FULL :
	default :
		{
			m_ConsoleTarget = ViewRect.GetHeight();
			m_ConsolePos = m_ConsoleTarget;
			m_ConsolePosPrim = 0;
		}
		break;
	}

	Moderate(m_ConsolePos, m_ConsoleTarget, m_ConsolePosPrim, 256);
	pos.p1.y = m_ConsolePos;

//	CMTime Time = CMTime::GetCPU();

	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	if (!pSC)
		return;

	if (!m_pVBM->Viewport_Push(pVP))
		return;
	m_pVBM->ScopeBegin("CRC_ConsoleRender::RenderRectShaded", false);

	CClipRect conrect(pos, pos.p0);
/*		CClipRect imgrect = img->GetClipRect();
	cr += imgrect;
	CClipRect conrect = CClipRect(pos.p0, pos.p1, pos.p0);
	conrect += cr;*/

	int vish = (pos.p1.y - pos.p0.y);
	int h = fnth+4;
h = 16;
	if (bShowBG)
	{
		int x0(pos.p0.x);
		int y0(pos.p0.y);
		int x1(pos.p1.x);
		int y1(pos.p1.y);

		// Calculate where the ornament should be
		int scrh = ViewRect.GetHeight();
		int x2 = x0 + (x1-x0-256) / 2;
		int y2 = y1 - scrh + (scrh-256) / 2;

		x2 = x0+16;
		y2 = y1-h;

		m_Util2D.Begin(NULL, const_cast<CRC_Viewport*>(pVP), m_pVBM);
		m_Util2D.GetAttrib()->Attrib_Enable(CRC_FLAGS_ZWRITE);
		m_Util2D.GetAttrib()->Attrib_Enable(CRC_FLAGS_ZCOMPARE);
		m_Util2D.GetAttrib()->Attrib_ZCompare(CRC_COMPARE_ALWAYS);
		m_Util2D.Rect(conrect, CRct(x0,y0-1, x1, y1), 0xff000000);
		m_Util2D.End();

		CXW_Surface* pSurfBG = pSC->GetSurface("CONSOLE_BG");
//		CXW_Surface* pSurfOrnament = pSC->GetSurface("CONSOLE_ORNAMENT");
		CXW_Surface* pSurfLBorder = pSC->GetSurface("CONSOLE_LBORDER");
		CXW_Surface* pSurfDBorder = pSC->GetSurface("CONSOLE_DBORDER");


		if (pSurfBG && pSurfLBorder && pSurfDBorder)
		{
			m_Util2D.Begin(NULL, const_cast<CRC_Viewport*>(pVP), m_pVBM);

			for(int iLight = 0; iLight < nLights; iLight++)
			{
				CXR_SurfaceShaderParams SSP;
				CXR_ShaderParams ShaderParams;

				m_Util2D.SetTextureScale(1, 1);

				// Render bg
				SSP.Create(pSurfBG, pSurfBG->GetBaseFrame());
				ShaderParams.Create(NULL, NULL, &m_Shader);
				ShaderParams.m_Flags |= XR_SHADERFLAGS_NOSTENCILTEST | XR_SHADERFLAGS_USEZLESS;
				m_Util2D.SetSurface(pSurfBG, Time, 0);
				m_Util2D.SetTextureOrigo(conrect, CPnt(x2,y2));
				m_Util2D.Rect(conrect, CRct(x0+16,y0-1, x1 /*-16*/, y1-h), 0xffffffff, &m_Shader, &ShaderParams, &SSP, lLights[iLight]);

				// Render edit bg
				SSP.Create(pSurfDBorder, pSurfDBorder->GetBaseFrame());
				ShaderParams.Create(NULL, NULL, &m_Shader);
				ShaderParams.m_Flags |= XR_SHADERFLAGS_NOSTENCILTEST | XR_SHADERFLAGS_USEZLESS;
				m_Util2D.SetSurface(pSurfDBorder, Time, 0);
				m_Util2D.SetTextureOrigo(conrect, CPnt(x0,y1));
				m_Util2D.Rect(conrect, CRct(x0,y1-h, x1, y1), 0xffffffff, &m_Shader, &ShaderParams, &SSP, lLights[iLight]);
				m_Util2D.Rect(conrect, CRct(x0+1,y1-h+1, x1-1, y1-1), 0xffffffff, &m_Shader, &ShaderParams, &SSP, lLights[iLight]);

				// Render ornament
/*				ShaderParams.Create(*pSurfOrnament->GetBaseFrame());
				ShaderParams.m_Flags |= XR_SHADERFLAGS_NOSTENCILTEST | XR_SHADERFLAGS_USEZLESS;
				m_Util2D.SetSurface(pSurfOrnament, Time, 0);
				m_Util2D.SetTextureOrigo(conrect, CPnt(x2,y2));
				m_Util2D.Rect(conrect, CRct(x2,y2,x2+256,y2+256), 0xff808080, &m_Shader, &ShaderParams, lLights[iLight]);
*/
				// Render left border
				SSP.Create(pSurfLBorder, pSurfLBorder->GetBaseFrame());
				ShaderParams.Create(NULL, NULL, &m_Shader);
				ShaderParams.m_Flags |= XR_SHADERFLAGS_NOSTENCILTEST | XR_SHADERFLAGS_USEZLESS;
				m_Util2D.SetSurface(pSurfLBorder, Time, 0);
				m_Util2D.SetTextureOrigo(cr, CPnt(x0, y1-h));
				m_Util2D.Rect(conrect, CRct(x0,y0, x0+16, y1-h-1), 0xffffffff, &m_Shader, &ShaderParams, &SSP, lLights[iLight]);

				// Render right border
//				m_Util2D.SetTextureOrigo(cr, CPnt(x1-16, y1-h));
//				m_Util2D.Rect(conrect, CRct(x1-16, y0, x1, y1-h-1), 0xff808080, &m_Shader, &ShaderParams, lLights[iLight]);
			}

//			m_Util2D.Flush();
			m_Util2D.End();

		}
	}
	m_pVBM->ScopeEnd();
	m_pVBM->ScopeBegin("Console Text", false);

	m_Util2D.Begin(NULL, const_cast<CRC_Viewport*>(pVP), m_pVBM);
	m_Util2D.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
	m_Util2D.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	int yp = vish-fnth-4;
	if (bShowEdit)
	{
		CFStr cl = CFStrF("> %s", pCon->GetCommandLineEdit()->GetStr().Str());

		CPnt& ERP = m_EditRelPos;
		EditText(conrect, m_pFont, pos.p0.x+ERP.x, yp+ERP.y, cl, 0.0f, m_FontSize);
		EditText(conrect, m_pFont, (int)(pos.p0.x+ERP.x + m_Util2D.TextWidth(m_pFont, (char*)cl.Copy(0, pCon->GetCommandLineEdit()->GetCursorPos()+2)) * m_FontScale), yp+ERP.y, "_", _PI, m_FontSize);
		yp -= fnth+4;
	}

	CClipRect textclip;
	textclip = conrect;
//	textclip.clip.p1.x -= 16;
	CConsoleString* pStr = pCon->GetOutputHead();
	while(pStr && yp > -fnth)
	{
		fp32 Age = (Time - pStr->m_Time).GetTime();
		if (!bTimeout || (Age < m_TextTimeOut))
		{
			int Intens = 255;
			if (bShowBG)
			{
//					if (yp < 255) Intens = Max(0, yp);
			}
			else
				if (bTimeout && (Age > m_TextTimeOut-m_TextFadeTime))
					Intens = (int)(255.0f*(m_TextTimeOut - Age) / m_TextFadeTime);

//				if (bShowBG) m_Util2D.Text(textclip, m_spFont, pos.p0.x+16+2, yp+2, pStr->m_Str, CPixel32(0, 0, 0, Intens), -1);

			m_Util2D.Text(textclip, m_pFont, pos.p0.x+16+4, yp, pStr->m_Str, CPixel32(255, 255, 255, Intens), m_FontSize);
		}
		yp -= fnth;
		pStr = pStr->m_pPrev;
	}

	if(bShowBG)
	{
		m_Util2D.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		m_Util2D.SetTexture(0);
		int yp = 0;
		for(;(yp < 256) && (yp < pos.p1.y-h); yp += 4)
		{
			int Intens = (int)(95.0f + 160.0f * (1.0f - (Sqr(1.0f - (fp32(yp) / 256.0f)))));
//				int Intens = (yp >> 1) + 127;
			m_Util2D.Rect(conrect, CRct(pos.p0.x, yp, pos.p1.x, yp+4), CPixel32(0, 0, 0,255-Intens));
		}
	}

	m_pVBM->Viewport_Pop();
	m_pVBM->ScopeEnd();
	m_Util2D.End();
};

//----------------------------------------------------------------
#ifdef PLATFORM_XBOX
	#include <xtl.h>
#elif defined PLATFORM_WIN_PC
	#include "../../Shared/MOS/Classes/Win32/MWin32.h"
	#include "../../Shared/MOS/Classes/Win32/MWin32Util.h"
	#include <commctrl.h>
//	#include "resource.h"

#elif defined TARGET_DREAMCAST_SHINOBI
	
#endif

#define NUMFPSHIST 120

#include "../../Shared/MOS/Classes/Registry/MRegistryNavigator.h"

class CFPSChecker : public MRTC_Thread
{
public:

	CFPSChecker()
	{

		m_DesiredFrameTime = 1.0f/10.0f;
		m_bBreak = false;
		m_bWarn = true;
	}

	MRTC_CriticalSection m_Lock;
	CMTime m_TimeSinceLastRefresh;

	fp32 m_DesiredFrameTime;
	bool m_bBreak;
	bool m_bWarn;
	
	void DoneFrame()
	{
		M_LOCK(m_Lock);
		m_TimeSinceLastRefresh = CMTime::GetCPU();
		m_bWarn = false;
	}

	void SetFPS(fp32 _FPS)
	{
		M_LOCK(m_Lock);
		m_DesiredFrameTime = 1.0/_FPS;

		if (!m_hThread)
			Thread_Create(NULL, 65536, MRTC_THREAD_PRIO_TIMECRITICAL);
	}

	void SetBreak(bool _bBreak)
	{
		m_bBreak = _bBreak;
	}

	const char* Thread_GetName() const
	{
		return "XRApp FPS Checker";
	}
	int Thread_Main()
	{
		while (!Thread_IsTerminating())
		{
			M_LOCK(m_Lock);
			CMTime Elapsed = CMTime::GetCPU() - m_TimeSinceLastRefresh;

//			static int Break = 0;
			if (Elapsed.GetTime() > m_DesiredFrameTime)
			{
				if (!m_bWarn)
				{
					m_bWarn = true;
					M_TRACEALWAYS("Warning Bad FPS\n");
					if (m_bBreak)
					{
						M_BREAKPOINT;
					}
				}

			}

			MRTC_SystemInfo::OS_Sleep(1);
		}

		return 0;
		
	}
};

static volatile bint s_bTraceRecordSystem = false;
static volatile bint s_bTraceRecordGameRefresh = false;
static volatile bint s_bTraceRecordRender = false;
static volatile bint s_bPerfCountSystem = false;
static volatile bint s_bPerfCountRender = false;



class CSystemThread : public MRTC_Thread
{
public:
	class CXRealityApp* m_pApp;
	CDisplayContext* m_pDC;
	spCXR_VBMContainer m_spVBMContainer;
	CXR_VBManager*	m_pVBM;
	spCGameContext	m_spGame;
	CSystem* m_pSystem;

	CSystemThread()
	{
	}

	void Create(class CXRealityApp* _pApp, spCXR_VBMContainer _spVBMContainer, CDisplayContext* _pDC, spCGameContext _spGame)
	{
		m_spVBMContainer = _spVBMContainer;
		m_pDC = _pDC;
		m_pApp = _pApp;
		m_spGame = _spGame;

		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		m_pSystem	= pSys;
		if (!m_hThread)
		{
			mint CPUBits = 32;
#if CPU_BITS > 32
			CPUBits = CPU_BITS;
#endif
			Thread_Create(NULL, 256*1024 * (CPUBits / 32), MRTC_THREAD_PRIO_NORMAL);
		}
	}

	void Destroy()
	{
		if(m_hThread)
		{
			Thread_Destroy();
			MRTC_GOM()->m_hScriptThread = MRTC_SystemInfo::OS_GetThreadID();
		}
	}

	const char* Thread_GetName() const
	{
		return "XRApp System Thread";
	}

	int Thread_Main();

	void SystemThread();
};

#ifdef PLATFORM_XENON
extern mint gf_GetFreePhysicalMemory();
extern mint gf_GetLargestBlockPhysicalMemory();
extern mint gf_RenderXenon_GetFreeMem();
extern mint gf_RenderXenon_GetExternalMemory();
extern mint gf_RenderXenon_GetLargetstBlock();
extern mint gf_RenderXenon_GetSavedMidblock();
extern mint gf_RenderXenon_GetUsedMidblock();
extern mint gf_RenderXenon_UsedHeap();
extern mint gf_RenderXenon_GetHeapSize();

#endif

class CXRealityApp;
class CXRealityApp : public CApplication
{
#ifdef PLATFORM_XENON
	PMCState m_PMCState;
#endif

#ifdef LOG_FRAME_INFO
//Used to export VBM info to monitor frame latency - ae
	class CLogFrame
	{
	public:
		fp32	m_Start,m_End;
		fp32 m_BreakStart,m_BreakEnd;
		uint8	m_iColor;
		uint16	m_iNumber;
	};

	CLogFrame	m_lLatencyLogMain[LATENCY_LOG_FRAMES];
	CLogFrame	m_lLatencyLogSys[LATENCY_LOG_FRAMES];
	uint32 m_iCurrentSys,m_iCurrentMain;
	CStr m_FrameLogFile;

	CXR_VBManager * m_lpVBM[10];

	void SaveLog();
#endif

	friend class CSystemThread;
	MRTC_DECLARE;

	void* m_MainThreadID;

#ifdef M_Profile
	CFPSChecker m_FPSChecker;
#endif

/*	fp32 m_FPSHistory[NUMFPSHIST];
	fp32 m_AverageFrameTime;
	int m_FPSHistPtr;
	fp32 m_AvgFPS;
	fp32 m_FPS;*/

	CSystemThread	m_SystemThread;
	spCXR_VBMContainer m_spVBMContainer;
	CXR_VBEContext m_VBEContext;

	spCRegistryNavigator m_spRegNav;

	spCXR_Engine m_spEngine;
	spCRC_ConsoleViewport m_spViewport;	// The game's rendering viewport should be up to CWorld_Client to define.

	CRC_ConsoleRender m_ConRender;
#ifdef PLATFORM_CONSOLE
	CRC_Font *m_spDebugFont;
#else
	TPtr<CRC_Font> m_spDebugFont;
#endif
	CRC_Util2D m_Util2D;

#ifdef M_Profile
	TArray<CStr> m_CallGraphStrings;
	int m_ProfilerFramesLeft;
	TArray<TArray<CStr> > m_ProfilerStrings;
#endif

#ifndef M_RTM
	int m_BadFPSDump;
#endif
	uint m_bConsoleInvis : 1;
	uint m_bShowConsole : 1;
	uint m_bShowFPS : 1;
	uint m_bShowFPSLite : 1;
	uint m_bShowVertexProgramWarning : 1;
	uint m_bShowFPSOnly : 1;
	uint m_bShowBorders : 1;
	uint m_bShowSoundDbg : 1;
	uint m_bShowVBE : 1;
	uint m_bShowMemInfo : 1;
	uint m_bLogKeys : 1;	
	uint m_bDebugKeys : 1;	
#ifdef M_Profile
	uint m_bShowCallGraph : 1;
	uint m_bShowCallGraphAutoUpdate : 1;
	uint m_bOnRenderCallGraph : 1;
#endif
	uint m_bShowStats : 1;
	uint m_bShowAnimationsDbg : 1;
	uint m_bShowTurtle : 1;
	uint m_bAutomaticScreenResize : 1;
	uint m_bAutomaticScreenResizeSticky : 1;
	uint m_bSound : 1;
	uint m_bLoading : 1;
	uint m_bPendingOptionsUpdate : 1;
	uint m_bCaptureNext : 1;

	int m_bMultithreaded;
	int m_bEnableMultithreaded;

	int m_TurtleMinFPS;
	int m_TurtleMaxFPS;
	CStr m_BuildMsg;
#ifdef M_DEMO_XBOX
	fp32 m_IdleQuitTimeOut;
	fp32 m_IdleQuitTimeOutPause;
	bool m_CanQuit;
	//fp32 m_IdleQuitTimeOutTimer;
	//CMTime m_LastTouchTime;
	//CMTime m_LastDownKey;
#endif
	int m_DrawMode;
	

	CStr m_PendingExecute;
	spCSoundContext m_spSoundContext;

	NThread::CMutual m_RenderGUILock;

	spCGameContext m_spGame;
	CXR_VBManager* m_pVBM;

#ifndef PLATFORM_CONSOLE
	spCVideoFile m_spVideoFile;
	spCImage m_spVideoFileCapture;
	spCImage m_spVideoFileConv;
#endif

	CFStr m_MemStatusMini;

#ifdef PLATFORM_WIN
#ifdef PLATFORM_WIN_PC
	spCLogFileWindow m_spLogWin;
#endif

	MEMORYSTATUS m_Win32MemoryStatus;
	CFStr m_Win32MemoryStatusStr;
	CMTime m_Win32MemoryStatusLastUpdate;

	virtual void Win32_UpdateMemoryStatus(bool _bForceUpdate = false)
	{
//		CMTime T = CMTime::GetCPU();
		CMTime T;
		T.Snapshot();
		if (_bForceUpdate || ((T - m_Win32MemoryStatusLastUpdate).Compare(1.0f) > 0))
		{
			m_Win32MemoryStatusLastUpdate = T;

			m_Win32MemoryStatus.dwLength = sizeof(MEMORYSTATUS);
			GlobalMemoryStatus(&m_Win32MemoryStatus);

			const MEMORYSTATUS& s = m_Win32MemoryStatus;
#ifdef PLATFORM_XENON
			const fp32 ScaleMiB = (1.0f / (1024.0f * 1024.0f));
			m_Win32MemoryStatusStr = CFStrF("§c55FP(F:%.1fM) §c5f5PH(F:%.1fM §c5f5L:%.1fM) §cf55GH(F:§cfff%.1fM §cf55L:%.1fM U:%.1fM(E%.1fM)/%.1fM MS:%.1fM/%.1fM) ", 
				fp32(s.dwAvailPhys) * ScaleMiB, 
				fp32(gf_GetFreePhysicalMemory()) * ScaleMiB,
				fp32(gf_GetLargestBlockPhysicalMemory()) * ScaleMiB,
				fp32(gf_RenderXenon_GetFreeMem()) * ScaleMiB,
				fp32(gf_RenderXenon_GetLargetstBlock()) * ScaleMiB,
				fp32(gf_RenderXenon_UsedHeap()) * ScaleMiB,
				fp32(gf_RenderXenon_GetExternalMemory()) * ScaleMiB,
				fp32(gf_RenderXenon_GetHeapSize()) * ScaleMiB,
				fp32(gf_RenderXenon_GetUsedMidblock()) * ScaleMiB,
				fp32(gf_RenderXenon_GetSavedMidblock()) * ScaleMiB
			);

			uint FreeMain = MRTC_GetMemoryManager()->GetFreeMem();
			uint FreeGfx = gf_RenderXenon_GetFreeMem();
			m_MemStatusMini = CFStrF("Free: %.1f (Main: %.1f - Gfx: %.1f)", 
									fp32(FreeMain + FreeGfx) * ScaleMiB, fp32(FreeMain) * ScaleMiB, fp32(FreeGfx) * ScaleMiB);
#else

			m_Win32MemoryStatusStr = CFStrF("P(%.1f)/%.1f ", 
				fp32(s.dwAvailPhys) / (1024*1024), 
				fp32(s.dwTotalPhys) / (1024*1024)
				);

			m_MemStatusMini = "(xenon only)";
#endif
		}
	}
#endif

	void DoRemoteDebuggerInput();
public:
	CXRealityApp();
	~CXRealityApp();
	virtual void Create();
	void InitWorld();
	virtual void OnRefreshSystem();
	virtual void RenderStats(CDisplayContext* _pDisplay, CRenderContext* _pRC, CXR_VBManager* _pVBM, CClipRect _ConsoleClip, CRct _ConsoleRect);
	virtual void OnRender(CDisplayContext* _pDisplay, CImage* _pFrame, CClipRect _Clip) {};
	virtual void OnRender(CDisplayContext* _pDisplay, CImage* _pFrame, CClipRect _Clip, int _Context);
	//virtual void OnRenderDoIt(CDisplayContext* _pDisplay, CImage* _pFrame, CClipRect _Clip, int _Context);
	virtual void OnBusy(int _Context);
	virtual void DoModal();

	int Options_GetScanKey(const CStr &_Action, CRegistry *_pOptions, CRegistry *_pDynamicStringTable = NULL, int _iType = 0);
	void Options_UpdateBinds();
	void Options_SetDefaultBinds(int _iType = 0);
	void Options_ClearBinds();

	void DoInput(bool _FromBusy);

#ifdef M_DEMO_XBOX
	void XBoxDemo_Init();
	void XBoxDemo_Update();
	void XBoxDemo_PauseTimer(int32 i) { m_IdleQuitTimeOutPause = i; }
	void XBoxDemo_ResetTimer();
#endif

#ifdef PLATFORM_XBOX1
	void Con_LaunchDashboard(CStr _What);
	void Con_OnlineTitleUpdate();
#endif


	void Con_Launch(CStr _Value);

	void Con_SwitchControllerMethod();
	void Con_SetDefaultOptions();
	void Con_SetDefaultBinds(int _iType);
	void Con_ClearBindings();
	void Con_SetDefaultOptionsGlobal();

	void Con_Vid_Gamma(fp32 _Value);
	void Con_Vid_BlackLevel(fp32 _Value);
	void Con_Vid_Brightness(fp32 _Value);
	void Con_Vid_GammaRamp(int32 _Value);

	void Con_Vid_PixelAspect(fp32 _Value);

	void Con_UpdateOptions();
	void CommitOptions();
	void CommitGamma();

	void Con_OptionsGetFromEnv();
	void Con_SaveOptions();
	void Con_ShowFPSToggle();
	void Con_ShowFPSLiteToggle();
	void Con_ShowStatsToggle();
	void Con_ShowSoundToggle();
	void Con_ShowVBEToggle();
	void Con_ToggleShowMemInfo();

	void Con_In_LogKeys(int _Log);
	void Con_In_DebugKeys(int _Log);	

#ifdef M_Profile
	void Con_DumpCallList();
	void Con_StartProfiler(int _nFrames);
	void Con_CheckFPS(int _nFrames, int _bBreak);
#endif
	void Con_ToggleAutomaticViewscale();
	void Con_SetAutomaticViewscale(int _bOn);
#ifndef M_RTM
	void Con_DumpBadFPS(int _Fps);
#endif
	void Con_ResetTimer();
	void Con_PauseTimer();
	void Con_PauseTimer5();
	void Con_CanQuit(int32 _i);
	void Con_Crash();
	void Con_CrashCException();
	void Con_DrawMode(int _v);

	void Con_VideoFileBegin(CStr _Path, int _FPS);
	void Con_VideoFileEnd();
	void Con_TraceRecordSystem();
	void Con_TraceRecordRender();
	void Con_TraceRecordGameRefresh();
	void Con_PerfCountSystem();
	void Con_PerfCountRender(); 


	void Con_EnableMultithread(int _bEnabled);
	void Con_EnableThreadPool(int _bEnabled);

#ifdef LOG_FRAME_INFO
	void Con_LogFrameTimes(CStr _FileName);
#endif

	void Register(CScriptRegisterContext &_RegContext);

	void SystemThread(CDisplayContext* _pDC);

#if 0
	class CStatistics
	{
	public:
		CPnt m_ScreenSize;
		CPnt m_3DVP;
		fp32 m_RefreshRate;

		CStatistics()
		{
			m_iCurrentHist = 0;

			m_StatAvg.m_BlockTime = 0;
			m_StatAvg.m_CPUUsage = 0;
			m_StatAvg.m_GPUUsage = 0;
			m_StatAvg.m_Time_FrameTime = 0;
			m_StatAvg.m_nPixelsDrawn = 0;
			m_iPauseCount = 0;

			m_StatAdder.m_BlockTime = 0;
			m_StatAdder.m_CPUUsage = 0;
			m_StatAdder.m_GPUUsage = 0;
			m_StatAdder.m_Time_FrameTime = 0;
			m_StatAdder.m_nPixelsDrawn = 0;
			memset(&m_History, 0, sizeof(m_History));

			m_ScaleAdder = 0;
			m_ScaleAvg = 0;
			memset(&m_ScaleHistory, 0, sizeof(m_ScaleHistory));

			m_3DVPAdder = 0;
			m_3DVPAvg = 0;
			memset(&m_3DVPHistory, 0, sizeof(m_3DVPHistory));

			m_ScaleLast = CVec2Dfp32(1.0f,1.0f);
		}

		enum 
		{
			EHistorySize = 6
		};

		CRC_Statistics m_StatAvg;
		CRC_Statistics m_StatAdder;
		CRC_Statistics m_History[EHistorySize];
		CVec2Dfp32 m_ScaleHistory[EHistorySize];
		CVec2Dfp32 m_ScaleAdder;
		CVec2Dfp32 m_ScaleAvg;
		CVec2Dfp32 m_ScaleLast;

		CVec2Dfp32 m_3DVPAdder;
		CVec2Dfp32 m_3DVPAvg;
		CVec2Dfp32 m_3DVPHistory[EHistorySize];


		int m_iCurrentHist;

		int m_iPauseCount;

		CVec2Dfp32 Feed(CRC_Statistics &_Stat, bool _bAllowChange)
		{
			m_StatAdder.m_BlockTime -= m_History[m_iCurrentHist].m_BlockTime;
			m_StatAdder.m_CPUUsage -= m_History[m_iCurrentHist].m_CPUUsage;
			m_StatAdder.m_GPUUsage -= m_History[m_iCurrentHist].m_GPUUsage;
			m_StatAdder.m_Time_FrameTime -= m_History[m_iCurrentHist].m_Time_FrameTime;
			m_StatAdder.m_nPixelsDrawn -= m_History[m_iCurrentHist].m_nPixelsDrawn;

			m_ScaleAdder -= m_ScaleHistory[m_iCurrentHist];

			m_3DVPAdder -= m_3DVPHistory[m_iCurrentHist];

			m_ScaleHistory[m_iCurrentHist] = m_ScaleLast;
			m_History[m_iCurrentHist] = _Stat;
			m_3DVPHistory[m_iCurrentHist] = CVec2Dfp32(m_3DVP.x, m_3DVP.y);

			m_StatAdder.m_BlockTime += m_History[m_iCurrentHist].m_BlockTime;
			m_StatAdder.m_CPUUsage += m_History[m_iCurrentHist].m_CPUUsage;
			m_StatAdder.m_GPUUsage += m_History[m_iCurrentHist].m_GPUUsage;
			m_StatAdder.m_Time_FrameTime += m_History[m_iCurrentHist].m_Time_FrameTime;
			m_StatAdder.m_nPixelsDrawn += m_History[m_iCurrentHist].m_nPixelsDrawn;

			m_ScaleAdder +=  m_ScaleHistory[m_iCurrentHist];
			m_3DVPAdder += m_3DVPHistory[m_iCurrentHist];

			++m_iCurrentHist;
			if (m_iCurrentHist >= EHistorySize)
				m_iCurrentHist = 0;

			m_StatAvg.m_BlockTime = m_StatAdder.m_BlockTime / EHistorySize;
			m_StatAvg.m_CPUUsage = m_StatAdder.m_CPUUsage / EHistorySize;
			m_StatAvg.m_GPUUsage = m_StatAdder.m_GPUUsage / EHistorySize;
			m_StatAvg.m_Time_FrameTime = m_StatAdder.m_Time_FrameTime / EHistorySize;
			m_StatAvg.m_nPixelsDrawn = m_StatAdder.m_nPixelsDrawn / EHistorySize;

			m_ScaleAvg.k[0] = m_ScaleAdder.k[0] / EHistorySize;
			m_ScaleAvg.k[1] = m_ScaleAdder.k[1] / EHistorySize;

			m_3DVPAvg.k[0] = m_3DVPAdder.k[0] / EHistorySize;
			m_3DVPAvg.k[1] = m_3DVPAdder.k[1] / EHistorySize;

			MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");

			static fp32 Threshold = 0.96f;
			static fp32 Threshold2 = 0.92f;
			static fp32 ChangeStat = 0.9910f;
			static fp32 PowUp = 0.028f;
			static fp32 PowDown = 0.032f;

			/*		bool bVSyncOn = pSys->GetEnvironment()->GetValuei("VID_VSYNC", 0, 0) != 0;
			if (bVSyncOn)
			{
			fp32 TargetFrameTime = 2.0f / m_RefreshRate;

			fp32 MaxUsage = Max(m_StatAvg.m_CPUUsage, m_StatAvg.m_GPUUsage);
			fp32 FramTime = m_StatAvg.m_Time_FrameTime * MaxUsage;

			fp32 GPUUsage = m_StatAvg.m_GPUUsage / m_StatAvg.m_CPUUsage;

			bool bDownScale = false;
			fp32 Fraction = (FramTime / TargetFrameTime);
			fp32 ChangeInv = 1.0f / Change;
			fp32 ThresholdInv = 1.0 / Threshold;

			if (Fraction > ThresholdInv)
			{
			if (m_StatAvg.m_GPUUsage > Threshold && m_StatAvg.m_CPUUsage < Threshold)
			{
			m_ScaleLast.k[0] *= Change;
			m_ScaleLast.k[1] *= Change;
			}
			else if (m_StatAvg.m_GPUUsage < Threshold)
			{
			bDownScale = true;
			}
			}
			else if (Fraction < Threshold || m_StatAvg.m_GPUUsage < Threshold)
			bDownScale = true;

			if (bDownScale)
			{
			m_ScaleLast.k[0] *= ChangeInv;
			m_ScaleLast.k[1] *= ChangeInv;
			}
			}
			else*/
			if (_bAllowChange)
			{
				if (m_iPauseCount > 0)
					--m_iPauseCount;
			}
			else
				m_iPauseCount = 10;

			if (m_iPauseCount <= 0)
			{
				fp32 Change = ChangeStat;

				fp32 TargetFrameTime = 3.0f / (m_RefreshRate / (Threshold2));

				fp32 MaxUsage = Max(m_StatAvg.m_CPUUsage, m_StatAvg.m_GPUUsage);
				fp32 FramTime = m_StatAvg.m_Time_FrameTime * MaxUsage;
				fp32 AvgCPU = m_StatAvg.m_CPUUsage / MaxUsage;
				fp32 AvgGPU = m_StatAvg.m_GPUUsage / MaxUsage;

				bool bDownScale = false;
				fp32 Fraction = (FramTime / TargetFrameTime);
				if (Fraction < 1.0f)
					Change *= powf(Fraction, PowUp);
				else 
					Change *= powf(1.0/Fraction, PowDown);
				fp32 ChangeInv = 1.0f / Change;
				fp32 ThresholdInv = 1.0 / Threshold;

				if (Fraction > ThresholdInv)
				{
					if (AvgGPU > Threshold && m_StatAvg.m_CPUUsage < Threshold)
					{
						m_ScaleLast.k[0] *= Change;
						m_ScaleLast.k[1] *= Change;
					}
					else if (m_StatAvg.m_GPUUsage < Threshold*Threshold)
					{
						bDownScale = true;
					}
				}
				else if (Fraction < Threshold || AvgGPU < Threshold)
					bDownScale = true;

				if (bDownScale)
				{
					m_ScaleLast.k[0] *= ChangeInv;
					m_ScaleLast.k[1] *= ChangeInv;
				}
			}

			m_ScaleLast.k[0] = Min(m_ScaleLast.k[0], 1.0f);
			m_ScaleLast.k[1] = Min(m_ScaleLast.k[1], 1.0f);
			m_ScaleLast.k[0] = Max(m_ScaleLast.k[0], 0.5f);
			m_ScaleLast.k[1] = Max(m_ScaleLast.k[1], 0.5f);

			//			CVec2Dfp32 Jittered; 
			//			Jittered.k[0] = ((fp32)((TruncToInt((m_ScaleLast.k[0] * m_ScreenSize.x) / 8)) * 8)) / m_ScreenSize.x;
			//			Jittered.k[1] = ((fp32)((TruncToInt((m_ScaleLast.k[1] * m_ScreenSize.y) / 8)) * 8)) / m_ScreenSize.y;

			return m_ScaleLast;
		}

	};

	CStatistics m_Statistics;
#endif

	MRTC_CriticalSection m_MainThreadStats_Lock;
	CRC_StatCounter<fp64, fp32, 60> m_MainThread_FrameTime;
	CRC_StatCounter<fp64, fp32, 60> m_MainThread_VBMWait;
	
	MRTC_CriticalSection m_SystemStats_Lock;
	CRC_StatCounter<fp64, fp32, 60> m_SystemThread_FrameTime;
	CRC_StatCounter<fp64, fp32, 60> m_SystemThread_VBMWait;
	
	CMTime m_lTimeStack[3];
#ifdef LOG_FRAME_INFO
	uint16 m_lFrameNumber[3];
	uint16 m_FrameCounter;
#endif
	CMTime m_LatencyTime;
	CXR_VBManager * m_lpStack[3];
	CRC_StatCounter<fp64, fp32, 60> m_SystemThread_FrameLatency;

	CMTime m_MainThread_LastTime;
	CMTime m_SystemThread_LastTime;

	int m_RefreshIntervalCanDo[16];
	int m_LastInterval;
	int m_LastSetInterval;

};

int CSystemThread::Thread_Main()
{
	MRTC_SystemInfo::Thread_SetProcessor(5);

	MRTC_GOM()->m_hScriptThread = MRTC_SystemInfo::OS_GetThreadID();
	while(!Thread_IsTerminating())
	{
#ifdef PLATFORM_XENON
		bint bTraceRecordSystem = s_bTraceRecordSystem;
		if (bTraceRecordSystem)
			XTraceStartRecording( "cache:\\TraceRecord_System.bin" );

		bint bPerfCountSystem = s_bPerfCountSystem;
		if (bPerfCountSystem)
		{
			PMCInstallSetup( &PMCDefaultSetups[PMC_SETUP_OVERVIEW_PB2T1] );
			// Reset the Performance Monitor Counters in preparation for a new sampling run.
			PMCResetCounters();
			// Start up the Performance Monitor Counters.
			PMCStart(); // Start up counters
		}

#endif
		SystemThread();
#ifdef PLATFORM_XENON
		if (bPerfCountSystem)
		{
			PMCStop();
			// Get the counters.
			PMCGetCounters( &m_pApp->m_PMCState);
			// Print out detailed information about all 16 counters.
			PMCDumpCountersVerbose( &m_pApp->m_PMCState, PMC_VERBOSE_NOL2ECC );
			s_bPerfCountSystem = false;
		}

		if (bTraceRecordSystem)
		{
			XTraceStopRecording();
			s_bTraceRecordSystem = false;
		}
#endif
	}

	return 0;
}

void CXRealityApp::SystemThread(CDisplayContext* _pDisplay)
{
	CMTime CMFrameTime;
	// -------------------------------------------------------------------
	//  FPS-History
	CMTime Time;

	Time.Snapshot();
#ifdef LOG_FRAME_INFO
	CLogFrame * pLogFrame = (m_iCurrentSys < LATENCY_LOG_FRAMES) ? &m_lLatencyLogSys[m_iCurrentSys] : NULL;
	if( pLogFrame )
	{
		pLogFrame->m_Start = Time.GetTime();
		pLogFrame->m_End = pLogFrame->m_BreakStart = pLogFrame->m_BreakEnd = pLogFrame->m_Start;
		pLogFrame->m_iColor = 5;
		pLogFrame->m_iNumber = m_FrameCounter;
	}
#endif
	m_spVBMContainer->BlockUntilSingleVBMFree(0.5);
	/*
	while( m_spVBMContainer->GetNumAvailVBM() < 1 )
	{
	CMTime Time2;
	Time2.Snapshot();
	if(!m_spVBMContainer->WaitForVBMAdd(0.5 - (Time2 - Time).GetTime())) break;
	}
	*/
	CMTime Time2;
	Time2.Snapshot();
	m_LatencyTime.Snapshot();
	m_SystemThread_VBMWait.AddData((Time2 - Time).GetTime());

#ifdef LOG_FRAME_INFO
	if( pLogFrame ) pLogFrame->m_BreakEnd = Time2.GetTime();
#endif

	Time.Snapshot();
	CMFrameTime = Time - m_SystemThread_LastTime;
	if (m_SystemThread_LastTime.IsReset())
		CMFrameTime = CMTime();
	m_SystemThread_LastTime = Time;

	{
		DLock(m_SystemStats_Lock);
		m_SystemThread_FrameTime.AddData(CMFrameTime.GetTime());
	}

//	MRTC_SystemInfo::MRTC_GetSystemInfo().OS_ClockFrequencyUpdate();
	MACRO_GetSystem;
	if (int Update = pSys->GetRegistry()->GetValuei("OPTG\\UPDATEFROMENV", 0))
	{				
		pSys->GetRegistry()->SetValuei("OPTG\\UPDATEFROMENV", 0);
		if (Update == 1)
		{
			Con_OptionsGetFromEnv();
			m_bPendingOptionsUpdate = true;
		}
	}
	if (m_bPendingOptionsUpdate || pSys->GetRegistry()->GetValuei("OPTG\\COMMITOPTIONS", 0))
	{
		pSys->GetRegistry()->SetValuei("OPTG\\COMMITOPTIONS", 0);
		CommitOptions();
	}

	if(pSys->GetRegistry()->GetValuei("OPTG\\UPDATEGAMMA", 0))
	{
		pSys->GetRegistry()->SetValuei("OPTG\\UPDATEGAMMA", 0);
		CommitGamma();
	}

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	if (MRTC_GetRD()->m_EnableFlags)
		MRTC_GetRD()->SendData(ERemoteDebug_NextFrame, 0, 0, false, false);
#endif


#ifdef PLATFORM_PS2
	MRTC_SystemInfo::CPU_AdvanceClock( 0.0f );
#endif

#ifdef M_Profile
	CMTime FrameTime;
#ifdef	M_RTM
	if (m_ProfilerFramesLeft)
#else
	if (m_BadFPSDump || m_ProfilerFramesLeft)
#endif
	{
		m_bShowCallGraphAutoUpdate = false;
		FrameTime.Start();
		MRTC_GetObjectManager()->m_pCallGraph->Start();
	}
#endif

	MPUSH(CXRealityApp::DoModal);

	int bCorrupt = false;

	// demo timeout
#ifdef M_DEMO_XBOX
	XBoxDemo_Update();
#endif

	// check for corrupted file system
	if (m_spGame != NULL &&  m_spGame->m_spWData != NULL && 
		(CDiskUtil::GetCorrupt() & DISKUTIL_STATUS_CORRUPTANY))
	{
		bCorrupt = true;
	}


	// PC PORT: Allow user to press a key to quit when game has become unstable
#ifdef PLATFORM_WIN_PC
	if(bCorrupt)
	{
		if(m_pSystem->m_spInput->KeyPressed())
		{
			//				m_pSystem->m_spCon->ExecuteString("quit()");

			// Quit (without using script)
			m_pSystem->m_bBreakRequested = true;
		}
	}
#endif

	// handle input
	if (m_pSystem->m_spInput!=NULL)
		DoInput(false);

	// execute scripts
	m_pSystem->m_spCon->ExecuteScripts();

	m_bShowConsole = m_pSystem->m_spCon->GetMode() == CONST_CONSOLE_INPUTKEY;
	if (m_spGame != NULL)
	{
//		M_TRY
//		{
#ifdef PLATFORM_XENON
			bint bTraceRecordGameRefresh = s_bTraceRecordGameRefresh;
			if (bTraceRecordGameRefresh)
			{
				m_spGame->Con_SingleStep();	// Only has effect if we're paused. If we don't single step in pause mode, refresh won't do anything and trace would be quite pointless.
				XTraceStartRecording( "cache:\\TraceRecord_GameRefresh.bin" );
			}
#endif
			m_spGame->Refresh(m_pVBM);
#ifdef PLATFORM_XENON
			if (bTraceRecordGameRefresh)
			{
				XTraceStopRecording();
				s_bTraceRecordGameRefresh = false;
			}
#endif
/*		}
		M_CATCH(
			catch(CCExceptionFile)
		{
			CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
		}
		)
#ifdef M_SUPPORTSTATUSCORRUPT
			M_CATCH(
			catch(CCException)
		{
			CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
		}
		)
#endif*/
	}

	bint bIsLoading = false;
	if (m_spGame && m_spGame->m_spFrontEnd)
		bIsLoading = m_spGame->m_spFrontEnd->m_GameIsLoading;

	if (!bIsLoading || !m_bEnableMultithreaded)
	{
//		M_TRY
//		{
			// Let the system render (?)
			if (m_pSystem->m_spInput!=NULL)
				DoInput(false);
			m_pSystem->Render();
/*		}
		M_CATCH(
			catch(CCExceptionFile)
		{
			CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
		}
		)
	#ifdef M_SUPPORTSTATUSCORRUPT
			M_CATCH(
			catch(CCException)
		{
			CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
		}
		)
	#endif*/

	}
	else if (m_spGame)
	{
		CRCLock RCLock;
		m_spGame->PrecachePerform(m_pSystem->m_spDisplay->GetRenderContext(&RCLock));
	}
		//		if (m_spGame->m_nTimeDemoFramesPlayed > 156)
		//			CHECKMEMORY("PostRender")

		// 
//		if (spDC2 != NULL)
//			m_pSystem->Render(spDC2->m_spDC);

	// refresh CD Audio
	/*		if (m_spCDAudio != NULL)
	{
	try { m_spCDAudio->Refresh(); }
	catch(CCException)
	{
	m_pSystem->m_spCon->WriteExceptions();
	ConOut("§cf80WARNING: CD-Audio shut-down.");
	LogFile("§cf80WARNING: CD-Audio shut-down.");
	m_spCDAudio = NULL;
	}
	}*/


	// Gave compile errors - JA
#if !defined(M_RTM) && defined(M_SUPPORTMEMORYDEBUG)
	if (((CSystemCore *)m_pSystem)->m_bMemoryCheck)
	{
		if (!MRTC_GetMemoryManager()->CheckMemory())
			M_ASSERT(0, "?");

	}
	MRTC_GetMemoryManager()->MemTracking_Report(true);
#endif

	MPOP // MPUSH(CXRealityApp::DoModal);


#ifdef PLATFORM_DOLPHIN
		OSYieldThread();
#endif	
#ifndef M_RTM
	if(m_BadFPSDump)
	{	
		MRTC_GetObjectManager()->m_pCallGraph->Stop();
		FrameTime.Stop();
		if((1.0f / FrameTime.GetTime()) < m_BadFPSDump)
			MRTC_GetObjectManager()->m_pCallGraph->GetStrList(m_CallGraphStrings);
	}
#endif

#ifdef M_Profile
	// MegaProf update
	if (m_ProfilerFramesLeft && !FrameTime.IsReset())
	{
		MRTC_GetObjectManager()->m_pCallGraph->Stop();

		//int iStrList = m_ProfilerStrings.Len() - m_ProfilerFramesLeft;
		int iStrList = m_ProfilerStrings.Len();
		m_ProfilerStrings.QuickSetLen(iStrList + 1);

		MRTC_GetObjectManager()->m_pCallGraph->GetStrList2(m_ProfilerStrings[iStrList]);

		// Time to store profile data to disk
		if (m_ProfilerFramesLeft < 0)
		{
			static int FileNo = 0;
			CStr Filename = CStrF("snapshot%02d.megaprof", FileNo++);

			bool bOK = true;
			M_TRY
			{
				CDiskUtil::CreatePath("dump");
				CCFile File;
				File.Open("dump/"+Filename, CFILE_WRITE);
				File.Writeln("<format 2>");
				File.Writeln(CStrF("<frequency %u>", (uint32)(MGetCPUFrequencyFp() / 1000000.0)));
				uint nFrames = m_ProfilerStrings.Len();
				for (uint n=0; n < nFrames; n++)
				{
					File.Writeln(CStrF("<frame %d>", n));
					const TArray<CStr>& StrList = m_ProfilerStrings[n];
					uint nStr = StrList.Len();
					for (uint i=0; i < nStr; i++)
						File.Writeln(StrList[i]);
				}
				File.Writeln("<end>\n");
			}
			M_CATCH(
				catch(...) 
			{ 
				bOK = false; 

			}
			)
				if (bOK)
				{
					CCFile File;
					File.Open("dump/newprof.tmp", CFILE_WRITE);
					File.Writeln(Filename);
				}
				m_ProfilerFramesLeft = 0;
				m_ProfilerStrings.QuickSetLen(0);
		}
	}
#endif
	if (m_PendingExecute != "")
	{
		m_pSystem->m_spCon->ExecuteString(m_PendingExecute);
		m_PendingExecute.Clear();
	}

#ifdef LOG_FRAME_INFO
	if( pLogFrame )
	{
		Time.Snapshot();
		pLogFrame->m_End = Time.GetTime();
		m_iCurrentSys++;
	}
#endif
}


//----------------------------------------------------------------
#if defined( PLATFORM_DOLPHIN ) || defined( PLATFORM_PS2 )
#pragma force_active on
#endif

MRTC_IMPLEMENT_DYNAMIC(CXRealityApp, CApplication)


#include "mfloat.h"
CXRealityApp::CXRealityApp()
{

#ifdef PLATFORM_WIN_PC
//	if (MRTC_GetMemoryManager()->m_RunningDebugRuntime)
//		_controlfp(0, _EM_INVALID | _EM_ZERODIVIDE);
#endif
	m_bMultithreaded	= STARTUP_MULTITHREADSTATE;
	m_bEnableMultithreaded	= STARTUP_MULTITHREADSTATE;
	m_bLoading = true;
	m_MainThreadID = 0;
#ifndef M_RTM
	m_BadFPSDump = 0;
#endif
#ifdef PLATFORM_WIN
	m_Win32MemoryStatusLastUpdate.Reset();
#endif	
#ifdef M_Profile
	m_bOnRenderCallGraph = false;
	m_ProfilerFramesLeft = 0;
#endif

#ifdef M_DEMO_XBOX
	XBoxDemo_Init();
#endif
	m_spDebugFont = NULL;

	m_bAutomaticScreenResize = false;
	m_bAutomaticScreenResizeSticky = false;
	m_bPendingOptionsUpdate	= true;

	memset(&m_RefreshIntervalCanDo, 0, sizeof(m_RefreshIntervalCanDo));
	m_LastInterval = 0;
	m_LastSetInterval = 0;
}

//
// XBox demo functions
//
#ifdef M_DEMO_XBOX
	void CXRealityApp::XBoxDemo_Init()
	{
		m_CanQuit = true;
		m_IdleQuitTimeOutPause = 100;
		m_IdleQuitTimeOut = g_XboxDemoParams.m_LaunchData.dwTimeout / 1000.0;
		//m_IdleQuitTimeOutTimer = -1;
		//m_LastTouchTime = CMTime::GetCPU();
		//m_LastDownKey = CMTime::GetCPU();
	}

	void CXRealityApp::XBoxDemo_Update()
	{
		if(g_XboxDemoParams.m_LaunchData.dwTimeout <= 0)
			return;

		static CMTime LastTime = CMTime::GetCPU();
		CMTime FrameTime = CMTime::GetCPU() - LastTime;

		if(!g_XboxDemoParams.m_KioskMode && m_IdleQuitTimeOutPause > 0)
			m_IdleQuitTimeOutPause -= FrameTime.GetTime();
		else if(m_IdleQuitTimeOut > 0)
			m_IdleQuitTimeOut -= FrameTime.GetTime();
		else if(m_CanQuit)
		{
			if(g_XboxDemoParams.m_bLaunchedFromDemoLauncher)
			{
				m_pSystem->m_bBreakRequested = true;
				ConExecute("deferredscript (\"quit\")");
			}
			else
			{
				CWorld_Client* pC = m_spGame ? m_spGame->GetCurrentClient() : NULL;
				if(pC && pC->GetClientState() == WCLIENT_STATE_INGAME)
					ConExecute("disconnect(); cg_rootmenu (\"attract\")");
			}
		}

		LastTime = CMTime::GetCPU();
	}

	void CXRealityApp::XBoxDemo_ResetTimer()
	{
		m_IdleQuitTimeOut = g_XboxDemoParams.m_LaunchData.dwTimeout / 1000.0;
	}
#endif

void ByteSwap_uint32_Working(uint32& _Value) 
{
	_Value = ((_Value & 0xff) << 24) | ((_Value & 0xff00) << 8) | ((_Value & 0xff0000) >> 8) | ((_Value & 0xff000000) >> 24);
}

// template<class T> class TAP_RCD : public TArrayPtr<T, TARRAYPTR_RC_DEBUG> {};

void Hirr(TThinArray<int>& _lDst, const TThinArray<int>& _lSrc)
{
	TAP_RCD<int> pDst = _lDst;
	TAP_RCD<const int> pSrc = _lSrc;

	int Len = Min(pDst.Len(), pSrc.Len());
	for(int i = 0; i < Len; i++)
	{
		pDst[i] = pSrc[i];
	}
}

void TAPTest()
{
	TThinArray<int> lInts;
	TThinArray<int> lInts2;
	lInts.SetLen(3);
	lInts2.SetLen(8);
	Hirr(lInts, lInts2);

	lInts.SetLen(2);
	TArrayPtr<int> pInts = lInts;
	pInts[0] = 0;
	pInts[1] = 1;

	const TThinArray<int>& lIntsRef = lInts;
	TArrayPtr<const int> pInts2 = lIntsRef;

//	int a = pInts[2];
//	pInts2[1] = 1;
}

//#define SIMDTEST

#ifdef SIMDTEST

template <int n, class B, class A1, class A2> struct TMUnroll {
	M_FORCEINLINE static void Iteration(int i, A1 a1, A2 a2) 
	{
		B::Iterate(i,  a1, a2);
		TMUnroll<n-1, B, A1, A2>::Iteration(i + 1, a1, a2);
	}
};

template <class B, class A1, class A2> struct TMUnroll<0, B, A1, A2> {
	M_FORCEINLINE static void Iteration(int i, A1 a1, A2 a2) {  } };


template <int n1, int n2, class B, class A1, class A2, class A3>
struct TMDivideNConqueror {
	enum { nsplit = (n1 + n2) / 2 };

	M_FORCEINLINE static void Iteration(int i1, int i2, A1 a1, A2& a2, 
A3& a3)
	{
		int split = (i1 + i2) / 2;
		A2 left1;
		A3 right1;
		A2 left2;
		A3 right2;

		TMDivideNConqueror<n1, nsplit, B, A1, A2,
A3>::Iteration(i1, split, a1, left1, right1);
		TMDivideNConqueror<nsplit+1, n2, B, A1, A2,
A3>::Iteration(split+1, i2, a1, left2, right2);

		B::Combine(left1, right1, left2, right2, a2, a3);
	}
};

template <int n, class B, class A1, class A2, class A3> struct 
TMDivideNConqueror<n,n,B,A1,A2,A3> {
	M_FORCEINLINE static void Iteration(int i1, int i2, A1 a1, A2& a2, 
A3& a3)
	{
		B::Base(i1,  a1, a2, a3);
	}
};


/*
  Tester
 */


#include "MMath_Vec128.h"

// typedef __vector4 vec128;



CVec3Dfp32 g_lV[8];
CVec3Dfp32 g_lP[4];

//CVec4fa g_lV4[8];

/*CStr M_VStr(vec128 _a)
{
	CVec4Dfp32 a;
	M_VStore(_a, &a);
	return CStrF("(%.10f, %.10f, %.10f, %.10f)", a[0], a[1], a[2], a[3]);
}*/

class COBBvec128
{
public:
	vec128 m_A[3];
	vec128 m_E;
	vec128 m_C;
};



template<class T, int TSize = sizeof(T)>
class TRef
{
public:
	typedef T CRef;
};


template<class T>
class TRef<T, 4>
{
public:
	typedef T& CRef;
};

template<class T>
class TRef<TPtr<T>, 4>
{
public:
	typedef T* CRef;
};


#define M_OPTIMALREF(T) TRef<T >::CRef

CVec4Dfp32 Mummel(vec128 _a, vec128 _b, vec128 _c, vec128p _d)
{
	return M_VAdd(_a, _b);
}

M_FORCEINLINE vec128 M_VSinNRR(vec128 _a)
{
	static fp32 M_ALIGN(16) lSinConst[11][4] = 
		{ 
			{ -0.166666667f, -0.166666667f, -0.166666667f, -0.166666667f },
			{ 8.333333333e-3f, 8.333333333e-3f, 8.333333333e-3f, 8.333333333e-3f },
			{ -1.984126984e-4f, -1.984126984e-4f, -1.984126984e-4f, -1.984126984e-4f },
			{ 2.755731922e-6f, 2.755731922e-6f, 2.755731922e-6f, 2.755731922e-6f },
			{ -2.505210839e-8f, -2.505210839e-8f, -2.505210839e-8f, -2.505210839e-8f },
			{ 1.605904384e-10f, 1.605904384e-10f, 1.605904384e-10f, 1.605904384e-10f },
			{ -7.647163732e-13f, -7.647163732e-13f, -7.647163732e-13f, -7.647163732e-13f },
			{ 2.811457254e-15f, 2.811457254e-15f, 2.811457254e-15f, 2.811457254e-15f },
			{  -8.220635247e-18f,  -8.220635247e-18f,  -8.220635247e-18f,  -8.220635247e-18f },
			{ 1.957294106e-20f, 1.957294106e-20f, 1.957294106e-20f, 1.957294106e-20f },
			{ -3.868170171e-23f, -3.868170171e-23f, -3.868170171e-23f, -3.868170171e-23f }
		};

	static fp32 M_ALIGN(16) Pi[4] = { 3.14159265358979323f, 3.14159265358979323f, 3.14159265358979323f, 3.14159265358979323f };
	static fp32 M_ALIGN(16) PiHalf[4] = { 3.14159265358979323f/2.0f, 3.14159265358979323f/2.0f, 3.14159265358979323f, 3.14159265358979323f/2.0f };

	vec128 pi = M_VLd(&Pi);
	vec128 pihalf = M_VLd(&PiHalf);

//	vec128 pi = M_VPi();
//	vec128 pihalf = M_VScalar(3.14159265358979323f/2.0f);
	fp32 pif = 3.14159265358979323f;
	fp32 pihalff = 3.14159265358979323f/2.0f;

//	vec128 x = M_VModAngles(_a);
	vec128 x = _a;
	vec128 xneg = M_VCmpGTMsk(x, pi);
	x = M_VSelMsk(xneg, M_VSub(x, pi), x);
	vec128 xflip = M_VCmpGTMsk(x, pihalf);
	x = M_VSelMsk(xflip, M_VSub(pi, x), x);

	vec128 x2 = M_VMul(x, x);
	vec128 x3 = M_VMul(x2, x);
	vec128 x5 = M_VMul(x3, x2);
	vec128 x7 = M_VMul(x5, x2);
	vec128 x9 = M_VMul(x7, x2);
	vec128 x11 = M_VMul(x9, x2);
	vec128 x13 = M_VMul(x11, x2);
	vec128 x15 = M_VMul(x13, x2);
	vec128 x17 = M_VMul(x15, x2);
	vec128 x19 = M_VMul(x17, x2);
	vec128 x21 = M_VMul(x19, x2);

	vec128 c0 = M_VOne();
	vec128 c1 = M_VLd(&lSinConst[0]);
	vec128 c2 = M_VLd(&lSinConst[1]);
	vec128 c3 = M_VLd(&lSinConst[2]);
	vec128 c4 = M_VLd(&lSinConst[3]);
	vec128 c5 = M_VLd(&lSinConst[4]);
	vec128 c6 = M_VLd(&lSinConst[5]);
	vec128 c7 = M_VLd(&lSinConst[6]);
	vec128 c8 = M_VLd(&lSinConst[7]);
	vec128 c9 = M_VLd(&lSinConst[8]);
	vec128 c10 = M_VLd(&lSinConst[9]);
	vec128 c11 = M_VLd(&lSinConst[10]);

	vec128 r0 = M_VMAdd(c2, x5, M_VMAdd(c1, x3, x));
	vec128 r2 = M_VMAdd(c4, x9, M_VMul(c3, x7));
	vec128 r4 = M_VMAdd(c6, x13, M_VMul(c5, x11));
	vec128 r6 = M_VMAdd(c8, x17, M_VMul(c7, x15));
//	vec128 r8 = M_VMAdd(c10, x21, M_VMul(c9, x19));

	vec128 s = M_VAdd(M_VAdd(r0, r2), M_VAdd(r4, r6));
//	s = M_VAdd(s, r8);

	s = M_VSelMsk(xneg, M_VNeg(s), s);
	return s;
}

M_FORCEINLINE vec128 M_VSin(vec128 _a)
{
	return M_VSinNRR(M_VWrap2pi(_a));
}

M_FORCEINLINE vec128 M_VCos2(vec128 _a)
{
	vec128 pi = M_VPi();
	vec128 pihalf = M_VScalar(3.14159265358979323f/2.0f);
	fp32 pif = 3.14159265358979323f;
	fp32 pihalff = 3.14159265358979323f/2.0f;

	vec128 x = M_VSub(M_VWrap2pi(M_VAdd(_a, pihalf)), pihalf);
	vec128 xneg = M_VCmpGTMsk(x, pihalf);
	x = M_VSelMsk(xneg, M_VSub(x, pi), x);
	vec128 xflip = M_VCmpLTMsk(x, M_VZero());
	x = M_VSelMsk(xflip, M_VNeg(x), x);

	vec128 x2 = M_VMul(x, x);
	vec128 x4 = M_VMul(x2, x2);
	vec128 x6 = M_VMul(x2, x4);
	vec128 x8 = M_VMul(x4, x4);
	vec128 x10 = M_VMul(x6, x4);
	vec128 x12 = M_VMul(x6, x6);
	vec128 x14 = M_VMul(x6, x8);
	vec128 x16 = M_VMul(x8, x8);
	vec128 x18 = M_VMul(x10, x8);
	vec128 x20 = M_VMul(x10, x10);
	vec128 x22 = M_VMul(x10, x12);
	vec128 x24 = M_VMul(x12, x12);
	vec128 x26 = M_VMul(x14, x12);

	static fp32 M_ALIGN(16) lCosConst[13][4] = 
	{ 
		{ -0.5f,-0.5f,-0.5f,-0.5f },
		{ 4.166666667e-2f,4.166666667e-2f,4.166666667e-2f,4.166666667e-2f },			// 1/4!
		{ -1.388888889e-3f,-1.388888889e-3f,-1.388888889e-3f,-1.388888889e-3f },		// -1/6!
		{ 2.480158730e-5f,2.480158730e-5f,2.480158730e-5f,2.480158730e-5f },			// 1/8!
		{ -2.755731922e-7f,-2.755731922e-7f,-2.755731922e-7f,-2.755731922e-7f },		// -1/10!
		{ 2.087675699e-9f,2.087675699e-9f,2.087675699e-9f,2.087675699e-9f },			// 1/12!
		{ -1.147074560e-11f,-1.147074560e-11f,-1.147074560e-11f,-1.147074560e-11f },	// -1/14!
		{ 4.779477332e-14f,4.779477332e-14f,4.779477332e-14f,4.779477332e-14f },		// 1/16!
		{ -1.561920697e-16f,-1.561920697e-16f,-1.561920697e-16f,-1.561920697e-16f },	// -1/18!
		{ 4.110317623e-19f,4.110317623e-19f,4.110317623e-19f,4.110317623e-19f },		// 1/20!
		{ -8.896791392e-22f,-8.896791392e-22f,-8.896791392e-22f,-8.896791392e-22f },	// -1/22!
		{ 1.6117375710961e-24,1.6117375710961e-24,1.6117375710961e-24,1.6117375710961e-24 },// 1/24!
		{ -2.479596263224e-27,2.479596263224e-27,2.479596263224e-27,2.479596263224e-27 }// -1/26!
	};

	vec128 c0 = M_VOne();
	vec128 c1 = M_VLd(&lCosConst[0]);
	vec128 c2 = M_VLd(&lCosConst[1]);
	vec128 c3 = M_VLd(&lCosConst[2]);
	vec128 c4 = M_VLd(&lCosConst[3]);
	vec128 c5 = M_VLd(&lCosConst[4]);
	vec128 c6 = M_VLd(&lCosConst[5]);
	vec128 c7 = M_VLd(&lCosConst[6]);
	vec128 c8 = M_VLd(&lCosConst[7]);
	vec128 c9 = M_VLd(&lCosConst[8]);
	vec128 c10 = M_VLd(&lCosConst[9]);
	vec128 c11 = M_VLd(&lCosConst[10]);
	vec128 c12 = M_VLd(&lCosConst[11]);
	vec128 c13 = M_VLd(&lCosConst[12]);

	/*	XMGLOBALCONST XMVECTOR  g_XMCosCoefficients0    = {1.0f, -0.5f, 4.166666667e-2f, -1.388888889e-3f};
	XMGLOBALCONST XMVECTOR  g_XMCosCoefficients1    = {2.480158730e-5f, -2.755731922e-7f, 2.087675699e-9f, -1.147074560e-11f};
	XMGLOBALCONST XMVECTOR  g_XMCosCoefficients2    = {4.779477332e-14f, -1.561920697e-16f, 4.110317623e-19f, -8.896791392e-22f};
	*/

	vec128 r0 = M_VMAdd(c2, x4, M_VMAdd(c1, x2, c0));
	vec128 r2 = M_VMAdd(c3, x6, M_VMul(c4, x8));
	vec128 r4 = M_VMAdd(c5, x10, M_VMul(c6, x12));
	vec128 r6 = M_VMAdd(c7, x14, M_VMul(c8, x16));
//	vec128 r8 = M_VMAdd(c9, x18, M_VMul(c10, x20));
//	vec128 r10 = M_VMAdd(c11, x22, M_VMul(c12, x24));

//	vec128 c = M_VAdd(M_VAdd(M_VAdd(r0, r2), M_VAdd(r4, r6)), M_VAdd(r8, r10));
	vec128 c = M_VAdd(M_VAdd(r0, r2), M_VAdd(r4, r6));
//	vec128 c = M_VAdd(r0, r2);

/*	vec128 r0 = M_VMAdd(c1, x2, c0);
	r0 = M_VMAdd(c2, x4, r0);
	r0 = M_VMAdd(c3, x6, r0);
	vec128 r4 = M_VMul(c5, x10);
	r4 = M_VMAdd(c6, x12, r4);
	r4 = M_VMAdd(c7, x14, r4);
	vec128 r3 = M_VMul(c4, x8);
	r3 = M_VMAdd(c8, x16, r3);
	vec128 r8 = M_VMul(c9, x18);
	r8 = M_VMAdd(c10, x20, r8);
	r8 = M_VMAdd(c11, x22, r8);

//	r8 = M_VMAdd(c12, x24, r8);
//	r8 = M_VMAdd(c13, x26, r8);

	r0 = M_VAdd(r0, r3);
	r4 = M_VAdd(r4, r8);

	vec128 c = M_VAdd(r0, r4);*/
	c = M_VSelMsk(xneg, M_VNeg(c), c);
	return c;
}

// -------------------------------------------------------------------
static M_INLINE void Krick_Interpolate2(const CMat4Dfp32& _Pos, const CMat4Dfp32& _Pos2, CMat4Dfp32& _Dest, fp32 t)
{
	MAUTOSTRIP(Interpolate2, MAUTOSTRIP_VOID);
	if (CVec3Dfp32::GetMatrixRow(_Pos, 3).DistanceSqr(CVec3Dfp32::GetMatrixRow(_Pos2, 3)) > Sqr(256.0f))
	{
		_Dest = _Pos;
	}
/*else if(!memcmp(&_Pos, &_Pos2, sizeof(_Pos)))
	{
		_Dest = _Pos;
	}*/
	else
	{
		fp32 tpos = ClampRange(t, 2.5f);
		t = ClampRange(t, 1.5f);
		_Dest.UnitNot3x3();
		CVec3Dfp32 v3 = CVec3Dfp32::GetMatrixRow(_Pos2, 3) - CVec3Dfp32::GetMatrixRow(_Pos, 3);
		CVec3Dfp32::GetMatrixRow(_Dest, 3) = CVec3Dfp32::GetMatrixRow(_Pos, 3) + v3*tpos;

		CVec3Dfp32 v0 = CVec3Dfp32::GetMatrixRow(_Pos2, 0) - CVec3Dfp32::GetMatrixRow(_Pos, 0);
		CVec3Dfp32::GetMatrixRow(_Dest, 0) = (CVec3Dfp32::GetMatrixRow(_Pos, 0) + v0*t).Normalize();
		CVec3Dfp32 v1 = CVec3Dfp32::GetMatrixRow(_Pos2, 1) - CVec3Dfp32::GetMatrixRow(_Pos, 1);
		CVec3Dfp32::GetMatrixRow(_Dest, 2) = -((CVec3Dfp32::GetMatrixRow(_Pos, 1) + v1*t) / CVec3Dfp32::GetMatrixRow(_Dest, 0)).Normalize();
		CVec3Dfp32::GetMatrixRow(_Dest, 1) = CVec3Dfp32::GetMatrixRow(_Dest, 2) / CVec3Dfp32::GetMatrixRow(_Dest, 0);
	}
}

__declspec(noalias)  M_FORCEINLINE void vec128_Interpolate2(const CMat4Dfp32& _Pos, const CMat4Dfp32& _Pos2, CMat4Dfp32 *M_RESTRICT _Dest, const fp32& _Time)		// fp32& because we want the option to avoid LHS.
{
	vec128 p0x = _Pos.r[0];	vec128 p0y = _Pos.r[1];	vec128 p0z = _Pos.r[2];	vec128 p0w = _Pos.r[3];
	vec128 p1x = _Pos2.r[0];vec128 p1y = _Pos2.r[1];vec128 p1z = _Pos2.r[2];vec128 p1w = _Pos2.r[3];

	vec128 t = M_VLdScalar(_Time);
	vec128 z = M_VZero();
	vec128 tpos = M_VClamp(t, z, M_VScalar(2.5f));
	vec128 trot = M_VClamp(t, z, M_VScalar(1.5f));

	vec128 dstw = M_VLrp(p0w, p1w, tpos);
	vec128 dstx = M_VLrp(p0x, p1x, trot);
	vec128 dstytmp = M_VLrp(p0y, p1y, trot);
	vec128 dstz = M_VXpd(dstx, dstytmp);
	vec128 dsty = M_VXpd(dstz, dstx);
	M_VNrm3x3(dstx, dsty, dstz);

	vec128 dmove = M_VSub(p0w, p1w);
	vec128 teleportmask = M_VCmpGTMsk(M_VDp3(dmove, dmove), M_VScalar(256.0f*256.0f));

	dstx = M_VSelMsk(teleportmask, p1x, dstx);
	dsty = M_VSelMsk(teleportmask, p1y, dsty);
	dstz = M_VSelMsk(teleportmask, p1z, dstz);
	dstw = M_VSelMsk(teleportmask, p1w, dstw);

	_Dest->r[0] = dstx;	_Dest->r[1] = dsty;	_Dest->r[2] = dstz;	_Dest->r[3] = dstw;
}

#define vec128_Interpolate2b(_Pos, _Pos2, _Dest, _Time)	\
{	\
	vec128 p0x = _Pos.r[0];	vec128 p0y = _Pos.r[1];	vec128 p0z = _Pos.r[2];	vec128 p0w = _Pos.r[3];		\
	vec128 p1x = _Pos2.r[0];vec128 p1y = _Pos2.r[1];vec128 p1z = _Pos2.r[2];vec128 p1w = _Pos2.r[3];	\
	vec128 vt = M_VLdScalar(_Time);	\
	vec128 z = M_VZero();	\
	vec128 tpos = M_VClamp(vt, z, M_VScalar(2.5f));	\
	vec128 trot = M_VClamp(vt, z, M_VScalar(1.5f));	\
	vec128 dstw = M_VLrp(p0w, p1w, tpos);	\
	vec128 dstx = M_VLrp(p0x, p1x, trot);	\
	vec128 dstytmp = M_VLrp(p0y, p1y, trot);	\
	vec128 dstz = M_VXpd(dstx, dstytmp);	\
	vec128 dsty = M_VXpd(dstz, dstx);	\
	M_VNrm3x3(dstx, dsty, dstz);	\
	vec128 dmove = M_VSub(p0w, p1w);	\
	vec128 teleportmask = M_VCmpGTMsk(M_VDp3(dmove, dmove), M_VScalar(256.0f*256.0f));	\
	dstx = M_VSelMsk(teleportmask, p1x, dstx);	\
	dsty = M_VSelMsk(teleportmask, p1y, dsty);	\
	dstz = M_VSelMsk(teleportmask, p1z, dstz);	\
	dstw = M_VSelMsk(teleportmask, p1w, dstw);	\
	_Dest.r[0] = dstx;	_Dest.r[1] = dsty;	_Dest.r[2] = dstz;	_Dest.r[3] = dstw;	\
}

M_FORCEINLINE void vec128_Interpolate3(const vec128 _Pos[4], const vec128 _Pos2[4], vec128 _Dest[4], const fp32& _Time)		// fp32& because we want the option to avoid LHS.
{
	vec128 p0x = _Pos[0];	vec128 p0y = _Pos[1];	vec128 p0z = _Pos[2];	vec128 p0w = _Pos[3];
	vec128 p1x = _Pos2[0];vec128 p1y = _Pos2[1];vec128 p1z = _Pos2[2];vec128 p1w = _Pos2[3];

	vec128 t = M_VLdScalar(_Time);
	vec128 z = M_VZero();
	vec128 tpos = M_VClamp(t, z, M_VScalar(2.5f));
	vec128 trot = M_VClamp(t, z, M_VScalar(1.5f));

	vec128 dstw = M_VLrp(p0w, p1w, tpos);
	vec128 dstx = M_VLrp(p0x, p1x, trot);
	vec128 dstytmp = M_VLrp(p0y, p1y, trot);
	vec128 dstz = M_VXpd(dstx, dstytmp);
	vec128 dsty = M_VXpd(dstz, dstx);
	M_VNrm3x3(dstx, dsty, dstz);

	vec128 dmove = M_VSub(p0w, p1w);
	vec128 teleportmask = M_VCmpGTMsk(M_VDp3(dmove, dmove), M_VScalar(256.0f*256.0f));

	dstx = M_VSelMsk(teleportmask, p1x, dstx);
	dsty = M_VSelMsk(teleportmask, p1y, dsty);
	dstz = M_VSelMsk(teleportmask, p1z, dstz);
	dstw = M_VSelMsk(teleportmask, p1w, dstw);

	_Dest[0] = dstx;	_Dest[1] = dsty;	_Dest[2] = dstz;	_Dest[3] = dstw;
}
/*
M_FORCEINLINE void vec128_Interpolate4(
	vec128 p0x, vec128 p0y, vec128 p0z, vec128 p0w, 
	vec128 p1x, vec128 p1y, vec128 p1z, vec128 p1w, 
	vec128& _dstx, vec128& _dsty, vec128& _dstz, vec128& _dstw, const fp32& _Time)		// fp32& because we want the option to avoid LHS.
{
	vec128 t = M_VLdScalar(_Time);
	vec128 z = M_VZero();
	vec128 tpos = M_VClamp(t, z, M_VScalar(2.5f));
	vec128 trot = M_VClamp(t, z, M_VScalar(1.5f));

	vec128 dstw = M_VLrp(p0w, p1w, tpos);
	vec128 dstx = M_VLrp(p0x, p1x, trot);
	vec128 dstytmp = M_VLrp(p0y, p1y, trot);
	vec128 dstz = M_VXpd(dstx, dstytmp);
	vec128 dsty = M_VXpd(dstz, dstx);
	M_VNrm3x3(dstx, dsty, dstz);

	vec128 dmove = M_VSub(p0w, p1w);
	vec128 teleportmask = M_VCmpGTMsk(M_VDp3(dmove, dmove), M_VScalar(256.0f*256.0f));

	dstx = M_VSelMsk(teleportmask, p1x, dstx);
	dsty = M_VSelMsk(teleportmask, p1y, dsty);
	dstz = M_VSelMsk(teleportmask, p1z, dstz);
	dstw = M_VSelMsk(teleportmask, p1w, dstw);

	_dstx = dstx;	_dsty = dsty;	_dstz = dstz;	_dstw = dstw;
}*/

void Test_krick(CMat4Dfp32& M, CMat4Dfp32& M2, CMat4Dfp32& M3, const fp32& t)
{
	for(int i = 0; i < 1000; i++)
	{
		Krick_Interpolate2(M, M2, M3, t);
		M = M3;
	}
}

void Test_v128(CMat4Dfp32& M, CMat4Dfp32& M2, CMat4Dfp32& M3, const fp32& t)
{
	for(int i = 0; i < 1000; i++)
	{
		vec128_Interpolate2(M, M2, &M3, t);
		M = M3;
	}
}

/*struct CMat4
{
public:
	vec128 r[4];

	CMat4& operator= (const CMat4& _Mat)
	{
		r[0] = _Mat.r[0];
		r[1] = _Mat.r[1];
		r[2] = _Mat.r[2];
		r[3] = _Mat.r[3];
		return *this;
	}

};*/

// _DECLSPEC_ALIGN_16_ 
//typedef struct _CMat4
class CMat4
{
public:
//	union
//	{
		vec128 r[4];
/*		struct
		{
			FLOAT _11, _12, _13, _14;
			FLOAT _21, _22, _23, _24;
			FLOAT _31, _32, _33, _34;
			FLOAT _41, _42, _43, _44;
		};
		FLOAT m[4][4];*/
//	};


	CMat4&  operator= (const CMat4& _Mat)
	{
		r[0] = _Mat.r[0];
		r[1] = _Mat.r[1];
		r[2] = _Mat.r[2];
		r[3] = _Mat.r[3];
		return *this;
	}

};// CMat4;

/*M_FORCEINLINE _CMat4& _CMat4::operator= (const _CMat4& _Mat)
{
	r[0] = _Mat.r[0];
	r[1] = _Mat.r[1];
	r[2] = _Mat.r[2];
	r[3] = _Mat.r[3];
	return *this;
}*/


M_FORCEINLINE void vec128_Interpolate5(const CMat4& _Pos, const CMat4& _Pos2, CMat4& _Dest, const fp32& _Time)		// fp32& because we want the option to avoid LHS.
{
	vec128 p0x = _Pos.r[0];	vec128 p0y = _Pos.r[1];	vec128 p0z = _Pos.r[2];	vec128 p0w = _Pos.r[3];
	vec128 p1x = _Pos2.r[0];vec128 p1y = _Pos2.r[1];vec128 p1z = _Pos2.r[2];vec128 p1w = _Pos2.r[3];

	vec128 t = M_VLdScalar(_Time);
	vec128 z = M_VZero();
	vec128 tpos = M_VClamp(t, z, M_VScalar(2.5f));
	vec128 trot = M_VClamp(t, z, M_VScalar(1.5f));

	vec128 dstw = M_VLrp(p0w, p1w, tpos);
	vec128 dstx = M_VLrp(p0x, p1x, trot);
	vec128 dstytmp = M_VLrp(p0y, p1y, trot);
	vec128 dstz = M_VXpd(dstx, dstytmp);
	vec128 dsty = M_VXpd(dstz, dstx);
	M_VNrm3x3(dstx, dsty, dstz);

	vec128 dmove = M_VSub(p0w, p1w);
	vec128 teleportmask = M_VCmpGTMsk(M_VDp3(dmove, dmove), M_VScalar(256.0f*256.0f));

	dstx = M_VSelMsk(teleportmask, p1x, dstx);
	dsty = M_VSelMsk(teleportmask, p1y, dsty);
	dstz = M_VSelMsk(teleportmask, p1z, dstz);
	dstw = M_VSelMsk(teleportmask, p1w, dstw);

	_Dest.r[0] = dstx;	_Dest.r[1] = dsty;	_Dest.r[2] = dstz;	_Dest.r[3] = dstw;
}

M_FORCEINLINE CMat4 vec128_Interpolate5b(const CMat4& _Pos, const CMat4& _Pos2, const fp32& _Time)		// fp32& because we want the option to avoid LHS.
{
	vec128 p0x = _Pos.r[0];	vec128 p0y = _Pos.r[1];	vec128 p0z = _Pos.r[2];	vec128 p0w = _Pos.r[3];
	vec128 p1x = _Pos2.r[0];vec128 p1y = _Pos2.r[1];vec128 p1z = _Pos2.r[2];vec128 p1w = _Pos2.r[3];

	vec128 t = M_VLdScalar(_Time);
	vec128 z = M_VZero();
	vec128 tpos = M_VClamp(t, z, M_VScalar(2.5f));
	vec128 trot = M_VClamp(t, z, M_VScalar(1.5f));

	vec128 dstw = M_VLrp(p0w, p1w, tpos);
	vec128 dstx = M_VLrp(p0x, p1x, trot);
	vec128 dstytmp = M_VLrp(p0y, p1y, trot);
	vec128 dstz = M_VXpd(dstx, dstytmp);
	vec128 dsty = M_VXpd(dstz, dstx);
	M_VNrm3x3(dstx, dsty, dstz);

	vec128 dmove = M_VSub(p0w, p1w);
	vec128 teleportmask = M_VCmpGTMsk(M_VDp3(dmove, dmove), M_VScalar(256.0f*256.0f));

	dstx = M_VSelMsk(teleportmask, p1x, dstx);
	dsty = M_VSelMsk(teleportmask, p1y, dsty);
	dstz = M_VSelMsk(teleportmask, p1z, dstz);
	dstw = M_VSelMsk(teleportmask, p1w, dstw);

	CMat4 _Dest;
	_Dest.r[0] = dstx;	_Dest.r[1] = dsty;	_Dest.r[2] = dstz;	_Dest.r[3] = dstw;
	return _Dest;
}

__declspec(noalias) M_FORCEINLINE CMat4Dfp32 vec128_Interpolate7b(const CMat4Dfp32& _Pos, const CMat4Dfp32& _Pos2, const fp32& _Time)		// fp32& because we want the option to avoid LHS.
{
	vec128 p0x = _Pos.r[0];	vec128 p0y = _Pos.r[1];	vec128 p0z = _Pos.r[2];	vec128 p0w = _Pos.r[3];
	vec128 p1x = _Pos2.r[0];vec128 p1y = _Pos2.r[1];vec128 p1z = _Pos2.r[2];vec128 p1w = _Pos2.r[3];

	vec128 t = M_VLdScalar(_Time);
	vec128 z = M_VZero();
	vec128 tpos = M_VClamp(t, z, M_VScalar(2.5f));
	vec128 trot = M_VClamp(t, z, M_VScalar(1.5f));

	vec128 dstw = M_VLrp(p0w, p1w, tpos);
	vec128 dstx = M_VLrp(p0x, p1x, trot);
	vec128 dstytmp = M_VLrp(p0y, p1y, trot);
	vec128 dstz = M_VXpd(dstx, dstytmp);
	vec128 dsty = M_VXpd(dstz, dstx);
	M_VNrm3x3(dstx, dsty, dstz);

	vec128 dmove = M_VSub(p0w, p1w);
	vec128 teleportmask = M_VCmpGTMsk(M_VDp3(dmove, dmove), M_VScalar(256.0f*256.0f));

	dstx = M_VSelMsk(teleportmask, p1x, dstx);
	dsty = M_VSelMsk(teleportmask, p1y, dsty);
	dstz = M_VSelMsk(teleportmask, p1z, dstz);
	dstw = M_VSelMsk(teleportmask, p1w, dstw);

	CMat4Dfp32 _Dest;
	_Dest.r[0] = dstx;	_Dest.r[1] = dsty;	_Dest.r[2] = dstz;	_Dest.r[3] = dstw;
	return _Dest;
}
__declspec(noalias)  M_FORCEINLINE void vec128_Interpolate8(const CMat4Dfp32& _Pos, const CMat4Dfp32& _Pos2, 
	vec128& _dstx, vec128& _dsty, vec128& _dstz, vec128& _dstw, const fp32& _Time)		// fp32& because we want the option to avoid LHS.
{
	vec128 p0x = _Pos.r[0];	vec128 p0y = _Pos.r[1];	vec128 p0z = _Pos.r[2];	vec128 p0w = _Pos.r[3];
	vec128 p1x = _Pos2.r[0];vec128 p1y = _Pos2.r[1];vec128 p1z = _Pos2.r[2];vec128 p1w = _Pos2.r[3];

	vec128 t = M_VLdScalar(_Time);
	vec128 z = M_VZero();
	vec128 tpos = M_VClamp(t, z, M_VScalar(2.5f));
	vec128 trot = M_VClamp(t, z, M_VScalar(1.5f));

	vec128 dstw = M_VLrp(p0w, p1w, tpos);
	vec128 dstx = M_VLrp(p0x, p1x, trot);
	vec128 dstytmp = M_VLrp(p0y, p1y, trot);
	vec128 dstz = M_VXpd(dstx, dstytmp);
	vec128 dsty = M_VXpd(dstz, dstx);
	M_VNrm3x3(dstx, dsty, dstz);

	vec128 dmove = M_VSub(p0w, p1w);
	vec128 teleportmask = M_VCmpGTMsk(M_VDp3(dmove, dmove), M_VScalar(256.0f*256.0f));

	dstx = M_VSelMsk(teleportmask, p1x, dstx);
	dsty = M_VSelMsk(teleportmask, p1y, dsty);
	dstz = M_VSelMsk(teleportmask, p1z, dstz);
	dstw = M_VSelMsk(teleportmask, p1w, dstw);

	_dstx = dstx; _dsty = dsty; _dstz = dstz; _dstw = dstw; 
}

/*#include "XboxMath.h"

M_FORCEINLINE XMMATRIX vec128_Interpolate6(XMMATRIX _Pos, XMMATRIX _Pos2, fp32 _Time)		// fp32& because we want the option to avoid LHS.
{
	vec128 p0x = _Pos.r[0];	vec128 p0y = _Pos.r[1];	vec128 p0z = _Pos.r[2];	vec128 p0w = _Pos.r[3];
	vec128 p1x = _Pos2.r[0];vec128 p1y = _Pos2.r[1];vec128 p1z = _Pos2.r[2];vec128 p1w = _Pos2.r[3];

	vec128 t = M_VLdScalar(_Time);
	vec128 z = M_VZero();
	vec128 tpos = M_VClamp(t, z, M_VScalar(2.5f));
	vec128 trot = M_VClamp(t, z, M_VScalar(1.5f));

	vec128 dstw = M_VLrp(p0w, p1w, tpos);
	vec128 dstx = M_VLrp(p0x, p1x, trot);
	vec128 dstytmp = M_VLrp(p0y, p1y, trot);
	vec128 dstz = M_VXpd(dstx, dstytmp);
	vec128 dsty = M_VXpd(dstz, dstx);
	M_VNrm3x3(dstx, dsty, dstz);

	vec128 dmove = M_VSub(p0w, p1w);
	vec128 teleportmask = M_VCmpGTMsk(M_VDp3(dmove, dmove), M_VScalar(256.0f*256.0f));

	dstx = M_VSelMsk(teleportmask, p1x, dstx);
	dsty = M_VSelMsk(teleportmask, p1y, dsty);
	dstz = M_VSelMsk(teleportmask, p1z, dstz);
	dstw = M_VSelMsk(teleportmask, p1w, dstw);

	XMMATRIX _Dest;
	_Dest.r[0] = dstx;	_Dest.r[1] = dsty;	_Dest.r[2] = dstz;	_Dest.r[3] = dstw;
	return _Dest;
}*/

/*M_FORCEINLINE void M_VCmpAllGE(vec128 _a, vec128 _b, uint32* _pBool)
{
	vec128 msk = __vcmpgefp(_a, _b);
	vec128 and = M_VAnd(M_VMrgXY(msk, msk), M_VMrgZW(msk, msk));	// xxyy | zzww = xz xz yw yw
	and = M_VAnd(M_VMrgXY(and, and), M_VMrgZW(and, and));
	M_VStAny32(and, _pBool);
}*/

M_FORCEINLINE void M_VStAny64r(vec128 _a, void *M_RESTRICT _pDest)
{ 
	void*M_RESTRICT pDest = _pDest;
	__stvewx(_a, pDest, 0); 
	__stvewx(_a, pDest, 4); 
}

void VecStore64(__vector4 _v, void * __restrict _pDest)
{
	void * __restrict pDest = _pDest;
	__stvewx(_v, pDest, 0);
	__stvewx(_v, pDest, 4); 
}

template<class T>
void VecStore64Ref(__vector4 _v, T* _pDest)
{
	__stvewx(_v, _pDest, 0);
	__stvewx(_v, _pDest, 4); 
}

void SIMDTest()
{
//	MassiveAdClient3::CMassiveClientCore* pMCC = MassiveAdClient3::CMassiveClientCore::Instance();

	{
		vec128 dest;
//		void* pDest = &dest;
//		VecStore64(M_VZero(), &dest);
		M_VStAny64(M_VZero(), &dest);
		M_TRACEALWAYS("dest = %f, %f, %f, %f\n", dest.x, dest.y, dest.z, dest.w);
	}


	CVec3Dfp32 hirr;
	M_OPTIMALREF(CVec3Dfp32) hirrref = hirr;
	M_OPTIMALREF(TPtr<CVec3Dfp32>) spHirrref = &hirr;

/*	__m128 a,b,c;
	a = _mm_set_ss(0);
	b = _mm_set_ss(0);
	c = _mm_add_ps(a, b);
	LogFile(CStrF("%f, %f, %f, %f", c.m128_f32[0], c.m128_f32[1], c.m128_f32[2], c.m128_f32[3]));
*/
#if 1

/*	Memory alignment test

	TThinArray<CVec3Dfp32> lV3;
	TThinArray<CVec4Dfp32> lV4;
	lV3.SetLen(5);
	lV4.SetLen(5);

	M_TRACEALWAYS("pV3 %.8x\n", lV3.GetBasePtr());
	M_TRACEALWAYS("pV4 %.8x\n", lV4.GetBasePtr());

	{
		for(int i = 0; i < 1000; i++)
		{
			void* p = MRTC_MemAlloc(RoundToInt(Random*7867)+1);
			if (mint(p) & 0x0f)
			{
				M_TRACEALWAYS("Unaligned alloc %.8x\n", p);
			}
		}

	}
	for(int q = 1; q < 4; q++)
	{
		int* pA = (int*)MRTC_MemAlloc(q*4);
		int* pB = (int*)MRTC_MemAlloc(q*4);
		int* pC = (int*)MRTC_MemAlloc(q*4);
		M_TRACEALWAYS(CStrF("%.8x, %.8x, %.8x, %.8x\n", pA, pB, pC));
	}
	*/


	vec128 a = M_VLd(2,3,4,5);
	vec128 b = M_VLd(2,2,2,2);
	vec128 c = M_VLd(5,6,8,9);
	vec128 d = M_VLd(10,2,2,2);

/*
	a = M_VSelComp(0, d, c);
	a = M_VRsq_Est(M_VNeg(M_VZero()));
	a = M_VRcp_Est(M_VNeg(M_VZero()));

	M_TRACEALWAYS("a: %s\n", M_VStr(a).Str() );
	M_TRACEALWAYS("NaN: %s\n", M_VStr(M_VNaN()).Str() );
	M_TRACEALWAYS("Inf: %s\n", M_VStr(M_VInf()).Str() );

	CQuatfp32 qa,qb,qc;
	qa.v = a;
	qb.v = b;
	qa.Multiply(qb, qc);
	M_TRACEALWAYS("Real quat: %s\n", M_VStr(qc.v).Str() );

	c = M_VQuatMul(a, b);
	M_TRACEALWAYS("v128 quat: %s\n", M_VStr(c).Str() );

*/
	CMat4Dfp32 M, M2, M3;
	M.r[0] = a;
	M.r[1] = b;
	M.r[2] = c;
	M.r[3] = d;

	M2.r[0] = M_VLd(0.1, 0.2, 0.3, 3.4);
	M2.r[1] = M_VLd(1.1, 2.2, 1.3, 2.4);
	M2.r[2] = M_VLd(2.1, 3.2, 2.3, 1.4);
	M2.r[3] = M_VLd(3.1, 0.2, 3.3, 0.4);

	M3.r[0] = M_VLd(0.3, 0.5, 0.7, 3.1);
	M3.r[1] = M_VLd(1.3, 2.5, 1.7, 2.1);
	M3.r[2] = M_VLd(2.3, 3.5, 2.7, 1.1);
	M3.r[3] = M_VLd(3.3, 0.5, 3.7, 0.1);

	CMTime T, T2;
	CVec3Dfp32 M_ALIGN(16) v3arr[4];
//	CVec3Dfp32 va,vb,vc,vd;
	v3arr[0] = M_VGetV3_Slow(a);
	v3arr[1] = M_VGetV3_Slow(b);
	v3arr[2] = M_VGetV3_Slow(c);
	v3arr[3] = M_VGetV3_Slow(d);
/*	M_VStore(a, &va);
	M_VStore(b, &vb);
	M_VStore(c, &vc);
	M_VStore(d, &vd);
*/
	T2.Start();
	{
//		CAxisRotfp32 Axis(v3arr[0], 0.0f);
//		CQuatfp32 Q;

		fp32 t = 0.1524334f;
	
		for(int i = 0; i < 1000; i++)
		{
//			Axis.CreateQuaternion(Q);
//			Axis.m_Axis.k[0] = Q.k[0];
			Krick_Interpolate2(M, M2, M3, t);
			M = M3;

//			v3arr[0].Normalize();
//			v3arr[1].Normalize();
//			v3arr[2].Normalize();
//			v3arr[3].Normalize();
		}
	}
	T2.Stop();
/*
	T2.Start();
	fp32 t = 0.1524334f;
	Test_krick(M, M2, M3, t);
	T2.Stop();
*/
/*	a = M_VLd(va);
	b = M_VLd(vb);
	c = M_VLd(vc);
	d = M_VLd(vd);*/


	static fp32 M_ALIGN(16) lFloats[32] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31 };
	M_VLd_V3x4(lFloats, a, b, c, d);
	M_TRACEALWAYS(CStrF("a = %s\n", M_VStr(a).Str() ));
	M_TRACEALWAYS(CStrF("b = %s\n", M_VStr(b).Str() ));
	M_TRACEALWAYS(CStrF("c = %s\n", M_VStr(c).Str() ));
	M_TRACEALWAYS(CStrF("d = %s\n", M_VStr(d).Str() ));

	M_VSt_V3x4(v3arr, a, b, c, d);
	M_TRACEALWAYS(CStrF("v0 = %s\n", v3arr[0].GetString().Str() ));
	M_TRACEALWAYS(CStrF("v1 = %s\n", v3arr[1].GetString().Str() ));
	M_TRACEALWAYS(CStrF("v2 = %s\n", v3arr[2].GetString().Str() ));
	M_TRACEALWAYS(CStrF("v3 = %s\n", v3arr[3].GetString().Str() ));

/*	{
		vec128* pV128 = (vec128*)lFloats;
		vec128 select = g_SelectXYZ;
		vec128 ta = __lvx(&pV128[0], 0);
		vec128 tb = M_VOr(__lvlx(&pV128[0], 12), __lvrx(&pV128[1], 12));
		vec128 tc = M_VOr(__lvlx(&pV128[1], 8), __lvrx(&pV128[2], 8));
		vec128 td = __lvlx(&pV128[2], 4);
		vec128 z = M_VZero();
		ta = M_VSel(select, ta, z);	
		tb = M_VSel(select, tb, z);	
		tc = M_VSel(select, tc, z);	
		a = ta; b = tb; c = tc; d = td;	
	}*/



/*
	a = M_VLd(&lFloats[1]);
	b = M_VLd(&lFloats[2]);
	c = M_VLd(&lFloats[3]);
	d = M_VLd(&lFloats[4]);
*/
	CVec4Dfp32 av;
	av.v = a;

	uint32 j = 0;
	T.Start();
	{
		fp32 t = 0.1524334f;
//		Test_v128(M, M2, M3, t);
#if 1
/*		CMat4 MT1 = { M.r[0], M.r[1], M.r[2], M.r[3] };
		CMat4 MT2 = { M2.r[0], M2.r[1], M2.r[2], M2.r[3] };
		CMat4 MT3 = { M3.r[0], M3.r[1], M3.r[2], M3.r[3] };*/
/*		XMMATRIX MM1 = (XMMATRIX&) M;
		XMMATRIX MM2 = (XMMATRIX&) M2;
		XMMATRIX MM3 = (XMMATRIX&) M3;*/
		CMat4Dfp32 MT1 = M;
		CMat4Dfp32 MT2 = M2;
		CMat4Dfp32 MT3 = M3;
		fp32 fa = 1.239878f;
		fp32 fb = 0.87187283f;
		fp32 fc = 76263723.837637842f;
/*		vec128 va = M_VLdScalar(fa);
		vec128 vb = M_VLdScalar(fb);
		vec128 vc = M_VLdScalar(fc);
		vec128 vj = M_VLdScalar_u32(j);*/
		for(int i = 0; i < 1000; i++)
		{
	//		a = M_VNrm3(a);
	/*		b = M_VNrm3(b);
			c = M_VNrm3(c);
			d = M_VNrm3(d);
	*/
//			M_VNrm3x4(a, b, c, d);

	/*		vec128 a0 = M_VMulMat4x3(a, M);
			vec128 a1 = M_VMulMat4x3(a0, M2);
			a = M_VMulMat4x3(a1, M3);*/
//			a = M_VSinNRR(a);
//			M_VCnv_AA_Quat_x4(a, b, c, d, a, b, c, d);
//			M_VMatLrpOrthNrm(M, M2, M3, t);
//			vec128_Interpolate5(MT1, MT2, MT3, t);
//			MT1 = MT3;
//			MT1 = vec128_Interpolate5b(MT1, MT2, t);
//			MT1 = vec128_Interpolate7b(MT1, MT2, t);
//			vec128_Interpolate2b(MT1, MT2, MT3, t);
//			vec128_Interpolate8(MT1, MT2, MT3.r[0], MT3.r[1], MT3.r[2], MT3.r[3], t);
//			MT1 = MT3;

/*			b = M_VAdd(b, c);
			if (M_VCmpAllGE(a, b))
				j++;*/
			fa += fb;
			if (fa > fc)
				j++;
/*			va = M_VAdd(va, vb);
			vec128 msk = M_VCmpGTMsk(va, vc);
			vj = M_VSelMsk(msk, M_VAdd_u32(vj, M_VScalar_u32(1)), vj);
*/			
			//			MM1 = vec128_Interpolate6(MM1, MM2, t);

//			MM1 = XMMatrixMultiply(MM1, MM2);
//			M_VMatMul(MT1, MT2, MT3);
			
/*			MT1.r[0] = MT3.r[0];
			MT1.r[1] = MT3.r[1];
			MT1.r[2] = MT3.r[2];
			MT1.r[3] = MT3.r[3];*/

//			av.v = M_VMulMat4x3(M_VMulMat4x3(M_VMulMat4x3(av.v, M), M2), M3);
		}
//		M_VStAny32(vj, &j);

#endif
#if 0
		vec128 MT1x = M.r[0]; vec128 MT1y = M.r[1]; vec128 MT1z = M.r[2]; vec128 MT1w = M.r[3]; 
		vec128 MT2x = M2.r[0]; vec128 MT2y = M2.r[1]; vec128 MT2z = M2.r[2]; vec128 MT2w = M2.r[3]; 
		vec128 MT3x = M3.r[0]; vec128 MT3y = M3.r[1]; vec128 MT3z = M3.r[2]; vec128 MT3w = M3.r[3]; 
		for(int i = 0; i < 1000; i++)
		{
			//		a = M_VNrm3(a);
			/*		b = M_VNrm3(b);
			c = M_VNrm3(c);
			d = M_VNrm3(d);
			*/
			//			M_VNrm3x4(a, b, c, d);

			/*		vec128 a0 = M_VMulMat4x3(a, M);
			vec128 a1 = M_VMulMat4x3(a0, M2);
			a = M_VMulMat4x3(a1, M3);*/
			//			a = M_VSinNRR(a);
			//			M_VCnv_AA_Quat_x4(a, b, c, d, a, b, c, d);
			//			M_VMatLrpOrthNrm(M, M2, M3, t);
			vec128_Interpolate4(
				MT1x, MT1y, MT1z, MT1w, 
				MT2x, MT2y, MT2z, MT2w, 
				MT3x, MT3y, MT3z, MT3w, 
				t);
//			MT1 = MT3;
			MT1x = MT3x;
			MT1y = MT3y;
			MT1z = MT3z;
			MT1w = MT3w;
			//			MT1 = MT3;

			//			av.v = M_VMulMat4x3(M_VMulMat4x3(M_VMulMat4x3(av.v, M), M2), M3);
		}
#endif
	}

	a = av.v;
	T.Stop();
	M_TRACEALWAYS("j = %d\n", j);
	M_TRACEALWAYS(CStrF("a = %s\n", M_VStr(a).Str() ));
	M_TRACEALWAYS(CStrF("b = %s\n", M_VStr(b).Str() ));
	M_TRACEALWAYS(CStrF("c = %s\n", M_VStr(c).Str() ));
	M_TRACEALWAYS(CStrF("d = %s\n", M_VStr(d).Str() ));

#ifdef PLATFORM_XENON
	fp64 RealCPUFreq = 2800000000.0;
#else
	fp64 RealCPUFreq = MGetCPUFrequencyFp();
#endif
	fp64 TimerToCycles = RealCPUFreq / MGetCPUFrequencyFp();
	M_TRACEALWAYS(CStrF("Freq = %f\n", MGetCPUFrequencyFp() ) );
	M_TRACEALWAYS(CStrF("TimerToCycles = %f\n", TimerToCycles ) );


	M_TRACEALWAYS(CStrF("T Nrm3x4 = %f\n", T.GetCycles()*TimerToCycles / 1000.0  ) );
	M_TRACEALWAYS(CStrF("T Nrm3x4 = %f\n", T2.GetCycles()*TimerToCycles / 1000.0  ) );

	CVec4Dfp32 r, vexp, vlog, vmul, vadd;
	M_VSt(M_VAdd(a, b), &vadd);
/*	M_VStore(M_VExp2(a), &vexp);
	M_VStore(M_VLog2(b), &vlog);*/
	M_VSt(M_VMul(a, b), &vmul);
//	c = M_VPow(a, b);
	M_VSt(c, &r);

	M_TRACEALWAYS("-------------------------------------------------------------------------------------------------------------\n");

/*	unsigned int CR, CR2;
	M_VCmpeqCR(a, a, &CR);
	M_VCmpeqCR(b, b, &CR2);
	if ((CR & M_V_ALLTRUE) && (CR2 & M_V_ALLTRUE))*/
/*	if ((a == a) && (b == b))
	{
		M_TRACEALWAYS("TRUE\n");
	}
	else
	{
		M_TRACEALWAYS("FALSE\n");
	}*/
	fp32 kalle = -1.0e0;

	M_TRACEALWAYS(CStrF("zero %s\n", M_VStr(M_VZero()).Str() ));
	M_TRACEALWAYS(CStrF("half %s\n", M_VStr(M_VHalf()).Str() ));
	M_TRACEALWAYS(CStrF("one %s\n", M_VStr(M_VOne()).Str() ));

	M_TRACEALWAYS(CStrF("add %f, %f, %f, %f\n", vadd[0], vadd[1], vadd[2], vadd[3]).Str());
	M_TRACEALWAYS(CStrF("mul %f, %f, %f, %f\n", vmul[0], vmul[1], vmul[2], vmul[3]).Str());
	M_TRACEALWAYS(CStrF("exp2 %f, %f, %f, %f\n", vexp[0], vexp[1], vexp[2], vexp[3]).Str());
	M_TRACEALWAYS(CStrF("log2 %f, %f, %f, %f\n", vlog[0], vlog[1], vlog[2], vlog[3]).Str());
	M_TRACEALWAYS(CStrF("%f, %f, %f, %f\n", r[0], r[1], r[2], r[3]).Str());

	M_TRACEALWAYS("-------------------------------------------------------------------------------------------------------------\n");
	int i = 0;
	for(i = -5; i < 15; i++)
	{
		CVec4Dfp32 v(Sign(i)*M_Pow(M_Fabs(fp32(i))*1.1276781f, fp32(i)+0.1f));
		vec128 v128 = v;

		M_TRACEALWAYS(CStrF("%d, Sqrt %f Ref %s, Vec128 %s, Vec128est %s\n", i, v[0], CStrF("%.10f", fp32(M_Sqrt(v[0]))).Str(), M_VStr(M_VSqrt(v128)).Str(), M_VStr(M_VSqrt_Est(v128)).Str() ).Str());
	}

	M_TRACEALWAYS("-------------------------------------------------------------------------------------------------------------\n");
	for(i = -5; i < 10; i++)
	{
		CVec4Dfp32 v(Sign(i)*M_Pow(M_Fabs(fp32(i))*1.1276781f, fp32(i)+0.1f));
		vec128 v128 = v;

		M_TRACEALWAYS(CStrF("%d, RSqrt %f Ref %s, Vec128 %s, Vec128est %s\n", i, v[0], CStrF("%.10f", 1.0f / fp32(M_Sqrt(v[0]))).Str(), M_VStr(M_VRsq(v128)).Str(), M_VStr(M_VRsq_Est(v128)).Str() ).Str());
	}

	M_TRACEALWAYS("-------------------------------------------------------------------------------------------------------------\n");
	for(i = -5; i < 10; i++)
	{
		CVec4Dfp32 v(Sign(i)*M_Pow(M_Fabs(fp32(i))*1.1276781f, fp32(i)+0.1f));
		vec128 v128 = v;

		M_TRACEALWAYS(CStrF("%d, Reciprocal 1 / %f = Ref %s, Vec128 %s, Vec128est %s\n", i, v[0], CStrF("%.10f", 1.0f / (v[0])).Str(), M_VStr(M_VRcp(v128)).Str(), M_VStr(M_VRcp_Est(v128)).Str() ).Str());
	}

	M_TRACEALWAYS("-------------------------------------------------------------------------------------------------------------\n");
	for(i = -5; i < 15; i++)
	{
		CVec4Dfp32 v(fp32(i) * 1.15127356f);
		vec128 v128 = v;
		CVec4Dfp32 cosv;
		cosv.v = M_VCos2(v128);
		fp32 Err = cosv[0] - M_Cos(v[0]);

		M_TRACEALWAYS(CStrF("%d, Cos %f Ref %s, Err %s, Vec128 %s, Vec128est %s\n", i, v[0], CStrF("%.10f", fp32(M_Cos(v[0]))).Str(), CStrF("%.10f", Err).Str(), M_VStr(M_VCos2(v128)).Str(), M_VStr(M_VCos_Est(v128)).Str() ).Str());
	}

	{
		CImage Plot;
		Plot.Create(1024,1024, IMAGE_FORMAT_BGR8, IMAGE_MEM_IMAGE);
		CClipRect cr = Plot.GetClipRect();
		Plot.Fill(cr, 0x00000000);

		fp32 MaxErr = 0;
		fp32 MaxRelErr = 0;

		const bool bSin = true;

		for(int x = 0; x < 1024; x++)
		{
			fp32 xf = fp32(x) / 1024.0f * _PI * 2.0f;
			CVec4Dfp32 v(xf,xf,xf,xf);
			fp32 c = (bSin) ? M_Sin(xf) : M_Cos(xf);
			CVec4Dfp32 vc = (bSin) ? M_VSin(v.v) : M_VCos2(v.v);

			fp32 err = M_Fabs(c-vc[0]);
			fp32 relerr = (M_Fabs(c) > 0.00000001f) ? M_Fabs(err / c) : 0.0f;

			if (relerr > 0.000001f)	
			{
				M_TRACEALWAYS("x %.10f, RelErr %.10f, ref %.10f, vcos %.10f\n", xf, relerr, c, vc[0]);
			}

			MaxErr = Max(err, MaxErr);
			MaxRelErr = Max(relerr, MaxRelErr);

			Plot.SetPixel(cr, CPnt(x, 1023-RoundToInt(relerr*1024.0f*10000.0f)), CPixel32(255,0,0,255));
			Plot.SetPixel(cr, CPnt(x, 1023-RoundToInt(err*1024.0f*1000000.0f)), CPixel32(0,255,0,255));
		}

#ifndef PLATFORM_CONSOLE
		if (bSin)
			Plot.Write("s:\\M_VSin_Plot.tga");
		else
			Plot.Write("s:\\M_VCos_Plot.tga");
#endif

		M_TRACEALWAYS("MaxError %f\n", MaxErr);
		M_TRACEALWAYS("MaxRelError %f\n", MaxRelErr);
	}

	M_TRACEALWAYS("-------------------------------------------------------------------------------------------------------------\n");

#endif
	{

//		CVec4fa min, max;
//		TestVector<4>(g_lV4+0, min, max);
//		GetMinMax4b(g_lV+0, 8, min, max, g_lP+0);
//		GetMinMax4(g_lV+0, 8, min, max, g_lP+0);
	}
}

#endif // SIMDTEST

//#define VPUTEST

#ifdef VPUTEST

#include "../../../Shared/MCC/MRTC_VPUManager.h"
#define SPLIT_COUNT 100
#define BUF_SIZE 30000
class VPUTest {
public:
	static void StreamBuffer()
	{
		static vec128 inBuffer[BUF_SIZE];
		static vec128 outBuffer[BUF_SIZE];
		float ff=0.0f;
		for (uint32 i=0;i<BUF_SIZE*4;i++)
		{
			((float*)inBuffer)[i]=ff;
			((float*)outBuffer)[i]=1.0f;
			ff+=.25f;
		}
		CMTime time;
		time.Start();

		uint taskid[SPLIT_COUNT];
		CVPU_JobDefinition jobDef[SPLIT_COUNT];
		for (uint i=0;i<SPLIT_COUNT;i++)
		{
			jobDef[i].SetJob(MHASH3('TEST','_STR','EAM'));
			jobDef[i].AddInStreamBuffer(0, inBuffer+i*BUF_SIZE/SPLIT_COUNT,BUF_SIZE/SPLIT_COUNT,64,16);
			jobDef[i].AddOutStreamBuffer(1,outBuffer+i*BUF_SIZE/SPLIT_COUNT,BUF_SIZE/SPLIT_COUNT,64,16);
			taskid[i]=MRTC_ThreadPoolManager::VPU_AddTask(jobDef[i]);
		}
		for (uint i=0;i<SPLIT_COUNT;i++)
		{
			MRTC_ThreadPoolManager::VPU_BlockOnTask(taskid[i]);
		}
		time.Stop();
		float msec=1000.0f*time.GetTime();
		float cyc=time.GetCycles();
		M_TRACE("%d",msec);
		M_TRACE("%d",cyc);
		for (uint32 i=0;i<BUF_SIZE*4;i++)
		{
			M_ASSERT(((float*)inBuffer)[i]==((float*)outBuffer)[i],"");
		}
	}

	static void CacheBuffer()
	{
		static vec128 inBuffer[BUF_SIZE];
		static vec128 outBuffer[BUF_SIZE];
		float ff=0.0f;
		for (uint32 i=0;i<BUF_SIZE*4;i++)
		{
			((float*)inBuffer)[i]=ff;
			((float*)outBuffer)[i]=1.0f;
			ff+=.25f;
		}
		CMTime time;
		time.Start();

		uint taskid[SPLIT_COUNT];
		CVPU_JobDefinition jobDef[SPLIT_COUNT];
		for (uint i=0;i<SPLIT_COUNT;i++)
		{
			jobDef[i].SetJob(MHASH3('TEST','_CAC','HE'));
			jobDef[i].AddCacheBuffer(0, inBuffer+i*BUF_SIZE/SPLIT_COUNT,BUF_SIZE/SPLIT_COUNT,16);
			jobDef[i].AddInStreamBuffer(1,outBuffer+i*BUF_SIZE/SPLIT_COUNT,BUF_SIZE/SPLIT_COUNT,64,16);
			taskid[i]=MRTC_ThreadPoolManager::VPU_AddTask(jobDef[i]);
		}
		for (uint i=0;i<SPLIT_COUNT;i++)
		{
			MRTC_ThreadPoolManager::VPU_BlockOnTask(taskid[i]);
		}
		time.Stop();
		float msec=1000.0f*time.GetTime();
		float cyc=time.GetCycles();
		M_TRACE("%d",msec);
		M_TRACE("%d",cyc);
		for (uint32 i=0;i<BUF_SIZE*4;i++)
		{
			M_ASSERT(((float*)inBuffer)[i]==((float*)outBuffer)[i],"");
		}
	}
	static void TestVec128()
	{
		CVPU_JobDefinition Job;
		Job.SetJob(MHASH3('VEC1','28TE','ST'));
		uint32 taskid=MRTC_ThreadPoolManager::VPU_AddTask(Job);
		MRTC_ThreadPoolManager::VPU_BlockOnTask(taskid);
	}
};

#endif // VPUTEST

//#define LISTTEST
#ifdef LISTTEST
class CListTest
{
public:
	int m_iList; 
	DLinkD_Link(CListTest, m_Link);
};

void TraceList(DLinkD_List(CListTest, m_Link) &_List)
{
	M_TRACEALWAYS("Tracing List\n");
	DLinkD_Iter(CListTest, m_Link) Iter = _List;

	while (Iter)
	{
		M_TRACEALWAYS("%d\n", Iter->m_iList);
		++Iter;
	}
}

#endif //LISTTEST

void CXRealityApp::Create()
{
#ifdef PLATFORM_XENON
	{
		PMCInstallSetup( &PMCDefaultSetups[PMC_SETUP_OVERVIEW_PB0T0] );
		// Reset the Performance Monitor Counters in preparation for a new sampling run.
		PMCResetCounters();
		// Start up the Performance Monitor Counters.
		PMCStart(); // Start up counters
	}

#endif
	for(int i = 0; i < 100; i++)
		FloatConformanceTest();
#ifdef PLATFORM_XENON
	{
		PMCState PMCS;
		PMCStop();
		// Get the counters.
		PMCGetCounters( &PMCS);
		// Print out detailed information about all 16 counters.
		PMCDumpCountersVerbose( &PMCS, PMC_VERBOSE_NOL2ECC );
	}
#endif

#ifdef USE_SN_TUNER
	snTunerInit();
#endif

#ifdef LISTTEST
	{
		DLinkD_List(CListTest, m_Link) List0;
		DLinkD_List(CListTest, m_Link) List1;
		DLinkD_List(CListTest, m_Link) List2;

		CListTest Temp[10];
		for (int i = 0; i < 10; ++i)
		{
			Temp[i].m_iList = i;
		}

		List0.Insert(Temp[0]);
		List0.Insert(Temp[1]);
		List0.Insert(Temp[2]);
		List0.Insert(Temp[3]);
		List0.Insert(Temp[4]);

		List1.Insert(Temp[5]);
		List1.Insert(Temp[6]);
		List1.Insert(Temp[7]);
		List1.Insert(Temp[8]);
		List1.Insert(Temp[9]);

		TraceList(List0);
		TraceList(List1);
		TraceList(List2);

		CListTest *pFirst0 = List0.GetFirst();
		CListTest *pLast0 = List0.GetLast();
		CListTest *pFirst1 = List1.GetFirst();
		CListTest *pLast1 = List1.GetLast();

		List0.Construct();
		List1.Construct();
		pLast0->m_Link.SetNextInit(&pFirst1->m_Link.m_Link);
		pFirst1->m_Link.SetPrevInit(&pLast0->m_Link.m_Link);
		List2.GetLink()->TransferList(&pFirst0->m_Link.m_Link, &pLast1->m_Link.m_Link);

		TraceList(List0);
		TraceList(List1);
		TraceList(List2);
	}

#endif




#ifdef SIMDTEST
	SIMDTest();
#endif



#ifdef VPUTEST
	VPUTest::TestVec128();
	VPUTest::StreamBuffer();
	VPUTest::CacheBuffer();
#endif


	/*	HANDLE hAlignHeap = HeapCreate(0,0,0);

	const int nAlloc = 64;
	for(int j = 0; j < 200; j++)
	{
//		size_t lSizes[nAlloc];
		void* lpBlocks[nAlloc];

		FillChar(&lpBlocks, sizeof(lpBlocks), 0);

		for(int i = 0; i < 10000; i++)
		{
			int iBlock = TruncToInt(Random*(nAlloc-1));

			if (lpBlocks[i])
			{
				lpBlocks[i] = NULL;
				HeapFree(hAlignHeap, 0, lpBlocks[i]);
			}
			else
			{
				int size = ((Random*8000)+1)*4;
				size += 8;
				size = (size+15) & 0xfffffff0;
				size -= 8;
				void* p = HeapAlloc(hAlignHeap, 0, size);
				if (size_t(p) & 0x0f)
				{
					M_TRACE("Failed align %d, size %.4x, p %.8x\n", i, size, p);
					__asm int 3;
				}

				lpBlocks[iBlock] = p;
			}
		}



		M_TRACE("\n");
	}
*/


//	TAPTest();



	MRTC_SystemInfo::Thread_SetName("XRApp Main");

//	for (int i = 0; i < 1024*8; ++i)
//		ConOut(CStrF("Temporary insanity %d", i));
	{
		MACRO_GetSystem;
		pSys->GetRegistry();
		pSys->m_spRegistry = pSys->m_spRegistry->MakeThreadSafe();
		pSys->GetEnvironment()->MakeThreadSafe();
	}
//	FloatTest();

/*	//Calculate unlockable keys
	uint64 Test;
	uint64 Test2 = 1;
	Test = 0;
	Test |= Test2 << (1 - 1);
	Test |= Test2 << (6 - 1);
	Test |= Test2 << (35 - 1);
	Test |= Test2 << (25 - 1);
	Test |= Test2 << (26 - 1);

	Test = 0;
	Test |= Test2 << (57 - 1);
	Test |= Test2 << (58 - 1);
	Test |= Test2 << (59 - 1);
	Test |= Test2 << (12 - 1);
	Test |= Test2 << (13 - 1);
	Test |= Test2 << (27 - 1);

	Test = 0;
	Test |= Test2 << (28 - 1);
	Test |= Test2 << (15 - 1);
	Test |= Test2 << (16 - 1);
	Test |= Test2 << (29 - 1);
	Test |= Test2 << (30 - 1);
	Test |= Test2 << (31 - 1);
	Test |= Test2 << (33 - 1);

	Test = 0;
	Test |= Test2 << (5 - 1);
	Test |= Test2 << (18 - 1);
	Test |= Test2 << (19 - 1);
	Test |= Test2 << (20 - 1);
	Test |= Test2 << (21 - 1);

	Test = 0;
	Test |= Test2 << (3 - 1);
	Test |= Test2 << (22 - 1);
	Test |= Test2 << (23 - 1);
	Test |= Test2 << (24 - 1);
	Test |= Test2 << (32 - 1);
	Test |= Test2 << (55 - 1);
	Test |= Test2 << (56 - 1);

	Test = 0;
	Test |= Test2 << (7 - 1);
	Test |= Test2 << (8 - 1);
	Test |= Test2 << (9 - 1);
	Test |= Test2 << (10 - 1);
	Test |= Test2 << (11 - 1);
	Test |= Test2 << (17 - 1);
	Test |= Test2 << (43 - 1);

	Test = 0;
	Test |= Test2 << (34 - 1);
	Test |= Test2 << (36 - 1);
	Test |= Test2 << (37 - 1);
	Test |= Test2 << (38 - 1);
	Test |= Test2 << (39 - 1);

	Test = 0;
	Test |= Test2 << (40 - 1);
	Test |= Test2 << (42 - 1);
	Test |= Test2 << (4 - 1);
	Test |= Test2 << (41 - 1);

	Test = 0;
	Test |= Test2 << (44 - 1);
	Test |= Test2 << (45 - 1);
	Test |= Test2 << (46 - 1);
	Test |= Test2 << (47 - 1);
	Test |= Test2 << (48 - 1);

	Test = 0;
	Test |= Test2 << (49 - 1);
	Test |= Test2 << (50 - 1);
	Test |= Test2 << (51 - 1);
	Test |= Test2 << (52 - 1);
	Test |= Test2 << (53 - 1);
	Test |= Test2 << (54 - 1);
	Test |= Test2 << (2 - 1);
	Test |= Test2 << (14 - 1);*/

#ifndef	M_RTM
	FloatConformanceTest();
#endif

	{
		m_DrawMode	= 0;
		m_MainThreadID = MRTC_SystemInfo::OS_GetThreadID();
		
		MSCOPE(CXRealityApp::Create, Init);
		
		LogFile("(CXRealityApp::Create) Begin...");
		CApplication::Create();
		
#ifdef PLATFORM_WIN
#ifndef PLATFORM_XBOX
//		InitCommonControls();
#endif
		Win32_UpdateMemoryStatus(true);
#endif
		
#ifndef PLATFORM_XBOX
#ifdef PLATFORM_WIN
		// Prevent two instances of the app from running.
		MACRO_GetRegisterObject(CWin32AppInfo, pWinInfo, "SYSTEM.WIN32APPINFO");
		if (pWinInfo)
		{
			pWinInfo->m_hIcon = LoadIcon((HINSTANCE)pWinInfo->m_hInstance, MAKEINTRESOURCE(IDI_P4));
			ConOutL(CStrF("hPrevInstance %.8x", pWinInfo->m_hPrevInstance));
			if (pWinInfo->m_hPrevInstance)
				Error("Create", "An instance of the application is already running.");
		}
		
#endif
#endif


		
		AddToConsole();

		// Get some data.
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		m_pSystem = pSys;
		m_pObjMgr = MRTC_GOM();
		if (!m_pSystem) Error("-", "No SYSTEM object available.");

#ifdef PLATFORM_CONSOLE
		{
			CStr Game = m_pSystem->GetEnvironment()->GetValue("DEFAULTGAME", "P5");
			if (Game.CompareNoCase("PB") != 0)
			{
				// For darkness Xenon/PS3, lower the texture quality to default picmip1, and add an extra picmip offset to some of the groups..
				s_DefaultTextureQuality = fp32(1.0f/3 * 1);
				s_EXTRA_PICMIP_WORLDOBJECTS = 0;
				s_EXTRA_PICMIP_WEAPONS = 1;
				s_EXTRA_PICMIP_PROJMAPS = 1;
			}
		}
#endif
		{ // Prevent the game from crashing before proper picmips have been set...
			int32 lnDefaultPicMip[16] = { 1, 1, 1 + s_EXTRA_PICMIP_WEAPONS, 1 + s_EXTRA_PICMIP_WORLDOBJECTS, 1, 1, 1, 1, 1 + s_EXTRA_PICMIP_PROJMAPS, 1, 1, 1, 1, 1, 1, 1, };
			for (uint i = 0 ; i < 16; i++)
			{
				CFStr28 Key;
				Key.CaptureFormated("r_picmip%d", i);
				int PicMip = Max(lnDefaultPicMip[i], m_pSystem->GetEnvironment()->GetValuei(Key, 0));
				m_pSystem->GetEnvironment()->SetValuei(Key, PicMip);
			}
		}

		m_spRegNav = MNew(CRegistryNavigator);
				
		m_spViewport = MNew(CRC_ConsoleViewport);
		if (!m_spViewport) MemError("-");
		m_spViewport->Create(false);
		m_spViewport->ReadSettings(m_pSystem->GetEnvironment());
		
		if (0)
		{

			M_TRACEALWAYS("\n\n");
			CCFile TestFile;
			
			CMTime Measure;
			const static int Testing = 4;
			static uint8 Buffer[Testing];
			
			TestFile.Open(m_pSystem->m_ExePath + "Content\\Gui.xtc", CFILE_BINARY | CFILE_READ);

			int ToRead = TestFile.Length();
			
			{
				TMeasureReset(Measure);
				M_CALLGRAPHT;
				while (ToRead)
				{
					int ReadThisTime = Min(Testing, ToRead);
					TestFile.Read(Buffer, ReadThisTime);
					ToRead -= ReadThisTime;
				}
			}
			M_TRACEALWAYS("Read Content\\Gui.xtc %f\n", (float)((TestFile.Length() / Measure.GetTime()) / (1024.0f * 1024.0f)));
			
/*			TestFile.Open(m_pSystem->m_ExePath + "Content\\Waves_Xbox\\Waves_0.xwc", CFILE_BINARY | CFILE_READ);

			ToRead = TestFile.Length();
			{
				TMeasureReset(Measure);
				while (ToRead)
				{
					int ReadThisTime = Min(Testing, ToRead);
					TestFile.Read(Buffer, ReadThisTime);
					ToRead -= ReadThisTime;
				}
			}
			M_TRACEALWAYS("Read Content\\Waves_Xbox\\Waves_0.xwc %f\n", (float)((TestFile.Length() / Measure.GetTime()) / (1024.0f * 1024.0f)));
			
			TestFile.Open(m_pSystem->m_ExePath + "Content\\Waves_Xbox\\Waves_3.xwc", CFILE_BINARY | CFILE_READ);
			ToRead = TestFile.Length();
			{
				TMeasureReset(Measure);
				while (ToRead)
				{
					int ReadThisTime = Min(Testing, ToRead);
					TestFile.Read(Buffer, ReadThisTime);
					ToRead -= ReadThisTime;
				}
			}
			M_TRACEALWAYS("Read Content\\Waves_Xbox\\Waves_3.xwc %f\n", (float)((TestFile.Length() / Measure.GetTime()) / (1024.0f * 1024.0f)));
			M_TRACEALWAYS("\n\n");*/
		}
		
		// FIXME:
		fp32 FOV = 95;
		m_spViewport->SetFOV(FOV);
		m_spViewport->Con_Zoom(FOV);
		
		m_bSound = true;
		m_bShowConsole = false;
#ifdef M_RTM
		m_bConsoleInvis = pSys->GetEnvironment()->GetValuei("CON_INVISIBLE", 1) == 1;
#else
		m_bConsoleInvis = pSys->GetEnvironment()->GetValuei("CON_INVISIBLE", 0) == 1;
#endif
		m_bShowBorders = m_pSystem->GetEnvironment()->GetValuei("CON_SHOWBORDERS", 0) != 0;
		m_bShowFPS = m_pSystem->GetEnvironment()->GetValuei("CON_SHOWFPS", 0) != 0;
		m_bShowFPSLite = m_pSystem->GetEnvironment()->GetValuei("CON_SHOWFPSLITE", 0) != 0;
//		m_bShowFPSLite = true;
		m_bShowVertexProgramWarning = m_pSystem->GetEnvironment()->GetValuei("CON_SHOWVPWARN", 0) != 0;
		m_bShowFPSOnly = m_pSystem->GetEnvironment()->GetValuei("CON_SHOWFPSONLY", 0) != 0;
		m_bShowSoundDbg = m_pSystem->GetEnvironment()->GetValuei("CON_SHOWSOUND", 0) != 0;
		m_bShowVBE = m_pSystem->GetEnvironment()->GetValuei("CON_SHOWVBE", 0) != 0;
		m_bShowMemInfo = m_pSystem->GetEnvironment()->GetValuei("CON_SHOWMEMINFO", 0) != 0;

//		m_bShowSoundDbg = true;
		m_bLogKeys = m_pSystem->GetEnvironment()->GetValuei("IN_LOGKEYS", 0) != 0;
#ifdef M_RTM
		m_bDebugKeys = m_pSystem->GetEnvironment()->GetValuei("IN_DEBUGKEYS", 0) != 0;
#else
		m_bDebugKeys = m_pSystem->GetEnvironment()->GetValuei("IN_DEBUGKEYS", 1) != 0;
#endif
#ifdef M_Profile
		m_bShowCallGraph = m_pSystem->GetEnvironment()->GetValuei("CON_SHOWCALLGRAPH", 0) != 0;
		m_bOnRenderCallGraph = m_pSystem->GetEnvironment()->GetValuei("CON_ONRENDERCALLGRAPH", 0) != 0;
		m_bShowCallGraphAutoUpdate = true;
#endif
		m_bShowStats = m_pSystem->GetEnvironment()->GetValuei("CON_SHOWSTAT", 0) != 0;
		m_bShowTurtle = m_pSystem->GetEnvironment()->GetValuei("CON_SHOWTURTLE", 0);
		m_TurtleMinFPS = m_pSystem->GetEnvironment()->GetValuei("CON_TURTLEMINFPS", 30);
		m_TurtleMaxFPS = m_pSystem->GetEnvironment()->GetValuei("CON_TURTLEMAXFPS", 45);
		m_BuildMsg = m_pSystem->GetEnvironment()->GetValue("CON_BUILDMSG", "");


#ifdef PLATFORM_XBOX1
		m_bAutomaticScreenResizeSticky = m_bAutomaticScreenResize = m_pSystem->GetEnvironment()->GetValuei("VID_AUTORESIZE", 1, 0);
#else
		m_bAutomaticScreenResizeSticky = m_bAutomaticScreenResize = m_pSystem->GetEnvironment()->GetValuei("VID_AUTORESIZE", 0, 0);
#endif
#ifdef M_DEMO_XBOX
		//RUNMODE_KIOSKMODE
	#ifdef PLATFORM_XBOX1
		if(g_XboxDemoParams.m_bLaunchedFromDemoLauncher)
		{
			if(g_XboxDemoParams.m_KioskMode)
				m_pSystem->GetEnvironment()->SetValue("KIOSKMODE", "1");
			if(g_XboxDemoParams.m_LaunchData.dwTimeout != 0)
				m_IdleQuitTimeOut = g_XboxDemoParams.m_LaunchData.dwTimeout / 1000.0f;
			else 
				m_IdleQuitTimeOut = -1;
		}
		else
		{
			m_IdleQuitTimeOut = -1;
		}
	#else
		m_IdleQuitTimeOut = m_pSystem->GetEnvironment()->GetValuei("IDLEQUITTIMEOUT", -1);
	#endif
#endif
		m_bShowAnimationsDbg = false;
		
//		m_pSystem->GetEnvironment()->SetValuei("HIDENOPADSCREEN", 1);
		
		m_bCaptureNext = false;
		
#ifndef PLATFORM_CONSOLE
		CStr Game = m_pSystem->GetEnvironment()->GetValue("DEFAULTGAME", "Default");
		CStr OrgPaths = m_pSystem->GetEnvironment()->GetValue("DEFAULTGAMEPATH", "Content\\");
		{
			MSCOPESHORT(ReadFont);
			CStr Paths = OrgPaths;
			CStr ValidPath;
			while(Paths != "")
			{
				CStr Path = Paths.GetStrSep(";");
				if(Path.GetDevice() == "")
					Path = m_pSystem->m_ExePath + Path;
				if(CDiskUtil::FileExists(Path + "FONTS\\" + MiniFont ".xfc"))
					ValidPath = Path;
			}
			
			if (ValidPath == "")
				Error("Create", CStrF("Could not find debugfont %s at: %s, Game: %s", CStrF("FONTS\\%s.xfc", MiniFont).Str(), m_pSystem->GetEnvironment()->GetValue("DEFAULTGAMEPATH", "Content\\").Str(), Game.Str()));
				
			m_spDebugFont = MNew(CRC_Font);
			if (!m_spDebugFont)
				MemError("Create");
			m_spDebugFont->ReadFromFile(ValidPath + "FONTS\\" + MiniFont ".xfc");
			m_ConRender.SetFont(m_spDebugFont);
			m_ConRender.m_EditRelPos = CPnt(4,3);
			m_ConRender.m_bEditGlow = false;
		}
#endif

#ifndef PLATFORM_XBOX
#ifdef PLATFORM_WIN_PC
		if (m_spLogWin!=NULL)
		{
			MRTC_GOM()->UnregisterObject((CReferenceCount*)m_spLogWin, "SYSTEM.LOG");
			m_spLogWin = NULL;
		}
#endif
#endif

		Con_OptionsGetFromEnv();

		CRegistry* optReg = m_pSystem->GetOptions();
		if(!optReg)
			Con_SetDefaultOptions();

		LogFile("(CXRealityApp::CXRealityApp) ExePath: " + m_pSystem->m_ExePath);
		

		CStr CmdLine = m_pSystem->m_CommandLine.UpperCase();
		
		m_bSound = m_pSystem->GetEnvironment()->GetValuei("SND_ENABLE", 1) != 0;
		
		if (CmdLine.Find("-NOSOUND") >= 0)
			m_bSound = false;
		
		InitWorld();

#ifdef PLATFORM_WIN32_PC
		// Check Direct X version
		try
		{
			CWin32_Registry Registry(EWin32RegRoot_LocalMachine);
			TArray<uint8> Version = Registry.Read_Bin("Software\\Microsoft\\DirectX", "InstalledVersion");
			if (Version.Len() >= 4)
			{

				uint32 BaseVersion = *((uint32 *)Version.GetBasePtr());
#ifdef CPU_LITTLEENDIAN
				ByteSwap_uint32_Working(BaseVersion);
#endif
				if (BaseVersion < 9)
				{
					CStr Msg = Localize_Str(CStr("§LSYS_WRONGDIRECTXVERSION"));
					CStr Title = Localize_Str(CStr("§LSYS_APPNAME"));

					MessageBoxW(NULL, (LPCWSTR)Msg.Unicode().StrW(), (LPCWSTR)Title.Unicode().StrW(), MB_OK | MB_ICONEXCLAMATION);
					m_pSystem->m_bBreakRequested = true;
					return;
				}
			}
		}
		catch (CCException)
		{
		}
#endif


		m_pSystem->CreateInput();
//		m_pSystem->Render();

		// Sound
		if (m_bSound)
		{
			MSCOPE(Create, SOUNDCONTEXT);
#ifdef PLATFORM_DOLPHIN
			CStr SoundContext = m_pSystem->GetEnvironment()->GetValue("SND_CLASS", "Dolphin", 0);
#elif defined(PLATFORM_PS2)
			CStr SoundContext = m_pSystem->GetEnvironment()->GetValue("SND_CLASS", "PS2", 0);
#else
//			CStr SoundContext = m_pSystem->GetEnvironment()->GetValue("SND_CLASS", "DSound", 0);
			CStr SoundContext = m_pSystem->GetEnvironment()->GetValue("SND_CLASS", "DSound2", 0);
//			CStr SoundContext = m_pSystem->GetEnvironment()->GetValue("SND_CLASS", "OpenAL", 0);
#endif
			M_TRY
			{
				m_spSoundContext = MCreateSoundContext("CSoundContext_" + SoundContext, 128); 

				//AR-ADD, register the sound context as "SYSTEM.SOUND"
				if (m_spSoundContext)
					MRTC_GOM()->RegisterObject((CReferenceCount*)m_spSoundContext, "SYSTEM.SOUND");
			} 
			M_CATCH(
			catch(CCException) 
			{ 
				ConOutL("        Failed to initialize sound.");
			}
			)
		}
		else
			ConOutL("Sound is disabled.");

		if (m_spGame != NULL) 
		{
			m_spGame->SetSoundContext(m_spSoundContext);
#ifdef PLATFORM_CONSOLE
			if (m_spGame->m_spWData)
			{
				int iFont = m_spGame->m_spWData->GetResourceIndex("XFC:" MiniFont, NULL);
				if(iFont > 0)
				{
					CWRes_XFC* pRcFont = (CWRes_XFC*)m_spGame->m_spWData->GetResource(iFont);
					CRC_Font *pFont = pRcFont->GetFont();
					if(pFont)
					{
						m_spDebugFont = pFont;
						m_ConRender.SetFont(pFont);
						m_ConRender.m_EditRelPos = CPnt(4,3);
						m_ConRender.m_bEditGlow = false;
					}
				}
			}
#endif
		}
		
		if (!m_spDebugFont)
			Error("Create", "Font not found");

		// Create CD-Audio object.
/*		{
			try
			{
#if !defined(PLATFORM_XBOX) && !defined(PLATFORM_DOLPHIN) && !defined(PLATFORM_PS2) && !defined(PLATFORM_MACOS)
				m_spCDAudio = MCreateCDAudio();
#endif
			}
			catch(CCException)
			{
				m_spCDAudio = NULL;
			}
		}*/
		
		LogFile("(CXRealityApp_Create) Done");
	}

	m_bLoading = false;
}

//
//
//

CXRealityApp::~CXRealityApp()
{
	MSCOPE(CXRealityApp::~CXRealityApp, XRAPP);

	LogFile("(CXRealityApp::~CXRealityApp) Begin");
//	M_TRY
//	{
		m_spEngine = NULL;

		m_spDebugFont = NULL;
		m_ConRender.m_pFont = NULL;
//		if ((m_spViewport != NULL) && m_pSystem)
//			m_spViewport->WriteSettings(m_pSystem->GetEnvironment());

		m_spRegNav = NULL;
		if (m_spVBMContainer)
		{
			MRTC_GOM()->UnregisterObject((CReferenceCount*)m_spVBMContainer, "SYSTEM.VBMCONTAINER");
			m_spVBMContainer = NULL;
		}
		if (m_spSoundContext)
		{
			m_spSoundContext->KillThreads();
			MRTC_GOM()->UnregisterObject((CReferenceCount*)m_spSoundContext, "SYSTEM.SOUND");
			m_spSoundContext = NULL;
		}
//		m_spCDAudio = NULL;

		if(m_spGame != NULL)
		{
			MRTC_GOM()->UnregisterObject((CReferenceCount*)m_spGame, "GAMECONTEXT");
			m_spGame = NULL;
		}
		
	#ifndef PLATFORM_XBOX
	#ifdef PLATFORM_WIN_PC
		if (m_spLogWin!=NULL)
		{
			MRTC_GOM()->UnregisterObject((CReferenceCount*)m_spLogWin, "SYSTEM.LOG");
			m_spLogWin = NULL;
		}
	#endif
	#endif
/*	}
	M_CATCH(
	catch(CCException)
	{
	}
	)*/

	LogFile("(CXRealityApp::~CXRealityApp) Done");
}


//
//
//
void CXRealityApp::InitWorld()
{

	MSCOPE(CXRealityApp::InitWorld, XRAPP);

	LogFile("(CXRealityApp_InitWorld) Begin");

	CRegistry* pEnv = m_pSystem->GetEnvironment();

	MRTC_SAFECREATEOBJECT(spVBMContainer, "CXR_VBMContainer", CXR_VBMContainer);
	spCReferenceCount spVBMC = (CXR_VBMContainer*)spVBMContainer;
	MRTC_GOM()->RegisterObject(spVBMC, "SYSTEM.VBMCONTAINER");
	m_spVBMContainer = spVBMContainer;

	int VBSize = pEnv->GetValuei("XR_VBHEAP", 4096);
	VBSize = Max(16, Min(1024*64, VBSize));
#if defined(PLATFORM_XENON) || defined(PLATFORM_PS3)
	m_spVBMContainer->Create(3, VBSize*1024, 1024);
#else
	m_spVBMContainer->Create(2, VBSize*1024, 1024);
#endif

	
	CStr Path = m_pSystem->GetEnvironment()->GetValue("DEFAULTGAMEPATH", "Content\\");
	CStr Game = m_pSystem->GetEnvironment()->GetValue("DEFAULTGAME", "P5");

	if(Game == "P6")
		m_spGame = MNew(CGameContext_P6);
	else
		m_spGame = MNew(CGameContextMod);
	if (m_spGame == NULL) MemError("-");

	//Create engine object
	{
		//Create engine
		spCReferenceCount spObj = (CReferenceCount*) MRTC_GOM()->CreateObject("CXR_EngineImpl");
		m_spEngine = safe_cast<CXR_Engine> ((CReferenceCount*)spObj);
		if (!m_spEngine) Error("Render", "Unable to create engine-object.");
		//		if (GetClientMode() & WCLIENT_MODE_SPLITSCREEN)
		//			m_spEngine->SetVarf(XR_ENGINE_NORENDERTOTEXTURE, 1);
	}

	m_spGame->Create(Path, Game, m_spEngine);
	MRTC_GOM()->RegisterObject((CReferenceCount*)m_spGame, "GAMECONTEXT");

	//Initialize engine object
	{
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		uint32 MaxRecurseDepth = 2;
		if( pSys )
		{
			MaxRecurseDepth = pSys->GetEnvironment()->GetValuei("XR_MAXRECURSE", 4);
		}
		m_spEngine->Create(MaxRecurseDepth, 0);
		m_spEngine->SetVarf(XR_ENGINE_LIGHTSCALE, 0.5f);	// Set light-scale for 2x overbrightness.
	}
	
	CStr CmdLine = m_pSystem->m_CommandLine.UpperCase();

	m_pSystem->DC_InitFromEnvironment();

	CRegistry* pCmdLine = m_pSystem->GetRegistry()->FindChild("CommandLine");

	if (pCmdLine)
	{
		int Count = pCmdLine->GetNumChildren();

		for( int Child = 0; Child < Count; Child++ )
		{
			CStr Value = pCmdLine->GetChild(Child)->GetThisValue();
			if( !Value.CompareNoCase( "-MAP" ) )
			{
				Child++;
				if( Child < Count )
				{
					CStr MapName = pCmdLine->GetChild(Child)->GetThisValue();
					CStr Script = CFStrF("cg_loadlastvalidprofile(\"map(\\\"%s\\\")\", \"cg_rootmenu(\\\"select_profile\\\")\")", MapName.Str());
					m_pSystem->m_spCon->ExecuteString(Script);
				}
				else
				{
					ConOutL(CStr("Map argument was missing followup argument"));
				}
			}
			else if( !Value.CompareNoCase( "-DEMO" ) )
			{
				Child++;
				if( Child < Count )
				{
					m_pSystem->m_spCon->ExecuteString(CStrF("play(\"%s\")", pCmdLine->GetChild(Child)->GetThisValue().Str()));
				}
				else
				{
					ConOutL(CStr("Demo argument was missing followup argument"));
				}
			}
			else if( !Value.CompareNoCase( "-EXEC" ) )
			{
				Child++;
				if( Child < Count )
				{
					m_PendingExecute = pCmdLine->GetChild(Child)->GetThisValue();
				}
				else
				{
					ConOutL(CStr("Exec argument was missing followup argument"));
				}
			}
		}
	}

	LogFile("(CXRealityApp::InitWorld) Done.");
}

//
//
//
void CXRealityApp::OnRefreshSystem()
{
}

void CXRealityApp::RenderStats(CDisplayContext* _pDisplay, CRenderContext* _pRC, CXR_VBManager* _pVBM, CClipRect _ConsoleClip, CRct _ConsoleRect)
{
	{
	//	CRenderContext* pRC = _pRC;
		CXR_VBManager* pVBM = _pVBM;

#ifndef PLATFORM_CONSOLE
		// -------------------------------------------------------------------
		// Video capture
		// -------------------------------------------------------------------
		if (m_spVideoFile != NULL)
		{
			class CSaveVideo
			{
			public:
				CXRealityApp *m_pApp;

				void VideoDo(CRenderContext* _pRC)
				{
					if (m_pApp->m_spVideoFile)
					{
						CImage* pImg = _pRC->GetDC()->GetFrameBuffer();
						if (pImg != NULL)
						{
							int w = pImg->GetWidth();
							int h = pImg->GetHeight();
							int Fmt = IMAGE_FORMAT_BGR8;

							// Create temporary images if they dont exist.
							if (!m_pApp->m_spVideoFileCapture || m_pApp->m_spVideoFileCapture->GetWidth() != w || m_pApp->m_spVideoFileCapture->GetHeight() != h)
							{
								m_pApp->m_spVideoFileCapture = MNew(CImage);
								m_pApp->m_spVideoFileCapture->Create(w, h, pImg->GetFormat(), IMAGE_MEM_IMAGE);
							}
							if (!m_pApp->m_spVideoFileConv || m_pApp->m_spVideoFileConv->GetWidth() != w || m_pApp->m_spVideoFileConv->GetHeight() != h)
							{
								m_pApp->m_spVideoFileConv = MNew(CImage);
								m_pApp->m_spVideoFileConv->Create(w, h, Fmt, IMAGE_MEM_IMAGE);
							}

							// Copy and convert from framebuffer
							CImage::Convert(pImg, m_pApp->m_spVideoFileCapture, IMAGE_CONVERT_RGB);
							CImage::Convert(m_pApp->m_spVideoFileCapture, m_pApp->m_spVideoFileConv, IMAGE_CONVERT_RGB);

							// Store frame
							m_pApp->m_spVideoFile->AddFrame(m_pApp->m_spVideoFileConv);
						};
					}
				}
				
				static void RenderCallback(CRenderContext* _pRC, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext, CXR_VBMScope* _pScope, int _Flags)
				{
					MSCOPESHORT(InitiateTexturePrecache);
					CSaveVideo* pData = (CSaveVideo*)_pContext;
					pData->VideoDo(_pRC);
				}
			};

			{
				CSaveVideo* pData = (CSaveVideo*)pVBM->Alloc(sizeof(CSaveVideo));
				if (pData)
				{
					pData->m_pApp = this;
					pVBM->AddCallback(CSaveVideo::RenderCallback, pData, 0);
				}
			}

		}
#endif

		MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
		if(!pTC)
			Error("OnRender", "No texture-context available.");

		CRC_Util2D Util2D;
		CRC_Viewport* pVP = pVBM->Viewport_Get();
//		CRct ViewRect = pVP->GetViewRect();
		Util2D.Begin(NULL, pVP, pVBM);

		{
			{
				MSCOPE(DebugStatistics, XRAPP_RENDER);
				{
					////////////////////////
					// TURTLE
					////////////////////////
					//					fp32 CurFPS = GetCPUFrequency()/TmpTime_Frame;
					fp32 CurFPS;
					fp32 CurFPSLast;
					{
						DLock(m_MainThreadStats_Lock);
						CurFPS = 1.0/(m_MainThread_FrameTime.m_Average+0.000000000001);
						CurFPSLast = 1.0/(m_MainThread_FrameTime.m_LastValue+0.000000000001);
					}
					if(m_bShowTurtle && CurFPS < m_TurtleMaxFPS)
					{
						fp32 Turtleness = Clamp01((m_TurtleMaxFPS - CurFPS) / (m_TurtleMaxFPS - m_TurtleMinFPS));
						CPixel32 TurtleColor = 0xff808000;
						if (Turtleness >= 0.99f)
							TurtleColor = 0xff800000;
						else if (Turtleness > 0.5f)
							TurtleColor = 0xff804000;

						CClipRect Clip(0, 0, 640, 480);
						Util2D.SetTexture("SPECIAL_TURTLE");
						CPnt Origo(410, 0);
						Util2D.SetCoordinateScale(CVec2Dfp32(_ConsoleRect.GetWidth() / 640.0f , _ConsoleRect.GetHeight() / 480.0f));
						Util2D.SetTextureOrigo(Clip, Origo);
						Util2D.SetTextureScale(2, 2);
						CRC_Attributes *pAttr = Util2D.GetAttrib();
						pAttr->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
						pAttr->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
						Util2D.Rect(CRct(0, 0, 640, 480), CRct(Origo, Origo + CPnt(128, 64)), TurtleColor);

						Util2D.SetTexture(NULL);
					}

#if defined(M_Profile) || defined (PLATFORM_WIN)
					if (!m_bShowConsole && (m_bShowFPS ||m_bShowFPSOnly || m_bShowFPSLite))
					{
						fp32 nTriTotal = _pRC->Attrib_GlobalGetVar(20);

#ifdef PLATFORM_WIN
						CDA_MemoryManager* pMemMgr = MRTC_GetMemoryManager();
						Win32_UpdateMemoryStatus();
						CFStr FPSStr;
						if (m_bShowFPSOnly)
						{
							FPSStr = CFStrF("%3.1f", CurFPS);
						}
						else
						{
							FPSStr = CFStrF("%s§cff5MH(F:§cfff%.1fM §cff5L:%.1fM MF:%.1fM U:%.1fM/%.1fM) §caaa %-9d", 
								(char*)m_Win32MemoryStatusStr, 
								(float)pMemMgr->GetFreeMem() / (1024.0f* 1024.0f), 
								(float)pMemMgr->GetLargestFreeMem() / (1024.0f* 1024.0f), 
								(float)pMemMgr->GetMinFreeMem() / (1024.0f* 1024.0f),
								(float)pMemMgr->GetUsedMem() / (1024.0f* 1024.0f),
								(float)pMemMgr->m_AllocatedMem / (1024.0f* 1024.0f), 
								int(nTriTotal*CurFPS)
								);

						}

						//						CFStr FPSStr = CFStrF("%.2f(%.2f, %d) %s", m_FPS, m_AvgFPS, int(nTriTotal*m_AvgFPS), (char*)m_Win32MemoryStatusStr);

#elif defined(PLATFORM_PS3)
						CDA_MemoryManager* pMemMgr = MRTC_GetMemoryManager();
						CFStr FPSStr;
						if (m_bShowFPSOnly)
						{
							FPSStr = CFStrF("%3.1f", CurFPS);
						}
						else
						{
							FPSStr = CFStrF("Mem: %0.1f, Min %0.1f(L %0.1f)/%0.1f U %0.1f %-9d", 
								(float)pMemMgr->GetFreeMem() / (1024.0f* 1024.0f), 
								(float)pMemMgr->GetMinFreeMem() / (1024.0f* 1024.0f),
								(float)pMemMgr->GetLargestFreeMem() / (1024.0f* 1024.0f), 
								(float)pMemMgr->m_AllocatedMem / (1024.0f* 1024.0f), 
								(float)pMemMgr->GetUsedMem() / (1024.0f* 1024.0f),
								int(nTriTotal*CurFPS)
								);
						}
#else // !PLATFORM_WIN
						CFStr FPSStr = CFStrF("%.2f(%.2f, %d)", CurFPSLast, CurFPS, int(nTriTotal*CurFPS));
#endif // PLATFORM_WIN

						CRC_Attributes *pAttr = Util2D.GetAttrib();

						pAttr->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
						pAttr->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	

#ifdef PLATFORM_CONSOLE
						Util2D.SetCoordinateScale(CVec2Dfp32(1.0f, 1.0f));
						int Width = (int)(m_spDebugFont->GetWidth(m_spDebugFont->GetOriginalSize(), FPSStr.Str()));
						Util2D.Rect(_ConsoleClip, CRct(30, 23, Width + 30, 35), CPixel32(0,0,0,128));
						Util2D.Text(_ConsoleClip, m_spDebugFont, 35, 25, (char*) FPSStr, 0xffffffff);
#else // !PLATFORM_CONSOLE
						Util2D.Text(_ConsoleClip, m_spDebugFont, 0, 0, (char*) FPSStr, 0xffffffff);
#endif // PLATFORM_CONSOLE
					}
#endif // defined(M_Profile) || defined (PLATFORM_WIN)

					if (m_BuildMsg != "")
					{
						Util2D.Text(_ConsoleClip, m_spDebugFont, 0, 0, m_BuildMsg.Str(), 0xffffffff);
					}


					if (!m_bShowConsole && m_bShowStats)
					{
#ifdef PLATFORM_CONSOLE
						Util2D.SetCoordinateScale(CVec2Dfp32(1.0,1.0));
#else
						Util2D.SetCoordinateScale(CVec2Dfp32(1,1));
#endif
						CRC_Attributes *pAttr = Util2D.GetAttrib();
						pAttr->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
						pAttr->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
						int MaxX = 0;
						int MaxY = 0;
						for (int j = 0; j < 2; ++j)
						{
							int x = 60;
							for (int i = 0; i <= 2; ++i)
							{
								const char * Stats = _pRC->GetRenderingStatus(i);
								mint Len = strlen(Stats);
								int Pos = 0;
								int y = 50;
								while(Pos < Len)
								{
									const char* pS = (char*)Stats;
									static wchar Buff[8192];
									int Pos2 = Pos;
									int yAdd = 7;
									while(Pos2 < Len && pS[Pos2] != ',')
									{
										Buff[Pos2-Pos] = (uint8)pS[Pos2];
										char moo = '§';
										if (pS[Pos2] == moo && pS[Pos2+1] == 'n')
										{
											int dy = ((uint8)pS[Pos2 + 2] - '0') * 100;
											dy += ((uint8)pS[Pos2 + 3] - '0') * 10;
											dy += ((uint8)pS[Pos2 + 4] - '0') * 1;
											yAdd += dy;
										}
										Pos2++;
									}
									Buff[Pos2-Pos] = 0;

									if (j == 1)
										Util2D.Text(_ConsoleClip, m_spDebugFont, x, y, Buff, 0xff3f90af);
									y += yAdd;
									Pos = Pos2+1;
									if (y > MaxY)
										MaxY = y;
									if (pS[Pos] == '\n')
									{
										++Pos;
										y = 50;
										x += 150;
									}

								}

								if(_pVBM && i == 0)
								{
									int VBHeapAvail = _pVBM->GetAvail();
									int VBHeapSize = _pVBM->GetHeap();

									y	+= 8;
									if (j == 1)
										Util2D.Text(_ConsoleClip, m_spDebugFont, x, y, CFStrF("VBHeap %dKiB/%dKiB", VBHeapAvail / 1024, VBHeapSize / 1024).GetStr(), 0xff3f90af);
									y	+= 8;
								}

								x += 150;
								if (x > MaxX)
									MaxX = x;
								if (y > MaxY)
									MaxY = y;
							}
							if (j == 0)
							{
								Util2D.Rect(_ConsoleClip, CRct(60-5, 50-5, MaxX+10, MaxY+10), CPixel32(0,0,0,128));
							}
						}

					}

#ifdef PLATFORM_WIN
					if (m_bShowMemInfo)
					{
						Win32_UpdateMemoryStatus();

						CRC_Attributes* pAttr = Util2D.GetAttrib();
						pAttr->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
						pAttr->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
						Util2D.SetCoordinateScale(CVec2Dfp32(1.0f, 1.0f));

						int Width = (int)(m_spDebugFont->GetWidth(m_spDebugFont->GetOriginalSize(), m_MemStatusMini.Str()));
						Util2D.Rect(_ConsoleClip, CRct(30, 23, Width + 40, 35), CPixel32(0,0,0,128));
						Util2D.Text(_ConsoleClip, m_spDebugFont, 35, 25, m_MemStatusMini.Str(), 0xffffffff);

						// Show current level + spawnflags        TODO: don't use "ShowMemInfo" for this!!!
						CWorld_Server* pWServer = m_spGame ? m_spGame->m_spWServer : NULL;
						if (pWServer)
						{
							int Height = (int)(m_spDebugFont->GetHeight(m_spDebugFont->GetOriginalSize(), m_MemStatusMini.Str()));
							CFStr Str = CFStrF("Level: %s  - SpawnFlags: %s", pWServer->m_WorldName.Str(), pWServer->World_GetCurrSpawnFlagsStr());

							int Width = (int)(m_spDebugFont->GetWidth(m_spDebugFont->GetOriginalSize(), Str.Str()));
							Util2D.Rect(_ConsoleClip, CRct(30, 23 + Height, Width + 40, 35 + Height), CPixel32(0,0,0,128));
							Util2D.Text(_ConsoleClip, m_spDebugFont, 35, 25 + Height, Str.Str(), 0xffffffff);
						}
					}
#endif

#if defined(PLATFORM_XBOX) && !defined(M_RTM)
					if (m_bShowSoundDbg)
#else
					if (m_bShowSoundDbg)
#endif
					{
						if (m_spSoundContext)
						{
							const char **ppSoundDbg = m_spSoundContext->GetDebugStrings();
							if (ppSoundDbg)
							{
								Util2D.SetCoordinateScale(CVec2Dfp32(1.0f,1.0f));
#ifdef PLATFORM_CONSOLE
//								Util2D.SetCoordinateScale(CVec2Dfp32(2.0f,2.0f));
#endif
								CRC_Attributes *pAttr = Util2D.GetAttrib();
								pAttr->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
								pAttr->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
								int y = 40;
								int x = 60;

								while(*ppSoundDbg)
								{
									Util2D.Text(_ConsoleClip, m_spDebugFont, x, y, *ppSoundDbg, 0xff3f90af);
									y += 8;

									++ppSoundDbg;
								}
							}
						}
					}

#ifndef M_RTM
					if(m_bShowAnimationsDbg)
					{
						if(m_spGame && m_spGame->m_spWServer)
						{
							CWObject_Message Msg(OBJSYSMSG_GETDEBUGSTRING, 1);
							CStr St;
							Msg.m_pData = &St;
							Msg.m_DataSize = sizeof(&St);
							CWObject_Game* pGame = m_spGame->m_spWServer->Game_GetObject();

							if (pGame)
							{
								CWO_Player* pPlayer = pGame->Player_Get(0);
								if (pPlayer)
								{
									if(m_spGame->m_spWServer->Message_SendToObject(Msg, pPlayer->m_iObject))
									{
										//										LogFile(St);
										CRC_Attributes *pAttr = Util2D.GetAttrib();
										pAttr->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
										pAttr->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
										int y = 40;
										int x = 0;

										while(St != "")
										{
											CStr Line = St.GetStrSep("|");
											Util2D.Text(_ConsoleClip, m_spDebugFont, x, y, Line, 0xff3f90af);
											y += 8;
										}
									}
								}
							}
						}
					}
#endif

#ifdef M_Profile
					if (m_bShowCallGraph)
					{
						MSCOPE(ShowCallGraph, XRAPP_RENDER);

						if (m_bShowCallGraphAutoUpdate)
						{
							MRTC_GetObjectManager()->m_pCallGraph->GetStrList(m_CallGraphStrings);
						}

						if (m_CallGraphStrings.Len())
						{
							fp32 scalex = 1.0f;
							fp32 scaley = 1.0f;
#if defined( PLATFORM_CONSOLE )
							scaley = 1.3f;
#endif
							Util2D.SetCoordinateScale(CVec2Dfp32(scalex,scaley));
							CRct Rect = pVP->GetViewRect();

							CRC_Attributes *pAttr = Util2D.GetAttrib();
							pAttr->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
							pAttr->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
							int y = 20;
							int x = 10;

							for (int i = 0; i < m_CallGraphStrings.Len(); ++i)
							{
								Util2D.Text(_ConsoleClip, m_spDebugFont, x, y, m_CallGraphStrings[i], 0xff3f90af);
								y += 8;
								if (y*scaley > (Rect.GetHeight() - 20))
								{
									y = 40;
									x += 450;
								}
							}
						}
					}
#endif
					int iGraphY = 20;
					int iGraphX = _ConsoleClip.GetWidth()-20;
					int GraphWidth = 60 * 4;
					int GraphHeight = 100;

					/*
					if (m_bShowFPS || m_bShowStats)
					{
						Util2D.SetCoordinateScale(CVec2Dfp32(1.0,1.0));

						fp32 Value[60];
						int nToDraw = 60;

						{
							DLock(m_MainThreadStats_Lock);
							m_MainThread_FrameTime.FillHistoryInv(Value, nToDraw);
						}

						Util2D.DrawGraph(_ConsoleClip, m_spDebugFont, CRct(iGraphX-GraphWidth,iGraphY,iGraphX,iGraphY+GraphHeight), 
							CPixel32(0,0,0,128), CPixel32(255,50,50,255), CPixel32(255,255,50,255), CPixel32(50,255,50,255), 
							CurFPS, Value, nToDraw, 30.0, "FPS History"); iGraphY += GraphHeight + 20;
					}*/

					if (m_bShowFPSLite)
					{
						CRC_Statistics Stats = _pRC->Statistics_Get();
						Util2D.SetCoordinateScale(CVec2Dfp32(1.0,1.0));

						fp32 Avg[3];

						fp32 Value[3][60];
						int nToDraw = 60;
						CPixel32 Colors[3];

						Colors[0] = CPixel32(255,50,50,255);
						Colors[1] = CPixel32(50,255,50,255);
						Colors[2] = CPixel32(50,50,255,255);

						// Main fps
						Stats.m_Time_FrameTime.FillHistoryInv(Value[0], nToDraw);
						Util2D.DrawGraph(_ConsoleClip, m_spDebugFont, CRct(iGraphX-GraphWidth,iGraphY,iGraphX,iGraphY+GraphHeight), 
							CPixel32(0,0,0,128), CPixel32(255,50,50,255), CPixel32(255,255,50,255), CPixel32(50,255,50,255), 
							1.0/Stats.m_Time_FrameTime.m_Average, Value[0], nToDraw, 30.0, "Real FPS"); iGraphY += GraphHeight + 20;

						const fp32 *pValues[3] = {Value[0], Value[1], Value[2]};

						{
							DLock(m_MainThreadStats_Lock);
							Avg[0] = 1.0/(Stats.m_Time_FrameTime.m_Average - m_MainThread_VBMWait.m_Average - Stats.m_Time_FrameTime.m_Average * (100.0 - Stats.m_CPUUsage.m_Average) * 0.01);
							Stats.m_Time_FrameTime.FillHistorySubRelationInv(Stats.m_CPUUsage, m_MainThread_VBMWait, Value[0], nToDraw);
						}
						{
							DLock(m_SystemStats_Lock);
							Avg[1] = 1.0/(m_SystemThread_FrameTime.m_Average - m_SystemThread_VBMWait.m_Average);
							m_SystemThread_FrameTime.FillHistorySubInv(m_SystemThread_VBMWait, Value[1], nToDraw);
						}
						{
							Avg[2] = 1.0/(Stats.m_Time_FrameTime.m_Average - Stats.m_Time_FrameTime.m_Average * (100.0 - Stats.m_GPUUsage.m_Average) * 0.01);
							Stats.m_Time_FrameTime.FillHistoryRelationInv(Stats.m_GPUUsage, Value[2], nToDraw);
						}


						Util2D.DrawGraph(_ConsoleClip, m_spDebugFont, CRct(iGraphX-GraphWidth,iGraphY,iGraphX,iGraphY+GraphHeight), 
							CPixel32(0,0,0,128), nToDraw, 30.0, "PFPS (Main,Sys,GPU)", 3, Colors, Colors, Colors,
							Avg, pValues); iGraphY += GraphHeight + 20;

						{
							DLock(m_MainThreadStats_Lock);
							Avg[0] = 100 - (((m_MainThread_VBMWait.m_Average / m_MainThread_FrameTime.m_Average) * 100.0) + (100.0 - Stats.m_CPUUsage.m_Average));
							m_MainThread_VBMWait.FillHistoryRelationSub(m_MainThread_FrameTime, Stats.m_CPUUsage, Value[0], nToDraw);
						}
						{
							DLock(m_SystemStats_Lock);
							Avg[1] = 100.0 - (m_SystemThread_VBMWait.m_Average / m_SystemThread_FrameTime.m_Average) * 100.0;
							m_SystemThread_VBMWait.FillHistoryRelation(m_SystemThread_FrameTime, Value[1], nToDraw);
						}
						{
							Avg[2] = Stats.m_GPUUsage.m_Average;
							Stats.m_GPUUsage.FillHistory(Value[2], nToDraw);
						}


						Util2D.DrawGraph(_ConsoleClip, m_spDebugFont, CRct(iGraphX-GraphWidth,iGraphY,iGraphX,iGraphY+GraphHeight), 
							CPixel32(0,0,0,128), nToDraw, 100.0, "Usage (Main,Sys,GPU)", 3, Colors, Colors, Colors,
							Avg, pValues); iGraphY += GraphHeight + 20;
					}
					else if (m_bShowStats || m_bShowFPS)
					{
						CRC_Statistics Stats = _pRC->Statistics_Get();
						Util2D.SetCoordinateScale(CVec2Dfp32(1.0,1.0));

						fp32 Avg;

						fp32 Value[60];
						int nToDraw = 60;


						Stats.m_Time_FrameTime.FillHistoryInv(Value, nToDraw);
						Util2D.DrawGraph(_ConsoleClip, m_spDebugFont, CRct(iGraphX-GraphWidth,iGraphY,iGraphX,iGraphY+GraphHeight), 
							CPixel32(0,0,0,128), CPixel32(255,50,50,255), CPixel32(255,255,50,255), CPixel32(50,255,50,255), 
							1.0/Stats.m_Time_FrameTime.m_Average, Value, nToDraw, 30.0, "Main Thread FPS"); iGraphY += GraphHeight + 20;

						{
							DLock(m_MainThreadStats_Lock);
							Avg = 100 - (((m_MainThread_VBMWait.m_Average / m_MainThread_FrameTime.m_Average) * 100.0) + (100.0 - Stats.m_CPUUsage.m_Average));

							m_MainThread_VBMWait.FillHistoryRelationSub(m_MainThread_FrameTime, Stats.m_CPUUsage, Value, nToDraw);
						}
						Util2D.DrawGraph(_ConsoleClip, m_spDebugFont, CRct(iGraphX-GraphWidth,iGraphY,iGraphX,iGraphY+GraphHeight), 
							CPixel32(0,0,0,128), CPixel32(255,50,50,255), CPixel32(255,255,50,255), CPixel32(50,255,50,255), 
							Avg, Value, nToDraw, 100.0, "Main Thread Active"); iGraphY += GraphHeight + 20;

						Stats.m_CPUUsage.FillHistoryComplemnt(Value, nToDraw);
						Util2D.DrawGraph(_ConsoleClip, m_spDebugFont, CRct(iGraphX-GraphWidth,iGraphY,iGraphX,iGraphY+GraphHeight), 
							CPixel32(0,0,0,128), CPixel32(50,255,50,255), CPixel32(255,255,50,255), CPixel32(255,50,50,255), 
							100.0-Stats.m_CPUUsage.m_Average, Value, nToDraw, 100.0, "Waiting for GPU"); iGraphY += GraphHeight + 20;

						Stats.m_BlockTime.FillHistory(Value, nToDraw);
						Util2D.DrawGraph(_ConsoleClip, m_spDebugFont, CRct(iGraphX-GraphWidth,iGraphY,iGraphX,iGraphY+GraphHeight), 
							CPixel32(0,0,0,128), CPixel32(50,255,50,255), CPixel32(255,255,50,255), CPixel32(255,50,50,255), 
							Stats.m_BlockTime.m_Average, Value, nToDraw, 100.0, "Blocked"); iGraphY += GraphHeight + 20;

						Stats.m_nPixlesTotal.FillHistory(Value, nToDraw);
						Util2D.DrawGraph(_ConsoleClip, m_spDebugFont, CRct(iGraphX-GraphWidth,iGraphY,iGraphX,iGraphY+GraphHeight), 
							CPixel32(0,0,0,128), CPixel32(255,50,50,255), CPixel32(255,255,50,255), CPixel32(50,255,50,255), 
							Stats.m_nPixlesTotal.m_Average, Value, nToDraw, 1000000.0, "Raster pixels"); iGraphY += GraphHeight + 20;


						iGraphX -= GraphWidth + 20;
						iGraphY = 13;

						Util2D.SetCoordinateScale(CVec2Dfp32(1.0,1.0));

						{
							DLock(m_SystemStats_Lock);
							Avg = 1.0 / m_SystemThread_FrameTime.m_Average;
							m_SystemThread_FrameTime.FillHistoryInv(Value, nToDraw);
						}
						Util2D.DrawGraph(_ConsoleClip, m_spDebugFont, CRct(iGraphX-GraphWidth,iGraphY,iGraphX,iGraphY+GraphHeight), 
							CPixel32(0,0,0,128), CPixel32(255,50,50,255), CPixel32(255,255,50,255), CPixel32(50,255,50,255), 
							Avg, Value, nToDraw, 30.0, "Game Thread FPS"); iGraphY += GraphHeight + 20;

						{
							DLock(m_SystemStats_Lock);
							Avg = 100.0 - (m_SystemThread_VBMWait.m_Average / m_SystemThread_FrameTime.m_Average) * 100.0;
							m_SystemThread_VBMWait.FillHistoryRelation(m_SystemThread_FrameTime, Value, nToDraw);
						}
						Util2D.DrawGraph(_ConsoleClip, m_spDebugFont, CRct(iGraphX-GraphWidth,iGraphY,iGraphX,iGraphY+GraphHeight), 
							CPixel32(0,0,0,128), CPixel32(255,50,50,255), CPixel32(255,255,50,255), CPixel32(50,255,50,255), 
							Avg, Value, nToDraw, 100.0, "Game Thread Active"); iGraphY += GraphHeight + 20;

						{
							DLock(m_SystemStats_Lock);
							Avg = m_SystemThread_FrameLatency.m_Average;
							m_SystemThread_FrameLatency.FillHistory(Value,nToDraw);
						}
						Util2D.DrawGraph(_ConsoleClip, m_spDebugFont, CRct(iGraphX-GraphWidth,iGraphY,iGraphX,iGraphY+GraphHeight), 
							CPixel32(0,0,0,128), CPixel32(255,50,50,255), CPixel32(255,255,50,255), CPixel32(50,255,50,255), 
							Avg, Value, nToDraw, 0.1, "Frame Latency"); iGraphY += GraphHeight + 20;

						Stats.m_GPUUsage.FillHistory(Value, nToDraw);
						Util2D.DrawGraph(_ConsoleClip, m_spDebugFont, CRct(iGraphX-GraphWidth,iGraphY,iGraphX,iGraphY+GraphHeight), 
							CPixel32(0,0,0,128), CPixel32(255,50,50,255), CPixel32(255,255,50,255), CPixel32(50,255,50,255), 
							Stats.m_GPUUsage.m_Average, Value, nToDraw, 100.0, "GPU Usage"); iGraphY += GraphHeight + 20;

						Stats.m_nPixlesZPass.FillHistory(Value, nToDraw);
						Util2D.DrawGraph(_ConsoleClip, m_spDebugFont, CRct(iGraphX-GraphWidth,iGraphY,iGraphX,iGraphY+GraphHeight), 
							CPixel32(0,0,0,128), CPixel32(255,50,50,255), CPixel32(255,255,50,255), CPixel32(50,255,50,255), 
							Stats.m_nPixlesZPass.m_Average, Value, nToDraw, 1000000.0, "Framebuffer Pixels"); iGraphY += GraphHeight + 20;

					}

#ifdef M_Profile
					// Show "profiling in progress" indicator
					if (m_ProfilerFramesLeft)
					{
						CMTime t; t.Snapshot(); t = t.Modulus(0.20f);
						bool bBlink = t.GetTime() > 0.10f;
						uint32 fg = bBlink ? 0xFFFFFFFF : 0xFF000000;
						uint32 bg = bBlink ? 0xFF000000 : 0xFFFFFFFF;

						const char* pText = "profiling";
						fp32 tw = m_spDebugFont->GetWidth(m_spDebugFont->GetOriginalSize(), pText);
						fp32 th = m_spDebugFont->GetHeight(m_spDebugFont->GetOriginalSize(), pText);
						fp32 x0 = _ConsoleRect.p1.x - tw - 30.0f, y0 = 20.0f;

						CRC_Attributes *pAttr = Util2D.GetAttrib();
						pAttr->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
						pAttr->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
						Util2D.Rect(_ConsoleClip, CRct((int)(x0 - 5), (int)(y0 - 5), (int)(x0 + tw + 5), (int)(y0 + th + 5)), bg);
						Util2D.Text(_ConsoleClip, m_spDebugFont, (int)x0, (int)y0, pText, fg);
					}
#endif

					//if(m_bShowBorders)
					if(0)
					{
						CRC_Attributes *pAttr = Util2D.GetAttrib();
						pAttr->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
						pAttr->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
						Util2D.SetCoordinateScale(CVec2Dfp32(1,1));
						int32 w = (int32)(640*0.15f/2);
						int32 h = (int32)(480*0.15f/2);
						Util2D.Line(_ConsoleClip, CPnt(w,h), CPnt(640-w,h), CPixel32(255,255,255,255));
						Util2D.Line(_ConsoleClip, CPnt(640-w,h), CPnt(640-w,480-h), CPixel32(255,255,255,255));
						Util2D.Line(_ConsoleClip, CPnt(640-w,480-h), CPnt(w,480-h), CPixel32(255,255,255,255));
						Util2D.Line(_ConsoleClip, CPnt(w,480-h), CPnt(w,h), CPixel32(255,255,255,255));

						w = (int32)(640*0.10f/2);
						h = (int32)(480*0.10f/2);
						Util2D.Line(_ConsoleClip, CPnt(w,h), CPnt(640-w,h), CPixel32(255,0,0,255));
						Util2D.Line(_ConsoleClip, CPnt(640-w,h), CPnt(640-w,480-h), CPixel32(255,0,0,255));
						Util2D.Line(_ConsoleClip, CPnt(640-w,480-h), CPnt(w,480-h), CPixel32(255,0,0,255));
						Util2D.Line(_ConsoleClip, CPnt(w,480-h), CPnt(w,h), CPixel32(255,0,0,255));
					}

				}

			}
		}

		Util2D.End();
	}
}

//
//
//
void CXRealityApp::OnRender(CDisplayContext* _pDisplay, CImage* _pFrame, CClipRect _Clip, int _Context)
{
	MSCOPE(CXRealityApp::OnRender, XRAPP_RENDER);

	m_pSystem->m_spCon->WriteExceptions();

	m_pSystem->m_spTC->Refresh();
	CMTime Time;

	// -------------------------------------------------------------------
	CRct Rect(_Clip.clip);
	Rect += _Clip.ofs;

	m_spViewport->SetView(_Clip, Rect);
 
	CRC_Viewport Viewport2D;
	Viewport2D.SetView(_Clip, Rect);
	
	if(m_spGame) m_spGame->DetermineWidescreen();

	CXR_VBManager* pVBM = NULL;
	{
		M_NAMEDEVENT("GetAvailVBM", 0xff00ff00);

		pVBM = m_spVBMContainer->GetAvailVBM(0.0f);
		if(m_spGame && (m_spGame->m_Mode & WMODE_TIMEDEMO)) 
		{
			while (!pVBM)
				pVBM = m_spVBMContainer->GetAvailVBM(0.5f);
		}
	}


#ifdef LOG_FRAME_INFO
	CLogFrame * pLogFrame = (m_iCurrentSys < LATENCY_LOG_FRAMES) ? &m_lLatencyLogSys[m_iCurrentSys] : NULL;	
	if( pLogFrame )
	{
		uint8 i;
		for(i = 0;i < 10;i++)
		{
			if( m_lpVBM[i] == NULL )
			{
				m_lpVBM[i] = pVBM;
				break;
			}
			else if( m_lpVBM[i] == pVBM )
			{
				break;
			}
		}
		pLogFrame->m_iColor = i;
	}
#endif

	if( !pVBM ) return;
	pVBM->SetOwner(MRTC_SystemInfo::OS_GetThreadID());
	m_pVBM = pVBM;

	CRCLock RCLock;
	CRenderContext* pRC = NULL;
#if !defined( PLATFORM_DOLPHIN ) && !defined( PLATFORM_PS2 )
	spCRenderContextCapture spCapture;
	if (m_bCaptureNext)
	{
		pRC = _pDisplay->GetRenderContext(&RCLock);

		m_bCaptureNext = false;
		spCapture = MNew(CRenderContextCapture);
		if (!spCapture)
			MemError("OnRender");
		spCapture->Create(NULL, "");
		if (pRC)
			spCapture->Attrib_GlobalSetVar(4, pRC->Attrib_GlobalGetVar(4));
		pRC = spCapture;
	}
	else
#endif
		pRC = _pDisplay->GetRenderContext(&RCLock);

	CMTime CMFrameTime;

	if (pRC)
	{
//		const char * Stats = pRC->GetRenderingStatus();

#ifndef	PLATFORM_PS2
		// Set gamma ramp scale.

		{
//			int bFake = m_pSystem->GetEnvironment()->GetValuei("vid_fakeoverbright", FAKEOVERBRIGHT) || !_pDisplay->IsFullScreen();
//			fp32 Contrast = 1.0 + ((m_pSystem->GetEnvironment()->GetValuef("VID_CONTRAST", 0.5f) - 0.5)) * 0.5;
//			float GammaScale = m_pSystem->GetEnvironment()->GetValuef("vid_overbright", 2.0f) * Contrast;
//			if (bFake)
//				GammaScale *= 0.5f;
			fp32 GammaScale = 1.0f;
			CVec4Dfp32 RampScale;
			RampScale.k[0] = RampScale.k[1] = RampScale.k[2] = RampScale.k[3] = GammaScale;
			pRC->Attrib_GlobalSetVarfv(CRC_GLOBALVAR_GAMMARAMPSCALE, RampScale.k);

			// Set gamma.
	//		RampScale[0] = m_pSystem->GetEnvironment()->GetValuef("vid_gamma", 1.0f);
	//		pRC->Attrib_GlobalSetVarfv(CRC_GLOBALVAR_GAMMA, RampScale.k);
		}
#endif
		m_spViewport->Refresh();

		// Create Viewports
		CRC_Viewport _3DVP  = *m_spViewport;
		CRC_Viewport _GUIVP = *m_spViewport;
		if (m_spGame != NULL)
		{
			m_spGame->GetViewport(&_3DVP, 0);
			m_spGame->GetViewport(&_GUIVP, 1);
		}
		else
		{
			CGameContext::GetViewport(m_pSystem, &_3DVP, 0);
			CGameContext::GetViewport(m_pSystem, &_GUIVP, 1);
		}

		CClipRect	_ConsoleClip = Viewport2D.GetViewClip(); 
		CRct		_ConsoleRect = Viewport2D.GetViewArea(); 


		{

			MSCOPE(BeginScene, XRAPP_RENDER);
			m_pVBM->Begin(pRC, &Viewport2D);

			{
				MSCOPE(Scene, XRAPP_RENDER);
				if(m_spGame != NULL)
				{
					if (m_pVBM->Viewport_Push(m_spViewport))
					{
						m_spGame->Render(m_pVBM, pRC, _3DVP, _GUIVP, _Context);
						m_pVBM->Viewport_Pop();
					}
				}
			}

			{
				MSCOPE(Console, XRAPP_RENDER);

	/*			bool bGameAvail = (m_spGame != NULL) ? m_spGame->CanRenderGame() : false;

				if (_Context)
					m_ConRender.SetMode(CRC_CONSOLERENDER_FULL);
				else
					m_ConRender.SetMode(m_bShowConsole ? ((bGameAvail) ? CRC_CONSOLERENDER_HALF : CRC_CONSOLERENDER_FULL) : ((m_bConsoleInvis) ? CRC_CONSOLERENDER_INVISIBLE : CRC_CONSOLERENDER_OVERLAY));
	*/
				m_ConRender.SetMode(m_bShowConsole ? (CRC_CONSOLERENDER_HALF) : ((m_bConsoleInvis) ? CRC_CONSOLERENDER_INVISIBLE : CRC_CONSOLERENDER_OVERLAY));
				if(!m_bConsoleInvis || m_ConRender.GetMode() != CRC_CONSOLERENDER_FULL)
				{
					m_pVBM->ScopeBegin("Console", false);
					// This viewport push isn't really required since the vbm is initialized with the same viewport
					if (m_pVBM->Viewport_Push(&Viewport2D))
					{
						m_ConRender.SetVBM(m_pVBM);
						m_ConRender.PrepareFrame(pRC, m_pVBM);
						m_ConRender.RenderRectShaded(m_pSystem->m_spCon, Viewport2D.GetViewClip(), Viewport2D.GetViewArea());

						m_pVBM->Viewport_Pop();
					}
					m_pVBM->ScopeEnd();
				}
			}

			// Stats
			{
				m_pVBM->ScopeBegin("Stats", false);
				// This viewport push isn't really required since the vbm is initialized with the same viewport
				if (m_pVBM->Viewport_Push(&Viewport2D))
				{
					M_LOCK(m_RenderGUILock);
					RenderStats(pRC->GetDC(), pRC, m_pVBM, _ConsoleClip, _ConsoleRect);
					m_pVBM->Viewport_Pop();
				}
				m_pVBM->ScopeEnd();
			}


//			m_pVBM->End();
		}
		// Lock render scope
	}

	m_pVBM->VBE_GetScopeInfo(m_VBEContext);

	m_spVBMContainer->AddDirtyVBM(m_pVBM);

	//For frame latency logging
	for(int i = 0;i < 3;i++)
	{
		if( m_lpStack[i] == NULL )
		{
			m_lpStack[i] = m_pVBM;
		}
		if( m_lpStack[i] == m_pVBM )
		{
			m_lTimeStack[i] = m_LatencyTime;
#ifdef LOG_FRAME_INFO
			m_lFrameNumber[i] = m_FrameCounter++;
#endif
			break;
		}
	}

	m_pVBM = NULL;

#if !defined( PLATFORM_DOLPHIN ) && !defined( PLATFORM_PS2 )
	if (spCapture!=NULL)
	{
		MSCOPE(CaptureEndScene, XRAPP_RENDER);
		pRC->EndScene();
		spCapture->GetCaptureBuffer()->Write("CAPTURE.XSC");
	}
#endif	
}

//
//
//
void CXRealityApp::OnBusy(int _Context)
{
	MSCOPE(CXRealityApp::OnBusy, XRAPP);

	if (MRTC_SystemInfo::OS_GetThreadID() != m_MainThreadID)
		return;

	if(m_bLoading)
		return;

//	if (m_pSystem->m_spInput!=NULL)
//		DoInput(true);

	if (m_pSystem->IsRendering())
		return;

	// Keep the updatespeed to max 10hz
	static CMTime LastCPUTime;
#ifdef	PLATFORM_PS2
	MRTC_SystemInfo::CPU_AdvanceClock( 0.0f );
#endif
//	CMTime TrueCPUTime = CMTime::GetCPU();
	CMTime TrueCPUTime;
	TrueCPUTime.Snapshot();
	if((TrueCPUTime - LastCPUTime).GetTime() > 0.1f)
	{
		LastCPUTime = TrueCPUTime;
		m_pSystem->Render(NULL, _Context);
	}
}

#if defined(MRTC_MEMORYDEBUG) && defined(PLATFORM_WIN)
	#include "crtdbg.h"
	#define CHECKMEMORY(s) { if (!_CrtCheckMemory()) Error(s, "Memory check failure."); }
#else
	#define CHECKMEMORY(s)
#endif


#ifdef PLATFORM_XBOX1
#include "D3d8perf.h"
#endif

//
//
//

void CXRealityApp::DoRemoteDebuggerInput()
{
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	{
		mint Size = 0;
		uint8 *pPacket = (uint8 *)m_pSystem->m_spCon->m_pRDChannel->GetHeadPacketData(Size);
		while (pPacket)
		{
			bint bDownKey = *pPacket; ++pPacket;
			if (bDownKey)
			{
				int32 ScanCode = *((int32 *)pPacket); pPacket += 4;
				int32 Char = *((int32 *)pPacket); pPacket += 4;
				int32 nRep = *((int32 *)pPacket); pPacket += 4;
				int32 Data0 = *((int32 *)pPacket); pPacket += 4;
				int32 Data1 = *((int32 *)pPacket); pPacket += 4;
				SwapLE(ScanCode);
				SwapLE(Char);
				SwapLE(nRep);
				SwapLE(Data0);
				SwapLE(Data1);
				switch (ScanCode)
				{
				case SKEY_MOUSE1:
				case SKEY_MOUSE2:
				case SKEY_MOUSE3:
				case SKEY_MOUSE4:
				case SKEY_MOUSE5:
				case SKEY_MOUSE6:
				case SKEY_MOUSE7:
				case SKEY_MOUSE8:
				case SKEY_MOUSEWHEELDOWN:
				case SKEY_MOUSEWHEELUP:
					{
						Data0 = m_pSystem->m_spInput->GetMousePosition().x;
						Data1 = m_pSystem->m_spInput->GetMousePosition().y;
					}
					break;
				case SKEY_MOUSEMOVE:
				case SKEY_MOUSEMOVEREL:
					{
						ScanCode += (m_pSystem->m_spInput->GetMouseButtons() << 16);
					}
					break;					
				}
				m_pSystem->m_spInput->RD_DownKey(ScanCode, Char, 0, nRep, Data0, Data1);
			}
			else
			{
				int32 ScanCode = *((int32 *)pPacket); pPacket += 4;
				int32 Data0 = *((int32 *)pPacket); pPacket += 4;
				int32 Data1 = *((int32 *)pPacket); pPacket += 4;
				SwapLE(ScanCode);
				SwapLE(Data0);
				SwapLE(Data1);
				switch (ScanCode)
				{
				case SKEY_MOUSE1:
				case SKEY_MOUSE2:
				case SKEY_MOUSE3:
				case SKEY_MOUSE4:
				case SKEY_MOUSE5:
				case SKEY_MOUSE6:
				case SKEY_MOUSE7:
				case SKEY_MOUSE8:
				case SKEY_MOUSEWHEELDOWN:
				case SKEY_MOUSEWHEELUP:
					{
						Data0 = m_pSystem->m_spInput->GetMousePosition().x;
						Data1 = m_pSystem->m_spInput->GetMousePosition().y;
					}
					break;
				case SKEY_MOUSEMOVE:
				case SKEY_MOUSEMOVEREL:
					{
						ScanCode += (m_pSystem->m_spInput->GetMouseButtons() << 16);
					}
					break;					
				}
				m_pSystem->m_spInput->RD_UpKey(ScanCode,  0, Data0, Data1);
			}

			m_pSystem->m_spInput->Update();

			m_pSystem->m_spCon->m_pRDChannel->PopPacket();
			pPacket = (uint8 *)m_pSystem->m_spCon->m_pRDChannel->GetHeadPacketData(Size);
		}
	}
	{
		mint Size = 0;
		uint8 *pPacket = (uint8 *)m_pSystem->m_spCon->m_pRDChannelTrace->GetHeadPacketData(Size);
		while (pPacket)
		{
			MRTC_SystemInfo::OS_TraceRaw((const char *)pPacket);
			m_pSystem->m_spCon->m_pRDChannelTrace->PopPacket();
			pPacket = (uint8 *)m_pSystem->m_spCon->m_pRDChannelTrace->GetHeadPacketData(Size);
		}
	}
#endif
}

void CXRealityApp::DoInput(bool _FromBusy)
{
	// This function can be called form various places.
	// That caouses some strange problems so make sure
	// that it doesn't recurse.
	static bool Running = false;
	if(Running)
		return;
	Running = true;


	bool bCorrupt = false;
	if (CDiskUtil::GetCorrupt() & DISKUTIL_STATUS_CORRUPTANY)
	{
		bCorrupt = true;
#ifdef M_DEMO_XBOX
		XBoxDemo_ResetTimer();
		//m_LastDownKey = CMTime::GetCPU();
#endif

	}

	m_pSystem->m_spInput->Update();
	DoRemoteDebuggerInput();

	if(m_spGame)
		m_spGame->UpdateControllerStatus();

	M_TRY
	{
		MSCOPESHORT(ProcessKeys);
		// Process all keys until queue is empty. 'key' means any kind of input; keyboard, mouse and joysticks.
		CScanKey lKeys[16];
		uint nKeys = m_pSystem->m_spInput->GetScanKeys(lKeys, 16);
		while (nKeys > 0) 
		{
			for(uint iK = 0; iK < nKeys; iK++)
			{
				CScanKey key = lKeys[iK];
	#ifdef M_DEMO_XBOX
				if (bCorrupt)
				{
					if (g_XboxDemoParams.m_bLaunchedFromDemoLauncher && key.IsDown() && key.GetKey9() == SKEY_JOY0_AXIS00)
						m_pSystem->m_bBreakRequested = true;

					continue;
				}
	#endif

				
	#ifdef M_DEMO_XBOX
				if (key.IsDown())
				{
					g_XboxDemoParams.m_KioskMode = false;
					XBoxDemo_ResetTimer();
				}
	#endif

 				if (m_bLogKeys)
				{
					if (key.GetKey9() > 0 && key.GetKey9() != SKEY_MOUSEMOVE && key.GetKey9() != SKEY_MOUSEMOVEREL)
					{
						ConOut(CStrF("Key pressed: %d Key32: 0x%08x Data: %d, %d Localized: %s", key.GetKey9(), key.m_ScanKey32, key.m_Data[0], key.m_Data[1], CScanKey::GetLocalizedKeyName(key.GetKey9()).Ansi().Str()));
					}
					else if (key.IsASCII())
						ConOut(CStrF("Asci Key: %d (%c)", key.GetASCII(), key.GetASCII()));
				}

		
				//ConOutL(CStrF("key.m_ScanKey32 %.8x", key.m_ScanKey32));
	#ifdef M_RTM
				if (key.m_ScanKey32 == (SKEY_PARAGRAPH | SKEY_MODIFIER_CONTROL | SKEY_MODIFIER_ALT))
	#else
				if (key.m_ScanKey32 == SKEY_PARAGRAPH || key.m_ScanKey32 == (SKEY_PARAGRAPH | SKEY_MODIFIER_CONTROL | SKEY_MODIFIER_ALT))
	#endif
				{

					// '§' Toggle console
					if (m_pSystem->m_spCon->GetMode() == CONST_CONSOLE_INPUTKEY)
						m_pSystem->m_spCon->SetMode(CONST_CONSOLE_EXECUTEKEY);
					else
						m_pSystem->m_spCon->SetMode(CONST_CONSOLE_INPUTKEY);

				}
				else if (m_pSystem->m_spCon->GetMode() == CONST_CONSOLE_INPUTKEY && !(key.GetKey9() == SKEY_LEFT_CONTROL || key.GetKey9() == SKEY_LEFT_ALT || key.GetKey9() == SKEY_RIGHT_CONTROL || key.GetKey9() == SKEY_RIGHT_ALT))
				{
					// We're in console typing mode
					m_pSystem->m_spCon->ProcessKey(key, CONST_CONSOLE_AUTO);
				}
				else if (key.m_ScanKey32 == (SKEY_RETURN | SKEY_MODIFIER_ALT))
				{
					CRegistry *pGlobalOpt = m_pSystem->GetRegistry()->FindChild("OPTG");
					if (!pGlobalOpt)
					{
						pGlobalOpt = m_pSystem->GetRegistry()->CreateDir("OPTG");		
					}

					pGlobalOpt->SetValuei("VIDEO_DISPLAY_FULLSCREEN", !pGlobalOpt->GetValuei("VIDEO_DISPLAY_FULLSCREEN"));
					m_bPendingOptionsUpdate = true;
				}
				else
				{
					bool bProcessed = false;

					// Paragraph keys
					if (m_bDebugKeys && !bProcessed)
					{
						bProcessed = true;
						switch (key.m_ScanKey32) 
						{					
						case SKEY_PARAGRAPH+SKEY_MODIFIER_SHIFT+SKEY_MODIFIER_CONTROL : 
							m_bShowFPS ^= TRUE; break;
						case SKEY_PARAGRAPH+SKEY_MODIFIER_CONTROL : 
							{
								m_bShowFPSLite ^= true;
								ConExecute(CStrF("xr_ppexposuredebug(%d)", m_bShowFPSLite));
								break;
							}
						case SKEY_PARAGRAPH+SKEY_MODIFIER_SHIFT : 
							m_bShowStats ^= true; break;
						case SKEY_PARAGRAPH+SKEY_MODIFIER_ALT : 
							m_bShowVBE ^= true; break;

						default:
							bProcessed = false;
							break;
						}
					}

					// VBE input
					if (!bProcessed && m_bShowVBE)
						bProcessed = m_VBEContext.ProcessKey(key);

					// Game input
					if (!bProcessed && (m_spGame != NULL))
						bProcessed = m_spGame->ProcessKey(key);

	#ifdef M_Profile
					// Debug keys
					if (m_bDebugKeys && !bProcessed)
					{
						bProcessed = true;
						switch (key.m_ScanKey32) 
						{					
						case SKEY_U+SKEY_MODIFIER_CONTROL : 
							MRTC_GetObjectManager()->m_pCallGraph->GetStrList(m_CallGraphStrings); break;
						case SKEY_G+SKEY_MODIFIER_CONTROL : 
							m_bShowCallGraph ^= TRUE; break;
						case SKEY_G+SKEY_MODIFIER_SHIFT : 
							m_bShowCallGraphAutoUpdate = !m_bShowCallGraphAutoUpdate; break;
						case SKEY_R+SKEY_MODIFIER_CONTROL : 
							m_bOnRenderCallGraph = !m_bOnRenderCallGraph; break;
						case SKEY_U+SKEY_MODIFIER_SHIFT : 
							{	
								if (m_bShowCallGraphAutoUpdate)
								{
									MRTC_GetObjectManager()->m_pCallGraph->GetStrList(m_CallGraphStrings);
								}

								if (m_CallGraphStrings.Len())
								{
									for (int i = 0; i < m_CallGraphStrings.Len(); ++i)
									{
										LogFile(m_CallGraphStrings[i]);
									}
								}
								break;
							}
						case SKEY_P + SKEY_MODIFIER_CONTROL : 
							MRTC_GetObjectManager()->m_pCallGraph->Start(4.0f); break;

						case SKEY_S+SKEY_MODIFIER_CONTROL : 
							m_bShowSoundDbg ^= TRUE; break;

							/*					case SKEY_A+SKEY_MODIFIER_CONTROL : 
							m_bShowAnimationsDbg ^= TRUE; break;*/

						case SKEY_Z+2 + SKEY_MODIFIER_CONTROL :
							{
								// Ctrl+C, renders next frame to capture-renderer and then writes the 
								// capture render-context to CAPTURE.XSC

								//m_bCaptureNext = true;
								// Disabled, since it crashed 041007 -JA
								break;
							}					
						default:
							bProcessed = false;
							break;
						}
					}
	#endif

					if (!bProcessed)
					{
						// Not processed, execute any binding associated with the key
						if (!_FromBusy && !m_pSystem->m_spCon->ProcessKey(key, CONST_CONSOLE_EXECUTEKEY)) 
						{
						}
					}
				}
			}
			
			if (m_pSystem->m_bBreakRequested) break;
			
			// Read some more keys if there are any
			nKeys = m_pSystem->m_spInput->GetScanKeys(lKeys, 16);
		}
	}
	M_CATCH(
	catch(CCException) 
	{
		Running = false;
		throw;
	};
	)

	Running = false;
}

#if 0
class CFileAsyncWriteFillDirOptions_Test : public CFileAsyncWriteFillDirOptions
{
public:
	char m_Data[16*1024];
	CFileAsyncWriteFillDirOptions_Test()
	{
		memset(m_Data, 0, 16*1024);
		m_pFillData = m_Data;

		m_pDirectory = "z:\\";
		m_pCalcSizeSearchString = "z:\\*";
		m_pFillFileName = "_filler";
		m_FillDataSize = 16*1024;
		m_FillSize = 1024*1024;
		m_PerFileSize = 128*1024;
	}
	
	void InitFile(mint _Size)
	{
		*((uint32 *)m_pFillData) = _Size;
	}
};
#endif


void CXRealityApp::DoModal()
{
	if (m_pSystem->m_bBreakRequested)
		return;

	LogFile("(CXRealityApp_DoModal) Begin");
	
#ifdef PLATFORM_WIN_PC
	if (m_spLogWin!=NULL)
	{
		MRTC_GOM()->UnregisterObject((CReferenceCount*)m_spLogWin, "SYSTEM.LOG");
		m_spLogWin = NULL;
	}
#endif

	// Init video
//	m_pSystem->DC_InitFromEnvironment();

//	spCDisplayContextContainer spDC2;
//	spDC2 = m_pSystem->CreateDisplayContext("OpenGL");
//	spDC2->m_spDC->Win32_SetPosition(CRct(700, 100, 320, 200));
//	spDC2->m_spDC->SetMode(0);


	// CDA
/*	if (m_spCDAudio!=NULL)
	{
		try { m_spCDAudio->Refresh(); }
		catch(CCException)
		{
			m_pSystem->m_spCon->WriteExceptions();
			ConOut("§fc80WARNING: CD-Audio shut-down.");
			LogFile("§cf80WARNING: CD-Audio shut-down.");
			m_spCDAudio = NULL;
		}
	}*/
	
//	m_pSystem->m_spCon->ExecuteString("map(elm2ff)");

#if 0
	CFileAsyncWriteChunk Chunks[3];

	Chunks[0].m_MemorySize = 100000;
	Chunks[0].m_pMemory = M_ALLOC(100000);
	memset((void *)Chunks[0].m_pMemory, 0, 100000);

	Chunks[1].m_MemorySize = 100000;
	Chunks[1].m_pMemory = M_ALLOC(100000);
	memset((void *)Chunks[1].m_pMemory, 0xf5, 100000);

	Chunks[2].m_MemorySize = 100000;
	Chunks[2].m_pMemory = M_ALLOC(100000);
	memset((void *)Chunks[2].m_pMemory, 0x03, 100000);
	
	CFileAsyncWrite Test;
	Test.SaveFile("z:\\Test.bin", Chunks, 3);
	Test.SaveFile("z:\\Test1.bin", Chunks, 3);
	Test.SaveFile("z:\\Test2.bin", Chunks, 3);
	Test.DelFile("z:\\Test?.bin");
	Test.DelFile("z:\\_filler???");

	CFileAsyncWriteFillDirOptions_Test FillDir;

	Test.FillDirecotry(&FillDir);

#endif

	// Capture stuff
	TArray<spCImage> lspCapture;
	lspCapture.SetGrow(512);
//	int CaptureFrame = 0;

	LogFile("(CXRealityApp_DoModal) pSys->Render...");
	m_pSystem->Render();

	if (!m_pSystem->m_spDisplay)
		return;

#ifdef PLATFORM_WIN32_PC
	{
		CRCLock RCLock;
		CRenderContext* pRC = m_pSystem->m_spDisplay->GetRenderContext(&RCLock);
		int Caps = (pRC) ? pRC->Caps_Flags() : 0;
		if (!(Caps & CRC_CAPS_FLAGS_MATRIXPALETTE) || !(Caps & CRC_CAPS_FLAGS_CUBEMAP) || !(Caps & CRC_CAPS_FLAGS_TEXENVMODE_COMBINE))
		{
			LogFile(CStrF("ERROR: Insufficient render capabilities: %.x8", Caps));

			m_pSystem->DC_Set(-1);

			CStr Msg = Localize_Str(CStr("§LSYS_INSUFFICIENTRENDERERFEATURES"));
			CStr Title = Localize_Str(CStr("§LSYS_APPNAME"));

			MessageBoxW(NULL, (LPCWSTR)Msg.Unicode().StrW(), (LPCWSTR)Title.Unicode().StrW(), MB_OK | MB_ICONEXCLAMATION);
			return;
		}
	}
#endif

	LogFile("(CXRealityApp_DoModal) pCon->SetMode...");
	m_pSystem->m_spCon->SetMode((m_bShowConsole) ? CONST_CONSOLE_INPUTKEY : CONST_CONSOLE_EXECUTEKEY);

	LogFile("(CXRealityApp_DoModal) Entering main loop...");

//	int m_LastActive = -1;

//	int MemTestWait = 0;
	
#ifdef M_DEMO_XBOX
	XBoxDemo_ResetTimer();
#endif

	m_spVBMContainer->Clean();

	MACRO_GetSystemEnvironment(pEnv);
	m_bMultithreaded = m_bEnableMultithreaded = pEnv->GetValuei("ENABLE_MULTITHREAD", STARTUP_MULTITHREADSTATE) != 0;

	// Create systemthread anv move ownership of VBM over to systemthread, wonder if this is soon enough...
	MRTC_ThreadPoolManager::Enable(m_bMultithreaded != 0);
	if(m_bMultithreaded) m_SystemThread.Create(this, m_spVBMContainer, m_pSystem->m_spDisplay, m_spGame);


	fp32 WaitTimeOutDefault = 1.0 / 20.0; // Try to keep 60 fps;
	fp32 WaitTimeout = WaitTimeOutDefault;

#ifdef LOG_FRAME_INFO
	m_iCurrentMain = LATENCY_LOG_FRAMES;
	m_iCurrentSys = LATENCY_LOG_FRAMES;
	memset(m_lpVBM,0,10*sizeof(CXR_VBManager*));
#endif

	memset(m_lpStack,0,3*sizeof(CXR_VBManager*));

#ifdef M_Profile
//	m_FPSChecker.SetFPS(15);
//	m_FPSChecker.SetBreak(true);
#endif


	// The main loop for the app.
	while (!m_pSystem->m_bBreakRequested)
	{
		// refresh system
		m_pSystem->Refresh();

		if (m_bEnableMultithreaded != m_bMultithreaded)
		{
			MRTC_ThreadPoolManager::Enable(m_bEnableMultithreaded != 0);
			if(m_bEnableMultithreaded)
			{
				m_SystemThread.Create(this, m_spVBMContainer, m_pSystem->m_spDisplay, m_spGame);
			}
			else
			{
				m_SystemThread.Destroy();
				m_spVBMContainer->Clean();
			}

			m_bMultithreaded	= m_bEnableMultithreaded;
		}
		
		if(!m_bMultithreaded)
		{
#ifdef PLATFORM_XENON
			bint bTraceRecordSystem = s_bTraceRecordSystem;
			if (bTraceRecordSystem)
				XTraceStartRecording( "cache:\\TraceRecord_SystemST.bin" );
#endif
			SystemThread(m_pSystem->m_spDisplay);
#ifdef PLATFORM_XENON
			if (bTraceRecordSystem)
			{
				XTraceStopRecording();
				s_bTraceRecordSystem = false;
			}
#endif
		}

		CDisplayContext* pDC = m_pSystem->m_spDisplay;
		CRCLock RCLock;
		CRenderContext* pRC = m_pSystem->m_spDisplay->GetRenderContext(&RCLock);

		CXR_VBManager* pVBM = NULL;
		CMTime Time;
		Time.Snapshot();

#ifdef LOG_FRAME_INFO
		CLogFrame * pLogFrame = (m_iCurrentMain < LATENCY_LOG_FRAMES) ? &m_lLatencyLogMain[m_iCurrentMain] : NULL;
		if( pLogFrame )
		{
			m_iCurrentMain ++;
			if( m_iCurrentMain == LATENCY_LOG_FRAMES )
			{
				m_iCurrentMain--;
				SaveLog();
				m_iCurrentMain = LATENCY_LOG_FRAMES;
			}
			else
			{
				pLogFrame->m_BreakStart = pLogFrame->m_Start = Time.GetTime();
				pLogFrame->m_BreakEnd = pLogFrame->m_End = pLogFrame->m_Start;
				pLogFrame->m_iColor = 5;
			}
		}
#endif


		{

			// Stress test sound system
			static bint bPlaySounds = true;
#if 0

			if (bPlaySound
			{
				CWaveContext *pWC = m_spSoundContext->Wave_GetContext();
				int iWave = pWC->GetWaveID("wp_tun_spas12_01");
				
				m_spSoundContext->Voice_Create(0, iWave, 0, 4.0, 1.0, false);
				
				bPlaySounds = false;
			}
#elif 0
			if (bPlaySounds)
			{
				static int iSound = 0;
				static bint bWrapped = false;
				CWaveContext *pWC = m_spSoundContext->Wave_GetContext();
				if (bWrapped)
				{
					if (pWC->IsWaveValid(iSound))
					{
						if (m_spSoundContext->Voice_Create(0, iSound, 0, Random * 20.0, 1.0, false))
							++iSound;
					}
				}
				else
				{
					if (pWC->IsWaveValid(iSound))
					{
						if (m_spSoundContext->Voice_Create(0, iSound, 0, 1.0, 1.0, false))
							++iSound;
					}
					if (pWC->IsWaveValid(iSound))
					{
						if (m_spSoundContext->Voice_Create(0, iSound, 0, 1.0, 1.0, false))
							++iSound;
					}
					if (pWC->IsWaveValid(iSound))
					{
						if (m_spSoundContext->Voice_Create(0, iSound, 0, 1.0, 1.0, false))
							++iSound;
					}
					if (pWC->IsWaveValid(iSound))
					{
						if (m_spSoundContext->Voice_Create(0, iSound, 0, 1.0, 1.0, false))
							++iSound;
					}
				}

				while (!pWC->IsWaveValid(iSound))
				{
					++iSound;
					if (iSound >= pWC->GetIDCapacity())
					{
						iSound = 0;
						bWrapped = true;
//						bPlaySounds = false;
					}
				}
				if (iSound >= pWC->GetIDCapacity())
				{
					iSound = 0;
					bWrapped = true;
//						bPlaySounds = false;
				}
			}
#endif

#ifdef PLATFORM_XENON
			static bint bTraceRecord = false;
			if (bTraceRecord)
				XTraceStartRecording( "cache:\\MainThread.bin" );
#endif

			if (!(pVBM = m_spVBMContainer->GetDirtyVBM(WaitTimeout)))
			{
				CMTime Time2;
				Time2.Snapshot();
				m_MainThread_VBMWait.AddData((Time2 - Time).GetTime());

				CMTime CMFrameTime = CMTime::CreateFromSeconds(0);
				if (!m_MainThread_LastTime.IsInvalid())
					CMFrameTime = Time2 - m_MainThread_LastTime;

				if (CMFrameTime.GetTime() < 5.0)
				{
			
					if (!m_spGame || m_spGame->CanRenderGame())
					{
						if(m_spGame && m_spGame->m_spFrontEnd != NULL)
							m_spGame->m_spFrontEnd->m_GameIsLoading = false;

						pRC->GetDC()->Update();
						WaitTimeout = WaitTimeOutDefault;
						continue;
					}
					pVBM = m_spVBMContainer->GetAvailVBM();
					if(!pVBM)
					{
						pRC->GetDC()->Update();
						continue;
					}

		#ifdef LOG_FRAME_INFO
					if( pLogFrame )
					{
						uint8 i;
						for(i = 0;i < 10;i++)
						{
							if( m_lpVBM[i] == NULL )
							{
								m_lpVBM[i] = pVBM;
								break;
							}
							else if( m_lpVBM[i] == pVBM )
							{
								break;
							}
						}
						pLogFrame->m_iColor = i;
						pLogFrame->m_BreakEnd = Time2.GetTime();
					}
		#endif

					pVBM->SetOwner(MRTC_SystemInfo::OS_GetThreadID());

					WaitTimeout = 0.0f;
					CRC_Viewport VP3D, VPGUI;
					VP3D = *m_spViewport;
					VPGUI = VP3D;
					m_spGame->GetViewport(&VP3D, 0);
					m_spGame->GetViewport(&VPGUI, 1);
					pVBM->Begin(pRC, &VP3D);
					pVBM->ScopeBegin("m_spGame->RenderGUI", false);
					m_spGame->RenderGUI(pVBM, pRC, VP3D, VPGUI, 0);
					pVBM->ScopeEnd();

					CRC_Viewport Viewport2D = *pVBM->Viewport_Get(1);
					CClipRect	_ConsoleClip = Viewport2D.GetViewClip(); 
					CRct		_ConsoleRect = Viewport2D.GetViewArea(); 

					pVBM->ScopeBegin("Stats", false);
					pVBM->Viewport_Push(&Viewport2D);
					{
						M_LOCK(m_RenderGUILock);
						RenderStats(pDC, pRC, pVBM, _ConsoleClip, _ConsoleRect);
					}
					pVBM->Viewport_Pop();
					pVBM->ScopeEnd();
					pVBM->End();
				}
			}
			else
			{
				WaitTimeout = WaitTimeOutDefault;
				CMTime Time2;
				Time2.Snapshot();
				m_MainThread_VBMWait.AddData((Time2 - Time).GetTime());

	#ifdef LOG_FRAME_INFO
				if( pLogFrame )
				{
					uint8 i;
					for(i = 0;i < 10;i++)
					{
						if( m_lpVBM[i] == NULL )
						{
							m_lpVBM[i] = pVBM;
							break;
						}
						else if( m_lpVBM[i] == pVBM )
						{
							break;
						}
					}
					pLogFrame->m_iColor = i;
					pLogFrame->m_BreakEnd = Time2.GetTime();
				}
	#endif

				pVBM->SetOwner(MRTC_SystemInfo::OS_GetThreadID());
				pVBM->End(); // Sort etc
			}

			if (pVBM)
			{
				CRC_Viewport Viewport2D = *pVBM->Viewport_Get(1);
				CClipRect	_ConsoleClip = Viewport2D.GetViewClip(); 
				CRct		_ConsoleRect = Viewport2D.GetViewArea(); 

				// Clear
				{
					CPixel32 ClearColor(0);
					if (m_spGame)
					{
						// Should we use this crap still?  -mh
						CWorld_Client* pClient = m_spGame->GetCurrentClient();

						if (pClient)
						{
							CXR_Engine *pEngine = pClient->Render_GetEngine();
							if (pEngine)
							{
								CXR_FogState* pFogState = pEngine->m_pCurrentFogState;
 								if (pFogState)
									ClearColor = pFogState->m_DepthFogColor;
							}
						}
					}
					MSCOPE(ClearFrameBuffer, XRAPP_RENDER);

					pRC->Viewport_Push();	
					pRC->Viewport_Set(&Viewport2D);

					int ClearFlags = CDC_CLEAR_ZBUFFER | CDC_CLEAR_STENCIL;
					if (m_pSystem->GetEnvironment()->GetValuei("CLEAR", 1, 0))
						ClearFlags |= CDC_CLEAR_COLOR;
					pDC->ClearFrameBuffer(ClearFlags, ClearColor);

					pRC->Viewport_Pop();	
				}

				{
#ifdef PLATFORM_XENON
					bint bTraceRecordRender = s_bTraceRecordRender;
					if (bTraceRecordRender)
						XTraceStartRecording( "cache:\\TraceRecord_Render.bin" );

					bint bPerfCountRender = s_bPerfCountRender;
					if (bPerfCountRender)
					{
						PMCInstallSetup( &PMCDefaultSetups[PMC_SETUP_OVERVIEW_PB0T0] );
						// Reset the Performance Monitor Counters in preparation for a new sampling run.
						PMCResetCounters();
						// Start up the Performance Monitor Counters.
						PMCStart(); // Start up counters
					}
#endif


					pRC->BeginScene(&Viewport2D);
					int DrawMode = m_DrawMode;
					pRC->Render_EnableHardwareMemoryRegion(pVBM, m_spVBMContainer);


					if (m_bShowVBE)
					{
						m_VBEContext.SetScreenSize(m_pSystem->m_spDisplay);
						CXR_VBEContext EC;
						EC = m_VBEContext;
						CRC_Util2D Util2D;
						EC.m_pUtil2D = &Util2D;
						EC.m_pFont = m_ConRender.m_pFont;

						if(!(DrawMode & 2))
							pVBM->Render(pRC, 0, &EC);	// Render everything
						else
							pVBM->Render(pRC, 4, &EC);	// Render everything that does not have wire flags

						m_VBEContext.m_CurrentVB = EC.m_CurrentVB;
					}
					else
					{
						if(!(DrawMode & 2))
							pVBM->Render(pRC, 0);		// Render everything
						else
							pVBM->Render(pRC, 4);		// Render everything that does not have wire flags
					}

					if(DrawMode & 1)
					{
						pRC->Attrib_Push();
						pRC->Attrib_GlobalEnable(CRC_GLOBALFLAGS_WIRE);
						CRC_Attributes Attr;
						Attr.SetDefault();

						Attr.Attrib_TextureID(0, pRC->Texture_GetTC()->GetTextureID("SPECIAL_WIREGREEN"));
						Attr.Attrib_TexEnvMode(0, CRC_TEXENVMODE_REPLACE);
						Attr.Attrib_RasterMode(CRC_RASTERMODE_ADD);
						Attr.Attrib_Disable(CRC_FLAGS_ZCOMPARE);
						pRC->Attrib_Set(Attr);

						pVBM->Render(pRC, 2);
						pRC->Attrib_GlobalDisable(CRC_GLOBALFLAGS_WIRE);
						pRC->Attrib_Pop();
					}

					pRC->Render_DisableHardwareMemoryRegion();

					pRC->EndScene();

#ifdef PLATFORM_XENON
					if (bTraceRecordRender)
					{
						XTraceStopRecording();
						s_bTraceRecordRender = false;
					}
					if (bPerfCountRender)
					{
						PMCStop();
						// Get the counters.
						PMCGetCounters( &m_PMCState);
						// Print out detailed information about all 16 counters.
						PMCDumpCountersVerbose( &m_PMCState, PMC_VERBOSE_NOL2ECC );
						s_bPerfCountRender = false;
					}
#endif
				}


		#ifdef LOG_FRAME_INFO
				if( pLogFrame )
				{
					Time.Snapshot();
					pLogFrame->m_End = Time.GetTime();
				}
		#endif
			}

			// Finished with VBM add to avail array
	//		m_spVBMContainer->AddAvailVBM(pVBM);
			// This is now done through renderer

			{
				MSCOPE(PageFlip, XRAPP_RENDER);
				bool bShouldFlip = true;
				/*
				// When we are loading the game restrict flipping to 10 Hz
				if (m_spGame && m_spGame->m_spWData && (m_spGame->m_spWData->Resource_AsyncCacheGetEnabled() & 6))
				{
				static CMTime LastCPUTime;
				CMTime TrueCPUTime = CMTime::GetCPU();
				if((TrueCPUTime - LastCPUTime).GetTime() > 0.1f)
				{
				LastCPUTime = TrueCPUTime;
				bShouldFlip = true;
				}
				}
				else
				bShouldFlip = true;
				*/
				if (bShouldFlip)
				{
#ifdef M_Profile
					m_FPSChecker.DoneFrame();
#endif


					pDC->PageFlip();
					CMTime CMFrameTime;
					// -------------------------------------------------------------------
					//  FPS-History
					CMTime Time;
					Time.Snapshot();
					CMFrameTime = Time - m_MainThread_LastTime;
					if (m_MainThread_LastTime.IsReset())
						CMFrameTime = CMTime();
					m_MainThread_LastTime = Time;
					{
						DLock(m_MainThreadStats_Lock);
						m_MainThread_FrameTime.AddData(CMFrameTime.GetTime());
					}

					for(int i = 0;i < 3;i++)
					{
						if( m_lpStack[i] == pVBM ) 
						{
							DLock(m_MainThreadStats_Lock);
							m_SystemThread_FrameLatency.AddData((Time - m_lTimeStack[i]).GetTime());

	#ifdef LOG_FRAME_INFO
							if( pLogFrame )
							{
								pLogFrame->m_iNumber = m_lFrameNumber[i];
							}
	#endif
							break;
						}
					}


					if (0)
					{
	#if 0
						CRC_Statistics Stat = pRC->Statistics_Get();
						CRC_Statistics *pStat = &Stat;
						if (pStat)
						{
							m_Statistics.m_ScreenSize = pRC->GetDC()->GetScreenSize();
							CRC_Viewport VP = *pRC->Viewport_Get();
							m_Statistics.m_3DVP = CPnt(VP.GetViewRect().GetWidth(), VP.GetViewRect().GetHeight());
							m_Statistics.m_RefreshRate = pDC->GetRefreshRate();
							pStat->m_Time_FrameTime = CMFrameTime.GetTime();
							bool bCanChange = true;

							if (m_spGame)
							{
								if (m_spGame->m_PauseCount || m_spGame->m_bTimeLeap)
									bCanChange = false;
							}

							CVec2Dfp32 Scale = m_Statistics.Feed(*pStat, bCanChange);

							if (m_bAutomaticScreenResize && m_spGame)
							{	
								m_spGame->Con_ViewScale(Scale.k[0], Scale.k[1]);
							}

							fp32 RefreshRate = pDC->GetRefreshRate();
							fp32 MaxUsage = Max(m_Statistics.m_StatAvg.m_CPUUsage, m_Statistics.m_StatAvg.m_GPUUsage);
							fp32 FrameTime;

							if (m_LastSetInterval == 1)
								FrameTime = MaxUsage * m_Statistics.m_StatAvg.m_Time_FrameTime * 1.03f;   // 10 % marginal
							else
								FrameTime = MaxUsage * m_Statistics.m_StatAvg.m_Time_FrameTime * 1.07f;   // 10 % marginal

							fp32 DesiredFrameTime = 1.0f / RefreshRate;
							int Interval = M_Ceil(FrameTime / DesiredFrameTime);
							if (Interval > 3) // Don't go over 3 or automatic resize wont work
								Interval = 3;

							++m_RefreshIntervalCanDo[Interval];

		#ifdef WCLIENT_FIXEDRATE
							Interval = 2;
		#endif


							if (Interval != m_LastInterval)
							{
								m_RefreshIntervalCanDo[m_LastInterval] = 0;
								if (Interval > m_LastInterval)
									pRC->Flip_SetInterval(m_LastSetInterval = Interval); // Always go down at once
							}

							if (m_RefreshIntervalCanDo[Interval] > 10) // Wait before we go to a higher FPS to demote twitches
								pRC->Flip_SetInterval(m_LastSetInterval = Interval);

							m_LastInterval = Interval;


						}
	#endif
					}
				}
			}

	#ifdef PLATFORM_XENON
			if (bTraceRecord)
			{
				XTraceStopRecording();
				bTraceRecord = false;
			}
	#endif

		}
	}

	// Signal context as available we won't have to timeout to kill the thread
	m_SystemThread.Thread_RequestTermination();
	m_SystemThread.Destroy();

	// Set main thread as scriptthread
	MRTC_GOM()->m_hScriptThread = MRTC_SystemInfo::OS_GetThreadID();

	LogFile("");
	LogFile("----------------------------------------------------------------------------------");
	LogFile("(CXRealityApp_DoModal) Shutting down...");

	m_bLoading = true;
	m_spEngine = NULL;
	m_spDebugFont = NULL;
	m_ConRender.m_pFont = NULL;
	if(m_spGame != NULL)
	{
		MRTC_GOM()->UnregisterObject((CReferenceCount*)m_spGame, "GAMECONTEXT");
		m_spGame = NULL;
	}

	m_pSystem->DC_Set(-1);

	LogFile("(CXRealityApp_DoModal) Done");
}


#ifdef LOG_FRAME_INFO
//Log latency
void CXRealityApp::SaveLog()
{
	M_TRY
	{
		uint32 nFrames = Min(m_iCurrentMain,m_iCurrentSys);
		CCFile file;
		file.Open(m_FrameLogFile,CFILE_WRITE);

		file.Writeln(CStrF("%d",nFrames));	//Frames
		file.Writeln(CStrF("%d",2));					//Clock count
		for(int i = 0;i < nFrames;i++)
		{
			file.Writeln(CStrF("%f",m_lLatencyLogSys[i].m_Start));
			file.Writeln(CStrF("%f",m_lLatencyLogSys[i].m_End));
			file.Writeln(CStrF("%f",m_lLatencyLogSys[i].m_BreakStart));
			file.Writeln(CStrF("%f",m_lLatencyLogSys[i].m_BreakEnd));
			file.Writeln(CStrF("%d",m_lLatencyLogSys[i].m_iColor));
			file.Writeln(CStrF("%d",m_lLatencyLogSys[i].m_iNumber));

			file.Writeln(CStrF("%f",m_lLatencyLogMain[i].m_Start));
			file.Writeln(CStrF("%f",m_lLatencyLogMain[i].m_End));
			file.Writeln(CStrF("%f",m_lLatencyLogMain[i].m_BreakStart));
			file.Writeln(CStrF("%f",m_lLatencyLogMain[i].m_BreakEnd));
			file.Writeln(CStrF("%d",m_lLatencyLogMain[i].m_iColor));
			file.Writeln(CStrF("%d",m_lLatencyLogMain[i].m_iNumber));
		}

		file.Close();
	}
	M_CATCH(
		catch(CCExceptionFile)
	{
		Error("SaveLog",CStrF("Unable to save file %s",m_FrameLogFile.Str()));
	}
	)
}
#endif


#ifdef M_Profile
void CXRealityApp::Con_StartProfiler(int /*_nFrames*/)
{
	if (m_ProfilerFramesLeft > 0)
	{
		// Stop profiling
		m_ProfilerFramesLeft = -1;
	}
	else
	{
		// Start profiling
		m_ProfilerFramesLeft = 1;
		//m_ProfilerFramesLeft = _nFrames;
		m_ProfilerStrings.QuickSetLen(0);
		m_ProfilerStrings.SetGrow(50);
	}
}

void CXRealityApp::Con_CheckFPS(int _nFrames, int _bBreak)
{
	m_FPSChecker.SetFPS(_nFrames);
	m_FPSChecker.SetBreak(_bBreak !=0);
}
#endif




void CXRealityApp::Con_SetDefaultOptionsGlobal()
{
	if (m_spGame && m_spGame->m_bBenchmark)
		return;

//	CRegistry *pEnv = m_pSystem->GetEnvironment();

	CRegistry *pGlobalOpt = m_pSystem->GetRegistry()->FindChild("OPTG");
	if (!pGlobalOpt)
	{
		pGlobalOpt = m_pSystem->GetRegistry()->CreateDir("OPTG");		
	}

	pGlobalOpt->SetValuei("UPDATEFROMENV", 2);
	pGlobalOpt->SetValuef("VIDEO_GAMMA", 0.5f);
	pGlobalOpt->SetValuef("VIDEO_BLACKLEVEL", 0.5f);
	pGlobalOpt->SetValuef("VIDEO_BRIGHTNESS", 0.5f);

	pGlobalOpt->SetValuef("SOUND_USE3D", 1);
	pGlobalOpt->SetValuef("SOUND_USEEAX", 0);
	pGlobalOpt->SetValuei("GAMEPAD_ENABLE", 1);


	pGlobalOpt->SetValuei("VIDEO_DISPLAY_ANTIALIAS", 1);
	pGlobalOpt->SetValuei("VIDEO_DISPLAY_HDR", IMAGE_FORMAT_RGBA8);
	pGlobalOpt->SetValuei("VIDEO_DISPLAY_FULLSCREEN", 1);
	pGlobalOpt->SetValuei("VIDEO_DISPLAY_WIDTH", 640);
	pGlobalOpt->SetValuei("VIDEO_DISPLAY_HEIGHT", 480);
	pGlobalOpt->SetValuei("VIDEO_DISPLAY_REFRESH", 85);
	pGlobalOpt->SetValuef("VIDEO_DISPLAY_PIXELASPECT", 1.0);
	pGlobalOpt->SetValuei("VIDEO_DISPLAY_VSYNC", 1);

	pGlobalOpt->SetValuei("VIDEO_MODELQUALITY", 0);	

	pGlobalOpt->SetValuef("VIDEO_TEXQUALITY", s_DefaultTextureQuality);
	pGlobalOpt->SetValuei("VIDEO_CHARSHADOWS", 1);
	pGlobalOpt->SetValuef("VIDEO_ANISOTROPY", 1.0);

	pGlobalOpt->SetValuei("VIDEO_SHADERMODE", 0);

	pGlobalOpt->SetValuei("MP_TIME", 10);
	pGlobalOpt->SetValuei("MP_SCORE", 20);
	pGlobalOpt->SetValuei("MP_CAPTURE", 5);
	pGlobalOpt->SetValuei("MP_PRIVATESLOTS", 1);
	
	Con_UpdateOptions();
}

//
//
//
void CXRealityApp::Con_SetDefaultOptions()
{
	if (m_spGame && m_spGame->m_bBenchmark)
		return;

	CRegistry *pOptions = m_pSystem->GetOptions();
//	CRegistry *pEnv = m_pSystem->GetEnvironment();

	CRegistry *pGlobalOpt = m_pSystem->GetRegistry()->FindChild("OPTG");
	if (!pGlobalOpt)
	{
		pGlobalOpt = m_pSystem->GetRegistry()->CreateDir("OPTG");		
	}

	if (!pGlobalOpt->GetValuei("UPDATEFROMENV", 0))
		pGlobalOpt->SetValuei("UPDATEFROMENV", 1);

	if(!pOptions)
	{
		pOptions = m_pSystem->GetOptions(true);
		if (!pOptions)
			Error("CreateSystems", "Internal error. (1)");
	}
//	pOptions->EnableAutoHashing(true);
//	pEnv->EnableAutoHashing(true);	
//	pGlobalOpt->EnableAutoHashing(true);

	pOptions->SetValuef("SND_SFXMSXBALANCE", 0.5f);
	pOptions->SetValuef("SND_VOLUMESFX", 1.0f);
	pOptions->SetValuef("SND_VOLUMEMUSIC", 0.7f);

	pOptions->SetValuef("GAME_BLOOD", 1);
	pOptions->SetValuef("CONTROLLER_TYPE", 0);

	pOptions->SetValuef("FOCUSFRAME_ENABLED", 1);

	pOptions->SetValuef("CONTROLLER_SENSITIVTY", 0.5f);
	pOptions->SetValuef("GAME_VIBRATION", 1);
	pOptions->SetValuef("CONTROLLER_INVERTYAXIS", 0);

	pOptions->SetValuef("GAME_SUBTITLE_INTERACTIVE", 0);
	pOptions->SetValuef("GAME_SUBTITLE_CUTSCENE", 0);
	pOptions->SetValuef("GAME_SUBTITLE_CASUAL", 0);
	pOptions->SetValuef("GAME_SUBTITLE_FIGHTING", 0);

	pOptions->SetValue("MP_HUMANTEMPLATE", "multiplayer_Mob_01");
	pOptions->SetValue("MP_DARKLINGTEMPLATE", "multiplayer_Darkling_05");
	pOptions->SetValuef("CONTROLLER_AUTOAIM", 0.5f);

	if(m_spGame)
		m_spGame->SetDefaultProfileSettings(pOptions);

	pGlobalOpt->SetValuei("MP_TIME", 10);
	pGlobalOpt->SetValuei("MP_SCORE", 20);
	pGlobalOpt->SetValuei("MP_CAPTURE", 5);

	// set default
	Options_SetDefaultBinds();

	Con_UpdateOptions();
}

//
//
//
void CXRealityApp::Con_SaveOptions()
{
//	CRegistry* optReg = m_pSystem->GetOptions();
//	if(optReg) 
//	{
//		m_pSystem->WriteOptions();
//	}
}

//
//
//
#ifndef M_RTM
void CXRealityApp::Con_DumpBadFPS(int _Fps)
{
	m_BadFPSDump = _Fps;
}
#endif

void CXRealityApp::Con_OptionsGetFromEnv()
{
	if (m_spGame && m_spGame->m_bBenchmark)
		return;

	CRegistry *pGlobalOpt = m_pSystem->GetRegistry()->FindChild("OPTG");
	if (!pGlobalOpt)
	{
		pGlobalOpt = m_pSystem->GetRegistry()->CreateDir("OPTG");		
	}
	CRegistry *pEnv = m_pSystem->GetEnvironment();

	pGlobalOpt->SetValuei("sound_use3d", pEnv->GetValuei("snd_use3d", 1));
	pGlobalOpt->SetValuei("sound_useeax", pEnv->GetValuei("snd_useeax", 0));
	pGlobalOpt->SetValuei("GAMEPAD_ENABLE", pEnv->GetValuei("IN_JoyEnable", 1));
	

	pGlobalOpt->SetValuef("VIDEO_GAMMA", pEnv->GetValuef("VID_GAMMA", 0.5f));
	pGlobalOpt->SetValuef("VIDEO_BLACKLEVEL", pEnv->GetValuef("VID_BLACKLEVEL", 0.5f));
	pGlobalOpt->SetValuef("VIDEO_BRIGHTNESS", pEnv->GetValuef("VID_BRIGHTNESS", 0.5f));

	pGlobalOpt->SetValuef("VIDEO_DISPLAY_PIXELASPECT", pEnv->GetValuef("VID_PIXELASPECT", 1.0f));
	pGlobalOpt->SetValuei("VIDEO_DISPLAY_VSYNC", pEnv->GetValuei("VID_VSYNC", 1));

	if (pEnv->GetValuef("XR_LODOFFSET", 0) <= -1000000.0)
		pGlobalOpt->SetValuei("VIDEO_MODELQUALITY", 1);
	else
		pGlobalOpt->SetValuei("VIDEO_MODELQUALITY", 0);	

	pGlobalOpt->SetValuei("GAME_COMMENTARY", pEnv->GetValuei("GAME_COMMENTARY", 0));

	if (m_pSystem->m_spDisplay)
	{
		int iMode = m_pSystem->m_spDisplay->GetMode();
		if (iMode >= 0)
		{
#ifndef PLATFORM_CONSOLE
			if (iMode == 0)
			{
				pGlobalOpt->SetValuei("VIDEO_DISPLAY_FULLSCREEN", 0);
				pGlobalOpt->SetValuei("VIDEO_DISPLAY_WIDTH", pEnv->GetValuei("VID_DWIDTH", 960));
				pGlobalOpt->SetValuei("VIDEO_DISPLAY_HEIGHT", pEnv->GetValuei("VID_DHEIGHT", 544));
				pGlobalOpt->SetValuei("VIDEO_DISPLAY_REFRESH", 85);
			}
			else
#endif
			{
				const CDC_VideoMode &Desc = m_pSystem->m_spDisplay->ModeList_GetDesc(iMode);
				pGlobalOpt->SetValuei("VIDEO_DISPLAY_FULLSCREEN", 1);
				pGlobalOpt->SetValuei("VIDEO_DISPLAY_WIDTH", Desc.m_Format.GetWidth());
				pGlobalOpt->SetValuei("VIDEO_DISPLAY_HEIGHT",Desc.m_Format.GetHeight());
				pGlobalOpt->SetValuei("VIDEO_DISPLAY_REFRESH", Desc.m_RefreshRate);
			}
		}
	}

	int nMaxTextureQuality = pEnv->GetValuei("MAXTEXTUREQUALITY", 3);
	fp32 TexQuality = MaxMT(MinMT(3 - pEnv->GetValuei("r_picmip0", 1), nMaxTextureQuality), 0) / 3.0f;
	pGlobalOpt->SetValuef("VIDEO_TEXQUALITY", TexQuality);

	pGlobalOpt->SetValuei("VIDEO_CHARSHADOWS", pEnv->GetValuei("XR_CHARSHADOWS", 1));

	pGlobalOpt->SetValuef("VIDEO_ANISOTROPY", pEnv->GetValuef("R_ANISOTROPY", 1));
	pGlobalOpt->SetValuei("VIDEO_SHADERMODE", pEnv->GetValuei("XR_SHADERMODE", 8));

	pGlobalOpt->SetValuei("VIDEO_DISPLAY_ANTIALIAS", pEnv->GetValuei("R_ANTIALIAS", 1));
	pGlobalOpt->SetValuei("VIDEO_DISPLAY_HDR", pEnv->GetValuei("R_BACKBUFFERFORMAT", IMAGE_FORMAT_RGB10A2_F));

	
}

void CXRealityApp::Con_UpdateOptions()
{
	m_bPendingOptionsUpdate = true;
}

void CXRealityApp::Con_SetDefaultBinds(int _iType)
{
	Options_SetDefaultBinds(_iType);
	Con_UpdateOptions();
}

void CXRealityApp::Con_ClearBindings()
{
	Options_ClearBinds();
	Con_UpdateOptions();
}




void CXRealityApp::CommitOptions()
{
	if (m_spGame && m_spGame->m_bBenchmark)
		return;
	/*
	// Enable all controls
	if (m_pSystem->m_spInput != NULL)
	{
	for(int i = 0; i < INPUT_MAXGAMEPADS; i++)
	m_pSystem->m_spInput->SetGamepadMapping(i, 0);
	}
	*/

	if (m_pSystem->IsRendering())
	{
		return;
	}

	m_bPendingOptionsUpdate = false;

	CRegistry *pOpt = m_pSystem->GetOptions();
	CRegistry *pGlobalOpt = m_pSystem->GetRegistry()->FindChild("OPTG");
	if (!pGlobalOpt)
	{
		pGlobalOpt = m_pSystem->GetRegistry()->CreateDir("OPTG");		
	}

/*	// Uncomment this to dump the current bindings
	#define DUMP_BINDING(name) MRTC_SystemInfo::OS_Trace("pOptions->SetValuei(\"%s\", %d);\n", name, pOpt->GetValuei(name))
		DUMP_BINDING("ACTION_MOVE_FORWARD2");
		DUMP_BINDING("ACTION_MOVE_BACKWARD2");
		DUMP_BINDING("ACTION_MOVE_LEFT2");
		DUMP_BINDING("ACTION_MOVE_RIGHT2");
		DUMP_BINDING("ACTION_LOOK_UP");
		DUMP_BINDING("ACTION_LOOK_DOWN");
		DUMP_BINDING("ACTION_LOOK_LEFT");
		DUMP_BINDING("ACTION_LOOK_RIGHT");
		DUMP_BINDING("ACTION_JUMP2");
		DUMP_BINDING("ACTION_CROUCH2");
		DUMP_BINDING("ACTION_PRIMARYFIRE2");
		DUMP_BINDING("ACTION_SECONDARYFIRE2");
		DUMP_BINDING("ACTION_DARKNESS2");
		DUMP_BINDING("ACTION_USE2");
		DUMP_BINDING("ACTION_ZOOM2");
		DUMP_BINDING("ACTION_CYCLEWEAPON2");
		DUMP_BINDING("ACTION_RELOAD2");
		DUMP_BINDING("ACTION_JOURNAL2");
		DUMP_BINDING("ACTION_DPAD_UP2");
		DUMP_BINDING("ACTION_DPAD_RIGHT2");
		DUMP_BINDING("ACTION_DPAD_DOWN2");
		DUMP_BINDING("ACTION_DPAD_LEFT2");
		DUMP_BINDING("ACTION_PAUSE2");
		DUMP_BINDING("ACTION_GUI_UP");
		DUMP_BINDING("ACTION_GUI_DOWN");
		DUMP_BINDING("ACTION_GUI_LEFT");
		DUMP_BINDING("ACTION_GUI_RIGHT");
		DUMP_BINDING("ACTION_GUI_OK");
		DUMP_BINDING("ACTION_GUI_CANCEL");//*/

	CRegistry *pEnv = m_pSystem->GetEnvironment();

	pEnv->SetValuef("VID_GAMMA", pGlobalOpt->GetValuef("VIDEO_GAMMA", 0.5f));
	pEnv->SetValuef("VID_BRIGHTNESS", pGlobalOpt->GetValuef("VIDEO_BRIGHTNESS", 0.5f));
	pEnv->SetValuef("VID_BLACKLEVEL", pGlobalOpt->GetValuef("VIDEO_BLACKLEVEL", 0.5f));
	pEnv->SetValuef("VID_GAMMARAMP", pGlobalOpt->GetValuef("VIDEO_GAMMARAMP", 0));
	pEnv->SetValuef("VID_PIXELASPECT", pGlobalOpt->GetValuef("VIDEO_DISPLAY_PIXELASPECT", 1.0f));
	int vsync = pGlobalOpt->GetValuei("VIDEO_DISPLAY_VSYNC", 1);
	pEnv->SetValuei("VID_VSYNC", vsync);

	if(pGlobalOpt->GetValuei("GAME_COMMENTARY", 0) == 0)
		pEnv->DeleteKey("GAME_COMMENTARY");
	else
		pEnv->SetValuei("GAME_COMMENTARY", 1);

	CDisplayContext* pDisplay = m_pSystem->m_spDisplay;

	if (pDisplay)
	{
#if defined(PLATFORM_XENON) || defined(PLATFORM_PS3)
		int VideoWidth = pGlobalOpt->GetValuei("VIDEO_DISPLAY_WIDTH", 1024);
		int VideoHeight = pGlobalOpt->GetValuei("VIDEO_DISPLAY_HEIGHT", 576);
#else
		int VideoWidth = pGlobalOpt->GetValuei("VIDEO_DISPLAY_WIDTH", 640);
		int VideoHeight = pGlobalOpt->GetValuei("VIDEO_DISPLAY_HEIGHT", 480);
#endif
#ifdef PLATFORM_CONSOLE
		if (1)
#else
		if(pGlobalOpt->GetValuei("VIDEO_DISPLAY_FULLSCREEN", 1))
#endif
		{
			pEnv->SetValue("VID_MODE", CStrF("%d %d 32 %d", 
				VideoWidth,
				VideoHeight,
				pGlobalOpt->GetValuei("VIDEO_DISPLAY_REFRESH", 85)
				));

			m_pSystem->DC_InitVidModeFromEnvironment();
		}
		else
		{
			// windowed is abit special
			pEnv->SetValue("VID_MODE", "desktop");

#ifndef PLATFORM_CONSOLE
			ConExecute(CStrF("vwinsize(%d,%d)", VideoWidth, VideoHeight));
#endif
			pDisplay->SetMode(0);

		}

		fp32 PixelAspect = pEnv->GetValuef("vid_pixelaspect", 1.0);
		//		if(!pDisplay->IsFullScreen())
		//			pDisplay->SetScreenAspect(1.0f);
		//		else
#ifndef PLATFORM_CONSOLE
		pDisplay->SetScreenAspect(((fp32)VideoWidth / (fp32)VideoHeight) / (4.0/3.0));
#endif
		pDisplay->SetPixelAspect(PixelAspect);
		pEnv->SetValuef("vid_pixelaspect", PixelAspect);

		{
			fp32 TexQuality = pGlobalOpt->GetValuef("VIDEO_TEXQUALITY", s_DefaultTextureQuality);

			uint nMaxTextureQuality = pEnv->GetValuei("MAXTEXTUREQUALITY", 3);
			TexQuality = MinMT(TexQuality, nMaxTextureQuality / 3.0);

			int PicMip = RoundToInt((1.0 - TexQuality) * 3.0);
			TexQuality = 1.0 - fp32(MaxMT(MinMT(PicMip, 3), 0)) / 3.0f;

			pGlobalOpt->SetValuef("VIDEO_TEXQUALITY", TexQuality);

			ConExecute(CStrF("r_picmip(0, %d)", PicMip));
			ConExecute(CStrF("r_picmip(1, %d)", PicMip));
			ConExecute(CStrF("r_picmip(2, %d)", PicMip + s_EXTRA_PICMIP_WEAPONS));
			ConExecute(CStrF("r_picmip(3, %d)", PicMip + s_EXTRA_PICMIP_WORLDOBJECTS));
			ConExecute(CStrF("r_picmip(4, %d)", PicMip));
			ConExecute(CStrF("r_picmip(5, %d)", PicMip));
			ConExecute(CStrF("r_picmip(6, %d)", PicMip));
			ConExecute(CStrF("r_picmip(7, %d)", PicMip));
			ConExecute(CStrF("r_picmip(8, %d)", PicMip + s_EXTRA_PICMIP_PROJMAPS));
			ConExecute(CStrF("r_picmip(9, %d)", PicMip));
			ConExecute(CStrF("r_picmip(10, %d)", PicMip));
			ConExecute(CStrF("r_picmip(11, %d)", PicMip));
			ConExecute(CStrF("r_picmip(12, %d)", PicMip));
			ConExecute(CStrF("r_picmip(13, %d)", PicMip));
			ConExecute(CStrF("r_picmip(14, %d)", PicMip));
			ConExecute(CStrF("r_picmip(15, %d)", PicMip));
			ConExecute(CStrF("r_anisotropy(%f)", pGlobalOpt->GetValuef("VIDEO_ANISOTROPY", 1)));
			ConExecute(CStrF("r_vsync(%d)", vsync));

			ConExecute(CStrF("r_antialias(%d)", pGlobalOpt->GetValuei("VIDEO_DISPLAY_ANTIALIAS", 4)));
			ConExecute(CStrF("r_backbufferformat(%d)", pGlobalOpt->GetValuei("VIDEO_DISPLAY_HDR", IMAGE_FORMAT_RGB10A2_F)));

		}
	}

	CommitGamma();

	// Set volumes
	if (m_spGame != NULL)
	{
		/*
		fp32 Balance = m_pSystem->GetOptions()->GetValuef("SND_SFXMSXBALANCE", 0.5f, 1);
		fp32 VolumeSfx = 1;
		fp32 VolumeMusic = 1;

		if(Balance > 0.5f)
			VolumeSfx = 1.0f-(Balance-0.5f)*2.0f;
		else if(Balance < 0.5f)
			VolumeMusic = 1.0f-(0.5f-Balance)*2.0f;
			*/
		fp32 VolumeSfx = pOpt->GetValuef("SND_VOLUMESFX", 1.0f, 1);
		fp32 VolumeMusic = pOpt->GetValuef("SND_VOLUMEMUSIC", 1.0f, 1);

		m_spGame->SetVolume(VolumeSfx, VolumeSfx, VolumeSfx, VolumeSfx);
		if (m_spSoundContext != NULL)
		{
			m_spSoundContext->Music_SetSettingsVolume(VolumeMusic);
//			m_spSoundContext->Music_SetGameVolume(VolumeMusic);
		}
	}

	if (m_spSoundContext)
	{
#ifndef PLATFORM_XENON
		if(pGlobalOpt->GetValuei("SOUND_USE3D", 1))
			ConExecute("snd_use3d(1)");
		else
			ConExecute("snd_use3d(0)");

		if(pGlobalOpt->GetValuei("SOUND_USEEAX", 0))
			ConExecute("snd_useeax(1)");
		else
			ConExecute("snd_useeax(0)");
#endif
	}

#ifndef PLATFORM_XENON
	if(pGlobalOpt->GetValuei("GAMEPAD_ENABLE", 1))
		ConExecute("in_gamepad(1)");
	else
		ConExecute("in_gamepad(0)");
#endif
	

	// Parental Guidance
	int bPGGore = 0;
#ifdef PLATFORM_XBOX1
	DWORD parentallvl = XGetParentalControlSetting();
	if(parentallvl == XC_PC_ESRB_ALL || parentallvl == XC_PC_ESRB_ADULT)
		bPGGore = pOpt->GetValuei("GAME_BLOOD", 1, 1);
	else
		pOpt->SetValuei("GAME_BLOOD", 0);
#else
	bPGGore = pOpt->GetValuei("GAME_BLOOD", 1, 1);
#endif
	
#ifdef MRTC_NOVIOLENCE
	bPGGore = 0;
#endif
	
	int SurfOptions = (bPGGore) ? XW_SURFOPTION_HQ0 : XW_SURFOPTION_HQ0 | XW_SURFOPTION_NOGORE; // NOTE: XW_SURFOPTION_NOGORE == XW_SURFOPTION_SIMPLE3
	pEnv->SetValuei("XR_SURFOPTIONS", SurfOptions);

	int ModelQuality = pGlobalOpt->GetValuei("VIDEO_MODELQUALITY", 0);
	if (ModelQuality)
		pEnv->SetValuef("XR_LODOFFSET", -1000000.0);
	else
		pEnv->SetValuef("XR_LODOFFSET", 0.0);

	pGlobalOpt->DeleteDir("VIDEO_MODELQUALITYLIST");
	CRegistry *pQualityList = pGlobalOpt->CreateDir("VIDEO_MODELQUALITYLIST");
	for(int32 i = 0; i < 2; i++)
	{
		pQualityList->AddKeyi(CStrF("LIST%d", i), i);
	}

	if (ModelQuality)
		pGlobalOpt->SetValue("VIDEO_MODELQUALITYSELECTED", Localize_Str("§LMENU_HIGH"));
	else
		pGlobalOpt->SetValue("VIDEO_MODELQUALITYSELECTED", Localize_Str("§LMENU_DYNAMIC"));
	
	// Set surface options
	if(m_spGame && m_spGame->GetCurrentClient() &&m_spGame->GetCurrentClient()->Render_GetEngine())
	{
		m_spGame->GetCurrentClient()->Render_GetEngine()->m_SurfOptions = SurfOptions;

		if (ModelQuality)
			ConExecute("xr_lodoffset(-1000000)");
		else
			ConExecute("xr_lodoffset(0)");
	}

	// Clear all bindings
	m_pSystem->m_spCon->Parser_ClearBindList();

	if(CDiskUtil::FileExists(m_pSystem->m_ExePath + "Config.mpp"))
		m_pSystem->m_spCon->Parser_ExecuteScriptFile("Config.mpp");

/*#ifdef PLATFORM_XBOX
	Options_SetDefaultBinds();
#endif*/

	Options_UpdateBinds();

	pEnv->SetValuei("XR_CHARSHADOWS", pGlobalOpt->GetValuei("VIDEO_CHARSHADOWS", 1));
	pEnv->SetValuei("XR_SHADERMODE", pGlobalOpt->GetValuei("VIDEO_SHADERMODE", 0));

	// make controller settings update
	if(m_spGame != NULL)
	{
		for(int32 i = 0; i < m_spGame->m_lspWClients.Len(); i++)
			m_spGame->m_lspWClients[i]->OnRegistryChange();
	}

	m_pSystem->WriteEnvironment();
					
//	Con_SaveOptions();
}

void CXRealityApp::CommitGamma()
{
	if (m_pSystem->IsRendering())
	{
		return;
	}

	CRegistry *pEnv = m_pSystem->GetEnvironment();

	// Brightness
	fp32 Gamma = 1.0 + ((0.5f - pEnv->GetValuef("VID_GAMMA", 0.5f))) * 0.75f;
	fp32 BlackLevelOpt = pEnv->GetValuef("VID_BLACKLEVEL", 0.5f) - 0.5f;
	int32 GammaRamp = pEnv->GetValuei("VID_GAMMARAMP", 0);
	fp32 BlackLevel = (BlackLevelOpt > 0.0f) ?
					BlackLevelOpt * 0.5f :
					BlackLevelOpt * 0.1f;

	CRenderContext* pRender = NULL;
	CDisplayContext* pDisplay = m_pSystem->m_spDisplay;

	CRCLock RCLock;
	if (pDisplay && (pRender = pDisplay->GetRenderContext(&RCLock)))
	{
		pRender->Attrib_GlobalSetVarfv(CRC_GLOBALVAR_GAMMA, &Gamma);
		pRender->Attrib_GlobalSetVarfv(CRC_GLOBALVAR_GAMMARAMPADD, CVec4Dfp32(BlackLevel).k);
		pRender->Attrib_GlobalSetVar(CRC_GLOBALVAR_GAMMARAMP, GammaRamp);
	}
}

#ifdef M_Profile
void CXRealityApp::Con_DumpCallList()
{
	bDumpCallList = true;
}
#endif

void CXRealityApp::Con_ResetTimer()
{
#ifdef M_DEMO_XBOX
	XBoxDemo_ResetTimer();
#endif
}

void CXRealityApp::Con_PauseTimer()
{
#ifdef M_DEMO_XBOX
	XBoxDemo_PauseTimer(1);
#endif
}

void CXRealityApp::Con_PauseTimer5()
{
#ifdef M_DEMO_XBOX
	XBoxDemo_PauseTimer(5);
#endif
}

void CXRealityApp::Con_CanQuit(int32 _i)
{
#ifdef M_DEMO_XBOX
	m_CanQuit = _i != 0;
#endif
}


void CXRealityApp::Con_Crash()
{
	M_BREAKPOINT;
}

void CXRealityApp::Con_CrashCException()
{
	Error("CXRealityApp::Con_CrashCException", "Test crash messag.");
}


void CXRealityApp::Con_ShowFPSToggle()
{
	m_bShowFPS = !m_bShowFPS;
}

void CXRealityApp::Con_ShowFPSLiteToggle()
{
	m_bShowFPSLite = !m_bShowFPSLite;
	ConExecute(CStrF("xr_ppexposuredebug(%d)", m_bShowFPSLite));
}

void CXRealityApp::Con_ShowStatsToggle()
{
	m_bShowStats = !m_bShowStats;
}

void CXRealityApp::Con_ShowSoundToggle()
{
	m_bShowSoundDbg = !m_bShowSoundDbg;
}

void CXRealityApp::Con_ShowVBEToggle()
{
	m_bShowVBE = !m_bShowVBE;
}

void CXRealityApp::Con_ToggleShowMemInfo()
{
	m_bShowMemInfo = !m_bShowMemInfo;
}

void CXRealityApp::Con_In_LogKeys(int _Log)
{
	m_bLogKeys = _Log;
}

void CXRealityApp::Con_In_DebugKeys(int _Log)
{
	m_bDebugKeys = _Log;
}


void CXRealityApp::Con_ToggleAutomaticViewscale()
{
	m_bAutomaticScreenResize = !m_bAutomaticScreenResize;
	if (m_spGame && !m_bAutomaticScreenResize)
		m_spGame->Con_ViewScale(1.0f, 1.0f);

}

void CXRealityApp::Con_SetAutomaticViewscale(int _bOn)
{
	if (m_bAutomaticScreenResizeSticky)
	{
		m_bAutomaticScreenResize = _bOn;
		if(m_spGame && !m_bAutomaticScreenResize)
			m_spGame->Con_ViewScale(1.0f, 1.0f);
	}
}


#ifdef PLATFORM_XBOX1
void CXRealityApp::Con_LaunchDashboard(CStr _What)
{
	LD_LAUNCH_DASHBOARD Info;
	memset(&Info, 0, sizeof(LD_LAUNCH_DASHBOARD));
	Info.dwReason = 0xFFFF;
	if(_What == "networkconf")
		Info.dwReason = XLD_LAUNCH_DASHBOARD_NETWORK_CONFIGURATION;
	if(_What == "manageaccount")
		Info.dwReason = XLD_LAUNCH_DASHBOARD_ACCOUNT_MANAGEMENT;
	if(_What == "onlinemenu")
		Info.dwReason = XLD_LAUNCH_DASHBOARD_ONLINE_MENU;
	if(_What == "newaccount")
		Info.dwReason = XLD_LAUNCH_DASHBOARD_NEW_ACCOUNT_SIGNUP;
	if(_What == "memory")
	{
		Info.dwReason = XLD_LAUNCH_DASHBOARD_MEMORY;
		Info.dwParameter1 = 'U';
		Info.dwParameter2 = 100;
	}

	if(Info.dwReason != 0xFFFF)
		XLaunchNewImage(NULL, (LAUNCH_DATA*)&Info);
}

void CXRealityApp::Con_OnlineTitleUpdate()
{
	#if !defined(M_DEMO_XBOX)
		XOnlineTitleUpdate(0);
	#endif
}

#endif

void CXRealityApp::Con_SwitchControllerMethod()
{
	int iConfig = m_pSystem->GetOptions()->GetValuei("CONTROLLER_TYPE", 0, 1);
	iConfig++;
	iConfig%=4;
	m_pSystem->GetOptions()->SetValuei("CONTROLLER_TYPE", iConfig);
}

void CXRealityApp::Con_Vid_Gamma(fp32 _Value)
{
	MACRO_GetSystem;
	pSys->GetRegistry()->SetValuef("OPTG\\VIDEO_GAMMA", _Value);
	Con_UpdateOptions();
}

void CXRealityApp::Con_Vid_Brightness(fp32 _Value)
{
	MACRO_GetSystem;
	pSys->GetRegistry()->SetValuef("OPTG\\VIDEO_BRIGHTNESS", _Value);
	Con_UpdateOptions();
}

void CXRealityApp::Con_Vid_BlackLevel(fp32 _Value)
{
	MACRO_GetSystem;
	pSys->GetRegistry()->SetValuef("OPTG\\VIDEO_BLACKLEVEL", _Value);
	Con_UpdateOptions();
}

void CXRealityApp::Con_Vid_GammaRamp(int32 _Value)
{
	MACRO_GetSystem;
	pSys->GetRegistry()->SetValuef("OPTG\\VIDEO_GAMMARAMP", _Value);
	Con_UpdateOptions();
}

void CXRealityApp::Con_Vid_PixelAspect(fp32 _Value)
{
	MACRO_GetSystem;
	pSys->GetRegistry()->SetValuef("OPTG\\VIDEO_DISPLAY_PIXELASPECT", _Value);
	Con_UpdateOptions();
}

void CXRealityApp::Con_Launch(CStr _Value)
{
#ifdef PLATFORM_WIN_PC
	MACRO_GetSystem;
	M_TRACE("Launching(%s)\n", _Value.Str());
	ShellExecute(NULL, NULL, _Value, pSys->m_ExePath, NULL, SW_SHOWNORMAL);
#endif
}

void CXRealityApp::Con_DrawMode(int _v)
{
	MAUTOSTRIP(CXRealityApp_Con_DrawMode, MAUTOSTRIP_VOID);
	m_DrawMode = _v;
}

void Dummy0(int)
{
}

void Dummy1(const ch8*, const ch8*, int)
{
}

void CXRealityApp::Register(CScriptRegisterContext & _RegContext)
{
	CApplication::Register(_RegContext);

	CStr CmdLine = m_pSystem->m_CommandLine.UpperCase();
	bool bSound = m_pSystem->GetEnvironment()->GetValuei("SND_ENABLE", 1) != 0;
	if (CmdLine.Find("-NOSOUND") >= 0)
		bSound = false;

	if (!bSound)
	{
		_RegContext.RegFunction("stream_stop", &Dummy0);
		_RegContext.RegFunction("stream_play", &Dummy1);
	}

	_RegContext.RegFunction("launch", this, &CXRealityApp::Con_Launch);

	_RegContext.RegFunction("showFPSToggle", this, &CXRealityApp::Con_ShowFPSToggle);
	_RegContext.RegFunction("showFPSLiteToggle", this, &CXRealityApp::Con_ShowFPSLiteToggle);
	_RegContext.RegFunction("showStatsToggle", this, &CXRealityApp::Con_ShowStatsToggle);
	_RegContext.RegFunction("showSoundToggle", this, &CXRealityApp::Con_ShowSoundToggle);
	_RegContext.RegFunction("showvbetoggle", this, &CXRealityApp::Con_ShowVBEToggle);
	_RegContext.RegFunction("toggleShowMemInfo", this, &CXRealityApp::Con_ToggleShowMemInfo);

	_RegContext.RegFunction("toggleASR", this, &CXRealityApp::Con_ToggleAutomaticViewscale);
	_RegContext.RegFunction("switchcontrollermethod", this, &CXRealityApp::Con_SwitchControllerMethod);
	
	_RegContext.RegFunction("setASR", this, &CXRealityApp::Con_SetAutomaticViewscale);

	_RegContext.RegFunction("vid_gamma", this, &CXRealityApp::Con_Vid_Gamma);
	_RegContext.RegFunction("vid_brightness", this, &CXRealityApp::Con_Vid_Brightness);
	_RegContext.RegFunction("vid_blacklevel", this, &CXRealityApp::Con_Vid_BlackLevel);
	_RegContext.RegFunction("vid_gammaramp", this, &CXRealityApp::Con_Vid_GammaRamp);


	_RegContext.RegFunction("vid_pixelaspect", this, &CXRealityApp::Con_Vid_PixelAspect);

	_RegContext.RegFunction("option_setdefault", this, &CXRealityApp::Con_SetDefaultOptions);
	_RegContext.RegFunction("option_setdefaultglobal", this, &CXRealityApp::Con_SetDefaultOptionsGlobal);
	_RegContext.RegFunction("option_setdefaultbinds", this, &CXRealityApp::Con_SetDefaultBinds);
	_RegContext.RegFunction("option_clearbinds", this, &CXRealityApp::Con_ClearBindings);
	

	_RegContext.RegFunction("option_getfromenv", this, &CXRealityApp::Con_OptionsGetFromEnv);

	_RegContext.RegFunction("in_logkeys", this, &CXRealityApp::Con_In_LogKeys);
	_RegContext.RegFunction("in_debugkeys", this, &CXRealityApp::Con_In_DebugKeys);
	
	
	_RegContext.RegFunction("option_update", this, &CXRealityApp::Con_UpdateOptions);
	_RegContext.RegFunction("option_save", this, &CXRealityApp::Con_SaveOptions);

#ifdef PLATFORM_XBOX1
	_RegContext.RegFunction("launchdashboard", this, &CXRealityApp::Con_LaunchDashboard);
	_RegContext.RegFunction("onlinetitleupdate", this, &CXRealityApp::Con_OnlineTitleUpdate);
#endif


#ifdef M_Profile
	_RegContext.RegFunction("callgraph", this, &CXRealityApp::Con_DumpCallList);
	_RegContext.RegFunction("startprofiler", this, &CXRealityApp::Con_StartProfiler);
	_RegContext.RegFunction("checkfps", this, &CXRealityApp::Con_CheckFPS);
#endif
	
#ifndef M_RTM
	_RegContext.RegFunction("sys_dumpbadfps", this, &CXRealityApp::Con_DumpBadFPS);
#endif
	_RegContext.RegFunction("resettimer", this, &CXRealityApp::Con_ResetTimer);
	_RegContext.RegFunction("pausetimer", this, &CXRealityApp::Con_PauseTimer);
	_RegContext.RegFunction("pausetimer5", this, &CXRealityApp::Con_PauseTimer5);
	_RegContext.RegFunction("canquit", this, &CXRealityApp::Con_CanQuit);

	_RegContext.RegFunction("crash", this, &CXRealityApp::Con_Crash);
	_RegContext.RegFunction("crashCCException", this, &CXRealityApp::Con_CrashCException);	

	_RegContext.RegFunction("xr_drawmode", this, &CXRealityApp::Con_DrawMode);

	_RegContext.RegFunction("multithread", this, &CXRealityApp::Con_EnableMultithread);
	_RegContext.RegFunction("threadpool", this, &CXRealityApp::Con_EnableThreadPool);

#ifdef LOG_FRAME_INFO
	_RegContext.RegFunction("logframetimes", this, &CXRealityApp::Con_LogFrameTimes);
#endif

#ifndef PLATFORM_CONSOLE
	_RegContext.RegFunction("videofilebegin", this, &CXRealityApp::Con_VideoFileBegin);
	_RegContext.RegFunction("videofileend", this, &CXRealityApp::Con_VideoFileEnd);
#endif

	_RegContext.RegFunction("sys_tracerecordsystem", this, &CXRealityApp::Con_TraceRecordSystem);
	_RegContext.RegFunction("sys_tracerecordrender", this, &CXRealityApp::Con_TraceRecordRender);
	_RegContext.RegFunction("sys_tracerecordgamerefresh", this, &CXRealityApp::Con_TraceRecordGameRefresh);

	_RegContext.RegFunction("sys_perfcountsystem", this, &CXRealityApp::Con_PerfCountSystem);
	_RegContext.RegFunction("sys_perfcountrender", this, &CXRealityApp::Con_PerfCountRender);
}


#ifdef PLATFORM_XBOX
	#include <xtl.h>
#elif defined PLATFORM_WIN
	#include <windows.h>
#endif

MACRO_MAIN("CXRealityApp")

#ifdef PLATFORM_XBOX
enum
{
	XBOX_CONTROLS_A = SKEY_JOY_BUTTON00,
	XBOX_CONTROLS_B,
	XBOX_CONTROLS_X,
	XBOX_CONTROLS_Y,
	XBOX_CONTROLS_BLACK,
	XBOX_CONTROLS_WHITE,
	XBOX_CONTROLS_LTRIGGER,
	XBOX_CONTROLS_RTRIGGER,
	XBOX_CONTROLS_START,
	XBOX_CONTROLS_BACK,
	XBOX_CONTROLS_LTHUMBSTICKBUT,
	XBOX_CONTROLS_RTHUMBSTICKBUT,
	XBOX_CONTROLS_LTHUMBSTICK,
	XBOX_CONTROLS_RTHUMBSTICK,
	XBOX_CONTROLS_DPAD = SKEY_JOY_POV0F,
};
#endif

int CXRealityApp::Options_GetScanKey(const CStr &_Action, CRegistry *_pOptions, CRegistry *_pDynamicStringTable, int _iType)
{
	int iScanKey = _pOptions->GetValuei("ACTION_" + _Action);
	if(_pDynamicStringTable)
	{
		if(iScanKey == 0)
		{
			_pDynamicStringTable->SetValue("CONTROLLER_" + _Action + "_A", "§LCONTROLLER_UNDEFINED_A");
			_pDynamicStringTable->SetValue("CONTROLLER_" + _Action + "_B", "§LCONTROLLER_UNDEFINED_B");
			_pDynamicStringTable->SetValue("CONTROLLER_" + _Action + "_C", "§LCONTROLLER_UNDEFINED_C");
		}
		else
		{
			CStr Prefix;
#ifdef PLATFORM_XBOX
			if(iScanKey <= XBOX_CONTROLS_Y)
				Prefix = "§LCONTROLLER_XBOX_BUTTON_1";
			else if(iScanKey <= XBOX_CONTROLS_WHITE)
				Prefix = "§LCONTROLLER_XBOX_BUTTON_2";
			else if(iScanKey <= XBOX_CONTROLS_RTRIGGER)
				Prefix = "§LCONTROLLER_XBOX_TRIGGER";
			else if(iScanKey <= XBOX_CONTROLS_BACK)
				Prefix = "§LCONTROLLER_XBOX_BUTTON_3";
			else if(iScanKey <= XBOX_CONTROLS_RTHUMBSTICKBUT)
				Prefix = "§LCONTROLLER_XBOX_THUMBSTICK_CLICK";
			else if(iScanKey <= XBOX_CONTROLS_RTHUMBSTICK)
			{
				Prefix = "§LCONTROLLER_XBOX_THUMBSTICK";
			}
			else if(iScanKey <= XBOX_CONTROLS_DPAD)
			{
				Prefix = "§LCONTROLLER_XBOX_DPAD";
			}
			else
#endif
			{
				if(_iType == 0)
					Prefix = "§LCONTROLLER_PC_BUTTON";
				else if(_iType == 1)
					Prefix = "§LCONTROLLER_PC_AXIS";
			}

			CStr KeyName;
#ifdef PLATFORM_XBOX
			if(iScanKey >= XBOX_CONTROLS_A && iScanKey <= XBOX_CONTROLS_DPAD)
				KeyName = CStrF("§LCONTROLLER_XBOX_%.2x", iScanKey - XBOX_CONTROLS_A);
			else
				KeyName = CScanKey::GetLocalizedKeyName(iScanKey);
#else
			KeyName = CScanKey::GetLocalizedKeyName(iScanKey);
#endif
			// Move this to CScanKey::GetLocalizedKeyName
			if(KeyName.CompareSubStr("JOY_") == 0)
			{
				CStr St, St2;
				while(true)
				{
					St2 = KeyName.GetStrSep("_");
					if(KeyName == "")
						break;
					if(St != "")
						St += "_" + St2;
					else
						St += St2;
				}
				// If the zeros in the beginning of the string is not removed,
				// there is a risk that this will be interpretated as an octal value.
				while(St2[0] == '0')
					St2 = St2.Copy(1, 1024);
				KeyName = "§L" + St + CStrF(" %i", ("0x" + St2).Val_int());
			}
			else if(KeyName.CompareSubStr("MOUSE") == 0)
				KeyName = "§LMOUSE_BUTTON " + KeyName.Copy(5, 1024);

			CStr Key = Prefix + "_A§p0" + KeyName + "§pq";
			_pDynamicStringTable->SetValue("CONTROLLER_" + _Action + "_A", Key);
			_pDynamicStringTable->SetValue("CONTROLLER_" + _Action + "_B", Prefix + "_B§p0" + KeyName + "§pq");
			_pDynamicStringTable->SetValue("CONTROLLER_" + _Action + "_C", Prefix + "_C§p0" + KeyName + "§pq");
		}
	}
	return iScanKey;
}


static CFStr GetInputEnumStr(int _ScanCode) //debug
{
#define ENTRY(x) case x: return CFStrF("%s (%d)", #x, x);
	switch (_ScanCode)
	{
	ENTRY(SKEY_JOY_AXIS00_POS)
	ENTRY(SKEY_JOY_AXIS00_NEG)
	ENTRY(SKEY_JOY_AXIS01_POS)
	ENTRY(SKEY_JOY_AXIS01_NEG)
	ENTRY(SKEY_JOY_AXIS02_POS)
	ENTRY(SKEY_JOY_AXIS02_NEG)
	ENTRY(SKEY_JOY_AXIS03_POS)
	ENTRY(SKEY_JOY_AXIS03_NEG)
	ENTRY(SKEY_JOY_AXIS04_POS)
	ENTRY(SKEY_JOY_AXIS04_NEG)
	ENTRY(SKEY_JOY_AXIS05_POS)
	ENTRY(SKEY_JOY_AXIS05_NEG)
	ENTRY(SKEY_JOY_AXIS06_POS)
	ENTRY(SKEY_JOY_AXIS06_NEG)
	ENTRY(SKEY_JOY_AXIS07_POS)
	ENTRY(SKEY_JOY_AXIS07_NEG)
	ENTRY(SKEY_JOY_AXIS08_POS)
	ENTRY(SKEY_JOY_AXIS08_NEG)
	ENTRY(SKEY_JOY_AXIS09_POS)
	ENTRY(SKEY_JOY_AXIS09_NEG)
	ENTRY(SKEY_JOY_AXIS0A_POS)
	ENTRY(SKEY_JOY_AXIS0A_NEG)
	ENTRY(SKEY_JOY_AXIS0B_POS)
	ENTRY(SKEY_JOY_AXIS0B_NEG)
	ENTRY(SKEY_JOY_AXIS0C_POS)
	ENTRY(SKEY_JOY_AXIS0C_NEG)
	ENTRY(SKEY_JOY_AXIS0D_POS)
	ENTRY(SKEY_JOY_AXIS0D_NEG)
	ENTRY(SKEY_JOY_AXIS0E_POS)
	ENTRY(SKEY_JOY_AXIS0E_NEG)
	ENTRY(SKEY_JOY_AXIS0F_POS)
	ENTRY(SKEY_JOY_AXIS0F_NEG)
	ENTRY(SKEY_JOY_BUTTON00)
	ENTRY(SKEY_JOY_BUTTON01)
	ENTRY(SKEY_JOY_BUTTON02)
	ENTRY(SKEY_JOY_BUTTON03)
	ENTRY(SKEY_JOY_BUTTON04)
	ENTRY(SKEY_JOY_BUTTON05)
	ENTRY(SKEY_JOY_BUTTON06)
	ENTRY(SKEY_JOY_BUTTON07)
	ENTRY(SKEY_JOY_BUTTON08)
	ENTRY(SKEY_JOY_BUTTON09)
	ENTRY(SKEY_JOY_BUTTON0A)
	ENTRY(SKEY_JOY_BUTTON0B)
	ENTRY(SKEY_JOY_BUTTON0C)
	ENTRY(SKEY_JOY_BUTTON0D)
	ENTRY(SKEY_JOY_BUTTON0E)
	ENTRY(SKEY_JOY_BUTTON0F)
	ENTRY(SKEY_JOY_POV00)
	ENTRY(SKEY_JOY_POV01)
	ENTRY(SKEY_JOY_POV02)
	ENTRY(SKEY_JOY_POV03)
	ENTRY(SKEY_JOY_POV04)
	ENTRY(SKEY_JOY_POV05)
	ENTRY(SKEY_JOY_POV06)
	ENTRY(SKEY_JOY_POV07)
	ENTRY(SKEY_JOY_POV08)
	ENTRY(SKEY_JOY_POV09)
	ENTRY(SKEY_JOY_POV0A)
	ENTRY(SKEY_JOY_POV0B)
	ENTRY(SKEY_JOY_POV0C)
	ENTRY(SKEY_JOY_POV0D)
	ENTRY(SKEY_JOY_POV0E)
	ENTRY(SKEY_JOY_POV0F)
	}
#undef ENTRY
	return "??";
}

static void DumpBinds(CRegistry* _pOptions)
{
	uint nCh = _pOptions->GetNumChildren();
	for (uint i = 0; i < nCh; i++)
	{
		CStr Name = _pOptions->GetName(i);
		if (Name.CompareSubStr("ACTION_") == 0)
		{
			int ScanCode = _pOptions->GetValuei(i);
			CFStr x = GetInputEnumStr(ScanCode);
			M_TRACEALWAYS("[DumpBinds] \"%s\", %s\n", Name.Str(), x.Str());
		}
	}
}


void CXRealityApp::Options_SetDefaultBinds(int _iType)
{
	CRegistry *pOptions = m_pSystem->GetOptions();
	/*
#ifdef PLATFORM_XBOX
	// These should be converted to gamepad binds
	// probably needs some work in the input system
	pOptions->SetValuei("ACTION_JUMP", XBOX_CONTROLS_A);
	pOptions->SetValuei("ACTION_USE", XBOX_CONTROLS_X);
	pOptions->SetValuei("ACTION_EYESHINE", XBOX_CONTROLS_RTHUMBSTICKBUT);
	pOptions->SetValuei("ACTION_MOVEAXIS", XBOX_CONTROLS_RTHUMBSTICK);
	pOptions->SetValuei("ACTION_LOOKAXIS", XBOX_CONTROLS_LTHUMBSTICK);
	pOptions->SetValuei("ACTION_CROUCHTOGGLE", XBOX_CONTROLS_LTHUMBSTICKBUT);

	pOptions->SetValuei("ACTION_PRIMARYFIRE", XBOX_CONTROLS_RTRIGGER);
	pOptions->SetValuei("ACTION_SECONDARYFIRE", XBOX_CONTROLS_LTRIGGER);
	pOptions->SetValuei("ACTION_FLASHLIGHT", XBOX_CONTROLS_WHITE);
	pOptions->SetValuei("ACTION_RELOAD", XBOX_CONTROLS_B);
	pOptions->SetValuei("ACTION_CYCLEWEAPON", XBOX_CONTROLS_Y);
	pOptions->SetValuei("ACTION_ZOOM", XBOX_CONTROLS_BLACK);
	pOptions->SetValuei("ACTION_LEAN", XBOX_CONTROLS_DPAD);

	pOptions->SetValuei("ACTION_JOURNAL", XBOX_CONTROLS_BACK);
	pOptions->SetValuei("ACTION_PAUSE", XBOX_CONTROLS_START);

	pOptions->SetValuei("ACTION_HELP", XBOX_CONTROLS_B);
	pOptions->SetValuei("ACTION_GUI_OK", XBOX_CONTROLS_A);
#else*/
	if(_iType != 2)
	{
		// Keyboard binds
		pOptions->SetValuei("ACTION_MOVE_FORWARD", SKEY_W);
		pOptions->SetValuei("ACTION_MOVE_BACKWARD", SKEY_S);
		pOptions->SetValuei("ACTION_MOVE_RIGHT", SKEY_D);
		pOptions->SetValuei("ACTION_MOVE_LEFT", SKEY_A);

		pOptions->SetValuei("ACTION_JUMP", SKEY_SPACE);
		pOptions->SetValuei("ACTION_CROUCH", SKEY_C);
		pOptions->SetValuei("ACTION_WALK", SKEY_LEFT_SHIFT);

		pOptions->SetValuei("ACTION_PRIMARYFIRE", SKEY_MOUSE1);
		pOptions->SetValuei("ACTION_DARKNESS", SKEY_MOUSE2);
		pOptions->SetValuei("ACTION_SECONDARYFIRE", SKEY_F);
		pOptions->SetValuei("ACTION_USE", SKEY_E);
		pOptions->SetValuei("ACTION_CYCLEWEAPON", SKEY_V);
		pOptions->SetValuei("ACTION_RELOAD", SKEY_R);
		pOptions->SetValuei("ACTION_DARKNESSDRAIN", SKEY_Z);
		pOptions->SetValuei("ACTION_JOURNAL", SKEY_J);

		pOptions->SetValuei("ACTION_DPAD_UP", SKEY_1);
		pOptions->SetValuei("ACTION_DPAD_RIGHT", SKEY_2);
		pOptions->SetValuei("ACTION_DPAD_DOWN", SKEY_3);
		pOptions->SetValuei("ACTION_DPAD_LEFT", SKEY_4);

		pOptions->SetValuei("ACTION_QUICKSAVE", SKEY_F5);
		pOptions->SetValuei("ACTION_QUICKLOAD", SKEY_F9);
		pOptions->SetValuei("ACTION_SCREENSHOT", SKEY_F12);

		pOptions->SetValuei("ACTION_WEAPONZOOM", SKEY_H);

		pOptions->SetValuei("ACTION_TOGGLEDARKLINGHUMAN", SKEY_Q);
	}
	if(_iType != 1)
	{
#if defined(PLATFORM_XENON) || defined(PLATFORM_PS3)
		switch(_iType)
		{
		case 3:
			pOptions->SetValuei("ACTION_MOVE_FORWARD2",			SKEY_JOY_AXIS01_POS);
			pOptions->SetValuei("ACTION_MOVE_BACKWARD2",		SKEY_JOY_AXIS01_NEG);
			pOptions->SetValuei("ACTION_MOVE_LEFT2",			SKEY_JOY_AXIS00_NEG);
			pOptions->SetValuei("ACTION_MOVE_RIGHT2",			SKEY_JOY_AXIS00_POS);
			pOptions->SetValuei("ACTION_LOOK_UP",				SKEY_JOY_AXIS03_POS);
			pOptions->SetValuei("ACTION_LOOK_DOWN",				SKEY_JOY_AXIS03_NEG);
			pOptions->SetValuei("ACTION_LOOK_LEFT",				SKEY_JOY_AXIS02_NEG);
			pOptions->SetValuei("ACTION_LOOK_RIGHT",			SKEY_JOY_AXIS02_POS);

			pOptions->SetValuei("ACTION_JUMP2",					SKEY_JOY_BUTTON00); // "A"
			pOptions->SetValuei("ACTION_USE2",					SKEY_JOY_BUTTON02); // "X"
			pOptions->SetValuei("ACTION_CYCLEWEAPON2",			SKEY_JOY_BUTTON03); // "Y"
			pOptions->SetValuei("ACTION_RELOAD2",				SKEY_JOY_BUTTON01); // "B"

			pOptions->SetValuei("ACTION_SECONDARYFIRE2",		SKEY_JOY_BUTTON0A); // left trigger
			pOptions->SetValuei("ACTION_PRIMARYFIRE2",			SKEY_JOY_BUTTON0B); // right trigger
			pOptions->SetValuei("ACTION_DARKNESSDRAIN2",		SKEY_JOY_BUTTON08); // left shoulder button
			//		pOptions->SetValuei("ACTION_TOGGLEDARKLINGHUMAN",	SKEY_JOY_BUTTON08); //		(multiplayer only)
			pOptions->SetValuei("ACTION_DARKNESS2",				SKEY_JOY_BUTTON09); // right shoulder button
			pOptions->SetValuei("ACTION_JOURNAL2",				SKEY_JOY_BUTTON05);	// "back" button

			pOptions->SetValuei("ACTION_CROUCH2",				SKEY_JOY_BUTTON06); // left stick button
			//pOptions->SetValuei("ACTION_SCREENSHOT",			SKEY_JOY_BUTTON01); // right stick button
			pOptions->SetValuei("ACTION_WEAPONZOOM",			SKEY_JOY_BUTTON07);

			pOptions->SetValuei("ACTION_DPAD_UP2",				SKEY_JOY_POV00);
			pOptions->SetValuei("ACTION_DPAD_RIGHT2",			SKEY_JOY_POV01);
			pOptions->SetValuei("ACTION_DPAD_DOWN2",			SKEY_JOY_POV02);
			pOptions->SetValuei("ACTION_DPAD_LEFT2",			SKEY_JOY_POV03);

			pOptions->SetValuei("ACTION_PAUSE2",				SKEY_JOY_BUTTON04);
			pOptions->SetValuei("ACTION_GUI_UP",				SKEY_JOY_POV00);
			pOptions->SetValuei("ACTION_GUI_DOWN",				SKEY_JOY_POV02);
			pOptions->SetValuei("ACTION_GUI_LEFT",				SKEY_JOY_POV03);
			pOptions->SetValuei("ACTION_GUI_RIGHT",				SKEY_JOY_POV01);
			pOptions->SetValuei("ACTION_GUI_OK",				SKEY_JOY_BUTTON00);
			pOptions->SetValuei("ACTION_GUI_CANCEL",			SKEY_JOY_BUTTON01);
			break;

		case 4:
			pOptions->SetValuei("ACTION_MOVE_FORWARD2",			SKEY_JOY_AXIS01_POS);
			pOptions->SetValuei("ACTION_MOVE_BACKWARD2",		SKEY_JOY_AXIS01_NEG);
			pOptions->SetValuei("ACTION_MOVE_LEFT2",			SKEY_JOY_AXIS00_NEG);
			pOptions->SetValuei("ACTION_MOVE_RIGHT2",			SKEY_JOY_AXIS00_POS);
			pOptions->SetValuei("ACTION_LOOK_UP",				SKEY_JOY_AXIS03_POS);
			pOptions->SetValuei("ACTION_LOOK_DOWN",				SKEY_JOY_AXIS03_NEG);
			pOptions->SetValuei("ACTION_LOOK_LEFT",				SKEY_JOY_AXIS02_NEG);
			pOptions->SetValuei("ACTION_LOOK_RIGHT",			SKEY_JOY_AXIS02_POS);

			pOptions->SetValuei("ACTION_JUMP2",					SKEY_JOY_BUTTON00); // "A"
			pOptions->SetValuei("ACTION_USE2",					SKEY_JOY_BUTTON02); // "X"
			pOptions->SetValuei("ACTION_CYCLEWEAPON2",			SKEY_JOY_BUTTON03); // "Y"
			pOptions->SetValuei("ACTION_RELOAD2",				SKEY_JOY_BUTTON01); // "B"

			pOptions->SetValuei("ACTION_SECONDARYFIRE2",		SKEY_JOY_BUTTON0A); // left trigger
			pOptions->SetValuei("ACTION_PRIMARYFIRE2",			SKEY_JOY_BUTTON0B); // right trigger
			pOptions->SetValuei("ACTION_DARKNESSDRAIN2",		SKEY_JOY_BUTTON08); // left shoulder button
			//		pOptions->SetValuei("ACTION_TOGGLEDARKLINGHUMAN",	SKEY_JOY_BUTTON08); //		(multiplayer only)
			pOptions->SetValuei("ACTION_DARKNESS2",				SKEY_JOY_BUTTON09); // right shoulder button
			pOptions->SetValuei("ACTION_JOURNAL2",				SKEY_JOY_BUTTON05);	// "back" button

			pOptions->SetValuei("ACTION_CROUCH2",				SKEY_JOY_BUTTON06); // left stick button
			//pOptions->SetValuei("ACTION_SCREENSHOT",			SKEY_JOY_BUTTON01); // right stick button
			pOptions->SetValuei("ACTION_WEAPONZOOM",			SKEY_JOY_BUTTON07);

			pOptions->SetValuei("ACTION_DPAD_UP2",				SKEY_JOY_POV00);
			pOptions->SetValuei("ACTION_DPAD_RIGHT2",			SKEY_JOY_POV01);
			pOptions->SetValuei("ACTION_DPAD_DOWN2",			SKEY_JOY_POV02);
			pOptions->SetValuei("ACTION_DPAD_LEFT2",			SKEY_JOY_POV03);

			pOptions->SetValuei("ACTION_PAUSE2",				SKEY_JOY_BUTTON04);
			pOptions->SetValuei("ACTION_GUI_UP",				SKEY_JOY_POV00);
			pOptions->SetValuei("ACTION_GUI_DOWN",				SKEY_JOY_POV02);
			pOptions->SetValuei("ACTION_GUI_LEFT",				SKEY_JOY_POV03);
			pOptions->SetValuei("ACTION_GUI_RIGHT",				SKEY_JOY_POV01);
			pOptions->SetValuei("ACTION_GUI_OK",				SKEY_JOY_BUTTON00);
			pOptions->SetValuei("ACTION_GUI_CANCEL",			SKEY_JOY_BUTTON01);
		    break;

		default:
			pOptions->SetValuei("ACTION_MOVE_FORWARD2",			SKEY_JOY_AXIS01_POS);
			pOptions->SetValuei("ACTION_MOVE_BACKWARD2",		SKEY_JOY_AXIS01_NEG);
			pOptions->SetValuei("ACTION_MOVE_LEFT2",			SKEY_JOY_AXIS00_NEG);
			pOptions->SetValuei("ACTION_MOVE_RIGHT2",			SKEY_JOY_AXIS00_POS);
			pOptions->SetValuei("ACTION_LOOK_UP",				SKEY_JOY_AXIS03_POS);
			pOptions->SetValuei("ACTION_LOOK_DOWN",				SKEY_JOY_AXIS03_NEG);
			pOptions->SetValuei("ACTION_LOOK_LEFT",				SKEY_JOY_AXIS02_NEG);
			pOptions->SetValuei("ACTION_LOOK_RIGHT",			SKEY_JOY_AXIS02_POS);

			pOptions->SetValuei("ACTION_JUMP2",					SKEY_JOY_BUTTON00); // "A"
			pOptions->SetValuei("ACTION_USE2",					SKEY_JOY_BUTTON02); // "X"
			pOptions->SetValuei("ACTION_CYCLEWEAPON2",			SKEY_JOY_BUTTON03); // "Y"
			pOptions->SetValuei("ACTION_RELOAD2",				SKEY_JOY_BUTTON01); // "B"

			pOptions->SetValuei("ACTION_SECONDARYFIRE2",		SKEY_JOY_BUTTON0A); // left trigger
			pOptions->SetValuei("ACTION_PRIMARYFIRE2",			SKEY_JOY_BUTTON0B); // right trigger
			pOptions->SetValuei("ACTION_DARKNESSDRAIN2",		SKEY_JOY_BUTTON08); // left shoulder button
			//		pOptions->SetValuei("ACTION_TOGGLEDARKLINGHUMAN",	SKEY_JOY_BUTTON08); //		(multiplayer only)
			pOptions->SetValuei("ACTION_DARKNESS2",				SKEY_JOY_BUTTON09); // right shoulder button
			pOptions->SetValuei("ACTION_JOURNAL2",				SKEY_JOY_BUTTON05);	// "back" button

			pOptions->SetValuei("ACTION_CROUCH2",				SKEY_JOY_BUTTON06); // left stick button
			//pOptions->SetValuei("ACTION_SCREENSHOT",			SKEY_JOY_BUTTON01); // right stick button
			pOptions->SetValuei("ACTION_WEAPONZOOM",			SKEY_JOY_BUTTON07);

			pOptions->SetValuei("ACTION_DPAD_UP2",				SKEY_JOY_POV00);
			pOptions->SetValuei("ACTION_DPAD_RIGHT2",			SKEY_JOY_POV01);
			pOptions->SetValuei("ACTION_DPAD_DOWN2",			SKEY_JOY_POV02);
			pOptions->SetValuei("ACTION_DPAD_LEFT2",			SKEY_JOY_POV03);

			pOptions->SetValuei("ACTION_PAUSE2",				SKEY_JOY_BUTTON04);
			pOptions->SetValuei("ACTION_GUI_UP",				SKEY_JOY_POV00);
			pOptions->SetValuei("ACTION_GUI_DOWN",				SKEY_JOY_POV02);
			pOptions->SetValuei("ACTION_GUI_LEFT",				SKEY_JOY_POV03);
			pOptions->SetValuei("ACTION_GUI_RIGHT",				SKEY_JOY_POV01);
			pOptions->SetValuei("ACTION_GUI_OK",				SKEY_JOY_BUTTON00);
			pOptions->SetValuei("ACTION_GUI_CANCEL",			SKEY_JOY_BUTTON01);
		    break;
		}
		DumpBinds(pOptions);

#else
		// Gamepad binds
		pOptions->SetValuei("ACTION_MOVE_FORWARD2", 129);
		pOptions->SetValuei("ACTION_MOVE_BACKWARD2", 128);
		pOptions->SetValuei("ACTION_MOVE_LEFT2", 131);
		pOptions->SetValuei("ACTION_MOVE_RIGHT2", 130);
		pOptions->SetValuei("ACTION_LOOK_UP", 133);
		pOptions->SetValuei("ACTION_LOOK_DOWN", 132);
		pOptions->SetValuei("ACTION_LOOK_LEFT", 135);
		pOptions->SetValuei("ACTION_LOOK_RIGHT", 134);

		pOptions->SetValuei("ACTION_JUMP2", 160);
		pOptions->SetValuei("ACTION_RELOAD2", 161);
		pOptions->SetValuei("ACTION_USE2", 162);
		pOptions->SetValuei("ACTION_CYCLEWEAPON2", 163);

		pOptions->SetValuei("ACTION_SECONDARYFIRE2", 136);
		pOptions->SetValuei("ACTION_PRIMARYFIRE2", 137);
		pOptions->SetValuei("ACTION_DARKNESSDRAIN2", 164);
		pOptions->SetValuei("ACTION_TOGGLEDARKLINGHUMAN", 164);
		
		//pOptions->SetValuei("ACTION_WEAPONZOOM", xxx?);
		
		pOptions->SetValuei("ACTION_DARKNESS2", 165);
		pOptions->SetValuei("ACTION_JOURNAL2", 166);

		pOptions->SetValuei("ACTION_CROUCH2", 168);

		pOptions->SetValuei("ACTION_DPAD_UP2", 176);
		pOptions->SetValuei("ACTION_DPAD_RIGHT2", 177);
		pOptions->SetValuei("ACTION_DPAD_DOWN2", 178);
		pOptions->SetValuei("ACTION_DPAD_LEFT2", 179);

		pOptions->SetValuei("ACTION_PAUSE2", 167);
		pOptions->SetValuei("ACTION_GUI_UP", 176);
		pOptions->SetValuei("ACTION_GUI_DOWN", 178);
		pOptions->SetValuei("ACTION_GUI_LEFT", 179);
		pOptions->SetValuei("ACTION_GUI_RIGHT", 177);
		pOptions->SetValuei("ACTION_GUI_OK", 160);
		pOptions->SetValuei("ACTION_GUI_CANCEL", 161);
#endif
	}

	// Read only
	pOptions->SetValuei("ACTION_LOOKAXIS", SKEY_MOUSEMOVEREL);
	pOptions->SetValuei("ACTION_PAUSE", SKEY_ESC);
	pOptions->SetValuei("ACTION_HELP", SKEY_F1);
	pOptions->SetValuei("ACTION_GUI_OK2", SKEY_RETURN);
//#endif
}

void CXRealityApp::Options_ClearBinds()
{
	CRegistry *pOptions = m_pSystem->GetOptions();
	for(int32 i = 0; i < pOptions->GetNumChildren(); i++)
	{
		if(pOptions->GetName(i).Left(7) == "ACTION_")
		{
			pOptions->DeleteKey(i);
			i--;
		}
	}
}

void CXRealityApp::Options_UpdateBinds()
{
	CRegistry *pSt = m_pSystem->GetRegistry()->FindChild("STRINGTABLES");
	CRegistry *pDynamicST = pSt ? pSt->FindChild("DYNAMIC") : NULL;
	CRegistry *pOptions = m_pSystem->GetOptions();
	if(!pSt || !pDynamicST)
		return;

	m_pSystem->m_spCon->Parser_BindRep("mousemove", "look(arg0,arg1)");

	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("MOVE_FORWARD", pOptions, pDynamicST, 0), "moveforward(1)", "moveforward(0)");
	m_pSystem->m_spCon->Parser_BindScanCode3(Options_GetScanKey("MOVE_FORWARD2", pOptions, pDynamicST, 1), "", "", "moveforward(Convert_fp32(arg0) / 128.0f)");
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("MOVE_BACKWARD", pOptions, pDynamicST, 0), "movebackward(1)", "movebackward(0)");
	m_pSystem->m_spCon->Parser_BindScanCode3(Options_GetScanKey("MOVE_BACKWARD2", pOptions, pDynamicST, 1), "", "", "moveforward(Convert_fp32(-arg0) / 128.0f)");
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("MOVE_RIGHT", pOptions, pDynamicST, 0), "moveright(1)", "moveright(0)");
	m_pSystem->m_spCon->Parser_BindScanCode3(Options_GetScanKey("MOVE_RIGHT2", pOptions, pDynamicST, 1), "", "", "moveright(Convert_fp32(arg0) / 128.0f)");
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("MOVE_LEFT", pOptions, pDynamicST, 0), "moveleft(1)", "moveleft(0)");
	m_pSystem->m_spCon->Parser_BindScanCode3(Options_GetScanKey("MOVE_LEFT2", pOptions, pDynamicST, 1), "", "", "moveright(Convert_fp32(-arg0) / 128.0f)");

	m_pSystem->m_spCon->Parser_BindScanCode3(Options_GetScanKey("LOOK_UP", pOptions, pDynamicST, 1), "", "", "lookvelocity_y(Convert_fp32(-arg0))");
	m_pSystem->m_spCon->Parser_BindScanCode3(Options_GetScanKey("LOOK_DOWN", pOptions, pDynamicST, 1), "", "", "lookvelocity_y(Convert_fp32(arg0))");
	m_pSystem->m_spCon->Parser_BindScanCode3(Options_GetScanKey("LOOK_RIGHT", pOptions, pDynamicST, 1), "", "", "lookvelocity_x(Convert_fp32(arg0))");
	m_pSystem->m_spCon->Parser_BindScanCode3(Options_GetScanKey("LOOK_LEFT", pOptions, pDynamicST, 1), "", "", "lookvelocity_x(Convert_fp32(-arg0))");

	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("JUMP", pOptions, pDynamicST, 0), "moveup(1); press(jump)", "moveup(0); release(jump)");
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("JUMP2", pOptions, pDynamicST, 0), "moveup(1); press(jump)", "moveup(0); release(jump)");
	m_pSystem->m_spCon->Parser_BindScanCode(Options_GetScanKey("CROUCH", pOptions, pDynamicST, 0), "togglecrouch()");
	m_pSystem->m_spCon->Parser_BindScanCode(Options_GetScanKey("CROUCH2", pOptions, pDynamicST, 0), "togglecrouch()");
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("WALK", pOptions, pDynamicST, 0), "walk(1)", "walk(0)");
	
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("PRIMARYFIRE", pOptions, pDynamicST, 0), "press(primary)", "release(primary)");
	m_pSystem->m_spCon->Parser_BindScanCode3(Options_GetScanKey("PRIMARYFIRE2", pOptions, pDynamicST, 0), "press(primary)", "release(primary)", "press_analog(0,arg0)");
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("SECONDARYFIRE", pOptions, pDynamicST, 0), "press(secondary)", "release(secondary)");
	m_pSystem->m_spCon->Parser_BindScanCode3(Options_GetScanKey("SECONDARYFIRE2", pOptions, pDynamicST, 0), "press(secondary)", "release(secondary)", "press_analog(1,arg0)");
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("DARKNESS", pOptions, pDynamicST, 0), "press(button2)", "release(button2)");
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("DARKNESS2", pOptions, pDynamicST, 0), "press(button2)", "release(button2)");
	m_pSystem->m_spCon->Parser_BindScanCode(Options_GetScanKey("TOGGLEDARKLINGHUMAN", pOptions, pDynamicST, 0), "cmd(mp_toggledarklinghuman())");
	m_pSystem->m_spCon->Parser_BindScanCode(Options_GetScanKey("WEAPONZOOM", pOptions, pDynamicST, 0), "cmd(sv_toggleweaponzoom())");

	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("USE", pOptions, pDynamicST, 0), "press(button0)", "release(button0)");
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("USE2", pOptions, pDynamicST, 0), "press(button0)", "release(button0)");
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("CYCLEWEAPON", pOptions, pDynamicST, 0), "press(button1)", "release(button1)");
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("CYCLEWEAPON2", pOptions, pDynamicST, 0), "press(button1)", "release(button1)");
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("RELOAD", pOptions, pDynamicST, 0), "press(button3)", "release(button3)"); // reload
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("RELOAD2", pOptions, pDynamicST, 0), "press(button3)", "release(button3)"); // reload
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("DARKNESSDRAIN", pOptions, pDynamicST, 0), "press(button5)", "release(button5)"); // zoom (changed to darknessdrain)
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("DARKNESSDRAIN2", pOptions, pDynamicST, 0), "press(button5); cmd(mp_toggledarklinghuman)", "release(button5)"); // zoom (changed to darknessdrain)
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("JOURNAL", pOptions, pDynamicST, 0), "press(button4)", "release(button4)"); // journal
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("JOURNAL2", pOptions, pDynamicST, 0), "press(button4)", "release(button4)"); // journal
//	m_pSystem->m_spCon->Parser_BindScanCode(Options_GetScanKey("JOURNAL", pOptions, pDynamicST, 0), "releaseall(); deferredscriptgrabscreen('cg_rootmenu(\"x06controller\")')");
//	m_pSystem->m_spCon->Parser_BindScanCode(Options_GetScanKey("JOURNAL2", pOptions, pDynamicST, 0), "releaseall(); deferredscriptgrabscreen('cg_rootmenu(\"x06controller\")')");

	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("DPAD_LEFT", pOptions, pDynamicST, 0), "press(dpad_left)", "release(dpad_left)");
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("DPAD_LEFT2", pOptions, pDynamicST, 0), "press(dpad_left)", "release(dpad_left)");
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("DPAD_RIGHT", pOptions, pDynamicST, 0), "press(dpad_right)", "release(dpad_right)");
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("DPAD_RIGHT2", pOptions, pDynamicST, 0), "press(dpad_right)", "release(dpad_right)");
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("DPAD_UP", pOptions, pDynamicST, 0), "press(dpad_up)", "release(dpad_up)");
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("DPAD_UP2", pOptions, pDynamicST, 0), "press(dpad_up)", "release(dpad_up)");
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("DPAD_DOWN", pOptions, pDynamicST, 0), "press(dpad_down)", "release(dpad_down)");
	m_pSystem->m_spCon->Parser_BindScanCode2(Options_GetScanKey("DPAD_DOWN2", pOptions, pDynamicST, 0), "press(dpad_down)", "release(dpad_down)");

	m_pSystem->m_spCon->Parser_BindScanCode(Options_GetScanKey("QUICKSAVE", pOptions, pDynamicST, 0), "savegame(\"Quick\")"); // save
	m_pSystem->m_spCon->Parser_BindScanCode(Options_GetScanKey("QUICKLOAD", pOptions, pDynamicST, 0), "deferredscriptgrabscreen(\"loadgame(\\\"Quick\\\")\")"); // load
	m_pSystem->m_spCon->Parser_BindScanCode(Options_GetScanKey("SCREENSHOT", pOptions, pDynamicST, 0), "capturescreen()"); // take screenshot
	
	m_pSystem->m_spCon->Parser_BindScanCode(Options_GetScanKey("HELP", pOptions, pDynamicST, 0), "cmd(helpbutton)");
	m_pSystem->m_spCon->Parser_BindScanCode(Options_GetScanKey("PAUSE", pOptions, pDynamicST, 0), "releaseall(); deferredscriptgrabscreen('cl_opengamemenu()')");
	m_pSystem->m_spCon->Parser_BindScanCode(Options_GetScanKey("PAUSE2", pOptions, pDynamicST, 0), "releaseall(); deferredscriptgrabscreen('cl_opengamemenu()')");

	// Just register for localization
	Options_GetScanKey("GUI_OK", pOptions, pDynamicST, 0);
	Options_GetScanKey("GUI_CANCEL", pOptions, pDynamicST, 0);
	Options_GetScanKey("GUI_UP", pOptions, pDynamicST, 0);
	Options_GetScanKey("GUI_DOWN", pOptions, pDynamicST, 0);
	Options_GetScanKey("GUI_LEFT", pOptions, pDynamicST, 0);
	Options_GetScanKey("GUI_RIGHT", pOptions, pDynamicST, 0);
	Options_GetScanKey("GUI_OK2", pOptions, pDynamicST, 0);

	// Uncomment this to dump all binding localization to the log-file
/*	for(int i = 0; i < pSt->GetNumChildren(); i++)
	{
		CRegistry *pChild = pSt->GetChild(i);
		for(int j = 0; j < pChild->GetNumChildren(); j++)
			if(pChild->GetValue(j).Find("§LCONTROLLER_") != -1)
				LogFile(pChild->GetName(j) + " = " + Localize_Str(pChild->GetValue(j)));
	}
	exit(0);*/
}

void CXRealityApp::Con_EnableMultithread(int _bEnabled)
{
	m_bEnableMultithreaded = (_bEnabled != 0);
}

#ifdef LOG_FRAME_INFO
void CXRealityApp::Con_LogFrameTimes(CStr _FileName)
{
	m_iCurrentMain = 0;
	m_iCurrentSys = 0;
	m_FrameCounter = 0;
	MACRO_GetSystem;
	m_FrameLogFile = pSys->m_ExePath + _FileName;
}
#endif

void CXRealityApp::Con_EnableThreadPool(int _bEnabled)
{
	MRTC_ThreadPoolManager::Enable(_bEnabled != 0);
}

#ifndef PLATFORM_CONSOLE

void CXRealityApp::Con_VideoFileBegin(CStr _Path, int _FPS)
{
	MAUTOSTRIP(CGameContext_Capture, MAUTOSTRIP_VOID);
	MSCOPE(CGameContext::Capture, GAMECONTEXT);


	CStr File;
#ifdef PLATFORM_XBOX
	CDiskUtil::CreatePath("T:\\DemoImages");
	File = CStr("T:\\DemoImages\\") + _Path;
#else
	File = (_Path.GetDevice().Len()) ? _Path : m_spGame->ResolvePath(_Path) + _Path.GetFilename();
#endif

	ConOut("Capturing to " + File);

	CImage* pImg = m_pSystem->m_spDisplay->GetFrameBuffer();
	if (!pImg) return;

#if !defined( PLATFORM_PS2 ) && !defined( PLATFORM_DOLPHIN )
	m_spVideoFile = MCreateVideoFile();
	if (!m_spVideoFile) return;
	if (!m_spVideoFile->Create(File, pImg->GetWidth(), pImg->GetHeight(), _FPS, 10))
	{
		ConOutL("(CGameContext::Capture) Video aborted.");
		m_spVideoFile = NULL;
	}

#endif
}

void CXRealityApp::Con_VideoFileEnd()
{
	if (m_spVideoFile != NULL)
		m_spVideoFile->Close();
	m_spVideoFile = NULL;
}

#endif

void CXRealityApp::Con_TraceRecordSystem()
{
	s_bTraceRecordSystem = true;
}

void CXRealityApp::Con_TraceRecordRender()
{
	s_bTraceRecordRender = true;
}

void CXRealityApp::Con_TraceRecordGameRefresh()
{
	s_bTraceRecordGameRefresh = true;
}

void CXRealityApp::Con_PerfCountSystem()
{
	s_bPerfCountSystem = true;
}

void CXRealityApp::Con_PerfCountRender()
{
	s_bPerfCountRender = true;
}



void CSystemThread::SystemThread()
{
	m_pApp->SystemThread(m_pDC);
}


#if defined(PLATFORM_XENON) && defined (M_Profile)
#pragma comment(lib, "xbdm.lib")
#endif
