-- White Dove

media = "white_dove_assets/"
script = "white_dove_scripts/"

dofile(script..'utils.lua')
dofile(script..'boy.lua')
dofile(script..'book.lua')
dofile(script..'chars.lua')
--dofile(script..'map.lua') -- current world map
--dofile(script..'map_man.lua')
dofile(script..'arenas.lua')

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
	
	arena.init()
	
	atlas = tex.load(media.."boy_moves.png")
	border_tex = tex.load(media.."remelis.png")
	book_tex = tex.load(media.."knyga.png")
	black_tex = tex.load(media.."black.png")
	active_icon_tex = tex.load(media.."active_icon.png")

	fnt = font.load(media.."lucida_grande_60px.bft", 0.4)

	--boy.init()
end


function close()
	font.free(fnt)

	tex.free(active_icon_tex)
	tex.free(black_tex)
	tex.free(book_tex)
	tex.free(border_tex)
	tex.free(atlas)
--	tex.free(pic)
	arena.close()
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
		elseif state == action.dialog then
			-- change to waiting for x to continue text
			state = action.move
		end
	end
end

function play()

	handle_keyboard()

	-- handle state methods
	if state == action.move then
		arena.update()
	elseif state == action.dialog then
		dialog_select_text()
		draw_dialog(dialog_text[1], dialog_q)
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
		exit = play()
	until not video.present() or exit
	close()
end

main()

