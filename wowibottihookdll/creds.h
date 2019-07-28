#pragma once

#include <string>

#include "timer.h"
#include "wowmem.h"

//static const char *fmtstring = R"(
//if (_G.lole_login_timestamp and (GetTime() - _G.lole_login_timestamp) > 8) then
//	_G.lole_charselect = nil
//	_G.lole_login_timestamp = nil
//	AccountLoginUI:Show() 
//end
//
//if not _G.lole_login_timestamp then 
//	if (AccountLoginUI and AccountLoginUI:IsShown()) then 
//		_G.lole_login_timestamp = GetTime(); 
//		DefaultServerLogin("%s", "%s"); 
//		AccountLoginUI:Hide();
//	end
//else
//	if (CharacterSelectUI and CharacterSelectUI:IsShown()) then 
//		for i = 0,GetNumCharacters() do 
//			local name = GetCharacterInfo(i); 
//			if (name and name == "%s") then 
//				CharacterSelect_SelectCharacter(i); 
//			end 
//		end
//		EnterWorld();
//	end
//end)";

static const char *fmtstring = 
"SetCVar(\"realmList\", \"logon.warmane.com\"); SetCVar(\"realmName\", \"Icecrown\"); DefaultServerLogin(\"%s\", \"%s\")";

struct cred_t {
	std::string account_name, password, character_name;
	std::string login_script;
	int valid, logged_in;
	Timer timer;
	cred_t(const std::string &account, const std::string &pw, const std::string &ch) 
		: account_name(account), password(pw), character_name(ch), valid(1), logged_in(0) {
		char buf[1024];

		sprintf_s(buf, fmtstring, account_name.c_str(), password.c_str(), character_name.c_str());
	
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

	void try_login() {
		if (account_name != "") {
			DoString(login_script.c_str());
			timer.start();
			logged_in = 1; // NOTE: THIS IS JUST FOR DEBUG
		}
	}
};

extern cred_t credentials;
