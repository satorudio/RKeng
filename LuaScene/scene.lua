function on_load()
    Engine.world_generate({ world_size=400, seed=1337 })
    Engine.optimize_broadphase()
    Engine.create_character(0, 2, 0)
    Engine.log_info("scene loaded OK")
end

function on_tick(dt)
    Engine.player_move()
end

function on_unload()
    Engine.world_destroy()
end
