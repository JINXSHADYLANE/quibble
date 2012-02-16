local hud = {}

local hud_layer = 14
local paper_layer = 15

function hud.init()
	hud.paper_spr = sprsheet.get_handle('paper')
	hud.tile = sprsheet.get_handle('tile_reflect')
	hud.shadow = sprsheet.get_handle('m_shadow')

	video.set_blendmode(paper_layer, 'multiply')
end

function hud.close()
end

function hud.render()
	-- paper
	sprsheet.draw(hud.paper_spr, paper_layer, vec2(0, 0))

	-- bottom menu tiles
	for i=1,5 do
		sprsheet.draw(hud.tile, hud_layer, vec2((i-1)*64, scr_size.y - 64))
	end

	-- bottom shadow
	sprsheet.draw(hud.shadow, hud_layer, rect(0, scr_size.y - 74, scr_size.x, scr_size.y - 64))

	-- top shadow
	sprsheet.draw(hud.shadow, hud_layer, rect(0, 32 + 10, scr_size.x, 32))
end

return hud
