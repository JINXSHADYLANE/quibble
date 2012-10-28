local laser = {}

-- called once in the beginning
function laser.init()
	the_beam = {}
	laser_on = false
end

-- called on game exit
function laser.close()
	laser.off()
end

-- Called once before turning on laser.
-- path is a table of grid space vec2s,
-- use grid2screen to convert them to 
-- screen coords if needed.
function laser.on(path)
	the_beam = {
		path = path,
		beams = {}	
		}

	local pos1 = grid2screen(the_beam.path[1])
	local pos2 = nil
	for i = 2,#the_beam.path do
		the_beam.beams[i] = {
			photon = {}
		}
		pos2 = grid2screen(the_beam.path[i])
		for j = 1,3 do
			the_beam.beams[i].photon[j] = {
				size = rand.float(0.5, 1.0),
				rand = rand.float(0, 1),
				pos = lerp(pos1, pos2, rand.float(0, 1))
			}
		end
		pos1 = pos2
	end
	laser_on = true
end

-- Called after laser.on, when laser
-- is about to move to a new column.
function laser.off()
	the_beam = {}
	laser_on = false
end

-- called once every frame
function laser.draw(layer)
	if laser_on then
		local pos1 = grid2screen(the_beam.path[1])
		local pos2 = nil
		for i = 2,#the_beam.path do
			pos2 = grid2screen(the_beam.path[i])
			video.draw_seg(layer, pos1, pos2, rgba(0, 0, 0.2, 1))
			pos1 = pos2
		end
		for i, b in ipairs(the_beam.beams) do			
			for j, ph in ipairs(b.photon) do
				sprsheet.draw_centered('beam', layer, ph.pos)
			end
		end
	end
end

function laser.update(dt)
	if laser_on then
		for i, b in ipairs(the_beam.beams) do
			for j, ph in ipairs(b.photon) do
				ph.pos = ph.pos-- + vec2(0.05*math.cos(ph.rand), 0.1*math.sin(ph.rand + dt))
			end
		end
	end
end

return laser

