
-- Test different src functions

local tests = {
   makeplural = {
      stamen = "stamens",
      caveman = "cavemen",
      zorkmid = "zorkmids",
      shaman = "shamans",
      algae = "algae",
      larvae = "larvae",
      woman = "women",
      nerf = "nerfs",
      serf = "serfs",
      knife = "knives",
      baluchitherium = "baluchitheria",
      mycelium = "mycelia",
      hyphae = "hyphae",
      amoeba = "amoebae",
      vertebra = "vertebrae",
      fungus = "fungi",
      homunculus = "homunculi",
      bus = "buses",
      lotus = "lotuses",
      wumpus = "wumpuses",
      nemesis = "nemeses",
      matzoh = "matzot",
      matzah = "matzot",
      gateau = "gateaus",
      gateaux = "gateauxes",
      bordeau = "bordeaus",
      ox = "oxen",
      VAX = "VAXES",
      goose = "geese",
      tomato = "tomatoes",
      potato = "potatoes",
      dingo = "dingoes",
      fox = "foxes",
      cookie = "cookies",
      ["bunch of grapes"] = "bunches of grapes",
      candelabrum = "candelabra",
      deer = "deer",
      fish = "fish",
      foot = "feet",
      tuna = "tuna",
      manes = "manes",
      ninja = "ninja",
      sheep = "sheep",
      ronin = "ronin",
      roshi = "roshi",
      shito = "shito",
      tengu = "tengu",
      vortex = "vortices",
      child = "children",
      scale = "scales",
      tooth = "teeth",
      gauntlet = "gauntlets",
      ["gauntlet of power"] = "gauntlets of power",
      priestess = "priestesses",
      valkyrie = "valkyries",
      hoof = "hooves",
      louse = "lice",
      mouse = "mice",
      ["slice of cake"] = "slices of cake",
   },
   an = {
      a = "an a",
      b = "a b",
      ["the foo"] = "the foo",
      ["molten lava"] = "molten lava",
      ["iron bars"] = "iron bars",
      ice = "ice",
      unicorn = "a unicorn",
      uranium = "a uranium",
      ["one-eyed"] = "a one-eyed",
      candy = "a candy",
      eucalyptus = "a eucalyptus",
   }
}

for func, fval in pairs(tests) do
   for instr, outstr in pairs(fval) do
      local ret = nh[func](instr)
      if ret ~= outstr then
         error(func .. "(\"" .. instr .. "\") != \"" .. outstr .. "\" (returned \"" .. ret .. "\") instead")
      end
   end
end
