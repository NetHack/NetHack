
local wishtest_objects = {
   ["a rock"] = { otyp_name = "rock", quan = 1, oclass = "*" },
   ["5 +3 blessed silver daggers"] = { otyp_name = "silver dagger", blessed = 1, cursed = 0, spe = 3, quan = 5 },
   ["an empty locked large box"] = { otyp_name = "large box", is_container = 1, has_contents = 0, olocked = 1 },
   ["potion of holy water"] = { otyp_name = "water", oclass = "!", blessed = 1, cursed = 0 },
   ["potion of unholy water"] = { otyp_name = "water", oclass = "!", blessed = 0, cursed = 1 },
   ["cursed greased +2 grey dragon scale mail"] = { otyp_name = "gray dragon scale mail", oclass = "[", blessed = 0, cursed = 1, spe = 2, greased = 1 },
   ["uncursed magic marker (11)"] = { otyp_name = "magic marker", blessed = 0, cursed = 0, spe = 11 },
   ["lit oil lamp"] = { otyp_name = "oil lamp", lamplit = 1 },
   ["unlit oil lamp"] = { otyp_name = "oil lamp", lamplit = 0 },
   ["2 blank scrolls"] = { otyp_name = "blank paper", quan = 2 },
   ["3 rusty poisoned darts"] = { otyp_name = "dart", quan = 3, opoisoned = 1, oeroded = 1 },
   ["broken empty chest"] = { otyp_name = "chest", obroken = 1 },
   ["4 diluted dark green potions named whisky"] = { otyp_descr = "dark green", oclass = "!", quan = 4, odiluted = 1, has_oname = 1, oname = "whisky" },
   ["poisoned food ration"] = { otyp_name = "food ration", oclass = "%", age = 1 },
   ["empty tin"] = { otyp_name = "tin", oclass = "%", corpsenm = -1, spe = 0 },
   ["blessed tin of spinach"] = { otyp_name = "tin", oclass = "%", corpsenm = -1, spe = 1, blessed = 1 },
   ["trapped tin of floating eye meat"] = { otyp_name = "tin", oclass = "%", otrapped = 1, corpsenm_name = "floating eye" },
   ["hill orc corpse"] = { otyp_name = "corpse", oclass = "%", corpsenm_name = "hill orc" },
   ["destroy armor"] = { otyp_name = "destroy armor", oclass = "?" },
   ["enchant weapon"] = { otyp_name = "enchant weapon", oclass = "?" },
   ["scroll of food detection"] = { otyp_name = "food detection", oclass = "?" },
   ["scroll of detect food"] = { otyp_name = "food detection", oclass = "?" },
   ["spellbook of food detection"] = { otyp_name = "detect food", oclass = "+" },
   ["spell"] = { NO_OBJ = 1 },
   ["-1 ring mail"] = { otyp_name = "ring mail", oclass = "[", spe = -1 },
   ["studded leather armor"] = { otyp_name = "studded leather armor", oclass = "[" },
   ["leather armor"] = { otyp_name = "leather armor", oclass = "[" },
   ["tooled horn"] = { otyp_name = "tooled horn", oclass = "(" },
   ["meat ring"] = { otyp_name = "meat ring", oclass = "%" },
   ["beartrap"] = { otyp_name = "beartrap", oclass = "(" },
   ["bear trap"] = { otyp_name = "beartrap", oclass = "(" },
   ["landmine"] = { otyp_name = "land mine", oclass = "(" },
   ["land mine"] = { otyp_name = "land mine", oclass = "(" },
   -- ["partly eaten orange"] = { otyp_descr = "orange", oclass = "%", oeaten = ... },
};


for str, tbl in pairs(wishtest_objects) do
   local o = obj.new(str):totable();
   for field, value in pairs(tbl) do
      if (o[field] ~= value) then
         error("wished " .. str .. ", but " .. field .. " is " .. o[field] .. ", not " .. value);
      end
   end
end
