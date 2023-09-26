#include <d3d9.h>

#include <queue>
#include <mutex>
#include <algorithm>

#include "wowmem.h"
#include "ctm.h"

#include "creds.h"
#include "hooks.h"

int const (*LUA_DoString)(const char*, const char*, const char*) = (int const(*)(const char*, const char*, const char*)) Addresses::TBC::LUA_DoString_addr;
int const (*SelectUnit)(GUID_t) = (int const(*)(GUID_t)) Addresses::TBC::SelectUnit_addr;

std::mutex hcache_t::mutex;

void DoString(const std::string format, ...) {
	char cmd[1024];

	va_list args;
	va_start(args, format);
	vsprintf_s(cmd, format.c_str(), args);
	va_end(args);

//	char cmd2[1024];
	//sprintf_s(cmd2, "getrvals(\"%s\")", cmd); // TODO decide if this is necessary

	LUA_DoString(cmd, cmd, NULL); // the last argument actually MUST be null :D otherwise taint->blocked
//	PRINT("(DoString: executed script \"%s\")\n", cmd);
}

static void echo(const char* msg) {
	//DoString("DEFAULT_CHAT_FRAME:AddMessage(\"|cff33ccff[C:%s]\")", msg); // colored text :D
	DoString("print(\"|cff33ccff[C:%s]\")", msg); // colored text :D
}

static std::queue<std::string> echo_msgqueue; // this is because lua can only be used from the main thread
static std::mutex echo_queue_mutex;

static void equeue_add(const std::string& msg) {
	echo_queue_mutex.lock();
	echo_msgqueue.push(msg);
	echo_queue_mutex.unlock();
}

void echo_queue_commit() {

	std::lock_guard lock(echo_queue_mutex);

	while (!echo_msgqueue.empty()) {
		echo(echo_msgqueue.front().c_str());
		echo_msgqueue.pop();
	}
}


static std::string echomsg_transform(const char* msg, int len) {
	// we transform all the "\""s to "\\\"", because otherwise DoString gets ripped
	const char* prevpos = msg;

	struct replaceinfo {
		std::string from;
		std::string to;
		const char* fpos;
	};

	replaceinfo transforms[] = {
		{"\"", "\\\"", msg},
		{"\\", "\\\\", msg},
		{"\n", "\\\\n", msg},
	};

	std::string buf;
	buf.reserve(2048);

	while (true) {

		replaceinfo* first_occurrence = nullptr;
		const char* firstpos = (const char*)0xFFFFFFFF;
		for (auto& t : transforms) {
			t.fpos = strstr(prevpos, t.from.c_str());
			if (t.fpos && t.fpos < firstpos) {
				firstpos = t.fpos;
				first_occurrence = &t;
			}
		}

		if (!first_occurrence) {
			if (prevpos == msg) {
				return msg;
			}
			break;
		}
		std::string toadd(prevpos, (size_t)(firstpos - prevpos));
		buf += toadd + first_occurrence->to;

		prevpos = firstpos + first_occurrence->from.length();

	}

	return buf;
}

void echo(ECHO TO, const char* format, ...) {
	char msg[1024];

	va_list args;
	va_start(args, format);
	int len = vsprintf_s(msg, format, args);
	va_end(args);

	if ((int)TO & (int)ECHO::STDOUT) PRINT("%s\n", msg); // console should be thread safe
	if ((int)TO & (int)ECHO::WOW) equeue_add(echomsg_transform(msg, len));
}

taint_caller_reseter::taint_caller_reseter() : current_taint_caller(0) {
	readAddr(this->taint_caller_addr_wow, &this->current_taint_caller);
	writeAddr(this->taint_caller_addr_wow, (DWORD)0);
}
taint_caller_reseter::~taint_caller_reseter() {
	writeAddr(this->taint_caller_addr_wow, &this->current_taint_caller);
}

const DWORD taint_caller_reseter::taint_caller_addr_wow = 0xD4139C;


static BYTE *const spellcast_counter = (BYTE*)0xD397D5;

BYTE get_spellcast_counter() {
	return *spellcast_counter;
}

void increment_spellcast_counter() {
	++*spellcast_counter;
}

GUID_t get_raid_target_GUID(int index) {
	return DEREF<GUID_t>(0xC6F700 + index * 8);
}

GUID_t get_raid_target_GUID(const std::string &marker_name) {
	static const std::unordered_map<std::string, int> marker_index_map = {
		{ "star", 1 },
		{ "circle", 2 },
		{ "diamond", 3 },
		{ "triangle", 4 },
		{ "crescent", 5 },
		{ "moon", 5 },
		{ "square", 6 },
		{ "cross", 7 },
		{ "skull", 8 }
	};

	std::string marker = marker_name;
	std::transform(marker.begin(), marker.end(), marker.begin(), ::tolower);

	auto it = marker_index_map.find(marker);
	if (it == marker_index_map.end()) {
		PRINT("get_raid_target_GUID (const std::string&): error! invalid marker \"%s\"!\n", marker_name.c_str());
		return 0;
	}

	return get_raid_target_GUID(it->second);
}


GUID_t get_target_GUID() {
	return DEREF<GUID_t>(Addresses::TBC::PLAYER_TARGET_ADDR);
}

GUID_t get_focus_GUID() {
	return DEREF<GUID_t>(Addresses::TBC::PLAYER_FOCUS_ADDR);
}


static const char* wowobject_type_names[] = {
	"OBJECT",
	"ITEM",
	"CONTAINER",
	"NPC",
	"PLAYER",
	"GAMEOBJECT",
	"DYNAMICOBJECT",
	"CORPSE",
	"AREATRIGGER",
	"SCENEOBJECT"
};

float WowObject::get_pos_x() const {
	return DEREF<float>(this->base + Addresses::TBC::WowObject::PosX);
}
float WowObject::get_pos_y() const {
	return DEREF<float>(this->base + Addresses::TBC::WowObject::PosY);
}
float WowObject::get_pos_z() const {
	return DEREF<float>(this->base + Addresses::TBC::WowObject::PosZ);
}


vec3 WowObject::get_pos() const {
	ObjectManager OM;

	// GUID_t mg = NPC_get_mounted_GUID();
	// int mount_type = 0;
	// auto mount = OM.get_object_by_GUID(mg);
	// if (mg == 0 || !mount) goto normal;
	
	// mount_type = mount->get_type(); // this business is needed in order not to bug out in Skybreaker fight (the "mounted GUID" is also set for mobs on the skybreaker :D)
	// if (this->get_type() == WOWOBJECT_TYPE::NPC && 
	// 	(mount_type == WOWOBJECT_TYPE::NPC || mount_type == WOWOBJECT_TYPE::UNIT)) {
	// 	// the coordinates for such "mounted" mobs are fucked up (stored relative to the mount), so have a separate block for those ^_^
	// 	// eg. vassals in Gormok (TOC)
	// 	return vec3(this->get_pos_x(), this->get_pos_y(), this->get_pos_z()) + vec3(mount->get_pos_x(), mount->get_pos_y(), mount->get_pos_z());
	// }

normal:
	return vec3(this->get_pos_x(), this->get_pos_y(), this->get_pos_z());
}

float WowObject::get_rot() const {
	return DEREF<float>(this->get_base() + Addresses::TBC::WowObject::Rot);
}

std::tuple<float, float, float, float> WowObject::get_xyzr() const {
	return std::make_tuple(this->get_pos_x(), this->get_pos_y(), this->get_pos_z(), this->get_rot());
}

vec3 WowObject::get_rotvec() const {
	return unitvec_from_rot(get_rot());
}


bool WowObject::valid() const {
	return ((this->get_base() != 0) && ((this->get_base() & 1) == 0));
}

std::optional<WowObject> WowObject::next() const {
	if (!this->valid()) { return std::nullopt; }
	DWORD next_base = DEREF<DWORD>(this->base + Addresses::TBC::WowObject::Next);
	if (next_base == 0 || (next_base & 0x3) != 0) { return std::nullopt; }
	return WowObject(next_base);
}


GUID_t WowObject::get_GUID() const {
	return DEREF<GUID_t>(this->base + Addresses::TBC::WowObject::GUID);
}

int WowObject::get_type() const {
	return DEREF<DWORD>(this->base + Addresses::TBC::WowObject::Type);
}

const char* WowObject::get_type_name() const {
	auto type = this->get_type();
	if (type < 0 || type >= sizeof(wowobject_type_names)) {
		return "?";
	}
	return wowobject_type_names[get_type()];
}


int WowObject::NPC_unit_is_dead() const {
	int d;
	d = (DEREF<DWORD>(DEREF<DWORD>(base + 0xD0) + 0x48));
	return d == 0;
}

const char* WowObject::get_name() const {
	switch (this->get_type()) {
		case WOWOBJECT_TYPE::NPC:
		case WOWOBJECT_TYPE::UNIT: {
			const DWORD get_name_addr = Addresses::TBC::GetUnitOrNPCNameAddr;
			const char* result;
			DWORD base = this->base;
			DWORD ecx_prev;
			_asm {
				mov ecx_prev, ecx
				mov ecx, base
				push 0
				call get_name_addr
				mov result, eax
				mov ecx, ecx_prev
			}
			return result ? result : "null";
		}
		default:
			return "Unknown";
	}
}

uint WowObject::get_health() const {
	switch (this->get_type()) {
		case WOWOBJECT_TYPE::NPC:
			return DEREF<uint>(this->base + Addresses::TBC::WowObject::NPCHealth);
		case WOWOBJECT_TYPE::UNIT:
			return DEREF<uint>(this->base + Addresses::TBC::WowObject::UnitHealth);
		default:
			return 0xFFFFFFFF;
	}
}

uint WowObject::get_health_max() const {
	switch (this->get_type()) {
		case WOWOBJECT_TYPE::NPC:
			return DEREF<uint>(this->base + Addresses::TBC::WowObject::NPCHealthMax);
		case WOWOBJECT_TYPE::UNIT:
			return DEREF<uint>(this->base + Addresses::TBC::WowObject::UnitHealthMax);
		default:
			return 0xFFFFFFFF;
	}
}

uint WowObject::get_mana() const {
	switch (this->get_type()) {
		case WOWOBJECT_TYPE::NPC:
			return DEREF<uint>(this->base + Addresses::TBC::WowObject::NPCMana);
		case WOWOBJECT_TYPE::UNIT:
			return DEREF<uint>(this->base + Addresses::TBC::WowObject::UnitMana);
		default:
			return 0xFFFFFFFF;
	}
}

uint WowObject::get_mana_max() const {
	switch (this->get_type()) {
		case WOWOBJECT_TYPE::NPC:
			return DEREF<uint>(this->base + Addresses::TBC::WowObject::NPCManaMax);
		case WOWOBJECT_TYPE::UNIT:
			return DEREF<uint>(this->base + Addresses::TBC::WowObject::UnitManaMax);
		default:
			return 0xFFFFFFFF;
	}
}

uint WowObject::get_rage() const {
	switch (this->get_type()) {
		case WOWOBJECT_TYPE::NPC:
			return DEREF<uint>(this->base + Addresses::TBC::WowObject::NPCRage);
		case WOWOBJECT_TYPE::UNIT:
			return DEREF<uint>(this->base + Addresses::TBC::WowObject::UnitRage);
		default:
			return 0xFFFFFFFF;
	}
}

uint WowObject::get_rage_max() const {
	switch (this->get_type()) {
		case WOWOBJECT_TYPE::NPC:
			return DEREF<uint>(this->base + Addresses::TBC::WowObject::NPCRageMax);
		case WOWOBJECT_TYPE::UNIT:
			return DEREF<uint>(this->base + Addresses::TBC::WowObject::UnitRageMax);
		default:
			return 0xFFFFFFFF;
	}
}

uint WowObject::get_focus() const {
	switch (this->get_type()) {
		case WOWOBJECT_TYPE::NPC:
			return DEREF<uint>(this->base + Addresses::TBC::WowObject::NPCFocus);
		case WOWOBJECT_TYPE::UNIT:
			return DEREF<uint>(this->base + Addresses::TBC::WowObject::UnitFocus);
		default:
			return 0xFFFFFFFF;
	}
}

uint WowObject::get_focus_max() const {
	switch (this->get_type()) {
		case WOWOBJECT_TYPE::NPC:
			return DEREF<uint>(this->base + Addresses::TBC::WowObject::NPCFocusMax);
		case WOWOBJECT_TYPE::UNIT:
			return DEREF<uint>(this->base + Addresses::TBC::WowObject::UnitFocusMax);
		default:
			return 0xFFFFFFFF;
	}
}

uint WowObject::NPC_get_buff(int index) const {
	if (!(DEREF<DWORD>(base + 0xF18) & 0x40000)) {
		// the function 615D40 will set up this flag. The first time one calls WOWAPI UnitBuff(), this is called. 
		// Segfault otherwise (ie. doing this without the flag 40000 set)
		GUID_t GUID = get_GUID();
		SelectUnit(GUID);

		DoString("UnitBuff(\"target\", 1)"); 
	}

	// 5469BA is the place where the buff spellID first appears in EAX
	
	uint spellid_addr = (index-1) * 4 + DEREF<DWORD>(base + 0x120) + 0xA8;
	uint buff_spellID = DEREF<DWORD>(spellid_addr);
	
	if (buff_spellID != 0)
		PRINT("base: %X, spellid_addr = %X, spellID = %u\n", get_base(), spellid_addr, buff_spellID);

	return buff_spellID;

	// UnitDebuff notes: the debuff duration is retrieved at 60B97E

}

uint WowObject::NPC_get_debuff(int index) const {
	return NPC_get_buff(index+0x28);
}

uint WowObject::NPC_get_buff_duration(int index, uint spellID) const {
	
	// if the mob has debuffs from another player, this segfaults
	// 2023: :D:D

	DWORD EDX1 = DEREF<DWORD>(this->base + 0x116C);

	if (EDX1 == 0) {
		PRINT("EDX1 was 0\n");
		return 0;
	}

	DWORD EDI1 = DEREF<DWORD>(this->base + 0x1170);
	uint EAX1 = (EDX1 << 0x4) + EDI1;

	while(1) {

		EAX1 -= 0x10;
		EDX1 -= 0x1;

		uint EAX_content = DEREF<DWORD>(EAX1);
		if (index == EAX_content) {
			uint EAX_spellid_content = DEREF<DWORD>(EAX1 + 0x4);
			if (EAX_spellid_content == spellID) {
				PRINT("EAX_spellid_content == spellID! (EAX == %08X)\n", EAX1);
				break;
			}
		}

		if (EDX1 == 0) {
			printf("EDX1 reached 0, returning 0 for duration!\n");
			return 0;
		}

	}

	EDX1 = EDX1 << 0x4;

	if (DEREF<DWORD>(EDI1 + EDX1 + 0x8) == 0) {
		PRINT("EDI1+EDX1+0x8 was 0 :(\n");
		return 0;
	}

	uint duration = DEREF<DWORD>(EDX1 + EAX1 + 0x8);

	PRINT("returned duration %u :D\n", duration);

	return duration;

}

uint WowObject::NPC_get_debuff_duration(int index, uint spellID) const {
	return NPC_get_buff_duration(index + 0x27, spellID);
}


int WowObject::NPC_has_buff(uint spellID) const {

	for (int i = 1; i < 40; ++i) {
		uint s = NPC_get_buff(i);
		if (s == spellID) {
			return 1;
		}
	}

	return 0;

}

int WowObject::NPC_has_debuff(uint spellID) const {

	for (int i = 1; i < 40; ++i) {
		uint s = NPC_get_debuff(i);
		if (s == 0) break;
		if (s == spellID) {
			return 1;
		}
	}

	return 0;
}

GUID_t WowObject::get_target_GUID() const {
	switch (this->get_type()) {
		case WOWOBJECT_TYPE::NPC:
			return DEREF<GUID_t>(this->base + Addresses::TBC::WowObject::NPCTargetGUID);
		case WOWOBJECT_TYPE::UNIT:
			return DEREF<GUID_t>(this->base + Addresses::TBC::WowObject::UnitTargetGUID);
		default:
			return 0;
	}
}

uint WowObject::unit_get_buff(int index) const {

	//static const void(*buff_init)(void *) = (const void(*)(void*))0x615D40;

	if (!(DEREF<DWORD>(base + 0xF18) & 0x40000)) {
		// the function 615D40 will set up this flag. The first time one calls WOWAPI UnitBuff(), this is called. 
		// Segfault otherwise (ie. doing this without the flag 40000 set)
		GUID_t GUID = get_GUID();
		SelectUnit(GUID);

		DoString("UnitBuff(\"target\", 1)"); // this should call the initialization function XDD Same with UnitDebuff
											
	}

	uint edx = DEREF<DWORD>(base + 0x1150);
	uint al = 0xFF & DEREF<DWORD>(edx + (index - 1));

	if (al == 0xFF) {
		return 0;
	}


	uint buff_spellid = DEREF<DWORD>(al * 4 + DEREF<DWORD>(base + 0x120) + 0xA8);

	//PRINT("info = %X, buff_spellid should be at %p\n", info, *(uint*)info + index*4 + 0xA8);

	return buff_spellid;
}

uint WowObject::unit_get_debuff(int index) const {
	return unit_get_buff(index + 0x28); // these appear to be stored in sequence with buffs, starting from 0x29
}

int WowObject::in_combat() const {
	uint mask = (DEREF<DWORD>(DEREF<DWORD>(base + 0xD0) + 0xD4)) >> 0x13; 
	return mask & 0x1;
}

uint WowObject::item_get_ID() const {
	uint ID = DEREF<DWORD>(DEREF<DWORD>(base + 0x8) + 0xC);
	return ID;
}

BYTE get_item_usecount() {
	BYTE *usecount = (BYTE*)(0xD397D5);
	return *usecount;
}

void increment_item_usecount() {
	BYTE *usecount = (BYTE*)(0xD397D5);
	++*usecount;
}

WowObject::WowObject(DWORD base_addr) : base(base_addr) {};

DWORD WowObject::get_base() const { return base; }

const WowObject& WowObject::operator=(const WowObject &o) {
	this->base = o.base;
	return *this;
}

int WowObject::DO_get_spellID() const {
	// this seems to work for wotlk (the spellid is written in about 100 different locations in the struct?)
	return DEREF<DWORD>(this->base + 0x17C);
}

vec3 WowObject::DO_get_pos() const {
	float coords[3];
	readAddr(this->base + 0xE8, &coords);
	return vec3(coords[0], coords[1], coords[2]);
}

std::string WowObject::GO_get_name() const {
	// see Wow.exe: 0x5FD820

	// WOTLK REVERSING NOTES:
	// GO NAME IS FETCHED FOR TOOLTIPS AT 0x6267AB
	// 0x70CDF0 is the function that actually fetches it

	DWORD step1 = DEREF<DWORD>(base + 0x1A4);
	if (!step1) { return "error"; }

	const char* namestr = (const char*)DEREF<DWORD>(step1 + 0x90);
	return namestr;

}

vec3 WowObject::GO_get_pos() const {
	float coords[3];
	readAddr(this->base + 0xE8, &coords);
	return vec3(coords[0], coords[1], coords[2]);
}


ObjectManager::iterator::iterator(DWORD base_addr) : current(WowObject(base_addr)) {}

ObjectManager::iterator& ObjectManager::iterator::operator++() {
	auto next = this->current.next();
	if (next) {
		this->current = *next;
	}
	else {
		this->current = WowObject(0);
	}
	return *this;
}

bool ObjectManager::iterator::operator!=(const ObjectManager::iterator& other) const {
	return this->current.get_base() != other.current.get_base();
}

WowObject& ObjectManager::iterator::operator*() { 
	return current; 
}

ObjectManager::iterator ObjectManager::begin() const {
	if (!this->valid()) { return this->end(); }
	auto o = this->get_first_object();
	if (!o) return this->end();
	else {
		return iterator(o->get_base());
	}
}
ObjectManager::iterator ObjectManager::end() const { 
	return iterator(0); 
}


std::optional<WowObject> ObjectManager::get_first_object() const {
	if (this->invalid) { return std::nullopt; }
	unsigned int object_base_addr = DEREF<DWORD>(this->base_addr + Addresses::TBC::ObjectManager::FirstObjectOffset);
	if (!object_base_addr) return std::nullopt;

	return WowObject(object_base_addr);
}

ObjectManager::ObjectManager() : base_addr(0), construction_frame_num(0), localGUID(0), invalid(true) {
	DWORD clientConnection = DEREF<DWORD>(Addresses::TBC::ObjectManager::ClientConnection);
	if (!clientConnection) {
		printf("Couldn't get ClientConnection!\n");
		return;
	}
	this->base_addr = DEREF<DWORD>(clientConnection + Addresses::TBC::ObjectManager::CurrentObjectManager);
	if (!this->base_addr) {
		printf("Couldn't get CurrentObjectManager!\n");
		return;
	}

	this->localGUID = DEREF<GUID_t>(this->base_addr + Addresses::TBC::ObjectManager::LocalGUIDOffset);
	this->construction_frame_num = get_current_frame_num();
	this->invalid = false;
}

std::optional<WowObject> ObjectManager::get_object_by_GUID(GUID_t GUID) const {
	if (!this->valid()) { return std::nullopt; }
	for (const auto o : *this) {
		if (o.get_GUID() == GUID) {
			return o;
		}
	}
	return std::nullopt;
}

std::optional<WowObject> ObjectManager::get_unit_by_name(const std::string &name) const {
	if (!this->valid()) { return std::nullopt; }
	auto next = this->get_first_object();
	while (next) {
		if (next->get_type() == WOWOBJECT_TYPE::UNIT) {
			if (next->get_name() == name) {
				//PRINT("Found %s!\n", next.get_name().c_str());
				return next;
			}
		}
		next = next->next();
	}

	PRINT("get_unit_by_name: couldn't find unit with name %s\n", name.c_str());
	return std::nullopt;
}

std::vector<WowObject> ObjectManager::get_NPCs_by_name(const std::string &name) {
	std::vector<WowObject> ret;
	if (!this->valid()) { return ret; }
	auto i = this->get_first_object();
	while (i) {
		std::string iname(i->get_name());
		if (iname.find(name) != std::string::npos) {
			ret.push_back(*i);
		}
		i = i->next();
	}

	return ret;
}

hcache_t ObjectManager::get_snapshot() const {

	hcache_t c;
	
	c.target_GUID = get_target_GUID();
	c.focus_GUID = get_focus_GUID();

	auto player = this->get_local_object();
	if (!player) { return c; }
	for (const auto iter : (*this)) {
		int type = iter.get_type();
		if (type == WOWOBJECT_TYPE::NPC || type == WOWOBJECT_TYPE::UNIT || type == WOWOBJECT_TYPE::DYNAMICOBJECT) {
			if (iter.get_GUID() == this->get_local_GUID()) {
				c.player = WO_cached(iter);
			}
			else {
				c.push(WO_cached(iter, get_reaction(*player, iter))); // get_reaction returns -1 if the types are incorrect
			}
		}
	}
	
	return c;

}

std::optional<WowObject> ObjectManager::get_GO_by_name(const std::string &name) const {
	auto n = this->get_first_object();
	while (n) {
		if (n->get_type() == WOWOBJECT_TYPE::GAMEOBJECT) {
			if (n->GO_get_name() == name) {
				return n;
			}
		}
		n = n->next();
	}
	return std::nullopt;
}

std::optional<WowObject> ObjectManager::get_item_by_itemID(uint itemID) const {
	auto n = this->get_first_object();
	while (n) {
		if (n->get_type() == WOWOBJECT_TYPE::ITEM) {
			if (n->item_get_ID() == itemID) {
				return n;
			}
		}

		n = n->next();
	}

	return std::nullopt;
}


GUID_t ObjectManager::get_local_GUID() const { return localGUID; }

int ObjectManager::valid() const {
	return base_addr && !invalid;
}

std::optional<WowObject> ObjectManager::get_local_object() const {
	return this->get_object_by_GUID(this->get_local_GUID()); // almost guaranteed to work :P
}

std::vector<WowObject> ObjectManager::get_spell_objects_with_spellID(long spellID) {
	std::vector<WowObject> matches;
	
	auto next = this->get_first_object();
	if (!next) {
		return matches;
	}

	while (next->valid()) {
		if (next->get_type() == WOWOBJECT_TYPE::DYNAMICOBJECT) {
			if (next->DO_get_spellID() == spellID) {
				matches.push_back(*next);
			}
		}
		next = next->next();
	}

	return matches;
}

std::vector<WowObject> ObjectManager::get_all_NPCs_at(const vec3 &pos, float radius) const {
	std::vector<WowObject> NPCs;

	auto o = this->get_first_object();
	while (o) {
		if (o->get_type() == WOWOBJECT_TYPE::NPC) {
			vec3 opos = o->get_pos();
			float dist = (opos - pos).length();
			if (dist < radius) { NPCs.push_back(*o); }
		}
		o = o->next();
	}

	return NPCs;
}

std::vector<WowObject> ObjectManager::get_all_units() const {
	
	std::vector<WowObject> units;
	auto i = this->get_first_object();
	while (i) {
		if (i->get_type() == WOWOBJECT_TYPE::UNIT) {
			units.push_back(*i);
		}
		i = i->next();
	}
	return units;
}


std::vector<WowObject> ObjectManager::get_all_combat_mobs() const {
	std::vector<WowObject> units;

	for (const auto &o : *this) {
		if (o.get_type() == WOWOBJECT_TYPE::NPC && o.in_combat()) {
			units.push_back(o);
		}
	}
	return units;
}


DWORD get_wow_d3ddevice() {
	// found at 0x5A49B7
	DWORD wow_static_DX9 = DEREF<DWORD>(Addresses::TBC::D3D9Device);
	if (!wow_static_DX9) return 0;

	return DEREF<DWORD>(wow_static_DX9 + Addresses::TBC::D3D9DeviceOffset);
}

DWORD get_EndScene() {
	DWORD wowd3d = get_wow_d3ddevice();
	if (!wowd3d) return 0;

	DWORD EndScene = DEREF<DWORD>(DEREF<DWORD>(wowd3d) + (0x2A*4));
	return EndScene;
}

DWORD get_BeginScene() {
	DWORD wowd3d = get_wow_d3ddevice();
	if (!wowd3d) return 0;

	DWORD BeginScene = DEREF<DWORD>(DEREF<DWORD>(wowd3d) + (0x29*4));

	return BeginScene;
}

IDirect3DDevice9 *get_wow_ID3D9() {
	return (IDirect3DDevice9*)get_wow_d3ddevice();
}

DWORD get_Present() {

	DWORD wowd3d = get_wow_d3ddevice();
	if (!wowd3d) return 0;

	DWORD Present = DEREF<DWORD>(DEREF<DWORD>(wowd3d) + (0x11*4));
	return Present;
}

DWORD get_DrawIndexedPrimitive() {
	DWORD wowd3d = get_wow_d3ddevice();
	if (!wowd3d) return 0;

	DWORD DrawIndexedPrimitive = DEREF<DWORD>(DEREF<DWORD>(wowd3d) + (0x52*4));
	return DrawIndexedPrimitive;
}

float get_distance3(const WowObject &a, const WowObject &b) {
	return (a.get_pos() - b.get_pos()).length();
}

float get_distance2(const WowObject &a, const WowObject &b) {
	vec3 ap = a.get_pos();
	vec3 bp = b.get_pos();
	return vec3(ap.x - bp.x, ap.y - bp.y, 0).length();
}	

int get_reaction(const WowObject &A, const WowObject &B) {
	const DWORD UnitReaction = Addresses::Wotlk::UnitReaction;
	
	DWORD typeA = A.get_type();
	DWORD typeB = B.get_type();

	if (!(typeA == WOWOBJECT_TYPE::UNIT || typeA == WOWOBJECT_TYPE::NPC)) return -1;
	if (!(typeB == WOWOBJECT_TYPE::UNIT || typeB == WOWOBJECT_TYPE::NPC)) return -1;
	
	DWORD baseA = A.get_base();
	DWORD baseB = B.get_base();

	int R;

	__asm {
		pushad
		mov ecx, baseA
		push baseB
		call UnitReaction
		mov R, eax
		popad
	}

	return R + 1;
}

GUID_t WowObject::NPC_get_mounted_GUID() const {
	return DEREF<GUID_t>(this->base + 0x790);
}

static const type_string_mapentry typestring_index_map[] = {
	{ "OBJECT", WOWOBJECT_TYPE::OBJECT },
	{ "ITEM", WOWOBJECT_TYPE::ITEM },
	{ "CONTAINER", WOWOBJECT_TYPE::CONTAINER },
	{ "NPC", WOWOBJECT_TYPE::NPC },
	{ "UNIT", WOWOBJECT_TYPE::UNIT },
	{ "GAMEOBJECT", WOWOBJECT_TYPE::GAMEOBJECT },
	{ "DYNAMICOBJECT", WOWOBJECT_TYPE::DYNAMICOBJECT},
	{"CORPSE", WOWOBJECT_TYPE::CORPSE },
	{ "AREATRIGGER", WOWOBJECT_TYPE::AREATRIGGER },
	{ "SCENEOBJECT", WOWOBJECT_TYPE::SCENEOBJECT },
};

int get_type_index_from_string(const std::string &type) {
	for (const auto &t : typestring_index_map) {
		if (type == t.typestring) return t.index;
	}
	return -1;
}
