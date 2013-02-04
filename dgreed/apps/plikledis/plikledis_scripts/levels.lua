module(..., package.seeall)

-- tile bitmasks
b_wall = 0xF
b_box = 0x10 
b_slot = 0x20
b_exit = 0x40
b_char = 0x80
b_fiend = 0x100

tile_dict = {
	['0'] = b_wall,
	['#'] = b_wall,
	['B'] = b_box,
	['S'] = b_slot,
	['E'] = b_exit,
	['C'] = b_char,
	['F'] = b_fiend,
	['X'] = b_slot + b_box
}

-- levels
prelude = {
	'  ######### ',
	'###        #',
	'#C S   B   E',
	'###        #',
	'  ######### '
}

onwards = {
	'##############',
	'#CBSXS      ##',
	'##BBS#       #',
	'## B #   ##  #',
	'#SB  #   ##B #',
	'#SS  B     B E',
	'##############'
}

around = {
	' #######   ',
	' #     #   ',
	' #     #   ',
	' ## ##S####',
	'  #  #SS  #',
	'### B###  #',
	'#   CB #  #',
	'#  B      #',
	'#      #  E',
	'###########'
}

dejavu = {
	'####E#######  ',
	'#SS  #     ###',
	'#SS  # B  B  #',
	'#SS  #B####  #',
	'#SS    C ##  #',
	'#SS  # #  B ##',
	'###### ##B B #',
	'  # B  B B B #',
	'  #    #     #',
	'  ############'  
}

slip = {
	'########',
	'#      #',
	'#    # #',
	'###SSBSE',
	'  # #B##',
	'  #  C #',
	'  ## B #',
	'   # # #',
	'   #   #',
	'   #####'
}

fall = {
	'    ####11111',
	'#####  00000#',
	'#F#   BB    #',
	'#  C#XSSSXXS#',
	'#E#    BB   #',
	'  ####  #####',
	'     ####    '  
}

hand = {
	' 11#####   ',
	'##00   #   ',
	'#  X   ####',
	'# ####X##F#',
	'#  X B X  #',
	'#C   S    E',
	'###########'
}

pull = {
	'111#####    ',
	'1000 C #####',
	'10 BBBBB  F#',
	'1# SSSSS ###',
	' E   #   #  ',
	' #########  '
}

doubt = {
	'#########',
	'#C  #   #',
	'# B # B #',
	'#  S S  #',
	'##1 # 1##',
	'#  S S1##',
	'# B # B #',
	'#   #F  #',
	'#######E#'
}

fright = {
	'00000000',
	'#111111#',
	'#S1CF S#',
	'#XBBBBX#',
	'#S111 S#',
	'#1111E##',
	'0000000 '
}

closure = {
	'    #####  ',
	'    #   #  ',
	'##### S ###',
	'E2F1 B  B #',
	'#### SSB  #',
	'   #####C #',
	'       ####'
}


levels = {
	{l=prelude, t={'It is about', 'dangers of desires.'}}, 
	{l=onwards, t={'Life moves forward', 'with each finished venture.'}},
	{l=around, t={'No matter how long it took,', 'or how many mistakes you made.'}}, 
	{l=dejavu, t={'All is good, until you', 'encounter a roadblock.'}},
	{l=slip, t={'A fiend.'}},
	{l=fall, t={'Fiend uses desire to steal time.'}},
	{l=hand, t={'Like gambling or television.'}},
	{l=pull, t={'The moment desire overcomes', 'you, life stops moving forward.'}},
	{l=doubt, t={'And the fiend wins.'}},
	{l=fright, t={'The worst thing is - once you realize', 'what happened, it might be too late.'}},
	{l=closure}
}

function init()
	current_level = 1 
end

function fiend(level, move, door_open) 
	local curr = levels[current_level].l
	local res = false

	if door_open then
		switch_walls(level, 1, 2)
		if curr == closure then
			res = true
			level.noise = 20
		end
	else
		switch_walls(level, 1)
	end

	move.switch_walls = 0
	level.fiend = nil 

	return res
end

function switch_walls(level, i, j)
	local curr = levels[current_level].l

	level.noise = 6
	for y,row in ipairs(curr) do
		local x = 0
		for c in row:gmatch('.') do
			local idx = idx2d(x, y-1, level.size_w)
			local n = tonumber(c)
			if n ~= nil then
				if n == i or n == j then
					-- turn wall on
					level.tiles[idx] = b_wall
				else
					-- turn wall off
					level.tiles[idx] = nil
				end
			end
			x = x + 1
		end
	end
	precalc_wall_bitmasks(level)
end

function precalc_wall_bitmasks(level)
	local is_wall = function(x, y)
		if x < 0 or y < 0 or x >= level.size_w or y >= level.size_h then
			return false
		end
		local idx = idx2d(x, y, level.size_w)
		local t = level.tiles[idx]
		if t ~= nil and t <= b_wall then
			return true
		end
		return false
	end

	for idx,t in pairs(level.tiles) do
		if t <= b_wall then
			local x,y = rev_idx2d(idx, level.size_w)
			local new_bitmask = 0

			if is_wall(x+1, y) then
				new_bitmask = bit.bor(new_bitmask, 0x1)
			end
			
			if is_wall(x, y-1) then
				new_bitmask = bit.bor(new_bitmask, 0x2)
			end

			if is_wall(x-1, y) then
				new_bitmask = bit.bor(new_bitmask, 0x4)
			end

			if is_wall(x, y+1) then
				new_bitmask = bit.bor(new_bitmask, 0x8)
			end

			level.tiles[idx] = new_bitmask
		end
	end
end

function get_text()
	return levels[current_level].t
end

function get_current()
	assert(current_level)
	local curr = levels[current_level].l

	local result = {
		tiles = {},
		size_w = 0,
		size_h = #curr,
		char_pos = nil,
		slots_to_trigger = 0
	}

	local width = nil
	for y,row in ipairs(curr) do
		-- make sure row length is right
		if width == nil then
			width = #row
		else
			assert(width == #row)
		end

		-- iterate over individual chars
		local x = 0
		for c in row:gmatch('.') do
			if c == 'S' then
				result.slots_to_trigger = result.slots_to_trigger + 1
			end
			if c == 'C' then
				assert(result.char_pos == nil)
				result.char_pos = {['x'] = x, ['y'] = y-1}
			else
				if tile_dict[c] ~= nil then
					local idx = idx2d(x, y-1, width)
					result.tiles[idx] = tile_dict[c]
				end
			end

			x = x+1
		end
	end
	result.size_w = width

	precalc_wall_bitmasks(result)

	return result
end

