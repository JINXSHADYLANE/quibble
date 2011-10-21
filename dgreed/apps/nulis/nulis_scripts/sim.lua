module(..., package.seeall)

linear_damping = 0.99
angular_damping = 0.99

particle = {
	center = vec2(),
	angle = 0,
	radius = 0,
	mass = 0,
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

	collide = function(self, p)
		local d = length_sq(self.center - p.center)
		return d < self.radius + p.radius
	end
}

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
	local sx = cxp + cx + cxn

	local cxc = cxp
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
			(sx - cx - cxr) + cyr + 1
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
	-- get cell id
	local cid = point_to_cell(p.center) 	

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
			p.center.x = math.fmod(p.center.x, w)
			p.center.y = math.fmod(p.center.y, h)
			p.angle = math.fmod(p.angle, 2 * math.pi)

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

	-- remove particles from old grid cells 
	for y=0,nh-1 do
		for x=0,nw-1 do
			local cid = x + y * nw + 1
			local c = cell[cid]
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


