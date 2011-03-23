-- Magic book is written here

page = {
	[1] = {
		left = "Magic book, which shows you the way to unseen world",
		right = "Press left or right arrow to search through pages"
	},
	[2] = {
		left = "",
		right = "You sould look for a red mage in this game"
	},
}

book_pages = 2
active_page = 1
have_book = false

book_src = rect(0, 0, 1024, 768)
left_rect = rect(130, 100, 422, 668)
right_rect = rect(587, 100, 919, 668)

function book_draw()
	local start_id, end_id = 1, 1
	local w, h = font.size(fnt, "a")
	local text_pos = vec2(left_rect.l + (left_rect.r - left_rect.l) / 2, 
		left_rect.t + h/2)
	local text_left = page[active_page].left
	local text_right = page[active_page].right

	video.draw_rect(book_tex, 5, book_src, screen)

	while end_id do
		end_temp = string.find(text_left, " ", end_id+1)
		if end_temp then
			w, h = font.size(fnt, string.sub(text_left, start_id, end_temp-1))
			if w > left_rect.r-left_rect.l then
				video.draw_text_centered(fnt, 6, string.sub(text_left, 
					start_id, end_id-1), text_pos)
				start_id = end_id + 1
				text_pos.y = text_pos.y + h
			end
		end
		end_id = end_temp
	end
	video.draw_text_centered(fnt, 6, string.sub(text_left, start_id, n), text_pos)

	start_id, end_id = 1, 1
	local text_pos = vec2(right_rect.l + (right_rect.r - right_rect.l) / 2, 
		right_rect.t + h/2)

	while end_id do
		end_temp = string.find(text_right, " ", end_id+1)
		if end_temp then
			w, h = font.size(fnt, string.sub(text_right, start_id, end_temp-1))
			if w > left_rect.r-left_rect.l then
				video.draw_text_centered(fnt, 6, string.sub(text_right, start_id,
					end_id-1), text_pos)
				start_id = end_id + 1
				text_pos.y = text_pos.y + h
			end
		end
		end_id = end_temp
	end
	video.draw_text_centered(fnt, 6, string.sub(text_right, start_id, n), text_pos)

end

function book_update()
	if state == action.book and key.down(key._left) then
		if active_page > 1 then
			active_page = active_page - 1
		end
	elseif state == action.book and key.down(key._right) then
		if active_page < book_pages then
			active_page = active_page + 1
		end
	end
end

