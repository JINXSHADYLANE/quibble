-- White Dove

media = "white_dove_assets/"
script = "white_dove_scripts/"

dofile(script..'utils.lua')
dofile(script..'boy.lua')
dofile(script..'book.lua')
dofile(script..'chars.lua')
dofile(script..'arenas.lua')
dofile(script..'gui.lua')

state = action.move
dialog_text = ""
dialog_q = q1

wall = {
	l = rect(0, 0, 0, screen.b),
	t = rect(0, 0, screen.r, 0),
	b = rect(0, screen.b, screen.r, screen.b),
	r = rect(screen.r, 0, screen.r, screen.b)
}

function init()
	video.init(screen.r, screen.b, 'White Dove')
	menu.init()
	arena.init()
	
	atlas = tex.load(media.."boy_moves.png")
	border_tex = tex.load(media.."remelis.png")
	book_tex = tex.load(media.."knyga.png")
	black_tex = tex.load(media.."black.png")
	active_icon_tex = tex.load(media.."active_icon.png")
	backgr = tex.load(media.."background.png")

	fnt = font.load(media.."lucida_grande_60px.bft", 0.4)
end


function close()
	font.free(fnt)

	tex.free(active_icon_tex)
	tex.free(black_tex)
	tex.free(book_tex)
	tex.free(border_tex)
	tex.free(atlas)
	
	arena.close()
	menu.close()
	video.close()
end

function handle_keyboard()
	-- handle keyboard keys
	if key.down(key.a) and have_book then
		if state == action.move then
			state = action.book
		elseif state == action.book then
			state = action.move
		end
	end

	if key.down(key.b) then
		if state == action.move and (arena.chars_active() or items_collide()) then
			state = action.dialog
			dialog_select_text()
		elseif state == action.dialog then
			if not text_id then
				if not dialog_text[text_part + 1] then state = action.move
				else text_part = text_part + 1
				end

				return
			end
	
			dialog_text[text_part][2] = string.sub(dialog_text[text_part][2], text_id)
		end
	end
end

function run_game()
	if menu.state == main_state.menu or menu.state == main_state.pause_menu then
		if not menu.draw() then return false end
	elseif menu.state == main_state.game then
		if play() then 
			menu.state = main_state.pause_menu
			menu.pause = true
		end
	end

	return true
end

function play()
	handle_keyboard()

	-- handle state methods
	if state == action.move then
		arena.update()
	elseif state == action.dialog then
		draw_dialog(dialog_text[text_part], dialog_q)
	elseif state == action.book then
		book_draw()
		book_update()
	end

	arena.draw()
	return key.pressed(key.quit)
end

function main()
	init()
	repeat
		run = run_game()
	until not video.present() or not run
	close()
end

main()

