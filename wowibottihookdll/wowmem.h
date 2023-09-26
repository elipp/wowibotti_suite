#pragma once

#include <string>
#include <tuple>
#include <unordered_map>
#include <algorithm>
#include <mutex>

#include "addrs.h"
#include "defs.h"

extern int const (*LUA_DoString)(const char*, const char*, const char*);
extern int const (*SelectUnit)(GUID_t);

enum class ECHO : int {
	NONE = 0,
	STDOUT = 1,
	WOW = 2,
	BOTH = 3
};

 
void DoString(const std::string format, ...); // because va_start can't start from reference type
void echo(ECHO TO, const char* msg, ...);

#define ECHO_WOW(msg, ...) echo(ECHO::WOW, msg, __VA_ARGS__)
#define ECHO_BOTH(msg, ...) echo(ECHO::BOTH, msg, __VA_ARGS__)

BYTE get_spellcast_counter();
void increment_spellcast_counter();

GUID_t get_raid_target_GUID(int index);
GUID_t get_raid_target_GUID(const std::string &marker_name);

GUID_t get_target_GUID();


namespace WOWOBJECT_TYPE {
	enum {
		OBJECT = 0,
		ITEM = 1,
		CONTAINER = 2,
		NPC = 3,
		UNIT = 4,
		GAMEOBJECT = 5,
		DYNAMICOBJECT = 6,
		CORPSE = 7,
		AREATRIGGER = 8,
		SCENEOBJECT = 9
	};
}

typedef struct type_string_mapentry {
	std::string typestring;
	int index;
} type_string_mapentry;

int get_type_index_from_string(const std::string &type);

constexpr vec3 unitvec_from_rot(float rot) {
	return vec3(std::cos(rot), std::sin(rot), 0);
}


class WowObject {
private:
	// these are offsets from the base addr of the object
	unsigned int base;

	float get_pos_x() const;
	float get_pos_y() const;
	float get_pos_z() const;

public:
	DWORD get_base() const;
	bool valid() const;
	int get_type() const;
	const char* get_type_name() const;
	const char* get_name() const;

	std::optional<WowObject> next() const;
	
	vec3 get_pos() const; // works for units and NPCs
	float get_rot() const;

	std::tuple<float, float, float, float> get_xyzr() const;

	vec3 get_rotvec() const;

	GUID_t get_GUID() const;

	int NPC_unit_is_dead() const;
	GUID_t NPC_get_target_GUID() const;

	uint NPC_get_buff(int index) const;
	uint NPC_get_debuff(int index) const;

	uint NPC_get_buff_duration(int index, uint spellID) const;
	uint NPC_get_debuff_duration(int index, uint spellID) const;

	int NPC_has_buff(uint spellID) const;
	int NPC_has_debuff(uint spellID) const;

	GUID_t NPC_get_mounted_GUID() const;

	GUID_t get_target_GUID() const;

	int in_combat() const;

	uint get_health() const;
	uint get_health_max() const;

	uint get_mana() const;
	uint get_mana_max() const;

	uint get_rage() const;
	uint get_rage_max() const;

	uint get_focus() const;
	uint get_focus_max() const;

	uint unit_get_buff(int index) const;
	uint unit_get_debuff(int index) const;


	int DO_get_spellID() const;
	vec3 DO_get_pos() const;

	std::string GO_get_name() const;
	vec3 GO_get_pos() const;

	uint item_get_ID() const;

	WowObject(DWORD base_addr);

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

		if (type == WOWOBJECT_TYPE::NPC) {
			pos = o.get_pos();
			health = o.get_health();
			health_max = o.get_health_max();
			name = o.get_name();
			in_combat = o.in_combat();
			dead = o.NPC_unit_is_dead();
			rot = o.get_rot();
			reaction = react;
		}
		else if (type == WOWOBJECT_TYPE::UNIT) {
			pos = o.get_pos();
			health = o.get_health();
			health_max = o.get_health_max();
			rot = o.get_rot();
			name = o.get_name();
		}
		else if (type == WOWOBJECT_TYPE::DYNAMICOBJECT) {
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


private:
	unsigned int base_addr;                       // the base address of the object manager
	GUID_t localGUID;

	bool invalid;
	long long construction_frame_num;

public:

	class iterator {
	public:
		iterator(DWORD base_addr);
		iterator& operator++();
		bool operator!=(const iterator& other) const;
		WowObject& operator*();
	private:
		WowObject current;
	};

	int valid() const;

	ObjectManager();

	std::optional<WowObject> get_first_object() const;
	std::optional<WowObject> get_object_by_GUID(GUID_t GUID) const;
	std::optional<WowObject> get_unit_by_name(const std::string &name) const;
	std::optional<WowObject> get_GO_by_name(const std::string &name) const;
	std::optional<WowObject> get_item_by_itemID(uint itemID) const;
	std::optional<WowObject> get_local_object() const;

	GUID_t get_local_GUID() const;
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
