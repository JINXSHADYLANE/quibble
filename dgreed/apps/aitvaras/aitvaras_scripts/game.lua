local game = {}

colour_counter = 1
move_len = 0.5

tiles_w = 16
tiles_h = 12

sprs = {'bug_red', 'bug_blue', 'bug_green', 'bug_yellow'}

local function map2world(p)
	return p * 64 + vec2(32, 32)
end

-- player

local player = {}
player.__index = player

function player:new(id)
	local obj = {dir = nil, sprite = sprs[colour_counter]}
	colour_counter = math.fmod(colour_counter+1, #sprs)
	setmetatable(obj, self)
	obj:randomize_pos()
	return obj
end

function player:randomize_pos()
	local x, y
	
	repeat
		x, y = rand.int(0, tiles_w+1), rand.int(0, tiles_h+1)
	until not tilemap.collide(game.map, map2world(vec2(x, y)))

	self.pos = vec2(x, y)
end

function player:update()
	if self.t ~= nil then
		local tt = (states.time() - self.t) / move_len

		if tt >= 1 then
			self.pos = self.next_pos
			self.next_pos = nil
			self.t = nil
		end
	end

	if self.t == nil and self.dir ~= nil then
		local next_pos = self.pos + self.dir
		if not tilemap.collide(game.map, map2world(next_pos)) then
			self.next_pos = next_pos
			self.t = states.time()
		end
	end
end

function player:draw()
	local frame = 0
	local pos = self.pos
	if self.t ~= nil then
		local t = states.time()
		local tt = (t - self.t) / move_len
		pos = lerp(pos, self.next_pos, tt)
		frame = math.floor(math.fmod(t*6, 2))
	end

	sprsheet.draw_anim_centered(self.sprite, frame, 4, map2world(pos), self.angle or 0)
end

local players = {}


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
			['l'] = -math.pi/2,
			['r'] = math.pi/2,
			['u'] = 0,
			['d'] = math.pi
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
	video.draw_text(fnt, 15, tostring(aitvaras.id()), vec2(10, 690), rgba(1, 1, 1, 1))

	tilemap.render(game.map, rect(0, 0, scr_size.x, scr_size.y))

	for id,player in pairs(players) do
		if player then
			player:draw()
		end
	end

	return true
end

return game
