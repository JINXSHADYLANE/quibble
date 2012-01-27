module(..., package.seeall)

function init()
end

function close()
end

function enter()
end

function leave()
end

function update()
	if key.down(key.quit) then
		states.pop()
	end

	return true
end

function render(t)
	return true
end

