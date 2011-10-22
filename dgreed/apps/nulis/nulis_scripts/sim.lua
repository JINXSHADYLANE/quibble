module(..., package.seeall)

require 'effects'

linear_damping = 0.995
angular_damping = 0.99

collide_force = 0.01
spawn_force = 0.05
spawn_torque = 0.01

particle_radius = 16

ghost_lifetime = 1.8

random_spawn_limit = 20

particle = {
	center = vec2(),
	angle = 0,
	radius = particle_radius,
	mass = 1,
	vel = vec2(),
	ang_vel = 0,
	remove = false,

	new = function(self, o)
		o = o or {}
		setmetatable(o, self)
		self.__index = self
		o.in_cols = {rand.float(), rand.float()}
		o.in_grad = rand.int(1, 10)
		o.in_rot = {rand.float(-1, 1), rand.float(-1, 1)}
		o.in_t = 0
		return o
	end,

	centers = function(self)
		if self.mass == 2 then
			local d = rotate(vec2(self.radius*0.75, 0), self.angle) 
			return {self.center + d, self.center - d}
		end
		return {self.center}
	end,

	collide = function(self, p)
		if self.mass == 1 and p.mass == 1 then
			local d, l = path(self.center, p.center) 
			local r = self.radius + p.radius
			return l < r*r 
		end

		local ac = self:centers()
		local bc = p:centers()

		for i,a in ipairs(ac) do
			for j,b in ipairs(bc) do
				local d, l = path(a, b)
				local r = particle_radius * 2
				if l < r*r then
					return true
				end
			end
		end
		return false
	end
}

-- shortest path between a and b in a toroidal space
path_offsets = nil
function path(a, b)
	local min_path = a - b
	local min_d = length_sq(min_path)
	for i,off in ipairs(path_offsets) do
		local p = a + off - b
		local d = length_sq(p)
		if d < min_d then
			min_d = d
			min_path = p
		end
	end
	return min_path, min_d
end

function init(width, height, cell_width, cell_height, rand_t, rand_start)
	w, h, cw, ch = width, height, cell_width, cell_height
	nw, nh = w / cw, h / ch

	last_random_t = time.s()
	random_t = rand_t
	random_spawn_limit = rand_start

	-- some sanity checks
	if math.floor(nw) ~= nw or math.floor(nh) ~= nh then
		log.error('Sim grid cells have wrong size')
	end
	if math.fmod(w, cw) ~= 0 or math.fmod(h, ch) ~= 0 then
		log.error('Something is wrong with your sim cell dimensions')
	end

	-- create list of all particles and cell lists, precalc cell rects
	ghosts = {}
	all = {}
	cell = {}
	cell_rects = {}
	for y = 0,nh-1 do
		for x = 0,nw-1 do
			local cid = x + y * nw + 1
			local r = rect(x * cw, y * ch, (x+1) * cw, (y+1) * ch)
			cell[cid] = {}
			cell_rects[cid] = r
		end
	end

	path_offsets = {
		vec2(w, 0),
		vec2(0, -h),
		vec2(-w, 0),
		vec2(0, h)
	}
end

function point_to_cell(pt)
	local cx = math.floor(pt.x / cw)
	local cy = math.floor(pt.y / ch)

	-- sanity check, comment out for performance
	--if cx < 0 or cx >= w or cy < 0 or cy >= h then
	--	print(debug.traceback())
	--	log.error('Pt is out of bounds')
	--end

	return cx + cy * nw + 1
end

function cell_neighbours(cid)
	-- let's think about efficiency here
	
	local cx = math.fmod(cid-1, nw)
	local cy = math.floor((cid-1) / nw)

	local cxp = math.fmod(nw + cx - 1, nw)
	local cxn = math.fmod(cx + 1, nw)
	local cyp = math.fmod(nh + cy - 1, nh) * nw
	local cyn = math.fmod(cy + 1, nh) * nw

	cy = cy * nw

	return {
		cxp + cyp + 1,
		cx + cyp + 1,
		cxn + cyp + 1,
		cxp + cy + 1,
		cxn + cy + 1,
		cxp + cyn + 1,
		cx + cyn + 1,
		cxn + cyn + 1
	}
end

function cell_neighbours_movement(cid, delta)
	-- physicists write code like this...

	local out = {}	
	local cx = math.fmod(cid-1, nw)
	local cy = math.floor((cid-1) / nw)

	local cxp = math.fmod(nw + cx - 1, nw)
	local cxn = math.fmod(cx + 1, nw)
	local cyp = math.fmod(nh + cy - 1, nh) * nw
	local cyn = math.fmod(cy + 1, nh) * nw

	cy = cy * nw
	local sx = cxp + cxn

	local cxr = cxp
	if delta.x > 0 then
		cxr = cxn
	end

	local cyr = cyp
	if delta.y > 0 then
		cyr = cyn
	end

	local new_col = delta.x ~= 0 
	local new_row = delta.y ~= 0

	if new_row and new_col then
		return {
			cxr + cyp + 1,
			cxr + cy + 1,
			cxr + cyn + 1,
			cx + cyr + 1,
			(sx - cxr) + cyr + 1
		}
	end
	if new_row then
		return {
			cxp + cyr + 1,
			cx + cyr + 1,
			cxn + cyr + 1
		}
	end
	if new_col then
		return {
			cxr + cyp + 1,
			cxr + cy + 1,
			cxr + cyn + 1
		}
	end

	return {}
end

function add_to_cell(cid, p)
	-- check if already added (disable later)
	--for i,part in ipairs(cell[cid]) do
	--	if part == p then
	--		print(debug.traceback())
	--		log.error('p already in cell!')
	--	end
	--end

	table.insert(cell[cid], p)
end

function add(p)
	-- wrap coords
	p.center.x = math.fmod(w + p.center.x, w)
	p.center.y = math.fmod(h + p.center.y, h)

	-- get cell id
	local cid = point_to_cell(p.center) 	

	-- check for collisions with existing particles
	for i,pt in ipairs(cell[cid]) do
		if not pt.remove and p:collide(pt) then
			return false
		end
	end

	-- get list of neighbour cell ids
	local n_cids = cell_neighbours(cid)

	-- add to center cell and neighbours, if there is intersection
	add_to_cell(cid, p)
	for i,n_cid in ipairs(n_cids) do
		if rect_circle_collision(cell_rects[n_cid], p.center, p.radius) then
			add_to_cell(n_cid, p)
		end
	end

	table.insert(all, p)
	return true
end

function spawn_random()
	local p = vec2(
		rand.float(0, w),
		rand.float(0, h)
	)

	local v = vec2(0, rand.float()/100)
	v = rotate(v, rand.float(0, math.pi * 2))

	local col = rand.int(1, 3)

	add(sim.particle:new({
		center = p,
		vel = v,
		color = col
	}))

	return p
end

function tick()
	update_ghosts()

	-- spawn random particles
	if #all < random_spawn_limit then
		if time.s() - last_random_t > random_t then
			last_random_t = time.s()
			local p = spawn_random()
			effects.spawn(p)
		end
	end

	-- iterate over all particles
	local i = 1
	while i <= #all do
		local p = all[i]

		if p.remove then
			-- remove by copying last element over this
			all[i] = all[#all]
			table.remove(all)
		else
			local dt = time.dt()

			-- update insides
			p.in_t = math.max(0, p.in_t - dt / 200)

			-- update position, rotation
			p.center = p.center + p.vel * dt
			p.angle = p.angle + p.ang_vel * dt

			-- wrap
			p.center.x = math.fmod(w + p.center.x, w)
			p.center.y = math.fmod(h + p.center.y, h)
			p.angle = math.fmod(
				2 * math.pi + p.angle, 
				2 * math.pi
			)

			-- add particle to new grid cells
			local cid = point_to_cell(p.center)
			new_cids = cell_neighbours_movement(cid, p.vel)
			for i,n_cid in ipairs(new_cids) do
				if rect_circle_collision(
					cell_rects[n_cid], p.center, p.radius) then
					local add = true
					for i,op in ipairs(cell[n_cid]) do
						if op == p then
							add = false
							break
						end
					end
					if add then
						add_to_cell(n_cid, p)
					end
				end
			end

			-- damp angular & linear velocities
			p.vel = p.vel * linear_damping
			p.ang_vel = p.ang_vel * angular_damping

			i = i + 1
		end
	end

	for y=0,nh-1 do
		for x=0,nw-1 do
			local cid = x + y * nw + 1
			local c = cell[cid]

			-- remove particles from old grid cells 
			local j = 1
			while j <= #c do
				local p = c[j]
				local hit = false
				if not p.remove then
					hit = rect_circle_collision(
						cell_rects[cid], p.center, p.radius
					)
				end

				if not hit then
					c[j] = c[#c]
					table.remove(c)
				else
					j = j + 1
				end
			end

			-- resolve collisions
			local j, k
			for j=1,#c do
				local pa = c[j]
				if not pa.remove then
					for k=j+1,#c do
						local pb = c[k]
						if not pb.remove and pa:collide(pb) then
							if pa.color == pb.color then
								-- same color
								pa.remove = true
								pb.remove = true
								local d = pa.center - pb.center
								local c = (pa.center + pb.center) / 2
								if pa.mass + pb.mass == 2 then
									effects.merge_two(c)
									add(particle:new({
										center = c,
										vel = pa.vel + pb.vel,
										color = pa.color,
										mass = 2,
										angle = math.atan2(d.y, d.x), 
										ang_vel = rand.float(-spawn_torque, spawn_torque),
									}))
								else
									effects.merge_three(c)
									-- create ghost
									local v = (pa.vel*pa.mass + pb.vel*pb.mass)
									table.insert(ghosts, particle:new({
										center = c,
										vel = v/3,
										color = pa.color,
										mass = 3,
										angle = math.atan2(d.y, d.x),
										ang_vel = rand.float(-spawn_torque, spawn_torque),
										death_time = time.s()
									}))

									-- additional real particle if merged 4
									if pa.mass + pb.mass == 4 then
										add(particle:new({
											center = c,
											vel = -v,
											color = pa.color
										}))
									end
								end

								break
							else
								-- different color, do something bad
								local d = normalize(pa.center - pb.center)
								local c = (pa.center + pb.center) / 2
								if pa.mass + pb.mass == 2 then
									effects.collide(c, math.atan2(d.y, d.x))
									pa.vel = pa.vel + d * collide_force
									pb.vel = pb.vel - d * collide_force
								else
									effects.create(c)
									local rot = rand.float(0, math.pi * 2)

									pa.remove = true
									pb.remove = true

									local n = 4
									local rf = 1.5
									if pa.mass + pb.mass == 4 then
										n = 6
										rf = 2.5
									end
									for i=1,n do
										rot = rot + (2*math.pi/n)
										local dp = vec2(
											math.cos(rot), 
											math.sin(rot)
										)
										local p = c + dp * (particle_radius*rf)
										add(particle:new({
											center = p,
											vel = dp * spawn_force,
											color = 1 + math.fmod(i, 2),
										}))
									end
									break
								end
							end
						end
					end
				end
			end
		end
	end
end

function update_ghosts()
	local t = time.s()
	local i = 1
	while i <= #ghosts do
		local p = ghosts[i]
		if t - p.death_time > ghost_lifetime then
			ghosts[i] = ghosts[#ghosts]
			table.remove(ghosts)
		else
			local dt = time.dt()

			-- update insides
			p.in_t = math.max(0, p.in_t - dt / 200)

			p.center = p.center + p.vel * dt
			p.angle = p.angle + p.ang_vel * dt

			-- wrap
			p.center.x = math.fmod(w + p.center.x, w)
			p.center.y = math.fmod(h + p.center.y, h)
			p.angle = math.fmod(
				2 * math.pi + p.angle, 
				2 * math.pi
			)

			-- damp
			p.vel = p.vel * linear_damping
			p.angle = p.angle * angular_damping

			i = i + 1
		end
	end
end

function draw_grid()
	local x = cw
	while x < w do
		video.draw_seg(1, vec2(x, 0), vec2(x, h))
		x = x + cw
	end

	local y = ch
	while y < h do
		video.draw_seg(1, vec2(0, y), vec2(w, y))
		y = y + ch
	end
end

-- returns all particles within distance r of point c
function query(c, r)
	local out = {}
	for i,p in ipairs(all) do
		local d, l = path(c, p.center)
		local ri = r + p.radius
		if l < ri*ri then
			table.insert(out, p)
		end
	end
	return out
end


