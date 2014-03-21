#include "KX_ResourceManager.h"


void KX_ResourceManager::RegisterGameObjectName(const STR_String& gameobjname, CValue* gameobj)
{
	STR_HashedString mn = gameobjname;
	m_mapStringToGameObjects.insert(mn, gameobj);
}



void KX_ResourceManager::RegisterGameMeshName(const STR_String& gamemeshname, void* blendobj)
{
	STR_HashedString mn = gamemeshname;
	m_map_gamemeshname_to_blendobj.insert(mn, blendobj);
}



void KX_ResourceManager::RegisterGameObj(void* blendobj, CValue* gameobj)
{
	m_map_blendobj_to_gameobj.insert(CHashedPtr(blendobj), gameobj);
}

void KX_ResourceManager::UnregisterGameObj(void* blendobj, CValue* gameobj)
{
	void **obp = m_map_blendobj_to_gameobj[CHashedPtr(blendobj)];
	if (obp && (CValue*)(*obp) == gameobj)
		m_map_blendobj_to_gameobj.remove(CHashedPtr(blendobj));
}

CValue* KX_ResourceManager::GetGameObjectByName(const STR_String& gameobjname)
{
	STR_HashedString mn = gameobjname;

	int total = m_mapStringToGameObjects.size();

	for (int i = 0; i < total; i++)
	{
		CValue** gam = m_mapStringToGameObjects.at(i);
	}

	CValue** gameptr = m_mapStringToGameObjects[mn];

	if (gameptr)
		return *gameptr;

	return NULL;
}


CValue* KX_ResourceManager::FindGameObjByBlendObj(void* blendobj)
{
	void **obp = m_map_blendobj_to_gameobj[CHashedPtr(blendobj)];
	return obp ? (CValue*)(*obp) : NULL;
}



void* KX_ResourceManager::FindBlendObjByGameMeshName(const STR_String& gamemeshname)
{
	STR_HashedString mn = gamemeshname;
	void **obp = m_map_gamemeshname_to_blendobj[mn];
	return obp ? *obp : NULL;
}


void *KX_ResourceManager::GetActionByName(const STR_String& actname)
{
	STR_HashedString an = actname;
	void** actptr = m_mapStringToActions[an];

	if (actptr)
		return *actptr;

	return NULL;
}

void* KX_ResourceManager::GetMeshByName(const STR_String& meshname)
{
	STR_HashedString mn = meshname;
	void** meshptr = m_mapStringToMeshes[mn];

	if (meshptr)
		return *meshptr;

	return NULL;
}

void KX_ResourceManager::RegisterMeshName(const STR_String& meshname, void* mesh)
{
	STR_HashedString mn = meshname;
	m_mapStringToMeshes.insert(mn, mesh);
}

void KX_ResourceManager::UnregisterMeshName(const STR_String& meshname, void* mesh)
{
	STR_HashedString mn = meshname;
	m_mapStringToMeshes.remove(mn);
}


void KX_ResourceManager::RegisterActionName(const STR_String& actname, void* action)
{
	STR_HashedString an = actname;
	m_mapStringToActions.insert(an, action);
}

void KX_ResourceManager::UnregisterActionName(const STR_String& actname, void* mesh)
{
	STR_HashedString an = actname;
	m_mapStringToActions.remove(an);
}

