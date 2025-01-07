#include "PCH.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_SimpleMessage.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_AutoVar.h"
#include "WObj_Trigger.h"
#include "../WObj_Char.h"


// -------------------------------------------------------------------
//  CWObject_MessageBranch
// -------------------------------------------------------------------
class CWObject_TeleportController : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	
	CWObject_TeleportController()
	{
		MAUTOSTRIP(CWObject_TeleportController_ctor, MAUTOSTRIP_VOID);
	}
	
	virtual aint OnMessage(const CWObject_Message& _Msg)
	{
		MAUTOSTRIP(CWObject_TeleportController_OnMessage, 0);

		if(_Msg.m_Msg == OBJMSG_TELEPORTCONTROLLER_TELEPORTOBJECT)
		{
			const char *pName = (const char *)_Msg.m_pData;
			CWObject *pObj = NULL;
			if(pName != "")
			{
				if (pName == "$activator")
				{
					//Use activator, which should be the supplied onject ID
					pObj = m_pWServer->Object_Get(_Msg.m_iSender);
				}
				
				else if (pName == "$player")
				{
					//Find closest player to sending object (if any)
					fp32 DistSqr;
					fp32 MinDistSqr = _FP32_MAX;
					CWObject * pChar;
					CWObject * pMinChar = NULL;
					CWObject *pSender = m_pWServer->Object_Get(_Msg.m_iSender);
					if(pSender)
					{
						CWObject_Game *pGame = m_pWServer->Game_GetObject();
						for (int i = 0; i < pGame->Player_GetNum(); i++)
						{
							if ((pChar = pGame->Player_GetObject(i)) &&											
								((DistSqr = pSender->GetPosition().DistanceSqr(pChar->GetPosition())) < MinDistSqr))	
							{
								pMinChar = pChar;
								MinDistSqr = DistSqr;
							};
						};
					}
					pObj = pMinChar;
				}
				else
				{
					//Find all objects with the given name
					int iSel;
					const int16* piObj;
					int nObj;
					
					//Set up selection
					try
					{
						TSelection<CSelection::LARGE_BUFFER> Selection;
					}
					catch (CCException) //Assume this is stack overflow... 
					{
						return 0;
					};
					//Add objects
					m_pWServer->Selection_AddTarget(Selection, pName);
					nObj = m_pWServer->Selection_Get(Selection, &piObj);
					m_pWServer->Selection_Pop();
					
					//Shuffle through objects until best random candidate is found
					if (nObj == 0)
						return 0;
					else if (nObj == 1)
						pObj = m_pWServer->Object_Get(piObj[0]);
					else
					{
						// Get a random object.
						int Rnd = (int)(Random * nObj * 0.9999f);
						pObj = m_pWServer->Object_Get(piObj[Rnd]);					
					};
				}
				if(pObj)
				{										
					//Look in the direction of the engine path object
					int NumChild = m_pWServer->Object_GetNumChildren(m_iObject);
					if(!NumChild)
					{
						CMat43fp32 Pos;
						Pos = GetPositionMatrix();
						
						CWO_PhysicsState Phys;
						Phys.m_MediumFlags |= XW_MEDIUM_PLAYERSOLID;
						Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, CVec3Dfp32(13.0f, 13.0f, 32.0f), CVec3Dfp32(0,0,32.0f), 1.0f);
						Phys.m_nPrim = 1;
						Phys.m_ObjectFlags = OBJECT_FLAGS_CHARACTER;
						Phys.m_ObjectIntersectFlags = OBJECT_FLAGS_CHARACTER;
						
						
						Pos.k[3][2] += 1.0f;
						
						TSelection<CSelection::LARGE_BUFFER> Selection;
						m_pWServer->Selection_AddIntersection(Selection, Pos, Phys);
						int nSelObj = m_pWServer->Selection_Get(Selection);
						m_pWServer->Selection_Pop();
						
						if (!nSelObj)
						{
/*							
							CWObject_Character* pChar = TDynamicCast<CWObject_Character>((CWObject_Character*)pObj);
							if (pChar != NULL)
							{
								pChar->TeleportTo(CVec3Dfp32::GetMatrixRow(Pos, 3), CVec3Dfp32::GetMatrixRow(Pos, 0));
							}
							else
*/
							{
								m_pWServer->Object_SetPosition(pObj->m_iObject, CVec3Dfp32::GetMatrixRow(Pos, 3));
							}
						}							
					}
					else
					{
						bool FoundPos = false;
						int nIters = NumChild;
						//Set up check array
						TArray<bool> lbChecked;
						lbChecked.QuickSetLen(NumChild);
						int i;
						for (i = 0; i < NumChild; i++) lbChecked[i] = false;
						
						int Rnd;
						while(!FoundPos && nIters > 0)
						{
							//Find random non-checked object index
							Rnd = (int)(Random * NumChild * 0.9999f);
							// if this index has been checked loop over all indexes and find an unchecked index.
							if(lbChecked[Rnd])
							{
								int j = 0;
								while (j<NumChild && lbChecked[j])
									 j++;
								Rnd = j;
								if(lbChecked[Rnd])
									return 0;
							}
							lbChecked[Rnd] = true;
							

							int iChild = m_pWServer->Object_GetFirstChild(m_iObject);
							while(iChild && Rnd >0)
							{
								int iNext = m_pWServer->Object_GetNextChild(iChild);
								iChild = iNext;
								Rnd--;
							}
							CWObject *pTeleportTo =  m_pWServer->Object_Get(iChild);
							CMat43fp32 Pos;
							if (pTeleportTo)
							{
								Pos = pTeleportTo->GetPositionMatrix();
								
								CWO_PhysicsState Phys;
								Phys.m_MediumFlags |= XW_MEDIUM_PLAYERSOLID;
								Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, CVec3Dfp32(13.0f, 13.0f, 32.0f), CVec3Dfp32(0,0,32.0f), 1.0f);
								Phys.m_nPrim = 1;
								Phys.m_ObjectFlags = OBJECT_FLAGS_CHARACTER;
								Phys.m_ObjectIntersectFlags = OBJECT_FLAGS_CHARACTER;
								
								
								Pos.k[3][2] += 1.0f;
								
								TSelection<CSelection::LARGE_BUFFER> Selection;
								m_pWServer->Selection_AddIntersection(Selection, Pos, Phys);
								int nSelObj = m_pWServer->Selection_Get(Selection);
								m_pWServer->Selection_Pop();
								
								if (!nSelObj)
								{
									FoundPos = true;
/*
									CWObject_Character* pChar = TDynamicCast<CWObject_Character>((CWObject_Character*)pObj);
									if (pChar != NULL)
									{
										pChar->TeleportTo(CVec3Dfp32::GetMatrixRow(Pos, 3), CVec3Dfp32::GetMatrixRow(Pos, 0));
									}
									else
*/
									{
										m_pWServer->Object_SetPosition(pObj->m_iObject, CVec3Dfp32::GetMatrixRow(Pos, 3));
									}

									pTeleportTo->OnMessage(CWObject_Message(OBJMSG_IMPULSE, 1, 0, m_iObject));									
								}							
								
							}
							nIters--;
						}
					}
				}
			}
			return 1;
		}
		else
			return CWObject::OnMessage(_Msg);
	}
	
	virtual void OnRefresh()
	{
		MAUTOSTRIP(CWObject_TeleportController_OnRefresh, MAUTOSTRIP_VOID);
		CWObject::OnRefresh();
	}
};

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_TeleportController, CWObject, 0x0100);