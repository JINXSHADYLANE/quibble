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
end

return laser

