local objects = {}

--[[
object is a table with some data and some functions

data:
	id - string
	pos - vec2
	sprite - sprhandle
	collision - cdobjhandle
	removed - bool

functions:
	update(self, time)
	collide_obj(self, obj)
	collide_wall(self)
	create(self, cdhandle) - responsible for coldet shape initialization
	destroy(self, cdhandle)
	
]]--


function objects.close()
	if objects.cd then
		for i,obj in ipairs(objects.list) do
			coldet.remove(obj.collision)
		end
		coldet.close(objects.cd)
	end
end

function objects.reset()
	objects.close()

	objects.list = {}
	objects.to_add = {}
	objects.player = nil

	objects.cd = coldet.init(64, level_size.x, level_size.y)
end

function objects.add(obj)
	assert(obj.sprite)

	table.insert(objects.to_add, obj)

	-- save ref to player for easy access
	if obj.id == 'player' then
		assert(not objects.player)
		objects.player = obj
	end
end

function objects.update()
	
	-- invoke update
	for i,obj in ipairs(objects.list) do
		local old_pos = vec2(obj.pos)
		if obj.update then
			obj.update(obj, states.time())
		end
	end

	-- process collisions
	coldet.process(objects.cd, function(ao, bo)
		local a = cdobj.userdata(ao)
		local b = cdobj.userdata(bo)
		if not a.remove and not b.remove then
			if a.collide_obj then
				a.collide_obj(a, b)
			end
			if b.collide_obj then
				b.collide_obj(b, a)
			end
		end
	end)

	-- remove dead objects
	local new_objs = {}
	for i,obj in ipairs(objects.list) do
		if not obj.remove then
			table.insert(new_objs, obj)
		else
			if obj.destroy then
				obj.destroy(obj)
			end
		end
	end
	objects.list = new_objs

	-- add new objects
	for i,obj in ipairs(objects.to_add) do
		if obj.create then
			obj.create(obj, objects.cd)
			cdobj.set_userdata(obj.collision, obj)
		end

		table.insert(objects.list, obj)
	end

end

function objects.render(camera, layer)
	local expanded_camera = rect(
		camera.l - 20, camera.t - 20,
		camera.r + 20, camera.b + 20
	)

	coldet.query_aabb(objects.cd, expanded_camera, function(o)
		local obj = cdobj.userdata(o)
		local pos = world2screen(obj.pos, camera)
		sprsheet.draw_centered(obj.sprite, layer, pos)
	end)
end

return objects
