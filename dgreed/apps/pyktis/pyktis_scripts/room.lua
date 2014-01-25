local room = {}
local rules = require('rules')

local obj_layer = 3
local glow_layer = 2

local width, height
local objects

local function world2screen(pos)
	return vec2(
		scr_size.x/2 + (pos.x - width/2) * 48 + 24,
		scr_size.y/2 + (pos.y - height/2) * 48 + 24
	)
end

function room.init()
	video.set_blendmode(glow_layer, 'add')
end

function room.close()
end

function room.preenter()
	objects, width, height = rules.parse_level(room.level)
end

function room.update()
	return not key.pressed(key.quit)
end

function room.render(t)
	for i,obj in ipairs(objects) do
		if obj.sprite then
			local pos = world2screen(obj.pos)
			sprsheet.draw_centered(
				obj.sprite, obj_layer, pos, obj.tint
			)
			if obj.glow then
				sprsheet.draw_centered(
					obj.glow, glow_layer, pos, obj.tint
				)
			end
		end
	end

	return true
end

return room
