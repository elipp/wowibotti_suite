#pragma once

#include <string>

#include "defs.h"

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
		Field = 0x120,
		Name = 0xDB8,
		NPCcurrentHealth = 0x11E8,
		NPCcurrentMana = 0x11EC,
		NPCcurrentRage = 0x11F0,
		NPCcurrentEnergy = 0x11F4,
		NPCcurrentFocus = 0x11F8,

		NPCmaxHealth = 0x1200,
		NPCmaxMana = 0x1204,
		NPCmaxRage = 0x1208,
		NPCmaxEnergy = 0x120C,
		NPCmaxFocus = 0x1210,

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

	int NPC_getCurHealth() const;
	int NPC_getMaxHealth() const;

	int NPC_getCurMana() const;
	int NPC_getMaxMana() const;

	int NPC_getCurRage() const;
	int NPC_getMaxRage() const;

	int NPC_getCurEnergy() const;
	int NPC_getMaxEnergy() const;

	int NPC_getCurFocus() const;
	int NPC_getMaxFocus() const;

	std::string NPC_get_name() const;
	std::string unit_get_name() const;

	GUID_t unit_get_target_GUID() const;

	std::string get_type_name() const;

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

public:

	int valid() const;

	WowObject getFirstObject() const;

	ObjectManager();

	WowObject get_object_by_GUID(GUID_t GUID) const;
	GUID_t get_localGUID() const;

};