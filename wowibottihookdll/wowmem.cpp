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
	readAddr(taint_caller_addr_wow, &current_taint_caller, sizeof(current_taint_caller));
	DWORD zero = 0;
	writeAddr(taint_caller_addr_wow, &zero, sizeof(zero));
}
taint_caller_reseter::~taint_caller_reseter() {
	writeAddr(taint_caller_addr_wow, &current_taint_caller, sizeof(current_taint_caller));
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
	GUID_t GUID;
	readAddr(0xC6F700 + index * 8, &GUID, sizeof(GUID)); // these are stored at the static address C6F700 until C6F764
	return GUID;
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


static const std::string wowobject_type_names[] = {
	"0 : OBJECT",
	"1 : ITEM",
	"2 : CONTAINER",
	"3 : NPC",
	"4 : PLAYER",
	"5 : GAMEOBJECT",
	"6 : DYNAMICOBJECT",
	"7 : CORPSE",
	"8 : AREATRIGGER",
	"9 : SCENEOBJECT"
};

float WowObject::get_pos_x() const {
	return DEREF<float>(this->base + Addresses::TBC::WowObject::UnitPosX);
}
float WowObject::get_pos_y() const {
	return DEREF<float>(this->base + Addresses::TBC::WowObject::UnitPosY);
}
float WowObject::get_pos_z() const {
	return DEREF<float>(this->base + Addresses::TBC::WowObject::UnitPosZ);
}


vec3 WowObject::get_pos() const {
	ObjectManager OM;

	GUID_t mg = NPC_get_mounted_GUID();
	int mount_type = 0;
	auto mount = OM.get_object_by_GUID(mg);
	if (mg == 0 || !mount) goto normal;
	
	mount_type = mount->get_type(); // this business is needed in order not to bug out in Skybreaker fight (the "mounted GUID" is also set for mobs on the skybreaker :D)
	if (this->get_type() == OBJECT_TYPE_NPC && 
		(mount_type == OBJECT_TYPE_NPC || mount_type == OBJECT_TYPE_UNIT)) {
		// the coordinates for such "mounted" mobs are fucked up (stored relative to the mount), so have a separate block for those ^_^
		// eg. vassals in Gormok (TOC)
		return vec3(this->get_pos_x(), this->get_pos_y(), this->get_pos_z()) + vec3(mount->get_pos_x(), mount->get_pos_y(), mount->get_pos_z());
	}

normal:
	return vec3(this->get_pos_x(), this->get_pos_y(), this->get_pos_z());
}

float WowObject::get_rot() const {
	//float r;
	//readAddr(base + UnitRot, &r, sizeof(r));
	//return r;

	DWORD temp = DEREF<DWORD>(get_base() + 0xD8);
	if (!temp) return 0;

	float* facing_angle = (float*)(temp + 0x20);
	return *facing_angle;
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
	if (!next_base) { return std::nullopt; }
	return WowObject(next_base);
}


GUID_t WowObject::get_GUID() const {
	return DEREF<GUID_t>(this->base + Addresses::TBC::WowObject::Next);
}

int WowObject::get_type() const {
	return DEREF<DWORD>(this->base + Addresses::TBC::WowObject::Type);
}

std::string WowObject::get_type_name() const {
	return wowobject_type_names[get_type()];
}


int WowObject::NPC_unit_is_dead() const {
	int d;
	d = (DEREF<DWORD>(DEREF<DWORD>(base + 0xD0) + 0x48));
	return d == 0;
}

int WowObject::NPC_get_health() const {
	int h = DEREF<DWORD>(base + 0xFB0);
	return h;
}

int WowObject::NPC_get_health_max() const {
	int h = (DEREF<DWORD>(DEREF<DWORD>(base + 0xD0) + 0x68));
	return h;
}

int WowObject::NPC_get_mana() const {
	return DEREF<int>(this->base + Addresses::TBC::WowObject::NPCMana);
}

int WowObject::NPC_get_mana_max() const {
	return DEREF<int>(this->base + Addresses::TBC::WowObject::NPCManaMax);
}

int WowObject::NPC_get_rage() const {
	return DEREF<int>(this->base + Addresses::TBC::WowObject::NPCRage);
}

int WowObject::NPC_get_rage_max() const {
	return DEREF<int>(this->base + Addresses::TBC::WowObject::NPCRageMax);
}

int WowObject::NPC_get_energy() const {
	return DEREF<int>(this->base + Addresses::TBC::WowObject::NPCEnergy);
}

int WowObject::NPC_get_energy_max() const {
	return DEREF<int>(this->base + Addresses::TBC::WowObject::NPCEnergyMax);
}

int WowObject::NPC_get_focus() const {
	return DEREF<int>(this->base + Addresses::TBC::WowObject::NPCFocus);
}

int WowObject::NPC_get_focus_max() const {
	return DEREF<int>(this->base + Addresses::TBC::WowObject::NPCFocusMax);
}

std::string WowObject::NPC_get_name() const {
	DWORD interm = DEREF<DWORD>(this->base + 0x964);
	if (!interm) {
		return "null";
	}
	return std::string(DEREF<const char*>(interm + 0x5C));
}

std::string WowObject::unit_get_name() const {

	//// 0xD29BB0 is some kind of static / global string-related thing, resides in the .data segment. (look at Wow.exe:0x6D9850)
	const uint ecx = 0xC5D940; // tbc: 0xD29BB0

	uint ECX24 = ecx + 0x24;
	readAddr(ECX24, &ECX24, sizeof(ECX24));

	uint ECX1C = ecx + 0x1C;
	readAddr(ECX1C, &ECX1C, sizeof(ECX1C));

	uint ESI_GUID = (uint)get_GUID();
	uint AND = ESI_GUID & ECX24;

	uint eax = AND * 0x2 + AND;

	eax = eax * 0x4 + ECX1C;
	eax = eax + 0x4;
	eax = eax + 0x4;

	eax = DEREF<DWORD>(eax);

	//fprintf(fp, "ECX + 0x24 = %X, ECX + 0x1C = %X, AND = %X, eax = %X, [eax] = %X\n", ECX24, ECX1C, AND, eax, DEREF<DWORD>(eax));
	// absolutely no fucking idea how this works :DD

	while (DEREF<DWORD>(eax) != ESI_GUID) {
		//fprintf(fp, "eax = %X, GUID ([eax]) = %X\n", eax, DEREF<DWORD>(eax));
		uint edx = AND * 0x2 + AND;
		edx = edx * 0x4 + ECX1C;
		edx = DEREF<DWORD>(edx) + eax;
		eax = DEREF<DWORD>(edx + 0x4);

		// THERES STILL THAT WEIRD EAX+0x18 thing to figure out. the GUID is at [eax], but is also at [eax + 0x18]? :D
	}

	const char *ret = (const char*)(eax + 0x20);

	//const char *ret = NULL;

	//typedef const char*(*getnamefunc_t)(GUID_t, void*);
	//getnamefunc_t getname = (getnamefunc_t)0x67D770;

	//ret = getname(get_GUID(), (void*)0xC5D938);

	return ret;
}

uint WowObject::unit_get_health() const {
	//uint info_field;
	//readAddr(base + unit_info_field, &info_field, sizeof(uint));

	uint cur_HP = 0;
	readAddr(base + 0xFB0, &cur_HP, sizeof(cur_HP));
	return cur_HP;
}

uint WowObject::unit_get_health_max() const {
	uint info_field;
	readAddr(base + 0xD0, &info_field, sizeof(uint));

	uint max_HP = 0;
	readAddr(info_field + 0x68, &max_HP, sizeof(max_HP));
	return max_HP;
}

GUID_t WowObject::NPC_get_target_GUID() const {
	GUID_t target_GUID;
	readAddr(base + 0xA20, &target_GUID, sizeof(target_GUID));
	return target_GUID;
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
	
	uint EDX1;
	readAddr(base + 0x116C, &EDX1, sizeof(EDX1));

	if (EDX1 == 0) {
		PRINT("EDX1 was 0\n");
		return 0;
	}

	uint EDI1;
	readAddr(base + 0x1170, &EDI1, sizeof(EDI1));

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

GUID_t WowObject::unit_get_target_GUID() const {
	return DEREF<GUID_t>(this->base + Addresses::TBC::WowObject::UnitTargetGUID);
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

WowObject::WowObject(unsigned int addr) : base(addr) {};
WowObject::WowObject() : base(0) {};

unsigned int WowObject::get_base() const { return base; }

const WowObject& WowObject::operator=(const WowObject &o) {
	this->base = o.base;
	return *this;
}

int WowObject::DO_get_spellID() const {
	int spellID = 0;
	// this seems to work for wotlk (the spellid is written in about 100 different locations in the struct?)
	readAddr(base + 0x17C, &spellID, sizeof(spellID)); 
	return spellID;
}

vec3 WowObject::DO_get_pos() const {
	float coords[3];

	readAddr(base + 0xE8, coords, 3 * sizeof(float));
	return vec3(coords[0], coords[1], coords[2]);
}

std::string WowObject::GO_get_name() const {
	// see Wow.exe: 0x5FD820

	// WOTLK REVERSING NOTES:
	// GO NAME IS FETCHED FOR TOOLTIPS AT 0x6267AB
	// 0x70CDF0 is the function that actually fetches it

	DWORD step1 = 0;
	readAddr(base + 0x1A4, &step1, sizeof(step1));

	if (!step1) { return "error"; }

	const char* namestr = NULL;
	readAddr(step1 + 0x90, &namestr, sizeof(namestr));

	return namestr;

}

vec3 WowObject::GO_get_pos() const {
	float coords[3];
	readAddr(base + 0xE8, coords, 3 * sizeof(float));

	return vec3(coords[0], coords[1], coords[2]);
}


ObjectManager::iterator::iterator(DWORD base_addr) : base(base_addr) {}

ObjectManager::iterator ObjectManager::iterator::operator++() {
	auto next = WowObject(base).next();
	if (next->valid()) {
		this->base = next->get_base();
	}
	else {
		this->base = 0;
	}
	return *this;
}

bool ObjectManager::iterator::operator!=(const ObjectManager::iterator& other) const {
	return base != other.base;
}

WowObject ObjectManager::iterator::operator*() const { 
	return WowObject(base); 
}

ObjectManager::iterator ObjectManager::begin() const {
	auto o = this->get_first_object();
	if (!o) return 0;
	else return iterator(o->get_base());
}
ObjectManager::iterator ObjectManager::end() const { 
	return iterator(0); 
}


void ObjectManager::LoadAddresses() {

	//if (!credentials.logged_in) { this->invalid = 1; return; }

	DWORD clientConnection = DEREF<DWORD>(Addresses::TBC::ObjectManager::ClientConnection);
	this->base_addr = DEREF<DWORD>(clientConnection + Addresses::TBC::ObjectManager::CurrentObjectManager);
	this->localGUID = DEREF<GUID_t>(this->base_addr + Addresses::TBC::ObjectManager::LocalGUIDOffset);
	this->invalid = 0;
}

void ObjectManager::initialize() {
	LoadAddresses();
	construction_frame_num = get_current_frame_num();
}


std::optional<WowObject> ObjectManager::get_first_object() const {
	unsigned int object_base_addr = DEREF<DWORD>(this->base_addr + Addresses::TBC::ObjectManager::FirstObjectOffset);
	if (!object_base_addr) return std::nullopt;

	return WowObject(object_base_addr);
}

ObjectManager::ObjectManager() : invalid(true) {
	initialize();
}

ObjectManager ObjectManager::thisframe_objectmanager;

ObjectManager* ObjectManager::get() {

	// this is most likely not necessary anyway, but just in case we reload the addresses of OM every frame

	if (get_current_frame_num() != thisframe_objectmanager.construction_frame_num) {
		thisframe_objectmanager.initialize();
	}
	
	return &thisframe_objectmanager;
}

std::optional<WowObject> ObjectManager::get_object_by_GUID(GUID_t GUID) const {
	auto next = this->get_first_object();
	while (next) {
		if (next->get_GUID() == GUID) {
			return next;
		}
	}
	return std::nullopt;
}

std::optional<WowObject> ObjectManager::get_unit_by_name(const std::string &name) const {
	auto next = this->get_first_object();
	while (next) {
		if (next->get_type() == OBJECT_TYPE_UNIT) {
			if (next->unit_get_name() == name) {
				//PRINT("Found %s!\n", next.unit_get_name().c_str());
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
	auto i = this->get_first_object();
	while (i) {
		if (i->get_type() == OBJECT_TYPE_NPC) {
			if (i->NPC_get_name().find(name) != std::string::npos) {
				ret.push_back(*i);
			}
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
	auto iter = this->get_first_object();
	while (iter) {
		int type = iter->get_type();
		if (type == OBJECT_TYPE_NPC || type == OBJECT_TYPE_UNIT || type == OBJECT_TYPE_DYNAMICOBJECT) {
			if (iter->get_GUID() == this->get_local_GUID()) {
				c.player = WO_cached(*iter);
			}
			else {
				c.push(WO_cached(*iter, get_reaction(*player, *iter))); // get_reaction returns -1 if the types are incorrect
			}
		}
		iter = iter->next();
	}
	
	return c;

}

std::optional<WowObject> ObjectManager::get_GO_by_name(const std::string &name) const {
	auto n = this->get_first_object();
	while (n) {
		if (n->get_type() == OBJECT_TYPE_GAMEOBJECT) {
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
		if (n->get_type() == OBJECT_TYPE_ITEM) {
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
	return !base_addr || !invalid;
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
		if (next->get_type() == OBJECT_TYPE_DYNAMICOBJECT) {
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
		if (o->get_type() == OBJECT_TYPE_NPC) {
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
		if (i->get_type() == OBJECT_TYPE_UNIT) {
			units.push_back(*i);
		}
		i = i->next();
	}
	return units;
}


std::vector<WowObject> ObjectManager::get_all_combat_mobs() const {
	std::vector<WowObject> units;

	for (const auto &o : *this) {
		if (o.get_type() == OBJECT_TYPE_NPC && o.in_combat()) {
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
	static const DWORD UnitReaction = 0x7251C0;
	
	DWORD typeA = A.get_type();
	DWORD typeB = B.get_type();

	if (!(typeA == OBJECT_TYPE_UNIT || typeA == OBJECT_TYPE_NPC)) return -1;
	if (!(typeB == OBJECT_TYPE_UNIT || typeB == OBJECT_TYPE_NPC)) return -1;
	
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
	{ "OBJECT", OBJECT_TYPE_OBJECT },
	{ "ITEM", OBJECT_TYPE_ITEM },
	{ "CONTAINER", OBJECT_TYPE_CONTAINER },
	{ "NPC", OBJECT_TYPE_NPC },
	{ "UNIT", OBJECT_TYPE_UNIT },
	{ "GAMEOBJECT", OBJECT_TYPE_GAMEOBJECT },
	{ "DYNAMICOBJECT", OBJECT_TYPE_DYNAMICOBJECT},
	{"CORPSE", OBJECT_TYPE_CORPSE },
	{ "AREATRIGGER", OBJECT_TYPE_AREATRIGGER },
	{ "SCENEOBJECT", OBJECT_TYPE_SCENEOBJECT },
};

int get_type_index_from_string(const std::string &type) {
	for (const auto &t : typestring_index_map) {
		if (type == t.typestring) return t.index;
	}
	return -1;
}
