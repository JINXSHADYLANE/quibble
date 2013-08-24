local textmode = {}
local textmode_mt = {}
textmode_mt.__index = textmode

textmode.texture = nil

function textmode.init()
	textmode.texture = tex.load(asset_dir..'chrsheet.png')
end

function textmode.close()
	tex.free(textmode.texture)
end

function textmode:new()
	assert(self.texture)
	local o = {
		size = vec2(40, 20),
		n_chars = 40 * 20,
		bg_color = {},
		fg_color = {},
		char = {},
		selected_fg = nil,
		selected_bg = nil,
		color_stack = {}
	}

	setmetatable(o, textmode_mt)
	return o
end

function textmode:push()
	table.insert(self.color_stack, self.selected_fg or rgba(1,1,1))	
	table.insert(self.color_stack, self.selected_bg or rgba(0,0,0))	
end

function textmode:pop()
	assert(#self.color_stack >= 2)
	self.selected_bg = table.remove(self.color_stack)
	self.selected_fg = table.remove(self.color_stack)
end

function textmode:clear(color)
	local col = color or rgba(0, 0, 0)
	local space = string.byte(' ')
	for i=0,self.n_chars-1 do
		self.char[i] = space 
		self.bg_color[i] = col
	end
end

function textmode:rect(l, t, r, b, char)
	local c = char:byte()
	for y=t,b do
		local idx1 = self.size.x * y + l	
		local idx2 = self.size.x * y + r	
		self.char[idx1] = c
		self.char[idx2] = c
		self.fg_color[idx1] = self.selected_fg
		self.bg_color[idx1] = self.selected_bg
		self.fg_color[idx2] = self.selected_fg
		self.bg_color[idx2] = self.selected_bg
	end
	for x=l+1,r-1 do
		local idx1 = self.size.x * t + x	
		local idx2 = self.size.x * b + x	
		self.char[idx1] = c
		self.char[idx2] = c
		self.fg_color[idx1] = self.selected_fg
		self.bg_color[idx1] = self.selected_bg
		self.fg_color[idx2] = self.selected_fg
		self.bg_color[idx2] = self.selected_bg
	end
end

function textmode:write_middle(y, str)
	local l = str:len()
	self:write(math.floor(20 - l/2), y, str)
end

function textmode:put(x, y, c)
	local idx = self.size.x * y + x
	self.char[idx] = c:byte()
	self.fg_color[idx] = self.selected_fg
	self.bg_color[idx] = self.selected_bg
end

function textmode:write(x, y, str)
	local idx = self.size.x * y + x
	for c in str:gmatch('.') do
		self.char[idx] = c:byte()
		self.fg_color[idx] = self.selected_fg
		self.bg_color[idx] = self.selected_bg
		idx = idx + 1
	end
end

function textmode:recolour(x, y, n)
	local idx = self.size.x * y + x
	for i=1,n do
		self.fg_color[idx] = self.selected_fg
		self.bg_color[idx] = self.selected_bg
		idx = idx + 1
	end
end

function textmode:present(layer, t)
	local char_width = 24
	local char_height = 28

	local white = rgba(1, 1, 1)
	local src = rect()
	local src_empty = rect(0, 512-char_height, 512, 512)
	local cursor = vec2(0, 0)
	for y=0,self.size.y-1 do
		for x=0,self.size.x-1 do
			local idx = self.size.x * y + x	
			local char = self.char[idx] 
			if char then
				local texture_x = char % 16
				local texture_y = (char - texture_x) / 16
				src.l = texture_x * char_width + 4
				src.t = texture_y * char_height + 4
				src.r = src.l + char_width - 8
				src.b = src.t + char_height - 8 

				local col = self.fg_color[idx] or white

				if t then
					col.a = 1 - math.abs(t)
				end


				video.draw_rect(
					self.texture, layer, src,
					cursor, 0, col
				)

				if t then
					col.a = 1
				end

				if self.bg_color[idx] then
					col = self.bg_color[idx]
					
					if t then
						col.a = 1 - math.abs(t)
					end

					video.draw_rect(
						self.texture, layer-1, src_empty,
						cursor, 0, col
					)

					if t then
						col.a = 1
					end
				end
			end

			cursor.x = cursor.x + char_width - 8 
		end
		cursor.x = 0
		cursor.y = cursor.y + char_height - 8
	end
end

return textmode
