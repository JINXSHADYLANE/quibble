local laser = {}

local laser_path = nil

-- called once in the beginning
function laser.init()
end

-- called on game exit
function laser.close()
end

-- Called once before turning on laser.
-- path is a table of grid space vec2s,
-- use grid2screen to convert them to 
-- screen coords if needed.
function laser.on(path)
	laser_path = path
end

-- Called after laser.on, when laser
-- is about to move to a new column.
function laser.off()
	laser_path = nil
end

-- called once every frame
function laser.draw(layer)
	if laser_path then
		if #laser_path > 0 then
			local p = grid2screen(laser_path[1] - vec2(0, 1))
			for i = 1,#laser_path do
				local p2 = grid2screen(laser_path[i])
				video.draw_seg(layer, p, p2, rgba(0.1, 0.1, 0.1))
				p = p2
			end
		end
	end
end

function laser.update(delta)
end

return laser

