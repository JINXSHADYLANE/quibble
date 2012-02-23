
dofile(src..'quadtree.lua')

game = {}

world_rect = rect(0, 0, 2880, 1920)

hero = {
        src = {
                [1] = rect(256-48, 0, 256-40, 12),
                [2] = rect(256-40, 0, 256-32, 12),
                [3] = rect(256-32, 0, 256-24, 12),
                [4] = rect(256-24, 0, 256-16, 12),
                [5] = rect(256-16, 0, 256-8, 12),
                [6] = rect(256-8, 0, 256, 12),
        },      
        frame = 0,
        
        p = vec2(),
        v = vec2(),
        dir = false,
        ground = false,
        savept = nil,

        -- tweaks
        move_acc = 0.8,
        move_damp = 0.84,
        jump_acc = 8,
        gravity = 0.5,
        height = 12,
        width = 8,
}

-- object rects in atlas
obj_src = {
        [2] = rect(0, 256-16, 16, 256),
        [5] = rect(16, 256-16, 16+16, 256),
        [3] = rect(0, 256-32, 16, 256-16),
        [4] = rect(16, 256-32, 16+16, 256-16),
        [60] = rect(0, 256-48, 16, 256-32),
        [6] = rect(0, 256-64, 16, 256-48),
}       

endscreen = {
        start_time = 0
}       

snd = {}

mode = {
        normal = true
}
        
camera_t = vec2(0.5, 0.5) -- normalized camera coord, for background scrolling

function game.init()
        snd.footsteps = sound.load_sample(pre..'footsteps.wav')
        snd.jump = sound.load_sample(pre..'jump.wav')
        sound.set_volume(snd.jump, 0.5)
        snd.death = sound.load_sample(pre..'death.wav')

        back = tex.load(pre..'background.png')
        atlas = tex.load(pre..'atlas.png')
        fnt = font.load(pre..'lucida_grande_60px.bft', 0.5)
        back_size = tex.size(back) * 3
        back_factor = back_size - vec2(width(screen), height(screen))

        mode.map = tilemap.load(pre..'mode.btm')
        mode.active = true
end

function mode.draw()
        video.draw_text_centered(fnt, 3, 'How tough are you?', vec2(241, 51))
        local o = tilemap.objects(mode.map)
        tilemap.set_camera(mode.map, o[1].pos + vec2(8, 8))
        tilemap.render(mode.map, screen)
        local p1 = tilemap.world2screen(mode.map, screen, o[2].pos + vec2(8, 8))
        local p2 = tilemap.world2screen(mode.map, screen, o[3].pos + vec2(8, 8))
        if mouse.down(mouse.primary) or key.down(key._left) or key.down(key._right) then
                if length(mouse.pos() - p1) < 30 or key.down(key._left) then
                        mode.normal = false 
                        mode.active = false
                        game.reset()
                        sound.play(music, true)
                end
                if length(mouse.pos() - p2) < 30 or key.down(key._right) then
                        mode.normal = true      
                        mode.active = false
                        game.reset()
                        sound.play(music, true)
                end
        end
end

function game.reset()
        hero.v = vec2() 
        hero.savept = nil
        hero.ground = false
        endscreen.active = false
        endscreen.deaths = 0
        if not mode.normal then
                level = tilemap.load(pre..'world_easy.btm')
        else
                level = tilemap.load(pre..'world.btm')
        end
        objects = tilemap.objects(level)
        objs = {}
        for i = 1,#objects do
                -- center camera on start object, which has id 0
                if objects[i].id == 0 then 
                        camera_t = vec2()
                        camera_pos = objects[i].pos
                        hero.p = objects[i].pos + vec2(0, -4)
                        local r = rect(objects[i].pos.x, objects[i].pos.y)
                        r.r, r.b = r.l+16, r.t+16
                        table.insert(objs, {rect=r, id=6})
                end
                if objects[i].id > 0 then
                        local r = rect(objects[i].pos.x, objects[i].pos.y)
                        r.r, r.b = r.l+16, r.t+16
                        table.insert(objs, {rect=r, id=objects[i].id})
                end
        end
        log.info(tostring(#objs)..' objs put into quadtree')
        qtree = Quadtree:new(objs)
        endscreen.start_time = time.s()

        snd.ft = sound.play(snd.footsteps, true)
        sound.set_src_volume(snd.ft, 0.0)
end

function game.set_camera(pos)
        local cam_pos = vec2(math.floor(pos.x), math.floor(pos.y))
        tilemap.set_camera(level, cam_pos)
        camera_t.x = pos.x / world_rect.r
        camera_t.y = pos.y / world_rect.b
end     

function game.close()
        tex.free(back)
        tex.free(atlas)
        if level then
                tilemap.free(level)
        end
        tilemap.free(mode.map)
end

function hero.update() 
        if key.pressed(key._left) then
                hero.v.x = hero.v.x - hero.move_acc
                hero.dir = true
        end
        if key.pressed(key._right) then
                hero.v.x = hero.v.x + hero.move_acc
                hero.dir = false
        end     
        if (key.down(key._up) or key.down(key.a)) and hero.ground then
                hero.ground = false
                hero.v.y = -hero.jump_acc
                sound.play(snd.jump)
        end
        hero.v.y = hero.v.y + hero.gravity
        hero.v.x = hero.v.x * hero.move_damp

        local bbox = rect(
                hero.p.x + hero.width/2,
                hero.p.y + hero.height/2
        )
        bbox.r = bbox.l + hero.width 
        bbox.b = bbox.t + hero.height

        dx = tilemap.collide_swept(level, bbox, vec2(hero.v.x, 0))
        hero.p = hero.p + dx
        bbox.l = bbox.l + dx.x
        bbox.r = bbox.r + dx.x
        hero.v.x = dx.x
        hero.frame = hero.frame + math.abs(dx.x)/10
        if math.floor(hero.frame) >= 6 then
                hero.frame = hero.frame - 6
        end
        local vol = math.abs(hero.v.x) / 5
        if not hero.ground then
                vol = 0
        end
        sound.set_src_volume(snd.ft, clamp(0, 0.5, vol)) 

        dy = tilemap.collide_swept(level, bbox, vec2(0, hero.v.y))
        if hero.v.y > 0 then
                hero.ground = dy.y == 0
        end
        hero.p = hero.p + dy
        hero.v.y = dy.y
        bbox.t = bbox.t + dy.y
        bbox.b = bbox.b + dy.y
        hero.bbox = bbox

        local colobjs = qtree:query(hero.bbox)
        for i, obj in ipairs(colobjs) do
                -- end game
                if obj.id == 1 then
                        sound.set_src_volume(snd.ft, 0)
                        endscreen.end_time = time.s()   
                        endscreen.active = true
                end

                -- save point
                if obj.id == 6 and hero.savept ~= obj then
                        hero.savept = obj
                end

                -- spikes
                if obj.id >= 2 and obj.id < 6 then
                        local r = rect(obj.rect)
                        local s = width(r) / 2

                        -- modify hitbox - up spike uses only lower half of tile, etc.
                        if obj.id == 2 then
                                r.t = r.b - s 
                        end
                        if obj.id == 3 then
                                r.b = r.t + s
                        end

                        -- actual collision check
                        if rect_rect_collision(r, hero.bbox) then
                                sound.play(snd.death)
                                endscreen.deaths = endscreen.deaths+1
                                hero.v = vec2(0, 0)
                                hero.p = vec2(hero.savept.rect.l, hero.savept.rect.t-4)
                        end     
                end
        end
end

function hero.draw()
        local b = tilemap.world2screen(level, screen, hero.bbox)
        local f = math.floor(hero.frame)
        if f == 0 or math.abs(hero.v.x) < 0.01 then
                f = 1
        end
        if not hero.ground then
                f = 6
        end
        if hero.dir then
                b.l, b.r = b.r, b.l
        end
        video.draw_rect(atlas, 2, hero.src[f], b)
end

function endscreen.draw()
        local cursor = vec2(10, 10)
        video.draw_text(fnt, 3, 'Well done!', cursor)
        cursor.y = cursor.y + 50
        video.draw_text(fnt, 3, 'deaths - '..tostring(endscreen.deaths), cursor)
        cursor.y = cursor.y + 30
        local t = math.floor(endscreen.end_time - endscreen.start_time)
        local min, sec = math.modf(t / 60)
        sec = math.fmod(t, 60)
        local secstr = tostring(sec)
        if #secstr == 1 then
                secstr = '0'..secstr
        end
        video.draw_text(fnt, 3, 'time - '..tostring(min)..':'..secstr, cursor)
        cursor.y = cursor.y + 150
        if normal then
                video.draw_text(fnt, 3, 'You deserve a cup of tea!', cursor)
        else
                video.draw_text(fnt, 3, 'Go play normal mode now.', cursor)
        end
end

function game.frame()
        -- show title text
        local tm = time.s() - endscreen.start_time
        if not mode.active and tm > 1 and tm < 5 then
                local t = (tm - 1) / 4  
                local t2 = math.sin(t * math.pi)
                local c1, c2 = rgba(0,0,0,0), rgba(0,0,0,1)
                local c = lerp(c1, c2, t2)
                video.draw_text(fnt, 3, 'sausra', vec2(30 + t * 50, 20), c)
        end

        -- calculate and draw background
        local back_dest = rect()
        back_dest.l = camera_t.x * -back_factor.x
        back_dest.t = camera_t.y * -back_factor.y
        back_dest.r = back_dest.l + back_size.x
        back_dest.b = back_dest.t + back_size.y
        video.draw_rect(back, 0, back_dest)

        -- scroll camera
        if not mode.active then
                local d = camera_pos - hero.p
                camera_pos.x = camera_pos.x - d.x * 0.1
                camera_pos.y = camera_pos.y - d.y * 0.03
                game.set_camera(camera_pos)
        end

        -- mode selection screen
        if mode.active then
                mode.draw()
                return
        end

        -- draw tilemap
        tilemap.render(level, screen)

        -- draw objects
        local world_screen = tilemap.screen2world(level, screen, screen)
        local vis_objs = qtree:query(world_screen)
        for i, obj in ipairs(vis_objs) do
                if obj_src[obj.id] ~= nil then
                        local b = tilemap.world2screen(level, screen, obj.rect) 
                        local p = vec2(math.floor(b.l), math.floor(b.t))
                        local src = obj_src[obj.id]
                        if obj.id == 6 and hero.savept == obj then
                                src = obj_src[60]
                        end
                        video.draw_rect(atlas, 2, src, p)
                end     
        end

        if not endscreen.active then
                hero.update()
        else
                endscreen.draw()
        end

        hero.draw()
end

