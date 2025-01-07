
#include "PCH.h"

#include "WClient_Core.h"
#include "../../../XR/XREngineVar.h"
#include "MFloat.h"
#include "../../Render/MRenderCapture.h"
#include "../WObjects/WObj_Game.h"
#include "../../../MSystem/Misc/MPerfGraph.h"
#include "../../../XRModels/Model_TriMesh/WTriMesh.h"

#include "../../../../../projects/main/gameclasses/WObj_Char.h"


// MUPPJOKKO -	Temp fix, this still fucks upp with translucent surfaces in the world!
#ifdef PLATFORM_PS2
#include "MRndrPS2_DmaEngine.h"
#include "MDispPS2.h"
#endif


/*
template<class T>
int M_CDECL QSortCompare(const void* _pE1, const void* _pE2)
{
MAUTOSTRIP(QSortCompare, 0);
const T& E1 = *_pE1;
const T& E2 = *_pE2;

if (E1 < E2)
return -1;
else if (E1 > E2)
return 1;
return 0;
}

template<class T>
void QSort(T* _pElem, int _nElem, int _bAscending)
{
MAUTOSTRIP(QSort, MAUTOSTRIP_VOID);
//	int(M_CDECL *pfnCompare)(const void*, const void*) = QSortCompare;
qsort((void*) _pElem, _nElem, sizeof(T), QSortCompare);
}
*/



#define MACRO_QSORT(T)												\
	int M_CDECL QSortCompare_##T(const void* _pE1, const void* _pE2);	\
	int M_CDECL QSortCompare_##T(const void* _pE1, const void* _pE2)	\
{																	\
	const T& E1 = *((T*)_pE1);										\
	const T& E2 = *((T*)_pE2);										\
	\
	if (E1 < E2)													\
	return -1;													\
	else if (E1 > E2)												\
	return 1;													\
	return 0;														\
}																	\
	\
	void QSort_##T(T* _pElem, int _nElem, int _bAscending);				\
	void QSort_##T(T* _pElem, int _nElem, int _bAscending)				\
{																	\
	qsort((void*) _pElem, _nElem, sizeof(T), QSortCompare_##T);		\
}

MACRO_QSORT(uint16);

class CThreadPoolOnClientRenderArg
{
public:
	CThreadPoolOnClientRenderArg(CWorld_ClientCore* _pWClient, CXR_Engine* _pEngine, int _EnumViewType, const uint16* _piObj) 
		: m_pWClient(_pWClient)
		, m_pEngine(_pEngine)
		, m_EnumViewType(_EnumViewType)
		, m_piObj(_piObj) 
	{
		m_BaseMat.Unit();
	}

	CWorld_ClientCore* m_pWClient;
	CXR_Engine* m_pEngine;
	CMat4Dfp32 m_BaseMat;
	int m_EnumViewType;
	const uint16* m_piObj;
};

static void Thread_OnClientRender(int _iArg, void* _pArg)
{
	CThreadPoolOnClientRenderArg* pArg = (CThreadPoolOnClientRenderArg*)_pArg;
	uint16 iObj = pArg->m_piObj[_iArg];
	CWObject_Client* pObj = pArg->m_pWClient->Object_Get(iObj);
	if(!pObj)
		return;

	if(pObj->m_ClientFlags & CWO_CLIENTFLAGS_INVISIBLE)
		return;

	pArg->m_pWClient->OnClientRender(pArg->m_pEngine, pArg->m_EnumViewType, iObj, pObj, pArg->m_BaseMat);
}

void CWorld_ClientCore::EngineClient_EnumerateView(CXR_Engine* _pEngine, int _iVC, int _EnumViewType)
{
	MAUTOSTRIP(CWorld_ClientCore_EngineClient_EnumerateView, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWorld_ClientCore::EngineClient_EnumerateView);
	M_NAMEDEVENT("EngineClient_EnumerateView", 0xffff8000);

	if (!m_pSceneGraph || !m_spSceneGraphInstance)
		return;

	//	CXR_FogState* pFog = _pEngine->GetFogState();
	//	if (pFog) pFog->m_DepthFogEnd = _pEngine->m_pRender->Viewport_Get()->GetBackPlane();

	TProfileDef(TEnum);
	//	T_ProfileDef(TPVS);
	{
		TMeasureProfile(TEnum);

		if (_EnumViewType == CXR_ENGINE_EMUMVIEWTYPE_VIEWCLIP)
		{
			m_spSceneGraphInstance->SceneGraph_Light_ClearDynamics();

			fp32 Range = m_HeadLightRange;
			if(m_ForcedHeadLightRange != 0)
				Range = m_ForcedHeadLightRange;
			if (Range > _FP32_EPSILON && (m_HeadLightIntensity.LengthSqr() > 0.001f))
			{
				CXR_Light Light(CVec3Dfp32::GetMatrixRow(_pEngine->GetVC()->m_CameraWMat, 3), m_HeadLightIntensity, Range, CXR_LIGHT_NOSHADOWS, CXR_LIGHTTYPE_POINT);
				Light.m_LightGUID = 0x2345;
				_pEngine->Render_Light_AddDynamic(Light);

				m_spSceneGraphInstance->SceneGraph_Light_LinkDynamic(Light);
			}

			/*		const uint8* pPVS = NULL;
			if (!(m_ClientDrawDisable & 4)) pPVS = (m_pSceneGraph) ? m_pSceneGraph->SceneGraph_PVSLock(0, _pEngine->GetVC()->m_VisPos) : NULL;

			if (!pPVS)
			Error("EngineClient_EnumerateView", "No PVS.");

			T_Start(TPVS);
			spCWObject_Client* lpObj = m_lspObjects.GetBasePtr();
			m_nEnumObj = 0;
			if (!(m_ClientDrawDisable & 1)) 
			{
			m_nEnumObj += m_pSceneGraph->SceneGraph_EnumeratePVS(m_hSceneGraph, pPVS, m_liEnumObj.GetBasePtr(), MaxObj);
			}
			else*/
			{
				MSCOPESHORT(ViewClip);
				//			m_liEnumObj[0] = 2;	// Shoule be the world-entity, but it's an ugly assumption.
				//			m_nEnumObj = 1;
				m_nEnumObj = m_iObjVisPool.EnumAll(m_liEnumObj.GetBasePtr(), m_liEnumObj.Len());

				QSort_uint16(m_liEnumObj.GetBasePtr(), m_nEnumObj, 0);
			}
			//		m_pSceneGraph->SceneGraph_PVSRelease(pPVS);
			//		T_Stop(TPVS);
			//		m_TPVS += TPVS;
		}
		else if (_EnumViewType == CXR_ENGINE_EMUMVIEWTYPE_NORMAL)
		{
			World_CommitDeferredSceneGraphLinkage();

			MSCOPESHORT(Normal);
			const int MaxObj = 4096;
			m_liEnumObj.SetLen(MaxObj);
			m_nEnumObj = m_spSceneGraphInstance->SceneGraph_EnumerateView(_iVC, m_liEnumObj.GetBasePtr(), MaxObj);
		}


//		M_TRY
//		{
#ifndef DEF_DISABLE_PERFGRAPH
			if (_EnumViewType == CXR_ENGINE_EMUMVIEWTYPE_VIEWCLIP)
				m_PerfGraph_Tick_OnClientRender++;
			//			fp64 CPUFreqRecp = 1.0 / GetCPUFrequency();
			if(m_PerfGraph)
			{
				if (m_PerfGraph_lObjects.Len() == 0)
				{
					MSCOPE(PerfGraph, IGNORE);
					int nClObj = m_lspObjects.Len() + m_lspClientObjects.Len();
					m_PerfGraph_lObjects.SetLen(nClObj);
					for(int i = 0; i < nClObj; i++)
						m_PerfGraph_lObjects[i].Create();
					m_PerfGraph_Tick_OnClientRender = 0;
					m_PerfGraph_Tick_OnClientRefresh = 0;
				}
			}
#endif

			SetMultiThreadState(PHYSSTATE_MTSTATE_ASSERTUNSAFE);

			CThreadPoolOnClientRenderArg Arg(this, _pEngine, _EnumViewType, m_liEnumObj.GetBasePtr());
			MRTC_ThreadPoolManager::ProcessEachInstance(m_nEnumObj, &Arg, Thread_OnClientRender, "OnClientRender", m_bSyncOnClientRender);

			SetMultiThreadState(0);

			// Calculate velocities for all models added.
			{
				M_NAMEDEVENT("ModelVelocity", 0xffffffff);

				CXR_ViewContext* pVC = _pEngine->GetVC();
				TAP<CXR_VCModelInstance> pMI = pVC->m_lObjects;

				TAP<TPtr<CWObject_Client> > lpCD = m_lspObjects;

				for(int iMI = 0; iMI < pMI.Len(); iMI++)
				{
					uint iObj = pMI[iMI].m_Anim.m_iObject;
					if (iObj && (iObj < lpCD.Len()))
					{
						CWObject_Client* pObj = lpCD[iObj];
						if (pObj)
						{
							CMat4Dfp32 PosInv;
							pObj->GetLastPositionMatrix().InverseOrthogonal(PosInv);
							PosInv.Multiply(pObj->GetPositionMatrix(), pMI[iMI].m_Velocity);

//							M_TRACEALWAYS("iMI %d, iObj %d, WVel %s\n", iMI, iObj, pMI[iMI].m_Velocity.GetString().Str());
						}
					}
				}

			}

#ifndef DEF_DISABLE_PERFGRAPH
			// Plot zero-time for all object's that were not visible.
			if (m_PerfGraph && _EnumViewType == CXR_ENGINE_EMUMVIEWTYPE_NORMAL)
				for(int i = 0; i < m_PerfGraph_lObjects.Len(); i++)
				{
					CWC_ObjectPerfGraph& ObjPerf = m_PerfGraph_lObjects[i];
					if (ObjPerf.m_LastTouch_OnClientRender != m_PerfGraph_Tick_OnClientRender)
					{
						ObjPerf.m_Time_OnClientRender.Reset();
						ObjPerf.m_spGraph_OnClientRender->Plot(0, 0xff101060);
					}
				}
#endif
				if (m_ClientDrawDisable & 16) ConOut(CStrF("%d objects in PVS. %d models.", m_nEnumObj, _pEngine->GetVC()->m_nObjects));
/*		}
		M_CATCH(
			catch(CCException)
		{
			Error("CWorld_ClientCore::Render_EnumerateView", "Exception during view-enumeration.");
			throw;
		}
		)*/
	}
	TAddProfile(m_TRender, TEnum);

	m_NumRender++;

#ifdef M_Profile
	if (m_ClientDrawDisable & 32) 
		ConOut(CStrF("OnClientRender %f ms", TEnum.GetTime() * 1000.0f));//, /*TPVS*/0 / GetCPUFrequency() * 1000.0f));
#endif
}

void CWorld_ClientCore::EngineClient_Refresh()
{
	MAUTOSTRIP(CWorld_ClientCore_EngineClient_Refresh, MAUTOSTRIP_VOID);
	//	CMTime Time = CMTime::GetCPU();
	CMTime Time;
	Time.Snapshot();
	if ((Time - m_LastEngineRefresh).GetTime() < 0.005f)
	{
		m_LastEngineRefresh = Time;
	}
}



CXR_Engine* CWorld_ClientCore::Render_GetEngine()
{
	MAUTOSTRIP(CWorld_ClientCore_Render_GetEngine, NULL);
	if (m_ClientMode & WCLIENT_MODE_MIRROR)
		Error("Render_GetEngine", "Invalid operation on client mirror.")

		/*
		if (!m_spEngine)
		{
			spCReferenceCount spObj = (CReferenceCount*) MRTC_GOM()->CreateObject("CXR_EngineImpl");
			m_spEngine = safe_cast<CXR_Engine> ((CReferenceCount*)spObj);
			if (!m_spEngine) Error("Render", "Unable to create engine-object.");
			m_spEngine->Create(m_MaxRecurseDepth, 0);
			m_spEngine->SetVarf(XR_ENGINE_LIGHTSCALE, 0.5f);	// Set light-scale for 2x overbrightness.
			if (GetClientMode() & WCLIENT_MODE_SPLITSCREEN)
				m_spEngine->SetVarf(XR_ENGINE_NORENDERTOTEXTURE, 1);
		}
		*/

		return m_spEngine;
}

void CWorld_ClientCore::Render_GetCamera(CMat4Dfp32& _Camera)
{
	MAUTOSTRIP(CWorld_ClientCore_Render_GetCamera, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ClientCore::Render_GetCamera, WORLD_CLIENT);
	// Default behaviour, can be completely overridden.

	// Ask client's player object for camera.
	CWObject_Message Msg(OBJSYSMSG_GETCAMERA);
	Msg.m_pData = &_Camera;
	Msg.m_DataSize = sizeof(_Camera);

	_Camera.Unit();

	if (!ClientMessage_SendToObject(Msg, Player_GetLocalObject()))
	{
		CWObject_CoreData* pObj = Object_GetCD(Player_GetLocalObject());
		if (!pObj) return;

		// Object did not respond: Interpolate direction and position for player-object.
		Interpolate2(pObj->GetLastPositionMatrix(), pObj->GetPositionMatrix(), _Camera, Min(1.0f, m_RenderTickFrac) );

		// Do some camera rotation. Object's forward vector is the matrix's x-axis, but for rendering the forward-vector needs to be z
		_Camera.RotX_x_M(-0.25f);
		_Camera.RotY_x_M(0.25f);
	}
}

void CWorld_ClientCore::Render_GetLastRenderCamera(CMat4Dfp32& _Camera)
{
	MAUTOSTRIP(CWorld_ClientCore_Render_GetLastRenderCamera, MAUTOSTRIP_VOID);
	_Camera = m_LastRenderCamera;
}

void CWorld_ClientCore::Render_ModifyViewport(CRC_Viewport& _Viewport)
{
	MAUTOSTRIP(CWorld_ClientCore_Render_GetViewport, MAUTOSTRIP_VOID);

	// Ask client's player object for viewport.
	CWObject_Message Msg(OBJSYSMSG_GETVIEWPORT);
	Msg.m_pData = &_Viewport;
	Msg.m_DataSize = sizeof(_Viewport);

	if(!ClientMessage_SendToObject(Msg, Player_GetLocalObject()))
	{
		// Found no player.. Check with the game-obj
		ClientMessage_SendToObject(Msg, Game_GetObjectIndex());
	}

	m_LastViewport = _Viewport;
}

void CWorld_ClientCore::Render_GetLastRenderViewport(CRC_Viewport& _Viewport)
{
	MAUTOSTRIP(CWorld_ClientCore_Render_GetLastRenderViewport, MAUTOSTRIP_VOID);
	_Viewport = m_LastViewport;
}

CVec2Dfp32 CWorld_ClientCore::Render_GetViewScaleMultiplier()
{
	return CVec2Dfp32(1,1);
}

void CWorld_ClientCore::Render_SetViewport(CXR_VBManager* _pVBM)
{
	MAUTOSTRIP(CWorld_ClientCore_Render_SetViewport, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ClientCore::Render_SetViewport, WORLD_CLIENT);

	// Override if not the whole screen should be rendered, or projection should be changed.
	CRC_Viewport VP;
	VP = *(_pVBM->Viewport_Get());
	Render_ModifyViewport(VP);
	*(_pVBM->Viewport_Get()) = VP;
}

void CWorld_ClientCore::Render_GUI(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _GUIVP)
{
	MAUTOSTRIP(CWorld_ClientCore_Render_GUI, MAUTOSTRIP_VOID);
	// Overridable
}

void CWorld_ClientCore::Render_World(CXR_VBManager* _pVBM, CRenderContext* _pRender, const CMat4Dfp32& _Camera, const CMat4Dfp32& _CameraVelocity, fp32 _InterpolationTime)
{
	MAUTOSTRIP(CWorld_ClientCore_Render_World, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWorld_ClientCore::Render_World); //AR-SCOPE

	// Create engine-object if it has not been done yet.
	CXR_Engine* pEngine = Render_GetEngine();

	int OldAllow = _pRender->Attrib_GlobalGetVar(CRC_GLOBALVAR_ALLOWTEXTURELOAD);
	_pRender->Attrib_GlobalSetVar(CRC_GLOBALVAR_ALLOWTEXTURELOAD, Render_World_AllowTextureLoad());

	m_RenderTickFrac = _InterpolationTime;
	m_RenderTime = GetGameTime() + CMTime::CreateFromSeconds(m_RenderTickFrac*GetGameTickTime());
	if ((m_RecMode != WCLIENT_RECMODE_PLAYBACK) && (GetClientMode() & WCLIENT_MODE_LOCAL))
		m_CurrentRenderTickFrac.SetScalar(m_LocalFrameFraction);
	else
		m_CurrentRenderTickFrac.SetScalar(m_RenderTickFrac);

	m_LastEngineRefresh = m_RenderTime;

	pEngine->SetEngineTime(m_RenderTime);	// Time wrapping every 2.77 hrs.
	pEngine->SetDebugFont(m_spGUIData->GetResource_Font(m_spGUIData->GetResourceIndex_Font("MONOPRO")));


#ifdef PLATFORM_PS2
	CDisplayContextPS2::EnableColorBuffer();
#endif

	M_TRY
	{ 
		pEngine->Engine_Render(this, _pVBM, _pRender, _Camera, _CameraVelocity, m_spSceneGraphInstance, m_spTMDC); 
	}
	M_CATCH(
		catch(CCException _Ex)
	{
		_pRender->Attrib_GlobalSetVar(CRC_GLOBALVAR_ALLOWTEXTURELOAD, OldAllow);
	//	Error("CWorld_ClientCore::Render", "Exception during world-rendering.");
		throw;
	}
	)
		_pRender->Attrib_GlobalSetVar(CRC_GLOBALVAR_ALLOWTEXTURELOAD, OldAllow);

#ifdef PLATFORM_PS2
	CDisplayContextPS2::DisableColorBuffer();
#endif




}

void CWorld_ClientCore::Render_ObjectPerfGraphs(CXR_VBManager* _pVBM, CRenderContext* _pRender, const CMat4Dfp32& _Camera)
{
	MAUTOSTRIP(CWorld_ClientCore_Render_ObjectPerfGraphs, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWorld_ClientCore::Render_ObjectPerfGraphs); //AR-SCOPE
#ifndef DEF_DISABLE_PERFGRAPH
	if (m_PerfGraph)
	{
		CRC_Font* pFont = m_spGUIData->GetResource_Font(m_spGUIData->GetResourceIndex_Font("MONOPRO"));
		if (pFont)
		{
			_pVBM->ScopeBegin("CWorld_ClientCore::Render_ObjectPerfGraphs", false);

			// Alloc/init attributes
			CRC_Attributes* pA = _pVBM->Alloc_Attrib();
			pA->SetDefault();
			pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			pA->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);

			// Alloc matrix
			CMat4Dfp32 VMat;
			const CMat4Dfp32& IVMat = _Camera;
			_Camera.InverseOrthogonal(VMat);

			CMat4Dfp32 *pMat = _pVBM->Alloc_M4(VMat);
			if (!pMat)
				return;

			//			fp64 FreqRecp = 1.0f / GetCPUFrequency();
			for(int iObj = 0; iObj < m_PerfGraph_lObjects.Len(); iObj++)
			{
				CWC_ObjectPerfGraph& ObjPerf = m_PerfGraph_lObjects[iObj];

				if (//ObjPerf.m_LastTouch_OnClientRefresh != m_PerfGraph_Tick_OnClientRefresh &&
					ObjPerf.m_LastTouch_OnClientRender != m_PerfGraph_Tick_OnClientRender)
					continue;

				CWObject_CoreData* pObj = Object_GetCD(iObj);
				if (!pObj) continue;

				if (CVec3Dfp32::GetRow(_Camera, 3).DistanceSqr(pObj->GetPosition()) > Sqr(512))
					continue;

				CFStr St = CFStrF("%.0f", ObjPerf.m_Time_OnClientRender.GetTime() * 1000000.0);
				CFStr St2 = CFStrF("%.0f", ObjPerf.m_Time_OnClientRefresh.GetTime() * 1000000.0);
				//				CFStr St = CFStrF("%4s", St2.Str());

				CVec2Dfp32 V(8, 8);
				CVec3Dfp32 Down = CVec3Dfp32::GetRow(IVMat, 1);//(0, 0, -1);
				CVec3Dfp32 Dir = CVec3Dfp32::GetRow(IVMat, 0);
				CBox3Dfp32 BoundW;
				CVec3Dfp32 Pos;

				pObj->GetAbsVisBoundBox(BoundW);

				BoundW.m_Min.Lerp(BoundW.m_Max, 0.5, Pos);
				Pos[2] = BoundW.m_Max[2] + 8.0f;

				if (pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER)
					Pos[2] += 32;

				{
					CXR_VertexBuffer *pVB = _pVBM->Alloc_VB();
					if (!pVB) break;
					pVB->m_pAttrib = pA;
					pFont->Write(_pVBM, pVB, Pos, Dir, Down, St.Str(), V, 0x6f4040ff);
					pVB->Matrix_Set(pMat);
					_pVBM->AddVB(pVB);
				}

				{
					CXR_VertexBuffer *pVB = _pVBM->Alloc_VB();
					if (!pVB) break;
					pVB->m_pAttrib = pA;
					pFont->Write(_pVBM, pVB, Pos+Dir*16, Dir, Down, St2.Str(), V, 0x6fffff00);
					pVB->Matrix_Set(pMat);
					_pVBM->AddVB(pVB);
				}


				fp32 DistSqr = CVec3Dfp32::GetRow(_Camera, 3).DistanceSqr(Pos);

				if (DistSqr < Sqr(384))
				{
					CMat4Dfp32 WMat;
					WMat.Unit();
					Pos.SetRow(WMat, 3);

					CMat4Dfp32 L2VMat;
					WMat.Multiply(VMat, L2VMat);
					L2VMat.Unit3x3();

					CXR_VertexBuffer* pVBOnClientRender = ObjPerf.m_spGraph_OnClientRender->Render(_pVBM, CVec2Dfp32(0, -16), L2VMat);
					CXR_VertexBuffer* pVBOnClientRefresh = ObjPerf.m_spGraph_OnClientRefresh->Render(_pVBM, CVec2Dfp32(16, -16), L2VMat);
					if (pVBOnClientRender)
					{
						pVBOnClientRender->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
						//						pVBOnClientRender->m_pAttrib = pA;
						_pVBM->AddVB(pVBOnClientRender);
					}
					if (pVBOnClientRefresh)
					{
						pVBOnClientRefresh->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
						//						pVBOnClientRefresh->m_pAttrib = pA;
						_pVBM->AddVB(pVBOnClientRefresh);
					}

					Pos += Down*V[1];

					if (DistSqr < Sqr(128))
					{
						CFStr St = CFStrF("ID %d, %s", pObj->m_iObject, pObj->m_pRTC->m_ClassName+9);

						CXR_VertexBuffer *pVB = _pVBM->Alloc_VB();
						if (!pVB) return;
						pFont->Write(_pVBM, pVB, Pos, Dir, Down, St.Str(), V, 0x6fffff00);
						pVB->Matrix_Set(pMat);
						pVB->m_pAttrib = pA;
						_pVBM->AddVB(pVB);
						Pos += Down*V[1];

						CVec3Dfp32 ObjPos = pObj->GetPosition();
						St = CFStrF("%.1f, %.1f, %.1f", ObjPos[0], ObjPos[1], ObjPos[2]);

						pVB = _pVBM->Alloc_VB();
						if (!pVB) return;
						pFont->Write(_pVBM, pVB, Pos, Dir, Down, St.Str(), V, 0x6fffff00);
						pVB->Matrix_Set(pMat);
						pVB->m_pAttrib = pA;
						_pVBM->AddVB(pVB);
						Pos += Down*V[1];
					}
				}
			}

			_pVBM->ScopeEnd();
		}
	}
#endif
}

void CWorld_ClientCore::Render_ResetCameraVelocity()
{
	m_CamWVel.Unit();
	m_CamWVel_LastGameTime.Reset();
	m_CamWVel_LastCmdTime.Reset();
	Render_GetLastRenderCamera(m_CamWVel_LastCamera);
}

void CWorld_ClientCore::Render_GetCameraVelocity(const CMat4Dfp32& _WCam, CMat4Dfp32& _WCamVelocity)
{
	CMTime T = GetRenderTime();
	CMTime dT = T - m_CamWVel_LastGameTime;
	if ((dT.Compare(0.0f) == 1) && (m_LastCmdTime.Compare(m_CamWVel_LastCmdTime) == 1) && !(m_ClientMode & WCLIENT_MODE_PAUSE))
	{
//		M_TRACEALWAYS("dT = %f\n", dT.GetTime());

		CMat4Dfp32 LastCamScaled;
		Interpolate2(_WCam, m_CamWVel_LastCamera, LastCamScaled, GetGameTickTime() / dT.GetTime() );

		CMat4Dfp32 LastInv;
		LastCamScaled.InverseOrthogonal(LastInv);
		LastInv.Multiply(_WCam, m_CamWVel);
		m_CamWVel_LastGameTime = T;
		m_CamWVel_LastCamera = _WCam;
		m_CamWVel_LastCmdTime = m_LastCmdTime;
	}

	_WCamVelocity = m_CamWVel;
}

#define FRAC(a) ((a) - M_Floor(a))

void CWorld_ClientCore::Render(CXR_VBManager* _pVBM, CRenderContext* _pRender, CRC_Viewport& _GUIVP, fp32 _InterpolationTime, int _Context)
{
	MAUTOSTRIP(CWorld_ClientCore_Render, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWorld_ClientCore::Render);
	M_NAMEDEVENT("Client::Render", 0xff400000);

	if (!_pRender) return;

	// NOTE: If _Context != 0 we must not render anything but the loading screen.

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (pSys)
		m_Render_CharacterShadows = pSys->GetEnvironment()->GetValuei("XR_CHARSHADOWS", 1);

	CRC_Viewport _3DVP = *_pVBM->Viewport_Get();

	// Lag time plot
#ifdef M_Profile
	{
		//		CMTime Time = CMTime::GetCPU();
		CMTime Time;
		Time.Snapshot();

		fp32 T = m_LocalPlayer.m_spCmdQueue->GetTotalCmdTime() + (Time - m_LastCmdTime).GetTime();
		T += m_LocalPlayer.m_spCmdQueue->m_LastAccumTime;

		if (T > 0.0f)
			AddGraphPlot(WCLIENT_GRAPH_LAGTIME, T, 0xffff0000);
		else
			AddGraphPlot(WCLIENT_GRAPH_LAGTIME, T, 0xffffff00);
	}

	// Render time plot
	{
		//		CMTime Time = CMTime::GetCPU();
		CMTime Time;
		Time.Snapshot();
		AddGraphPlot(WCLIENT_GRAPH_RENDERTIME, (Time - m_LastRenderTime).GetTime(), 0xff00ff00);
		m_LastRenderTime = Time;
	}
#endif


	// Create engine-object if it has not been done yet.
	CXR_Engine* pEngine = Render_GetEngine();
	pEngine->Engine_SetRenderContext(_pRender);

	// Check that everything is ok for rendering.
	bool bInGame = CanRenderWorld();
	/*		(m_ClientState == WCLIENT_STATE_INGAME) &&
	(m_spMapData != NULL) &&
	m_lObjects.Len();*/
	//		(m_iLocalPlayer >= 0) &&
	//		(m_iLocalPlayer < m_lPlayers.Len());


	if (_Context || m_Precache || !bInGame)
	{
		if(m_Precache && _Context == 0)
			Precache_Perform(_pRender, 1.0f);
		return;
	}

	// Interpolate a bit differently if we're running as a true network client.
	//	if (m_lspFrameIn.Len() && (m_hConnection >= 0))

	// We can't mess with the interpolation time if we're in demo playback mode.
	CMTime Time = GetTime();
	if (m_RecMode != WCLIENT_RECMODE_PLAYBACK)
		if(!(WCLIENT_MODE_LOCAL & GetClientMode()))
			_InterpolationTime = Max(fp32(-100.0f), Min(fp32(500.000f), ((Time - m_LastFrameTime).GetTime() * m_TimeScale))) / GetGameTickTime();

	//	if (m_ClientMode & WCLIENT_MODE_PAUSE)
	//		_InterpolationTime = 0;

#ifdef WCLIENT_FIXEDRATE
	_InterpolationTime = 1;
#endif

	// These need to be set before Render_GetCamera(), otherwise the predicted camera will be wrong.
	m_RenderTickFrac = _InterpolationTime;
	CMTime NewTime = GetGameTime() + CMTime::CreateFromSeconds(m_RenderTickFrac*GetGameTickTime());
	m_LastRenderFrameTime = NewTime - m_RenderTime;
	m_RenderTime = NewTime;

	if ((m_RecMode != WCLIENT_RECMODE_PLAYBACK) && (GetClientMode() & WCLIENT_MODE_LOCAL))
		m_CurrentRenderTickFrac.SetScalar(m_LocalFrameFraction);
	else
		m_CurrentRenderTickFrac.SetScalar(m_RenderTickFrac);


#if defined(M_Profile) && !defined(DEF_DISABLE_PERFGRAPH)
	if (GetGraph(WCLIENT_GRAPH_SOUNDTIME))
		GetGraph(WCLIENT_GRAPH_SOUNDTIME)->Plot(FRAC(fp32(GetGameTick() + GetRenderTickFrac()) / 15.0f) * 0.2f, 0xff40c080);
#endif
	if (m_ClientDrawDisable & 64)
		ConOutL(CStrF("Tick %d, RT %f, LFT %f, FP %f, TS %f, GT %f => %f", 
		GetGameTick(), Time.GetTime(), m_LastFrameTime.GetTime(), m_FramePeriod, m_TimeScale, GetGameTickTime(), _InterpolationTime ));

	int iGameObj = Game_GetObjectIndex();
	bool bCanRenderWorld = ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_NEVERRENDERWORLD), iGameObj) == 0;

	// Render 3D World
	CMat4Dfp32 CameraMat;
	Render_GetCamera(CameraMat);

	{
		CMat4Dfp32 Camera = CameraMat;
		Camera.RotY_x_M(-0.25f);
		Camera.RotX_x_M(0.25f);

		CMat4Dfp32 LastCamera = m_LastRenderCamera;
		LastCamera.RotY_x_M(-0.25f);
		LastCamera.RotX_x_M(0.25f);

		fp32 FrameTime = m_LastRenderFrameTime.GetTime();
		if (FrameTime > 0.0f)
		{
			m_LastRenderCameraVelocity = (CVec3Dfp32::GetMatrixRow(Camera,3) - CVec3Dfp32::GetMatrixRow(LastCamera,3));
			m_LastRenderCameraVelocity.Scale(1.0f / FrameTime, m_LastRenderCameraVelocity);
		}

		m_LastRenderCamera = CameraMat;
	}

	CMat4Dfp32 CameraWVelMat;
	Render_GetCameraVelocity(CameraMat, CameraWVelMat);

	if(iGameObj <= 0 || bCanRenderWorld)
	{
		//ConOut("----------- BEGIN RENDER -----------------");
		{
			/*			bool bPushVP = false;
			if(IsFullScreenGUI() && !IsCutscene())
			{
			CRC_Viewport VP = _3DVP;
			CVec2Dfp32 border = 0;
			MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
			border[0] = pSys->GetEnvironment()->GetValuef("VID_BORDERX");
			border[1] = pSys->GetEnvironment()->GetValuef("VID_BORDERY");

			pEngine->GetVBM()->Viewport_Push(&VP);
			bPushVP = true;
			}*/

			Render_SetViewport(_pVBM);
			Render_World(_pVBM, _pRender, CameraMat, CameraWVelMat, _InterpolationTime);
			Render_ObjectPerfGraphs(_pVBM, _pRender, CameraMat);

			//		if(bPushVP)
			//			pEngine->GetVBM()->Viewport_Pop();
		}

		// Need to restore active client copy for the player object since a predicted version may be active.
		Object_SetActiveClientCopy(Player_GetLocalObject(), 0);
	}

	// Render wire container and phys capture renderer.
	if (m_bPhysRender && (m_spWireContainer != NULL))
	{
		//		_pRender->Matrix_Push();
		//		_pRender->Attrib_Push();

		CMat4Dfp32 VMat;
		m_LastRenderCamera.InverseOrthogonal(VMat);
		//		_pRender->Matrix_Set(VMat.Get4x4());

		if (m_bPhysRender == 1 && m_spPhysCapture != NULL)
		{
			//			CCaptureBuffer* pCapture = m_spPhysCapture->GetCaptureBuffer();
			//			CRC_Viewport DummyVP;
			//pCapture->Render(_pRender, &DummyVP);
		}

		_pVBM->ScopeBegin(false, 1);
		m_spWireContainer->Render(_pVBM, VMat);
		_pVBM->ScopeEnd();

		//		_pRender->Matrix_Pop();
		//		_pRender->Attrib_Pop();
	}

	// Render 2D GUI
	if (!(m_ClientDrawDisable & 2))
	{
		M_NAMEDEVENT("Render_GUI", 0xff400000);
		if (_pVBM->Viewport_Push(&_GUIVP))
		{
			Render_SetViewport(_pVBM);
			Render_GUI(_pVBM, _pRender, _3DVP);
			//Render_Msg(_InterpolationTime);
			_pVBM->Viewport_Pop();
		}
	}

	if (m_spSound != NULL)
	{
		M_NAMEDEVENT("SoundUpdate", 0xff400000);

		m_bSound_PosSet = true;
		m_spSound->Listen3D_SetCoordinateScale(1.0f/32.0f);
		CMat4Dfp32 Camera = m_LastRenderCamera;
		Camera.RotY_x_M(-0.25f);
		Camera.RotX_x_M(0.25f);
		m_spSound->Listen3D_SetOrigin(Camera);
		m_spSound->Listen3D_SetVelocity(m_LastRenderCameraVelocity);

		Sound_UpdateSoundVolumes(CVec3Dfp32::GetRow(m_LastRenderCamera, 3));
		Sound_UpdateTrackSounds();
	}
}


// -------------------------------------------------------------------
void Interpolate(const CMat4Dfp64& _Pos, const CMat4Dfp64& _dPos, CMat4Dfp64& _Dest, fp64 t);
void Interpolate(const CMat4Dfp64& _Pos, const CMat4Dfp64& _dPos, CMat4Dfp64& _Dest, fp64 t)
{
	MAUTOSTRIP(Interpolate, MAUTOSTRIP_VOID);
	if (t < 0.0f) 
		t = 0.0f;
	else if (t > 1.5f) 
		t = 1.5f;

	CMat4Dfp64 Pos2;
	_Pos.Multiply(_dPos, Pos2);

	_Dest.UnitNot3x3();
	CVec3Dfp64 v3 = CVec3Dfp64::GetMatrixRow(Pos2, 3) - CVec3Dfp64::GetMatrixRow(_Pos, 3);
	CVec3Dfp64::GetMatrixRow(_Dest, 3) = CVec3Dfp64::GetMatrixRow(_Pos, 3) + v3*t;

	CVec3Dfp64 v0 = CVec3Dfp64::GetMatrixRow(Pos2, 0) - CVec3Dfp64::GetMatrixRow(_Pos, 0);
	CVec3Dfp64::GetMatrixRow(_Dest, 0) = (CVec3Dfp64::GetMatrixRow(_Pos, 0) + v0*t).Normalize();
	CVec3Dfp64 v1 = CVec3Dfp64::GetMatrixRow(Pos2, 1) - CVec3Dfp64::GetMatrixRow(_Pos, 1);
	CVec3Dfp64::GetMatrixRow(_Dest, 2) = -((CVec3Dfp64::GetMatrixRow(_Pos, 1) + v1*t) / CVec3Dfp64::GetMatrixRow(_Dest, 0)).Normalize();
	CVec3Dfp64::GetMatrixRow(_Dest, 1) = CVec3Dfp64::GetMatrixRow(_Dest, 2) / CVec3Dfp64::GetMatrixRow(_Dest, 0);
}

CRenderContext* CWorld_ClientCore::Debug_GetRender()
{
	MAUTOSTRIP(CWorld_ClientCore_Debug_GetRender, NULL);
	return m_spPhysCapture;
}

CDebugRenderContainer* CWorld_ClientCore::Debug_GetWireContainer(uint/* _Flags*/)
{
	MAUTOSTRIP(CWorld_ClientCore_Debug_GetWireContainer, NULL);
	return m_spWireContainer;
}

// -------------------------------------------------------------------
// Wallmark
int CWorld_ClientCore::Wallmark_Create(const CMat4Dfp32 &_Pos, fp32 _Size, fp32 _Tolerance, CMTime _Time, const char* _pSurfName, int _Flags)
{
	MAUTOSTRIP(CWorld_ClientCore_Wallmark_Create, 0);
	if (!m_pWallmarkInterface)
		return 0;

	CXR_Engine* pEngine = Render_GetEngine();
	if (!pEngine->GetVar(XR_ENGINE_WALLMARKS))
		return 0;

	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	if (!pSC)
		return 0;

	CXR_WallmarkDesc WMD;
	WMD.m_SurfaceID = pSC->GetSurfaceID(_pSurfName);
	WMD.m_Size = _Size;
	WMD.m_SpawnTime = _Time;
	return Wallmark_Create(WMD, _Pos, _Tolerance, _Flags);
}

int CWorld_ClientCore::Wallmark_Create(const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags)
{
	MAUTOSTRIP(CWorld_ClientCore_Wallmark_Create_2, MAUTOSTRIP_VOID);
	if (!m_pWallmarkInterface)
		return 0;

	return m_pWallmarkInterface->Wallmark_Create(m_hWallmarkContext, _WM, _Origin, _Tolerance, _Flags);
}

int CWorld_ClientCore::Wallmark_Create(CXR_Model* _pModel, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags)
{
	if (!m_spTMDC)
		return 0;
	if (!_pModel)
		return 0;
	if (_pModel->GetModelClass() != CXR_MODEL_CLASS_TRIMESH)
		return 0;

	CXR_WallmarkInterface* pWMI = _pModel->Wallmark_GetInterface();
	if (!pWMI)
		return 0;

	pWMI->Wallmark_CreateWithContainer(m_spTMDC, _WM, _Origin, _Tolerance, _Flags);
	return 0;
}

bool CWorld_ClientCore::Wallmark_Destroy(int _GUID)
{
	MAUTOSTRIP(CWorld_ClientCore_Wallmark_Destroy, false);
	if (!m_pWallmarkInterface)
		return false;

	return m_pWallmarkInterface->Wallmark_Destroy(_GUID);
}

void CWorld_ClientCore::OnClientRender(CXR_Engine* _pEngine, int _EnumViewType, int _iObj, CWObject_Client* _pObj, const CMat4Dfp32& _BaseMat)
{
	// Call OnClientRender()
#ifndef DEF_DISABLE_PERFGRAPH
	CMTime T; TStart(T);
#endif
	if (_EnumViewType == CXR_ENGINE_EMUMVIEWTYPE_VIEWCLIP)
		_pObj->m_pRTC->m_pfnOnClientRenderVis(_pObj, this, _pEngine, _BaseMat);
	else
		_pObj->m_pRTC->m_pfnOnClientRender(_pObj, this, _pEngine, _BaseMat);
#ifndef DEF_DISABLE_PERFGRAPH
	TStop(T);

	//JK-TODO: Threadsafe these statistics
	CWorldData::CWD_ClassStatistics* pCS = m_spMapData->GetResource_ClassStatistics(_pObj->m_iClass);
	if (pCS)
	{
		pCS->m_ClientRenderTime += T;
		pCS->m_nClientRender++;
	}

	// Plot OnClientRender execution time
	if (m_PerfGraph)
	{
		// This should be thread-safe since creation is done before threading
		CWC_ObjectPerfGraph& ObjPerf = m_PerfGraph_lObjects[_iObj];
		ObjPerf.m_Time_OnClientRender = T;
		ObjPerf.m_spGraph_OnClientRender->Plot2(T.GetTime(), Clamp01(0.001f - T.GetTime()), 0x6f4040ff, 0x3f000000);
		ObjPerf.m_LastTouch_OnClientRender = m_PerfGraph_Tick_OnClientRender;
	}
#endif

	// Render physics-primitives? (Debug feature)
	if (m_bShowObjPhys && m_pPhysicsPrim)
	{
		bool bIP = true;
		while(_pObj)
		{
			fp32 IPTime = GetRenderTickFrac();
			CMat4Dfp32 MatIP;
			if (bIP)
				Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);
			else
				MatIP = _pObj->GetPositionMatrix();

			CXR_AnimState AnimState(CMTime(), CMTime(), _pObj->GetPhysState().m_PhysFlags, 0, 0, 0);
			AnimState.m_pContext = _pObj;
			//								AnimState.m_pContext = &_pObj->m_PhysState.m_Prim[i];
			_pEngine->Render_AddModel(m_pPhysicsPrim, MatIP, AnimState);

			_pObj = _pObj->GetNext();
			bIP = false;
		}
	}
}

