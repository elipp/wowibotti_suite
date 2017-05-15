#include "wowmem.h"
#include "ctm.h"

#include "creds.h"

int const (*LUA_DoString)(const char*, const char*, const char*) = (int const(*)(const char*, const char*, const char*)) LUA_DoString_addr;
int const (*SelectUnit)(GUID_t) = (int const(*)(GUID_t)) SelectUnit_addr;

void DoString(const char* format, ...) {
	char cmd[1024];

	va_list args;
	va_start(args, format);
	vsprintf_s(cmd, format, args);
	va_end(args);

	LUA_DoString(cmd, cmd, "");
	PRINT("(DoString: executed script \"%s\")\n", cmd);
}

static const char* taint_caller;
static const char **taint_addr = (const char**)0xD4139C;

void set_taint_caller_zero() {
	//PRINT("setting taint target zero (was %s)\n", *taint_addr);
	taint_caller = *taint_addr;
	*taint_addr = 0;
}

void reset_taint_caller() {
	*taint_addr = taint_caller;
}

// this __declspec(noinline) thing has got to do with the msvc optimizer.
// seems like the inline assembly is discarded when this func is inlined, in which case were fucked
//__declspec(noinline) static void set_facing(float x) {
//	void const (*SetFacing)(float) = (void const (*)(float))0x007B9DE0;
//
//	uint this_ecx = DEREF(0xE29D28) + 0xBE0;
//
//	// printf("set_facing: this_ecx: %X\n", this_ecx);
//	// this is due to the fact that SetFacing is uses a __thiscall calling convention, 
//	// so the base addr needs to be passed in ECX. no idea which base address this is though,
//	// but it seems to be constant enough :D
//
//	__asm push ecx
//	__asm mov ecx, this_ecx
//	SetFacing(x);
//	__asm pop ecx;
//
//}

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
	return *(GUID_t*)PLAYER_TARGET_ADDR;
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
	float x;
	readAddr(base + UnitPosX, &x, sizeof(x));
	return x;
}
float WowObject::get_pos_y() const {
	float y;
	readAddr(base + UnitPosY, &y, sizeof(y));
	return y;
}
float WowObject::get_pos_z() const {
	float z;
	readAddr(base + UnitPosZ, &z, sizeof(z));
	return z;
}


vec3 WowObject::get_pos() const {
	return vec3(get_pos_x(), get_pos_y(), get_pos_z());
}

float WowObject::get_rot() const {
	float r;
	readAddr(base + UnitRot, &r, sizeof(r));
	return r;
}

bool WowObject::valid() const {
	return ((this->get_base() != 0) && ((this->get_base() & 1) == 0));
}

WowObject WowObject::next() const {
	unsigned int next_base;
	readAddr(base + this->Next, &next_base, sizeof(next_base));
	return WowObject(next_base);
}


GUID_t WowObject::get_GUID() const {
	GUID_t g;
	readAddr(base + GUID, &g, sizeof(g));
	return g;
}

int WowObject::get_type() const {
	int t;
	readAddr(base + Type, &t, sizeof(t));
	return t;
}


std::string WowObject::get_type_name() const {
	return wowobject_type_names[get_type()];
}


int WowObject::NPC_get_health() const {
	int h;
	readAddr(base + NPCHealth, &h, sizeof(h));
	return h;
}

int WowObject::NPC_get_health_max() const {
	int h;
	readAddr(base + NPCHealthMax, &h, sizeof(h));
	return h;
}

int WowObject::NPC_get_mana() const {
	int h;
	readAddr(base + NPCMana, &h, sizeof(h));
	return h;
}

int WowObject::NPC_get_mana_max() const {
	int h;
	readAddr(base + NPCManaMax, &h, sizeof(h));
	return h;
}

int WowObject::NPC_get_rage() const {
	int h;
	readAddr(base + NPCRage, &h, sizeof(h));
	return h;
}

int WowObject::NPC_get_rage_max() const {
	int h;
	readAddr(base + NPCRageMax, &h, sizeof(h));
	return h;
}
int WowObject::NPC_get_energy() const {
	int h;
	readAddr(base + NPCEnergy, &h, sizeof(h));
	return h;
}

int WowObject::NPC_get_energy_max() const {
	int h;
	readAddr(base + NPCEnergyMax, &h, sizeof(h));
	return h;
}

int WowObject::NPC_get_focus() const {
	int h;
	readAddr(base + NPCFocus, &h, sizeof(h));
	return h;
}

int WowObject::NPC_get_focus_max() const {
	int h;
	readAddr(base + NPCFocusMax, &h, sizeof(h));
	return h;
}

std::string WowObject::NPC_get_name() const {
	DWORD interm;
	readAddr(base + 0x964, &interm, sizeof(interm));
	if (!interm) {
		return "null";
	}

	const char *name;
	readAddr(interm + 0x5C, &name, sizeof(name));
	return name;
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

	eax = DEREF(eax);

	//fprintf(fp, "ECX + 0x24 = %X, ECX + 0x1C = %X, AND = %X, eax = %X, [eax] = %X\n", ECX24, ECX1C, AND, eax, DEREF(eax));
	// absolutely no fucking idea how this works :DD

	while (DEREF(eax) != ESI_GUID) {
		//fprintf(fp, "eax = %X, GUID ([eax]) = %X\n", eax, DEREF(eax));
		uint edx = AND * 0x2 + AND;
		edx = edx * 0x4 + ECX1C;
		edx = DEREF(edx) + eax;
		eax = DEREF(edx + 0x4);

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
	uint info_field;
	readAddr(base + unit_info_field, &info_field, sizeof(uint));

	uint cur_HP = 0;
	readAddr(info_field + UnitHealth, &cur_HP, sizeof(cur_HP));
	return cur_HP;
}

uint WowObject::unit_get_health_max() const {
	uint info_field;
	readAddr(base + unit_info_field, &info_field, sizeof(uint));

	uint max_HP = 0;
	readAddr(info_field + UnitHealthMax, &max_HP, sizeof(max_HP));
	return max_HP;
}

GUID_t WowObject::NPC_get_target_GUID() const {
	GUID_t target_GUID;
	readAddr(base + 0xA20, &target_GUID, sizeof(target_GUID));
	return target_GUID;
}


uint WowObject::NPC_get_buff(int index) const {
	if (!(DEREF(base + 0xF18) & 0x40000)) {
		// the function 615D40 will set up this flag. The first time one calls WOWAPI UnitBuff(), this is called. 
		// Segfault otherwise (ie. doing this without the flag 40000 set)
		GUID_t GUID = get_GUID();
		SelectUnit(GUID);

		DoString("UnitBuff(\"target\", 1)"); 
	}

	// 5469BA is the place where the buff spellID first appears in EAX
	
	uint spellid_addr = (index-1) * 4 + DEREF(base + 0x120) + 0xA8;
	uint buff_spellID = DEREF(spellid_addr);
	
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

		uint EAX_content = DEREF(EAX1);
		if (index == EAX_content) {
			uint EAX_spellid_content = DEREF(EAX1 + 0x4);
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

	if (DEREF(EDI1 + EDX1 + 0x8) == 0) {
		PRINT("EDI1+EDX1+0x8 was 0 :(\n");
		return 0;
	}

	uint duration = DEREF(EDX1 + EAX1 + 0x8);

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
		if (s == spellID) {
			return 1;
		}
	}

	return 0;
}

GUID_t WowObject::unit_get_target_GUID() const {
	GUID_t target_GUID;
	readAddr(base + UnitTargetGUID, &target_GUID, sizeof(target_GUID));
	return target_GUID;
}

uint WowObject::unit_get_buff(int index) const {

	//static const void(*buff_init)(void *) = (const void(*)(void*))0x615D40;

	if (!(DEREF(base + 0xF18) & 0x40000)) {
		// the function 615D40 will set up this flag. The first time one calls WOWAPI UnitBuff(), this is called. 
		// Segfault otherwise (ie. doing this without the flag 40000 set)
		GUID_t GUID = get_GUID();
		SelectUnit(GUID);

		DoString("UnitBuff(\"target\", 1)"); // this should call the initialization function XDD Same with UnitDebuff
											
	}

	uint edx = DEREF(base + 0x1150);
	uint al = 0xFF & DEREF(edx + (index - 1));

	if (al == 0xFF) {
		return 0;
	}


	uint buff_spellid = DEREF(al * 4 + DEREF(base + 0x120) + 0xA8);

	//PRINT("info = %X, buff_spellid should be at %p\n", info, *(uint*)info + index*4 + 0xA8);

	return buff_spellid;
}

uint WowObject::unit_get_debuff(int index) const {
	return unit_get_buff(index + 0x28); // these appear to be stored in sequence with buffs, starting from 0x29
}

int WowObject::in_combat() const {
	uint mask = (DEREF(DEREF(base + 0x120) + 0xA0)) >> 0x13; // ref: 544BA0 (WOWAPI UnitAffectingCombat): 544BE0 onwards
	return mask & 0x1;
}



WowObject::WowObject(unsigned int addr) : base(addr) {};
WowObject::WowObject() {};

unsigned int WowObject::get_base() const { return base; }

const WowObject& WowObject::operator=(const WowObject &o) {
	this->base = o.base;
	return *this;
}

int WowObject::DO_get_spellID() const {
	int spellID = 0;
	readAddr(base + 0x1BC, &spellID, sizeof(spellID)); 
	return spellID;
}

vec3 WowObject::DO_get_pos() const {
	float coords[3];
	readAddr(base + 0x1DC, coords, 3 * sizeof(float));
	return vec3(coords[0], coords[1], coords[2]);
}

std::string WowObject::GO_get_name() const {
	// see Wow.exe: 0x5FD820

	DWORD step1 = 0;
	readAddr(base + 0x224, &step1, sizeof(step1));

	if (!step1) { return "error"; }

	const char* namestr = NULL;
	readAddr(step1 + 0x78, &namestr, sizeof(namestr));

	return namestr;

}

vec3 WowObject::GO_get_pos() const {
	float coords[3];
	readAddr(base + 0x258, coords, 3 * sizeof(float));

	return vec3(coords[0], coords[1], coords[2]);
}


void ObjectManager::LoadAddresses() {

	//if (!credentials.logged_in) { this->invalid = 1; return; }

	DWORD currentManager_pre = 0;
	readAddr(clientConnection_addr_static, &currentManager_pre, sizeof(currentManager_pre));
	//PRINT("objectmanager: loadaddresses\n[0x%X] = 0x%X\n", clientConnection_addr_static, currentManager_pre);

	readAddr(currentManager_pre + objectManagerOffset, &base_addr, sizeof(base_addr));
	//PRINT("[0x%X + 0x%X] = 0x%X\n", currentManager_pre, objectManagerOffset, base_addr);

	readAddr(base_addr + localGUIDOffset, &localGUID, sizeof(localGUID));

	this->invalid = 0;

}


WowObject ObjectManager::get_first_object() const {
	unsigned int object_base_addr;
	readAddr(this->base_addr + firstObjectOffset, &object_base_addr, sizeof(object_base_addr));
	return WowObject(object_base_addr);
}

ObjectManager::ObjectManager() {
	LoadAddresses();
}

int ObjectManager::get_object_by_GUID(GUID_t GUID, WowObject *o) const {
	WowObject next = get_first_object();

	while (next.valid()) {
		if (next.get_GUID() == GUID) {
			*o = next;
			return 1;
		}
		next = next.next();
	}
	return 0;
}

int ObjectManager::get_unit_by_name(const std::string &name, WowObject *o) const {
	WowObject next = get_first_object();

	while (next.valid()) {
		if (next.get_type() == OBJECT_TYPE_UNIT) {
			if (next.unit_get_name() == name) {
				PRINT("Found %s!\n", next.unit_get_name().c_str());
				*o = next;
				return 1;
			}
		}

		next = next.next();
	}

	PRINT("get_unit_by_name: couldn't find unit with name %s\n", name.c_str());

	return 0;

}

int ObjectManager::get_GO_by_name(const std::string &name, WowObject *o) const {
	WowObject n = get_first_object();

	while (n.valid()) {
		if (n.get_type() == OBJECT_TYPE_GAMEOBJECT) {
			if (n.GO_get_name() == name) {
				*o = n;
				return 1;
			}
		}
		
		n = n.next();
	}
	
	return 0;

}


GUID_t ObjectManager::get_local_GUID() const { return localGUID; }

int ObjectManager::valid() const {
	return !base_addr || !invalid;
}

int ObjectManager::get_local_object(WowObject *o) const {
	return get_object_by_GUID(get_local_GUID(), o); // almost guaranteed to work :P
}

std::vector<WowObject> ObjectManager::get_spell_objects_with_spellID(long spellID) {
	std::vector<WowObject> matches;
	
	WowObject next = get_first_object();

	while (next.valid()) {
		if (next.get_type() == OBJECT_TYPE_DYNAMICOBJECT) {
			if (next.DO_get_spellID() == spellID) {
				matches.push_back(next);
			}
		}
		next = next.next();
	}

	return matches;
}

std::vector<WowObject> ObjectManager::find_all_NPCs_at(const vec3 &pos, float radius) {
	std::vector<WowObject> NPCs;

	WowObject o = get_first_object();

	while (o.valid()) {
		if (o.get_type() == OBJECT_TYPE_NPC) {
			vec3 opos = o.get_pos();
			float dist = (opos - pos).length();
			if (dist < radius) { NPCs.push_back(o); }
		}
		o = o.next();
	}

	return NPCs;
}

