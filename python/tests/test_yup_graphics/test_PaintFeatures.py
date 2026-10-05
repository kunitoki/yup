import yup

#==================================================================================================

def test_stroke_type_position():
    stroke = yup.StrokeType(2.0)
    assert stroke.getPosition() == yup.StrokePosition.Center

    inside = stroke.withPosition(yup.StrokePosition.Inside)
    assert inside.getPosition() == yup.StrokePosition.Inside
    assert inside.withWidth(4.0).getPosition() == yup.StrokePosition.Inside
    assert inside != stroke

#==================================================================================================

def test_image_sampling():
    sampling = yup.ImageSampling()
    assert sampling.wrapX == yup.ImageWrap.Clamp
    assert sampling.wrapY == yup.ImageWrap.Clamp
    assert sampling.filter == yup.ImageFilter.Linear

    tiled = yup.ImageSampling(yup.ImageWrap.Repeat, yup.ImageWrap.Mirror, yup.ImageFilter.Nearest)
    assert tiled.wrapX == yup.ImageWrap.Repeat
    assert tiled.wrapY == yup.ImageWrap.Mirror
    assert tiled.filter == yup.ImageFilter.Nearest

#==================================================================================================

def test_additive_blend_mode():
    assert yup.BlendMode.Additive != yup.BlendMode.SrcOver

#==================================================================================================

def test_clip_stroke_is_bound():
    assert hasattr(yup.Graphics, "setClipStroke")
