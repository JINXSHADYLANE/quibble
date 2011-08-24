module(..., package.seeall)

local img = nil

function init()
	img = tex.load(pre..'yerka1.png')
	print('init winter')
end

function close()
	tex.free(img)
	print('close winter')
end

function enter()
	print('enter winter')
end

function leave()
	print('leave winter')
end

function update()
	if mouse.down(mouse.primary) then
		states.replace('spring')
	end
	return true
end

function render(t)
	local sgn = t / math.abs(t+0.00001)
	local tt = smoothstep(0, 1, math.abs(t)) * sgn
	video.draw_rect(img, 0, vec2(31 + tt * 512, 55))
	return true
end
