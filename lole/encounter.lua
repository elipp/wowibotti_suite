function lole_parse_encounter()
    local s = read_file(".\\Interface\\AddOns\\lole\\encounters\\encounter_template.json")
    local j = json.decode(s)
    console_print(to_string(j))
end
