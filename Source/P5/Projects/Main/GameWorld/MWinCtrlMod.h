
#ifndef _INC_MWINCTRLMOD
#define _INC_MWINCTRLMOD

#include "../../Shared/MOS/Classes/Win/MWinCtrl3D.h"
#include "../../Shared/MOS/Classes/GameWorld/WMapData.h"
#include "../../Shared/MOS/Classes/Win/MWinMovieController.h"
#include "WFrontEndInfo.h"

#define WMSG_SETSTATE	0x0200
#define WMSG_GETSTATE	0x0201

#define WMSG_REGISTRYCHANGED 0x0202

#define WMSG_GETMAPDATA 0x0203

// -------------------------------------------------------------------
//  CM_Mod_ColorController
// -------------------------------------------------------------------

enum
{
	CM_Mod_ColorController_White,
	CM_Mod_ColorController_Yellow,
	CM_Mod_ColorController_Blue,
};

class CM_Mod_ColorController
{
public:

	// Create

	CM_Mod_ColorController();

	// Get

	int iGetColor(fp32 fpTransparentScale, fp32 fpFreqScale, int iSlot, bool bDisabled, int iColorId);
	int iGetColorFrameTimed(fp32 fpTransparentScale, fp32 fpFreqScale, int iSlot, bool bDisabled, int iColorId);

	fp32 m_fpTime;

	// User

	fp32 m_fpTransparentScale;
	fp32 m_fpFreqScale;
	int m_iSlot;
	bool m_bDisabled;
	int m_iColorId;
};


// -------------------------------------------------------------------
//  CCellInfo, UGLY AS HELL, but.. bah
// -------------------------------------------------------------------
class CCellInfo
{
public:
	enum
	{
		MODE_NORMAL=0,	// SUPPORTED, also got flags
		MODE_HUGE,		// SUPPORTED
		MODE_SMALL,	// SUPPORTED, one faces split into two
		MODE_SECONDARY,	// SUPPORTED
		PART_TL=0,
		PART_TR,
		PART_BL,
		PART_BR,

		FLAG_FASTSWITCH=1,

		RANDOM_BELOW=16*12-2,

	};

	CCellInfo()
	{
		//memset(this, 0, sizeof(*this));
		m_Data[0] = 0;
		m_Data[1] = 0;
		m_Mode = MODE_NORMAL;
		m_Flags = 0;
		Cell() = Empty();
	}

	uint8 m_Mode;
	uint8 m_Flags;

	//
	uint16 m_Data[2];
	static uint16 Empty() { return uint16(Random*RANDOM_BELOW); }
	bool CellIsEmpty() const { return Cell() < RANDOM_BELOW; } // Normal and Huge mode
	uint16 &Cell() { return m_Data[0]; } // Normal and Huge mode
	const uint16 &Cell() const { return m_Data[0]; } // Normal and Huge mode
	uint8 &Flags() { return m_Flags; }
	const uint8 &Flags() const { return m_Flags; }
	uint16 &Part() { return m_Data[1]; } // Huge mode
	const uint16 &Part() const { return m_Data[1]; } // Huge mode
	uint16 &SubCell(int32 _i) { return m_Data[_i]; } // small mode
	const uint16 &SubCell(int32 _i) const { return m_Data[_i]; } // small mode


	/*bool IsOK()
	{
		if(m_Mode == MODE_NORMAL)
		{
			if(Flags() == 0 || Flags() == FLAG_FASTSWITCH)
				return true;
		}
		else if(m_Mode == MODE_HUGE)
		{
			if(Part() >= PART_TL && Part() <= PART_BR && Flags() == 0 || Flags() == FLAG_FASTSWITCH)
				return true;
		}
		else if(m_Mode == MODE_SMALL)
		{
			if(Flags() == 0 || Flags() == FLAG_FASTSWITCH)
				return true;
		}

		return false;
	}*/

	bool operator ==(CCellInfo &_rCell) const
	{
		if(m_Mode == _rCell.m_Mode)
		{
			if(m_Mode == MODE_SECONDARY)
				return true;

			if(m_Mode == MODE_NORMAL)
			{
				if(Cell() == _rCell.Cell())
					return true;
				if(Cell() < RANDOM_BELOW && _rCell.Cell() < RANDOM_BELOW)
					return true;
				
				return false;
			}

			if(m_Mode == MODE_HUGE && Cell() == _rCell.Cell() && Part() == _rCell.Part())
				return true;

			if(m_Mode == MODE_SMALL)
			{
				if((SubCell(0) == _rCell.SubCell(0)) && (SubCell(1) == _rCell.SubCell(1)))
					return true;
				if((SubCell(0) < RANDOM_BELOW && _rCell.SubCell(0) < RANDOM_BELOW) && (SubCell(1) == _rCell.SubCell(1)))
					return true;
				if((SubCell(1) < RANDOM_BELOW && _rCell.SubCell(1) < RANDOM_BELOW) && (SubCell(0) == _rCell.SubCell(0)))
					return true;
				if((SubCell(0) < RANDOM_BELOW && _rCell.SubCell(0) < RANDOM_BELOW) && (SubCell(1) < RANDOM_BELOW && _rCell.SubCell(1) < RANDOM_BELOW))
					return true;
			}
		}

		return false;
	}
};

// -------------------------------------------------------------------
//  CMWnd_CubeRgn
// -------------------------------------------------------------------
class CMWnd_CubeRgn : public CMWnd
{
	MRTC_DECLARE;
public:
	CRct GetPositionCube();
	virtual void EvaluateKeyOrdered(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	void OnPaintFocus(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client) {}
};

// -------------------------------------------------------------------
//  CMWnd_CubeLayout
// -------------------------------------------------------------------
class CMWnd_CubeLayout : public CMWnd
{
	MRTC_DECLARE;
public:

	TArray<CCellInfo> m_lLookup;

	CCellInfo m_aMap[20][20];
	fp32 m_aDepth[20][20];

	CRct m_SecondaryRect;
	int32 m_iSecondaryID;
	int32 m_BlendMode;

	//CMWnd *m_pDescription;
	TArray<CMWnd*> m_lButtons;

	CRct m_CurrentItemRect;
	int32 m_iCurrentListItem;
	int32 m_iFirstListItem;
	CMTime m_LastListScroll;
	uint16 m_iTentacleTemplate;
	virtual int32 GetNumListItems(bool _Drawing) { return -1; }
	virtual bool GetListItem(int32 _Index, CStr &_Name, bool _Focus) {return false;}; // should return false when no more there are nomore items
	virtual void AfterItemRender(int32 _Index) {};
	CRct GetCurrentListItemRct() { return m_CurrentItemRect; };

	enum
	{
		FLAG_ADJUST_SIZE=1,
		FLAG_KEEP_BOUNDS=2,
		FLAG_NO_CONVERT=4,
		FLAG_CENTER=8,
		FLAG_NO_WRAP=16,
		FLAG_FASTSWITCH=32,
	};

	CRct Layout_Viewspace2CubeSpace(CRct _Pos);
	CRct Layout_CubeSpace2ViewSpace(CRct _Pos);
	void Layout_AddLookup(int32 _x, int32 _y);
	void Layout_SetMappingBig(int32 _x, int32 _y, int32 _ID, int32 _Flags=0);
	void Layout_SetMapping(int32 _x, int32 _y, int32 _ID, int32 _Flags=0);
	void Layout_SetMapping(CRct _rPos, int32 _ID, int32 _Flags=0);
	void Layout_SetDepth(CRct _rPos, fp32 _Depth);
	virtual void Layout_Clear(CRct _rPos, int32 _Flags=0);
	void Layout_Copy(CMWnd_CubeLayout *pLayout);
	void Layout_WriteText(CRct &_rPos, int32 FontSize, const wchar *aText, int32 _Flags = 0);
	void Layout_WriteText(CRct &_rPos, CStr _Value, int32 _Flags = 0);
	void Layout_SetSecondary(int32 _iSomething, int32 _BlendMode);
	bool Layout_WndIsButton(CMWnd*);

	CMWnd_CubeLayout();
	virtual ~CMWnd_CubeLayout();
	virtual void OnCreate();
	virtual	int ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey); // this can override the normal GUI cursor movement to handle the list
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual void OnRefresh();
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void EvaluateRegisty(CMWnd_Param* _pParam, CRegistry *_pReg);
};

// -------------------------------------------------------------------
//  CMWnd_CubeRect
// -------------------------------------------------------------------
class CMWnd_CubeSecondaryRect : public CMWnd_CubeRgn
{
	MRTC_DECLARE;

public:
	bool m_Set;
	CMWnd_CubeSecondaryRect();
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void OnRefresh();
};

// -------------------------------------------------------------------
//  CMWnd_CubeText
// -------------------------------------------------------------------
class CMWnd_CubeText : public CMWnd_CubeRgn
{
	MRTC_DECLARE;

public:

	CMWnd_CubeText()
	{
		m_AlwaysPaint = false;
	}

	CStr m_Text;
	int m_AlwaysPaint;
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
};

// -------------------------------------------------------------------
//  CMWnd_CubeMeter
// -------------------------------------------------------------------
class CMWnd_CubeMeter : public CMWnd_CubeRgn
{
	MRTC_DECLARE;
public:
	virtual fp32 GetValue() { return 0.5f; };
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
};

// -------------------------------------------------------------------
//  CMWnd_CubeOptionMeter
// -------------------------------------------------------------------
class CMWnd_CubeOptionMeter : public CMWnd_CubeMeter
{
	MRTC_DECLARE;
public:
	CStr m_Option;
	fp32 m_Max;
	fp32 m_Min;

	CMWnd_CubeOptionMeter();
	virtual fp32 GetValue();
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
};


// -------------------------------------------------------------------
//  CMWnd_ModInfoScreen
// -------------------------------------------------------------------
class CMWnd_ModInfoScreen : public CMWnd
{
	MRTC_DECLARE;

public:
	CM_Mod_ColorController m_CC;

	int		m_Result;
	bool	m_ShouldCreate;
	bool	m_bActive;
	CStr	m_Information;
	CMWnd*	m_LastFocus;
	int		m_ScreenId;	// Helps the calling window seperate the different screens
	bool    m_bNoSound;

	bool	m_bTimerMode;
	CMTime	m_StartTime;
	fp32		m_TotalTime;

	bool	m_HasRendered;

	bool	m_bCloseAfterQuestion;

	CMWnd_ModInfoScreen();
	~CMWnd_ModInfoScreen();
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual void OnRefresh();
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
	
	void vActivate(const CStr& _sText, const CStr& _sAButton, const CStr& _sBButton, int _iId,  CMWnd * _pFocus);
	void vActivate(const CStr& _sText, int _nType, CMWnd * _pFocus);
	void vActivate(const CStr& _sText, fp32 fpTimeInfoMessage);
	void vDeactivate();
};

// -------------------------------------------------------------------
//  CMWnd_ModEdit
// -------------------------------------------------------------------
class CMWnd_ModEdit : public CMWnd_Edit
{
	MRTC_DECLARE;

public:
	CStr m_Script_Pressed;
	int m_ScrollPos;
	bool m_FocusEdit;
	CMWnd* m_SpawnedWindow;
	bool m_RemoveSpawnedWindow;

	CMWnd_ModEdit();

	virtual void Execute(const CStr& _Script);
	virtual int OnPressed(const CMWnd_Message* _pMsg);
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual void OnRefresh();
	virtual aint OnMessage(const CMWnd_Message* _pMsg);

	virtual CStr OnGetDumpInformation() { return CStrF("%s, Script '%s'", (char*)CMWnd_Edit::OnGetDumpInformation(), (char*)m_Script_Pressed); };
};

// -------------------------------------------------------------------
//  CMWnd_ModButton
// -------------------------------------------------------------------
class CMWnd_ModButton : public CMWnd_Button
{
	MRTC_DECLARE;

public:
	CStr m_Script_Pressed;
	
	// Other
	
	CStr m_Padedit_Scankey;

	bool m_NoBackground;
	bool m_NoOnPaintBackground;
	bool m_NoSounds;
	
	int m_OffsetX;

	int m_ModCommand;
	
	CM_Mod_ColorController m_CC;

	CMWnd_ModButton();

	virtual void Execute(const CStr& _Script);
	virtual int OnPressed(const CMWnd_Message* _pMsg);
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void OnPaintFocus(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
	virtual void OnSetFocus();

	virtual CStr OnGetDumpInformation() { return CStrF("%s, Script '%s'", (char*)CMWnd_Button::OnGetDumpInformation(), (char*)m_Script_Pressed); };
};

// -------------------------------------------------------------------
//  CMWnd_CubeButton
// -------------------------------------------------------------------

class CMWnd_CubeButton : public CMWnd_ModButton
{
	MRTC_DECLARE;
public:
	CMWnd_FrontEndInfo m_FrontEndInfo;
	CMWnd_FrontEndInfo *GetFrontEndInfo()
	{
		return &m_FrontEndInfo;
	}

	CMWnd_CubeButton();
	bool m_AlwaysPaint;
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);

	virtual void EvaluateRegisty(CMWnd_Param* _pParam, CRegistry *_pReg);
	virtual void EvaluateKeyOrdered(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);

	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual void OnPaintFocus(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);

	virtual void SoundEvent(uint _SoundEvent, const char* _pSoundName = NULL);
};

// -------------------------------------------------------------------
//  CMWnd_CubeOptionButton
// -------------------------------------------------------------------
class CMWnd_CubeOptionButton : public CMWnd_CubeButton
{
	MRTC_DECLARE;
public:
	CStr m_Option;
	CStr m_OptionList;
	fp32 m_Max;
	fp32 m_Min;
	int32 m_Steps;
	fp32 m_StepUp;
	fp32 m_StepDown;
	
	bool m_Enviroment;
	bool m_SfxVol;

	CMWnd_CubeOptionButton();
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);

	virtual void SoundEvent(uint _SoundEvent, const char* _pSoundName = NULL);
};

// -------------------------------------------------------------------
//  CMWnd_CubeSwitchButton
// -------------------------------------------------------------------
class CMWnd_CubeSwitchButton : public CMWnd_CubeButton
{
	MRTC_DECLARE;

	bool				m_bStrValue;
	TThinArray<CStr>	m_lStrValues;
	TThinArray<int32>	m_lIntValues;

	bool				m_bDisplayStrValue;
	TThinArray<CStr>	m_lDisplayStrValues;
	TThinArray<int32>	m_lDisplayIntValues;

	CStr				m_OptionKey;

	uint32				m_iSelected;

public:

	CMWnd_CubeSwitchButton();
	virtual void OnCreate();
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
};

// -------------------------------------------------------------------
//  CMWnd_CubeText
// -------------------------------------------------------------------
/*
class CMWnd_CubeText : public CMWnd_CubeRgn
{
	MRTC_DECLARE;
	
	CStr m_Text;
public:
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
};*/

// -------------------------------------------------------------------
//  CMWnd_ModText
// -------------------------------------------------------------------
class CMWnd_ModText : public CMWnd_Text
{
	MRTC_DECLARE;
	
public:
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
};

// -------------------------------------------------------------------
//  Phobos: CMWnd_ModTexture
// -------------------------------------------------------------------
class CMWnd_ModTexture : public CMWnd
{
	MRTC_DECLARE;

protected:
	enum CMWnd_ModTextureMode
	{
		CMWnd_ModTextureMode_Texture,
		CMWnd_ModTextureMode_Repeat_Forever,
		CMWnd_ModTextureMode_Repeat_N,
		CMWnd_ModTextureMode_Link
	};

	CMWnd_ModTextureMode		m_TextureMode;
	int						m_Repeat;
	int						m_Color;
	CStr					m_LinkWindow;

	// Which texture to use
	CStr					m_TextureName;
	CMWinMoviewController	m_MovieController;
	CVec2Dfp32				m_Offset;
	fp32						m_PixelAspect;

	void LinkWindow();
public:
	CMWnd_ModTexture();
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual void PaintVideo(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual void PaintTexture(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
};

// -------------------------------------------------------------------
//  CMWnd_ModText
// -------------------------------------------------------------------
class CMWnd_ModOptionBox : public CMWnd_Text
{
	MRTC_DECLARE;
	TArray<CStr> m_lOptions;
	int m_iOption;
	
public:
	CMWnd_ModOptionBox();
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual int OnPressed(const CMWnd_Message* _pMsg);
};

// -------------------------------------------------------------------
//  CMWnd_ModInvObject
// -------------------------------------------------------------------
/*
#define MODINVOBJ_MAXMODELS	10

class CMWnd_ModInvObject : public CMWnd_Viewport
{
	MRTC_DECLARE;

public:
	static CMTime ms_Time;

	int m_iModels[MODINVOBJ_MAXMODELS];			// Resource index
	spCMapData m_spMapData;
	CMat4Dfp32 m_ModelPos;			// Local to World matrix (WMat)
	CMat4Dfp32 m_Camera;				// Camera

	int m_Flags;
	int m_ItemNr;

	TPtr<CXR_WorldLightState> m_spWLS;

	virtual void Clear();

	CMWnd_ModInvObject();
	CMWnd_ModInvObject(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID);
	virtual void SetModel(spCMapData _spMapData, int _iModel, int _ModelNr = 0);
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
};

typedef TPtr<CMWnd_ModInvObject> spCMWnd_ModInvObject;
*/
// -------------------------------------------------------------------
//  CMWnd_ModInvSquare
// -------------------------------------------------------------------
class CMWnd_ModInvSquare : public CMWnd
{
	MRTC_DECLARE;
public:

	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
};

// -------------------------------------------------------------------
//  CMWnd_ModFrame
// -------------------------------------------------------------------
class CMWnd_ModFrame : public CMWnd
{
	MRTC_DECLARE;
public:

	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
};

// -------------------------------------------------------------------
//  CMWnd_ModPlaneFrame
// -------------------------------------------------------------------
class CMWnd_ModPlaneFrame : public CMWnd
{
	MRTC_DECLARE;
public:

	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
};

// -------------------------------------------------------------------
//  CMWnd_ModChainFrame
// -------------------------------------------------------------------
class CMWnd_ModChainFrame : public CMWnd
{
	MRTC_DECLARE;
public:

	CMWnd_ModChainFrame()
	{
		m_TopTitleTiles = 1;
		m_BottomTitleTiles = 2;
	}

	int m_TopTitleTiles;
	CStr m_Title;

	int m_BottomTitleTiles;
	CStr m_BottomLine;

	void DrawTile(const CStr& TileName, const CPnt& Point, const CPnt& Size, CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
};

// -------------------------------------------------------------------
//  CMWnd_ModBitmap
// -------------------------------------------------------------------
class CMWnd_ModBitmap : public CMWnd
{
	MRTC_DECLARE;
public:
	CStr m_Texture;

	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
};

// -------------------------------------------------------------------
//  CMWnd_List2
// -------------------------------------------------------------------
class CWL2_Column
{
public:
	CStr m_Name[32];
	int m_Width;
	CMWnd_Button* m_pWndBtn;

	CWL2_Column()
	{
		m_Width = 10;
		m_pWndBtn = NULL;
	}
};

// -------------------------------------------------------------------
class CMWnd_L2Client : public CMWnd
{
	MRTC_DECLARE;

protected:
	class CMWnd_List2* m_pList;
	int m_Action;

public:
	CMWnd_L2Client();
	int Pos2Item(const CPnt& _Pos);
	virtual void SetContext(CMWnd_List2* _pContext);
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
};

// -------------------------------------------------------------------
class CMWnd_L2ClientContainer : public CMWnd_Client
{
	MRTC_DECLARE;

	CStr m_ClassName;

public:
	CMWnd_L2ClientContainer();
	virtual void PreCreate(CMWnd_Param* _pParam);
	virtual void Create(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, const CStr& _Class);
	virtual void SetContext(CMWnd_List2* _pContext);

	virtual spCMWnd CreateClient();
};

// -------------------------------------------------------------------
class CMWnd_List2 : public CMWnd
{
	MRTC_DECLARE;

public:
	CMWnd_L2ClientContainer* m_pWndClient;

	TArray<CWL2_Column> m_lCol;
	TArray<uint8> m_lSelect;
	int m_ItemHeight;
	int m_TextHeight;
	int m_nItems;
	int m_MinListWidth;
	int m_ListWidth;
	bool m_bUpdate;
	int m_ClientStyle;
	int m_iItemFocus;

	virtual void DoCreate(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID);

	void Update();
	void Clear();

public:
	CMWnd_List2();

	virtual CStr GetClientClass();

	virtual void PreCreate(CMWnd_Param* _pParam);
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void OnMove();
	virtual void OnRefresh();

	int IsValidItem(int _iItem);
	int GetItemCount();
	virtual void SetItemCount(int _nItems);
	void SetColumnCount(int _nColumns);
	void SetColumn(const CStr& _Name, int _OrgWidth, int _Flags);

	int GetSelected(int _iLast = -1);
	virtual void SelectItem(int _iItem);
	void DeselectItem(int _iItem);
	void DeselectAll();
	int ToggleItem(int _iItem);
	void AdjustItemFocus();
	virtual int GetItemFocus();

	virtual void OnIncrementItemFocus();
	virtual void OnDecrementItemFocus();

	virtual void OnPaintList(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual void OnPaintItem(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client, int _iItem);
	virtual void OnTouchItem(int _iItem, int _bSelected);
	virtual void OnAction(int _iItem);
	virtual void OnMakeVisible(int _iItem);

	friend class CMWnd_ListClient;
};

typedef TPtr<CMWnd_List2> spCMWnd_List2;


#endif // _INC_MWINCTRLMOD
