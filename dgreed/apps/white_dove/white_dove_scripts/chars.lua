-- All charaters except main boy are described here

mage = 1
math = 2
girl = 3
other = 0

chars = {
	[1] = {
		src = rect(0, 0, 179, 395),
		pos = rect(800, 200, 979, 575)
	},
	[2] = {
		src = rect(179, 0, 358, 395),
		pos = rect(300, 250, 479, 625)
	},
	[3] = {
		src = rect(333, 0, 512, 395),
		pos = rect(800, 200, 979, 595)
	},

	tex = nil
}

function chars.init()
	chars.tex = tex.load(media.."chars.png")
end

function chars.close()
	tex.free(chars.tex)
end

function chars.draw(id)
	video.draw_rect(chars.tex, 1, chars[id].src, chars[id].pos)
end


