local editor = require('editor')

assets_dir = 'lijo_assets/'
tilesets_dir = 'lijo_tilesets/'

function game_init()
	video.init(1024, 768, 'lijo')

	states.register('editor', editor)
	states.push('editor')
end

function game_close()
	video.close()
end
