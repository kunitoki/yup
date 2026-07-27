import yup

#==================================================================================================

def test_midi_message_construction():
    # Test default construction
    msg = yup.MidiMessage()
    assert msg is not None

#==================================================================================================

def test_midi_message_note_on():
    msg = yup.MidiMessage.noteOn(1, 60, 0.8)
    assert msg.isNoteOn()
    assert not msg.isNoteOff()
    assert msg.getNoteNumber() == 60
    assert msg.getChannel() == 1
    assert abs(msg.getFloatVelocity() - 0.8) < 0.01

#==================================================================================================

def test_midi_message_note_on_uint8():
    """Test noteOn with uint8 velocity."""
    msg = yup.MidiMessage.noteOn(1, 60, 0.8)
    assert msg.isNoteOn()
    assert msg.getNoteNumber() == 60
    assert msg.getVelocity() == int(0.8 * 127 + 0.5)

#==================================================================================================

def test_midi_message_note_on_velocity_zero():
    """Test noteOn with zero velocity (should be note off by MIDI spec)."""
    msg = yup.MidiMessage.noteOn(1, 60, 0.0)

    # With returnTrueForVelocity0=False, velocity 0 note-on is NOT considered note-on
    assert not msg.isNoteOn(returnTrueForVelocity0=False)

    # With returnTrueForVelocity0=True, velocity 0 note-on IS considered note-on
    assert msg.isNoteOn(returnTrueForVelocity0=True)

#==================================================================================================

def test_midi_message_note_off():
    msg = yup.MidiMessage.noteOff(1, 60, 0.0)
    assert msg.isNoteOff()
    assert not msg.isNoteOn()
    assert msg.getNoteNumber() == 60
    assert msg.getChannel() == 1

#==================================================================================================

def test_midi_message_note_off_uint8():
    """Test noteOff with uint8 velocity."""
    msg = yup.MidiMessage.noteOff(1, 60, 0.5)
    assert msg.isNoteOff()
    assert msg.getNoteNumber() == 60
    assert abs(msg.getFloatVelocity() - 0.5) < 0.01

#==================================================================================================

def test_midi_message_note_off_no_velocity():
    """Test noteOff without velocity parameter."""
    msg = yup.MidiMessage.noteOff(1, 60)
    assert msg.isNoteOff()
    assert msg.getNoteNumber() == 60
    assert abs(msg.getFloatVelocity() - 0.0) < 0.01

#==================================================================================================

def test_midi_message_program_change():
    msg = yup.MidiMessage.programChange(1, 42)
    assert msg.isProgramChange()
    assert msg.getProgramChangeNumber() == 42
    assert msg.getChannel() == 1

#==================================================================================================

def test_midi_message_pitch_wheel():
    msg = yup.MidiMessage.pitchWheel(1, 8192)
    assert msg.isPitchWheel()
    assert msg.getPitchWheelValue() == 8192
    assert msg.getChannel() == 1

#==================================================================================================

def test_midi_message_controller():
    msg = yup.MidiMessage.controllerEvent(1, 7, 100)  # Volume controller
    assert msg.isController()
    assert msg.getControllerNumber() == 7
    assert msg.getControllerValue() == 100
    assert msg.getChannel() == 1

#==================================================================================================

def test_midi_message_all_notes_off():
    msg = yup.MidiMessage.allNotesOff(1)
    assert msg.isAllNotesOff()
    assert msg.getChannel() == 1

#==================================================================================================

def test_midi_message_timestamp():
    msg = yup.MidiMessage.noteOn(1, 60, 0.8)

    # Set timestamp
    msg.setTimeStamp(123.456)
    assert abs(msg.getTimeStamp() - 123.456) < 0.001

    # Add to timestamp
    msg.addToTimeStamp(10.0)
    assert abs(msg.getTimeStamp() - 133.456) < 0.001

#==================================================================================================

def test_midi_message_with_timestamp():
    msg1 = yup.MidiMessage.noteOn(1, 60, 0.8)
    msg1.setTimeStamp(100.0)

    msg2 = msg1.withTimeStamp(200.0)

    assert abs(msg1.getTimeStamp() - 100.0) < 0.001
    assert abs(msg2.getTimeStamp() - 200.0) < 0.001
    assert msg2.isNoteOn()
    assert msg2.getNoteNumber() == 60

#==================================================================================================

def test_midi_message_description():
    msg = yup.MidiMessage.noteOn(1, 60, 0.8)
    desc = msg.getDescription()

    assert isinstance(desc, str)
    assert len(desc) > 0

#==================================================================================================

def test_midi_buffer_construction():
    buffer = yup.MidiBuffer()
    assert buffer is not None
    assert buffer.isEmpty()
    assert buffer.getNumEvents() == 0

#==================================================================================================

def test_midi_buffer_construction_with_message():
    msg = yup.MidiMessage.noteOn(1, 60, 0.8)
    buffer = yup.MidiBuffer(msg)

    assert not buffer.isEmpty()
    assert buffer.getNumEvents() == 1

#==================================================================================================

def test_midi_buffer_add_event():
    buffer = yup.MidiBuffer()

    msg1 = yup.MidiMessage.noteOn(1, 60, 0.8)
    msg2 = yup.MidiMessage.noteOff(1, 60, 0.0)

    buffer.addEvent(msg1, 0)
    buffer.addEvent(msg2, 100)

    assert not buffer.isEmpty()
    assert buffer.getNumEvents() == 2

#==================================================================================================

def test_midi_buffer_clear():
    buffer = yup.MidiBuffer()

    msg = yup.MidiMessage.noteOn(1, 60, 0.8)
    buffer.addEvent(msg, 0)

    assert not buffer.isEmpty()

    buffer.clear()

    assert buffer.isEmpty()
    assert buffer.getNumEvents() == 0

#==================================================================================================

def test_midi_buffer_clear_range():
    buffer = yup.MidiBuffer()

    # Add events at different timestamps
    buffer.addEvent(yup.MidiMessage.noteOn(1, 60, 0.8), 0)
    buffer.addEvent(yup.MidiMessage.noteOn(1, 64, 0.8), 100)
    buffer.addEvent(yup.MidiMessage.noteOn(1, 67, 0.8), 200)

    assert buffer.getNumEvents() == 3

    # Clear events in middle range
    buffer.clear(100, 100)

    # Should still have events (at 0 and 200)
    assert buffer.getNumEvents() == 2

#==================================================================================================

def test_midi_buffer_event_times():
    buffer = yup.MidiBuffer()

    buffer.addEvent(yup.MidiMessage.noteOn(1, 60, 0.8), 50)
    buffer.addEvent(yup.MidiMessage.noteOn(1, 64, 0.8), 150)

    assert buffer.getFirstEventTime() == 50
    assert buffer.getLastEventTime() == 150

#==================================================================================================

def test_midi_buffer_iteration():
    buffer = yup.MidiBuffer()

    # Add some events
    buffer.addEvent(yup.MidiMessage.noteOn(1, 60, 0.8), 0)
    buffer.addEvent(yup.MidiMessage.noteOn(1, 64, 0.8), 100)
    buffer.addEvent(yup.MidiMessage.noteOff(1, 60, 0.0), 200)

    # Iterate over events
    events = list(buffer)

    assert len(events) == 3

    # Check first event
    msg1 = events[0].getMessage()
    assert msg1.isNoteOn()
    assert msg1.getNoteNumber() == 60
    assert events[0].samplePosition == 0

    # Check second event
    msg2 = events[1].getMessage()
    assert msg2.isNoteOn()
    assert msg2.getNoteNumber() == 64
    assert events[1].samplePosition == 100

    # Check third event
    msg3 = events[2].getMessage()
    assert msg3.isNoteOff()
    assert msg3.getNoteNumber() == 60
    assert events[2].samplePosition == 200

#==================================================================================================

def test_midi_buffer_len():
    buffer = yup.MidiBuffer()

    assert len(buffer) == 0

    buffer.addEvent(yup.MidiMessage.noteOn(1, 60, 0.8), 0)
    assert len(buffer) == 1

    buffer.addEvent(yup.MidiMessage.noteOff(1, 60, 0.0), 100)
    assert len(buffer) == 2

#==================================================================================================

def test_midi_buffer_add_events():
    buffer1 = yup.MidiBuffer()
    buffer2 = yup.MidiBuffer()

    # Add events to first buffer
    buffer1.addEvent(yup.MidiMessage.noteOn(1, 60, 0.8), 0)
    buffer1.addEvent(yup.MidiMessage.noteOn(1, 64, 0.8), 100)

    # Add events from first buffer to second buffer with time offset
    buffer2.addEvents(buffer1, 0, -1, 50)

    assert buffer2.getNumEvents() == 2
    assert buffer2.getFirstEventTime() == 50
    assert buffer2.getLastEventTime() == 150

#==================================================================================================

def test_midi_buffer_swap():
    buffer1 = yup.MidiBuffer()
    buffer2 = yup.MidiBuffer()

    # Add events to first buffer
    buffer1.addEvent(yup.MidiMessage.noteOn(1, 60, 0.8), 0)
    buffer1.addEvent(yup.MidiMessage.noteOn(1, 64, 0.8), 100)

    assert buffer1.getNumEvents() == 2
    assert buffer2.getNumEvents() == 0

    # Swap buffers
    buffer1.swapWith(buffer2)

    assert buffer1.getNumEvents() == 0
    assert buffer2.getNumEvents() == 2

#==================================================================================================

def test_midi_message_channel_manipulation():
    """Test setting and checking MIDI channels."""
    msg = yup.MidiMessage.noteOn(1, 60, 0.8)

    assert msg.getChannel() == 1
    assert msg.isForChannel(1)
    assert not msg.isForChannel(2)

    # Change channel
    msg.setChannel(5)
    assert msg.getChannel() == 5
    assert msg.isForChannel(5)
    assert not msg.isForChannel(1)

#==================================================================================================

def test_midi_message_velocity_manipulation():
    """Test velocity setting and multiplication."""
    msg = yup.MidiMessage.noteOn(1, 60, 1.0)
    assert abs(msg.getFloatVelocity() - 1.0) < 0.01

    # Set velocity
    msg.setVelocity(0.5)
    assert abs(msg.getFloatVelocity() - 0.5) < 0.01

    # Multiply velocity
    msg.multiplyVelocity(0.5)
    assert abs(msg.getFloatVelocity() - 0.25) < 0.01

#==================================================================================================

def test_midi_message_note_number_manipulation():
    """Test note number changes."""
    msg = yup.MidiMessage.noteOn(1, 60, 0.8)

    assert msg.getNoteNumber() == 60

    msg.setNoteNumber(72)
    assert msg.getNoteNumber() == 72

#==================================================================================================

def test_midi_message_aftertouch():
    """Test aftertouch messages."""
    msg = yup.MidiMessage.aftertouchChange(1, 60, 100)

    assert msg.isAftertouch()
    assert msg.getNoteNumber() == 60
    assert msg.getAfterTouchValue() == 100

#==================================================================================================

def test_midi_message_channel_pressure():
    """Test channel pressure (aftertouch)."""
    msg = yup.MidiMessage.channelPressureChange(1, 80)

    assert msg.getChannel() == 1
    # Channel pressure is a type of aftertouch

#==================================================================================================

def test_midi_message_all_sound_off():
    """Test all sound off message."""
    msg = yup.MidiMessage.allSoundOff(1)

    assert msg.isAllSoundOff()
    assert msg.getChannel() == 1

#==================================================================================================

def test_midi_message_all_controllers_off():
    """Test all controllers off message."""
    msg = yup.MidiMessage.allControllersOff(1)

    assert msg.isResetAllControllers()
    assert msg.getChannel() == 1

#==================================================================================================

def test_midi_message_controller_of_type():
    """Test checking specific controller types."""
    # Volume controller (CC 7)
    msg = yup.MidiMessage.controllerEvent(1, 7, 100)

    assert msg.isController()
    assert msg.isControllerOfType(7)
    assert not msg.isControllerOfType(1)  # Not modulation wheel

#==================================================================================================

def test_midi_message_note_on_or_off():
    """Test isNoteOnOrOff method."""
    note_on = yup.MidiMessage.noteOn(1, 60, 0.8)
    note_off = yup.MidiMessage.noteOff(1, 60, 0.0)
    controller = yup.MidiMessage.controllerEvent(1, 7, 100)

    assert note_on.isNoteOnOrOff()
    assert note_off.isNoteOnOrOff()
    assert not controller.isNoteOnOrOff()

#==================================================================================================

def test_midi_buffer_ensure_size():
    """Test pre-allocating buffer space."""
    buffer = yup.MidiBuffer()

    # Pre-allocate space
    buffer.ensureSize(1024)

    # Buffer should still be empty after ensureSize
    assert buffer.isEmpty()
    assert buffer.getNumEvents() == 0

#==================================================================================================

def test_midi_buffer_empty_iteration():
    """Test iterating over empty buffer."""
    buffer = yup.MidiBuffer()

    events = list(buffer)
    assert len(events) == 0

#==================================================================================================

def test_midi_buffer_sorted_insertion():
    """Test that events are kept sorted by timestamp."""
    buffer = yup.MidiBuffer()

    # Add events out of order
    buffer.addEvent(yup.MidiMessage.noteOn(1, 64, 0.8), 200)
    buffer.addEvent(yup.MidiMessage.noteOn(1, 60, 0.8), 50)
    buffer.addEvent(yup.MidiMessage.noteOn(1, 67, 0.8), 150)

    # Events should be sorted by timestamp
    events = list(buffer)

    assert events[0].samplePosition == 50
    assert events[0].getMessage().getNoteNumber() == 60

    assert events[1].samplePosition == 150
    assert events[1].getMessage().getNoteNumber() == 67

    assert events[2].samplePosition == 200
    assert events[2].getMessage().getNoteNumber() == 64

#==================================================================================================

def test_midi_buffer_multiple_events_same_time():
    """Test adding multiple events at the same timestamp."""
    buffer = yup.MidiBuffer()

    # Add multiple events at time 0
    buffer.addEvent(yup.MidiMessage.noteOn(1, 60, 0.8), 0)
    buffer.addEvent(yup.MidiMessage.noteOn(1, 64, 0.8), 0)
    buffer.addEvent(yup.MidiMessage.noteOn(1, 67, 0.8), 0)

    assert buffer.getNumEvents() == 3
    assert buffer.getFirstEventTime() == 0
    assert buffer.getLastEventTime() == 0
