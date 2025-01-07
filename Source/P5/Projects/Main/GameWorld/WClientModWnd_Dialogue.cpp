#include "PCH.h"
#include "MWinCtrlMod.h"
#include "WClientModWnd.h"

#include "../../GameClasses/WObj_CharMsg.h"

// -------------------------------------------------------------------
//  CMWnd_ModDialogue
// -------------------------------------------------------------------
class CMWnd_ModDialogue : public CMWnd
{
	MRTC_DECLARE;
	
public:
	int m_MouseRel;
	
	CMWnd_ClientInfo m_ClientInfo;
	CMWnd_ClientInfo *GetClientInfo()
	{
		return &m_ClientInfo;
	}

	CMWnd_ModDialogue()
	{
		m_MouseRel = 0;
	}
	
	virtual aint OnMessage(const CMWnd_Message* _pMsg)
	{
		switch(_pMsg->m_Msg)
		{
		case WMSG_KEY :
			{
				CScanKey Key;
				Key.m_ScanKey32 = _pMsg->m_Param0;
				Key.m_Char = _pMsg->m_Param1;
				Key.m_Data[0] = int(_pMsg->m_fParam0);
				Key.m_Data[1] = int(_pMsg->m_fParam1);
				CScanKey OriginalKey;
				OriginalKey.m_ScanKey32 = _pMsg->m_Param2;
				OriginalKey.m_Char = _pMsg->m_Param3;

				if (Key.IsDown())
				{
					MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
					if(!pCon)
						return 0;

					if(Key.GetKey9() == SKEY_GUI_UP)
						m_ClientInfo.m_pClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_CHAR_NEXTDIALOGUECHOICE, 1), m_ClientInfo.m_pClient->Player_GetLocalObject());
//						pCon->ExecuteString("cmd(prevdialogueitem)");
					
					else if(Key.GetKey9() == SKEY_GUI_DOWN)
						m_ClientInfo.m_pClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_CHAR_NEXTDIALOGUECHOICE, 0), m_ClientInfo.m_pClient->Player_GetLocalObject());
//						pCon->ExecuteString("cmd(nextdialogueitem)");

					else if(Key.GetKey9() == SKEY_GUI_OK || Key.GetKey9() == SKEY_GUI_BUTTON2)
						m_ClientInfo.m_pClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_CHAR_SELECTDIALOGUECHOICE), m_ClientInfo.m_pClient->Player_GetLocalObject());
//						pCon->ExecuteString("cmd(selectdialogueitem)");

					else if(Key.GetKey9() == SKEY_MOUSEMOVEREL)
					{
						m_MouseRel += Key.GetPosY();
						if(m_MouseRel < -30)
						{
							m_MouseRel = 0;
							m_ClientInfo.m_pClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_CHAR_NEXTDIALOGUECHOICE, 1), m_ClientInfo.m_pClient->Player_GetLocalObject());
						}
						else if(m_MouseRel > 30)
						{
							m_MouseRel = 0;
							m_ClientInfo.m_pClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_CHAR_NEXTDIALOGUECHOICE, 0), m_ClientInfo.m_pClient->Player_GetLocalObject());
						}
					}

/*#if !defined(M_RTM) || defined(M_Profile)
					else if(Key.GetKey9() == SKEY_E || Key.GetKey9() == SKEY_GUI_BUTTON5)
						pCon->ExecuteString("cmd(enddialogue)");
#endif*/

					return 1;
				}
			}
		}

		return CMWnd::OnMessage(_pMsg);
	}
};

MRTC_IMPLEMENT_DYNAMIC(CMWnd_ModDialogue, CMWnd)

class CMWnd_ModTelephone : public CMWnd
{
	MRTC_DECLARE;

public:
	CVec2Dint32 m_MouseRel;

	CMWnd_ClientInfo m_ClientInfo;
	CMWnd_ClientInfo *GetClientInfo()
	{
		return &m_ClientInfo;
	}

	CMWnd_ModTelephone()
	{
		m_MouseRel = 0;
	}

	virtual aint OnMessage(const CMWnd_Message* _pMsg)
	{
		switch(_pMsg->m_Msg)
		{
		case WMSG_KEY :
			{
				CScanKey Key;
				Key.m_ScanKey32 = _pMsg->m_Param0;
				Key.m_Char = _pMsg->m_Param1;
				Key.m_Data[0] = int(_pMsg->m_fParam0);
				Key.m_Data[1] = int(_pMsg->m_fParam1);
				CScanKey OriginalKey;
				OriginalKey.m_ScanKey32 = _pMsg->m_Param2;
				OriginalKey.m_Char = _pMsg->m_Param3;

				if (Key.IsDown())
				{
					MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
					if(!pCon)
						return 0;
					enum Types
					 {
						LEFT	= 1,
						RIGHT	= 2,
						UP		= 3,
						DOWN	= 4,
						SELECT	= 5,
						QUIT	= 6,
					 };

					int32 Type = 0;

					switch (Key.GetKey9())
					{
					case SKEY_GUI_LEFT:
						{
							Type = LEFT;
							break;
						}
					case SKEY_GUI_RIGHT:
						{
							Type = RIGHT;
							break;
						}
					case SKEY_GUI_UP:
						{
							Type = UP;
							break;
						}
					case SKEY_GUI_DOWN:
						{
							Type = DOWN;
							break;
						}
					case SKEY_GUI_OK:
						{
							Type = SELECT;
							break;
						}
					case SKEY_GUI_BUTTON2:
						{
							Type = SELECT;
							break;
						}
					case SKEY_MOUSEMOVEREL:
						{
							m_MouseRel.k[0] += Key.GetPosX();
							m_MouseRel.k[1] += Key.GetPosY();
							if(m_MouseRel.k[0] < -30)
							{
								m_MouseRel.k[0] = 0;
								Type = LEFT;
							}
							else if(m_MouseRel[0] > 30)
							{
								m_MouseRel.k[0] = 0;
								Type = RIGHT;
							}
							else if (m_MouseRel[1] < -30)
							{
								m_MouseRel.k[1] = 0;
								Type = UP;
							}
							else if (m_MouseRel[1] > 30)
							{
								m_MouseRel.k[1] = 0;
								Type = DOWN;
							}
							break;
						}
					case SKEY_GUI_CANCEL:
						{
							Type = QUIT;
						}
					default:
						break;
					}

					if (Type != 0)
						m_ClientInfo.m_pClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_CHAR_TELEPHONESELECT, Type), m_ClientInfo.m_pClient->Player_GetLocalObject());

					/*#if !defined(M_RTM) || defined(M_Profile)
					else if(Key.GetKey9() == SKEY_E || Key.GetKey9() == SKEY_GUI_BUTTON5)
					pCon->ExecuteString("cmd(enddialogue)");
					#endif*/

					return 1;
				}
			}
		}

		return CMWnd::OnMessage(_pMsg);
	}
};

MRTC_IMPLEMENT_DYNAMIC(CMWnd_ModTelephone, CMWnd)

