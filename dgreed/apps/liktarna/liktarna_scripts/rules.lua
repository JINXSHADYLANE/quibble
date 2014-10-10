local rules = {}

-- puzzlescript inspired declaration language

rules.objects = {
	player = {legend = 'a', sprite = 'player', tint = rgba(0.8, 0.2, 0.2)},
	brick = {legend = 'B', sprite = 'brick', glow = 'brick_glow', tint = rgba(0.9, 0.9, 0.9)},
	wall = {legend = '#', sprite = 'brick', glow = 'brick_glow', tint = rgba(0.4, 0.4, 0.4)},
	eye = {legend = 'e', sprite = 'brick', tint = rgba(0.5, 0.5, 0.5), layer=1}, 
}

rules.desc = {
	-- player can push brick
	{'>', 'player', 'brick', '>', '>'},

	-- brick can push brick
	{'>', 'brick', 'brick', '>', '>'},

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
						pos = vec2(x, y-1)
					})
				end
			end
			x = x + 1
		end
	end

	return objs, width, height
end

return rules
