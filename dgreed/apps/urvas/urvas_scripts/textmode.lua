local textmode = {}

textmode.size = vec2(40, 20)
textmode.n_chars = textmode.size.x * textmode.size.y

textmode.bg_color = {}
textmode.fg_color = {}
textmode.char = {}

textmode.selected_fg = nil
textmode.selected_bg = nil

textmode.texture = nil

function textmode.init()
	textmode.texture = tex.load(asset_dir..'chrsheet.png')
end

function textmode.close()
	tex.free(textmode.texture)
end

function textmode.clear(color)
	local col = color or rgba(0, 0, 0)
	local space = string.byte(' ')
	for i,c in ipairs(textmode.char) do
		textmode.char[i] = space 
		textmode.bg_color[i] = col
	end
end

function textmode.rect(l, t, r, b, char)
	local c = char:byte()
	for y=t,b do
		local idx1 = textmode.size.x * y + l	
		local idx2 = textmode.size.x * y + r	
		textmode.char[idx1] = c
		textmode.char[idx2] = c
		textmode.fg_color[idx1] = textmode.selected_fg
		textmode.bg_color[idx1] = textmode.selected_bg
		textmode.fg_color[idx2] = textmode.selected_fg
		textmode.bg_color[idx2] = textmode.selected_bg
	end
	for x=l+1,r-1 do
		local idx1 = textmode.size.x * t + x	
		local idx2 = textmode.size.x * b + x	
		textmode.char[idx1] = c
		textmode.char[idx2] = c
		textmode.fg_color[idx1] = textmode.selected_fg
		textmode.bg_color[idx1] = textmode.selected_bg
		textmode.fg_color[idx2] = textmode.selected_fg
		textmode.bg_color[idx2] = textmode.selected_bg
	end
end

function textmode.write_middle(y, str)
	local l = str:len()
	textmode.write(math.floor(20 - l/2), y, str)
end

function textmode.write(x, y, str)
	local idx = textmode.size.x * y + x
	for c in str:gmatch('.') do
		textmode.char[idx] = c:byte()
		textmode.fg_color[idx] = textmode.selected_fg
		textmode.bg_color[idx] = textmode.selected_bg
		idx = idx + 1
	end
end

function textmode.present(layer)
	local char_width = 24
	local char_height = 28

	local white = rgba(1, 1, 1)
	local src = rect()
	local src_empty = rect(0, 512-char_height, 512, 512)
	local cursor = vec2(0, 0)
	for y=0,textmode.size.y-1 do
		for x=0,textmode.size.x-1 do
			local idx = textmode.size.x * y + x	
			local char = textmode.char[idx] 
			if char then
				local texture_x = char % 16
				local texture_y = (char - texture_x) / 16
				src.l = texture_x * char_width + 4
				src.t = texture_y * char_height + 4
				src.r = src.l + char_width - 8
				src.b = src.t + char_height - 8 

				video.draw_rect(
					textmode.texture, layer, src,
					cursor, 0, textmode.fg_color[idx] or white
				)

				if textmode.bg_color[idx] then
					video.draw_rect(
						textmode.texture, layer-1, src_empty,
						cursor, 0, textmode.bg_color[idx]
					)
				end
			end

			cursor.x = cursor.x + char_width - 8 
		end
		cursor.x = 0
		cursor.y = cursor.y + char_height - 8
	end
end

return textmode
