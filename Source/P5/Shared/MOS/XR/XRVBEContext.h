#ifndef _INC_XRVBECONTEXT
#define _INC_XRVBECONTEXT

#include "XRVertexBuffer.h"

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Vertex buffer explorer context

	Author:			Magnus Högdahl

	Copyright:		Starbreeze Studios, 2006

\*____________________________________________________________________________________________*/

enum
{
	VBE_MAXSCOPES = 64,
};

class CXR_VBEContext
{
public:
	class CScope
	{
	public:
		int m_nVB;
	};

	CScope m_lScopeInfo[VBE_MAXSCOPES];
	int m_nScopes;

	uint m_bInputEnable : 1;
	uint m_bShowTextures : 1;
	uint m_bShowSelection : 1;
	uint m_bShowAsWire : 1;
	uint m_bShowWithClip : 1;
	uint m_bShowWithZCompare : 2;
	uint m_bShowGPUTime : 1;
	uint m_bShowVBChain : 1;
	uint m_bRenderToSelection : 1;
	uint m_bQuickStep : 1;
	int m_iScope;
	int m_iVB;
	class CRC_Font* m_pFont;
	class CRC_Util2D* m_pUtil2D;

	CPnt m_ScreenSize;
	CRct m_GUIRect;

	CXR_VertexBuffer* m_pCurrentVB;
	CXR_VertexBuffer m_CurrentVB;
	CRC_Attributes m_CurrentAttr;

	CXR_VBEContext();

	void ClearVB()
	{
		m_pCurrentVB = NULL;
		m_CurrentVB.Clear();
		m_CurrentAttr.Clear();
	}

	void SetScreenSize(class CDisplayContext* _pDC);

	void Step(int _nStep);
	bool ProcessKey(CScanKey _Key);
};

#endif // _INC_XRVBECONTEXT
