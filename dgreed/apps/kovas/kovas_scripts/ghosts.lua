local ghosts = {}

local ghost = {}
ghost.__index = ghost

function ghost:new()
	local o = {
		pos = vec2(rand.float(-2000, 2000), rand.float(-2000, 2000)),
		vel = vec2(),
		frame = 1,
		blink = -100,
		angry = false
	}
	setmetatable(o, self)
	return o
end

function ghost:draw(camera)
	local w, h = width(screen_rect), height(screen_rect)
	local topleft = camera - vec2(w/2, h/2)

	local f = self.frame
	if self.angry then
		f = f + 2
	else
		if time.s() - self.blink < 1 then
			local t = (time.s() - self.blink)
			f = math.floor(t * 8)
		end
	end

	local p = self.pos - topleft
	sprsheet.draw_anim_centered('eyes', f, 15, p)
end

function ghosts.init()
	ghosts.list = {}
	for i=1,32 do
		table.insert(ghosts.list, ghost:new())
	end
end

function ghosts.draw(camera)
	for i,g in ipairs(ghosts.list) do
		g:draw(camera)
	end
end

function ghosts.update(camera, lights, player)
	local w, h = width(screen_rect), height(screen_rect)
	local topleft = camera - vec2(w/2, h/2)

	for i,g in ipairs(ghosts.list) do
		local dir = vec2()

		-- light pushes ghosts away
		for j,l in ipairs(lights) do
			local lp = l.pos + topleft
			if length(lp - g.pos) < 1.1 * l.radius then
				if rand.int(0, 3) == 2 then
					dir = dir + normalize(g.pos - lp) * 180
				end
			end
		end

		-- ghosts pushes ghosts away
		for j,gg in ipairs(ghosts.list) do
			if j ~= i and length(g.pos - gg.pos) < 50 then
				if rand.int(0, 4) == 2 then
					dir = dir + normalize(g.pos - gg.pos) * 60
				end
			end
		end

		-- player attracts ghosts
		if length(g.pos - player) > 400 then
			if rand.int(0, 43) == 42 then
				dir = dir + normalize(player - g.pos) * 800
			end
		end

		-- move just for the fun of it
		if dir.x == 0 and dir.y == 0 then
			-- in random direction
			if rand.int(0, 100) == 42 then
				dir = vec2(rand.float(), rand.float()) * 120
			end
			-- towards player
			if rand.int(0, 60) == 42 then
				dir = normalize(player - g.pos) * 200 
			end
		end

		-- blink
		if time.s() - g.blink > 1 then
			if rand.int(0, 250) == 42 then
				g.blink = time.s()
			end
		end

		-- perform move
		g.vel = g.vel + dir
		g.pos = g.pos + g.vel * time.dt()/1000
		g.vel = g.vel * 0.9
	end
end

return ghosts
