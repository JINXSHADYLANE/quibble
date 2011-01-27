-- Main character is described here

boy = {
	src = {
		-- numbers represent direction from utils.lua
		[3] = {
			-- numbers represents motion number
			[1] = rect(0, 0, 91, 273),
			[2] = rect(91, 0, 91*2, 273),
			[3] = rect(91*2, 0, 91*3, 273),
			[4] = rect(91, 0, 91*2, 273)
		},
		[1] = {
			[1] = rect(91*3, 0, 91*4, 273),
			[2] = rect(91*4, 0, 91*5, 273),
			[3] = rect(91*5, 0, 91*6, 273),
			[4] = rect(91*4, 0, 91*5, 273)

		},
		[4] = {
			[1] = rect(91*6, 0, 91*7, 273),
			[2] = rect(91*7, 0, 91*8, 273),
			[3] = rect(2+91*8, 0, 2+91*9, 273),
			[4] = rect(91*7, 0, 91*8, 273)

		},
		[2] = {
			[1] = rect(0, 273, 91, 273*2),
			[2] = rect(91, 273, 91*2, 273*2),
			[3] = rect(91*2, 273, 91*3, 273*2),
			[4] = rect(91, 273, 91*2, 273*2)
		}
	},
	active_src = vec2(), -- give direction and picture number
	pos = vec2(), -- top left corner
	col_rect = rect(), -- rectangle for collision checking

	size = 1,
	speed = 0.7,
	dx = 3,
	w = 91,
	h = 273
}

function boy.set_col_rect()
	boy.col_rect = rect(boy.pos.x - 3, boy.pos.y + 197, boy.pos.x + boy.w + 3,
		boy.pos.y + boy.h + 3)
end

function boy.init()
	boy.active_src = vec2(direction.down, 1)
	boy.pos = vec2(10, 450)

	boy.set_col_rect()

	move_dt = 0
end

function boy.draw()
	video.draw_rect(atlas, 1, boy.src[boy.active_src.x][boy.active_src.y], 
		boy.pos, 0)
end

function boy.collide()
	local boy_pos = rect(boy.pos.x, boy.pos.y, boy.pos.x+boy.w, boy.pos.y+boy.h)

	-- collisions with screen bounds
	if rect_rect_collision(boy_pos, wall.l) or
		rect_rect_collision(boy_pos, wall.t) or
		rect_rect_collision(boy_pos, wall.b) or
		rect_rect_collision(boy_pos, wall.r) or 
		arena.chars_collide() then
		return true
	end

	for id, item in pairs(arena[active_arena].items) do
		if rect_rect_collision(boy.col_rect, item) then
			return true
		end
	end

	return false
	-- ToDo: add collisions to things
end

function boy.update_pos()
	if (key.pressed(key._up) or key.down(key._up)) then
		boy.pos.y = boy.pos.y - boy.dx
		boy.col_rect.t = boy.col_rect.t - boy.dx
		boy.col_rect.b = boy.col_rect.b - boy.dx
		if boy.collide() then
			boy.pos.y = boy.pos.y + boy.dx
			boy.col_rect.t = boy.col_rect.t + boy.dx
			boy.col_rect.b = boy.col_rect.b + boy.dx
		end
	end
	if (key.pressed(key._down) or key.down(key._down)) then
		boy.pos.y = boy.pos.y + boy.dx
		boy.col_rect.t = boy.col_rect.t + boy.dx
		boy.col_rect.b = boy.col_rect.b + boy.dx
		if boy.collide() then
			boy.pos.y = boy.pos.y - boy.dx
			boy.col_rect.t = boy.col_rect.t - boy.dx
			boy.col_rect.b = boy.col_rect.b - boy.dx
		end
	end
	if (key.pressed(key._left) or key.down(key._left)) then
		boy.pos.x = boy.pos.x - boy.dx
		boy.col_rect.l = boy.col_rect.l - boy.dx
		boy.col_rect.r = boy.col_rect.r - boy.dx
		if boy.collide() then
			boy.pos.x = boy.pos.x + boy.dx
			boy.col_rect.l = boy.col_rect.l + boy.dx
			boy.col_rect.r = boy.col_rect.r + boy.dx
		end
	end
	if (key.pressed(key._right) or key.down(key._right)) then
		boy.pos.x = boy.pos.x + boy.dx
		boy.col_rect.l = boy.col_rect.l + boy.dx
		boy.col_rect.r = boy.col_rect.r + boy.dx
		if boy.collide() then
			boy.pos.x = boy.pos.x - boy.dx
			boy.col_rect.l = boy.col_rect.l - boy.dx
			boy.col_rect.r = boy.col_rect.r - boy.dx
		end
	end
end

function boy.update_move()
	if move_dt > 170 then
		if key.pressed(key._up) then
			if (boy.active_src.x == direction.up) then
				boy.active_src.y = boy.active_src.y + 1
				if boy.active_src.y > 4 then
					boy.active_src.y = 1
				end
			else 
				boy.active_src = vec2(direction.up, 1)
			end
		elseif key.pressed(key._down) then
			if (boy.active_src.x == direction.down) then
				boy.active_src.y = boy.active_src.y + 1
				if boy.active_src.y > 4 then
					boy.active_src.y = 1
				end
			else 
				boy.active_src = vec2(direction.down, 1)
			end
		elseif key.pressed(key._left) then
			if (boy.active_src.x == direction.left) then
				boy.active_src.y = boy.active_src.y + 1
				if boy.active_src.y > 4 then
					boy.active_src.y = 1
				end
			else 
				boy.active_src = vec2(direction.left, 1)
			end	
		elseif key.pressed(key._right) then
			if (boy.active_src.x == direction.right) then
				boy.active_src.y = boy.active_src.y + 1
				if boy.active_src.y > 4 then
					boy.active_src.y = 1
				end
			else 
				boy.active_src = vec2(direction.right, 1)
			end
		end
		move_dt = 0
	end

end

function boy.update()
	move_dt = move_dt + time.dt() 
	boy.update_move()
	boy.update_pos()
end

