-- The switch's action: negate the `enabled` view model boolean, so a click in
-- the picture writes back to whoever is editing the instance.
--
-- A script for the same reason as count_plus_one.luau: a
-- `DataConverterBooleanNegate` on the read context of a `ListenerViewModelChange`
-- drops the read, and the write then lands as the bindable's default (`false`)
-- rather than the negated value — which looks like a toggle that works once.

type ToggleEnabled = {
    -- Nil until the second init; `context:viewModel()` is nil on the first.
    enabled: Property<boolean>?,
}

function init(self: ToggleEnabled, context: Context): boolean
    local vm = context:viewModel()
    if vm == nil then
        return true
    end
    self.enabled = vm:getBoolean('enabled')
    return true
end

function performAction(self: ToggleEnabled, context: ListenerContext)
    if self.enabled == nil then
        return
    end
    self.enabled.value = not self.enabled.value
end

return function(): ListenerAction<ToggleEnabled>
    return {
        enabled = nil,
        init = init,
        performAction = performAction,
    }
end
