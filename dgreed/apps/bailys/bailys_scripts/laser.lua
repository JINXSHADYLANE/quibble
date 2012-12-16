local laser = {}

-- called once in the beginning
function laser.init()
	the_beam = {}
	ph_count = 20
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
		photon = {}	
		}

	for j = 1,ph_count*#path do
		the_beam.photon[j] = {
				size = 0.8,
				rand = rand.float(0.5, 2),
				pos = rand.float(0, 1)
			}
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
		for i, ph in ipairs(the_beam.photon) do
			local pos = pos_in_beam(ph.pos)	
			local size = ph.size + 0.6*math.sin(ph.rand * time.s() + ph.rand)
			sprsheet.draw_centered('beam', layer, pos, 0, size)
		end
	end
end

function laser.update(dt)
	if laser_on then
		for i, ph in ipairs(the_beam.photon) do
				ph.pos = ph.pos + 0.05/#the_beam.path
				if ph.pos >= 1 then
					ph.pos = ph.pos-1
				end
			--	ph.size = ph.size + 0.005
		end
	end
end

function pos_in_beam(t)
	assert(t >= 0 and t < 1)
	local n = (#the_beam.path-1) * t
	local i = math.floor(n) + 1
	local j = n - i + 1
	assert(i >= 1 and i < #the_beam.path)
	local pos1 = grid2screen(the_beam.path[i])
	local pos2 = grid2screen(the_beam.path[i+1])
	return lerp(pos1, pos2, j)
end

return laser

