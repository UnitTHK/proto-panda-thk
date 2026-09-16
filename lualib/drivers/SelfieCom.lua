local selfiecom = {
    type = "hid",
    match = {
        byname = 'SelfieCom'
    },
    mode = {
        'generic'
    },
}

function selfiecom.processPackets(connectionId, controllerId, data)
    local dev = drivers.generic[controllerId]
    
    if not data or #data < 7 then return end

    -- 1. BUTTON PRESSED
    if data[7] == 1 then
        local target = 0
        
        -- STRICT 1-TO-1 MAPPING
    if data[7] == 1 then
        -- 1. Check AUX/BACK first using data[2]
        if data[2] == 8 or data[5] == 252 then
            target = 8 -- AUX / BACK

    -- 2. Check exact CENTER press (neutral position)
        elseif data[3] == 112 and data[5] == 112 then
            target = 5 -- CENTER / CONFIRM

    -- 3. Check UP vs DOWN using coarse Y-axis (data[6])
        elseif data[6] <= 3 then
            target = 1 -- UP (Triggers only at extreme UP)

        elseif data[6] >= 10 then
            target = 2 -- DOWN (Triggers only at extreme DOWN)

    -- 4. Check LEFT vs RIGHT using coarse X-axis (data[4])
        elseif data[4] <= 2 then
            target = 3 -- LEFT

        elseif data[4] >= 11 then
            target = 4 -- RIGHT
    end
end

        local isNoise = (data[5] == 76 or data[5] == 40 or data[5] == 208)

        if target > 0 and not isNoise then
            -- Only fire if the button is NOT currently latched down
            if not dev.physicalLatch then
                for i = 1, 8 do dev.buttons[i] = 0 end
                
                dev.buttons[target] = 1
                dev.timeout = millis() + 150
                dev.physicalLatch = true -- Lock the latch immediately
            end
        end

    -- 2. BUTTON RELEASED
    elseif data[7] == 0 then
        -- The physical release packet has arrived. Unlock the latch so we can click again.
        dev.physicalLatch = false
    end
end

function selfiecom.onUpdate(drivers, i)
    local dev = drivers.generic[i]
    if dev.timeout and dev.timeout < millis() then  
        dev.timeout = nil
        for a = 1, 8 do dev.buttons[a] = 0 end
    end
end

return selfiecom