#ifndef _INC_WOBJ_AREAINFO
#define _INC_WOBJ_AREAINFO

#include "../AI_Def.h"

//Area info object. Generates information about the area which it encompasses
class CWObject_AreaInfo : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;

protected:
	int32 m_iBSPModel;

	//Gets this objects corresponding physics model, or NULL if physmodel is lacking
	CXR_PhysicsModel * GetPhysModel();
	
public:
	//Messages
	enum {
		//Param1 and the vecparams are the parameters for the getareaposition method, see below
		//Param0 should be a CVec3Dfp32 * were the result is stored. 
		OBJMSG_GET_POSITION = OBJMSGBASE_AREAINFO, 
		
		//Param0 should be a bool * which is set to true if VecParam0 is in area
		OBJMSG_POSITION_IN_AREA, 

		//Param0 should be a bool * which is set to true if the box spanned by VecParam0 and 
		//Vecparam1 intersects area.
		OBJMSG_BOX_IN_AREA, 

		//Param0 should be a bool * which is set to true if the objects (indexed by param1) 
		//bounding box intersects the area 
		OBJMSG_OBJECT_IN_AREA,
		
		//Debug render area stuff. Param0 is colour pointer (CPixel32*), param1 time pointer (fp32*)
		OBJMSG_DEBUG_RENDER,
	};

	//Constructor
	CWObject_AreaInfo();
	
	//Get object "parameters" from keys
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	//Set up object 
	virtual void OnFinishEvalKeys();

	//Object message interface
	virtual aint OnMessage(const CWObject_Message& _Msg);

	//Modes for the GetAreaPosition method
	enum {
		MODE_RANDOM,
		MODE_EXACT,
	};

	//Return position within the area info object, depending on mode etc, or CVec3Dfp32(_FP32_MAX)
	//if fail to get position
	virtual CVec3Dfp32 GetAreaPosition(int _iMode, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir);

	//Succeeds iff position is inside the area
	virtual bool IsInArea(const CVec3Dfp32& _Pos);

	//Succeeds iff object's bounding box intersects area
	virtual bool IsInArea(CWObject * pObj);

	//Succeeds iff box spanned by the two positions intersects area
	virtual bool IsInArea(const CVec3Dfp32& _Min, const CVec3Dfp32& _Max);

	//Debug method. Render wire shape, using debug render methods. 
	void DebugRender(CPixel32 _Colour, fp32 _Time, CPixel32 _BoundBoxColour = 0);
};




#endif
