require "winter"
require "spring"
require "summer"

pre = 'states_assets/'

function game_init()
	video.init(512, 512, 'states')

	states.register("winter", winter)
	states.register("spring", spring)
	states.register("summer", summer)

	states.push("winter")

	states.transition_len = 1
end

function game_close()
	video.close()
end
