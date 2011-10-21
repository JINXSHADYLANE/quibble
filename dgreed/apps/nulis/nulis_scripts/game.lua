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
	return not key.down(key.quit) 
end

function render(t)
	return true
end
