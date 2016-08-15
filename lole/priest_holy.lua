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

combat_priest_holy = function()
--    local mage_tank = "Dissona";
	local mage_tank = MAIN_TANK

    if UnitName("player") == "Kasio" then
		heal_target = OFF_TANK
    else
		heal_target = mage_tank;
	end

	if casting_legit_heal() then return end

	local mana_left = UnitMana("player");

	if mana_left < 3000 and GetSpellCooldown("Shadowfiend") == 0 and validate_target() then
		CastSpellByName("Shadowfiend");
		return;
	end

	TargetUnit(heal_target);

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

	caster_range_check(35);

	local health_max = UnitHealthMax("target");
	local health_cur = UnitHealth("target");
	local targeting_self = UnitName("target") == UnitName("player");

    if UnitName("player") == "Kasio" and (health_max - health_cur) > 2000 then
        cast_spell("Greater Heal");
	elseif (health_cur < health_max * 0.30) then
		cast_spell("Greater Heal");
	elseif (should_cast_PoH()) then
		cast_spell("Prayer of Healing");
    elseif UnitHealth("player") < UnitHealthMax("player")*0.30 then
        TargetUnit("player");
        cast_spell("Greater Heal");
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
	elseif not has_buff("target", "Renew") then
		cast_spell("Renew");
    elseif heal_target == mage_tank and not has_buff("target", "Power Word: Shield") then
        cast_spell("Power Word: Shield");
	end

end
