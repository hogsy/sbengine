#include "PCH.h"
#include "XRVBContainer.h"
#include "XRVBContext.h"

void CXR_VBContainer::VBC_Add()
{
	if (m_iVBC >= 0)
		Error_static("CXR_VBContainer::VBC_Add", "Already added.");

	m_iVBC = m_pVBCtx->AddVBContainer(this);
}

void CXR_VBContainer::VBC_Remove()
{
	if (m_iVBC < 0)
		Error_static("CXR_VBContainer::VBC_Add", "Already removed.");

	m_pVBCtx->RemoveVBContainer(m_iVBC);
}

CXR_VBContainer::CXR_VBContainer()
{
	m_iVBC = -1;

	// Get VBContext
	MACRO_GetRegisterObject(CXR_VBContext, pVBCtx, "SYSTEM.VBCONTEXT");
	if (!pVBCtx) Error_static("CXR_VBContainer::-", "No texture-context available.");
	m_pVBCtx = pVBCtx;

	VBC_Add();
}

CXR_VBContainer::~CXR_VBContainer()
{
//LogFile("Destroying TC.");
	if (m_iVBC >= 0)
		VBC_Remove();

//LogFile("Destroying TC..");
	m_pVBCtx = NULL;
	m_iVBC = -1;
//LogFile("Destroying TC...");
}

void CXR_VBContainer::OnRefresh()
{
}

void CXR_VBContainer::Release(int _iLocal)
{
}
