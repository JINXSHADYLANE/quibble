local game = {}

tiles_w = 16
tiles_h = 12

local function map2world(p)
	return p * 64 + vec2(32, 32)
end

-- player

local player = {}
player.__index = player

function player:new(id)
	local obj = {}
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

function player:draw()
	sprsheet.draw_anim_centered('bug_red', 0, 4, map2world(self.pos))
end

local players = {}


function game.join(id)
	print('player id '..tostring(id)..' joined')
	players[id] = player:new(id)
	print(tostring(players[id].pos))
end

function game.leave(id)
	print('player id '..tostring(id)..' left')
	players[id] = nil
end

function game.control(id, msg)
	print(tostring(id).. ' -> '..tostring(msg))
end

function game.init()
	aitvaras.lobby_addr = 'http://89.249.93.114:80'
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
