SCRIPT_HOME = "/SCRIPTS/BF"
--protocol = assert(loadScript(SCRIPT_HOME.."/protocols.lua"))()

-- copied from betaflight, since they changed how scripts are loaded
local supportedProtocols =
{
    smartPort =
    {
        transport       = SCRIPT_HOME.."/MSP/sp.lua",
        rssi            = function() return getValue("RSSI") end,
        stateSensor     = "Tmp1",
        push            = sportTelemetryPush,
        maxTxBufferSize = 6,
        maxRxBufferSize = 6,
        saveMaxRetries  = 2,
        saveTimeout     = 500
    },
    crsf =
    {
        transport       = SCRIPT_HOME.."/MSP/crsf.lua",
        rssi            = function() return getValue("TQly") end,
        stateSensor     = "1RSS",
        push            = crossfireTelemetryPush,
        maxTxBufferSize = 8,
        maxRxBufferSize = 58,
        saveMaxRetries  = 2,
        saveTimeout     = 150
    }
}

local function getProtocol()
    if supportedProtocols.smartPort.push() ~= nil then
        return supportedProtocols.smartPort
    elseif supportedProtocols.crsf.push() ~= nil then
        return supportedProtocols.crsf
    end
end

protocol = assert(getProtocol(), "Telemetry protocol not supported!")

assert(loadScript(protocol.transport))()
assert(loadScript(SCRIPT_HOME.."/MSP/common.lua"))()

local MSP_ADD_LAP = 11

local current_screen = 1
local current_item = 0
local is_editing = false

local msp_string = "Msp msg: none"
local debug_string = "debug string!"

local last_cmd = ""

local last_sent = 0
local send_interval = 500

local last_lap = {}
local last_lap_int = 0
local last_lap_ack_needed = 0
local lap_sent = false

local laps_int = {{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}

local wifi_status = 0
local wifi_rssi = 0
local connection_status = 0
local wifi_ip = 0

local CHORUS_CMD_NUM_RX = 1
local CHORUS_CMD_ADC_TYPE = 2
local CHORUS_CMD_ADC_VAL = 3
local CHORUS_CMD_WIFI_PROTO = 4
local CHORUS_CMD_WIFI_CHANNEL = 5
local CHORUS_CMD_FILTER_CUTOFF = 6
local CHORUS_CMD_MIN_LAPTIME = 7
local CHORUS_CMD_SOUNDS = 8
local CHORUS_CMD_SKIP_FIRST = 9
local CHORUS_CMD_RACE_MODE = 10
local CHORUS_CMD_BAND = 11
local CHORUS_CMD_CHANNEL = 12
local CHORUS_CMD_ACTIVE = 13
local CHORUS_CMD_THRESHOLD = 14
local chorus_cmds = {}

chorus_cmds[CHORUS_CMD_NUM_RX] =  { cmd="ER*M", bits=4, last_val=nil }
chorus_cmds[CHORUS_CMD_ADC_TYPE] =  { cmd="ER*v", bits=4, last_val=nil }
chorus_cmds[CHORUS_CMD_ADC_VAL] =  { cmd="ER*V", bits=16, last_val=nil }
chorus_cmds[CHORUS_CMD_WIFI_PROTO] =  { cmd="ER*w", bits=4, last_val=nil }
chorus_cmds[CHORUS_CMD_WIFI_CHANNEL] =  { cmd="ER*W", bits=4, last_val=nil }
chorus_cmds[CHORUS_CMD_FILTER_CUTOFF] =  { cmd="ER*F", bits=16, last_val=nil }
chorus_cmds[CHORUS_CMD_MIN_LAPTIME] =  { cmd="R*M", bits=8, last_val=nil }
chorus_cmds[CHORUS_CMD_SOUNDS] =  { cmd="R*S", bits=4, last_val=nil }
chorus_cmds[CHORUS_CMD_SKIP_FIRST] =  { cmd="R*1", bits=4, last_val=nil }
chorus_cmds[CHORUS_CMD_RACE_MODE] =  { cmd="R*R", bits=4, last_val=nil }
chorus_cmds[CHORUS_CMD_BAND] =  { cmd="R*B", bits=4, last_val=nil, individual=true}
chorus_cmds[CHORUS_CMD_CHANNEL] =  { cmd="R*C", bits=4, last_val=nil, individual=true}
chorus_cmds[CHORUS_CMD_ACTIVE] =  { cmd="R*A", bits=4, last_val=nil, individual=true }
chorus_cmds[CHORUS_CMD_THRESHOLD] =  { cmd="R*T", bits=16, last_val=nil, individual=true }


local settings_labels = {}
local settings_fields = {}

local pages = {}

local y = 0
local x = 0
local function inc_y(val)
	y = y + val
	return y
end

local function inc_x(val)
	x = x + val
	return x
end
local linespacing = 20
settings_labels[#settings_labels +1] = {t="Num Rx", x=x, y=inc_y(linespacing)}
settings_labels[#settings_labels + 1] = {t="ADC Type", x=x, y=inc_y(linespacing)}
settings_labels[#settings_labels + 1] = {t="ADC Val", x=x, y=inc_y(linespacing)}
settings_labels[#settings_labels + 1] = {t="WiFi Protocol", x=x, y=inc_y(linespacing)}
settings_labels[#settings_labels + 1] = {t="WiFi Channel", x=x, y=inc_y(linespacing)}
settings_labels[#settings_labels + 1] = {t="Filter cutoff", x=x, y=inc_y(linespacing)}
y = 0
x = 70+30
settings_fields[#settings_fields + 1] = {x= x, y=inc_y(linespacing), min = 1, max = 6, cmd_id=CHORUS_CMD_NUM_RX}
settings_fields[#settings_fields + 1] = {x= x, y=inc_y(linespacing), min = 0, max = 3, labels={"OFF", "ADC5", "ADC6", "INA219"}, cmd_id=CHORUS_CMD_ADC_TYPE}
settings_fields[#settings_fields + 1] = {x= x, y=inc_y(linespacing), min = 1, max = 10000, cmd_id=CHORUS_CMD_ADC_VAL}
settings_fields[#settings_fields + 1] = {x= x, y=inc_y(linespacing), min = 0, max = 1, labels={"bgn", "b"}, cmd_id=CHORUS_CMD_WIFI_PROTO}
settings_fields[#settings_fields + 1] = {x= x, y=inc_y(linespacing), min = 1, max = 13, cmd_id=CHORUS_CMD_WIFI_CHANNEL}
settings_fields[#settings_fields + 1] = {x= x, y=inc_y(linespacing), min = 1, max = 10000, cmd_id=CHORUS_CMD_FILTER_CUTOFF}
x = 100+30
y = 0
settings_labels[#settings_labels + 1] = {t="Min Laptime", x=x, y=inc_y(linespacing)}
settings_labels[#settings_labels + 1] = {t="Device Sounds", x=x, y=inc_y(linespacing)}
settings_labels[#settings_labels + 1] = {t="Skip First Lap", x=x, y=inc_y(linespacing)}


y = 0
x= 170+30
settings_fields[#settings_fields + 1] = {x= x, y=inc_y(linespacing), min = 0, max = 254, cmd_id=CHORUS_CMD_MIN_LAPTIME}
settings_fields[#settings_fields + 1] = {x= x, y=inc_y(linespacing), min = 0, max = 1, labels={"OFF","ON"}, cmd_id=CHORUS_CMD_SOUNDS}
settings_fields[#settings_fields + 1] = {x= x, y=inc_y(linespacing), min = 0, max = 1, labels={"OFF","ON"}, cmd_id=CHORUS_CMD_SKIP_FIRST}


local settings_page = { title = "Chorus32 Settings", labels=settings_labels, fields=settings_fields}

local rx_labels = {}
local rx_fields = {}
x = 0
y = 0
for i=1, 6 do
	rx_labels[#rx_labels +1] = {t="Pilot " .. tostring(i), x=x, y=inc_y(linespacing)}
end

y = 0
for i=1,6 do
	x = 40
	rx_fields[#rx_fields + 1] = {x= x, y=inc_y(linespacing), min = 0, max = 7, cmd_id=CHORUS_CMD_BAND, labels={"R", "A", "B", "E", "F", "D", "Connex", "Connex2"}, node=i-1}
	rx_fields[#rx_fields + 1] = {x= inc_x(15), y=y, min = 0, max = 7, labels={"1", "2", "3", "4", "5", "6", "7", "8"}, cmd_id=CHORUS_CMD_CHANNEL, node = i-1}
	rx_fields[#rx_fields + 1] = {x= inc_x(20), y=y, min = 0, max = 1, labels={"OFF","ON"}, cmd_id=CHORUS_CMD_ACTIVE, node=i-1}
	rx_fields[#rx_fields + 1] = {x= inc_x(20), y=y, min = 0, max = 300, cmd_id=CHORUS_CMD_THRESHOLD, node=i-1}
	-- TODO: add RSSI bar
	-- Add enter/leave command for pages
	-- add custom function call for labels, to add a gauge lcd.drawGauge(x, y, w, h, fill, maxfill [, flags])
end

local rx_page = {title = "RX Configuration", labels=rx_labels, fields=rx_fields}
local race_labels = {}

local function fill_node_id(cmd_id, node)
	local node_pos = 2
	local cmd = chorus_cmds[cmd_id].cmd
	if string.sub(cmd, 1, 1) == "E" then
		node_pos = 3
	end
	return string.sub(cmd, 1, node_pos - 1) .. tostring(node) .. string.sub(cmd, node_pos + 1)
end

local function chorus_get_value_node(cmd_id, node)
	local cmd = fill_node_id(cmd_id, node)
	serialWrite(cmd .. "\n")
end

local function chorus_get_value(cmd_id)
	serialWrite(chorus_cmds[cmd_id].cmd .. "\n")
end


local function chorus_set_value_node(cmd_id, value, node)
	local cmd = fill_node_id(cmd_id, node)
	serialWrite(cmd .. string.format("%0" .. math.floor(chorus_cmds[cmd_id].bits/4) .. "X\n", math.floor(value)))
end

local function chorus_set_value(cmd_id, value)
	serialWrite(chorus_cmds[cmd_id].cmd .. string.format("%0" .. math.floor(chorus_cmds[cmd_id].bits/4) .. "X\n", math.floor(value)))
end

local function ms_to_string(ms)
	local seconds = ms/1000
	local minutes = math.floor(seconds/60)
	seconds = seconds % 60
	return  (string.format("%02d:%05.2f", minutes, seconds))
end

local function draw_header(title)

	local conn_status = "disconnected"
	if tonumber(connection_status) == 1 then
		conn_status = "connected"
	end
	--lcd.drawScreenTitle(title, current_screen, #pages)
	lcd.drawText(120, 0, conn_status, INVERS)
end

local function draw_debug_screen()
	
	lcd.drawText(10, 20, "WiFi status:", 0)
	lcd.drawText(160, 20, wifi_status,0)
	lcd.drawText(200, 20, ms_to_string(last_lap_int),0)
	lcd.drawText(300, 20, "RSSI:",0)
	lcd.drawText(360, 20, wifi_rssi,0)
	--lcd.drawText(400, 20, "IP:",0)
	--lcd.drawText(450, 20, wifi_ip,0)
	--lcd.drawText(10, 40, "Connection status:", 0)
	--lcd.drawText(400, 40, connection_status,0)
	lcd.drawText(10, 60, "Last rx: ",0)
	lcd.drawText(40, 60, last_cmd,0)
	lcd.drawText(10, 80, msp_string,0)
	lcd.drawText(10, 100, debug_string,0)
end

local function draw_race_screen()
	local race_status = chorus_cmds[CHORUS_CMD_RACE_MODE].last_val
	if race_status == nil then
		chorus_get_value(CHORUS_CMD_RACE_MODE)
		race_status = "--"
	end
	draw_header("Racing Status: " .. tostring(race_status))
	local textopt = SMLSIZE;
	local y_offset = 15
	linespacing = (128 - y_offset)/6
	y = y_offset
	x = 0
	lcd.drawText(x+2, y+2, "Laps", textopt)
	for i=1,5 do
		--lcd.drawLine(0, inc_y(linespacing), 212, y, SOLID, 0)
		inc_y(linespacing)
			lcd.drawText(x+2, y+2, "Pilot " .. i, textopt)
	end
	local x_spacing = 212/5
	x = 0
	y = y_offset
	for i=1,4 do
		--lcd.drawLine(inc_x(x_spacing), y_offset, x, 64, SOLID, 0)
		inc_x(x_spacing)
		lcd.drawText(x+x_spacing/2 - 4, y+2, i, textopt)
	end

	for i=1,#laps_int do
		for j=1, #(laps_int[i]) do
			if laps_int[i][j] ~= 0 then
				lcd.drawText(x_spacing*j+2, y_offset + linespacing*i+2, ms_to_string(laps_int[i][j]), textopt)
			end
		end
	end
end

local function getMinutesSecondsHundrethsAsString(miliseconds)
  -- Returns MM:SS.hh as a string
 -- miliseconds = miliseconds or 0
   --local seconds = miliseconds/1000
   local seconds = miliseconds/1000
  local minutes = math.floor(seconds/60) -- seconds/60 gives minutes
  seconds = seconds % 60 -- seconds % 60 gives seconds
  return  (string.format("%02d:%05.2f", minutes, seconds))
end


-- Time Tracking
local StartTimeMiliseconds = -1
local ElapsedTimeMiliseconds = 0
local PreviousElapsedTimeMiliseconds = 0
local LapTime = 0
local LapTimeList = {ElapsedTimeMiliseconds}
local LapTimeRecorded = false


local function draw_lap_screen()
  local TextSize = 0
  TextSize = MIDSIZE
  local TextHeight = 20 
  -- lcd.drawText( lcd.getLastPos(), 15, "s", SMLSIZE)
  local x = 10   
  --lcd.drawText( x, 0, "Running..", TextSize)
  --lcd.drawText( x, 0, freq, TextSize)
  --x = lcd.getLastPos() + 4
  --lcd.drawText( x, 0, threshold, TextSize)
  --x = lcd.getLastPos() + 4
  --lcd.drawText( x, 0, "Lap", TextSize + INVERS)
  --x = lcd.getLastPos() + 4
  --lcd.drawText( x, 0, #LapTimeList-1, TextSize)
  --x = lcd.getLastPos() + 4
  --lcd.drawText( x, 0, getMinutesSecondsHundrethsAsString(ElapsedTimeMiliseconds), TextSize)
  draw_header("LapList:")
  local rowHeight = math.floor(TextHeight + 8) --12
  local rows = math.floor(LCD_H/rowHeight) 
  local rowsMod=rows*rowHeight 

  x = 0
  local y = rowHeight
  local c = 1
  lcd.drawText(x,rowHeight," ")
  -- i = 2 first entry is always 0:00.00 so skippind it
  for i = #LapTimeList, 2, -1 do
    if y %  (rowsMod or 60) == 0 then
      c = c + 300-- next column
      x = 150
      y = rowHeight
    end
    if (c > 1) and x > LCD_W - x/(c-1) then
    else
      lcd.drawText( x, y, LapTimeList[i],TextSize)
    end
    y = y + rowHeight
  end
end


local race_page = {custom_draw_func=draw_race_screen}
local lap_page = {custom_draw_func=draw_lap_screen}
local debug_page = { custom_draw_func=draw_debug_screen }
pages = {race_page, debug_page, settings_page, rx_page,lap_page}

local last_chorus_update = 0


local function process_msp_reply(cmd,rx_buf)
	if cmd == MSP_ADD_LAP then
		-- TODO: any way to match with id?
		last_lap_ack_needed = last_lap_ack_needed - 1
	end
end

local function get_connection_status()
	serialWrite("PR*w\n")
	serialWrite("PR*c\n")
	serialWrite("PR*t\n")
	serialWrite("PR*i\n")
end

local busy_count = 0
local function send_msp(values)
	mspProcessTxQ()
	if protocol.mspWrite(MSP_ADD_LAP, values) == nil then
		busy_count = busy_count + 1
		debug_string = "MSP busy!" .. tostring(busy_count)
	end
	mspProcessTxQ()
	--process_msp_reply(mspPollReply())


end

local function convert_lap(pilot, lap, laptime)
	msp_string = "Last lap " .. pilot .. "L" .. lap .. "T" .. laptime
	--local values = {}
	--values[1] = pilot
	--values[2] = lap
	--for i = 3, 6 do
		--values[i] = bit32.band(laptime, 0xFF)
		--laptime = bit32.rshift(laptime, 8)
	--end
	--return values
    local valsTemp = {}
	local time1=laptime


	
	if string.byte(time1,1) == nil then valsTemp[1]=32 else valsTemp[1]=string.byte(time1,1) end
    if string.byte(time1,2) == nil then valsTemp[2]=32 else valsTemp[2]=string.byte(time1,2) end
	if string.byte(time1,3) == nil then valsTemp[3]=32 else valsTemp[3]=string.byte(time1,3) end
	if string.byte(time1,4) == nil then valsTemp[4]=32 else valsTemp[4]=string.byte(time1,4) end
	if string.byte(time1,5) == nil then valsTemp[5]=32 else valsTemp[5]=string.byte(time1,5) end
	if string.byte(time1,6) == nil then valsTemp[6]=32 else valsTemp[6]=string.byte(time1,6) end

	return valsTemp
end


local function process_extended_cmd(cmd)
	local node = string.sub(cmd, 2,2)
	local chorus_cmd = string.sub(cmd, 3,3)
	for i=1,#chorus_cmds do
		if string.sub(chorus_cmds[i].cmd, 1, 1) == "E" and string.sub(chorus_cmds[i].cmd, 4, 4) == chorus_cmd then
			chorus_cmds[i].last_val = tonumber(string.sub(cmd, 4), 16)
		end
	end
end

local function process_proxy_cmd(cmd)
	local chorus_cmd = string.sub(cmd, 3,3)
	if(chorus_cmd == "w") then
		wifi_status = tonumber(string.sub(cmd, 4), 16)
	elseif (chorus_cmd == 'c') then
		connection_status = tonumber(string.sub(cmd, 4), 16)
	elseif (chorus_cmd == 't') then
		wifi_rssi = tonumber(string.sub(cmd, 4), 16)
	elseif (chorus_cmd == 'i') then
		wifi_ip = tonumber(string.sub(cmd, 4), 16)
	end

end

local function process_cmd(cmd)
	local type = string.sub(cmd, 1,1)

	if (type == "E") then
		process_extended_cmd(string.sub(cmd, 2))
	elseif (type == "P") then
		process_proxy_cmd(string.sub(cmd, 2))
	else
		local node = tonumber(string.sub(cmd, 2,2))
		local chorus_cmd = string.sub(cmd, 3,3)
		if (chorus_cmd == "L") then -- laptime. for now only for pilot 1
			local new_time = tonumber(string.sub(cmd, 6), 16)
			local lap = tonumber(string.sub(cmd, 4, 5), 16)
            -- ADD NEW LAP FOR OTHER SCREEN
			LapTimeList[#LapTimeList+1] = getMinutesSecondsHundrethsAsString(new_time)
			
			if lap < 5 and lap > 0 then -- just limit the number of laps for now
				laps_int[node + 1][lap] = new_time
			end
			if node == 1 then
				last_lap_int = new_time
				last_lap = convert_lap(node, lap, new_time)
				last_lap_ack = false
				lap_sent = false
				last_lap_ack_needed = last_lap_ack_needed + 1
			end
		end
		for i=1,#chorus_cmds do
			if string.sub(chorus_cmds[i].cmd, 3, 3) == chorus_cmd then
				if chorus_cmds[i].individual then
					if chorus_cmds[i].last_val == nil then
						chorus_cmds[i].last_val = {} -- TODO: make this better
					end
					chorus_cmds[i].last_val[node] = tonumber(string.sub(cmd,4), 16)
				else
					chorus_cmds[i].last_val = tonumber(string.sub(cmd, 4), 16)
				end
			end
		end

	end
end

local function check_setting_pos(setting)
	if setting >= 0 then return setting else return "--" end
end

local function check_blinking(item)
	if current_item == item then
		if is_editing then
			return INVERS+BLINK
		else
			return INVERS
		end
	end
	return 0
end

local function draw_screen()
	local page = pages[current_screen]
	if page.custom_draw_func ~= nil then
		page.custom_draw_func()
		return
	end
	local labels = page.labels
	local fields = page.fields
	draw_header(page.title)
	for i=1,#labels do
		lcd.drawText(labels[i].x, labels[i].y, labels[i].t, SMLSIZE)
	end

	for i=1,#fields do
		local val = chorus_cmds[fields[i].cmd_id].last_val
		if val ~= nil and chorus_cmds[fields[i].cmd_id].individual then
			if #val >= fields[i].node then
				val = val[fields[i].node]
			else
				val = nil
			end
		end
		local field_text = "--"
		if val ~= nil then
			if fields[i].scaling ~= nil then
				val = math.floor(val * fields[i].scaling)
			end
			if fields[i].labels ~= nil then -- apply custom labels
				field_text = tostring(fields[i].labels[1 + val - fields[i].min])
			else
				field_text = tostring(val)
			end
		else
			if(getTime() - last_chorus_update > 10) then
				if fields[i].node ~= nil then
					chorus_get_value_node(fields[i].cmd_id, fields[i].node)
				else
					chorus_get_value(fields[i].cmd_id)
				end
				last_chorus_update = getTime()
			end
		end
		lcd.drawText(fields[i].x, fields[i].y, field_text, check_blinking(i) + SMLSIZE)
	end
end


local function draw_ui()
	lcd.clear()

	draw_screen()
end

local function handle_input_edit(event)
	local field = pages[current_screen].fields[current_item]
	local current_val = chorus_cmds[field.cmd_id].last_val
	if chorus_cmds[field.cmd_id].individual then
		current_val = current_val[field.node]
	end
	local scaling = 1
	if field.scaling ~= nil then
		scaling = 1/field.scaling
	end
	if event == EVT_PLUS_BREAK then
		if current_val + 1 <= field.max then
			if field.node ~= nil then
				chorus_set_value_node(field.cmd_id, current_val + 1*scaling, field.node)
			else
				chorus_set_value(field.cmd_id, current_val + 1*scaling)
			end
		end
	elseif event == EVT_MINUS_BREAK then
		if current_val > field.min then
			if field.node ~= nil then
				chorus_set_value_node(field.cmd_id, current_val - 1*scaling, field.node)
			else
				chorus_set_value(field.cmd_id, current_val - 1*scaling)
			end
		end
	end

end

local function handle_input(event)
	if(event == EVT_ENTER_BREAK) then
		is_editing = not is_editing
	end
	if is_editing then
		handle_input_edit(event)
	elseif(event == EVT_VIRTUAL_NEXT_PAGE) then
		current_item = 1
		current_screen = (current_screen + 1)
		if current_screen > #(pages) then
			current_screen = 1
		end
	elseif pages[current_screen].fields ~= nil then
		if(event == EVT_MINUS_BREAK) then
			current_item = current_item + 1
			if(current_item > #(pages[current_screen].fields)) then
				current_item = 1
			end
		elseif(event == EVT_PLUS_BREAK) then
			current_item = current_item - 1
			if(current_item < 0) then
				current_item = #(pages[current_screen].fields)
			end
		end
	elseif event == EVT_MENU_BREAK then
		if chorus_cmds[CHORUS_CMD_RACE_MODE].last_val == 0 then
			chorus_set_value(CHORUS_CMD_RACE_MODE, 1)
		else
			chorus_set_value(CHORUS_CMD_RACE_MODE, 0)
		end
	end

end

local function run_bg()
	mspProcessTxQ()
	return 0
end

local run = function (event)
	handle_input(event)
	draw_ui()
--	if type(serialReadLine) ~= "nil" then
--		local rx = serialReadLine()
--	end
	local rx = serialRead()
	if(rx ~= nil and string.len(rx) > 0) then
	
	--if(rx ~= nil) then
		last_cmd = rx
		process_cmd(last_cmd)
	end

	if(last_sent + send_interval < getTime()) then
		last_sent = getTime()
		get_connection_status()
	end
	mspProcessTxQ()
	if last_lap_int ~= 0 then
		--if lap_sent == false then
			if protocol.mspWrite(MSP_ADD_LAP, last_lap) ~= nil then
				lap_sent = true
				debug_string = "Lap sent!"
			else
				debug_string = "MSP is busy"
			end
		--end
	end
	return 0
end

return {run=run, run_bg=run_bg}
