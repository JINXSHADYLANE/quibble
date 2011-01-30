
-- Tile size
tsize = 32
-- Screen size
scrSize = { x = 960,
			y = 640,
		}
-- Texture
blackT = nil 

-- Tint
bTint = nil

-- Best x, y
bx = 0
by = 0

-- X, Y
sx = 0
sy = 0

-- Accuracy
acu = 1
		
-- Alpha massive
alphaMap = {}
isDraw = {}

lighting = {}

function lighting.init()
	log.info((scrSize.x / tsize)*(scrSize.y / tsize))
	sx = scrSize.x / tsize
	sy = scrSize.y / tsize
	log.info(sx)
	log.info(sy)

	blackT = tex.load(pre..'dark.png')
	lighting.reset()
end
		
function lighting.render(layer, lights)
	lighting.reset()
	for i=1,scrSize.x / tsize do
		for j=1,scrSize.y / tsize do
			local cord = vec2((i-1)*tsize + (tsize/2), (j-1)*tsize + (tsize/2))
			for k, l in ipairs(lights) do
				if ( length_sq(cord - l.pos) < l.inten*l.inten) then
				--	log.info(tostring(3))
					alphaMap[i][j] = alphaMap[i][j] - (1 - length(cord - l.pos)/l.inten) * l.base
					alphaMap[i][j] = clamp(0, 1, alphaMap[i][j])
				--	log.info(tostring(aplhaMap[i][j]))
				end
			end
	--		log.info(tostring(aplhaMap[i][j]))
		end
	end
	lighting.draw(layer)
	
end

function lighting.reset()
	for i=1,scrSize.x / tsize do
		alphaMap[i] = {}
		isDraw[i] = {}
		for j=1,scrSize.y / tsize do
			alphaMap[i][j] = 1
			isDraw[i][j] = false
		end
	end
end

function lighting.draw(layer) 
	local rectCord
	local rec = 0
	for i=1, sx do
		for j=1, sy  do
			if ((acu <= alphaMap[i][j]) and not isDraw[i][j]) then 
				lighting.optimaze(i, j)
				rectCord = rect((i-1)*tsize, (j-1)*tsize, (i+bx-1)*tsize, (j+by-1)*tsize)
				video.draw_rect(blackT, layer, rectCord, rgba(1, 1, 1, alphaMap[i][j]))
				--video.draw_rect(blackT, layer, rectCord, rgba(math.random(), math.random(), math.random(), 1))
				rec = rec + 1
			else if (alphaMap[i][j] < acu) then
				isDraw[i][j] = true
				rectCord = rect((i-1)*tsize, (j-1)*tsize, i*tsize, j*tsize)
				video.draw_rect(blackT, layer, rectCord, rgba(1, 1, 1, alphaMap[i][j]))
				rec = rec + 1
				end
			end
			--rectCord = rect((i-1)*tsize, (j-1)*tsize, i*tsize, j*tsize)
			--video.draw_rect(blackT, layer, rectCord, rgba(1, 1, 1, alphaMap[i][j]))
		end
	end	
end


function lighting.optimaze(x, y)
	local minimum = math.min(sx - x, sy - y)
	--log.info("x y")
	--log.info(x)
	--log.info(y)
	local found, len, bestrx, bestdy = false, 1
	for i=1, minimum do
		for j=1, i do
			if (not (acu <= alphaMap[x+j][y+i]) or isDraw[x+j][y+i]) then
				found = true
				break
			else if (not (acu <= alphaMap[x+i][y+j]) or isDraw[x+i][y+j]) then
					found = true
					break
				end
			end
		end
		if (not found) then
			len = len + 1
		else
			break
		end
	end
	found = false
	bestrx = len 
	bestdy = len 
	for i = len, sx - x do
		for j=0, len -1 do
			if (not (acu <= alphaMap[x+i][y+j]) or  isDraw[x+i][y+j]) then
				found = true
				break
			end
		end
		if (not found) then
			bestrx = bestrx + 1
		else
			break
		end
	end	
	found = false
	--log.info("len")
	--log.info(len)
	for i = len, sy - y do
		for j=0, len - 1 do
			if (not (acu <= alphaMap[x+j][y+i]) or  isDraw[x+j][y+i]) then
				found = true
				break
			end
		end
		if (not found) then
			bestdy = bestdy + 1
		else
			break
		end
	end
	if ((len*bestdy < len*len) and (len*bestrx < len*len)) then
		bx = len
		by = len
		else if (len*bestdy < len*bestrx) then
				bx = bestrx
				by = len
			else
				bx = len
				by = bestdy
		end
	end
    --bx  = len
	--by = len
	for i=x,x+bx-1 do
		for j=y,y+by-1 do
			isDraw[i][j] = true
		end
	end
	--log.info("bx by")
	--log.info(bx)
	--log.info(by)
end


function lighting.destroy()
	tex.free(blackT)
end
