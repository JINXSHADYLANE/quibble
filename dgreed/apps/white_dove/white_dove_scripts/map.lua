-- First map of White Dove

-- must be created!
init_arena = 1
arenas_count = 4

-- list of all arenas
empty = 0
room = 1
fountain = 2
corner = 3
ocean = 4

-- arena:
--     adjacent screens
--     initial positions
--     items positions (rect & tri)
--     image
--     extra character number

arena = {
	[1] = {
		-- adjacent screens
		top_id = empty,
		left_id = empty,
		bottom_id = fountain,
		right_id = empty,

		-- main char initial positions
		top_pos = vec2(),
		left_pos = vec2(),
		bottom_pos = vec2(screen.r/2, screen.b-boy.h-20),
		right_pos = vec2(),

		items_rect = {
			[0] = rect(0, 0, 880, 400),
			[1] = rect(740, 415, 1024, 465),
			[2] = rect(947, 475, 1024, 666), 
			[3] = rect(990, 666, 1024, 768),
			[4] = rect(385, 425, 550, 583),
			[5] = rect(200, 425, 350, 590),
			[6] = rect(100, 445, 200, 540)
		},
		
		items_tri = {
		},

		img = nil,
		img_name = media.."room.png",
		char = 0
	},

	[2] = {
		-- adjacent screens
		top_id = corner,
		left_id = room,
		bottom_id = empty,
		right_id = empty,

		-- main char initial positions
		top_pos = vec2(362, 90),
		left_pos = vec2(170, 315),
		bottom_pos = vec2(),
		right_pos = vec2(),

		items_rect = {
			[0] = rect(670, 270, 1024, 410),
			[1] = rect(480, 220, 650, 310),
			[2] = rect(170, 190, 470, 220),
			[3] = rect(130, 220, 255, 345),
			[4] = rect(30, 330, 220, 415),
			[5] = rect(0, 430, 160, 460),
			[6] = rect(60, 460, 110, 580),
			[7] = rect(0, 495, 50, 768),
			[8] = rect(364, 532, 554, 600),
			[9] = rect(400, 470, 510, 550)
		},

		items_tri = {
		},

		img = nil,
		img_name = media.."fountain.png",
		char = girl
	},

	[3] = {
		-- adjacent screens
		top_id = empty,
		left_id = empty,
		bottom_id = fountain,
		right_id = ocean,

		-- main char initial positions
		top_pos = vec2(),
		left_pos = vec2(),
		bottom_pos = vec2(screen.r/2, screen.b-boy.h-20),
		right_pos = vec2(screen.r-boy.w-20, screen.b/2+boy.h/3-30),

		items_rect = {
			[0] = rect(822, 405, 1024, 492),
			[1] = rect(930, 500, 970, 560),
			[2] = rect(280, 250, 1024, 360),
			[3] = rect(635, 380, 1024, 400),
			[4] = rect(815, 390, 1024, 420),
			[5] = rect(0, 210, 300, 380),
			[6] = rect(0, 380, 200, 415),
			[7] = rect(0, 420, 150, 450),
			[8] = rect(0, 450, 60, 500)
		},

		items_tri = {
		},

		img = nil,
		img_name = media.."corner.png",
		char = math
	},

	[4] = {
		-- adjacent screens
		top_id = empty,
		left_id = corner,
		bottom_id = empty,
		right_id = empty,

		-- main char initial positions
		top_pos = vec2(),
		left_pos = vec2(20, screen.b/2-boy.h/2),
		bottom_pos = vec2(),
		right_pos = vec2(),


		items_rect = {
			[0] = rect(0, 225, 660, 390),
			[1] = rect(670, 220, 910, 330),
			[2] = rect(900, 210, 1024, 300)
		},

		items_tri = {
		},

		img = nil,
		img_name = media.."ocean.png",
		char = mage
	},

	color = rgba(1, 1, 1, 0),
	state = fade_state.other,
	switch_dir = direction.other
}

function arena.check_exits()
	
	local door = rect(40, 440, 130, 645)
	if active_arena == fountain and rect_rect_collision(boy.col_rect, door)
		and key.down(key.b) then
		arena.switch_dir = direction.left
		arena.state = fade_state.fadeout
	end
end

