module(..., package.seeall)

require 'tutorials'
require 'editor'

function init()
	clevels.reset(pre..levels_file)
	tutorials.init()
	csim.init(
		scr_size.x, scr_size.y,
		sim_size.x, sim_size.y
	)
	csim.reset('l1')
end

function close()
	tutorials.close()
	csim.close()
	clevels.close()
end

function enter()
end

function leave()
end

function update()
	csim.update()
	return true
end

function render()
	if sim_size.x < scr_size.x then
		editor.render_iphone_screen()
	end

	tutorials.render(csim.level())
	csim.render(true)
	return true
end
