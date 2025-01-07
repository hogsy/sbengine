/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_Television.h

	Author:			Anton Ragnarsson

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_Television

	Comments:		

	History:		
		050811:		Created File
\*____________________________________________________________________________________________*/
#ifndef __WObj_Television_h__
#define __WObj_Television_h__

#include "WObj_Object.h"



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Television
|
| Data[2] = TV Surface ID | FlushTextureID
| Data[4] = TV Sound ID
| Data[6] = AttnMin | AttnMax
| Data[7] = ViewMax | Volume
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWObject_Television : public CWObject_Object
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	typedef CWObject_Object parent;

	TArray<int16> m_liSurfaces;
	TArray<int16> m_liSounds;
	uint m_iChannel;
	fp32 m_AttnMin;
	fp32 m_AttnMax;
	fp32 m_ViewMax;
	fp32 m_Volume;

	int m_ChannelChangeTicks;

	void UpdateClient();
	virtual void ObjectSpawn(bool _bSpawn);

	static int GetTextureFromSurface(CWorld_Client* _pWClient, int _iSurface);

public:
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);
	virtual void OnRefresh();

	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine*, const CMat4Dfp32& _ParentMat);
	static int  OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);
};

#endif // __WObj_Television_h__
