-- arenas manager

arenas_count = 14

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

		n = 0,
		doors = {},
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
		
		n = 1,
		doors = {
			[1] = {
				d = rect(40, 440, 130, 645),
				dir = direction.left
		}	},
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
			pos = vec2(362, 70)
		},
		right = {
			id = ocean,
			pos = vec2(0, screen.b - boy.h - 10)
		},
		
		n = 0,
		doors = {},
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
			pos = vec2(5, screen.b - boy.h - 10) 
		}, 

		n = 0,
		doors = {},
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
			id = cross,
			pos = vec2(419, 232 - boy.h)
		},
		right = {
			id = empty,
			pos = vec2()
		}, 
		
		n = 0,
		doors = {},
		img = nil,
		tilemp = nil,
		char = 0
	},
	
	[6] = {  --potter_place
		top = {
			id = empty,
			pos = vec2()
		},
		left = {
			id = empty,
			pos = vec2()
		},
		bottom = {
			id = street,
			pos = vec2(820, 600 - boy.h)
		},
		right = {
			id = door_to_b,
			pos = vec2(screen.r/2 - boy.w, 700 - boy.h)
		}, 
		n = 1,
		doors = {
			[1] = {
				dir = direction.right,
				d = rect(667, 439, 704, 511)
			}
		},
		img = nil,
		tilemp = nil,
		char = 0
	},	
	
	[7] = {  -- door
		top = {
			id = backyard,
			pos = vec2(4*screen.r/3 - boy.w, screen.b - boy.h/3 - 30)
		},
		left = {
			id = empty,
			pos = vec2()
		},
		bottom = {
			id = potter_place,
			pos = vec2(606, 461)
		},
		right = {
			id = empty,
			pos = vec2()
		}, 

		n = 1,
		doors = {
			[1] = {
				d = rect(536, 378, 682, 440),
				dir = direction.top
			}
		},
		img = nil,
		tilemp = nil,
		char = 0
	},
	
	[8] = {  -- old_street
		top = {
			id = kitchen,
			pos = vec2(518 , 732 - boy.h)
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
		
		n = 1,
		doors = {
			[1] = {
				d = rect(391, 480, 496, 596),
				dir = direction.up
			}
		},
		img = nil,
		tilemp = nil,
		char = 0
	},
	
	[9] = {  --cross
		top = {
			id = park,
			pos = vec2(screen.r/2 - boy.w, 700 - boy.h)
		},
		left = {
			id = empty,
			pos = vec2()
		},
		bottom = {
			id = old_street,
			pos = vec2(screen.r - boy.w,  screen.b - boy.h - 20)
		},
		right = {
			id = street,
			pos = vec2(screen.r/2 - boy.w, 700 - boy.h)
		}, 

		n = 2,
		doors = {
			[1] = {
				d = rect(802, 102, 972, 272),
				dir = direction.top
			},
			[2] = {
				d = rect(983, 397, 1024, 577),
				dir = direction.right
			}
			
		},
		img = nil,
		tilemp = nil,
		char = 0
	},
	
	[10] = {  -- kitchen
		top = {
			id = empty,
			pos = vec2()
		},
		left = {
			id = empty,
			pos = vec2()
		},
		bottom = {
			id = old_street,
			pos = vec2(429, 606 - boy.h)
		},
		right = {
			id = empty,
			pos = vec2()
		}, 

		n = 0,
		doors = {},
		img = nil,
		tilemp = nil,
		char = mage
	},	
	
	[11] = {  -- backyard
		top = {
			id = empty,
			pos = vec2()
		},
		left = {
			id = empty,
			pos = vec2()
		},
		bottom = {
			id = door_to_b,
			pos = vec2(screen.r/5 - boy.w, 700 - boy.h)
		},
		right = {
			id = empty,
			pos = vec2()
		}, 

		n = 0,
		doors = {},
		img = nil,
		tilemp = nil,
		char = 0
	},	
	
	[12] = {  -- park
		top = {
			id = pond,
			pos = vec2(3*screen.r/4, 700 - boy.h)
		},
		left = {
			id = empty,
			pos = vec2()
		},
		bottom = {
			id = cross,
			pos = vec2(837, 189 - boy.h)
		},
		right = {
			id = empty,
			pos = vec2()
		}, 

		n = 0,
		doors = {},
		img = nil,
		tilemp = nil,
		char = 0
	},

	[13] = {  -- pond
		top = {
			id = empty,
			pos = vec2()
		},
		left = {
			id = empty,
			pos = vec2()
		},
		bottom = {
			id = park,
			pos = vec2(screen.r/2, boy.h + 3)
		},
		right = {
			id = empty,
			pos = vec2()
		}, 

		n = 0,
		doors = {},
		img = nil,
		tilemp = nil,
		char = 0
	},	
	
	[14] = {  -- street
		top = {
			id = empty,
			pos = vec2()
		},
		left = {
			id = empty,
			pos = vec2()
		},
		bottom = {
			id = cross,
			pos = vec2(914, 485 - boy.h)
		},
		right = {
			id = potter_place,
			pos = vec2(screen.r/2 - boy.w, 700 - boy.h)
		}, 

		n = 1,
		doors = {
			[1] = {
				dir = direction.right,
				d = rect(903, 561, 1007, 650)
			}
		},
		img = nil,
		tilemp = nil,
		char = 0
	},	
	
	color = rgba(1, 1, 1, 0),
	state = fade_state.other,
	switch_dir = direction.other
}

function arena.init()
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

-- Arena change is made here
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

-- Check boy collisions with other characters
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

-- Check if boy is near other characters
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

-- Select other (neighbour) arena
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
	-- check if boy is going to the other arena
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
	else
		for i = 1, arena[active_arena].n do
			if (rect_rect_collision(boy.col_rect, arena[active_arena].doors[i].d) and key.down(key.b)) then
				arena.switch_dir = arena[active_arena].doors[i].dir
				arena.state = fade_state.fadeout
			end
		end
	end

	-- check if boy is going home 
	--[[
	local door = rect(40, 440, 130, 645)
	if active_arena == fountain and rect_rect_collision(boy.col_rect, door)
		and key.down(key.b) then
		arena.switch_dir = direction.left
		arena.state = fade_state.fadeout
	end]]


	-- show 'x' icon
	if arena.chars_active() or items_collide() then
		if key.down(key.b) then 
			have_book = true 
			return
		end
		
		local icon_pos = vec2(boy.pos.x, boy.pos.y - 100)
		video.draw_rect(active_icon_tex, 5, icon_pos)
	end
end

function arena.update()
	arena.screen_update()
	arena.switch()
	boy.update()
end
