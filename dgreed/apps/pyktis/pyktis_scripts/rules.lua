local rules = {}

-- puzzlescript inspired declaration language

rules.objects = {
	player_a = {legend = 'a', sprite = 'player', tint = rgba(0.8, 0.2, 0.2)},
	player_b = {legend = 'b', sprite = 'player', tint = rgba(0.2, 0.2, 0.8)},
	brick = {legend = 'B', sprite = 'brick', glow = 'brick_glow', tint = rgba(1, 1, 1)},
	wall = {legend = '#', sprite = 'brick', glow = 'brick_glow', tint = rgba(0.3, 0.3, 0.3)}
}

rules.desc = {
	{'player_a >', 'brick', 'player_a >', 'brick >'},
	{'player_b >', 'brick', 'player_a >', 'brick >'},
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
