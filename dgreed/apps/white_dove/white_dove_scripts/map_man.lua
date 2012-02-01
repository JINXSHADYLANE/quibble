-- Maps manager file

active_arena = 0

function arena.init()
	for id = 1, arenas_count do
		arena[id].img = tex.load(arena[id].img_name)
	end

	-- add boy.init()
	chars.init()
	active_arena = init_arena
end

function arena.close()
	chars.close()

	for id = 1, arenas_count do
		tex.free(arena[id].img)
	end
end

function arena.draw()
	video.draw_rect(arena[active_arena].img, 0, screen, screen, 0)
	if arena[active_arena].char ~= other then
		chars.draw(arena[active_arena].char)
	end

	boy.draw()

	if arena.state ~= fade_state.other then
		video.draw_rect(black_tex, 15, rect(0,0,32,32), screen, 0, arena.color)
	end
end

function arena.make_active(dir)
	if dir == direction.down then
		if arena.bottom_id ~= empty then
			active_arena = arena.bottom_id
			boy.pos = vec2(arena[active_arena].top_pos)
			boy.set_set_col_rect()
		end
	elseif dir == direction.up then
		if arena.top_id ~= empty then
			active_arena = arena.top_id
			boy.pos = vec2(arena[active_arena].bottom_pos)
			boy.set_col_rect()
		end
	elseif dir == direction.left then
		if arena.left_id ~= empty then
			active_arena = arena[active_arena].left_id
			boy.pos = vec2(arena[active_arena].right_pos)
			boy.set_col_rect()
		end
	elseif dir == direction.right then
		if arena.right_id ~= empty then
			active_arena = arena[active_arena].right_id
			boy.pos = vec2(arena[active_arena].left_pos)
			boy.set_col_rect()
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
		return arena[active_arena].left_id
	elseif dir == direction.right then
		return arena[active_arena].right_id
	elseif dir == direction.up then
		return arena[active_arena].top_id
	elseif dir == direction.down then
		return arena[active_arena].bottom_id
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

	arena.check_exits()
end

function arena.update()
	arena.screen_update()
	arena.switch()
	boy.update()
end

