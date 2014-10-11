local room = {}
local rules = require('rules')

local obj_layer = 1
local glow_layer = 3
local light_layer = 4
local text_layer = 7

local width, height
local objects

-- all input is queued 
local input_queue = {}

-- old world states are kept here
local undo_buffer = {}

-- room is either in animation state or not,
local animation = false
local animation_len = 0.083
local animation_start = 0.0

local story_object = nil
local player_object = nil

-- story text state
local text_idx = 1
local text_vis = 0

-- light rays (array of arrays of positions)
local light_rays = {}
local old_light_rays = {}
local update_light = false

-- controls
local controls = {
	{'key', '_up', 'player', 'up'},
	{'key', '_down', 'player', 'down'},
	{'key', '_left', 'player', 'left'},
	{'key', '_right', 'player', 'right'},
	{'char', ' ', '', 'undo'},
	{'key', 'quit', '', 'pause'}
}

local move_offset = {
	['up'] = vec2(0, -1),
	['down'] = vec2(0, 1),
	['left'] = vec2(-1, 0),
	['right'] = vec2(1, 0)
}

local function world2screen(pos)
	return vec2(
		scr_size.x/2 + (pos.x - width/2) * 64 + 32,
		scr_size.y/2 + (pos.y - height/2) * 64 + 32
	)
end

function room.init()
	video.set_blendmode(glow_layer, 'add')
	video.set_blendmode(light_layer, 'add')
end

function room.close()
end

function room.preenter()
	objects, width, height = rules.parse_level(room.level)
	room.win = false

	if room.texts then
		local text1_half_size = vfont.size(room.texts[1]) * 0.5
		local text2_half_size = vfont.size(room.texts[2]) * 0.5
		room.texts_pos = {
			vec2(scr_size.x/2, 30) - text1_half_size,
			vec2(scr_size.x/2, scr_size.y - 30) - text2_half_size
		}
	end
	text_vis = 0

	for i,obj in ipairs(objects) do
		-- find some specific objects
		if obj.id == 'story' then
			story_object = obj
		elseif obj.id == 'player' then
			player_object = obj
		end

		-- correctly rotate all eyes
		if obj.id == 'eye' then
			if obj.pos.x == 0 then
				obj.rot = 0
			elseif obj.pos.x == width-1 then
				obj.rot = math.pi
			elseif obj.pos.y == 0 then
				obj.rot = math.pi/2
			elseif obj.pos.y == height-1 then
				obj.rot = math.pi/2 * 3
			end
		end
	end

	undo_buffer = {}
	light_rays = room.cast_light()
end

-- returns nil or list of objs at pos
function room.find_at_pos(pos, at_next)
	local res = {}
	for i,obj in ipairs(objects) do
		local p = obj.pos
		if at_next and obj.next_pos then
			p = obj.next_pos
		end
		if feql(p.x, pos.x) and feql(p.y, pos.y) then
			table.insert(res, obj)
		end
	end
	if #res == 0 then
		return nil
	else
		return res
	end
end

-- returns true if moved, false if blocked
function room.move(obj, dir)
	local target_pos = obj.pos + move_offset[dir]
	local back_pos = obj.pos - move_offset[dir]
	local in_front = room.find_at_pos(target_pos)
	local in_back = room.find_at_pos(back_pos)

	local result = true 

	if in_front then
		-- apply > rule
		for i,fobj in ipairs(in_front) do
			local moved = false
			for j,r in ipairs(rules.desc) do
				local d, a, b, dir_a, dir_b = unpack(r)
				if d == '>' and obj.id == a and fobj.id == b then
					if dir_b == '>' then
						if room.move(fobj, dir) then
							if dir_a ~= '>' then
								result = false
							else
								moved = true
							end
						else
							result = false
						end
					else
						if dir_a == '>' then
							moved = true
						else
							result = false
						end
					end
				end
			end
			if not moved then
				result = false
			end
		end
	end

	if result and in_back then
		-- apply < rule
		for _,fobj in ipairs(in_back) do
			for _,r in ipairs(rules.desc) do
				local d, a, b, dir_a, dir_b = unpack(r)
				if d == '<' and obj.id == a and fobj.id == b then
					if dir_b == '<' then
						local old_pos = obj.pos
						obj.pos = target_pos
						room.move(fobj, dir)
						obj.pos = old_pos
					end
				end
			end
		end
	end

	if result then
		obj.next_pos = target_pos
	end

	return result
end

function room.clone_state()
	local objects_clone = {}
	for i,obj in ipairs(objects) do
		table.insert(objects_clone, {
			pos = vec2(obj.pos)
		})
	end

	return objects_clone
end

function room.input(id, action)
	-- find right object
	local moved = false
	local obj = nil
	local old_state = room.clone_state()
	if id ~= '' then
		for _,iobj in ipairs(objects) do
			if iobj.id == id then
				if room.move(iobj, action) then
					moved = true
				end
			end
		end
	end

	local recalc_light = false

	-- if moved, keep old state in undo buffer
	if moved then
		table.insert(undo_buffer, old_state)
		recalc_light = true
	end

	if action == 'undo' and #undo_buffer > 0 then
		local old_state = undo_buffer[#undo_buffer]
		table.remove(undo_buffer)

		for i,obj in ipairs(objects) do
			obj.pos = old_state[i].pos
		end
		recalc_light = true
	end

	update_light = false
	if recalc_light then
		local new_light_rays = room.cast_light()
		local equal = room.rays_eql(new_light_rays, light_rays)
		if not equal then
			old_light_rays = light_rays
			light_rays = new_light_rays
			update_light = true
		end
	end

	if action == 'pause' then
		states.pop()
	end
end

function room.cast_light()
	local light_rays = {}

	local function cast_ray(pos, dir) 
		local res = {}
		local p = vec2(pos)

		while true do
			table.insert(res, vec2(p) + dir * 0.5)
			p = p + dir

			local objs = room.find_at_pos(p, true)
			local reflect = false

			if objs and #objs == 1 then
				local o = objs[1]

				-- mark when light hits an eye
				if o.id == 'eye' then
					o.lit = true
				end

				-- reflect light from mirrors
				if o.id == 'mirror_l' then
					if dir.y > 0 or dir.x > 0 then
						o.lit = bit.bor(o.lit, 1)
					end
					if dir.x < 0 or dir.y < 0 then
						o.lit = bit.bor(o.lit, 2)
					end
					dir = vec2(-dir.y, -dir.x)
					reflect = true
				end
				if o.id == 'mirror_r' then
					if dir.y > 0 or dir.x < 0 then
						o.lit = bit.bor(o.lit, 1)
					end
					if dir.x > 0 or dir.y < 0 then
						o.lit = bit.bor(o.lit, 2)
					end
					dir = vec2(dir.y, dir.x)
					reflect = true
				end

				-- add one more tile if player is hit
				if o.id == 'player' then
					table.insert(res, p)
				end
			end

			-- stop at any object
			if not reflect and objs then
				break
			end
		end

		-- add half a tile at the end
		if #res > 1 then
			table.insert(res, p - dir * 0.5)
		end

		return res
	end

	-- reset all eyes and mirrors
	for _,obj in ipairs(objects) do
		if obj.id == 'eye' then
			obj.lit = false
		end
		if obj.id == 'mirror_r' or obj.id == 'mirror_l' then
			obj.lit = 0
		end
	end

	-- cast light from all bricks
	for _,obj in ipairs(objects) do
		if obj.id == 'brick' then
			for _,off in pairs(move_offset) do
				local p = obj.next_pos or obj.pos
				local ray = cast_ray(p, off)
				if #ray > 1 then
					table.insert(light_rays, ray)
				end
			end
		end
	end

	return light_rays
end

function room.ray_eql(a, b)
	if #a == #b then
		for i=1,#a do
			if not eql_pos(a[i], b[i]) then
				return false
			end
		end
		return true
	end
	return false
end

function room.rays_eql(a, b)
	if #a == #b then
		for i=1,#a do
			if not room.ray_eql(a[i], b[i]) then
				return false
			end
		end
		return true
	end
	return false
end

function room.win_condition()
	-- if all eyes have a light shining into them - condition is met
	for _,obj in ipairs(objects) do
		if obj.id == 'eye' then
			if not obj.lit then
				return false
			end
		end
	end
	return true
end

function room.update_text()
	if not story_object then
		return
	end

	if eql_pos(player_object.pos, story_object.pos) then
		text_vis = text_vis + 0.02
		text_idx = 1
	else
		text_vis = text_vis - 0.02
	end

	text_vis = clamp(0, 1, text_vis)
end

function room.update()
	-- queue input
	if #input_queue < 3 then
		for _,cntrl in ipairs(controls) do
			if cntrl[1] == 'key' then
				if key.down(key[cntrl[2]]) then
					table.insert(input_queue, {cntrl[3], cntrl[4]})
				end
			else
				if char.down(cntrl[2]) then
					table.insert(input_queue, {cntrl[3], cntrl[4]})
				end
			end
		end
	end

	-- dequeue and perform input actions
	if not animation and #input_queue > 0 then
		animation = true
		animation_start = states.time()
		local action = input_queue[1]
		table.remove(input_queue, 1)
		room.input(action[1], action[2])
	end

	room.update_text()

	return true
end

function room.fade_alpha(x, y, t)
	return math.min(t*2, 1)
end

local n_rays = 8
local ray_gap = 5
local sqrt_2_by_2 = math.sqrt(2) / 2
function room.render_light(light_rays, t)
	local light_color = rgba(1, 1, 1, 1 - math.abs(math.sqrt(t)))
	for i,ray in ipairs(light_rays) do
		local s = ray[1]
		for j,pos in ipairs(ray) do
			-- skip first light, it is a brick
			if j > 1 then
				local e = pos
				local tan = normalize(rotate(e - s, math.pi/2))
				if not feql(math.abs(tan.x), sqrt_2_by_2) then
					-- straight rays
					local ws, we = world2screen(s), world2screen(e)
					for k=1,n_rays do
						local off = ((-n_rays / 2) + (k-1)) + 0.5
						off = tan * (off * ray_gap)
						video.draw_seg(light_layer,
							ws + off, we + off, light_color
						)
					end
				end
				s = e
			end
		end
	end
end

function room.render(t)
	local anim_t = nil
	if animation then
		anim_t = (states.time() - animation_start) / animation_len
		anim_t = math.min(1, anim_t)
	end
	for i,obj in ipairs(objects) do
		if obj.sprite then
			local pos = world2screen(obj.pos)
			if anim_t and obj.next_pos then
				local next_pos = world2screen(obj.next_pos)
				pos = smoothstep(pos, next_pos, anim_t)
			end
			pos.x = math.floor(pos.x)
			pos.y = math.floor(pos.y)

			local alpha = 1
			if t ~= 0 then
				if t < 0 then
					t = - t
				end
				alpha = room.fade_alpha(pos.x, pos.y, math.abs(1-t))
			end

			-- eye active/inactive rendering hack
			local sprite = obj.sprite
			local glow_sprite = obj.glow
			if obj.id == 'eye' and obj.lit == true then
				sprite = 'receiver_active'
				glow_sprite = 'receiver_active_blur'
			end

			-- mirror hack
			if obj.id == 'mirror_r' then
				if obj.lit > 0 then
					sprite = 'mirror_2_r'..tostring(obj.lit)
				end
			end
			if obj.id == 'mirror_l' then
				if obj.lit > 0 then
					sprite = 'mirror_1_r'..tostring(obj.lit)
				end
			end


			local layer = obj_layer
			if obj.layer then
				layer = layer + obj.layer
			end
			local old_alpha = obj.tint.a
			obj.tint.a = alpha
			sprsheet.draw_centered(
				sprite, layer, pos, obj.rot, 1, obj.tint
			)
			if obj.glow then
				layer = glow_layer
				if obj.layer then
					layer = layer + obj.layer
				end
				sprsheet.draw_centered(
					glow_sprite, layer, pos, obj.rot, 1, obj.tint
				)
			end

			-- player heartbeat hack
			if obj.id == 'player' then
				local t = math.fmod(time.s(), 1)
				local scale = 1 - math.sin(((t*t))*math.pi)/5
				sprsheet.draw_centered(
					'pulse', obj_layer + obj.layer+2, pos, obj.rot, scale, obj.tint
				)
				sprsheet.draw_centered(
					'pulse_blur', obj_layer + obj.layer+1, pos, obj.rot, scale, obj.tint
				)

			end

			obj.tint.a = old_alpha
		end
	end

	-- update objects after animation, check win condition
	if anim_t == 1 then
		for i,obj in ipairs(objects) do
			if obj.next_pos then
				obj.pos = obj.next_pos
				obj.next_pos = nil
			end
		end
		animation = false
		update_light = false
		if room.win_condition() then
			room.win = true
			states.pop()
		end
	end

	if text_vis > 0 then
		local col = rgba(1, 1, 1, text_vis)
		vfont.draw(
			room.texts[text_idx], 10, 
			room.texts_pos[text_idx], col
		)
	end

	--local light_t = math.max(0, math.abs(t) - 0.5)
	local light_t = t
	if not update_light and animation then
		room.render_light(light_rays, light_t)
	end

	if not animation then
		room.render_light(light_rays, light_t)
	end

	if update_light then
		local t = math.sqrt(math.sqrt(anim_t))
		room.render_light(old_light_rays, t)
		room.render_light(light_rays, 1-t)
	end

	return true
end

return room
