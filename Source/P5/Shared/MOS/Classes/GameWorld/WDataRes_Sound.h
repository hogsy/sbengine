#ifndef __WDATARES_SOUND_H
#define __WDATARES_SOUND_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Misc sound related resources and helper-classes
					
	Author:			Jens Andersson
					
	Copyright:		Starbreeze Studios 2003-2006
					
	Contents:		CWRes_Sound
					CWRes_Wave
					CDialogueToken
					CDialogueInstance
					CWRes_Dialogue

	History:		
		020811:		Created File
\*____________________________________________________________________________________________*/

#include "WData.h"
#include "WDataRes_Core.h"
#include "../MOS/Classes/Win/MWinGrph.h"
#include "../Win/MWindows.h"


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CWRes_Sound
					
	Comments:		The basic sound resource
\*____________________________________________________________________*/

class CWRes_Sound : public CWResource
{
	MRTC_DECLARE;

	spCWaveContainer_Plain m_spWC;
	CWorldData* m_pWData;
	int16 m_iSFX;			// Index in that WC.

public:
	CWRes_Sound();
	virtual CSC_SFXDesc* GetSound();
	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
	virtual void OnLoad();
	virtual void OnPrecache(class CXR_Engine* _pEngine);

};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CWRes_Sound
					
	Comments:		The basic sound resource
\*____________________________________________________________________*/

class CWRes_Wave : public CWResource
{
	MRTC_DECLARE;

	static int32 m_CurrentID;

	//int16 m_iWC;			// Local WaveContainer nr.
	//int16 m_WaveID;
	//int16 m_iSFX;			// Index in that WC.
	spCWaveContainer_Plain m_spWC;
	CWorldData *m_pWData;
	CSC_SFXDesc m_SoundDesc;

public:
	CWRes_Wave();
	virtual CSC_SFXDesc* GetSound();
	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
	virtual void OnLoad();
	virtual void OnPrecache(class CXR_Engine* _pEngine);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Dialogue Stuff
|__________________________________________________________________________________________________
\*************************************************************************************************/
#define DEFAULT_SUBTITLEDURATION 3.5f
#define DEFAULT_SIZE 14

enum
{
	SUBTITLE_PLACEMENT_BOTTOM = 0,
	SUBTITLE_PLACEMENT_MID,
	SUBTITLE_PLACEMENT_TOP,

	SUBTITLE_TYPE_IDLE = 0,		// Default off
	SUBTITLE_TYPE_INTERACTIVE,	// Default on
	SUBTITLE_TYPE_AI,			// Default off
	SUBTITLE_TYPE_CUTSCENE,		// Default on
	SUBTITLE_TYPE_CUTSCENEKEEPENDANIM,
	SUBTITLE_TYPE_MASK = 0x7,

	
	CAMERA_PIVOT_NORMAL = 0,
	CAMERA_PIVOT_LEFT_NEAR,
	CAMERA_PIVOT_RIGHT_NEAR,
	CAMERA_PIVOT_LEFT_FAR,
	CAMERA_PIVOT_RIGHT_FAR,
	CAMERA_PIVOT_ATTACHPOINT0,
	CAMERA_PIVOT_ATTACHPOINT1,
	CAMERA_PIVOT_ATTACHPOINT2,
	CAMERA_PIVOT_ATTACHPOINT3,
	CAMERA_PIVOT_ATTACHPOINT4,
	CAMERA_PIVOT_ATTACHPOINT5,
	CAMERA_PIVOT_ATTACHPOINT6,
	CAMERA_PIVOT_ATTACHPOINT7,

	DIALOGUEFLAGS_PAUSED = 1,
	DIALOGUEFLAGS_FROMLINK = 2,
	DIALOGUEFLAGS_FORCE2D = 4,
	DIALOGUEFLAGS_PLAYERFORCE2D = 8,

	PRIORITY_IDLE = 0x60,
	PRIORITY_DEFAULT = 0xA0,
};

// Used by dialogue code
#define IS_VALID_ITEMHASH(x) (((x) != 0) && ((x) != MHASH1('0')))


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				The Token class that controls a dialogue.
						
	Comments:			Who ever has control over the Token has
						control over the dialogue. The Token is
						refreshed with the current DialogueItem to
						check for new events in the dialogue such as
						instructions to change the holder of the
						token (a link).

\*____________________________________________________________________*/

class CDialogueToken
{
public:
	int m_iOwner;
	int m_iLastSpeaker;

	int m_Delay;
	CMTime m_StartGameTime;

	int m_CameraMode;
	int m_CameraModeParameter;
	int16 m_CameraTelephoneScript;
	CStr m_Camera_Scripted;

	CVec3Dfp32 m_CameraSpeakerOffset;
	CVec3Dfp32 m_CameraListenerOffset;
	fp32 m_FOV;
	fp32 m_dFOV;
	fp32 m_CameraShakeVal;
	fp32 m_CameraShakeSpeed;

	CDialogueToken()
	{
		Clear();
	}

	void Clear()
	{
		m_iOwner = 0;

		m_Delay = 0;
		m_StartGameTime = CMTime();
		m_iLastSpeaker = 0;

		m_CameraMode = 0;
		m_CameraModeParameter = 0;
		
		m_FOV = 35.0f;
		m_dFOV = 0;
		m_CameraShakeVal = 0.15f;
		m_CameraShakeSpeed = 0.55f;

		m_CameraSpeakerOffset = CVec3Dfp32(0.0f, 0.0f, 0.0f);
		m_CameraListenerOffset = CVec3Dfp32(0.0f, 0.0f, 0.0f);
	}
	
	bool IsValid()
	{
		return (m_CameraMode >= 0) && (m_CameraMode < 16);
	}
};

	
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Instance class for particapant of a dialogue

	Comments:			Contains information about subtitles and
						dialogue driven animation.
	
\*____________________________________________________________________*/
class CDialogueInstance
{
public:
	uint32 m_DialogueItemHash;
	CMTime m_StartTime;
	CStr m_Subtitle;

	CStr m_Choice;
	int16 m_Flags;
	int16 m_SubtitleFlags;
	uint8 m_iSubtitlePlacement;
	uint8 m_SubtitleSize;
	uint8 m_Priority;
	int32 m_SubtitleColor;
	fp32 m_SampleLength;

	M_FORCEINLINE bool IsValid() const { return IS_VALID_ITEMHASH(m_DialogueItemHash); }

	// Can't seem to get these into the .cpp file without getting unresolved extrnal,
	// even if they are made virtual...
	void Reset(const CMTime& _Time, uint32 _ItemHash, fp32 _SampleLength, int _Flags)
	{
		m_SampleLength = _SampleLength;
		m_DialogueItemHash = _ItemHash;
		m_StartTime = _Time;
		m_Flags = _Flags;

		m_iSubtitlePlacement = SUBTITLE_PLACEMENT_BOTTOM;
		m_SubtitleSize = DEFAULT_SIZE;
		m_SubtitleColor = 0x00FFFFFF;
		m_Subtitle = "";
		m_Choice = "";
		m_Priority = PRIORITY_DEFAULT;
	}

	// Can't seem to get these into the .cpp file without getting unresolved extrnal,
	// even if they are made virtual...
	void Render(CRC_Util2D *_pUtil2D, CRC_Font *_pFont, fp32 _Fade, int _Param)
	{
		if(m_Subtitle == "")
			return;

		_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

		CStr Text = CStrF("§Z%i", m_SubtitleSize) + m_Subtitle;

		char moo = '§';	// GCC workaround
		if(m_Subtitle != "" && m_Subtitle.Ansi().Str()[0] == moo)
			m_Subtitle += CStrF("§p0%i§pq", _Param);

		int y = 0;
		switch(m_iSubtitlePlacement)
		{
		case SUBTITLE_PLACEMENT_BOTTOM: y = 376; break;
		case SUBTITLE_PLACEMENT_MID: y = 221; break;
		case SUBTITLE_PLACEMENT_TOP: y = 10; break;
		}

		CClipRect Clip(0, 0, 640, 480);
		int Alpha = RoundToInt(_Fade * 255) << 24;
		int Col = m_SubtitleColor | Alpha;
		int ShadowCol = Alpha;

		int Style = WSTYLE_TEXT_WORDWRAP | WSTYLE_TEXT_CENTER;

		int Border = 120;
		int XStart = Border / 2;
		_pUtil2D->Text_DrawFormatted(Clip, _pFont, Text, XStart, y, Style, ShadowCol, ShadowCol, ShadowCol, Clip.GetWidth()-Border, Clip.GetHeight(), true);
		_pUtil2D->Text_DrawFormatted(Clip, _pFont, Text, XStart, y, Style, Col, Col, ShadowCol, Clip.GetWidth()-Border, Clip.GetHeight(), false);
	}
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Root class that contains multiple
						"DialogueItems"
						
	Comments:			DialogueItem struct looks something like this
						
						-------------- Base structure
						SoundIndex				2 bytes
						nEvents					2 bytes
						[Events]				nEvents * x bytes
						
						-------------- Generic event header
						EventSize				1 byte
						EventFlags+EventType	1 byte (3 bit Flags + 5 bit Type)
						Time					2 bytes (Fixed point 8:8)

						-------------- Subtitle event structure
						Event header			4 bytes
						Flags					2 bytes
						Subtitle				n bytes (String)

						-------------- Link event structure
						Event header			4 bytes
						Link					n bytes (String)

						-------------- Camera event structure
						Event header			4 bytes
						FOV						2 bytes (Fixed point 8/8)
						FOV DElta				2 bytes (Fixed point 8/8)
						Speaker X offset		2 bytes (Fixed point 8/8)
						Speaker Y offset		2 bytes (Fixed point 8/8)
						Speaker Z offset		2 bytes (Fixed point 8/8)
						Listener X offset		2 bytes (Fixed point 8/8)
						Listener Y offset		2 bytes (Fixed point 8/8)
						Listener Z offset		2 bytes (Fixed point 8/8)
						Camera mode parameter	2 bytes (Fixed point 8/8)
						Camera mode				1 byte

						-------------- Animation event structure
						Event header			4 bytes
						Animationindex			2 bytes
						Bonebase				1 byte

						-------------- Impulse event structure
						Event header			4 bytes
						ImpulseNr				1 bytes

						-------------- Custom animation sequence (used to override vocap animations)
						Event header			4 bytes
						Sequence name hash		4 bytes

\*____________________________________________________________________*/


#define DIALOGUE_COMPILED_VERSION 0x101
class CWRes_Dialogue : public CWResource
{
	MRTC_DECLARE;

private:
	CWorldData* m_pWData;
	
	class SDialogueItem
	{
	public:
		uint32 m_Hash;
		uint32 m_StartPos;
		uint16 m_Size;
		void SwapLE()
		{
			::SwapLE(m_Hash);
			::SwapLE(m_StartPos);
			::SwapLE(m_Size);
		}
	};
	TThinArray<SDialogueItem> m_lDialogueIndexes;
	TThinArray<char> m_lDialogueData;
	bool m_bTaggedAnimations;

	//Static hash-table for frequently used values
	static uint32 ms_lHashTable[128];

public:
	enum
	{
		ITEM_APPROACH = 0,		// Used when approaching a character
		ITEM_APPROACH_SCARED,	// Used when approaching a scared character
		ITEM_THREATEN,			// Used when threatening a character in dialogue
		ITEM_IGNORE,			// Used when looking away / walking away from character in dialogue
		ITEM_TIMEOUT,
		ITEM_EXIT,				// Used when exiting dialogue (leaving character)
		NUMDIALOGUEITEMS,

		LIGHT_ON = 0,
		LIGHT_FADE,
		LIGHT_OFF,		

		EVENTTYPE_SUBTITLE = 0,
		EVENTTYPE_CHOICE,
		EVENTTYPE_LINK,
		EVENTTYPE_RANDOMLINK,
		EVENTTYPE_CAMERA,
		EVENTTYPE_MOVETOKEN,
		EVENTTYPE_SETANIMPROPERTY,
		EVENTTYPE_SETANIMPROPERTIES,
		EVENTTYPE_IMPULSE,
		EVENTTYPE_MESSAGE,
		EVENTTYPE_PRIORITY,
		EVENTTYPE_LISTENER,
		EVENTTYPE_USERS,
		EVENTTYPE_MOVINGHOLD,
		EVENTTYPE_SPECIALVOICE,
		EVENTTYPE_SETITEM_APPROACH,			// 
		EVENTTYPE_SETITEM_APPROACH_SCARED,	//
		EVENTTYPE_SETITEM_THREATEN,			// These must be grouped together
		EVENTTYPE_SETITEM_IGNORE,			//
		EVENTTYPE_SETITEM_TIMEOUT,			//
		EVENTTYPE_SETITEM_EXIT,				//
		EVENTTYPE_LIGHT,
		EVENTTYPE_NOCAMERA,
		EVENTTYPE_SETHOLD,
		EVENTTYPE_CUSTOM_ANIM,				//
		EVENTTYPE_ANIMFLAGS,				// #25

		EVENTTYPE_MAX,	// max 32!

		EVENTFLAGS_SAMPLELENGTHRELATIVE = 1,	// max 3 flags!
	};

	class CDialoguePropertyChange
	{
	public:
		int32 m_Property;
		fp32 m_Value;
	};

	class CRefreshRes
	{
	public:
		CRefreshRes()
		{
			m_Events = 0;
		}
		
		uint32 m_Events;
		int16 m_Impulse;
		const char *m_pLink;
		const char *m_pItems[NUMDIALOGUEITEMS];
		const char *m_pMessage;
		uint8 m_iToken;
		const char *m_pAction;
		uint32 m_SetHoldAnim;
		fp32 m_SetHoldBegin;
		fp32 m_SetHoldEnd;
		TThinArray<CDialoguePropertyChange> m_lPropertyChanges;
	};

	fp32 m_SubtitleRange;
	fp32 m_AttnMaxDist;
	fp32 m_AttnMinDist;
	bool m_bSpecialVoice;
	
	CWRes_Dialogue();
	
	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
	void CreateFromRegistry(CRegistry *_pReg, const char* _pName, CMapData* _pMapData, bool _bCompile = false);

	dllvirtual void PreCache(CMapData *_pMapData);
	dllvirtual void TagAnimations(CMapData* _pMapData, class CWAG2I *_pWAGI, class CWAG2I_Context *_pContext);
	dllvirtual int GetNumItems() const;

	dllvirtual uint16	GetHashPosition(uint32 _Hash) const;
	dllvirtual const char *GetDialogueItem(int _Index) const;
	dllvirtual const char *GetHashDialogueItem(uint32 _Hash) const;
	dllvirtual const char *GetHashDialogueItem(const char * _pName) const;
	const char * GetDialogueItemAtPos(uint _Pos) const
	{
		return (_Pos < m_lDialogueIndexes.Len()) ? 
			m_lDialogueData.GetBasePtr() + m_lDialogueIndexes[_Pos].m_StartPos : NULL;
	}

	dllvirtual bool HasLink(int _iDialogue) const;
	dllvirtual bool HasLink_Hash(uint32 _iDialogue) const;
	dllvirtual bool IsQuickSound(int _iDialogue) const;
	dllvirtual bool IsQuickSound_Hash(uint32 _Dialogue) const;
	dllvirtual int GetSoundIndex(int _iItem, CMapData *_pMapData, uint16 *_piType = NULL) const;
	dllvirtual int GetSoundIndex_Hash(uint32 _ItemHash, CMapData *_pMapData, uint16 *_piType = NULL) const;
	dllvirtual fp32 GetSampleLength(int _iItem) const; // In seconds
	dllvirtual fp32 GetSampleLength_Hash(uint32 _ItemHash) const; // In seconds
	dllvirtual uint8 GetLightType(uint32 _iDialogue) const;
	dllvirtual CFStr Parse(CRegistry *_pReg, CMapData *_pMapData,TArray<char> &_lDialogueItem, bool _bCompile = false);
	dllvirtual const char* FindEvent(int _ItemHash, int _Event, fp32 *_pTime = NULL, int *_pFlags = NULL) const;
	dllvirtual const char* FindEvent_Hash(uint32 _iHashItem, int _Event, fp32* _pTime = NULL, int *_pFlags = NULL) const;
	dllvirtual int GetPriority(int _iDialogue) const;
	dllvirtual int GetPriority_Hash(uint32 _ItemHash) const;

	dllvirtual int GetMovingHold(uint32 _ItemHash) const;
	dllvirtual uint32 GetCustomAnim(uint32 _ItemHash) const;
	dllvirtual uint16 GetDialogueAnimFlags(uint32 _ItemHash) const;

	/*dllvirtual bool HasLink(int _iDialogue);
	dllvirtual bool IsQuickSound(int _iDialogue);
	dllvirtual int GetSoundIndex(int _iItem, CMapData *_pMapData, int *_piType = NULL);
	dllvirtual fp32 GetSampleLength(int _iItem); // In seconds
	dllvirtual CFStr Parse(CRegistry *_pReg, CMapData *_pMapData,TArray<char> &_lDialogueItem, bool _bCompile = false);
	dllvirtual const char* FindEvent(int _iItem, int _Event, fp32 *_pTime = NULL, int *_pFlags = NULL);
	dllvirtual int GetPriority(int _iDialogue);*/

	dllvirtual bool HasLinkBuf(const char* _pBuf) const;
	dllvirtual int GetSoundIndexBuf(const char * _pBuf, CMapData *_pMapData, uint16 *_piType = NULL) const;
	dllvirtual fp32 GetSampleLengthBuf(const char * _pBuf) const;
	dllvirtual const char * FindEventBuf(const char * _pBuf, int _Event, fp32 *_pTime = NULL, int * _pFlags = NULL) const;
	dllvirtual int GetPriorityBuf(const char * _pBuf) const;

	int AddHeader(int _Flags, int _Type, fp32 _Time, int8 *_pBuf, int &_Pos);
	void AddString(const char *_pSt, int8 *_pBuf, int &_Pos);
	void AddUnicodeString(const wchar *_pSt, int8 *_pBuf, int &_Pos);
	void AddTailer(int _OrgPos, int8 *_pBuf, int &_Pos);

	dllvirtual bool Refresh(const CMTime& _Time, fp32 _dTime, CDialogueInstance &_Instance, CDialogueToken *_pToken = NULL, CRefreshRes *_pRes = NULL);

	dllvirtual uint32 IntToHash(int _iIndex) const
	{
		uint32 Hash;
		if(uint(_iIndex) < 128)
		{
			Hash = ms_lHashTable[_iIndex];
		}
		else
		{
			Hash = StringToHash(CFStrF("%d", _iIndex).Str());
		} 
		return Hash;
	}

#ifndef PLATFORM_CONSOLE
	void Write(CDataFile* _pDFile);
	void Write_XCD(const char* _pFileName);
#endif

	void Read(CDataFile* _pDFile);
	void Read_XCD(const char* _pFileName, CMapData* _pMapData);
	void UpdateSoundIndexes(CMapData* _pMapData);

	// Is needed for the content compile. It will be empty in RTM though
	TArray<CStr> m_lSoundNames;


#ifndef M_RTM
	CStr m_Base;
	spCRegistry m_spParent;

	virtual CStr Ogr_GetDescription(int _iItem);
	virtual CStr Ogr_GetDescription_Hash(uint32 _iItem);
	virtual CStr Ogr_GetSound(int _iItem);
	virtual CStr Ogr_GetSound_Hash(uint32 _iItem);
	virtual CStr Ogr_GetFirstSound();
	virtual CStr Ogr_Link(int _iItem, fp32 &_Duration);
	virtual CStr Ogr_Link_Hash(uint32 _iItem, fp32 &_Duration);
	virtual void Ogr_Recreate(CRegistry *_pReg, CMapData *_pMapData);
	virtual void Ogr_GetEvents(int _iItem, TArray<fp32> &_lDurations);
	virtual void Ogr_GetEvents_Hash(uint32 _iItem, TArray<fp32> &_lDurations);
	virtual bool Ogr_GetDialogueItemHashAtPos(uint _Pos, uint32 &oHashValue);	// Returns true or false if the operation was successful or not.
#endif
};

#endif
