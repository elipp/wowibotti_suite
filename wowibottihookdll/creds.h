#pragma once

#include <string>

struct cred_t {
	std::string account_name, password, character_name;
	std::string login_script;
	cred_t(const std::string &account, const std::string &pw, const std::string &ch) : account_name(account), password(pw), character_name(ch) {
		char buf[512];

		sprintf(buf, "if (AccountLoginUI and AccountLoginUI:IsShown()) then AccountLoginUI:Hide(); DefaultServerLogin('%s', '%s');\
					 elseif (CharacterSelectUI and CharacterSelectUI:IsShown()) \
					then for i=0,GetNumCharacters() do local name = GetCharacterInfo(i);\
					if (name and name == '%s') then CharacterSelect_SelectCharacter(i); end end EnterWorld(); end", account_name.c_str(), password.c_str(), character_name.c_str());

		login_script = std::string(buf);

		//printf("%s, %s, %s\n", account.c_str(), password.c_str(), character_name.c_str());
	}

	cred_t() {
		account_name = "";
		password = "";
		character_name = "";
		login_script = "do end";
	}
};

extern cred_t credentials;