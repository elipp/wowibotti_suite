#include "stdafx.h"
#include "dungeon_script.h"
#include "defs.h"
#include "wowmem.h"

#include <fstream>

static int num_default_script = 1;

std::unordered_map<std::string, dscript_t> dungeon_scripts;

static dscript_t *current_script = NULL;

static int parse_coords(const std::string &coords, vec3 &out) {
	std::string cvalues;
	if (!find_stuff_between(coords, '(', ')', cvalues)) {
		return 0;
	}

	std::vector<std::string> c;
	tokenize_string(cvalues, ",", c);

	if (c.size() != 3) {
		PRINT("parse_coords: error: expected 3D coords (xyz), got %d (\"%s\")\n", coords.size(), coords.c_str());
		return 0;
	}

	double values[3];
	char *endptr;

	for (int i = 0; i < 3; i++) {
		values[i] = strtod(c[i].c_str(), &endptr);
		if (endptr != (c[i].c_str() + c[i].size())) {
			PRINT("parse_coords: error: couldn't fully parse number string \"%s\"\n", c[i].c_str());
		}
	}

	//PRINT("parse_coords: got (%f, %f, %f)\n", values[0], values[1], values[2]);

	out = vec3(values[0], values[1], values[2]);

	return 1;
}

static int parse_statement(const std::string &line_stripped, dscript_objective_t *o) {
	
	std::vector<std::string> L1;
	tokenize_string(line_stripped, ";", L1);

	for (auto &statement : L1) {

		std::vector<std::string> e;
		tokenize_string(statement, "=", e);

		if (e.size() != 2) {
			PRINT("dscript::parse_statement: invalid statement \"%s\", ignoring.\n", statement.c_str());
			return 0;
		}

		const std::string &keyword = e[0];

		if (keyword == "coords") {
			if (!parse_coords(e[1], o->wp_pos)) {
				return 0;
			}
		}
		else if (keyword == "mobpack") {
			if (!parse_coords(e[1], o->pack_pos)) {
				return 0;
			}
		}

		else if (keyword == "pack_type") {
			const std::string &v = e[1];
			if (v == "static") {
				o->pack_type = SCRIPT_PACKTYPE_STATIC;
			}

			else if (v == "patrol") {
				o->pack_type = SCRIPT_PACKTYPE_PATROL;
			}

			else {
				PRINT("dscript-parse_statement: error: invalid value for pack_type \"%s\", expected either \"static\" or \"patrol\".\n", v.c_str());
				return 0;
			}
		}
		else if (keyword == "num_mobs") {
			int num;
			char *endptr;
			num = strtol(e[1].c_str(), &endptr, 10);

			// TODO: error checking

			o->num_mobs = num;
		}
		else if (keyword == "radius") {
			float r;
			char *endptr;
			r = strtod(e[1].c_str(), &endptr);

			// TODO: error checking

			o->radius = r;
		}
		else {
			PRINT("dscript-parse_statement: error: unknown keyword \"%s\" with value \"%s\"\n", e[0].c_str(), e[1].c_str());
			return 0;
		}
	}
	return 1;
}

int dscript_t::read_from_file(const std::string &filename) {
	std::ifstream scriptfile(filename);
	if (!scriptfile.is_open()) {
		PRINT("dscript_t::read_from_file(): error: couldn't open file \"%s\"!\n", filename.c_str());
		return 0;
	}

	std::vector <std::string> lines;

	std::string line;
	while (std::getline(scriptfile, line)) {
		size_t nonspace_pos = line.find_first_not_of(' ');
		
		if (nonspace_pos != std::string::npos) {
			size_t comment_pos = line.find_first_of('#');
			if (comment_pos == std::string::npos) { // "if not commented out"
				line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
				lines.push_back(line);

				//PRINT("\"%s\"\n", line.c_str());
			}
		}

	}

	scriptfile.close();

	if (lines.size() < 1) { 
		PRINT("dscript_t::read_from_file: warning: script file \"%s\" seems to be empty!\n", filename.c_str());
		return 1; 
	}

	for (auto &l : lines) {
		if (l.front() == '{') {
			if (l.back() != '}') {
				// then we most likely have a syntax error
				PRINT("dscript_t::read_from_file: syntax error: line beginning with '{' didn't end with '}'. \nLine: \"%s\"\n", l.c_str());
				return 0;
			}

			std::string between = l.substr(1, l.size() - 2);

			dscript_objective_t obj;

			if (!parse_statement(between, &obj)) {
				return 0;
			}
	
			this->tasks.push_back(obj);

		}
			
		else {
			// we should have a simple assignment of type var = value
			std::vector <std::string> tt;
			tokenize_string(l, "=", tt);
			
			if (tt.size() != 2) {
				PRINT("dscript_t::read_from_file: syntax error: expected an assignment of type \"<var> = value\", got \"%s\"\n", line.c_str());
				return 0;
			}

			if (tt[0] == "scriptname") {
				if (!find_stuff_between(tt[1], '"', '"', this->script_name)) {
					return 0;
				}
				PRINT("dscript_t::read_from_file: found scriptname directive: \"%s\"\n", this->script_name.c_str());
			}
		}
	}

	if (script_name == "") {
		script_name = "unnamed_" + std::to_string(num_default_script);
		PRINT("dscript_t::read_from_file: didn't find scriptname directive in script. Assigned name \"%s\"\n", script_name.c_str());
	}
	

	return 1;

}


int dscript_read_all() {
	
	
	dungeon_scripts = std::unordered_map<std::string, dscript_t>();

	static const std::string wd = "C:\\Users\\Elias\\Documents\\Visual Studio 2015\\Projects\\wowibotti_suite\\dscript\\";
	std::string search_path = wd + "*";

	PRINT("search_path: %s\n", search_path.c_str());

	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			// read all (real) files in current folder
			// , delete '!' read other 2 default folder . and ..
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				dscript_t newscript;
				std::string filename = wd + fd.cFileName;
				if (newscript.read_from_file(filename)) {
					if (dungeon_scripts.find(newscript.script_name) != dungeon_scripts.end()) {
						PRINT("dscript_read_all: script with scriptname \"%s\" already loaded, ignoring %s\n", newscript.script_name.c_str(), fd.cFileName);
					}
					else {
						dungeon_scripts[newscript.script_name] = newscript;
						PRINT("dscript_read_all: script \"%s\" (file %s): read OK.\n", newscript.script_name.c_str(), filename.c_str());
						}
				}
				else {
					PRINT("dscript_read_all: script \"%s\": read FAIL.\n", filename.c_str());
				}
			}
		} while (::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}
	else {
		PRINT("dscript_read_all: FindFirstFile() failed: %d\n", GetLastError());
		return 0;
	}

	PRINT("dscript_read_all: read %lu scripts\n", dungeon_scripts.size());

	return 1;
}


int dscript_run() {
	
	if (!current_script) {
		return 0;
	}

	return 1;

}


int dscript_next() {

	if (!current_script) {
		return 0;
	}

	const dscript_objective_t &o = current_script->tasks.front();

	ObjectManager OM;
	//PRINT("script: num_tasks = %u\n", current_script->tasks.size());

	std::vector<WowObject> mobs = OM.find_all_NPCs_at(o.pack_pos, o.radius);

	if (mobs.size() != o.num_mobs) {
		PRINT("dscript_next: warning: found %u mobs at (%.1f, %.1f, %.1f), radius %f, expected %u\n", mobs.size(), o.pack_pos.x, o.pack_pos.y, o.pack_pos.z, o.radius, o.num_mobs);
	}

	PRINT("list of mobs found at objective point (%.0f, %.0f, %.0f):\n", o.pack_pos.x, o.pack_pos.y, o.pack_pos.z);
	for (auto &m : mobs) {
		PRINT("Name: %s, GUID: 0x%llX\n", m.NPC_get_name().c_str(), m.get_GUID());
	}

	return 1;
}

int dscript_load(const std::string &scriptname) {
	auto s = dungeon_scripts.find(scriptname);

	if (s == dungeon_scripts.end()) {
		PRINT("dscript_load: couldn't find a dscript with name \"%s\"!\n", scriptname.c_str());
		return 0;
	}

	current_script = &(s->second);
	PRINT("dscript_load: loaded script %s!\n", current_script->script_name.c_str());

	return 1;

}

int dscript_unload() {
	if (current_script) {
		PRINT("dscript_unload: unloading script %s\n", current_script->script_name.c_str());
	}
	
	current_script = NULL;
	return 1;
}

int dscript_state(const std::string &arg) {

	if (!current_script) return 1;

	if (arg == "SCRIPT_STATE_IDLE") {
		// nop
		current_script->script_state = SCRIPT_STATE_IDLE;
	}
	else if (arg == "SCRIPT_STATE_DRINKING") {
		current_script->script_state = SCRIPT_STATE_DRINKING;
	}
	else if (arg == "SCRIPT_STATE_FIGHTING") {
		current_script->script_state = SCRIPT_STATE_FIGHTING;
	}
	else if (arg == "SCRIPT_STATE_MOVING") {
		current_script->script_state = SCRIPT_STATE_MOVING;
	}

}

std::vector<GUID_t> dscript_get_current_mobpack_GUIDs() {
	if (!current_script) return std::vector<GUID_t>();

	ObjectManager OM;

	auto mobs = OM.find_all_NPCs_at(current_script->tasks[0].pack_pos, current_script->tasks[0].radius);
	
	current_script->tasks[0].mob_GUIDs = std::vector<GUID_t>();
	for (auto &m : mobs) {
		current_script->tasks[0].mob_GUIDs.push_back(m.get_GUID());
	}

	return current_script->tasks[0].mob_GUIDs;

}