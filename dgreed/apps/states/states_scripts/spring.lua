module(..., package.seeall)

local img = nil

function init()
	img = tex.load(pre..'yerka2.png')
	print('init spring')
end

function close()
	tex.free(img)
	print('close spring')
end

function enter()
	print('enter spring')
end

function leave()
	print('leave spring')
end

function update()
	if mouse.down(mouse.primary) then
		states.replace('summer')
	end
	return true
end

function render(t)
	local sgn = t / math.abs(t+0.00001)
	local tt = smoothstep(0, 1, math.abs(t)) * sgn
	video.draw_rect(img, 0, vec2(31 + tt * 512, 61))
	return true
end
