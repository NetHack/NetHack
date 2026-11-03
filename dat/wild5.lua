-- Wilderness Trial 5: the garden. The Sword of the Spirit is on the altar.
des.level_init({ style = "solidfill", fg = " " })
des.level_flags("mazelevel", "nomongen", "noflip", "hardfloor", "noteleport")

des.map([[
---------------
|.............|
|.............|
|..{.......{..|
|......_......|
|..{.......{..|
|.............|
|.............|
---------------
]])
des.region(selection.area(1,1,13,7), "lit")
des.feature({ x=1, y=1, type="tree" })
des.feature({ x=13, y=1, type="tree" })
des.feature({ x=1, y=7, type="tree" })
des.feature({ x=13, y=7, type="tree" })
des.stair("up", 7, 1)
des.feature("fountain", 3, 3)
des.feature("fountain", 11, 3)
des.feature("fountain", 3, 5)
des.feature("fountain", 11, 5)
des.altar({ x=7, y=4, align="law", type="altar" })
-- No verse on the altar. The sword's pickup line must stay visible.
-- The last of Psalm 91, ringing the altar. Exact KJV.
des.engraving({ x=7, y=3, type="verse", degrade=false, text="Because he hath set his love upon me, therefore will I deliver him:" })
des.engraving({ x=8, y=3, type="verse", degrade=false, text="I will set him on high, because he hath known my name." })
des.engraving({ x=8, y=4, type="verse", degrade=false, text="He shall call upon me, and I will answer him:" })
des.engraving({ x=8, y=5, type="verse", degrade=false, text="I will be with him in trouble; I will deliver him, and honour him." })
des.engraving({ x=7, y=5, type="verse", degrade=false, text="With long life will I satisfy him, and shew him my salvation." })
des.engraving({ x=6, y=5, type="verse", degrade=false, text="Because he hath set his love upon me, therefore will I deliver him: I will set him on high, because he hath known my name." })
des.engraving({ x=6, y=4, type="verse", degrade=false, text="He shall call upon me, and I will answer him: I will be with him in trouble; I will deliver him, and honour him." })
des.engraving({ x=6, y=3, type="verse", degrade=false, text="With long life will I satisfy him, and shew him my salvation." })
des.object({ id="doubled-edged blade", x=7, y=4, spe=2, buc="blessed", name="Sword of the Spirit" })
