#include "PCH.h"
#include "WObj_ObjAnim.h"
#include "../WObj_Sys/WObj_Trigger.h"

/*	static const char *ms_AttribStr[] =
{
"msg", "impulse", 
"anim0", "anim1", "anim2", "animtime", 
"data0", "data1", "data2", "data3",
"data4", "data5", "data6", "data7", NULL
	};*/

void CWObject_ObjAnim::OnCreate()
{
	MAUTOSTRIP(CWObject_ObjAnim_OnCreate, MAUTOSTRIP_VOID);
	CWObject_Model::OnCreate();
	
	m_iIndex = 0;
	m_Wait = 0;
	m_ImpulseWait = -1;
	m_CriticalSection = 0;
	m_nCurImpulses = 0;
}

void CWObject_ObjAnim::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_ObjAnim_OnEvalKey, MAUTOSTRIP_VOID);
	if(_pKey->GetThisName().Copy(0, 6) == "ACTION")
	{
		CStr st = _pKey->GetThisValue().LowerCase();
		CKeyFrame Frame;
		Frame.m_iAction = _pKey->GetThisName().Copy(6, 1024).Val_int();
		Frame.m_Cmd = st.GetStrSep(" ");
		Frame.m_Target = st.GetStrSep(" ");
		Frame.m_Value = st.GetStrSep(" ").Val_fp64();
		//			ConOutL(CStrF("%s %s %f", (char *)Frame.m_Cmd, (char *)Frame.m_Target, Frame.m_Value));
		int i;
		for(i = 0; i < m_KeyFrames.Len(); i++)
		{
			if(Frame.m_iAction < m_KeyFrames[i].m_iAction)
			{
				m_KeyFrames.Insert(i, Frame);
				break;
			}
		}
		if(i == m_KeyFrames.Len())
			m_KeyFrames.Add(Frame);
	}
	
	else
		CWObject_Model::OnEvalKey(_pKey);
}

int CWObject_ObjAnim::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_ObjAnim_OnMessage, 0);
	switch(_Msg.m_Msg)
	{
	case OBJMSG_IMPULSE:
		if(m_Wait == -1 && m_ImpulseWait == _Msg.m_Param0)
		{
			m_Wait = 0;
			m_ImpulseWait = -1;
		}
		
		{
			//Remember all impulse we got during this tick
			if(m_nCurImpulses >= MAX_TRACKED_IMPULSES)
			{
				ConOutL("(CWObject_ObjAnim::OnMessage) Too many different impulses");
				break;
			}
			
			int i;
			for(i = 0; i < m_nCurImpulses; i++)
				if(m_CurImpulse[i] == _Msg.m_Param0)
				{
					//Impulse already registred
					m_ImpulseTime[i] = m_pWServer->GetGameTick();
					break;
				}
				
				if(i == m_nCurImpulses)
				{
					m_CurImpulse[m_nCurImpulses] = _Msg.m_Param0;
					m_ImpulseTime[m_nCurImpulses++] = m_pWServer->GetGameTick();
				}
		}
		
		break;
	}
	return CWObject::OnMessage(_Msg);
}

int CWObject_ObjAnim::GetObjectIndexes(const CStr _Name, const int16 **_pSel)
{
	MAUTOSTRIP(CWObject_ObjAnim_GetObjectIndexes, 0);
	int iSel = m_pWServer->Selection_Push();
	m_pWServer->Selection_AddTarget(Selection, _Name);
	const int16* pSel = NULL;
	int nSel = m_pWServer->Selection_Get(Selection, &pSel);
	m_pWServer->Selection_Pop();
	
	_pSel = &pSel;
	return nSel;
}

void CWObject_ObjAnim::OnRefresh()
{
	MAUTOSTRIP(CWObject_ObjAnim_OnRefresh, MAUTOSTRIP_VOID);
	CWObject_Model::OnRefresh();
	
	{
		//Remove old impulses
		for(int i = 0; i < m_nCurImpulses; i++)
		{
			if(m_ImpulseTime[i] < m_pWServer->GetGameTick() - 2)
			{
				if(i < m_nCurImpulses - 1)
				{
					m_ImpulseTime[i] = m_ImpulseTime[i + 1];
					m_CurImpulse[i] = m_CurImpulse[i + 1];
				}
				i--;
				m_nCurImpulses--;
			}
		}
	}
	
	if(m_Wait)
	{
		if(m_Wait == -1)
			return;
		m_Wait--;
		return;
	}
	
	bool bContinue = true;
	while(bContinue)
	{
		int nFrames = m_KeyFrames.Len();
		if(!nFrames || m_iIndex >= nFrames)
			return;
		
		CKeyFrame *pFrame = &m_KeyFrames[m_iIndex];
		
		if(m_KeyFrames[m_iIndex].m_Cmd == "msg")
			m_pWServer->Message_SendToTarget(CWObject_Message(OBJMSG_OBJANIM_TRIGGER, m_KeyFrames[m_iIndex].m_Value), m_KeyFrames[m_iIndex].m_Target);
		else if(m_KeyFrames[m_iIndex].m_Cmd == "impulse")
		{
			//				ConOut(CStrF("(CWObject_ObjAnim::OnRefresh) Impulse %s %i", (char *)m_KeyFrames[m_iIndex].m_Target, (int)m_KeyFrames[m_iIndex].m_Value));
			m_pWServer->Message_SendToTarget(CWObject_Message(OBJMSG_IMPULSE, m_KeyFrames[m_iIndex].m_Value), m_KeyFrames[m_iIndex].m_Target);
		}
		
		else if(m_KeyFrames[m_iIndex].m_Cmd == "wait")
		{
			if(m_KeyFrames[m_iIndex].m_Target == "impulse")
			{
				m_ImpulseWait = m_KeyFrames[m_iIndex].m_Value;
				m_Wait = -1;
			}
			else if(m_KeyFrames[m_iIndex].m_Target == "time")
				m_Wait = m_KeyFrames[m_iIndex].m_Value * SERVER_TICKSPERSECOND;
			bContinue = false;
		}
		
		else if(m_KeyFrames[m_iIndex].m_Cmd == "goto")
			m_iIndex = m_KeyFrames[m_iIndex].m_Target.Val_int() - 1;
		
		else if(m_KeyFrames[m_iIndex].m_Cmd == "sound")
			m_pWServer->Sound_At(GetPosition(), m_pWServer->GetMapData()->GetResourceIndex_Sound(m_KeyFrames[m_iIndex].m_Target), 0);
		
		else if(m_KeyFrames[m_iIndex].m_Cmd == "startsound")
		{
			m_iSound[0] = m_pWServer->GetMapData()->GetResourceIndex_Sound(m_KeyFrames[m_iIndex].m_Target);
			SetDirty(-1);
		}
		
		else if(m_KeyFrames[m_iIndex].m_Cmd == "stopsound")
		{
			m_iSound[0] = 0;
			SetDirty(-1);
		}
		
		else if(m_KeyFrames[m_iIndex].m_Cmd == "if" || m_KeyFrames[m_iIndex].m_Cmd == "ifnot")
		{				
			bool bNot = (m_KeyFrames[m_iIndex].m_Cmd == "ifnot") != 0;
			int iIndex = m_iIndex++;
			
			if(m_KeyFrames[iIndex].m_Target == "impulse")
			{
				int i;
				for(i = 0; i < m_nCurImpulses; i++)
					if(m_CurImpulse[i] == m_KeyFrames[iIndex].m_Value)
						break;
					if(bNot ? (i == m_nCurImpulses) : (i != m_nCurImpulses))
						continue;
			}
		}
		
		/*			else if(m_KeyFrames[m_iIndex].m_Cmd == "if_criticalsection")
		{
		int iObj = m_pWServer->Selection_GetSingleTarget(selection[m_iIndex].m_Target);
		CWObject_ObjAnim *pAnim = TDynamicCast<CWObject_ObjAnim >(m_pWServer->Object_Get(iObj));
		
		  m_iIndex++;
		  if(pAnim)
		  {
		  if(pAnim->m_CriticalSection)
		  {
		  //						ConOutL("In critical section");
		  continue;
		  }
		  }
		  else
		  ConOutL("(CWObject_Anim::OnRefresh) if_criticalsection didn't target a valid objanim-object");
		  }
		  
			else if(m_KeyFrames[m_iIndex].m_Cmd == "criticalsection")
			{
			ConOutL(CStrF("critialsection %f", m_KeyFrames[m_iIndex].m_Value));
			m_CriticalSection = m_KeyFrames[m_iIndex].m_Target.Val_int();
	}*/
		
		else if(m_KeyFrames[m_iIndex].m_Cmd == "reset")
		{
			//				ConOutL("reset");
			m_iIndex = 0;
			m_Wait = -1;
			m_ImpulseWait = -1;
			continue;
		}
		
		/*			else if(m_KeyFrames[m_iIndex].m_Cmd == "set")
		{
		const int16 *pSel;
		int nSel = GetObjectIndexes(m_KeyFrames[m_iIndex].m_Target, &pSel);
		for(int s = 0; s < nSel; s++)
		{
		CWObject *pObj = m_pWServer->Object_Get(pSel[s]);
		switch(m_KeyFrames[m_iIndex].m_Attrib)
		{
		case 2: pObj->m_iAnim0 = m_KeyFrames[m_iIndex].m_Value; break;
		case 3: pObj->m_iAnim1 = m_KeyFrames[m_iIndex].m_Value; break;
		case 4: pObj->m_iAnim2 = m_KeyFrames[m_iIndex].m_Value; break;
		case 5: pObj->m_AnimTime = m_KeyFrames[m_iIndex].m_Value; break;
		case 6: pObj->m_Data[0] = m_KeyFrames[m_iIndex].m_Value; break;
		case 7: pObj->m_Data[1] = m_KeyFrames[m_iIndex].m_Value; break;
		case 8: pObj->m_Data[2] = m_KeyFrames[m_iIndex].m_Value; break;
		case 9: pObj->m_Data[3] = m_KeyFrames[m_iIndex].m_Value; break;
		case 10: pObj->m_Data[4] = m_KeyFrames[m_iIndex].m_Value; break;
		case 11: pObj->m_Data[5] = m_KeyFrames[m_iIndex].m_Value; break;
		case 12: pObj->m_Data[6] = m_KeyFrames[m_iIndex].m_Value; break;
		case 13: pObj->m_Data[7] = m_KeyFrames[m_iIndex].m_Value; break;
		}
		}
			}*/
		
		m_iIndex++;
		}
	}
	
	void CWObject_ObjAnim::OnLoad(CCFile* _pFile)
	{
		CWObject_Model::OnLoad(_pFile);
	}
	
	void CWObject_ObjAnim::OnSave(CCFile* _pFile)
	{
		CWObject_Model::OnSave(_pFile);
	}
	
	MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_ObjAnim, CWObject_Model, 0x0100);
	
