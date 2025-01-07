/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WListener.h

	Author:			Anton Ragnarsson

	Copyright:		Copyright Starbreeze AB 2006

	Contents:		CWO_LinkContext

	History:		
		060601:		Created File
\*____________________________________________________________________________________________*/
#ifndef __WListener_h__
#define __WListener_h__


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			Generic class for keeping track of relations
					between objects.

	Comments:		This container keeps track of relations between
					objects, and lets you enumerate all objects
					related to a given object.

					Example of relations can be child/parent where
					a child can have multiple parents, an object 
					subscribing to events from another object, an object 
					linked to one or more portal leaves and a portal leave 
					linked to one or more objects.

					There is no speed difference between enumerating
					in different directions, i.e. parent -> children or
					child -> parents.

	Todo:			Find a good name for this class...
\*____________________________________________________________________*/
class CWO_LinkContext
{
public:
	// element in linked list
	struct CLink
	{
		uint16 m_ID;
		uint16 m_iNext;
		uint16 m_iPrev;
	};

	// element in two linked lists simultaneous
	struct CDualLink
	{
		CLink m_Link[2];
		uint16 m_UserMask;
	};

	CIDHeap m_LinkHeap;
	TThinArray<CDualLink> m_lLinks;
	TThinArray<uint16> m_liHash[2];			// ID -> link

	TAP_RCD<CDualLink> m_pLinks;
	TAP_RCD<uint16> m_piHash[2];

protected:
	uint InsertNewLink(uint _ID1, uint _ID2, uint16 _UserMask = ~0);
	uint FindLink(uint _ID1, uint _ID2) const;
	void RemoveLink(uint _iLink);

public:
	void Create(uint _nMaxIDs, uint _nMaxLinks);
	void Destroy();

	void AddOrUpdateLink(uint _ID1, uint _ID2, uint16 _UserMask = ~0);
	void RemoveOrUpdateLink(uint _ID1, uint _ID2, uint16 _UserMaskToRemove = ~0);
	void RemoveElement(uint _ID);
	uint EnumLinks(uint _ID, uint _iDir, uint* _piRet, uint _nMaxRet, uint16 _Mask = ~0) const;
	uint EnumLinks(uint _ID, uint _iDir, uint16* _piRet, uint _nMaxRet, uint16 _Mask = ~0) const;
};




#endif // __WListener_h__
