/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_ActionCutsceneCamera.cpp
					
	Author:			Olle Rosenquist
					
	Copyright:		Copyright O3 Games AB 2002
					
	Contents:		
					
	Comments:		Thirdperson camera for action cutscenes
					
	History:		
		021015:		Created File
\*____________________________________________________________________________________________*/

#include "PCH.h"
#include "WObj_ActionCutsceneCamera.h"
#include "../WObj_Char.h"
#include "../WObj_Game/WObj_GameMessages.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Hook.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_PosHistory.h"
#include "WObj_Model_Ladder.h"
#include "WObj_Ledge.h"

#define DEFAULTCHARHEIGHTOFFSET (70.0f)
#define DEFAULTCHARHEIGHTOFFSETCROUCH (20.0f)
#define DEFAULTDISTANCEOFFSET (70.0f)
#define DEFAULTHEIGHTOFFSET (10.0f)

#define FIXEDPOSOFFSET (0)

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Resolves a camera mode from a string
						
	Parameters:		
		_Str:			Camera mode as a string
			
	Returns:		The resolved cameramode from a string or 
					CAMERAMODE_UNDEFINED
						
	Comments:		
\*____________________________________________________________________*/
int CActionCutsceneCamera::ResolveCameraMode(CStr _Str)
{
	CStr Upper = _Str.UpperCase();
		
	if (Upper.Find("CHARACTER") != -1)
		return ACS_CAMERAMODE_CHARACTER;
	else if (Upper.Find("OBJECT") != -1)
		return ACS_CAMERAMODE_OBJECT;
	else if (Upper.Find("COMBINED") != -1)
		return ACS_CAMERAMODE_COMBINED;
	else if (Upper.Find("CONTROLLEDONCHAR") != -1)
		return ACS_CAMERAMODE_CONTROLLEDONCHAR;
	else if (Upper.Find("CONTROLLED") != -1)
		return ACS_CAMERAMODE_CONTROLLED;
	else if (Upper.Find("ENGINEPATH") != -1)
		return ACS_CAMERAMODE_ENGINEPATH;
	else if (Upper.Find("FIXEDPOS") != -1)
		return ACS_CAMERAMODE_FIXEDPOS;
	else if (Upper.Find("LEDGE") != -1)
		return ACS_CAMERAMODE_LEDGE;
	else if (Upper.Find("LADDER") != -1)
		return ACS_CAMERAMODE_LADDER;

	return ACS_CAMERAMODE_UNDEFINED;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Resolves a cameraview from a string
						
	Parameters:		
		_Str:			Camera view as a string
			
	Returns:	The resolved cameraview from a string or 
				CAMERAVIEW_UNDEFINED	
						
	Comments:		
\*____________________________________________________________________*/
int CActionCutsceneCamera::ResolveCameraView(CStr _Str)
{
	CStr Upper = _Str.UpperCase();

	if (Upper.Find("BEHIND") != -1)
		return CAMERAVIEW_BEHIND;
	else if (Upper.Find("FRONT") != -1)
		return CAMERAVIEW_FRONT;
	else if (Upper.Find("RIGHTSIDE") != -1)
		return CAMERAVIEW_RIGHTSIDE;
	else if (Upper.Find("LEFTSIDE") != -1)
		return CAMERAVIEW_LEFTSIDE;
	else if (Upper.Find("ABOVE") != -1)
		return CAMERAVIEW_ABOVE;
	else if (Upper.Find("BELOW") != -1)
		return CAMERAVIEW_BELOW;
	else if (Upper.Find("LEVELED") != -1)
		return CAMERAVIEW_LEVELED;
	else if (Upper.Find("DEFAULTXY") != -1)
		return CAMERAVIEW_DEFAULTXY;
	else if (Upper.Find("FIXEDPOSLEFT") != -1)
		return CAMERAVIEW_FIXEDPOSLEFT;
	else if (Upper.Find("FIXEDPOSRIGHT") != -1)
		return CAMERAVIEW_FIXEDPOSRIGHT;

	return CAMERAVIEW_UNDEFINED;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Resets the camera variables
						
	Comments:		
\*____________________________________________________________________*/
void CActionCutsceneCamera::OnCreate()
{
	// Just reset the camera variables
	m_CamVars.Reset();
}
// Form:    "Mode:ViewXY:ViewXY:ViewZ:DistanceOffset:HeightOffset"
// Example: "CHARACTER:BEHIND:NONE:LEVELED:100.0:60.0"
// Or if the camera is controlled
// Example: "CONTROLLED:CameraPos"
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Configures the camera from a string
						
	Parameters:		
		_Str:			Configuration string
			
	Returns:		Whether the configuration in the string was valid 
					or not
						
	Comments:		Even if the config string was invalid a default 
					mode is set so the camera should be usable
\*____________________________________________________________________*/
bool CActionCutsceneCamera::ConfigureFromString(CStr _Str)
{
	// Reset cam vars
	m_CamVars.Reset();

	// Set mode and view
	m_CamVars.m_CameraMode = ResolveCameraMode(_Str.GetStrSep(":"));

	if (m_CamVars.m_CameraMode & ACS_CAMERAMODE_MASKCONTROLLED)
	{
		// Ok, next value is the id of the position matrix for the camera (in the world)
		// Set the value in charheightoffset....
		m_CamVars.m_iCameraObject = m_pWServer->Selection_GetSingleTarget(_Str.GetStrSep(":"));
	}
	else
	{
		m_CamVars.m_CameraMode |= ResolveCameraView(_Str.GetStrSep(":"));
		m_CamVars.m_CameraMode |= ResolveCameraView(_Str.GetStrSep(":"));
		m_CamVars.m_CameraMode |= ResolveCameraView(_Str.GetStrSep(":"));
		m_CamVars.m_DistanceOffset = _Str.GetStrSep(":").Val_fp64();
		m_CamVars.m_HeightOffset = _Str.GetStrSep(":").Val_fp64();
	}

	// Verify camera
	return VerifyCameraMode(m_CamVars);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Copies the camera from the given one

	Parameters:		
		_Vars:		Camera to copy
						
	Comments:		
\*____________________________________________________________________*/
void CActionCutsceneCamera::CopyFrom(const CActionCutsceneCamera& _Camera)
{
	m_CamVars.CopyFrom(_Camera.m_CamVars);
	m_pWServer = _Camera.m_pWServer;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Makes a configurationblock from a configuration
					string
						
	Parameters:		
		_ConfigBlock:	Configuration block to use
		_Str:			Configuration string
		_pWServer:		World server, contains useful functions
			
	Returns:		Whether the configuration in the string was valid 
					or not
						
	Comments:		Even if the config string was invalid a default 
					mode is set so the config block should be usable
\*____________________________________________________________________*/
bool CActionCutsceneCamera::MakeConfigBlockFromString(ConfigBlock& _ConfigBlock, CStr _Str, 
													  CWorld_Server* _pWServer)
{
	// Set mode and view
	memset(&_ConfigBlock, 0, sizeof(ConfigBlock));
	_ConfigBlock.m_CameraMode = ResolveCameraMode(_Str.GetStrSep(":"));

	if (_ConfigBlock.m_CameraMode & ACS_CAMERAMODE_MASKCONTROLLED)
	{
		// Ok, next value is the id of the position matrix for the camera (in the world)
		// Set the value in charheightoffset....
		*(int*)&_ConfigBlock.m_DistanceOffset = _pWServer->Selection_GetSingleTarget(_Str.GetStrSep(":"));
	}
	else
	{
		_ConfigBlock.m_CameraMode |= ResolveCameraView(_Str.GetStrSep(":"));
		_ConfigBlock.m_CameraMode |= ResolveCameraView(_Str.GetStrSep(":"));
		_ConfigBlock.m_CameraMode |= ResolveCameraView(_Str.GetStrSep(":"));
		_ConfigBlock.m_DistanceOffset = _Str.GetStrSep(":").Val_fp64();
		_ConfigBlock.m_HeightOffset = _Str.GetStrSep(":").Val_fp64();
	}

	return VerifyCameraMode(_ConfigBlock);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Sets the cameratarget on a controlled camera
						
	Parameters:			
		_CameraTarget:		Camera target object
						
	Comments:			Use carefully shares variables with other types of 
						cameras in a config block
\*_____________________________________________________________________________*/
bool CActionCutsceneCamera::MakeConfigBlock(ConfigBlock& _ConfigBlock, const int32& _CameraMode, 
											const int32& _CameraTarget, const fp32& _DistanceOffset,
											const fp32& _HeightOffset)
{
	// Set mode and view
	memset(&_ConfigBlock, 0, sizeof(ConfigBlock));
	_ConfigBlock.m_CameraMode = _CameraMode;

	if (_ConfigBlock.m_CameraMode & ACS_CAMERAMODE_MASKCONTROLLED)
	{
		// Ok, next value is the id of the position matrix for the camera (in the world)
		// Set the value in charheightoffset....
		*(int*)&_ConfigBlock.m_DistanceOffset = _CameraTarget;
	}
	else
	{
		_ConfigBlock.m_DistanceOffset = _DistanceOffset;
		_ConfigBlock.m_HeightOffset = _HeightOffset;
	}

	return VerifyCameraMode(_ConfigBlock);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Configures the camera from a number of paramters
						
	Parameters:		
		_CameraMode:	The mode the camera should have
		_DistanceOffset: Distance from the target the camera should be
		_HeightOffset:	Height above/below the target the camera should 
						be
			
	Returns:		Whether the given configuration was valid or not
						
	Comments:		Even if the paramters were invalid a default 
					mode is set so the camera should be usable
\*____________________________________________________________________*/
bool CActionCutsceneCamera::ConfigureCamera(int32 _CameraMode, fp32 _DistanceOffset, 
											fp32 _HeightOffset)
{
	// Reset cam vars
	m_CamVars.Reset();

	m_CamVars.m_CameraMode = _CameraMode;

	if (_CameraMode & ACS_CAMERAMODE_MASKCONTROLLED)
	{
		m_CamVars.m_iObject = *(int*)&_DistanceOffset;
	}
	else
	{
		m_CamVars.m_DistanceOffset = _DistanceOffset;
		m_CamVars.m_HeightOffset = _HeightOffset;
	}

	// Verify camera
	return VerifyCameraMode(m_CamVars);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Configures the camera from a configuration block
						
	Parameters:		
		_ConfigBlock:	Configuration block
			
	Returns:		Whether the configuration block was valid or not	
						
	Comments:		Even if the config block was invalid a default 
					mode is set so the camera should be usable
\*____________________________________________________________________*/
bool CActionCutsceneCamera::ConfigureCamera(const ConfigBlock& _ConfigBlock)
{
		// Reset cam vars
	m_CamVars.Reset();

	m_CamVars.m_CameraMode = _ConfigBlock.m_CameraMode;

	if (_ConfigBlock.m_CameraMode & ACS_CAMERAMODE_MASKCONTROLLED)
	{
		m_CamVars.m_iCameraObject = *(int*)&_ConfigBlock.m_DistanceOffset;
		return true;
	}
	else
	{
		m_CamVars.m_DistanceOffset = _ConfigBlock.m_DistanceOffset;
		m_CamVars.m_HeightOffset = _ConfigBlock.m_HeightOffset;
		// Verify camera
		return VerifyCameraMode(m_CamVars);
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Configures a default camera
						
	Comments:		
\*____________________________________________________________________*/
void CActionCutsceneCamera::MakeDefaultCamera()
{
	// Reset cam vars
	m_CamVars.Reset();

	m_CamVars.m_CameraMode = ACS_CAMERAMODE_DEFAULT | CAMERAVIEW_DEFAULT;

	m_CamVars.m_CharacterHeightOffset = DEFAULTCHARHEIGHTOFFSET;
	m_CamVars.m_CharacterHeightOffsetCrouch = DEFAULTCHARHEIGHTOFFSET;
	m_CamVars.m_DistanceOffset = DEFAULTDISTANCEOFFSET;
	m_CamVars.m_HeightOffset = DEFAULTHEIGHTOFFSET;

	// Verify camera
	VerifyCameraMode(m_CamVars);
}

#define NUMBEREXLUDE (2)
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Refreshes the camera if it's active
						
	Comments:		
\*____________________________________________________________________*/
void CActionCutsceneCamera::OnRefresh()
{
	// If the camera isn't controlled, trace it
	if ((m_CamVars.m_CameraMode & ACS_CAMERAMODE_ACTIVE) && 
		!(m_CamVars.m_CameraMode & ACS_CAMERAMODE_MASKCONTROLLED))
	{
		// Update the camera trace
		CWObject* pObj = m_pWServer->Object_Get(m_CamVars.m_iCharacter);
		CMat4Dfp32 CamMat;
		GetCameraPosition(CamMat,pObj);

		CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
		
		// Update to pcd
		if (pCD)
			m_CamVars.SetToPCD(pCD);
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Verifies the cameramode from camera variables
						
	Parameters:		
		_CamVars:		The camera configuration to verify
			
	Returns:		Whether the configuration was ok or not
						
	Comments:		akes sure the resulting cameramode is ok, ie
					the resulting CamVars is safe to use
\*____________________________________________________________________*/
bool CActionCutsceneCamera::VerifyCameraMode(CamVars& _CamVars)
{
	return VerifyCameraMode(_CamVars.m_CameraMode, _CamVars.m_CharacterHeightOffset, 
		_CamVars.m_CharacterHeightOffsetCrouch, _CamVars.m_DistanceOffset, 
		_CamVars.m_HeightOffset);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Verifies the cameramode from a configuration block
						
	Parameters:		
		_ConfigBlock:	Configuration block to verify
			
	Returns:		Whether the configuration was ok or not
						
	Comments:		Makes sure the resulting cameramode is ok, ie
					the resulting ConfigBlock is safe to use
\*____________________________________________________________________*/
bool CActionCutsceneCamera::VerifyCameraMode(ConfigBlock& _ConfigBlock)
{
	fp32 Zero = 0.0f;
	return VerifyCameraMode(_ConfigBlock.m_CameraMode, Zero, Zero, _ConfigBlock.m_DistanceOffset, 
		_ConfigBlock.m_HeightOffset);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Verifies the camera mode
						
	Parameters:		
		_CameraMode:	Cameramode to verify
		_CharacterHeightOffset: Character height offset to verify
		_CharacterHeightOffsetCrouch: Crouching character height offset 
									 to verify
		_DistanceOffset: Distance offset to verify
		_HeightOffset:	Height offset to verify
			
	Returns:		Whether the camera configuration was ok or not
						
	Comments:		Makes sure the resulting cameramode is ok
\*____________________________________________________________________*/
bool CActionCutsceneCamera::VerifyCameraMode(int32& _CameraMode, fp32& _CharacterHeightOffset, 
		fp32& _CharacterHeightOffsetCrouch, fp32& _DistanceOffset, fp32& _HeightOffset)
{
	bool bResult = true;
	bool bIsControlled = (_CameraMode & ACS_CAMERAMODE_MASKCONTROLLED) != 0;

	// Make sure the modes and views are set
	if (!(_CameraMode & CAMERAMODE_MODESET))
	{
		_CameraMode |= ACS_CAMERAMODE_DEFAULT;
		bResult = false;
	}

	if (!bIsControlled)
	{
		if (!(_CameraMode & CAMERAVIEW_XYVIEWSET))
		{
			_CameraMode |= CAMERAVIEW_DEFAULTXY;
			bResult = false;
		}

		if (!(_CameraMode & CAMERAVIEW_ZVIEWSET))
		{
			_CameraMode |= CAMERAVIEW_DEFAULTZ;
			bResult = false;
		}
	}

	// Make sure the distance and heightoffsets are valid
	if (bIsControlled)
	{
		// Hmm, make sure the object index is ok
		if (*(int*)&_DistanceOffset == 0)
		{
			bResult = false;

			// Set default params instead...
			_CameraMode = 0;
			_CameraMode |= ACS_CAMERAMODE_DEFAULT;
			_CameraMode |= CAMERAVIEW_DEFAULTXY;
			_CameraMode |= CAMERAVIEW_DEFAULTZ;
		}
	}
	else
	{
		if (_CharacterHeightOffset < 0.0f)
		{
			_CharacterHeightOffset = 0.0f;
			bResult = false;
		}
		if (_CharacterHeightOffsetCrouch < 0.0f)
		{
			_CharacterHeightOffsetCrouch = 0.0f;
			bResult = false;
		}
		
		if (_DistanceOffset < 0.0f)
		{
			_DistanceOffset = 0.0f;
			bResult = false;
		}
		else if (_DistanceOffset > 255.0f)
		{
			_DistanceOffset = 255.0f;
			bResult = false;
		}
		
		if (_HeightOffset < 0.0f)
		{
			_HeightOffset = 0.0f;
			bResult = false;
		}
		else if (_HeightOffset > 255.0f)
		{
			_HeightOffset = 255.0f;
			bResult = false;
		}
	}

	return bResult;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Merges two cameraviews, making sure they do not
					conflict with each other
						
	Parameters:		
		_View:			Camera view to merge in
			
	Returns:		New cameraview mode
						
	Comments:		
\*____________________________________________________________________*/
int CActionCutsceneCamera::CombineViews(int _View)
{
	if (((_View & CAMERAVIEW_XSET) && !(m_CamVars.m_CameraMode & CAMERAVIEW_XSET)))
		return _View;
	if (((_View & CAMERAVIEW_YSET) && !(m_CamVars.m_CameraMode & CAMERAVIEW_YSET)))
		return _View;
	else if (!(_View & m_CamVars.m_CameraMode))
	{
		return _View;
	}

	return 0;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Configures the camera from registry keys
						
	Parameters:		
		_pKey:		Registry key
			
	Returns:		Whether a match to the keys were made or not
						
	Comments:		
\*____________________________________________________________________*/
bool CActionCutsceneCamera::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	
	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();

	if (KeyName.Find("CAMERAMODE") != -1)
	{
		m_CamVars.m_CameraMode = ResolveCameraMode(KeyValue);
	}
	else if (KeyName.Find("CAMERAVIEW") != -1)
	{
		int TempView = ResolveCameraView(KeyValue);

		// Make sure the views doesn't collide
		m_CamVars.m_CameraMode |= CombineViews(TempView);
	}
	else if (KeyName.Find("HEIGHTOFFSETCROUCH") != -1)
	{
		m_CamVars.m_CharacterHeightOffsetCrouch = KeyValuef;
	}
	else if (KeyName.Find("HEIGHTOFFSET") != -1)
	{
		m_CamVars.m_CharacterHeightOffset = KeyValuef;
	}
	else if (KeyName.Find("HEIGHTOFFSET") != -1)
	{
		m_CamVars.m_HeightOffset = KeyValuef;
	}
	else if (KeyName.Find("DISTANCEOFFSET") != -1)
	{
		m_CamVars.m_DistanceOffset = KeyValuef;
	}

	return true;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Verifies the camera mode
						
	Comments:		
\*____________________________________________________________________*/
void CActionCutsceneCamera::OnFinishEvalKeys()
{
	// Make sure mode and view has been set
	VerifyCameraMode(m_CamVars);

	return;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Sets the give camera mode (if valid)
						
	Parameters:		
		_CameraMode:	New camera mode
			
	Returns:		
						
	Comments:		
\*____________________________________________________________________*/
bool CActionCutsceneCamera::SetCameraMode(int _CameraMode)
{
	// Make sure the cameramode is usable
	if (_CameraMode & CAMERAMODE_MODESET)
	{
		m_CamVars.m_CameraMode &= ~ACS_CAMERAMODE_MASK;
		m_CamVars.m_CameraMode |= _CameraMode;

		return true;
	}

	// No correct cameramode was given
	return false;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Sets the given camera view (if valid)
						
	Parameters:		
		_CameraView:	New camera view
			
	Returns:		Whether the operation was ok or not
						
	Comments:		
\*____________________________________________________________________*/
bool CActionCutsceneCamera::SetCameraView(int _CameraView)
{
	// Make sure the cameraview is usable
	if ((_CameraView & CAMERAVIEW_XYVIEWSET) && (_CameraView & CAMERAVIEW_ZVIEWSET))
	{
		m_CamVars.m_CameraMode &= ~CAMERAVIEW_MASK;
		m_CamVars.m_CameraMode |= _CameraView;

		return true;
	}

	// No correct cameraview was given
	return false;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Toggles whether the camera is valid or not
						
	Comments:		
\*____________________________________________________________________*/
void CActionCutsceneCamera::ToggleValid()
{
	if (m_CamVars.m_CameraMode & ACS_CAMERAMODE_ACTIVE)
	{
		m_CamVars.m_CameraMode &= ~ACS_CAMERAMODE_ACTIVE;
	}
	else
	{
		m_CamVars.m_CameraMode |= ACS_CAMERAMODE_ACTIVE;
	}
}

void CActionCutsceneCamera::SetActive()
{
	m_CamVars.m_CameraMode |= ACS_CAMERAMODE_ACTIVE;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Cycles the camera modes
						
	Returns:		Whether the operation was ok or not
						
	Comments:		Does not cycle to controlled mode
\*____________________________________________________________________*/
bool CActionCutsceneCamera::CycleCameraMode()
{
	int NewMode = 0;
	switch (m_CamVars.m_CameraMode & ACS_CAMERAMODE_MASK)
	{
	case ACS_CAMERAMODE_CHARACTER:
		{
			if (m_CamVars.m_iObject != 0)
				NewMode = ACS_CAMERAMODE_OBJECT;
			else
				NewMode = ACS_CAMERAMODE_CHARACTER;

			break;
		}
	case ACS_CAMERAMODE_OBJECT:
		{
			if (m_CamVars.m_iObject != 0)
				NewMode = ACS_CAMERAMODE_COMBINED;
			else
				NewMode = ACS_CAMERAMODE_CHARACTER;

			break;
		}
	case ACS_CAMERAMODE_COMBINED:
		{
			NewMode = ACS_CAMERAMODE_CHARACTER;

			break;
		}
	}

	m_CamVars.m_CameraMode &= ~ACS_CAMERAMODE_MASK;
	m_CamVars.m_CameraMode |= NewMode;
	m_CamVars.ResetTraceHistory();

	//ConOut(GetCameraModeString(m_CamVars.m_CameraMode));

	return true;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Cycles XY plane view mode
						
	Returns:		Whether the operation was ok or not
						
	Comments:	
\*____________________________________________________________________*/
bool CActionCutsceneCamera::CycleCameraXYView()
{
	// Cycle XY view
	int NewXYView = 0;
	switch (m_CamVars.m_CameraMode & CAMERAVIEW_XYMASK)
	{
	case CAMERAVIEW_BEHIND:
		{
			NewXYView = CAMERAVIEW_BEHIND | CAMERAVIEW_RIGHTSIDE;
			break;
		}
	case CAMERAVIEW_BEHIND | CAMERAVIEW_RIGHTSIDE:
		{
			NewXYView = CAMERAVIEW_RIGHTSIDE;
			break;
		}
	case CAMERAVIEW_RIGHTSIDE:
		{
			NewXYView = CAMERAVIEW_FRONT | CAMERAVIEW_RIGHTSIDE;
			break;
		}
	case CAMERAVIEW_RIGHTSIDE | CAMERAVIEW_FRONT:
		{
			NewXYView = CAMERAVIEW_FRONT;
			break;
		}
	case CAMERAVIEW_FRONT:
		{
			NewXYView = CAMERAVIEW_LEFTSIDE | CAMERAVIEW_FRONT;
			break;
		}
	case CAMERAVIEW_LEFTSIDE | CAMERAVIEW_FRONT:
		{
			NewXYView = CAMERAVIEW_LEFTSIDE;
			break;
		}
	case CAMERAVIEW_LEFTSIDE:
		{
			NewXYView = CAMERAVIEW_BEHIND | CAMERAVIEW_LEFTSIDE;
			break;
		}
	case CAMERAVIEW_BEHIND | CAMERAVIEW_LEFTSIDE:
		{
			NewXYView = CAMERAVIEW_BEHIND;
			break;
		}
	default:
		{
			NewXYView = CAMERAVIEW_DEFAULTXY;
			break;
		}
	}

	m_CamVars.m_CameraMode &= ~CAMERAVIEW_XYMASK;
	m_CamVars.m_CameraMode |= NewXYView;
	m_CamVars.ResetTraceHistory();
	
	//ConOut(GetCameraViewString(m_CamVars.m_CameraMode));

	return true;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Cycles between existing Z axis views
						
	Returns:		Whether the operation was ok or not
						
	Comments:		
\*____________________________________________________________________*/
bool CActionCutsceneCamera::CycleCameraZView()
{
	int NewZView = 0;
	switch (m_CamVars.m_CameraMode & CAMERAVIEW_ZMASK)
	{
	case CAMERAVIEW_ABOVE:
		{
			NewZView = CAMERAVIEW_BELOW;
			break;
		}
	case CAMERAVIEW_BELOW:
		{
			NewZView = CAMERAVIEW_LEVELED;
			break;
		}
	case CAMERAVIEW_LEVELED:
		{
			NewZView = CAMERAVIEW_ABOVE;
			break;
		}
	default:
		{
			NewZView = CAMERAVIEW_DEFAULTZ;
			break;
		}
	}

	m_CamVars.m_CameraMode &= ~CAMERAVIEW_ZMASK;
	m_CamVars.m_CameraMode |= NewZView;
	m_CamVars.ResetTraceHistory();

//	ConOut(GetCameraViewString(m_CamVars.m_CameraMode));

	return true;	
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Finds the height offset of the character
						
	Parameters:		
		_CamVars:		Camera variables
		_pObj:			Character object
			
	Returns:		The height offset (if any)
						
	Comments:		
\*____________________________________________________________________*/
fp32 CActionCutsceneCamera::GetHeightOffset(CamVars& _CamVars, CWObject_CoreData* _pObj)
{
	if ((_CamVars.m_iCharacter != 0) && _pObj)
	{
		return 54.0f;
		/*return (CWObject_Character::Char_GetPhysType(_pObj) == PLAYER_PHYS_CROUCH) ? _CamVars.m_CharacterHeightOffset 
			: _CamVars.m_CharacterHeightOffsetCrouch;*/
	}

	return 0.0f;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Finds the target position for the camera
						
	Parameters:		
		_CameraMode:	Cameramode
		_CamVars:		Camera variables
		_pObj:			Character object
		_pWPhysState:	World phys state
			
	Returns:	Camera target position	
						
	Comments:		
\*____________________________________________________________________*/
CVec3Dfp32 CActionCutsceneCamera::GetCameraTargetPos(int _CameraMode, CamVars& _CamVars, 
								CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, 
								const fp32& _IPTime)
{
	switch (_CameraMode & ACS_CAMERAMODE_MASK)
	{
	case ACS_CAMERAMODE_CHARACTER:
		{
			// If on the client, use the moderated camera position
			CVec3Dfp32 Target;
			CWObject_CoreData* pObj = _pWPhysState->Object_GetCD(_CamVars.m_iCharacter);
			CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);

			if (!pObj || !pCD)
			{
				Target = _pWPhysState->Object_GetPosition(_CamVars.m_iCharacter);
			}
			else if (CWObject_Character::Char_GetControlMode(pObj) == PLAYER_CONTROLMODE_ACTIONCUTSCENE)
			{
				CMat4Dfp32 Mat;
				CWObject_ActionCutscene::GetAnimPosition(Mat, pCD, _pWPhysState);
				Target = CVec3Dfp32::GetMatrixRow(Mat,3);
			}
			else if (_pWPhysState->IsServer())
			{
				Target = _pWPhysState->Object_GetPosition(_CamVars.m_iCharacter);
			}
			else
			{
				if (_CamVars.m_LastCameraTarget == CVec3Dfp32(0.0f,0.0f,0.0f))
				{
					CVec3Dfp32 Pos = _pWPhysState->Object_GetPosition(_CamVars.m_iCharacter);
					_CamVars.m_LastCameraTarget = _CamVars.m_CameraTarget = Pos;
				}
				Target = LERP(_CamVars.m_LastCameraTarget, _CamVars.m_CameraTarget, _IPTime);
			}

			Target.k[2] += GetHeightOffset(_CamVars, _pObj);

			return Target;
		}
	case ACS_CAMERAMODE_OBJECT:
		{
			return _pWPhysState->Object_GetPosition(_CamVars.m_iObject);
		}
	case ACS_CAMERAMODE_COMBINED:
		{
			// Find position between the two objects
			if ((_CamVars.m_iObject == 0) || (_CamVars.m_iCharacter == 0))
			{
				if (_CameraMode != ACS_CAMERAMODE_DEFAULT)
					return GetCameraTargetPos(ACS_CAMERAMODE_DEFAULT, _CamVars, _pObj, 
					_pWPhysState,_IPTime);

				return 0;
			}

			// Set the target as the point in the middle between the character
			// and the object
			CVec3Dfp32 CharPos = _pWPhysState->Object_GetPosition(_CamVars.m_iCharacter);
			CharPos.k[2] += GetHeightOffset(_CamVars, _pObj);

			return (_pWPhysState->Object_GetPosition(_CamVars.m_iObject) + CharPos) / 2.0f;
		}
	case ACS_CAMERAMODE_FIXEDPOS:
		{
			// Fixedpos offsets (anglex,angley,distance)
			// If on the client, use the moderated camera position
			CVec3Dfp32 Target;
			if (_pWPhysState->IsServer())
			{
				Target = _pWPhysState->Object_GetPosition(_CamVars.m_iCharacter);
				Target[2] += FIXEDPOSOFFSET;
			}
			else
			{
				if (_CamVars.m_LastCameraTarget == CVec3Dfp32(0.0f,0.0f,0.0f))
				{
					CVec3Dfp32 Pos = _pWPhysState->Object_GetPosition(_CamVars.m_iCharacter);
					_CamVars.m_LastCameraTarget = _CamVars.m_CameraTarget = Pos;
				}
				Target = LERP(_CamVars.m_LastCameraTarget, _CamVars.m_CameraTarget, _IPTime);
			}

			Target.k[2] += GetHeightOffset(_CamVars, _pObj);

			return Target;
		}
	case ACS_CAMERAMODE_LEDGE:
		{
			// Fixedpos offsets (anglex,angley,distance)
			// If on the client, use the moderated camera position
			CVec3Dfp32 Target;
			if (_pWPhysState->IsServer())
			{
				// Get closest ledge position from character
				// Ledge object in _CamVars.m_iObject, just ask for closest position?

				CVec3Dfp32 Position = 0;
				if (_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LEDGE_GETCHARLEDGEPOS,0,0,0,0,_pWPhysState->Object_GetPosition(_CamVars.m_iCharacter),0,&Position,sizeof(CVec3Dfp32)),_CamVars.m_iObject))
				{
					Target = Position;
				}
				else
				{
					Target = _pWPhysState->Object_GetPosition(_CamVars.m_iCharacter);
				}
			}
			else
			{
				if (_CamVars.m_LastCameraTarget == CVec3Dfp32(0.0f,0.0f,0.0f))
				{
					CVec3Dfp32 Pos;// = _pWPhysState->Object_GetPosition(_CamVars.m_iCharacter);
					if (!_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LEDGE_GETCHARLEDGEPOS,0,0,0,0,_pWPhysState->Object_GetPosition(_CamVars.m_iCharacter),0,&Pos,sizeof(CVec3Dfp32)),_CamVars.m_iObject))
						Pos = _pWPhysState->Object_GetPosition(_CamVars.m_iCharacter);
					_CamVars.m_LastCameraTarget = _CamVars.m_CameraTarget = Pos;
				}
				Target = LERP(_CamVars.m_LastCameraTarget, _CamVars.m_CameraTarget, _IPTime);
			}

			//Target.k[2] += GetHeightOffset(_CamVars, _pObj);

			return Target;
		}
	case ACS_CAMERAMODE_LADDER:
		{
			// Fixedpos offsets (anglex,angley,distance)
			// If on the client, use the moderated camera position
			CVec3Dfp32 Target;
			if (_pWPhysState->IsServer())
			{
				// Get closest ledge position from character
				// Ledge object in _CamVars.m_iObject, just ask for closest position?

				CVec3Dfp32 Position = 0;
				if (_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETCHARLADDERPOS,0,0,0,0,_pWPhysState->Object_GetPosition(_CamVars.m_iCharacter),0,&Position,sizeof(CVec3Dfp32)),_CamVars.m_iObject))
				{
					Target = Position;
				}
				else
				{
					Target = _pWPhysState->Object_GetPosition(_CamVars.m_iCharacter);
				}
			}
			else
			{
				if (_CamVars.m_LastCameraTarget == CVec3Dfp32(0.0f,0.0f,0.0f))
				{
					CVec3Dfp32 Pos;// = _pWPhysState->Object_GetPosition(_CamVars.m_iCharacter);
					if (!_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETCHARLADDERPOS,0,0,0,0,_pWPhysState->Object_GetPosition(_CamVars.m_iCharacter),0,&Pos,sizeof(CVec3Dfp32)),_CamVars.m_iObject))
						Pos = _pWPhysState->Object_GetPosition(_CamVars.m_iCharacter);
					_CamVars.m_LastCameraTarget = _CamVars.m_CameraTarget = Pos;
				}
				Target = LERP(_CamVars.m_LastCameraTarget, _CamVars.m_CameraTarget, _IPTime);
			}

			Target.k[2] += GetHeightOffset(_CamVars, _pObj);

			return Target;
		}
	default:
		{
			// Hmm, no suitable cameramode found, if the given cameramode wasn't default
			// try it, otherwise return 0
			if (_CameraMode != ACS_CAMERAMODE_DEFAULT)
				return GetCameraTargetPos(ACS_CAMERAMODE_DEFAULT, _CamVars, _pObj, _pWPhysState,
										_IPTime);

			return 0;
		}
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Finds the camera view position
						
	Parameters:		
		_CameraView:	Camera mode
		_CamVars:		Cameravariables
		_pObj:			Character object
		_pWPhysState:	World phys state
		_Distance:		Wanted distance from target
			
	Returns:		The camera view position
						
	Comments:		
\*____________________________________________________________________*/
CVec3Dfp32 CActionCutsceneCamera::GetCameraViewPos(int _CameraView, CamVars& _CamVars, 
				CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, fp32& _Distance)
{
	// Get the camera view positions
	CVec3Dfp32 ViewPos;
	
	fp32 DistanceOffset = _CamVars.m_DistanceOffset;
	fp32 HeightOffset = GetCameraViewPosZ(_CameraView, _CamVars, _pObj);

	if (HeightOffset != 0)
		_Distance = M_Sqrt(HeightOffset * HeightOffset + DistanceOffset * DistanceOffset);
	else
		_Distance = DistanceOffset;

	ViewPos = GetCameraViewPosXY(_CameraView, _CamVars, _pWPhysState) * DistanceOffset;
	ViewPos.k[2] += HeightOffset;

	return ViewPos;
}

CVec3Dfp32 CActionCutsceneCamera::GetCameraViewPosClient(int _CameraView, CamVars& _CamVars, 
														CWObject_CoreData* _pObj, 
														CWorld_PhysState* _pWPhysState, 
														fp32& _Distance, const fp32& _IPTime)
{
	// Get the camera view positions
	CVec3Dfp32 ViewPos;
	
	fp32 DistanceOffset = _CamVars.m_DistanceOffset;
	fp32 HeightOffset = GetCameraViewPosZ(_CameraView, _CamVars, _pObj);

	fp32 CameraAngle = LERP(_CamVars.m_LastTargetRotation, _CamVars.m_TargetRotation, _IPTime);
	CameraAngle = MACRO_ADJUSTEDANGLE(CameraAngle);

	CVec3Dfp32 Dir;
	Dir.k[2] = 0;
	Dir.k[0] = M_Cos(CameraAngle * _PI2);
	Dir.k[1] = M_Sin(CameraAngle * _PI2);
	Dir.Normalize();

	if (HeightOffset != 0)
		_Distance = M_Sqrt(HeightOffset * HeightOffset + DistanceOffset * DistanceOffset);
	else
		_Distance = DistanceOffset;

	ViewPos = Dir * DistanceOffset;
	ViewPos.k[2] += HeightOffset;

	return ViewPos;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Gets the camera offset in the XY plane
						
	Parameters:		
		_CameraView:	Cameramode
		_CamVars:		Camera mode
		_pWPhysState:	World phys state
			
	Returns:		Resulting offset in the XY plane
						
	Comments:		
\*____________________________________________________________________*/
CVec3Dfp32 CActionCutsceneCamera::GetCameraViewPosXY(int _CameraView, CamVars& _CamVars, 
													CWorld_PhysState* _pWPhysState)
{
	// Find object direction
	if (_CamVars.m_iCharacter == 0)
		return CVec3Dfp32(1,0,0);

	CVec3Dfp32 Direction = CVec3Dfp32::GetMatrixRow(_pWPhysState->Object_GetPositionMatrix(_CamVars.m_iCharacter), 0);
	if ((_CamVars.m_CameraMode & ACS_CAMERAMODE_LEDGE) == ACS_CAMERAMODE_LEDGE)
	{
		// Get normal of ledge
		CVec3Dfp32 Data[3];
		if (_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LEDGE_GETACTIVELEDGEINFO,0,0,0,0,0,0,&Data,sizeof(Data)),_CamVars.m_iObject))
			Direction = -Data[2];
	}
	else if ((_CamVars.m_CameraMode & ACS_CAMERAMODE_LADDER) == ACS_CAMERAMODE_LADDER)
	{
		CVec3Dfp32 Data;
		if (_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETNORMAL,0,0,0,0,0,0,&Data,sizeof(CVec3Dfp32)),_CamVars.m_iObject))
			Direction = -Data;
	}
	// Find right direction
	CVec3Dfp32 Right;
	Direction.CrossProd(CVec3Dfp32(0,0,1),Right);
	Right.Normalize();

	CVec3Dfp32 Result = 0;

	if ((_CameraView & CAMERAVIEW_FIXEDPOSLEFT) == CAMERAVIEW_FIXEDPOSLEFT)
	{
		Result = Direction + Right * -0.35f;
		Result.Normalize();
		return Result;
	}
	if ((_CameraView & CAMERAVIEW_FIXEDPOSLEFT) == CAMERAVIEW_FIXEDPOSRIGHT)
	{
		Result = Direction + Right * 0.35f;
		Result.Normalize();
		return Result;
	}

	int YMode = _CameraView & CAMERAVIEW_YMASK;
	int XMode = _CameraView & CAMERAVIEW_XMASK;

	if ((YMode & CAMERAVIEW_BEHIND) == CAMERAVIEW_BEHIND)
	{
		Result = -Direction;
	}
	else if ((YMode & CAMERAVIEW_FRONT) == CAMERAVIEW_FRONT)
	{
		Result = Direction;
	}

	if ((XMode & CAMERAVIEW_RIGHTSIDE) == CAMERAVIEW_RIGHTSIDE)
	{
		Result += Right;
	}
	else if ((XMode & CAMERAVIEW_LEFTSIDE) == CAMERAVIEW_LEFTSIDE)
	{
		Result -= Right;
	}

	if (Result == CVec3Dfp32(0))
	{
		if ((_CameraView & CAMERAVIEW_XYMASK) != CAMERAVIEW_DEFAULTXY)
			return GetCameraViewPosXY(CAMERAVIEW_DEFAULTXY, _CamVars, _pWPhysState);
		else
			return Direction;
	}

	Result.Normalize();

	return Result;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Gets the camera offset on the Z axis
						
	Parameters:		
		_CameraView:	Camera view mode
		_CamVars:		Camera variables
		_pObj:			Character object
			
	Returns:		Height offset
						
	Comments:		
\*____________________________________________________________________*/
fp32 CActionCutsceneCamera::GetCameraViewPosZ(int _CameraView, CamVars& _CamVars, CWObject_CoreData* _pObj)
{
	fp32 HeightOffset = GetHeightOffset(_CamVars, _pObj);

	switch (_CameraView & CAMERAVIEW_ZMASK)
	{
	case CAMERAVIEW_ABOVE:
		{
			return _CamVars.m_HeightOffset;
		}
	case CAMERAVIEW_BELOW:
		{
			return -_CamVars.m_HeightOffset;
		}
	case CAMERAVIEW_LEVELED:
		{
			return 0;
		}
	default:
		{
			// Hmm, no correct view set, so try another
			if ((_CameraView & CAMERAVIEW_ZMASK) != CAMERAVIEW_DEFAULTZ)
				return GetCameraViewPosZ(CAMERAVIEW_DEFAULTZ, _CamVars, _pObj);
			
			return HeightOffset;
		}
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Gets the camera position on the server side, also
					does the camera tracing, should be called each 
					tick when active (to update possible collisions)
						
	Parameters:		
		_CamMat:		Resulting cameramatrix
			
	Comments:		
\*____________________________________________________________________*/
void CActionCutsceneCamera::GetCameraPosition(CMat4Dfp32& _CamMat, CWObject_CoreData* _pObj)
{
	// Camera is the target position + the view position

	if ((m_CamVars.m_CameraMode & ACS_CAMERAMODE_MASKCONTROLLED) && 
		(m_CamVars.m_iObject != 0))
	{
		if (m_CamVars.m_CameraMode & ACS_CAMERAMODE_ENGINEPATH)
		{
			if (!_pObj)
				return;
			// Get camera position from enginepath
			// Get the cameramatrix from the enginepath
			CWObject_CoreData* pEnginePath = m_pWServer->Object_GetCD(m_CamVars.m_iCameraObject);
			CWO_PosHistory *pCData = CWObject_Engine_Path::GetClientData(pEnginePath);
			
			if(pEnginePath->m_iAnim0 != -1 && !pCData->IsValid())
				CWObject_Engine_Path::LoadPath(m_pWServer, pEnginePath, pEnginePath->m_iAnim0);

			// TEST FIXME
			CMat4Dfp32 PosMat;
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pObj);
			CMTime Time;
			CWObject_Phys_Ladder::GetRelativeLadderPosition(m_pWServer,pCD,
													_pObj->GetPosition(), Time);
			pCData->GetMatrix(pEnginePath->m_iAnim2, Time.GetTime(),
				(pEnginePath->m_ClientFlags & CWObject_Attach::CLIENTFLAGS_LOOP) != 0, 
				pEnginePath->m_iAnim1, PosMat);

			CVec3Dfp32::GetMatrixRow(PosMat, 0).SetMatrixRow(_CamMat, 2);
			CVec3Dfp32::GetMatrixRow(PosMat, 1).SetMatrixRow(_CamMat, 0);
			CVec3Dfp32 Up = CVec3Dfp32::GetMatrixRow(PosMat, 2);
			Up = -Up;
			Up.SetMatrixRow(_CamMat, 1);
			CVec3Dfp32::GetMatrixRow(PosMat, 3).SetMatrixRow(_CamMat, 3);
			// Create the cam matrix
			_CamMat.RecreateMatrix(2,1);
		}
		else
		{
			// Set the matrix as the object position
			_CamMat = m_pWServer->Object_GetPositionMatrix(m_CamVars.m_iObject);

			// Create the cam matrix
			_CamMat.RecreateMatrix(2,1);
		}
	}
	else
	{
		CVec3Dfp32 Target = GetCameraTargetPos();
		fp32 Distance;
		CVec3Dfp32 ViewPos = GetCameraViewPos(Distance);
		CVec3Dfp32 CamPos = Target + ViewPos;
		CVec3Dfp32 Direction = Target - CamPos;
		Direction.Normalize();

		CVec3Dfp32(0.0f,0.0f,-1.0f).SetMatrixRow(_CamMat, 1);
		Direction.SetMatrixRow(_CamMat, 2);
		CamPos.SetMatrixRow(_CamMat, 3);

		// Set camera angle
		CVec3Dfp32 Angles = CWObject_Character::GetLook(Direction);
		m_CamVars.m_CameraAngle = 1.0f - Angles[2] + 0.5;
		m_CamVars.m_CameraAngle = MACRO_ADJUSTEDANGLE(m_CamVars.m_CameraAngle);

		// Create the cam matrix
		_CamMat.RecreateMatrix(2,1);

		int liExclude[NUMBEREXLUDE];
		int NumberExclude = NUMBEREXLUDE;
		GetExcludeObject(liExclude, NumberExclude);

		fp32 TraceDistance = Distance;
		CWObject* pObj = m_pWServer->Object_Get(m_CamVars.m_iCharacter);
		if (pObj)
			CameraTrace(pObj, m_pWServer, _CamMat, Target, TraceDistance, 
			true, liExclude, NumberExclude, false);

		// We hit something
		if (TraceDistance != Distance)
		{
			// Decrease distance with 5% to get away from any walls
			CamPos = Target - Direction * TraceDistance * 0.95f;
			CamPos.SetMatrixRow(_CamMat, 3);
			// Create the cam matrix
			//_CamMat.RecreateMatrix(2,1);
		}
	}
}

extern fp32 AngleAdjust(fp32 _AngleA, fp32 _AngleB);
extern CVec3Dfp32 VectorFromAngles(fp32 _Pitch, fp32 _Yaw);

void CActionCutsceneCamera::OnClientRefresh(CamVars& _CamVars, CWorld_PhysState* _pWPhys, CWO_Character_ClientData* _pCD)
{
	MSCOPESHORT( CActionCutsceneCamera::OnClientRefresh );
	if (!_pCD)
		return;
	int32 CamMode = _CamVars.m_CameraMode & ~CAMERAMODE_MODESET;
	int32 ModSpeed = (CamMode & (ACS_CAMERAMODE_LEDGE|ACS_CAMERAMODE_LADDER)? 100 : 160);
	// Update camera variables if not controlled
	if (!(m_CamVars.m_CameraMode & ACS_CAMERAMODE_MASKCONTROLLED))
	{
		// Moderate to new distance..
		fp32 WantedDistance = (_CamVars.m_fTraceHistory == 0.0f ? DEFAULTDISTANCEOFFSET : _CamVars.m_fTraceHistory);
		if ((_CamVars.m_LastTargetDistance == 0.0f) && (_CamVars.m_TargetDistance == 0.0f) && 
			(WantedDistance != 0.0f))
		{
			// If new camera, reset variables
			_CamVars.m_LastTargetDistance = _CamVars.m_TargetDistance = WantedDistance;
			_CamVars.m_DistanceChange = 0.0f;
			_CamVars.m_LastTargetRotation = _CamVars.m_TargetRotation = _CamVars.m_CameraAngle;
			_CamVars.m_RotationChange = 0.0f;
			CVec3Dfp32 Pos = _pWPhys->Object_GetPosition(_CamVars.m_iCharacter);
			if (CamMode & ACS_CAMERAMODE_LEDGE)
			{
				CVec3Dfp32 Temp;
				if (_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LEDGE_GETCHARLEDGEPOS,0,0,0,0,Pos,0,&Temp,sizeof(CVec3Dfp32)),_CamVars.m_iObject))
					Pos = Temp;
			}
			else if (CamMode & ACS_CAMERAMODE_LADDER)
			{
				CVec3Dfp32 Temp;
				if (_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETCHARLADDERPOS,0,0,0,0,Pos,0,&Temp,sizeof(CVec3Dfp32)),_CamVars.m_iObject))
					Pos = Temp;
			}
			_CamVars.m_LastCameraTarget = _CamVars.m_CameraTarget = Pos;
			_CamVars.m_CameraTargetChange = 0.0f;
			_CamVars.m_AngleOffsetX = _CamVars.m_LastAngleOffsetX = 
				_CamVars.m_TargetAngleOffsetX = _pCD->m_ActionCutSceneCameraOffsetX;
			_CamVars.m_AngleOffsetXChange = 0.0f;
			_CamVars.m_AngleOffsetY = _CamVars.m_LastAngleOffsetY = 
				_CamVars.m_TargetAngleOffsetY = _pCD->m_ActionCutSceneCameraOffsetY;
			_CamVars.m_AngleOffsetYChange = 0.0f;
		}

		_CamVars.m_LastTargetDistance = _CamVars.m_TargetDistance;
		WantedDistance *= 100.0f;
		_CamVars.m_TargetDistance *= 100.0f;
		_CamVars.m_DistanceChange *= 100.0f;
		Moderatef(_CamVars.m_TargetDistance, WantedDistance, _CamVars.m_DistanceChange, 
			ModSpeed);
		_CamVars.m_TargetDistance *= (1.0f/100.0f);
		_CamVars.m_DistanceChange *= (1.0f/100.0f);
	
		// Update camera rotation angle, wanted angle is
		fp32 WantedAngle = _CamVars.m_CameraAngle;
		WantedAngle = MACRO_ADJUSTEDANGLE(WantedAngle);
		WantedAngle += AngleAdjust(_CamVars.m_TargetRotation, WantedAngle);

		_CamVars.m_LastTargetRotation = _CamVars.m_TargetRotation;
		WantedAngle *= 100.0f;
		_CamVars.m_TargetRotation *= 100.0f;
		_CamVars.m_RotationChange *= 100.0f;
		Moderatef(_CamVars.m_TargetRotation, WantedAngle, _CamVars.m_RotationChange, 
			ModSpeed);
		_CamVars.m_TargetRotation *= (1.0f/100.0f);
		_CamVars.m_RotationChange *= (1.0f/100.0f);

		_CamVars.m_LastCameraTarget = _CamVars.m_CameraTarget;
		// Wanted target is the characters position
		CVec3Dfp32 Pos = _pWPhys->Object_GetPosition(_CamVars.m_iCharacter);
		if (CamMode & ACS_CAMERAMODE_LEDGE)
		{
			CVec3Dfp32 Temp;
			if (_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LEDGE_GETCHARLEDGEPOS,0,0,0,0,Pos,0,&Temp,sizeof(CVec3Dfp32)),_CamVars.m_iObject))
				Pos = Temp;
		}
		else if (CamMode & ACS_CAMERAMODE_LADDER)
		{
			CVec3Dfp32 Temp;
			if (_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETCHARLADDERPOS,0,0,0,0,Pos,0,&Temp,sizeof(CVec3Dfp32)),_CamVars.m_iObject))
				Pos = Temp;
		}

		ModerateVec3f(_CamVars.m_CameraTarget, Pos, _CamVars.m_CameraTargetChange,ModSpeed);
	}

	// Update angle offsets
	//ConOut(CStrF("Massa update: %f", _pWPhys->GetGameTime()));
	fp32 WantedAngleOffset = _CamVars.m_AngleOffsetX * 100;
	_CamVars.m_LastAngleOffsetX = _CamVars.m_TargetAngleOffsetX;
	_CamVars.m_TargetAngleOffsetX *= 100.0f;
	_CamVars.m_AngleOffsetXChange *= 100.0f;
	Moderatef(_CamVars.m_TargetAngleOffsetX, WantedAngleOffset, _CamVars.m_AngleOffsetXChange, 
		ModSpeed);
	_CamVars.m_TargetAngleOffsetX *= (1.0f/100.0f);
	_CamVars.m_AngleOffsetXChange *= (1.0f/100.0f);

	WantedAngleOffset = _CamVars.m_AngleOffsetY * 100;
	_CamVars.m_LastAngleOffsetY = _CamVars.m_TargetAngleOffsetY;
	_CamVars.m_TargetAngleOffsetY *= 100.0f;
	_CamVars.m_AngleOffsetYChange *= 100.0f;
	Moderatef(_CamVars.m_TargetAngleOffsetY, WantedAngleOffset, _CamVars.m_AngleOffsetYChange, 
		ModSpeed);
	_CamVars.m_TargetAngleOffsetY *= (1.0f/100.0f);
	_CamVars.m_AngleOffsetYChange *= (1.0f/100.0f);

	_CamVars.UpdateModVars(_pCD);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Gets the camera matrix on the client side
						
	Parameters:		
		_CamMat:		Result of the operation will be put here
		_CamVars:		Camera variables
		_pObj:			Character object that uses the camera
		_pWPhysState:	World physstate
		_IPTime:		Time fraction
			
	Comments:		
\*____________________________________________________________________*/
void CActionCutsceneCamera::GetCameraPositionClient(CMat4Dfp32& _CamMat, CamVars& _CamVars, 
							CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, fp32 _IPTime)
{
	if ((_CamVars.m_CameraMode & ACS_CAMERAMODE_MASKCONTROLLED) && 
		(_CamVars.m_iCameraObject != 0))
	{
		// Check if it's a enginepath target first, which can only be used on ladders and 
		// hangrails
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pObj);
		CMTime Time;
		bool bGotLadder = CWObject_Phys_Ladder::GetRelativeLadderPosition(_pWPhysState,pCD,
			_pObj->GetPosition(), Time);
		if (bGotLadder)
		{
			// Scale with enginepath duration
			int32 Scale = _pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_ENGINEPATH_GETDURATION,0),
				_CamVars.m_iCameraObject);
			Time.Scale(*(fp32*)&Scale);
			CMat4Dfp32 PosMat;
			int32 ControlMode = CWObject_Character::Char_GetControlMode(_pObj);
			CWObject_Message EPRM(OBJMSG_HOOK_GETRENDERMATRIX,(aint)&Time,(aint)&PosMat);
			if (_pWPhysState->Phys_Message_SendToObject(EPRM,_CamVars.m_iCameraObject) &&
				(ControlMode != PLAYER_CONTROLMODE_ACTIONCUTSCENE))
			{
				// Get the cameramatrix from the enginepath
				CVec3Dfp32::GetMatrixRow(PosMat, 0).SetMatrixRow(_CamMat, 2);
				CVec3Dfp32::GetMatrixRow(PosMat, 1).SetMatrixRow(_CamMat, 0);
				CVec3Dfp32 Up = CVec3Dfp32::GetMatrixRow(PosMat, 2);
				Up = -Up;
				Up.SetMatrixRow(_CamMat, 1);
				CVec3Dfp32::GetMatrixRow(PosMat, 3).SetMatrixRow(_CamMat, 3);
				// Create the cam matrix
				_CamMat.RecreateMatrix(2,1);
			}
			else
			{
				bGotLadder = false;
			}
		}
		if (!bGotLadder)
		{
			CMat4Dfp32 PosMat;
			// Set the matrix as the object position
			PosMat = _pWPhysState->Object_GetPositionMatrix(_CamVars.m_iCameraObject);

			CVec3Dfp32::GetMatrixRow(PosMat, 0).SetMatrixRow(_CamMat, 2);
			CVec3Dfp32::GetMatrixRow(PosMat, 1).SetMatrixRow(_CamMat, 0);
			CVec3Dfp32 Up = CVec3Dfp32::GetMatrixRow(PosMat, 2);
			Up = -Up;
			Up.SetMatrixRow(_CamMat, 1);
			CVec3Dfp32::GetMatrixRow(PosMat, 3).SetMatrixRow(_CamMat, 3);
			
			if ((_CamVars.m_CameraMode & ACS_CAMERAMODE_MASK) == ACS_CAMERAMODE_CONTROLLEDONCHAR)
			{
				// Change view direction
				CVec3Dfp32 CharPos = _pWPhysState->Object_GetPosition(_CamVars.m_iCharacter);
				CharPos.k[2] += GetHeightOffset(_CamVars, 
					_pWPhysState->Object_GetCD(_CamVars.m_iCharacter));
				
				CVec3Dfp32 Direction = CharPos - CVec3Dfp32::GetMatrixRow(_CamMat, 3);
				Direction.Normalize();
				Direction.SetMatrixRow(_CamMat, 2);

				CVec3Dfp32 Up(0,0,-1.0f);
				Up.SetMatrixRow(_CamMat,1);
			}

			// Create the cam matrix
			_CamMat.RecreateMatrix(2,1);
		}

		/*if ((_CamVars.m_CameraMode & ACS_CAMERAMODE_ENGINEPATH) == ACS_CAMERAMODE_ENGINEPATH)
		{
			// Get the cameramatrix from the enginepath
			CWObject_CoreData* pEnginePath = _pWPhysState->Object_GetCD(_CamVars.m_iCameraObject);
			CWO_PosHistory *pCData = CWObject_Engine_Path::GetClientData(pEnginePath);
			
			if(pEnginePath->m_iAnim0 != -1 && !pCData->IsValid())
				CWObject_Engine_Path::LoadPath(_pWPhysState, pEnginePath, pEnginePath->m_iAnim0);

			// TEST FIXME
			CMat4Dfp32 PosMat;
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pObj);
			fp32 Time = CWObject_Phys_Ladder::GetRelativeLadderPosition(_pWPhysState,pCD,
													_pObj->GetPosition());

			// Scale with enginepath duration
			if (pCData->m_lSequences.Len() > 0)
			{
				Time *= pCData->m_lSequences[0].GetDuration();

				pCData->GetMatrix(pEnginePath->m_iAnim2, Time,
					(pEnginePath->m_ClientFlags & CWObject_Attach::CLIENTFLAGS_LOOP) != 0, 
					pEnginePath->m_iAnim1, PosMat);

				CVec3Dfp32::GetMatrixRow(PosMat, 0).SetMatrixRow(_CamMat, 2);
				CVec3Dfp32::GetMatrixRow(PosMat, 1).SetMatrixRow(_CamMat, 0);
				CVec3Dfp32 Up = CVec3Dfp32::GetMatrixRow(PosMat, 2);
				Up = -Up;
				Up.SetMatrixRow(_CamMat, 1);
				CVec3Dfp32::GetMatrixRow(PosMat, 3).SetMatrixRow(_CamMat, 3);
				// Create the cam matrix
				_CamMat.RecreateMatrix(2,1);
			}
		}
		else
		{
			// Set the matrix as the object position
			CMat4Dfp32 PosMat = _pWPhysState->Object_GetPositionMatrix(_CamVars.m_iCameraObject);

			CVec3Dfp32::GetMatrixRow(PosMat, 0).SetMatrixRow(_CamMat, 2);
			CVec3Dfp32::GetMatrixRow(PosMat, 1).SetMatrixRow(_CamMat, 0);
			CVec3Dfp32 Up = CVec3Dfp32::GetMatrixRow(PosMat, 2);
			Up = -Up;
			Up.SetMatrixRow(_CamMat, 1);
			CVec3Dfp32::GetMatrixRow(PosMat, 3).SetMatrixRow(_CamMat, 3);
			
			if ((_CamVars.m_CameraMode & ACS_CAMERAMODE_MASK) == ACS_CAMERAMODE_CONTROLLEDONCHAR)
			{
				// Change view direction
				CVec3Dfp32 CharPos = _pWPhysState->Object_GetPosition(_CamVars.m_iCharacter);
				CharPos.k[2] += GetHeightOffset(_CamVars, 
					_pWPhysState->Object_GetCD(_CamVars.m_iCharacter));
				
				CVec3Dfp32 Direction = CharPos - CVec3Dfp32::GetMatrixRow(_CamMat, 3);
				Direction.Normalize();
				Direction.SetMatrixRow(_CamMat, 2);

				CVec3Dfp32 Up(0,0,-1.0f);
				Up.SetMatrixRow(_CamMat,1);
			}

			// Create the cam matrix
			_CamMat.RecreateMatrix(2,1);
		}*/
	}
	else
	{
		// Camera is the target position + the view position
		fp32 Distance;
		CVec3Dfp32 Target = GetCameraTargetPos(_CamVars.m_CameraMode, _CamVars, _pObj, 
			_pWPhysState, _IPTime);
		CVec3Dfp32 ViewPos = GetCameraViewPosClient(_CamVars.m_CameraMode, _CamVars, _pObj, _pWPhysState,
			Distance, _IPTime);
		CVec3Dfp32 CamPos = Target + ViewPos;
		CVec3Dfp32 Direction = Target - CamPos;
		Direction.Normalize();

		CVec3Dfp32(0.0f,0.0f,-1.0f).SetMatrixRow(_CamMat, 1);
		Direction.SetMatrixRow(_CamMat, 2);

		// Decrease distance with 5% to get away from any walls
		CamPos = Target - Direction * 
			LERP(_CamVars.m_LastTargetDistance, _CamVars.m_TargetDistance, _IPTime) * 0.95f;
		CamPos.SetMatrixRow(_CamMat, 3);

		// Create the cam matrix (maybe not neccessary because of additional stuff...)
		_CamMat.RecreateMatrix(2,1);
	}

	// Ok, extract angles from direction
	// Add the offsets
	// Transform back into direction and set to camera
	CVec3Dfp32 Direction = CVec3Dfp32::GetMatrixRow(_CamMat,2);
	CVec3Dfp32 Angles = CWObject_Character::GetLook(Direction);

	// Add "x" "y" offsets

	fp32 OffsetX = _CamVars.m_TargetAngleOffsetX;
	fp32 OffsetY = _CamVars.m_TargetAngleOffsetY;
	fp32 LastOffsetX = _CamVars.m_LastAngleOffsetX;
	fp32 LastOffsetY = _CamVars.m_LastAngleOffsetY;
	if ((_CamVars.m_CameraMode & ACS_CAMERAMODE_FIXEDPOS)== ACS_CAMERAMODE_FIXEDPOS)
	{
		OffsetX -= 0.07f;
		OffsetY -= 0.03f;
		LastOffsetX -= 0.07f;
		LastOffsetY -= 0.03f;
	}
	
	Angles.k[2] += LERP(LastOffsetX, OffsetX, _IPTime);
	Angles.k[1] += LERP(LastOffsetY, OffsetY, _IPTime);
	
	/*Direction = VectorFromAngles(Angles.k[1],Angles.k[2]);
	Direction.k[1] = -Direction.k[1]; //hmm?*/

	CMat4Dfp32 Mat;
	Angles.CreateMatrixFromAngles(1,Mat);
	Mat.RotX_x_M(-0.25f);
	Mat.RotY_x_M(0.25f);
	CVec3Dfp32::GetMatrixRow(_CamMat,3).SetMatrixRow(Mat,3);
	_CamMat = Mat;
	
	// Must recreate up vector (can't trust side anymore either)
	/*CVec3Dfp32 Side;
	CVec3Dfp32 Up;
	Direction.CrossProd(CVec3Dfp32(0.0f,0.0f,1.0f),Side);
	Direction.CrossProd(Side,Up);
	
	Up.SetMatrixRow(_CamMat,1);
	Direction.SetMatrixRow(_CamMat, 2);
	
	// Create the cam matrix
	_CamMat.RecreateMatrix(2,1);*/
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Gets the objects to exclude from camera trace
						
	Parameters:		
		_liExlude:		int array to place objects to exclude
		_Number:		How many positions in the exlude array, and 
						when the function returns, how many objects	
						were actually added to the array
			
	Returns:		True if any object ids were added to the array
					false if no objects were added to the array or 
					something went wrong
						
	Comments:		
\*____________________________________________________________________*/
bool CActionCutsceneCamera::GetExcludeObject(int* _liExlude, int& _Number)
{
	bool Result = false;
	if (_liExlude == NULL)
	{
		_Number = 0;
		return false;
	}
	
	switch (m_CamVars.m_CameraMode & ACS_CAMERAMODE_MASK)
	{
	case ACS_CAMERAMODE_CHARACTER:
		{
			if (_Number > 0)
			{
				_liExlude[0] = m_CamVars.m_iCharacter;
				_Number = 1;
				if (m_CamVars.m_iObject != 0)
				{
					_liExlude[1] = m_CamVars.m_iObject;
					_Number++;
				}
				
				Result = true;
			}
			break;
		}
	case ACS_CAMERAMODE_OBJECT:
		{
			if (_Number > 0)
			{
				_liExlude[0] = m_CamVars.m_iObject;
				_Number = 1;
				if (m_CamVars.m_iCharacter != 0)
				{
					_liExlude[1] = m_CamVars.m_iCharacter;
					_Number++;
				}
				Result = true;
			}
			break;
		}
	case ACS_CAMERAMODE_COMBINED:
		{
			if (_Number > 1)
			{
				_liExlude[0] = m_CamVars.m_iCharacter;
				_liExlude[1] = m_CamVars.m_iObject;
				_Number = 2;
				Result = true;
			}
			break;
		}
	case ACS_CAMERAMODE_FIXEDPOS:
	case ACS_CAMERAMODE_LEDGE:
	case ACS_CAMERAMODE_LADDER:
		{
			if (_Number > 0)
			{
				_liExlude[0] = m_CamVars.m_iCharacter;
				_Number = 1;
				if (m_CamVars.m_iObject != 0)
				{
					_liExlude[1] = m_CamVars.m_iObject;
					_Number++;
				}
				
				Result = true;
			}
			break;
		}
	default:
		{
			_Number = 0;
			break;
		}
	}

	return Result;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Draws the camera and the target of the camera
						
	Comments:		
\*____________________________________________________________________*/
void CActionCutsceneCamera::DebugDrawCamera()
{
	CMat4Dfp32 Mat;

	CWObject* pObj = m_pWServer->Object_Get(m_CamVars.m_iCharacter);
	GetCameraPosition(Mat,pObj);

	m_pWServer->Debug_RenderMatrix(Mat, 1.0f, true, 0xff7f0000, 0xff007f00, 0xff00007f);
	
	CMat4Dfp32 PosMat = m_pWServer->Object_GetPositionMatrix(m_CamVars.m_iCharacter);
	CVec3Dfp32 Temp = CVec3Dfp32::GetMatrixRow(PosMat, 3);
	CVec3Dfp32 Target = GetCameraTargetPos();
	Temp.k[2] += 20;
	Temp.SetMatrixRow(PosMat, 3);

	m_pWServer->Debug_RenderVertex(Target, 0xFFFF0000, 1.0f);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Sets the character and object variables
						
	Parameters:		
		_iCharacter:	Character object id
		_iObject:		Focus object id
			
	Comments:		
\*____________________________________________________________________*/
void CActionCutsceneCamera::SetCharacterAndObject(int _iCharacter, int _iObject)
{
	// Set character and object indices
	m_CamVars.m_iCharacter = _iCharacter;

	if (!(m_CamVars.m_CameraMode & ACS_CAMERAMODE_MASKCONTROLLED))
	{
		m_CamVars.m_iObject = _iObject;
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Draws the given box
						
	Parameters:		
		_Box:			The box to draw
			
	Comments:		
\*____________________________________________________________________*/
void CActionCutsceneCamera::DebugRenderBox(const CBox3Dfp32& _Box)
{
	CVec3Dfp32 Center;
	_Box.GetCenter(Center);
	fp32 Width = Abs(_Box.m_Min.k[0]  - _Box.m_Max.k[0]);
	fp32 Depth = Abs(_Box.m_Min.k[1]  - _Box.m_Max.k[1]);
	fp32 BoxHeight = Abs(_Box.m_Min.k[2]  - _Box.m_Max.k[2]);
	CVec3Dfp32 Lower = _Box.m_Min;/*(Min(_Box.m_Min.k[0], _Box.m_Max.k[0]), Min(_Box.m_Min.k[0], _Box.m_Max.k[0]),
	Min(_Box.m_Min.k[2], _Box.m_Max.k[2]));*/
	CVec3Dfp32 Point1 = Lower + CVec3Dfp32(Width,0,0);
	CVec3Dfp32 Point2 = Lower + CVec3Dfp32(0,Depth,0);
	CVec3Dfp32 Point3 = Lower + CVec3Dfp32(Width,Depth,0);
	CVec3Dfp32 Height = CVec3Dfp32(0,0,BoxHeight);

	m_pWServer->Debug_RenderWire(Lower, Point1, 0xffffffff, 1.0f);
	m_pWServer->Debug_RenderWire(Lower, Point2, 0xffffffff, 1.0f);
	m_pWServer->Debug_RenderWire(Point1, Point3, 0xffffffff, 1.0f);
	m_pWServer->Debug_RenderWire(Point2, Point3, 0xffffffff, 1.0f);

	m_pWServer->Debug_RenderWire(Lower, Lower + Height, 0xffffffff, 1.0f);
	m_pWServer->Debug_RenderWire(Point1, Point1 + Height, 0xffffffff, 1.0f);
	m_pWServer->Debug_RenderWire(Point2, Point2 + Height, 0xffffffff, 1.0f);
	m_pWServer->Debug_RenderWire(Point3, Point3 + Height, 0xffffffff, 1.0f);

	m_pWServer->Debug_RenderWire(Lower + Height, Point1 + Height, 0xffffffff, 1.0f);
	m_pWServer->Debug_RenderWire(Lower + Height, Point2 + Height, 0xffffffff, 1.0f);
	m_pWServer->Debug_RenderWire(Point1 + Height, Point3 + Height, 0xffffffff, 1.0f);
	m_pWServer->Debug_RenderWire(Point2 + Height, Point3 + Height, 0xffffffff, 1.0f);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Translates the camera mode into a string
						
	Parameters:		
		_CameraMode:	Cameramode flags
			
	Returns:		Cameramode as a string
						
	Comments:		
\*____________________________________________________________________*/
CStr CActionCutsceneCamera::GetCameraModeString(int _CameraMode)
{
	switch (_CameraMode & ACS_CAMERAMODE_MASK)
	{
	case ACS_CAMERAMODE_OBJECT:
		return CStr("CameraMode: Object");
	case ACS_CAMERAMODE_CHARACTER:
		return CStr("CameraMode: Character");
	case ACS_CAMERAMODE_COMBINED:
		return CStr("CameraMode: Combined");
	default:
		return CStr("CameraMode: Error, no cameramode defined");
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Translates the camera view into a string
						
	Parameters:		
		_CameraView:	Cameraview flags
			
	Returns:		Cameraview as a string
						
	Comments:		
\*____________________________________________________________________*/
CStr CActionCutsceneCamera::GetCameraViewString(int _CameraView)
{
	CStr Result;
	switch (_CameraView & CAMERAVIEW_YMASK)
	{
	case CAMERAVIEW_BEHIND:
		Result = CStr("CameraViewXY: Behind +");
		break;
	case CAMERAVIEW_FRONT:
		Result = CStr("CameraViewXY: Front +");
		break;
	default:
		Result = CStr("CameraViewXY: None +");
		break;
	}

	switch (_CameraView & CAMERAVIEW_XMASK)
	{
	case CAMERAVIEW_RIGHTSIDE:
		Result += CStr(" Rightside ");
		break;
	case CAMERAVIEW_LEFTSIDE:
		Result += CStr(" Leftside ");
		break;
	default:
		Result += CStr(" None ");
		break;
	}

	switch (_CameraView & CAMERAVIEW_ZMASK)
	{
	case CAMERAVIEW_ABOVE:
		Result += CStr("CameraViewZ: Above");
		break;
	case CAMERAVIEW_BELOW:
		Result += CStr("CameraViewZ: Below");
		break;
	case CAMERAVIEW_LEVELED:
		Result += CStr("CameraViewZ: Leveled");
		break;
	default:
		Result += CStr("CameraViewZ: Error, no cameraview z defined");
		break;
	}

	return Result;
}

// Copied from CharCamera
#define CAMTRACE_RINGS (4)
#define CAMTRACE_SIDES (4)
#define MAX_ROTATIONS	5
#define ROTATION_ANGLE	0.05f
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		
						
	Parameters:		
		_pObj:			Character object
		_pWPhysState:	World physstate
		_CameraMatrix:	Camera position, direction and so on
		Target:			Target position
		_WantedDistance:Wanted distance between target and camera 
						position
		_Rotate:		Whether to rotate the trace rays or not
		_liExclude:		Array of objects to exclude from trace
		_NumberExclude:	Number of objects to exclude from trace
		bDebugRender:	Whether to draw debug information
			
	Returns:		Whether something was hit or not
						
	Comments:		
\*____________________________________________________________________*/
bool CActionCutsceneCamera::CameraTrace(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, 
							  CMat4Dfp32& _CameraMatrix, CVec3Dfp32 Target, fp32& _WantedDistance, 
							  bool _Rotate, int* _liExclude, int _NumberExclude, bool bDebugRender)
{
	// Debug stuff
	fp32 DebugDuration	= 1.0f;
	bool bWallHit		= false;
	
	if (bDebugRender) _pWPhysState->Debug_RenderVertex(Target, 0xFFFF0000, DebugDuration);

	// Get the lookat directions
	CVec3Dfp32 Side, Up, Fwd, ViewPos;
	Side = CVec3Dfp32::GetMatrixRow(_CameraMatrix, 0);
	Up = CVec3Dfp32::GetMatrixRow(_CameraMatrix, 1);
	Fwd	= CVec3Dfp32::GetMatrixRow(_CameraMatrix, 2);
	ViewPos = CVec3Dfp32::GetMatrixRow(_CameraMatrix, 3);

	// =====================================================================================
	// Calculate the rays we're gonna use for tracing (so we can optimize the selection box)
	CBox3Dfp32 Box(Target, Target);
	CVec3Dfp32 Rays[CAMTRACE_RINGS * CAMTRACE_SIDES];
	int iNumRays = 0;
	Rays[iNumRays] = ViewPos;
	Box.Expand(CBox3Dfp32(ViewPos, ViewPos));
	
	for (int iRing = 1; (iRing - 1) < CAMTRACE_RINGS; iRing++)
	{
		fp32 segmentfraction = ((fp32)iRing *(1.0f/(fp32)CAMTRACE_RINGS));
		if(_Rotate) segmentfraction += (ROTATION_ANGLE * m_CamVars.m_iRotationIndex);
		fp32 a = segmentfraction * (0.25f * _PIHALF);
		fp32 cosa = M_Cos(a);
		fp32 sina = M_Sin(a);
		
		for (int iSide = 0; iSide < CAMTRACE_SIDES; iSide++)
		{
			fp32 ringfraction = ((fp32)iSide *(1.0f/ (fp32)CAMTRACE_SIDES));
			if(_Rotate) ringfraction += (ROTATION_ANGLE * m_CamVars.m_iRotationIndex);
			fp32 b = ringfraction * _PI2;
			fp32 cosb = M_Cos(b) * sina;
			fp32 sinb = M_Sin(b) * sina;
			fp32 DistanceScale = 0.4f + 0.6f * Sqr(Sqr(cosa));
			CVec3Dfp32 ray = (-Fwd * cosa + Side * cosb + Up * sinb) * _WantedDistance * DistanceScale;
			iNumRays++;
			if(iNumRays < (CAMTRACE_RINGS * CAMTRACE_SIDES))
			{
				CVec3Dfp32 Ray = Target + ray;
				Rays[iNumRays] = Ray;
				Box.Expand(CBox3Dfp32(Ray,Ray));
			}
		}
	}

	// Debug render the resulting "trace box"
	if (bDebugRender)
		DebugRenderBox(Box);


	bool bHit;
	CCollisionInfo CInfo;
	int OwnFlags = OBJECT_FLAGS_PROJECTILE;
	// NEW TEST WITH CHARS AND OBJECTS....
	// Exclude characters
	int Objects = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
	int Mediums = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_CAMERASOLID | 
		XW_MEDIUM_PLAYERSOLID | XW_MEDIUM_DYNAMICSSOLID;
	
	// Create selection, to optimize tracing.
	TSelection<CSelection::LARGE_BUFFER> Selection;
	_pWPhysState->Selection_AddBoundBox(Selection, Objects, Box.m_Min, Box.m_Max);
	
	// Remove possible target object from selection (might not be needed later on...)
	if (_liExclude != NULL && _NumberExclude > 0)
	{
		for (int i = 0; i < _NumberExclude; i++)
			_pWPhysState->Selection_RemoveOnIndex(Selection, _liExclude[i]);
	}

	fp32 NewDistance = _WantedDistance;
	
	// Trace the precalculated rays
	bHit = _pWPhysState->Phys_IntersectLine(Selection, Target, Rays[0], OwnFlags, Objects, 
		Mediums, &CInfo);

	if (bHit && CInfo.m_bIsValid)
	{
		// Calculate your own distance since the one in the collision info doesn't seem to work
		fp32 Distance = (CInfo.m_Pos - Target).Length();
		NewDistance = Min(NewDistance, Distance);
		bWallHit = true;
	}

	if (bDebugRender)
	{
		CVec3Dfp32 Direction = Rays[0] - Target;
		Direction.Normalize();

		/*if(bHit && CInfo.m_bIsValid)
			_pWPhysState->Debug_RenderWire(Target, Rays[0], 0xffff0000, DebugDuration);
		else
			_pWPhysState->Debug_RenderWire(Target, Rays[0], 0xff00ff00, DebugDuration);*/
	}

	for (int iTraces = 1; iTraces < iNumRays; iTraces++)
	{
		bHit = _pWPhysState->Phys_IntersectLine(Selection, Target, Rays[iTraces], OwnFlags, 
			Objects, Mediums, &CInfo);

		if (bHit && CInfo.m_bIsValid)
		{
			fp32 Distance = (CInfo.m_Pos - Target).Length();
			NewDistance = Min(Distance, NewDistance);
			bWallHit = true;
		}
		
		if (bDebugRender)
		{
			CVec3Dfp32 Direction = Rays[iTraces] - Target;
			Direction.Normalize();

			/*if(bHit && CInfo.m_bIsValid)
				_pWPhysState->Debug_RenderWire(Target, Rays[iTraces], 0xffff0000, DebugDuration);
			else
				_pWPhysState->Debug_RenderWire(Target, Rays[iTraces], 0xff00ff00, DebugDuration);*/
		}
	}

	if(!_Rotate) 
	{
		_WantedDistance = Max(0.0f, NewDistance);
		return bWallHit;
	}
	
	m_CamVars.m_fTraceHistory = ((m_CamVars.m_fTraceHistory > 0.0f) ? 
		Min(NewDistance, m_CamVars.m_fTraceHistory) : NewDistance);
	if (m_CamVars.m_fTraceHistory > 0.0f && m_CamVars.m_fTraceHistory < 15.0f)
		m_CamVars.m_fTraceHistory = 15.0f;

	m_CamVars.m_iRotationIndex++;
	
	if(CWObject_Character::Char_GetPhysType(_pObj) == PLAYER_PHYS_DEAD)
	{
		_WantedDistance = m_CamVars.m_fTraceHistory;
	}
	else if(m_CamVars.m_iRotationIndex == MAX_ROTATIONS)
	{
		fp32 MaxDistance = _WantedDistance;
		_WantedDistance = Min(Max(0.0f, m_CamVars.m_fTraceHistory), _WantedDistance); 
		m_CamVars.m_fTraceHistory = MaxDistance;//_WantedDistance;
		m_CamVars.m_fOldTraceHistory = _WantedDistance;
		m_CamVars.m_iRotationIndex = 0;
	}
	else if (m_CamVars.m_fOldTraceHistory == 0.0f)
	{
		_WantedDistance = m_CamVars.m_fTraceHistory;
		m_CamVars.m_fOldTraceHistory = m_CamVars.m_fTraceHistory;
	}
	else
	{
		_WantedDistance = m_CamVars.m_fOldTraceHistory;
	}

	//ConOut(CStrF("Distance: %f", _WantedDistance));

	return bWallHit;
}

spCActionCutsceneCamera CActionCutsceneCamera::CreateRuntimeClassObject(const char *_pName, 
														 CWorld_Server *_pWServer)
{
	spCActionCutsceneCamera spObject = 
		(CActionCutsceneCamera*)MRTC_GetObjectManager()->CreateObject(CFStrF("%s", _pName));

	if(spObject!=NULL)
	{
		spObject->m_pWServer = _pWServer;
		spObject->OnCreate();
	}
	return spObject;
}

spCActionCutsceneCamera CActionCutsceneCamera::CreateObject(const char *_pName, CWorld_Server* _pWServer)
{
	spCActionCutsceneCamera spItem = CreateRuntimeClassObject(_pName, _pWServer);
	if(spItem != NULL)
		return spItem;

	//Not a runtime class. Looking for a rpg-template
	spCRegistry spReg = CRPG_Object::GetEvaledRegistry(_pName, _pWServer);
	if(spReg)
	{
		CRegistry *pClass = spReg->FindChild("CLASS");
		if(!pClass)
			return NULL;
		
		spItem = CreateRuntimeClassObject(pClass->GetThisValue(), _pWServer);
		if(spItem!=NULL)
		{
			int nKeys = spReg->GetNumChildren();
			for(int k = 0; k < nKeys; k++)
			{
				const CRegistry* pChild = spReg->GetChild(k);
				spItem->OnEvalKey(pChild->GetThisNameHash(), pChild);
			}
			
			spItem->OnFinishEvalKeys();
			return spItem;
		}
	}

	ConOutL(CStrF("§cf80WARNING: (CActionCutsceneCamera::CreateObject) Could not create item: %s", _pName));
/*	//If runtime-class can't be created a dummy object is created instead.
	pItem = DNew(CRPG_Object) CRPG_Object;
	if(!pItem)
		Error_static("CRPG_Object::CreateObject", "Out of memory");
	pItem->m_Name = _pName;
	pItem->m_pWServer = _pWServer;*/

	return spItem;
}

 /*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Sets the camera valid in the characters client data
						
	Parameters:		
		_pCD:			The characters client data
			
	Comments:		
\*____________________________________________________________________*/
void CActionCutsceneCamera::SetValid(CWO_Character_ClientData* _pCD)
{ 
	m_CamVars.m_CameraMode |= ACS_CAMERAMODE_ACTIVE;
	m_CamVars.SetToPCD(_pCD);
	// Updates the height variables
	m_CamVars.UpdateHeightOffset(_pCD);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Invalidates the camera in the characters client 
					data
						
	Parameters:		
		_pCD:			The characters client data
			
	Comments:		
\*____________________________________________________________________*/
void CActionCutsceneCamera::SetInValid(CWO_Character_ClientData* _pCD)
{ 
	m_CamVars.m_CameraMode &= ~ACS_CAMERAMODE_ACTIVE;
	m_CamVars.SetToPCD(_pCD);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Sets the camera in the pcd to invalid

	Parameters:			
		_pCD:			Characters client data, sets the camera 
						invalid in here
						
\*____________________________________________________________________*/
void CActionCutsceneCamera::SetInvalidToPcd(CWO_Character_ClientData* _pCD)
{
	if (_pCD)
	{
		bool b3PI = ((_pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_MASK) == THIRDPERSONINTERACTIVE_MODE_ACS);
		if (!b3PI)
		{
			// Reset camvars and set to pcd
			CamVars Temp;
			Temp.SetToPCD(_pCD);
			_pCD->m_ActionCutSceneCameraMode = _pCD->m_ActionCutSceneCameraMode & ~ACS_CAMERAMODE_ACTIVE;
		}
	}
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Activates the camera, and marks it as interactive.
					The character client data is updated aswell.
	Parameters:			
		_CD:		Characters client data. 
\*____________________________________________________________________*/
void CActionCutsceneCamera::InteractiveMode_Start(CWO_Character_ClientData& _CD)
{
	m_CamVars.m_CameraMode |= ACS_CAMERAMODE_ACTIVE;
	m_CamVars.m_CameraMode |= ACS_CAMERAMODE_INTERACTIVE;
	m_CamVars.SetToPCD(&_CD);
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Deactivates the camera.
					The character client data is updated aswell.

	Parameters:	
		_CD:		Characters client data. 
\*____________________________________________________________________*/
void CActionCutsceneCamera::InteractiveMode_Stop(CWO_Character_ClientData& _CD)
{
	m_CamVars.m_CameraMode &= ~ACS_CAMERAMODE_ACTIVE;
	m_CamVars.m_CameraMode &= ~ACS_CAMERAMODE_INTERACTIVE;
	m_CamVars.SetToPCD(&_CD);
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CamVars
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Resets the camera variables
						
	Comments:		
\*____________________________________________________________________*/
void CamVars::Reset()
{
	m_iRotationIndex = 0;
	m_iCharacter = 0;
	m_iObject = 0;
	m_iCameraObject = 0;
	m_CameraMode = CActionCutsceneCamera::ACS_CAMERAMODE_UNDEFINED;
	m_fTraceHistory = 0.0f;
	m_fOldTraceHistory = 0.0f;
	m_CharacterHeightOffset = 0.0f;
	m_CharacterHeightOffsetCrouch = 0.0f;
	m_DistanceOffset = 0.0f;
	m_HeightOffset = 0.0f;
	m_CameraAngle = 0.0f;

	m_LastTargetDistance = 0.0f;
	m_TargetDistance = 0.0f;
	m_DistanceChange = 0.0f;

	m_LastTargetRotation = 0.0f;
	m_TargetRotation = 0.0f;
	m_RotationChange = 0.0f;

	// Whatever...
	m_AngleOffsetX = 0.0f;
	m_LastAngleOffsetX = 0.0f;
	m_TargetAngleOffsetX = 0.0f;
	m_AngleOffsetXChange = 0.0f;

	m_AngleOffsetY = 0.0f;
	m_LastAngleOffsetY = 0.0f;
	m_TargetAngleOffsetY = 0.0f;
	m_AngleOffsetYChange = 0.0f;

	m_LastCameraTarget = 0.0f;
	m_CameraTarget = 0.0f;
	m_CameraTargetChange = 0.0f;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Copies the camera variables from the given one

	Parameters:		
		_Vars:		Camera variables to copy
						
	Comments:		
\*____________________________________________________________________*/
void CamVars::CopyFrom(const CamVars& _Vars)
{
	m_iRotationIndex = _Vars.m_iRotationIndex;
	m_iCharacter = _Vars.m_iCharacter;
	m_iObject = _Vars.m_iObject;
	m_iCameraObject = _Vars.m_iCameraObject;
	m_CameraMode = _Vars.m_CameraMode;
	m_fTraceHistory = _Vars.m_fTraceHistory;
	m_fOldTraceHistory = _Vars.m_fOldTraceHistory;
	m_CharacterHeightOffset = _Vars.m_CharacterHeightOffset;
	m_CharacterHeightOffsetCrouch = _Vars.m_CharacterHeightOffset;
	m_DistanceOffset = _Vars.m_DistanceOffset;
	m_HeightOffset = _Vars.m_HeightOffset;
	m_CameraAngle = _Vars.m_CameraAngle;

	m_LastTargetDistance = _Vars.m_LastTargetDistance;
	m_TargetDistance = _Vars.m_TargetDistance;
	m_DistanceChange = _Vars.m_DistanceChange;

	m_LastTargetRotation = _Vars.m_LastTargetRotation;
	m_TargetRotation = _Vars.m_TargetRotation;
	m_RotationChange = _Vars.m_RotationChange;

	m_LastCameraTarget = _Vars.m_LastCameraTarget;
	m_CameraTarget = _Vars.m_CameraTarget;
	m_CameraTargetChange = _Vars.m_CameraTargetChange;
}

 /*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Resets the trace variables
						
	Comments:		
\*____________________________________________________________________*/
void CamVars::ResetTraceHistory()
{
	m_fTraceHistory = 0.0f;
	m_fOldTraceHistory = 0.0f;

	m_LastTargetDistance = 0.0f;
	m_TargetDistance = 0.0f;
	m_DistanceChange = 0.0f;

	m_LastTargetRotation = 0.0f;
	m_TargetRotation = 0.0f;
	m_RotationChange = 0.0f;

	m_LastCameraTarget = 0.0f;
	m_CameraTarget = 0.0f;
	m_CameraTargetChange = 0.0f;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Constructor that resets the data
						
	Comments:		
\*____________________________________________________________________*/
CamVars::CamVars()
{
	Reset();
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Sets the camera configuration to a characters 
					client data
						
	Parameters:		
		_pCD:		Client data to write camera configuration to
			
	Comments:		
\*____________________________________________________________________*/
void CamVars::SetToPCD(CWO_Character_ClientData* _pCD)
{
	_pCD->m_iActionCutSceneCameraObject = m_iCameraObject;
	_pCD->m_iActionCutSceneCameraTarget = m_iObject;
	_pCD->m_ActionCutSceneCameraMode = m_CameraMode;
	_pCD->m_ActionCutSceneCameraWantedDistance = Min(m_fTraceHistory, m_fOldTraceHistory);
	_pCD->m_ActionCutSceneCameraDistanceOffset = (uint8)m_DistanceOffset;
	_pCD->m_ActionCutSceneCameraHeightOffset = (uint8)m_HeightOffset;
	_pCD->m_ActionCutSceneCameraAngle = m_CameraAngle;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Updates the mod variables to the client data
						
	Parameters:		
		_pCD:			The client data to write the variables in
			
	Comments:		
\*____________________________________________________________________*/
void CamVars::UpdateModVars(CWO_Character_ClientData* _pCD)
{
	// save space -> use existing variables not currently in use
	_pCD->m_ACSLastTargetDistance = m_LastTargetDistance;
	_pCD->m_ACSTargetDistance = m_TargetDistance;
	_pCD->m_ACSDistanceChange = m_DistanceChange;

	_pCD->m_ACSLastTargetRotation = m_LastTargetRotation;
	_pCD->m_ACSTargetRotation = m_TargetRotation;
	_pCD->m_ACSRotationChange = m_RotationChange;

	_pCD->m_ACSLastAngleOffsetX = m_LastAngleOffsetX;
	_pCD->m_ACSTargetAngleOffsetX = m_TargetAngleOffsetX;
	_pCD->m_ACSAngleOffsetXChange = m_AngleOffsetXChange;

	_pCD->m_ACSLastAngleOffsetY = m_LastAngleOffsetY;
	_pCD->m_ACSTargetAngleOffsetY = m_TargetAngleOffsetY;
	_pCD->m_ACSAngleOffsetYChange = m_AngleOffsetYChange;

	_pCD->m_ACSLastCameraTarget = m_LastCameraTarget;
	_pCD->m_ACSCameraTarget = m_CameraTarget;
	_pCD->m_ACSCameraTargetChange = m_CameraTargetChange;
	
	/*_pCD->m_ActionCutsceneCameraLastTargetDistance = m_LastTargetDistance;
	_pCD->m_ActionCutsceneCameraTargetDistance = m_TargetDistance;
	_pCD->m_ActionCutsceneCameraDistanceChange = m_DistanceChange;

	_pCD->m_ActionCutsceneCameraLastTargetRotation = m_LastTargetRotation;
	_pCD->m_ActionCutsceneCameraTargetRotation = m_TargetRotation;
	_pCD->m_ActionCutsceneCameraRotationChange = m_RotationChange;*/

	// Update angle offset crapor (try to find some more unused variables...)
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Configures the camera from a characters client data
						
	Parameters:		
		_pCD:			Client data to import from
		_iCharacter:	Character object id
			
	Comments:		
\*____________________________________________________________________*/
void CamVars::ImportFromPCD(CWO_Character_ClientData* _pCD, int _iCharacter)
{
	Reset();
	m_iCharacter = _iCharacter;
	m_iCameraObject = _pCD->m_iActionCutSceneCameraObject;
	m_iObject = _pCD->m_iActionCutSceneCameraTarget;
	m_CameraMode = _pCD->m_ActionCutSceneCameraMode;
	// Set the character heights to be twice the physical height
	m_CharacterHeightOffset = 2 * (fp32)_pCD->m_Phys_Height;
	m_CharacterHeightOffsetCrouch = 2 * (fp32)_pCD->m_Phys_HeightCrouch;
	m_fTraceHistory = _pCD->m_ActionCutSceneCameraWantedDistance;
	m_DistanceOffset = (fp32)_pCD->m_ActionCutSceneCameraDistanceOffset;
	m_HeightOffset = (fp32)_pCD->m_ActionCutSceneCameraHeightOffset;
	m_CameraAngle = _pCD->m_ActionCutSceneCameraAngle;
	m_AngleOffsetX = _pCD->m_ActionCutSceneCameraOffsetX;
	m_AngleOffsetY = _pCD->m_ActionCutSceneCameraOffsetY;

	
	// Import the mod variables, save space -> use existing variables not currently in use
	m_LastTargetDistance = _pCD->m_ACSLastTargetDistance;
	m_TargetDistance = _pCD->m_ACSTargetDistance;
	m_DistanceChange = _pCD->m_ACSDistanceChange;

	m_LastTargetRotation = _pCD->m_ACSLastTargetRotation;
	m_TargetRotation = _pCD->m_ACSTargetRotation;
	m_RotationChange = _pCD->m_ACSRotationChange;

	m_LastAngleOffsetX = _pCD->m_ACSLastAngleOffsetX;
	m_TargetAngleOffsetX = _pCD->m_ACSTargetAngleOffsetX;
	m_AngleOffsetXChange = _pCD->m_ACSAngleOffsetXChange;

	m_LastAngleOffsetY = _pCD->m_ACSLastAngleOffsetY;
	m_TargetAngleOffsetY = _pCD->m_ACSTargetAngleOffsetY;
	m_AngleOffsetYChange = _pCD->m_ACSAngleOffsetYChange;

	m_LastCameraTarget = _pCD->m_ACSLastCameraTarget;
	m_CameraTarget = _pCD->m_ACSCameraTarget;
	m_CameraTargetChange = _pCD->m_ACSCameraTargetChange;
	/*m_LastTargetDistance = _pCD->m_ActionCutsceneCameraLastTargetDistance;
	m_TargetDistance = _pCD->m_ActionCutsceneCameraTargetDistance;
	m_DistanceChange = _pCD->m_ActionCutsceneCameraDistanceChange;

	m_LastTargetRotation = _pCD->m_ActionCutsceneCameraLastTargetRotation;
	m_TargetRotation = _pCD->m_ActionCutsceneCameraTargetRotation;
	m_RotationChange = _pCD->m_ActionCutsceneCameraRotationChange;*/
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Extracts the height variables from the character
					data and sets them to the client data
						
	Parameters:		
		_pCD:			Client data to import from
			
	Comments:		
\*____________________________________________________________________*/
void CamVars::UpdateHeightOffset(CWO_Character_ClientData* _pCD)
{
	if (_pCD)
	{
		// Set the height values to the cam variables
		m_CharacterHeightOffset = 2 * (fp32)_pCD->m_Phys_Height;
		m_CharacterHeightOffsetCrouch = 2 * (fp32)_pCD->m_Phys_HeightCrouch;
	}
}
