#pragma once

#include <string>
#include <unordered_map>
#include <algorithm>

#include "addrs.h"
#include "defs.h"

extern int const (*LUA_DoString)(const char*, const char*, const char*);
extern int const (*SelectUnit)(GUID_t);

void DoString(const char* format, ...); 

GUID_t get_raid_target_GUID(int index);
GUID_t get_raid_target_GUID(const std::string &marker_name);

GUID_t get_target_GUID();

enum {
	OBJECT_TYPE_OBJECT = 0,
	OBJECT_TYPE_ITEM = 1,
	OBJECT_TYPE_CONTAINER = 2,
	OBJECT_TYPE_NPC = 3,
	OBJECT_TYPE_UNIT = 4,
	OBJECT_TYPE_GAMEOBJECT = 5,
	OBJECT_TYPE_DYNAMICOBJECT = 6,
	OBJECT_TYPE_CORPSE = 7,
	OBJECT_TYPE_AREATRIGGER = 8,
	OBJECT_TYPE_SCENEOBJECT = 9
};


class WowObject {
private:
	// these are offsets from the base addr of the object

	const unsigned int
		GUID = 0x30,
		Next = 0x3C,
		Type = 0x14,
		X = 0xBF0,
		Y = X + 4,
		Z = X + 8,
		R = X + 12,
		unit_info_field = 0x120,
		Name = 0xDB8,

		UnitHealth = 0x40,
		UnitHealthMax = 0x58,

		NPCHealth = 0x11E8,
		NPCMana = 0x11EC,
		NPCRage = 0x11F0,
		NPCEnergy = 0x11F4,
		NPCFocus = 0x11F8,

		NPCHealthMax = 0x1200,
		NPCManaMax = 0x1204,
		NPCRageMax = 0x1208,
		NPCEnergyMax = 0x120C,
		NPCFocusMax = 0x1210,

		UnitTargetGUID = 0x2680;

	unsigned int base;

	float get_pos_x() const;
	float get_pos_y() const;
	float get_pos_z() const;

public:
	bool valid() const;
	WowObject getNextObject() const;

	vec3 get_pos() const;
	float get_rot() const;

	GUID_t get_GUID() const;

	int get_type() const;

	int NPC_get_health() const;
	int NPC_get_health_max() const;

	int NPC_get_mana() const;
	int NPC_get_mana_max() const;

	int NPC_get_rage() const;
	int NPC_get_rage_max() const;

	int NPC_get_energy() const;
	int NPC_get_energy_max() const;

	int NPC_get_focus() const;
	int NPC_get_focus_max() const;

	std::string NPC_get_name() const;
	GUID_t NPC_get_target_GUID() const;

	uint NPC_get_buff(int index) const;
	uint NPC_get_debuff(int index) const;

	uint NPC_get_buff_duration(int index, uint spellID) const;
	uint NPC_get_debuff_duration(int index, uint spellID) const;

	std::string unit_get_name() const;
	GUID_t unit_get_target_GUID() const;

	int in_combat() const;

	uint unit_get_health() const;
	uint unit_get_health_max() const;

	uint unit_get_buff(int index) const;
	uint unit_get_debuff(int index) const;

	std::string get_type_name() const;

	int DO_get_spellID() const;
	vec3 DO_get_pos() const;

	WowObject(unsigned int addr);
	WowObject();

	unsigned int get_base() const;

	const WowObject& operator=(const WowObject &o);
};


class ObjectManager {

private:
	const unsigned int clientConnection_addr_static = 0x00D43318,
		clientConnection = 0,
		objectManagerOffset = 0x2218, // 0x46E134 has this [edx + 0x2218] thing in it
		localGUIDOffset = 0xC0,                                             // offset from the object manager to the local guid

		firstObjectOffset = 0xAC,                                          // offset from the object manager to the first object
		nextObjectOffset = 0x3C;                                       // offset from one object to the next

	void LoadAddresses();

	unsigned int base_addr;                       // the base address of the object manager
	GUID_t localGUID;

	int invalid;

public:

	int valid() const;

	WowObject get_first_object() const;

	ObjectManager();

	WowObject get_object_by_GUID(GUID_t GUID) const;
	GUID_t get_local_GUID() const;
	WowObject get_local_object() const;
	uint get_base_address() const { return base_addr; }

	std::vector<WowObject> get_spell_objects_with_spellID(long spellID);

};

struct WO_cached {
	GUID_t GUID;
	vec3 pos;
	uint health;
	uint health_max;
	int deficit;
	std::string name;

	WO_cached(GUID_t guid, vec3 p, uint hp, uint hp_max, const std::string &n) 
		: GUID(guid), pos(p), health(hp), health_max(hp_max), name(n) {
		deficit = hp_max - hp;
	}

	WO_cached() {
		memset(this, 0, sizeof(*this));
	}
};