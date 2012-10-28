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
local half_grid = vec2(grid_size / 2, grid_size / 2)
function grid2screen(pos)
	return (scr_size / 2 + (pos - level_size / 2) * grid_size) + half_grid
end

-- converts grid space x, y pair into 
-- flattened 2d list index
function grid_idx(x, y)
	return y * level_size.x + x
end

local game = require('game')

function game_init()
	local scale = 1
	local real_size = scr_size * scale
	video.init_ex(real_size.x, real_size.y, scr_size.x, scr_size.y, 'bailys', false)
	sound.init()

	--mfx.init(asset_dir..'effects.mml')
	sprsheet.init(asset_dir..'sprsheet.mml')
	mus = sound.load_stream(asset_dir..'distressed.ogg')
	sound.play(mus, true)
	--fnt = font.load(asset_dir..'Georgia_60px.bft', 0.5, asset_dir)

	states.register('game', game)
	states.push('game')
end

function game_close()
	--font.free(fnt)
	sound.free(mus)
	sprsheet.close()
	--mfx.close()
	sound.close()
	video.close()
end

