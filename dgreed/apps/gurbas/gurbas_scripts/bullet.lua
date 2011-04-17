bullet = {
	layer = 3,
	list = {},
	size = vec2(24, 24),
	last_t = 0,
	shoot_interval = 700,
	speed = 17,

	img = nil
}

function bullet.init() 
	bullet.img = tex.load(pre..'bullet.png')
end

function bullet.close()
	tex.free(bullet.img)
end

function bullet.reset()
	bullet.list = {}
end

function bullet.rect(p)
	return rect(
		p.x - bullet.size.x / 2,
		p.y - bullet.size.y / 2,
		p.x + bullet.size.x / 2,
		p.y + bullet.size.y / 2
	)	
end

function bullet.spawn()
	if time.ms() < bullet.last_t + bullet.shoot_interval then
		return	
	end

	local new_bullet = {
		pos = vec2(guy.p.x + guy.size.x/2, guy.p.y)
	}

	table.insert(bullet.list, new_bullet)
	bullet.last_t = time.ms()
end

function bullet.update()
	if key.pressed(key.a) then
		bullet.spawn()
	end

	local new_list = {}
	local remove_block = false
	for i,b in ipairs(bullet.list) do
		local remove = false
		b.pos.y = b.pos.y - bullet.speed
		local r = bullet.rect(b.pos)
		
		if well.collide_rect(r) then
			remove = true
		end		

		if block.collide_rect(r) then
			remove = true
			remove_block = true
		end

		if not remove then
			table.insert(new_list, b)
		end
	end
	bullet.list = new_list

	return remove_block
end

function bullet.draw()
	for i,b in ipairs(bullet.list) do
		local r = bullet.rect(b.pos)

		video.draw_rect(bullet.img, bullet.layer, rect(0, 0, 24, 24), r)
	end
end

