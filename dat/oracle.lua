
des.level_flags("noflip");

des.room({ type = "ordinary", lit=1, x=3,y=3, xalign="center",yalign="center", w=11,h=9, contents = function()
              des.object({ id = "statue", x = 0, y = 0, montype = "C", historic = true });
              des.object({ id = "statue", x = 0, y = 8, montype = "C", historic = true });
              des.object({ id = "statue", x =10, y = 0, montype = "C", historic = true });
              des.object({ id = "statue", x =10, y = 8, montype = "C", historic = true });
              des.object({ id = "statue", x = 5, y = 1, montype = "C", historic = true });
              des.object({ id = "statue", x = 5, y = 7, montype = "C", historic = true });
              des.object({ id = "statue", x = 2, y = 4, montype = "C", historic = true });
              des.object({ id = "statue", x = 8, y = 4, montype = "C", historic = true });

              des.room({ type = "delphi", lit = 1, x=4,y=3, w=3,h=3, contents = function()
                            des.feature("fountain", 0, 1);
                            des.feature("fountain", 1, 0);
                            des.feature("fountain", 1, 2);
                            des.feature("fountain", 2, 1);
                            des.monster("Oracle", 1, 1);
                            des.door({ state="nodoor", wall="all" });
                         end
              });

              des.monster();
              des.monster();
           end
});

des.room({ contents = function()
                 des.stair("up");
                 des.object();
              end
});

des.room({ contents = function()
                 des.stair("down");
                 des.object();
                 des.trap();
                 des.monster();
                 des.monster();
              end
});

des.room({ contents = function()
                 des.object();
                 des.object();
                 des.monster();
              end
});

des.room({ contents = function()
                 des.object();
                 des.trap();
                 des.monster();
              end
});

des.room({ contents = function()
                 des.object();
                 des.trap();
                 des.monster();
              end
});

des.random_corridors();
