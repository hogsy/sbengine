#ifndef __WDATARES_ANIMGRAPH2_H
#define __WDATARES_ANIMGRAPH2_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Animation-Graph2 resources classes

	Author:			Olle Rosenquist

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWRes_AnimGraph2
\*____________________________________________________________________________________________*/

#include "../../XR/XRAnimGraph2/AnimGraph2.h"
#include "WData.h"

//--------------------------------------------------------------------------------

typedef TPtr<class CWRes_AnimGraph2> spCWRes_AnimGraph2;
class CWRes_AnimGraph2 : public CWResource
{
	MRTC_DECLARE;

	private:

		TPtr<CXRAG2>			m_spAnimGraph;
		CWorldData*				m_pWData;

	public:

		virtual CXRAG2* GetAnimGraph();
		virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
		virtual int IsLoaded();
		virtual void OnLoad();
		virtual void OnUnload();
		virtual void OnPrecache(class CXR_Engine* _pEngine);
};

//--------------------------------------------------------------------------------

#endif /* WDataRes_AnimGraph2_h */
