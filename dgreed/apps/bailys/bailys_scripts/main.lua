-- let's define some generally useful data before
-- loading submodules, so that it will be available in
-- their global scopes

asset_dir = 'bailys_assets/'

scr_size = vec2(960, 640)
scr_rect = rect(0, 0, scr_size.x, scr_size.y)

grid_size = 64

-- game.load_level sets this
level_size = nil

-- converts grid space to screen space
function grid2screen(pos)
	return scr_size / 2 + (pos - level_size / 2) * grid_size
end

local game = require('game')

function game_init()
	local scale = 1
	local real_size = scr_size * scale
	video.init_ex(real_size.x, real_size.y, scr_size.x, scr_size.y, 'bailys', false)
	sound.init()

	--mfx.init(asset_dir..'effects.mml')
	sprsheet.init(asset_dir..'sprsheet.mml')
	--mus = sound.load_stream(asset_dir..'aftermath.ogg')
	--fnt = font.load(asset_dir..'Georgia_60px.bft', 0.5, asset_dir)
	--sound.play(mus, true)

	states.register('game', game)
	states.push('game')
end

function game_close()
	--font.free(fnt)
	--sound.free(mus)
	sprsheet.close()
	--mfx.close()
	sound.close()
	video.close()
end

