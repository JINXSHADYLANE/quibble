local spells = {}

spells[1] = {
	name = 'Push',
	desc = 'Invisible force pushes objects or',
	desc2 = 'enemies away from you.',
	cost = 1,
	have = true
}

spells[2] = {
	name = 'Fireball',
	desc = 'Conjure and throw a swirling',
	desc2 = 'orb of flame.',
	cost = 2,
	have = true
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

function spells.update()
	if char.down('j') or key.down(key._down) then
		spells.selected = math.min(9, spells.selected + 1) 
	end
	if char.down('k') or key.down(key._up) then
		spells.selected = math.max(1, spells.selected - 1)
	end

	for i=1,9 do
		if char.pressed(tostring(i)) then
			spells.selected = i
		end
	end
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

return spells
