
asset_dir = 'gruodis_assets/'

scr_size = vec2(200, 150)
scr_rect = rect(0, 0, scr_size.x, scr_size.y)

local math_floor = math.floor
function feql(a, b)
	local d = a - b
	return d*d < 0.0000001
end

function snap_pos(p)
	return vec2(math_floor(p.x), math_floor(p.y))	
end

function calc_bbox(obj)
	local hs = obj.size * 0.5
	return rect(
		obj.pos.x - hs, obj.pos.y - hs,
		obj.pos.x + hs, obj.pos.y + hs
	)
end

local game = require('game')

function game_init()
	local scale = 4
	local real_size = scr_size * scale
	video.init_exr(real_size.x, real_size.y, scr_size.x, scr_size.y, 'gruodis', false)
	sound.init()

	particles.init(asset_dir, 2)
	mfx.init(asset_dir..'effects.mml')
	sprsheet.init(asset_dir..'sprsheet.mml')
--	mus = sound.load_stream(asset_dir..'aftermath.ogg')
--	fnt = font.load(asset_dir..'Georgia_60px.bft', 0.5, asset_dir)
--	sound.play(mus, true)

	states.prerender_callback(function()
		mfx.update()
		particles.update()
		particles.draw()
	end)

	states.register('game', game)
	states.push('game')
	states.transition_len = 0.5
end

function game_close()
--	font.free(fnt)
--	sound.free(mus)
	sprsheet.close()
	mfx.close()
	particles.close()
	sound.close()
	video.close()
end
