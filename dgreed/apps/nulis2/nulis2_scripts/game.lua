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
end

function close()
	tutorials.close()
	csim.close()
	clevels.close()
end

function enter()
	if not first_enter then
		csim.reset('l1')
		first_enter = true
	end
end

function leave()
end

function update()
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
