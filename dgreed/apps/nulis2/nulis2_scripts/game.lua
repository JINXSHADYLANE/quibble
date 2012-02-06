module(..., package.seeall)

require 'tutorials'
require 'editor'
require 'menu'

function init()
	clevels.reset(pre..levels_file)
	tutorials.init()
	csim.init(
		scr_size.x, scr_size.y,
		sim_size.x, sim_size.y
	)
end

function close()
	tutorials.close()
	csim.close()
	clevels.close()
end

function enter()
	csim.enter()
end

function leave()
	csim.leave()
end

function update()
	if not first_frame then
		csim.reset(clevels.first_unsolved())
		first_frame = true
	end

	menu.update_orientation()
	csim.update()
	return true
end

function render(t)
	if t == 0 then
		if sim_size.x < scr_size.x then
			editor.render_iphone_screen()
		end

		tutorials.render(csim.level())
		csim.render(true)
	end
	return true
end
