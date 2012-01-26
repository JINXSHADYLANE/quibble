require 'game'
require 'editor'
require 'menu'

pre = 'nulis2_assets/'
scr_size = { x = 1024, y = 768 }

music = nil
music_source = nil

function game_init()
	states.register('game', game)
	states.register('editor', editor)
	states.register('menu', menu)

	states.push('game')

	music = sound.load_stream(pre..'theme.ogg')
	music_source = sound.play(music, true)
end

function game_close()
	sound.free(music)
end
