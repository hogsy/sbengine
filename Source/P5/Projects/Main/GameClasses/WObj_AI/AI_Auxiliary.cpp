#include "PCH.h"
#include "AICore.h"
#include "../../GameWorld/WClientMod_Defines.h"

 

//CAI_ControlHandler

//Return the control state bit for the given device, or 0 if device does not have a corresponding state bit
uint32 CAI_ControlHandler::DeviceStateBit(int _iDevice, int _iData)
{
	MAUTOSTRIP(CAI_ControlHandler_DeviceStateBit, 0);
	switch (_iDevice)
	{
	/* No jumping zone ;)
	case CAI_Device::JUMP:
		return CONTROLBITS_JUMP;
	*/
	case CAI_Device::WEAPON:
		{
			//Check if we use (contrary to switch) weapon 
			if (_iData == CAI_Device_Item::USE)
				//Jupp we use (or have stopped using) weapon
				return CONTROLBITS_PRIMARY;
			else
				//Weapon switch
				return 0;
		};
	case CAI_Device::ITEM:		
		{
			//Check if we use (contrary to switch) item 
			if (_iData == CAI_Device_Item::USE)
				//Jupp we use (or have stopped using) weapon
				return CONTROLBITS_SECONDARY;
			else
				//Weapon switch
				return 0;
		};
	case CAI_Device::STANCE:
		return CONTROLBITS_CROUCH;
	case CAI_Device::MELEE:
		{
			//Get the bits to set
			int Bits = 0;
			if (_iData)
			{
				/* Old Melee system
				if (_iData & CAI_Device_Melee::ATTACK_PUNCH)
					Bits |= CONTROLBITS_BUTTON0;
				if (_iData & CAI_Device_Melee::ATTACK_KICK)
					Bits |= CONTROLBITS_BUTTON1;
				if (_iData & CAI_Device_Melee::BLOCK)
					Bits |= CONTROLBITS_BUTTON2;
				if (_iData & CAI_Device_Melee::INITIATE)
					Bits |= CONTROLBITS_BUTTON3;
				*/
				/*if (_iData & CAI_Device_Melee::INITIATE)
					Bits |= CONTROLBITS_BUTTON3;*/
				if (_iData & CAI_Device_Melee::ATTACK_PUNCH)
					Bits |= CONTROLBITS_PRIMARY;
				if (_iData & CAI_Device_Melee::BLOCK)
					Bits |= CONTROLBITS_SECONDARY;
			}
			return Bits;
		}
	case CAI_Device::DARKNESS:
		{
			int Bits = 0;
			if (_iData)
			{
				if (_iData & CAI_Device_Darkness::USAGE_DRAIN)
					Bits |= CONTROLBITS_BUTTON5;
				if (_iData & CAI_Device_Darkness::USAGE_ACTIVATE)
					Bits |= CONTROLBITS_BUTTON2;
				if (_iData & CAI_Device_Darkness::USAGE_SELECT)
					Bits |= CONTROLBITS_DPAD_UP;
			}
			return Bits;
		}
	default:
		return 0;
	};
};

//Construct and set given command to corresponding command from the given device.
void CAI_ControlHandler::AddCommand(int _iDevice, int _iData, const CVec3Dfp32& _VecData, CControlFrame * _pCtrl)
{
	MAUTOSTRIP(CAI_ControlHandler_GetCommand, MAUTOSTRIP_VOID);
	if (!_pCtrl)
		return;
	
	//Create command, if corresponding such exists
	switch (_iDevice)
	{
	case CAI_Device::LOOK:
		{
			CCmd Cmd;
			Cmd.Create_Look(_VecData * 65536.0f, 0);
			_pCtrl->AddCmd(Cmd);
			return;
		};
	case CAI_Device::MOVE:
		{
			CCmd Cmd;
			Cmd.Create_Move(_VecData * 256.0f, 0);
			_pCtrl->AddCmd(Cmd);
			m_pAI->m_pGameObject->AI_SetAnimControlMode(_iData != 0);
			return;
		};
	case CAI_Device::WEAPON:
		{
			//Check if and how we switch (contrary to use) weapon 
			switch (_iData)
			{
			case CAI_Device_Item::SWITCH_NEXT:
				{
					CCmd Cmd;
					Cmd.Create_Cmd0(CMD_NEXTWEAPON, 0);
					_pCtrl->AddCmd(Cmd);
					return;
				};
			case CAI_Device_Item::SWITCH_PREVIOUS:
				{
					CCmd Cmd;
					Cmd.Create_Cmd0(CMD_PREVWEAPON, 0);
					_pCtrl->AddCmd(Cmd);
					return;
				};
			case CAI_Device_Item::SWITCH:
				{
					//Do nothing... This is done asynchronously nowadays
					//CCmd Cmd;
					//Cmd.Create_Cmd1(CMD_WEAPON, _VecData[0], 0);
					//_pCtrl->AddCmd(Cmd);
					return;
				};
			default:
				//Not switch useage
				return;
			};
		};
	case CAI_Device::ITEM:		
		{
			//Check if and how we switch (contrary to use) item 
			switch (_iData)
			{
			case CAI_Device_Item::SWITCH_NEXT:
				{
					CCmd Cmd;
					Cmd.Create_Cmd0(CMD_NEXTITEM, 0);
					_pCtrl->AddCmd(Cmd);
					return;		
				};
			case CAI_Device_Item::SWITCH_PREVIOUS:
				{
					CCmd Cmd;
					Cmd.Create_Cmd0(CMD_PREVITEM, 0);
					_pCtrl->AddCmd(Cmd);
					return;		
				};
			case CAI_Device_Item::SWITCH:
				{
					//There is no general item switch functionality in character, so we'll have to do it ugly
					return;
				};
			default:
				//Not switch useage
				return;
			};
		};
	case CAI_Device::MELEE:
		{
			//Check melee actions
			if (_iData)
			{
				//Break off is exclusive
				if (_iData & CAI_Device_Melee::BREAK_OFF)
				{
					//Move back
					CCmd Cmd;
					Cmd.Create_Move(CVec3Dfp32(-1, 0, 0) * 256.0f, 0);
					_pCtrl->AddCmd(Cmd);
				}
				else
				{
					//Set move dir based on action combination
					CVec3Dfp32 Move = CVec3Dfp32(0);

					if (_iData & CAI_Device_Melee::MOVE_FWD)
					{
						Move[0] = 1;
					}
					else if (_iData & CAI_Device_Melee::MOVE_BWD)
					{
						Move[0] = -1;
					}

					if (_iData & CAI_Device_Melee::MOVE_LEFT)
					{
						Move[1] = 1;
					}
					else if (_iData & CAI_Device_Melee::MOVE_RIGHT)
					{
						Move[1] = -1;
					}

					//Create and add move
					if (Move != CVec3Dfp32(0))
						Move.Normalize();
					CCmd Cmd;
					Cmd.Create_Move(Move * 256.0f, 0);
					_pCtrl->AddCmd(Cmd);
				}
			}
			return;
		}
	case CAI_Device::DARKNESS:
		{
			if (_iData & CAI_Device_Darkness::USEMOVE)
			{
				CCmd Cmd;
				Cmd.Create_Move(_VecData * 256.0f, 0);
				_pCtrl->AddCmd(Cmd);
			}
			return;
		};
	default:
		return;
	};
};

//Constructor
CAI_ControlHandler::CAI_ControlHandler()
{
	MAUTOSTRIP(CAI_ControlHandler_ctor, MAUTOSTRIP_VOID);
	m_pAI = NULL;
	// m_Ctrl = CControlFrame();
};

void CAI_ControlHandler::SetAI(CAI_Core* _pAI)
{
	m_pAI = _pAI;
};

void CAI_ControlHandler::ReInit(CAI_Core* _pAI)
{
	MAUTOSTRIP(CAI_ControlHandler_ReInit, MAUTOSTRIP_VOID);
	SetAI(_pAI);
};



//Build and get the AI control frame, by polling the devices
const CControlFrame* CAI_ControlHandler::GetControlFrame()
{
	MSCOPESHORT(CAI_ControlHandler::GetControlFrame);
	MAUTOSTRIP(CAI_ControlHandler_GetControlFrame, NULL);
	//Non-optimised version FIX

	if (!m_pAI || !m_pAI->IsValid(true))
		return NULL;

	m_Ctrl = CControlFrame();

	//Collect control info from devices. 
	uint32 iStateBits = 0;
	int iData;
	CVec3Dfp32 VecData;
	CCmd Cmd;
	for (int i = 0; i < CAI_Device::NUM_DEVICE; i++)
	{
		//Get data from device if appropriate
		if (m_pAI->m_lDevices[i]->GetData(&iData, &VecData))
		{
			//Set any control state bits for device
			iStateBits |= DeviceStateBit(i, iData);

			//Add any appropriate commands for device
			AddCommand(i, iData, VecData, &m_Ctrl);
		}
	};

	//Add state bits command
	m_Ctrl.AddCmd_StateBits(iStateBits, 1);

	//m_pAI->OnSpeak(m_pAI->m_DebugInfo);//DEBUG

	return &m_Ctrl;
};



//CNameValue/////////////////////////////////////////////////////////////////////////////////
CNameValue::CNameValue()
{
	MAUTOSTRIP(CNameValue_ctor, MAUTOSTRIP_VOID);
	m_Name = "";
	m_Value = "";
};

CNameValue::CNameValue(CStr _Name, CStr _Value)
{
	MAUTOSTRIP(CNameValue_ctor_2, MAUTOSTRIP_VOID);
	m_Name = _Name; 
	m_Value = _Value;
};




//CPositionGenerator///////////////////////////////////////////////////////////////////////

//Reset stuff when we're done generating positions
void CPositionGenerator::OnFinished()
{
	MAUTOSTRIP(CPositionGenerator_OnFinished, MAUTOSTRIP_VOID);
	m_bValid = false;
};

//Constructor
CPositionGenerator::CPositionGenerator()
{
	MAUTOSTRIP(CPositionGenerator_ctor, MAUTOSTRIP_VOID);
	m_CurPos = CVec3Dfp32(_FP32_MAX);
	m_bValid = false;
};

//(Re-)Initialize generator with default arguments
void CPositionGenerator::Init()
{
	MAUTOSTRIP(CPositionGenerator_Init, MAUTOSTRIP_VOID);
	m_bValid = true;
	m_CurPos = CVec3Dfp32(_FP32_MAX);
};


//Retrieve current position or CVec3Dfp32(_FP32_MAX) if this is invalid
CVec3Dfp32 CPositionGenerator::GetPosition()
{
	MAUTOSTRIP(CPositionGenerator_GetPosition, CVec3Dfp32());
	return m_CurPos;
};

//Check if this is initialized and can generate more positions
bool CPositionGenerator::IsValid()
{
	MAUTOSTRIP(CPositionGenerator_IsValid, false);
	return m_bValid;
};


//CPositionGenerator_HalfSphere

//Set up stuff for generating positions on next circle
bool CPositionGenerator_HalfSphere::OnNextCircle()
{
	MAUTOSTRIP(CPositionGenerator_HalfSphere_OnNextCircle, false);
	//Deviate to next circle
	m_Dev += m_dDev;

	//Check if we're done
	if (m_Dev > m_MaxDeviation)
	{
		OnFinished();
		return false;
	}
	else
	{
		//Calculate and reset stuff for this circle
		m_Rad = m_Radius * m_Dev;
		m_dAngle = Min(2.0f * (fp32)_PI * 0.34f, 1.1f * m_Density / m_Radius); //At least three positions per circle
		m_Angle = 0;
		return true;
	};
};


//Default initilization is a unit half circle with the yz-plane as base.
void CPositionGenerator_HalfSphere::Init()
{
	MAUTOSTRIP(CPositionGenerator_HalfSphere_Init, MAUTOSTRIP_VOID);
	CMat4Dfp32 Mat;
	Mat.Unit();
	Init(Mat);
};

//(Re-)Initialize generator. The (optional) arguments are the center and orientation, specified as 
//a matrix the radius, the density of generated positions (i.e. the minimum distance between any 
//two positions) and the maximum deviation in fractions from the front point. 
void CPositionGenerator_HalfSphere::Init(const CMat4Dfp32& _Mat, fp32 _Radius, fp32 _Density, fp32 _MaxDeviation)
{
	MAUTOSTRIP(CPositionGenerator_HalfSphere_Init_2, MAUTOSTRIP_VOID);
	CPositionGenerator::Init();

	m_Mat = _Mat;
	m_Radius = _Radius;
	m_Density = _Density; 

	//Deviation is measured in fraction of the total radius instead of angles, internally
	if (Abs(m_MaxDeviation) > 0.25f)
		m_MaxDeviation = 1.0f;
	else
		m_MaxDeviation = QSin(_MaxDeviation * 2 * _PI);

	//Set up values for first round
	m_Dev = 0;
	m_Rad = 0;
	m_Angle = 0;
	m_dAngle = 2 * _PI;

	//Consecutive circles are at increasingly greater distances from each other.
	//(i.e. m_dDev will be constant throughout the generation process)
	m_dDev = (M_Sqrt(Sqr(m_Density) - Sqr(m_Density) * 0.25f / Sqr(m_Radius))) / m_Radius;
};

//(Re-)Initialize generator with a position vector and an orientation vector (tilt, pitch, heading)-angles in fractions instead of matrix
void CPositionGenerator_HalfSphere::Init(const CVec3Dfp32& _Center, const CVec3Dfp32& _Orientation, fp32 _Radius, fp32 _Density, fp32 _MaxDeviation)
{
	MAUTOSTRIP(CPositionGenerator_HalfSphere_Init_3, MAUTOSTRIP_VOID);
	CMat4Dfp32 Mat;
	Mat.Unit();
	if (_Orientation[0])
		Mat.RotX_x_M(_Orientation[0]);
	if (_Orientation[1])
		Mat.RotY_x_M(_Orientation[1]);
	if (_Orientation[2])
		Mat.RotZ_x_M(-_Orientation[2]);
	_Center.SetRow(Mat, 3);
	Init(Mat, _Radius, _Density, _MaxDeviation);
};

//First position is always the front point of the halfcircle. Consequtive positions will be placed
//along the rim of circle further along the half-sphere. The distance between each circle, and the
//number of positions on each circle will depend on the density
CVec3Dfp32 CPositionGenerator_HalfSphere::GetNextPosition()
{
	MAUTOSTRIP(CPositionGenerator_HalfSphere_GetNextPosition, CVec3Dfp32());
	if (!IsValid())
		return CVec3Dfp32(_FP32_MAX);

	//Calculate position 
	if (m_Rad == 0)
			m_CurPos = CVec3Dfp32::GetRow(m_Mat, 3) + CVec3Dfp32::GetRow(m_Mat, 0) * m_Radius;
	else
	{
		fp32 Fwd = M_Sqrt(Sqr(m_Radius) - Sqr(m_Rad));
		fp32 Right = m_Rad * QCos(m_Angle);
		fp32 Up = m_Rad * QSin(m_Angle);
		m_CurPos = CVec3Dfp32::GetRow(m_Mat, 3) + CVec3Dfp32::GetRow(m_Mat, 0) * Fwd + CVec3Dfp32::GetRow(m_Mat, 1) * Right + CVec3Dfp32::GetRow(m_Mat, 2) * Up;
	};

	//Should we try to move on to next circle?
	m_Angle += m_dAngle;
	if (m_Angle >= 2 * _PI)
		OnNextCircle();

	return m_CurPos;
};


//Send release order to user given object ID
bool CAI_Resource_Pathfinding::SendReleaseOrder(int _iObj, CWorld_Server * _pServer)
{
	MAUTOSTRIP(CAI_Resource_Pathfinding_SendReleaseOrder, false);
	return _pServer->Message_SendToObject(CWObject_Message(CAI_Core::OBJMSG_RELEASEPATH, m_Type), _iObj) != 0;
};

//Check if there is an available resource
bool CAI_Resource_Pathfinding::IsAvailable()
{
	MAUTOSTRIP(CAI_Resource_Pathfinding_IsAvailable, false);
	for (int i = 0; i < m_lUsers.Len(); i++)
		if (m_lUsers[i].m_iUser == 0)
			//Available slot found!
			return true;
	//No available slots
	return false;
};

//Request a pathfinding instance with a priority
bool CAI_Resource_Pathfinding::Request(int _iObj, uint8 _iPriority, CWorld_Server * _pServer)
{
	MAUTOSTRIP(CAI_Resource_Pathfinding_Request, false);
	int u1,u2,p1,p2;
	if (m_lUsers.Len() > 1)
	{
		u1 = m_lUsers[0].m_iUser; 
		p1 = m_lUsers[0].m_iPrio; 
		u2 = m_lUsers[1].m_iUser; 
		p2 = m_lUsers[1].m_iPrio; 
	}

	if (!_pServer)
		return false;
	
	int i;

	//Check if requesting user already has a slot
	for (i = 0; i < m_lUsers.Len(); i++)
	{
		if (m_lUsers[i].m_iUser == _iObj)
		{
			//Found used slot!

			//If slot is reserved, check if we can allocate it now
			if (m_lUsers[i].m_bReservedOnly)
			{
				if (_pServer->GetGameTick() != m_lUsers[i].m_ReleaseTick)
				{
					ResourceHolder User;
					User.m_iUser = _iObj;
					User.m_iPrio = _iPriority;
					User.m_bReservedOnly = false;
					User.m_ReleaseTick = -1;
					m_lUsers[i] = User;
					m_bWantsResource = false;
					return true;
				}
				else
				{
					//Can't allocate it now
					return false;
				}
			}
			//If we've been using this slot, make sure we release it and then reserve it again
			else if (SendReleaseOrder(_iObj, _pServer))
			{
				//Slot released, we can reuse it
				ResourceHolder User;
				User.m_iUser = _iObj;
				User.m_iPrio = _iPriority;
				User.m_bReservedOnly = true;
				User.m_ReleaseTick = _pServer->GetGameTick();
				m_lUsers[i] = User;
				m_bWantsResource = false;
				return false;
			}
			else
			{
				//Cannot release slot... Weird, but we can't let this user have another slot!
				return false;
			}
		}
	}

	//User does not have an allocated/reserved slot yet, check if there's an available slot, or
	//a used slot with lower priority than this request
	int iLowestPrio = PRIO_MAX;
	int iLowestSlot = -1;
	for (i = 0; i < m_lUsers.Len(); i++)
	{
		if (m_lUsers[i].m_iUser == 0)
		{
			//Available slot found, use it immediately!
			ResourceHolder User;
			User.m_iUser = _iObj;
			User.m_iPrio = _iPriority;
			User.m_bReservedOnly = false;
			User.m_ReleaseTick = -1;
			m_lUsers[i] = User;
			m_bWantsResource = false;
			return true;
		}
		//Non-available slot, check prio
		else if (m_lUsers[i].m_iPrio < iLowestPrio)
		{
			iLowestPrio = m_lUsers[i].m_iPrio;
			iLowestSlot = i;
		};
	};

	//We didn't have an available slot, check if we should oust the current lowest-prio user
	//and reserve this slot for this user
	if ((iLowestPrio < _iPriority) && 
		(iLowestSlot != -1) &&
		SendReleaseOrder(m_lUsers[iLowestSlot].m_iUser, _pServer))
	{
		//We've ousted the bastard!
		ResourceHolder User;
		User.m_iUser = _iObj;
		User.m_iPrio = _iPriority;
		User.m_bReservedOnly = true;
		User.m_ReleaseTick = _pServer->GetGameTick();
		m_lUsers[iLowestSlot] = User;
		m_bWantsResource = false;

		//Note that we cannot allocate slot this frame, but must wait until next
		return false;
	}
	else
	{
		//No available slot, and cannot oust anyone. Notify all users that someone wants resource
		m_bWantsResource = true;
		return false;
	};
};

//Release currently held resource
void CAI_Resource_Pathfinding::Release(int _iObj, CWorld_Server * _pServer, bool _bNoMessage)
{
	MAUTOSTRIP(CAI_Resource_Pathfinding_Release, MAUTOSTRIP_VOID);
	int u1,u2,p1,p2;
	if (m_lUsers.Len() > 1)
	{
		u1 = m_lUsers[0].m_iUser; 
		p1 = m_lUsers[0].m_iPrio; 
		u2 = m_lUsers[1].m_iUser; 
		p2 = m_lUsers[1].m_iPrio; 
	}
	
	//Release slot and make sure path instance is released
	bool bReleased = true;

	for (int i = 0; i < m_lUsers.Len(); i++)
	{
		if (m_lUsers[i].m_iUser == _iObj)
		{
			m_lUsers[i].m_iUser = 0;
			m_lUsers[i].m_bReservedOnly = false;
			m_lUsers[i].m_iPrio = PRIO_MIN;
			m_lUsers[i].m_ReleaseTick = _pServer->GetGameTick();
			bReleased = false;
		}
	};

	if (!_bNoMessage && !bReleased && _pServer)
		SendReleaseOrder(_iObj, _pServer);
};

//Set number of resource slots and type
void CAI_Resource_Pathfinding::Init(int _n, int _Type, CWorld_Server * _pServer)
{
	MAUTOSTRIP(CAI_Resource_Pathfinding_Init, MAUTOSTRIP_VOID);
	m_Type = _Type;
	m_bWantsResource = false;

	if (_n < 0)
		_n = 0;

	int PrevLen = m_lUsers.Len();

	//Make sure users  that are about to be ousted release their path searches
	if (_pServer)
	{
		for (int i = _n; i < PrevLen; i++)
		{
			if (!m_lUsers[i].m_bReservedOnly)
				SendReleaseOrder(m_lUsers[i].m_iUser, _pServer);
		}
	};

	m_lUsers.SetLen(_n);

	//Nullify any new slots
	ResourceHolder Empty;
	Empty.m_iUser = 0;
	Empty.m_iPrio = PRIO_MIN;
	Empty.m_bReservedOnly = false;
	Empty.m_ReleaseTick = -1;
	for (int i = PrevLen; i < _n; i++)
	{
		m_lUsers[i] = Empty;
	}
};

void CAI_Resource_Pathfinding::Clean()
{	// *** CleanStatic clears this AFTER Init and it never gets reinitialized ***
	// m_lUsers.Destroy();
}

//Is this resource handler initialized?
bool CAI_Resource_Pathfinding::IsInitialized()
{
	MAUTOSTRIP(CAI_Resource_Pathfinding_IsInitialized, false);
	return m_lUsers.Len() > 0;
};

//Does someone want this search instance?
bool CAI_Resource_Pathfinding::IsWanted()
{
	MAUTOSTRIP(CAI_Resource_Pathfinding_IsWanted, false);
	return m_bWantsResource;
};


//Does this user hold a resource slot?
bool CAI_Resource_Pathfinding::SanctionedUser(int _iObj)
{
	MAUTOSTRIP(CAI_Resource_Pathfinding_SanctionedUser, false);
	for (int i = 0; i < m_lUsers.Len(); i++)
	{
		if ((m_lUsers[i].m_iUser != 0) &&
			(m_lUsers[i].m_iUser == _iObj) &&
			!m_lUsers[i].m_bReservedOnly)
			return true;
	}
	return false;
};




//CAI_Resource_Activity///////////////////////////////////////////////////////////////////

CAI_Resource_Activity::CAI_Resource_Activity()
{
	MAUTOSTRIP(CAI_Resource_Activity_ctor, MAUTOSTRIP_VOID);
	m_lUsers.Clear();
	m_MinReservedTime = 0;
	m_PollInterval = 1;
};


void CAI_Resource_Activity::Destroy()
{
	m_lUsers.Destroy();
	m_MinReservedTime = 0;
	m_PollInterval = 1;
}

//Poll resource handler if this activity level is allowed, given a priority. 
//This may cause other resource holders to lose their current activity level.
//If a reservation time is used, this is the time the slot will be considered 
//automatically polled. If the NewUse flag pointer is given this will be set 
//to true if we didn't previoulsy hold a slot or false otherwise.
bool CAI_Resource_Activity::Poll(int _iObj, uint8 _Priority, int _GameTick, int _ReservationTime, bool * _bNewUse)
{
	MAUTOSTRIP(CAI_Resource_Activity_Poll, false);
	//Check if user already has a slot, if there's available slots, or if not gets lowest prio of any 
	//slots eligible for release
	uint8 MinPrio = PRIO_MAX;
	int iBestSlot = -1;
	bool bUnusedFound = false;
	if (_bNewUse)
		*_bNewUse = false;
	for (int i = 0; i < m_lUsers.Len(); i++)
	{
#ifndef M_RTM
		ResourceHolder DebugRes = m_lUsers[i];//DEBUG
#endif
		if (m_lUsers[i].m_iUser == _iObj)
		{
			//Found slot reserved for this user. Change prio and succeed
			m_lUsers[i].m_Priority = _Priority;
			m_lUsers[i].m_LastPollTick = _GameTick + _ReservationTime;
			return true;
		}
		else if (!bUnusedFound && (m_lUsers[i].m_iUser == 0) && (m_lUsers[i].m_LastPollTick < _GameTick))
		{
			//Found first unused slot (i.e. released slot not polled this frame)
			MinPrio = PRIO_MIN;
			iBestSlot = i;
			bUnusedFound = true;
		}
		else if (m_lUsers[i].m_LastPollTick < _GameTick - m_PollInterval)
		{
			//Found a slot which hasn't been polled within the poll interval and is thus considered unused
			m_lUsers[i] = ResourceHolder();
			MinPrio = PRIO_MIN;
			iBestSlot = i;
		}
		else if ((_GameTick > m_lUsers[i].m_ReservedTick + m_MinReservedTime) &&
				 (m_lUsers[i].m_Priority < MinPrio))
		{
			//Found eligible slot with lowest prio yet
 			MinPrio = m_lUsers[i].m_Priority;
			iBestSlot = i;
		}
	}

	//Check if we can reserve best found slot
	if (m_lUsers.ValidPos(iBestSlot) &&
		((m_lUsers[iBestSlot].m_iUser == 0) || (_Priority > MinPrio)))
	{
		//Reserve slot for this user
		m_lUsers[iBestSlot].m_iUser = _iObj;
		m_lUsers[iBestSlot].m_Priority = _Priority;
		m_lUsers[iBestSlot].m_ReservedTick = _GameTick;	
		m_lUsers[iBestSlot].m_LastPollTick = _GameTick + _ReservationTime;
		if (_bNewUse)
			*_bNewUse = true;
		return true;
	}
	else
	{
		//Couldn't reserve any slot (or find reserved slot)
		return false;
	}

};


//Check if we can poll successfully, without actually polling
bool CAI_Resource_Activity::Peek(int _iObj, uint8 _Priority, int _GameTick)
{
	MAUTOSTRIP(CAI_Resource_Activity_Poll, false);

	//Check if user already has a slot, if there's an available slot or if there's a user with lower prio
	for (int i = 0; i < m_lUsers.Len(); i++)
	{
		if (m_lUsers[i].m_iUser == _iObj)
		{
			//Found slot reserved for this user
			return true;
		}
		else if ((m_lUsers[i].m_iUser == 0) && (m_lUsers[i].m_LastPollTick < _GameTick))
		{
			//Found unused slot (i.e. released slot not polled this frame)
			return true;
		}
		else if (m_lUsers[i].m_LastPollTick < _GameTick - m_PollInterval)
		{
			//Found a slot which hasn't been polled within the poll interval and is thus considered unused
			return true;
		}
		else if ((_GameTick > m_lUsers[i].m_ReservedTick + m_MinReservedTime) &&
				 (m_lUsers[i].m_Priority < _Priority))
		{
			//Found eligible slot with lower prio
			return true;
		}
	}

	//No slot available or oustable at given prio
	return false;
};


//Explicitly release resource slot held by this user
void CAI_Resource_Activity::Release(int _iObj, int _GameTick)
{
	MAUTOSTRIP(CAI_Resource_Activity_Release, MAUTOSTRIP_VOID);
	for (int i = 0; i < m_lUsers.Len(); i++)
	{
		if (m_lUsers[i].m_iUser	== _iObj)
		{
			m_lUsers[i].m_iUser = 0;
			m_lUsers[i].m_Priority = PRIO_MIN;
			m_lUsers[i].m_ReservedTick = -1;	
			//Keep last polled tick, so that a slot cannot get reused twice in the same frame
			//If reserved ahead (polled with reservation time), reset polled tick to current
			if (m_lUsers[i].m_LastPollTick > _GameTick)
				m_lUsers[i].m_LastPollTick = _GameTick;
			break;
		}
	}
};

void CAI_Resource_Activity::ReleaseAll()
{
	MAUTOSTRIP(CAI_Resource_Activity_ReleaseAll, MAUTOSTRIP_VOID);
	for (int i = 0; i < m_lUsers.Len(); i++)
	{
		m_lUsers[i].m_iUser = 0;
		m_lUsers[i].m_Priority = PRIO_MIN;
		m_lUsers[i].m_ReservedTick = -1;	
		m_lUsers[i].m_LastPollTick = -1;
		break;
	}
};

//Set number of resource slots and the minimum number of frames a slot is reserved
void CAI_Resource_Activity::Init(int _nSlots, int _MinReservedTime, int _PollInterval)
{
	MAUTOSTRIP(CAI_Resource_Activity_Init, MAUTOSTRIP_VOID);
	m_lUsers.SetLen(_nSlots);	
	m_MinReservedTime = _MinReservedTime;
	m_PollInterval = _PollInterval;
};


//Is this resource handler initialized?
bool CAI_Resource_Activity::IsInitialized()
{
	MAUTOSTRIP(CAI_Resource_Activity_IsInitialized, false);
	return m_lUsers.Len() > 0;
};

// Returns a random user from m_lUsers if any or 0 if there are none
int CAI_Resource_Activity::GetRandomUser()
{
	MAUTOSTRIP(CAI_Resource_Activity_GetRandomUser, 0);
	if (m_lUsers.Len() > 0)
	{
		int iRnd = (int)(Random * 0.999f * m_lUsers.Len());
//		if (iRnd >= m_lUsers.Len())
//		{
//			iRnd = m_lUsers.Len()-1;
//		}
		return(m_lUsers[iRnd].m_iUser);
	}
	else
	{
		return(0);
	}
};

//CAI_ActivityCounter///////////////////////////////////////////////////////////////////////////

CAI_ActivityCounter::CAI_ActivityCounter()
{
	MAUTOSTRIP(CAI_ActivityCounter_ctor, MAUTOSTRIP_VOID);
	m_CurrentActivity = 0;
	m_PreviousActivity = 0;
	m_CurrentTick = 0;
};	

//Lets a bot report current activity level. Make sure a player doesn't report or it might fuck things up.
void CAI_ActivityCounter::Report(int _ActivityLevel, int _GameTick)
{
	MAUTOSTRIP(CAI_ActivityCounter_Report, MAUTOSTRIP_VOID);
	if (_GameTick > m_CurrentTick)
	{
		//First report in the new tick, time to advance counter
		m_CurrentTick = _GameTick;

		//Current activity has been completely reported (hopefully). 
		//Transfer score to previous and reset new score
		m_PreviousActivity = m_CurrentActivity;
		m_CurrentActivity = 0;
	}

	//Add reported score to current if gametick match
	if (_GameTick == m_CurrentTick)
	{
		m_CurrentActivity += _ActivityLevel;
	}
};

//Get previous activity score (i.e. total score for all reporting bots the previous frame)
int CAI_ActivityCounter::GetScore()
{
	MAUTOSTRIP(CAI_ActivityCounter_GetScore, 0);
	return m_PreviousActivity;
};

