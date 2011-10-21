module(..., package.seeall)

require 'sim'

part_tex = nil
part_layer = 2

function init()
	part_tex = {
		tex.load(pre..'particle1.png'),
		tex.load(pre..'particle2.png')
	}

	sim.init(scr_size.x, scr_size.y, 64, 64)

	sim.add(sim.particle:new({
		center = vec2(scr_size.x / 2, scr_size.y / 2),
		vel = vec2(0.5, 0.5),
		radius = 32,
		mass = 10,

		render = function(self)
			video.draw_rect_centered(
				part_tex[1], part_layer, self.center 
			)
		end
	}))
end

function close()
	for i,t in ipairs(part_tex) do
		tex.free(t)
	end
end

function enter()
end

function leave()
end

function update()
	sim.tick()

	-- quit game on esc
	return not key.down(key.quit) 
end

function render(t)
	sim.draw_grid()
	for i,p in ipairs(sim.all) do
		p:render()
	end

	return true
end
