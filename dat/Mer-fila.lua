des.level_init({ style="mines", fg=".", bg="T", smoothed=true, joined=true, walled=true })
des.level_flags("mazelevel", "noflip");

--
des.stair("up")
des.stair("down")
--
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
--
des.trap()
des.trap()
des.trap()
des.trap()
--
des.monster({ id = "gold golem", peaceful=0 })
des.monster({ id = "gold golem", peaceful=0 })
des.monster({ id = "stone golem", peaceful=0 })
des.monster({ id = "stone golem", peaceful=0 })
des.monster({ id = "stone golem", peaceful=0 })
des.monster({ class = "'", peaceful=0 })
des.monster({ id = "giant mimic", peaceful=0 })
des.monster({ id = "giant mimic", peaceful=0 })
