module(..., package.seeall)

local img = nil

function init()
	img = tex.load(pre..'yerka3.png')
	print('init summer')
end

function close()
	tex.free(img)
	print('close summer')
end

function enter()
	print('enter summer')
end

function leave()
	print('leave summer')
end

function update()
	if mouse.down(mouse.primary) then
		states.replace('winter')
	end
	return true
end

function render(t)
	local sgn = t / math.abs(t+0.00001)
	local tt = smoothstep(0, 1, math.abs(t)) * sgn
	video.draw_rect(img, 0, vec2(tt * 512, 29))
	return true
end
