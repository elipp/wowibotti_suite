local pom_time = 0;
local mt_healer = true;

local function should_cast_PoH()
	local r = false;
	local HP_deficits = get_HP_deficits(true);
	if next(HP_deficits) == nil then return false; end

	local num_deficients = 0;
	for unit, deficit in pairs(HP_deficits) do
		if deficit > 3000 then
			num_deficients = num_deficients + 1;
			if num_deficients > 2 then
				r = true;
				break;
			end
		end
	end

	return r;
end

local last_target_checked = nil

local function alternate_targets()
	if not last_target_checked then
		last_target_checked = MAIN_TANK
	elseif last_target_checked == MAIN_TANK then
		last_target_checked = OFF_TANK
	else
		last_target_checked = OFF_TANK
	end
end

combat_priest_holy = function()
    if UnitName("player") == "Kasio" then
		alternate_targets()
    end

	if casting_legit_heal() then return end

	local mana_left = UnitMana("player");

	if mana_left < 3000 and GetSpellCooldown("Shadowfiend") == 0 and validate_target() then
		CastSpellByName("Shadowfiend");
		return;
	end

	if GetSpellCooldown("Dispel Magic") > 0 then -- dont waste cycles on GCD
		return;
	end

	TargetUnit(last_target_checked);

	if time() - pom_time > 10 then
		if cast_if_nocd("Prayer of Mending") then
			pom_time = time();
			return;
		end
	end

	if not has_buff("target", "Renew") then
		CastSpellByName("Renew");
		return;
	end

	local heal_target = "";
	if not mt_healer then
		local HP_deficits = get_HP_deficits();
		if next(HP_deficits) == nil then return; end

		local lowest = get_lowest_hp(HP_deficits);
		if not lowest then return; end

		if (HP_deficits[lowest] < 1500) then
			return;
		end
		heal_target = lowest;
	else
		heal_target = MAIN_TANK;
	end

	TargetUnit(heal_target);
	caster_range_check(30);

	local health_max = UnitHealthMax("target");
	local health_cur = UnitHealth("target");
	local targeting_self = UnitName(heal_target) == UnitName("player");

	if (UnitName(heal_target) == MAIN_TANK and health_cur < health_max * 0.30) then
		cast_spell("Greater Heal");
	elseif (should_cast_PoH()) then
		cast_spell("Prayer of Healing");
	elseif (health_cur < health_max * 0.60) then
		if not targeting_self and (UnitHealth("player") < UnitHealthMax("player")*0.50) then
			cast_spell("Binding Heal");
		else
			cast_spell("Greater Heal");
		end
	elseif (health_cur < health_max * 0.80) then
		if not targeting_self and (UnitHealth("player") < UnitHealthMax("player")*0.70) then
			cast_spell("Binding Heal");
		else
			cast_spell("Greater Heal(Rank 1)");
		end
	elseif not has_buff(heal_target, "Renew") then
		CastSpellByName("Renew");
	end

end
