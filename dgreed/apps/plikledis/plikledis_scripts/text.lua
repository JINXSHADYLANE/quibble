module(..., package.seeall)

fade_len = 0.5

last_line_t = nil
fadeout = false

finished = false

queue = {}

function prologue()
	center = vec2(scr_size.x/2, scr_size.y/2)
	color_light = rgba(1, 1, 1, 1)
	color_dark = rgba(0.8, 0.8, 0.8, 1)
	pos_light = center - vec2(0, 30)
	pos_dark = center + vec2(0, 30)

	push({	'This is your stop.'})

	push({	'I know. I can get out',
			'at the next one.'}, true)

	push({	'Do you want to hear',
			'my theory then?'})

	push({	'Sure.'}, true)
end

function epilogue()
	push({	'What an interesting theory.'}, true)

	push({	'Thanks.'})
	push({	'Look, your stop.'})

	push({	'I will get out at the next one.',
			'I really enjoy talking to you.'}, true)

	push({''})
end

function push(str, light)
	local p = pos_dark
	local c = color_dark
	if light then
		p = pos_light
		c = color_light
	end

	table.insert(queue, {['t']=str, ['c']=c, ['p']=p})
	last_line_t = time.s()
end

function update()
	if #queue > 0 then
		local faded_in = time.s() - last_line_t > fade_len
		if not fadout and faded_in and char.down(' ') then
			fadeout = true
			last_line_t = time.s()
		end
		return true
	else
		return finished 
	end
end

function render(layer)
	local fullscr = rect(0, 0, scr_size.x, scr_size.y)
	if #queue > 0 then
		local skip_text = false
		local top = queue[1]
		local t = 1
		if fadeout then
			t = (time.s() - last_line_t) / fade_len
			if t > 1 then
				table.remove(queue, 1)
				last_line_t = time.s()
				fadeout = false
				skip_text = true
			end
			t = 1 - t
		else
			t = (time.s() - last_line_t) / fade_len
			t = math.min(t, 1)
		end
		t = clamp(0, 1, t)

		local col = top.c
		col.a = t


		if not skip_text then
			local p = vec2(top.p)
			for i,line in ipairs(top.t) do		
				if line == '' then
					finished = true
				end
				video.draw_text_centered(fnt, layer, line, p, col)
				p.y = p.y + 14
			end
		end

		local back_col = rgba(0, 0, 0, 1)
		if fadeout and #queue <= 1 then
			back_col.a = t
		end

		if skip_text and #queue == 0 then
			back_col.a = 0
		end

		sprsheet.draw('empty', layer-1, fullscr, back_col)
	end

	if finished then
		sprsheet.draw('empty', layer-1, fullscr, rgba(0,0,0,1))
	end
end


