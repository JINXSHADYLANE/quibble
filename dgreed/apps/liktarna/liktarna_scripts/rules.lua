local rules = {}

-- puzzlescript inspired declaration language

rules.objects = {
	player = {legend = 'a', sprite = 'heart', tint = rgba(1, 1, 1), layer=5},
	brick = {legend = 'B', sprite = 'cube', glow = 'box_blur', tint = rgba(1.0, 1.0, 1.0)},
	wall = {legend = '#', sprite = 'wall', glow = 'wall_blur', tint = rgba(1.0, 1.0, 1.0)},
	eye = {legend = 'e', sprite = 'receiver', glow = 'receiver_blur', tint = rgba(1.0, 1.0, 1.0), layer=1}, 
	mirror_l = {legend = 'l', sprite = 'mirror_1', glow = 'mirror_1_blur', tint = rgba(1.0, 1.0, 1.0)},
	mirror_r = {legend = 'r', sprite = 'mirror_2', glow = 'mirror_2_blur', tint = rgba(1.0, 1.0, 1.0)},
}

rules.desc = {
	-- player can push brick and mirrors
	{'>', 'player', 'brick', '>', '>'},
	{'>', 'player', 'mirror_l', '>', '>'},
	{'>', 'player', 'mirror_r', '>', '>'},

	-- brick can push brick and mirrors
	{'>', 'brick', 'brick', '>', '>'},
	{'>', 'brick', 'mirror_l', '>', '>'},
	{'>', 'brick', 'mirror_r', '>', '>'},

	-- mirrors can push brick and mirrors
	{'>', 'mirror_l', 'brick', '>', '>'},
	{'>', 'mirror_l', 'mirror_l', '>', '>'},
	{'>', 'mirror_l', 'mirror_r', '>', '>'},
	{'>', 'mirror_r', 'brick', '>', '>'},
	{'>', 'mirror_r', 'mirror_l', '>', '>'},
	{'>', 'mirror_r', 'mirror_r', '>', '>'},

	-- players and bricks can stand on stories
	{'>', 'brick', 'story', '>', ''},
	{'>', 'player', 'story', '>', ''},
}

---

function rules.parse_level(level)
	local width = nil
	local height = #level
	local objs = {}

	for y,row in ipairs(level) do
		if width == nil then
			width = #row
		end

		-- iterate over chars
		local x = 0
		for c in row:gmatch('.') do
			for id,obj in pairs(rules.objects) do
				if c == obj.legend then
					local glow = nil
					if obj.glow then
						glow = sprsheet.get_handle(obj.glow)
					end
					table.insert(objs, {
						id = id,
						sprite = sprsheet.get_handle(obj.sprite),
						glow = glow,
						tint = lerp(obj.tint, obj.tint, 1),
						layer = obj.layer,
						pos = vec2(x, y-1),
						rot = 0
					})
				end
			end
			x = x + 1
		end
	end

	return objs, width, height
end

return rules
