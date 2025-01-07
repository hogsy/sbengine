#ifndef __MWINCTRL3D_H
#define __MWINCTRL3D_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Misc 3d-window classes

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CMWnd_Viewport
					CMWnd_XMDView
\*____________________________________________________________________________________________*/

#include "MWindows.h"
#include "MWndCtrlEdit.h"
#include "../../XRModels/Model_TriMesh/WTriMesh.h"
/*
// ----------------------------------------------------------------
//  CMWnd_Viewport
// ----------------------------------------------------------------
class CMWnd_Viewport : public CMWnd
{
	MRTC_DECLARE;
public:
	CRC_Viewport m_Viewport;

	CMWnd_Viewport();
	CMWnd_Viewport(CMWnd* _pWndParent, const CMWnd_Param& _Param, CStr _ID);
	virtual void Render(CRenderContext* _pRC);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

// ----------------------------------------------------------------
//  CMWnd_XMDView
// ----------------------------------------------------------------
class CMWnd_XMDView : public CMWnd_Viewport
{
	MRTC_DECLARE;

protected:
	spCXR_Model_TriangleMesh m_spModel;
	spCXR_VBManager m_spVBM;
	spCXR_WorldLightState m_spWLS;
	CMat4Dfp32 m_Pos;
	CMat4Dfp32 m_Camera;
	int m_RenderMode;

	CVec3Dfp32 m_ObjRot;
	CXR_AnimState m_AnimState;

//	spCTextureContainer_Plain m_spTC;
	TArray<spCTextureContainer_VirtualXTC> m_lspTC;

	void ScanTextureContainers(CStr _Path);
public:
	CMWnd_XMDView();
	virtual void OnCreate();
	void SetModel(CStr _FileName);
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
	virtual void Render(CRenderContext* _pRC);
};
*/
#endif	// __INC_MWINCTRL3D
