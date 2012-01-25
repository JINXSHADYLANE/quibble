module(..., package.seeall)

require 'tutorials'

function init()
	clevels.reset(pre.."levels.mml")
	tutorials.init()
	csim.init(scr_size.x, scr_size.y)
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
	tutorials.render(csim.level())
	csim.render(true)
	return true
end
