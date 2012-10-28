local laser = {}

-- called once in the beginning
function laser.init()
	the_beam = {}
	ph_count = 10
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
		the_beam.beams[i-1] = {
			photon = {}
		}
		pos2 = grid2screen(the_beam.path[i])
		for j = 1,ph_count do
			the_beam.beams[i-1].photon[j] = {
				size = rand.float(0.5, 1.0),
				rand = rand.float(0, 2),
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
				sprsheet.draw_centered('beam', layer, ph.pos, 0, ph.size)
			end
		end
	end
end

function laser.update(dt)
	if laser_on then
		for i, b in ipairs(the_beam.beams) do
			for j, ph in ipairs(b.photon) do
				local diff = the_beam.path[i+1] - the_beam.path[i]
				local dx = sign(diff.x) * ph.rand*0.4
				local dy = sign(diff.y) * ph.rand*0.4
				
				ph.pos = ph.pos + vec2(dx, dy)
			end
		end
	end
end

function sign(x)
  return x>0 and 1 or x<0 and -1 or 0
end

return laser

