function Rotation(spells, reset_interval)
    spells = spells or {};
    reset_interval = reset_interval or 3;
    local last_cast_time = 0;
    return {
        spells = spells,
        reset_interval = reset_interval,
        spell_index = 1,
        last_cast_time = last_cast_time,

        increment_spell_index = function(self)
            local next_index = self.spell_index + 1;
            if next_index > #self.spells then
                next_index = 1;
            end
            self.spell_index = next_index;
        end,

        check_rotation_reset = function(self)
            if GetTime() - self.last_cast_time > self.reset_interval then
                self.spell_index = 1;
            end
        end,

        get_next_spell = function(self)
            self:check_rotation_reset();
            return self.spells[self.spell_index];
        end,

        print_spells = function(self)
            for key, value in pairs(self.spells) do
                echo(key, value)
            end
        end,

        run = function(self)
            local spell = self:get_next_spell();
            --echo("Casting spell: " .. self.spells[self.spell_index].name)
            if spell:cast() then
                -- Only go to next spell if previous was cast
                self.last_cast_time = GetTime();
                self:increment_spell_index();
            end
        end
    }
end

function Spell(name, condition, is_debuff)
    condition = condition or function() return true end;
    is_debuff = is_debuff or false;
    return {
        name = name,
        condition = condition,
        is_debuff = is_debuff,

        check_cast_registered = function(self)
            return nil;
        end,

        refresh_debuff = function(self)
            local _, _, _, _, _, _, expirationTime, _, _, _, _ = UnitDebuff("target", self.name)
            if expirationTime - GetTime() < 1 then
                L_CastSpellByName(self.name);
                return true;
            elseif expirationTime - GetTime() > 3 then
                -- skip
                return true;
            else
                -- wait to cast
                return false;
            end
        end,

        cast = function(self)
            if self.condition() and not self:on_gcd() then
                if self.is_debuff == true then
                    return self:refresh_debuff();
                else
                    L_CastSpellByName(self.name);
                    -- HOW TO KNOW IF THE CAST WAS REGISTERED?
                    return true;
                end
            end
            return nil;
        end,

        on_cd = function(self)
            local start, _, _ = GetSpellCooldown(self.name);
            if start == 0 then
                return false;
            end
            return true;
        end
    }
end
