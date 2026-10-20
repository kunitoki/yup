// From the demos on this page: a live panel lit in 3D on the GPU,
// and a pulse of the drone that plays under it.

#include <yup_gui/yup_gui.h>
#include <yup_dsp/yup_dsp.h>

// Draws the panel, rendered to a texture, on its mesh through the RHI.
void SurfaceView::paint (yup::Graphics& g)
{
    auto& context = g.getGraphicsContext();
    auto panelTexture = panel->renderToTexture (context, g.getContextScale());

    auto frame = yup::GpuFrame::begin (device);
    if (! frame.isValid())
        return;

    {
        auto pass = target.color->beginRenderPass (frame, { true, yup::Colors::transparentBlack });
        pass.setDepthStencilAttachment (target.depth);
        pass.setPipeline (pipeline);
        pass.setUniformBuffer (0, 0, &uniforms, sizeof (uniforms));
        pass.setTexture (0, 1, panelTexture);
        pass.setVertexBuffer (0, vertexBuffer);
        pass.setIndexBuffer (yup::GpuIndexFormat::uint32, indexBuffer);
        pass.drawIndexed (numIndices);
        pass.finish();
    }

    frame.submit();
    g.drawTexture (target.color->asTexture(), getLocalBounds());
}

// One sample of the pulsing drone: a saw plucked through a yup_dsp filter.
float Pulse::next()
{
    phase += increment;
    phase -= std::floor (phase);

    const auto cutoff = 120.0f * std::exp2 (3.5f * brightness) * (1.0f + 8.0f * envelope);
    filter.setParameters (yup::FilterMode::lowpass, cutoff, 1.2, 0.0, sampleRate);

    envelope *= decay;
    return level * filter.processSample (polyBlepSaw (phase, increment));
}

// The audio thread hands every block to the demo that is open.
void Sound::audioDeviceIOCallbackWithContext (const float* const*, int,
                                              float* const* outputs, int, int numSamples,
                                              const yup::AudioIODeviceCallbackContext&)
{
    source->render (outputs[0], outputs[1], numSamples);
    analysis.push (outputs[0], outputs[1], numSamples, sampleRate);
}
