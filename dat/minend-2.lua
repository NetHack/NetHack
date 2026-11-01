-- Mines' End layout 2: PETRA north, champion south.
-- Approved geometry, 76x21, placed at map origin (1,0).
-- Only the PETRA room is lit. The rest of the level is dark.
-- Champion chest: 4 random real gems (no glass), 350 gold,
-- one random prophetic book. Unlocked and untrapped.
-- Chest square on this map: (53,15).
des.level_init({ style = "solidfill", fg = " " })
des.level_flags("mazelevel", "nomongen", "noteleport", "hardfloor", "noflip")
des.map({ x = 1, y = 0, map = [[
                       ---      -----------        -------                  
       --------------- |.---    |...|.|...|    -----.--..--------           
    ----.............---...|    |...|.|...|    |.--...........-.-- ---      
   --.........--...........---  ---+-+-+---    |.................---.-----  
  --...-...---.-...-.........|  |-.......-|    |.---.....................|  
  |..------|.-...........-..--  |.........|    |.| ---.-....----..-.....--  
  |..|     |.....----....--..|  |.........|   --.|   ------.|  -------..|   
 --.--     --.......-.....|-.|  |.........|   |..|       --.|---  ---...|   
 |..--      |.............----  |.........|   |..--     --..-..-- |....--   
 |...|--    --..--.........----------+-----   --..|     |.......| --....|   
 |...-.----  |.----...................|       --..|     ---------  ---..--  
 ---......|  ---  |.....--..-----------     ---..--                  --..|  
 --...-...|   -----.....|...|               |...--                  ----.-- 
 |........-- --......-----.--    ---------- |..--                  --.....| 
 |.........|--...-.....-...---- --...-.-..|--.--    -----------    |......--
 |........-|..................| |.........--..|     |.........|    |.-.....|
 --.......|--..--.......--.-..---.............|     |.........|-----..-...--
  --.....--.-.--|.......|----..|........-.....|     |.........+.....-....-- 
   --..........|---....--   --.-.......---....|     |.........|--..----..|  
    ---------.--  ------     |.....----|...----     |.........| --......--  
            ---              -------   -----        -----------  --------   
]]})
des.non_diggable()
des.replace_terrain({ selection = selection.floodfill(38, 7), fromterrain = ".", toterrain = ".", lit = 1 })
des.replace_terrain({ selection = selection.floodfill(39, 2), fromterrain = ".", toterrain = ".", lit = 1 })
des.replace_terrain({ selection = selection.floodfill(35, 2), fromterrain = ".", toterrain = ".", lit = 1 })
des.replace_terrain({ selection = selection.floodfill(37, 2), fromterrain = ".", toterrain = ".", lit = 1 })
des.stair("up", 20, 5)
des.petra_letter({ x=34, y=4, text="P", step=0 })
des.petra_letter({ x=35, y=4, text="R", step=0 })
des.petra_letter({ x=36, y=4, text="T", step=0 })
des.petra_letter({ x=37, y=4, text="L", step=0 })
des.petra_letter({ x=38, y=4, text="N", step=0 })
des.petra_letter({ x=39, y=4, text="A", step=5 })
des.petra_letter({ x=40, y=4, text="K", step=0 })
des.petra_letter({ x=34, y=5, text="L", step=0 })
des.petra_letter({ x=35, y=5, text="O", step=0 })
des.petra_letter({ x=36, y=5, text="R", step=0 })
des.petra_letter({ x=37, y=5, text="E", step=0 })
des.petra_letter({ x=38, y=5, text="E", step=0 })
des.petra_letter({ x=39, y=5, text="R", step=4 })
des.petra_letter({ x=40, y=5, text="G", step=0 })
des.petra_letter({ x=34, y=6, text="P", step=0 })
des.petra_letter({ x=35, y=6, text="P", step=0 })
des.petra_letter({ x=36, y=6, text="R", step=0 })
des.petra_letter({ x=37, y=6, text="L", step=0 })
des.petra_letter({ x=38, y=6, text="E", step=2 })
des.petra_letter({ x=39, y=6, text="T", step=3 })
des.petra_letter({ x=40, y=6, text="P", step=0 })
des.petra_letter({ x=34, y=7, text="M", step=0 })
des.petra_letter({ x=35, y=7, text="I", step=0 })
des.petra_letter({ x=36, y=7, text="U", step=0 })
des.petra_letter({ x=37, y=7, text="U", step=0 })
des.petra_letter({ x=38, y=7, text="P", step=1 })
des.petra_letter({ x=39, y=7, text="G", step=0 })
des.petra_letter({ x=40, y=7, text="S", step=0 })
des.engraving({ x=33, y=8, type="burn", degrade=false, text="Upon this rock I will build my church. Walk upon the rock in the tongue of the Greeks." })
des.door("closed", 35, 3)
des.door("closed", 37, 3)
des.door("closed", 39, 3)
des.door("closed", 37, 9)
des.door("closed", 62, 17)
des.object({ id="loadstone", x=35, y=2 })
des.object({ id="loadstone", x=37, y=2 })
des.object({ id="Peter's net", x=39, y=2 })
des.object({ id="Favor Stone", x=39, y=1, buc="not-cursed", achievement=1 })
des.monster({ id = "Philistine Champion", peaceful = 0, asleep = false, x = 57, y = 17,
    inventory = function()
        des.object({ id = "two-handed sword", spe = 2, buc = "uncursed" })
        des.object({ id = "bronze plate mail", buc = "uncursed" })
        des.object({ id = "helmet", buc = "uncursed" })
    end })
des.object({ id = "chest", locked = 0, trapped = 0, x = 53, y = 15,
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
