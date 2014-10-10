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
		scr_size.x/2 + (pos.x - width/2) * 48 + 24,
		scr_size.y/2 + (pos.y - height/2) * 48 + 24
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

	-- find some specific objects
	for i,obj in ipairs(objects) do
		if obj.id == 'story' then
			story_object = obj
		elseif obj.id == 'player' then
			player_object = obj
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

	if recalc_light then
		light_rays = room.cast_light()
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
			table.insert(res, vec2(p))
			p = p + dir

			-- stop at any object
			local objs = room.find_at_pos(p, true)
			if objs then
				break
			end
		end

		return res
	end

	-- for each brick
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

	-- todo: remove duplicate rays?
	
	--[[
	for i,ray in ipairs(light_rays) do
		print(unpack(ray))
	end
	print('---')
	]]

	return light_rays
end

function room.win_condition()
	-- if all eyes have a light shining into them - condition is met
	return false
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

	--return true
	return not key.down(key.quit)
end

function room.fade_alpha(x, y, t)
	return t
end

function room.render_light(light_rays, t)
	local light_color = rgba(0.2, 0.2, 0.2)
	for i,ray in ipairs(light_rays) do
		for j,pos in ipairs(ray) do
			-- skip first light, its brick
			if j > 1 then
				local p = world2screen(pos)
				sprsheet.draw_centered(
					'brick', light_layer, p, 0, 1, light_color
				)
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

			local layer = obj_layer
			if obj.layer then
				layer = layer + obj.layer
			end
			local old_alpha = obj.tint.a
			obj.tint.a = alpha
			sprsheet.draw_centered(
				obj.sprite, layer, pos, 0, 1, obj.tint
			)
			if obj.glow then
				layer = glow_layer
				if obj.layer then
					layer = layer + obj.layer
				end
				sprsheet.draw_centered(
					obj.glow, layer, pos, 0, 1, obj.tint
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

	if not animation then
		room.render_light(light_rays, t)
	end

	return true
end

return room
