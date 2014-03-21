#include "KX_HashedPtr.h"
#include "CTR_Map.h"
#include "STR_HashedString.h"
#include "Value.h"

class KX_ResourceManager
{
	CTR_Map<STR_HashedString, CValue*>	m_mapStringToGameObjects;
	CTR_Map<STR_HashedString, void*>	m_mapStringToMeshes;
	CTR_Map<STR_HashedString, void*>	m_mapStringToActions;

	CTR_Map<STR_HashedString, void*>	m_map_gamemeshname_to_blendobj;
	CTR_Map<CHashedPtr, void*>			m_map_blendobj_to_gameobj;

public:
	void *GetActionByName(const STR_String& meshname);
	void *GetMeshByName(const STR_String& meshname);
	void RegisterMeshName(const STR_String& meshname, void* mesh);
	void UnregisterMeshName(const STR_String& meshname, void* mesh);
	void RegisterActionName(const STR_String& actname, void* action);
	void UnregisterActionName(const STR_String& actname, void* action);

	void			RegisterGameObjectName(const STR_String& gameobjname, CValue* gameobj);
	class CValue*	GetGameObjectByName(const STR_String& gameobjname);

	void	RegisterGameMeshName(const STR_String& gamemeshname, void* blendobj);
	void*	FindBlendObjByGameMeshName(const STR_String& gamemeshname);

	void	RegisterGameObj(void* blendobj, CValue* gameobj);
	void	UnregisterGameObj(void* blendobj, CValue* gameobj);
	CValue*	FindGameObjByBlendObj(void* blendobj);

	void	RemoveGameObject(const STR_String& gameobjname);

	// for the scripting... needs a FactoryManager later (if we would have time... ;)
	CTR_Map<STR_HashedString, void*>&	GetMeshMap() { return m_mapStringToMeshes; };
	CTR_Map<STR_HashedString, void*>&	GetActionMap() { return m_mapStringToActions; };
};