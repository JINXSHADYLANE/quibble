local laser = {}

-- called once in the beginning
function laser.init()
	the_beam = {
		path = {},
		beams = {}
	}
end

-- called on game exit
function laser.close()
end

-- Called once before turning on laser.
-- path is a table of grid space vec2s,
-- use grid2screen to convert them to 
-- screen coords if needed.
function laser.on(path)
	the_beam.path = path
	for i = 1,#path-1 do
		the_beam.beams[i] = {
			photon = {}
		}
		for j = 1,3 do
			the_beam.beams[i].photon[j] = {
				size = rand.float(0.5, 1.0),
				rand = rand.float(0, 1),
				pos = lerp(grid2screen(path[i]), grid2screen(path[i+1]), rand.float(0, 1))
			}
		end
	end
end

-- Called after laser.on, when laser
-- is about to move to a new column.
function laser.off()
	the_beam = {
		path = {},
		beams = {}
		}
end

-- called once every frame
function laser.draw(layer)
	for i = 1,#the_beam.path-1 do
		local pos1 = grid2screen(the_beam.path[i])
		local pos2 = grid2screen(the_beam.path[i+1])
		video.draw_seg(layer, pos1, pos2, rgba(0, 0, 0.2, 1))
		
		for j, ph in ipairs(the_beam.beams[i].photon) do
			sprsheet.draw_centered('beam', layer, ph.pos)
		end
	end
end

function laser.update(dt)
	--[[for i, b in ipairs(the_beam.beams) do
		for i = 1, ph in ipairs(b.photon) do
			
		end
	end]]

end

return laser

