/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_Cloth.h

	Author:			Anton Ragnarsson

	Copyright:		Copyright Starbreeze AB 2005

	Contents:		CWObject_Cloth

	History:		
		050426:		Created File
\*____________________________________________________________________________________________*/
#ifndef __WObj_Cloth_h__
#define __WObj_Cloth_h__

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_Cloth_ClientData
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWO_Cloth_ClientData : public CReferenceCount, public CAutoVarContainer
{
	AUTOVAR_SETCLASS(CWO_Cloth_ClientData, CAutoVarContainer);
public:
	CAUTOVAR_OP(CAutoVar_uint8, m_Temp, DIRTYMASK_0_0);

	AUTOVAR_PACK_BEGIN
	AUTOVAR_PACK_VAR(m_Temp)
	AUTOVAR_PACK_END

	TArray<CXR_SkeletonCloth> m_lCloth;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Cloth
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWObject_Cloth : public CWObject_Model
{
	typedef CWObject_Model parent;
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	static const CWO_Cloth_ClientData& GetClientData(const CWObject_CoreData* _pObj);
	static       CWO_Cloth_ClientData& GetClientData(CWObject_CoreData* _pObj);

	virtual void OnRefresh();
	virtual int OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const;

	static void OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	static int  OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
};

#endif // __WObj_Cloth_h__
