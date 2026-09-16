local _M = {
	VERSION_REQUIRED="1.0.0",
	shouldStop = false,
}
local input = require("input")

function _M.onSetup()
	setPanelManaged(false)
	print("vamo")
	_M.wah = {}
	_M.wah[1] = decodePng("/scripts/wah3.png")
	_M.wah[2] = decodePng("/scripts/wah2.png")
	_M.wah[3] = decodePng("/scripts/wah1.png")
	_M.wah[4] = _M.wah[2]
	_M.restart()

end

function _M.restart()
	_M.timer = 1000
	_M.frame = 1
	_M.y = 0
	_M.x = 10
	_M.vy = 70
	_M.grounded = false
	_M.gameOver = false
	oledClearScreen()
	oledDisplay()

	_M.keks = {}
	_M.stars = {}
	_M.keks[1] = 63

	for i=1,7 do
		_M.stars[#_M.stars+1] = {x = math.random(1,63), y=math.random(1,30)}
	end
end

function _M.drawText(x,y, str, color)
	for i=1,#str do
		drawPanelChar(x + (i-1)*6, y, string.byte(str:sub(i,i)), color, 0, 1)
	end
end

function _M.drawGameOver()
	local red = color565(255, 40, 40)
	local white = color565(255, 255, 255)
	_M.drawText(20, 3, "GAME", red)
	_M.drawText(84, 3, "GAME", red)
	_M.drawText(20, 12, "OVER", red)
	_M.drawText(84, 12, "OVER", red)
	_M.drawText(8, 22, "UP OR OK", white)
	_M.drawText(72, 22, "UP OR OK", white)
	oledClearScreen()
	oledSetFontSize(1)
	oledSetCursor(37, 7)
	oledDrawText("GAME OVER")
	oledSetCursor(40, 19)
	oledDrawText("UP OR OK")
	oledDisplay()
end

function _M.starDraw(x,y)
	drawPanelLine(127-x, y-1, 127-x, y+1, color565(40, 100, 210))
	drawPanelLine(127-x-1, y, 127-x+1, y, color565(40, 100, 210))

	drawPanelLine(x, y-1, x, y+1, color565(40, 100, 210))
	drawPanelLine(x-1, y, x+1, y, color565(40, 100, 210))

	drawPanelPixel(x,y, color565(255, 255, 255))
	drawPanelPixel(127-x,y, color565(255, 255, 255))
end

function _M.kektus(x,y)
	drawPanelLine(127-x,y,127-x,y-7, color565(190, 255, 120))
	drawPanelLine(x,y,x,y-7, color565(190, 255, 120))
	oledDrawLine(x,y,x,y-7, 1)
	drawPanelLine(x,y-2,x-2,y-7, color565(190, 255, 120))
	drawPanelLine(127-x,y-2,127-x-2,y-7, color565(190, 255, 120))
	oledDrawLine(x,y-2,x-2,y-7, 1)
	drawPanelLine(x,y-2,x+3,y-7, color565(190, 255, 120))
	drawPanelLine(127-x,y-2,127-x+3,y-7, color565(190, 255, 120))
	oledDrawLine(x,y-2,x+3,y-7, 1)
end

function _M.onLoop(dt)
	clearPanelBuffer()
	if input.readButtonStatus(BUTTON_BACK) == BUTTON_JUST_PRESSED or (input.readButtonStatus(BUTTON_CONFIRM) == BUTTON_PRESSED and input.readButtonStatus(BUTTON_LEFT) == BUTTON_PRESSED and input.readButtonStatus(BUTTON_RIGHT) == BUTTON_PRESSED and input.readButtonStatus(BUTTON_UP) == BUTTON_PRESSED and input.readButtonStatus(BUTTON_DOWN) == BUTTON_PRESSED) then
		_M.shouldStop = true
		return true
	end

	if _M.gameOver then
		_M.drawGameOver()
		flipPanelBuffer()
		if input.readButtonStatus(BUTTON_UP) == BUTTON_JUST_PRESSED or input.readButtonStatus(BUTTON_CONFIRM) == BUTTON_JUST_PRESSED then
			_M.restart()
		end
		return
	end

	local pix = 1
	local wahframe = _M.wah[_M.frame]



	_M.timer = _M.timer  - dt 

	if (_M.timer < 0) then 
		_M.timer = 200
		_M.frame = _M.frame +1
		if (_M.frame > 4) then 
			_M.frame = 1
		end
		if math.random(0, 1000) <= 100 and #_M.keks <= 1 then 
			_M.keks[#_M.keks+1] = 63
		end

		if math.random(0, 1000) <= 500 then 
			_M.stars[#_M.stars+1] = {x = 63, y=math.random(1,30)}
		end
	end
	if not _M.grounded then 
		_M.frame = 1
	end
	dt = dt/1000;

	if input.readButtonStatus(BUTTON_UP) == BUTTON_JUST_PRESSED and _M.grounded then 
		_M.vy = -40
		toneDuration(1440, 200)
		_M.grounded = false
	end

	_M.vy = _M.vy + 70*dt

	_M.y = _M.y + _M.vy*dt;
	if _M.y >= 32-15 then  
		_M.y = 32-15
		_M.grounded = true
	end

	drawPanelLine(0,31,127,31, color565(0, 255, 0))

	oledDrawRect(0,0,65, 33, 1)
	oledDrawLine(0, 31, 63, 31, 1)
	for i=30,5,-1 do 
		local range = math.min(1, math.max(0, (i-5)/25))
		drawPanelLine(0,i,127,i, color565( 0, 0, range * 255))
	end

	local stars = _M.stars
	local rm = -1
	for i,b in pairs(stars) do 
		b.x = b.x - ((b.y)*6) * dt
		_M.starDraw(math.floor(b.x),b.y)
		if (b.x <= 0) then 
			rm = i
		end
	end
	if rm ~= -1 then 
		table.remove(stars, rm)
	end

	for y=1, 13 do 
		for x=1, 23 do  
			if wahframe[pix] ~= 0 then 
				drawPanelPixel(_M.x+x, y+_M.y, wahframe[pix])
				drawPanelPixel((-_M.x)+127-x, y+_M.y, wahframe[pix])
				oledDrawPixel(_M.x+x+1,y+0+_M.y, 1)
			end
			pix = pix+1
		end
	end
	rm = -1
	local keks = _M.keks
	for i,b in pairs(keks) do 
		keks[i] = keks[i] - 30 * dt
		_M.kektus(math.floor(keks[i]),30)
		if keks[i] >= _M.x + 6 and keks[i] <= _M.x + 18 and _M.y + 13 >= 23 then
			_M.gameOver = true
		end
		if (keks[i] <= 0) then 
			rm = i
		end
	end
	if rm ~= -1 then 
		table.remove(keks, rm)
	end


	flipPanelBuffer()
end 

function _M.onClose()
	_M.wah = nil
	collectgarbage()
end

return _M