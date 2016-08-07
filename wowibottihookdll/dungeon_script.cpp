#include "dungeon_script.h"

#include "defs.h"

#include <fstream>

static int num_default_script = 1;

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

static int read_statements(const std::string &statement, dscript_objective_t &o) {
	
	std::vector<std::string> e;
	tokenize_string(statement, "=", e);

	if (e.size() != 2) {
		PRINT("dscript-read_statements: invalid statement \"%s\", ignoring.\n", statement.c_str());
		return 0;
	}

	if (e[0] == "coords") {
		if (!parse_coords(e[1], o.wp_pos)) {
			return 0;
		}
	}
	else if (e[0] == "mobpack") {
		if (!parse_coords(e[1], o.pack_pos)) {
			return 0;
		}
	}

	else if (e[0] == "pack_type") {
		if (e[1] == "static") {
			o.pack_type = SCRIPT_PACKTYPE_STATIC;
		} 

		else if (e[1] == "patrol") {
			o.pack_type = SCRIPT_PACKTYPE_PATROL;
		}
		
		else {
			PRINT("dscript-read_statements: error: invalid value for pack_type \"%s\", expected either \"static\" or \"patrol\".\n", e[1].c_str());
			return 0;
		}
	}
	else if (e[0] == "num_mobs") {
		int num;
		char *endptr;
		num = strtol(e[1].c_str(), &endptr, 10);

		// TODO: error checking

		o.num_mobs = num;
	}
	else {
		PRINT("dscript-read_statements: error: unknown parameter \"%s\" with value \"%s\"\n", e[0].c_str(), e[1].c_str());
		return 0;
	}
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
			else {
				std::string between = l.substr(1, l.size() - 2);
				std::vector<std::string> L1;

				tokenize_string(between, ";", L1);

				dscript_objective_t obj;

				for (auto &statement : L1) {
					if (!read_statements(statement, obj)) {
						return 0;
					}

					this->tasks.push(obj);
				}
			}
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