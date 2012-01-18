require 'editor'
require 'menu'

pre = 'nulis2_assets/'
scr_size = { x = 1024, y = 768 }

function game_init()
	states.register('editor', editor)
	states.register('menu', menu)
end

function game_close()
end
