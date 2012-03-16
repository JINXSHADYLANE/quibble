local game = {}

function game.join(id)
	print('player id '..tostring(id)..' joined')
end

function game.leave(id)
	print('player id '..tostring(id)..' left')
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
end

function game.close()
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
	video.draw_text(fnt, 1, tostring(aitvaras.id()), vec2(10, 690), rgba(1, 1, 1, 1))

	return true
end

return game
