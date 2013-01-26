local character = {}
local character_mt = {}
character_mt.__index = character

function character:new(pos, sprite)
	local o = {
	}
	setmetatable(o, character_mt)
	return o
end

function character:update()
end

function character:render()
end

return character
