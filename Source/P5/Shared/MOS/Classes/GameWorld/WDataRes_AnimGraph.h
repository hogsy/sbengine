#ifndef __WDATARES_ANIMGRAPH_H
#define __WDATARES_ANIMGRAPH_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Animation-graph resources classes

	Author:			David Mondelore

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWRes_AnimGraph
					CWRes_AGAnimList
\*____________________________________________________________________________________________*/

#include "../../XR/XRAnimGraph/AnimGraph.h"
#include "../../XR/XRAnimGraph/AnimList.h"
#include "WData.h"

//--------------------------------------------------------------------------------

typedef TPtr<class CWRes_AnimGraph> spCWRes_AnimGraph;
class CWRes_AnimGraph : public CWResource
{
	MRTC_DECLARE;

	private:

		TPtr<CXRAG>		m_spAnimGraph;
		CWorldData*				m_pWData;

	public:

		virtual CXRAG* GetAnimGraph();
		virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
		virtual int IsLoaded();
		virtual void OnLoad();
		virtual void OnUnload();
};

//--------------------------------------------------------------------------------

typedef TPtr<class CWRes_AGAnimList> spCWRes_AGAnimList;
class CWRes_AGAnimList : public CWResource
{
	MRTC_DECLARE;

	private:

		TPtr<CXRAG_AnimList>			m_spAnimList;

		CMapData*						m_pMapData;
		CWorldData*						m_pWData;

	public:

		virtual CXRAG_AnimList* GetAnimList();
		virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
		virtual int IsLoaded();
		virtual void OnPrecache(class CXR_Engine* _pEngine);
		void ParseReg();
};

//--------------------------------------------------------------------------------

#endif /* WDataRes_AnimGraph_h */
