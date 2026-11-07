package.path = "lualib/?.lua;" .. package.path

local now = 1000
local file = assert(io.open("keybinds.json", "r"))
local configuration = require("json").decode(file:read("*a"))
file:close()
configuration.input.maxBleDevices = 2
if configuredDrivers then configuration.input.drivers = configuredDrivers end
package.loaded.configloader = {
    Get = function() return configuration end,
}
configloader = package.loaded.configloader
function millis() return now end
function log(message)
    assert(not message:find("[KEYBIND] Skipped", 1, true), message)
    print(message)
end
function hasBLEStarted() return true end
function setScanModeByAddress() end
function setLogDiscoveredBleDevices() end
function startBLE() end
function setMaximumControls(maximum) assert(maximum == 2) end
function startBLERadio() end
local scanning = false
function beginBleScanning() scanning = true end
generic = {displaySplashMessage = function() end}

local handlers = {}
function BleServiceHandler(uuid)
    local handler = {uuid = uuid}
    function handler:SetOnConnectCallback(callback) self.onConnect = callback end
    function handler:SetOnDisconnectCallback(callback) self.onDisconnect = callback end
    function handler:SetEncryptionRequired(required) self.encryptionRequired = required end
    function handler:AddCharacteristics(characteristic)
        self.characteristic = characteristic
        local listener = {}
        function listener:SetSubscribeCallback(callback) handler.onPacket = callback end
        function listener:SetCallbackModeStream(stream) handler.stream = stream end
        return listener
    end
    handlers[uuid] = handler
    return handler
end

local originalDofile = dofile
function dofile(path)
    return originalDofile(path:gsub("^/lualib/", "lualib/"))
end

drivers = require("drivers")
local input = require("input")
input.Load()
assert(drivers.loaded.SelfieCom, "SelfieCom was skipped: its hid core driver must be enabled")
input.Start()
assert(scanning, "Input startup must reach BLE scanning")

local handler = handlers["00001812-0000-1000-8000-00805f9b34fb"]
assert(handler and handler.characteristic == "2a4d", "HID report subscription was not registered")
assert(handler.encryptionRequired == true, "HID encryption must remain enabled")
assert(handler.stream == false)
assert(drivers.loaded.SelfieCom.handler == handler)
assert(drivers.inputs.panda[0], "Panda support must remain enabled")

local function assertAction(mode, target)
    input.updatGenericButtonStates({mode}, 0, 0)
    for i = 1, 8 do
        assert(input.panda_buttons[i] == (i == target and 1 or 0), "Unexpected mapped action at " .. i)
    end
end
local joystick = drivers.inputs.joystick[0]
for resource, action in pairs({
    hat_left = BUTTON_LEFT,
    hat_down = BUTTON_DOWN,
    hat_right = BUTTON_RIGHT,
    hat_up = BUTTON_UP,
}) do
    joystick[resource] = 1
    assertAction("joystick", action)
    joystick[resource] = 0
end
joystick.button = 1
assertAction("joystick", BUTTON_CONFIRM)
joystick.button = 8
assertAction("joystick", BUTTON_BACK)
joystick.button = 0
assertAction("joystick", 0)
local genericActions = {
    [1] = BUTTON_DOWN, [2] = BUTTON_UP, [3] = BUTTON_RIGHT,
    [4] = BUTTON_LEFT, [5] = BUTTON_CONFIRM, [8] = BUTTON_BACK,
}
for button, action in pairs(genericActions) do
    drivers.inputs.generic[0].buttons[button] = 1
    assertAction("generic", action)
    drivers.inputs.generic[0].buttons[button] = 0
end
assertAction("generic", 0)

handler.onConnect(1, 0, "aa:bb:cc:dd:ee:ff", "SelfieCom")
handler.onConnect(2, 1, "aa:bb:cc:dd:ee:00", "selfiecom")
assert(drivers.type_by_id[0] == drivers.loaded.SelfieCom)
assert(drivers.type_by_id[1] == drivers.loaded.SelfieCom)

local dev = drivers.inputs.generic[0]
local function assertButtons(target)
    for i = 1, 8 do
        assert(dev.buttons[i] == (i == target and 1 or 0), "Unexpected button state at " .. i)
    end
end

handler.onPacket(1, 0, nil, 42)
handler.onPacket(1, 0, {0, 0}, 42)
assertButtons(0)

local packets = {
    {button = 8, data = {0, 8, 112, 7, 112, 7, 1}},
    {button = 8, data = {0, 0, 112, 7, 252, 7, 1}},
    {button = 5, data = {0, 0, 112, 7, 112, 7, 1}},
    {button = 1, data = {0, 0, 0, 7, 0, 3, 1}},
    {button = 2, data = {0, 0, 0, 7, 0, 10, 1}},
    {button = 3, data = {0, 0, 0, 2, 0, 7, 1}},
    {button = 4, data = {0, 0, 0, 11, 0, 7, 1}},
}
for _, packet in ipairs(packets) do
    handler.onPacket(1, 0, {0, 0, 0, 0, 0, 0, 0}, 42)
    handler.onPacket(1, 0, packet.data, 42)
    assertButtons(packet.button)
    assert(dev.physicalLatch == true)
    now = now + 150
    drivers.update()
    assertButtons(packet.button)
    now = now + 1
    drivers.update()
    assertButtons(0)
    assert(dev.timeout == nil)
    handler.onPacket(1, 0, packet.data, 42)
    assertButtons(0)
end

handler.onPacket(1, 0, {0, 0, 0, 0, 0, 0, 0}, 42)
for _, noise in ipairs({76, 40, 208}) do
    handler.onPacket(1, 0, {0, 0, 0, 2, noise, 7, 1}, 42)
    assertButtons(0)
    assert(dev.physicalLatch == false)
end
handler.onPacket(2, 1, packets[3].data, 42)
assert(drivers.inputs.generic[1].buttons[5] == 1)
assertButtons(0)
handler.onDisconnect(2, 1, "test")
assert(drivers.type_by_id[1] == nil)
print("SelfieCom regression tests passed")
