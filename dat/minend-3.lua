-- Mines' End layout 3: PETRA north-east, champion south-west.
-- Approved geometry, 76x21, placed at map origin (1,0).
-- Only the PETRA room is lit. The rest of the level is dark.
-- Champion chest: 4 random real gems (no glass), 350 gold,
-- one random prophetic book. Unlocked and untrapped.
-- Chest square on this map: (2,18).
des.level_init({ style = "solidfill", fg = " " })
des.level_flags("mazelevel", "nomongen", "noteleport", "hardfloor", "noflip")
des.map({ x = 1, y = 0, map = [[
         ------         ---         ---                         ----------- 
   -------....-------  --.---    ----.------------              |...|.|...| 
   |...-........-|..----....|  ---........-..-.-.--             |...|.|...| 
   |.........-...--........-----.......-..........|     ---     ---+-+-+--- 
   |........---...---....-.......-----------......---   |.--    |-.......-| 
   ---.....----....-|..--|.......---       -----....-----..|    |.........| 
     -----.....--...|-.| ------....|   ---     |.-...-.....|    |.........| 
         --.....-..-----  --......--  --.|     ----.....----    |.........| 
          --.......---    |.......|  --..---     --....--       |.........| 
        ---|..-......|    |.......-- |.....|     |...-..------- -----+----- 
        |..-.-|...-..|    |..-.....-------.|     --.......-.-.--    |.|--   
        |.....--...-.|    --........|   |..|      ---..-.......|    |.-.|   
        |.---........----  ------...-----..---      ---------..------...--  
 --------+-----.-..-....--   ---.............----           |...--.-.....|  
 |.........|  --.......-.|----....-...........-.--     ---- --..........--- 
 |.........|   -----.....|-................-.....--- ---..--|.--.--....--.| 
 |.........|       |.....-....-----........|.......|--.....--.............| 
 |.........|       |....-...---   --.-..-----.-....-............-.........| 
 |.........|       --.......--     --|.............................-.-..--- 
 -----------        ----.-...|       |.------...--.....--.----..-.-------   
                       -------       ---    ---------------  ------         
]]})
des.non_diggable()
des.replace_terrain({ selection = selection.floodfill(68, 7), fromterrain = ".", toterrain = ".", lit = 1 })
des.replace_terrain({ selection = selection.floodfill(67, 2), fromterrain = ".", toterrain = ".", lit = 1 })
des.replace_terrain({ selection = selection.floodfill(69, 2), fromterrain = ".", toterrain = ".", lit = 1 })
des.replace_terrain({ selection = selection.floodfill(71, 2), fromterrain = ".", toterrain = ".", lit = 1 })
des.stair("up", 46, 3)
des.petra_letter({ x=66, y=4, text="O", step=0 })
des.petra_letter({ x=67, y=4, text="A", step=5 })
des.petra_letter({ x=68, y=4, text="T", step=0 })
des.petra_letter({ x=69, y=4, text="P", step=0 })
des.petra_letter({ x=70, y=4, text="D", step=0 })
des.petra_letter({ x=71, y=4, text="T", step=0 })
des.petra_letter({ x=72, y=4, text="S", step=0 })
des.petra_letter({ x=66, y=5, text="V", step=0 })
des.petra_letter({ x=67, y=5, text="R", step=4 })
des.petra_letter({ x=68, y=5, text="V", step=0 })
des.petra_letter({ x=69, y=5, text="P", step=0 })
des.petra_letter({ x=70, y=5, text="M", step=0 })
des.petra_letter({ x=71, y=5, text="P", step=0 })
des.petra_letter({ x=72, y=5, text="L", step=0 })
des.petra_letter({ x=66, y=6, text="M", step=0 })
des.petra_letter({ x=67, y=6, text="T", step=3 })
des.petra_letter({ x=68, y=6, text="E", step=2 })
des.petra_letter({ x=69, y=6, text="R", step=0 })
des.petra_letter({ x=70, y=6, text="M", step=0 })
des.petra_letter({ x=71, y=6, text="T", step=0 })
des.petra_letter({ x=72, y=6, text="P", step=0 })
des.petra_letter({ x=66, y=7, text="P", step=0 })
des.petra_letter({ x=67, y=7, text="P", step=0 })
des.petra_letter({ x=68, y=7, text="P", step=1 })
des.petra_letter({ x=69, y=7, text="U", step=0 })
des.petra_letter({ x=70, y=7, text="K", step=0 })
des.petra_letter({ x=71, y=7, text="N", step=0 })
des.petra_letter({ x=72, y=7, text="N", step=0 })
des.engraving({ x=65, y=8, type="burn", degrade=false, text="Upon this rock I will build my church. Walk upon the rock in the tongue of the Greeks." })
des.door("closed", 67, 3)
des.door("closed", 69, 3)
des.door("closed", 71, 3)
des.door("closed", 69, 9)
des.door("closed", 9, 13)
des.object({ id="loadstone", x=69, y=2 })
des.object({ id="loadstone", x=71, y=2 })
des.object({ id="Peter's net", x=67, y=2 })
des.object({ id="Favor Stone", x=67, y=1, buc="not-cursed", achievement=1 })
des.monster({ id = "Philistine Champion", peaceful = 0, asleep = false, x = 6, y = 16,
    inventory = function()
        des.object({ id = "two-handed sword", spe = 2, buc = "uncursed" })
        des.object({ id = "bronze plate mail", buc = "uncursed" })
        des.object({ id = "helmet", buc = "uncursed" })
    end })
des.object({ id = "chest", locked = 0, trapped = 0, x = 2, y = 18,
    contents = function()
        local gems = {
            "dilithium crystal",
            "diamond",
            "ruby",
            "jacinth",
            "sapphire",
            "black opal",
            "emerald",
            "turquoise",
            "citrine",
            "aquamarine",
            "amber",
            "topaz",
            "jet",
            "opal",
            "chrysoberyl",
            "garnet",
            "amethyst",
            "jasper",
            "fluorite",
            "obsidian",
            "agate",
            "jade",
        }
        local i
        for i = 1, 4 do
            local k = math.random(i, #gems)
            gems[i], gems[k] = gems[k], gems[i]
            des.object({ id = gems[i], buc = "uncursed", quantity = 1 })
        end
        des.object({ id = "gold piece", quantity = 350 })
        local books = {
            "move mountains",
            "hidden arrows",
            "fiery tempest",
            "northern wind",
            "tranquility",
            "finger of judgement",
            "light",
            "reveal foes",
            "healing",
            "door shall be open",
            "rebuke",
            "bewilder",
            "cure blindness",
            "noisome pestilence",
            "staggering wine",
            "seal entrance",
            "heavenly host",
            "reveal sustenance",
            "righteous fear",
            "far sight",
            "cure sickness",
            "pacification",
            "quickening",
            "piercing sight",
            "ascension",
            "abundant healing",
            "divine ability",
            "divine concealment",
            "render unto Caesar",
            "remove curse",
            "path revealed",
            "discernment",
            "resist evil",
            "transformation",
            "banishment",
            "angelic help",
            "cancellation",
            "protection",
            "leaping like a deer",
            "flesh renewed",
            "lightning flash",
        }
        des.object({ id = books[math.random(#books)], buc = "uncursed" })
    end })
