local spells = {}

local timeline = require('timeline')

local function move_cursor(dest_x, dest_y, last_keypress)
	if char.down('\r') then
		return dest_x, dest_y, last_keypress, true
	end

	if time.s() - last_keypress < 0.20 then
		return dest_x, dest_y, last_keypress, false
	end

	if char.pressed('h') or key.pressed(key._left) then
		return dest_x - 1, dest_y, time.s(), false
	end
	if (char.pressed('j') or key.pressed(key._down)) then
		return dest_x, dest_y + 1, time.s(), false
	end
	if char.pressed('k') or key.pressed(key._up) then
		return dest_x, dest_y - 1, time.s(), false
	end
	if char.pressed('l') or key.pressed(key._right) then
		return dest_x + 1, dest_y, time.s(), false
	end

	return dest_x, dest_y, last_keypress, false
end

spells[1] = {
	name = 'Push',
	desc = 'Invisible force pushes objects or',
	desc2 = 'enemies away from you.',
	cost = 1,
	have = true,
	effect_len = 0.3,

	pre = nil,
	effect = function(player, room, textmode, t)
		room:render_circle(
			textmode, player.pos.x, player.pos.y, t * 3, rgba(0, 0, 0.6*(1-t))
		)
		return spells[1]
	end,
	post = nil
}

spells[2] = {
	name = 'Fireball',
	desc = 'Conjure and throw a swirling',
	desc2 = 'orb of flame.',
	cost = 2,
	have = true,

	-- state
	effect_len = inf,
	dest_x = nil,
	dest_y = nil,
	last_keypress = 0,
	path = nil,

	pre = function(player, room)
		local self = spells[2] 
		self.effect_len = -1
		self.dest_x = player.pos.x
		self.dest_y = player.pos.y
		timeline.text2 = 'hjkl/arrows - target, enter - confirm' 
	end,
	effect = function(player, room, textmode, t)
		textmode:push()
		local self = spells[2] 
		if self.effect_len == -1 then
			-- target select
			local sel
			self.dest_x, self.dest_y, self.last_keypress, sel = move_cursor(
				self.dest_x, self.dest_y, self.last_keypress
			)

			textmode.selected_bg = rgba(0.4, 0.4, 0.4)
			textmode:recolour(self.dest_x, self.dest_y, 1)

			if sel then
				self.path = room:ray(
					player.pos.x, player.pos.y, self.dest_x, self.dest_y
				)
				self.effect_len = #self.path * 0.05
				room.spell_t = time.s()
				timeline.text = nil
			end
		else
			local tt = t * #self.path + 1
			for i,p in ipairs(self.path) do
				local c = (i - tt)
				
				if c >= -1 and c < 1 then
					textmode.selected_bg = rgba(0.9*math.sqrt(math.abs(1-c)), 0.1, 0.2)
					textmode:recolour(p[1], p[2], 1)
				end
			end
		end

		textmode:pop()

		return self
	end,
	post = function(player, room)
	end
}

spells[3] = {
	name = 'Explode',
	desc = 'Makes a living creature explode,',
	desc2 = 'damaging everything nearby.',
	cost = 3
}

spells[4] = {
	name = 'Summon',
	desc = 'Summon an army of skeletons to fight',
	desc2 = 'for you.',
	cost = 4
}

spells[5] = {
	name = 'Teleport',
	desc = 'Vanish and appear somewhere else.',
	desc2 = '',
	cost = 5
}

spells[6] = {
	name = 'Freeze',
	desc = 'Turn every living creature around you',
	desc2 = 'into statues of ice.',
	cost = 6
}

spells[7] = {
	name = 'Shatter',
	desc = 'Cast a powerful ray, destroying all',
	desc2 = 'in its path, including walls of stone.',
	cost = 7
}

spells[8] = {
	name = 'Spell8',
	desc = 'Description.',
	desc2 = '',
	cost = 8
}

spells[9] = {
	name = 'Ascend',
	desc = 'Become one with the stars, leaving your',
	desc2 = 'physical body behind.',
	cost = 9
}

spells.selected = 1

function spells.update(room)
	if char.down('j') or key.down(key._down) then
		spells.selected = math.min(9, spells.selected + 1) 
	end
	if char.down('k') or key.down(key._up) then
		spells.selected = math.max(1, spells.selected - 1)
	end

	for i=1,9 do
		if char.down(tostring(i)) then
			spells.selected = i
		end
	end

	if char.down('\r') then
		spells.delegate = function()
			spells.cast(spells.selected, room)
		end
		return false
	end

	return true
end

function spells.render(tm)
	tm:push()
	local color_selected = rgba(0.2, 0.2, 0.2)
	local color_bg = rgba(0.1, 0.1, 0.1)
	tm.selected_bg = color_bg
	tm.selected_fg = rgba(0.8, 0.3, 0.2)
	tm:write_middle(3, '======== Spells ========')
	for i=1,9 do
		tm:write_middle(3 + i, string.format(
			'| %d %s%s |', i, spells[i].name, string.rep(' ', 18 - #spells[i].name)
		))
		if spells.selected == i then
			tm.selected_bg = color_selected
			tm:recolour((40 - 24)/2, 3 + i, 24)
			tm.selected_bg = color_bg
		end
	end
	tm:write_middle(13, '========================')
	tm:pop()
	tm:write(0,18, spells[spells.selected].desc)
	tm:write(0,19, spells[spells.selected].desc2)
end

function spells.cast(i, room)
	assert(i >= 1 and i <= 9)
	--if spells[i].have then
		if timeline.current <= spells[i].cost then
			timeline.text = string.format('Need %s seconds to cast %s',
				spells[i].cost, spells[i].name
			)
			return
		end

		room.spell = spells[i]
		room.spell_t = time.s()
		if spells[i].pre then
			local player = nil
			for i,o in ipairs(room.objs) do
				if o.char == '@' then
					player = o
				end
			end
			spells[i].pre(player, room)
		end
	--end
end

return spells
