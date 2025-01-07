#include "PCH.h"
#include "WObj_CharPlayer_P6.h"
#include "WObj_CharNPC_P6.h"
#include "../../GameWorld/WClientMod_Defines.h"
#include "../WRPG/WRPGChar.h"
#include "../WObj_Game/WObj_Game_P6.h"

#define SCROLLTIME 0.15

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_CharPlayer_P6, CWObject_CharPlayer, 0x0100);

const CWO_CharPlayer_ClientData_P6& CWObject_CharPlayer_P6::GetClientData(const CWObject_CoreData* _pObj)
{
	M_ASSERT(_pObj, "[CWO_Character_ClientData] Bad this-pointer!");
	const CWO_CharPlayer_ClientData_P6* pCD = safe_cast<const CWO_CharPlayer_ClientData_P6>((const CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
	M_ASSERT(pCD, "[CWObject_CharPlayer] No clientdata?!");
	return *pCD;
};

CWO_CharPlayer_ClientData_P6& CWObject_CharPlayer_P6::GetClientData(CWObject_CoreData* _pObj)
{
	M_ASSERT(_pObj, "[CWO_Character_ClientData_P6] Bad this-pointer!");
	CWO_CharPlayer_ClientData_P6* pCD = safe_cast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
	M_ASSERT(pCD, "[CWObject_CharPlayer_P6] No clientdata?!");
	return *pCD;
};

void CWObject_CharPlayer_P6::Char_CheckDarknessActivation(int _ControlPress, int _ControlLastPress, const CVec3Dfp32& _ControlMove)
{

}

void CWObject_CharPlayer_P6::OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	// Check if we need to (re)allocate client data
	CReferenceCount* pData = _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA];
	if (!TDynamicCast<CWO_CharPlayer_ClientData_P6>(pData))
	{
		CWO_CharPlayer_ClientData_P6* pCD = MNew(CWO_CharPlayer_ClientData_P6);
		if (!pCD)
			Error_static("CWObject_CharPlayer_P6", "Could not allocate client data!");

		_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA] = pCD;
		pCD->Clear(_pObj, _pWPhysState);
	}

	if (!InitClientObjects(_pObj, _pWPhysState))
		Error_static("CWObject_CharPlayer_P6", "InitClientObjects failed");
}

void CWObject_CharPlayer_P6::OnCreate()
{
	parent::OnCreate();

	SInventoryItem Item;
	Item.item.m_LootType = INV_WEAPON;
	Item.item.m_LootObject = "Desert Eagles";
	Item.item.m_LootTemplate = "weapon_deserteagle";
	Item.item.m_GUI_Icon = "P6_inv_w_guns02";
	Item.item.m_Info = "Desert Eagles Desert Eagles";

	Item.x = 1;
	Item.y = 0;
	CWO_CharPlayer_ClientData_P6 &CD = GetClientData();
	CD.m_lInventory.Add(Item);
	CD.m_DarknessSelectedPower = -1;

//	CD.m_DarknessSelectionMode.m_Value = CD.m_DarknessSelectionMode.m_Value | PLAYER_DARKNESSMODE_ACTIVATEPOWERMASK;
}

void CWObject_CharPlayer_P6::OnSpawnWorld()
{
	parent::OnSpawnWorld();
	Char()->RemoveItemByType(m_iObject, 1, true);
	Char()->RemoveItemByType(m_iObject, 13, true);
}

void CWObject_CharPlayer_P6::Char_RefreshDarkness()
{

}

void CWObject_CharPlayer_P6::OnRefresh()
{
	parent::OnRefresh();

	CWO_CharPlayer_ClientData_P6 &CD = GetClientData();

	if(Char()->Health() < Char()->MaxHealth() && Char()->m_LastDamageTick + 200 < m_pWServer->GetGameTick())
	{
		Char()->ReceiveHealth(1);
		CD.m_Health = Char()->Health();
	}
}

void CWObject_CharPlayer_P6::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	CWObject_CoreData *pCoreData = _pWClient->Object_GetFirstCopy(_pWClient->Player_GetLocalObject());
	CWO_CharPlayer_ClientData_P6 *pCD = pCoreData ? safe_cast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*)pCoreData->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;
	if(pCD && pCD->m_bDoOnce)
	{
		int32 iObj = pCD->m_pObj->m_iObject;
		ConExecute(CStrF("inv_forceupdate(%i)", iObj));
		pCD->m_bDoOnce = false;
	}
}

void CWObject_CharPlayer_P6::OnPress()
{
	CWO_CharPlayer_ClientData_P6 &CD = GetClientData(this);
	if(CD.m_Control_Press & CONTROLBITS_BUTTON0 && !(CD.m_Control_LastPress & CONTROLBITS_BUTTON0))
	{
		CVec3Dfp32 StartPos;
		StartPos = Char_GetMouthPos(this);
		CVec3Dfp32 EndPos = StartPos + GetPositionMatrix().GetRow(0) * 50.0f;
		CCollisionInfo cinfo;
		if(m_pWServer->Phys_IntersectLine(StartPos, EndPos, OBJECT_FLAGS_PROJECTILE, OBJECT_FLAGS_CHARACTER, NULL, &cinfo, m_iObject) && cinfo.m_bIsValid)
		{	
			CWObject *pObj = m_pWServer->Object_Get(cinfo.m_iObject);
			CWObject_CharNPC_P6 *pChar = TDynamicCast<CWObject_CharNPC_P6>(pObj);
			if(pChar && Char_GetPhysType(pChar) == PLAYER_PHYS_DEAD)
			{	//Looooooooooot!!!!
				int iClient = m_pWServer->Game_GetObject()->Player_GetClient(CD.m_iPlayer);
				CRegistry* pReg = m_pWServer->Registry_GetClient(iClient, WCLIENT_REG_GAMESTATE);
				CWO_CharNPC_ClientData_P6 &CDLoot = pChar->GetClientData();
				bool bLoot = false;
				for(int i = 0; i < CDLoot.m_lLoot.Len(); i++)
				{
					if(CDLoot.m_lLoot[i].m_LootObject.Len())
						bLoot = true;
				}
				if(pReg && (CDLoot.m_Money || bLoot))
				{
					M_TRACEALWAYS(CStrF("Client %i is looting NPC %i", iClient, cinfo.m_iObject));
					CD.m_iLootTarget = cinfo.m_iObject;
					m_pWServer->NetMsg_SendToClient(CNetMsg(WPACKET_RELEASECONTROLS), iClient);

					pReg->SetValue("WINDOW", "P6_Loot");
				}
			}
		}
	}
	parent::OnPress();
}

aint CWObject_CharPlayer_P6::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_GAME_RENDERSTATUSBAR:
		{
			OnClientRenderStatusBar(_pObj, _pWClient, (CXR_Engine *)_Msg.m_Param0, (CRC_Util2D *)_Msg.m_Param1);
			return 1;
		}
	default :
		return CWObject_CharPlayer::OnClientMessage(_pObj, _pWClient, _Msg);
	}
}

void CWObject_CharPlayer_P6::OnClientRenderStatusBar(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D)
{
	MAUTOSTRIP(CWObject_Character_OnClientRenderStatusBar, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_Character::OnClientRenderStatusBar);
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	if(!pCD)
		return;

	CClipRect Clip(0, 0, 640, 480);
#ifdef WDEBUGCONTROL_ENABLED
#endif
#ifndef PLATFORM_CONSOLE
#ifndef M_RTM
	CWObject_Client* pDebugObj = _pObj;
#ifdef WDEBUGCONTROL_ENABLED
	if (pCD->m_iDebugControlObject != 0)
	{
		CWObject_Client* pDObj = _pWClient->Object_Get(pCD->m_iDebugControlObject);
		if (pDObj)
			pDebugObj = pDObj;
	}
#endif
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	int32 AGIDebugFlags = 0;
	CRegistry* pReg = pSys ? pSys->GetEnvironment() : NULL;
	if (pReg != NULL)
		AGIDebugFlags = pReg->GetValuei("AG2I_DEBUG_HUD");

#endif
#endif

	_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

	bool bCutSceneView = (_pObj->m_ClientFlags & (PLAYER_CLIENTFLAGS_CUTSCENE | PLAYER_CLIENTFLAGS_DIALOGUE)) != 0;
	int PhysType = Char_GetPhysType(_pObj);
	if (bCutSceneView || (PhysType != PLAYER_PHYS_STAND && PhysType != PLAYER_PHYS_CROUCH))
		return;

	OnClientRenderHealthHud(_pObj, _pWClient, _pEngine, _pUtil2D);

	OnClientRenderChoices(_pObj, _pWClient, _pEngine, _pUtil2D);
}


void CWObject_CharPlayer_P6::OnClientRenderChoices(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D)
{
	CWO_CharPlayer_ClientData_P6 &CD = GetClientData(_pObj);

	if(CD.m_nChoices == 0)
		return;

	CRC_Font *pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("P6_text"));
	if(!pFont)
		return;

	CClipRect Clip(0, 0, 640, 480);

	CPixel32 PixColor(255, 255, 255, 255);
	CPixel32 PixColorBlack(0, 0, 0, 255);
	CPixel32 PixColorBlackOutline(0, 0, 0, 105);
	CPixel32 PixColorTextBox(255, 255, 255, 64);

	int Border = 60;
	int XStart = Border;
	const int ChoiceHeight = 20;
	const int YStart = 480 - Border / 2 - ChoiceHeight * 3;

	_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

	CStr lChoice[3];

	CStr Choice = CD.m_Choices.Ansi();
	int i;
	for(i = 0; i < CD.m_iCurChoice - 1; i++)
		Choice.GetStrSep(";");

	if(CD.m_iCurChoice > 0)
		lChoice[0] = Choice.GetStrSep(";");
	lChoice[1] = Choice.GetStrSep(";");
	lChoice[2] = Choice.GetStrSep(";");

	fp32 FontSize = 15.0f; 
	for(int i = 0; i < 3; i++)
	{
		CStr St = lChoice[i];

		if(St != "")
		{
			CStr Text;
			if(St.Find("PHONENUMBER") != -1)
				Text = Localize_Str(St);
			else 
				Text = St;

			if(i == 1)
			{
				CRct FlashRect;
				FlashRect.p0 = CPnt(XStart - 6, (YStart + ChoiceHeight) - 4);
				FlashRect.p1 = CPnt(XStart + _pUtil2D->TextWidth(pFont, Text, (FontSize / pFont->GetOriginalSize())) + 6, (YStart + ChoiceHeight - 2) + ChoiceHeight);
				_pUtil2D->Rect(Clip, FlashRect, PixColorTextBox);
				_pUtil2D->Text_DrawFormatted(Clip, pFont, Text, XStart, YStart + i * ChoiceHeight, 0, PixColorBlackOutline, PixColorBlackOutline, PixColorBlackOutline, Clip.GetWidth()-Border, Clip.GetHeight(), true, 0, (FontSize / pFont->GetOriginalSize()));
				_pUtil2D->Text_DrawFormatted(Clip, pFont, Text, XStart, YStart + i * ChoiceHeight, 0, PixColor, PixColor, PixColorBlack, Clip.GetWidth()-Border, Clip.GetHeight(), false, 0, (FontSize / pFont->GetOriginalSize()));
			}
			else
			{
				_pUtil2D->Text_DrawFormatted(Clip, pFont, Text, XStart, YStart + i * ChoiceHeight, 0, PixColorBlackOutline, PixColorBlackOutline, PixColorBlackOutline, Clip.GetWidth()-Border, Clip.GetHeight(), true, 0, (FontSize / pFont->GetOriginalSize()));
				_pUtil2D->Text_DrawFormatted(Clip, pFont, Text, XStart, YStart + i * ChoiceHeight, 0, PixColor, PixColor, PixColorBlack, Clip.GetWidth()-Border, Clip.GetHeight(), false, 0, (FontSize / pFont->GetOriginalSize()));
			}
		}
	}

	// render the up/down arrows
	bool bPressed;
	int TextureID = 0;
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	CXR_VBManager *pVBM = _pUtil2D->GetVBM();
	CRC_Viewport*pViewport = pVBM->Viewport_Get();
	CVec2Dfp32 CoordScale = _pUtil2D->GetCoordinateScale();

	CMat4Dfp32* pMat2D = pVBM->Alloc_M4_Proj2DRelBackPlane();
	if (!pMat2D)
		return;

	fp32 PictureWidthBase	 =  12;
	fp32 PicSize  = PictureWidthBase * CoordScale.k[0];

	CRect2Duint16 PictureScreenCoord;
	CPnt ScrollButtonPointBase;
	ScrollButtonPointBase.x = 140;
	ScrollButtonPointBase.y = YStart -  TruncToInt(FontSize) - 6;
 
	CMTime Time = CMTime::GetCPU();
	fp32 CurTime = Time.GetTime();
	fp32 DeltaTime = CurTime - CD.m_LastTime;

	if(CD.m_ScrollDownTimer != 0.0f)
	{
		CD.m_ScrollDownTimer += DeltaTime;
		if(CD.m_ScrollDownTimer > SCROLLTIME)
			CD.m_ScrollDownTimer = 0.0f;
	}

	if(CD.m_ScrollUpTimer)
	{
		CD.m_ScrollUpTimer += DeltaTime;
		if(CD.m_ScrollUpTimer > SCROLLTIME)
			CD.m_ScrollUpTimer = 0.0f;
	}

	CD.m_LastTime = CurTime;

	for(int i = 0; i < 2; i++)
	{	
		if(i == 0 && CD.m_iCurChoice == 0)
		{
			ScrollButtonPointBase.y += (3 * ChoiceHeight ) + TruncToInt(PicSize) + 6;
			continue;
		}
		else if(i == 1 && CD.m_iCurChoice == CD.m_nChoices-1)
		{
			continue;
		}

		bPressed = false;
		if(((CD.m_ScrollDownTimer != 0.0f) && i) || ((CD.m_ScrollUpTimer != 0.0f) && !i))
		{
			bPressed = true;
		}

		if(i)
		{
			if(bPressed)
				TextureID = pTC->GetTextureID("GUI_Arrow_Down_pressed");
			else
				TextureID = pTC->GetTextureID("GUI_Arrow_Down");
		}
		else
		{
			if(bPressed)
				TextureID = pTC->GetTextureID("GUI_Arrow_Up_pressed");
			else
				TextureID = pTC->GetTextureID("GUI_Arrow_Up");
		}

		bPressed = false;

		if(TextureID)
		{
			PictureScreenCoord.m_Min = CVec2Duint16(TruncToInt(ScrollButtonPointBase.x * CoordScale.k[0]), TruncToInt(ScrollButtonPointBase.y * CoordScale.k[1]));
			PictureScreenCoord.m_Max = CVec2Duint16(PictureScreenCoord.m_Min.k[0] + TruncToInt(PicSize), PictureScreenCoord.m_Min.k[1] + TruncToInt(PicSize));

			// for flip or no-flip UV cords
			CVec2Dfp32 UVMin(0.0f, 0.0f);
			CVec2Dfp32 UVMax(1.0f, 1.0f);

			CVec2Dfp32* pTV;
			pTV = CXR_Util::VBM_CreateRectUV(pVBM, UVMin, UVMax);

			CRC_Attributes* pA = pVBM->Alloc_Attrib();
			pA->SetDefault();
			pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
			pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
			pA->Attrib_TextureID(0, TextureID);

			CXR_VertexBuffer* pVBScreenQuad = CXR_Util::VBM_RenderRect(pVBM, pMat2D, PictureScreenCoord, PixColor, 0.1f, pA);
			pVBScreenQuad->Geometry_TVertexArray(pTV, 0);
			pVBM->AddVB(pVBScreenQuad);
		}

		// second scrollbutton
		ScrollButtonPointBase.y += (3 * ChoiceHeight ) + TruncToInt(PicSize) + 6;
	}
}
/*
void CWObject_CharPlayer_P6::OnClientRenderHealthHud(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D)
{
	MSCOPESHORT(CWObject_CharPlayer_P6::OnClientRenderHealthHud);


	// Get health
	// If fade is zero, just skip the rendering
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pObj);
	M_ASSERT(pCD, "no client data!");
	//	CWO_CharPlayer_ClientData& CD = *safe_cast<CWO_CharPlayer_ClientData>(pCD);

	CWObject_CharPlayer::OnClientRenderHealthHud(_pObj, _pWClient, _pEngine, _pUtil2D);

	_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	_pUtil2D->SetTexture(0);
	CClipRect Clip(0, 0, 640, 480);
	_pUtil2D->SetTexture(NULL);

	fp32 Fade = LERP(pCD->m_LastHealthHudFade, pCD->m_HealthHudFade, pCD->m_PredictFrameFrac);
	if (Fade <= 0.0f)
		return;
	//fp32 Fade = 1.0f;

	// Depending on whether a power is active, change color
	int32 ColorHealth;
	ColorHealth = CPixel32((215), (235), (255), (int)(Fade * 0.50f * 255.0f));

	{
		// Render shield
		const int32 SizeX = 100;
		const int32 SizeY = 8;
		const int32 StartX = 42;
		const int32 StartY = 36;

		const int CenterX = StartX - 13;
		const int CenterY = StartY + 3;
		const int Size = 22;
		fp32 Aspect = _pUtil2D->GetRC()->GetDC()->GetScreenAspect() * _pUtil2D->GetRC()->GetDC()->GetPixelAspect();
		_pUtil2D->SetSurface("GUI_HUD_DARKNESS", pCD->m_GameTime);
		_pUtil2D->SetTextureOrigo(Clip, CPnt((int)(CenterX - Size / Aspect / 2), CenterY - Size / 2));
		_pUtil2D->SetTextureScale(fp32(_pUtil2D->GetTextureWidth()) / Size * Aspect, fp32(_pUtil2D->GetTextureHeight()) / Size);
		_pUtil2D->Rect(Clip, CRct((int)(CenterX - Size / 2 / Aspect), CenterY - Size / 2, (int)(CenterX + Size / 2 / Aspect), CenterY + Size / 2), ColorHealth);

		_pUtil2D->SetTexture(0);
		_pUtil2D->Rect(Clip, CRct(StartX - 2, StartY - 2, StartX + SizeX + 2, StartY - 1), ColorHealth);
		_pUtil2D->Rect(Clip, CRct(StartX - 2, StartY - 1, StartX - 1, StartY + SizeY + 2), ColorHealth);
		_pUtil2D->Rect(Clip, CRct(StartX - 1, StartY + SizeY + 1, StartX + SizeX + 2, StartY + SizeY + 2), ColorHealth);
		_pUtil2D->Rect(Clip, CRct(StartX + SizeX + 1, StartY - 1, StartX + SizeX + 2, StartY + SizeY + 1), ColorHealth);

		// Reserve 1 health for bottom stuff
		int32 nHealth = pCD->m_MaxHealth > 0 ? Max(Min(((pCD->m_Health-1) * SizeX) / (pCD->m_MaxHealth-1),SizeX),(int32)0) : 0;
		if (nHealth > 0)
		{
			int Color = ColorHealth;
			if(pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_GODMODE)
				Color &= 0xffffff00;

			_pUtil2D->Rect(Clip, CRct(StartX, StartY, StartX + nHealth, StartY + SizeY), Color);
		}
	}
}*/

bool CWObject_CharPlayer_P6::InventoryIsArmorEquipped(void)
{
	CWO_CharPlayer_ClientData_P6 &CD = GetClientData();
	for(int i = 0; i < CD.m_lInventory.Len(); i++)
	{
		if(CD.m_lInventory[i].y == 1 && (CD.m_lInventory[i].x == 0 || CD.m_lInventory[i].x == 1))
		{
			return true;
		}
	}

	return false;
}

aint CWObject_CharPlayer_P6::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_DAMAGE:
		{
			const CWO_DamageMsg *pMsg = CWO_DamageMsg::GetSafe(_Msg);
			CWO_DamageMsg MsgDam = *pMsg;
			
			if(InventoryIsArmorEquipped())
				MsgDam.m_Damage /= 10;

			if(pMsg)
				return Physics_Damage(MsgDam, _Msg.m_iSender);
			else
				return parent::OnMessage(_Msg);
		}
		break;
	
	case P6_OBJMSG_CHAR_START_TRADING:
		{
//			ConExecute("removepadlock(); cg_rootmenu(\"MainMenu\")");
//			ConExecute("deferredscript(\"cg_submenu(\\\"p6_trading\\\")\")");
//			OnSetClientWindow(0, "");
			CWObject_Game *pGame = m_pWServer->Game_GetObject();
			if(pGame)
			{
				for(int i = 0; i < pGame->Player_GetNum(); i++)
				{
					CWO_Player *pPlayer = pGame->Player_Get(i);
					if(pPlayer->m_iObject == m_iObject)
					{
						CRegistry* pReg = m_pWServer->Registry_GetClient(i, WCLIENT_REG_GAMESTATE);
						if(pReg)
						{
							m_pWServer->NetMsg_SendToClient(CNetMsg(WPACKET_RELEASECONTROLS), i);
							pReg->SetValue("WINDOW", "p6_trading");
						}
						return 1;
					}
				}
			}
			return 1;
		}
		break;

	case P6_OBJMSG_CHAR_REWARD:
		{
			CWO_CharPlayer_ClientData_P6 &CD = GetClientData();
			CD.m_Money.m_Value += 75;
		}
		break;

	case P6_OBJMSG_CHAR_EQUIP_WEAPON:
		{
			CWO_CharPlayer_ClientData_P6 &CD = GetClientData();
			Char()->PickupItem(CD.m_lInventory[_Msg.m_Param0].item.m_LootTemplate, true, true, NULL, 0, true);
		}
		break;

	case P6_OBJMSG_CHAR_UNEQUIP_WEAPON:
		{
			Char()->RemoveItemByType(m_iObject, 1, true);
			Char()->RemoveItemByType(m_iObject, 13, true);
			CRPG_Object_Item* pItemDualWield = Char()->GetFinalSelectedItem(AG2_ITEMSLOT_DUALWIELD);
			if(pItemDualWield)
				pItemDualWield->Unequip(m_iObject, Char());
		}
		break;
	default:
		return parent::OnMessage(_Msg);
	}

	return 1;
}


