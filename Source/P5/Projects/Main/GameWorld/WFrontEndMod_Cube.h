#ifndef _INC_WFRONTENDMOD_CUBE
#define _INC_WFRONTENDMOD_CUBE

#include "../../../Shared/MOS/MSystem/Raster/MRCCore.h" 
#include "../../../Shared/MOS/XR/XRShader.h"
#include "../../../Shared/MOS/XR/XRVBContainer.h"
#include "../../../Shared/MOS/XR/XRVBContext.h"
#ifdef PLATFORM_XBOX
#include "../../../Shared/MOS/Classes/Render/MRenderVPGen.h"
#endif

#include "../../../Shared/MOS/XRModels/Model_BSP/WBSPModel.h"
#include "../../../Shared/MOS/XRModels/Model_BSP/WBSPDef.h"
//#include "../../../Shared/MOS/XRModels/Model_Multi/WModel_Multi.h"

// for vocap stuff

#include "../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_VoCap.h"
#include "../../../Shared/MOS/Classes/GameWorld/WDataRes_Sound.h"
#include "../../../Shared/MOS/Classes/GameWorld/WAnimGraph2Instance/WAG2I.h"
#include "../../../Shared/MOS/Classes/GameWorld/WAnimGraph2Instance/WAG2_ClientData.h"

#include "../../../Shared/MOS/XR/XRCloth.h"
#include "../../../Shared/MOS/Classes/GameWorld/WDataRes_FacialSetup.h"
#include "../../../Shared/mos/MSystem/Sound/LipSync/LipSync.h"

#define NUM_OF_TENTACLES 6

class CWFrontEnd_Mod;
//
//
//
class CSequence
{
public:
	class CTrack
	{
	public:
		class CKey
		{
		public:
			fp32 m_Time;
			fp32 m_Value;
			uint8 m_Flag;

			void Set(fp32 _Time, fp32 _Value) { m_Time = _Time; m_Value = _Value; }
		};

		CKey *m_pKeys;
		int32 m_NumKeys;

		CTrack();
		~CTrack();
		void CreateKeys(int32 _NumEntries);
		void Clear();
		fp32 GetValue(fp32 _Time);
		void SetValue(int32 _Index, fp32 _Time, fp32 _Value) { m_pKeys[_Index].Set(_Time, _Value); }
	};

	CTrack *m_pTracks;
	int32 m_NumTracks;
	void CreateTracks(int32 _NumTracks);

	fp32 GetValue(int32 _Track, fp32 _Time) { return m_pTracks[_Track].GetValue(_Time); };
	CVec3Dfp32 GetVector(int32 _StartTrack, fp32 _Time);
	void SetVector(int32 _StartTrack, int32 _Index, fp32 _Time, CVec3Dfp32 _Vector);

	void CreateFromSeqSingleFrame(CSequence &_rSeq, fp32 _Time);
	void CopyLastKey(CSequence &_rSeq);
	void Copy(CSequence &_rSeq);

	CSequence();
	~CSequence();
};


class CXR_Model_GUIBackground : public CXR_Model
{
public:
	CXR_Model_GUIBackground();

	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0);
	virtual void RenderFinalGlow(CXR_Engine* _pEngine, fp32 _GlowTime, CRenderContext* _pRC, CXR_VBManager* _pVBM, fp32 _StartPrio, int _TextureIDFinal, int _TextureIDTmp, fp32 _RadialVal);
};


//
//
//
class CCube : public CXR_EngineClient
{
	// Texture id for engine post process texture and container for gui process
	int						m_TextureID_ProjectionTexture;
	int						m_TextureID_ScreencapToProcess;
	int						m_TextureID_FinalImage;
	int						m_TextureID_PostProcessTemp;
	int						m_iTentacleImage;
	TPtr<CTextureContainer>	m_spGUICubeMaps;
	CXR_Model_GUIBackground	m_BG_GUI;

public:
	CTextureContainer_Screen* GetGUIProcessContainerCM()
	{
		CTextureContainer_Screen* pGUIProcess = safe_cast<CTextureContainer_Screen>((CTextureContainer*)m_spGUICubeMaps);
		return pGUIProcess;
	}

	static const int ms_StateTime[];

	enum
	{
		CUBE_BLOCKS_PER_SECOND=10000,
		CUBE_RES=20,
		CUBE_RES_DIV=2,
		CUBE_SPEED=350, // in percent

		CUBE_MAX_ACTIVE_ELEMENTS=350, // 170 for xbox

		STATE_WAITING=0,
		STATE_DONE=6,

		BLOCK_NONE=0,
		BLOCK_NORMAL=1,
		BLOCK_DONTDRAW=2,

		FLAG_BLOCKED=1,
		FLAG_FASTSWITCH=2, //


		TRACK_OFFSET_X=0,
		TRACK_OFFSET_Y,
		TRACK_OFFSET_Z,

		TRACK_ROT_X,
		TRACK_ROT_Y,
		TRACK_ROT_Z,

		TRACK_ADDLAYER_R,
		TRACK_ADDLAYER_G,
		TRACK_ADDLAYER_B,
		TRACK_ADDLAYER_INTENS,

		TRACK_LIGHT_INTENS,
		TRACK_LIGHT_RANGE,

		TRACK_MOVEBLOCK,
		TRACK_MAPBLOCK,
		TRACK_TIMESCALE,

		TRACK_BGINTENS,

		TRACK_VIEWPORT,

		TRACK_KEYLOCK,

		TRACK_DROPGUI,

		TRACK_NUM,
	};

	//
	//
	//
	class CActiveElement
	{
	public:
		int8 m_DirX:4;
		int8 m_DirY:4;
		int8 m_Stage:4;
		int8 m_WantedNumber:4;

		CCellInfo m_WantedCell;

		fp32 m_Scale;
		fp32 m_Time;

		fp32 m_CurrentDepth;
		fp32 m_WantedDepth;

		DLinkD_Link(CActiveElement, m_Link);

		//
		//		int16 m_iNext;
		//		int16 m_iPrev;
	};

	//
	class CElement
	{
	public:
		uint8 m_Flags;
		int16 m_iActive; // index in CCube::m_aActiveData

		CElement()
		{
			m_Flags = 0;
			m_iActive = -1;
		}

		CCellInfo m_Cell;
	};

	//
	class CSide
	{
	public:
		// elements
		CCube *m_pCube;
		CElement m_aElements[CUBE_RES][CUBE_RES];

		CMat4Dfp32 m_Transform;
		CMat4Dfp32 m_CameraMatrix;
		CMat4Dfp32 m_CameraMatrixDrawZ;
		CMat4Dfp32 m_InvCameraMatrix;

		CVec4Dfp32 m_aSecondaryTextureTransform[4];
		int32 m_iSecondaryID; // -1 = disableds
		int32 m_BlendMode;

		void SetSecondary(int32 _iSomething, int32 _iBlend);
		void SetSecondaryArea(int32 _x, int32 _y, int32 _w, int32 _h);

		void Update();

		void SetMap(CCellInfo *_pMap, bool _ForceExact, bool _QuickChange=false);
		//void SetMapOld(uint16 *_pMap, bool _ForceExact);
		void SetDepthMap(fp32 *_pMap);
		void Reset(); // resets all elements
		void Clear(); // transforms all elemenets to "base"
		int32 ActivateElement(int32 x, int32 y, int32 dx=0, int32 dy=0); // returns an index to the m_aActiveData, -1 on failure
		void UnblockElement(int32 x, int32 y);
		void DeactivateElement(int32 x, int32 y);
	};

	void OnPrecache(CXR_Engine* _pEngine, CWorld_Server *_pServer);
	void PrecacheSound(CSC_SFXDesc *_pSound, CWaveContext *_pWC);
	void SetGUIProcessProperties(CTextureContainer_Screen* _pGUIProcess);

	// active elements and functions to handle them
	CActiveElement m_aActiveData[CUBE_MAX_ACTIVE_ELEMENTS];

	DLinkD_List(CActiveElement, m_Link) m_ActiveList;
	DLinkD_List(CActiveElement, m_Link) m_InactiveActiveList;

	int32 m_NumActiveElements;

	void ResetActiveElementData(); // resets m_aActiveData/m_NumActive, m_aInactiveList
	int16 AllocateActiveElement(); // Call CSide::ActivateElement instead
	void FreeActiveElement(int16 _Element); // Call CSide::DeactivateElement instead 

	// vertex and indices buffers

	enum
	{
#if defined(PLATFORM_XBOX)
		ECubeMaxMPEntries = (188 - CRC_VPFormat::EMatrixPalette) >> 1,
#else
		ECubeMaxMPEntries = (96 - 30) >> 1,
#endif
		ECubeMinFreeMPEntries = 3,
		ECubeMesh_Front = 0,
		ECubeMesh_SplitFront,
		ECubeMesh_ShadowFront,
		ECubeMesh_Cube,
		ECubeMesh_SplitCube,
		ECubeMesh_ShadowCube,
		ECubeMesh_Sides, // For Secondary texture
		ECubeMesh_Num
	};

	class CVertexBuffers : public CXR_VBContainer
	{
	public:
		enum 
		{
			ENumIds = ECubeMesh_Num
		};
		int32 m_IDs[ENumIds];

		static fp32 *ms_pVertices[ENumIds];
		static fp32 *ms_pNormals[ENumIds];
		static fp32 *ms_pDiffuseTexture[ENumIds];
		static uint16 *ms_pIndices[ENumIds];
		static uint8  *ms_pTextureMatrixPalette[ENumIds];
		static uint8  *ms_pTextureScaleZ[ENumIds];
		static int ms_nVertices[ENumIds];
		static int ms_nIndices[ENumIds];

		void Precache()
		{
			for (int i = 0; i < ENumIds; ++i)
			{
				m_pVBCtx->VB_SetFlags(m_IDs[i], m_pVBCtx->VB_GetFlags(m_IDs[i]) | CXR_VBFLAGS_PRECACHE);
			}
		}

		CVertexBuffers();

		~CVertexBuffers()
		{
			for (int i = 0; i < ENumIds; ++i)
			{
				m_pVBCtx->FreeID(m_IDs[i]);
			}
		}

		virtual int GetNumLocal()
		{
			return ENumIds;
		}

		virtual int GetID(int _iLocal)
		{
			return m_IDs[_iLocal];
		}

		void MakeDirty(CRenderContext *_pRC)
		{
			for (int i = 0; i < ENumIds; ++i)
			{
				_pRC->VB_GetVBIDInfo()[m_IDs[i]].m_Fresh &= ~1;
			}			
		}

		TArray<uint8> m_TempData;

		virtual void Get(int _iLocal, CRC_BuildVertexBuffer& _VB, int _Flags);

		virtual void Release(int _iLocal)
		{
			m_TempData.Clear();
		}

	};

	CVertexBuffers m_VertexBuffers;

	class CMatrixPaletteEntry
	{
	public:
		fp32 m_TranslateX;
		fp32 m_TranslateY;
		fp32 m_TranslateZ;
		fp32 m_ScaleZ;
		fp32 m_TextureTranslateX0;
		fp32 m_TextureTranslateY0;
		fp32 m_TextureTranslateX1;
		fp32 m_TextureTranslateY1;
	};

	class CRenderBatch
	{
	public:
		CMatrixPaletteEntry *m_pMatrixPalette; // Front, shadow and z share the same MP
		int m_iMatrixPalette;
		int m_nFreeNeeded;
		int AddEntry(CMatrixPaletteEntry &_Entry);
		void BeginBatch(CXR_VBManager *_pVBM);

		CRenderBatch()
		{
			m_pMatrixPalette = NULL; 
			m_iMatrixPalette = ECubeMaxMPEntries;
			m_nFreeNeeded = 3;
		}

		bool Full()
		{
			return ((m_iMatrixPalette) + m_nFreeNeeded) > ECubeMaxMPEntries;
		}

		void FillRemaining()
		{
			M_ASSERT(m_iMatrixPalette <= ECubeMaxMPEntries, "Overstep");
			for (int i = m_iMatrixPalette; i < ECubeMaxMPEntries; ++i)
			{
				m_pMatrixPalette[i].m_TranslateX = 1000000000.0f;
				m_pMatrixPalette[i].m_TranslateY = 1000000000.0f;
				m_pMatrixPalette[i].m_TranslateZ = 1000000000.0f;
				m_pMatrixPalette[i].m_ScaleZ = 0.0f;
			}
			m_iMatrixPalette = ECubeMaxMPEntries;
		}
	};

	enum ERenderBatch
	{
		ERenderBatch_Cube = 0,
		ERenderBatch_SplitCube,
		ERenderBatch_Face,
		ERenderBatch_SplitFace,
		ERenderBatch_SecondaryCube,
		ERenderBatch_SecondaryFace,
		ERenderBatch_HugeCube,
		ERenderBatch_HugeFace,
		ERenderBatch_Max
	};

	CRenderBatch m_RenderBatches[ERenderBatch_Max];
	int m_bBufferFull;

	CRC_Attributes m_Attrib_Shadow0;
	CRC_Attributes m_Attrib_Shadow1;
	CRC_Attributes m_Attrib_Mul;

	CRC_Attributes m_Attrib_RenderShading;

	//CRC_Attributes m_Attrib_ColorMul;
	CRC_Attributes m_Attrib_ColorAdd;
	CRC_Attributes m_Attrib_ColorSecondary;

	int m_iCurrentSide;

	CRC_Attributes *m_pAttrib_Shadow0;
	CRC_Attributes *m_pAttrib_Shadow1;

	CRC_Attributes *m_pAttrib_Shadow0PolyOffset;
	CRC_Attributes *m_pAttrib_Shadow1PolyOffset;

	CVec3Dfp32 m_LightPosition;

	CRC_Attributes *m_pAttrib_ZBuffer;
	CRC_Attributes *m_pAttrib_ZBufferWholeSide;
	CRC_Attributes *m_pAttrib_Shading;

	CMat4Dfp32 *m_pMatrix_CameraMatrix;

	int m_MulTextureID;

	CXR_Light m_RenderShadingLight;
	CXR_SurfaceShaderParams m_SSP;
	CXR_ShaderParams m_RenderShadingParams;
//	CXR_ShaderParams m_RenderShadingParamsSecondary;	// Never initialized

	CRC_MatrixPalette *AllocMatrixPalette(CXR_VBManager* _pVBM, uint32 _Flags, CMatrixPaletteEntry *_pEntries);
	void UpdateAttribs(CRenderContext *_pRC, CXR_VBManager* _pVBM);
	void UpdateAttribsSideDependant(CRenderContext *_pRC, CXR_VBManager* _pVBM);

	void InitStaticAttribs();

	void StartMPBuild(CXR_VBManager* _pVBM);
	void EndMPBuild(CXR_VBManager* _pVBM, bool _bFlush);

	void RenderShading(CXR_VBManager* _pVBM, CRC_MatrixPalette *_pMP, int _VBID);
	void RenderShadingSecondaryTexture(CXR_VBManager* _pVBM, CRC_MatrixPalette *_pMP, int _VBID);
//	void RenderShadingSecondary(CXR_VBManager* _pVBM, CRC_MatrixPalette *_pMP, int _VBID);
	void RenderZ(CXR_VBManager* _pVBM, CRC_MatrixPalette *_pMP, int _VBID);
	void RenderShadow(CXR_VBManager* _pVBM, CRC_MatrixPalette *_pMP, int _VBID);
	void RenderMul(CXR_VBManager* _pVBM, CRC_MatrixPalette *_pMP, int _VBID, CPixel32 _Color);
	//void RenderColorMul(CRC_MatrixPalette *_pMP, int _VBID);
	void RenderColorAdd(CXR_VBManager* _pVBM, CRC_MatrixPalette *_pMP, int _VBID, CPixel32 _Color);

	// surface and shader
	//uint16 m_iTexture;
	CXW_Surface *m_pSurface;
	CXW_Surface *m_pSurfaceSecondary;
	CXR_Shader m_Shader;
	CXR_Shader m_ShaderSecondary;		// Why are there 2 shaders? What's wrong with the one in the engine

	// times
	fp32 m_FrameTime;
	mutable fp32 m_MoveTime;
	CMTime m_AnimTime;

	// matrices and zoom
	CMat4Dfp32 m_Current;
	CMat4Dfp32 m_Wanted;

	//
	int32 m_CurrentSide;
	int32 m_RenderingSide;

	//
	CSide m_aSides[6];

	CVec2Dfp32 m_aReferencePoints[4];

	// up down left right
	static const int ms_aSideSwapIndex[6][4];

	void ViewSide(int32 _Side, bool _Instant);
	void Move(int32 _Dir);

	CMat4Dfp32 CreateTranslationMatrix(fp32 x, fp32 y, fp32 z);
	void CreateRotationMatrix(fp32 x, fp32 y, fp32 z, CMat4Dfp32 *_pRetMat);

	fp32 m_SpeedModifyer;

	// CSequence
	void UpdateSequence(fp32 _DeltaTime) const;
	void SetSequence(CSequence *_pSequence);
	void SetDefaultSequenceParams() const;
	CSequence m_Seq_normal;
	CSequence m_Seq_moveout;
	CSequence m_Seq_inside;
	CSequence m_Seq_fromgame;
	CSequence m_Seq_backtogame;
	CSequence m_Seq_startup;
	CSequence m_Seq_beginload;

	CSequence m_Seq_removeloading;

	CSequence *m_pCurrentSequence;
	//	struct
	//	{
	// Render parameters (they change in OnPaint)
	mutable fp32 m_SequenceTime;
	mutable int32 m_SequenceOverride;

	// these is updated by the sequence
	mutable CVec3Dfp32 m_Offset;
	mutable CVec3Dfp32 m_RotationOffset;
	mutable bool m_CanMove;
	mutable bool m_CanDoLayout;
	mutable bool m_KeyLock;
	mutable fp32 m_TimeScale;
	mutable fp32 m_LightIntens;
	mutable fp32 m_LightRange;
	mutable fp32 m_BgItens;
	mutable fp32 m_ViewportMorph;

	mutable fp32 m_AddLayer_R;
	mutable fp32 m_AddLayer_G;
	mutable fp32 m_AddLayer_B;
	mutable fp32 m_AddLayer_Intens;
	//	};

	//
	// Settings from the system
	//
	//	CStr m_SurfaceName;
	//	CStr m_MulTextureName;
	CFStr m_SurfaceName;
	CFStr m_MulTextureName;
	int32 m_MulMode;

	CCube();
	~CCube();

	void Reset();
	CMTime m_LastTime;
	void UpdateElements(fp32 _Delta);
	virtual void Update();
	CMat4Dfp32 GetCurrentMatrix();

	enum
	{
		SOUNDEVENT_TENTACLES_MOVING_OUT=0,		// Exiting GUI
		SOUNDEVENT_TENTACLES_MOVING_IN,			// Entering GUI
		SOUNDEVENT_TENTACLES_IDLE,				// Idling sound (tentacles)
		SOUNDEVENT_TENTACLES_CHANGEPAGE,		// Switching page (settings/options/controls/video etc.)
		SOUNDEVENT_TENTACLES_SELECT,			// Selecting hilited item
		SOUNDEVENT_TENTACLES_BACK,				// Pressing back or previous
		SOUNDEVENT_TENTACLES_HILITE_SWAP,		// Switching hilite position (like cursor movement)
		SOUNDEVENT_TENTACLES_SCROLL_UP,			// Scrolling upwards in list box
		SOUNDEVENT_TENTACLES_SCROLL_DOWN,		// Scrolling downwards in list box
		SOUNDEVENT_TENTACLES_FLIP_CTRL,			// Toggling button/scrollbar/radio buttons etc.
		SOUNDEVENT_TENTACLES_TAB,				// Map/Items/Darkness menu

		//SOUNDEVENT_LIGHT_FLICKER_1,
		//SOUNDEVENT_LIGHT_FLICKER_2,
		//SOUNDEVENT_LIGHT_FLICKER_BACKONLINE,

		SOUNDEVENT_OLD,
		SOUNDEVENT_DOWN = SOUNDEVENT_OLD,
		SOUNDEVENT_UP = SOUNDEVENT_OLD,
		SOUNDEVENT_ROTATESLOW = SOUNDEVENT_OLD,
		SOUNDEVENT_ROTATEFAST = SOUNDEVENT_OLD,

		SOUNDEVENT_MOVEDOWN = SOUNDEVENT_OLD,
		SOUNDEVENT_MOVEUP = SOUNDEVENT_OLD,
		SOUNDEVENT_SLIDELEFT = SOUNDEVENT_OLD,
		SOUNDEVENT_SLIDERIGHT = SOUNDEVENT_OLD,
		SOUNDEVENT_SELECT = SOUNDEVENT_OLD,

		NUMSOUNDEVENTS,
	};

	int16 m_aSounds[NUMSOUNDEVENTS];
	fp32 m_aEventTimes[NUMSOUNDEVENTS];

	enum
	{
		GUI_FLAGS_PAUSEDGAME			=				M_Bit(0),
		GUI_FLAGS_BACKGROUNDPROCESSDONE	=				M_Bit(1),
		GUI_FLAGS_RUNTENTACLEANIMATION  =				M_Bit(2),
		GUI_FLAGS_VALUESINITIATED		=				M_Bit(3),
		GUI_FLAGS_CAMERAMOVINGRIGHT		=				M_Bit(4),
		GUI_FLAGS_CAMERAMOVINGUP		=				M_Bit(5),
		GUI_FLAGS_NEEDONEFRAMEFAKEBG	=				M_Bit(6),
		GUI_FLAGS_RENDERSAFEBORDES		=				M_Bit(7),
		GUI_FLAGS_FLIPUV				=				M_Bit(8),
		GUI_FLAGS_EXITINGGUI			=				M_Bit(9),
		GUI_FLAGS_EXITCOMPLETE			=				M_Bit(10),
		GUI_FLAGS_PROJECTIONMOVINGIN	=				M_Bit(11),
		GUI_FLAGS_PROJECTIONMOVINGOUT	=				M_Bit(12),
		GUI_FLAGS_FLICKERHALFTHISFRAME	=				M_Bit(13),
		GUI_FLAGS_SOUNDINITIALIZED		=				M_Bit(14),
		GUI_FLAGS_DISABLEPROJECTIONANIM	=				M_Bit(15),
		GUI_FLAGS_ISJOURNAL				=				M_Bit(16),
		GUI_FLAGS_DONTPROJECT			=				M_Bit(17),
		GUI_FLAGS_DISABLENEWGUI			=				M_Bit(18),
		GUI_FLAGS_LOADINGSCENERUNNING	=				M_Bit(19),
		GUI_FADE_IN_IN_PROGRESS			=				M_Bit(20),
		GUI_FADE_OUT_IN_PROGRESS		=				M_Bit(21),
		GUI_FLAGS_LOADINGSCENE_RESETTED	=				M_Bit(22),
		GUI_FLAGS_READYTOSKIP_LS		=				M_Bit(23),
		GUI_FLAGS_READYTOSTART_LS		=				M_Bit(24),
		GUI_FLAGS_DISABLELOADINGMONOLOG	=				M_Bit(25),
		GUI_FLAGS_WAIT_FOR_SCREENGRAB	=				M_Bit(26),
		GUI_FLAGS_SCREENGRAB_OK			=				M_Bit(27),
		GUI_FLAGS_ISWIDESCREEN			=				M_Bit(28),
		GUI_FLAGS_SETFIRSTTEMPLATE		=				M_Bit(29),
		GUI_FLAGS_DEATHSEQUENCE			=				M_Bit(30),
		GUI_FLAGS_DISABLE_TENTACLES		=				M_Bit(31),

		TENTACLE_TEMPLATE_DEFAULT = 0,	// SS
		TENTACLE_TEMPLATE_KNOT,			// MH
		TENTACLE_TEMPLATE_FULLWALL,		// wall
		TENTACLE_TEMPLATE_SIZE,

		DS_FADE_NONE	= 0,
		DS_FADE_OUT		= 1,
		DS_FADE_IN		= 2,
	};

	uint32 m_GUIFlags;
	

	// context
	mutable spCMapData m_spMapData;
	mutable spCSoundContext m_spSoundContext;
	int m_iChannel;

	// functions
	void InitSound(spCMapData _spMapData, spCSoundContext _Context, int _Channel);
	void Init();
	void UpdateSound(fp32 _Delta);
	void PlayEvent(int32 _Event, fp32 _Volume = 1.0f, bool _MuteAll = false, bool _LoopIt = false);
	int PlaySoundFX(int32 _SoundID, const CVec3Dfp32* _pPos = NULL, bool _LoopIt = false, fp32 _Delay = 0.0f);
	int PlaySoundFXEvent(uint _SoundEvent, const CVec3Dfp32* _pPos = NULL, bool _LoopIt = false, fp32 _Delay = 0.0f);
	void StopSoundFX(int _Voice, bool _bNow = true);

	int32 RenderSide(CXR_VBManager* _pVBM, int32 _Side, int32 _StartIndex=0); // returns -1 when done
	inline fp32 EaseIn(fp32 x) { return 1-(fp32)M_Pow(2, -x)*((fp32)M_Sin(x+_PI/2-0.331395f)*2.116f-1); }

	// For P5-prototype
	void Render_P5(CRenderContext* _pRC, CXR_VBManager* _pVBM, fp32 _DeltaTime, bool _bIsIngame,  CXR_Engine *_pOtherEgine, bool _bGAmeIsLoading, bool _bMoveOut);

	// For P6-prototype
	void Render_P6(CRenderContext* _pRC, CXR_VBManager* _pVBM, fp32 _DeltaTime, CTextureContainer_Screen* _pGUIProcess, bool _bIsIngame,  CXR_Engine *_pOtherEgine, bool _bGAmeIsLoading, bool _bMoveOut, CStr _BG, bool _IsJournal = false);

	void ResetGuiProjectionValues();
	bool GetTentaclesMovingOut();
	void SetTentaclesMovingOut();
	bool GetExitComplete();

	void AddCube(CCellInfo &_rCell, fp32 _StartDepth, fp32 _Depth, fp32 _x, fp32 _y, fp32 _ox, fp32 _oy);

	fp32 m_CubeSize; // set before calling AddCube
	bool m_FacingAway; // set to true of the current side rendering is facing away, this will remove some polygons

	M_INLINE int GetProjectionSize() const { return m_CubeProjectionSize; }
	M_INLINE fp32 GetDeltaTime() const { return m_DeltaTime; }
	M_INLINE bool GetOkToGrabScreen() const { return ((m_GUIFlags & GUI_FLAGS_WAIT_FOR_SCREENGRAB) && !(m_GUIFlags & GUI_FLAGS_DISABLENEWGUI));}
	M_INLINE void SetScreenIsGrabbed() { m_GUIFlags |= GUI_FLAGS_SCREENGRAB_OK ;}
	M_INLINE void SetTentacleTemplate(int _Template) {m_CurrentTentacleTemplate = _Template; }
	M_INLINE bool ReadyToDrawMenu() const {return (m_iCurrentTentacleAnim != TENTACLE_ANIM_IN && m_iCurrentTentacleAnim != TENTACLE_ANIM_OUT);}

	virtual void SetLoadingSceneAnimation(CStr _LoadingSceneAnim, CStr MapName, CStr _LoadingBGImage = "");
	virtual void PrecacheDeathScene(CXR_Engine * _pEngine, CWorld_Server *_pServer);
	virtual void SetDeathScene(CStr _Past, CStr _Present, CStr _Future, CStr _Sound);

	void AddProjImage(CRect2Duint16 _Rect, int _TexID);
	M_INLINE CMat4Dfp32 GetCameraMatrix() {return m_GuiCameraMatrix;}

	void RenderFullScreenImage(CRenderContext* _pRC, CXR_Engine *_pEngine, CXR_VBManager* _pVBM, int _TextureID, fp32 _Prio, CPixel32 _PixColor, fp32 _Width = 0.0f, fp32 _Height = 0.0f, bool _FullScreen = true, int _RasterMode = CRC_RASTERMODE_NONE, bool _UVMin01 = false, fp32 _Scale = 1.0f, const CVec2Dfp32 *_pUseTextureSize = NULL, fp32 _VOffset = 0.0f);
	void Render_Quad(CXR_VBManager* _pVBM, CXR_Engine* _pEngine, int _TextureID, const CVec3Dfp32& _LocalPos, fp32 _Width, fp32 _Height, const CMat4Dfp32& _WMat, const CMat4Dfp32& _CameraMat, bool _bNoZ, fp32 _Priority, CVec2Dfp32 _UVMax);
	//void RenderSafeBorders(CRenderContext* _pRC, CXR_VBManager* _pVBM);

private:

	//---------------------------------------------------------------------------
	// tentacle templates/animations
	enum 
	{
		TENTACLE_ANIM_IN = 0,
		TENTACLE_ANIM_DEFAULT_IDLE,
		TENTACLE_ANIM_TRANS_DEFAULT_TO_KNOT,
		TENTACLE_ANIM_KNOT_IDLE,
		TENTACLE_ANIM_TRANS_KNOT_TO_DEFAULT,
		TENTACLE_ANIM_OUT,
		TENTACLE_ANIM_WALL,
	};


	// first step to new gui
	void LoadAllResources();
	void LoadAndSetupGUITentacles();
	void UpdateTentacles(fp32 _DeltaTime, CXR_Engine * _pEngine, CRenderContext* _pRC, CXR_VBManager* _pVBM, int8 _Scroll);
	void UpdateTentacleCameraPos(CMat4Dfp32* _pCameraMat, fp32 _DeltaTime);
	void UpdateTentacleAnimation(CXR_VBManager* _pVBM, fp32 _DeltaTime);
	void RenderTentacles(CXR_Engine *_pEngine, CXR_VBManager *_pVBM, CRenderContext *_pRC);

	bool m_bOutAnimDone;
	uint8 m_CurrentTentacleTemplate;
	uint8 m_LastTentacleTemplate;

	fp32 m_TentacleBlendVal;
	bool m_bForceTentacleBlend;

	int m_iTentacleModel;
	int m_iTentacleAnim;
	CXR_AnimState m_TentacleAnimState;
	fp32 m_TentacleTotalAnimLength;
	fp32 m_TentacleAnimTime;
	int m_iCurrentTentacleAnim;
	int m_iNextTentacleAnim;

	// list of all resources needed
	CStr m_AllJackieModels;
	CStr m_AllVocapSounds;
	CStr m_AllTextures;


	// camera movement and rotation
	CMat4Dfp32 m_GuiCameraMatrix;
	CVec3Dfp32 m_CameraMaxRot;
	CVec3Dfp32 m_CameraMinRot;
	CVec3Dfp32 m_CameraMaxPos;
	CVec3Dfp32 m_CameraMinPos;
	CVec3Dfp32 m_CurRot;
	CVec3Dfp32 m_CurPos;

	//-----------------------------------------------------------------------------------------------------------
	// for loading GUI (Jackie monologue)

	void LoadAndSetupLoadingScene();
	void StartLoadingScene();
	void UpdateLoadingScene(CXR_VBManager *_pVBM, fp32 _DeltaTime, bool _bGameIsLoading = true);
	void RenderLoadingScene(CXR_Engine *_pEngine, CXR_VBManager *_pVBM, CRenderContext *_pRC);
	void RenderDeathScene(CXR_Engine *_pEngine, CXR_VBManager *_pVBM, CRenderContext *_pRC);
	void RenderSubtitles(CXR_VBManager *_pVBM, CRenderContext *_pRC);
	void LoadingScene_EnumerateView(CXR_Engine* _pEngine, int _iVC, int _EnumViewType);
	void ClearResourcesAfterLoadingScene(CXR_Engine *_pEngine);

	bool RefreshSubtitles(const CMTime& _Time, fp32 _DeltaTime, CDialogueInstance &_Instance);

	void UpdateLSCameraPos(CMat4Dfp32* _pCameraMat);
	void UpdateLSAnimation(CXR_VBManager* _pVBM, fp32 _DeltaTime);

	void SetTextureBlack(CRenderContext* _pRC, CXR_VBManager* _pVBM, int _iTextureID, CXR_Engine *_pEngine);
	bool FadeProcessImageScreen(CRenderContext* _pRC, CXR_VBManager* _pVBM, int _iTextureID, fp32 _DeltaTime, bool _bFadeIn);

	CStr m_LoadingSceneAnim;
	int m_iCurrentAnim;
	fp32 m_LoadingSceneAnimTime;

	CStr m_LoadingBGImage;

	TArray<CStr>	m_lUsedDeathSurfaces;
	TArray<CStr>	m_lDeathSounds;
	CStr			m_DeathSeqPast;
	CStr			m_DeathSeqPresent;
	CStr			m_DeathSeqFuture;
	fp32			m_DeathAlpha;
	fp32			m_DeathLastSwitchTime;
	fp32			m_DeathTime;
	fp32			m_DeathFadeTimer;
	uint32			m_DeathDialogueHash;
	uint8			m_DeathCurrentSurface;
	uint8			m_DeathFadeStatus;

	int m_iModelJackie;
	int m_iModelWeaponMain;
	int m_iModelWeaponSecondary;
	int m_iModelExtra;

	CVoCap m_VoCap;
	CLipSync m_LipSync;
	int	m_hVoice;
	int m_iFacialSetup;
	fp32 m_TotalAnimLength;
	int m_iAnimID;
	fp32 m_SoundDelay;

	fp32 m_LastRenderTime; // remove this..
	TArray<CXR_SkeletonCloth> m_lCloth;

	fp32 m_FadeTimer;

	int m_iDialogue;
	CDialogueInstance m_DialogueInstance;
	fp32 m_ProjectionFlickerTimer;
	CVec3Dfp32 m_ProjLightPos;

	CXR_AnimState m_CharAnimState;
	int m_iAnimGripBasePistol;
	//CXR_Anim_Base *m_pAnimGripBaseRifle;

	int m_iLSMParticles;
	
	int32 m_LastiActive;
	int32 m_LastSide;
	//-----------------------------------------------------------------------------------------------------------

	struct SProjImage
	{
		CRect2Duint16 Rect;
		int	TexID;
	};
	TArray<SProjImage> m_lProjImages;

	void SetupProjectionTexture(CRenderContext* _pRC, CXR_VBManager* _pVBM, bool _bStoreImage = false, fp32 _DeltaTime = 0.0f, CXR_Engine *_pEngine = NULL, bool _bIngame = false);
	int32 m_LastCurrentSide;
	fp32 m_GuiSoundVol;

	fp32 m_BackgrounProcessTimer;

	int m_iGUIWorld;  // resource index
	spCXR_SceneGraphInstance m_spGUISGI;

	int m_iLoadingWorld; // resource index
	spCXR_SceneGraphInstance m_spLoadingSGI;

	void SetGUIAndLoadingXRWorlds(CXR_Engine *_pEngine);
	virtual void EngineClient_EnumerateView(CXR_Engine* _pEngine, int _iVC, int _EnumViewType);
	virtual void EngineClient_Refresh() {}

	// for light rotations
	void UpdateProjectionMatrix(fp32 _DeltaTime, int8 _Scroll);
	CVec3Dfp32 m_LightPos;
	CVec3Dfp32 m_LightRotation;
	CVec3Dfp32 m_OrigLightRotation;
	fp32 m_LightRotationSpeed;

	CVec3Dfp32 m_LightIntesity;
	fp32 m_LightSpotRange;
	CVec2Dfp32 m_LightSpotSizeOrig;
	CVec2Dfp32 m_LightSpotSize;

	// the addlight spot
	void UpdateAddlight(fp32 _DeltaTime);
	fp32 m_AddSpotFlickerPermissionTimer;
	fp32 m_AddSpotFlickerTimer;

	CVec3Dfp32 m_AddSpotPos;
	CVec3Dfp32 m_AddSpotRotation;
	CVec3Dfp32 m_OrigAddSpotRotation;
	fp32 m_AddSpotRotationSpeed;

	CVec3Dfp32 m_OrigAddSpotIntesity;
	CVec3Dfp32 m_AddSpotIntesity;
	fp32 m_AddSpotRange;
	CVec2Dfp32 m_AddSpotSizeOrig;
	CVec2Dfp32 m_AddSpotSize;

	int m_BackgroundSoundLoopID;

	int m_CubeProjectionSize;
	fp32 m_DeltaTime;

	fp32 m_LoadingSceneFOV;
};


class CCubeUser
{
	bool m_Using;
public:
	CWFrontEnd_Mod *m_pFrontEndMod;		// you can assume that these will always exist
	class CGameContextMod *m_pGameContextMod;
public:
	CCubeUser();
	~CCubeUser();

	void Use(bool _Use);

	CWFrontEnd_Mod *GetFrontEnd() const { return m_pFrontEndMod; }
	//CGameContextMod *GetGameContext() const { return m_pGameContextMod; }
};

#endif // _INC_WFRONTENDMOD_CUBE
