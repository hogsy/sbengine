#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/Mos/Classes/GameWorld/WObjects/WObj_System.h"

//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Wallmark
//----------------------------------------------------------------------

#define ANIM1_SIZEMASK			(0x0FFF)
#define ANIM1_FLAGS_SPAWNNOW	(0x1000)
#define ANIM1_FLAGS_SPAWNONCE	(0x2000)
#define ANIM1_FLAGS_NORECYCLE	(0x4000)

//----------------------------------------------------------------------

class CWObject_Wallmark : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

private:

	bool m_bWaitSpawn;

public:

	virtual void OnCreate()
	{
		MAUTOSTRIP(CWObject_Wallmark_ctor, MAUTOSTRIP_VOID);
		m_bWaitSpawn = false;
		iAnim0() = 0;
		iAnim1() = ANIM1_FLAGS_SPAWNNOW | 10;
		//ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
	}

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
	{
		MAUTOSTRIP(CWObject_Wallmark_OnEvalKey, MAUTOSTRIP_VOID);

		CStr KeyName = _pKey->GetThisName();

		switch (_KeyHash)
		{
		case MHASH3('WMSU','RFAC','E'): // "WMSURFACE"
			{
				m_iAnim0 = m_pWServer->GetMapData()->GetResourceIndex_Surface(_pKey->GetThisValue());
				break;
			}
		case MHASH2('WMSI','ZE'): // "WMSIZE"
			{
				iAnim1() |= _pKey->GetThisValuei() & ANIM1_SIZEMASK;
				break;
			}
			iAnim1() = (m_iAnim1 & ~ANIM1_SIZEMASK) | (_pKey->GetThisValuei() & ANIM1_SIZEMASK);
		case MHASH3('WMTO','LERA','NCE'): // "WMTOLERANCE"
			{
				iAnim2() |= _pKey->GetThisValuei();
				break;
			}
		case MHASH3('WAIT','SPAW','N'): // "WAITSPAWN"
			{
				m_bWaitSpawn = (_pKey->GetThisValuei() != 0);
				iAnim1() &=	~ANIM1_FLAGS_SPAWNNOW;
				break;
			}
		case MHASH3('SPAW','NONC','E'): // "SPAWNONCE"
			{
				iAnim1() |= (_pKey->GetThisValuei() != 0) ? ANIM1_FLAGS_SPAWNONCE : 0;
				break;
			}
		default:
			{
				CWObject_Model::OnEvalKey(_KeyHash, _pKey);
				break;
			}
		}
	}

	virtual aint OnMessage(const CWObject_Message& _Msg)
	{
		MAUTOSTRIP(CWObject_Wallmark_OnMessage, 0);

		switch(_Msg.m_Msg)
		{
			case OBJMSG_IMPULSE: iAnim1() |= m_bWaitSpawn ? ANIM1_FLAGS_SPAWNNOW : 0; break;
			default: return CWObject_Model::OnMessage(_Msg); break;
		}
		return 1; 
	}

	virtual void OnRefresh()
	{
//		m_pWServer->Object_SetDirtyTree(m_iObject, 0);
	}

	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
	{
		MAUTOSTRIP(CWObject_Wallmark_OnClientRefresh, MAUTOSTRIP_VOID);
		MSCOPESHORT( CWObject_Wallmark::OnClientRefresh );

		int iWMSurface = _pObj->m_iAnim0;
		int WMSize = _pObj->m_iAnim1 & ANIM1_SIZEMASK;
		bool bSpawnNow = ((_pObj->m_iAnim1 & ANIM1_FLAGS_SPAWNNOW) != 0);
		bool bSpawnOnce = ((_pObj->m_iAnim1 & ANIM1_FLAGS_SPAWNONCE) != 0);
		
		MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
		if(pSC && bSpawnNow && _pObj->m_ClientData[0] == 0)
		{
			_pObj->m_ClientData[0] = 1;

			CMat4Dfp32 WMMat = _pObj->GetPositionMatrix();
			CXW_Surface *pSurf = _pWClient->GetMapData()->GetResource_Surface(iWMSurface);
			int WMSurfaceID = pSC->GetSurfaceID(pSurf->m_Name);

			CVec3Dfp32 Out =	-CVec3Dfp32::GetMatrixRow(WMMat, 0);
			CVec3Dfp32 Up = CVec3Dfp32::GetMatrixRow(WMMat, 1);
			CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(WMMat, 3);

			Out.SetRow(WMMat, 2);
			Up.SetRow(WMMat, 0);
			WMMat.RecreateMatrix(2, 0);
			Pos.SetMatrixRow(WMMat, 3);

//			fp32 AnimTime = _pWClient->GetGameTick() * _pWClient->GetGameTickTime() + _pWClient->GetInterpolateTime();

			CXR_WallmarkDesc WMD;
			WMD.m_SpawnTime = _pWClient->GetRenderTime();
			WMD.m_SurfaceID = WMSurfaceID;
			WMD.m_Size = WMSize;
			fp32 Tolerance = _pObj->m_iAnim2;
			if(Tolerance == 0)
				Tolerance = WMD.m_Size;
			_pWClient->Wallmark_Create(WMD, WMMat, Tolerance, XR_WALLMARK_NEVERDESTROY);

			if (bSpawnOnce)
				_pWClient->Object_Destroy(_pObj->m_iObject);
		}
	}

	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
	{
		MAUTOSTRIP(CWObject_Wallmark_OnClientRender, MAUTOSTRIP_VOID);
	}

	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags)
	{
		_pFile->ReadLE(m_iAnim1);
	}

	virtual void OnDeltaSave(CCFile* _pFile)
	{
		_pFile->WriteLE(m_iAnim1);
	}
};

//----------------------------------------------------------------------

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Wallmark, CWObject_Model, 0x0100);

//----------------------------------------------------------------------
