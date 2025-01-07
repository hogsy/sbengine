
#ifndef _INC_MWINCLIENTWNDMAPMOD
#define _INC_MWINCLIENTWNDMAPMOD

#include "WClientModWnd.h"
#include "MWinCtrlMod.h"

// -------------------------------------------------------------------
//  Phobos: CMWnd_ModMissionMap
// -------------------------------------------------------------------
#define CMWnd_ModMissionMapParent CMWnd_ModTexture
class CMWnd_ModMissionMap : public CMWnd_ModMissionMapParent
{
	MRTC_DECLARE;
protected:
	// x = 0 in real world = CenterX on map
	fp32		m_CenterX;
	// y = 0 in real world = CenterY on map
	fp32		m_CenterY;

	// Scale of coordinate in x,y direction
	fp32		m_ScaleX;
	fp32		m_ScaleY;
	// So final pos (x,y) = (m_CenterX + RealX * m_ScaleX, m_CenterY + RealY * m_ScaleY)

	// Sprite for positioning pointer thingy....
	CStr	m_PointerName;
	CPnt	m_PointerRange;				
	fp32		m_AngleOffset;
public:
	CMWnd_ClientInfo m_ClientInfo;
	CMWnd_ClientInfo *GetClientInfo()
	{
		return &m_ClientInfo;
	}

	CMWnd_ModMissionMap();
	virtual void EvaluateKey(CMWnd_Param* _pParam, CStr _Key, CStr _Value);
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
	virtual void PaintTexture(CRC_Util2D* _pRCUtil, CClipRect _Clip, CRct _Text);
};

#endif
