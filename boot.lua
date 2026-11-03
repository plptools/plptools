local keyboard = manager.machine.natkeyboard
local ports = manager.machine.ioport.ports
local LEFT = ports[':COL0']:field(0x010)
local CTRL = ports[':COL2']:field(0x080)
local MENU = ports[':COL5']:field(0x080)

local function press(...)
  local keys = {...}

  -- Press each key.
  for _, key in ipairs(keys) do
    key:set_value(1)
    emu.wait(0.05)
  end

  -- Release each key (starting with the last).
  for i = #keys, 1, -1 do
    keys[i]:clear_value()
    emu.wait(0.05)
  end

end

local function send(s)
  keyboard:post_coded(s)
  while keyboard.is_posting do emu.wait_next_frame() end
end

coroutine.resume(coroutine.create(function()
  press(CTRL, MENU)
  emu.wait(1)
  send('{F11}l{ENTER}')
  emu.wait(1)
  press(LEFT)
  send('{ENTER}')
end))
