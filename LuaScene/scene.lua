-- scene.lua — базовая сцена без WorldGen и PlayerMove
-- Всё что раньше делали C++ классы — теперь здесь

local floor_id   = nil
local wall_ids   = {}

function on_load()
    -- Пол
    floor_id = Engine.spawn_static_box(0, 0, 0,  200, 1, 200)
    Engine.log_info("Floor spawned: " .. tostring(floor_id))

    -- 4 стены
    wall_ids[1] = Engine.spawn_static_box( 200, 10, 0,   1, 10, 200)
    wall_ids[2] = Engine.spawn_static_box(-200, 10, 0,   1, 10, 200)
    wall_ids[3] = Engine.spawn_static_box(0, 10,  200, 200, 10,   1)
    wall_ids[4] = Engine.spawn_static_box(0, 10, -200, 200, 10,   1)

    -- BVH оптимизация после спавна статики
    Engine.optimize_broadphase()

    -- Персонаж
    local ok = Engine.create_character(0, 2, 0)
    Engine.log_info("Character created: " .. tostring(ok))

    Engine.log_info("Scene loaded OK")
end

function on_tick(dt)
    -- Движение персонажа теперь обрабатывается внутри движка автоматически
    -- через InputState — PlayerMove.cpp больше не нужен
end

function on_unload()
    -- Очистка тел
    for _, id in ipairs(wall_ids) do
        Engine.destroy_body(id)
    end
    if floor_id then
        Engine.destroy_body(floor_id)
    end
    Engine.log_info("Scene unloaded")
end
