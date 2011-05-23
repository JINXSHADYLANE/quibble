devmode_on = false

devmode = {}
devmode.a = 16
blue = rgba(0.3, 0.28, 0.9)
red = rgba(0.85, 0, 0)
black = rgba(0, 0, 0)
	
function devmode.init()
	tile = tex.load(media.."tile.png")
end

function devmode.close()
	tex.free(tile)
end

function devmode.draw()
	local i, j, x1, y1, x2, y2
	for i = 0, 63 do
		for j = 0, 47 do
			x1 = i * devmode.a
			y1 = j * devmode.a
			x2 = x1 + devmode.a
			y2 = y1 + devmode.a
			if tilemap.collide(arena[active_arena].tilemp, rect(x1, y1, x2, y2)) then 
				video.draw_rect(tile, 0, rect(x1, y1, x2, y2), black)
			end
		end
	end
	video.draw_rect(tile, 0, boy.col_rect, red)
end
