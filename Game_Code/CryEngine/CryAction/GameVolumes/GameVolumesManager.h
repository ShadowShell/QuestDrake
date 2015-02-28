// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.


#ifndef _GAME_VOLUMES_MANAGER_H_
#define _GAME_VOLUMES_MANAGER_H_

#pragma once

#include <IGameVolumes.h>

class CGameVolumesManager : public IGameVolumes, IGameVolumesEdit
{

private:

	typedef std::vector<Vec3> Vertices;
	struct EntityVolume
	{
		EntityVolume()
			: entityId(0)
			, height(1.0f)
		{

		}

		bool operator == (const EntityId& id) const
		{
			return entityId == id;
		}

		EntityId	entityId;
		f32				height;
		Vertices	vertices;
	};

	typedef std::vector<EntityVolume>	TEntityVolumes;
	typedef std::vector<IEntityClass*> TVolumeClasses;

public:
	CGameVolumesManager();
	virtual ~CGameVolumesManager();

	// IGameVolumes
	virtual IGameVolumesEdit* GetEditorInterface();
	virtual bool GetVolumeInfoForEntity(EntityId entityId, VolumeInfo* pOutInfo) const;
	virtual void Load( const char* fileName );
	virtual void Reset();
	// ~IGameVolumes


	// IGameVolumesEdit
	virtual void SetVolume(EntityId entityId, const IGameVolumes::VolumeInfo& volumeInfo);
	virtual void DestroyVolume(EntityId entityId);

	virtual void RegisterEntityClass( const char* className );
	virtual size_t GetVolumeClassesCount() const;
	virtual const char* GetVolumeClass(size_t index) const;

	virtual void Export( const char* fileName ) const;
	// ~IGameVolumesEdit

private:

	TEntityVolumes m_volumesData;		// Level memory
	TVolumeClasses m_classes;				// Global memory, initialized at start-up

	const static uint32 GAME_VOLUMES_FILE_VERSION = 1;
};

#endif