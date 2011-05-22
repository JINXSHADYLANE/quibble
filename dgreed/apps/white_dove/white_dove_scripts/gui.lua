
-- Here goes everything about GUI

menu = {
	state = main_state.menu,
	pause = false,
	options = false
}

function menu.init()
	menu.style = gui.default_style(media)
	gui.init(menu.style)
end

function menu.close()
	gui.close()
end

function menu.set_pause()
	play_name = "BACK TO GAME"
	exit_name = "EXIT"

	play_pos = vec2(376, 200)
	exit_pos = vec2(376, 280)
end

function menu.set_options()
	title_name = "    Options"
	exit_name = "BACK"
end

function menu.draw()
	video.draw_rect(backgr, 0, screen, screen, 0)
	
	title_name = "*** White Dove ***"
	play_name = "PLAY"
	options_name = "OPTIONS"
	exit_name = "EXIT"

	title_pos = vec2(350, 100)
	play_pos = vec2(549, 200)
	snd_slider_pos = vec2(376, 240)
	options_pos = vec2(376, 240)
	exit_pos = vec2(203, 280)

	if menu.pause then menu.set_pause() end
	if menu.options then menu.set_options() end

	gui.label(title_pos, title_name)

	if menu.options then
		gui.slider(snd_slider_pos)
	else
		if gui.button(play_pos, play_name) then	menu.state = main_state.game end
		if gui.button(options_pos, options_name) then menu.options = true end
	end

	if gui.button(exit_pos, exit_name) then
		if menu.options then menu.options = false
		else return false end
	end

	return true
end
