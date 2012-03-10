module(..., package.seeall)

require 'menu'

image_layer = 1
image_show_len = 4

images = {
	['l1'] = {'title', 'tut_one_finger'},
	['l2'] = {'tut_two_fingers','hint1'},
	['l3'] = {'tut_menu_fingers','hint2','hint3'},
	['l10'] = {'','clock'},
	['l20'] = {'','magnet'},
	['l29'] = {'','stopwatch'}
}

tut_fingers = {
	['tut_one_finger'] = 1,
	['tut_two_fingers'] = 2,
	['tut_menu_fingers'] = 5 
}

tut_menu_offsets = {
	vec2(0, 0),
	vec2(-80, 20),
	vec2(-180, 200),
	vec2(80, 20),
	vec2(155, 100)
}

function init()
	screen_center = vec2(scr_size.x/2, scr_size.y/2)

	if scr_type == 'ipad' then
		tut_offset = vec2(0, 150)
	elseif scr_type == 'iphone' then
		tut_offset = vec2(0, 60)
		tut_fingers['tut_menu_fingers'] = 3
	end
end

function close()
end

last_level = nil
scenario = nil
start_t = nil
freeze_t = nil

function render(level)

	if last_level ~= level then
		last_level = level
		if scenario == nil then
			-- old scenario finished, start a new one

			-- clone table rather than copy reference
			scenario = nil
			if images[level] and #images[level] > 0 then
				scenario = {}
				for i,img in ipairs(images[level]) do
					table.insert(scenario, img)
				end
			end

			start_t = states.time('game') 
			freeze_t = nil
			unfreeze_t = nil
		else
			if images[level] then
				if freeze_t and not unfreeze_t then
					unfreeze_t = states.time('game') - start_t
				end
				-- append images to old scenario
				for i,img in ipairs(images[level]) do
					table.insert(scenario, img)
				end
			end
		end
	end

	if scenario then
		local n = #scenario
		local t = nil
		local gt = states.time('game')

		if freeze_t then
			t = freeze_t
		else
			t = gt - start_t
		end

		if t < n * image_show_len then
			local i = math.floor(t / image_show_len)
			assert(i < n)
			if img ~= scenario[i+1] then
				tut = nil
			end
			img = scenario[i+1]
			if not img or img == '' then
				return
			end
			
			local pos = screen_center
			local tt = (t - i * image_show_len) / image_show_len
			local img_alpha = math.sin(tt * math.pi)
			local color = rgba(1, 1, 1, img_alpha) 

			-- finger image hacks
			if img:find('tut') == 1 then
				
				if not tut and not freeze_t and tt > 0.5 then
					tut = tut_fingers[img]
					freeze_t = t
				end

				if freeze_t then
					if touch.count() == tut or (tut == 2 and mouse.down(mouse.secondary)) or
						(tut == 5 and key.pressed(key.quit)) then
						local unfreeze_t = gt - start_t 
						start_t = start_t + (unfreeze_t - freeze_t)
						freeze_t = nil
					end
				end

				local tut_anim = tut_fingers[img]
				pos = pos + rotate(tut_offset, menu.angle) 
				if tut_anim == 1 or tut_anim == 2 or tut_anim == 5 then
					local ttt = gt / image_show_len * 2
					if tut_anim == 2 then
						ttt = 1 - ttt
					end
					local alpha = clamp(0, 1, math.sin(ttt * math.pi)) * img_alpha
					local circle_color = rgba(0.7, 0.7, 0.7, alpha)
					local size = ttt - math.floor(ttt)
					local circle_size = lerp(1.5, 0.4, size)

					if tut_anim == 5 then
						circle_size = lerp(0.3, 1, size)
					end

					if tut_fingers[img] ~= 5 then
						-- draw 5 circles for menu tutorial
						sprsheet.draw_centered('circle', image_layer, screen_center, 
							0, circle_size, circle_color)
					else
						for i=1,5 do
							local pos = screen_center + rotate(tut_menu_offsets[i], menu.angle)
							sprsheet.draw_centered('circle', image_layer, pos, 
								0, circle_size, circle_color)
						end
					end
				end
			end

			sprsheet.draw_centered(img, image_layer, pos, menu.angle, 1.0, color) 
		else
			scenario = nil
		end
	end
end

