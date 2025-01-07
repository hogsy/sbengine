#ifndef _INC_WFRONTENDMOD
#define _INC_WFRONTENDMOD

#include "WFrontEndInfo.h"

#include "../../Shared/MOS/Classes/GameWorld/FrontEnd/WFrontEnd.h"
#include "../../Shared/MOS/XR/XREngineImp.h"

#include "MWinCtrlMod.h"
#include "WFrontEndMod_Cube.h"

#define NUMBEROFMAPPIECES 16
#define NUMBEROFMAPPIECESOW 7
//
// ---------------------------------------------------------
//  CMapPiece
//	Used to get a better structure for rendering out the map in FrontEndMod_Menus
// -------------------------------------------------------------------

class CMapPieces
{
public:
	
	enum
	{
		GUI_CANAL = 0,
		GUI_CEMETARY,
		GUI_CHINATOWN,
		GUI_CHURCH,
		GUI_CITYHALL,
		GUI_CONSITE,
		GUI_FULTON,
		GUI_GRINDER,
		GUI_GUNHILL,
		GUI_HUNTER,
		GUI_LOW_EAST,
		GUI_MANSION,
		GUI_ORPHANGE,
		GUI_PIER,
		GUI_TUNNEL,
		GUI_TURKISH,

		GUI_TRENCH,
		GUI_VILLAGE,
		GUI_SEWERS,
		GUI_CANNON,
		GUI_CASTLE,
		GUI_HILLS,
		GUI_TANKRIDE,

		/*

		GUI_trench,
		GUI_village,
		GUI_sewers,
		GUI_cannon,
		GUI_castle,
		GUI_hills,
		GUI_tankride,

		OW1_Trench,
		OW1_Village
		OW1_Sewers
		OW1_Cannon
		OW2_Castle
		OW1_Hills
		OW2_Tankride
		*/


	};

	CMapPieces();
	void SetupAndPrecacheImages();
	
	M_INLINE CPnt GetPos(int16 _iPiece) const { return m_lPieces[_iPiece].m_Pos;}
	M_INLINE CPnt GetGlowPos(int16 _iPiece) const { return m_lPieces[_iPiece].m_GlowPos;}
	M_INLINE int32 GetImageID(int16 _iPiece) const { return m_lPieces[_iPiece].m_ImageID;}
	M_INLINE int32 GetGlowImageID(int16 _iPiece) const { return m_lPieces[_iPiece].m_GlowImageID;}
	M_INLINE int32 GetBackGroundImage(bool _bNY) const { if(_bNY) return m_BackgroundImageID; else return m_BackgroundImageIDOW;}
	M_INLINE bool ShouldRender(int16 _iPiece) const { return m_lPieces[_iPiece].m_bEnabled;}
	M_INLINE int16 GetNumberOfPieces(bool _bNY) const { if(_bNY) return Min(m_lPieces.Len(), NUMBEROFMAPPIECES); else return Min(m_lPieces.Len(), NUMBEROFMAPPIECESOW);}
	M_INLINE int16 GetGlowingPiece() const { return m_GlowingCurrentPiece;}
	M_INLINE CStr GetActualPieceLevelName(int16 _iPiece) const { return m_lPieces[_iPiece].m_ActualLevelName;}
	
	M_INLINE void SetPieceVisited(int16 _iPiece) { m_lPieces[_iPiece].m_bEnabled = true;}
	M_INLINE void SetPieceCurrentPiece(int16 _iPiece) { m_GlowingCurrentPiece = _iPiece;}


private:

	struct SPieceInfo 
	{
		CPnt	m_Pos;
		int32	m_ImageID;
		CPnt	m_GlowPos;
		int32	m_GlowImageID;
		bool	m_bEnabled;
		CStr	m_ActualLevelName;
	
	};

	int m_GlowingCurrentPiece;
	TArray<SPieceInfo> m_lPieces;
	int m_BackgroundImageID;
	int m_BackgroundImageIDOW;

	void SetupAndPrecacheImage(CTextureContext *pTC, CStr _ImageName, CPnt _ImagePos, CPnt _ImageGlowPos, int16 _iPieceID, CStr _ActualLevelName);
};

//
// ---------------------------------------------------------
//  CWFrontEnd_Mod
// -------------------------------------------------------------------
class CWFrontEnd_Mod : public CWFrontEnd
{
	MRTC_DECLARE;
public:

	CWFrontEnd_Mod();

	CRC_Viewport m_3DViewPort;

	bool m_bShowNoBlocksMessage;

	int32 m_QueuedSide;
	bool m_bSwapSide;
	int32 m_iPrevWindow;
	bool m_ReadyToChange;
	bool m_PausedGame;
	bool m_bShowConfidential;


	CPnt m_ScreenSize;
	CPnt m_MousePosition;
	bool m_MouseMoved;
	fp32 m_CurAspectYScale;
	CPnt GetTransformedMousePosition();


	void SetFrontEndInfo_r(CMWnd *_pWnd);

	CMTime m_LoadingTimeout;

	CCube m_Cube;
	CMWnd *m_pCurrentWnd;
	bool m_ShouldRender;
	int32 m_CubeReferneces;
	bool m_bDonePrecache;
	virtual void Cube_Render(class CRenderContext* _pRC, class CXR_VBManager* _pVBM, bool _IsJournal = false, bool _SpecialOverride = false);
	virtual void Cube_ShouldWorldRender(bool _Yes);
	virtual void Cube_SetStateFromLayout(CMWnd *_pWnd, bool _Instant = false);
	virtual void Cube_Update(CMWnd *_pWnd);
	virtual void Cube_Raise(CMWnd *_pWnd);
	virtual bool Cube_ShouldRender();
	virtual void Cube_SetSecondaryRect(CRct _Rect);
	//virtual void Cube_SetSecondaryTexture(CStr _TexName);
	virtual void Cube_SetSecondary(int32 _iSomething, int32 _iBlend);
	
	virtual void Cube_UpdateMouse(CMWnd *_pWnd);
	virtual bool Cube_MouseClick(CMWnd *_pWnd);
	virtual bool Cube_IsMouseOver(CMWnd *_pWnd, COutsideButton &rButton);

	virtual void Cube_InitLoadingCubeRemoval();
	virtual void Cube_DoLoadingLayout();
	virtual bool Cube_LoadingIsTakingToMuchTime();

	virtual void LoadAllResources();
	virtual void OnPrecache(class CXR_Engine* _pEngine);
	virtual void DoPrecache(class CRenderContext* _pRC, class CXR_Engine *_pEngine);

	virtual CMWnd *GetCursorWindow(bool _bOnlyTabstops);

	dllvirtual void DoClientRenderCheck();

	TPtr<CTextureContainer_Screen> m_spGameTexture;

	CMTime m_LastTime;
	bool m_OutOfGUI;
	bool m_ClientMouse;


	CMTime m_AttractTime;


	CStr m_aWindowTransferData[2];


	// Screen grab
	int32 m_PrevScreenGrabRef;
	int32 m_ScreenGrabRef;
	bool m_bGrabScreen;
	bool GrabScreenDone();

	void GrabScreen(class CRenderContext* _pRC);
	void GrabScreen();

	// Chooser data
	CStr m_Question;
	TArray<CStr> m_lChoices;
	TArray<CStr> m_lChoicesCommands;
	CStr m_BackoutCommand;

	void Chooser_Init(CStr _Question);
	void Chooser_AddChoice(CStr _Name, CStr _Cmd);
	void Chooser_Invoke(bool _Sub=true);

	//
	spCMWnd CreateWindowFromName(const char *_pName);
	/*
	int32 m_ActiveGUITexture;
	CCube::CSideTexture m_GUITextureInfo[2];
	TPtr<CTextureContainer_RenderCallback> m_spGUITexture[2];
	//*/
	

	CStr m_CachedCommand;

	CPnt GetPointFromReg(const CRegistry *pReg, const char *_pName) const;

	virtual void Create(CWF_CreateInfo& _CreateInfo);
	virtual	bool ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey);
	
	virtual void OnRefresh();
	virtual void OnRender(class CRenderContext* _pRC, class CXR_VBManager* _pVBM);
	void OnBuildInto(class CRenderContext* _pRC);

	virtual void SetSoundContext(spCSoundContext _spSoundContext);
	virtual void SetWindow(CStr _ID);
	virtual void OnPostRenderInterface(class CRC_Util2D* _pRCUtil, CClipRect _Clip, CVec2Dfp32 *_pScale);
	virtual void RenderHelpButtons(CMWnd *_pWnd, class CRC_Util2D* _pRCUtil, CClipRect _Clip, CVec2Dfp32 *_pScale = NULL);
	
	virtual void Con_GrabScreen();
	virtual void Con_Cheat(CStr _Cheat);
	virtual void Con_CacheCommand(CStr _Cmd);
	virtual void Con_DoCacheCommand();
	virtual void Con_CG_Backout();
	virtual void Con_CG_BlackLoading();
	virtual void Con_CG_CubeSide(int i);
	virtual void Con_CG_CubeSeq(CStr _Seq);
	virtual void Con_CG_CubeResetWindow();
	virtual void Con_CG_CubeSeqForce(CStr _Seq);
	virtual void Con_CG_ReloadUIRegistry();
	virtual void Con_PlayRandomMusic(CStr _Music);
	virtual void Con_CG_SelectSaveUnit();
	virtual void Register(CScriptRegisterContext & _RegContext);

	CStr	m_CubeWndName;

	CMapPieces m_MapPieces;
};

#include "WFrontEndMod_Menus.h"

#endif // _INC_WCLIENTGAMEMOD


