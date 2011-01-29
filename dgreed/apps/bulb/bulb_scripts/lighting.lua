
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
		
-- Alpha massive
alphaMap = {}

lighting = {}

function lighting.init()
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
					alphaMap[i][j] = alphaMap[i][j] - (1 - length(cord - l.pos)/l.inten) * l.base
					alphaMap[i][j] = clamp(0, 1, alphaMap[i][j])
				end
			end
		end
	end
	lighting.draw(layer)
	
end

function lighting.reset()
	for i=1,scrSize.x / tsize do
		alphaMap[i] = {}
		for j=1,scrSize.y / tsize do
			alphaMap[i][j] = 1
		end
	end
end

function lighting.draw(layer) 
	local rectCord
	for i=1,scrSize.x / tsize do
		for j=1,scrSize.y / tsize do
			rectCord = rect((i-1)*tsize, (j-1)*tsize, i*tsize, j*tsize)
			video.draw_rect(blackT, layer, rectCord, rgba(1, 1, 1, alphaMap[i][j]))
		end
	end	
end

function lighting.destroy()
	tex.free(blackT)
end
