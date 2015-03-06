#ifndef __MODINFOMANAGER_H__
#define __MODINFOMANAGER_H__

#include "ModInfo.h"

// a small utility to list available Mods and SP levels.
class CModInfoManager
{
public:
	CModInfoManager();

	void Refresh(); 
	void Free();

	size_t GetModCount() const;
	const ModInfo* GetModByIndex(size_t index) const;
	const ModInfo * FindModByDisplayName(const char *pName) const;

	// lists levels for active Mod
	size_t GetLevelCount() const;
	const ModInfo* GetLevelByIndex(size_t index) const;

	// returns NULL when no Mod is active
	const ModInfo* GetActiveMod() const; 
	const char* GetActiveModName() const;
	const char* GetActiveModDisplayName() const;
	ILINE bool IsRunningMod() const { return m_bRunningMod; }
	ILINE uint32 GetModCheckSum() const { return m_modCheckSum; }

	bool LoadMod(const char* modName);
	bool UnloadMod();

	static uint32 GetDirectoryHash( const char *pPath );

private:
	typedef std::vector<ModInfo> Mods;
	Mods m_mods;
	Mods m_modLevels;
	uint32 m_modCheckSum;
	bool m_bRunningMod;
};

#endif
