-- Mines' End layout 1: PETRA west, champion east.
-- Approved geometry, 76x21, placed at map origin (1,0).
-- Only the PETRA room is lit. The rest of the level is dark.
-- Champion chest: 4 random real gems (no glass), 350 gold,
-- one random prophetic book. Unlocked and untrapped.
-- Chest square on this map: (72,8).
des.level_init({ style = "solidfill", fg = " " })
des.level_flags("mazelevel", "nomongen", "noteleport", "hardfloor", "noflip")
des.map({ x = 1, y = 0, map = [[
                      -----------                                           
         ------      --.-.......|             -----------------             
        --.-..--    --.....--.-.--          ---...............--            
        |......|    |.......|-...---       --......--.-........|            
        -----.--------.....----....|       |....-.----|.......--            
 -----------|...-..--..-------....--       --...-.|   ----.-..|             
 |...|.|...||..........|   --..----        ---....|      ----.|             
 |...|.|...||..-..-..-.|   |....|--       --......|       |...|-----------  
 ---+-+-+------|.-------   ---.--.--      |..-..-.|     ---...||.........|  
 |-.......-|  --..----       |.-...-------|.......--    |....--|.........|  
 |.........|  |......---     |...-.......||........|    |......+.........|  
 |.........|  -----....--    |...|-.--------.......--   ----..-|.........|  
 |.........|   -----....|    --.-------- ----.......|     |...||.........|  
 |.........|  --......---     |..-.....--|..-......--   ---...|-----------  
 -----+-----  |..-....|       --........|--..-----.|    |.....------------  
     |.-----  |.......|      ----.......-.-..|   |.|    |........-.--....-- 
     |..--.----.......|     --.||...........------.|    ----..............| 
     |...............--    --..--.....---......-...|       |.............-- 
     --------..-------     |.........-- ---......-.|       |...........---  
            ----           -----..----    --....----       |...-----.---    
                               ----        ------          -----   ---      
]]})
des.non_diggable()
des.replace_terrain({ selection = selection.floodfill(5, 12), fromterrain = ".", toterrain = ".", lit = 1 })
des.replace_terrain({ selection = selection.floodfill(6, 7), fromterrain = ".", toterrain = ".", lit = 1 })
des.replace_terrain({ selection = selection.floodfill(4, 7), fromterrain = ".", toterrain = ".", lit = 1 })
des.replace_terrain({ selection = selection.floodfill(8, 7), fromterrain = ".", toterrain = ".", lit = 1 })
des.stair("up", 18, 17)
des.petra_letter({ x=3, y=9, text="N", step=0 })
des.petra_letter({ x=4, y=9, text="T", step=0 })
des.petra_letter({ x=5, y=9, text="K", step=0 })
des.petra_letter({ x=6, y=9, text="A", step=5 })
des.petra_letter({ x=7, y=9, text="E", step=0 })
des.petra_letter({ x=8, y=9, text="E", step=0 })
des.petra_letter({ x=9, y=9, text="I", step=0 })
des.petra_letter({ x=3, y=10, text="R", step=0 })
des.petra_letter({ x=4, y=10, text="P", step=0 })
des.petra_letter({ x=5, y=10, text="V", step=0 })
des.petra_letter({ x=6, y=10, text="R", step=4 })
des.petra_letter({ x=7, y=10, text="P", step=0 })
des.petra_letter({ x=8, y=10, text="I", step=0 })
des.petra_letter({ x=9, y=10, text="H", step=0 })
des.petra_letter({ x=3, y=11, text="V", step=0 })
des.petra_letter({ x=4, y=11, text="E", step=0 })
des.petra_letter({ x=5, y=11, text="E", step=2 })
des.petra_letter({ x=6, y=11, text="T", step=3 })
des.petra_letter({ x=7, y=11, text="P", step=0 })
des.petra_letter({ x=8, y=11, text="T", step=0 })
des.petra_letter({ x=9, y=11, text="R", step=0 })
des.petra_letter({ x=3, y=12, text="N", step=0 })
des.petra_letter({ x=4, y=12, text="H", step=0 })
des.petra_letter({ x=5, y=12, text="P", step=1 })
des.petra_letter({ x=6, y=12, text="V", step=0 })
des.petra_letter({ x=7, y=12, text="U", step=0 })
des.petra_letter({ x=8, y=12, text="U", step=0 })
des.petra_letter({ x=9, y=12, text="G", step=0 })
des.engraving({ x=2, y=13, type="burn", degrade=false, text="Upon this rock I will build my church. Walk upon the rock in the tongue of the Greeks." })
des.door("closed", 4, 8)
des.door("closed", 6, 8)
des.door("closed", 8, 8)
des.door("closed", 63, 10)
des.door("closed", 6, 14)
des.object({ id="loadstone", x=4, y=7 })
des.object({ id="loadstone", x=8, y=7 })
des.object({ id="Peter's net", x=6, y=7 })
des.object({ id="Favor Stone", x=6, y=6, buc="not-cursed", achievement=1 })
des.monster({ id = "Philistine Champion", peaceful = 0, asleep = false, x = 68, y = 10,
    inventory = function()
        des.object({ id = "two-handed sword", spe = 2, buc = "uncursed" })
        des.object({ id = "bronze plate mail", buc = "uncursed" })
        des.object({ id = "helmet", buc = "uncursed" })
    end })
des.object({ id = "chest", locked = 0, trapped = 0, x = 72, y = 8,
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
