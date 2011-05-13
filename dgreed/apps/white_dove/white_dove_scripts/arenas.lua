-- arenas manager

arenas_count = 9

empty = 0
room = 1
fountain = 2
corner = 3
ocean = 4
ocean2 = 5
potter_place = 6
door_to_b = 7
old_street = 8
cross = 9
kitchen = 10
backyard = 11
park = 12
pond = 13
street = 14
forest = 15
portal = 16

active_arena = room

fade_state = {
	fadein = 1,
	fadeout = 2,
	switch = 3,
	other = 0
}

fade_speed = 0.04

arena = {
	[1] = {
		top = {
			id = empty,
			pos = vec2()
		},
		left = {
			id = empty,
			pos = vec2()
		},
		bottom = {
			id = fountain,
			pos = vec2(170, 315)
		},
		right = {
			id = empty,
			pos = vec2()
		},

		--[[items = {
			[0] = rect(0, 0, 880, 400),
			[1] = rect(740, 415, 1024, 465),
			[2] = rect(947, 475, 1024, 666), 
			[3] = rect(990, 666, 1024, 768),
			[4] = rect(385, 425, 550, 583),
			[5] = rect(200, 425, 350, 590),
			[6] = rect(100, 445, 200, 540)
		},]]
		
		img = nil,
		tilemp = nil,
		char = 0 --barrel
	},

	[2] = {
		top = {
			id = corner,
			pos = vec2(screen.r/2, screen.b - boy.h - 20)
		},
		left = {
			id = room,
			pos = vec2(screen.r/2, screen.b - boy.h - 20)
		},
		bottom = {
			id = empty,
			pos = vec2()
		},
		right = {
			id = old_street,
			pos = vec2(screen.r/3, screen.b - boy.h - 20)
		},

		--[[items = {
			[0] = rect(670, 270, 1024, 410),
			[1] = rect(480, 220, 650, 310),
			[2] = rect(170, 190, 470, 220),
			[3] = rect(130, 220, 255, 345),
			[4] = rect(30, 330, 220, 415),
			[5] = rect(0, 430, 160, 460),
			[6] = rect(60, 460, 110, 580),
			[7] = rect(0, 495, 50, 768),
			[8] = rect(364, 532, 554, 600),
			[9] = rect(400, 470, 510, 550)
		},]]

		img = nil,
		tilemp = nil,
		char = girl

	},

	[3] = {
		top = {
			id = empty,
			pos = vec2()
		},
		left = {
			id = empty,
			pos = vec2()
		},
		bottom = {
			id = fountain,
			pos = vec2(362, 90)
		},
		right = {
			id = ocean,
			pos = vec2(0, screen.b - boy.h - 10)
		},
		
		--[[items = {
			[0] = rect(822, 405, 1024, 492),
			[1] = rect(930, 500, 970, 560),
			[2] = rect(280, 250, 1024, 360),
			[3] = rect(635, 380, 1024, 400),
			[4] = rect(815, 390, 1024, 420),
			[5] = rect(0, 210, 300, 380),
			[6] = rect(0, 380, 200, 415),
			[7] = rect(0, 420, 150, 450),
			[8] = rect(0, 450, 60, 500)
		},]]
		img = nil,
		tilemp = nil,
		char = math

	},

	[4] = {  --ocean
		top = {
			id = empty,
			pos = vec2()
		},
		left = {
			id = corner,
			pos = vec2(screen.r - boy.w - 20, screen.b - boy.h - 10)
		},
		bottom = {
			id = empty,
			pos = vec2()
		},
		right = {
			id = ocean2,
			pos = vec2(0, screen.b - boy.h - 10)  -- coords not yet set
		}, 

		--[[items = {
			[0] = rect(0, 225, 660, 390),
			[1] = rect(670, 220, 910, 330),
			[2] = rect(900, 210, 1024, 300)
		},]]
		img = nil,
		tilemp = nil,
		char = mage
	},
	
	[5] = {  --ocean2
		top = {
			id = empty,
			pos = vec2()
		},
		left = {
			id = ocean,
			pos = vec2(screen.r - boy.w - 20, screen.b - boy.h - 20) 
		},
		bottom = {
			id = empty,
			pos = vec2()
		},
		right = {
			id = empty,
			pos = vec2()
		}, 

		--[[items = {
			[0] = rect(0, 225, 660, 390),
		},]]
		img = nil,
		tilemp = nil,
		char = 0
	},
	
	[6] = {
		top = {
			id = empty,
			pos = vec2()
		},
		left = {
			id = cross,
			pos = vec2(screen.l + boy.w, screen.b + boy.h/3 - 30)
		},
		bottom = {
			id = empty,
			pos = vec2()
		},
		right = {
			id = empty,
			pos = vec2()
		}, 

		--[[items = {
			[0] = rect(0, 225, 660, 390),
		},]]
		img = nil,
		tilemp = nil,
		char = 0
	},	
	
	[7] = {
		top = {
			id = backyard,
			pos = vec2(screen.r/3, screen.b + boy.h/3 - 30)
		},
		left = {
			id = cross,
			pos = vec2(screen.l + boy.w, screen.b + boy.h/3 - 30)
		},
		bottom = {
			id = empty,
			pos = vec2()
		},
		right = {
			id = door_to_b,
			pos = vec2()
		}, 

		--[[items = {
			[0] = rect(0, 225, 660, 390),
		},]]
		img = nil,
		tilemp = nil,
		char = 0
	},
	
	[8] = {  -- old_street
		top = {
			id = empty,
			pos = vec2()
		},
		left = {
			id = empty,
			pos = vec2()
		},
		bottom = {
			id = fountain,
			pos = vec2(screen.r - boy.w, screen.b - boy.h - 50)
		},
		right = {
			id = cross,
			pos = vec2(0, screen.b - boy.h - 20)
		}, 

		--[[items = {
			[0] = rect(0, 1, 1, 2),
		},]]
		img = nil,
		tilemp = nil,
		char = 0
	},
	
	[9] = {  --cross
		top = {
			id = empty,
			pos = vec2()
		},
		left = {
			id = old_street,
			pos = vec2(screen.r - boy.w,  screen.b - boy.h - 20)
		},
		bottom = {
			id = empty,
			pos = vec2()
		},
		right = {
			id = empty,
			pos = vec2()
		}, 

		--[[items = {
			[0] = rect(0, 1, 1, 2),
		},]]
		img = nil,
		tilemp = nil,
		char = 0
	},
	
	
	color = rgba(1, 1, 1, 0),
	state = fade_state.other,
	switch_dir = direction.other
}

function arena.init()
	local i
	for i = 1, arenas_count do
		arena[i].img = tex.load(media..i..".png")
		arena[i].tilemp = tilemap.load(media..i..".btm")
		tilemap.set_camera(arena[i].tilemp, vec2(screen.r/2, screen.b/2))
	end
	boy.init()
	chars.init()

	active_arena = room
end

function arena.close()
	chars.close()
	local i
	for i = 1, arenas_count do
		tex.free(arena[i].img)
		tilemap.free(arena[i].tilemp)
	end
end

function arena.draw()
	video.draw_rect(arena[active_arena].img, 0, screen, screen, 0)
	if arena[active_arena].char ~= 0 then
		chars.draw(arena[active_arena].char)
	end
	boy.draw()

	if arena.state ~= fade_state.other then
		video.draw_rect(black_tex, 15, rect(0,0,32,32), screen, 0, arena.color)
	end
end

function arena.make_active(dir)
	if dir == direction.down then
		local arena_temp = arena[active_arena].bottom
		if arena_temp.id ~= empty then
			boy.pos = vec2(arena_temp.pos)
			boy.set_col_rect()
			active_arena = arena_temp.id
		end
	elseif dir == direction.up then
		local arena_temp = arena[active_arena].top
		if arena_temp.id ~= empty then
			boy.pos = vec2(arena_temp.pos)
			boy.set_col_rect()
			active_arena = arena_temp.id
		end
	elseif dir == direction.left then
		local arena_temp = arena[active_arena].left
		if arena_temp.id ~= empty then
			boy.pos = vec2(arena_temp.pos)
			boy.set_col_rect()
			active_arena = arena_temp.id
		end
	elseif dir == direction.right then
		local arena_temp = arena[active_arena].right
		if arena_temp.id ~= empty then
			boy.pos = vec2(arena_temp.pos)
			boy.set_col_rect()
			active_arena = arena_temp.id
		end
	end
end

function arena.chars_active()
	if arena[active_arena].char ~= 0 then
		local char_rect = rect(chars[arena[active_arena].char].pos)
		if rect_rect_collision(boy.col_rect, char_rect) then
			dialog_q = dialog_quarter()
			return true
		end
	end
	return false
end

function arena.chars_collide()
	if arena[active_arena].char ~= 0 then
		local char_rect = rect(chars[arena[active_arena].char].pos)
		char_rect.t = char_rect.t + 20
		char_rect.l = char_rect.l + 20
		char_rect.b = char_rect.b - 20
		char_rect.r = char_rect.r - 20

		if rect_rect_collision(boy.col_rect, char_rect) then
			return true
		end
	end
	return false
end

function arena.get_neighbour(dir)
	if dir == direction.left then
		return arena[active_arena].left.id
	elseif dir == direction.right then
		return arena[active_arena].right.id
	elseif dir == direction.up then
		return arena[active_arena].top.id
	elseif dir == direction.down then
		return arena[active_arena].bottom.id
	else 
		return empty
	end

end

function arena.switch()
	local neighbour = arena.get_neighbour(arena.switch_dir)

	if arena.state == fade_state.fadeout and neighbour ~= empty then
		arena.color.a = arena.color.a + fade_speed
		if arena.color.a > 1.0 then 
			arena.color.a = 1
			arena.state = fade_state.switch
		end
	elseif arena.state == fade_state.fadein then
		arena.color.a = arena.color.a - fade_speed
		if arena.color.a < 0.0 then
			arena.color.a = 0
			arena.state = fade_state.other
		end
	elseif arena.state == fade_state.switch then
		arena.make_active(arena.switch_dir)
		arena.state = fade_state.fadein
	end

end

function arena.screen_update()
	-- check if character is near border and x is pressed
	if (boy.pos.y + boy.h + 5 >= screen.b) and key.down(key.b) then
			arena.switch_dir = direction.down
			arena.state = fade_state.fadeout
	elseif (boy.pos.y <= 25) and key.down(key.b) then
		arena.switch_dir = direction.up
		arena.state = fade_state.fadeout
	elseif (boy.pos.x + boy.w + 5 >= screen.r) and key.down(key.b) then
		arena.switch_dir = direction.right
		arena.state = fade_state.fadeout
	elseif (boy.pos.x <= 5) and key.down(key.b) then
		arena.switch_dir = direction.left
		arena.state = fade_state.fadeout
	end

	local door = rect(40, 440, 130, 645)
	if active_arena == fountain and rect_rect_collision(boy.col_rect, door)
		and key.down(key.b) then
		arena.switch_dir = direction.left
		arena.state = fade_state.fadeout
	end

end

function arena.update()
	arena.screen_update()
	arena.switch()
	boy.update()
end

