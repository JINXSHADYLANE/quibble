local game = {}

move_len = 0.5
color_counter = 1

local function map2world(p)
	return p * 64 + vec2(63, 63)
end

-- candies

local candy_pos = {
	vec2(2, 2),
	vec2(6, 2),
	vec2(9, 2),
	vec2(12, 2),
	vec2(16, 2),
	vec2(19, 2),
	vec2(22, 2),
	vec2(26, 2),

	vec2(2, 14),
	vec2(6, 14),
	vec2(9, 14),
	vec2(12, 14),
	vec2(16, 14),
	vec2(19, 14),
	vec2(22, 14),
	vec2(26, 14),

	vec2(2, 6),
	vec2(6, 6),
	vec2(9, 6),
	vec2(12, 6),
	vec2(16, 6),
	vec2(19, 6),
	vec2(22, 6),
	vec2(26, 6),

	vec2(2, 10),
	vec2(6, 10),
	vec2(9, 10),
	vec2(12, 10),
	vec2(16, 10),
	vec2(19, 10),
	vec2(22, 10),
	vec2(26, 10)
}

local candy_state = {}

local candy_spr = {}

function init_candies()
	for i,pos in ipairs(candy_pos) do
		local s = rand.int(0, 6)
		candy_spr[i] = 'candy' .. tostring(s)
	end
end

function eat_candy(pos)
	local i = 1
	while candy_pos[i].x ~= pos.x or candy_pos[i].y ~= pos.y do
		i = i + 1
		if i > #candy_pos then
			return false
		end
	end

	local res = (candy_state[i] == nil)

	candy_state[i] = states.time()

	return res
end

function draw_candies()
	local ts = states.time()

	for i,pos in ipairs(candy_pos) do
		local worldpos = map2world(pos)

		if candy_state[i] == nil then
			sprsheet.draw_centered(candy_spr[i], 3, worldpos, ts)
		else
			local t = (ts - candy_state[i]) / 7
			if t >= 1 then
				candy_state[i] = nil
			end

			if t > 0.8 then
				local tt = (t - 0.8) * 5
				local col = rgba(1, 1, 1, tt)
				local scale = lerp(3, 1, tt)

				sprsheet.draw_centered(candy_spr[i], 3, worldpos, ts, scale, col)
			end
		end
	end
end

-- player

local player = {}
player.__index = player

function player:new(id)
	local hues = {0, 0.5, 0.7, 0.3, 0.9, 0.1, 0.8};

	local obj = {
		dir = nil,
		sprite = 'box',
		col = to_rgba(hsva(hues[color_counter], 0.6, 0.9, 1)),
		candies = 0
	}

	color_counter = color_counter + 1
	if color_counter > #hues then
		color_counter = 1
	end

	setmetatable(obj, self)
	obj:randomize_pos()
	return obj
end

function player:randomize_pos()
	local x, y

	local positions = {
		vec2(2, 2),
		vec2(26, 2),
		vec2(2, 14),
		vec2(26, 14),
		vec2(6, 6),
		vec2(6, 10),
		vec2(22, 6),
		vec2(22, 10),
		vec2(12, 2),
		vec2(16, 2),
		vec2(12, 14),
		vec2(16, 14)
	}

	local i = rand.int(0, #positions)

	self.pos = positions[i+1]
end

function player:update()
	if self.dead then
		return
	end

	if self.t ~= nil then
		local tt = (states.time() - self.t) / move_len

		if tt >= 1 then
			self.pos = self.next_pos
			self.next_pos = nil
			self.t = nil
		end
	end

	if self.eaten_t then
		if states.time() - self.eaten_t > 1 then
			self.dead = true
		end
	end

	if self.is_berserk and self.next_pos then
		if eat_player(me, self.next_pos) then
			self.eating_t = states.time()
		end
	end

	if eat_candy(self.pos) then
		self.candies = self.candies + 1
		if self.candies >= 2 then
			self.candies = 0
			self.is_berserk = states.time()
		end
	end

	if self.is_berserk then
		if states.time() - self.is_berserk > 10 then
			self.is_berserk = nil
		end
	end

	if self.t == nil and self.dir ~= nil then
		local next_pos = self.pos + self.dir
		local nw_pos = map2world(next_pos)
		local next_rect = rect(
			nw_pos.x - 96,
			nw_pos.y - 96,
			nw_pos.x + 96,
			nw_pos.y + 96
		)
		if not tilemap.collide(game.map, next_rect) then
			self.next_pos = next_pos
			self.t = states.time()
		end
	end
end

function player:draw()
	if self.dead then
		return
	end

	local t = states.time()
	local frame = 0
	local pos = self.pos
	if self.t ~= nil then
		local tt = (t - self.t) / move_len
		pos = lerp(pos, self.next_pos, tt)
		frame = math.floor(math.fmod(t*10, 6))
	end

	local col = self.col
	if self.is_berserk then
		col = rgba(col.r, col.g, col.b, 0.5 + (math.sin(t*10) + 1) / 4)
	end

	local scale = 1

	if self.eating_t then
		local tt = (t - self.eating_t)
		if tt >= 1 then
			self.eating_t = nil
		end

		scale = 1 + math.sin(tt * math.pi) / 5
	end

	if self.eaten_t then
		local tt = clamp(0, 1, (t - self.eaten_t))
		local ttt = math.floor(tt * 7)
		scale = 1 - tt

		if math.fmod(ttt, 2) == 0 then
			col = rgba(col.r, col.g, col.b, 1 - tt)
		else
			col = rgba(col.r, col.g, col.b, 0)
		end
	end

	sprsheet.draw_anim_centered(self.sprite, frame, 4, map2world(pos), self.angle or 0,
		scale, col)
end

local players = {}

function eat_player(me, pos)
	for i,p in pairs(players) do
		if not p.dead and p ~= me then
			if pos.x == p.pos.x and pos.y == p.pos.y then
				if not p.eaten_t then
					p.eaten_t = states.time()
					return true
				end
			end
		end
	end

	return false
end

function game.join(id)
	print('player id '..tostring(id)..' joined')
	players[id] = player:new(id)
end

function game.leave(id)
	print('player id '..tostring(id)..' left')
	players[id] = nil
end

function game.control(id, msg)
	local p = players[id]

	if p then
		local dir = p.dir or vec2(0, 0)
		local d, s = msg:match('(%a):(%a)')

		local dirs =  {
			['l'] = vec2(-1, 0),
			['r'] = vec2(1, 0),
			['u'] = vec2(0, -1),
			['d'] = vec2(0, 1)
		}

		local angles = {
			['u'] = -math.pi/2,
			['d'] = math.pi/2,
			['r'] = 0,
			['l'] = math.pi
		}

		if s == 'd' then
			dir = dir + dirs[d]
			p.angle = angles[d]
		end

		if s == 'u' then
			dir = dir - dirs[d]
		end

		if length_sq(dir) > 0 then
			p.dir = dir
		else
			p.dir = nil
		end
	end
end

function game.init()
	aitvaras.lobby_addr = 'http://www.antrasekranas.lt'
	aitvaras.server_addr = 'http://89.249.93.114:8008'
	aitvaras.listening_port = 8008
	aitvaras.document_root = 'aitvaras_html/'
	aitvaras.join_cb = game.join
	aitvaras.leave_cb = game.leave
	aitvaras.input_cb = game.control

	aitvaras.init()

	game.map = tilemap.load(pre..'level1.btm')
	tilemap.set_camera(game.map, scr_size / 2, 1, 0)

	init_candies()
end

function game.close()
	tilemap.free(game.map)

	aitvaras.close()
end

function game.enter()
end

function game.leave()
end

function game.update()
	for id,player in pairs(players) do
		if player then
			player:update()
		end
	end

	return true
end

function game.render(t)
	local msg = 'antrasekranas.lt -> ' .. tostring(aitvaras.id())

	video.draw_text(fnt, 6, msg, vec2(90, 25), rgba(1, 1, 1, 1))

	sprsheet.draw('background', 0, vec2(0, 0))
	sprsheet.draw('wall', 5, vec2(0, 0))

	for id,player in pairs(players) do
		if player then
			player:draw()
		end
	end

	draw_candies()

	return true
end

return game
