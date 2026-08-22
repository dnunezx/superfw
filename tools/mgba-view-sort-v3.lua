-- Copyright (C) 2026 Danny Nunez (dnunezx)

local output = script.dir .. "/../artifacts/view-sort-v3/"
local start_frame = nil

local function shot(name)
  local display_control = emu:read16(0x04000000)
  local page = 0x06000000
  if math.floor(display_control / 0x10) % 2 == 1 then
    page = page + 0xA000
  end
  local frame = assert(io.open(output .. name .. ".frame", "wb"))
  frame:write(emu:readRange(0x05000000, 512))
  frame:write(emu:readRange(page, 240 * 160))
  frame:close()
end

local taps = {
  {100, C.GBA_KEY.START},
  {160, C.GBA_KEY.A},
  {220, C.GBA_KEY.DOWN},
  {240, C.GBA_KEY.A},
  {300, C.GBA_KEY.A},
  {360, C.GBA_KEY.A},
  {420, C.GBA_KEY.A},
  {480, C.GBA_KEY.A},
  {500, C.GBA_KEY.A},
  {520, C.GBA_KEY.A},
  {540, C.GBA_KEY.DOWN},
  {560, C.GBA_KEY.A},
  {580, C.GBA_KEY.DOWN},
  {600, C.GBA_KEY.A},
  {660, C.GBA_KEY.START},
  {780, C.GBA_KEY.L},
  {860, C.GBA_KEY.START},
}

local shots = {
  [140] = "popup-all-files",
  [200] = "popup-z-a",
  [280] = "popup-all-games",
  [340] = "popup-gba-only",
  [400] = "popup-gb-only",
  [460] = "popup-gbc-only",
  [640] = "popup-gba-hidden",
  [750] = "browse-gba-z-a",
  [850] = "recent-before-start",
  [900] = "recent-after-start",
}

callbacks:add("frame", function()
  if not start_frame then start_frame = emu:currentFrame() end
  local frame = emu:currentFrame() - start_frame

  for _, tap in ipairs(taps) do
    if frame == tap[1] then emu:addKey(tap[2])
    elseif frame == tap[1] + 5 then emu:clearKey(tap[2])
    end
  end

  if shots[frame] then shot(shots[frame]) end
  if frame == 930 then
    local marker = assert(io.open(output .. "complete.txt", "w"))
    marker:write("SuperR7 View & Sort flow completed\n")
    marker:close()
  end
end)
