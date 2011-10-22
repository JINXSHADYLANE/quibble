module(..., package.seeall)

linear_damping = 0.995
angular_damping = 0.99

collide_force = 0.01
spawn_force = 0.05
spawn_torque = 0.01

particle_radius = 16

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
		return o
	end,

	render = function(self) 
	end,

	centers = function(self)
		if self.mass == 2 then
			local d = rotate(vec2(self.radius, 0), self.angle) 
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

function init(width, height, cell_width, cell_height)
	w, h, cw, ch = width, height, cell_width, cell_height
	nw, nh = w / cw, h / ch

	-- some sanity checks
	if math.floor(nw) ~= nw or math.floor(nh) ~= nh then
		log.error('Sim grid cells have wrong size')
	end
	if math.fmod(w, cw) ~= 0 or math.fmod(h, ch) ~= 0 then
		log.error('Something is wrong with your sim cell dimensions')
	end

	-- create list of all particles and cell lists, precalc cell rects
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
	if cx < 0 or cx >= w or cy < 0 or cy >= h then
		print(debug.traceback())
		log.error('Pt is out of bounds')
	end

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
	for i,part in ipairs(cell[cid]) do
		if part == p then
			print(debug.traceback())
			log.error('p already in cell!')
		end
	end

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

function tick()
	-- iterate over all particles
	local i = 1
	while i <= #all do
		local p = all[i]

		if p.remove then
			-- remove by copying last element over this
			all[i] = all[#all]
			table.remove(all)
		else
			-- update position, rotation
			p.center = p.center + p.vel * time.dt()
			p.angle = p.angle + p.ang_vel * time.dt()

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
			p.angle = p.angle * angular_damping

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
								if pa.mass + pb.mass == 2 then
									add(sim.particle:new({
										center = (pa.center + pb.center) / 2,
										vel = pa.vel + pb.vel,
										color = pa.color,
										mass = 2,
										angle = math.atan2(d.y, d.x), 
										ang_vel = rand.float(-0.001, 0.001),
										render = pa.render
									}))
								end
								break
							else
								-- different color, do something bad
								local d = normalize(pa.center - pb.center)
								if pa.mass + pb.mass == 2 then
									pa.vel = pa.vel + d * collide_force
									pb.vel = pb.vel - d * collide_force
								else
									local c = (pa.center + pb.center) / 2
									local rot = rand.float(0, math.pi * 2)

									pa.remove = true
									pb.remove = true

									for i=1,4 do
										rot = rot + math.pi/2
										local dp = vec2(
											math.cos(rot), 
											math.sin(rot)
										)
										local p = c + dp * (particle_radius*1.5)
										add(sim.particle:new({
											center = p,
											vel = dp * spawn_force,
											color = rand.int(1, 3),
											render = pa.render
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


