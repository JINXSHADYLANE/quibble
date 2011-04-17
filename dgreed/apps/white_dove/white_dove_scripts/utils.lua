-- Common functions to use in White Dove

screen = rect(0, 0, 1024, 768)

direction = {
	up = 1,
	left = 2,
	down = 3,
	right = 4,
	other = 0
}

action = {
	move = 1,
	book = 2,
	dialog = 3
}

fade_speed = 0.04
fade_state = {
	fadein = 1,
	fadeout = 2,
	switch = 3,
	other = 0
}

q1 = rect(20, 20, screen.r/2-20, screen.b/2-20)
q2 = rect(screen.r/2+20, 20, screen.r-20, screen.b/2-20)
q3 = rect(20, screen.b/2+20, screen.r/2-20, screen.b-20)
q4 = rect(screen.r/2+20, screen.b/2+20, screen.r-20, screen.b-20)

function dialog_quarter()
	if not rect_rect_collision(boy.col_rect, q1) and
		not rect_rect_collision(chars[arena[active_arena].char].pos, q1) then
		return q1
	elseif not rect_rect_collision(boy.col_rect, q2) and
		not rect_rect_collision(chars[arena[active_arena].char].pos, q2) then
		return q2
	elseif not rect_rect_collision(boy.col_rect, q3) and
		not rect_rect_collision(chars[arena[active_arena].char].pos, q3) then
		return q3
	elseif not rect_rect_collision(boy.col_rect, q4) and
		not rect_rect_collision(chars[arena[active_arena].char].pos, q4) then
		return q4
	end
	return q1
end

function draw_dialog(text, pos_rect)
	local shift = 50
	local start_id, end_id = 1, 1
	local w, h = font.size(fnt, "a")
	local text_pos = vec2(pos_rect.l + shift, pos_rect.t + shift)
	local tex_src = rect(0, 0, 512, 384)

	video.draw_rect(border_tex, 3, tex_src, pos_rect)

	while end_id do
		end_temp = string.find(text, " ", end_id+1)
	
		if end_temp then
			w, h = font.size(fnt, string.sub(text, start_id, end_temp-1))
			if w > (pos_rect.r - pos_rect.l - 2*shift) then
				video.draw_text(fnt, 4, string.sub(text, start_id, end_id-1), 
					text_pos)
				start_id = end_id + 1
				text_pos.y = text_pos.y + h
			end
		end

		end_id = end_temp
	end

	video.draw_text(fnt, 4, string.sub(text, start_id, n), text_pos)
end

