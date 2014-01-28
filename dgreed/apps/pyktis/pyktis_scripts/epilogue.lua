local epilogue = {}

function epilogue.update()
	if char.pressed(' ') then
		return false
	end
	return true
end

local function quote1(black, grey)
	vfont.select(fnt, 48)
	vfont.draw('The first principle is that', 15, vec2(60, 80), black)
	vfont.draw('you must not fool yourself,', 15, vec2(60, 130), black)
	vfont.draw('and you are the easiest', 15, vec2(60, 180), black)
	vfont.draw('person to fool.', 15, vec2(60, 230), black)
	vfont.select(fnt, 32)
	vfont.draw('Richard P. Feynman', 15, vec2(280, 300), grey)
	vfont.select(fnt, 40)
end

local function quote2(black, grey)
	vfont.select(fnt, 48)
	vfont.draw('Principles', 15, vec2(60, 80), black)
	vfont.draw('You can\'t say A is made of B', 15, vec2(60, 130), black)
	vfont.draw('or vice versa.', 15, vec2(60, 180), black)
	vfont.draw('All mass is interaction.', 15, vec2(60, 230), black)
	vfont.select(fnt, 32)
	vfont.draw('Richard P. Feynman', 15, vec2(280, 300), grey)
	vfont.select(fnt, 40)
end

local function quote3(black, grey)
	vfont.select(fnt, 48)
	vfont.draw('What I cannot create,', 15, vec2(30, 80), black)
	vfont.draw('I do not understand.', 15, vec2(30, 130), black)
	vfont.draw('Know how to solve every problem', 15, vec2(30, 200), black)
	vfont.draw('that has been solved.', 15, vec2(30, 250), black)
	vfont.select(fnt, 32)
	vfont.draw('Richard P. Feynman', 15, vec2(280, 340), grey)
	vfont.select(fnt, 40)
end

local quote = nil

function epilogue.render(t)
	local alpha = 1 - math.abs(t)
	local white = rgba(1, 1, 1, alpha)
	local black = rgba(0, 0, 0, alpha)
	local grey = rgba(0.5, 0.5, 0.5, alpha)
	sprsheet.draw('empty', 0, scr_rect, white)

	if not quote then
		quote = rand.int(1, 4)
	end

	if quote == 1 then
		quote1(black, grey)
	elseif quote == 2 then
		quote2(black, grey)
	else
		quote3(black, grey)
	end

	return true
end

return epilogue 
