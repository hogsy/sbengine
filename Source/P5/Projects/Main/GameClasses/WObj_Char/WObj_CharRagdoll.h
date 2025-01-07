/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
    File:				Character ragdoll functionality
					
    Author:				Anders Ekermo
					
    Copyright:			Copyright Starbreeze Studios AB 2006
					
    Contents:			CWPhys_RagdollClientData
						Misc. Ragdoll functions
					
    Comments:			I created this file for links between
						CWPhys_Cluster and the character ragdoll,
						put as much of the ragdoll code as possible 
						in here - anek 
					
    History:
        061106: anek, created file
\*____________________________________________________________________*/

#ifndef __WObj_CharRagdoll_h__
#define __WObj_CharRagdoll_h__


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
    Class:      Ragdoll Client MetaData

    Comments:   To avoid having to send data for 90+ bones when
				the ragdoll only supports ~12 different values anyway
\*____________________________________________________________________*/
class CWPhys_RagdollClientMetaData : public CReferenceCount
{
public:

	CMat4Dfp32		m_OrgMat;
	TArray<uint8>	m_liBoneId;

	void CopyFrom(const CWPhys_RagdollClientMetaData &_From);

	void Pack(uint8 *&_pD, class CMapData* _pMapData) const;

	void Unpack(const uint8 *&_pD, class CMapData* _pMapData);
};



/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
    Class:     Ragdoll ClientData		
\*____________________________________________________________________*/
class CWPhys_RagdollClientData : public CReferenceCount
{
public:
	uint16				m_nTracks;
	uint16				m_State;
	TArray<CQuatfp32>	m_lRot;
	TArray<CVec3Dfp32>	m_lMove;

	CWPhys_RagdollClientData() : m_State(0) { };

	void Init(int _nTracks);

	void CopyFrom(const CWPhys_RagdollClientData &_From);

	void Pack(uint8 *&_pD, class CMapData* _pMapData) const;

	void Unpack(const uint8 *&_pD, class CMapData* _pMapData);

	CVec3Dfp32 EvalAnim(CXR_Skeleton * _pSkel,CXR_SkeletonInstance * _pSkelInst,const CWPhys_RagdollClientMetaData *_pMD) const;
};


#define RAGDOLL_FORCE_MULTIPLIER	24.0f

#endif
