
function speak()
	if active_arena == room then
		local book_shelf = rect(930, 310, 1024, 768)
		if not have_book and rect_rect_collision(book_shelf, boy.col_rect) then
			have_book = true
			doalog_text = "You have found an old book... "
			dialog_q = q1
			return true
		end
	end
	return false

	if active_arena == fountain then
		if key.down(key.b) and arena.chars_active() then
			dialog = 
end
