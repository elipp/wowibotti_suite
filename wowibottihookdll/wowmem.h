#pragma once

#include <string>
#include <unordered_map>
#include <algorithm>
#include <mutex>

#include "addrs.h"
#include "defs.h"

extern int const (*LUA_DoString)(const char*, const char*, const char*);
extern int const (*SelectUnit)(GUID_t);

void DoString(const std::string format, ...); // because must not have reference type
void echo_wow(const char* format, ...);
void dual_echo(const char* format, ...); // echo to both console and wow chat

void set_taint_caller_zero();
void reset_taint_caller();

BYTE get_spellcast_counter();
void increment_spellcast_counter();

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

typedef struct type_string_mapentry {
	std::string typestring;
	int index;
} type_string_mapentry;

int get_type_index_from_string(const std::string &type);

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
		UnitPosX = 0x798,
		UnitPosY = UnitPosX + 0x4,
		UnitPosZ = UnitPosX + 0x8,
		UnitRot = 0x7A8,


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
	WowObject next() const;

	vec3 get_pos() const; // works for units and NPCs
	float get_rot() const;
	vec3 get_rotvec() const;

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

	int NPC_unit_is_dead() const;

	std::string NPC_get_name() const;
	GUID_t NPC_get_target_GUID() const;

	uint NPC_get_buff(int index) const;
	uint NPC_get_debuff(int index) const;

	uint NPC_get_buff_duration(int index, uint spellID) const;
	uint NPC_get_debuff_duration(int index, uint spellID) const;

	int NPC_has_buff(uint spellID) const;
	int NPC_has_debuff(uint spellID) const;

	GUID_t NPC_get_mounted_GUID() const;

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

	std::string GO_get_name() const;
	vec3 GO_get_pos() const;

	uint item_get_ID() const;

	WowObject(unsigned int addr);
	WowObject();

	unsigned int get_base() const;

	const WowObject& operator=(const WowObject &o);
};


struct WO_cached {
	GUID_t GUID;
	vec3 pos;
	float rot;
	int type;
	uint health;
	uint health_max;
	int deficit;
	uint inc_heals;
	float heal_urgency;
	std::string name;
	unsigned DO_spellid;
	int reaction;
	int in_combat;
	int dead;

	WO_cached(GUID_t guid, vec3 p, uint hp, uint hp_max, uint ih, float hu, const std::string& n)
		: GUID(guid), pos(p), health(hp), health_max(hp_max), inc_heals(ih), heal_urgency(hu), name(n) {
		deficit = hp_max - hp;
	}

	WO_cached(const WowObject& o, int react = -1) {
		GUID = o.get_GUID();
		type = o.get_type();

		if (type == OBJECT_TYPE_NPC) {
			pos = o.get_pos();
			health = o.NPC_get_health();
			health_max = o.NPC_get_health_max();
			name = o.NPC_get_name();
			in_combat = o.in_combat();
			dead = o.NPC_unit_is_dead();
			rot = o.get_rot();
			reaction = react;
		}
		else if (type == OBJECT_TYPE_UNIT) {
			pos = o.get_pos();
			health = o.unit_get_health();
			health_max = o.unit_get_health_max();
			rot = o.get_rot();
			name = o.unit_get_name();
		}
		else if (type == OBJECT_TYPE_DYNAMICOBJECT) {
			pos = o.DO_get_pos();
			DO_spellid = o.DO_get_spellID();
		}
	}

	WO_cached() {
		memset(this, 0, sizeof(*this));
	}
};

class hcache_t {

public:
	static std::mutex mutex;
	WO_cached player;
	GUID_t target_GUID;
	GUID_t focus_GUID;
	std::vector<WO_cached> objects;
	const WO_cached *find(const std::string& name) const {
		hcache_t::mutex.lock();
		for (const auto& o : this->objects) {
			if (name == o.name) {
				hcache_t::mutex.unlock();
				return &o;
			}
		}
		hcache_t::mutex.unlock();
		return NULL;
	}

	const WO_cached* find(GUID_t g) const {
		hcache_t::mutex.lock();
		for (const auto& o : this->objects) {
			if (g == o.GUID) {
				hcache_t::mutex.unlock();
				return &o;
			}
		}
		hcache_t::mutex.unlock();
		return NULL;
	}

	void push(const WO_cached& o) {
		objects.push_back(o);
	}
};

class ObjectManager {

	class iterator {
	public:
		iterator(DWORD base_addr);
		iterator operator++();
		bool operator!=(const iterator& other) const;
		WowObject operator*() const;
	private:
		DWORD base;
	};


private:
	const unsigned int clientConnection_addr_static = 0xC79CE0,
		clientConnection = 0,
		objectManagerOffset = 0x2ED0, 
		localGUIDOffset = 0xC0,                                             // offset from the object manager to the local guid

		firstObjectOffset = 0xAC,                                          // offset from the object manager to the first object
		nextObjectOffset = 0x3C;                                       // offset from one object to the next

	void LoadAddresses();

	unsigned int base_addr;                       // the base address of the object manager
	GUID_t localGUID;

	int invalid;
	long long construction_frame_num;

	static ObjectManager* thisframe_objectmanager;

public:

	int valid() const;

	int get_first_object(WowObject *o) const;

	ObjectManager();

	int get_object_by_GUID(GUID_t GUID, WowObject *o) const;
	int get_unit_by_name(const std::string &name, WowObject *o) const;
	int get_GO_by_name(const std::string &name, WowObject *o) const;
	int get_item_by_itemID(uint itemID, WowObject *o) const;

	GUID_t get_local_GUID() const;
	int get_local_object(WowObject *o) const;
	uint get_base_address() const { return base_addr; }
	std::vector<WowObject> get_all_NPCs_at(const vec3 &pos, float radius) const;
	std::vector<WowObject> get_all_units() const;
	std::vector<WowObject> get_all_combat_mobs() const;
	std::vector<WowObject> get_spell_objects_with_spellID(long spellID);
	std::vector<WowObject> get_NPCs_by_name(const std::string &name);

	static ObjectManager* get();
	hcache_t get_snapshot() const; // get snapshot of current units, NPCs and DOs (for GLAUX)

	ObjectManager::iterator begin() const;
	ObjectManager::iterator end() const;

};

#define OMgr (ObjectManager::get())

DWORD get_wow_d3ddevice();
DWORD get_BeginScene();
DWORD get_EndScene();
DWORD get_Present();
DWORD get_DrawIndexedPrimitive();
IDirect3DDevice9 *get_wow_ID3D9();

GUID_t get_focus_GUID();

float get_distance3(const WowObject &a, const WowObject &b);
float get_distance2(const WowObject &a, const WowObject &b);

int get_reaction(const WowObject &A, const WowObject &B);

BYTE get_item_usecount();
void increment_item_usecount();

typedef struct ERRMSG_t {
	int code;
	std::string text;
} ERRMSG_t;

typedef struct last_spellerror {
	const ERRMSG_t *msg;
	LONG timestamp;
	int err_id;
} last_spellerror;

extern last_spellerror last_errmsg;

struct taint_caller_reseter {
	DWORD current_taint_caller;
	static const DWORD taint_caller_addr_wow;

	taint_caller_reseter();
	~taint_caller_reseter();
};
