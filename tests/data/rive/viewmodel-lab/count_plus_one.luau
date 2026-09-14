-- The `count +1` pill's action: read the `count` view model number, add one,
-- write it back. The host sees the new value.
--
-- This is a script rather than a `ListenerViewModelChange` with a
-- `DataConverterOperationValue` because a converter on a listener's *read*
-- context does not survive: the read resolves to nothing, so the write lands as
-- the bindable property's default (0) instead of the incremented value. The
-- read itself is fine — a two-context bindable with no converter on either
-- context writes the source's value through, which is the shape to reach for
-- when the arithmetic is a constant or two properties; reach for a script when
-- it is not.
--
-- See docs/data.md, "Writing a value back from a listener", and
-- docs/luau/protocols.md, "Listener action".

type CountPlusOne = {
    -- Nil until the second init; `context:viewModel()` is nil on the first.
    count: Property<number>?,
}

function init(self: CountPlusOne, context: Context): boolean
    local vm = context:viewModel()
    if vm == nil then
        return true
    end
    self.count = vm:getNumber('count')
    return true
end

function performAction(self: CountPlusOne, context: ListenerContext)
    if self.count == nil then
        return
    end
    self.count.value = self.count.value + 1
end

return function(): ListenerAction<CountPlusOne>
    return {
        count = nil,
        init = init,
        performAction = performAction,
    }
end
