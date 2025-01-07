#ifndef __FRONTEND_P6_H__
#define __FRONTEND_P6_H__

#include "WFrontEndMod.h"

class CWFrontEnd_P6 : public CWFrontEnd_Mod
{
	MRTC_DECLARE;
private:
public:
	CWFrontEnd_P6();

	virtual void OnPrecache(class CXR_Engine* _pEngine);
	virtual void Cube_Render(class CRenderContext* _pRC, class CXR_VBManager* _pVBM, bool _IsJournal = false);
	virtual void OnRender(class CRenderContext* _pRC, class CXR_VBManager* _pVBM);

	CStr	m_BG;
};

#endif

