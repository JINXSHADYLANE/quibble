module(..., package.seeall)

image_layer = 4
image_show_len = 5

images = {
	['l1'] = {'', 'title', 'tut_one_finger'},
	['l2'] = {'', 'tut_two_fingers'}
}

function init()
	screen_center = vec2(scr_size.x/2, scr_size.y/2)
end

function close()
end

last_level = nil
scenario = nil
start_t = nil
function render(level)
	if last_level ~= level then
		last_level = level
		scenario = images[level]
		start_t = time.s()
	end

	if scenario then
		local n = #scenario
		local t = states.time('game') - start_t 
		if t < n * image_show_len then
			local i = math.floor(t / image_show_len)
			assert(i < n)
			local img = scenario[i+1]
			if not img or img == '' then
				return
			end
			
			local pos = screen_center
			local tt = (t - i * image_show_len) / image_show_len
			local color = rgba(1, 1, 1, math.sin(tt * math.pi)) 

			-- finger image hacks
			if img:find('tut') == 1 then
				pos = pos + vec2(0, 150)
			end
			if img == 'tut_one_finger' or img == 'tut_two_fingers' then
				if tt > 0.3 and tt < 0.8 then
					local ttt = (tt - 0.3) * 2
					if img == 'tut_two_fingers' then
						ttt = 1 - ttt
					end
					local circle_color = rgba(0.7, 0.7, 0.7, math.sin(ttt * math.pi))
					local circle_size = lerp(1.7, 0.3, ttt)
					sprsheet.draw_centered('circle', image_layer, screen_center, 
						0, circle_size, circle_color)
				end
			end

			sprsheet.draw_centered(img, image_layer, pos, color) 
		else
			scenario = nil
		end
	end
end

