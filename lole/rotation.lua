function Rotation(spells, reset_interval)
    spells = spells or {};
    reset_interval = reset_interval or 3;
    local last_cast_time = 0;
    local target = nil;
    return {
        spells = spells,
        reset_interval = reset_interval,
        spell_index = 1,
        last_cast_time = last_cast_time,
        target = target,

        increment_spell_index = function(self)
            local next_index = self.spell_index + 1;
            if next_index > #self.spells then
                next_index = 1;
            end
            self.spell_index = next_index;
        end,

        check_rotation_reset = function(self)
            if GetTime() - self.last_cast_time > self.reset_interval or
                UnitGUID("target") ~= self.target then
                self.spell_index = 1;
                self.target = UnitGUID("target");
            end
        end,

        get_next_spell = function(self)
            return self.spells[self.spell_index];
        end,

        print_spells = function(self)
            for key, value in pairs(self.spells) do
                echo(key, value)
            end
        end,

        run = function(self)
            self:check_rotation_reset();
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
            return last_cast_spell.name == self.name;
        end,

        refresh_debuff = function(self)
            --local _, _, _, _, _, _, expirationTime, _, _, _, _ = UnitDebuff("target", self.name, nil)
            local active, timeleft = has_debuff("target", self.name)
            if not active then
                L_CastSpellByName(self.name);
                return true;
            end

            if active and timeleft < 1 then
                L_CastSpellByName(self.name);
                return true;
            elseif active and timeleft > 3 then
                -- skip on this rotation to not waste mana
                last_cast_spell.name = self.name;
                return true;
            else
                -- wait to cast
                return false;
            end
        end,

        on_cd = function(self)
            local start, _, _ = GetSpellCooldown(self.name);
            if start == 0 then
                return false;
            end
            return true;
        end,

        cast = function(self)
            -- Attempt to cast the spell.
            -- Return true if cast was successful (registered on server),
            -- return nil if could not cast for any reason.
            if self.condition() and not self:on_cd() and not on_gcd() then
                if self.is_debuff == true then
                    self:refresh_debuff();
                else
                    L_CastSpellByName(self.name);
                end
            end
            return self:check_cast_registered();
        end,
    }
end
