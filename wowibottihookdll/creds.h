#pragma once

#include <string>

#include "wowmem.h"

struct cred_t {
	std::string account_name, password, character_name;
	std::string login_script;
	int valid, logged_in;
	cred_t(const std::string &account, const std::string &pw, const std::string &ch) 
		: account_name(account), password(pw), character_name(ch), valid(1), logged_in(0) {
		char buf[512];

		sprintf_s(buf, "if (AccountLoginUI and AccountLoginUI:IsShown()) then AccountLoginUI:Hide(); DefaultServerLogin(\"%s\", \"%s\"); elseif (CharacterSelectUI and CharacterSelectUI:IsShown()) then for i=0,GetNumCharacters() do local name = GetCharacterInfo(i); if (name and name == \"%s\") then CharacterSelect_SelectCharacter(i); end end EnterWorld(); end", 
			account_name.c_str(), password.c_str(), character_name.c_str());

		login_script = std::string(buf);

	}

	cred_t() {
		account_name = "";
		password = "";
		character_name = "";
		login_script = "do end";
		valid = 0;
		logged_in = 0;
	}

	void try_login() const {
		PRINT("login script: %s\n", login_script.c_str());
		DoString(login_script.c_str());
	}
};

extern cred_t credentials;
