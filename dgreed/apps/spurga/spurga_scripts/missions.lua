local missions = {}

function missions.init()
end

function missions.close()
end

-- event callbacks
function missions.moved(name, current_state, offset, mask_x, mask_y)
end

function missions.start(name)
end

function missions.solved(name, moves, score)
end

function missions.relax_solved(name)
end

function missions.remind(name)
end

function missions.shake()
end

return missions
