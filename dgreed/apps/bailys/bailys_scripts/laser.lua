local laser = {}

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
end

-- Called after laser.on, when laser
-- is about to move to a new column.
function laser.off()
end

-- called once every frame
function laser.draw(layer)
	local start0 = vec2(32, 32)
	local end0 = vec2(64, 32)
	sprsheet.draw_centered('beam', layer, lerp(start0, end0, 0))
	video.draw_seg(layer, start0, end0, rgba(0, 0, 0, 1))
	
end

function laser.update(delta)
end

return laser

