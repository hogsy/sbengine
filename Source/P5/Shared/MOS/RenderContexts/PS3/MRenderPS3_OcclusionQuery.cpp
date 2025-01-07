
#include "PCH.h"
#include "MRenderPS3_Context.h"

void CRenderContextPS3::OcclusionQuery_Init()
{
	for(int i = 0; i < OCID_HASHFRAMES; i++)
		m_OC_lQueryIDHash[i].Create(128, OCID_HASHNUM, false);

	m_OC_iHashQuery = 0;
	m_OC_iHashInsert = 1;
}

void CRenderContextPS3::OcclusionQuery_PrepareFrame()
{
	m_OC_iHashQuery = m_OC_iHashInsert;
	m_OC_iHashInsert++;
	if (m_OC_iHashInsert >= OCID_HASHFRAMES)
		m_OC_iHashInsert = 0;

	m_OC_lQueryIDHash[m_OC_iHashInsert].Create(128, OCID_HASHNUM, false);
	m_OC_iHashOCIDNext[m_OC_iHashInsert] = 1;
}

int CRenderContextPS3::OcclusionQuery_AddQueryID(int _QueryID)
{
	if (m_OC_iHashOCIDNext[m_OC_iHashInsert] >= 128)
		return 0;

	m_OC_lQueryIDHash[m_OC_iHashInsert].Insert(m_OC_iHashOCIDNext[m_OC_iHashInsert], _QueryID);

	int OCID = m_OC_iHashOCIDNext[m_OC_iHashInsert] + m_OC_iHashInsert*128;
	m_OC_iHashOCIDNext[m_OC_iHashInsert]++;

	return OCID;
}

int CRenderContextPS3::OcclusionQuery_FindOCID(int _QueryID)
{
	return m_OC_lQueryIDHash[m_OC_iHashQuery].GetOCID(_QueryID) + m_OC_iHashQuery*128;
}

void CRenderContextPS3::OcclusionQuery_Begin(int _QueryID)
{
	int OCID = OcclusionQuery_AddQueryID(_QueryID);
	if (!OCID)
		Error("OcclusionQuery_Begin", "Fixme!");

	if (m_AttribChanged) Attrib_Update();
	if (m_MatrixChanged) Matrix_Update();

	glBeginQueryARB(GL_SAMPLES_PASSED_ARB, OCID);
	GLESErr("OcclusionQuery_Begin");
}

void CRenderContextPS3::OcclusionQuery_End()
{
	// Fix: _QueryID not necessary

	glEndQueryARB(GL_SAMPLES_PASSED_ARB);
	GLESErr("OcclusionQuery_End");
}

int CRenderContextPS3::OcclusionQuery_GetVisiblePixelCount(int _QueryID)
{
	int OCID = OcclusionQuery_FindOCID(_QueryID);
	if (OCID < 0)
		return 0;

	if (!glIsQueryARB(OCID))
		return 0;

	int bIsQueryAvail = 0;
	glGetQueryivARB(OCID, GL_QUERY_RESULT_AVAILABLE_ARB, &bIsQueryAvail);
	if (!bIsQueryAvail)
	{
		CMTime T;
		TStart(T);

		while(!bIsQueryAvail)
		{
			glGetQueryivARB(OCID, GL_QUERY_RESULT_AVAILABLE_ARB, &bIsQueryAvail);
		}

		TStop(T);

		if (T.GetTime() > 0.001f)
			ConOutL(CStrF("(GL) Block on occlusion query: %.2f ms.", T.GetTime()*1000.0));
	}

	int nPixels = 0;
	glGetQueryivARB(OCID, GL_QUERY_RESULT_ARB, &nPixels);
//ConOut(CStrF("Query %d, VisPixels %d", _QueryID, nPixels));
	return nPixels;
}

