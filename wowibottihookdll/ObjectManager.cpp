#include "ObjectManager.h"

extern int const (*SelectUnit)(GUID_t);
extern void DoString(const char* format, ...);

static const std::string type_names[] = {
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
	return type_names[get_type()];
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
		DoString("UnitBuff(\"target\", 1)"); // this should call the initialization function XDD
		
											 //return 0;
	}

	uint edx = DEREF(base + 0x1150);
	uint al = 0xFF & DEREF(edx + (index - 1));

	if (al == 0xFF) {
		printf("no buff at index %d!\n", index);
		return 0;
	}


	uint buff_spellid = DEREF(al * 4 + DEREF(base + 0x120) + 0xA8);

	// ok, it's not this straight forward. the function 616000 (called at 546603) needs to be reversed. affects the index*4 part.

	//printf("info = %X, buff_spellid should be at %p\n", info, *(uint*)info + index*4 + 0xA8);

	return buff_spellid;
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


WowObject ObjectManager::getFirstObject() const {
	unsigned int object_base_addr;
	readAddr(this->base_addr + firstObjectOffset, &object_base_addr, sizeof(object_base_addr));
	return WowObject(object_base_addr);
}

ObjectManager::ObjectManager() {
	LoadAddresses();
}

WowObject ObjectManager::get_object_by_GUID(GUID_t GUID) const {
	WowObject next = getFirstObject();

	while (next.valid()) {
		if (next.get_GUID() == GUID) {
			return next;
		}
		next = next.getNextObject();
	}
	return next;
}

GUID_t ObjectManager::get_localGUID() const { return localGUID; }

int ObjectManager::valid() const {
	return !base_addr;
}
