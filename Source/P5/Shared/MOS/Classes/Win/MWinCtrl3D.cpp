
#include "PCH.h"
#include "MWinCtrl3D.h"
/*
// ----------------------------------------------------------------
//  CMWnd_Viewport
// ----------------------------------------------------------------
CMWnd_Viewport::CMWnd_Viewport()
{
	MAUTOSTRIP(CMWnd_Viewport_ctor, MAUTOSTRIP_VOID);
	m_Viewport.SetFOV(90);
	m_Viewport.SetBackPlane(3000);
}

CMWnd_Viewport::CMWnd_Viewport(CMWnd* _pWndParent, const CMWnd_Param& _Param, CStr _ID) :
	CMWnd(_pWndParent, _Param, _ID)
{
	MAUTOSTRIP(CMWnd_Viewport_ctor_with_args, MAUTOSTRIP_VOID);
	m_Viewport.SetFOV(90);
	m_Viewport.SetBackPlane(3000);
}


void CMWnd_Viewport::Render(CRenderContext* _pRC)
{
	MAUTOSTRIP(CMWnd_Viewport_Render, MAUTOSTRIP_VOID);
}

void CMWnd_Viewport::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	MAUTOSTRIP(CMWnd_Viewport_OnPaint, MAUTOSTRIP_VOID);
	CRenderContext* pRC = _pRCUtil->GetRC();

	Error_static("CMWnd_Viewport::OnPaint", "Direct manipulation of RenderContext doesn't work");

	_pRCUtil->SetTexture(0);
	_pRCUtil->Rect(_Clip, _Client, m_Col_Background);

	CRct Client = _Client;
	Client += _Clip.ofs;
	m_Viewport.SetView(_Clip, Client);

	CRct VP = m_Viewport.GetViewArea();

	_pRCUtil->GetVBM()->Viewport_Push(&m_Viewport);
	try 
	{
		pRC->Matrix_Push();
		CMat4Dfp32 Mat; Mat.Unit();
		pRC->Matrix_Set(Mat);
		Render(pRC); 
		pRC->Matrix_Pop();
	}
	catch(CCException) 
	{ 
		pRC->Matrix_Pop();
		_pRCUtil->GetVBM()->Viewport_Pop();
		throw; 
	};
	_pRCUtil->GetVBM()->Viewport_Pop();
}

MRTC_IMPLEMENT_DYNAMIC(CMWnd_Viewport, CMWnd_Window);

// ----------------------------------------------------------------
//  CMWnd_XMDView
// ----------------------------------------------------------------
void CMWnd_XMDView::ScanTextureContainers(CStr _Path)
{
	MAUTOSTRIP(CMWnd_XMDView_ScanTextureContainers, MAUTOSTRIP_VOID);
	CDirectoryNode Dir;
//LogFile("(CWorldData::ScanTextureContainers) Reading directory " + _Path +"*.*");
	Dir.ReadDirectory(_Path + "*");

	int nFiles = Dir.GetFileCount();
	for(int i = 0; i < nFiles; i++)
	{
		CDir_FileRec* pRec  = Dir.GetFileRec(i);
		if (pRec->IsDirectory())
		{
			if (pRec->m_Name.Copy(0,1) != ".")
				ScanTextureContainers(_Path + pRec->m_Name + "\\");
		}
		else //if (pRec->IsArchive())
		{
			if (pRec->m_Ext.CompareNoCase("XTC") == 0)
			{
				try 
				{ 
					spCTextureContainer_VirtualXTC spTC = MNew(CTextureContainer_VirtualXTC);
					if (!spTC) MemError("ScanTextureContainers");
					spTC->Create(_Path + pRec->m_Name);
					m_lspTC.Add(spTC);
				}
				catch(...) { ConOutL("§cf80WARNING: Failure reading texture-container: " + _Path + pRec->m_Name); }
			}
		}
	}
}

CMWnd_XMDView::CMWnd_XMDView()
{
	MAUTOSTRIP(CMWnd_XMDView_ctor, MAUTOSTRIP_VOID);
//		m_spTC = DNew(CTextureContainer_VirtualXTC) CTextureContainer_VirtualXTC("OBJTXT.XTC");

	m_ObjRot = 0;

	m_Camera.Unit();
	m_Camera.M_x_RotX(0.25f);
	m_Camera.M_x_RotZ(0.5f);

	m_Camera.k[3][2] = 64.0f;

	m_RenderMode = 1;
}

void CMWnd_XMDView::OnCreate()
{
	MAUTOSTRIP(CMWnd_XMDView_OnCreate, MAUTOSTRIP_VOID);
//		m_WLS.AddDynamic(CVec3Dfp32(80,-10,0), CVec3Dfp32(2.5,2.5,0.6), 512);
//		m_WLS.AddDynamic(CVec3Dfp32(-80,-20,0), CVec3Dfp32(1.5,1.5,3.5), 512);
//		m_WLS.AddDynamic(CVec3Dfp32(80,-10,0), CVec3Dfp32(0.7,0.7,0.4), 512);
//		m_WLS.AddDynamic(CVec3Dfp32(-80,-20,0), CVec3Dfp32(0.4,0.4,0.8), 512);
	
	m_spWLS = MNew(CXR_WorldLightState);
	if (!m_spWLS)
		MemError("OnCreate");
	m_spWLS->Create(256, 4, 0);

	m_spWLS->AddDynamic(CVec3Dfp32(80,40,-10), CVec3Dfp32(0.6f,0.6f,0.6f), 512);
	m_spWLS->AddDynamic(CVec3Dfp32(-40,80,-20), CVec3Dfp32(0.8f,0.8f,0.8f), 512);

	MRTC_SAFECREATEOBJECT_NOEX(spVBM, "CXR_VBManager", CXR_VBManager);
	m_spVBM = spVBM;
	if (!m_spVBM) Error("OnCreate", "No CXR_VBManager available.");
	m_spVBM->SetOwner(MRTC_SystemInfo::OS_GetThreadID());
	m_spVBM->Create(128*1024, 256);
}

void CMWnd_XMDView::SetModel(CStr _FileName)
{
	MAUTOSTRIP(CMWnd_XMDView_SetModel, MAUTOSTRIP_VOID);
	ScanTextureContainers(_FileName.GetPath());
	MRTC_SAFECREATEOBJECT_NOEX(spModel, "CXR_Model_TriangleMesh", CXR_Model_TriangleMesh);
	if (!spModel) Error("SetModel", "CXR_Model_TriangleMesh is not a registered class of the process.");
	m_spModel = spModel;
	m_spModel->Read(_FileName);
}

aint CMWnd_XMDView::OnMessage(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_XMDView_OnMessage, 0);
	switch(_pMsg->m_Msg)
	{
		case WMSG_KEY :
			{
				LogFile(CStrF("(CMWnd_XMDView::OnMessage) Key: %.4x", _pMsg->m_Param0));
				switch(_pMsg->m_Param0)
				{
					case SKEY_MODIFIER_CONTROL + SKEY_A+8 : m_RenderMode ^= 4; break;		// Ctrl+L, Lighting toggle
					case SKEY_MODIFIER_CONTROL + SKEY_Q+1 : m_RenderMode ^= 2; break;		// Ctrl+W, Wire toggle
					case SKEY_MODIFIER_CONTROL + SKEY_A+1 : m_RenderMode ^= 1; break;		// Ctrl+S, Solid toggle
					default : return CMWnd_Viewport::OnMessage(_pMsg);
				}
			};
			return true;
		default : 
			return CMWnd_Viewport::OnMessage(_pMsg);
	}
//		CMWnd_Viewport::OnMessage(_pMsg);
}

void CMWnd_XMDView::Render(CRenderContext* _pRC)
{
	MAUTOSTRIP(CMWnd_XMDView_Render, MAUTOSTRIP_VOID);
	_pRC->Attrib_Push();
	_pRC->Attrib_Enable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
	_pRC->Attrib_ZCompare(CRC_COMPARE_LESSEQUAL);

	m_Pos.Unit();
	m_ObjRot.CreateMatrixFromAngles(5, m_Pos);

	if (m_spModel!=NULL)
	{
		m_spVBM->ScopeBegin(true);
		if (m_RenderMode & 4)
			m_spModel->SetRenderFlags(m_spModel->GetRenderFlags() | CTM_RFLAGS_NOLIGHTING);

		if (m_RenderMode & 1)
			m_spModel->OnRender(NULL, _pRC, m_spVBM, NULL, m_spWLS, &m_AnimState, m_Pos, m_Camera, 0);

		if (m_RenderMode & 2)
		{
			m_spModel->SetRenderFlags(m_spModel->GetRenderFlags() | CTM_RFLAGS_WIRE);
			m_spModel->OnRender(NULL, _pRC, m_spVBM, NULL, m_spWLS, &m_AnimState, m_Pos, m_Camera, 0);
			m_spModel->SetRenderFlags(m_spModel->GetRenderFlags() & ~CTM_RFLAGS_WIRE);
		}
		m_spModel->SetRenderFlags(m_spModel->GetRenderFlags() & ~CTM_RFLAGS_NOLIGHTING);
		m_spVBM->ScopeEnd();
	}

	_pRC->Attrib_Pop();
}

MRTC_IMPLEMENT_DYNAMIC(CMWnd_XMDView, CMWnd_Viewport);
*/
