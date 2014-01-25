local room = {}
local rules = require('rules')

local obj_layer = 3
local glow_layer = 2

local width, height
local objects

-- all input is queued 
local input_queue = {}

-- room is either in animation state or not,
-- no input is processed in animation
local animation = false
local animation_len = 0.2
local animation_start = 0.0

-- controls
local controls = {
	{'key', '_up', 'player_a', 'up'},
	{'key', '_down', 'player_a', 'down'},
	{'key', '_left', 'player_a', 'left'},
	{'key', '_right', 'player_a', 'right'},
	{'char', 'w', 'player_b', 'up'},
	{'char', 's', 'player_b', 'down'},
	{'char', 'a', 'player_b', 'left'},
	{'char', 'd', 'player_b', 'right'},
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
	if pos == nil then
		print(debug.traceback())
	end
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

-- returns nil or list of objs at pos
function room.find_at_pos(pos)
	local res = {}
	for i,obj in ipairs(objects) do
		if feql(obj.pos.x, pos.x) and feql(obj.pos.y, pos.y) then
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
	local blockers = room.find_at_pos(target_pos)
	if not blockers then
		obj.next_pos = target_pos
		return true
	else
		return false
	end
end

function room.input(id, action)
	-- find right object
	local obj = nil
	if id ~= '' then
		for _,iobj in ipairs(objects) do
			if iobj.id == id then
				room.move(iobj, action)
			end
		end
	end

	-- todo: undo and pause actions
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

	return not key.pressed(key.quit)
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

	if anim_t == 1 then
		for i,obj in ipairs(objects) do
			if obj.next_pos then
				obj.pos = obj.next_pos
				obj.next_pos = nil
			end
		end
		animation = false
	end

	return true
end

return room
