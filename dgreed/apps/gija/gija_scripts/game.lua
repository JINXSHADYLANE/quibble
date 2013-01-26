local game = {}

local tmap0 = nil
local tmap1 = nil
local objs = nil

local camera = {
	center = vec2()
}

local obj_type = {
	start0 = 0,
	start1 = 1,
	finish = 2
}

function game.init()
	game.reset()
end

function game.reset(level_name)
	tmap0 = nil
	tmap1 = nil
	objs = nil

	if level_name == nil then
		level_name = 'level1'
	end

	tmap0 = tilemap.load(pre..level_name..'.0.btm')
	tmap1 = tilemap.load(pre..level_name..'.1.btm')
	objs = tilemap.objects(tmap0)

	-- set up objects
	for i,obj in ipairs(objs) do
		print(obj.id, obj.pos)
		if obj.id == obj_type.start0 then
			camera.center = vec2(obj.pos)
		end
	end
end

function game.close()
	if tmap0 and tmap1 then
		tilemap.free(tmap0)
		tilemap.free(tmap1)
	end
end

function game.update_camera()
	local pos, scale, rot = tilemap.camera(tmap0)
	--local d = camera.center - cat.p
	local d = vec2(0, 0)
	camera.center = camera.center - d * 0.05
	pos = vec2(camera.center)
	tilemap.set_camera(tmap0, pos, scale, rot)
	tilemap.set_camera(tmap1, pos, scale, rot)
end

function game.update()
	local ts = time.s()

	sound.update()

	game.update_camera()

	return not key.down(key.quit)
end

function game.render(t)
	local ts = time.s()

	sprsheet.draw('empty', 0, scr_rect)

	if tmap0 then
		tilemap.render(tmap1, scr_rect)
	end

	return true
end

return game

