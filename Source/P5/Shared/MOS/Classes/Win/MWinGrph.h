#ifndef __MWINGRPH_H
#define __MWINGRPH_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			2D to 3D helper class for drawing simple graphics

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CRC_Util2D
\*____________________________________________________________________________________________*/

#include "../../MOS.h"
#include "../Render/MRenderUtil.h"
#include "../../XR/XRSurf.h"
#include "../../XR/XRClass.h"
#include "../../XR/XRSkeleton.h"

class CRC_Font;

// -------------------------------------------------------------------
//  CRC_Util
// -------------------------------------------------------------------
class CRC_Util2D : public CReferenceCount, public CXR_ViewClipInterface
{
	TList_Vector<spCTextureContainer> m_lspTC;
	TList_Vector<int> m_lTCRef;
	CTextureContext* m_pTC;
	class CXR_SurfaceContext* m_pSC;

	CRenderContext* m_pCurRC;
	CXR_VBManager* m_pVBM;
	TPtr<CXW_SurfaceKeyFrame> m_spTmpSurfKey;
	CXW_Surface *m_pSurf;
	CXW_SurfaceKeyFrame *m_pSurfKeyFrame;
	spCXR_WorldLightState m_spWorldLightState;

	fp4 m_VBPriorityBase;
	fp4 m_VBPriorityAdd;
	fp4 m_VBPriority;
	fp4 m_TextOffset;
	
	int m_SurfOptions;
	int m_SurfCaps;

//	CRC_Attributes* m_pCurAttrib;
	CRC_Attributes m_Attrib;
	CRC_Attributes* m_pLastSubmittedAttrib;
//	int m_CurTextureID;
	CPnt m_CurTextureOrigo;
	CVec2Dfp4 m_CurTextureScale;
	int m_CurTxtW;
	int m_CurTxtH;
	CMat43fp4 m_CurTransform;
	CVec2Dfp4 m_CurScale;
	CVec2Dfp4 m_CurFontScale;

	CRC_Viewport m_CurVP;

	// Static:
	static uint32 ms_IndexRamp[16];
	static uint16 ms_DualTringle[6];

	int GetTCIndex(CTextureContainer* _pTC);
	void Clear();

	void UpdateTransform();

	CRC_Attributes* GetLastSubmitted();

public:
	CRC_Util2D();
	~CRC_Util2D();
	dllvirtual void AddTxtContainer(spCTextureContainer _spTC);
	dllvirtual void RemoveTxtContainer(spCTextureContainer _spTC);

	dllvirtual void Begin(CRenderContext* _pRC, CRC_Viewport* _pVP, CXR_VBManager* _pVBM = NULL);
	dllvirtual void End();

	dllvirtual void Flush();
//	bool GetAutoFlush() const { return m_bAutoFlush; };
//	void SetAutoFlush(bool _bFlush) { m_bAutoFlush = _bFlush; };
	void ClearTempSurface() { m_spTmpSurfKey = NULL; }

	dllvirtual CRenderContext* GetRC()
	{
		if (!m_pCurRC) 
			Error("GetRC", "No RC. (Did you call this outside Begin/End?");
		return m_pCurRC;
	};

	dllvirtual CXR_VBManager* GetVBM()
	{
		if (!m_pCurRC) 
Error("GetVBM", "No VBM. (Did you call this outside Begin/End?");
		return m_pVBM;
	};

	dllvirtual CTextureContext* GetTC()
	{
		return m_pTC;
	};


	dllvirtual void SetCoordinateScale(const CVec2Dfp4& _Scale);
	dllvirtual CVec2Dfp4 GetCoordinateScale() { return m_CurScale; };

	dllvirtual void SetPriorityValues(fp4 _PriorityBase, fp4 _PriorityAdd = 0.001f);

	dllvirtual const CMat43fp4& GetTransform() const { return m_CurTransform; };
	dllvirtual void SetTransform(const CMat4Dfp4& _Pos, const CVec2Dfp4& _Scale);

	dllvirtual void SetAttrib(CRC_Attributes* _pAttrib);	// _pAttrib is assumed to be valid until rendering/flushing is complete.

	dllvirtual void SetSurfaceOptions(int _Options);

	dllvirtual CRC_Attributes* GetAttrib();
	dllvirtual void SetSurface(class CXW_Surface *_pSurf, CMTime _AnimTime, int _Sequence = 0);
	dllvirtual void SetSurface(const char *_pSurfName, CMTime _AnimTime, int _Sequence = 0);
	dllvirtual void SetSurface(int _SurfaceID, CMTime _AnimTime, int _Sequence = 0);

	dllvirtual void SetTexture(int _TextureID);
	dllvirtual bool SetTexture(const char* _pTxtName);

	dllvirtual void SetTextureOrigo(const CClipRect& _Clip, const CPnt& _Origo);
	dllvirtual void SetTextureScale(fp4 _xScale, fp4 _yScale);
	dllvirtual void SetFontScale(fp4 _xScale, fp4 _yScale);
	inline int GetTextureWidth() { return m_CurTxtW; };
	inline int GetTextureHeight() { return m_CurTxtH; };
	
	dllvirtual void SetWorldLightState(spCXR_WorldLightState _spLightState);
	dllvirtual CXR_WorldLightState* GetWorldLightState() {return m_spWorldLightState;};
	
	dllvirtual void Pixel(const CClipRect& _Clip, const CPnt& _Pos, const CPixel32& _Color);
	dllvirtual void Rect(const CClipRect& _Clip, const CRct& _Rect, const CPixel32& _Color, CXR_Engine *_pEngine = NULL);
	dllvirtual void Rects(const CClipRect& _Clip, const CRct *_Rect, const CPixel32 *_Color, int _nRects);
	dllvirtual void Rect(const CClipRect& _Clip, const CRct& _Rect, const CPixel32& _Color, class CXR_Shader*, const class CXR_ShaderParams* _pShaderParams, const CXR_Light& _pLight);
#ifndef M_RTM
	dllvirtual void Circle(const CClipRect& _Clip, const CVec2Dfp4& _Mid, fp4 _Radius, int32 _nSegments, const CPixel32& _Color, bool _bBorder = false, const CPixel32& _BorderColor = CPixel32(255,255,255,255));
	dllvirtual void Line(const CClipRect& _Clip, const CVec2Dfp4& _Start, CVec2Dfp4& _End, fp4 _Width, const CPixel32& _Color);
#endif
	dllvirtual void AspectRect(const CClipRect& _Clip, const CRct& _Rect, const CPnt& _SourceSize, fp4 _SourcePixelAspect, const CPixel32& _Color = 0xffffffff);
	
	dllvirtual bool DrawTexture(const CClipRect& _Clip, const CPnt& _Pos, const char *_pTexture, const CPixel32 _Color = 0xffffffff, const CVec2Dfp4& _Scale = CVec2Dfp4(1.0f, 1.0f));
	dllvirtual void DrawSurface(const CClipRect& _Clip, const CPnt& _Pos, const char *_pSurface, const CPixel32 _Color = 0xffffffff, const CVec2Dfp4& _Scale = CVec2Dfp4(1.0f, 1.0f));
		
	// These functions don't work with particle models, since the models position is always the same
	dllvirtual void Model(CXR_Engine* _pEngine, const CClipRect& _Clip, const CRct& _Rect, class CXR_Model *_pModel, class CXR_AnimState *_pAnimState, CMat43fp4* _Mat=NULL); // Added by Talbot
	dllvirtual void Model(const CClipRect& _Clip, const CRct& _Rect, class CXR_Model *_pModel, class CXR_AnimState *_pAnimState, CMat43fp4* _Mat=NULL);
	dllvirtual void Model(CXR_Engine* _pEngine, class CWorld_Client* _pWClient, const CClipRect& _Clip, const CRct& _Rect, CXR_Anim_Base* pAnim, class CXR_Model* _pModel, class CXR_AnimState *_pAnimState); // Added by Talbot
	dllvirtual void Model(class CWorld_Client* _pWClient, const CClipRect& _Clip, const CRct& _Rect, CXR_Anim_Base* pAnim, class CXR_Model* _pModel, class CXR_AnimState *_pAnimState); // Added by Talbot

	dllvirtual void Rect3D(const CClipRect& _Clip, const CRct& _Rect, const CPixel32& _ColorH, const CPixel32& _ColorM, const CPixel32& _ColorD);
	dllvirtual void Sprite(const CClipRect& _Clip, const CPnt& _Pos, const CPixel32& _Color = 0xffffffff);
	dllvirtual void RotatedSprite(const CClipRect& _Clip, const CPnt& _Pos, const CPnt& _Range, fp4 _Angle, const CPixel32& _Color = 0xffffffff);
	dllvirtual void ScaleSprite(const CClipRect& _Clip, const CPnt& _Pos, const CPnt& _Size, const CPixel32& _Color = 0xffffffff);
	dllvirtual void Line(const CClipRect& _Clip, const CPnt& _p0, const CPnt& _p1, const CPixel32& _Color);
	dllvirtual void Lines(const CClipRect& _Clip, const CPnt *_p0, const CPnt *_p1, const CPixel32 *_Color0, const CPixel32 *_Color1, int _nLines);
	dllvirtual void Frame(const CClipRect& _Clip, int _x0, int _y0, int _x1, int _y1, const CPixel32& _Color0, const CPixel32& _Color1, bool _bInverse = false);
	dllvirtual void Text(const CClipRect& _Clip, CRC_Font* _pFont, int _x0, int _y0, const char* _pStr, const CPixel32& _Color, fp4 _Size = -1);
	dllvirtual void Text(const CClipRect& _Clip, CRC_Font* _pFont, int _x0, int _y0, const wchar* _pStr, const CPixel32& _Color, fp4 _Size = -1);
	dllvirtual void Text(const CClipRect& _Clip, CRC_Font* _pFont, int _x0, int _y0, const CStr& _Str, const CPixel32& _Color, fp4 _Size = -1);
	dllvirtual void TextFloat(const CClipRect& _Clip, CRC_Font* _pFont, fp4 _x0, fp4 _y0, const CStr& _Str, const CPixel32& _Color, fp4 _Size = -1);
	dllvirtual void Localize_Text(const CClipRect& _Clip, CRC_Font* _pFont, int _x0, int _y0, const char* _pStr, const CPixel32& _Color, fp4 _Size = -1);
	dllvirtual void Localize_Text(const CClipRect& _Clip, CRC_Font* _pFont, int _x0, int _y0, const wchar* _pStr, const CPixel32& _Color, fp4 _Size = -1);
	dllvirtual void Localize_Text(const CClipRect& _Clip, CRC_Font* _pFont, int _x0, int _y0, const CStr& _Str, const CPixel32& _Color, fp4 _Size = -1);
	
	dllvirtual int Text_DrawFormatted(const CClipRect& _Clip, CRC_Font* _pF, const CStr& _Text, int _x, int _y, int _Style, int _ColM, int _ColH, int _ColD, int _Width, int _Height, bool _bShadow);
	dllvirtual int Text_DrawFormatted(const CClipRect& _Clip, CRC_Font* _pF, const CStr& _Text, int& _x, int& _y, int _Style, int _ColM, int _ColH, int _ColD, int _Width, int _Height, fp4 _PercentVis);

	dllvirtual int Text_WordWrap(CRC_Font* _pF, int _Width, wchar* _pStr, int _Len, wchar** _ppLines, int _MaxLines);
	dllvirtual void Text_Draw(const CClipRect& _Clip, CRC_Font* _pF, wchar* _pStr, int _x, int _y, int _Style, int ColM, int ColH, int ColD);

	dllvirtual void DrawGraph(const CClipRect& _Clip, CRC_Font* _pF, const CRct &_Rect, const CPixel32 &_Background, const CPixel32 &_ColorLow, const CPixel32 &_ColorMid, const CPixel32 &_ColorHigh, fp4 _AvgValue, const fp4 *_pValues, int _nValues, fp4 _OriginalHeight, const ch8 *_pName);


	static int TextHeight(CRC_Font* _pFont, const char* _pStr = NULL);
	static int TextHeight(CRC_Font* _pFont, const wchar* _pStr);
	static int TextHeight(CRC_Font* _pFont, const CStr& _Str);
	static int TextWidth(CRC_Font* _pFont, const char* _pStr);
	static int TextWidth(CRC_Font* _pFont, const wchar* _pStr);
	static int TextWidth(CRC_Font* _pFont, const CStr& _Str);
	static int TextFit(CRC_Font* _pFont, const char* _pStr, int _Width, bool _bWordWrap);
	static int TextFit(CRC_Font* _pFont, const wchar* _pStr, int _Width, bool _bWordWrap);
	static int TextFit(CRC_Font* _pFont, const CStr& _Str, int _Width, bool _bWordWrap);

	//CXR_ViewClipInterface
	dllvirtual void View_Init(int _iView, CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_ViewClipInterface* _pViewClip,
		const CXR_AnimState* _pAnimState, const CMat43fp4& _WMat, const CMat43fp4& _VMat, int _Flags = 0);
	dllvirtual void View_Init(int _iView, CXR_Engine* _pEngine, CRenderContext* _pRender, CVec3Dfp4* _pVPortal, int _nVPortal,
		const CXR_AnimState* _pAnimState, const CMat43fp4& _WMat, const CMat43fp4& _VMat, int _Flags = 0);
	dllvirtual void View_SetCurrent(int _iView, CXR_SceneGraphInstance* _pSceneGraphInstance);
	dllvirtual bool View_GetClip_Sphere(CVec3Dfp4 _v0, fp4 _Radius, int _MediumFlags, int _ObjectMask, CRC_ClipVolume* _pClipVolume, CXR_RenderInfo* _pRenderInfo);
	dllvirtual bool View_GetClip_Box(CVec3Dfp4 _min, CVec3Dfp4 _max, int _MediumFlags, int _ObjectMask, CRC_ClipVolume* _pClipVolume, CXR_RenderInfo* _pRenderInfo);
};

typedef TPtr<CRC_Util2D> spCRC_Util2D;

#endif // _INC_MWINGRPH
