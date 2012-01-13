require "editor"

pre = 'nulis2_assets/'
scr_size = { x = 1024, y = 768 }

function game_init()
	states.register('editor', editor)
end

function game_close()
end
