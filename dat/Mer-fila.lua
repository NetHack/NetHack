des.room({ type = "ordinary",
           contents = function()
              des.stair("up")
              des.object()
              des.monster({ class = "'", peaceful=0 })
              des.monster("gold golem")
           end
})

des.room({ type = "ordinary",
           contents = function()
              des.object()
              des.object()
              des.monster({ class = "'", peaceful=0 })
              des.monster("gold golem")
           end
})

des.room({ type = "ordinary",
           contents = function()
              des.object()
              des.trap()
              des.object()
              des.monster("giant mimic")
              des.monster("giant mimic")
           end
})

des.room({ type = "ordinary",
           contents = function()
              des.stair("down")
              des.object()
              des.trap()
              des.monster({ class = "'", peaceful=0 })
              des.monster("gold golem")
              des.monster("giant mimic")
           end
})

des.room({ type = "ordinary",
           contents = function()
              des.object()
              des.object()
              des.trap()
              des.monster({ class = "'", peaceful=0 })
              des.monster("gold golem")
           end
})

des.room({ type = "ordinary",
           contents = function()
              des.object()
              des.trap()
              des.monster("giant mimic")
           end
})

des.random_corridors()
