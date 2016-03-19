#include "wowmem.h"
#include "ctm.h"


int const (*LUA_DoString)(const char*, const char*, const char*) = (int const(*)(const char*, const char*, const char*)) LUA_DoString_addr;
int const (*SelectUnit)(GUID_t) = (int const(*)(GUID_t)) SelectUnit_addr;

void DoString(const char* format, ...) {
	char cmd[1024];

	va_list args;
	va_start(args, format);
	vsprintf(cmd, format, args);
	va_end(args);

	LUA_DoString(cmd, cmd, "");
}

// this __declspec(noinline) thing has got to do with the msvc optimizer.
// seems like the inline assembly is discarded when this func is inlined, in which case were fucked
__declspec(noinline) static void set_facing(float x) {
	void const (*SetFacing)(float) = (void const (*)(float))0x007B9DE0;

	uint this_ecx = DEREF(0xE29D28) + 0xBE0;

	// printf("set_facing: this_ecx: %X\n", this_ecx);
	// this is due to the fact that SetFacing is uses a __thiscall calling convention, 
	// so the base addr needs to be passed in ECX. no idea which base address this is though,
	// but it seems to be constant enough :D

	__asm push ecx
	__asm mov ecx, this_ecx
	SetFacing(x);
	__asm pop ecx;

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
		printf("get_raid_target_GUID (const std::string&): error! invalid marker \"%s\"!\n", marker_name.c_str());
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
	readAddr(base + X, &x, sizeof(x));
	return x;
}
float WowObject::get_pos_y() const {
	float y;
	readAddr(base + Y, &y, sizeof(y));
	return y;
}
float WowObject::get_pos_z() const {
	float z;
	readAddr(base + Z, &z, sizeof(z));
	return z;
}

bool WowObject::valid() const {
	return ((this->get_base() != 0) && ((this->get_base() & 1) == 0));
}

WowObject WowObject::getNextObject() const {
	unsigned int next_base;
	readAddr(base + this->Next, &next_base, sizeof(next_base));
	return WowObject(next_base);
}

vec3 WowObject::get_pos() const {
	return vec3(get_pos_x(), get_pos_y(), get_pos_z());
}

float WowObject::get_rot() const {
	float r;
	readAddr(base + R, &r, sizeof(r));
	return r;
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


int WowObject::NPC_getCurHealth() const {
	int h;
	readAddr(base + NPCcurrentHealth, &h, sizeof(h));
	return h;
}

int WowObject::NPC_getMaxHealth() const {
	int h;
	readAddr(base + NPCmaxHealth, &h, sizeof(h));
	return h;
}

int WowObject::NPC_getCurMana() const {
	int h;
	readAddr(base + NPCcurrentMana, &h, sizeof(h));
	return h;
}

int WowObject::NPC_getMaxMana() const {
	int h;
	readAddr(base + NPCmaxMana, &h, sizeof(h));
	return h;
}

int WowObject::NPC_getCurRage() const {
	int h;
	readAddr(base + NPCcurrentRage, &h, sizeof(h));
	return h;
}

int WowObject::NPC_getMaxRage() const {
	int h;
	readAddr(base + NPCmaxRage, &h, sizeof(h));
	return h;
}
int WowObject::NPC_getCurEnergy() const {
	int h;
	readAddr(base + NPCcurrentHealth, &h, sizeof(h));
	return h;
}

int WowObject::NPC_getMaxEnergy() const {
	int h;
	readAddr(base + NPCmaxEnergy, &h, sizeof(h));
	return h;
}

int WowObject::NPC_getCurFocus() const {
	int h;
	readAddr(base + NPCcurrentFocus, &h, sizeof(h));
	return h;
}

int WowObject::NPC_getMaxFocus() const {
	int h;
	readAddr(base + NPCmaxFocus, &h, sizeof(h));
	return h;
}

std::string WowObject::NPC_get_name() const {
	unsigned int name;
	readAddr(base + Name, &name, sizeof(name));
	return *(char**)(name + 0x40);
}

std::string WowObject::unit_get_name() const {

	// 0xD29BB0 is some kind of static / global string-related thing, resides in the .data segment. (look at Wow.exe:0x6D9850)
	const uint ecx = 0xD29BB0;

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

	char *ret = (char*)(eax + 0x20);

	return ret;
}

GUID_t WowObject::NPC_get_target_GUID() const {
	GUID_t target_GUID;
	readAddr(base + 0xF08, &target_GUID, sizeof(target_GUID));
	return target_GUID;
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

	//printf("info = %X, buff_spellid should be at %p\n", info, *(uint*)info + index*4 + 0xA8);

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


void ObjectManager::LoadAddresses() {
	unsigned int currentManager_pre = 0;
	readAddr(clientConnection_addr_static, &currentManager_pre, sizeof(currentManager_pre));
	readAddr(currentManager_pre + objectManagerOffset, &base_addr, sizeof(base_addr));
	readAddr(base_addr + localGUIDOffset, &localGUID, sizeof(localGUID));
}


WowObject ObjectManager::get_first_object() const {
	unsigned int object_base_addr;
	readAddr(this->base_addr + firstObjectOffset, &object_base_addr, sizeof(object_base_addr));
	return WowObject(object_base_addr);
}

ObjectManager::ObjectManager() {
	LoadAddresses();
}

WowObject ObjectManager::get_object_by_GUID(GUID_t GUID) const {
	WowObject next = get_first_object();

	while (next.valid()) {
		if (next.get_GUID() == GUID) {
			return next;
		}
		next = next.getNextObject();
	}
	return next;
}

GUID_t ObjectManager::get_local_GUID() const { return localGUID; }

int ObjectManager::valid() const {
	return !base_addr;
}

WowObject ObjectManager::get_local_object() const {
	return get_object_by_GUID(get_local_GUID()); // almost guaranteed to work :P
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
		next = next.getNextObject();
	}

	return matches;
}

