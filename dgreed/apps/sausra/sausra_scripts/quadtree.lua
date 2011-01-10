
Quadtree = {}
Quadtree_mt = {}

-- splits rect into four
function split(r)
	local mx, my = r.l+width(r)/2, r.t+height(r)/2
	local res = {}
	res[1] = rect(r.l, r.t, mx, my)
	res[2] = rect(mx, r.t, r.r, my)
	res[3] = rect(r.l, my, mx, r.b)
	res[4] = rect(mx, my, r.r, r.b)
	return res
end

-- builds quadtree from list of objects
-- each object must have field 'rect'
function Quadtree:new(objs)
	-- tweakable values
	local maxdepth = 8
	local maxobjs = 4

	-- calc bounding box 
	local bbox = rect(objs[1].rect)
	for i, obj in ipairs(objs) do
		bbox.l = math.min(bbox.l, obj.rect.l)
		bbox.t = math.min(bbox.t, obj.rect.t)
		bbox.r = math.max(bbox.r, obj.rect.r)
		bbox.b = math.max(bbox.b, obj.rect.b)
	end
	-- recursive subdivision 
	local function subdiv(r, objs, depth)
		if objs == nil then
			return nil
		end
		if depth >= maxdepth or #objs <= maxobjs then
			return objs
		end
		
		local rects = split(r)
		local nobjs = {{}, {}, {}, {}}
		
		-- build lists of objects in each bin
		for i, obj in ipairs(objs) do
			for j = 1,4 do
				if rect_rect_collision(obj.rect, rects[j]) then 
					table.insert(nobjs[j], obj)
				end
			end
		end

		-- recursively build nodes
		local res = {}
		for i = 1,4 do
			res[i] = subdiv(rects[i], nobjs[i], depth+1) 	
		end
		return res
	end

	local root = subdiv(bbox, objs, 0)
	root.bbox = bbox
	setmetatable(root, Quadtree_mt)
	return root
end

-- returns list of objects which intersect rectangle qr,
function Quadtree:query(qr)
	-- recursive query
	local function rquery(r, node) 
		if #node ~= 4 or node[1].rect then
			return node
		end
		local rects = split(r)
		local res = {}
		for i = 1,4 do
			if node[i] ~= nil and rect_rect_collision(qr, rects[i]) then
				local nobjs = rquery(rects[i], node[i])
				for j, obj in ipairs(nobjs) do
					if rect_rect_collision(qr, obj.rect) then
						-- look for duplicates
						local good = true
						for k=1,#res do
							if res[k] == obj then
								good = false
								break
							end
						end
						if good then
							table.insert(res, obj)
						end
					end
				end
			end
		end
		return res
	end
	local res = rquery(self.bbox, self)
	return res 
end

Quadtree_mt.__index = Quadtree

