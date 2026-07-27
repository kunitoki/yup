// Returns the name of an extension, as an index into kStrings
IndexRange ExtensionToIndexRange(Extension extension) {
  switch(extension) {
    case Extension::kSPV_ALTERA_arbitrary_precision_fixed_point: return IR(0, 43);
    case Extension::kSPV_ALTERA_arbitrary_precision_floating_point: return IR(43, 46);
    case Extension::kSPV_ALTERA_arbitrary_precision_integers: return IR(89, 40);
    case Extension::kSPV_ALTERA_blocking_pipes: return IR(129, 26);
    case Extension::kSPV_ALTERA_fpga_argument_interfaces: return IR(155, 36);
    case Extension::kSPV_ALTERA_fpga_buffer_location: return IR(191, 32);
    case Extension::kSPV_ALTERA_fpga_cluster_attributes: return IR(223, 35);
    case Extension::kSPV_ALTERA_fpga_dsp_control: return IR(258, 28);
    case Extension::kSPV_ALTERA_fpga_invocation_pipelining_attributes: return IR(286, 49);
    case Extension::kSPV_ALTERA_fpga_latency_control: return IR(335, 32);
    case Extension::kSPV_ALTERA_fpga_loop_controls: return IR(367, 30);
    case Extension::kSPV_ALTERA_fpga_memory_accesses: return IR(397, 32);
    case Extension::kSPV_ALTERA_fpga_memory_attributes: return IR(429, 34);
    case Extension::kSPV_ALTERA_fpga_reg: return IR(463, 20);
    case Extension::kSPV_ALTERA_global_variable_fpga_decorations: return IR(483, 44);
    case Extension::kSPV_ALTERA_io_pipes: return IR(527, 20);
    case Extension::kSPV_ALTERA_loop_fuse: return IR(547, 21);
    case Extension::kSPV_ALTERA_runtime_aligned: return IR(568, 27);
    case Extension::kSPV_ALTERA_task_sequence: return IR(595, 25);
    case Extension::kSPV_ALTERA_usm_storage_classes: return IR(620, 31);
    case Extension::kSPV_AMDX_shader_enqueue: return IR(651, 24);
    case Extension::kSPV_AMD_gcn_shader: return IR(675, 19);
    case Extension::kSPV_AMD_gpu_shader_half_float: return IR(694, 30);
    case Extension::kSPV_AMD_gpu_shader_half_float_fetch: return IR(724, 36);
    case Extension::kSPV_AMD_gpu_shader_int16: return IR(760, 25);
    case Extension::kSPV_AMD_shader_ballot: return IR(785, 22);
    case Extension::kSPV_AMD_shader_early_and_late_fragment_tests: return IR(807, 45);
    case Extension::kSPV_AMD_shader_explicit_vertex_parameter: return IR(852, 41);
    case Extension::kSPV_AMD_shader_fragment_mask: return IR(893, 29);
    case Extension::kSPV_AMD_shader_image_load_store_lod: return IR(922, 36);
    case Extension::kSPV_AMD_shader_trinary_minmax: return IR(958, 30);
    case Extension::kSPV_AMD_texture_gather_bias_lod: return IR(988, 32);
    case Extension::kSPV_AMD_weak_linkage: return IR(1020, 21);
    case Extension::kSPV_ARM_cooperative_matrix_layouts: return IR(1041, 35);
    case Extension::kSPV_ARM_core_builtins: return IR(1076, 22);
    case Extension::kSPV_ARM_graph: return IR(1098, 14);
    case Extension::kSPV_ARM_tensors: return IR(1112, 16);
    case Extension::kSPV_EXT_arithmetic_fence: return IR(1128, 25);
    case Extension::kSPV_EXT_demote_to_helper_invocation: return IR(1153, 36);
    case Extension::kSPV_EXT_descriptor_heap: return IR(1189, 24);
    case Extension::kSPV_EXT_descriptor_indexing: return IR(1213, 28);
    case Extension::kSPV_EXT_float8: return IR(1241, 15);
    case Extension::kSPV_EXT_fragment_fully_covered: return IR(1256, 31);
    case Extension::kSPV_EXT_fragment_invocation_density: return IR(1287, 36);
    case Extension::kSPV_EXT_fragment_shader_interlock: return IR(1323, 34);
    case Extension::kSPV_EXT_long_vector: return IR(1357, 20);
    case Extension::kSPV_EXT_mesh_shader: return IR(1377, 20);
    case Extension::kSPV_EXT_ocp_microscaling_types: return IR(1397, 31);
    case Extension::kSPV_EXT_opacity_micromap: return IR(1428, 25);
    case Extension::kSPV_EXT_optnone: return IR(1453, 16);
    case Extension::kSPV_EXT_physical_storage_buffer: return IR(1469, 32);
    case Extension::kSPV_EXT_relaxed_printf_string_address_space: return IR(1501, 44);
    case Extension::kSPV_EXT_replicated_composites: return IR(1545, 30);
    case Extension::kSPV_EXT_shader_64bit_indexing: return IR(1575, 30);
    case Extension::kSPV_EXT_shader_atomic_float16_add: return IR(1605, 34);
    case Extension::kSPV_EXT_shader_atomic_float_add: return IR(1639, 32);
    case Extension::kSPV_EXT_shader_atomic_float_min_max: return IR(1671, 36);
    case Extension::kSPV_EXT_shader_image_int64: return IR(1707, 27);
    case Extension::kSPV_EXT_shader_invocation_reorder: return IR(1734, 34);
    case Extension::kSPV_EXT_shader_stencil_export: return IR(1768, 30);
    case Extension::kSPV_EXT_shader_subgroup_partitioned: return IR(1798, 36);
    case Extension::kSPV_EXT_shader_tile_image: return IR(1834, 26);
    case Extension::kSPV_EXT_shader_viewport_index_layer: return IR(1860, 36);
    case Extension::kSPV_EXT_split_barrier: return IR(1896, 22);
    case Extension::kSPV_GOOGLE_decorate_string: return IR(1918, 27);
    case Extension::kSPV_GOOGLE_hlsl_functionality1: return IR(1945, 31);
    case Extension::kSPV_GOOGLE_user_type: return IR(1976, 21);
    case Extension::kSPV_INTEL_2d_block_io: return IR(1997, 22);
    case Extension::kSPV_INTEL_arbitrary_precision_fixed_point: return IR(2019, 42);
    case Extension::kSPV_INTEL_arbitrary_precision_floating_point: return IR(2061, 45);
    case Extension::kSPV_INTEL_arbitrary_precision_integers: return IR(2106, 39);
    case Extension::kSPV_INTEL_bfloat16_conversion: return IR(2145, 30);
    case Extension::kSPV_INTEL_bindless_images: return IR(2175, 26);
    case Extension::kSPV_INTEL_blocking_pipes: return IR(2201, 25);
    case Extension::kSPV_INTEL_cache_controls: return IR(2226, 25);
    case Extension::kSPV_INTEL_debug_module: return IR(2251, 23);
    case Extension::kSPV_INTEL_device_side_avc_motion_estimation: return IR(2274, 44);
    case Extension::kSPV_INTEL_float_controls2: return IR(2318, 26);
    case Extension::kSPV_INTEL_fp_fast_math_mode: return IR(2344, 28);
    case Extension::kSPV_INTEL_fp_max_error: return IR(2372, 23);
    case Extension::kSPV_INTEL_fpga_argument_interfaces: return IR(2395, 35);
    case Extension::kSPV_INTEL_fpga_buffer_location: return IR(2430, 31);
    case Extension::kSPV_INTEL_fpga_cluster_attributes: return IR(2461, 34);
    case Extension::kSPV_INTEL_fpga_dsp_control: return IR(2495, 27);
    case Extension::kSPV_INTEL_fpga_invocation_pipelining_attributes: return IR(2522, 48);
    case Extension::kSPV_INTEL_fpga_latency_control: return IR(2570, 31);
    case Extension::kSPV_INTEL_fpga_loop_controls: return IR(2601, 29);
    case Extension::kSPV_INTEL_fpga_memory_accesses: return IR(2630, 31);
    case Extension::kSPV_INTEL_fpga_memory_attributes: return IR(2661, 33);
    case Extension::kSPV_INTEL_fpga_reg: return IR(2694, 19);
    case Extension::kSPV_INTEL_function_pointers: return IR(2713, 28);
    case Extension::kSPV_INTEL_function_variants: return IR(2741, 28);
    case Extension::kSPV_INTEL_global_variable_fpga_decorations: return IR(2769, 43);
    case Extension::kSPV_INTEL_global_variable_host_access: return IR(2812, 38);
    case Extension::kSPV_INTEL_inline_assembly: return IR(2850, 26);
    case Extension::kSPV_INTEL_int4: return IR(2876, 15);
    case Extension::kSPV_INTEL_io_pipes: return IR(2891, 19);
    case Extension::kSPV_INTEL_kernel_attributes: return IR(2910, 28);
    case Extension::kSPV_INTEL_long_composites: return IR(2938, 26);
    case Extension::kSPV_INTEL_loop_fuse: return IR(2964, 20);
    case Extension::kSPV_INTEL_masked_gather_scatter: return IR(2984, 32);
    case Extension::kSPV_INTEL_maximum_registers: return IR(3016, 28);
    case Extension::kSPV_INTEL_media_block_io: return IR(3044, 25);
    case Extension::kSPV_INTEL_memory_access_aliasing: return IR(3069, 33);
    case Extension::kSPV_INTEL_optnone: return IR(3102, 18);
    case Extension::kSPV_INTEL_predicated_io: return IR(3120, 24);
    case Extension::kSPV_INTEL_rounded_divide_sqrt: return IR(3144, 30);
    case Extension::kSPV_INTEL_runtime_aligned: return IR(3174, 26);
    case Extension::kSPV_INTEL_shader_integer_functions2: return IR(3200, 36);
    case Extension::kSPV_INTEL_split_barrier: return IR(3236, 24);
    case Extension::kSPV_INTEL_subgroup_buffer_prefetch: return IR(3260, 35);
    case Extension::kSPV_INTEL_subgroup_matrix_multiply_accumulate: return IR(3295, 46);
    case Extension::kSPV_INTEL_subgroups: return IR(3341, 20);
    case Extension::kSPV_INTEL_task_sequence: return IR(3361, 24);
    case Extension::kSPV_INTEL_tensor_float32_conversion: return IR(3385, 36);
    case Extension::kSPV_INTEL_ternary_bitwise_function: return IR(3421, 35);
    case Extension::kSPV_INTEL_unstructured_loop_controls: return IR(3456, 37);
    case Extension::kSPV_INTEL_usm_storage_classes: return IR(3493, 30);
    case Extension::kSPV_INTEL_variable_length_array: return IR(3523, 32);
    case Extension::kSPV_INTEL_vector_compute: return IR(3555, 25);
    case Extension::kSPV_KHR_16bit_storage: return IR(3580, 22);
    case Extension::kSPV_KHR_8bit_storage: return IR(3602, 21);
    case Extension::kSPV_KHR_abort: return IR(3623, 14);
    case Extension::kSPV_KHR_bfloat16: return IR(3637, 17);
    case Extension::kSPV_KHR_bit_instructions: return IR(3654, 25);
    case Extension::kSPV_KHR_compute_shader_derivatives: return IR(3679, 35);
    case Extension::kSPV_KHR_constant_data: return IR(3714, 22);
    case Extension::kSPV_KHR_cooperative_matrix: return IR(3736, 27);
    case Extension::kSPV_KHR_device_group: return IR(3763, 21);
    case Extension::kSPV_KHR_expect_assume: return IR(3784, 22);
    case Extension::kSPV_KHR_float_controls: return IR(3806, 23);
    case Extension::kSPV_KHR_float_controls2: return IR(3829, 24);
    case Extension::kSPV_KHR_fma: return IR(3853, 12);
    case Extension::kSPV_KHR_fragment_shader_barycentric: return IR(3865, 36);
    case Extension::kSPV_KHR_fragment_shading_rate: return IR(3901, 30);
    case Extension::kSPV_KHR_integer_dot_product: return IR(3931, 28);
    case Extension::kSPV_KHR_linkonce_odr: return IR(3959, 21);
    case Extension::kSPV_KHR_maximal_reconvergence: return IR(3980, 30);
    case Extension::kSPV_KHR_multiview: return IR(4010, 18);
    case Extension::kSPV_KHR_no_integer_wrap_decoration: return IR(4028, 35);
    case Extension::kSPV_KHR_non_semantic_info: return IR(4063, 26);
    case Extension::kSPV_KHR_opacity_micromap: return IR(4089, 25);
    case Extension::kSPV_KHR_physical_storage_buffer: return IR(4114, 32);
    case Extension::kSPV_KHR_poison_freeze: return IR(4146, 22);
    case Extension::kSPV_KHR_post_depth_coverage: return IR(4168, 28);
    case Extension::kSPV_KHR_quad_control: return IR(4196, 21);
    case Extension::kSPV_KHR_ray_cull_mask: return IR(4217, 22);
    case Extension::kSPV_KHR_ray_query: return IR(4239, 18);
    case Extension::kSPV_KHR_ray_tracing: return IR(4257, 20);
    case Extension::kSPV_KHR_ray_tracing_position_fetch: return IR(4277, 35);
    case Extension::kSPV_KHR_relaxed_extended_instruction: return IR(4312, 37);
    case Extension::kSPV_KHR_shader_atomic_counter_ops: return IR(4349, 34);
    case Extension::kSPV_KHR_shader_ballot: return IR(4383, 22);
    case Extension::kSPV_KHR_shader_clock: return IR(4405, 21);
    case Extension::kSPV_KHR_shader_draw_parameters: return IR(4426, 31);
    case Extension::kSPV_KHR_storage_buffer_storage_class: return IR(4457, 37);
    case Extension::kSPV_KHR_subgroup_rotate: return IR(4494, 24);
    case Extension::kSPV_KHR_subgroup_uniform_control_flow: return IR(4518, 38);
    case Extension::kSPV_KHR_subgroup_vote: return IR(4556, 22);
    case Extension::kSPV_KHR_terminate_invocation: return IR(4578, 29);
    case Extension::kSPV_KHR_uniform_group_instructions: return IR(4607, 35);
    case Extension::kSPV_KHR_untyped_pointers: return IR(4642, 25);
    case Extension::kSPV_KHR_variable_pointers: return IR(4667, 26);
    case Extension::kSPV_KHR_vulkan_memory_model: return IR(4693, 28);
    case Extension::kSPV_KHR_workgroup_memory_explicit_layout: return IR(4721, 41);
    case Extension::kSPV_NVX_multiview_per_view_attributes: return IR(4762, 38);
    case Extension::kSPV_NV_bindless_texture: return IR(4800, 24);
    case Extension::kSPV_NV_cluster_acceleration_structure: return IR(4824, 38);
    case Extension::kSPV_NV_compute_shader_derivatives: return IR(4862, 34);
    case Extension::kSPV_NV_cooperative_matrix: return IR(4896, 26);
    case Extension::kSPV_NV_cooperative_matrix2: return IR(4922, 27);
    case Extension::kSPV_NV_cooperative_matrix_decode_vector: return IR(4949, 40);
    case Extension::kSPV_NV_cooperative_vector: return IR(4989, 26);
    case Extension::kSPV_NV_displacement_micromap: return IR(5015, 29);
    case Extension::kSPV_NV_fragment_shader_barycentric: return IR(5044, 35);
    case Extension::kSPV_NV_geometry_shader_passthrough: return IR(5079, 35);
    case Extension::kSPV_NV_linear_swept_spheres: return IR(5114, 28);
    case Extension::kSPV_NV_mesh_shader: return IR(5142, 19);
    case Extension::kSPV_NV_push_constant_bank: return IR(5161, 26);
    case Extension::kSPV_NV_raw_access_chains: return IR(5187, 25);
    case Extension::kSPV_NV_ray_tracing: return IR(5212, 19);
    case Extension::kSPV_NV_ray_tracing_motion_blur: return IR(5231, 31);
    case Extension::kSPV_NV_sample_mask_override_coverage: return IR(5262, 37);
    case Extension::kSPV_NV_shader_atomic_fp16_vector: return IR(5299, 33);
    case Extension::kSPV_NV_shader_image_footprint: return IR(5332, 30);
    case Extension::kSPV_NV_shader_invocation_reorder: return IR(5362, 33);
    case Extension::kSPV_NV_shader_sm_builtins: return IR(5395, 26);
    case Extension::kSPV_NV_shader_subgroup_partitioned: return IR(5421, 35);
    case Extension::kSPV_NV_shading_rate: return IR(5456, 20);
    case Extension::kSPV_NV_stereo_view_rendering: return IR(5476, 29);
    case Extension::kSPV_NV_tensor_addressing: return IR(5505, 25);
    case Extension::kSPV_NV_viewport_array2: return IR(5530, 23);
    case Extension::kSPV_QCOM_cooperative_matrix_conversion: return IR(5553, 39);
    case Extension::kSPV_QCOM_image_processing: return IR(5592, 26);
    case Extension::kSPV_QCOM_image_processing2: return IR(5618, 27);
    case Extension::kSPV_QCOM_image_processing3: return IR(5645, 27);
    case Extension::kSPV_QCOM_multiple_wait_queues: return IR(5672, 30);
    case Extension::kSPV_QCOM_tile_shading: return IR(5702, 22);
    case Extension::kSPV_VALIDATOR_ignore_type_decl_unique: return IR(5724, 38);
    case Extension::kSPV_VALVE_mixed_float_dot_product: return IR(5762, 34);
    default: break;
  }
  return {};
}

// Extension names and values, ordered by name
// The fields in order are:
//   name, indexing into kStrings
//   enum value
static const std::array<NameValue,200> kExtensionNames{{
    {IR(0, 43), static_cast<uint32_t>(kSPV_ALTERA_arbitrary_precision_fixed_point)},
    {IR(43, 46), static_cast<uint32_t>(kSPV_ALTERA_arbitrary_precision_floating_point)},
    {IR(89, 40), static_cast<uint32_t>(kSPV_ALTERA_arbitrary_precision_integers)},
    {IR(129, 26), static_cast<uint32_t>(kSPV_ALTERA_blocking_pipes)},
    {IR(155, 36), static_cast<uint32_t>(kSPV_ALTERA_fpga_argument_interfaces)},
    {IR(191, 32), static_cast<uint32_t>(kSPV_ALTERA_fpga_buffer_location)},
    {IR(223, 35), static_cast<uint32_t>(kSPV_ALTERA_fpga_cluster_attributes)},
    {IR(258, 28), static_cast<uint32_t>(kSPV_ALTERA_fpga_dsp_control)},
    {IR(286, 49), static_cast<uint32_t>(kSPV_ALTERA_fpga_invocation_pipelining_attributes)},
    {IR(335, 32), static_cast<uint32_t>(kSPV_ALTERA_fpga_latency_control)},
    {IR(367, 30), static_cast<uint32_t>(kSPV_ALTERA_fpga_loop_controls)},
    {IR(397, 32), static_cast<uint32_t>(kSPV_ALTERA_fpga_memory_accesses)},
    {IR(429, 34), static_cast<uint32_t>(kSPV_ALTERA_fpga_memory_attributes)},
    {IR(463, 20), static_cast<uint32_t>(kSPV_ALTERA_fpga_reg)},
    {IR(483, 44), static_cast<uint32_t>(kSPV_ALTERA_global_variable_fpga_decorations)},
    {IR(527, 20), static_cast<uint32_t>(kSPV_ALTERA_io_pipes)},
    {IR(547, 21), static_cast<uint32_t>(kSPV_ALTERA_loop_fuse)},
    {IR(568, 27), static_cast<uint32_t>(kSPV_ALTERA_runtime_aligned)},
    {IR(595, 25), static_cast<uint32_t>(kSPV_ALTERA_task_sequence)},
    {IR(620, 31), static_cast<uint32_t>(kSPV_ALTERA_usm_storage_classes)},
    {IR(651, 24), static_cast<uint32_t>(kSPV_AMDX_shader_enqueue)},
    {IR(675, 19), static_cast<uint32_t>(kSPV_AMD_gcn_shader)},
    {IR(694, 30), static_cast<uint32_t>(kSPV_AMD_gpu_shader_half_float)},
    {IR(724, 36), static_cast<uint32_t>(kSPV_AMD_gpu_shader_half_float_fetch)},
    {IR(760, 25), static_cast<uint32_t>(kSPV_AMD_gpu_shader_int16)},
    {IR(785, 22), static_cast<uint32_t>(kSPV_AMD_shader_ballot)},
    {IR(807, 45), static_cast<uint32_t>(kSPV_AMD_shader_early_and_late_fragment_tests)},
    {IR(852, 41), static_cast<uint32_t>(kSPV_AMD_shader_explicit_vertex_parameter)},
    {IR(893, 29), static_cast<uint32_t>(kSPV_AMD_shader_fragment_mask)},
    {IR(922, 36), static_cast<uint32_t>(kSPV_AMD_shader_image_load_store_lod)},
    {IR(958, 30), static_cast<uint32_t>(kSPV_AMD_shader_trinary_minmax)},
    {IR(988, 32), static_cast<uint32_t>(kSPV_AMD_texture_gather_bias_lod)},
    {IR(1020, 21), static_cast<uint32_t>(kSPV_AMD_weak_linkage)},
    {IR(1041, 35), static_cast<uint32_t>(kSPV_ARM_cooperative_matrix_layouts)},
    {IR(1076, 22), static_cast<uint32_t>(kSPV_ARM_core_builtins)},
    {IR(1098, 14), static_cast<uint32_t>(kSPV_ARM_graph)},
    {IR(1112, 16), static_cast<uint32_t>(kSPV_ARM_tensors)},
    {IR(1128, 25), static_cast<uint32_t>(kSPV_EXT_arithmetic_fence)},
    {IR(1153, 36), static_cast<uint32_t>(kSPV_EXT_demote_to_helper_invocation)},
    {IR(1189, 24), static_cast<uint32_t>(kSPV_EXT_descriptor_heap)},
    {IR(1213, 28), static_cast<uint32_t>(kSPV_EXT_descriptor_indexing)},
    {IR(1241, 15), static_cast<uint32_t>(kSPV_EXT_float8)},
    {IR(1256, 31), static_cast<uint32_t>(kSPV_EXT_fragment_fully_covered)},
    {IR(1287, 36), static_cast<uint32_t>(kSPV_EXT_fragment_invocation_density)},
    {IR(1323, 34), static_cast<uint32_t>(kSPV_EXT_fragment_shader_interlock)},
    {IR(1357, 20), static_cast<uint32_t>(kSPV_EXT_long_vector)},
    {IR(1377, 20), static_cast<uint32_t>(kSPV_EXT_mesh_shader)},
    {IR(1397, 31), static_cast<uint32_t>(kSPV_EXT_ocp_microscaling_types)},
    {IR(1428, 25), static_cast<uint32_t>(kSPV_EXT_opacity_micromap)},
    {IR(1453, 16), static_cast<uint32_t>(kSPV_EXT_optnone)},
    {IR(1469, 32), static_cast<uint32_t>(kSPV_EXT_physical_storage_buffer)},
    {IR(1501, 44), static_cast<uint32_t>(kSPV_EXT_relaxed_printf_string_address_space)},
    {IR(1545, 30), static_cast<uint32_t>(kSPV_EXT_replicated_composites)},
    {IR(1575, 30), static_cast<uint32_t>(kSPV_EXT_shader_64bit_indexing)},
    {IR(1605, 34), static_cast<uint32_t>(kSPV_EXT_shader_atomic_float16_add)},
    {IR(1639, 32), static_cast<uint32_t>(kSPV_EXT_shader_atomic_float_add)},
    {IR(1671, 36), static_cast<uint32_t>(kSPV_EXT_shader_atomic_float_min_max)},
    {IR(1707, 27), static_cast<uint32_t>(kSPV_EXT_shader_image_int64)},
    {IR(1734, 34), static_cast<uint32_t>(kSPV_EXT_shader_invocation_reorder)},
    {IR(1768, 30), static_cast<uint32_t>(kSPV_EXT_shader_stencil_export)},
    {IR(1798, 36), static_cast<uint32_t>(kSPV_EXT_shader_subgroup_partitioned)},
    {IR(1834, 26), static_cast<uint32_t>(kSPV_EXT_shader_tile_image)},
    {IR(1860, 36), static_cast<uint32_t>(kSPV_EXT_shader_viewport_index_layer)},
    {IR(1896, 22), static_cast<uint32_t>(kSPV_EXT_split_barrier)},
    {IR(1918, 27), static_cast<uint32_t>(kSPV_GOOGLE_decorate_string)},
    {IR(1945, 31), static_cast<uint32_t>(kSPV_GOOGLE_hlsl_functionality1)},
    {IR(1976, 21), static_cast<uint32_t>(kSPV_GOOGLE_user_type)},
    {IR(1997, 22), static_cast<uint32_t>(kSPV_INTEL_2d_block_io)},
    {IR(2019, 42), static_cast<uint32_t>(kSPV_INTEL_arbitrary_precision_fixed_point)},
    {IR(2061, 45), static_cast<uint32_t>(kSPV_INTEL_arbitrary_precision_floating_point)},
    {IR(2106, 39), static_cast<uint32_t>(kSPV_INTEL_arbitrary_precision_integers)},
    {IR(2145, 30), static_cast<uint32_t>(kSPV_INTEL_bfloat16_conversion)},
    {IR(2175, 26), static_cast<uint32_t>(kSPV_INTEL_bindless_images)},
    {IR(2201, 25), static_cast<uint32_t>(kSPV_INTEL_blocking_pipes)},
    {IR(2226, 25), static_cast<uint32_t>(kSPV_INTEL_cache_controls)},
    {IR(2251, 23), static_cast<uint32_t>(kSPV_INTEL_debug_module)},
    {IR(2274, 44), static_cast<uint32_t>(kSPV_INTEL_device_side_avc_motion_estimation)},
    {IR(2318, 26), static_cast<uint32_t>(kSPV_INTEL_float_controls2)},
    {IR(2344, 28), static_cast<uint32_t>(kSPV_INTEL_fp_fast_math_mode)},
    {IR(2372, 23), static_cast<uint32_t>(kSPV_INTEL_fp_max_error)},
    {IR(2395, 35), static_cast<uint32_t>(kSPV_INTEL_fpga_argument_interfaces)},
    {IR(2430, 31), static_cast<uint32_t>(kSPV_INTEL_fpga_buffer_location)},
    {IR(2461, 34), static_cast<uint32_t>(kSPV_INTEL_fpga_cluster_attributes)},
    {IR(2495, 27), static_cast<uint32_t>(kSPV_INTEL_fpga_dsp_control)},
    {IR(2522, 48), static_cast<uint32_t>(kSPV_INTEL_fpga_invocation_pipelining_attributes)},
    {IR(2570, 31), static_cast<uint32_t>(kSPV_INTEL_fpga_latency_control)},
    {IR(2601, 29), static_cast<uint32_t>(kSPV_INTEL_fpga_loop_controls)},
    {IR(2630, 31), static_cast<uint32_t>(kSPV_INTEL_fpga_memory_accesses)},
    {IR(2661, 33), static_cast<uint32_t>(kSPV_INTEL_fpga_memory_attributes)},
    {IR(2694, 19), static_cast<uint32_t>(kSPV_INTEL_fpga_reg)},
    {IR(2713, 28), static_cast<uint32_t>(kSPV_INTEL_function_pointers)},
    {IR(2741, 28), static_cast<uint32_t>(kSPV_INTEL_function_variants)},
    {IR(2769, 43), static_cast<uint32_t>(kSPV_INTEL_global_variable_fpga_decorations)},
    {IR(2812, 38), static_cast<uint32_t>(kSPV_INTEL_global_variable_host_access)},
    {IR(2850, 26), static_cast<uint32_t>(kSPV_INTEL_inline_assembly)},
    {IR(2876, 15), static_cast<uint32_t>(kSPV_INTEL_int4)},
    {IR(2891, 19), static_cast<uint32_t>(kSPV_INTEL_io_pipes)},
    {IR(2910, 28), static_cast<uint32_t>(kSPV_INTEL_kernel_attributes)},
    {IR(2938, 26), static_cast<uint32_t>(kSPV_INTEL_long_composites)},
    {IR(2964, 20), static_cast<uint32_t>(kSPV_INTEL_loop_fuse)},
    {IR(2984, 32), static_cast<uint32_t>(kSPV_INTEL_masked_gather_scatter)},
    {IR(3016, 28), static_cast<uint32_t>(kSPV_INTEL_maximum_registers)},
    {IR(3044, 25), static_cast<uint32_t>(kSPV_INTEL_media_block_io)},
    {IR(3069, 33), static_cast<uint32_t>(kSPV_INTEL_memory_access_aliasing)},
    {IR(3102, 18), static_cast<uint32_t>(kSPV_INTEL_optnone)},
    {IR(3120, 24), static_cast<uint32_t>(kSPV_INTEL_predicated_io)},
    {IR(3144, 30), static_cast<uint32_t>(kSPV_INTEL_rounded_divide_sqrt)},
    {IR(3174, 26), static_cast<uint32_t>(kSPV_INTEL_runtime_aligned)},
    {IR(3200, 36), static_cast<uint32_t>(kSPV_INTEL_shader_integer_functions2)},
    {IR(3236, 24), static_cast<uint32_t>(kSPV_INTEL_split_barrier)},
    {IR(3260, 35), static_cast<uint32_t>(kSPV_INTEL_subgroup_buffer_prefetch)},
    {IR(3295, 46), static_cast<uint32_t>(kSPV_INTEL_subgroup_matrix_multiply_accumulate)},
    {IR(3341, 20), static_cast<uint32_t>(kSPV_INTEL_subgroups)},
    {IR(3361, 24), static_cast<uint32_t>(kSPV_INTEL_task_sequence)},
    {IR(3385, 36), static_cast<uint32_t>(kSPV_INTEL_tensor_float32_conversion)},
    {IR(3421, 35), static_cast<uint32_t>(kSPV_INTEL_ternary_bitwise_function)},
    {IR(3456, 37), static_cast<uint32_t>(kSPV_INTEL_unstructured_loop_controls)},
    {IR(3493, 30), static_cast<uint32_t>(kSPV_INTEL_usm_storage_classes)},
    {IR(3523, 32), static_cast<uint32_t>(kSPV_INTEL_variable_length_array)},
    {IR(3555, 25), static_cast<uint32_t>(kSPV_INTEL_vector_compute)},
    {IR(3580, 22), static_cast<uint32_t>(kSPV_KHR_16bit_storage)},
    {IR(3602, 21), static_cast<uint32_t>(kSPV_KHR_8bit_storage)},
    {IR(3623, 14), static_cast<uint32_t>(kSPV_KHR_abort)},
    {IR(3637, 17), static_cast<uint32_t>(kSPV_KHR_bfloat16)},
    {IR(3654, 25), static_cast<uint32_t>(kSPV_KHR_bit_instructions)},
    {IR(3679, 35), static_cast<uint32_t>(kSPV_KHR_compute_shader_derivatives)},
    {IR(3714, 22), static_cast<uint32_t>(kSPV_KHR_constant_data)},
    {IR(3736, 27), static_cast<uint32_t>(kSPV_KHR_cooperative_matrix)},
    {IR(3763, 21), static_cast<uint32_t>(kSPV_KHR_device_group)},
    {IR(3784, 22), static_cast<uint32_t>(kSPV_KHR_expect_assume)},
    {IR(3806, 23), static_cast<uint32_t>(kSPV_KHR_float_controls)},
    {IR(3829, 24), static_cast<uint32_t>(kSPV_KHR_float_controls2)},
    {IR(3853, 12), static_cast<uint32_t>(kSPV_KHR_fma)},
    {IR(3865, 36), static_cast<uint32_t>(kSPV_KHR_fragment_shader_barycentric)},
    {IR(3901, 30), static_cast<uint32_t>(kSPV_KHR_fragment_shading_rate)},
    {IR(3931, 28), static_cast<uint32_t>(kSPV_KHR_integer_dot_product)},
    {IR(3959, 21), static_cast<uint32_t>(kSPV_KHR_linkonce_odr)},
    {IR(3980, 30), static_cast<uint32_t>(kSPV_KHR_maximal_reconvergence)},
    {IR(4010, 18), static_cast<uint32_t>(kSPV_KHR_multiview)},
    {IR(4028, 35), static_cast<uint32_t>(kSPV_KHR_no_integer_wrap_decoration)},
    {IR(4063, 26), static_cast<uint32_t>(kSPV_KHR_non_semantic_info)},
    {IR(4089, 25), static_cast<uint32_t>(kSPV_KHR_opacity_micromap)},
    {IR(4114, 32), static_cast<uint32_t>(kSPV_KHR_physical_storage_buffer)},
    {IR(4146, 22), static_cast<uint32_t>(kSPV_KHR_poison_freeze)},
    {IR(4168, 28), static_cast<uint32_t>(kSPV_KHR_post_depth_coverage)},
    {IR(4196, 21), static_cast<uint32_t>(kSPV_KHR_quad_control)},
    {IR(4217, 22), static_cast<uint32_t>(kSPV_KHR_ray_cull_mask)},
    {IR(4239, 18), static_cast<uint32_t>(kSPV_KHR_ray_query)},
    {IR(4257, 20), static_cast<uint32_t>(kSPV_KHR_ray_tracing)},
    {IR(4277, 35), static_cast<uint32_t>(kSPV_KHR_ray_tracing_position_fetch)},
    {IR(4312, 37), static_cast<uint32_t>(kSPV_KHR_relaxed_extended_instruction)},
    {IR(4349, 34), static_cast<uint32_t>(kSPV_KHR_shader_atomic_counter_ops)},
    {IR(4383, 22), static_cast<uint32_t>(kSPV_KHR_shader_ballot)},
    {IR(4405, 21), static_cast<uint32_t>(kSPV_KHR_shader_clock)},
    {IR(4426, 31), static_cast<uint32_t>(kSPV_KHR_shader_draw_parameters)},
    {IR(4457, 37), static_cast<uint32_t>(kSPV_KHR_storage_buffer_storage_class)},
    {IR(4494, 24), static_cast<uint32_t>(kSPV_KHR_subgroup_rotate)},
    {IR(4518, 38), static_cast<uint32_t>(kSPV_KHR_subgroup_uniform_control_flow)},
    {IR(4556, 22), static_cast<uint32_t>(kSPV_KHR_subgroup_vote)},
    {IR(4578, 29), static_cast<uint32_t>(kSPV_KHR_terminate_invocation)},
    {IR(4607, 35), static_cast<uint32_t>(kSPV_KHR_uniform_group_instructions)},
    {IR(4642, 25), static_cast<uint32_t>(kSPV_KHR_untyped_pointers)},
    {IR(4667, 26), static_cast<uint32_t>(kSPV_KHR_variable_pointers)},
    {IR(4693, 28), static_cast<uint32_t>(kSPV_KHR_vulkan_memory_model)},
    {IR(4721, 41), static_cast<uint32_t>(kSPV_KHR_workgroup_memory_explicit_layout)},
    {IR(4762, 38), static_cast<uint32_t>(kSPV_NVX_multiview_per_view_attributes)},
    {IR(4800, 24), static_cast<uint32_t>(kSPV_NV_bindless_texture)},
    {IR(4824, 38), static_cast<uint32_t>(kSPV_NV_cluster_acceleration_structure)},
    {IR(4862, 34), static_cast<uint32_t>(kSPV_NV_compute_shader_derivatives)},
    {IR(4896, 26), static_cast<uint32_t>(kSPV_NV_cooperative_matrix)},
    {IR(4922, 27), static_cast<uint32_t>(kSPV_NV_cooperative_matrix2)},
    {IR(4949, 40), static_cast<uint32_t>(kSPV_NV_cooperative_matrix_decode_vector)},
    {IR(4989, 26), static_cast<uint32_t>(kSPV_NV_cooperative_vector)},
    {IR(5015, 29), static_cast<uint32_t>(kSPV_NV_displacement_micromap)},
    {IR(5044, 35), static_cast<uint32_t>(kSPV_NV_fragment_shader_barycentric)},
    {IR(5079, 35), static_cast<uint32_t>(kSPV_NV_geometry_shader_passthrough)},
    {IR(5114, 28), static_cast<uint32_t>(kSPV_NV_linear_swept_spheres)},
    {IR(5142, 19), static_cast<uint32_t>(kSPV_NV_mesh_shader)},
    {IR(5161, 26), static_cast<uint32_t>(kSPV_NV_push_constant_bank)},
    {IR(5187, 25), static_cast<uint32_t>(kSPV_NV_raw_access_chains)},
    {IR(5212, 19), static_cast<uint32_t>(kSPV_NV_ray_tracing)},
    {IR(5231, 31), static_cast<uint32_t>(kSPV_NV_ray_tracing_motion_blur)},
    {IR(5262, 37), static_cast<uint32_t>(kSPV_NV_sample_mask_override_coverage)},
    {IR(5299, 33), static_cast<uint32_t>(kSPV_NV_shader_atomic_fp16_vector)},
    {IR(5332, 30), static_cast<uint32_t>(kSPV_NV_shader_image_footprint)},
    {IR(5362, 33), static_cast<uint32_t>(kSPV_NV_shader_invocation_reorder)},
    {IR(5395, 26), static_cast<uint32_t>(kSPV_NV_shader_sm_builtins)},
    {IR(5421, 35), static_cast<uint32_t>(kSPV_NV_shader_subgroup_partitioned)},
    {IR(5456, 20), static_cast<uint32_t>(kSPV_NV_shading_rate)},
    {IR(5476, 29), static_cast<uint32_t>(kSPV_NV_stereo_view_rendering)},
    {IR(5505, 25), static_cast<uint32_t>(kSPV_NV_tensor_addressing)},
    {IR(5530, 23), static_cast<uint32_t>(kSPV_NV_viewport_array2)},
    {IR(5553, 39), static_cast<uint32_t>(kSPV_QCOM_cooperative_matrix_conversion)},
    {IR(5592, 26), static_cast<uint32_t>(kSPV_QCOM_image_processing)},
    {IR(5618, 27), static_cast<uint32_t>(kSPV_QCOM_image_processing2)},
    {IR(5645, 27), static_cast<uint32_t>(kSPV_QCOM_image_processing3)},
    {IR(5672, 30), static_cast<uint32_t>(kSPV_QCOM_multiple_wait_queues)},
    {IR(5702, 22), static_cast<uint32_t>(kSPV_QCOM_tile_shading)},
    {IR(5724, 38), static_cast<uint32_t>(kSPV_VALIDATOR_ignore_type_decl_unique)},
    {IR(5762, 34), static_cast<uint32_t>(kSPV_VALVE_mixed_float_dot_product)},
}};

// Operand descriptions, ordered by (operand kind, operand enum value).
// The fields in order are:
//   enum value
//   operands, an IndexRange into kOperandSpans
//   name, a character-counting IndexRange into kStrings
//   aliases, an IndexRange into kAliasSpans
//   capabilities, an IndexRange into kCapabilitySpans
//   extensions, as an IndexRange into kExtensionSpans
//   version, first version of SPIR-V that has it
//   lastVersion, last version of SPIR-V that has it
static const std::array<OperandDesc, 1240> kOperandsByValue{{
  {0,IR(0, 0),IR(5796, 9)/* ReadOnly */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_ACCESS_QUALIFIER
  {1,IR(0, 0),IR(5812, 10)/* WriteOnly */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_ACCESS_QUALIFIER
  {2,IR(0, 0),IR(5822, 10)/* ReadWrite */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_ACCESS_QUALIFIER
  {0,IR(0, 0),IR(5832, 8)/* Logical */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_ADDRESSING_MODEL
  {1,IR(0, 0),IR(5840, 11)/* Physical32 */,IR(0, 0),IR(1, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_ADDRESSING_MODEL
  {2,IR(0, 0),IR(5861, 11)/* Physical64 */,IR(0, 0),IR(1, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_ADDRESSING_MODEL
  {5348,IR(0, 0),IR(5872, 24)/* PhysicalStorageBuffer64 */,IR(0, 1),IR(2, 1),IR(0, 2),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_ADDRESSING_MODEL
  {0,IR(0, 0),IR(5954, 9)/* Position */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {1,IR(0, 0),IR(5970, 10)/* PointSize */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {3,IR(0, 0),IR(5980, 13)/* ClipDistance */,IR(0, 0),IR(4, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4,IR(0, 0),IR(5993, 13)/* CullDistance */,IR(0, 0),IR(5, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5,IR(0, 0),IR(6006, 9)/* VertexId */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {6,IR(0, 0),IR(6015, 11)/* InstanceId */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {7,IR(0, 0),IR(6026, 12)/* PrimitiveId */,IR(0, 0),IR(6, 6),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {8,IR(0, 0),IR(6116, 13)/* InvocationId */,IR(0, 0),IR(12, 2),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {9,IR(0, 0),IR(6129, 6)/* Layer */,IR(0, 0),IR(14, 5),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {10,IR(0, 0),IR(6175, 14)/* ViewportIndex */,IR(0, 0),IR(19, 5),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {11,IR(0, 0),IR(6223, 15)/* TessLevelOuter */,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {12,IR(0, 0),IR(6238, 15)/* TessLevelInner */,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {13,IR(0, 0),IR(6253, 10)/* TessCoord */,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {14,IR(0, 0),IR(6263, 14)/* PatchVertices */,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {15,IR(0, 0),IR(6277, 10)/* FragCoord */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {16,IR(0, 0),IR(6287, 11)/* PointCoord */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {17,IR(0, 0),IR(6298, 12)/* FrontFacing */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {18,IR(0, 0),IR(6310, 9)/* SampleId */,IR(0, 0),IR(25, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {19,IR(0, 0),IR(6337, 15)/* SamplePosition */,IR(0, 0),IR(25, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {20,IR(0, 0),IR(6352, 11)/* SampleMask */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {22,IR(0, 0),IR(6363, 10)/* FragDepth */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {23,IR(0, 0),IR(6373, 17)/* HelperInvocation */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {24,IR(0, 0),IR(6390, 14)/* NumWorkgroups */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {25,IR(0, 0),IR(6404, 14)/* WorkgroupSize */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {26,IR(0, 0),IR(6418, 12)/* WorkgroupId */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {27,IR(0, 0),IR(6430, 18)/* LocalInvocationId */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {28,IR(0, 0),IR(6448, 19)/* GlobalInvocationId */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {29,IR(0, 0),IR(6467, 21)/* LocalInvocationIndex */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {30,IR(0, 0),IR(6488, 8)/* WorkDim */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {31,IR(0, 0),IR(6496, 11)/* GlobalSize */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {32,IR(0, 0),IR(6507, 22)/* EnqueuedWorkgroupSize */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {33,IR(0, 0),IR(6529, 13)/* GlobalOffset */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {34,IR(0, 0),IR(6542, 15)/* GlobalLinearId */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {36,IR(0, 0),IR(6557, 13)/* SubgroupSize */,IR(0, 0),IR(26, 3),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {37,IR(0, 0),IR(6604, 16)/* SubgroupMaxSize */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {38,IR(0, 0),IR(6620, 13)/* NumSubgroups */,IR(0, 0),IR(29, 2),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {39,IR(0, 0),IR(6633, 21)/* NumEnqueuedSubgroups */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {40,IR(0, 0),IR(6654, 11)/* SubgroupId */,IR(0, 0),IR(29, 2),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {41,IR(0, 0),IR(6665, 26)/* SubgroupLocalInvocationId */,IR(0, 0),IR(26, 3),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {42,IR(0, 0),IR(6691, 12)/* VertexIndex */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {43,IR(0, 0),IR(6703, 14)/* InstanceIndex */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4160,IR(0, 0),IR(6717, 10)/* CoreIDARM */,IR(0, 0),IR(31, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4161,IR(0, 0),IR(6743, 13)/* CoreCountARM */,IR(0, 0),IR(31, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4162,IR(0, 0),IR(6756, 13)/* CoreMaxIDARM */,IR(0, 0),IR(31, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4163,IR(0, 0),IR(6769, 10)/* WarpIDARM */,IR(0, 0),IR(31, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4164,IR(0, 0),IR(6779, 13)/* WarpMaxIDARM */,IR(0, 0),IR(31, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4416,IR(0, 0),IR(6792, 15)/* SubgroupEqMask */,IR(1, 1),IR(32, 2),IR(2, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4417,IR(0, 0),IR(6847, 15)/* SubgroupGeMask */,IR(2, 1),IR(32, 2),IR(2, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4418,IR(0, 0),IR(6880, 15)/* SubgroupGtMask */,IR(3, 1),IR(32, 2),IR(2, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4419,IR(0, 0),IR(6913, 15)/* SubgroupLeMask */,IR(4, 1),IR(32, 2),IR(2, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4420,IR(0, 0),IR(6946, 15)/* SubgroupLtMask */,IR(5, 1),IR(32, 2),IR(2, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4424,IR(0, 0),IR(6979, 11)/* BaseVertex */,IR(0, 0),IR(34, 1),IR(3, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4425,IR(0, 0),IR(7005, 13)/* BaseInstance */,IR(0, 0),IR(34, 1),IR(3, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4426,IR(0, 0),IR(7018, 10)/* DrawIndex */,IR(0, 0),IR(35, 3),IR(4, 3),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4432,IR(0, 0),IR(7028, 24)/* PrimitiveShadingRateKHR */,IR(0, 0),IR(38, 1),IR(7, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4438,IR(0, 0),IR(7075, 12)/* DeviceIndex */,IR(0, 0),IR(39, 1),IR(8, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4440,IR(0, 0),IR(7099, 10)/* ViewIndex */,IR(0, 0),IR(40, 1),IR(9, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4444,IR(0, 0),IR(7119, 15)/* ShadingRateKHR */,IR(0, 0),IR(38, 1),IR(7, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4492,IR(0, 0),IR(7134, 15)/* TileOffsetQCOM */,IR(0, 0),IR(41, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4493,IR(0, 0),IR(7165, 18)/* TileDimensionQCOM */,IR(0, 0),IR(41, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4494,IR(0, 0),IR(7183, 18)/* TileApronSizeQCOM */,IR(0, 0),IR(41, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4992,IR(0, 0),IR(7201, 20)/* BaryCoordNoPerspAMD */,IR(0, 0),IR(1, 0),IR(10, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4993,IR(0, 0),IR(7221, 28)/* BaryCoordNoPerspCentroidAMD */,IR(0, 0),IR(1, 0),IR(10, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4994,IR(0, 0),IR(7249, 26)/* BaryCoordNoPerspSampleAMD */,IR(0, 0),IR(1, 0),IR(10, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4995,IR(0, 0),IR(7275, 19)/* BaryCoordSmoothAMD */,IR(0, 0),IR(1, 0),IR(10, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4996,IR(0, 0),IR(7294, 27)/* BaryCoordSmoothCentroidAMD */,IR(0, 0),IR(1, 0),IR(10, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4997,IR(0, 0),IR(7321, 25)/* BaryCoordSmoothSampleAMD */,IR(0, 0),IR(1, 0),IR(10, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {4998,IR(0, 0),IR(7346, 22)/* BaryCoordPullModelAMD */,IR(0, 0),IR(1, 0),IR(10, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5014,IR(0, 0),IR(7368, 18)/* FragStencilRefEXT */,IR(0, 0),IR(42, 1),IR(11, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5021,IR(0, 0),IR(7403, 29)/* RemainingRecursionLevelsAMDX */,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5073,IR(0, 0),IR(7450, 16)/* ShaderIndexAMDX */,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5122,IR(0, 0),IR(7466, 15)/* SamplerHeapEXT */,IR(0, 0),IR(44, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5123,IR(0, 0),IR(7499, 16)/* ResourceHeapEXT */,IR(0, 0),IR(44, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5253,IR(0, 0),IR(7515, 15)/* ViewportMaskNV */,IR(0, 0),IR(45, 2),IR(12, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5257,IR(0, 0),IR(7551, 20)/* SecondaryPositionNV */,IR(0, 0),IR(47, 1),IR(14, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5258,IR(0, 0),IR(7590, 24)/* SecondaryViewportMaskNV */,IR(0, 0),IR(47, 1),IR(14, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5261,IR(0, 0),IR(7614, 18)/* PositionPerViewNV */,IR(0, 0),IR(48, 2),IR(15, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5262,IR(0, 0),IR(7652, 22)/* ViewportMaskPerViewNV */,IR(0, 0),IR(48, 2),IR(15, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5264,IR(0, 0),IR(7674, 16)/* FullyCoveredEXT */,IR(0, 0),IR(50, 1),IR(17, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5274,IR(0, 0),IR(7714, 12)/* TaskCountNV */,IR(0, 0),IR(51, 1),IR(18, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5275,IR(0, 0),IR(7726, 17)/* PrimitiveCountNV */,IR(0, 0),IR(51, 1),IR(18, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5276,IR(0, 0),IR(7743, 19)/* PrimitiveIndicesNV */,IR(0, 0),IR(51, 1),IR(18, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5277,IR(0, 0),IR(7762, 22)/* ClipDistancePerViewNV */,IR(0, 0),IR(51, 1),IR(18, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5278,IR(0, 0),IR(7784, 22)/* CullDistancePerViewNV */,IR(0, 0),IR(51, 1),IR(18, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5279,IR(0, 0),IR(7806, 15)/* LayerPerViewNV */,IR(0, 0),IR(51, 1),IR(18, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5280,IR(0, 0),IR(7821, 16)/* MeshViewCountNV */,IR(0, 0),IR(51, 1),IR(18, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5281,IR(0, 0),IR(7837, 18)/* MeshViewIndicesNV */,IR(0, 0),IR(51, 1),IR(18, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5286,IR(0, 0),IR(7855, 13)/* BaryCoordKHR */,IR(6, 1),IR(52, 1),IR(19, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5287,IR(0, 0),IR(7903, 20)/* BaryCoordNoPerspKHR */,IR(7, 1),IR(52, 1),IR(19, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5292,IR(0, 0),IR(7942, 12)/* FragSizeEXT */,IR(8, 1),IR(53, 1),IR(21, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5293,IR(0, 0),IR(7988, 23)/* FragInvocationCountEXT */,IR(9, 1),IR(53, 1),IR(21, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5294,IR(0, 0),IR(8033, 25)/* PrimitivePointIndicesEXT */,IR(0, 0),IR(54, 1),IR(23, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5295,IR(0, 0),IR(8058, 24)/* PrimitiveLineIndicesEXT */,IR(0, 0),IR(54, 1),IR(23, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5296,IR(0, 0),IR(8082, 28)/* PrimitiveTriangleIndicesEXT */,IR(0, 0),IR(54, 1),IR(23, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5299,IR(0, 0),IR(8110, 17)/* CullPrimitiveEXT */,IR(0, 0),IR(54, 1),IR(23, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5319,IR(0, 0),IR(8127, 12)/* LaunchIdKHR */,IR(10, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5320,IR(0, 0),IR(8150, 14)/* LaunchSizeKHR */,IR(11, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5321,IR(0, 0),IR(8177, 18)/* WorldRayOriginKHR */,IR(12, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5322,IR(0, 0),IR(8212, 21)/* WorldRayDirectionKHR */,IR(13, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5323,IR(0, 0),IR(8253, 19)/* ObjectRayOriginKHR */,IR(14, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5324,IR(0, 0),IR(8290, 22)/* ObjectRayDirectionKHR */,IR(15, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5325,IR(0, 0),IR(8333, 11)/* RayTminKHR */,IR(16, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5326,IR(0, 0),IR(8354, 11)/* RayTmaxKHR */,IR(17, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5327,IR(0, 0),IR(8375, 23)/* InstanceCustomIndexKHR */,IR(18, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5330,IR(0, 0),IR(8420, 17)/* ObjectToWorldKHR */,IR(19, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5331,IR(0, 0),IR(8453, 17)/* WorldToObjectKHR */,IR(20, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5332,IR(0, 0),IR(8486, 7)/* HitTNV */,IR(0, 0),IR(57, 1),IR(26, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5333,IR(0, 0),IR(8493, 11)/* HitKindKHR */,IR(21, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5334,IR(0, 0),IR(8514, 17)/* CurrentRayTimeNV */,IR(0, 0),IR(58, 1),IR(27, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5335,IR(0, 0),IR(8554, 30)/* HitTriangleVertexPositionsKHR */,IR(0, 0),IR(59, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5337,IR(0, 0),IR(8611, 34)/* HitMicroTriangleVertexPositionsNV */,IR(0, 0),IR(60, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5344,IR(0, 0),IR(8678, 37)/* HitMicroTriangleVertexBarycentricsNV */,IR(0, 0),IR(60, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5351,IR(0, 0),IR(8715, 20)/* IncomingRayFlagsKHR */,IR(22, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5352,IR(0, 0),IR(8754, 20)/* RayGeometryIndexKHR */,IR(0, 0),IR(61, 1),IR(28, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5359,IR(0, 0),IR(8774, 14)/* HitIsSphereNV */,IR(0, 0),IR(62, 1),IR(29, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5360,IR(0, 0),IR(8816, 11)/* HitIsLSSNV */,IR(0, 0),IR(63, 1),IR(29, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5361,IR(0, 0),IR(8866, 20)/* HitSpherePositionNV */,IR(0, 0),IR(62, 1),IR(29, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5374,IR(0, 0),IR(8886, 13)/* WarpsPerSMNV */,IR(0, 0),IR(64, 1),IR(30, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5375,IR(0, 0),IR(8918, 10)/* SMCountNV */,IR(0, 0),IR(64, 1),IR(30, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5376,IR(0, 0),IR(8928, 9)/* WarpIDNV */,IR(0, 0),IR(64, 1),IR(30, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5377,IR(0, 0),IR(8937, 7)/* SMIDNV */,IR(0, 0),IR(64, 1),IR(30, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5396,IR(0, 0),IR(8944, 18)/* HitLSSPositionsNV */,IR(0, 0),IR(63, 1),IR(29, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5405,IR(0, 0),IR(8962, 34)/* HitKindFrontFacingMicroTriangleNV */,IR(0, 0),IR(60, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5406,IR(0, 0),IR(8996, 33)/* HitKindBackFacingMicroTriangleNV */,IR(0, 0),IR(60, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5420,IR(0, 0),IR(9029, 18)/* HitSphereRadiusNV */,IR(0, 0),IR(62, 1),IR(29, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5421,IR(0, 0),IR(9047, 14)/* HitLSSRadiiNV */,IR(0, 0),IR(63, 1),IR(29, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {5436,IR(0, 0),IR(9061, 12)/* ClusterIDNV */,IR(0, 0),IR(65, 1),IR(31, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {6021,IR(0, 0),IR(9114, 12)/* CullMaskKHR */,IR(0, 0),IR(66, 1),IR(32, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_BUILT_IN
  {0,IR(0, 0),IR(9141, 7)/* Matrix */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {1,IR(0, 0),IR(5963, 7)/* Shader */,IR(0, 0),IR(67, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {2,IR(0, 0),IR(6038, 9)/* Geometry */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {3,IR(0, 0),IR(6047, 13)/* Tessellation */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4,IR(0, 0),IR(5851, 10)/* Addresses */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5,IR(0, 0),IR(9148, 8)/* Linkage */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6,IR(0, 0),IR(5805, 7)/* Kernel */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {7,IR(0, 0),IR(9156, 9)/* Vector16 */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {8,IR(0, 0),IR(9165, 14)/* Float16Buffer */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {9,IR(0, 0),IR(9179, 8)/* Float16 */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {10,IR(0, 0),IR(9187, 8)/* Float64 */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {11,IR(0, 0),IR(9195, 6)/* Int64 */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {12,IR(0, 0),IR(9201, 13)/* Int64Atomics */,IR(0, 0),IR(68, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {13,IR(0, 0),IR(9214, 11)/* ImageBasic */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {14,IR(0, 0),IR(9225, 15)/* ImageReadWrite */,IR(0, 0),IR(69, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {15,IR(0, 0),IR(9240, 12)/* ImageMipmap */,IR(0, 0),IR(69, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {17,IR(0, 0),IR(9252, 6)/* Pipes */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {18,IR(0, 0),IR(9258, 7)/* Groups */,IR(0, 0),IR(1, 0),IR(33, 1),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {19,IR(0, 0),IR(9265, 14)/* DeviceEnqueue */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {20,IR(0, 0),IR(9279, 15)/* LiteralSampler */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {21,IR(0, 0),IR(9294, 14)/* AtomicStorage */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {22,IR(0, 0),IR(9308, 6)/* Int16 */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {23,IR(0, 0),IR(9314, 22)/* TessellationPointSize */,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {24,IR(0, 0),IR(9336, 18)/* GeometryPointSize */,IR(0, 0),IR(70, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {25,IR(0, 0),IR(9354, 20)/* ImageGatherExtended */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {27,IR(0, 0),IR(9374, 24)/* StorageImageMultisample */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {28,IR(0, 0),IR(9398, 34)/* UniformBufferArrayDynamicIndexing */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {29,IR(0, 0),IR(9432, 33)/* SampledImageArrayDynamicIndexing */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {30,IR(0, 0),IR(9465, 34)/* StorageBufferArrayDynamicIndexing */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {31,IR(0, 0),IR(9499, 33)/* StorageImageArrayDynamicIndexing */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {32,IR(0, 0),IR(5980, 13)/* ClipDistance */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {33,IR(0, 0),IR(5993, 13)/* CullDistance */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {34,IR(0, 0),IR(9532, 15)/* ImageCubeArray */,IR(0, 0),IR(71, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {35,IR(0, 0),IR(6319, 18)/* SampleRateShading */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {36,IR(0, 0),IR(9564, 10)/* ImageRect */,IR(0, 0),IR(72, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {37,IR(0, 0),IR(9574, 12)/* SampledRect */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {38,IR(0, 0),IR(9586, 15)/* GenericPointer */,IR(0, 0),IR(1, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {39,IR(0, 0),IR(9601, 5)/* Int8 */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {40,IR(0, 0),IR(9606, 16)/* InputAttachment */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {41,IR(0, 0),IR(9622, 16)/* SparseResidency */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {42,IR(0, 0),IR(9638, 7)/* MinLod */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {43,IR(0, 0),IR(9645, 10)/* Sampled1D */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {44,IR(0, 0),IR(9655, 8)/* Image1D */,IR(0, 0),IR(73, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {45,IR(0, 0),IR(9547, 17)/* SampledCubeArray */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {46,IR(0, 0),IR(9663, 14)/* SampledBuffer */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {47,IR(0, 0),IR(9677, 12)/* ImageBuffer */,IR(0, 0),IR(74, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {48,IR(0, 0),IR(9689, 13)/* ImageMSArray */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {49,IR(0, 0),IR(9702, 28)/* StorageImageExtendedFormats */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {50,IR(0, 0),IR(9730, 11)/* ImageQuery */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {51,IR(0, 0),IR(9741, 18)/* DerivativeControl */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {52,IR(0, 0),IR(9759, 22)/* InterpolationFunction */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {53,IR(0, 0),IR(9781, 18)/* TransformFeedback */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {54,IR(0, 0),IR(9799, 16)/* GeometryStreams */,IR(0, 0),IR(70, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {55,IR(0, 0),IR(9815, 30)/* StorageImageReadWithoutFormat */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {56,IR(0, 0),IR(9845, 31)/* StorageImageWriteWithoutFormat */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {57,IR(0, 0),IR(6189, 14)/* MultiViewport */,IR(0, 0),IR(70, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {58,IR(0, 0),IR(9876, 17)/* SubgroupDispatch */,IR(0, 0),IR(75, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,1),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {59,IR(0, 0),IR(9893, 13)/* NamedBarrier */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,1),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {60,IR(0, 0),IR(9906, 12)/* PipeStorage */,IR(0, 0),IR(76, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,1),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {61,IR(0, 0),IR(6570, 16)/* GroupNonUniform */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {62,IR(0, 0),IR(9918, 20)/* GroupNonUniformVote */,IR(0, 0),IR(77, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {63,IR(0, 0),IR(9938, 26)/* GroupNonUniformArithmetic */,IR(0, 0),IR(77, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {64,IR(0, 0),IR(6825, 22)/* GroupNonUniformBallot */,IR(0, 0),IR(77, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {65,IR(0, 0),IR(9964, 23)/* GroupNonUniformShuffle */,IR(0, 0),IR(77, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {66,IR(0, 0),IR(9987, 31)/* GroupNonUniformShuffleRelative */,IR(0, 0),IR(77, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {67,IR(0, 0),IR(10018, 25)/* GroupNonUniformClustered */,IR(0, 0),IR(77, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {68,IR(0, 0),IR(10043, 20)/* GroupNonUniformQuad */,IR(0, 0),IR(77, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {69,IR(0, 0),IR(6135, 12)/* ShaderLayer */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {70,IR(0, 0),IR(6203, 20)/* ShaderViewportIndex */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {71,IR(0, 0),IR(10063, 18)/* UniformDecoration */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,6),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4165,IR(0, 0),IR(6727, 16)/* CoreBuiltinsARM */,IR(0, 0),IR(1, 0),IR(34, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4166,IR(0, 0),IR(10081, 28)/* TileImageColorReadAccessEXT */,IR(0, 0),IR(1, 0),IR(35, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4167,IR(0, 0),IR(10109, 28)/* TileImageDepthReadAccessEXT */,IR(0, 0),IR(1, 0),IR(35, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4168,IR(0, 0),IR(10137, 30)/* TileImageStencilReadAccessEXT */,IR(0, 0),IR(1, 0),IR(35, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4174,IR(0, 0),IR(10167, 11)/* TensorsARM */,IR(0, 0),IR(1, 0),IR(36, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4175,IR(0, 0),IR(10178, 37)/* StorageTensorArrayDynamicIndexingARM */,IR(0, 0),IR(1, 0),IR(36, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4176,IR(0, 0),IR(10215, 40)/* StorageTensorArrayNonUniformIndexingARM */,IR(0, 0),IR(1, 0),IR(36, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4191,IR(0, 0),IR(10255, 9)/* GraphARM */,IR(0, 0),IR(1, 0),IR(37, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4201,IR(0, 0),IR(10264, 28)/* CooperativeMatrixLayoutsARM */,IR(0, 0),IR(1, 0),IR(38, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4212,IR(0, 0),IR(10292, 10)/* Float8EXT */,IR(0, 0),IR(1, 0),IR(39, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4213,IR(0, 0),IR(10302, 27)/* Float8CooperativeMatrixEXT */,IR(0, 0),IR(78, 2),IR(39, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4228,IR(0, 0),IR(10350, 10)/* Float6EXT */,IR(0, 0),IR(1, 0),IR(40, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4229,IR(0, 0),IR(10360, 10)/* Float4EXT */,IR(0, 0),IR(1, 0),IR(40, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4230,IR(0, 0),IR(10370, 22)/* Float8UnsignedE8M0EXT */,IR(0, 0),IR(1, 0),IR(40, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4231,IR(0, 0),IR(10392, 10)/* MXInt8EXT */,IR(0, 0),IR(1, 0),IR(40, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4232,IR(0, 0),IR(10402, 18)/* BitcastExtractEXT */,IR(0, 0),IR(1, 0),IR(40, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4422,IR(0, 0),IR(7052, 23)/* FragmentShadingRateKHR */,IR(0, 0),IR(3, 1),IR(7, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4423,IR(0, 0),IR(6586, 18)/* SubgroupBallotKHR */,IR(0, 0),IR(1, 0),IR(2, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4427,IR(0, 0),IR(6990, 15)/* DrawParameters */,IR(0, 0),IR(3, 1),IR(3, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4428,IR(0, 0),IR(10420, 33)/* WorkgroupMemoryExplicitLayoutKHR */,IR(0, 0),IR(3, 1),IR(41, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4429,IR(0, 0),IR(10453, 43)/* WorkgroupMemoryExplicitLayout8BitAccessKHR */,IR(0, 0),IR(80, 1),IR(41, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4430,IR(0, 0),IR(10496, 44)/* WorkgroupMemoryExplicitLayout16BitAccessKHR */,IR(0, 0),IR(80, 1),IR(41, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4431,IR(0, 0),IR(10540, 16)/* SubgroupVoteKHR */,IR(0, 0),IR(1, 0),IR(42, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4433,IR(0, 0),IR(10556, 25)/* StorageBuffer16BitAccess */,IR(23, 1),IR(1, 0),IR(43, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4434,IR(0, 0),IR(10609, 35)/* UniformAndStorageBuffer16BitAccess */,IR(24, 1),IR(81, 1),IR(43, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4435,IR(0, 0),IR(10661, 22)/* StoragePushConstant16 */,IR(0, 0),IR(1, 0),IR(43, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4436,IR(0, 0),IR(10683, 21)/* StorageInputOutput16 */,IR(0, 0),IR(1, 0),IR(43, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4437,IR(0, 0),IR(7087, 12)/* DeviceGroup */,IR(0, 0),IR(1, 0),IR(8, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4439,IR(0, 0),IR(7109, 10)/* MultiView */,IR(0, 0),IR(3, 1),IR(9, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4441,IR(0, 0),IR(10704, 30)/* VariablePointersStorageBuffer */,IR(0, 0),IR(3, 1),IR(44, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4442,IR(0, 0),IR(10734, 17)/* VariablePointers */,IR(0, 0),IR(82, 1),IR(44, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4445,IR(0, 0),IR(10751, 17)/* AtomicStorageOps */,IR(0, 0),IR(83, 1),IR(45, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4447,IR(0, 0),IR(10768, 28)/* SampleMaskPostDepthCoverage */,IR(0, 0),IR(1, 0),IR(46, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4448,IR(0, 0),IR(10796, 24)/* StorageBuffer8BitAccess */,IR(0, 0),IR(1, 0),IR(47, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4449,IR(0, 0),IR(10820, 34)/* UniformAndStorageBuffer8BitAccess */,IR(0, 0),IR(84, 1),IR(47, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4450,IR(0, 0),IR(10854, 21)/* StoragePushConstant8 */,IR(0, 0),IR(1, 0),IR(47, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4464,IR(0, 0),IR(10875, 15)/* DenormPreserve */,IR(0, 0),IR(1, 0),IR(48, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4465,IR(0, 0),IR(10890, 18)/* DenormFlushToZero */,IR(0, 0),IR(1, 0),IR(48, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4466,IR(0, 0),IR(10908, 25)/* SignedZeroInfNanPreserve */,IR(0, 0),IR(1, 0),IR(48, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4467,IR(0, 0),IR(10933, 16)/* RoundingModeRTE */,IR(0, 0),IR(1, 0),IR(48, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4468,IR(0, 0),IR(10949, 16)/* RoundingModeRTZ */,IR(0, 0),IR(1, 0),IR(48, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4471,IR(0, 0),IR(10965, 23)/* RayQueryProvisionalKHR */,IR(0, 0),IR(3, 1),IR(49, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4472,IR(0, 0),IR(10988, 12)/* RayQueryKHR */,IR(0, 0),IR(3, 1),IR(49, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4473,IR(0, 0),IR(11000, 19)/* UntypedPointersKHR */,IR(0, 0),IR(1, 0),IR(50, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4478,IR(0, 0),IR(11019, 32)/* RayTraversalPrimitiveCullingKHR */,IR(0, 0),IR(85, 2),IR(51, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4479,IR(0, 0),IR(6073, 14)/* RayTracingKHR */,IR(0, 0),IR(3, 1),IR(28, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4484,IR(0, 0),IR(11051, 26)/* TextureSampleWeightedQCOM */,IR(0, 0),IR(1, 0),IR(53, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4485,IR(0, 0),IR(11077, 21)/* TextureBoxFilterQCOM */,IR(0, 0),IR(1, 0),IR(53, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4486,IR(0, 0),IR(11098, 22)/* TextureBlockMatchQCOM */,IR(0, 0),IR(1, 0),IR(53, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4495,IR(0, 0),IR(7149, 16)/* TileShadingQCOM */,IR(0, 0),IR(3, 1),IR(54, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4496,IR(0, 0),IR(11120, 32)/* CooperativeMatrixConversionQCOM */,IR(0, 0),IR(87, 1),IR(55, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4498,IR(0, 0),IR(11152, 23)/* TextureBlockMatch2QCOM */,IR(0, 0),IR(1, 0),IR(56, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4539,IR(0, 0),IR(11175, 23)/* MultipleWaitQueuesQCOM */,IR(0, 0),IR(1, 0),IR(57, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4543,IR(0, 0),IR(11198, 22)/* ImageGatherLinearQCOM */,IR(0, 0),IR(1, 0),IR(58, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {4544,IR(0, 0),IR(11220, 29)/* ImageGatherExtendedModesQCOM */,IR(0, 0),IR(1, 0),IR(58, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5008,IR(0, 0),IR(11249, 16)/* Float16ImageAMD */,IR(0, 0),IR(3, 1),IR(59, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5009,IR(0, 0),IR(11265, 22)/* ImageGatherBiasLodAMD */,IR(0, 0),IR(3, 1),IR(60, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5010,IR(0, 0),IR(11287, 16)/* FragmentMaskAMD */,IR(0, 0),IR(3, 1),IR(61, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5013,IR(0, 0),IR(7386, 17)/* StencilExportEXT */,IR(0, 0),IR(3, 1),IR(11, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5015,IR(0, 0),IR(11303, 21)/* ImageReadWriteLodAMD */,IR(0, 0),IR(3, 1),IR(62, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5016,IR(0, 0),IR(11324, 14)/* Int64ImageEXT */,IR(0, 0),IR(3, 1),IR(63, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5055,IR(0, 0),IR(11338, 15)/* ShaderClockKHR */,IR(0, 0),IR(1, 0),IR(64, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5067,IR(0, 0),IR(7432, 18)/* ShaderEnqueueAMDX */,IR(0, 0),IR(3, 1),IR(65, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5087,IR(0, 0),IR(11353, 15)/* QuadControlKHR */,IR(0, 0),IR(1, 0),IR(66, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5112,IR(0, 0),IR(11368, 14)/* Int4TypeINTEL */,IR(0, 0),IR(1, 0),IR(67, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5114,IR(0, 0),IR(11382, 27)/* Int4CooperativeMatrixINTEL */,IR(0, 0),IR(88, 2),IR(67, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5116,IR(0, 0),IR(11409, 16)/* BFloat16TypeKHR */,IR(0, 0),IR(1, 0),IR(68, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5117,IR(0, 0),IR(11425, 22)/* BFloat16DotProductKHR */,IR(0, 0),IR(90, 1),IR(68, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5118,IR(0, 0),IR(11447, 29)/* BFloat16CooperativeMatrixKHR */,IR(0, 0),IR(91, 2),IR(68, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5120,IR(0, 0),IR(11476, 9)/* AbortKHR */,IR(0, 0),IR(1, 0),IR(69, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5128,IR(0, 0),IR(7481, 18)/* DescriptorHeapEXT */,IR(0, 0),IR(93, 1),IR(70, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5146,IR(0, 0),IR(11485, 16)/* ConstantDataKHR */,IR(0, 0),IR(1, 0),IR(71, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5156,IR(0, 0),IR(11501, 16)/* PoisonFreezeKHR */,IR(0, 0),IR(1, 0),IR(72, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5181,IR(0, 0),IR(11517, 15)/* WeakLinkageAMD */,IR(0, 0),IR(94, 1),IR(73, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5249,IR(0, 0),IR(11532, 29)/* SampleMaskOverrideCoverageNV */,IR(0, 0),IR(25, 1),IR(74, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5251,IR(0, 0),IR(11561, 28)/* GeometryShaderPassthroughNV */,IR(0, 0),IR(70, 1),IR(75, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5254,IR(0, 0),IR(6147, 28)/* ShaderViewportIndexLayerEXT */,IR(25, 1),IR(95, 1),IR(76, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5255,IR(0, 0),IR(7530, 21)/* ShaderViewportMaskNV */,IR(0, 0),IR(96, 1),IR(78, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5259,IR(0, 0),IR(7571, 19)/* ShaderStereoViewNV */,IR(0, 0),IR(97, 1),IR(14, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5260,IR(0, 0),IR(7632, 20)/* PerViewAttributesNV */,IR(0, 0),IR(40, 1),IR(79, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5265,IR(0, 0),IR(7690, 24)/* FragmentFullyCoveredEXT */,IR(0, 0),IR(3, 1),IR(17, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5266,IR(0, 0),IR(6087, 14)/* MeshShadingNV */,IR(0, 0),IR(3, 1),IR(18, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5282,IR(0, 0),IR(11616, 17)/* ImageFootprintNV */,IR(0, 0),IR(1, 0),IR(80, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5283,IR(0, 0),IR(6101, 15)/* MeshShadingEXT */,IR(0, 0),IR(3, 1),IR(23, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5284,IR(0, 0),IR(7880, 23)/* FragmentBarycentricKHR */,IR(26, 1),IR(1, 0),IR(19, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5288,IR(0, 0),IR(11655, 31)/* ComputeDerivativeGroupQuadsKHR */,IR(27, 1),IR(3, 1),IR(81, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5291,IR(0, 0),IR(7969, 19)/* FragmentDensityEXT */,IR(28, 1),IR(3, 1),IR(21, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5297,IR(0, 0),IR(11730, 30)/* GroupNonUniformPartitionedEXT */,IR(29, 1),IR(1, 0),IR(83, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5301,IR(0, 0),IR(11789, 17)/* ShaderNonUniform */,IR(30, 1),IR(3, 1),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5302,IR(0, 0),IR(11826, 23)/* RuntimeDescriptorArray */,IR(31, 1),IR(3, 1),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5303,IR(0, 0),IR(11875, 36)/* InputAttachmentArrayDynamicIndexing */,IR(32, 1),IR(98, 1),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5304,IR(0, 0),IR(11950, 39)/* UniformTexelBufferArrayDynamicIndexing */,IR(33, 1),IR(74, 1),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5305,IR(0, 0),IR(12031, 39)/* StorageTexelBufferArrayDynamicIndexing */,IR(34, 1),IR(99, 1),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5306,IR(0, 0),IR(12112, 37)/* UniformBufferArrayNonUniformIndexing */,IR(35, 1),IR(100, 1),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5307,IR(0, 0),IR(12189, 36)/* SampledImageArrayNonUniformIndexing */,IR(36, 1),IR(100, 1),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5308,IR(0, 0),IR(12264, 37)/* StorageBufferArrayNonUniformIndexing */,IR(37, 1),IR(100, 1),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5309,IR(0, 0),IR(12341, 36)/* StorageImageArrayNonUniformIndexing */,IR(38, 1),IR(100, 1),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5310,IR(0, 0),IR(12416, 39)/* InputAttachmentArrayNonUniformIndexing */,IR(39, 1),IR(101, 2),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5311,IR(0, 0),IR(12497, 42)/* UniformTexelBufferArrayNonUniformIndexing */,IR(40, 1),IR(103, 2),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5312,IR(0, 0),IR(12584, 42)/* StorageTexelBufferArrayNonUniformIndexing */,IR(41, 1),IR(105, 2),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5336,IR(0, 0),IR(8584, 27)/* RayTracingPositionFetchKHR */,IR(0, 0),IR(3, 1),IR(86, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5340,IR(0, 0),IR(6060, 13)/* RayTracingNV */,IR(0, 0),IR(3, 1),IR(26, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5341,IR(0, 0),IR(8531, 23)/* RayTracingMotionBlurNV */,IR(0, 0),IR(3, 1),IR(27, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5345,IR(0, 0),IR(12671, 18)/* VulkanMemoryModel */,IR(42, 1),IR(1, 0),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5346,IR(0, 0),IR(12710, 29)/* VulkanMemoryModelDeviceScope */,IR(43, 1),IR(1, 0),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5347,IR(0, 0),IR(5923, 31)/* PhysicalStorageBufferAddresses */,IR(44, 1),IR(3, 1),IR(0, 2),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5350,IR(0, 0),IR(12805, 32)/* ComputeDerivativeGroupLinearKHR */,IR(45, 1),IR(3, 1),IR(81, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5353,IR(0, 0),IR(12868, 25)/* RayTracingProvisionalKHR */,IR(0, 0),IR(3, 1),IR(28, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5357,IR(0, 0),IR(12893, 20)/* CooperativeMatrixNV */,IR(0, 0),IR(3, 1),IR(88, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5363,IR(0, 0),IR(12913, 33)/* FragmentShaderSampleInterlockEXT */,IR(0, 0),IR(3, 1),IR(89, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5372,IR(0, 0),IR(12946, 38)/* FragmentShaderShadingRateInterlockEXT */,IR(0, 0),IR(3, 1),IR(89, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5373,IR(0, 0),IR(8899, 19)/* ShaderSMBuiltinsNV */,IR(0, 0),IR(3, 1),IR(30, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5378,IR(0, 0),IR(12984, 32)/* FragmentShaderPixelInterlockEXT */,IR(0, 0),IR(3, 1),IR(89, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5379,IR(0, 0),IR(13016, 25)/* DemoteToHelperInvocation */,IR(46, 1),IR(3, 1),IR(90, 1),SPV_SPIRV_VERSION_WORD(1,6),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5380,IR(0, 0),IR(13069, 23)/* DisplacementMicromapNV */,IR(0, 0),IR(3, 1),IR(91, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5381,IR(0, 0),IR(13092, 29)/* RayTracingOpacityMicromapKHR */,IR(47, 1),IR(3, 1),IR(92, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5383,IR(0, 0),IR(13150, 26)/* ShaderInvocationReorderNV */,IR(0, 0),IR(61, 1),IR(94, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5388,IR(0, 0),IR(13176, 27)/* ShaderInvocationReorderEXT */,IR(0, 0),IR(61, 1),IR(95, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5390,IR(0, 0),IR(13203, 18)/* BindlessTextureNV */,IR(0, 0),IR(1, 0),IR(96, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5391,IR(0, 0),IR(13221, 25)/* RayQueryPositionFetchKHR */,IR(0, 0),IR(3, 1),IR(86, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5394,IR(0, 0),IR(13246, 20)/* CooperativeVectorNV */,IR(0, 0),IR(1, 0),IR(97, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5404,IR(0, 0),IR(13266, 22)/* AtomicFloat16VectorNV */,IR(0, 0),IR(1, 0),IR(98, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5409,IR(0, 0),IR(8645, 33)/* RayTracingDisplacementMicromapNV */,IR(0, 0),IR(61, 1),IR(91, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5414,IR(0, 0),IR(13288, 18)/* RawAccessChainsNV */,IR(0, 0),IR(1, 0),IR(99, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5418,IR(0, 0),IR(8788, 28)/* RayTracingSpheresGeometryNV */,IR(0, 0),IR(1, 0),IR(29, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5419,IR(0, 0),IR(8827, 39)/* RayTracingLinearSweptSpheresGeometryNV */,IR(0, 0),IR(1, 0),IR(29, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5423,IR(0, 0),IR(13306, 20)/* PushConstantBanksNV */,IR(0, 0),IR(3, 1),IR(100, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5425,IR(0, 0),IR(13326, 14)/* LongVectorEXT */,IR(0, 0),IR(1, 0),IR(101, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5426,IR(0, 0),IR(13340, 23)/* Shader64BitIndexingEXT */,IR(0, 0),IR(1, 0),IR(102, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5430,IR(0, 0),IR(13363, 30)/* CooperativeMatrixReductionsNV */,IR(0, 0),IR(1, 0),IR(103, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5431,IR(0, 0),IR(13393, 31)/* CooperativeMatrixConversionsNV */,IR(0, 0),IR(1, 0),IR(103, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5432,IR(0, 0),IR(13424, 40)/* CooperativeMatrixPerElementOperationsNV */,IR(0, 0),IR(1, 0),IR(103, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5433,IR(0, 0),IR(13464, 36)/* CooperativeMatrixTensorAddressingNV */,IR(0, 0),IR(1, 0),IR(103, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5434,IR(0, 0),IR(13500, 30)/* CooperativeMatrixBlockLoadsNV */,IR(0, 0),IR(1, 0),IR(103, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5435,IR(0, 0),IR(13530, 28)/* CooperativeVectorTrainingNV */,IR(0, 0),IR(1, 0),IR(97, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5437,IR(0, 0),IR(9073, 41)/* RayTracingClusterAccelerationStructureNV */,IR(0, 0),IR(61, 1),IR(31, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5439,IR(0, 0),IR(13558, 19)/* TensorAddressingNV */,IR(0, 0),IR(1, 0),IR(104, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5447,IR(0, 0),IR(13577, 32)/* CooperativeMatrixDecodeVectorNV */,IR(0, 0),IR(107, 1),IR(105, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5568,IR(0, 0),IR(13609, 21)/* SubgroupShuffleINTEL */,IR(0, 0),IR(1, 0),IR(106, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5569,IR(0, 0),IR(13630, 27)/* SubgroupBufferBlockIOINTEL */,IR(0, 0),IR(1, 0),IR(106, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5570,IR(0, 0),IR(13657, 26)/* SubgroupImageBlockIOINTEL */,IR(0, 0),IR(1, 0),IR(106, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5579,IR(0, 0),IR(13683, 31)/* SubgroupImageMediaBlockIOINTEL */,IR(0, 0),IR(1, 0),IR(107, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5582,IR(0, 0),IR(13714, 21)/* RoundToInfinityINTEL */,IR(0, 0),IR(1, 0),IR(108, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5583,IR(0, 0),IR(13735, 23)/* FloatingPointModeINTEL */,IR(0, 0),IR(1, 0),IR(108, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5584,IR(0, 0),IR(13758, 23)/* IntegerFunctions2INTEL */,IR(0, 0),IR(1, 0),IR(109, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5603,IR(0, 0),IR(13781, 22)/* FunctionPointersINTEL */,IR(0, 0),IR(1, 0),IR(110, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5604,IR(0, 0),IR(13803, 24)/* IndirectReferencesINTEL */,IR(0, 0),IR(1, 0),IR(110, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5606,IR(0, 0),IR(13827, 9)/* AsmINTEL */,IR(0, 0),IR(1, 0),IR(111, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5612,IR(0, 0),IR(13836, 23)/* AtomicFloat32MinMaxEXT */,IR(0, 0),IR(1, 0),IR(112, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5613,IR(0, 0),IR(13859, 23)/* AtomicFloat64MinMaxEXT */,IR(0, 0),IR(1, 0),IR(112, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5616,IR(0, 0),IR(13882, 23)/* AtomicFloat16MinMaxEXT */,IR(0, 0),IR(1, 0),IR(112, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5617,IR(0, 0),IR(13905, 19)/* VectorComputeINTEL */,IR(0, 0),IR(108, 1),IR(113, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5619,IR(0, 0),IR(13924, 15)/* VectorAnyINTEL */,IR(0, 0),IR(1, 0),IR(113, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5629,IR(0, 0),IR(13939, 16)/* ExpectAssumeKHR */,IR(0, 0),IR(1, 0),IR(114, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5696,IR(0, 0),IR(13955, 33)/* SubgroupAvcMotionEstimationINTEL */,IR(0, 0),IR(1, 0),IR(115, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5697,IR(0, 0),IR(13988, 38)/* SubgroupAvcMotionEstimationIntraINTEL */,IR(0, 0),IR(1, 0),IR(115, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5698,IR(0, 0),IR(14026, 39)/* SubgroupAvcMotionEstimationChromaINTEL */,IR(0, 0),IR(1, 0),IR(115, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5817,IR(0, 0),IR(14065, 25)/* VariableLengthArrayINTEL */,IR(0, 0),IR(1, 0),IR(116, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5821,IR(0, 0),IR(14090, 26)/* FunctionFloatControlINTEL */,IR(0, 0),IR(1, 0),IR(108, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5824,IR(0, 0),IR(14116, 27)/* FPGAMemoryAttributesALTERA */,IR(48, 1),IR(1, 0),IR(117, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5837,IR(0, 0),IR(14169, 20)/* FPFastMathModeINTEL */,IR(0, 0),IR(0, 1),IR(119, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5844,IR(0, 0),IR(14189, 33)/* ArbitraryPrecisionIntegersALTERA */,IR(49, 1),IR(1, 0),IR(120, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5845,IR(0, 0),IR(14254, 38)/* ArbitraryPrecisionFloatingPointALTERA */,IR(50, 1),IR(1, 0),IR(122, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5886,IR(0, 0),IR(14329, 30)/* UnstructuredLoopControlsINTEL */,IR(0, 0),IR(1, 0),IR(124, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5888,IR(0, 0),IR(14359, 23)/* FPGALoopControlsALTERA */,IR(51, 1),IR(1, 0),IR(125, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5892,IR(0, 0),IR(14404, 22)/* KernelAttributesINTEL */,IR(0, 0),IR(1, 0),IR(127, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5897,IR(0, 0),IR(14426, 26)/* FPGAKernelAttributesINTEL */,IR(0, 0),IR(1, 0),IR(127, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5898,IR(0, 0),IR(14452, 25)/* FPGAMemoryAccessesALTERA */,IR(52, 1),IR(1, 0),IR(128, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5904,IR(0, 0),IR(14501, 28)/* FPGAClusterAttributesALTERA */,IR(53, 1),IR(1, 0),IR(130, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5906,IR(0, 0),IR(14556, 15)/* LoopFuseALTERA */,IR(54, 1),IR(1, 0),IR(132, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5908,IR(0, 0),IR(14585, 21)/* FPGADSPControlALTERA */,IR(55, 1),IR(1, 0),IR(134, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5910,IR(0, 0),IR(14626, 26)/* MemoryAccessAliasingINTEL */,IR(0, 0),IR(1, 0),IR(136, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5916,IR(0, 0),IR(14652, 41)/* FPGAInvocationPipeliningAttributesALTERA */,IR(56, 1),IR(1, 0),IR(137, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5920,IR(0, 0),IR(14733, 25)/* FPGABufferLocationALTERA */,IR(57, 1),IR(1, 0),IR(139, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5922,IR(0, 0),IR(14782, 35)/* ArbitraryPrecisionFixedPointALTERA */,IR(58, 1),IR(1, 0),IR(141, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5935,IR(0, 0),IR(14851, 24)/* USMStorageClassesALTERA */,IR(59, 1),IR(1, 0),IR(143, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5939,IR(0, 0),IR(14898, 30)/* RuntimeAlignedAttributeALTERA */,IR(60, 1),IR(1, 0),IR(145, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5943,IR(0, 0),IR(14957, 14)/* IOPipesALTERA */,IR(61, 1),IR(1, 0),IR(147, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5945,IR(0, 0),IR(14984, 20)/* BlockingPipesALTERA */,IR(62, 1),IR(1, 0),IR(149, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {5948,IR(0, 0),IR(15023, 14)/* FPGARegALTERA */,IR(63, 1),IR(1, 0),IR(151, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6016,IR(0, 0),IR(15050, 19)/* DotProductInputAll */,IR(64, 1),IR(1, 0),IR(153, 1),SPV_SPIRV_VERSION_WORD(1,6),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6017,IR(0, 0),IR(15091, 22)/* DotProductInput4x8Bit */,IR(65, 1),IR(109, 1),IR(153, 1),SPV_SPIRV_VERSION_WORD(1,6),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6018,IR(0, 0),IR(15138, 28)/* DotProductInput4x8BitPacked */,IR(66, 1),IR(1, 0),IR(153, 1),SPV_SPIRV_VERSION_WORD(1,6),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6019,IR(0, 0),IR(15197, 11)/* DotProduct */,IR(67, 1),IR(1, 0),IR(153, 1),SPV_SPIRV_VERSION_WORD(1,6),0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6020,IR(0, 0),IR(9126, 15)/* RayCullMaskKHR */,IR(0, 0),IR(1, 0),IR(32, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6022,IR(0, 0),IR(10329, 21)/* CooperativeMatrixKHR */,IR(0, 0),IR(1, 0),IR(154, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6024,IR(0, 0),IR(15222, 24)/* ReplicatedCompositesEXT */,IR(0, 0),IR(1, 0),IR(155, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6025,IR(0, 0),IR(15246, 16)/* BitInstructions */,IR(0, 0),IR(1, 0),IR(156, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6026,IR(0, 0),IR(15262, 25)/* GroupNonUniformRotateKHR */,IR(0, 0),IR(77, 1),IR(157, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6029,IR(0, 0),IR(15287, 15)/* FloatControls2 */,IR(0, 0),IR(1, 0),IR(158, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6030,IR(0, 0),IR(15302, 7)/* FMAKHR */,IR(0, 0),IR(1, 0),IR(159, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6032,IR(0, 0),IR(15309, 42)/* RayTracingOpacityMicromapExecutionModeKHR */,IR(0, 0),IR(3, 1),IR(160, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6033,IR(0, 0),IR(15351, 20)/* AtomicFloat32AddEXT */,IR(0, 0),IR(1, 0),IR(161, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6034,IR(0, 0),IR(15371, 20)/* AtomicFloat64AddEXT */,IR(0, 0),IR(1, 0),IR(161, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6089,IR(0, 0),IR(15391, 20)/* LongCompositesINTEL */,IR(0, 0),IR(1, 0),IR(162, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6094,IR(0, 0),IR(15411, 11)/* OptNoneEXT */,IR(68, 1),IR(1, 0),IR(163, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6095,IR(0, 0),IR(15435, 20)/* AtomicFloat16AddEXT */,IR(0, 0),IR(1, 0),IR(165, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6114,IR(0, 0),IR(15455, 21)/* DebugInfoModuleINTEL */,IR(0, 0),IR(1, 0),IR(166, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6115,IR(0, 0),IR(15476, 24)/* BFloat16ConversionINTEL */,IR(0, 0),IR(1, 0),IR(167, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6141,IR(0, 0),IR(15500, 16)/* SplitBarrierEXT */,IR(69, 1),IR(1, 0),IR(168, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6144,IR(0, 0),IR(15534, 19)/* ArithmeticFenceEXT */,IR(0, 0),IR(1, 0),IR(170, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6150,IR(0, 0),IR(15553, 30)/* FPGAClusterAttributesV2ALTERA */,IR(70, 1),IR(110, 1),IR(130, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6161,IR(0, 0),IR(15612, 28)/* FPGAKernelAttributesv2INTEL */,IR(0, 0),IR(111, 1),IR(127, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6162,IR(0, 0),IR(15640, 19)/* TaskSequenceALTERA */,IR(71, 1),IR(1, 0),IR(171, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6169,IR(0, 0),IR(15677, 16)/* FPMaxErrorINTEL */,IR(0, 0),IR(1, 0),IR(173, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6171,IR(0, 0),IR(15693, 25)/* FPGALatencyControlALTERA */,IR(72, 1),IR(1, 0),IR(174, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6174,IR(0, 0),IR(15742, 29)/* FPGAArgumentInterfacesALTERA */,IR(73, 1),IR(1, 0),IR(176, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6187,IR(0, 0),IR(15799, 30)/* GlobalVariableHostAccessINTEL */,IR(0, 0),IR(1, 0),IR(178, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6189,IR(0, 0),IR(15829, 36)/* GlobalVariableFPGADecorationsALTERA */,IR(74, 1),IR(1, 0),IR(179, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6220,IR(0, 0),IR(15900, 28)/* SubgroupBufferPrefetchINTEL */,IR(0, 0),IR(1, 0),IR(181, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6228,IR(0, 0),IR(15928, 23)/* Subgroup2DBlockIOINTEL */,IR(0, 0),IR(1, 0),IR(182, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6229,IR(0, 0),IR(15951, 30)/* Subgroup2DBlockTransformINTEL */,IR(0, 0),IR(112, 1),IR(182, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6230,IR(0, 0),IR(15981, 30)/* Subgroup2DBlockTransposeINTEL */,IR(0, 0),IR(112, 1),IR(182, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6236,IR(0, 0),IR(16011, 38)/* SubgroupMatrixMultiplyAccumulateINTEL */,IR(0, 0),IR(1, 0),IR(183, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6241,IR(0, 0),IR(16049, 28)/* TernaryBitwiseFunctionINTEL */,IR(0, 0),IR(1, 0),IR(184, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6243,IR(0, 0),IR(16077, 32)/* UntypedVariableLengthArrayINTEL */,IR(0, 0),IR(113, 2),IR(116, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6245,IR(0, 0),IR(16109, 21)/* SpecConditionalINTEL */,IR(0, 0),IR(1, 0),IR(185, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6246,IR(0, 0),IR(16130, 22)/* FunctionVariantsINTEL */,IR(0, 0),IR(115, 1),IR(185, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6257,IR(0, 0),IR(16152, 18)/* PredicatedIOINTEL */,IR(0, 0),IR(1, 0),IR(186, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6265,IR(0, 0),IR(16170, 23)/* RoundedDivideSqrtINTEL */,IR(0, 0),IR(1, 0),IR(187, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6400,IR(0, 0),IR(16193, 26)/* GroupUniformArithmeticKHR */,IR(0, 0),IR(1, 0),IR(188, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6425,IR(0, 0),IR(16219, 27)/* TensorFloat32RoundingINTEL */,IR(0, 0),IR(1, 0),IR(189, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6427,IR(0, 0),IR(16246, 25)/* MaskedGatherScatterINTEL */,IR(0, 0),IR(1, 0),IR(190, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6441,IR(0, 0),IR(16271, 19)/* CacheControlsINTEL */,IR(0, 0),IR(1, 0),IR(191, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6460,IR(0, 0),IR(16290, 20)/* RegisterLimitsINTEL */,IR(0, 0),IR(1, 0),IR(192, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6528,IR(0, 0),IR(16310, 20)/* BindlessImagesINTEL */,IR(0, 0),IR(1, 0),IR(193, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6912,IR(0, 0),IR(16330, 33)/* DotProductFloat16AccFloat32VALVE */,IR(0, 0),IR(116, 1),IR(194, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6913,IR(0, 0),IR(16363, 33)/* DotProductFloat16AccFloat16VALVE */,IR(0, 0),IR(116, 1),IR(194, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6914,IR(0, 0),IR(16396, 27)/* DotProductBFloat16AccVALVE */,IR(0, 0),IR(90, 1),IR(194, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {6915,IR(0, 0),IR(16423, 32)/* DotProductFloat8AccFloat32VALVE */,IR(0, 0),IR(117, 1),IR(194, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_CAPABILITY
  {0,IR(0, 0),IR(16455, 12)/* Unspecified */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {1,IR(0, 0),IR(16467, 8)/* Address */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {2,IR(0, 0),IR(16475, 8)/* Boolean */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {3,IR(0, 0),IR(16483, 6)/* Float */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {4,IR(0, 0),IR(16489, 7)/* Signed */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {5,IR(0, 0),IR(16496, 11)/* SignedChar */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {6,IR(0, 0),IR(16507, 9)/* Unsigned */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {7,IR(0, 0),IR(16516, 13)/* UnsignedChar */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {0,IR(0, 0),IR(16529, 6)/* Class */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_COMPOSITE_TYPE
  {1,IR(0, 0),IR(16535, 10)/* Structure */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_COMPOSITE_TYPE
  {2,IR(0, 0),IR(16545, 6)/* Union */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_COMPOSITE_TYPE
  {0,IR(0, 0),IR(16551, 15)/* ImportedModule */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_IMPORTED_ENTITY
  {1,IR(0, 0),IR(16566, 20)/* ImportedDeclaration */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_IMPORTED_ENTITY
  {0,IR(0, 0),IR(16586, 5)/* None */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {1,IR(0, 0),IR(16591, 16)/* FlagIsProtected */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {2,IR(0, 0),IR(16607, 14)/* FlagIsPrivate */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {3,IR(0, 0),IR(16621, 13)/* FlagIsPublic */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {4,IR(0, 0),IR(16634, 12)/* FlagIsLocal */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {8,IR(0, 0),IR(16646, 17)/* FlagIsDefinition */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {16,IR(0, 0),IR(16663, 12)/* FlagFwdDecl */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {32,IR(0, 0),IR(16675, 15)/* FlagArtificial */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {64,IR(0, 0),IR(16690, 13)/* FlagExplicit */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {128,IR(0, 0),IR(16703, 15)/* FlagPrototyped */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {256,IR(0, 0),IR(16718, 18)/* FlagObjectPointer */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {512,IR(0, 0),IR(16736, 17)/* FlagStaticMember */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {1024,IR(0, 0),IR(16753, 21)/* FlagIndirectVariable */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {2048,IR(0, 0),IR(16774, 20)/* FlagLValueReference */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {4096,IR(0, 0),IR(16794, 20)/* FlagRValueReference */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {8192,IR(0, 0),IR(16814, 16)/* FlagIsOptimized */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {16384,IR(0, 0),IR(16830, 16)/* FlagIsEnumClass */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {32768,IR(0, 0),IR(16846, 20)/* FlagTypePassByValue */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {65536,IR(0, 0),IR(16866, 24)/* FlagTypePassByReference */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {0,IR(0, 0),IR(16890, 6)/* Deref */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION
  {1,IR(0, 0),IR(16896, 5)/* Plus */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION
  {2,IR(0, 0),IR(16901, 6)/* Minus */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION
  {3,IR(0, 1),IR(16940, 11)/* PlusUconst */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION
  {4,IR(1, 2),IR(16951, 9)/* BitPiece */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION
  {5,IR(0, 0),IR(16960, 5)/* Swap */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION
  {6,IR(0, 0),IR(16965, 7)/* Xderef */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION
  {7,IR(0, 0),IR(16972, 11)/* StackValue */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION
  {8,IR(0, 1),IR(16983, 7)/* Constu */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION
  {9,IR(1, 2),IR(16990, 9)/* Fragment */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION
  {0,IR(0, 0),IR(16999, 10)/* ConstType */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_TYPE_QUALIFIER
  {1,IR(0, 0),IR(17009, 13)/* VolatileType */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_TYPE_QUALIFIER
  {2,IR(0, 0),IR(17022, 13)/* RestrictType */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_TYPE_QUALIFIER
  {3,IR(0, 0),IR(17035, 11)/* AtomicType */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_TYPE_QUALIFIER
  {0,IR(0, 0),IR(17046, 10)/* Float16NV */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COMPONENT_TYPE
  {1,IR(0, 0),IR(17056, 10)/* Float32NV */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COMPONENT_TYPE
  {2,IR(0, 0),IR(17066, 10)/* Float64NV */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COMPONENT_TYPE
  {3,IR(0, 0),IR(17076, 13)/* SignedInt8NV */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COMPONENT_TYPE
  {4,IR(0, 0),IR(17089, 14)/* SignedInt16NV */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COMPONENT_TYPE
  {5,IR(0, 0),IR(17103, 14)/* SignedInt32NV */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COMPONENT_TYPE
  {6,IR(0, 0),IR(17117, 14)/* SignedInt64NV */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COMPONENT_TYPE
  {7,IR(0, 0),IR(17131, 15)/* UnsignedInt8NV */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COMPONENT_TYPE
  {8,IR(0, 0),IR(17146, 16)/* UnsignedInt16NV */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COMPONENT_TYPE
  {9,IR(0, 0),IR(17162, 16)/* UnsignedInt32NV */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COMPONENT_TYPE
  {10,IR(0, 0),IR(17178, 16)/* UnsignedInt64NV */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COMPONENT_TYPE
  {1000491000,IR(0, 0),IR(17194, 19)/* SignedInt8PackedNV */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COMPONENT_TYPE
  {1000491001,IR(0, 0),IR(17213, 21)/* UnsignedInt8PackedNV */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COMPONENT_TYPE
  {1000491002,IR(0, 0),IR(17234, 12)/* FloatE4M3NV */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COMPONENT_TYPE
  {1000491003,IR(0, 0),IR(17246, 12)/* FloatE5M2NV */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COMPONENT_TYPE
  {0,IR(0, 0),IR(17258, 12)/* RowMajorKHR */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_LAYOUT
  {1,IR(0, 0),IR(17270, 15)/* ColumnMajorKHR */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_LAYOUT
  {4202,IR(0, 0),IR(17285, 25)/* RowBlockedInterleavedARM */,IR(0, 0),IR(118, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_LAYOUT
  {4203,IR(0, 0),IR(17310, 28)/* ColumnBlockedInterleavedARM */,IR(0, 0),IR(118, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_LAYOUT
  {0,IR(0, 0),IR(17338, 8)/* NoneKHR */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_OPERANDS
  {1,IR(0, 0),IR(17346, 27)/* MatrixASignedComponentsKHR */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_OPERANDS
  {2,IR(0, 0),IR(17373, 27)/* MatrixBSignedComponentsKHR */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_OPERANDS
  {4,IR(0, 0),IR(17400, 27)/* MatrixCSignedComponentsKHR */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_OPERANDS
  {8,IR(0, 0),IR(17427, 32)/* MatrixResultSignedComponentsKHR */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_OPERANDS
  {16,IR(0, 0),IR(17459, 26)/* SaturatingAccumulationKHR */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_OPERANDS
  {1,IR(0, 0),IR(17485, 4)/* Row */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_REDUCE
  {2,IR(0, 0),IR(17489, 7)/* Column */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_REDUCE
  {4,IR(0, 0),IR(17496, 4)/* 2x2 */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_REDUCE
  {0,IR(0, 0),IR(17500, 11)/* MatrixAKHR */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_USE
  {1,IR(0, 0),IR(17511, 11)/* MatrixBKHR */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_USE
  {2,IR(0, 0),IR(17522, 21)/* MatrixAccumulatorKHR */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_USE
  {0,IR(0, 0),IR(17543, 11)/* RowMajorNV */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COOPERATIVE_VECTOR_MATRIX_LAYOUT
  {1,IR(0, 0),IR(17554, 14)/* ColumnMajorNV */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COOPERATIVE_VECTOR_MATRIX_LAYOUT
  {2,IR(0, 0),IR(17568, 21)/* InferencingOptimalNV */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COOPERATIVE_VECTOR_MATRIX_LAYOUT
  {3,IR(0, 0),IR(17589, 18)/* TrainingOptimalNV */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_COOPERATIVE_VECTOR_MATRIX_LAYOUT
  {0,IR(0, 0),IR(16455, 12)/* Unspecified */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {1,IR(0, 0),IR(16467, 8)/* Address */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {2,IR(0, 0),IR(16475, 8)/* Boolean */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {4,IR(0, 0),IR(16483, 6)/* Float */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {5,IR(0, 0),IR(16489, 7)/* Signed */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {6,IR(0, 0),IR(16496, 11)/* SignedChar */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {7,IR(0, 0),IR(16507, 9)/* Unsigned */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {8,IR(0, 0),IR(16516, 13)/* UnsignedChar */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {0,IR(0, 0),IR(16529, 6)/* Class */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_COMPOSITE_TYPE
  {1,IR(0, 0),IR(16535, 10)/* Structure */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_COMPOSITE_TYPE
  {2,IR(0, 0),IR(16545, 6)/* Union */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_COMPOSITE_TYPE
  {0,IR(0, 0),IR(16586, 5)/* None */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {1,IR(0, 0),IR(16591, 16)/* FlagIsProtected */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {2,IR(0, 0),IR(16607, 14)/* FlagIsPrivate */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {3,IR(0, 0),IR(16621, 13)/* FlagIsPublic */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {4,IR(0, 0),IR(16634, 12)/* FlagIsLocal */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {8,IR(0, 0),IR(16646, 17)/* FlagIsDefinition */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {16,IR(0, 0),IR(16663, 12)/* FlagFwdDecl */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {32,IR(0, 0),IR(16675, 15)/* FlagArtificial */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {64,IR(0, 0),IR(16690, 13)/* FlagExplicit */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {128,IR(0, 0),IR(16703, 15)/* FlagPrototyped */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {256,IR(0, 0),IR(16718, 18)/* FlagObjectPointer */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {512,IR(0, 0),IR(16736, 17)/* FlagStaticMember */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {1024,IR(0, 0),IR(16753, 21)/* FlagIndirectVariable */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {2048,IR(0, 0),IR(16774, 20)/* FlagLValueReference */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {4096,IR(0, 0),IR(16794, 20)/* FlagRValueReference */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {8192,IR(0, 0),IR(16814, 16)/* FlagIsOptimized */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {0,IR(0, 0),IR(16890, 6)/* Deref */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_OPERATION
  {1,IR(0, 0),IR(16896, 5)/* Plus */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_OPERATION
  {2,IR(0, 0),IR(16901, 6)/* Minus */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_OPERATION
  {3,IR(0, 1),IR(16940, 11)/* PlusUconst */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_OPERATION
  {4,IR(1, 2),IR(16951, 9)/* BitPiece */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_OPERATION
  {5,IR(0, 0),IR(16960, 5)/* Swap */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_OPERATION
  {6,IR(0, 0),IR(16965, 7)/* Xderef */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_OPERATION
  {7,IR(0, 0),IR(16972, 11)/* StackValue */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_OPERATION
  {8,IR(0, 1),IR(16983, 7)/* Constu */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_OPERATION
  {0,IR(0, 0),IR(16999, 10)/* ConstType */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_TYPE_QUALIFIER
  {1,IR(0, 0),IR(17009, 13)/* VolatileType */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_TYPE_QUALIFIER
  {2,IR(0, 0),IR(17022, 13)/* RestrictType */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_DEBUG_TYPE_QUALIFIER
  {0,IR(0, 0),IR(17607, 17)/* RelaxedPrecision */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {1,IR(0, 1),IR(17624, 7)/* SpecId */,IR(0, 0),IR(119, 2),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {2,IR(0, 0),IR(17631, 6)/* Block */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {3,IR(0, 0),IR(17637, 12)/* BufferBlock */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),SPV_SPIRV_VERSION_WORD(1,3)}, // SPV_OPERAND_TYPE_DECORATION
  {4,IR(0, 0),IR(17649, 9)/* RowMajor */,IR(0, 0),IR(67, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5,IR(0, 0),IR(17658, 9)/* ColMajor */,IR(0, 0),IR(67, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6,IR(0, 1),IR(17667, 12)/* ArrayStride */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {7,IR(0, 1),IR(17679, 13)/* MatrixStride */,IR(0, 0),IR(67, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {8,IR(0, 0),IR(17692, 11)/* GLSLShared */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {9,IR(0, 0),IR(17703, 11)/* GLSLPacked */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {10,IR(0, 0),IR(17714, 8)/* CPacked */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {11,IR(3, 1),IR(17748, 8)/* BuiltIn */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {13,IR(0, 0),IR(17756, 14)/* NoPerspective */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {14,IR(0, 0),IR(17770, 5)/* Flat */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {15,IR(0, 0),IR(17775, 6)/* Patch */,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {16,IR(0, 0),IR(17781, 9)/* Centroid */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {17,IR(0, 0),IR(17790, 7)/* Sample */,IR(0, 0),IR(25, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {18,IR(0, 0),IR(17797, 10)/* Invariant */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {19,IR(0, 0),IR(17807, 9)/* Restrict */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {20,IR(0, 0),IR(17816, 8)/* Aliased */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {21,IR(0, 0),IR(17824, 9)/* Volatile */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {22,IR(0, 0),IR(17833, 9)/* Constant */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {23,IR(0, 0),IR(17842, 9)/* Coherent */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {24,IR(0, 0),IR(17851, 12)/* NonWritable */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {25,IR(0, 0),IR(17863, 12)/* NonReadable */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {26,IR(0, 0),IR(17875, 8)/* Uniform */,IR(0, 0),IR(121, 2),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {27,IR(4, 1),IR(17909, 10)/* UniformId */,IR(0, 0),IR(121, 2),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {28,IR(0, 0),IR(17919, 20)/* SaturatedConversion */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {29,IR(0, 1),IR(17939, 7)/* Stream */,IR(0, 0),IR(123, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {30,IR(0, 1),IR(17946, 9)/* Location */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {31,IR(0, 1),IR(17955, 10)/* Component */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {32,IR(0, 1),IR(17965, 6)/* Index */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {33,IR(0, 1),IR(17971, 8)/* Binding */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {34,IR(0, 1),IR(17979, 14)/* DescriptorSet */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {35,IR(0, 1),IR(17993, 7)/* Offset */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {36,IR(0, 1),IR(18000, 10)/* XfbBuffer */,IR(0, 0),IR(124, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {37,IR(0, 1),IR(18010, 10)/* XfbStride */,IR(0, 0),IR(124, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {38,IR(5, 1),IR(18066, 14)/* FuncParamAttr */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {39,IR(6, 1),IR(18114, 15)/* FPRoundingMode */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {40,IR(7, 1),IR(18164, 15)/* FPFastMathMode */,IR(0, 0),IR(125, 2),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {41,IR(8, 2),IR(18241, 18)/* LinkageAttributes */,IR(0, 0),IR(94, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {42,IR(0, 0),IR(18259, 14)/* NoContraction */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {43,IR(0, 1),IR(18273, 21)/* InputAttachmentIndex */,IR(0, 0),IR(98, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {44,IR(0, 1),IR(18294, 10)/* Alignment */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {45,IR(0, 1),IR(18304, 14)/* MaxByteOffset */,IR(0, 0),IR(1, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,1),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {46,IR(10, 1),IR(18338, 12)/* AlignmentId */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,2),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {47,IR(10, 1),IR(18350, 16)/* MaxByteOffsetId */,IR(0, 0),IR(1, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,2),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {4216,IR(0, 0),IR(18366, 44)/* SaturatedToLargestFloat8NormalConversionEXT */,IR(0, 0),IR(117, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {4469,IR(0, 0),IR(18410, 13)/* NoSignedWrap */,IR(0, 0),IR(1, 0),IR(195, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {4470,IR(0, 0),IR(18423, 15)/* NoUnsignedWrap */,IR(0, 0),IR(1, 0),IR(195, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {4487,IR(0, 0),IR(18438, 18)/* WeightTextureQCOM */,IR(0, 0),IR(1, 0),IR(53, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {4488,IR(0, 0),IR(18456, 22)/* BlockMatchTextureQCOM */,IR(0, 0),IR(1, 0),IR(53, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {4499,IR(0, 0),IR(18478, 22)/* BlockMatchSamplerQCOM */,IR(0, 0),IR(1, 0),IR(56, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {4999,IR(0, 0),IR(18500, 18)/* ExplicitInterpAMD */,IR(0, 0),IR(1, 0),IR(10, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5019,IR(10, 1),IR(18518, 32)/* NodeSharesPayloadLimitsWithAMDX */,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5020,IR(10, 1),IR(18550, 20)/* NodeMaxPayloadsAMDX */,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5078,IR(0, 0),IR(18570, 23)/* TrackFinishWritingAMDX */,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5091,IR(10, 1),IR(18593, 20)/* PayloadNodeNameAMDX */,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5098,IR(10, 1),IR(18613, 25)/* PayloadNodeBaseIndexAMDX */,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5099,IR(0, 0),IR(18638, 27)/* PayloadNodeSparseArrayAMDX */,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5100,IR(10, 1),IR(18665, 25)/* PayloadNodeArraySizeAMDX */,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5105,IR(0, 0),IR(18690, 28)/* PayloadDispatchIndirectAMDX */,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5124,IR(10, 1),IR(18718, 17)/* ArrayStrideIdEXT */,IR(0, 0),IR(44, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5125,IR(10, 1),IR(18735, 12)/* OffsetIdEXT */,IR(0, 0),IR(44, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5145,IR(0, 0),IR(18747, 14)/* UTFEncodedKHR */,IR(0, 0),IR(127, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5248,IR(0, 0),IR(18761, 19)/* OverrideCoverageNV */,IR(0, 0),IR(128, 1),IR(74, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5250,IR(0, 0),IR(18780, 14)/* PassthroughNV */,IR(0, 0),IR(129, 1),IR(75, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5252,IR(0, 0),IR(18794, 19)/* ViewportRelativeNV */,IR(0, 0),IR(97, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5256,IR(0, 1),IR(18813, 28)/* SecondaryViewportRelativeNV */,IR(0, 0),IR(47, 1),IR(14, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5271,IR(0, 0),IR(18841, 16)/* PerPrimitiveEXT */,IR(75, 1),IR(130, 2),IR(196, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5272,IR(0, 0),IR(18872, 10)/* PerViewNV */,IR(0, 0),IR(51, 1),IR(18, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5273,IR(0, 0),IR(18882, 10)/* PerTaskNV */,IR(0, 0),IR(51, 1),IR(18, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5285,IR(0, 0),IR(18892, 13)/* PerVertexKHR */,IR(76, 1),IR(52, 1),IR(19, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5300,IR(0, 0),IR(18917, 11)/* NonUniform */,IR(77, 1),IR(100, 1),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5355,IR(0, 0),IR(18942, 16)/* RestrictPointer */,IR(78, 1),IR(2, 1),IR(0, 2),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5356,IR(0, 0),IR(18977, 15)/* AliasedPointer */,IR(79, 1),IR(2, 1),IR(0, 2),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5358,IR(0, 1),IR(19010, 15)/* MemberOffsetNV */,IR(0, 0),IR(132, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5386,IR(0, 0),IR(19025, 30)/* HitObjectShaderRecordBufferNV */,IR(0, 0),IR(133, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5389,IR(0, 0),IR(19055, 31)/* HitObjectShaderRecordBufferEXT */,IR(0, 0),IR(134, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5397,IR(0, 1),IR(19086, 7)/* BankNV */,IR(0, 0),IR(132, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5398,IR(0, 0),IR(19093, 18)/* BindlessSamplerNV */,IR(0, 0),IR(135, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5399,IR(0, 0),IR(19111, 16)/* BindlessImageNV */,IR(0, 0),IR(135, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5400,IR(0, 0),IR(19127, 15)/* BoundSamplerNV */,IR(0, 0),IR(135, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5401,IR(0, 0),IR(19142, 13)/* BoundImageNV */,IR(0, 0),IR(135, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5599,IR(0, 1),IR(19155, 14)/* SIMTCallINTEL */,IR(0, 0),IR(136, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5602,IR(0, 0),IR(19169, 26)/* ReferencedIndirectlyINTEL */,IR(0, 0),IR(137, 1),IR(110, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5607,IR(11, 1),IR(19195, 13)/* ClobberINTEL */,IR(0, 0),IR(138, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5608,IR(0, 0),IR(19208, 17)/* SideEffectsINTEL */,IR(0, 0),IR(138, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5624,IR(0, 0),IR(19225, 27)/* VectorComputeVariableINTEL */,IR(0, 0),IR(136, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5625,IR(0, 1),IR(19252, 21)/* FuncParamIOKindINTEL */,IR(0, 0),IR(136, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5626,IR(0, 0),IR(19273, 27)/* VectorComputeFunctionINTEL */,IR(0, 0),IR(136, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5627,IR(0, 0),IR(19300, 15)/* StackCallINTEL */,IR(0, 0),IR(136, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5628,IR(0, 1),IR(19315, 26)/* GlobalVariableOffsetINTEL */,IR(0, 0),IR(136, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5634,IR(10, 1),IR(19341, 14)/* CounterBuffer */,IR(80, 1),IR(1, 0),IR(198, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5635,IR(11, 1),IR(19379, 13)/* UserSemantic */,IR(81, 1),IR(1, 0),IR(198, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5636,IR(11, 1),IR(19411, 15)/* UserTypeGOOGLE */,IR(0, 0),IR(1, 0),IR(199, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5822,IR(12, 2),IR(19426, 26)/* FunctionRoundingModeINTEL */,IR(0, 0),IR(139, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5823,IR(14, 2),IR(19483, 24)/* FunctionDenormModeINTEL */,IR(0, 0),IR(139, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5825,IR(0, 0),IR(19507, 15)/* RegisterALTERA */,IR(82, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5826,IR(11, 1),IR(19536, 13)/* MemoryALTERA */,IR(83, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5827,IR(0, 1),IR(19561, 15)/* NumbanksALTERA */,IR(84, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5828,IR(0, 1),IR(19590, 16)/* BankwidthALTERA */,IR(85, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5829,IR(0, 1),IR(19621, 23)/* MaxPrivateCopiesALTERA */,IR(86, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5830,IR(0, 0),IR(19666, 17)/* SinglepumpALTERA */,IR(87, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5831,IR(0, 0),IR(19699, 17)/* DoublepumpALTERA */,IR(88, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5832,IR(0, 1),IR(19732, 20)/* MaxReplicatesALTERA */,IR(89, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5833,IR(0, 0),IR(19771, 21)/* SimpleDualPortALTERA */,IR(90, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5834,IR(16, 2),IR(19812, 12)/* MergeALTERA */,IR(91, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5835,IR(18, 1),IR(19877, 15)/* BankBitsALTERA */,IR(92, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5836,IR(0, 1),IR(19906, 21)/* ForcePow2DepthALTERA */,IR(93, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5883,IR(0, 1),IR(19947, 17)/* StridesizeALTERA */,IR(94, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5884,IR(0, 1),IR(19980, 15)/* WordsizeALTERA */,IR(95, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5885,IR(0, 0),IR(20009, 19)/* TrueDualPortALTERA */,IR(96, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5899,IR(0, 0),IR(20046, 20)/* BurstCoalesceALTERA */,IR(97, 1),IR(141, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5900,IR(0, 1),IR(20085, 16)/* CacheSizeALTERA */,IR(98, 1),IR(141, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5901,IR(0, 0),IR(20116, 29)/* DontStaticallyCoalesceALTERA */,IR(99, 1),IR(141, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5902,IR(0, 1),IR(20173, 15)/* PrefetchALTERA */,IR(100, 1),IR(141, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5905,IR(0, 0),IR(20202, 18)/* StallEnableALTERA */,IR(101, 1),IR(110, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5907,IR(0, 0),IR(20237, 26)/* FuseLoopsInFunctionALTERA */,IR(102, 1),IR(142, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5909,IR(1, 2),IR(20288, 20)/* MathOpDSPModeALTERA */,IR(103, 1),IR(143, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5914,IR(10, 1),IR(20327, 16)/* AliasScopeINTEL */,IR(0, 0),IR(144, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5915,IR(10, 1),IR(20343, 13)/* NoAliasINTEL */,IR(0, 0),IR(144, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5917,IR(0, 1),IR(20356, 25)/* InitiationIntervalALTERA */,IR(104, 1),IR(145, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5918,IR(0, 1),IR(20405, 21)/* MaxConcurrencyALTERA */,IR(105, 1),IR(145, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5919,IR(0, 1),IR(20446, 21)/* PipelineEnableALTERA */,IR(106, 1),IR(145, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5921,IR(0, 1),IR(20487, 21)/* BufferLocationALTERA */,IR(107, 1),IR(146, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {5944,IR(0, 1),IR(20528, 20)/* IOPipeStorageALTERA */,IR(108, 1),IR(147, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6080,IR(19, 2),IR(20601, 31)/* FunctionFloatingPointModeINTEL */,IR(0, 0),IR(139, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6085,IR(0, 0),IR(20632, 25)/* SingleElementVectorINTEL */,IR(0, 0),IR(136, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6087,IR(0, 0),IR(20657, 35)/* VectorComputeCallableFunctionINTEL */,IR(0, 0),IR(136, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6140,IR(0, 0),IR(20692, 18)/* MediaBlockIOINTEL */,IR(0, 0),IR(136, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6151,IR(0, 0),IR(20710, 16)/* StallFreeALTERA */,IR(109, 1),IR(148, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6170,IR(21, 1),IR(20772, 26)/* FPMaxErrorDecorationINTEL */,IR(0, 0),IR(149, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6172,IR(0, 1),IR(20798, 26)/* LatencyControlLabelALTERA */,IR(110, 1),IR(150, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6173,IR(22, 3),IR(20849, 31)/* LatencyControlConstraintALTERA */,IR(111, 1),IR(150, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6175,IR(0, 0),IR(20910, 28)/* ConduitKernelArgumentALTERA */,IR(112, 1),IR(151, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6176,IR(0, 0),IR(20965, 32)/* RegisterMapKernelArgumentALTERA */,IR(113, 1),IR(151, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6177,IR(0, 1),IR(21028, 34)/* MMHostInterfaceAddressWidthALTERA */,IR(114, 1),IR(151, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6178,IR(0, 1),IR(21095, 31)/* MMHostInterfaceDataWidthALTERA */,IR(115, 1),IR(151, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6179,IR(0, 1),IR(21156, 29)/* MMHostInterfaceLatencyALTERA */,IR(116, 1),IR(151, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6180,IR(25, 1),IR(21247, 35)/* MMHostInterfaceReadWriteModeALTERA */,IR(117, 1),IR(151, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6181,IR(0, 1),IR(21316, 30)/* MMHostInterfaceMaxBurstALTERA */,IR(118, 1),IR(151, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6182,IR(0, 1),IR(21375, 33)/* MMHostInterfaceWaitRequestALTERA */,IR(119, 1),IR(151, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6183,IR(0, 0),IR(21440, 27)/* StableKernelArgumentALTERA */,IR(120, 1),IR(151, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6188,IR(26, 2),IR(21532, 16)/* HostAccessINTEL */,IR(0, 0),IR(152, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6190,IR(28, 1),IR(21595, 15)/* InitModeALTERA */,IR(121, 1),IR(153, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6191,IR(0, 1),IR(21624, 29)/* ImplementInRegisterMapALTERA */,IR(122, 1),IR(153, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6247,IR(10, 1),IR(21681, 17)/* ConditionalINTEL */,IR(0, 0),IR(115, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6442,IR(29, 2),IR(21734, 22)/* CacheControlLoadINTEL */,IR(0, 0),IR(154, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {6443,IR(31, 2),IR(21793, 23)/* CacheControlStoreINTEL */,IR(0, 0),IR(154, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DECORATION
  {0,IR(0, 0),IR(21816, 3)/* 1D */,IR(0, 0),IR(73, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DIMENSIONALITY
  {1,IR(0, 0),IR(21819, 3)/* 2D */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DIMENSIONALITY
  {2,IR(0, 0),IR(21822, 3)/* 3D */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DIMENSIONALITY
  {3,IR(0, 0),IR(21825, 5)/* Cube */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DIMENSIONALITY
  {4,IR(0, 0),IR(21830, 5)/* Rect */,IR(0, 0),IR(72, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DIMENSIONALITY
  {5,IR(0, 0),IR(21835, 7)/* Buffer */,IR(0, 0),IR(74, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DIMENSIONALITY
  {6,IR(0, 0),IR(21842, 12)/* SubpassData */,IR(0, 0),IR(98, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_DIMENSIONALITY
  {4173,IR(0, 0),IR(21854, 17)/* TileImageDataEXT */,IR(0, 0),IR(155, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_DIMENSIONALITY
  {0,IR(0, 1),IR(21871, 12)/* Invocations */,IR(0, 0),IR(70, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {1,IR(0, 0),IR(21883, 13)/* SpacingEqual */,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {2,IR(0, 0),IR(21896, 22)/* SpacingFractionalEven */,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {3,IR(0, 0),IR(21918, 21)/* SpacingFractionalOdd */,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {4,IR(0, 0),IR(21939, 14)/* VertexOrderCw */,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5,IR(0, 0),IR(21953, 15)/* VertexOrderCcw */,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {6,IR(0, 0),IR(21968, 19)/* PixelCenterInteger */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {7,IR(0, 0),IR(21987, 16)/* OriginUpperLeft */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {8,IR(0, 0),IR(22003, 16)/* OriginLowerLeft */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {9,IR(0, 0),IR(22019, 19)/* EarlyFragmentTests */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {10,IR(0, 0),IR(22038, 10)/* PointMode */,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {11,IR(0, 0),IR(22048, 4)/* Xfb */,IR(0, 0),IR(124, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {12,IR(0, 0),IR(22052, 15)/* DepthReplacing */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {14,IR(0, 0),IR(22067, 13)/* DepthGreater */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {15,IR(0, 0),IR(22080, 10)/* DepthLess */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {16,IR(0, 0),IR(22090, 15)/* DepthUnchanged */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {17,IR(22, 3),IR(22105, 10)/* LocalSize */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {18,IR(22, 3),IR(22115, 14)/* LocalSizeHint */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {19,IR(0, 0),IR(22129, 12)/* InputPoints */,IR(0, 0),IR(70, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {20,IR(0, 0),IR(22141, 11)/* InputLines */,IR(0, 0),IR(70, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {21,IR(0, 0),IR(22152, 20)/* InputLinesAdjacency */,IR(0, 0),IR(70, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {22,IR(0, 0),IR(22172, 10)/* Triangles */,IR(0, 0),IR(12, 2),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {23,IR(0, 0),IR(22182, 24)/* InputTrianglesAdjacency */,IR(0, 0),IR(70, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {24,IR(0, 0),IR(22206, 6)/* Quads */,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {25,IR(0, 0),IR(22212, 9)/* Isolines */,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {26,IR(0, 1),IR(22221, 15)/* OutputVertices */,IR(0, 0),IR(156, 4),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {27,IR(0, 0),IR(22236, 13)/* OutputPoints */,IR(0, 0),IR(160, 3),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {28,IR(0, 0),IR(22249, 16)/* OutputLineStrip */,IR(0, 0),IR(70, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {29,IR(0, 0),IR(22265, 20)/* OutputTriangleStrip */,IR(0, 0),IR(70, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {30,IR(0, 1),IR(22285, 12)/* VecTypeHint */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {31,IR(0, 0),IR(22297, 15)/* ContractionOff */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {33,IR(0, 0),IR(22312, 12)/* Initializer */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,1),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {34,IR(0, 0),IR(22324, 10)/* Finalizer */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,1),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {35,IR(0, 1),IR(6557, 13)/* SubgroupSize */,IR(0, 0),IR(163, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,1),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {36,IR(0, 1),IR(22334, 22)/* SubgroupsPerWorkgroup */,IR(0, 0),IR(163, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,1),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {37,IR(10, 1),IR(22356, 24)/* SubgroupsPerWorkgroupId */,IR(0, 0),IR(163, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,2),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {38,IR(33, 3),IR(22380, 12)/* LocalSizeId */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,2),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {39,IR(33, 3),IR(22392, 16)/* LocalSizeHintId */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,2),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {4169,IR(0, 0),IR(22408, 34)/* NonCoherentColorAttachmentReadEXT */,IR(0, 0),IR(155, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {4170,IR(0, 0),IR(22442, 34)/* NonCoherentDepthAttachmentReadEXT */,IR(0, 0),IR(164, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {4171,IR(0, 0),IR(22476, 36)/* NonCoherentStencilAttachmentReadEXT */,IR(0, 0),IR(165, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {4421,IR(0, 0),IR(22512, 30)/* SubgroupUniformControlFlowKHR */,IR(0, 0),IR(3, 1),IR(200, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {4446,IR(0, 0),IR(22542, 18)/* PostDepthCoverage */,IR(0, 0),IR(166, 1),IR(46, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {4459,IR(0, 1),IR(10875, 15)/* DenormPreserve */,IR(0, 0),IR(167, 1),IR(48, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {4460,IR(0, 1),IR(10890, 18)/* DenormFlushToZero */,IR(0, 0),IR(168, 1),IR(48, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {4461,IR(0, 1),IR(10908, 25)/* SignedZeroInfNanPreserve */,IR(0, 0),IR(169, 1),IR(48, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {4462,IR(0, 1),IR(10933, 16)/* RoundingModeRTE */,IR(0, 0),IR(170, 1),IR(48, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {4463,IR(0, 1),IR(10949, 16)/* RoundingModeRTZ */,IR(0, 0),IR(171, 1),IR(48, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {4489,IR(0, 0),IR(22560, 34)/* NonCoherentTileAttachmentReadQCOM */,IR(0, 0),IR(41, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {4490,IR(22, 3),IR(22594, 20)/* TileShadingRateQCOM */,IR(0, 0),IR(41, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5017,IR(0, 0),IR(22614, 29)/* EarlyAndLateFragmentTestsAMD */,IR(0, 0),IR(3, 1),IR(201, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5027,IR(0, 0),IR(22643, 23)/* StencilRefReplacingEXT */,IR(0, 0),IR(42, 1),IR(11, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5069,IR(0, 0),IR(22666, 15)/* CoalescingAMDX */,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5070,IR(10, 1),IR(22681, 15)/* IsApiEntryAMDX */,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5071,IR(10, 1),IR(22696, 21)/* MaxNodeRecursionAMDX */,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5072,IR(33, 3),IR(22717, 24)/* StaticNumWorkgroupsAMDX */,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5073,IR(10, 1),IR(7450, 16)/* ShaderIndexAMDX */,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5077,IR(33, 3),IR(22741, 21)/* MaxNumWorkgroupsAMDX */,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5079,IR(0, 0),IR(22762, 28)/* StencilRefUnchangedFrontAMD */,IR(0, 0),IR(42, 1),IR(202, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5080,IR(0, 0),IR(22790, 26)/* StencilRefGreaterFrontAMD */,IR(0, 0),IR(42, 1),IR(202, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5081,IR(0, 0),IR(22816, 23)/* StencilRefLessFrontAMD */,IR(0, 0),IR(42, 1),IR(202, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5082,IR(0, 0),IR(22839, 27)/* StencilRefUnchangedBackAMD */,IR(0, 0),IR(42, 1),IR(202, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5083,IR(0, 0),IR(22866, 25)/* StencilRefGreaterBackAMD */,IR(0, 0),IR(42, 1),IR(202, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5084,IR(0, 0),IR(22891, 22)/* StencilRefLessBackAMD */,IR(0, 0),IR(42, 1),IR(202, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5088,IR(0, 0),IR(22913, 19)/* QuadDerivativesKHR */,IR(0, 0),IR(172, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5089,IR(0, 0),IR(22932, 20)/* RequireFullQuadsKHR */,IR(0, 0),IR(172, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5102,IR(36, 2),IR(22952, 20)/* SharesInputWithAMDX */,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5157,IR(0, 0),IR(22972, 20)/* ArithmeticPoisonKHR */,IR(0, 0),IR(173, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5269,IR(0, 0),IR(22992, 15)/* OutputLinesEXT */,IR(123, 1),IR(130, 2),IR(196, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5270,IR(0, 1),IR(23021, 20)/* OutputPrimitivesEXT */,IR(124, 1),IR(130, 2),IR(196, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5289,IR(0, 0),IR(23060, 24)/* DerivativeGroupQuadsKHR */,IR(125, 1),IR(174, 1),IR(81, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5290,IR(0, 0),IR(23107, 25)/* DerivativeGroupLinearKHR */,IR(126, 1),IR(175, 1),IR(81, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5298,IR(0, 0),IR(23156, 19)/* OutputTrianglesEXT */,IR(127, 1),IR(130, 2),IR(196, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5366,IR(0, 0),IR(23193, 25)/* PixelInterlockOrderedEXT */,IR(0, 0),IR(176, 1),IR(89, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5367,IR(0, 0),IR(23218, 27)/* PixelInterlockUnorderedEXT */,IR(0, 0),IR(176, 1),IR(89, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5368,IR(0, 0),IR(23245, 26)/* SampleInterlockOrderedEXT */,IR(0, 0),IR(177, 1),IR(89, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5369,IR(0, 0),IR(23271, 28)/* SampleInterlockUnorderedEXT */,IR(0, 0),IR(177, 1),IR(89, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5370,IR(0, 0),IR(23299, 31)/* ShadingRateInterlockOrderedEXT */,IR(0, 0),IR(178, 1),IR(89, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5371,IR(0, 0),IR(23330, 33)/* ShadingRateInterlockUnorderedEXT */,IR(0, 0),IR(178, 1),IR(89, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5427,IR(0, 0),IR(13340, 23)/* Shader64BitIndexingEXT */,IR(0, 0),IR(179, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5618,IR(0, 1),IR(23363, 27)/* SharedLocalMemorySizeINTEL */,IR(0, 0),IR(136, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5620,IR(0, 1),IR(23390, 21)/* RoundingModeRTPINTEL */,IR(0, 0),IR(180, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5621,IR(0, 1),IR(23411, 21)/* RoundingModeRTNINTEL */,IR(0, 0),IR(180, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5622,IR(0, 1),IR(23432, 26)/* FloatingPointModeALTINTEL */,IR(0, 0),IR(180, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5623,IR(0, 1),IR(23458, 27)/* FloatingPointModeIEEEINTEL */,IR(0, 0),IR(180, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5893,IR(22, 3),IR(23485, 22)/* MaxWorkgroupSizeINTEL */,IR(0, 0),IR(181, 1),IR(127, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5894,IR(0, 1),IR(23507, 16)/* MaxWorkDimINTEL */,IR(0, 0),IR(181, 1),IR(127, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5895,IR(0, 0),IR(23523, 20)/* NoGlobalOffsetINTEL */,IR(0, 0),IR(181, 1),IR(127, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5896,IR(0, 1),IR(23543, 22)/* NumSIMDWorkitemsINTEL */,IR(0, 0),IR(111, 1),IR(127, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {5903,IR(0, 1),IR(23565, 28)/* SchedulerTargetFmaxMhzINTEL */,IR(0, 0),IR(111, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {6023,IR(0, 0),IR(23593, 24)/* MaximallyReconvergesKHR */,IR(0, 0),IR(3, 1),IR(204, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {6028,IR(36, 2),IR(23617, 18)/* FPFastMathDefault */,IR(0, 0),IR(182, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {6031,IR(10, 1),IR(23635, 21)/* OpacityMicromapIdKHR */,IR(0, 0),IR(183, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {6154,IR(0, 1),IR(23656, 24)/* StreamingInterfaceINTEL */,IR(0, 0),IR(111, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {6160,IR(0, 1),IR(23680, 26)/* RegisterMapInterfaceINTEL */,IR(0, 0),IR(184, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {6417,IR(0, 1),IR(23706, 23)/* NamedBarrierCountINTEL */,IR(0, 0),IR(136, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {6461,IR(0, 1),IR(23729, 22)/* MaximumRegistersINTEL */,IR(0, 0),IR(185, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {6462,IR(10, 1),IR(23751, 24)/* MaximumRegistersIdINTEL */,IR(0, 0),IR(185, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {6463,IR(38, 1),IR(23826, 27)/* NamedMaximumRegistersINTEL */,IR(0, 0),IR(185, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODE
  {0,IR(0, 0),IR(23853, 7)/* Vertex */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODEL
  {1,IR(0, 0),IR(23860, 20)/* TessellationControl */,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODEL
  {2,IR(0, 0),IR(23880, 23)/* TessellationEvaluation */,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODEL
  {3,IR(0, 0),IR(6038, 9)/* Geometry */,IR(0, 0),IR(70, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODEL
  {4,IR(0, 0),IR(16990, 9)/* Fragment */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODEL
  {5,IR(0, 0),IR(23903, 10)/* GLCompute */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODEL
  {6,IR(0, 0),IR(5805, 7)/* Kernel */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODEL
  {5267,IR(0, 0),IR(23913, 7)/* TaskNV */,IR(0, 0),IR(51, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODEL
  {5268,IR(0, 0),IR(23920, 7)/* MeshNV */,IR(0, 0),IR(51, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODEL
  {5313,IR(0, 0),IR(23927, 17)/* RayGenerationKHR */,IR(128, 1),IR(55, 2),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODEL
  {5314,IR(0, 0),IR(23960, 16)/* IntersectionKHR */,IR(129, 1),IR(55, 2),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODEL
  {5315,IR(0, 0),IR(23991, 10)/* AnyHitKHR */,IR(130, 1),IR(55, 2),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODEL
  {5316,IR(0, 0),IR(24010, 14)/* ClosestHitKHR */,IR(131, 1),IR(55, 2),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODEL
  {5317,IR(0, 0),IR(24037, 8)/* MissKHR */,IR(132, 1),IR(55, 2),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODEL
  {5318,IR(0, 0),IR(24052, 12)/* CallableKHR */,IR(133, 1),IR(55, 2),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODEL
  {5364,IR(0, 0),IR(24075, 8)/* TaskEXT */,IR(0, 0),IR(54, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODEL
  {5365,IR(0, 0),IR(24083, 8)/* MeshEXT */,IR(0, 0),IR(54, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_EXECUTION_MODEL
  {0,IR(0, 0),IR(24091, 9)/* Preserve */,IR(0, 0),IR(139, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_FPDENORM_MODE
  {1,IR(0, 0),IR(24100, 12)/* FlushToZero */,IR(0, 0),IR(139, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_FPDENORM_MODE
  {0,IR(0, 0),IR(24112, 12)/* BFloat16KHR */,IR(0, 0),IR(90, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_FPENCODING
  {4214,IR(0, 0),IR(24124, 14)/* Float8E4M3EXT */,IR(0, 0),IR(117, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_FPENCODING
  {4215,IR(0, 0),IR(24138, 14)/* Float8E5M2EXT */,IR(0, 0),IR(117, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_FPENCODING
  {4223,IR(0, 0),IR(24152, 14)/* Float6E2M3EXT */,IR(0, 0),IR(186, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_FPENCODING
  {4224,IR(0, 0),IR(24166, 14)/* Float6E3M2EXT */,IR(0, 0),IR(186, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_FPENCODING
  {4225,IR(0, 0),IR(24180, 14)/* Float4E2M1EXT */,IR(0, 0),IR(187, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_FPENCODING
  {4226,IR(0, 0),IR(10370, 22)/* Float8UnsignedE8M0EXT */,IR(0, 0),IR(188, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_FPENCODING
  {4227,IR(0, 0),IR(10392, 10)/* MXInt8EXT */,IR(0, 0),IR(189, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_FPENCODING
  {0,IR(0, 0),IR(24194, 5)/* IEEE */,IR(0, 0),IR(139, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_FPOPERATION_MODE
  {1,IR(0, 0),IR(24199, 4)/* ALT */,IR(0, 0),IR(139, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_FPOPERATION_MODE
  {0,IR(0, 0),IR(16586, 5)/* None */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FP_FAST_MATH_MODE
  {1,IR(0, 0),IR(24203, 7)/* NotNaN */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FP_FAST_MATH_MODE
  {2,IR(0, 0),IR(24210, 7)/* NotInf */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FP_FAST_MATH_MODE
  {4,IR(0, 0),IR(24217, 4)/* NSZ */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FP_FAST_MATH_MODE
  {8,IR(0, 0),IR(24221, 11)/* AllowRecip */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FP_FAST_MATH_MODE
  {16,IR(0, 0),IR(24232, 5)/* Fast */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FP_FAST_MATH_MODE
  {65536,IR(0, 0),IR(24237, 14)/* AllowContract */,IR(134, 1),IR(190, 2),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_FP_FAST_MATH_MODE
  {131072,IR(0, 0),IR(24274, 13)/* AllowReassoc */,IR(135, 1),IR(190, 2),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_FP_FAST_MATH_MODE
  {262144,IR(0, 0),IR(24305, 15)/* AllowTransform */,IR(0, 0),IR(182, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_FP_FAST_MATH_MODE
  {0,IR(0, 0),IR(24320, 4)/* RTE */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FP_ROUNDING_MODE
  {1,IR(0, 0),IR(24324, 4)/* RTZ */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FP_ROUNDING_MODE
  {2,IR(0, 0),IR(24328, 4)/* RTP */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FP_ROUNDING_MODE
  {3,IR(0, 0),IR(24332, 4)/* RTN */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FP_ROUNDING_MODE
  {1,IR(0, 0),IR(24336, 16)/* Vertical2Pixels */,IR(0, 0),IR(38, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_FRAGMENT_SHADING_RATE
  {2,IR(0, 0),IR(24352, 16)/* Vertical4Pixels */,IR(0, 0),IR(38, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_FRAGMENT_SHADING_RATE
  {4,IR(0, 0),IR(24368, 18)/* Horizontal2Pixels */,IR(0, 0),IR(38, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_FRAGMENT_SHADING_RATE
  {8,IR(0, 0),IR(24386, 18)/* Horizontal4Pixels */,IR(0, 0),IR(38, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_FRAGMENT_SHADING_RATE
  {0,IR(0, 0),IR(16586, 5)/* None */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FUNCTION_CONTROL
  {1,IR(0, 0),IR(24404, 7)/* Inline */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FUNCTION_CONTROL
  {2,IR(0, 0),IR(24411, 11)/* DontInline */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FUNCTION_CONTROL
  {4,IR(0, 0),IR(24422, 5)/* Pure */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FUNCTION_CONTROL
  {8,IR(0, 0),IR(24427, 6)/* Const */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FUNCTION_CONTROL
  {65536,IR(0, 0),IR(15411, 11)/* OptNoneEXT */,IR(68, 1),IR(192, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_FUNCTION_CONTROL
  {0,IR(0, 0),IR(24433, 5)/* Zext */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE
  {1,IR(0, 0),IR(24438, 5)/* Sext */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE
  {2,IR(0, 0),IR(24443, 6)/* ByVal */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE
  {3,IR(0, 0),IR(24449, 5)/* Sret */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE
  {4,IR(0, 0),IR(24454, 8)/* NoAlias */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE
  {5,IR(0, 0),IR(24462, 10)/* NoCapture */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE
  {6,IR(0, 0),IR(24472, 8)/* NoWrite */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE
  {7,IR(0, 0),IR(24480, 12)/* NoReadWrite */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE
  {5940,IR(0, 0),IR(24492, 21)/* RuntimeAlignedALTERA */,IR(136, 1),IR(193, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE
  {0,IR(0, 0),IR(24533, 14)/* Gather4x1QCOM */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_GATHER_MODES
  {1,IR(0, 0),IR(24547, 12)/* GatherDQCOM */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_GATHER_MODES
  {2,IR(0, 0),IR(24559, 13)/* GatherH2QCOM */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_GATHER_MODES
  {3,IR(0, 0),IR(24572, 13)/* GatherV2QCOM */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_GATHER_MODES
  {0,IR(0, 0),IR(24585, 7)/* Reduce */,IR(0, 0),IR(194, 3),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_GROUP_OPERATION
  {1,IR(0, 0),IR(24592, 14)/* InclusiveScan */,IR(0, 0),IR(194, 3),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_GROUP_OPERATION
  {2,IR(0, 0),IR(24606, 14)/* ExclusiveScan */,IR(0, 0),IR(194, 3),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_GROUP_OPERATION
  {3,IR(0, 0),IR(24620, 16)/* ClusteredReduce */,IR(0, 0),IR(197, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_GROUP_OPERATION
  {6,IR(0, 0),IR(24636, 21)/* PartitionedReduceEXT */,IR(137, 1),IR(198, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_GROUP_OPERATION
  {7,IR(0, 0),IR(24677, 28)/* PartitionedInclusiveScanEXT */,IR(138, 1),IR(198, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_GROUP_OPERATION
  {8,IR(0, 0),IR(24732, 28)/* PartitionedExclusiveScanEXT */,IR(139, 1),IR(198, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_GROUP_OPERATION
  {0,IR(0, 0),IR(24787, 10)/* NoneINTEL */,IR(0, 0),IR(152, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_HOST_ACCESS_QUALIFIER
  {1,IR(0, 0),IR(24797, 10)/* ReadINTEL */,IR(0, 0),IR(152, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_HOST_ACCESS_QUALIFIER
  {2,IR(0, 0),IR(24807, 11)/* WriteINTEL */,IR(0, 0),IR(152, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_HOST_ACCESS_QUALIFIER
  {3,IR(0, 0),IR(24818, 15)/* ReadWriteINTEL */,IR(0, 0),IR(152, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_HOST_ACCESS_QUALIFIER
  {0,IR(0, 0),IR(16586, 5)/* None */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE
  {1,IR(10, 1),IR(24833, 5)/* Bias */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE
  {2,IR(10, 1),IR(24838, 4)/* Lod */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE
  {4,IR(36, 2),IR(24842, 5)/* Grad */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE
  {8,IR(10, 1),IR(24847, 12)/* ConstOffset */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE
  {16,IR(10, 1),IR(17993, 7)/* Offset */,IR(0, 0),IR(199, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE
  {32,IR(10, 1),IR(24859, 13)/* ConstOffsets */,IR(0, 0),IR(199, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE
  {64,IR(10, 1),IR(17790, 7)/* Sample */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE
  {128,IR(10, 1),IR(9638, 7)/* MinLod */,IR(0, 0),IR(200, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE
  {256,IR(4, 1),IR(24872, 19)/* MakeTexelAvailable */,IR(140, 1),IR(201, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE
  {512,IR(4, 1),IR(24913, 17)/* MakeTexelVisible */,IR(141, 1),IR(201, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE
  {1024,IR(0, 0),IR(24950, 16)/* NonPrivateTexel */,IR(142, 1),IR(201, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE
  {2048,IR(0, 0),IR(24985, 14)/* VolatileTexel */,IR(143, 1),IR(201, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE
  {4096,IR(0, 0),IR(25016, 11)/* SignExtend */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE
  {8192,IR(0, 0),IR(25027, 11)/* ZeroExtend */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE
  {16384,IR(0, 0),IR(25038, 12)/* Nontemporal */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,6),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE
  {65536,IR(10, 1),IR(25050, 8)/* Offsets */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE
  {0,IR(0, 0),IR(25058, 10)/* SnormInt8 */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {1,IR(0, 0),IR(25068, 11)/* SnormInt16 */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {2,IR(0, 0),IR(25079, 10)/* UnormInt8 */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {3,IR(0, 0),IR(25089, 11)/* UnormInt16 */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {4,IR(0, 0),IR(25100, 14)/* UnormShort565 */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {5,IR(0, 0),IR(25114, 14)/* UnormShort555 */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {6,IR(0, 0),IR(25128, 15)/* UnormInt101010 */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {7,IR(0, 0),IR(25143, 11)/* SignedInt8 */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {8,IR(0, 0),IR(25154, 12)/* SignedInt16 */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {9,IR(0, 0),IR(25166, 12)/* SignedInt32 */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {10,IR(0, 0),IR(25178, 13)/* UnsignedInt8 */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {11,IR(0, 0),IR(25191, 14)/* UnsignedInt16 */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {12,IR(0, 0),IR(25205, 14)/* UnsignedInt32 */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {13,IR(0, 0),IR(25219, 10)/* HalfFloat */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {14,IR(0, 0),IR(16483, 6)/* Float */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {15,IR(0, 0),IR(25229, 11)/* UnormInt24 */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {16,IR(0, 0),IR(25240, 17)/* UnormInt101010_2 */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {17,IR(0, 0),IR(25257, 16)/* UnormInt10X6EXT */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {19,IR(0, 0),IR(25273, 20)/* UnsignedIntRaw10EXT */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {20,IR(0, 0),IR(25293, 20)/* UnsignedIntRaw12EXT */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {21,IR(0, 0),IR(25313, 20)/* UnormInt2_101010EXT */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {22,IR(0, 0),IR(25333, 19)/* UnsignedInt10X6EXT */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {23,IR(0, 0),IR(25352, 19)/* UnsignedInt12X4EXT */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {24,IR(0, 0),IR(25371, 19)/* UnsignedInt14X2EXT */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {25,IR(0, 0),IR(25390, 16)/* UnormInt12X4EXT */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {26,IR(0, 0),IR(25406, 16)/* UnormInt14X2EXT */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {0,IR(0, 0),IR(25422, 2)/* R */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {1,IR(0, 0),IR(25424, 2)/* A */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {2,IR(0, 0),IR(25426, 3)/* RG */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {3,IR(0, 0),IR(25429, 3)/* RA */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {4,IR(0, 0),IR(25432, 4)/* RGB */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {5,IR(0, 0),IR(25436, 5)/* RGBA */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {6,IR(0, 0),IR(25441, 5)/* BGRA */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {7,IR(0, 0),IR(25446, 5)/* ARGB */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {8,IR(0, 0),IR(25451, 10)/* Intensity */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {9,IR(0, 0),IR(25461, 10)/* Luminance */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {10,IR(0, 0),IR(25471, 3)/* Rx */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {11,IR(0, 0),IR(25474, 4)/* RGx */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {12,IR(0, 0),IR(25478, 5)/* RGBx */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {13,IR(0, 0),IR(25483, 6)/* Depth */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {14,IR(0, 0),IR(25489, 13)/* DepthStencil */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {15,IR(0, 0),IR(25502, 5)/* sRGB */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {16,IR(0, 0),IR(25507, 6)/* sRGBx */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {17,IR(0, 0),IR(25513, 6)/* sRGBA */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {18,IR(0, 0),IR(25519, 6)/* sBGRA */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {19,IR(0, 0),IR(25525, 5)/* ABGR */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {0,IR(0, 0),IR(25530, 28)/* InitOnDeviceReprogramALTERA */,IR(144, 1),IR(153, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_INITIALIZATION_MODE_QUALIFIER
  {1,IR(0, 0),IR(25585, 24)/* InitOnDeviceResetALTERA */,IR(145, 1),IR(153, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_INITIALIZATION_MODE_QUALIFIER
  {0,IR(0, 0),IR(25632, 7)/* NoWait */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_KERNEL_ENQ_FLAGS
  {1,IR(0, 0),IR(25639, 11)/* WaitKernel */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_KERNEL_ENQ_FLAGS
  {2,IR(0, 0),IR(25650, 14)/* WaitWorkGroup */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_KERNEL_ENQ_FLAGS
  {0,IR(0, 0),IR(16586, 5)/* None */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_KERNEL_PROFILING_INFO
  {1,IR(0, 0),IR(25664, 12)/* CmdExecTime */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_KERNEL_PROFILING_INFO
  {0,IR(0, 0),IR(16586, 5)/* None */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_KERNEL_PROPERTY_FLAGS
  {1,IR(0, 0),IR(25676, 13)/* MayUsePrintf */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_KERNEL_PROPERTY_FLAGS
  {0,IR(0, 0),IR(25689, 7)/* Export */,IR(0, 0),IR(94, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_LINKAGE_TYPE
  {1,IR(0, 0),IR(25696, 7)/* Import */,IR(0, 0),IR(94, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_LINKAGE_TYPE
  {2,IR(0, 0),IR(25703, 12)/* LinkOnceODR */,IR(0, 0),IR(94, 1),IR(205, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_LINKAGE_TYPE
  {3,IR(0, 0),IR(25715, 8)/* WeakAMD */,IR(0, 0),IR(202, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_LINKAGE_TYPE
  {0,IR(0, 0),IR(25723, 14)/* UncachedINTEL */,IR(0, 0),IR(154, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_LOAD_CACHE_CONTROL
  {1,IR(0, 0),IR(25737, 12)/* CachedINTEL */,IR(0, 0),IR(154, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_LOAD_CACHE_CONTROL
  {2,IR(0, 0),IR(25749, 15)/* StreamingINTEL */,IR(0, 0),IR(154, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_LOAD_CACHE_CONTROL
  {3,IR(0, 0),IR(25764, 25)/* InvalidateAfterReadINTEL */,IR(0, 0),IR(154, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_LOAD_CACHE_CONTROL
  {4,IR(0, 0),IR(25789, 17)/* ConstCachedINTEL */,IR(0, 0),IR(154, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_LOAD_CACHE_CONTROL
  {0,IR(0, 0),IR(16586, 5)/* None */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_LOOP_CONTROL
  {1,IR(0, 0),IR(25806, 7)/* Unroll */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_LOOP_CONTROL
  {2,IR(0, 0),IR(25813, 11)/* DontUnroll */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_LOOP_CONTROL
  {4,IR(0, 0),IR(25824, 19)/* DependencyInfinite */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,1),0xffffffffu}, // SPV_OPERAND_TYPE_LOOP_CONTROL
  {8,IR(0, 1),IR(25843, 17)/* DependencyLength */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,1),0xffffffffu}, // SPV_OPERAND_TYPE_LOOP_CONTROL
  {16,IR(0, 1),IR(25860, 14)/* MinIterations */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu}, // SPV_OPERAND_TYPE_LOOP_CONTROL
  {32,IR(0, 1),IR(25874, 14)/* MaxIterations */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu}, // SPV_OPERAND_TYPE_LOOP_CONTROL
  {64,IR(0, 1),IR(25888, 18)/* IterationMultiple */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu}, // SPV_OPERAND_TYPE_LOOP_CONTROL
  {128,IR(0, 1),IR(25906, 10)/* PeelCount */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu}, // SPV_OPERAND_TYPE_LOOP_CONTROL
  {256,IR(0, 1),IR(25916, 13)/* PartialCount */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu}, // SPV_OPERAND_TYPE_LOOP_CONTROL
  {65536,IR(0, 1),IR(20356, 25)/* InitiationIntervalALTERA */,IR(104, 1),IR(203, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_LOOP_CONTROL
  {131072,IR(0, 1),IR(20405, 21)/* MaxConcurrencyALTERA */,IR(105, 1),IR(203, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_LOOP_CONTROL
  {262144,IR(0, 1),IR(25929, 22)/* DependencyArrayALTERA */,IR(146, 1),IR(203, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_LOOP_CONTROL
  {524288,IR(0, 1),IR(20446, 21)/* PipelineEnableALTERA */,IR(106, 1),IR(203, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_LOOP_CONTROL
  {1048576,IR(0, 1),IR(25972, 19)/* LoopCoalesceALTERA */,IR(147, 1),IR(203, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_LOOP_CONTROL
  {2097152,IR(0, 1),IR(26009, 22)/* MaxInterleavingALTERA */,IR(148, 1),IR(203, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_LOOP_CONTROL
  {4194304,IR(0, 1),IR(26052, 27)/* SpeculatedIterationsALTERA */,IR(149, 1),IR(203, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_LOOP_CONTROL
  {8388608,IR(0, 0),IR(26105, 15)/* NoFusionALTERA */,IR(150, 1),IR(203, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_LOOP_CONTROL
  {16777216,IR(0, 1),IR(26134, 16)/* LoopCountALTERA */,IR(151, 1),IR(203, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_LOOP_CONTROL
  {33554432,IR(0, 1),IR(26165, 27)/* MaxReinvocationDelayALTERA */,IR(152, 1),IR(203, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_LOOP_CONTROL
  {268435456,IR(0, 1),IR(11175, 23)/* MultipleWaitQueuesQCOM */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_LOOP_CONTROL
  {0,IR(0, 0),IR(16586, 5)/* None */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {1,IR(0, 0),IR(26218, 29)/* MatrixASignedComponentsINTEL */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {2,IR(0, 0),IR(26247, 29)/* MatrixBSignedComponentsINTEL */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {4,IR(0, 0),IR(26276, 21)/* MatrixCBFloat16INTEL */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {8,IR(0, 0),IR(26297, 26)/* MatrixResultBFloat16INTEL */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {16,IR(0, 0),IR(26323, 23)/* MatrixAPackedInt8INTEL */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {32,IR(0, 0),IR(26346, 23)/* MatrixBPackedInt8INTEL */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {64,IR(0, 0),IR(26369, 23)/* MatrixAPackedInt4INTEL */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {128,IR(0, 0),IR(26392, 23)/* MatrixBPackedInt4INTEL */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {256,IR(0, 0),IR(26415, 17)/* MatrixATF32INTEL */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {512,IR(0, 0),IR(26432, 17)/* MatrixBTF32INTEL */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {1024,IR(0, 0),IR(26449, 26)/* MatrixAPackedFloat16INTEL */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {2048,IR(0, 0),IR(26475, 26)/* MatrixBPackedFloat16INTEL */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {4096,IR(0, 0),IR(26501, 27)/* MatrixAPackedBFloat16INTEL */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {8192,IR(0, 0),IR(26528, 27)/* MatrixBPackedBFloat16INTEL */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {0,IR(0, 0),IR(16586, 5)/* None */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_ACCESS
  {1,IR(0, 0),IR(17824, 9)/* Volatile */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_ACCESS
  {2,IR(0, 1),IR(26555, 8)/* Aligned */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_ACCESS
  {4,IR(0, 0),IR(25038, 12)/* Nontemporal */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_ACCESS
  {8,IR(4, 1),IR(26563, 21)/* MakePointerAvailable */,IR(153, 1),IR(201, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_ACCESS
  {16,IR(4, 1),IR(26608, 19)/* MakePointerVisible */,IR(154, 1),IR(201, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_ACCESS
  {32,IR(0, 0),IR(26649, 18)/* NonPrivatePointer */,IR(155, 1),IR(201, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_ACCESS
  {65536,IR(10, 1),IR(26688, 20)/* AliasScopeINTELMask */,IR(0, 0),IR(144, 1),IR(136, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_ACCESS
  {131072,IR(10, 1),IR(26708, 17)/* NoAliasINTELMask */,IR(0, 0),IR(144, 1),IR(136, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_ACCESS
  {0,IR(0, 0),IR(26725, 7)/* Simple */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_MODEL
  {1,IR(0, 0),IR(26732, 8)/* GLSL450 */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_MODEL
  {2,IR(0, 0),IR(26740, 7)/* OpenCL */,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_MODEL
  {3,IR(0, 0),IR(26747, 7)/* Vulkan */,IR(156, 1),IR(201, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_MODEL
  {0,IR(0, 0),IR(26764, 8)/* Relaxed */,IR(157, 1),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {2,IR(0, 0),IR(26772, 8)/* Acquire */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {4,IR(0, 0),IR(26780, 8)/* Release */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {8,IR(0, 0),IR(26788, 15)/* AcquireRelease */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {16,IR(0, 0),IR(26803, 23)/* SequentiallyConsistent */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {64,IR(0, 0),IR(26826, 14)/* UniformMemory */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {128,IR(0, 0),IR(26840, 15)/* SubgroupMemory */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {256,IR(0, 0),IR(26855, 16)/* WorkgroupMemory */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {512,IR(0, 0),IR(26871, 21)/* CrossWorkgroupMemory */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {1024,IR(0, 0),IR(26892, 20)/* AtomicCounterMemory */,IR(0, 0),IR(83, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {2048,IR(0, 0),IR(26912, 12)/* ImageMemory */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {4096,IR(0, 0),IR(26924, 13)/* OutputMemory */,IR(158, 1),IR(201, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {8192,IR(0, 0),IR(26953, 14)/* MakeAvailable */,IR(159, 1),IR(201, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {16384,IR(0, 0),IR(26984, 12)/* MakeVisible */,IR(160, 1),IR(201, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {32768,IR(0, 0),IR(17824, 9)/* Volatile */,IR(0, 0),IR(201, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {0,IR(0, 0),IR(27011, 10)/* AutoINTEL */,IR(0, 0),IR(185, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_NAMED_MAXIMUM_NUMBER_OF_REGISTERS
  {0,IR(0, 0),IR(27021, 5)/* WRAP */,IR(0, 0),IR(204, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_OVERFLOW_MODES
  {1,IR(0, 0),IR(27026, 4)/* SAT */,IR(0, 0),IR(204, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_OVERFLOW_MODES
  {2,IR(0, 0),IR(27030, 9)/* SAT_ZERO */,IR(0, 0),IR(204, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_OVERFLOW_MODES
  {3,IR(0, 0),IR(27039, 8)/* SAT_SYM */,IR(0, 0),IR(204, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_OVERFLOW_MODES
  {0,IR(0, 0),IR(27047, 25)/* PackedVectorFormat4x8Bit */,IR(161, 1),IR(1, 0),IR(153, 1),SPV_SPIRV_VERSION_WORD(1,6),0xffffffffu}, // SPV_OPERAND_TYPE_PACKED_VECTOR_FORMAT
  {0,IR(0, 0),IR(27100, 4)/* TRN */,IR(0, 0),IR(204, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_QUANTIZATION_MODES
  {1,IR(0, 0),IR(27104, 9)/* TRN_ZERO */,IR(0, 0),IR(204, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_QUANTIZATION_MODES
  {2,IR(0, 0),IR(27113, 4)/* RND */,IR(0, 0),IR(204, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_QUANTIZATION_MODES
  {3,IR(0, 0),IR(27117, 9)/* RND_ZERO */,IR(0, 0),IR(204, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_QUANTIZATION_MODES
  {4,IR(0, 0),IR(27126, 8)/* RND_INF */,IR(0, 0),IR(204, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_QUANTIZATION_MODES
  {5,IR(0, 0),IR(27134, 12)/* RND_MIN_INF */,IR(0, 0),IR(204, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_QUANTIZATION_MODES
  {6,IR(0, 0),IR(27146, 9)/* RND_CONV */,IR(0, 0),IR(204, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_QUANTIZATION_MODES
  {7,IR(0, 0),IR(27155, 13)/* RND_CONV_ODD */,IR(0, 0),IR(204, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_QUANTIZATION_MODES
  {0,IR(0, 0),IR(16586, 5)/* None */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_RAW_ACCESS_CHAIN_OPERANDS
  {1,IR(0, 0),IR(27168, 25)/* RobustnessPerComponentNV */,IR(0, 0),IR(205, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_RAW_ACCESS_CHAIN_OPERANDS
  {2,IR(0, 0),IR(27193, 23)/* RobustnessPerElementNV */,IR(0, 0),IR(205, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_RAW_ACCESS_CHAIN_OPERANDS
  {0,IR(0, 0),IR(17338, 8)/* NoneKHR */,IR(0, 0),IR(85, 2),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_RAY_FLAGS
  {1,IR(0, 0),IR(27216, 10)/* OpaqueKHR */,IR(0, 0),IR(85, 2),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_RAY_FLAGS
  {2,IR(0, 0),IR(27226, 12)/* NoOpaqueKHR */,IR(0, 0),IR(85, 2),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_RAY_FLAGS
  {4,IR(0, 0),IR(27238, 23)/* TerminateOnFirstHitKHR */,IR(0, 0),IR(85, 2),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_RAY_FLAGS
  {8,IR(0, 0),IR(27261, 24)/* SkipClosestHitShaderKHR */,IR(0, 0),IR(85, 2),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_RAY_FLAGS
  {16,IR(0, 0),IR(27285, 27)/* CullBackFacingTrianglesKHR */,IR(0, 0),IR(85, 2),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_RAY_FLAGS
  {32,IR(0, 0),IR(27312, 28)/* CullFrontFacingTrianglesKHR */,IR(0, 0),IR(85, 2),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_RAY_FLAGS
  {64,IR(0, 0),IR(27340, 14)/* CullOpaqueKHR */,IR(0, 0),IR(85, 2),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_RAY_FLAGS
  {128,IR(0, 0),IR(27354, 16)/* CullNoOpaqueKHR */,IR(0, 0),IR(85, 2),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_RAY_FLAGS
  {256,IR(0, 0),IR(27370, 17)/* SkipTrianglesKHR */,IR(162, 1),IR(206, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_RAY_FLAGS
  {512,IR(0, 0),IR(27411, 13)/* SkipAABBsKHR */,IR(0, 0),IR(206, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_RAY_FLAGS
  {1024,IR(0, 0),IR(27424, 30)/* ForceOpacityMicromap2StateKHR */,IR(163, 1),IR(207, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_RAY_FLAGS
  {0,IR(0, 0),IR(27484, 41)/* RayQueryCandidateIntersectionTriangleKHR */,IR(0, 0),IR(208, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_RAY_QUERY_CANDIDATE_INTERSECTION_TYPE
  {1,IR(0, 0),IR(27525, 37)/* RayQueryCandidateIntersectionAABBKHR */,IR(0, 0),IR(208, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_RAY_QUERY_CANDIDATE_INTERSECTION_TYPE
  {0,IR(0, 0),IR(27562, 37)/* RayQueryCommittedIntersectionNoneKHR */,IR(0, 0),IR(208, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_RAY_QUERY_COMMITTED_INTERSECTION_TYPE
  {1,IR(0, 0),IR(27599, 41)/* RayQueryCommittedIntersectionTriangleKHR */,IR(0, 0),IR(208, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_RAY_QUERY_COMMITTED_INTERSECTION_TYPE
  {2,IR(0, 0),IR(27640, 42)/* RayQueryCommittedIntersectionGeneratedKHR */,IR(0, 0),IR(208, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_RAY_QUERY_COMMITTED_INTERSECTION_TYPE
  {0,IR(0, 0),IR(27682, 33)/* RayQueryCandidateIntersectionKHR */,IR(0, 0),IR(208, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_RAY_QUERY_INTERSECTION
  {1,IR(0, 0),IR(27715, 33)/* RayQueryCommittedIntersectionKHR */,IR(0, 0),IR(208, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_RAY_QUERY_INTERSECTION
  {0,IR(0, 0),IR(16586, 5)/* None */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_ADDRESSING_MODE
  {1,IR(0, 0),IR(27748, 12)/* ClampToEdge */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_ADDRESSING_MODE
  {2,IR(0, 0),IR(27760, 6)/* Clamp */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_ADDRESSING_MODE
  {3,IR(0, 0),IR(27766, 7)/* Repeat */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_ADDRESSING_MODE
  {4,IR(0, 0),IR(27773, 15)/* RepeatMirrored */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_ADDRESSING_MODE
  {0,IR(0, 0),IR(27788, 8)/* Nearest */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_FILTER_MODE
  {1,IR(0, 0),IR(27796, 7)/* Linear */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_FILTER_MODE
  {0,IR(0, 0),IR(27803, 8)/* Unknown */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {1,IR(0, 0),IR(27811, 8)/* Rgba32f */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {2,IR(0, 0),IR(27819, 8)/* Rgba16f */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {3,IR(0, 0),IR(27827, 5)/* R32f */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {4,IR(0, 0),IR(27832, 6)/* Rgba8 */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {5,IR(0, 0),IR(27838, 11)/* Rgba8Snorm */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {6,IR(0, 0),IR(27849, 6)/* Rg32f */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {7,IR(0, 0),IR(27855, 6)/* Rg16f */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {8,IR(0, 0),IR(27861, 13)/* R11fG11fB10f */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {9,IR(0, 0),IR(27874, 5)/* R16f */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {10,IR(0, 0),IR(27879, 7)/* Rgba16 */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {11,IR(0, 0),IR(27886, 8)/* Rgb10A2 */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {12,IR(0, 0),IR(27894, 5)/* Rg16 */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {13,IR(0, 0),IR(27899, 4)/* Rg8 */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {14,IR(0, 0),IR(27903, 4)/* R16 */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {15,IR(0, 0),IR(27907, 3)/* R8 */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {16,IR(0, 0),IR(27910, 12)/* Rgba16Snorm */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {17,IR(0, 0),IR(27922, 10)/* Rg16Snorm */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {18,IR(0, 0),IR(27932, 9)/* Rg8Snorm */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {19,IR(0, 0),IR(27941, 9)/* R16Snorm */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {20,IR(0, 0),IR(27950, 8)/* R8Snorm */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {21,IR(0, 0),IR(27958, 8)/* Rgba32i */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {22,IR(0, 0),IR(27966, 8)/* Rgba16i */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {23,IR(0, 0),IR(27974, 7)/* Rgba8i */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {24,IR(0, 0),IR(27981, 5)/* R32i */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {25,IR(0, 0),IR(27986, 6)/* Rg32i */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {26,IR(0, 0),IR(27992, 6)/* Rg16i */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {27,IR(0, 0),IR(27998, 5)/* Rg8i */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {28,IR(0, 0),IR(28003, 5)/* R16i */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {29,IR(0, 0),IR(28008, 4)/* R8i */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {30,IR(0, 0),IR(28012, 9)/* Rgba32ui */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {31,IR(0, 0),IR(28021, 9)/* Rgba16ui */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {32,IR(0, 0),IR(28030, 8)/* Rgba8ui */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {33,IR(0, 0),IR(28038, 6)/* R32ui */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {34,IR(0, 0),IR(28044, 10)/* Rgb10a2ui */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {35,IR(0, 0),IR(28054, 7)/* Rg32ui */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {36,IR(0, 0),IR(28061, 7)/* Rg16ui */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {37,IR(0, 0),IR(28068, 6)/* Rg8ui */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {38,IR(0, 0),IR(28074, 6)/* R16ui */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {39,IR(0, 0),IR(28080, 5)/* R8ui */,IR(0, 0),IR(209, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {40,IR(0, 0),IR(28085, 6)/* R64ui */,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {41,IR(0, 0),IR(28091, 5)/* R64i */,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {0,IR(0, 0),IR(28096, 12)/* CrossDevice */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SCOPE_ID
  {1,IR(0, 0),IR(28108, 7)/* Device */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SCOPE_ID
  {2,IR(0, 0),IR(28115, 10)/* Workgroup */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SCOPE_ID
  {3,IR(0, 0),IR(28125, 9)/* Subgroup */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SCOPE_ID
  {4,IR(0, 0),IR(28134, 11)/* Invocation */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SCOPE_ID
  {5,IR(0, 0),IR(28145, 12)/* QueueFamily */,IR(164, 1),IR(201, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_SCOPE_ID
  {6,IR(0, 0),IR(28172, 14)/* ShaderCallKHR */,IR(0, 0),IR(61, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_SCOPE_ID
  {0,IR(0, 0),IR(16586, 5)/* None */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SELECTION_CONTROL
  {1,IR(0, 0),IR(28186, 8)/* Flatten */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SELECTION_CONTROL
  {2,IR(0, 0),IR(28194, 12)/* DontFlatten */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SELECTION_CONTROL
  {1,IR(0, 0),IR(28206, 29)/* IdentifierPossibleDuplicates */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_BUILD_IDENTIFIER_FLAGS
  {0,IR(0, 0),IR(16455, 12)/* Unspecified */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {1,IR(0, 0),IR(16467, 8)/* Address */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {2,IR(0, 0),IR(16475, 8)/* Boolean */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {3,IR(0, 0),IR(16483, 6)/* Float */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {4,IR(0, 0),IR(16489, 7)/* Signed */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {5,IR(0, 0),IR(16496, 11)/* SignedChar */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {6,IR(0, 0),IR(16507, 9)/* Unsigned */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {7,IR(0, 0),IR(16516, 13)/* UnsignedChar */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {0,IR(0, 0),IR(16529, 6)/* Class */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_COMPOSITE_TYPE
  {1,IR(0, 0),IR(16535, 10)/* Structure */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_COMPOSITE_TYPE
  {2,IR(0, 0),IR(16545, 6)/* Union */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_COMPOSITE_TYPE
  {0,IR(0, 0),IR(16551, 15)/* ImportedModule */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_IMPORTED_ENTITY
  {1,IR(0, 0),IR(16566, 20)/* ImportedDeclaration */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_IMPORTED_ENTITY
  {0,IR(0, 0),IR(16586, 5)/* None */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {1,IR(0, 0),IR(16591, 16)/* FlagIsProtected */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {2,IR(0, 0),IR(16607, 14)/* FlagIsPrivate */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {3,IR(0, 0),IR(16621, 13)/* FlagIsPublic */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {4,IR(0, 0),IR(16634, 12)/* FlagIsLocal */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {8,IR(0, 0),IR(16646, 17)/* FlagIsDefinition */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {16,IR(0, 0),IR(16663, 12)/* FlagFwdDecl */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {32,IR(0, 0),IR(16675, 15)/* FlagArtificial */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {64,IR(0, 0),IR(16690, 13)/* FlagExplicit */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {128,IR(0, 0),IR(16703, 15)/* FlagPrototyped */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {256,IR(0, 0),IR(16718, 18)/* FlagObjectPointer */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {512,IR(0, 0),IR(16736, 17)/* FlagStaticMember */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {1024,IR(0, 0),IR(16753, 21)/* FlagIndirectVariable */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {2048,IR(0, 0),IR(16774, 20)/* FlagLValueReference */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {4096,IR(0, 0),IR(16794, 20)/* FlagRValueReference */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {8192,IR(0, 0),IR(16814, 16)/* FlagIsOptimized */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {16384,IR(0, 0),IR(16830, 16)/* FlagIsEnumClass */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {32768,IR(0, 0),IR(16846, 20)/* FlagTypePassByValue */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {65536,IR(0, 0),IR(16866, 24)/* FlagTypePassByReference */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {131072,IR(0, 0),IR(28235, 26)/* FlagUnknownPhysicalLayout */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {0,IR(0, 0),IR(16890, 6)/* Deref */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION
  {1,IR(0, 0),IR(16896, 5)/* Plus */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION
  {2,IR(0, 0),IR(16901, 6)/* Minus */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION
  {3,IR(10, 1),IR(16940, 11)/* PlusUconst */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION
  {4,IR(36, 2),IR(16951, 9)/* BitPiece */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION
  {5,IR(0, 0),IR(16960, 5)/* Swap */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION
  {6,IR(0, 0),IR(16965, 7)/* Xderef */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION
  {7,IR(0, 0),IR(16972, 11)/* StackValue */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION
  {8,IR(10, 1),IR(16983, 7)/* Constu */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION
  {9,IR(36, 2),IR(16990, 9)/* Fragment */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION
  {0,IR(0, 0),IR(16999, 10)/* ConstType */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_TYPE_QUALIFIER
  {1,IR(0, 0),IR(17009, 13)/* VolatileType */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_TYPE_QUALIFIER
  {2,IR(0, 0),IR(17022, 13)/* RestrictType */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_TYPE_QUALIFIER
  {3,IR(0, 0),IR(17035, 11)/* AtomicType */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu}, // SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_TYPE_QUALIFIER
  {0,IR(0, 0),IR(27803, 8)/* Unknown */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {1,IR(0, 0),IR(28261, 5)/* ESSL */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {2,IR(0, 0),IR(28266, 5)/* GLSL */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {3,IR(0, 0),IR(28271, 9)/* OpenCL_C */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {4,IR(0, 0),IR(28280, 11)/* OpenCL_CPP */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {5,IR(0, 0),IR(28291, 5)/* HLSL */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {6,IR(0, 0),IR(28296, 15)/* CPP_for_OpenCL */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {7,IR(0, 0),IR(28311, 5)/* SYCL */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {8,IR(0, 0),IR(28316, 7)/* HERO_C */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {9,IR(0, 0),IR(28323, 5)/* NZSL */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {10,IR(0, 0),IR(28328, 5)/* WGSL */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {11,IR(0, 0),IR(28333, 6)/* Slang */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {12,IR(0, 0),IR(28339, 4)/* Zig */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {13,IR(0, 0),IR(28343, 5)/* Rust */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {14,IR(0, 0),IR(28348, 5)/* Pred */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {15,IR(0, 0),IR(28353, 9)/* ApilaJai */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {0,IR(0, 0),IR(28362, 16)/* UniformConstant */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {1,IR(0, 0),IR(28378, 6)/* Input */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {2,IR(0, 0),IR(17875, 8)/* Uniform */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {3,IR(0, 0),IR(28384, 7)/* Output */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {4,IR(0, 0),IR(28115, 10)/* Workgroup */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {5,IR(0, 0),IR(28391, 15)/* CrossWorkgroup */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {6,IR(0, 0),IR(28406, 8)/* Private */,IR(0, 0),IR(211, 2),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {7,IR(0, 0),IR(28414, 9)/* Function */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {8,IR(0, 0),IR(28423, 8)/* Generic */,IR(0, 0),IR(213, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {9,IR(0, 0),IR(28431, 13)/* PushConstant */,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {10,IR(0, 0),IR(28444, 14)/* AtomicCounter */,IR(0, 0),IR(83, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {11,IR(0, 0),IR(28458, 6)/* Image */,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {12,IR(0, 0),IR(28464, 14)/* StorageBuffer */,IR(0, 0),IR(3, 1),IR(206, 2),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {4172,IR(0, 0),IR(28478, 13)/* TileImageEXT */,IR(0, 0),IR(155, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {4491,IR(0, 0),IR(28491, 19)/* TileAttachmentQCOM */,IR(0, 0),IR(41, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {5068,IR(0, 0),IR(28510, 16)/* NodePayloadAMDX */,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {5328,IR(0, 0),IR(28526, 16)/* CallableDataKHR */,IR(165, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {5329,IR(0, 0),IR(28557, 24)/* IncomingCallableDataKHR */,IR(166, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {5338,IR(0, 0),IR(28604, 14)/* RayPayloadKHR */,IR(167, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {5339,IR(0, 0),IR(28631, 16)/* HitAttributeKHR */,IR(168, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {5342,IR(0, 0),IR(28662, 22)/* IncomingRayPayloadKHR */,IR(169, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {5343,IR(0, 0),IR(28705, 22)/* ShaderRecordBufferKHR */,IR(170, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {5349,IR(0, 0),IR(28748, 22)/* PhysicalStorageBuffer */,IR(171, 1),IR(2, 1),IR(0, 2),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {5385,IR(0, 0),IR(28795, 21)/* HitObjectAttributeNV */,IR(0, 0),IR(133, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {5402,IR(0, 0),IR(28816, 24)/* TaskPayloadWorkgroupEXT */,IR(0, 0),IR(54, 1),IR(23, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {5411,IR(0, 0),IR(28840, 22)/* HitObjectAttributeEXT */,IR(0, 0),IR(134, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {5605,IR(0, 0),IR(28862, 17)/* CodeSectionINTEL */,IR(0, 0),IR(214, 1),IR(110, 1),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {5936,IR(0, 0),IR(28879, 17)/* DeviceOnlyALTERA */,IR(172, 1),IR(215, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {5937,IR(0, 0),IR(28912, 15)/* HostOnlyALTERA */,IR(173, 1),IR(215, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_STORAGE_CLASS
  {0,IR(0, 0),IR(25723, 14)/* UncachedINTEL */,IR(0, 0),IR(154, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_STORE_CACHE_CONTROL
  {1,IR(0, 0),IR(28941, 18)/* WriteThroughINTEL */,IR(0, 0),IR(154, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_STORE_CACHE_CONTROL
  {2,IR(0, 0),IR(28959, 15)/* WriteBackINTEL */,IR(0, 0),IR(154, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_STORE_CACHE_CONTROL
  {3,IR(0, 0),IR(25749, 15)/* StreamingINTEL */,IR(0, 0),IR(154, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_STORE_CACHE_CONTROL
  {0,IR(0, 0),IR(16586, 5)/* None */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_TENSOR_ADDRESSING_OPERANDS
  {1,IR(10, 1),IR(28974, 11)/* TensorView */,IR(0, 0),IR(216, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_TENSOR_ADDRESSING_OPERANDS
  {2,IR(10, 1),IR(28985, 11)/* DecodeFunc */,IR(0, 0),IR(107, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_TENSOR_ADDRESSING_OPERANDS
  {4,IR(10, 1),IR(28996, 17)/* DecodeVectorFunc */,IR(0, 0),IR(217, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_TENSOR_ADDRESSING_OPERANDS
  {0,IR(0, 0),IR(29013, 10)/* Undefined */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_TENSOR_CLAMP_MODE
  {1,IR(0, 0),IR(17833, 9)/* Constant */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_TENSOR_CLAMP_MODE
  {2,IR(0, 0),IR(27748, 12)/* ClampToEdge */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_TENSOR_CLAMP_MODE
  {3,IR(0, 0),IR(27766, 7)/* Repeat */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_TENSOR_CLAMP_MODE
  {4,IR(0, 0),IR(27773, 15)/* RepeatMirrored */,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_TENSOR_CLAMP_MODE
  {0,IR(0, 0),IR(29023, 8)/* NoneARM */,IR(0, 0),IR(218, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_TENSOR_OPERANDS
  {1,IR(0, 0),IR(29031, 15)/* NontemporalARM */,IR(0, 0),IR(218, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_TENSOR_OPERANDS
  {2,IR(10, 1),IR(29046, 20)/* OutOfBoundsValueARM */,IR(0, 0),IR(218, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_TENSOR_OPERANDS
  {4,IR(10, 1),IR(29066, 24)/* MakeElementAvailableARM */,IR(0, 0),IR(218, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_TENSOR_OPERANDS
  {8,IR(10, 1),IR(29090, 22)/* MakeElementVisibleARM */,IR(0, 0),IR(218, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_TENSOR_OPERANDS
  {16,IR(0, 0),IR(29112, 21)/* NonPrivateElementARM */,IR(0, 0),IR(218, 1),IR(0, 0),0xffffffffu,0xffffffffu}, // SPV_OPERAND_TYPE_TENSOR_OPERANDS
}};

// Maps an operand kind to possible operands for that kind.
// The result is an IndexRange into kOperandsByValue, and the operands
// are sorted by value within that span.
// An optional variant of a kind maps to the details for the corresponding
// concrete operand kind.
IndexRange OperandByValueRangeForKind(spv_operand_type_t type) {
  switch(type) {
    case SPV_OPERAND_TYPE_ACCESS_QUALIFIER: return IR(0, 3);
    case SPV_OPERAND_TYPE_ADDRESSING_MODEL: return IR(3, 4);
    case SPV_OPERAND_TYPE_BUILT_IN: return IR(7, 128);
    case SPV_OPERAND_TYPE_CAPABILITY: return IR(135, 300);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING: return IR(435, 8);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_COMPOSITE_TYPE: return IR(443, 3);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_IMPORTED_ENTITY: return IR(446, 2);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS: return IR(448, 19);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION: return IR(467, 10);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_TYPE_QUALIFIER: return IR(477, 4);
    case SPV_OPERAND_TYPE_COMPONENT_TYPE: return IR(481, 15);
    case SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_LAYOUT: return IR(496, 4);
    case SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_OPERANDS: return IR(500, 6);
    case SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_REDUCE: return IR(506, 3);
    case SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_USE: return IR(509, 3);
    case SPV_OPERAND_TYPE_COOPERATIVE_VECTOR_MATRIX_LAYOUT: return IR(512, 4);
    case SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING: return IR(516, 8);
    case SPV_OPERAND_TYPE_DEBUG_COMPOSITE_TYPE: return IR(524, 3);
    case SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS: return IR(527, 16);
    case SPV_OPERAND_TYPE_DEBUG_OPERATION: return IR(543, 9);
    case SPV_OPERAND_TYPE_DEBUG_TYPE_QUALIFIER: return IR(552, 3);
    case SPV_OPERAND_TYPE_DECORATION: return IR(555, 150);
    case SPV_OPERAND_TYPE_DIMENSIONALITY: return IR(705, 8);
    case SPV_OPERAND_TYPE_EXECUTION_MODE: return IR(713, 99);
    case SPV_OPERAND_TYPE_EXECUTION_MODEL: return IR(812, 17);
    case SPV_OPERAND_TYPE_FPDENORM_MODE: return IR(829, 2);
    case SPV_OPERAND_TYPE_FPENCODING: return IR(831, 8);
    case SPV_OPERAND_TYPE_FPOPERATION_MODE: return IR(839, 2);
    case SPV_OPERAND_TYPE_FP_FAST_MATH_MODE: return IR(841, 9);
    case SPV_OPERAND_TYPE_FP_ROUNDING_MODE: return IR(850, 4);
    case SPV_OPERAND_TYPE_FRAGMENT_SHADING_RATE: return IR(854, 4);
    case SPV_OPERAND_TYPE_FUNCTION_CONTROL: return IR(858, 6);
    case SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE: return IR(864, 9);
    case SPV_OPERAND_TYPE_GATHER_MODES: return IR(873, 4);
    case SPV_OPERAND_TYPE_GROUP_OPERATION: return IR(877, 7);
    case SPV_OPERAND_TYPE_HOST_ACCESS_QUALIFIER: return IR(884, 4);
    case SPV_OPERAND_TYPE_IMAGE: return IR(888, 17);
    case SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE: return IR(905, 26);
    case SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER: return IR(931, 20);
    case SPV_OPERAND_TYPE_INITIALIZATION_MODE_QUALIFIER: return IR(951, 2);
    case SPV_OPERAND_TYPE_KERNEL_ENQ_FLAGS: return IR(953, 3);
    case SPV_OPERAND_TYPE_KERNEL_PROFILING_INFO: return IR(956, 2);
    case SPV_OPERAND_TYPE_KERNEL_PROPERTY_FLAGS: return IR(958, 2);
    case SPV_OPERAND_TYPE_LINKAGE_TYPE: return IR(960, 4);
    case SPV_OPERAND_TYPE_LOAD_CACHE_CONTROL: return IR(964, 5);
    case SPV_OPERAND_TYPE_LOOP_CONTROL: return IR(969, 21);
    case SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS: return IR(990, 15);
    case SPV_OPERAND_TYPE_MEMORY_ACCESS: return IR(1005, 9);
    case SPV_OPERAND_TYPE_MEMORY_MODEL: return IR(1014, 4);
    case SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID: return IR(1018, 15);
    case SPV_OPERAND_TYPE_NAMED_MAXIMUM_NUMBER_OF_REGISTERS: return IR(1033, 1);
    case SPV_OPERAND_TYPE_OVERFLOW_MODES: return IR(1034, 4);
    case SPV_OPERAND_TYPE_PACKED_VECTOR_FORMAT: return IR(1038, 1);
    case SPV_OPERAND_TYPE_QUANTIZATION_MODES: return IR(1039, 8);
    case SPV_OPERAND_TYPE_RAW_ACCESS_CHAIN_OPERANDS: return IR(1047, 3);
    case SPV_OPERAND_TYPE_RAY_FLAGS: return IR(1050, 12);
    case SPV_OPERAND_TYPE_RAY_QUERY_CANDIDATE_INTERSECTION_TYPE: return IR(1062, 2);
    case SPV_OPERAND_TYPE_RAY_QUERY_COMMITTED_INTERSECTION_TYPE: return IR(1064, 3);
    case SPV_OPERAND_TYPE_RAY_QUERY_INTERSECTION: return IR(1067, 2);
    case SPV_OPERAND_TYPE_SAMPLER_ADDRESSING_MODE: return IR(1069, 5);
    case SPV_OPERAND_TYPE_SAMPLER_FILTER_MODE: return IR(1074, 2);
    case SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT: return IR(1076, 42);
    case SPV_OPERAND_TYPE_SCOPE_ID: return IR(1118, 7);
    case SPV_OPERAND_TYPE_SELECTION_CONTROL: return IR(1125, 3);
    case SPV_OPERAND_TYPE_SHDEBUG100_BUILD_IDENTIFIER_FLAGS: return IR(1128, 1);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING: return IR(1129, 8);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_COMPOSITE_TYPE: return IR(1137, 3);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_IMPORTED_ENTITY: return IR(1140, 2);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS: return IR(1142, 20);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION: return IR(1162, 10);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_TYPE_QUALIFIER: return IR(1172, 4);
    case SPV_OPERAND_TYPE_SOURCE_LANGUAGE: return IR(1176, 16);
    case SPV_OPERAND_TYPE_STORAGE_CLASS: return IR(1192, 29);
    case SPV_OPERAND_TYPE_STORE_CACHE_CONTROL: return IR(1221, 4);
    case SPV_OPERAND_TYPE_TENSOR_ADDRESSING_OPERANDS: return IR(1225, 4);
    case SPV_OPERAND_TYPE_TENSOR_CLAMP_MODE: return IR(1229, 5);
    case SPV_OPERAND_TYPE_TENSOR_OPERANDS: return IR(1234, 6);
    case SPV_OPERAND_TYPE_OPTIONAL_IMAGE: return IR(888, 17);
    case SPV_OPERAND_TYPE_OPTIONAL_ACCESS_QUALIFIER: return IR(0, 3);
    case SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS: return IR(1005, 9);
    case SPV_OPERAND_TYPE_OPTIONAL_PACKED_VECTOR_FORMAT: return IR(1038, 1);
    case SPV_OPERAND_TYPE_OPTIONAL_COOPERATIVE_MATRIX_OPERANDS: return IR(500, 6);
    case SPV_OPERAND_TYPE_OPTIONAL_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS: return IR(990, 15);
    case SPV_OPERAND_TYPE_OPTIONAL_RAW_ACCESS_CHAIN_OPERANDS: return IR(1047, 3);
    case SPV_OPERAND_TYPE_OPTIONAL_FPENCODING: return IR(831, 8);
    case SPV_OPERAND_TYPE_OPTIONAL_TENSOR_OPERANDS: return IR(1234, 6);
    case SPV_OPERAND_TYPE_OPTIONAL_CAPABILITY: return IR(135, 300);
    default: break;
  }
  return IR(0,0);
}

// Operand names and index into kOperandsByValue, ordered by (operand kind, name)
// The fields in order are:
//   name, either the primary name or an alias, indexing into kStrings
//   index into the kOperandsByValue array
static const std::array<NameIndex, 1418> kOperandNames{{
  {IR(5796, 9), 0}, // 0 ReadOnly in SPV_OPERAND_TYPE_ACCESS_QUALIFIER
  {IR(5822, 10), 2}, // 1 ReadWrite in SPV_OPERAND_TYPE_ACCESS_QUALIFIER
  {IR(5812, 10), 1}, // 2 WriteOnly in SPV_OPERAND_TYPE_ACCESS_QUALIFIER
  {IR(5832, 8), 3}, // 3 Logical in SPV_OPERAND_TYPE_ADDRESSING_MODEL
  {IR(5840, 11), 4}, // 4 Physical32 in SPV_OPERAND_TYPE_ADDRESSING_MODEL
  {IR(5861, 11), 5}, // 5 Physical64 in SPV_OPERAND_TYPE_ADDRESSING_MODEL
  {IR(5872, 24), 6}, // 6 PhysicalStorageBuffer64 in SPV_OPERAND_TYPE_ADDRESSING_MODEL
  {IR(5896, 27), 6}, // 7 PhysicalStorageBuffer64EXT in SPV_OPERAND_TYPE_ADDRESSING_MODEL
  {IR(7855, 13), 94}, // 8 BaryCoordKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7868, 12), 94}, // 9 BaryCoordNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7201, 20), 68}, // 10 BaryCoordNoPerspAMD in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7221, 28), 69}, // 11 BaryCoordNoPerspCentroidAMD in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7903, 20), 95}, // 12 BaryCoordNoPerspKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7923, 19), 95}, // 13 BaryCoordNoPerspNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7249, 26), 70}, // 14 BaryCoordNoPerspSampleAMD in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7346, 22), 74}, // 15 BaryCoordPullModelAMD in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7275, 19), 71}, // 16 BaryCoordSmoothAMD in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7294, 27), 72}, // 17 BaryCoordSmoothCentroidAMD in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7321, 25), 73}, // 18 BaryCoordSmoothSampleAMD in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7005, 13), 59}, // 19 BaseInstance in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6979, 11), 58}, // 20 BaseVertex in SPV_OPERAND_TYPE_BUILT_IN
  {IR(5980, 13), 9}, // 21 ClipDistance in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7762, 22), 89}, // 22 ClipDistancePerViewNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(9061, 12), 133}, // 23 ClusterIDNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6743, 13), 49}, // 24 CoreCountARM in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6717, 10), 48}, // 25 CoreIDARM in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6756, 13), 50}, // 26 CoreMaxIDARM in SPV_OPERAND_TYPE_BUILT_IN
  {IR(5993, 13), 10}, // 27 CullDistance in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7784, 22), 90}, // 28 CullDistancePerViewNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(9114, 12), 134}, // 29 CullMaskKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8110, 17), 101}, // 30 CullPrimitiveEXT in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8514, 17), 115}, // 31 CurrentRayTimeNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7075, 12), 62}, // 32 DeviceIndex in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7018, 10), 60}, // 33 DrawIndex in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6507, 22), 37}, // 34 EnqueuedWorkgroupSize in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6277, 10), 21}, // 35 FragCoord in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6363, 10), 27}, // 36 FragDepth in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7988, 23), 97}, // 37 FragInvocationCountEXT in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7942, 12), 96}, // 38 FragSizeEXT in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7368, 18), 75}, // 39 FragStencilRefEXT in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7954, 15), 96}, // 40 FragmentSizeNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6298, 12), 23}, // 41 FrontFacing in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7674, 16), 85}, // 42 FullyCoveredEXT in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6448, 19), 33}, // 43 GlobalInvocationId in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6542, 15), 39}, // 44 GlobalLinearId in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6529, 13), 38}, // 45 GlobalOffset in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6496, 11), 36}, // 46 GlobalSize in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6373, 17), 28}, // 47 HelperInvocation in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8816, 11), 122}, // 48 HitIsLSSNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8774, 14), 121}, // 49 HitIsSphereNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8996, 33), 130}, // 50 HitKindBackFacingMicroTriangleNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8962, 34), 129}, // 51 HitKindFrontFacingMicroTriangleNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8493, 11), 114}, // 52 HitKindKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8504, 10), 114}, // 53 HitKindNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8944, 18), 128}, // 54 HitLSSPositionsNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(9047, 14), 132}, // 55 HitLSSRadiiNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8678, 37), 118}, // 56 HitMicroTriangleVertexBarycentricsNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8611, 34), 117}, // 57 HitMicroTriangleVertexPositionsNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8866, 20), 123}, // 58 HitSpherePositionNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(9029, 18), 131}, // 59 HitSphereRadiusNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8486, 7), 113}, // 60 HitTNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8554, 30), 116}, // 61 HitTriangleVertexPositionsKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8715, 20), 119}, // 62 IncomingRayFlagsKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8735, 19), 119}, // 63 IncomingRayFlagsNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8375, 23), 110}, // 64 InstanceCustomIndexKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8398, 22), 110}, // 65 InstanceCustomIndexNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6015, 11), 12}, // 66 InstanceId in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6703, 14), 47}, // 67 InstanceIndex in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6116, 13), 14}, // 68 InvocationId in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8011, 22), 97}, // 69 InvocationsPerPixelNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8127, 12), 102}, // 70 LaunchIdKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8139, 11), 102}, // 71 LaunchIdNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8150, 14), 103}, // 72 LaunchSizeKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8164, 13), 103}, // 73 LaunchSizeNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6129, 6), 15}, // 74 Layer in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7806, 15), 91}, // 75 LayerPerViewNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6430, 18), 32}, // 76 LocalInvocationId in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6467, 21), 34}, // 77 LocalInvocationIndex in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7821, 16), 92}, // 78 MeshViewCountNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7837, 18), 93}, // 79 MeshViewIndicesNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6633, 21), 43}, // 80 NumEnqueuedSubgroups in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6620, 13), 42}, // 81 NumSubgroups in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6390, 14), 29}, // 82 NumWorkgroups in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8290, 22), 107}, // 83 ObjectRayDirectionKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8312, 21), 107}, // 84 ObjectRayDirectionNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8253, 19), 106}, // 85 ObjectRayOriginKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8272, 18), 106}, // 86 ObjectRayOriginNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8420, 17), 111}, // 87 ObjectToWorldKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8437, 16), 111}, // 88 ObjectToWorldNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6263, 14), 20}, // 89 PatchVertices in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6287, 11), 22}, // 90 PointCoord in SPV_OPERAND_TYPE_BUILT_IN
  {IR(5970, 10), 8}, // 91 PointSize in SPV_OPERAND_TYPE_BUILT_IN
  {IR(5954, 9), 7}, // 92 Position in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7614, 18), 83}, // 93 PositionPerViewNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7726, 17), 87}, // 94 PrimitiveCountNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6026, 12), 13}, // 95 PrimitiveId in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7743, 19), 88}, // 96 PrimitiveIndicesNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8058, 24), 99}, // 97 PrimitiveLineIndicesEXT in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8033, 25), 98}, // 98 PrimitivePointIndicesEXT in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7028, 24), 61}, // 99 PrimitiveShadingRateKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8082, 28), 100}, // 100 PrimitiveTriangleIndicesEXT in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8754, 20), 120}, // 101 RayGeometryIndexKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8354, 11), 109}, // 102 RayTmaxKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8365, 10), 109}, // 103 RayTmaxNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8333, 11), 108}, // 104 RayTminKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8344, 10), 108}, // 105 RayTminNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7403, 29), 76}, // 106 RemainingRecursionLevelsAMDX in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7499, 16), 79}, // 107 ResourceHeapEXT in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8918, 10), 125}, // 108 SMCountNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8937, 7), 127}, // 109 SMIDNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6310, 9), 24}, // 110 SampleId in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6352, 11), 26}, // 111 SampleMask in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6337, 15), 25}, // 112 SamplePosition in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7466, 15), 78}, // 113 SamplerHeapEXT in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7551, 20), 81}, // 114 SecondaryPositionNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7590, 24), 82}, // 115 SecondaryViewportMaskNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7450, 16), 77}, // 116 ShaderIndexAMDX in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7119, 15), 64}, // 117 ShadingRateKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6792, 15), 53}, // 118 SubgroupEqMask in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6807, 18), 53}, // 119 SubgroupEqMaskKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6847, 15), 54}, // 120 SubgroupGeMask in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6862, 18), 54}, // 121 SubgroupGeMaskKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6880, 15), 55}, // 122 SubgroupGtMask in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6895, 18), 55}, // 123 SubgroupGtMaskKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6654, 11), 44}, // 124 SubgroupId in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6913, 15), 56}, // 125 SubgroupLeMask in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6928, 18), 56}, // 126 SubgroupLeMaskKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6665, 26), 45}, // 127 SubgroupLocalInvocationId in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6946, 15), 57}, // 128 SubgroupLtMask in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6961, 18), 57}, // 129 SubgroupLtMaskKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6604, 16), 41}, // 130 SubgroupMaxSize in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6557, 13), 40}, // 131 SubgroupSize in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7714, 12), 86}, // 132 TaskCountNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6253, 10), 19}, // 133 TessCoord in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6238, 15), 18}, // 134 TessLevelInner in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6223, 15), 17}, // 135 TessLevelOuter in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7183, 18), 67}, // 136 TileApronSizeQCOM in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7165, 18), 66}, // 137 TileDimensionQCOM in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7134, 15), 65}, // 138 TileOffsetQCOM in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6006, 9), 11}, // 139 VertexId in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6691, 12), 46}, // 140 VertexIndex in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7099, 10), 63}, // 141 ViewIndex in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6175, 14), 16}, // 142 ViewportIndex in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7515, 15), 80}, // 143 ViewportMaskNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(7652, 22), 84}, // 144 ViewportMaskPerViewNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6769, 10), 51}, // 145 WarpIDARM in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8928, 9), 126}, // 146 WarpIDNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6779, 13), 52}, // 147 WarpMaxIDARM in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8886, 13), 124}, // 148 WarpsPerSMNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6488, 8), 35}, // 149 WorkDim in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6418, 12), 31}, // 150 WorkgroupId in SPV_OPERAND_TYPE_BUILT_IN
  {IR(6404, 14), 30}, // 151 WorkgroupSize in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8212, 21), 105}, // 152 WorldRayDirectionKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8233, 20), 105}, // 153 WorldRayDirectionNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8177, 18), 104}, // 154 WorldRayOriginKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8195, 17), 104}, // 155 WorldRayOriginNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8453, 17), 112}, // 156 WorldToObjectKHR in SPV_OPERAND_TYPE_BUILT_IN
  {IR(8470, 16), 112}, // 157 WorldToObjectNV in SPV_OPERAND_TYPE_BUILT_IN
  {IR(11476, 9), 274}, // 158 AbortKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(5851, 10), 139}, // 159 Addresses in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14782, 35), 379}, // 160 ArbitraryPrecisionFixedPointALTERA in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14817, 34), 379}, // 161 ArbitraryPrecisionFixedPointINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14254, 38), 367}, // 162 ArbitraryPrecisionFloatingPointALTERA in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14292, 37), 367}, // 163 ArbitraryPrecisionFloatingPointINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14189, 33), 366}, // 164 ArbitraryPrecisionIntegersALTERA in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14222, 32), 366}, // 165 ArbitraryPrecisionIntegersINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15534, 19), 405}, // 166 ArithmeticFenceEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13827, 9), 352}, // 167 AsmINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15435, 20), 401}, // 168 AtomicFloat16AddEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13882, 23), 355}, // 169 AtomicFloat16MinMaxEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13266, 22), 326}, // 170 AtomicFloat16VectorNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15351, 20), 397}, // 171 AtomicFloat32AddEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13836, 23), 353}, // 172 AtomicFloat32MinMaxEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15371, 20), 398}, // 173 AtomicFloat64AddEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13859, 23), 354}, // 174 AtomicFloat64MinMaxEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9294, 14), 155}, // 175 AtomicStorage in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10751, 17), 236}, // 176 AtomicStorageOps in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15476, 24), 403}, // 177 BFloat16ConversionINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11447, 29), 273}, // 178 BFloat16CooperativeMatrixKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11425, 22), 272}, // 179 BFloat16DotProductKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11409, 16), 271}, // 180 BFloat16TypeKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(16310, 20), 430}, // 181 BindlessImagesINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13203, 18), 323}, // 182 BindlessTextureNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15246, 16), 392}, // 183 BitInstructions in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10402, 18), 220}, // 184 BitcastExtractEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14984, 20), 383}, // 185 BlockingPipesALTERA in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15004, 19), 383}, // 186 BlockingPipesINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(16271, 19), 428}, // 187 CacheControlsINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(5980, 13), 165}, // 188 ClipDistance in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12805, 32), 311}, // 189 ComputeDerivativeGroupLinearKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12837, 31), 311}, // 190 ComputeDerivativeGroupLinearNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11655, 31), 290}, // 191 ComputeDerivativeGroupQuadsKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11686, 30), 290}, // 192 ComputeDerivativeGroupQuadsNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11485, 16), 276}, // 193 ConstantDataKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13500, 30), 338}, // 194 CooperativeMatrixBlockLoadsNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11120, 32), 255}, // 195 CooperativeMatrixConversionQCOM in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13393, 31), 335}, // 196 CooperativeMatrixConversionsNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13577, 32), 342}, // 197 CooperativeMatrixDecodeVectorNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10329, 21), 390}, // 198 CooperativeMatrixKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10264, 28), 213}, // 199 CooperativeMatrixLayoutsARM in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12893, 20), 313}, // 200 CooperativeMatrixNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13424, 40), 336}, // 201 CooperativeMatrixPerElementOperationsNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13363, 30), 334}, // 202 CooperativeMatrixReductionsNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13464, 36), 337}, // 203 CooperativeMatrixTensorAddressingNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13246, 20), 325}, // 204 CooperativeVectorNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13530, 28), 339}, // 205 CooperativeVectorTrainingNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(6727, 16), 205}, // 206 CoreBuiltinsARM in SPV_OPERAND_TYPE_CAPABILITY
  {IR(5993, 13), 166}, // 207 CullDistance in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15455, 21), 402}, // 208 DebugInfoModuleINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13016, 25), 318}, // 209 DemoteToHelperInvocation in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13041, 28), 318}, // 210 DemoteToHelperInvocationEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10890, 18), 242}, // 211 DenormFlushToZero in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10875, 15), 241}, // 212 DenormPreserve in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9741, 18), 184}, // 213 DerivativeControl in SPV_OPERAND_TYPE_CAPABILITY
  {IR(7481, 18), 275}, // 214 DescriptorHeapEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9265, 14), 153}, // 215 DeviceEnqueue in SPV_OPERAND_TYPE_CAPABILITY
  {IR(7087, 12), 232}, // 216 DeviceGroup in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13069, 23), 319}, // 217 DisplacementMicromapNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15197, 11), 388}, // 218 DotProduct in SPV_OPERAND_TYPE_CAPABILITY
  {IR(16396, 27), 433}, // 219 DotProductBFloat16AccVALVE in SPV_OPERAND_TYPE_CAPABILITY
  {IR(16363, 33), 432}, // 220 DotProductFloat16AccFloat16VALVE in SPV_OPERAND_TYPE_CAPABILITY
  {IR(16330, 33), 431}, // 221 DotProductFloat16AccFloat32VALVE in SPV_OPERAND_TYPE_CAPABILITY
  {IR(16423, 32), 434}, // 222 DotProductFloat8AccFloat32VALVE in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15091, 22), 386}, // 223 DotProductInput4x8Bit in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15113, 25), 386}, // 224 DotProductInput4x8BitKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15138, 28), 387}, // 225 DotProductInput4x8BitPacked in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15166, 31), 387}, // 226 DotProductInput4x8BitPackedKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15050, 19), 385}, // 227 DotProductInputAll in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15069, 22), 385}, // 228 DotProductInputAllKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15208, 14), 388}, // 229 DotProductKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(6990, 15), 223}, // 230 DrawParameters in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13939, 16), 358}, // 231 ExpectAssumeKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15302, 7), 395}, // 232 FMAKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14169, 20), 365}, // 233 FPFastMathModeINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15742, 29), 411}, // 234 FPGAArgumentInterfacesALTERA in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15771, 28), 411}, // 235 FPGAArgumentInterfacesINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14733, 25), 378}, // 236 FPGABufferLocationALTERA in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14758, 24), 378}, // 237 FPGABufferLocationINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14501, 28), 373}, // 238 FPGAClusterAttributesALTERA in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14529, 27), 373}, // 239 FPGAClusterAttributesINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15553, 30), 406}, // 240 FPGAClusterAttributesV2ALTERA in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15583, 29), 406}, // 241 FPGAClusterAttributesV2INTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14585, 21), 375}, // 242 FPGADSPControlALTERA in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14606, 20), 375}, // 243 FPGADSPControlINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14652, 41), 377}, // 244 FPGAInvocationPipeliningAttributesALTERA in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14693, 40), 377}, // 245 FPGAInvocationPipeliningAttributesINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14426, 26), 371}, // 246 FPGAKernelAttributesINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15612, 28), 407}, // 247 FPGAKernelAttributesv2INTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15693, 25), 410}, // 248 FPGALatencyControlALTERA in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15718, 24), 410}, // 249 FPGALatencyControlINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14359, 23), 369}, // 250 FPGALoopControlsALTERA in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14382, 22), 369}, // 251 FPGALoopControlsINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14452, 25), 372}, // 252 FPGAMemoryAccessesALTERA in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14477, 24), 372}, // 253 FPGAMemoryAccessesINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14116, 27), 364}, // 254 FPGAMemoryAttributesALTERA in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14143, 26), 364}, // 255 FPGAMemoryAttributesINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15023, 14), 384}, // 256 FPGARegALTERA in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15037, 13), 384}, // 257 FPGARegINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15677, 16), 409}, // 258 FPMaxErrorINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9179, 8), 144}, // 259 Float16 in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9165, 14), 143}, // 260 Float16Buffer in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11249, 16), 260}, // 261 Float16ImageAMD in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10360, 10), 217}, // 262 Float4EXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9187, 8), 145}, // 263 Float64 in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10350, 10), 216}, // 264 Float6EXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10302, 27), 215}, // 265 Float8CooperativeMatrixEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10292, 10), 214}, // 266 Float8EXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10370, 22), 218}, // 267 Float8UnsignedE8M0EXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15287, 15), 394}, // 268 FloatControls2 in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13735, 23), 348}, // 269 FloatingPointModeINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(7880, 23), 289}, // 270 FragmentBarycentricKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11633, 22), 289}, // 271 FragmentBarycentricNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(7969, 19), 291}, // 272 FragmentDensityEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(7690, 24), 285}, // 273 FragmentFullyCoveredEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11287, 16), 262}, // 274 FragmentMaskAMD in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12984, 32), 317}, // 275 FragmentShaderPixelInterlockEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12913, 33), 314}, // 276 FragmentShaderSampleInterlockEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12946, 38), 315}, // 277 FragmentShaderShadingRateInterlockEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(7052, 23), 221}, // 278 FragmentShadingRateKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14090, 26), 363}, // 279 FunctionFloatControlINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13781, 22), 350}, // 280 FunctionPointersINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(16130, 22), 422}, // 281 FunctionVariantsINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9586, 15), 171}, // 282 GenericPointer in SPV_OPERAND_TYPE_CAPABILITY
  {IR(6038, 9), 137}, // 283 Geometry in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9336, 18), 158}, // 284 GeometryPointSize in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11561, 28), 280}, // 285 GeometryShaderPassthroughNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9799, 16), 187}, // 286 GeometryStreams in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15829, 36), 413}, // 287 GlobalVariableFPGADecorationsALTERA in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15865, 35), 413}, // 288 GlobalVariableFPGADecorationsINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15799, 30), 412}, // 289 GlobalVariableHostAccessINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10255, 9), 212}, // 290 GraphARM in SPV_OPERAND_TYPE_CAPABILITY
  {IR(6570, 16), 194}, // 291 GroupNonUniform in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9938, 26), 196}, // 292 GroupNonUniformArithmetic in SPV_OPERAND_TYPE_CAPABILITY
  {IR(6825, 22), 197}, // 293 GroupNonUniformBallot in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10018, 25), 200}, // 294 GroupNonUniformClustered in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11730, 30), 292}, // 295 GroupNonUniformPartitionedEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11760, 29), 292}, // 296 GroupNonUniformPartitionedNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10043, 20), 201}, // 297 GroupNonUniformQuad in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15262, 25), 393}, // 298 GroupNonUniformRotateKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9964, 23), 198}, // 299 GroupNonUniformShuffle in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9987, 31), 199}, // 300 GroupNonUniformShuffleRelative in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9918, 20), 195}, // 301 GroupNonUniformVote in SPV_OPERAND_TYPE_CAPABILITY
  {IR(16193, 26), 425}, // 302 GroupUniformArithmeticKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9258, 7), 152}, // 303 Groups in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14957, 14), 382}, // 304 IOPipesALTERA in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14971, 13), 382}, // 305 IOPipesINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9655, 8), 177}, // 306 Image1D in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9214, 11), 148}, // 307 ImageBasic in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9677, 12), 180}, // 308 ImageBuffer in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9532, 15), 167}, // 309 ImageCubeArray in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11616, 17), 287}, // 310 ImageFootprintNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11265, 22), 261}, // 311 ImageGatherBiasLodAMD in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9354, 20), 159}, // 312 ImageGatherExtended in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11220, 29), 259}, // 313 ImageGatherExtendedModesQCOM in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11198, 22), 258}, // 314 ImageGatherLinearQCOM in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9689, 13), 181}, // 315 ImageMSArray in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9240, 12), 150}, // 316 ImageMipmap in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9730, 11), 183}, // 317 ImageQuery in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9225, 15), 149}, // 318 ImageReadWrite in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11303, 21), 264}, // 319 ImageReadWriteLodAMD in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9564, 10), 169}, // 320 ImageRect in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13803, 24), 351}, // 321 IndirectReferencesINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9606, 16), 173}, // 322 InputAttachment in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11875, 36), 295}, // 323 InputAttachmentArrayDynamicIndexing in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11911, 39), 295}, // 324 InputAttachmentArrayDynamicIndexingEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12416, 39), 302}, // 325 InputAttachmentArrayNonUniformIndexing in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12455, 42), 302}, // 326 InputAttachmentArrayNonUniformIndexingEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9308, 6), 156}, // 327 Int16 in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11382, 27), 270}, // 328 Int4CooperativeMatrixINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11368, 14), 269}, // 329 Int4TypeINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9195, 6), 146}, // 330 Int64 in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9201, 13), 147}, // 331 Int64Atomics in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11324, 14), 265}, // 332 Int64ImageEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9601, 5), 172}, // 333 Int8 in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13758, 23), 349}, // 334 IntegerFunctions2INTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9759, 22), 185}, // 335 InterpolationFunction in SPV_OPERAND_TYPE_CAPABILITY
  {IR(5805, 7), 141}, // 336 Kernel in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14404, 22), 370}, // 337 KernelAttributesINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9148, 8), 140}, // 338 Linkage in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9279, 15), 154}, // 339 LiteralSampler in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15391, 20), 399}, // 340 LongCompositesINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13326, 14), 332}, // 341 LongVectorEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14556, 15), 374}, // 342 LoopFuseALTERA in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14571, 14), 374}, // 343 LoopFuseINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10392, 10), 219}, // 344 MXInt8EXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(16246, 25), 427}, // 345 MaskedGatherScatterINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9141, 7), 135}, // 346 Matrix in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14626, 26), 376}, // 347 MemoryAccessAliasingINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(6101, 15), 288}, // 348 MeshShadingEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(6087, 14), 286}, // 349 MeshShadingNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9638, 7), 175}, // 350 MinLod in SPV_OPERAND_TYPE_CAPABILITY
  {IR(7109, 10), 233}, // 351 MultiView in SPV_OPERAND_TYPE_CAPABILITY
  {IR(6189, 14), 190}, // 352 MultiViewport in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11175, 23), 257}, // 353 MultipleWaitQueuesQCOM in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9893, 13), 192}, // 354 NamedBarrier in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15411, 11), 400}, // 355 OptNoneEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15422, 13), 400}, // 356 OptNoneINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(7632, 20), 284}, // 357 PerViewAttributesNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(5923, 31), 310}, // 358 PhysicalStorageBufferAddresses in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12771, 34), 310}, // 359 PhysicalStorageBufferAddressesEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9906, 12), 193}, // 360 PipeStorage in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9252, 6), 151}, // 361 Pipes in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11501, 16), 277}, // 362 PoisonFreezeKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(16152, 18), 423}, // 363 PredicatedIOINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13306, 20), 331}, // 364 PushConstantBanksNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11353, 15), 268}, // 365 QuadControlKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13288, 18), 328}, // 366 RawAccessChainsNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9126, 15), 389}, // 367 RayCullMaskKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10988, 12), 247}, // 368 RayQueryKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13221, 25), 324}, // 369 RayQueryPositionFetchKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10965, 23), 246}, // 370 RayQueryProvisionalKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9073, 41), 340}, // 371 RayTracingClusterAccelerationStructureNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(8645, 33), 327}, // 372 RayTracingDisplacementMicromapNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(6073, 14), 250}, // 373 RayTracingKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(8827, 39), 330}, // 374 RayTracingLinearSweptSpheresGeometryNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(8531, 23), 307}, // 375 RayTracingMotionBlurNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(6060, 13), 306}, // 376 RayTracingNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13121, 29), 320}, // 377 RayTracingOpacityMicromapEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15309, 42), 396}, // 378 RayTracingOpacityMicromapExecutionModeKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13092, 29), 320}, // 379 RayTracingOpacityMicromapKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(8584, 27), 305}, // 380 RayTracingPositionFetchKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12868, 25), 312}, // 381 RayTracingProvisionalKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(8788, 28), 329}, // 382 RayTracingSpheresGeometryNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11019, 32), 249}, // 383 RayTraversalPrimitiveCullingKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(16290, 20), 429}, // 384 RegisterLimitsINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15222, 24), 391}, // 385 ReplicatedCompositesEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13714, 21), 347}, // 386 RoundToInfinityINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(16170, 23), 424}, // 387 RoundedDivideSqrtINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10933, 16), 244}, // 388 RoundingModeRTE in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10949, 16), 245}, // 389 RoundingModeRTZ in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14898, 30), 381}, // 390 RuntimeAlignedAttributeALTERA in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14928, 29), 381}, // 391 RuntimeAlignedAttributeINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11826, 23), 294}, // 392 RuntimeDescriptorArray in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11849, 26), 294}, // 393 RuntimeDescriptorArrayEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11532, 29), 279}, // 394 SampleMaskOverrideCoverageNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10768, 28), 237}, // 395 SampleMaskPostDepthCoverage in SPV_OPERAND_TYPE_CAPABILITY
  {IR(6319, 18), 168}, // 396 SampleRateShading in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9645, 10), 176}, // 397 Sampled1D in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9663, 14), 179}, // 398 SampledBuffer in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9547, 17), 178}, // 399 SampledCubeArray in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9432, 33), 162}, // 400 SampledImageArrayDynamicIndexing in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12189, 36), 299}, // 401 SampledImageArrayNonUniformIndexing in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12225, 39), 299}, // 402 SampledImageArrayNonUniformIndexingEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9574, 12), 170}, // 403 SampledRect in SPV_OPERAND_TYPE_CAPABILITY
  {IR(5963, 7), 136}, // 404 Shader in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13340, 23), 333}, // 405 Shader64BitIndexingEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11338, 15), 266}, // 406 ShaderClockKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(7432, 18), 267}, // 407 ShaderEnqueueAMDX in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13176, 27), 322}, // 408 ShaderInvocationReorderEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13150, 26), 321}, // 409 ShaderInvocationReorderNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(6135, 12), 202}, // 410 ShaderLayer in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11789, 17), 293}, // 411 ShaderNonUniform in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11806, 20), 293}, // 412 ShaderNonUniformEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(8899, 19), 316}, // 413 ShaderSMBuiltinsNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(7571, 19), 283}, // 414 ShaderStereoViewNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(6203, 20), 203}, // 415 ShaderViewportIndex in SPV_OPERAND_TYPE_CAPABILITY
  {IR(6147, 28), 281}, // 416 ShaderViewportIndexLayerEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11589, 27), 281}, // 417 ShaderViewportIndexLayerNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(7530, 21), 282}, // 418 ShaderViewportMaskNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11716, 14), 291}, // 419 ShadingRateNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10908, 25), 243}, // 420 SignedZeroInfNanPreserve in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9622, 16), 174}, // 421 SparseResidency in SPV_OPERAND_TYPE_CAPABILITY
  {IR(16109, 21), 421}, // 422 SpecConditionalINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15500, 16), 404}, // 423 SplitBarrierEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15516, 18), 404}, // 424 SplitBarrierINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(7386, 17), 263}, // 425 StencilExportEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10556, 25), 228}, // 426 StorageBuffer16BitAccess in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10796, 24), 238}, // 427 StorageBuffer8BitAccess in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9465, 34), 163}, // 428 StorageBufferArrayDynamicIndexing in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12264, 37), 300}, // 429 StorageBufferArrayNonUniformIndexing in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12301, 40), 300}, // 430 StorageBufferArrayNonUniformIndexingEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9499, 33), 164}, // 431 StorageImageArrayDynamicIndexing in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12341, 36), 301}, // 432 StorageImageArrayNonUniformIndexing in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12377, 39), 301}, // 433 StorageImageArrayNonUniformIndexingEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9702, 28), 182}, // 434 StorageImageExtendedFormats in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9374, 24), 160}, // 435 StorageImageMultisample in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9815, 30), 188}, // 436 StorageImageReadWithoutFormat in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9845, 31), 189}, // 437 StorageImageWriteWithoutFormat in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10683, 21), 231}, // 438 StorageInputOutput16 in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10661, 22), 230}, // 439 StoragePushConstant16 in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10854, 21), 240}, // 440 StoragePushConstant8 in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10178, 37), 210}, // 441 StorageTensorArrayDynamicIndexingARM in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10215, 40), 211}, // 442 StorageTensorArrayNonUniformIndexingARM in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12031, 39), 297}, // 443 StorageTexelBufferArrayDynamicIndexing in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12070, 42), 297}, // 444 StorageTexelBufferArrayDynamicIndexingEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12584, 42), 304}, // 445 StorageTexelBufferArrayNonUniformIndexing in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12626, 45), 304}, // 446 StorageTexelBufferArrayNonUniformIndexingEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10644, 17), 229}, // 447 StorageUniform16 in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10581, 28), 228}, // 448 StorageUniformBufferBlock16 in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15928, 23), 415}, // 449 Subgroup2DBlockIOINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15951, 30), 416}, // 450 Subgroup2DBlockTransformINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15981, 30), 417}, // 451 Subgroup2DBlockTransposeINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14026, 39), 361}, // 452 SubgroupAvcMotionEstimationChromaINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13955, 33), 359}, // 453 SubgroupAvcMotionEstimationINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13988, 38), 360}, // 454 SubgroupAvcMotionEstimationIntraINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(6586, 18), 222}, // 455 SubgroupBallotKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13630, 27), 344}, // 456 SubgroupBufferBlockIOINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15900, 28), 414}, // 457 SubgroupBufferPrefetchINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9876, 17), 191}, // 458 SubgroupDispatch in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13657, 26), 345}, // 459 SubgroupImageBlockIOINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13683, 31), 346}, // 460 SubgroupImageMediaBlockIOINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(16011, 38), 418}, // 461 SubgroupMatrixMultiplyAccumulateINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13609, 21), 343}, // 462 SubgroupShuffleINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10540, 16), 227}, // 463 SubgroupVoteKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15640, 19), 408}, // 464 TaskSequenceALTERA in SPV_OPERAND_TYPE_CAPABILITY
  {IR(15659, 18), 408}, // 465 TaskSequenceINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13558, 19), 341}, // 466 TensorAddressingNV in SPV_OPERAND_TYPE_CAPABILITY
  {IR(16219, 27), 426}, // 467 TensorFloat32RoundingINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10167, 11), 209}, // 468 TensorsARM in SPV_OPERAND_TYPE_CAPABILITY
  {IR(16049, 28), 419}, // 469 TernaryBitwiseFunctionINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(6047, 13), 138}, // 470 Tessellation in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9314, 22), 157}, // 471 TessellationPointSize in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11152, 23), 256}, // 472 TextureBlockMatch2QCOM in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11098, 22), 253}, // 473 TextureBlockMatchQCOM in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11077, 21), 252}, // 474 TextureBoxFilterQCOM in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11051, 26), 251}, // 475 TextureSampleWeightedQCOM in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10081, 28), 206}, // 476 TileImageColorReadAccessEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10109, 28), 207}, // 477 TileImageDepthReadAccessEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10137, 30), 208}, // 478 TileImageStencilReadAccessEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(7149, 16), 254}, // 479 TileShadingQCOM in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9781, 18), 186}, // 480 TransformFeedback in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14851, 24), 380}, // 481 USMStorageClassesALTERA in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14875, 23), 380}, // 482 USMStorageClassesINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10609, 35), 229}, // 483 UniformAndStorageBuffer16BitAccess in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10820, 34), 239}, // 484 UniformAndStorageBuffer8BitAccess in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9398, 34), 161}, // 485 UniformBufferArrayDynamicIndexing in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12112, 37), 298}, // 486 UniformBufferArrayNonUniformIndexing in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12149, 40), 298}, // 487 UniformBufferArrayNonUniformIndexingEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10063, 18), 204}, // 488 UniformDecoration in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11950, 39), 296}, // 489 UniformTexelBufferArrayDynamicIndexing in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11989, 42), 296}, // 490 UniformTexelBufferArrayDynamicIndexingEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12497, 42), 303}, // 491 UniformTexelBufferArrayNonUniformIndexing in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12539, 45), 303}, // 492 UniformTexelBufferArrayNonUniformIndexingEXT in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14329, 30), 368}, // 493 UnstructuredLoopControlsINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11000, 19), 248}, // 494 UntypedPointersKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(16077, 32), 420}, // 495 UntypedVariableLengthArrayINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(14065, 25), 362}, // 496 VariableLengthArrayINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10734, 17), 235}, // 497 VariablePointers in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10704, 30), 234}, // 498 VariablePointersStorageBuffer in SPV_OPERAND_TYPE_CAPABILITY
  {IR(9156, 9), 142}, // 499 Vector16 in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13924, 15), 357}, // 500 VectorAnyINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(13905, 19), 356}, // 501 VectorComputeINTEL in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12671, 18), 308}, // 502 VulkanMemoryModel in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12710, 29), 309}, // 503 VulkanMemoryModelDeviceScope in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12739, 32), 309}, // 504 VulkanMemoryModelDeviceScopeKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(12689, 21), 308}, // 505 VulkanMemoryModelKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(11517, 15), 278}, // 506 WeakLinkageAMD in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10496, 44), 226}, // 507 WorkgroupMemoryExplicitLayout16BitAccessKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10453, 43), 225}, // 508 WorkgroupMemoryExplicitLayout8BitAccessKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(10420, 33), 224}, // 509 WorkgroupMemoryExplicitLayoutKHR in SPV_OPERAND_TYPE_CAPABILITY
  {IR(16467, 8), 436}, // 510 Address in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16475, 8), 437}, // 511 Boolean in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16483, 6), 438}, // 512 Float in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16489, 7), 439}, // 513 Signed in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16496, 11), 440}, // 514 SignedChar in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16507, 9), 441}, // 515 Unsigned in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16516, 13), 442}, // 516 UnsignedChar in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16455, 12), 435}, // 517 Unspecified in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16529, 6), 443}, // 518 Class in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_COMPOSITE_TYPE
  {IR(16535, 10), 444}, // 519 Structure in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_COMPOSITE_TYPE
  {IR(16545, 6), 445}, // 520 Union in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_COMPOSITE_TYPE
  {IR(16566, 20), 447}, // 521 ImportedDeclaration in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_IMPORTED_ENTITY
  {IR(16551, 15), 446}, // 522 ImportedModule in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_IMPORTED_ENTITY
  {IR(16675, 15), 455}, // 523 FlagArtificial in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {IR(16690, 13), 456}, // 524 FlagExplicit in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {IR(16663, 12), 454}, // 525 FlagFwdDecl in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {IR(16753, 21), 460}, // 526 FlagIndirectVariable in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {IR(16646, 17), 453}, // 527 FlagIsDefinition in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {IR(16830, 16), 464}, // 528 FlagIsEnumClass in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {IR(16634, 12), 452}, // 529 FlagIsLocal in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {IR(16814, 16), 463}, // 530 FlagIsOptimized in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {IR(16607, 14), 450}, // 531 FlagIsPrivate in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {IR(16591, 16), 449}, // 532 FlagIsProtected in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {IR(16621, 13), 451}, // 533 FlagIsPublic in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {IR(16774, 20), 461}, // 534 FlagLValueReference in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {IR(16718, 18), 458}, // 535 FlagObjectPointer in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {IR(16703, 15), 457}, // 536 FlagPrototyped in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {IR(16794, 20), 462}, // 537 FlagRValueReference in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {IR(16736, 17), 459}, // 538 FlagStaticMember in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {IR(16866, 24), 466}, // 539 FlagTypePassByReference in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {IR(16846, 20), 465}, // 540 FlagTypePassByValue in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {IR(16586, 5), 448}, // 541 None in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS
  {IR(16951, 9), 471}, // 542 BitPiece in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION
  {IR(16983, 7), 475}, // 543 Constu in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION
  {IR(16890, 6), 467}, // 544 Deref in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION
  {IR(16990, 9), 476}, // 545 Fragment in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION
  {IR(16901, 6), 469}, // 546 Minus in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION
  {IR(16896, 5), 468}, // 547 Plus in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION
  {IR(16940, 11), 470}, // 548 PlusUconst in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION
  {IR(16972, 11), 474}, // 549 StackValue in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION
  {IR(16960, 5), 472}, // 550 Swap in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION
  {IR(16965, 7), 473}, // 551 Xderef in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION
  {IR(17035, 11), 480}, // 552 AtomicType in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_TYPE_QUALIFIER
  {IR(16999, 10), 477}, // 553 ConstType in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_TYPE_QUALIFIER
  {IR(17022, 13), 479}, // 554 RestrictType in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_TYPE_QUALIFIER
  {IR(17009, 13), 478}, // 555 VolatileType in SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_TYPE_QUALIFIER
  {IR(17046, 10), 481}, // 556 Float16NV in SPV_OPERAND_TYPE_COMPONENT_TYPE
  {IR(17056, 10), 482}, // 557 Float32NV in SPV_OPERAND_TYPE_COMPONENT_TYPE
  {IR(17066, 10), 483}, // 558 Float64NV in SPV_OPERAND_TYPE_COMPONENT_TYPE
  {IR(17234, 12), 494}, // 559 FloatE4M3NV in SPV_OPERAND_TYPE_COMPONENT_TYPE
  {IR(17246, 12), 495}, // 560 FloatE5M2NV in SPV_OPERAND_TYPE_COMPONENT_TYPE
  {IR(17089, 14), 485}, // 561 SignedInt16NV in SPV_OPERAND_TYPE_COMPONENT_TYPE
  {IR(17103, 14), 486}, // 562 SignedInt32NV in SPV_OPERAND_TYPE_COMPONENT_TYPE
  {IR(17117, 14), 487}, // 563 SignedInt64NV in SPV_OPERAND_TYPE_COMPONENT_TYPE
  {IR(17076, 13), 484}, // 564 SignedInt8NV in SPV_OPERAND_TYPE_COMPONENT_TYPE
  {IR(17194, 19), 492}, // 565 SignedInt8PackedNV in SPV_OPERAND_TYPE_COMPONENT_TYPE
  {IR(17146, 16), 489}, // 566 UnsignedInt16NV in SPV_OPERAND_TYPE_COMPONENT_TYPE
  {IR(17162, 16), 490}, // 567 UnsignedInt32NV in SPV_OPERAND_TYPE_COMPONENT_TYPE
  {IR(17178, 16), 491}, // 568 UnsignedInt64NV in SPV_OPERAND_TYPE_COMPONENT_TYPE
  {IR(17131, 15), 488}, // 569 UnsignedInt8NV in SPV_OPERAND_TYPE_COMPONENT_TYPE
  {IR(17213, 21), 493}, // 570 UnsignedInt8PackedNV in SPV_OPERAND_TYPE_COMPONENT_TYPE
  {IR(17310, 28), 499}, // 571 ColumnBlockedInterleavedARM in SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_LAYOUT
  {IR(17270, 15), 497}, // 572 ColumnMajorKHR in SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_LAYOUT
  {IR(17285, 25), 498}, // 573 RowBlockedInterleavedARM in SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_LAYOUT
  {IR(17258, 12), 496}, // 574 RowMajorKHR in SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_LAYOUT
  {IR(17346, 27), 501}, // 575 MatrixASignedComponentsKHR in SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_OPERANDS
  {IR(17373, 27), 502}, // 576 MatrixBSignedComponentsKHR in SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_OPERANDS
  {IR(17400, 27), 503}, // 577 MatrixCSignedComponentsKHR in SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_OPERANDS
  {IR(17427, 32), 504}, // 578 MatrixResultSignedComponentsKHR in SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_OPERANDS
  {IR(17338, 8), 500}, // 579 NoneKHR in SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_OPERANDS
  {IR(17459, 26), 505}, // 580 SaturatingAccumulationKHR in SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_OPERANDS
  {IR(17496, 4), 508}, // 581 2x2 in SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_REDUCE
  {IR(17489, 7), 507}, // 582 Column in SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_REDUCE
  {IR(17485, 4), 506}, // 583 Row in SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_REDUCE
  {IR(17500, 11), 509}, // 584 MatrixAKHR in SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_USE
  {IR(17522, 21), 511}, // 585 MatrixAccumulatorKHR in SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_USE
  {IR(17511, 11), 510}, // 586 MatrixBKHR in SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_USE
  {IR(17554, 14), 513}, // 587 ColumnMajorNV in SPV_OPERAND_TYPE_COOPERATIVE_VECTOR_MATRIX_LAYOUT
  {IR(17568, 21), 514}, // 588 InferencingOptimalNV in SPV_OPERAND_TYPE_COOPERATIVE_VECTOR_MATRIX_LAYOUT
  {IR(17543, 11), 512}, // 589 RowMajorNV in SPV_OPERAND_TYPE_COOPERATIVE_VECTOR_MATRIX_LAYOUT
  {IR(17589, 18), 515}, // 590 TrainingOptimalNV in SPV_OPERAND_TYPE_COOPERATIVE_VECTOR_MATRIX_LAYOUT
  {IR(16467, 8), 517}, // 591 Address in SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16475, 8), 518}, // 592 Boolean in SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16483, 6), 519}, // 593 Float in SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16489, 7), 520}, // 594 Signed in SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16496, 11), 521}, // 595 SignedChar in SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16507, 9), 522}, // 596 Unsigned in SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16516, 13), 523}, // 597 UnsignedChar in SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16455, 12), 516}, // 598 Unspecified in SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16529, 6), 524}, // 599 Class in SPV_OPERAND_TYPE_DEBUG_COMPOSITE_TYPE
  {IR(16535, 10), 525}, // 600 Structure in SPV_OPERAND_TYPE_DEBUG_COMPOSITE_TYPE
  {IR(16545, 6), 526}, // 601 Union in SPV_OPERAND_TYPE_DEBUG_COMPOSITE_TYPE
  {IR(16675, 15), 534}, // 602 FlagArtificial in SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {IR(16690, 13), 535}, // 603 FlagExplicit in SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {IR(16663, 12), 533}, // 604 FlagFwdDecl in SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {IR(16753, 21), 539}, // 605 FlagIndirectVariable in SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {IR(16646, 17), 532}, // 606 FlagIsDefinition in SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {IR(16634, 12), 531}, // 607 FlagIsLocal in SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {IR(16814, 16), 542}, // 608 FlagIsOptimized in SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {IR(16607, 14), 529}, // 609 FlagIsPrivate in SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {IR(16591, 16), 528}, // 610 FlagIsProtected in SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {IR(16621, 13), 530}, // 611 FlagIsPublic in SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {IR(16774, 20), 540}, // 612 FlagLValueReference in SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {IR(16718, 18), 537}, // 613 FlagObjectPointer in SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {IR(16703, 15), 536}, // 614 FlagPrototyped in SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {IR(16794, 20), 541}, // 615 FlagRValueReference in SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {IR(16736, 17), 538}, // 616 FlagStaticMember in SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {IR(16586, 5), 527}, // 617 None in SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS
  {IR(16951, 9), 547}, // 618 BitPiece in SPV_OPERAND_TYPE_DEBUG_OPERATION
  {IR(16983, 7), 551}, // 619 Constu in SPV_OPERAND_TYPE_DEBUG_OPERATION
  {IR(16890, 6), 543}, // 620 Deref in SPV_OPERAND_TYPE_DEBUG_OPERATION
  {IR(16901, 6), 545}, // 621 Minus in SPV_OPERAND_TYPE_DEBUG_OPERATION
  {IR(16896, 5), 544}, // 622 Plus in SPV_OPERAND_TYPE_DEBUG_OPERATION
  {IR(16940, 11), 546}, // 623 PlusUconst in SPV_OPERAND_TYPE_DEBUG_OPERATION
  {IR(16972, 11), 550}, // 624 StackValue in SPV_OPERAND_TYPE_DEBUG_OPERATION
  {IR(16960, 5), 548}, // 625 Swap in SPV_OPERAND_TYPE_DEBUG_OPERATION
  {IR(16965, 7), 549}, // 626 Xderef in SPV_OPERAND_TYPE_DEBUG_OPERATION
  {IR(16999, 10), 552}, // 627 ConstType in SPV_OPERAND_TYPE_DEBUG_TYPE_QUALIFIER
  {IR(17022, 13), 554}, // 628 RestrictType in SPV_OPERAND_TYPE_DEBUG_TYPE_QUALIFIER
  {IR(17009, 13), 553}, // 629 VolatileType in SPV_OPERAND_TYPE_DEBUG_TYPE_QUALIFIER
  {IR(20327, 16), 675}, // 630 AliasScopeINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(17816, 8), 574}, // 631 Aliased in SPV_OPERAND_TYPE_DECORATION
  {IR(18977, 15), 630}, // 632 AliasedPointer in SPV_OPERAND_TYPE_DECORATION
  {IR(18992, 18), 630}, // 633 AliasedPointerEXT in SPV_OPERAND_TYPE_DECORATION
  {IR(18294, 10), 598}, // 634 Alignment in SPV_OPERAND_TYPE_DECORATION
  {IR(18338, 12), 600}, // 635 AlignmentId in SPV_OPERAND_TYPE_DECORATION
  {IR(17667, 12), 561}, // 636 ArrayStride in SPV_OPERAND_TYPE_DECORATION
  {IR(18718, 17), 617}, // 637 ArrayStrideIdEXT in SPV_OPERAND_TYPE_DECORATION
  {IR(19877, 15), 663}, // 638 BankBitsALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(19892, 14), 663}, // 639 BankBitsINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(19086, 7), 634}, // 640 BankNV in SPV_OPERAND_TYPE_DECORATION
  {IR(19590, 16), 656}, // 641 BankwidthALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(19606, 15), 656}, // 642 BankwidthINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(17971, 8), 587}, // 643 Binding in SPV_OPERAND_TYPE_DECORATION
  {IR(19111, 16), 636}, // 644 BindlessImageNV in SPV_OPERAND_TYPE_DECORATION
  {IR(19093, 18), 635}, // 645 BindlessSamplerNV in SPV_OPERAND_TYPE_DECORATION
  {IR(17631, 6), 557}, // 646 Block in SPV_OPERAND_TYPE_DECORATION
  {IR(18478, 22), 607}, // 647 BlockMatchSamplerQCOM in SPV_OPERAND_TYPE_DECORATION
  {IR(18456, 22), 606}, // 648 BlockMatchTextureQCOM in SPV_OPERAND_TYPE_DECORATION
  {IR(19142, 13), 638}, // 649 BoundImageNV in SPV_OPERAND_TYPE_DECORATION
  {IR(19127, 15), 637}, // 650 BoundSamplerNV in SPV_OPERAND_TYPE_DECORATION
  {IR(17637, 12), 558}, // 651 BufferBlock in SPV_OPERAND_TYPE_DECORATION
  {IR(20487, 21), 680}, // 652 BufferLocationALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(20508, 20), 680}, // 653 BufferLocationINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(17748, 8), 566}, // 654 BuiltIn in SPV_OPERAND_TYPE_DECORATION
  {IR(20046, 20), 668}, // 655 BurstCoalesceALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(20066, 19), 668}, // 656 BurstCoalesceINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(17714, 8), 565}, // 657 CPacked in SPV_OPERAND_TYPE_DECORATION
  {IR(21734, 22), 703}, // 658 CacheControlLoadINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(21793, 23), 704}, // 659 CacheControlStoreINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(20085, 16), 669}, // 660 CacheSizeALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(20101, 15), 669}, // 661 CacheSizeINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(17781, 9), 570}, // 662 Centroid in SPV_OPERAND_TYPE_DECORATION
  {IR(19195, 13), 641}, // 663 ClobberINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(17842, 9), 577}, // 664 Coherent in SPV_OPERAND_TYPE_DECORATION
  {IR(17658, 9), 560}, // 665 ColMajor in SPV_OPERAND_TYPE_DECORATION
  {IR(17955, 10), 585}, // 666 Component in SPV_OPERAND_TYPE_DECORATION
  {IR(21681, 17), 702}, // 667 ConditionalINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(20910, 28), 690}, // 668 ConduitKernelArgumentALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(20938, 27), 690}, // 669 ConduitKernelArgumentINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(17833, 9), 576}, // 670 Constant in SPV_OPERAND_TYPE_DECORATION
  {IR(19341, 14), 648}, // 671 CounterBuffer in SPV_OPERAND_TYPE_DECORATION
  {IR(17979, 14), 588}, // 672 DescriptorSet in SPV_OPERAND_TYPE_DECORATION
  {IR(20116, 29), 670}, // 673 DontStaticallyCoalesceALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(20145, 28), 670}, // 674 DontStaticallyCoalesceINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(19699, 17), 659}, // 675 DoublepumpALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(19716, 16), 659}, // 676 DoublepumpINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(18500, 18), 608}, // 677 ExplicitInterpAMD in SPV_OPERAND_TYPE_DECORATION
  {IR(18164, 15), 594}, // 678 FPFastMathMode in SPV_OPERAND_TYPE_DECORATION
  {IR(20772, 26), 687}, // 679 FPMaxErrorDecorationINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(18114, 15), 593}, // 680 FPRoundingMode in SPV_OPERAND_TYPE_DECORATION
  {IR(17770, 5), 568}, // 681 Flat in SPV_OPERAND_TYPE_DECORATION
  {IR(19906, 21), 664}, // 682 ForcePow2DepthALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(19927, 20), 664}, // 683 ForcePow2DepthINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(18066, 14), 592}, // 684 FuncParamAttr in SPV_OPERAND_TYPE_DECORATION
  {IR(19252, 21), 644}, // 685 FuncParamIOKindINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(19483, 24), 652}, // 686 FunctionDenormModeINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(20601, 31), 682}, // 687 FunctionFloatingPointModeINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(19426, 26), 651}, // 688 FunctionRoundingModeINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(20237, 26), 673}, // 689 FuseLoopsInFunctionALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(20263, 25), 673}, // 690 FuseLoopsInFunctionINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(17703, 11), 564}, // 691 GLSLPacked in SPV_OPERAND_TYPE_DECORATION
  {IR(17692, 11), 563}, // 692 GLSLShared in SPV_OPERAND_TYPE_DECORATION
  {IR(19315, 26), 647}, // 693 GlobalVariableOffsetINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(19055, 31), 633}, // 694 HitObjectShaderRecordBufferEXT in SPV_OPERAND_TYPE_DECORATION
  {IR(19025, 30), 632}, // 695 HitObjectShaderRecordBufferNV in SPV_OPERAND_TYPE_DECORATION
  {IR(19355, 24), 648}, // 696 HlslCounterBufferGOOGLE in SPV_OPERAND_TYPE_DECORATION
  {IR(19392, 19), 649}, // 697 HlslSemanticGOOGLE in SPV_OPERAND_TYPE_DECORATION
  {IR(21532, 16), 699}, // 698 HostAccessINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(20528, 20), 681}, // 699 IOPipeStorageALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(20548, 19), 681}, // 700 IOPipeStorageINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(21624, 29), 701}, // 701 ImplementInRegisterMapALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(21653, 28), 701}, // 702 ImplementInRegisterMapINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(17965, 6), 586}, // 703 Index in SPV_OPERAND_TYPE_DECORATION
  {IR(21595, 15), 700}, // 704 InitModeALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(21610, 14), 700}, // 705 InitModeINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(20356, 25), 677}, // 706 InitiationIntervalALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(20381, 24), 677}, // 707 InitiationIntervalINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(18273, 21), 597}, // 708 InputAttachmentIndex in SPV_OPERAND_TYPE_DECORATION
  {IR(17797, 10), 572}, // 709 Invariant in SPV_OPERAND_TYPE_DECORATION
  {IR(20849, 31), 689}, // 710 LatencyControlConstraintALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(20880, 30), 689}, // 711 LatencyControlConstraintINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(20798, 26), 688}, // 712 LatencyControlLabelALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(20824, 25), 688}, // 713 LatencyControlLabelINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(18241, 18), 595}, // 714 LinkageAttributes in SPV_OPERAND_TYPE_DECORATION
  {IR(17946, 9), 584}, // 715 Location in SPV_OPERAND_TYPE_DECORATION
  {IR(21028, 34), 692}, // 716 MMHostInterfaceAddressWidthALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(21062, 33), 692}, // 717 MMHostInterfaceAddressWidthINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(21095, 31), 693}, // 718 MMHostInterfaceDataWidthALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(21126, 30), 693}, // 719 MMHostInterfaceDataWidthINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(21156, 29), 694}, // 720 MMHostInterfaceLatencyALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(21185, 28), 694}, // 721 MMHostInterfaceLatencyINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(21316, 30), 696}, // 722 MMHostInterfaceMaxBurstALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(21346, 29), 696}, // 723 MMHostInterfaceMaxBurstINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(21247, 35), 695}, // 724 MMHostInterfaceReadWriteModeALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(21282, 34), 695}, // 725 MMHostInterfaceReadWriteModeINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(21375, 33), 697}, // 726 MMHostInterfaceWaitRequestALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(21408, 32), 697}, // 727 MMHostInterfaceWaitRequestINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(20288, 20), 674}, // 728 MathOpDSPModeALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(20308, 19), 674}, // 729 MathOpDSPModeINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(17679, 13), 562}, // 730 MatrixStride in SPV_OPERAND_TYPE_DECORATION
  {IR(18304, 14), 599}, // 731 MaxByteOffset in SPV_OPERAND_TYPE_DECORATION
  {IR(18350, 16), 601}, // 732 MaxByteOffsetId in SPV_OPERAND_TYPE_DECORATION
  {IR(20405, 21), 678}, // 733 MaxConcurrencyALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(20426, 20), 678}, // 734 MaxConcurrencyINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(19621, 23), 657}, // 735 MaxPrivateCopiesALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(19644, 22), 657}, // 736 MaxPrivateCopiesINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(19732, 20), 660}, // 737 MaxReplicatesALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(19752, 19), 660}, // 738 MaxReplicatesINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(20692, 18), 685}, // 739 MediaBlockIOINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(19010, 15), 631}, // 740 MemberOffsetNV in SPV_OPERAND_TYPE_DECORATION
  {IR(19536, 13), 654}, // 741 MemoryALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(19549, 12), 654}, // 742 MemoryINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(19812, 12), 662}, // 743 MergeALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(19824, 11), 662}, // 744 MergeINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(20343, 13), 676}, // 745 NoAliasINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(18259, 14), 596}, // 746 NoContraction in SPV_OPERAND_TYPE_DECORATION
  {IR(17756, 14), 567}, // 747 NoPerspective in SPV_OPERAND_TYPE_DECORATION
  {IR(18410, 13), 603}, // 748 NoSignedWrap in SPV_OPERAND_TYPE_DECORATION
  {IR(18423, 15), 604}, // 749 NoUnsignedWrap in SPV_OPERAND_TYPE_DECORATION
  {IR(18550, 20), 610}, // 750 NodeMaxPayloadsAMDX in SPV_OPERAND_TYPE_DECORATION
  {IR(18518, 32), 609}, // 751 NodeSharesPayloadLimitsWithAMDX in SPV_OPERAND_TYPE_DECORATION
  {IR(17863, 12), 579}, // 752 NonReadable in SPV_OPERAND_TYPE_DECORATION
  {IR(18917, 11), 628}, // 753 NonUniform in SPV_OPERAND_TYPE_DECORATION
  {IR(18928, 14), 628}, // 754 NonUniformEXT in SPV_OPERAND_TYPE_DECORATION
  {IR(17851, 12), 578}, // 755 NonWritable in SPV_OPERAND_TYPE_DECORATION
  {IR(19561, 15), 655}, // 756 NumbanksALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(19576, 14), 655}, // 757 NumbanksINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(17993, 7), 589}, // 758 Offset in SPV_OPERAND_TYPE_DECORATION
  {IR(18735, 12), 618}, // 759 OffsetIdEXT in SPV_OPERAND_TYPE_DECORATION
  {IR(18761, 19), 620}, // 760 OverrideCoverageNV in SPV_OPERAND_TYPE_DECORATION
  {IR(18780, 14), 621}, // 761 PassthroughNV in SPV_OPERAND_TYPE_DECORATION
  {IR(17775, 6), 569}, // 762 Patch in SPV_OPERAND_TYPE_DECORATION
  {IR(18690, 28), 616}, // 763 PayloadDispatchIndirectAMDX in SPV_OPERAND_TYPE_DECORATION
  {IR(18665, 25), 615}, // 764 PayloadNodeArraySizeAMDX in SPV_OPERAND_TYPE_DECORATION
  {IR(18613, 25), 613}, // 765 PayloadNodeBaseIndexAMDX in SPV_OPERAND_TYPE_DECORATION
  {IR(18593, 20), 612}, // 766 PayloadNodeNameAMDX in SPV_OPERAND_TYPE_DECORATION
  {IR(18638, 27), 614}, // 767 PayloadNodeSparseArrayAMDX in SPV_OPERAND_TYPE_DECORATION
  {IR(18841, 16), 624}, // 768 PerPrimitiveEXT in SPV_OPERAND_TYPE_DECORATION
  {IR(18857, 15), 624}, // 769 PerPrimitiveNV in SPV_OPERAND_TYPE_DECORATION
  {IR(18882, 10), 626}, // 770 PerTaskNV in SPV_OPERAND_TYPE_DECORATION
  {IR(18892, 13), 627}, // 771 PerVertexKHR in SPV_OPERAND_TYPE_DECORATION
  {IR(18905, 12), 627}, // 772 PerVertexNV in SPV_OPERAND_TYPE_DECORATION
  {IR(18872, 10), 625}, // 773 PerViewNV in SPV_OPERAND_TYPE_DECORATION
  {IR(20446, 21), 679}, // 774 PipelineEnableALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(20467, 20), 679}, // 775 PipelineEnableINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(20173, 15), 671}, // 776 PrefetchALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(20188, 14), 671}, // 777 PrefetchINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(19169, 26), 640}, // 778 ReferencedIndirectlyINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(19507, 15), 653}, // 779 RegisterALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(19522, 14), 653}, // 780 RegisterINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(20965, 32), 691}, // 781 RegisterMapKernelArgumentALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(20997, 31), 691}, // 782 RegisterMapKernelArgumentINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(17607, 17), 555}, // 783 RelaxedPrecision in SPV_OPERAND_TYPE_DECORATION
  {IR(17807, 9), 573}, // 784 Restrict in SPV_OPERAND_TYPE_DECORATION
  {IR(18942, 16), 629}, // 785 RestrictPointer in SPV_OPERAND_TYPE_DECORATION
  {IR(18958, 19), 629}, // 786 RestrictPointerEXT in SPV_OPERAND_TYPE_DECORATION
  {IR(17649, 9), 559}, // 787 RowMajor in SPV_OPERAND_TYPE_DECORATION
  {IR(19155, 14), 639}, // 788 SIMTCallINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(17790, 7), 571}, // 789 Sample in SPV_OPERAND_TYPE_DECORATION
  {IR(17919, 20), 582}, // 790 SaturatedConversion in SPV_OPERAND_TYPE_DECORATION
  {IR(18366, 44), 602}, // 791 SaturatedToLargestFloat8NormalConversionEXT in SPV_OPERAND_TYPE_DECORATION
  {IR(18813, 28), 623}, // 792 SecondaryViewportRelativeNV in SPV_OPERAND_TYPE_DECORATION
  {IR(19208, 17), 642}, // 793 SideEffectsINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(19771, 21), 661}, // 794 SimpleDualPortALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(19792, 20), 661}, // 795 SimpleDualPortINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(20632, 25), 683}, // 796 SingleElementVectorINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(19666, 17), 658}, // 797 SinglepumpALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(19683, 16), 658}, // 798 SinglepumpINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(17624, 7), 556}, // 799 SpecId in SPV_OPERAND_TYPE_DECORATION
  {IR(21440, 27), 698}, // 800 StableKernelArgumentALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(21467, 26), 698}, // 801 StableKernelArgumentINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(19300, 15), 646}, // 802 StackCallINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(20202, 18), 672}, // 803 StallEnableALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(20220, 17), 672}, // 804 StallEnableINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(20710, 16), 686}, // 805 StallFreeALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(20726, 15), 686}, // 806 StallFreeINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(17939, 7), 583}, // 807 Stream in SPV_OPERAND_TYPE_DECORATION
  {IR(19947, 17), 665}, // 808 StridesizeALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(19964, 16), 665}, // 809 StridesizeINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(18570, 23), 611}, // 810 TrackFinishWritingAMDX in SPV_OPERAND_TYPE_DECORATION
  {IR(20009, 19), 667}, // 811 TrueDualPortALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(20028, 18), 667}, // 812 TrueDualPortINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(18747, 14), 619}, // 813 UTFEncodedKHR in SPV_OPERAND_TYPE_DECORATION
  {IR(17875, 8), 580}, // 814 Uniform in SPV_OPERAND_TYPE_DECORATION
  {IR(17909, 10), 581}, // 815 UniformId in SPV_OPERAND_TYPE_DECORATION
  {IR(19379, 13), 649}, // 816 UserSemantic in SPV_OPERAND_TYPE_DECORATION
  {IR(19411, 15), 650}, // 817 UserTypeGOOGLE in SPV_OPERAND_TYPE_DECORATION
  {IR(20657, 35), 684}, // 818 VectorComputeCallableFunctionINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(19273, 27), 645}, // 819 VectorComputeFunctionINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(19225, 27), 643}, // 820 VectorComputeVariableINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(18794, 19), 622}, // 821 ViewportRelativeNV in SPV_OPERAND_TYPE_DECORATION
  {IR(17824, 9), 575}, // 822 Volatile in SPV_OPERAND_TYPE_DECORATION
  {IR(18438, 18), 605}, // 823 WeightTextureQCOM in SPV_OPERAND_TYPE_DECORATION
  {IR(19980, 15), 666}, // 824 WordsizeALTERA in SPV_OPERAND_TYPE_DECORATION
  {IR(19995, 14), 666}, // 825 WordsizeINTEL in SPV_OPERAND_TYPE_DECORATION
  {IR(18000, 10), 590}, // 826 XfbBuffer in SPV_OPERAND_TYPE_DECORATION
  {IR(18010, 10), 591}, // 827 XfbStride in SPV_OPERAND_TYPE_DECORATION
  {IR(21816, 3), 705}, // 828 1D in SPV_OPERAND_TYPE_DIMENSIONALITY
  {IR(21819, 3), 706}, // 829 2D in SPV_OPERAND_TYPE_DIMENSIONALITY
  {IR(21822, 3), 707}, // 830 3D in SPV_OPERAND_TYPE_DIMENSIONALITY
  {IR(21835, 7), 710}, // 831 Buffer in SPV_OPERAND_TYPE_DIMENSIONALITY
  {IR(21825, 5), 708}, // 832 Cube in SPV_OPERAND_TYPE_DIMENSIONALITY
  {IR(21830, 5), 709}, // 833 Rect in SPV_OPERAND_TYPE_DIMENSIONALITY
  {IR(21842, 12), 711}, // 834 SubpassData in SPV_OPERAND_TYPE_DIMENSIONALITY
  {IR(21854, 17), 712}, // 835 TileImageDataEXT in SPV_OPERAND_TYPE_DIMENSIONALITY
  {IR(22972, 20), 780}, // 836 ArithmeticPoisonKHR in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22666, 15), 765}, // 837 CoalescingAMDX in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22297, 15), 743}, // 838 ContractionOff in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(10890, 18), 757}, // 839 DenormFlushToZero in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(10875, 15), 756}, // 840 DenormPreserve in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22067, 13), 726}, // 841 DepthGreater in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22080, 10), 727}, // 842 DepthLess in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22052, 15), 725}, // 843 DepthReplacing in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22090, 15), 728}, // 844 DepthUnchanged in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23107, 25), 784}, // 845 DerivativeGroupLinearKHR in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23132, 24), 784}, // 846 DerivativeGroupLinearNV in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23060, 24), 783}, // 847 DerivativeGroupQuadsKHR in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23084, 23), 783}, // 848 DerivativeGroupQuadsNV in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22614, 29), 763}, // 849 EarlyAndLateFragmentTestsAMD in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22019, 19), 722}, // 850 EarlyFragmentTests in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23617, 18), 804}, // 851 FPFastMathDefault in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22324, 10), 745}, // 852 Finalizer in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23432, 26), 796}, // 853 FloatingPointModeALTINTEL in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23458, 27), 797}, // 854 FloatingPointModeIEEEINTEL in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22312, 12), 744}, // 855 Initializer in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22141, 11), 732}, // 856 InputLines in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22152, 20), 733}, // 857 InputLinesAdjacency in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22129, 12), 731}, // 858 InputPoints in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22182, 24), 735}, // 859 InputTrianglesAdjacency in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(21871, 12), 713}, // 860 Invocations in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22681, 15), 766}, // 861 IsApiEntryAMDX in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22212, 9), 737}, // 862 Isolines in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22105, 10), 729}, // 863 LocalSize in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22115, 14), 730}, // 864 LocalSizeHint in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22392, 16), 750}, // 865 LocalSizeHintId in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22380, 12), 749}, // 866 LocalSizeId in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22696, 21), 767}, // 867 MaxNodeRecursionAMDX in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22741, 21), 770}, // 868 MaxNumWorkgroupsAMDX in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23507, 16), 799}, // 869 MaxWorkDimINTEL in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23485, 22), 798}, // 870 MaxWorkgroupSizeINTEL in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23593, 24), 803}, // 871 MaximallyReconvergesKHR in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23729, 22), 809}, // 872 MaximumRegistersINTEL in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23751, 24), 810}, // 873 MaximumRegistersIdINTEL in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23706, 23), 808}, // 874 NamedBarrierCountINTEL in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23826, 27), 811}, // 875 NamedMaximumRegistersINTEL in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23523, 20), 800}, // 876 NoGlobalOffsetINTEL in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22408, 34), 751}, // 877 NonCoherentColorAttachmentReadEXT in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22442, 34), 752}, // 878 NonCoherentDepthAttachmentReadEXT in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22476, 36), 753}, // 879 NonCoherentStencilAttachmentReadEXT in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22560, 34), 761}, // 880 NonCoherentTileAttachmentReadQCOM in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23543, 22), 801}, // 881 NumSIMDWorkitemsINTEL in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23635, 21), 805}, // 882 OpacityMicromapIdKHR in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22003, 16), 721}, // 883 OriginLowerLeft in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(21987, 16), 720}, // 884 OriginUpperLeft in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22249, 16), 740}, // 885 OutputLineStrip in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22992, 15), 781}, // 886 OutputLinesEXT in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23007, 14), 781}, // 887 OutputLinesNV in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22236, 13), 739}, // 888 OutputPoints in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23021, 20), 782}, // 889 OutputPrimitivesEXT in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23041, 19), 782}, // 890 OutputPrimitivesNV in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22265, 20), 741}, // 891 OutputTriangleStrip in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23156, 19), 785}, // 892 OutputTrianglesEXT in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23175, 18), 785}, // 893 OutputTrianglesNV in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22221, 15), 738}, // 894 OutputVertices in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(21968, 19), 719}, // 895 PixelCenterInteger in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23193, 25), 786}, // 896 PixelInterlockOrderedEXT in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23218, 27), 787}, // 897 PixelInterlockUnorderedEXT in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22038, 10), 723}, // 898 PointMode in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22542, 18), 755}, // 899 PostDepthCoverage in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22913, 19), 777}, // 900 QuadDerivativesKHR in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22206, 6), 736}, // 901 Quads in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23680, 26), 807}, // 902 RegisterMapInterfaceINTEL in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22932, 20), 778}, // 903 RequireFullQuadsKHR in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(10933, 16), 759}, // 904 RoundingModeRTE in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23411, 21), 795}, // 905 RoundingModeRTNINTEL in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23390, 21), 794}, // 906 RoundingModeRTPINTEL in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(10949, 16), 760}, // 907 RoundingModeRTZ in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23245, 26), 788}, // 908 SampleInterlockOrderedEXT in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23271, 28), 789}, // 909 SampleInterlockUnorderedEXT in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23565, 28), 802}, // 910 SchedulerTargetFmaxMhzINTEL in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(13340, 23), 792}, // 911 Shader64BitIndexingEXT in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(7450, 16), 769}, // 912 ShaderIndexAMDX in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23299, 31), 790}, // 913 ShadingRateInterlockOrderedEXT in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23330, 33), 791}, // 914 ShadingRateInterlockUnorderedEXT in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23363, 27), 793}, // 915 SharedLocalMemorySizeINTEL in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22952, 20), 779}, // 916 SharesInputWithAMDX in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(10908, 25), 758}, // 917 SignedZeroInfNanPreserve in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(21883, 13), 714}, // 918 SpacingEqual in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(21896, 22), 715}, // 919 SpacingFractionalEven in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(21918, 21), 716}, // 920 SpacingFractionalOdd in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22717, 24), 768}, // 921 StaticNumWorkgroupsAMDX in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22866, 25), 775}, // 922 StencilRefGreaterBackAMD in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22790, 26), 772}, // 923 StencilRefGreaterFrontAMD in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22891, 22), 776}, // 924 StencilRefLessBackAMD in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22816, 23), 773}, // 925 StencilRefLessFrontAMD in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22643, 23), 764}, // 926 StencilRefReplacingEXT in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22839, 27), 774}, // 927 StencilRefUnchangedBackAMD in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22762, 28), 771}, // 928 StencilRefUnchangedFrontAMD in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23656, 24), 806}, // 929 StreamingInterfaceINTEL in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(6557, 13), 746}, // 930 SubgroupSize in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22512, 30), 754}, // 931 SubgroupUniformControlFlowKHR in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22334, 22), 747}, // 932 SubgroupsPerWorkgroup in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22356, 24), 748}, // 933 SubgroupsPerWorkgroupId in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22594, 20), 762}, // 934 TileShadingRateQCOM in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22172, 10), 734}, // 935 Triangles in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22285, 12), 742}, // 936 VecTypeHint in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(21953, 15), 718}, // 937 VertexOrderCcw in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(21939, 14), 717}, // 938 VertexOrderCw in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(22048, 4), 724}, // 939 Xfb in SPV_OPERAND_TYPE_EXECUTION_MODE
  {IR(23991, 10), 823}, // 940 AnyHitKHR in SPV_OPERAND_TYPE_EXECUTION_MODEL
  {IR(24001, 9), 823}, // 941 AnyHitNV in SPV_OPERAND_TYPE_EXECUTION_MODEL
  {IR(24052, 12), 826}, // 942 CallableKHR in SPV_OPERAND_TYPE_EXECUTION_MODEL
  {IR(24064, 11), 826}, // 943 CallableNV in SPV_OPERAND_TYPE_EXECUTION_MODEL
  {IR(24010, 14), 824}, // 944 ClosestHitKHR in SPV_OPERAND_TYPE_EXECUTION_MODEL
  {IR(24024, 13), 824}, // 945 ClosestHitNV in SPV_OPERAND_TYPE_EXECUTION_MODEL
  {IR(16990, 9), 816}, // 946 Fragment in SPV_OPERAND_TYPE_EXECUTION_MODEL
  {IR(23903, 10), 817}, // 947 GLCompute in SPV_OPERAND_TYPE_EXECUTION_MODEL
  {IR(6038, 9), 815}, // 948 Geometry in SPV_OPERAND_TYPE_EXECUTION_MODEL
  {IR(23960, 16), 822}, // 949 IntersectionKHR in SPV_OPERAND_TYPE_EXECUTION_MODEL
  {IR(23976, 15), 822}, // 950 IntersectionNV in SPV_OPERAND_TYPE_EXECUTION_MODEL
  {IR(5805, 7), 818}, // 951 Kernel in SPV_OPERAND_TYPE_EXECUTION_MODEL
  {IR(24083, 8), 828}, // 952 MeshEXT in SPV_OPERAND_TYPE_EXECUTION_MODEL
  {IR(23920, 7), 820}, // 953 MeshNV in SPV_OPERAND_TYPE_EXECUTION_MODEL
  {IR(24037, 8), 825}, // 954 MissKHR in SPV_OPERAND_TYPE_EXECUTION_MODEL
  {IR(24045, 7), 825}, // 955 MissNV in SPV_OPERAND_TYPE_EXECUTION_MODEL
  {IR(23927, 17), 821}, // 956 RayGenerationKHR in SPV_OPERAND_TYPE_EXECUTION_MODEL
  {IR(23944, 16), 821}, // 957 RayGenerationNV in SPV_OPERAND_TYPE_EXECUTION_MODEL
  {IR(24075, 8), 827}, // 958 TaskEXT in SPV_OPERAND_TYPE_EXECUTION_MODEL
  {IR(23913, 7), 819}, // 959 TaskNV in SPV_OPERAND_TYPE_EXECUTION_MODEL
  {IR(23860, 20), 813}, // 960 TessellationControl in SPV_OPERAND_TYPE_EXECUTION_MODEL
  {IR(23880, 23), 814}, // 961 TessellationEvaluation in SPV_OPERAND_TYPE_EXECUTION_MODEL
  {IR(23853, 7), 812}, // 962 Vertex in SPV_OPERAND_TYPE_EXECUTION_MODEL
  {IR(24100, 12), 830}, // 963 FlushToZero in SPV_OPERAND_TYPE_FPDENORM_MODE
  {IR(24091, 9), 829}, // 964 Preserve in SPV_OPERAND_TYPE_FPDENORM_MODE
  {IR(24112, 12), 831}, // 965 BFloat16KHR in SPV_OPERAND_TYPE_FPENCODING
  {IR(24180, 14), 836}, // 966 Float4E2M1EXT in SPV_OPERAND_TYPE_FPENCODING
  {IR(24152, 14), 834}, // 967 Float6E2M3EXT in SPV_OPERAND_TYPE_FPENCODING
  {IR(24166, 14), 835}, // 968 Float6E3M2EXT in SPV_OPERAND_TYPE_FPENCODING
  {IR(24124, 14), 832}, // 969 Float8E4M3EXT in SPV_OPERAND_TYPE_FPENCODING
  {IR(24138, 14), 833}, // 970 Float8E5M2EXT in SPV_OPERAND_TYPE_FPENCODING
  {IR(10370, 22), 837}, // 971 Float8UnsignedE8M0EXT in SPV_OPERAND_TYPE_FPENCODING
  {IR(10392, 10), 838}, // 972 MXInt8EXT in SPV_OPERAND_TYPE_FPENCODING
  {IR(24199, 4), 840}, // 973 ALT in SPV_OPERAND_TYPE_FPOPERATION_MODE
  {IR(24194, 5), 839}, // 974 IEEE in SPV_OPERAND_TYPE_FPOPERATION_MODE
  {IR(24237, 14), 847}, // 975 AllowContract in SPV_OPERAND_TYPE_FP_FAST_MATH_MODE
  {IR(24251, 23), 847}, // 976 AllowContractFastINTEL in SPV_OPERAND_TYPE_FP_FAST_MATH_MODE
  {IR(24274, 13), 848}, // 977 AllowReassoc in SPV_OPERAND_TYPE_FP_FAST_MATH_MODE
  {IR(24287, 18), 848}, // 978 AllowReassocINTEL in SPV_OPERAND_TYPE_FP_FAST_MATH_MODE
  {IR(24221, 11), 845}, // 979 AllowRecip in SPV_OPERAND_TYPE_FP_FAST_MATH_MODE
  {IR(24305, 15), 849}, // 980 AllowTransform in SPV_OPERAND_TYPE_FP_FAST_MATH_MODE
  {IR(24232, 5), 846}, // 981 Fast in SPV_OPERAND_TYPE_FP_FAST_MATH_MODE
  {IR(24217, 4), 844}, // 982 NSZ in SPV_OPERAND_TYPE_FP_FAST_MATH_MODE
  {IR(16586, 5), 841}, // 983 None in SPV_OPERAND_TYPE_FP_FAST_MATH_MODE
  {IR(24210, 7), 843}, // 984 NotInf in SPV_OPERAND_TYPE_FP_FAST_MATH_MODE
  {IR(24203, 7), 842}, // 985 NotNaN in SPV_OPERAND_TYPE_FP_FAST_MATH_MODE
  {IR(24320, 4), 850}, // 986 RTE in SPV_OPERAND_TYPE_FP_ROUNDING_MODE
  {IR(24332, 4), 853}, // 987 RTN in SPV_OPERAND_TYPE_FP_ROUNDING_MODE
  {IR(24328, 4), 852}, // 988 RTP in SPV_OPERAND_TYPE_FP_ROUNDING_MODE
  {IR(24324, 4), 851}, // 989 RTZ in SPV_OPERAND_TYPE_FP_ROUNDING_MODE
  {IR(24368, 18), 856}, // 990 Horizontal2Pixels in SPV_OPERAND_TYPE_FRAGMENT_SHADING_RATE
  {IR(24386, 18), 857}, // 991 Horizontal4Pixels in SPV_OPERAND_TYPE_FRAGMENT_SHADING_RATE
  {IR(24336, 16), 854}, // 992 Vertical2Pixels in SPV_OPERAND_TYPE_FRAGMENT_SHADING_RATE
  {IR(24352, 16), 855}, // 993 Vertical4Pixels in SPV_OPERAND_TYPE_FRAGMENT_SHADING_RATE
  {IR(24427, 6), 862}, // 994 Const in SPV_OPERAND_TYPE_FUNCTION_CONTROL
  {IR(24411, 11), 860}, // 995 DontInline in SPV_OPERAND_TYPE_FUNCTION_CONTROL
  {IR(24404, 7), 859}, // 996 Inline in SPV_OPERAND_TYPE_FUNCTION_CONTROL
  {IR(16586, 5), 858}, // 997 None in SPV_OPERAND_TYPE_FUNCTION_CONTROL
  {IR(15411, 11), 863}, // 998 OptNoneEXT in SPV_OPERAND_TYPE_FUNCTION_CONTROL
  {IR(15422, 13), 863}, // 999 OptNoneINTEL in SPV_OPERAND_TYPE_FUNCTION_CONTROL
  {IR(24422, 5), 861}, // 1000 Pure in SPV_OPERAND_TYPE_FUNCTION_CONTROL
  {IR(24443, 6), 866}, // 1001 ByVal in SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE
  {IR(24454, 8), 868}, // 1002 NoAlias in SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE
  {IR(24462, 10), 869}, // 1003 NoCapture in SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE
  {IR(24480, 12), 871}, // 1004 NoReadWrite in SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE
  {IR(24472, 8), 870}, // 1005 NoWrite in SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE
  {IR(24492, 21), 872}, // 1006 RuntimeAlignedALTERA in SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE
  {IR(24513, 20), 872}, // 1007 RuntimeAlignedINTEL in SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE
  {IR(24438, 5), 865}, // 1008 Sext in SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE
  {IR(24449, 5), 867}, // 1009 Sret in SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE
  {IR(24433, 5), 864}, // 1010 Zext in SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE
  {IR(24533, 14), 873}, // 1011 Gather4x1QCOM in SPV_OPERAND_TYPE_GATHER_MODES
  {IR(24547, 12), 874}, // 1012 GatherDQCOM in SPV_OPERAND_TYPE_GATHER_MODES
  {IR(24559, 13), 875}, // 1013 GatherH2QCOM in SPV_OPERAND_TYPE_GATHER_MODES
  {IR(24572, 13), 876}, // 1014 GatherV2QCOM in SPV_OPERAND_TYPE_GATHER_MODES
  {IR(24620, 16), 880}, // 1015 ClusteredReduce in SPV_OPERAND_TYPE_GROUP_OPERATION
  {IR(24606, 14), 879}, // 1016 ExclusiveScan in SPV_OPERAND_TYPE_GROUP_OPERATION
  {IR(24592, 14), 878}, // 1017 InclusiveScan in SPV_OPERAND_TYPE_GROUP_OPERATION
  {IR(24732, 28), 883}, // 1018 PartitionedExclusiveScanEXT in SPV_OPERAND_TYPE_GROUP_OPERATION
  {IR(24760, 27), 883}, // 1019 PartitionedExclusiveScanNV in SPV_OPERAND_TYPE_GROUP_OPERATION
  {IR(24677, 28), 882}, // 1020 PartitionedInclusiveScanEXT in SPV_OPERAND_TYPE_GROUP_OPERATION
  {IR(24705, 27), 882}, // 1021 PartitionedInclusiveScanNV in SPV_OPERAND_TYPE_GROUP_OPERATION
  {IR(24636, 21), 881}, // 1022 PartitionedReduceEXT in SPV_OPERAND_TYPE_GROUP_OPERATION
  {IR(24657, 20), 881}, // 1023 PartitionedReduceNV in SPV_OPERAND_TYPE_GROUP_OPERATION
  {IR(24585, 7), 877}, // 1024 Reduce in SPV_OPERAND_TYPE_GROUP_OPERATION
  {IR(24787, 10), 884}, // 1025 NoneINTEL in SPV_OPERAND_TYPE_HOST_ACCESS_QUALIFIER
  {IR(24797, 10), 885}, // 1026 ReadINTEL in SPV_OPERAND_TYPE_HOST_ACCESS_QUALIFIER
  {IR(24818, 15), 887}, // 1027 ReadWriteINTEL in SPV_OPERAND_TYPE_HOST_ACCESS_QUALIFIER
  {IR(24807, 11), 886}, // 1028 WriteINTEL in SPV_OPERAND_TYPE_HOST_ACCESS_QUALIFIER
  {IR(24833, 5), 889}, // 1029 Bias in SPV_OPERAND_TYPE_IMAGE
  {IR(24847, 12), 892}, // 1030 ConstOffset in SPV_OPERAND_TYPE_IMAGE
  {IR(24859, 13), 894}, // 1031 ConstOffsets in SPV_OPERAND_TYPE_IMAGE
  {IR(24842, 5), 891}, // 1032 Grad in SPV_OPERAND_TYPE_IMAGE
  {IR(24838, 4), 890}, // 1033 Lod in SPV_OPERAND_TYPE_IMAGE
  {IR(24872, 19), 897}, // 1034 MakeTexelAvailable in SPV_OPERAND_TYPE_IMAGE
  {IR(24891, 22), 897}, // 1035 MakeTexelAvailableKHR in SPV_OPERAND_TYPE_IMAGE
  {IR(24913, 17), 898}, // 1036 MakeTexelVisible in SPV_OPERAND_TYPE_IMAGE
  {IR(24930, 20), 898}, // 1037 MakeTexelVisibleKHR in SPV_OPERAND_TYPE_IMAGE
  {IR(9638, 7), 896}, // 1038 MinLod in SPV_OPERAND_TYPE_IMAGE
  {IR(24950, 16), 899}, // 1039 NonPrivateTexel in SPV_OPERAND_TYPE_IMAGE
  {IR(24966, 19), 899}, // 1040 NonPrivateTexelKHR in SPV_OPERAND_TYPE_IMAGE
  {IR(16586, 5), 888}, // 1041 None in SPV_OPERAND_TYPE_IMAGE
  {IR(25038, 12), 903}, // 1042 Nontemporal in SPV_OPERAND_TYPE_IMAGE
  {IR(17993, 7), 893}, // 1043 Offset in SPV_OPERAND_TYPE_IMAGE
  {IR(25050, 8), 904}, // 1044 Offsets in SPV_OPERAND_TYPE_IMAGE
  {IR(17790, 7), 895}, // 1045 Sample in SPV_OPERAND_TYPE_IMAGE
  {IR(25016, 11), 901}, // 1046 SignExtend in SPV_OPERAND_TYPE_IMAGE
  {IR(24985, 14), 900}, // 1047 VolatileTexel in SPV_OPERAND_TYPE_IMAGE
  {IR(24999, 17), 900}, // 1048 VolatileTexelKHR in SPV_OPERAND_TYPE_IMAGE
  {IR(25027, 11), 902}, // 1049 ZeroExtend in SPV_OPERAND_TYPE_IMAGE
  {IR(16483, 6), 919}, // 1050 Float in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25219, 10), 918}, // 1051 HalfFloat in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25154, 12), 913}, // 1052 SignedInt16 in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25166, 12), 914}, // 1053 SignedInt32 in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25143, 11), 912}, // 1054 SignedInt8 in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25068, 11), 906}, // 1055 SnormInt16 in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25058, 10), 905}, // 1056 SnormInt8 in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25128, 15), 911}, // 1057 UnormInt101010 in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25240, 17), 921}, // 1058 UnormInt101010_2 in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25257, 16), 922}, // 1059 UnormInt10X6EXT in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25390, 16), 929}, // 1060 UnormInt12X4EXT in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25406, 16), 930}, // 1061 UnormInt14X2EXT in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25089, 11), 908}, // 1062 UnormInt16 in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25229, 11), 920}, // 1063 UnormInt24 in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25313, 20), 925}, // 1064 UnormInt2_101010EXT in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25079, 10), 907}, // 1065 UnormInt8 in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25114, 14), 910}, // 1066 UnormShort555 in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25100, 14), 909}, // 1067 UnormShort565 in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25333, 19), 926}, // 1068 UnsignedInt10X6EXT in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25352, 19), 927}, // 1069 UnsignedInt12X4EXT in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25371, 19), 928}, // 1070 UnsignedInt14X2EXT in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25191, 14), 916}, // 1071 UnsignedInt16 in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25205, 14), 917}, // 1072 UnsignedInt32 in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25178, 13), 915}, // 1073 UnsignedInt8 in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25273, 20), 923}, // 1074 UnsignedIntRaw10EXT in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25293, 20), 924}, // 1075 UnsignedIntRaw12EXT in SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE
  {IR(25424, 2), 932}, // 1076 A in SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {IR(25525, 5), 950}, // 1077 ABGR in SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {IR(25446, 5), 938}, // 1078 ARGB in SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {IR(25441, 5), 937}, // 1079 BGRA in SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {IR(25483, 6), 944}, // 1080 Depth in SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {IR(25489, 13), 945}, // 1081 DepthStencil in SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {IR(25451, 10), 939}, // 1082 Intensity in SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {IR(25461, 10), 940}, // 1083 Luminance in SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {IR(25422, 2), 931}, // 1084 R in SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {IR(25429, 3), 934}, // 1085 RA in SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {IR(25426, 3), 933}, // 1086 RG in SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {IR(25432, 4), 935}, // 1087 RGB in SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {IR(25436, 5), 936}, // 1088 RGBA in SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {IR(25478, 5), 943}, // 1089 RGBx in SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {IR(25474, 4), 942}, // 1090 RGx in SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {IR(25471, 3), 941}, // 1091 Rx in SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {IR(25519, 6), 949}, // 1092 sBGRA in SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {IR(25502, 5), 946}, // 1093 sRGB in SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {IR(25513, 6), 948}, // 1094 sRGBA in SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {IR(25507, 6), 947}, // 1095 sRGBx in SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER
  {IR(25530, 28), 951}, // 1096 InitOnDeviceReprogramALTERA in SPV_OPERAND_TYPE_INITIALIZATION_MODE_QUALIFIER
  {IR(25558, 27), 951}, // 1097 InitOnDeviceReprogramINTEL in SPV_OPERAND_TYPE_INITIALIZATION_MODE_QUALIFIER
  {IR(25585, 24), 952}, // 1098 InitOnDeviceResetALTERA in SPV_OPERAND_TYPE_INITIALIZATION_MODE_QUALIFIER
  {IR(25609, 23), 952}, // 1099 InitOnDeviceResetINTEL in SPV_OPERAND_TYPE_INITIALIZATION_MODE_QUALIFIER
  {IR(25632, 7), 953}, // 1100 NoWait in SPV_OPERAND_TYPE_KERNEL_ENQ_FLAGS
  {IR(25639, 11), 954}, // 1101 WaitKernel in SPV_OPERAND_TYPE_KERNEL_ENQ_FLAGS
  {IR(25650, 14), 955}, // 1102 WaitWorkGroup in SPV_OPERAND_TYPE_KERNEL_ENQ_FLAGS
  {IR(25664, 12), 957}, // 1103 CmdExecTime in SPV_OPERAND_TYPE_KERNEL_PROFILING_INFO
  {IR(16586, 5), 956}, // 1104 None in SPV_OPERAND_TYPE_KERNEL_PROFILING_INFO
  {IR(25676, 13), 959}, // 1105 MayUsePrintf in SPV_OPERAND_TYPE_KERNEL_PROPERTY_FLAGS
  {IR(16586, 5), 958}, // 1106 None in SPV_OPERAND_TYPE_KERNEL_PROPERTY_FLAGS
  {IR(25689, 7), 960}, // 1107 Export in SPV_OPERAND_TYPE_LINKAGE_TYPE
  {IR(25696, 7), 961}, // 1108 Import in SPV_OPERAND_TYPE_LINKAGE_TYPE
  {IR(25703, 12), 962}, // 1109 LinkOnceODR in SPV_OPERAND_TYPE_LINKAGE_TYPE
  {IR(25715, 8), 963}, // 1110 WeakAMD in SPV_OPERAND_TYPE_LINKAGE_TYPE
  {IR(25737, 12), 965}, // 1111 CachedINTEL in SPV_OPERAND_TYPE_LOAD_CACHE_CONTROL
  {IR(25789, 17), 968}, // 1112 ConstCachedINTEL in SPV_OPERAND_TYPE_LOAD_CACHE_CONTROL
  {IR(25764, 25), 967}, // 1113 InvalidateAfterReadINTEL in SPV_OPERAND_TYPE_LOAD_CACHE_CONTROL
  {IR(25749, 15), 966}, // 1114 StreamingINTEL in SPV_OPERAND_TYPE_LOAD_CACHE_CONTROL
  {IR(25723, 14), 964}, // 1115 UncachedINTEL in SPV_OPERAND_TYPE_LOAD_CACHE_CONTROL
  {IR(25929, 22), 981}, // 1116 DependencyArrayALTERA in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(25951, 21), 981}, // 1117 DependencyArrayINTEL in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(25824, 19), 972}, // 1118 DependencyInfinite in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(25843, 17), 973}, // 1119 DependencyLength in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(25813, 11), 971}, // 1120 DontUnroll in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(20356, 25), 979}, // 1121 InitiationIntervalALTERA in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(20381, 24), 979}, // 1122 InitiationIntervalINTEL in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(25888, 18), 976}, // 1123 IterationMultiple in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(25972, 19), 983}, // 1124 LoopCoalesceALTERA in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(25991, 18), 983}, // 1125 LoopCoalesceINTEL in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(26134, 16), 987}, // 1126 LoopCountALTERA in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(26150, 15), 987}, // 1127 LoopCountINTEL in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(20405, 21), 980}, // 1128 MaxConcurrencyALTERA in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(20426, 20), 980}, // 1129 MaxConcurrencyINTEL in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(26009, 22), 984}, // 1130 MaxInterleavingALTERA in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(26031, 21), 984}, // 1131 MaxInterleavingINTEL in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(25874, 14), 975}, // 1132 MaxIterations in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(26165, 27), 988}, // 1133 MaxReinvocationDelayALTERA in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(26192, 26), 988}, // 1134 MaxReinvocationDelayINTEL in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(25860, 14), 974}, // 1135 MinIterations in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(11175, 23), 989}, // 1136 MultipleWaitQueuesQCOM in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(26105, 15), 986}, // 1137 NoFusionALTERA in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(26120, 14), 986}, // 1138 NoFusionINTEL in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(16586, 5), 969}, // 1139 None in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(25916, 13), 978}, // 1140 PartialCount in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(25906, 10), 977}, // 1141 PeelCount in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(20446, 21), 982}, // 1142 PipelineEnableALTERA in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(20467, 20), 982}, // 1143 PipelineEnableINTEL in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(26052, 27), 985}, // 1144 SpeculatedIterationsALTERA in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(26079, 26), 985}, // 1145 SpeculatedIterationsINTEL in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(25806, 7), 970}, // 1146 Unroll in SPV_OPERAND_TYPE_LOOP_CONTROL
  {IR(26501, 27), 1003}, // 1147 MatrixAPackedBFloat16INTEL in SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {IR(26449, 26), 1001}, // 1148 MatrixAPackedFloat16INTEL in SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {IR(26369, 23), 997}, // 1149 MatrixAPackedInt4INTEL in SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {IR(26323, 23), 995}, // 1150 MatrixAPackedInt8INTEL in SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {IR(26218, 29), 991}, // 1151 MatrixASignedComponentsINTEL in SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {IR(26415, 17), 999}, // 1152 MatrixATF32INTEL in SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {IR(26528, 27), 1004}, // 1153 MatrixBPackedBFloat16INTEL in SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {IR(26475, 26), 1002}, // 1154 MatrixBPackedFloat16INTEL in SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {IR(26392, 23), 998}, // 1155 MatrixBPackedInt4INTEL in SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {IR(26346, 23), 996}, // 1156 MatrixBPackedInt8INTEL in SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {IR(26247, 29), 992}, // 1157 MatrixBSignedComponentsINTEL in SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {IR(26432, 17), 1000}, // 1158 MatrixBTF32INTEL in SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {IR(26276, 21), 993}, // 1159 MatrixCBFloat16INTEL in SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {IR(26297, 26), 994}, // 1160 MatrixResultBFloat16INTEL in SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {IR(16586, 5), 990}, // 1161 None in SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS
  {IR(26688, 20), 1012}, // 1162 AliasScopeINTELMask in SPV_OPERAND_TYPE_MEMORY_ACCESS
  {IR(26555, 8), 1007}, // 1163 Aligned in SPV_OPERAND_TYPE_MEMORY_ACCESS
  {IR(26563, 21), 1009}, // 1164 MakePointerAvailable in SPV_OPERAND_TYPE_MEMORY_ACCESS
  {IR(26584, 24), 1009}, // 1165 MakePointerAvailableKHR in SPV_OPERAND_TYPE_MEMORY_ACCESS
  {IR(26608, 19), 1010}, // 1166 MakePointerVisible in SPV_OPERAND_TYPE_MEMORY_ACCESS
  {IR(26627, 22), 1010}, // 1167 MakePointerVisibleKHR in SPV_OPERAND_TYPE_MEMORY_ACCESS
  {IR(26708, 17), 1013}, // 1168 NoAliasINTELMask in SPV_OPERAND_TYPE_MEMORY_ACCESS
  {IR(26649, 18), 1011}, // 1169 NonPrivatePointer in SPV_OPERAND_TYPE_MEMORY_ACCESS
  {IR(26667, 21), 1011}, // 1170 NonPrivatePointerKHR in SPV_OPERAND_TYPE_MEMORY_ACCESS
  {IR(16586, 5), 1005}, // 1171 None in SPV_OPERAND_TYPE_MEMORY_ACCESS
  {IR(25038, 12), 1008}, // 1172 Nontemporal in SPV_OPERAND_TYPE_MEMORY_ACCESS
  {IR(17824, 9), 1006}, // 1173 Volatile in SPV_OPERAND_TYPE_MEMORY_ACCESS
  {IR(26732, 8), 1015}, // 1174 GLSL450 in SPV_OPERAND_TYPE_MEMORY_MODEL
  {IR(26740, 7), 1016}, // 1175 OpenCL in SPV_OPERAND_TYPE_MEMORY_MODEL
  {IR(26725, 7), 1014}, // 1176 Simple in SPV_OPERAND_TYPE_MEMORY_MODEL
  {IR(26747, 7), 1017}, // 1177 Vulkan in SPV_OPERAND_TYPE_MEMORY_MODEL
  {IR(26754, 10), 1017}, // 1178 VulkanKHR in SPV_OPERAND_TYPE_MEMORY_MODEL
  {IR(26772, 8), 1019}, // 1179 Acquire in SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {IR(26788, 15), 1021}, // 1180 AcquireRelease in SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {IR(26892, 20), 1027}, // 1181 AtomicCounterMemory in SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {IR(26871, 21), 1026}, // 1182 CrossWorkgroupMemory in SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {IR(26912, 12), 1028}, // 1183 ImageMemory in SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {IR(26953, 14), 1030}, // 1184 MakeAvailable in SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {IR(26967, 17), 1030}, // 1185 MakeAvailableKHR in SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {IR(26984, 12), 1031}, // 1186 MakeVisible in SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {IR(26996, 15), 1031}, // 1187 MakeVisibleKHR in SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {IR(16586, 5), 1018}, // 1188 None in SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {IR(26924, 13), 1029}, // 1189 OutputMemory in SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {IR(26937, 16), 1029}, // 1190 OutputMemoryKHR in SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {IR(26764, 8), 1018}, // 1191 Relaxed in SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {IR(26780, 8), 1020}, // 1192 Release in SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {IR(26803, 23), 1022}, // 1193 SequentiallyConsistent in SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {IR(26840, 15), 1024}, // 1194 SubgroupMemory in SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {IR(26826, 14), 1023}, // 1195 UniformMemory in SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {IR(17824, 9), 1032}, // 1196 Volatile in SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {IR(26855, 16), 1025}, // 1197 WorkgroupMemory in SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID
  {IR(27011, 10), 1033}, // 1198 AutoINTEL in SPV_OPERAND_TYPE_NAMED_MAXIMUM_NUMBER_OF_REGISTERS
  {IR(27026, 4), 1035}, // 1199 SAT in SPV_OPERAND_TYPE_OVERFLOW_MODES
  {IR(27039, 8), 1037}, // 1200 SAT_SYM in SPV_OPERAND_TYPE_OVERFLOW_MODES
  {IR(27030, 9), 1036}, // 1201 SAT_ZERO in SPV_OPERAND_TYPE_OVERFLOW_MODES
  {IR(27021, 5), 1034}, // 1202 WRAP in SPV_OPERAND_TYPE_OVERFLOW_MODES
  {IR(27047, 25), 1038}, // 1203 PackedVectorFormat4x8Bit in SPV_OPERAND_TYPE_PACKED_VECTOR_FORMAT
  {IR(27072, 28), 1038}, // 1204 PackedVectorFormat4x8BitKHR in SPV_OPERAND_TYPE_PACKED_VECTOR_FORMAT
  {IR(27113, 4), 1041}, // 1205 RND in SPV_OPERAND_TYPE_QUANTIZATION_MODES
  {IR(27146, 9), 1045}, // 1206 RND_CONV in SPV_OPERAND_TYPE_QUANTIZATION_MODES
  {IR(27155, 13), 1046}, // 1207 RND_CONV_ODD in SPV_OPERAND_TYPE_QUANTIZATION_MODES
  {IR(27126, 8), 1043}, // 1208 RND_INF in SPV_OPERAND_TYPE_QUANTIZATION_MODES
  {IR(27134, 12), 1044}, // 1209 RND_MIN_INF in SPV_OPERAND_TYPE_QUANTIZATION_MODES
  {IR(27117, 9), 1042}, // 1210 RND_ZERO in SPV_OPERAND_TYPE_QUANTIZATION_MODES
  {IR(27100, 4), 1039}, // 1211 TRN in SPV_OPERAND_TYPE_QUANTIZATION_MODES
  {IR(27104, 9), 1040}, // 1212 TRN_ZERO in SPV_OPERAND_TYPE_QUANTIZATION_MODES
  {IR(16586, 5), 1047}, // 1213 None in SPV_OPERAND_TYPE_RAW_ACCESS_CHAIN_OPERANDS
  {IR(27168, 25), 1048}, // 1214 RobustnessPerComponentNV in SPV_OPERAND_TYPE_RAW_ACCESS_CHAIN_OPERANDS
  {IR(27193, 23), 1049}, // 1215 RobustnessPerElementNV in SPV_OPERAND_TYPE_RAW_ACCESS_CHAIN_OPERANDS
  {IR(27285, 27), 1055}, // 1216 CullBackFacingTrianglesKHR in SPV_OPERAND_TYPE_RAY_FLAGS
  {IR(27312, 28), 1056}, // 1217 CullFrontFacingTrianglesKHR in SPV_OPERAND_TYPE_RAY_FLAGS
  {IR(27354, 16), 1058}, // 1218 CullNoOpaqueKHR in SPV_OPERAND_TYPE_RAY_FLAGS
  {IR(27340, 14), 1057}, // 1219 CullOpaqueKHR in SPV_OPERAND_TYPE_RAY_FLAGS
  {IR(27454, 30), 1061}, // 1220 ForceOpacityMicromap2StateEXT in SPV_OPERAND_TYPE_RAY_FLAGS
  {IR(27424, 30), 1061}, // 1221 ForceOpacityMicromap2StateKHR in SPV_OPERAND_TYPE_RAY_FLAGS
  {IR(27226, 12), 1052}, // 1222 NoOpaqueKHR in SPV_OPERAND_TYPE_RAY_FLAGS
  {IR(17338, 8), 1050}, // 1223 NoneKHR in SPV_OPERAND_TYPE_RAY_FLAGS
  {IR(27216, 10), 1051}, // 1224 OpaqueKHR in SPV_OPERAND_TYPE_RAY_FLAGS
  {IR(27411, 13), 1060}, // 1225 SkipAABBsKHR in SPV_OPERAND_TYPE_RAY_FLAGS
  {IR(27387, 24), 1059}, // 1226 SkipBuiltinPrimitivesNV in SPV_OPERAND_TYPE_RAY_FLAGS
  {IR(27261, 24), 1054}, // 1227 SkipClosestHitShaderKHR in SPV_OPERAND_TYPE_RAY_FLAGS
  {IR(27370, 17), 1059}, // 1228 SkipTrianglesKHR in SPV_OPERAND_TYPE_RAY_FLAGS
  {IR(27238, 23), 1053}, // 1229 TerminateOnFirstHitKHR in SPV_OPERAND_TYPE_RAY_FLAGS
  {IR(27525, 37), 1063}, // 1230 RayQueryCandidateIntersectionAABBKHR in SPV_OPERAND_TYPE_RAY_QUERY_CANDIDATE_INTERSECTION_TYPE
  {IR(27484, 41), 1062}, // 1231 RayQueryCandidateIntersectionTriangleKHR in SPV_OPERAND_TYPE_RAY_QUERY_CANDIDATE_INTERSECTION_TYPE
  {IR(27640, 42), 1066}, // 1232 RayQueryCommittedIntersectionGeneratedKHR in SPV_OPERAND_TYPE_RAY_QUERY_COMMITTED_INTERSECTION_TYPE
  {IR(27562, 37), 1064}, // 1233 RayQueryCommittedIntersectionNoneKHR in SPV_OPERAND_TYPE_RAY_QUERY_COMMITTED_INTERSECTION_TYPE
  {IR(27599, 41), 1065}, // 1234 RayQueryCommittedIntersectionTriangleKHR in SPV_OPERAND_TYPE_RAY_QUERY_COMMITTED_INTERSECTION_TYPE
  {IR(27682, 33), 1067}, // 1235 RayQueryCandidateIntersectionKHR in SPV_OPERAND_TYPE_RAY_QUERY_INTERSECTION
  {IR(27715, 33), 1068}, // 1236 RayQueryCommittedIntersectionKHR in SPV_OPERAND_TYPE_RAY_QUERY_INTERSECTION
  {IR(27760, 6), 1071}, // 1237 Clamp in SPV_OPERAND_TYPE_SAMPLER_ADDRESSING_MODE
  {IR(27748, 12), 1070}, // 1238 ClampToEdge in SPV_OPERAND_TYPE_SAMPLER_ADDRESSING_MODE
  {IR(16586, 5), 1069}, // 1239 None in SPV_OPERAND_TYPE_SAMPLER_ADDRESSING_MODE
  {IR(27766, 7), 1072}, // 1240 Repeat in SPV_OPERAND_TYPE_SAMPLER_ADDRESSING_MODE
  {IR(27773, 15), 1073}, // 1241 RepeatMirrored in SPV_OPERAND_TYPE_SAMPLER_ADDRESSING_MODE
  {IR(27796, 7), 1075}, // 1242 Linear in SPV_OPERAND_TYPE_SAMPLER_FILTER_MODE
  {IR(27788, 8), 1074}, // 1243 Nearest in SPV_OPERAND_TYPE_SAMPLER_FILTER_MODE
  {IR(27861, 13), 1084}, // 1244 R11fG11fB10f in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27903, 4), 1090}, // 1245 R16 in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27941, 9), 1095}, // 1246 R16Snorm in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27874, 5), 1085}, // 1247 R16f in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(28003, 5), 1104}, // 1248 R16i in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(28074, 6), 1114}, // 1249 R16ui in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27827, 5), 1079}, // 1250 R32f in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27981, 5), 1100}, // 1251 R32i in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(28038, 6), 1109}, // 1252 R32ui in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(28091, 5), 1117}, // 1253 R64i in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(28085, 6), 1116}, // 1254 R64ui in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27907, 3), 1091}, // 1255 R8 in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27950, 8), 1096}, // 1256 R8Snorm in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(28008, 4), 1105}, // 1257 R8i in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(28080, 5), 1115}, // 1258 R8ui in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27894, 5), 1088}, // 1259 Rg16 in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27922, 10), 1093}, // 1260 Rg16Snorm in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27855, 6), 1083}, // 1261 Rg16f in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27992, 6), 1102}, // 1262 Rg16i in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(28061, 7), 1112}, // 1263 Rg16ui in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27849, 6), 1082}, // 1264 Rg32f in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27986, 6), 1101}, // 1265 Rg32i in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(28054, 7), 1111}, // 1266 Rg32ui in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27899, 4), 1089}, // 1267 Rg8 in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27932, 9), 1094}, // 1268 Rg8Snorm in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27998, 5), 1103}, // 1269 Rg8i in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(28068, 6), 1113}, // 1270 Rg8ui in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27886, 8), 1087}, // 1271 Rgb10A2 in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(28044, 10), 1110}, // 1272 Rgb10a2ui in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27879, 7), 1086}, // 1273 Rgba16 in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27910, 12), 1092}, // 1274 Rgba16Snorm in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27819, 8), 1078}, // 1275 Rgba16f in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27966, 8), 1098}, // 1276 Rgba16i in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(28021, 9), 1107}, // 1277 Rgba16ui in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27811, 8), 1077}, // 1278 Rgba32f in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27958, 8), 1097}, // 1279 Rgba32i in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(28012, 9), 1106}, // 1280 Rgba32ui in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27832, 6), 1080}, // 1281 Rgba8 in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27838, 11), 1081}, // 1282 Rgba8Snorm in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27974, 7), 1099}, // 1283 Rgba8i in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(28030, 8), 1108}, // 1284 Rgba8ui in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(27803, 8), 1076}, // 1285 Unknown in SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT
  {IR(28096, 12), 1118}, // 1286 CrossDevice in SPV_OPERAND_TYPE_SCOPE_ID
  {IR(28108, 7), 1119}, // 1287 Device in SPV_OPERAND_TYPE_SCOPE_ID
  {IR(28134, 11), 1122}, // 1288 Invocation in SPV_OPERAND_TYPE_SCOPE_ID
  {IR(28145, 12), 1123}, // 1289 QueueFamily in SPV_OPERAND_TYPE_SCOPE_ID
  {IR(28157, 15), 1123}, // 1290 QueueFamilyKHR in SPV_OPERAND_TYPE_SCOPE_ID
  {IR(28172, 14), 1124}, // 1291 ShaderCallKHR in SPV_OPERAND_TYPE_SCOPE_ID
  {IR(28125, 9), 1121}, // 1292 Subgroup in SPV_OPERAND_TYPE_SCOPE_ID
  {IR(28115, 10), 1120}, // 1293 Workgroup in SPV_OPERAND_TYPE_SCOPE_ID
  {IR(28194, 12), 1127}, // 1294 DontFlatten in SPV_OPERAND_TYPE_SELECTION_CONTROL
  {IR(28186, 8), 1126}, // 1295 Flatten in SPV_OPERAND_TYPE_SELECTION_CONTROL
  {IR(16586, 5), 1125}, // 1296 None in SPV_OPERAND_TYPE_SELECTION_CONTROL
  {IR(28206, 29), 1128}, // 1297 IdentifierPossibleDuplicates in SPV_OPERAND_TYPE_SHDEBUG100_BUILD_IDENTIFIER_FLAGS
  {IR(16467, 8), 1130}, // 1298 Address in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16475, 8), 1131}, // 1299 Boolean in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16483, 6), 1132}, // 1300 Float in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16489, 7), 1133}, // 1301 Signed in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16496, 11), 1134}, // 1302 SignedChar in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16507, 9), 1135}, // 1303 Unsigned in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16516, 13), 1136}, // 1304 UnsignedChar in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16455, 12), 1129}, // 1305 Unspecified in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING
  {IR(16529, 6), 1137}, // 1306 Class in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_COMPOSITE_TYPE
  {IR(16535, 10), 1138}, // 1307 Structure in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_COMPOSITE_TYPE
  {IR(16545, 6), 1139}, // 1308 Union in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_COMPOSITE_TYPE
  {IR(16566, 20), 1141}, // 1309 ImportedDeclaration in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_IMPORTED_ENTITY
  {IR(16551, 15), 1140}, // 1310 ImportedModule in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_IMPORTED_ENTITY
  {IR(16675, 15), 1149}, // 1311 FlagArtificial in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {IR(16690, 13), 1150}, // 1312 FlagExplicit in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {IR(16663, 12), 1148}, // 1313 FlagFwdDecl in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {IR(16753, 21), 1154}, // 1314 FlagIndirectVariable in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {IR(16646, 17), 1147}, // 1315 FlagIsDefinition in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {IR(16830, 16), 1158}, // 1316 FlagIsEnumClass in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {IR(16634, 12), 1146}, // 1317 FlagIsLocal in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {IR(16814, 16), 1157}, // 1318 FlagIsOptimized in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {IR(16607, 14), 1144}, // 1319 FlagIsPrivate in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {IR(16591, 16), 1143}, // 1320 FlagIsProtected in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {IR(16621, 13), 1145}, // 1321 FlagIsPublic in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {IR(16774, 20), 1155}, // 1322 FlagLValueReference in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {IR(16718, 18), 1152}, // 1323 FlagObjectPointer in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {IR(16703, 15), 1151}, // 1324 FlagPrototyped in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {IR(16794, 20), 1156}, // 1325 FlagRValueReference in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {IR(16736, 17), 1153}, // 1326 FlagStaticMember in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {IR(16866, 24), 1160}, // 1327 FlagTypePassByReference in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {IR(16846, 20), 1159}, // 1328 FlagTypePassByValue in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {IR(28235, 26), 1161}, // 1329 FlagUnknownPhysicalLayout in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {IR(16586, 5), 1142}, // 1330 None in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS
  {IR(16951, 9), 1166}, // 1331 BitPiece in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION
  {IR(16983, 7), 1170}, // 1332 Constu in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION
  {IR(16890, 6), 1162}, // 1333 Deref in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION
  {IR(16990, 9), 1171}, // 1334 Fragment in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION
  {IR(16901, 6), 1164}, // 1335 Minus in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION
  {IR(16896, 5), 1163}, // 1336 Plus in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION
  {IR(16940, 11), 1165}, // 1337 PlusUconst in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION
  {IR(16972, 11), 1169}, // 1338 StackValue in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION
  {IR(16960, 5), 1167}, // 1339 Swap in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION
  {IR(16965, 7), 1168}, // 1340 Xderef in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION
  {IR(17035, 11), 1175}, // 1341 AtomicType in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_TYPE_QUALIFIER
  {IR(16999, 10), 1172}, // 1342 ConstType in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_TYPE_QUALIFIER
  {IR(17022, 13), 1174}, // 1343 RestrictType in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_TYPE_QUALIFIER
  {IR(17009, 13), 1173}, // 1344 VolatileType in SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_TYPE_QUALIFIER
  {IR(28353, 9), 1191}, // 1345 ApilaJai in SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {IR(28296, 15), 1182}, // 1346 CPP_for_OpenCL in SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {IR(28261, 5), 1177}, // 1347 ESSL in SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {IR(28266, 5), 1178}, // 1348 GLSL in SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {IR(28316, 7), 1184}, // 1349 HERO_C in SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {IR(28291, 5), 1181}, // 1350 HLSL in SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {IR(28323, 5), 1185}, // 1351 NZSL in SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {IR(28271, 9), 1179}, // 1352 OpenCL_C in SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {IR(28280, 11), 1180}, // 1353 OpenCL_CPP in SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {IR(28348, 5), 1190}, // 1354 Pred in SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {IR(28343, 5), 1189}, // 1355 Rust in SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {IR(28311, 5), 1183}, // 1356 SYCL in SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {IR(28333, 6), 1187}, // 1357 Slang in SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {IR(27803, 8), 1176}, // 1358 Unknown in SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {IR(28328, 5), 1186}, // 1359 WGSL in SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {IR(28339, 4), 1188}, // 1360 Zig in SPV_OPERAND_TYPE_SOURCE_LANGUAGE
  {IR(28444, 14), 1202}, // 1361 AtomicCounter in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28526, 16), 1208}, // 1362 CallableDataKHR in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28542, 15), 1208}, // 1363 CallableDataNV in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28862, 17), 1218}, // 1364 CodeSectionINTEL in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28391, 15), 1197}, // 1365 CrossWorkgroup in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28879, 17), 1219}, // 1366 DeviceOnlyALTERA in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28896, 16), 1219}, // 1367 DeviceOnlyINTEL in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28414, 9), 1199}, // 1368 Function in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28423, 8), 1200}, // 1369 Generic in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28631, 16), 1211}, // 1370 HitAttributeKHR in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28647, 15), 1211}, // 1371 HitAttributeNV in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28840, 22), 1217}, // 1372 HitObjectAttributeEXT in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28795, 21), 1215}, // 1373 HitObjectAttributeNV in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28912, 15), 1220}, // 1374 HostOnlyALTERA in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28927, 14), 1220}, // 1375 HostOnlyINTEL in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28458, 6), 1203}, // 1376 Image in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28557, 24), 1209}, // 1377 IncomingCallableDataKHR in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28581, 23), 1209}, // 1378 IncomingCallableDataNV in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28662, 22), 1212}, // 1379 IncomingRayPayloadKHR in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28684, 21), 1212}, // 1380 IncomingRayPayloadNV in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28378, 6), 1193}, // 1381 Input in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28510, 16), 1207}, // 1382 NodePayloadAMDX in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28384, 7), 1195}, // 1383 Output in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28748, 22), 1214}, // 1384 PhysicalStorageBuffer in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28770, 25), 1214}, // 1385 PhysicalStorageBufferEXT in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28406, 8), 1198}, // 1386 Private in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28431, 13), 1201}, // 1387 PushConstant in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28604, 14), 1210}, // 1388 RayPayloadKHR in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28618, 13), 1210}, // 1389 RayPayloadNV in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28705, 22), 1213}, // 1390 ShaderRecordBufferKHR in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28727, 21), 1213}, // 1391 ShaderRecordBufferNV in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28464, 14), 1204}, // 1392 StorageBuffer in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28816, 24), 1216}, // 1393 TaskPayloadWorkgroupEXT in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28491, 19), 1206}, // 1394 TileAttachmentQCOM in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28478, 13), 1205}, // 1395 TileImageEXT in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(17875, 8), 1194}, // 1396 Uniform in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28362, 16), 1192}, // 1397 UniformConstant in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(28115, 10), 1196}, // 1398 Workgroup in SPV_OPERAND_TYPE_STORAGE_CLASS
  {IR(25749, 15), 1224}, // 1399 StreamingINTEL in SPV_OPERAND_TYPE_STORE_CACHE_CONTROL
  {IR(25723, 14), 1221}, // 1400 UncachedINTEL in SPV_OPERAND_TYPE_STORE_CACHE_CONTROL
  {IR(28959, 15), 1223}, // 1401 WriteBackINTEL in SPV_OPERAND_TYPE_STORE_CACHE_CONTROL
  {IR(28941, 18), 1222}, // 1402 WriteThroughINTEL in SPV_OPERAND_TYPE_STORE_CACHE_CONTROL
  {IR(28985, 11), 1227}, // 1403 DecodeFunc in SPV_OPERAND_TYPE_TENSOR_ADDRESSING_OPERANDS
  {IR(28996, 17), 1228}, // 1404 DecodeVectorFunc in SPV_OPERAND_TYPE_TENSOR_ADDRESSING_OPERANDS
  {IR(16586, 5), 1225}, // 1405 None in SPV_OPERAND_TYPE_TENSOR_ADDRESSING_OPERANDS
  {IR(28974, 11), 1226}, // 1406 TensorView in SPV_OPERAND_TYPE_TENSOR_ADDRESSING_OPERANDS
  {IR(27748, 12), 1231}, // 1407 ClampToEdge in SPV_OPERAND_TYPE_TENSOR_CLAMP_MODE
  {IR(17833, 9), 1230}, // 1408 Constant in SPV_OPERAND_TYPE_TENSOR_CLAMP_MODE
  {IR(27766, 7), 1232}, // 1409 Repeat in SPV_OPERAND_TYPE_TENSOR_CLAMP_MODE
  {IR(27773, 15), 1233}, // 1410 RepeatMirrored in SPV_OPERAND_TYPE_TENSOR_CLAMP_MODE
  {IR(29013, 10), 1229}, // 1411 Undefined in SPV_OPERAND_TYPE_TENSOR_CLAMP_MODE
  {IR(29066, 24), 1237}, // 1412 MakeElementAvailableARM in SPV_OPERAND_TYPE_TENSOR_OPERANDS
  {IR(29090, 22), 1238}, // 1413 MakeElementVisibleARM in SPV_OPERAND_TYPE_TENSOR_OPERANDS
  {IR(29112, 21), 1239}, // 1414 NonPrivateElementARM in SPV_OPERAND_TYPE_TENSOR_OPERANDS
  {IR(29023, 8), 1234}, // 1415 NoneARM in SPV_OPERAND_TYPE_TENSOR_OPERANDS
  {IR(29031, 15), 1235}, // 1416 NontemporalARM in SPV_OPERAND_TYPE_TENSOR_OPERANDS
  {IR(29046, 20), 1236}, // 1417 OutOfBoundsValueARM in SPV_OPERAND_TYPE_TENSOR_OPERANDS
}};

IndexRange OperandNameRangeForKind(spv_operand_type_t type) {
  switch(type) {
    case SPV_OPERAND_TYPE_ACCESS_QUALIFIER: return IR(0, 3);
    case SPV_OPERAND_TYPE_ADDRESSING_MODEL: return IR(3, 5);
    case SPV_OPERAND_TYPE_BUILT_IN: return IR(8, 150);
    case SPV_OPERAND_TYPE_CAPABILITY: return IR(158, 352);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING: return IR(510, 8);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_COMPOSITE_TYPE: return IR(518, 3);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_IMPORTED_ENTITY: return IR(521, 2);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS: return IR(523, 19);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION: return IR(542, 10);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_TYPE_QUALIFIER: return IR(552, 4);
    case SPV_OPERAND_TYPE_COMPONENT_TYPE: return IR(556, 15);
    case SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_LAYOUT: return IR(571, 4);
    case SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_OPERANDS: return IR(575, 6);
    case SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_REDUCE: return IR(581, 3);
    case SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_USE: return IR(584, 3);
    case SPV_OPERAND_TYPE_COOPERATIVE_VECTOR_MATRIX_LAYOUT: return IR(587, 4);
    case SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING: return IR(591, 8);
    case SPV_OPERAND_TYPE_DEBUG_COMPOSITE_TYPE: return IR(599, 3);
    case SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS: return IR(602, 16);
    case SPV_OPERAND_TYPE_DEBUG_OPERATION: return IR(618, 9);
    case SPV_OPERAND_TYPE_DEBUG_TYPE_QUALIFIER: return IR(627, 3);
    case SPV_OPERAND_TYPE_DECORATION: return IR(630, 198);
    case SPV_OPERAND_TYPE_DIMENSIONALITY: return IR(828, 8);
    case SPV_OPERAND_TYPE_EXECUTION_MODE: return IR(836, 104);
    case SPV_OPERAND_TYPE_EXECUTION_MODEL: return IR(940, 23);
    case SPV_OPERAND_TYPE_FPDENORM_MODE: return IR(963, 2);
    case SPV_OPERAND_TYPE_FPENCODING: return IR(965, 8);
    case SPV_OPERAND_TYPE_FPOPERATION_MODE: return IR(973, 2);
    case SPV_OPERAND_TYPE_FP_FAST_MATH_MODE: return IR(975, 11);
    case SPV_OPERAND_TYPE_FP_ROUNDING_MODE: return IR(986, 4);
    case SPV_OPERAND_TYPE_FRAGMENT_SHADING_RATE: return IR(990, 4);
    case SPV_OPERAND_TYPE_FUNCTION_CONTROL: return IR(994, 7);
    case SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE: return IR(1001, 10);
    case SPV_OPERAND_TYPE_GATHER_MODES: return IR(1011, 4);
    case SPV_OPERAND_TYPE_GROUP_OPERATION: return IR(1015, 10);
    case SPV_OPERAND_TYPE_HOST_ACCESS_QUALIFIER: return IR(1025, 4);
    case SPV_OPERAND_TYPE_IMAGE: return IR(1029, 21);
    case SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE: return IR(1050, 26);
    case SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER: return IR(1076, 20);
    case SPV_OPERAND_TYPE_INITIALIZATION_MODE_QUALIFIER: return IR(1096, 4);
    case SPV_OPERAND_TYPE_KERNEL_ENQ_FLAGS: return IR(1100, 3);
    case SPV_OPERAND_TYPE_KERNEL_PROFILING_INFO: return IR(1103, 2);
    case SPV_OPERAND_TYPE_KERNEL_PROPERTY_FLAGS: return IR(1105, 2);
    case SPV_OPERAND_TYPE_LINKAGE_TYPE: return IR(1107, 4);
    case SPV_OPERAND_TYPE_LOAD_CACHE_CONTROL: return IR(1111, 5);
    case SPV_OPERAND_TYPE_LOOP_CONTROL: return IR(1116, 31);
    case SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS: return IR(1147, 15);
    case SPV_OPERAND_TYPE_MEMORY_ACCESS: return IR(1162, 12);
    case SPV_OPERAND_TYPE_MEMORY_MODEL: return IR(1174, 5);
    case SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID: return IR(1179, 19);
    case SPV_OPERAND_TYPE_NAMED_MAXIMUM_NUMBER_OF_REGISTERS: return IR(1198, 1);
    case SPV_OPERAND_TYPE_OVERFLOW_MODES: return IR(1199, 4);
    case SPV_OPERAND_TYPE_PACKED_VECTOR_FORMAT: return IR(1203, 2);
    case SPV_OPERAND_TYPE_QUANTIZATION_MODES: return IR(1205, 8);
    case SPV_OPERAND_TYPE_RAW_ACCESS_CHAIN_OPERANDS: return IR(1213, 3);
    case SPV_OPERAND_TYPE_RAY_FLAGS: return IR(1216, 14);
    case SPV_OPERAND_TYPE_RAY_QUERY_CANDIDATE_INTERSECTION_TYPE: return IR(1230, 2);
    case SPV_OPERAND_TYPE_RAY_QUERY_COMMITTED_INTERSECTION_TYPE: return IR(1232, 3);
    case SPV_OPERAND_TYPE_RAY_QUERY_INTERSECTION: return IR(1235, 2);
    case SPV_OPERAND_TYPE_SAMPLER_ADDRESSING_MODE: return IR(1237, 5);
    case SPV_OPERAND_TYPE_SAMPLER_FILTER_MODE: return IR(1242, 2);
    case SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT: return IR(1244, 42);
    case SPV_OPERAND_TYPE_SCOPE_ID: return IR(1286, 8);
    case SPV_OPERAND_TYPE_SELECTION_CONTROL: return IR(1294, 3);
    case SPV_OPERAND_TYPE_SHDEBUG100_BUILD_IDENTIFIER_FLAGS: return IR(1297, 1);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING: return IR(1298, 8);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_COMPOSITE_TYPE: return IR(1306, 3);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_IMPORTED_ENTITY: return IR(1309, 2);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS: return IR(1311, 20);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION: return IR(1331, 10);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_TYPE_QUALIFIER: return IR(1341, 4);
    case SPV_OPERAND_TYPE_SOURCE_LANGUAGE: return IR(1345, 16);
    case SPV_OPERAND_TYPE_STORAGE_CLASS: return IR(1361, 38);
    case SPV_OPERAND_TYPE_STORE_CACHE_CONTROL: return IR(1399, 4);
    case SPV_OPERAND_TYPE_TENSOR_ADDRESSING_OPERANDS: return IR(1403, 4);
    case SPV_OPERAND_TYPE_TENSOR_CLAMP_MODE: return IR(1407, 5);
    case SPV_OPERAND_TYPE_TENSOR_OPERANDS: return IR(1412, 6);
    case SPV_OPERAND_TYPE_OPTIONAL_IMAGE: return IR(1029, 21);
    case SPV_OPERAND_TYPE_OPTIONAL_ACCESS_QUALIFIER: return IR(0, 3);
    case SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS: return IR(1162, 12);
    case SPV_OPERAND_TYPE_OPTIONAL_PACKED_VECTOR_FORMAT: return IR(1203, 2);
    case SPV_OPERAND_TYPE_OPTIONAL_COOPERATIVE_MATRIX_OPERANDS: return IR(575, 6);
    case SPV_OPERAND_TYPE_OPTIONAL_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS: return IR(1147, 15);
    case SPV_OPERAND_TYPE_OPTIONAL_RAW_ACCESS_CHAIN_OPERANDS: return IR(1213, 3);
    case SPV_OPERAND_TYPE_OPTIONAL_FPENCODING: return IR(965, 8);
    case SPV_OPERAND_TYPE_OPTIONAL_TENSOR_OPERANDS: return IR(1412, 6);
    case SPV_OPERAND_TYPE_OPTIONAL_CAPABILITY: return IR(158, 352);
    default: break;
  }
  return IR(0,0);
}

// Instruction descriptions, ordered by opcode.
// The fields in order are:
//   opcode
//   a boolean indicating if the instruction produces a result ID
//   a boolean indicating if the instruction result ID has a type
//   operands, an IndexRange into kOperandSpans
//   opcode name (without the 'Op' prefix), a character-counting IndexRange into kStrings
//   aliases, an IndexRange into kAliasSpans
//   capabilities, an IndexRange into kCapabilitySpans
//   extensions, as an IndexRange into kExtensionSpans
//   version, first version of SPIR-V that has it
//   lastVersion, last version of SPIR-V that has it
static const std::array<InstructionDesc, 875> kInstructionDesc{{
  {spv::Op::OpNop, false, false, IR(0, 0), IR(29133, 4), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMiscellaneous},
  {spv::Op::OpUndef, true, true, IR(39, 2), IR(29189, 6), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMiscellaneous},
  {spv::Op::OpSourceContinued, false, false, IR(11, 1), IR(29195, 16), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDebug},
  {spv::Op::OpSource, false, false, IR(41, 4), IR(29314, 7), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDebug},
  {spv::Op::OpSourceExtension, false, false, IR(11, 1), IR(29321, 16), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDebug},
  {spv::Op::OpName, false, false, IR(45, 2), IR(29337, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDebug},
  {spv::Op::OpMemberName, false, false, IR(47, 3), IR(29342, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDebug},
  {spv::Op::OpString, true, false, IR(50, 2), IR(29353, 7), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDebug},
  {spv::Op::OpLine, false, false, IR(52, 3), IR(29360, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDebug},
  {spv::Op::OpExtension, false, false, IR(11, 1), IR(29365, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kExtension},
  {spv::Op::OpExtInstImport, true, false, IR(50, 2), IR(29375, 14), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kExtension},
  {spv::Op::OpExtInst, true, true, IR(55, 4), IR(29435, 8), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kExtension},
  {spv::Op::OpMemoryModel, false, false, IR(59, 2), IR(29507, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMode_Setting},
  {spv::Op::OpEntryPoint, false, false, IR(61, 4), IR(29581, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMode_Setting},
  {spv::Op::OpExecutionMode, false, false, IR(65, 2), IR(29624, 14), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMode_Setting},
  {spv::Op::OpCapability, false, false, IR(67, 1), IR(29666, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMode_Setting},
  {spv::Op::OpTypeVoid, true, false, IR(68, 1), IR(29677, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeBool, true, false, IR(68, 1), IR(29686, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeInt, true, false, IR(69, 3), IR(29695, 8), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeFloat, true, false, IR(72, 3), IR(29740, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeVector, true, false, IR(75, 3), IR(29750, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeMatrix, true, false, IR(75, 3), IR(29761, 11), IR(0, 0), IR(67, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeImage, true, false, IR(78, 9), IR(29885, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeSampler, true, false, IR(68, 1), IR(29895, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeSampledImage, true, false, IR(87, 2), IR(29907, 17), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeArray, true, false, IR(89, 3), IR(29924, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeRuntimeArray, true, false, IR(87, 2), IR(29934, 17), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeStruct, true, false, IR(92, 2), IR(29951, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeOpaque, true, false, IR(50, 2), IR(29962, 11), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypePointer, true, false, IR(94, 3), IR(30004, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeFunction, true, false, IR(97, 3), IR(30016, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeEvent, true, false, IR(68, 1), IR(30029, 10), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeDeviceEvent, true, false, IR(68, 1), IR(30039, 16), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeReserveId, true, false, IR(68, 1), IR(30055, 14), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeQueue, true, false, IR(68, 1), IR(30069, 10), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypePipe, true, false, IR(100, 2), IR(30079, 9), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeForwardPointer, false, false, IR(102, 2), IR(30088, 19), IR(0, 0), IR(219, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpConstantTrue, true, true, IR(39, 2), IR(30107, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpConstantFalse, true, true, IR(39, 2), IR(30120, 14), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpConstant, true, true, IR(104, 3), IR(17833, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpConstantComposite, true, true, IR(107, 3), IR(30172, 18), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpConstantSampler, true, true, IR(110, 5), IR(30268, 16), IR(0, 0), IR(221, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpConstantNull, true, true, IR(39, 2), IR(30284, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpSpecConstantTrue, true, true, IR(39, 2), IR(30297, 17), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpSpecConstantFalse, true, true, IR(39, 2), IR(30314, 18), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpSpecConstant, true, true, IR(104, 3), IR(30332, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpSpecConstantComposite, true, true, IR(107, 3), IR(30345, 22), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpSpecConstantOp, true, true, IR(115, 3), IR(30408, 15), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpFunction, true, true, IR(118, 4), IR(28414, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kFunction},
  {spv::Op::OpFunctionParameter, true, true, IR(39, 2), IR(30457, 18), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kFunction},
  {spv::Op::OpFunctionEnd, false, false, IR(0, 0), IR(30475, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kFunction},
  {spv::Op::OpFunctionCall, true, true, IR(122, 4), IR(30487, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kFunction},
  {spv::Op::OpVariable, true, true, IR(126, 4), IR(30500, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpImageTexelPointer, true, true, IR(130, 5), IR(30509, 18), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpLoad, true, true, IR(135, 4), IR(30567, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpStore, false, false, IR(139, 3), IR(30572, 6), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpCopyMemory, false, false, IR(142, 4), IR(30578, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpCopyMemorySized, false, false, IR(146, 5), IR(30589, 16), IR(0, 0), IR(222, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpAccessChain, true, true, IR(122, 4), IR(30605, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpInBoundsAccessChain, true, true, IR(122, 4), IR(30617, 20), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpPtrAccessChain, true, true, IR(151, 5), IR(30637, 15), IR(0, 0), IR(224, 4), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpArrayLength, true, true, IR(156, 4), IR(30652, 12), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpGenericPtrMemSemantics, true, true, IR(160, 3), IR(30664, 23), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpInBoundsPtrAccessChain, true, true, IR(151, 5), IR(30687, 23), IR(0, 0), IR(1, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpDecorate, false, false, IR(163, 2), IR(30738, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAnnotation},
  {spv::Op::OpMemberDecorate, false, false, IR(165, 3), IR(30747, 15), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAnnotation},
  {spv::Op::OpDecorationGroup, true, false, IR(68, 1), IR(30762, 16), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAnnotation},
  {spv::Op::OpGroupDecorate, false, false, IR(168, 2), IR(30778, 14), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAnnotation},
  {spv::Op::OpGroupMemberDecorate, false, false, IR(170, 2), IR(30837, 20), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAnnotation},
  {spv::Op::OpVectorExtractDynamic, true, true, IR(172, 4), IR(30857, 21), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpVectorInsertDynamic, true, true, IR(130, 5), IR(30878, 20), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpVectorShuffle, true, true, IR(176, 5), IR(30898, 14), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpCompositeConstruct, true, true, IR(107, 3), IR(30912, 19), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpCompositeExtract, true, true, IR(181, 4), IR(30931, 17), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpCompositeInsert, true, true, IR(176, 5), IR(30948, 16), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpCopyObject, true, true, IR(160, 3), IR(30964, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpTranspose, true, true, IR(160, 3), IR(30975, 10), IR(0, 0), IR(67, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpSampledImage, true, true, IR(172, 4), IR(30985, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSampleImplicitLod, true, true, IR(185, 5), IR(31030, 23), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSampleExplicitLod, true, true, IR(190, 5), IR(31076, 23), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSampleDrefImplicitLod, true, true, IR(195, 6), IR(31099, 27), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSampleDrefExplicitLod, true, true, IR(201, 6), IR(31126, 27), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSampleProjImplicitLod, true, true, IR(185, 5), IR(31153, 27), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSampleProjExplicitLod, true, true, IR(190, 5), IR(31180, 27), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSampleProjDrefImplicitLod, true, true, IR(195, 6), IR(31207, 31), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSampleProjDrefExplicitLod, true, true, IR(201, 6), IR(31238, 31), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageFetch, true, true, IR(185, 5), IR(31269, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageGather, true, true, IR(195, 6), IR(31280, 12), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageDrefGather, true, true, IR(195, 6), IR(31292, 16), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageRead, true, true, IR(185, 5), IR(31308, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageWrite, false, false, IR(207, 4), IR(31318, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImage, true, true, IR(160, 3), IR(28458, 6), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageQueryFormat, true, true, IR(160, 3), IR(31329, 17), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageQueryOrder, true, true, IR(160, 3), IR(31346, 16), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageQuerySizeLod, true, true, IR(172, 4), IR(31362, 18), IR(0, 0), IR(228, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageQuerySize, true, true, IR(160, 3), IR(31380, 15), IR(0, 0), IR(228, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageQueryLod, true, true, IR(172, 4), IR(31395, 14), IR(0, 0), IR(230, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageQueryLevels, true, true, IR(160, 3), IR(31409, 17), IR(0, 0), IR(228, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageQuerySamples, true, true, IR(160, 3), IR(31426, 18), IR(0, 0), IR(228, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpConvertFToU, true, true, IR(160, 3), IR(31444, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpConvertFToS, true, true, IR(160, 3), IR(31456, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpConvertSToF, true, true, IR(160, 3), IR(31468, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpConvertUToF, true, true, IR(160, 3), IR(31480, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpUConvert, true, true, IR(160, 3), IR(31492, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpSConvert, true, true, IR(160, 3), IR(31501, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpFConvert, true, true, IR(160, 3), IR(31510, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpQuantizeToF16, true, true, IR(160, 3), IR(31519, 14), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpConvertPtrToU, true, true, IR(160, 3), IR(31533, 14), IR(0, 0), IR(219, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpSatConvertSToU, true, true, IR(160, 3), IR(31547, 15), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpSatConvertUToS, true, true, IR(160, 3), IR(31562, 15), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpConvertUToPtr, true, true, IR(160, 3), IR(31577, 14), IR(0, 0), IR(219, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpPtrCastToGeneric, true, true, IR(160, 3), IR(31591, 17), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpGenericCastToPtr, true, true, IR(160, 3), IR(31608, 17), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpGenericCastToPtrExplicit, true, true, IR(211, 4), IR(31625, 25), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpBitcast, true, true, IR(160, 3), IR(31650, 8), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpSNegate, true, true, IR(160, 3), IR(31658, 8), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpFNegate, true, true, IR(160, 3), IR(31666, 8), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpIAdd, true, true, IR(172, 4), IR(31674, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpFAdd, true, true, IR(172, 4), IR(31679, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpISub, true, true, IR(172, 4), IR(31684, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpFSub, true, true, IR(172, 4), IR(31689, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpIMul, true, true, IR(172, 4), IR(31694, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpFMul, true, true, IR(172, 4), IR(31699, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpUDiv, true, true, IR(172, 4), IR(31704, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpSDiv, true, true, IR(172, 4), IR(31709, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpFDiv, true, true, IR(172, 4), IR(31714, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpUMod, true, true, IR(172, 4), IR(31719, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpSRem, true, true, IR(172, 4), IR(31724, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpSMod, true, true, IR(172, 4), IR(31729, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpFRem, true, true, IR(172, 4), IR(31734, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpFMod, true, true, IR(172, 4), IR(31739, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpVectorTimesScalar, true, true, IR(172, 4), IR(31744, 18), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpMatrixTimesScalar, true, true, IR(172, 4), IR(31762, 18), IR(0, 0), IR(67, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpVectorTimesMatrix, true, true, IR(172, 4), IR(31780, 18), IR(0, 0), IR(67, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpMatrixTimesVector, true, true, IR(172, 4), IR(31798, 18), IR(0, 0), IR(67, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpMatrixTimesMatrix, true, true, IR(172, 4), IR(31816, 18), IR(0, 0), IR(67, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpOuterProduct, true, true, IR(172, 4), IR(31834, 13), IR(0, 0), IR(67, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpDot, true, true, IR(172, 4), IR(31847, 4), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpIAddCarry, true, true, IR(172, 4), IR(31851, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpISubBorrow, true, true, IR(172, 4), IR(31861, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpUMulExtended, true, true, IR(172, 4), IR(31872, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpSMulExtended, true, true, IR(172, 4), IR(31885, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpAny, true, true, IR(160, 3), IR(31898, 4), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpAll, true, true, IR(160, 3), IR(31902, 4), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpIsNan, true, true, IR(160, 3), IR(31906, 6), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpIsInf, true, true, IR(160, 3), IR(31912, 6), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpIsFinite, true, true, IR(160, 3), IR(31918, 9), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpIsNormal, true, true, IR(160, 3), IR(31927, 9), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpSignBitSet, true, true, IR(160, 3), IR(31936, 11), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpLessOrGreater, true, true, IR(172, 4), IR(31947, 14), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), SPV_SPIRV_VERSION_WORD(1,5), PrintingClass::kRelational_and_Logical},
  {spv::Op::OpOrdered, true, true, IR(172, 4), IR(31961, 8), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpUnordered, true, true, IR(172, 4), IR(31969, 10), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpLogicalEqual, true, true, IR(172, 4), IR(31979, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpLogicalNotEqual, true, true, IR(172, 4), IR(31992, 16), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpLogicalOr, true, true, IR(172, 4), IR(32008, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpLogicalAnd, true, true, IR(172, 4), IR(32018, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpLogicalNot, true, true, IR(160, 3), IR(32029, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpSelect, true, true, IR(130, 5), IR(32040, 7), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpIEqual, true, true, IR(172, 4), IR(32047, 7), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpINotEqual, true, true, IR(172, 4), IR(32054, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpUGreaterThan, true, true, IR(172, 4), IR(32064, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpSGreaterThan, true, true, IR(172, 4), IR(32077, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpUGreaterThanEqual, true, true, IR(172, 4), IR(32090, 18), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpSGreaterThanEqual, true, true, IR(172, 4), IR(32108, 18), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpULessThan, true, true, IR(172, 4), IR(32126, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpSLessThan, true, true, IR(172, 4), IR(32136, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpULessThanEqual, true, true, IR(172, 4), IR(32146, 15), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpSLessThanEqual, true, true, IR(172, 4), IR(32161, 15), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFOrdEqual, true, true, IR(172, 4), IR(32176, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFUnordEqual, true, true, IR(172, 4), IR(32186, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFOrdNotEqual, true, true, IR(172, 4), IR(32198, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFUnordNotEqual, true, true, IR(172, 4), IR(32211, 15), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFOrdLessThan, true, true, IR(172, 4), IR(32226, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFUnordLessThan, true, true, IR(172, 4), IR(32239, 15), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFOrdGreaterThan, true, true, IR(172, 4), IR(32254, 16), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFUnordGreaterThan, true, true, IR(172, 4), IR(32270, 18), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFOrdLessThanEqual, true, true, IR(172, 4), IR(32288, 18), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFUnordLessThanEqual, true, true, IR(172, 4), IR(32306, 20), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFOrdGreaterThanEqual, true, true, IR(172, 4), IR(32326, 21), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFUnordGreaterThanEqual, true, true, IR(172, 4), IR(32347, 23), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpShiftRightLogical, true, true, IR(172, 4), IR(32370, 18), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpShiftRightArithmetic, true, true, IR(172, 4), IR(32388, 21), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpShiftLeftLogical, true, true, IR(172, 4), IR(32409, 17), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpBitwiseOr, true, true, IR(172, 4), IR(32426, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpBitwiseXor, true, true, IR(172, 4), IR(32436, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpBitwiseAnd, true, true, IR(172, 4), IR(32447, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpNot, true, true, IR(160, 3), IR(32458, 4), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpBitFieldInsert, true, true, IR(215, 6), IR(32462, 15), IR(0, 0), IR(231, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpBitFieldSExtract, true, true, IR(130, 5), IR(32477, 17), IR(0, 0), IR(231, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpBitFieldUExtract, true, true, IR(130, 5), IR(32494, 17), IR(0, 0), IR(231, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpBitReverse, true, true, IR(160, 3), IR(32511, 11), IR(0, 0), IR(231, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpBitCount, true, true, IR(160, 3), IR(32522, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpDPdx, true, true, IR(160, 3), IR(32531, 5), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDerivative},
  {spv::Op::OpDPdy, true, true, IR(160, 3), IR(32536, 5), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDerivative},
  {spv::Op::OpFwidth, true, true, IR(160, 3), IR(32541, 7), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDerivative},
  {spv::Op::OpDPdxFine, true, true, IR(160, 3), IR(32548, 9), IR(0, 0), IR(233, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDerivative},
  {spv::Op::OpDPdyFine, true, true, IR(160, 3), IR(32557, 9), IR(0, 0), IR(233, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDerivative},
  {spv::Op::OpFwidthFine, true, true, IR(160, 3), IR(32566, 11), IR(0, 0), IR(233, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDerivative},
  {spv::Op::OpDPdxCoarse, true, true, IR(160, 3), IR(32577, 11), IR(0, 0), IR(233, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDerivative},
  {spv::Op::OpDPdyCoarse, true, true, IR(160, 3), IR(32588, 11), IR(0, 0), IR(233, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDerivative},
  {spv::Op::OpFwidthCoarse, true, true, IR(160, 3), IR(32599, 13), IR(0, 0), IR(233, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDerivative},
  {spv::Op::OpEmitVertex, false, false, IR(0, 0), IR(32612, 11), IR(0, 0), IR(70, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPrimitive},
  {spv::Op::OpEndPrimitive, false, false, IR(0, 0), IR(32623, 13), IR(0, 0), IR(70, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPrimitive},
  {spv::Op::OpEmitStreamVertex, false, false, IR(10, 1), IR(32636, 17), IR(0, 0), IR(123, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPrimitive},
  {spv::Op::OpEndStreamPrimitive, false, false, IR(10, 1), IR(32653, 19), IR(0, 0), IR(123, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPrimitive},
  {spv::Op::OpControlBarrier, false, false, IR(221, 3), IR(32709, 15), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBarrier},
  {spv::Op::OpMemoryBarrier, false, false, IR(224, 2), IR(32724, 14), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBarrier},
  {spv::Op::OpAtomicLoad, true, true, IR(226, 5), IR(32738, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicStore, false, false, IR(231, 4), IR(32749, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicExchange, true, true, IR(235, 6), IR(32761, 15), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicCompareExchange, true, true, IR(241, 8), IR(32776, 22), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicCompareExchangeWeak, true, true, IR(241, 8), IR(32798, 26), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), SPV_SPIRV_VERSION_WORD(1,3), PrintingClass::kAtomic},
  {spv::Op::OpAtomicIIncrement, true, true, IR(226, 5), IR(32824, 17), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicIDecrement, true, true, IR(226, 5), IR(32841, 17), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicIAdd, true, true, IR(235, 6), IR(32858, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicISub, true, true, IR(235, 6), IR(32869, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicSMin, true, true, IR(235, 6), IR(32880, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicUMin, true, true, IR(235, 6), IR(32891, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicSMax, true, true, IR(235, 6), IR(32902, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicUMax, true, true, IR(235, 6), IR(32913, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicAnd, true, true, IR(235, 6), IR(32924, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicOr, true, true, IR(235, 6), IR(32934, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicXor, true, true, IR(235, 6), IR(32943, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpPhi, true, true, IR(107, 3), IR(32953, 4), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpLoopMerge, false, false, IR(249, 3), IR(32987, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpSelectionMerge, false, false, IR(252, 2), IR(33032, 15), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpLabel, true, false, IR(68, 1), IR(33047, 6), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpBranch, false, false, IR(10, 1), IR(33053, 7), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpBranchConditional, false, false, IR(254, 4), IR(33060, 18), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpSwitch, false, false, IR(258, 3), IR(33123, 7), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpKill, false, false, IR(0, 0), IR(33130, 5), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpReturn, false, false, IR(0, 0), IR(33135, 7), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpReturnValue, false, false, IR(10, 1), IR(33142, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpUnreachable, false, false, IR(0, 0), IR(33154, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpLifetimeStart, false, false, IR(261, 2), IR(33166, 14), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpLifetimeStop, false, false, IR(261, 2), IR(33180, 13), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpGroupAsyncCopy, true, true, IR(263, 8), IR(33193, 15), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupWaitEvents, false, false, IR(271, 3), IR(33208, 16), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupAll, true, true, IR(274, 4), IR(33224, 9), IR(0, 0), IR(234, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupAny, true, true, IR(274, 4), IR(33233, 9), IR(0, 0), IR(234, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupBroadcast, true, true, IR(278, 5), IR(33242, 15), IR(0, 0), IR(234, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupIAdd, true, true, IR(283, 5), IR(33290, 10), IR(0, 0), IR(234, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupFAdd, true, true, IR(283, 5), IR(33300, 10), IR(0, 0), IR(234, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupFMin, true, true, IR(283, 5), IR(33310, 10), IR(0, 0), IR(234, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupUMin, true, true, IR(283, 5), IR(33320, 10), IR(0, 0), IR(234, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupSMin, true, true, IR(283, 5), IR(33330, 10), IR(0, 0), IR(234, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupFMax, true, true, IR(283, 5), IR(33340, 10), IR(0, 0), IR(234, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupUMax, true, true, IR(283, 5), IR(33350, 10), IR(0, 0), IR(234, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupSMax, true, true, IR(283, 5), IR(33360, 10), IR(0, 0), IR(234, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpReadPipe, true, true, IR(215, 6), IR(33370, 9), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpWritePipe, true, true, IR(215, 6), IR(33379, 10), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpReservedReadPipe, true, true, IR(288, 8), IR(33389, 17), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpReservedWritePipe, true, true, IR(288, 8), IR(33406, 18), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpReserveReadPipePackets, true, true, IR(215, 6), IR(33424, 23), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpReserveWritePipePackets, true, true, IR(215, 6), IR(33447, 24), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpCommitReadPipe, false, false, IR(296, 4), IR(33471, 15), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpCommitWritePipe, false, false, IR(296, 4), IR(33486, 16), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpIsValidReserveId, true, true, IR(160, 3), IR(33502, 17), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpGetNumPipePackets, true, true, IR(130, 5), IR(33519, 18), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpGetMaxPipePackets, true, true, IR(130, 5), IR(33537, 18), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpGroupReserveReadPipePackets, true, true, IR(300, 7), IR(33555, 28), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpGroupReserveWritePipePackets, true, true, IR(300, 7), IR(33583, 29), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpGroupCommitReadPipe, false, false, IR(307, 5), IR(33612, 20), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpGroupCommitWritePipe, false, false, IR(307, 5), IR(33632, 21), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpEnqueueMarker, true, true, IR(215, 6), IR(33653, 14), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpEnqueueKernel, true, true, IR(312, 13), IR(33667, 14), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpGetKernelNDrangeSubGroupCount, true, true, IR(325, 7), IR(33681, 30), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpGetKernelNDrangeMaxSubGroupSize, true, true, IR(325, 7), IR(33711, 32), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpGetKernelWorkGroupSize, true, true, IR(215, 6), IR(33743, 23), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpGetKernelPreferredWorkGroupSizeMultiple, true, true, IR(215, 6), IR(33766, 40), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpRetainEvent, false, false, IR(10, 1), IR(33806, 12), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpReleaseEvent, false, false, IR(10, 1), IR(33818, 13), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpCreateUserEvent, true, true, IR(39, 2), IR(33831, 16), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpIsValidEvent, true, true, IR(160, 3), IR(33847, 13), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpSetUserEventStatus, false, false, IR(36, 2), IR(33860, 19), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpCaptureEventProfilingInfo, false, false, IR(33, 3), IR(33879, 26), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpGetDefaultQueue, true, true, IR(39, 2), IR(33905, 16), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpBuildNDRange, true, true, IR(130, 5), IR(33921, 13), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpImageSparseSampleImplicitLod, true, true, IR(185, 5), IR(33934, 29), IR(0, 0), IR(235, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSparseSampleExplicitLod, true, true, IR(190, 5), IR(33963, 29), IR(0, 0), IR(235, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSparseSampleDrefImplicitLod, true, true, IR(195, 6), IR(33992, 33), IR(0, 0), IR(235, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSparseSampleDrefExplicitLod, true, true, IR(201, 6), IR(34025, 33), IR(0, 0), IR(235, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSparseSampleProjImplicitLod, true, true, IR(185, 5), IR(34058, 33), IR(0, 0), IR(235, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSparseSampleProjExplicitLod, true, true, IR(190, 5), IR(34091, 33), IR(0, 0), IR(235, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSparseSampleProjDrefImplicitLod, true, true, IR(195, 6), IR(34124, 37), IR(0, 0), IR(235, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSparseSampleProjDrefExplicitLod, true, true, IR(201, 6), IR(34161, 37), IR(0, 0), IR(235, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSparseFetch, true, true, IR(185, 5), IR(34198, 17), IR(0, 0), IR(235, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSparseGather, true, true, IR(195, 6), IR(34215, 18), IR(0, 0), IR(235, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSparseDrefGather, true, true, IR(195, 6), IR(34233, 22), IR(0, 0), IR(235, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSparseTexelsResident, true, true, IR(160, 3), IR(34255, 26), IR(0, 0), IR(235, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpNoLine, false, false, IR(0, 0), IR(34281, 7), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDebug},
  {spv::Op::OpAtomicFlagTestAndSet, true, true, IR(226, 5), IR(34288, 21), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicFlagClear, false, false, IR(332, 3), IR(34309, 16), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpImageSparseRead, true, true, IR(185, 5), IR(34325, 16), IR(0, 0), IR(235, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpSizeOf, true, true, IR(160, 3), IR(34341, 7), IR(0, 0), IR(1, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,1), 0xffffffffu, PrintingClass::kMiscellaneous},
  {spv::Op::OpTypePipeStorage, true, false, IR(68, 1), IR(34348, 16), IR(0, 0), IR(236, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,1), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpConstantPipeStorage, true, true, IR(335, 5), IR(34364, 20), IR(0, 0), IR(236, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,1), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpCreatePipeFromPipeStorage, true, true, IR(160, 3), IR(34384, 26), IR(0, 0), IR(236, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,1), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpGetKernelLocalSizeForSubgroupCount, true, true, IR(325, 7), IR(34410, 35), IR(0, 0), IR(163, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,1), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpGetKernelMaxNumSubgroups, true, true, IR(215, 6), IR(34445, 25), IR(0, 0), IR(163, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,1), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpTypeNamedBarrier, true, false, IR(68, 1), IR(34470, 17), IR(0, 0), IR(237, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,1), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpNamedBarrierInitialize, true, true, IR(160, 3), IR(34487, 23), IR(0, 0), IR(237, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,1), 0xffffffffu, PrintingClass::kBarrier},
  {spv::Op::OpMemoryNamedBarrier, false, false, IR(332, 3), IR(34510, 19), IR(0, 0), IR(237, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,1), 0xffffffffu, PrintingClass::kBarrier},
  {spv::Op::OpModuleProcessed, false, false, IR(11, 1), IR(34529, 16), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,1), 0xffffffffu, PrintingClass::kDebug},
  {spv::Op::OpExecutionModeId, false, false, IR(65, 2), IR(34545, 16), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,2), 0xffffffffu, PrintingClass::kMode_Setting},
  {spv::Op::OpDecorateId, false, false, IR(163, 2), IR(34561, 11), IR(0, 0), IR(1, 0), IR(198, 1), SPV_SPIRV_VERSION_WORD(1,2), 0xffffffffu, PrintingClass::kAnnotation},
  {spv::Op::OpGroupNonUniformElect, true, true, IR(340, 3), IR(34572, 21), IR(0, 0), IR(77, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformAll, true, true, IR(274, 4), IR(34593, 19), IR(0, 0), IR(238, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformAny, true, true, IR(274, 4), IR(34612, 19), IR(0, 0), IR(238, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformAllEqual, true, true, IR(274, 4), IR(34631, 24), IR(0, 0), IR(238, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformBroadcast, true, true, IR(278, 5), IR(34655, 25), IR(0, 0), IR(239, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformBroadcastFirst, true, true, IR(274, 4), IR(34680, 30), IR(0, 0), IR(239, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformBallot, true, true, IR(274, 4), IR(6825, 22), IR(0, 0), IR(239, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformInverseBallot, true, true, IR(274, 4), IR(34710, 29), IR(0, 0), IR(239, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformBallotBitExtract, true, true, IR(278, 5), IR(34739, 32), IR(0, 0), IR(239, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformBallotBitCount, true, true, IR(283, 5), IR(34771, 30), IR(0, 0), IR(239, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformBallotFindLSB, true, true, IR(274, 4), IR(34801, 29), IR(0, 0), IR(239, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformBallotFindMSB, true, true, IR(274, 4), IR(34830, 29), IR(0, 0), IR(239, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformShuffle, true, true, IR(278, 5), IR(9964, 23), IR(0, 0), IR(240, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformShuffleXor, true, true, IR(278, 5), IR(34859, 26), IR(0, 0), IR(240, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformShuffleUp, true, true, IR(278, 5), IR(34885, 25), IR(0, 0), IR(241, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformShuffleDown, true, true, IR(278, 5), IR(34910, 27), IR(0, 0), IR(241, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformIAdd, true, true, IR(343, 6), IR(34937, 20), IR(0, 0), IR(242, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformFAdd, true, true, IR(343, 6), IR(34957, 20), IR(0, 0), IR(242, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformIMul, true, true, IR(343, 6), IR(34977, 20), IR(0, 0), IR(242, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformFMul, true, true, IR(343, 6), IR(34997, 20), IR(0, 0), IR(242, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformSMin, true, true, IR(343, 6), IR(35017, 20), IR(0, 0), IR(242, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformUMin, true, true, IR(343, 6), IR(35037, 20), IR(0, 0), IR(242, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformFMin, true, true, IR(343, 6), IR(35057, 20), IR(0, 0), IR(242, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformSMax, true, true, IR(343, 6), IR(35077, 20), IR(0, 0), IR(242, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformUMax, true, true, IR(343, 6), IR(35097, 20), IR(0, 0), IR(242, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformFMax, true, true, IR(343, 6), IR(35117, 20), IR(0, 0), IR(242, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformBitwiseAnd, true, true, IR(343, 6), IR(35137, 26), IR(0, 0), IR(242, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformBitwiseOr, true, true, IR(343, 6), IR(35163, 25), IR(0, 0), IR(242, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformBitwiseXor, true, true, IR(343, 6), IR(35188, 26), IR(0, 0), IR(242, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformLogicalAnd, true, true, IR(343, 6), IR(35214, 26), IR(0, 0), IR(242, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformLogicalOr, true, true, IR(343, 6), IR(35240, 25), IR(0, 0), IR(242, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformLogicalXor, true, true, IR(343, 6), IR(35265, 26), IR(0, 0), IR(242, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformQuadBroadcast, true, true, IR(278, 5), IR(35291, 29), IR(0, 0), IR(245, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformQuadSwap, true, true, IR(278, 5), IR(35320, 24), IR(0, 0), IR(245, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpCopyLogical, true, true, IR(160, 3), IR(35344, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,4), 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpPtrEqual, true, true, IR(172, 4), IR(35356, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,4), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpPtrNotEqual, true, true, IR(172, 4), IR(35365, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,4), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpPtrDiff, true, true, IR(172, 4), IR(35377, 8), IR(0, 0), IR(246, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,4), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpColorAttachmentReadEXT, true, true, IR(349, 4), IR(35385, 23), IR(0, 0), IR(155, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpDepthAttachmentReadEXT, true, true, IR(353, 3), IR(35408, 23), IR(0, 0), IR(164, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpStencilAttachmentReadEXT, true, true, IR(353, 3), IR(35431, 25), IR(0, 0), IR(165, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpTypeTensorARM, true, false, IR(356, 4), IR(35456, 14), IR(0, 0), IR(218, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTensorReadARM, true, true, IR(360, 5), IR(35512, 14), IR(0, 0), IR(218, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kTensor},
  {spv::Op::OpTensorWriteARM, false, false, IR(365, 4), IR(35526, 15), IR(0, 0), IR(218, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kTensor},
  {spv::Op::OpTensorQuerySizeARM, true, true, IR(172, 4), IR(35541, 19), IR(0, 0), IR(218, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kTensor},
  {spv::Op::OpGraphConstantARM, true, true, IR(369, 3), IR(35560, 17), IR(0, 0), IR(249, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGraph},
  {spv::Op::OpGraphEntryPointARM, false, false, IR(372, 3), IR(35577, 19), IR(0, 0), IR(249, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGraph},
  {spv::Op::OpGraphARM, true, true, IR(39, 2), IR(10255, 9), IR(0, 0), IR(249, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGraph},
  {spv::Op::OpGraphInputARM, true, true, IR(122, 4), IR(35596, 14), IR(0, 0), IR(249, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGraph},
  {spv::Op::OpGraphSetOutputARM, false, false, IR(375, 3), IR(35610, 18), IR(0, 0), IR(249, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGraph},
  {spv::Op::OpGraphEndARM, false, false, IR(0, 0), IR(35628, 12), IR(0, 0), IR(249, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGraph},
  {spv::Op::OpTypeGraphARM, true, false, IR(378, 3), IR(35640, 13), IR(0, 0), IR(249, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpBitcastExtractEXT, true, true, IR(172, 4), IR(10402, 18), IR(0, 0), IR(250, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpTerminateInvocation, false, false, IR(0, 0), IR(35653, 20), IR(0, 0), IR(3, 1), IR(208, 1), SPV_SPIRV_VERSION_WORD(1,6), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpTypeUntypedPointerKHR, true, false, IR(381, 2), IR(35673, 22), IR(0, 0), IR(93, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpUntypedVariableKHR, true, true, IR(383, 5), IR(35695, 19), IR(0, 0), IR(93, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpUntypedAccessChainKHR, true, true, IR(151, 5), IR(35714, 22), IR(0, 0), IR(93, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpUntypedInBoundsAccessChainKHR, true, true, IR(151, 5), IR(35736, 30), IR(0, 0), IR(93, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpSubgroupBallotKHR, true, true, IR(160, 3), IR(6586, 18), IR(0, 0), IR(251, 1), IR(2, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupFirstInvocationKHR, true, true, IR(160, 3), IR(35766, 27), IR(0, 0), IR(251, 1), IR(2, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpUntypedPtrAccessChainKHR, true, true, IR(388, 6), IR(35793, 25), IR(0, 0), IR(93, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpUntypedInBoundsPtrAccessChainKHR, true, true, IR(388, 6), IR(35818, 33), IR(0, 0), IR(93, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpUntypedArrayLengthKHR, true, true, IR(394, 5), IR(35851, 22), IR(0, 0), IR(93, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpUntypedPrefetchKHR, false, false, IR(399, 5), IR(35873, 19), IR(0, 0), IR(93, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpFmaKHR, true, true, IR(130, 5), IR(35892, 7), IR(0, 0), IR(252, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpSubgroupAllKHR, true, true, IR(160, 3), IR(35899, 15), IR(0, 0), IR(253, 1), IR(42, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupAnyKHR, true, true, IR(160, 3), IR(35914, 15), IR(0, 0), IR(253, 1), IR(42, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupAllEqualKHR, true, true, IR(160, 3), IR(35929, 20), IR(0, 0), IR(253, 1), IR(42, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupNonUniformRotateKHR, true, true, IR(404, 6), IR(15262, 25), IR(0, 0), IR(254, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupReadInvocationKHR, true, true, IR(172, 4), IR(35949, 26), IR(0, 0), IR(251, 1), IR(2, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpExtInstWithForwardRefsKHR, true, true, IR(410, 5), IR(35975, 26), IR(0, 0), IR(1, 0), IR(209, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kExtension},
  {spv::Op::OpUntypedGroupAsyncCopyKHR, true, true, IR(415, 11), IR(36001, 25), IR(0, 0), IR(93, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpTraceRayKHR, false, false, IR(426, 11), IR(36026, 12), IR(0, 0), IR(61, 1), IR(28, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpExecuteCallableKHR, false, false, IR(36, 2), IR(36038, 19), IR(0, 0), IR(61, 1), IR(28, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpConvertUToAccelerationStructureKHR, true, true, IR(160, 3), IR(36057, 35), IR(0, 0), IR(255, 2), IR(210, 2), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpIgnoreIntersectionKHR, false, false, IR(0, 0), IR(36092, 22), IR(0, 0), IR(61, 1), IR(28, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTerminateRayKHR, false, false, IR(0, 0), IR(36114, 16), IR(0, 0), IR(61, 1), IR(28, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpSDot, true, true, IR(437, 5), IR(36177, 5), IR(174, 1), IR(257, 1), IR(153, 1), SPV_SPIRV_VERSION_WORD(1,6), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpUDot, true, true, IR(437, 5), IR(36190, 5), IR(175, 1), IR(257, 1), IR(153, 1), SPV_SPIRV_VERSION_WORD(1,6), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpSUDot, true, true, IR(437, 5), IR(36203, 6), IR(176, 1), IR(257, 1), IR(153, 1), SPV_SPIRV_VERSION_WORD(1,6), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpSDotAccSat, true, true, IR(442, 6), IR(36218, 11), IR(177, 1), IR(257, 1), IR(153, 1), SPV_SPIRV_VERSION_WORD(1,6), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpUDotAccSat, true, true, IR(442, 6), IR(36243, 11), IR(178, 1), IR(257, 1), IR(153, 1), SPV_SPIRV_VERSION_WORD(1,6), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpSUDotAccSat, true, true, IR(442, 6), IR(36268, 12), IR(179, 1), IR(257, 1), IR(153, 1), SPV_SPIRV_VERSION_WORD(1,6), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpTypeCooperativeMatrixKHR, true, false, IR(448, 6), IR(36295, 25), IR(0, 0), IR(87, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpCooperativeMatrixLoadKHR, true, true, IR(454, 6), IR(36320, 25), IR(0, 0), IR(87, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpCooperativeMatrixStoreKHR, false, false, IR(460, 5), IR(36345, 26), IR(0, 0), IR(87, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpCooperativeMatrixMulAddKHR, true, true, IR(465, 6), IR(36425, 27), IR(0, 0), IR(87, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpCooperativeMatrixLengthKHR, true, true, IR(160, 3), IR(36452, 27), IR(0, 0), IR(87, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMiscellaneous},
  {spv::Op::OpConstantCompositeReplicateEXT, true, true, IR(160, 3), IR(36479, 30), IR(0, 0), IR(258, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpSpecConstantCompositeReplicateEXT, true, true, IR(160, 3), IR(36509, 34), IR(0, 0), IR(258, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpCompositeConstructReplicateEXT, true, true, IR(160, 3), IR(36543, 31), IR(0, 0), IR(258, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpTypeRayQueryKHR, true, false, IR(68, 1), IR(36574, 16), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpRayQueryInitializeKHR, false, false, IR(471, 8), IR(36590, 22), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryTerminateKHR, false, false, IR(10, 1), IR(36612, 21), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGenerateIntersectionKHR, false, false, IR(36, 2), IR(36633, 32), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryConfirmIntersectionKHR, false, false, IR(10, 1), IR(36665, 31), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryProceedKHR, true, true, IR(160, 3), IR(36696, 19), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionTypeKHR, true, true, IR(172, 4), IR(36715, 31), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpImageSampleWeightedQCOM, true, true, IR(130, 5), IR(36746, 24), IR(0, 0), IR(259, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageBoxFilterQCOM, true, true, IR(130, 5), IR(36770, 19), IR(0, 0), IR(260, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageBlockMatchSSDQCOM, true, true, IR(325, 7), IR(36789, 23), IR(0, 0), IR(261, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageBlockMatchSADQCOM, true, true, IR(325, 7), IR(36812, 23), IR(0, 0), IR(261, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpBitCastArrayQCOM, true, true, IR(160, 3), IR(36835, 17), IR(0, 0), IR(262, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpImageBlockMatchWindowSSDQCOM, true, true, IR(325, 7), IR(36852, 29), IR(0, 0), IR(263, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageBlockMatchWindowSADQCOM, true, true, IR(325, 7), IR(36881, 29), IR(0, 0), IR(263, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageBlockMatchGatherSSDQCOM, true, true, IR(325, 7), IR(36910, 29), IR(0, 0), IR(263, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageBlockMatchGatherSADQCOM, true, true, IR(325, 7), IR(36939, 29), IR(0, 0), IR(263, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpCompositeConstructCoopMatQCOM, true, true, IR(160, 3), IR(36968, 30), IR(0, 0), IR(262, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpCompositeExtractCoopMatQCOM, true, true, IR(160, 3), IR(36998, 28), IR(0, 0), IR(262, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpExtractSubArrayQCOM, true, true, IR(172, 4), IR(37026, 20), IR(0, 0), IR(262, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpImageGatherQCOM, true, true, IR(479, 7), IR(37046, 16), IR(0, 0), IR(264, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpGroupIAddNonUniformAMD, true, true, IR(283, 5), IR(37062, 23), IR(0, 0), IR(234, 1), IR(33, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupFAddNonUniformAMD, true, true, IR(283, 5), IR(37085, 23), IR(0, 0), IR(234, 1), IR(33, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupFMinNonUniformAMD, true, true, IR(283, 5), IR(37108, 23), IR(0, 0), IR(234, 1), IR(33, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupUMinNonUniformAMD, true, true, IR(283, 5), IR(37131, 23), IR(0, 0), IR(234, 1), IR(33, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupSMinNonUniformAMD, true, true, IR(283, 5), IR(37154, 23), IR(0, 0), IR(234, 1), IR(33, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupFMaxNonUniformAMD, true, true, IR(283, 5), IR(37177, 23), IR(0, 0), IR(234, 1), IR(33, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupUMaxNonUniformAMD, true, true, IR(283, 5), IR(37200, 23), IR(0, 0), IR(234, 1), IR(33, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupSMaxNonUniformAMD, true, true, IR(283, 5), IR(37223, 23), IR(0, 0), IR(234, 1), IR(33, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpFragmentMaskFetchAMD, true, true, IR(172, 4), IR(37246, 21), IR(0, 0), IR(266, 1), IR(61, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpFragmentFetchAMD, true, true, IR(130, 5), IR(37267, 17), IR(0, 0), IR(266, 1), IR(61, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpReadClockKHR, true, true, IR(340, 3), IR(37284, 13), IR(0, 0), IR(267, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpAllocateNodePayloadsAMDX, true, true, IR(278, 5), IR(37297, 25), IR(0, 0), IR(43, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpEnqueueNodePayloadsAMDX, false, false, IR(10, 1), IR(37322, 24), IR(0, 0), IR(43, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTypeNodePayloadArrayAMDX, true, false, IR(87, 2), IR(37346, 25), IR(0, 0), IR(43, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpFinishWritingNodePayloadAMDX, true, true, IR(160, 3), IR(37371, 29), IR(0, 0), IR(43, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpNodePayloadArrayLengthAMDX, true, true, IR(160, 3), IR(37400, 27), IR(0, 0), IR(43, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpIsNodePayloadValidAMDX, true, true, IR(172, 4), IR(37427, 23), IR(0, 0), IR(43, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpConstantStringAMDX, true, false, IR(50, 2), IR(37450, 19), IR(0, 0), IR(43, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpSpecConstantStringAMDX, true, false, IR(50, 2), IR(37469, 23), IR(0, 0), IR(43, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpGroupNonUniformQuadAllKHR, true, true, IR(160, 3), IR(37492, 26), IR(0, 0), IR(172, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformQuadAnyKHR, true, true, IR(160, 3), IR(37518, 26), IR(0, 0), IR(172, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpTypeBufferEXT, true, false, IR(381, 2), IR(37544, 14), IR(0, 0), IR(44, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpBufferPointerEXT, true, true, IR(160, 3), IR(37558, 17), IR(0, 0), IR(44, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpAbortKHR, false, false, IR(36, 2), IR(11476, 9), IR(0, 0), IR(268, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpUntypedImageTexelPointerEXT, true, true, IR(215, 6), IR(37575, 28), IR(0, 0), IR(44, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpMemberDecorateIdEXT, false, false, IR(165, 3), IR(37603, 20), IR(0, 0), IR(44, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kAnnotation},
  {spv::Op::OpConstantSizeOfEXT, true, true, IR(160, 3), IR(37623, 18), IR(0, 0), IR(44, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpConstantDataKHR, true, true, IR(486, 3), IR(11485, 16), IR(0, 0), IR(127, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpSpecConstantDataKHR, true, true, IR(486, 3), IR(37641, 20), IR(0, 0), IR(127, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpPoisonKHR, true, true, IR(39, 2), IR(37661, 10), IR(0, 0), IR(173, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMiscellaneous},
  {spv::Op::OpFreezeKHR, true, true, IR(160, 3), IR(37671, 10), IR(0, 0), IR(173, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMiscellaneous},
  {spv::Op::OpHitObjectRecordHitMotionNV, false, false, IR(489, 14), IR(37681, 27), IR(0, 0), IR(269, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectRecordHitWithIndexMotionNV, false, false, IR(503, 13), IR(37708, 36), IR(0, 0), IR(269, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectRecordMissMotionNV, false, false, IR(516, 7), IR(37744, 28), IR(0, 0), IR(269, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetWorldToObjectNV, true, true, IR(160, 3), IR(37772, 28), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetObjectToWorldNV, true, true, IR(160, 3), IR(37800, 28), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetObjectRayDirectionNV, true, true, IR(160, 3), IR(37828, 33), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetObjectRayOriginNV, true, true, IR(160, 3), IR(37861, 30), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectTraceRayMotionNV, false, false, IR(503, 13), IR(37891, 26), IR(0, 0), IR(269, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetShaderRecordBufferHandleNV, true, true, IR(160, 3), IR(37917, 39), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetShaderBindingTableRecordIndexNV, true, true, IR(160, 3), IR(37956, 44), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectRecordEmptyNV, false, false, IR(10, 1), IR(38000, 23), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectTraceRayNV, false, false, IR(523, 12), IR(38023, 20), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectRecordHitNV, false, false, IR(503, 13), IR(38043, 21), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectRecordHitWithIndexNV, false, false, IR(523, 12), IR(38064, 30), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectRecordMissNV, false, false, IR(535, 6), IR(38094, 22), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectExecuteShaderNV, false, false, IR(36, 2), IR(38116, 25), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetCurrentTimeNV, true, true, IR(160, 3), IR(38141, 26), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetAttributesNV, false, false, IR(36, 2), IR(38167, 25), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetHitKindNV, true, true, IR(160, 3), IR(38192, 22), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetPrimitiveIndexNV, true, true, IR(160, 3), IR(38214, 29), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetGeometryIndexNV, true, true, IR(160, 3), IR(38243, 28), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetInstanceIdNV, true, true, IR(160, 3), IR(38271, 25), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetInstanceCustomIndexNV, true, true, IR(160, 3), IR(38296, 34), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetWorldRayDirectionNV, true, true, IR(160, 3), IR(38330, 32), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetWorldRayOriginNV, true, true, IR(160, 3), IR(38362, 29), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetRayTMaxNV, true, true, IR(160, 3), IR(38391, 22), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetRayTMinNV, true, true, IR(160, 3), IR(38413, 22), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectIsEmptyNV, true, true, IR(160, 3), IR(38435, 19), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectIsHitNV, true, true, IR(160, 3), IR(38454, 17), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectIsMissNV, true, true, IR(160, 3), IR(38471, 18), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpReorderThreadWithHitObjectNV, false, false, IR(541, 3), IR(38489, 29), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpReorderThreadWithHintNV, false, false, IR(36, 2), IR(38518, 24), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTypeHitObjectNV, true, false, IR(68, 1), IR(38542, 16), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpImageSampleFootprintNV, true, true, IR(479, 7), IR(38558, 23), IR(0, 0), IR(271, 1), IR(80, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpTypeVectorIdEXT, true, false, IR(89, 3), IR(38581, 16), IR(180, 1), IR(272, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpCooperativeVectorMatrixMulNV, true, true, IR(544, 13), IR(38621, 29), IR(0, 0), IR(274, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpCooperativeVectorOuterProductAccumulateNV, false, false, IR(557, 7), IR(38650, 42), IR(0, 0), IR(275, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpCooperativeVectorReduceSumAccumulateNV, false, false, IR(33, 3), IR(38692, 39), IR(0, 0), IR(275, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpCooperativeVectorMatrixMulAddNV, true, true, IR(564, 16), IR(38731, 32), IR(0, 0), IR(274, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpCooperativeMatrixConvertNV, true, true, IR(160, 3), IR(38763, 27), IR(0, 0), IR(276, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpEmitMeshTasksEXT, false, false, IR(580, 4), IR(38790, 17), IR(0, 0), IR(54, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpSetMeshOutputsEXT, false, false, IR(36, 2), IR(38807, 18), IR(0, 0), IR(54, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpGroupNonUniformPartitionEXT, true, true, IR(160, 3), IR(38825, 28), IR(181, 1), IR(198, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpWritePackedPrimitiveIndices4x8NV, false, false, IR(36, 2), IR(38880, 33), IR(0, 0), IR(51, 1), IR(18, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpFetchMicroTriangleVertexPositionNV, true, true, IR(325, 7), IR(38913, 35), IR(0, 0), IR(277, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpFetchMicroTriangleVertexBarycentricNV, true, true, IR(325, 7), IR(38948, 38), IR(0, 0), IR(277, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpCooperativeVectorLoadNV, true, true, IR(584, 5), IR(38986, 24), IR(0, 0), IR(274, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpCooperativeVectorStoreNV, false, false, IR(589, 4), IR(39010, 25), IR(0, 0), IR(274, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpHitObjectRecordFromQueryEXT, false, false, IR(593, 5), IR(39035, 28), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectRecordMissEXT, false, false, IR(516, 7), IR(39063, 23), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectRecordMissMotionEXT, false, false, IR(471, 8), IR(39086, 29), IR(0, 0), IR(278, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetIntersectionTriangleVertexPositionsEXT, true, true, IR(160, 3), IR(39115, 51), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetRayFlagsEXT, true, true, IR(160, 3), IR(39166, 24), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectSetShaderBindingTableRecordIndexEXT, false, false, IR(36, 2), IR(39190, 45), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectReorderExecuteShaderEXT, false, false, IR(598, 4), IR(39235, 33), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectTraceReorderExecuteEXT, false, false, IR(602, 14), IR(39268, 32), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectTraceMotionReorderExecuteEXT, false, false, IR(616, 15), IR(39300, 38), IR(0, 0), IR(278, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTypeHitObjectEXT, true, false, IR(68, 1), IR(39338, 17), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpReorderThreadWithHintEXT, false, false, IR(36, 2), IR(39355, 25), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpReorderThreadWithHitObjectEXT, false, false, IR(541, 3), IR(39380, 30), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectTraceRayEXT, false, false, IR(523, 12), IR(39410, 21), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectTraceRayMotionEXT, false, false, IR(503, 13), IR(39431, 27), IR(0, 0), IR(278, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectRecordEmptyEXT, false, false, IR(10, 1), IR(39458, 24), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectExecuteShaderEXT, false, false, IR(36, 2), IR(39482, 26), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetCurrentTimeEXT, true, true, IR(160, 3), IR(39508, 27), IR(0, 0), IR(278, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetAttributesEXT, false, false, IR(36, 2), IR(39535, 26), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetHitKindEXT, true, true, IR(160, 3), IR(39561, 23), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetPrimitiveIndexEXT, true, true, IR(160, 3), IR(39584, 30), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetGeometryIndexEXT, true, true, IR(160, 3), IR(39614, 29), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetInstanceIdEXT, true, true, IR(160, 3), IR(39643, 26), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetInstanceCustomIndexEXT, true, true, IR(160, 3), IR(39669, 35), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetObjectRayOriginEXT, true, true, IR(160, 3), IR(39704, 31), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetObjectRayDirectionEXT, true, true, IR(160, 3), IR(39735, 34), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetWorldRayDirectionEXT, true, true, IR(160, 3), IR(39769, 33), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetWorldRayOriginEXT, true, true, IR(160, 3), IR(39802, 30), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetObjectToWorldEXT, true, true, IR(160, 3), IR(39832, 29), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetWorldToObjectEXT, true, true, IR(160, 3), IR(39861, 29), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetRayTMaxEXT, true, true, IR(160, 3), IR(39890, 23), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpReportIntersectionKHR, true, true, IR(172, 4), IR(39913, 22), IR(182, 1), IR(55, 2), IR(24, 2), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpIgnoreIntersectionNV, false, false, IR(0, 0), IR(39956, 21), IR(0, 0), IR(57, 1), IR(26, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTerminateRayNV, false, false, IR(0, 0), IR(39977, 15), IR(0, 0), IR(57, 1), IR(26, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTraceNV, false, false, IR(426, 11), IR(39992, 8), IR(0, 0), IR(57, 1), IR(26, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTraceMotionNV, false, false, IR(523, 12), IR(40000, 14), IR(0, 0), IR(58, 1), IR(27, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTraceRayMotionNV, false, false, IR(523, 12), IR(40014, 17), IR(0, 0), IR(58, 1), IR(27, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionTriangleVertexPositionsKHR, true, true, IR(172, 4), IR(40031, 50), IR(0, 0), IR(280, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTypeAccelerationStructureKHR, true, false, IR(68, 1), IR(40081, 29), IR(183, 1), IR(281, 4), IR(212, 4), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpExecuteCallableNV, false, false, IR(36, 2), IR(40138, 18), IR(0, 0), IR(57, 1), IR(26, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionClusterIdNV, true, true, IR(172, 4), IR(40156, 35), IR(184, 1), IR(65, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetClusterIdNV, true, true, IR(160, 3), IR(40214, 24), IR(0, 0), IR(65, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetRayTMinEXT, true, true, IR(160, 3), IR(40238, 23), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetShaderBindingTableRecordIndexEXT, true, true, IR(160, 3), IR(40261, 45), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetShaderRecordBufferHandleEXT, true, true, IR(160, 3), IR(40306, 40), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectIsEmptyEXT, true, true, IR(160, 3), IR(40346, 20), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectIsHitEXT, true, true, IR(160, 3), IR(40366, 18), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectIsMissEXT, true, true, IR(160, 3), IR(40384, 19), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTypeCooperativeMatrixNV, true, false, IR(631, 5), IR(40403, 24), IR(0, 0), IR(285, 1), IR(88, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpCooperativeMatrixLoadNV, true, true, IR(636, 6), IR(40427, 24), IR(0, 0), IR(285, 1), IR(88, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpCooperativeMatrixStoreNV, false, false, IR(642, 5), IR(40451, 25), IR(0, 0), IR(285, 1), IR(88, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpCooperativeMatrixMulAddNV, true, true, IR(130, 5), IR(40476, 26), IR(0, 0), IR(285, 1), IR(88, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpCooperativeMatrixLengthNV, true, true, IR(160, 3), IR(40502, 26), IR(0, 0), IR(285, 1), IR(88, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpBeginInvocationInterlockEXT, false, false, IR(0, 0), IR(40528, 28), IR(0, 0), IR(286, 3), IR(89, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpEndInvocationInterlockEXT, false, false, IR(0, 0), IR(40556, 26), IR(0, 0), IR(286, 3), IR(89, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpCooperativeMatrixReduceNV, true, true, IR(647, 5), IR(40625, 26), IR(0, 0), IR(289, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpCooperativeMatrixLoadTensorNV, true, true, IR(652, 7), IR(40726, 30), IR(0, 0), IR(216, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpCooperativeMatrixStoreTensorNV, false, false, IR(659, 5), IR(40756, 31), IR(0, 0), IR(216, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpCooperativeMatrixPerElementOpNV, true, true, IR(151, 5), IR(40787, 32), IR(0, 0), IR(290, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kFunction},
  {spv::Op::OpTypeTensorLayoutNV, true, false, IR(89, 3), IR(40819, 19), IR(0, 0), IR(291, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeTensorViewNV, true, false, IR(664, 4), IR(40838, 17), IR(0, 0), IR(291, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpCreateTensorLayoutNV, true, true, IR(39, 2), IR(40855, 21), IR(0, 0), IR(291, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTensorLayoutSetDimensionNV, true, true, IR(122, 4), IR(40876, 27), IR(0, 0), IR(291, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTensorLayoutSetStrideNV, true, true, IR(122, 4), IR(40903, 24), IR(0, 0), IR(291, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTensorLayoutSliceNV, true, true, IR(122, 4), IR(40927, 20), IR(0, 0), IR(291, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTensorLayoutSetClampValueNV, true, true, IR(172, 4), IR(40947, 28), IR(0, 0), IR(291, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpCreateTensorViewNV, true, true, IR(39, 2), IR(40975, 19), IR(0, 0), IR(291, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTensorViewSetDimensionNV, true, true, IR(122, 4), IR(40994, 25), IR(0, 0), IR(291, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTensorViewSetStrideNV, true, true, IR(122, 4), IR(41019, 22), IR(0, 0), IR(291, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpDemoteToHelperInvocation, false, false, IR(0, 0), IR(13016, 25), IR(46, 1), IR(292, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,6), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpIsHelperInvocationEXT, true, true, IR(39, 2), IR(41041, 22), IR(0, 0), IR(292, 1), IR(90, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTensorViewSetClipNV, true, true, IR(325, 7), IR(41063, 20), IR(0, 0), IR(291, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTensorLayoutSetBlockSizeNV, true, true, IR(122, 4), IR(41083, 27), IR(0, 0), IR(291, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpCooperativeMatrixTransposeNV, true, true, IR(160, 3), IR(41110, 29), IR(0, 0), IR(276, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpConvertUToImageNV, true, true, IR(160, 3), IR(41139, 18), IR(0, 0), IR(135, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpConvertUToSamplerNV, true, true, IR(160, 3), IR(41157, 20), IR(0, 0), IR(135, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpConvertImageToUNV, true, true, IR(160, 3), IR(41177, 18), IR(0, 0), IR(135, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpConvertSamplerToUNV, true, true, IR(160, 3), IR(41195, 20), IR(0, 0), IR(135, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpConvertUToSampledImageNV, true, true, IR(160, 3), IR(41215, 25), IR(0, 0), IR(135, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpConvertSampledImageToUNV, true, true, IR(160, 3), IR(41240, 25), IR(0, 0), IR(135, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpSamplerImageAddressingModeNV, false, false, IR(0, 1), IR(41265, 29), IR(0, 0), IR(135, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRawAccessChainNV, true, true, IR(668, 7), IR(41346, 17), IR(0, 0), IR(205, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpRayQueryGetIntersectionSpherePositionNV, true, true, IR(172, 4), IR(41363, 40), IR(0, 0), IR(62, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionSphereRadiusNV, true, true, IR(172, 4), IR(41403, 38), IR(0, 0), IR(62, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionLSSPositionsNV, true, true, IR(172, 4), IR(41441, 38), IR(0, 0), IR(63, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionLSSRadiiNV, true, true, IR(172, 4), IR(41479, 34), IR(0, 0), IR(63, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionLSSHitValueNV, true, true, IR(172, 4), IR(41513, 37), IR(0, 0), IR(63, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetSpherePositionNV, true, true, IR(160, 3), IR(41550, 29), IR(0, 0), IR(62, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetSphereRadiusNV, true, true, IR(160, 3), IR(41579, 27), IR(0, 0), IR(62, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetLSSPositionsNV, true, true, IR(160, 3), IR(41606, 27), IR(0, 0), IR(63, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetLSSRadiiNV, true, true, IR(160, 3), IR(41633, 23), IR(0, 0), IR(63, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectIsSphereHitNV, true, true, IR(160, 3), IR(41656, 23), IR(0, 0), IR(62, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectIsLSSHitNV, true, true, IR(160, 3), IR(41679, 20), IR(0, 0), IR(63, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryIsSphereHitNV, true, true, IR(172, 4), IR(41699, 22), IR(0, 0), IR(62, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryIsLSSHitNV, true, true, IR(172, 4), IR(41721, 19), IR(0, 0), IR(63, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpSubgroupShuffleINTEL, true, true, IR(172, 4), IR(13609, 21), IR(0, 0), IR(293, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupShuffleDownINTEL, true, true, IR(130, 5), IR(41740, 25), IR(0, 0), IR(293, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupShuffleUpINTEL, true, true, IR(130, 5), IR(41765, 23), IR(0, 0), IR(293, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupShuffleXorINTEL, true, true, IR(172, 4), IR(41788, 24), IR(0, 0), IR(293, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupBlockReadINTEL, true, true, IR(160, 3), IR(41812, 23), IR(0, 0), IR(294, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupBlockWriteINTEL, false, false, IR(36, 2), IR(41835, 24), IR(0, 0), IR(294, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupImageBlockReadINTEL, true, true, IR(172, 4), IR(41859, 28), IR(0, 0), IR(295, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupImageBlockWriteINTEL, false, false, IR(33, 3), IR(41887, 29), IR(0, 0), IR(295, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupImageMediaBlockReadINTEL, true, true, IR(215, 6), IR(41916, 33), IR(0, 0), IR(296, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupImageMediaBlockWriteINTEL, false, false, IR(675, 5), IR(41949, 34), IR(0, 0), IR(296, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpUCountLeadingZerosINTEL, true, true, IR(160, 3), IR(41983, 24), IR(0, 0), IR(297, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpUCountTrailingZerosINTEL, true, true, IR(160, 3), IR(42007, 25), IR(0, 0), IR(297, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpAbsISubINTEL, true, true, IR(172, 4), IR(42032, 13), IR(0, 0), IR(297, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpAbsUSubINTEL, true, true, IR(172, 4), IR(42045, 13), IR(0, 0), IR(297, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpIAddSatINTEL, true, true, IR(172, 4), IR(42058, 13), IR(0, 0), IR(297, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpUAddSatINTEL, true, true, IR(172, 4), IR(42071, 13), IR(0, 0), IR(297, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpIAverageINTEL, true, true, IR(172, 4), IR(42084, 14), IR(0, 0), IR(297, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpUAverageINTEL, true, true, IR(172, 4), IR(42098, 14), IR(0, 0), IR(297, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpIAverageRoundedINTEL, true, true, IR(172, 4), IR(42112, 21), IR(0, 0), IR(297, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpUAverageRoundedINTEL, true, true, IR(172, 4), IR(42133, 21), IR(0, 0), IR(297, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpISubSatINTEL, true, true, IR(172, 4), IR(42154, 13), IR(0, 0), IR(297, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpUSubSatINTEL, true, true, IR(172, 4), IR(42167, 13), IR(0, 0), IR(297, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpIMul32x16INTEL, true, true, IR(172, 4), IR(42180, 15), IR(0, 0), IR(297, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpUMul32x16INTEL, true, true, IR(172, 4), IR(42195, 15), IR(0, 0), IR(297, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpConstantFunctionPointerINTEL, true, true, IR(160, 3), IR(42210, 29), IR(0, 0), IR(214, 1), IR(110, 1), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFunctionPointerCallINTEL, true, true, IR(107, 3), IR(42239, 25), IR(0, 0), IR(214, 1), IR(110, 1), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpAsmTargetINTEL, true, false, IR(50, 2), IR(42264, 15), IR(0, 0), IR(138, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpAsmINTEL, true, true, IR(680, 6), IR(13827, 9), IR(0, 0), IR(138, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpAsmCallINTEL, true, true, IR(122, 4), IR(42279, 13), IR(0, 0), IR(138, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpAtomicFMinEXT, true, true, IR(235, 6), IR(42292, 14), IR(0, 0), IR(298, 4), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicFMaxEXT, true, true, IR(235, 6), IR(42306, 14), IR(0, 0), IR(298, 4), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAssumeTrueKHR, false, false, IR(10, 1), IR(42320, 14), IR(0, 0), IR(302, 1), IR(114, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kMiscellaneous},
  {spv::Op::OpExpectKHR, true, true, IR(172, 4), IR(42334, 10), IR(0, 0), IR(302, 1), IR(114, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kMiscellaneous},
  {spv::Op::OpDecorateString, false, false, IR(163, 2), IR(42344, 15), IR(185, 1), IR(1, 0), IR(216, 2), SPV_SPIRV_VERSION_WORD(1,4), 0xffffffffu, PrintingClass::kAnnotation},
  {spv::Op::OpMemberDecorateString, false, false, IR(165, 3), IR(42380, 21), IR(186, 1), IR(1, 0), IR(216, 2), SPV_SPIRV_VERSION_WORD(1,4), 0xffffffffu, PrintingClass::kAnnotation},
  {spv::Op::OpVmeImageINTEL, true, true, IR(172, 4), IR(42428, 14), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeVmeImageINTEL, true, false, IR(87, 2), IR(42442, 18), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcImePayloadINTEL, true, false, IR(68, 1), IR(42460, 23), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcRefPayloadINTEL, true, false, IR(68, 1), IR(42483, 23), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcSicPayloadINTEL, true, false, IR(68, 1), IR(42506, 23), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcMcePayloadINTEL, true, false, IR(68, 1), IR(42529, 23), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcMceResultINTEL, true, false, IR(68, 1), IR(42552, 22), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcImeResultINTEL, true, false, IR(68, 1), IR(42574, 22), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcImeResultSingleReferenceStreamoutINTEL, true, false, IR(68, 1), IR(42596, 46), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcImeResultDualReferenceStreamoutINTEL, true, false, IR(68, 1), IR(42642, 44), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcImeSingleReferenceStreaminINTEL, true, false, IR(68, 1), IR(42686, 39), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcImeDualReferenceStreaminINTEL, true, false, IR(68, 1), IR(42725, 37), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcRefResultINTEL, true, false, IR(68, 1), IR(42762, 22), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcSicResultINTEL, true, false, IR(68, 1), IR(42784, 22), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetDefaultInterBaseMultiReferencePenaltyINTEL, true, true, IR(172, 4), IR(42806, 60), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceSetInterBaseMultiReferencePenaltyINTEL, true, true, IR(172, 4), IR(42866, 53), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetDefaultInterShapePenaltyINTEL, true, true, IR(172, 4), IR(42919, 47), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceSetInterShapePenaltyINTEL, true, true, IR(172, 4), IR(42966, 40), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetDefaultInterDirectionPenaltyINTEL, true, true, IR(172, 4), IR(43006, 51), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceSetInterDirectionPenaltyINTEL, true, true, IR(172, 4), IR(43057, 44), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetDefaultIntraLumaShapePenaltyINTEL, true, true, IR(172, 4), IR(43101, 51), IR(0, 0), IR(304, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetDefaultInterMotionVectorCostTableINTEL, true, true, IR(172, 4), IR(43152, 56), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetDefaultHighPenaltyCostTableINTEL, true, true, IR(39, 2), IR(43208, 50), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetDefaultMediumPenaltyCostTableINTEL, true, true, IR(39, 2), IR(43258, 52), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetDefaultLowPenaltyCostTableINTEL, true, true, IR(39, 2), IR(43310, 49), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceSetMotionVectorCostFunctionINTEL, true, true, IR(215, 6), IR(43359, 47), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetDefaultIntraLumaModePenaltyINTEL, true, true, IR(172, 4), IR(43406, 50), IR(0, 0), IR(304, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetDefaultNonDcLumaIntraPenaltyINTEL, true, true, IR(39, 2), IR(43456, 51), IR(0, 0), IR(304, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetDefaultIntraChromaModeBasePenaltyINTEL, true, true, IR(39, 2), IR(43507, 56), IR(0, 0), IR(306, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceSetAcOnlyHaarINTEL, true, true, IR(160, 3), IR(43563, 33), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceSetSourceInterlacedFieldPolarityINTEL, true, true, IR(172, 4), IR(43596, 52), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceSetSingleReferenceInterlacedFieldPolarityINTEL, true, true, IR(172, 4), IR(43648, 61), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceSetDualReferenceInterlacedFieldPolaritiesINTEL, true, true, IR(130, 5), IR(43709, 61), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceConvertToImePayloadINTEL, true, true, IR(160, 3), IR(43770, 39), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceConvertToImeResultINTEL, true, true, IR(160, 3), IR(43809, 38), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceConvertToRefPayloadINTEL, true, true, IR(160, 3), IR(43847, 39), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceConvertToRefResultINTEL, true, true, IR(160, 3), IR(43886, 38), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceConvertToSicPayloadINTEL, true, true, IR(160, 3), IR(43924, 39), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceConvertToSicResultINTEL, true, true, IR(160, 3), IR(43963, 38), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetMotionVectorsINTEL, true, true, IR(160, 3), IR(44001, 36), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetInterDistortionsINTEL, true, true, IR(160, 3), IR(44037, 39), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetBestInterDistortionsINTEL, true, true, IR(160, 3), IR(44076, 43), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetInterMajorShapeINTEL, true, true, IR(160, 3), IR(44119, 38), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetInterMinorShapeINTEL, true, true, IR(160, 3), IR(44157, 38), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetInterDirectionsINTEL, true, true, IR(160, 3), IR(44195, 38), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetInterMotionVectorCountINTEL, true, true, IR(160, 3), IR(44233, 45), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetInterReferenceIdsINTEL, true, true, IR(160, 3), IR(44278, 40), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetInterReferenceInterlacedFieldPolaritiesINTEL, true, true, IR(130, 5), IR(44318, 62), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeInitializeINTEL, true, true, IR(130, 5), IR(44380, 30), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeSetSingleReferenceINTEL, true, true, IR(130, 5), IR(44410, 38), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeSetDualReferenceINTEL, true, true, IR(215, 6), IR(44448, 36), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeRefWindowSizeINTEL, true, true, IR(172, 4), IR(44484, 33), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeAdjustRefOffsetINTEL, true, true, IR(215, 6), IR(44517, 35), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeConvertToMcePayloadINTEL, true, true, IR(160, 3), IR(44552, 39), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeSetMaxMotionVectorCountINTEL, true, true, IR(172, 4), IR(44591, 43), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeSetUnidirectionalMixDisableINTEL, true, true, IR(160, 3), IR(44634, 47), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeSetEarlySearchTerminationThresholdINTEL, true, true, IR(172, 4), IR(44681, 54), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeSetWeightedSadINTEL, true, true, IR(172, 4), IR(44735, 34), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeEvaluateWithSingleReferenceINTEL, true, true, IR(130, 5), IR(44769, 47), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeEvaluateWithDualReferenceINTEL, true, true, IR(215, 6), IR(44816, 45), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeEvaluateWithSingleReferenceStreaminINTEL, true, true, IR(215, 6), IR(44861, 55), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeEvaluateWithDualReferenceStreaminINTEL, true, true, IR(325, 7), IR(44916, 53), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeEvaluateWithSingleReferenceStreamoutINTEL, true, true, IR(130, 5), IR(44969, 56), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeEvaluateWithDualReferenceStreamoutINTEL, true, true, IR(215, 6), IR(45025, 54), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeEvaluateWithSingleReferenceStreaminoutINTEL, true, true, IR(215, 6), IR(45079, 58), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeEvaluateWithDualReferenceStreaminoutINTEL, true, true, IR(325, 7), IR(45137, 56), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeConvertToMceResultINTEL, true, true, IR(160, 3), IR(45193, 38), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetSingleReferenceStreaminINTEL, true, true, IR(160, 3), IR(45231, 46), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetDualReferenceStreaminINTEL, true, true, IR(160, 3), IR(45277, 44), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeStripSingleReferenceStreamoutINTEL, true, true, IR(160, 3), IR(45321, 49), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeStripDualReferenceStreamoutINTEL, true, true, IR(160, 3), IR(45370, 47), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeMotionVectorsINTEL, true, true, IR(172, 4), IR(45417, 70), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeDistortionsINTEL, true, true, IR(172, 4), IR(45487, 68), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeReferenceIdsINTEL, true, true, IR(172, 4), IR(45555, 69), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeMotionVectorsINTEL, true, true, IR(130, 5), IR(45624, 68), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeDistortionsINTEL, true, true, IR(130, 5), IR(45692, 66), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeReferenceIdsINTEL, true, true, IR(130, 5), IR(45758, 67), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetBorderReachedINTEL, true, true, IR(172, 4), IR(45825, 36), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetTruncatedSearchIndicationINTEL, true, true, IR(160, 3), IR(45861, 48), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetUnidirectionalEarlySearchTerminationINTEL, true, true, IR(160, 3), IR(45909, 59), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetWeightingPatternMinimumMotionVectorINTEL, true, true, IR(160, 3), IR(45968, 58), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetWeightingPatternMinimumDistortionINTEL, true, true, IR(160, 3), IR(46026, 56), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcFmeInitializeINTEL, true, true, IR(686, 9), IR(46082, 30), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcBmeInitializeINTEL, true, true, IR(695, 10), IR(46112, 30), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcRefConvertToMcePayloadINTEL, true, true, IR(160, 3), IR(46142, 39), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcRefSetBidirectionalMixDisableINTEL, true, true, IR(160, 3), IR(46181, 46), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcRefSetBilinearFilterEnableINTEL, true, true, IR(160, 3), IR(46227, 43), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcRefEvaluateWithSingleReferenceINTEL, true, true, IR(130, 5), IR(46270, 47), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcRefEvaluateWithDualReferenceINTEL, true, true, IR(215, 6), IR(46317, 45), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcRefEvaluateWithMultiReferenceINTEL, true, true, IR(130, 5), IR(46362, 46), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcRefEvaluateWithMultiReferenceInterlacedINTEL, true, true, IR(215, 6), IR(46408, 56), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcRefConvertToMceResultINTEL, true, true, IR(160, 3), IR(46464, 38), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicInitializeINTEL, true, true, IR(160, 3), IR(46502, 30), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicConfigureSkcINTEL, true, true, IR(288, 8), IR(46532, 32), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicConfigureIpeLumaINTEL, true, true, IR(695, 10), IR(46564, 36), IR(0, 0), IR(304, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicConfigureIpeLumaChromaINTEL, true, true, IR(705, 13), IR(46600, 42), IR(0, 0), IR(306, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicGetMotionVectorMaskINTEL, true, true, IR(172, 4), IR(46642, 39), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicConvertToMcePayloadINTEL, true, true, IR(160, 3), IR(46681, 39), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicSetIntraLumaShapePenaltyINTEL, true, true, IR(172, 4), IR(46720, 44), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicSetIntraLumaModeCostFunctionINTEL, true, true, IR(215, 6), IR(46764, 48), IR(0, 0), IR(304, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicSetIntraChromaModeCostFunctionINTEL, true, true, IR(172, 4), IR(46812, 50), IR(0, 0), IR(306, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicSetBilinearFilterEnableINTEL, true, true, IR(160, 3), IR(46862, 43), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicSetSkcForwardTransformEnableINTEL, true, true, IR(172, 4), IR(46905, 48), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicSetBlockBasedRawSkipSadINTEL, true, true, IR(172, 4), IR(46953, 43), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicEvaluateIpeINTEL, true, true, IR(172, 4), IR(46996, 31), IR(0, 0), IR(304, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicEvaluateWithSingleReferenceINTEL, true, true, IR(130, 5), IR(47027, 47), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicEvaluateWithDualReferenceINTEL, true, true, IR(215, 6), IR(47074, 45), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicEvaluateWithMultiReferenceINTEL, true, true, IR(130, 5), IR(47119, 46), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicEvaluateWithMultiReferenceInterlacedINTEL, true, true, IR(215, 6), IR(47165, 56), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicConvertToMceResultINTEL, true, true, IR(160, 3), IR(47221, 38), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicGetIpeLumaShapeINTEL, true, true, IR(160, 3), IR(47259, 35), IR(0, 0), IR(304, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicGetBestIpeLumaDistortionINTEL, true, true, IR(160, 3), IR(47294, 44), IR(0, 0), IR(304, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicGetBestIpeChromaDistortionINTEL, true, true, IR(160, 3), IR(47338, 46), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicGetPackedIpeLumaModesINTEL, true, true, IR(160, 3), IR(47384, 41), IR(0, 0), IR(304, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicGetIpeChromaModeINTEL, true, true, IR(160, 3), IR(47425, 36), IR(0, 0), IR(306, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicGetPackedSkcLumaCountThresholdINTEL, true, true, IR(160, 3), IR(47461, 50), IR(0, 0), IR(304, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicGetPackedSkcLumaSumThresholdINTEL, true, true, IR(160, 3), IR(47511, 48), IR(0, 0), IR(304, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicGetInterRawSadsINTEL, true, true, IR(160, 3), IR(47559, 35), IR(0, 0), IR(303, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpVariableLengthArrayINTEL, true, true, IR(160, 3), IR(14065, 25), IR(0, 0), IR(308, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpSaveMemoryINTEL, true, true, IR(39, 2), IR(47594, 16), IR(0, 0), IR(308, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpRestoreMemoryINTEL, false, false, IR(10, 1), IR(47610, 19), IR(0, 0), IR(308, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpArbitraryFloatSinCosPiALTERA, true, true, IR(718, 8), IR(47629, 29), IR(187, 1), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatCastALTERA, true, true, IR(718, 8), IR(47686, 25), IR(188, 1), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatCastFromIntALTERA, true, true, IR(718, 8), IR(47735, 32), IR(189, 1), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatCastToIntALTERA, true, true, IR(718, 8), IR(47798, 30), IR(190, 1), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatAddALTERA, true, true, IR(726, 10), IR(47857, 24), IR(191, 1), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatSubALTERA, true, true, IR(726, 10), IR(47904, 24), IR(192, 1), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatMulALTERA, true, true, IR(726, 10), IR(47951, 24), IR(193, 1), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatDivALTERA, true, true, IR(726, 10), IR(47998, 24), IR(194, 1), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatGTALTERA, true, true, IR(736, 6), IR(48045, 23), IR(195, 1), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatGEALTERA, true, true, IR(736, 6), IR(48090, 23), IR(196, 1), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatLTALTERA, true, true, IR(736, 6), IR(48135, 23), IR(197, 1), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatLEALTERA, true, true, IR(736, 6), IR(48180, 23), IR(198, 1), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatEQALTERA, true, true, IR(736, 6), IR(48225, 23), IR(199, 1), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatRecipALTERA, true, true, IR(718, 8), IR(48270, 26), IR(200, 1), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatRSqrtALTERA, true, true, IR(718, 8), IR(48321, 26), IR(201, 1), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatCbrtALTERA, true, true, IR(718, 8), IR(48372, 25), IR(202, 1), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatHypotALTERA, true, true, IR(726, 10), IR(48421, 26), IR(203, 1), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatSqrtALTERA, true, true, IR(718, 8), IR(48472, 25), IR(204, 1), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatLogINTEL, true, true, IR(718, 8), IR(48521, 23), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatLog2INTEL, true, true, IR(718, 8), IR(48544, 24), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatLog10INTEL, true, true, IR(718, 8), IR(48568, 25), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatLog1pINTEL, true, true, IR(718, 8), IR(48593, 25), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatExpINTEL, true, true, IR(718, 8), IR(48618, 23), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatExp2INTEL, true, true, IR(718, 8), IR(48641, 24), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatExp10INTEL, true, true, IR(718, 8), IR(48665, 25), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatExpm1INTEL, true, true, IR(718, 8), IR(48690, 25), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatSinINTEL, true, true, IR(718, 8), IR(48715, 23), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatCosINTEL, true, true, IR(718, 8), IR(48738, 23), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatSinCosINTEL, true, true, IR(718, 8), IR(48761, 26), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatSinPiINTEL, true, true, IR(718, 8), IR(48787, 25), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatCosPiINTEL, true, true, IR(718, 8), IR(48812, 25), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatASinINTEL, true, true, IR(718, 8), IR(48837, 24), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatASinPiINTEL, true, true, IR(718, 8), IR(48861, 26), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatACosINTEL, true, true, IR(718, 8), IR(48887, 24), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatACosPiINTEL, true, true, IR(718, 8), IR(48911, 26), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatATanINTEL, true, true, IR(718, 8), IR(48937, 24), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatATanPiINTEL, true, true, IR(718, 8), IR(48961, 26), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatATan2INTEL, true, true, IR(726, 10), IR(48987, 25), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatPowINTEL, true, true, IR(726, 10), IR(49012, 23), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatPowRINTEL, true, true, IR(726, 10), IR(49035, 24), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatPowNINTEL, true, true, IR(726, 10), IR(49059, 24), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpLoopControlINTEL, false, false, IR(18, 1), IR(49083, 17), IR(0, 0), IR(310, 1), IR(124, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpAliasDomainDeclINTEL, true, false, IR(742, 2), IR(49100, 21), IR(0, 0), IR(144, 1), IR(136, 1), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpAliasScopeDeclINTEL, true, false, IR(744, 3), IR(49121, 20), IR(0, 0), IR(144, 1), IR(136, 1), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpAliasScopeListDeclINTEL, true, false, IR(92, 2), IR(49141, 24), IR(0, 0), IR(144, 1), IR(136, 1), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFixedSqrtALTERA, true, true, IR(718, 8), IR(49165, 16), IR(205, 1), IR(204, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFixedRecipALTERA, true, true, IR(718, 8), IR(49196, 17), IR(206, 1), IR(204, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFixedRsqrtALTERA, true, true, IR(718, 8), IR(49229, 17), IR(207, 1), IR(204, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFixedSinALTERA, true, true, IR(718, 8), IR(49262, 15), IR(208, 1), IR(204, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFixedCosALTERA, true, true, IR(718, 8), IR(49291, 15), IR(209, 1), IR(204, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFixedSinCosALTERA, true, true, IR(718, 8), IR(49320, 18), IR(210, 1), IR(204, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFixedSinPiALTERA, true, true, IR(718, 8), IR(49355, 17), IR(211, 1), IR(204, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFixedCosPiALTERA, true, true, IR(718, 8), IR(49388, 17), IR(212, 1), IR(204, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFixedSinCosPiALTERA, true, true, IR(718, 8), IR(49421, 20), IR(213, 1), IR(204, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFixedLogALTERA, true, true, IR(718, 8), IR(49460, 15), IR(214, 1), IR(204, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFixedExpALTERA, true, true, IR(718, 8), IR(49489, 15), IR(215, 1), IR(204, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpPtrCastToCrossWorkgroupALTERA, true, true, IR(160, 3), IR(49518, 30), IR(216, 1), IR(215, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpCrossWorkgroupCastToPtrALTERA, true, true, IR(160, 3), IR(49577, 30), IR(217, 1), IR(215, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpReadPipeBlockingALTERA, false, false, IR(296, 4), IR(49636, 23), IR(218, 1), IR(311, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpWritePipeBlockingALTERA, false, false, IR(296, 4), IR(49681, 24), IR(219, 1), IR(311, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFPGARegALTERA, true, true, IR(160, 3), IR(15023, 14), IR(63, 1), IR(312, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpRayQueryGetRayTMinKHR, true, true, IR(160, 3), IR(49728, 22), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetRayFlagsKHR, true, true, IR(160, 3), IR(49750, 23), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionTKHR, true, true, IR(172, 4), IR(49773, 28), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionInstanceCustomIndexKHR, true, true, IR(172, 4), IR(49801, 46), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionInstanceIdKHR, true, true, IR(172, 4), IR(49847, 37), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionInstanceShaderBindingTableRecordOffsetKHR, true, true, IR(172, 4), IR(49884, 65), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionGeometryIndexKHR, true, true, IR(172, 4), IR(49949, 40), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionPrimitiveIndexKHR, true, true, IR(172, 4), IR(49989, 41), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionBarycentricsKHR, true, true, IR(172, 4), IR(50030, 39), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionFrontFaceKHR, true, true, IR(172, 4), IR(50069, 36), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionCandidateAABBOpaqueKHR, true, true, IR(160, 3), IR(50105, 46), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionObjectRayDirectionKHR, true, true, IR(172, 4), IR(50151, 45), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionObjectRayOriginKHR, true, true, IR(172, 4), IR(50196, 42), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetWorldRayDirectionKHR, true, true, IR(160, 3), IR(50238, 32), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetWorldRayOriginKHR, true, true, IR(160, 3), IR(50270, 29), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionObjectToWorldKHR, true, true, IR(172, 4), IR(50299, 40), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionWorldToObjectKHR, true, true, IR(172, 4), IR(50339, 40), IR(0, 0), IR(208, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpAtomicFAddEXT, true, true, IR(235, 6), IR(50379, 14), IR(0, 0), IR(313, 4), IR(161, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpTypeBufferSurfaceINTEL, true, false, IR(100, 2), IR(50393, 23), IR(0, 0), IR(136, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeStructContinuedINTEL, false, false, IR(747, 1), IR(50416, 25), IR(0, 0), IR(317, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpConstantCompositeContinuedINTEL, false, false, IR(747, 1), IR(50441, 32), IR(0, 0), IR(317, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpSpecConstantCompositeContinuedINTEL, false, false, IR(747, 1), IR(50473, 36), IR(0, 0), IR(317, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpCompositeConstructContinuedINTEL, false, false, IR(747, 1), IR(50509, 33), IR(0, 0), IR(317, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpConvertFToBF16INTEL, true, true, IR(160, 3), IR(50542, 20), IR(0, 0), IR(318, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpConvertBF16ToFINTEL, true, true, IR(160, 3), IR(50562, 20), IR(0, 0), IR(318, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpControlBarrierArriveEXT, false, false, IR(221, 3), IR(50582, 24), IR(220, 1), IR(319, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kBarrier},
  {spv::Op::OpControlBarrierWaitEXT, false, false, IR(221, 3), IR(50632, 22), IR(221, 1), IR(319, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kBarrier},
  {spv::Op::OpArithmeticFenceEXT, true, true, IR(160, 3), IR(15534, 19), IR(0, 0), IR(320, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMiscellaneous},
  {spv::Op::OpTaskSequenceCreateALTERA, true, true, IR(748, 7), IR(50678, 25), IR(222, 1), IR(321, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTaskSequenceAsyncALTERA, false, false, IR(168, 2), IR(50727, 24), IR(223, 1), IR(321, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTaskSequenceGetALTERA, true, true, IR(160, 3), IR(50774, 22), IR(224, 1), IR(321, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTaskSequenceReleaseALTERA, false, false, IR(10, 1), IR(50817, 26), IR(225, 1), IR(321, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeTaskSequenceALTERA, true, false, IR(68, 1), IR(50868, 23), IR(226, 1), IR(321, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupBlockPrefetchINTEL, false, false, IR(139, 3), IR(50913, 27), IR(0, 0), IR(322, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroup2DBlockLoadINTEL, false, false, IR(755, 10), IR(50940, 25), IR(0, 0), IR(112, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroup2DBlockLoadTransformINTEL, false, false, IR(755, 10), IR(50965, 34), IR(0, 0), IR(323, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroup2DBlockLoadTransposeINTEL, false, false, IR(755, 10), IR(50999, 34), IR(0, 0), IR(324, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroup2DBlockPrefetchINTEL, false, false, IR(765, 9), IR(51033, 29), IR(0, 0), IR(112, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroup2DBlockStoreINTEL, false, false, IR(755, 10), IR(51062, 26), IR(0, 0), IR(112, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupMatrixMultiplyAccumulateINTEL, true, true, IR(774, 7), IR(16011, 38), IR(0, 0), IR(325, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpBitwiseFunctionINTEL, true, true, IR(215, 6), IR(51150, 21), IR(0, 0), IR(326, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpUntypedVariableLengthArrayINTEL, true, true, IR(172, 4), IR(16077, 32), IR(0, 0), IR(327, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpConditionalExtensionINTEL, false, false, IR(45, 2), IR(51171, 26), IR(0, 0), IR(115, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kExtension},
  {spv::Op::OpConditionalEntryPointINTEL, false, false, IR(781, 5), IR(51197, 27), IR(0, 0), IR(115, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMode_Setting},
  {spv::Op::OpConditionalCapabilityINTEL, false, false, IR(786, 2), IR(51224, 27), IR(0, 0), IR(115, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMode_Setting},
  {spv::Op::OpSpecConstantTargetINTEL, true, true, IR(788, 4), IR(51251, 24), IR(0, 0), IR(328, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpSpecConstantArchitectureINTEL, true, true, IR(792, 6), IR(51275, 30), IR(0, 0), IR(328, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpSpecConstantCapabilitiesINTEL, true, true, IR(798, 3), IR(51342, 30), IR(0, 0), IR(328, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpConditionalCopyObjectINTEL, true, true, IR(107, 3), IR(51372, 27), IR(0, 0), IR(115, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpPredicatedLoadINTEL, true, true, IR(636, 6), IR(51399, 20), IR(0, 0), IR(329, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpPredicatedStoreINTEL, false, false, IR(589, 4), IR(51419, 21), IR(0, 0), IR(329, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpGroupIMulKHR, true, true, IR(283, 5), IR(51440, 13), IR(0, 0), IR(330, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupFMulKHR, true, true, IR(283, 5), IR(51453, 13), IR(0, 0), IR(330, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupBitwiseAndKHR, true, true, IR(283, 5), IR(51466, 19), IR(0, 0), IR(330, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupBitwiseOrKHR, true, true, IR(283, 5), IR(51485, 18), IR(0, 0), IR(330, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupBitwiseXorKHR, true, true, IR(283, 5), IR(51503, 19), IR(0, 0), IR(330, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupLogicalAndKHR, true, true, IR(283, 5), IR(51522, 19), IR(0, 0), IR(330, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupLogicalOrKHR, true, true, IR(283, 5), IR(51541, 18), IR(0, 0), IR(330, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupLogicalXorKHR, true, true, IR(283, 5), IR(51559, 19), IR(0, 0), IR(330, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpRoundFToTF32INTEL, true, true, IR(160, 3), IR(51578, 18), IR(0, 0), IR(331, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpMaskedGatherINTEL, true, true, IR(801, 6), IR(51596, 18), IR(0, 0), IR(332, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpMaskedScatterINTEL, false, false, IR(807, 4), IR(51614, 19), IR(0, 0), IR(332, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpConvertHandleToImageINTEL, true, true, IR(160, 3), IR(51633, 26), IR(0, 0), IR(333, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpConvertHandleToSamplerINTEL, true, true, IR(160, 3), IR(51659, 28), IR(0, 0), IR(333, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpConvertHandleToSampledImageINTEL, true, true, IR(160, 3), IR(51687, 33), IR(0, 0), IR(333, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpFDot2MixAcc32VALVE, true, true, IR(130, 5), IR(51720, 19), IR(0, 0), IR(334, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpFDot2MixAcc16VALVE, true, true, IR(130, 5), IR(51739, 19), IR(0, 0), IR(336, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpFDot4MixAcc32VALVE, true, true, IR(130, 5), IR(51758, 19), IR(0, 0), IR(338, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
}};

// Opcode strings (without the 'Op' prefix) and opcode values, ordered by name.
// The fields in order are:
//   name, either the primary name or an alias, indexing into kStrings
//   index into kInstructionDesc
static const std::array<NameIndex, 930> kInstructionNames{{
  {IR(11476, 9), 440}, // 0 AbortKHR
  {IR(42032, 13), 602}, // 1 AbsISubINTEL
  {IR(42045, 13), 603}, // 2 AbsUSubINTEL
  {IR(30605, 12), 58}, // 3 AccessChain
  {IR(49100, 21), 788}, // 4 AliasDomainDeclINTEL
  {IR(49121, 20), 789}, // 5 AliasScopeDeclINTEL
  {IR(49141, 24), 790}, // 6 AliasScopeListDeclINTEL
  {IR(31902, 4), 143}, // 7 All
  {IR(37297, 25), 428}, // 8 AllocateNodePayloadsAMDX
  {IR(31898, 4), 142}, // 9 Any
  {IR(48887, 24), 779}, // 10 ArbitraryFloatACosINTEL
  {IR(48911, 26), 780}, // 11 ArbitraryFloatACosPiINTEL
  {IR(48837, 24), 777}, // 12 ArbitraryFloatASinINTEL
  {IR(48861, 26), 778}, // 13 ArbitraryFloatASinPiINTEL
  {IR(48987, 25), 783}, // 14 ArbitraryFloatATan2INTEL
  {IR(48937, 24), 781}, // 15 ArbitraryFloatATanINTEL
  {IR(48961, 26), 782}, // 16 ArbitraryFloatATanPiINTEL
  {IR(47857, 24), 750}, // 17 ArbitraryFloatAddALTERA
  {IR(47881, 23), 750}, // 18 ArbitraryFloatAddINTEL
  {IR(47686, 25), 747}, // 19 ArbitraryFloatCastALTERA
  {IR(47735, 32), 748}, // 20 ArbitraryFloatCastFromIntALTERA
  {IR(47767, 31), 748}, // 21 ArbitraryFloatCastFromIntINTEL
  {IR(47711, 24), 747}, // 22 ArbitraryFloatCastINTEL
  {IR(47798, 30), 749}, // 23 ArbitraryFloatCastToIntALTERA
  {IR(47828, 29), 749}, // 24 ArbitraryFloatCastToIntINTEL
  {IR(48372, 25), 761}, // 25 ArbitraryFloatCbrtALTERA
  {IR(48397, 24), 761}, // 26 ArbitraryFloatCbrtINTEL
  {IR(48738, 23), 773}, // 27 ArbitraryFloatCosINTEL
  {IR(48812, 25), 776}, // 28 ArbitraryFloatCosPiINTEL
  {IR(47998, 24), 753}, // 29 ArbitraryFloatDivALTERA
  {IR(48022, 23), 753}, // 30 ArbitraryFloatDivINTEL
  {IR(48225, 23), 758}, // 31 ArbitraryFloatEQALTERA
  {IR(48248, 22), 758}, // 32 ArbitraryFloatEQINTEL
  {IR(48665, 25), 770}, // 33 ArbitraryFloatExp10INTEL
  {IR(48641, 24), 769}, // 34 ArbitraryFloatExp2INTEL
  {IR(48618, 23), 768}, // 35 ArbitraryFloatExpINTEL
  {IR(48690, 25), 771}, // 36 ArbitraryFloatExpm1INTEL
  {IR(48090, 23), 755}, // 37 ArbitraryFloatGEALTERA
  {IR(48113, 22), 755}, // 38 ArbitraryFloatGEINTEL
  {IR(48045, 23), 754}, // 39 ArbitraryFloatGTALTERA
  {IR(48068, 22), 754}, // 40 ArbitraryFloatGTINTEL
  {IR(48421, 26), 762}, // 41 ArbitraryFloatHypotALTERA
  {IR(48447, 25), 762}, // 42 ArbitraryFloatHypotINTEL
  {IR(48180, 23), 757}, // 43 ArbitraryFloatLEALTERA
  {IR(48203, 22), 757}, // 44 ArbitraryFloatLEINTEL
  {IR(48135, 23), 756}, // 45 ArbitraryFloatLTALTERA
  {IR(48158, 22), 756}, // 46 ArbitraryFloatLTINTEL
  {IR(48568, 25), 766}, // 47 ArbitraryFloatLog10INTEL
  {IR(48593, 25), 767}, // 48 ArbitraryFloatLog1pINTEL
  {IR(48544, 24), 765}, // 49 ArbitraryFloatLog2INTEL
  {IR(48521, 23), 764}, // 50 ArbitraryFloatLogINTEL
  {IR(47951, 24), 752}, // 51 ArbitraryFloatMulALTERA
  {IR(47975, 23), 752}, // 52 ArbitraryFloatMulINTEL
  {IR(49012, 23), 784}, // 53 ArbitraryFloatPowINTEL
  {IR(49059, 24), 786}, // 54 ArbitraryFloatPowNINTEL
  {IR(49035, 24), 785}, // 55 ArbitraryFloatPowRINTEL
  {IR(48321, 26), 760}, // 56 ArbitraryFloatRSqrtALTERA
  {IR(48347, 25), 760}, // 57 ArbitraryFloatRSqrtINTEL
  {IR(48270, 26), 759}, // 58 ArbitraryFloatRecipALTERA
  {IR(48296, 25), 759}, // 59 ArbitraryFloatRecipINTEL
  {IR(48761, 26), 774}, // 60 ArbitraryFloatSinCosINTEL
  {IR(47629, 29), 746}, // 61 ArbitraryFloatSinCosPiALTERA
  {IR(47658, 28), 746}, // 62 ArbitraryFloatSinCosPiINTEL
  {IR(48715, 23), 772}, // 63 ArbitraryFloatSinINTEL
  {IR(48787, 25), 775}, // 64 ArbitraryFloatSinPiINTEL
  {IR(48472, 25), 763}, // 65 ArbitraryFloatSqrtALTERA
  {IR(48497, 24), 763}, // 66 ArbitraryFloatSqrtINTEL
  {IR(47904, 24), 751}, // 67 ArbitraryFloatSubALTERA
  {IR(47928, 23), 751}, // 68 ArbitraryFloatSubINTEL
  {IR(15534, 19), 834}, // 69 ArithmeticFenceEXT
  {IR(30652, 12), 61}, // 70 ArrayLength
  {IR(42279, 13), 618}, // 71 AsmCallINTEL
  {IR(13827, 9), 617}, // 72 AsmINTEL
  {IR(42264, 15), 616}, // 73 AsmTargetINTEL
  {IR(42320, 14), 621}, // 74 AssumeTrueKHR
  {IR(32924, 10), 220}, // 75 AtomicAnd
  {IR(32776, 22), 210}, // 76 AtomicCompareExchange
  {IR(32798, 26), 211}, // 77 AtomicCompareExchangeWeak
  {IR(32761, 15), 209}, // 78 AtomicExchange
  {IR(50379, 14), 824}, // 79 AtomicFAddEXT
  {IR(42306, 14), 620}, // 80 AtomicFMaxEXT
  {IR(42292, 14), 619}, // 81 AtomicFMinEXT
  {IR(34309, 16), 292}, // 82 AtomicFlagClear
  {IR(34288, 21), 291}, // 83 AtomicFlagTestAndSet
  {IR(32858, 11), 214}, // 84 AtomicIAdd
  {IR(32841, 17), 213}, // 85 AtomicIDecrement
  {IR(32824, 17), 212}, // 86 AtomicIIncrement
  {IR(32869, 11), 215}, // 87 AtomicISub
  {IR(32738, 11), 207}, // 88 AtomicLoad
  {IR(32934, 9), 221}, // 89 AtomicOr
  {IR(32902, 11), 218}, // 90 AtomicSMax
  {IR(32880, 11), 216}, // 91 AtomicSMin
  {IR(32749, 12), 208}, // 92 AtomicStore
  {IR(32913, 11), 219}, // 93 AtomicUMax
  {IR(32891, 11), 217}, // 94 AtomicUMin
  {IR(32943, 10), 222}, // 95 AtomicXor
  {IR(40528, 28), 548}, // 96 BeginInvocationInterlockEXT
  {IR(36835, 17), 408}, // 97 BitCastArrayQCOM
  {IR(32522, 9), 191}, // 98 BitCount
  {IR(32462, 15), 187}, // 99 BitFieldInsert
  {IR(32477, 17), 188}, // 100 BitFieldSExtract
  {IR(32494, 17), 189}, // 101 BitFieldUExtract
  {IR(32511, 11), 190}, // 102 BitReverse
  {IR(31650, 8), 114}, // 103 Bitcast
  {IR(10402, 18), 358}, // 104 BitcastExtractEXT
  {IR(32447, 11), 185}, // 105 BitwiseAnd
  {IR(51150, 21), 847}, // 106 BitwiseFunctionINTEL
  {IR(32426, 10), 183}, // 107 BitwiseOr
  {IR(32436, 11), 184}, // 108 BitwiseXor
  {IR(33053, 7), 227}, // 109 Branch
  {IR(33060, 18), 228}, // 110 BranchConditional
  {IR(37558, 17), 439}, // 111 BufferPointerEXT
  {IR(33921, 13), 277}, // 112 BuildNDRange
  {IR(29666, 11), 15}, // 113 Capability
  {IR(33879, 26), 275}, // 114 CaptureEventProfilingInfo
  {IR(35385, 23), 344}, // 115 ColorAttachmentReadEXT
  {IR(33471, 15), 255}, // 116 CommitReadPipe
  {IR(33486, 16), 256}, // 117 CommitWritePipe
  {IR(30912, 19), 72}, // 118 CompositeConstruct
  {IR(50509, 33), 829}, // 119 CompositeConstructContinuedINTEL
  {IR(36968, 30), 413}, // 120 CompositeConstructCoopMatQCOM
  {IR(36543, 31), 396}, // 121 CompositeConstructReplicateEXT
  {IR(30931, 17), 73}, // 122 CompositeExtract
  {IR(36998, 28), 414}, // 123 CompositeExtractCoopMatQCOM
  {IR(30948, 16), 74}, // 124 CompositeInsert
  {IR(51224, 27), 851}, // 125 ConditionalCapabilityINTEL
  {IR(51372, 27), 855}, // 126 ConditionalCopyObjectINTEL
  {IR(51197, 27), 850}, // 127 ConditionalEntryPointINTEL
  {IR(51171, 26), 849}, // 128 ConditionalExtensionINTEL
  {IR(17833, 9), 39}, // 129 Constant
  {IR(30172, 18), 40}, // 130 ConstantComposite
  {IR(50441, 32), 827}, // 131 ConstantCompositeContinuedINTEL
  {IR(36479, 30), 394}, // 132 ConstantCompositeReplicateEXT
  {IR(11485, 16), 444}, // 133 ConstantDataKHR
  {IR(30120, 14), 38}, // 134 ConstantFalse
  {IR(42210, 29), 614}, // 135 ConstantFunctionPointerINTEL
  {IR(30284, 13), 42}, // 136 ConstantNull
  {IR(34364, 20), 296}, // 137 ConstantPipeStorage
  {IR(30268, 16), 41}, // 138 ConstantSampler
  {IR(37623, 18), 443}, // 139 ConstantSizeOfEXT
  {IR(37450, 19), 434}, // 140 ConstantStringAMDX
  {IR(30107, 13), 37}, // 141 ConstantTrue
  {IR(32709, 15), 205}, // 142 ControlBarrier
  {IR(50582, 24), 832}, // 143 ControlBarrierArriveEXT
  {IR(50606, 26), 832}, // 144 ControlBarrierArriveINTEL
  {IR(50632, 22), 833}, // 145 ControlBarrierWaitEXT
  {IR(50654, 24), 833}, // 146 ControlBarrierWaitINTEL
  {IR(50562, 20), 831}, // 147 ConvertBF16ToFINTEL
  {IR(50542, 20), 830}, // 148 ConvertFToBF16INTEL
  {IR(31456, 12), 100}, // 149 ConvertFToS
  {IR(31444, 12), 99}, // 150 ConvertFToU
  {IR(51633, 26), 869}, // 151 ConvertHandleToImageINTEL
  {IR(51687, 33), 871}, // 152 ConvertHandleToSampledImageINTEL
  {IR(51659, 28), 870}, // 153 ConvertHandleToSamplerINTEL
  {IR(41177, 18), 571}, // 154 ConvertImageToUNV
  {IR(31533, 14), 107}, // 155 ConvertPtrToU
  {IR(31468, 12), 101}, // 156 ConvertSToF
  {IR(41240, 25), 574}, // 157 ConvertSampledImageToUNV
  {IR(41195, 20), 572}, // 158 ConvertSamplerToUNV
  {IR(36057, 35), 380}, // 159 ConvertUToAccelerationStructureKHR
  {IR(31480, 12), 102}, // 160 ConvertUToF
  {IR(41139, 18), 569}, // 161 ConvertUToImageNV
  {IR(31577, 14), 110}, // 162 ConvertUToPtr
  {IR(41215, 25), 573}, // 163 ConvertUToSampledImageNV
  {IR(41157, 20), 570}, // 164 ConvertUToSamplerNV
  {IR(38763, 27), 487}, // 165 CooperativeMatrixConvertNV
  {IR(36452, 27), 393}, // 166 CooperativeMatrixLengthKHR
  {IR(40502, 26), 547}, // 167 CooperativeMatrixLengthNV
  {IR(36320, 25), 390}, // 168 CooperativeMatrixLoadKHR
  {IR(40427, 24), 544}, // 169 CooperativeMatrixLoadNV
  {IR(40726, 30), 551}, // 170 CooperativeMatrixLoadTensorNV
  {IR(36425, 27), 392}, // 171 CooperativeMatrixMulAddKHR
  {IR(40476, 26), 546}, // 172 CooperativeMatrixMulAddNV
  {IR(40787, 32), 553}, // 173 CooperativeMatrixPerElementOpNV
  {IR(40625, 26), 550}, // 174 CooperativeMatrixReduceNV
  {IR(36345, 26), 391}, // 175 CooperativeMatrixStoreKHR
  {IR(40451, 25), 545}, // 176 CooperativeMatrixStoreNV
  {IR(40756, 31), 552}, // 177 CooperativeMatrixStoreTensorNV
  {IR(41110, 29), 568}, // 178 CooperativeMatrixTransposeNV
  {IR(38986, 24), 494}, // 179 CooperativeVectorLoadNV
  {IR(38731, 32), 486}, // 180 CooperativeVectorMatrixMulAddNV
  {IR(38621, 29), 483}, // 181 CooperativeVectorMatrixMulNV
  {IR(38650, 42), 484}, // 182 CooperativeVectorOuterProductAccumulateNV
  {IR(38692, 39), 485}, // 183 CooperativeVectorReduceSumAccumulateNV
  {IR(39010, 25), 495}, // 184 CooperativeVectorStoreNV
  {IR(35344, 12), 340}, // 185 CopyLogical
  {IR(30578, 11), 56}, // 186 CopyMemory
  {IR(30589, 16), 57}, // 187 CopyMemorySized
  {IR(30964, 11), 75}, // 188 CopyObject
  {IR(34384, 26), 297}, // 189 CreatePipeFromPipeStorage
  {IR(40855, 21), 556}, // 190 CreateTensorLayoutNV
  {IR(40975, 19), 561}, // 191 CreateTensorViewNV
  {IR(33831, 16), 272}, // 192 CreateUserEvent
  {IR(49577, 30), 803}, // 193 CrossWorkgroupCastToPtrALTERA
  {IR(49607, 29), 803}, // 194 CrossWorkgroupCastToPtrINTEL
  {IR(32531, 5), 192}, // 195 DPdx
  {IR(32577, 11), 198}, // 196 DPdxCoarse
  {IR(32548, 9), 195}, // 197 DPdxFine
  {IR(32536, 5), 193}, // 198 DPdy
  {IR(32588, 11), 199}, // 199 DPdyCoarse
  {IR(32557, 9), 196}, // 200 DPdyFine
  {IR(30738, 9), 64}, // 201 Decorate
  {IR(34561, 11), 305}, // 202 DecorateId
  {IR(42344, 15), 623}, // 203 DecorateString
  {IR(42359, 21), 623}, // 204 DecorateStringGOOGLE
  {IR(30762, 16), 66}, // 205 DecorationGroup
  {IR(13016, 25), 564}, // 206 DemoteToHelperInvocation
  {IR(13041, 28), 564}, // 207 DemoteToHelperInvocationEXT
  {IR(35408, 23), 345}, // 208 DepthAttachmentReadEXT
  {IR(31847, 4), 137}, // 209 Dot
  {IR(38790, 17), 488}, // 210 EmitMeshTasksEXT
  {IR(32636, 17), 203}, // 211 EmitStreamVertex
  {IR(32612, 11), 201}, // 212 EmitVertex
  {IR(40556, 26), 549}, // 213 EndInvocationInterlockEXT
  {IR(32623, 13), 202}, // 214 EndPrimitive
  {IR(32653, 19), 204}, // 215 EndStreamPrimitive
  {IR(33667, 14), 265}, // 216 EnqueueKernel
  {IR(33653, 14), 264}, // 217 EnqueueMarker
  {IR(37322, 24), 429}, // 218 EnqueueNodePayloadsAMDX
  {IR(29581, 11), 13}, // 219 EntryPoint
  {IR(36038, 19), 379}, // 220 ExecuteCallableKHR
  {IR(40138, 18), 534}, // 221 ExecuteCallableNV
  {IR(29624, 14), 14}, // 222 ExecutionMode
  {IR(34545, 16), 304}, // 223 ExecutionModeId
  {IR(42334, 10), 622}, // 224 ExpectKHR
  {IR(29435, 8), 11}, // 225 ExtInst
  {IR(29375, 14), 10}, // 226 ExtInstImport
  {IR(35975, 26), 376}, // 227 ExtInstWithForwardRefsKHR
  {IR(29365, 10), 9}, // 228 Extension
  {IR(37026, 20), 415}, // 229 ExtractSubArrayQCOM
  {IR(31679, 5), 118}, // 230 FAdd
  {IR(31510, 9), 105}, // 231 FConvert
  {IR(31714, 5), 125}, // 232 FDiv
  {IR(51739, 19), 873}, // 233 FDot2MixAcc16VALVE
  {IR(51720, 19), 872}, // 234 FDot2MixAcc32VALVE
  {IR(51758, 19), 874}, // 235 FDot4MixAcc32VALVE
  {IR(31739, 5), 130}, // 236 FMod
  {IR(31699, 5), 122}, // 237 FMul
  {IR(31666, 8), 116}, // 238 FNegate
  {IR(32176, 10), 168}, // 239 FOrdEqual
  {IR(32254, 16), 174}, // 240 FOrdGreaterThan
  {IR(32326, 21), 178}, // 241 FOrdGreaterThanEqual
  {IR(32226, 13), 172}, // 242 FOrdLessThan
  {IR(32288, 18), 176}, // 243 FOrdLessThanEqual
  {IR(32198, 13), 170}, // 244 FOrdNotEqual
  {IR(15023, 14), 806}, // 245 FPGARegALTERA
  {IR(15037, 13), 806}, // 246 FPGARegINTEL
  {IR(31734, 5), 129}, // 247 FRem
  {IR(31689, 5), 120}, // 248 FSub
  {IR(32186, 12), 169}, // 249 FUnordEqual
  {IR(32270, 18), 175}, // 250 FUnordGreaterThan
  {IR(32347, 23), 179}, // 251 FUnordGreaterThanEqual
  {IR(32239, 15), 173}, // 252 FUnordLessThan
  {IR(32306, 20), 177}, // 253 FUnordLessThanEqual
  {IR(32211, 15), 171}, // 254 FUnordNotEqual
  {IR(38948, 38), 493}, // 255 FetchMicroTriangleVertexBarycentricNV
  {IR(38913, 35), 492}, // 256 FetchMicroTriangleVertexPositionNV
  {IR(37371, 29), 431}, // 257 FinishWritingNodePayloadAMDX
  {IR(49291, 15), 795}, // 258 FixedCosALTERA
  {IR(49306, 14), 795}, // 259 FixedCosINTEL
  {IR(49388, 17), 798}, // 260 FixedCosPiALTERA
  {IR(49405, 16), 798}, // 261 FixedCosPiINTEL
  {IR(49489, 15), 801}, // 262 FixedExpALTERA
  {IR(49504, 14), 801}, // 263 FixedExpINTEL
  {IR(49460, 15), 800}, // 264 FixedLogALTERA
  {IR(49475, 14), 800}, // 265 FixedLogINTEL
  {IR(49196, 17), 792}, // 266 FixedRecipALTERA
  {IR(49213, 16), 792}, // 267 FixedRecipINTEL
  {IR(49229, 17), 793}, // 268 FixedRsqrtALTERA
  {IR(49246, 16), 793}, // 269 FixedRsqrtINTEL
  {IR(49262, 15), 794}, // 270 FixedSinALTERA
  {IR(49320, 18), 796}, // 271 FixedSinCosALTERA
  {IR(49338, 17), 796}, // 272 FixedSinCosINTEL
  {IR(49421, 20), 799}, // 273 FixedSinCosPiALTERA
  {IR(49441, 19), 799}, // 274 FixedSinCosPiINTEL
  {IR(49277, 14), 794}, // 275 FixedSinINTEL
  {IR(49355, 17), 797}, // 276 FixedSinPiALTERA
  {IR(49372, 16), 797}, // 277 FixedSinPiINTEL
  {IR(49165, 16), 791}, // 278 FixedSqrtALTERA
  {IR(49181, 15), 791}, // 279 FixedSqrtINTEL
  {IR(35892, 7), 370}, // 280 FmaKHR
  {IR(37267, 17), 426}, // 281 FragmentFetchAMD
  {IR(37246, 21), 425}, // 282 FragmentMaskFetchAMD
  {IR(37671, 10), 447}, // 283 FreezeKHR
  {IR(28414, 9), 48}, // 284 Function
  {IR(30487, 13), 51}, // 285 FunctionCall
  {IR(30475, 12), 50}, // 286 FunctionEnd
  {IR(30457, 18), 49}, // 287 FunctionParameter
  {IR(42239, 25), 615}, // 288 FunctionPointerCallINTEL
  {IR(32541, 7), 194}, // 289 Fwidth
  {IR(32599, 13), 200}, // 290 FwidthCoarse
  {IR(32566, 11), 197}, // 291 FwidthFine
  {IR(31608, 17), 112}, // 292 GenericCastToPtr
  {IR(31625, 25), 113}, // 293 GenericCastToPtrExplicit
  {IR(30664, 23), 62}, // 294 GenericPtrMemSemantics
  {IR(33905, 16), 276}, // 295 GetDefaultQueue
  {IR(34410, 35), 298}, // 296 GetKernelLocalSizeForSubgroupCount
  {IR(34445, 25), 299}, // 297 GetKernelMaxNumSubgroups
  {IR(33711, 32), 267}, // 298 GetKernelNDrangeMaxSubGroupSize
  {IR(33681, 30), 266}, // 299 GetKernelNDrangeSubGroupCount
  {IR(33766, 40), 269}, // 300 GetKernelPreferredWorkGroupSizeMultiple
  {IR(33743, 23), 268}, // 301 GetKernelWorkGroupSize
  {IR(33537, 18), 259}, // 302 GetMaxPipePackets
  {IR(33519, 18), 258}, // 303 GetNumPipePackets
  {IR(10255, 9), 353}, // 304 GraphARM
  {IR(35560, 17), 351}, // 305 GraphConstantARM
  {IR(35628, 12), 356}, // 306 GraphEndARM
  {IR(35577, 19), 352}, // 307 GraphEntryPointARM
  {IR(35596, 14), 354}, // 308 GraphInputARM
  {IR(35610, 18), 355}, // 309 GraphSetOutputARM
  {IR(33224, 9), 238}, // 310 GroupAll
  {IR(33233, 9), 239}, // 311 GroupAny
  {IR(33193, 15), 236}, // 312 GroupAsyncCopy
  {IR(51466, 19), 860}, // 313 GroupBitwiseAndKHR
  {IR(51485, 18), 861}, // 314 GroupBitwiseOrKHR
  {IR(51503, 19), 862}, // 315 GroupBitwiseXorKHR
  {IR(33242, 15), 240}, // 316 GroupBroadcast
  {IR(33612, 20), 262}, // 317 GroupCommitReadPipe
  {IR(33632, 21), 263}, // 318 GroupCommitWritePipe
  {IR(30778, 14), 67}, // 319 GroupDecorate
  {IR(33300, 10), 242}, // 320 GroupFAdd
  {IR(37085, 23), 418}, // 321 GroupFAddNonUniformAMD
  {IR(33340, 10), 246}, // 322 GroupFMax
  {IR(37177, 23), 422}, // 323 GroupFMaxNonUniformAMD
  {IR(33310, 10), 243}, // 324 GroupFMin
  {IR(37108, 23), 419}, // 325 GroupFMinNonUniformAMD
  {IR(51453, 13), 859}, // 326 GroupFMulKHR
  {IR(33290, 10), 241}, // 327 GroupIAdd
  {IR(37062, 23), 417}, // 328 GroupIAddNonUniformAMD
  {IR(51440, 13), 858}, // 329 GroupIMulKHR
  {IR(51522, 19), 863}, // 330 GroupLogicalAndKHR
  {IR(51541, 18), 864}, // 331 GroupLogicalOrKHR
  {IR(51559, 19), 865}, // 332 GroupLogicalXorKHR
  {IR(30837, 20), 68}, // 333 GroupMemberDecorate
  {IR(34593, 19), 307}, // 334 GroupNonUniformAll
  {IR(34631, 24), 309}, // 335 GroupNonUniformAllEqual
  {IR(34612, 19), 308}, // 336 GroupNonUniformAny
  {IR(6825, 22), 312}, // 337 GroupNonUniformBallot
  {IR(34771, 30), 315}, // 338 GroupNonUniformBallotBitCount
  {IR(34739, 32), 314}, // 339 GroupNonUniformBallotBitExtract
  {IR(34801, 29), 316}, // 340 GroupNonUniformBallotFindLSB
  {IR(34830, 29), 317}, // 341 GroupNonUniformBallotFindMSB
  {IR(35137, 26), 332}, // 342 GroupNonUniformBitwiseAnd
  {IR(35163, 25), 333}, // 343 GroupNonUniformBitwiseOr
  {IR(35188, 26), 334}, // 344 GroupNonUniformBitwiseXor
  {IR(34655, 25), 310}, // 345 GroupNonUniformBroadcast
  {IR(34680, 30), 311}, // 346 GroupNonUniformBroadcastFirst
  {IR(34572, 21), 306}, // 347 GroupNonUniformElect
  {IR(34957, 20), 323}, // 348 GroupNonUniformFAdd
  {IR(35117, 20), 331}, // 349 GroupNonUniformFMax
  {IR(35057, 20), 328}, // 350 GroupNonUniformFMin
  {IR(34997, 20), 325}, // 351 GroupNonUniformFMul
  {IR(34937, 20), 322}, // 352 GroupNonUniformIAdd
  {IR(34977, 20), 324}, // 353 GroupNonUniformIMul
  {IR(34710, 29), 313}, // 354 GroupNonUniformInverseBallot
  {IR(35214, 26), 335}, // 355 GroupNonUniformLogicalAnd
  {IR(35240, 25), 336}, // 356 GroupNonUniformLogicalOr
  {IR(35265, 26), 337}, // 357 GroupNonUniformLogicalXor
  {IR(38825, 28), 490}, // 358 GroupNonUniformPartitionEXT
  {IR(38853, 27), 490}, // 359 GroupNonUniformPartitionNV
  {IR(37492, 26), 436}, // 360 GroupNonUniformQuadAllKHR
  {IR(37518, 26), 437}, // 361 GroupNonUniformQuadAnyKHR
  {IR(35291, 29), 338}, // 362 GroupNonUniformQuadBroadcast
  {IR(35320, 24), 339}, // 363 GroupNonUniformQuadSwap
  {IR(15262, 25), 374}, // 364 GroupNonUniformRotateKHR
  {IR(35077, 20), 329}, // 365 GroupNonUniformSMax
  {IR(35017, 20), 326}, // 366 GroupNonUniformSMin
  {IR(9964, 23), 318}, // 367 GroupNonUniformShuffle
  {IR(34910, 27), 321}, // 368 GroupNonUniformShuffleDown
  {IR(34885, 25), 320}, // 369 GroupNonUniformShuffleUp
  {IR(34859, 26), 319}, // 370 GroupNonUniformShuffleXor
  {IR(35097, 20), 330}, // 371 GroupNonUniformUMax
  {IR(35037, 20), 327}, // 372 GroupNonUniformUMin
  {IR(33555, 28), 260}, // 373 GroupReserveReadPipePackets
  {IR(33583, 29), 261}, // 374 GroupReserveWritePipePackets
  {IR(33360, 10), 248}, // 375 GroupSMax
  {IR(37223, 23), 424}, // 376 GroupSMaxNonUniformAMD
  {IR(33330, 10), 245}, // 377 GroupSMin
  {IR(37154, 23), 421}, // 378 GroupSMinNonUniformAMD
  {IR(33350, 10), 247}, // 379 GroupUMax
  {IR(37200, 23), 423}, // 380 GroupUMaxNonUniformAMD
  {IR(33320, 10), 244}, // 381 GroupUMin
  {IR(37131, 23), 420}, // 382 GroupUMinNonUniformAMD
  {IR(33208, 16), 237}, // 383 GroupWaitEvents
  {IR(39482, 26), 511}, // 384 HitObjectExecuteShaderEXT
  {IR(38116, 25), 463}, // 385 HitObjectExecuteShaderNV
  {IR(39535, 26), 513}, // 386 HitObjectGetAttributesEXT
  {IR(38167, 25), 465}, // 387 HitObjectGetAttributesNV
  {IR(40214, 24), 536}, // 388 HitObjectGetClusterIdNV
  {IR(39508, 27), 512}, // 389 HitObjectGetCurrentTimeEXT
  {IR(38141, 26), 464}, // 390 HitObjectGetCurrentTimeNV
  {IR(39614, 29), 516}, // 391 HitObjectGetGeometryIndexEXT
  {IR(38243, 28), 468}, // 392 HitObjectGetGeometryIndexNV
  {IR(39561, 23), 514}, // 393 HitObjectGetHitKindEXT
  {IR(38192, 22), 466}, // 394 HitObjectGetHitKindNV
  {IR(39669, 35), 518}, // 395 HitObjectGetInstanceCustomIndexEXT
  {IR(38296, 34), 470}, // 396 HitObjectGetInstanceCustomIndexNV
  {IR(39643, 26), 517}, // 397 HitObjectGetInstanceIdEXT
  {IR(38271, 25), 469}, // 398 HitObjectGetInstanceIdNV
  {IR(39115, 51), 499}, // 399 HitObjectGetIntersectionTriangleVertexPositionsEXT
  {IR(41606, 27), 584}, // 400 HitObjectGetLSSPositionsNV
  {IR(41633, 23), 585}, // 401 HitObjectGetLSSRadiiNV
  {IR(39735, 34), 520}, // 402 HitObjectGetObjectRayDirectionEXT
  {IR(37828, 33), 453}, // 403 HitObjectGetObjectRayDirectionNV
  {IR(39704, 31), 519}, // 404 HitObjectGetObjectRayOriginEXT
  {IR(37861, 30), 454}, // 405 HitObjectGetObjectRayOriginNV
  {IR(39832, 29), 523}, // 406 HitObjectGetObjectToWorldEXT
  {IR(37800, 28), 452}, // 407 HitObjectGetObjectToWorldNV
  {IR(39584, 30), 515}, // 408 HitObjectGetPrimitiveIndexEXT
  {IR(38214, 29), 467}, // 409 HitObjectGetPrimitiveIndexNV
  {IR(39166, 24), 500}, // 410 HitObjectGetRayFlagsEXT
  {IR(39890, 23), 525}, // 411 HitObjectGetRayTMaxEXT
  {IR(38391, 22), 473}, // 412 HitObjectGetRayTMaxNV
  {IR(40238, 23), 537}, // 413 HitObjectGetRayTMinEXT
  {IR(38413, 22), 474}, // 414 HitObjectGetRayTMinNV
  {IR(40261, 45), 538}, // 415 HitObjectGetShaderBindingTableRecordIndexEXT
  {IR(37956, 44), 457}, // 416 HitObjectGetShaderBindingTableRecordIndexNV
  {IR(40306, 40), 539}, // 417 HitObjectGetShaderRecordBufferHandleEXT
  {IR(37917, 39), 456}, // 418 HitObjectGetShaderRecordBufferHandleNV
  {IR(41550, 29), 582}, // 419 HitObjectGetSpherePositionNV
  {IR(41579, 27), 583}, // 420 HitObjectGetSphereRadiusNV
  {IR(39769, 33), 521}, // 421 HitObjectGetWorldRayDirectionEXT
  {IR(38330, 32), 471}, // 422 HitObjectGetWorldRayDirectionNV
  {IR(39802, 30), 522}, // 423 HitObjectGetWorldRayOriginEXT
  {IR(38362, 29), 472}, // 424 HitObjectGetWorldRayOriginNV
  {IR(39861, 29), 524}, // 425 HitObjectGetWorldToObjectEXT
  {IR(37772, 28), 451}, // 426 HitObjectGetWorldToObjectNV
  {IR(40346, 20), 540}, // 427 HitObjectIsEmptyEXT
  {IR(38435, 19), 475}, // 428 HitObjectIsEmptyNV
  {IR(40366, 18), 541}, // 429 HitObjectIsHitEXT
  {IR(38454, 17), 476}, // 430 HitObjectIsHitNV
  {IR(41679, 20), 587}, // 431 HitObjectIsLSSHitNV
  {IR(40384, 19), 542}, // 432 HitObjectIsMissEXT
  {IR(38471, 18), 477}, // 433 HitObjectIsMissNV
  {IR(41656, 23), 586}, // 434 HitObjectIsSphereHitNV
  {IR(39458, 24), 510}, // 435 HitObjectRecordEmptyEXT
  {IR(38000, 23), 458}, // 436 HitObjectRecordEmptyNV
  {IR(39035, 28), 496}, // 437 HitObjectRecordFromQueryEXT
  {IR(37681, 27), 448}, // 438 HitObjectRecordHitMotionNV
  {IR(38043, 21), 460}, // 439 HitObjectRecordHitNV
  {IR(37708, 36), 449}, // 440 HitObjectRecordHitWithIndexMotionNV
  {IR(38064, 30), 461}, // 441 HitObjectRecordHitWithIndexNV
  {IR(39063, 23), 497}, // 442 HitObjectRecordMissEXT
  {IR(39086, 29), 498}, // 443 HitObjectRecordMissMotionEXT
  {IR(37744, 28), 450}, // 444 HitObjectRecordMissMotionNV
  {IR(38094, 22), 462}, // 445 HitObjectRecordMissNV
  {IR(39235, 33), 502}, // 446 HitObjectReorderExecuteShaderEXT
  {IR(39190, 45), 501}, // 447 HitObjectSetShaderBindingTableRecordIndexEXT
  {IR(39300, 38), 504}, // 448 HitObjectTraceMotionReorderExecuteEXT
  {IR(39410, 21), 508}, // 449 HitObjectTraceRayEXT
  {IR(39431, 27), 509}, // 450 HitObjectTraceRayMotionEXT
  {IR(37891, 26), 455}, // 451 HitObjectTraceRayMotionNV
  {IR(38023, 20), 459}, // 452 HitObjectTraceRayNV
  {IR(39268, 32), 503}, // 453 HitObjectTraceReorderExecuteEXT
  {IR(31674, 5), 117}, // 454 IAdd
  {IR(31851, 10), 138}, // 455 IAddCarry
  {IR(42058, 13), 604}, // 456 IAddSatINTEL
  {IR(42084, 14), 606}, // 457 IAverageINTEL
  {IR(42112, 21), 608}, // 458 IAverageRoundedINTEL
  {IR(32047, 7), 158}, // 459 IEqual
  {IR(31694, 5), 121}, // 460 IMul
  {IR(42180, 15), 612}, // 461 IMul32x16INTEL
  {IR(32054, 10), 159}, // 462 INotEqual
  {IR(31684, 5), 119}, // 463 ISub
  {IR(31861, 11), 139}, // 464 ISubBorrow
  {IR(42154, 13), 610}, // 465 ISubSatINTEL
  {IR(36092, 22), 381}, // 466 IgnoreIntersectionKHR
  {IR(39956, 21), 527}, // 467 IgnoreIntersectionNV
  {IR(28458, 6), 91}, // 468 Image
  {IR(36939, 29), 412}, // 469 ImageBlockMatchGatherSADQCOM
  {IR(36910, 29), 411}, // 470 ImageBlockMatchGatherSSDQCOM
  {IR(36812, 23), 407}, // 471 ImageBlockMatchSADQCOM
  {IR(36789, 23), 406}, // 472 ImageBlockMatchSSDQCOM
  {IR(36881, 29), 410}, // 473 ImageBlockMatchWindowSADQCOM
  {IR(36852, 29), 409}, // 474 ImageBlockMatchWindowSSDQCOM
  {IR(36770, 19), 405}, // 475 ImageBoxFilterQCOM
  {IR(31292, 16), 88}, // 476 ImageDrefGather
  {IR(31269, 11), 86}, // 477 ImageFetch
  {IR(31280, 12), 87}, // 478 ImageGather
  {IR(37046, 16), 416}, // 479 ImageGatherQCOM
  {IR(31329, 17), 92}, // 480 ImageQueryFormat
  {IR(31409, 17), 97}, // 481 ImageQueryLevels
  {IR(31395, 14), 96}, // 482 ImageQueryLod
  {IR(31346, 16), 93}, // 483 ImageQueryOrder
  {IR(31426, 18), 98}, // 484 ImageQuerySamples
  {IR(31380, 15), 95}, // 485 ImageQuerySize
  {IR(31362, 18), 94}, // 486 ImageQuerySizeLod
  {IR(31308, 10), 89}, // 487 ImageRead
  {IR(31126, 27), 81}, // 488 ImageSampleDrefExplicitLod
  {IR(31099, 27), 80}, // 489 ImageSampleDrefImplicitLod
  {IR(31076, 23), 79}, // 490 ImageSampleExplicitLod
  {IR(38558, 23), 481}, // 491 ImageSampleFootprintNV
  {IR(31030, 23), 78}, // 492 ImageSampleImplicitLod
  {IR(31238, 31), 85}, // 493 ImageSampleProjDrefExplicitLod
  {IR(31207, 31), 84}, // 494 ImageSampleProjDrefImplicitLod
  {IR(31180, 27), 83}, // 495 ImageSampleProjExplicitLod
  {IR(31153, 27), 82}, // 496 ImageSampleProjImplicitLod
  {IR(36746, 24), 404}, // 497 ImageSampleWeightedQCOM
  {IR(34233, 22), 288}, // 498 ImageSparseDrefGather
  {IR(34198, 17), 286}, // 499 ImageSparseFetch
  {IR(34215, 18), 287}, // 500 ImageSparseGather
  {IR(34325, 16), 293}, // 501 ImageSparseRead
  {IR(34025, 33), 281}, // 502 ImageSparseSampleDrefExplicitLod
  {IR(33992, 33), 280}, // 503 ImageSparseSampleDrefImplicitLod
  {IR(33963, 29), 279}, // 504 ImageSparseSampleExplicitLod
  {IR(33934, 29), 278}, // 505 ImageSparseSampleImplicitLod
  {IR(34161, 37), 285}, // 506 ImageSparseSampleProjDrefExplicitLod
  {IR(34124, 37), 284}, // 507 ImageSparseSampleProjDrefImplicitLod
  {IR(34091, 33), 283}, // 508 ImageSparseSampleProjExplicitLod
  {IR(34058, 33), 282}, // 509 ImageSparseSampleProjImplicitLod
  {IR(34255, 26), 289}, // 510 ImageSparseTexelsResident
  {IR(30509, 18), 53}, // 511 ImageTexelPointer
  {IR(31318, 11), 90}, // 512 ImageWrite
  {IR(30617, 20), 59}, // 513 InBoundsAccessChain
  {IR(30687, 23), 63}, // 514 InBoundsPtrAccessChain
  {IR(31918, 9), 146}, // 515 IsFinite
  {IR(41041, 22), 565}, // 516 IsHelperInvocationEXT
  {IR(31912, 6), 145}, // 517 IsInf
  {IR(31906, 6), 144}, // 518 IsNan
  {IR(37427, 23), 433}, // 519 IsNodePayloadValidAMDX
  {IR(31927, 9), 147}, // 520 IsNormal
  {IR(33847, 13), 273}, // 521 IsValidEvent
  {IR(33502, 17), 257}, // 522 IsValidReserveId
  {IR(33130, 5), 230}, // 523 Kill
  {IR(33047, 6), 226}, // 524 Label
  {IR(31947, 14), 149}, // 525 LessOrGreater
  {IR(33166, 14), 234}, // 526 LifetimeStart
  {IR(33180, 13), 235}, // 527 LifetimeStop
  {IR(29360, 5), 8}, // 528 Line
  {IR(30567, 5), 54}, // 529 Load
  {IR(32018, 11), 155}, // 530 LogicalAnd
  {IR(31979, 13), 152}, // 531 LogicalEqual
  {IR(32029, 11), 156}, // 532 LogicalNot
  {IR(31992, 16), 153}, // 533 LogicalNotEqual
  {IR(32008, 10), 154}, // 534 LogicalOr
  {IR(49083, 17), 787}, // 535 LoopControlINTEL
  {IR(32987, 10), 224}, // 536 LoopMerge
  {IR(51596, 18), 867}, // 537 MaskedGatherINTEL
  {IR(51614, 19), 868}, // 538 MaskedScatterINTEL
  {IR(31816, 18), 135}, // 539 MatrixTimesMatrix
  {IR(31762, 18), 132}, // 540 MatrixTimesScalar
  {IR(31798, 18), 134}, // 541 MatrixTimesVector
  {IR(30747, 15), 65}, // 542 MemberDecorate
  {IR(37603, 20), 442}, // 543 MemberDecorateIdEXT
  {IR(42380, 21), 624}, // 544 MemberDecorateString
  {IR(42401, 27), 624}, // 545 MemberDecorateStringGOOGLE
  {IR(29342, 11), 6}, // 546 MemberName
  {IR(32724, 14), 206}, // 547 MemoryBarrier
  {IR(29507, 12), 12}, // 548 MemoryModel
  {IR(34510, 19), 302}, // 549 MemoryNamedBarrier
  {IR(34529, 16), 303}, // 550 ModuleProcessed
  {IR(29337, 5), 5}, // 551 Name
  {IR(34487, 23), 301}, // 552 NamedBarrierInitialize
  {IR(34281, 7), 290}, // 553 NoLine
  {IR(37400, 27), 432}, // 554 NodePayloadArrayLengthAMDX
  {IR(29133, 4), 0}, // 555 Nop
  {IR(32458, 4), 186}, // 556 Not
  {IR(31961, 8), 150}, // 557 Ordered
  {IR(31834, 13), 136}, // 558 OuterProduct
  {IR(32953, 4), 223}, // 559 Phi
  {IR(37661, 10), 446}, // 560 PoisonKHR
  {IR(51399, 20), 856}, // 561 PredicatedLoadINTEL
  {IR(51419, 21), 857}, // 562 PredicatedStoreINTEL
  {IR(30637, 15), 60}, // 563 PtrAccessChain
  {IR(49518, 30), 802}, // 564 PtrCastToCrossWorkgroupALTERA
  {IR(49548, 29), 802}, // 565 PtrCastToCrossWorkgroupINTEL
  {IR(31591, 17), 111}, // 566 PtrCastToGeneric
  {IR(35377, 8), 343}, // 567 PtrDiff
  {IR(35356, 9), 341}, // 568 PtrEqual
  {IR(35365, 12), 342}, // 569 PtrNotEqual
  {IR(31519, 14), 106}, // 570 QuantizeToF16
  {IR(41346, 17), 576}, // 571 RawAccessChainNV
  {IR(36665, 31), 401}, // 572 RayQueryConfirmIntersectionKHR
  {IR(36633, 32), 400}, // 573 RayQueryGenerateIntersectionKHR
  {IR(40191, 23), 535}, // 574 RayQueryGetClusterIdNV
  {IR(50030, 39), 815}, // 575 RayQueryGetIntersectionBarycentricsKHR
  {IR(50105, 46), 817}, // 576 RayQueryGetIntersectionCandidateAABBOpaqueKHR
  {IR(40156, 35), 535}, // 577 RayQueryGetIntersectionClusterIdNV
  {IR(50069, 36), 816}, // 578 RayQueryGetIntersectionFrontFaceKHR
  {IR(49949, 40), 813}, // 579 RayQueryGetIntersectionGeometryIndexKHR
  {IR(49801, 46), 810}, // 580 RayQueryGetIntersectionInstanceCustomIndexKHR
  {IR(49847, 37), 811}, // 581 RayQueryGetIntersectionInstanceIdKHR
  {IR(49884, 65), 812}, // 582 RayQueryGetIntersectionInstanceShaderBindingTableRecordOffsetKHR
  {IR(41513, 37), 581}, // 583 RayQueryGetIntersectionLSSHitValueNV
  {IR(41441, 38), 579}, // 584 RayQueryGetIntersectionLSSPositionsNV
  {IR(41479, 34), 580}, // 585 RayQueryGetIntersectionLSSRadiiNV
  {IR(50151, 45), 818}, // 586 RayQueryGetIntersectionObjectRayDirectionKHR
  {IR(50196, 42), 819}, // 587 RayQueryGetIntersectionObjectRayOriginKHR
  {IR(50299, 40), 822}, // 588 RayQueryGetIntersectionObjectToWorldKHR
  {IR(49989, 41), 814}, // 589 RayQueryGetIntersectionPrimitiveIndexKHR
  {IR(41363, 40), 577}, // 590 RayQueryGetIntersectionSpherePositionNV
  {IR(41403, 38), 578}, // 591 RayQueryGetIntersectionSphereRadiusNV
  {IR(49773, 28), 809}, // 592 RayQueryGetIntersectionTKHR
  {IR(40031, 50), 532}, // 593 RayQueryGetIntersectionTriangleVertexPositionsKHR
  {IR(36715, 31), 403}, // 594 RayQueryGetIntersectionTypeKHR
  {IR(50339, 40), 823}, // 595 RayQueryGetIntersectionWorldToObjectKHR
  {IR(49750, 23), 808}, // 596 RayQueryGetRayFlagsKHR
  {IR(49728, 22), 807}, // 597 RayQueryGetRayTMinKHR
  {IR(50238, 32), 820}, // 598 RayQueryGetWorldRayDirectionKHR
  {IR(50270, 29), 821}, // 599 RayQueryGetWorldRayOriginKHR
  {IR(36590, 22), 398}, // 600 RayQueryInitializeKHR
  {IR(41721, 19), 589}, // 601 RayQueryIsLSSHitNV
  {IR(41699, 22), 588}, // 602 RayQueryIsSphereHitNV
  {IR(36696, 19), 402}, // 603 RayQueryProceedKHR
  {IR(36612, 21), 399}, // 604 RayQueryTerminateKHR
  {IR(37284, 13), 427}, // 605 ReadClockKHR
  {IR(33370, 9), 249}, // 606 ReadPipe
  {IR(49636, 23), 804}, // 607 ReadPipeBlockingALTERA
  {IR(49659, 22), 804}, // 608 ReadPipeBlockingINTEL
  {IR(33818, 13), 271}, // 609 ReleaseEvent
  {IR(39355, 25), 506}, // 610 ReorderThreadWithHintEXT
  {IR(38518, 24), 479}, // 611 ReorderThreadWithHintNV
  {IR(39380, 30), 507}, // 612 ReorderThreadWithHitObjectEXT
  {IR(38489, 29), 478}, // 613 ReorderThreadWithHitObjectNV
  {IR(39913, 22), 526}, // 614 ReportIntersectionKHR
  {IR(39935, 21), 526}, // 615 ReportIntersectionNV
  {IR(33424, 23), 253}, // 616 ReserveReadPipePackets
  {IR(33447, 24), 254}, // 617 ReserveWritePipePackets
  {IR(33389, 17), 251}, // 618 ReservedReadPipe
  {IR(33406, 18), 252}, // 619 ReservedWritePipe
  {IR(47610, 19), 745}, // 620 RestoreMemoryINTEL
  {IR(33806, 12), 270}, // 621 RetainEvent
  {IR(33135, 7), 231}, // 622 Return
  {IR(33142, 12), 232}, // 623 ReturnValue
  {IR(51578, 18), 866}, // 624 RoundFToTF32INTEL
  {IR(31501, 9), 104}, // 625 SConvert
  {IR(31709, 5), 124}, // 626 SDiv
  {IR(36177, 5), 383}, // 627 SDot
  {IR(36218, 11), 386}, // 628 SDotAccSat
  {IR(36229, 14), 386}, // 629 SDotAccSatKHR
  {IR(36182, 8), 383}, // 630 SDotKHR
  {IR(32077, 13), 161}, // 631 SGreaterThan
  {IR(32108, 18), 163}, // 632 SGreaterThanEqual
  {IR(32136, 10), 165}, // 633 SLessThan
  {IR(32161, 15), 167}, // 634 SLessThanEqual
  {IR(31729, 5), 128}, // 635 SMod
  {IR(31885, 13), 141}, // 636 SMulExtended
  {IR(31658, 8), 115}, // 637 SNegate
  {IR(31724, 5), 127}, // 638 SRem
  {IR(36203, 6), 385}, // 639 SUDot
  {IR(36268, 12), 388}, // 640 SUDotAccSat
  {IR(36280, 15), 388}, // 641 SUDotAccSatKHR
  {IR(36209, 9), 385}, // 642 SUDotKHR
  {IR(30985, 13), 77}, // 643 SampledImage
  {IR(41265, 29), 575}, // 644 SamplerImageAddressingModeNV
  {IR(31547, 15), 108}, // 645 SatConvertSToU
  {IR(31562, 15), 109}, // 646 SatConvertUToS
  {IR(47594, 16), 744}, // 647 SaveMemoryINTEL
  {IR(32040, 7), 157}, // 648 Select
  {IR(33032, 15), 225}, // 649 SelectionMerge
  {IR(38807, 18), 489}, // 650 SetMeshOutputsEXT
  {IR(33860, 19), 274}, // 651 SetUserEventStatus
  {IR(32409, 17), 182}, // 652 ShiftLeftLogical
  {IR(32388, 21), 181}, // 653 ShiftRightArithmetic
  {IR(32370, 18), 180}, // 654 ShiftRightLogical
  {IR(31936, 11), 148}, // 655 SignBitSet
  {IR(34341, 7), 294}, // 656 SizeOf
  {IR(29314, 7), 3}, // 657 Source
  {IR(29195, 16), 2}, // 658 SourceContinued
  {IR(29321, 16), 4}, // 659 SourceExtension
  {IR(30332, 13), 45}, // 660 SpecConstant
  {IR(51275, 30), 853}, // 661 SpecConstantArchitectureINTEL
  {IR(51342, 30), 854}, // 662 SpecConstantCapabilitiesINTEL
  {IR(30345, 22), 46}, // 663 SpecConstantComposite
  {IR(50473, 36), 828}, // 664 SpecConstantCompositeContinuedINTEL
  {IR(36509, 34), 395}, // 665 SpecConstantCompositeReplicateEXT
  {IR(37641, 20), 445}, // 666 SpecConstantDataKHR
  {IR(30314, 18), 44}, // 667 SpecConstantFalse
  {IR(30408, 15), 47}, // 668 SpecConstantOp
  {IR(37469, 23), 435}, // 669 SpecConstantStringAMDX
  {IR(51251, 24), 852}, // 670 SpecConstantTargetINTEL
  {IR(30297, 17), 43}, // 671 SpecConstantTrue
  {IR(35431, 25), 346}, // 672 StencilAttachmentReadEXT
  {IR(30572, 6), 55}, // 673 Store
  {IR(29353, 7), 7}, // 674 String
  {IR(50940, 25), 841}, // 675 Subgroup2DBlockLoadINTEL
  {IR(50965, 34), 842}, // 676 Subgroup2DBlockLoadTransformINTEL
  {IR(50999, 34), 843}, // 677 Subgroup2DBlockLoadTransposeINTEL
  {IR(51033, 29), 844}, // 678 Subgroup2DBlockPrefetchINTEL
  {IR(51062, 26), 845}, // 679 Subgroup2DBlockStoreINTEL
  {IR(35929, 20), 373}, // 680 SubgroupAllEqualKHR
  {IR(35899, 15), 371}, // 681 SubgroupAllKHR
  {IR(35914, 15), 372}, // 682 SubgroupAnyKHR
  {IR(46112, 30), 708}, // 683 SubgroupAvcBmeInitializeINTEL
  {IR(46082, 30), 707}, // 684 SubgroupAvcFmeInitializeINTEL
  {IR(44517, 35), 677}, // 685 SubgroupAvcImeAdjustRefOffsetINTEL
  {IR(44552, 39), 678}, // 686 SubgroupAvcImeConvertToMcePayloadINTEL
  {IR(45193, 38), 691}, // 687 SubgroupAvcImeConvertToMceResultINTEL
  {IR(44816, 45), 684}, // 688 SubgroupAvcImeEvaluateWithDualReferenceINTEL
  {IR(44916, 53), 686}, // 689 SubgroupAvcImeEvaluateWithDualReferenceStreaminINTEL
  {IR(45137, 56), 690}, // 690 SubgroupAvcImeEvaluateWithDualReferenceStreaminoutINTEL
  {IR(45025, 54), 688}, // 691 SubgroupAvcImeEvaluateWithDualReferenceStreamoutINTEL
  {IR(44769, 47), 683}, // 692 SubgroupAvcImeEvaluateWithSingleReferenceINTEL
  {IR(44861, 55), 685}, // 693 SubgroupAvcImeEvaluateWithSingleReferenceStreaminINTEL
  {IR(45079, 58), 689}, // 694 SubgroupAvcImeEvaluateWithSingleReferenceStreaminoutINTEL
  {IR(44969, 56), 687}, // 695 SubgroupAvcImeEvaluateWithSingleReferenceStreamoutINTEL
  {IR(45825, 36), 702}, // 696 SubgroupAvcImeGetBorderReachedINTEL
  {IR(45277, 44), 693}, // 697 SubgroupAvcImeGetDualReferenceStreaminINTEL
  {IR(45231, 46), 692}, // 698 SubgroupAvcImeGetSingleReferenceStreaminINTEL
  {IR(45692, 66), 700}, // 699 SubgroupAvcImeGetStreamoutDualReferenceMajorShapeDistortionsINTEL
  {IR(45624, 68), 699}, // 700 SubgroupAvcImeGetStreamoutDualReferenceMajorShapeMotionVectorsINTEL
  {IR(45758, 67), 701}, // 701 SubgroupAvcImeGetStreamoutDualReferenceMajorShapeReferenceIdsINTEL
  {IR(45487, 68), 697}, // 702 SubgroupAvcImeGetStreamoutSingleReferenceMajorShapeDistortionsINTEL
  {IR(45417, 70), 696}, // 703 SubgroupAvcImeGetStreamoutSingleReferenceMajorShapeMotionVectorsINTEL
  {IR(45555, 69), 698}, // 704 SubgroupAvcImeGetStreamoutSingleReferenceMajorShapeReferenceIdsINTEL
  {IR(45861, 48), 703}, // 705 SubgroupAvcImeGetTruncatedSearchIndicationINTEL
  {IR(45909, 59), 704}, // 706 SubgroupAvcImeGetUnidirectionalEarlySearchTerminationINTEL
  {IR(46026, 56), 706}, // 707 SubgroupAvcImeGetWeightingPatternMinimumDistortionINTEL
  {IR(45968, 58), 705}, // 708 SubgroupAvcImeGetWeightingPatternMinimumMotionVectorINTEL
  {IR(44380, 30), 673}, // 709 SubgroupAvcImeInitializeINTEL
  {IR(44484, 33), 676}, // 710 SubgroupAvcImeRefWindowSizeINTEL
  {IR(44448, 36), 675}, // 711 SubgroupAvcImeSetDualReferenceINTEL
  {IR(44681, 54), 681}, // 712 SubgroupAvcImeSetEarlySearchTerminationThresholdINTEL
  {IR(44591, 43), 679}, // 713 SubgroupAvcImeSetMaxMotionVectorCountINTEL
  {IR(44410, 38), 674}, // 714 SubgroupAvcImeSetSingleReferenceINTEL
  {IR(44634, 47), 680}, // 715 SubgroupAvcImeSetUnidirectionalMixDisableINTEL
  {IR(44735, 34), 682}, // 716 SubgroupAvcImeSetWeightedSadINTEL
  {IR(45370, 47), 695}, // 717 SubgroupAvcImeStripDualReferenceStreamoutINTEL
  {IR(45321, 49), 694}, // 718 SubgroupAvcImeStripSingleReferenceStreamoutINTEL
  {IR(43770, 39), 658}, // 719 SubgroupAvcMceConvertToImePayloadINTEL
  {IR(43809, 38), 659}, // 720 SubgroupAvcMceConvertToImeResultINTEL
  {IR(43847, 39), 660}, // 721 SubgroupAvcMceConvertToRefPayloadINTEL
  {IR(43886, 38), 661}, // 722 SubgroupAvcMceConvertToRefResultINTEL
  {IR(43924, 39), 662}, // 723 SubgroupAvcMceConvertToSicPayloadINTEL
  {IR(43963, 38), 663}, // 724 SubgroupAvcMceConvertToSicResultINTEL
  {IR(44076, 43), 666}, // 725 SubgroupAvcMceGetBestInterDistortionsINTEL
  {IR(43208, 50), 647}, // 726 SubgroupAvcMceGetDefaultHighPenaltyCostTableINTEL
  {IR(42806, 60), 639}, // 727 SubgroupAvcMceGetDefaultInterBaseMultiReferencePenaltyINTEL
  {IR(43006, 51), 643}, // 728 SubgroupAvcMceGetDefaultInterDirectionPenaltyINTEL
  {IR(43152, 56), 646}, // 729 SubgroupAvcMceGetDefaultInterMotionVectorCostTableINTEL
  {IR(42919, 47), 641}, // 730 SubgroupAvcMceGetDefaultInterShapePenaltyINTEL
  {IR(43507, 56), 653}, // 731 SubgroupAvcMceGetDefaultIntraChromaModeBasePenaltyINTEL
  {IR(43406, 50), 651}, // 732 SubgroupAvcMceGetDefaultIntraLumaModePenaltyINTEL
  {IR(43101, 51), 645}, // 733 SubgroupAvcMceGetDefaultIntraLumaShapePenaltyINTEL
  {IR(43310, 49), 649}, // 734 SubgroupAvcMceGetDefaultLowPenaltyCostTableINTEL
  {IR(43258, 52), 648}, // 735 SubgroupAvcMceGetDefaultMediumPenaltyCostTableINTEL
  {IR(43456, 51), 652}, // 736 SubgroupAvcMceGetDefaultNonDcLumaIntraPenaltyINTEL
  {IR(44195, 38), 669}, // 737 SubgroupAvcMceGetInterDirectionsINTEL
  {IR(44037, 39), 665}, // 738 SubgroupAvcMceGetInterDistortionsINTEL
  {IR(44119, 38), 667}, // 739 SubgroupAvcMceGetInterMajorShapeINTEL
  {IR(44157, 38), 668}, // 740 SubgroupAvcMceGetInterMinorShapeINTEL
  {IR(44233, 45), 670}, // 741 SubgroupAvcMceGetInterMotionVectorCountINTEL
  {IR(44278, 40), 671}, // 742 SubgroupAvcMceGetInterReferenceIdsINTEL
  {IR(44318, 62), 672}, // 743 SubgroupAvcMceGetInterReferenceInterlacedFieldPolaritiesINTEL
  {IR(44001, 36), 664}, // 744 SubgroupAvcMceGetMotionVectorsINTEL
  {IR(43563, 33), 654}, // 745 SubgroupAvcMceSetAcOnlyHaarINTEL
  {IR(43709, 61), 657}, // 746 SubgroupAvcMceSetDualReferenceInterlacedFieldPolaritiesINTEL
  {IR(42866, 53), 640}, // 747 SubgroupAvcMceSetInterBaseMultiReferencePenaltyINTEL
  {IR(43057, 44), 644}, // 748 SubgroupAvcMceSetInterDirectionPenaltyINTEL
  {IR(42966, 40), 642}, // 749 SubgroupAvcMceSetInterShapePenaltyINTEL
  {IR(43359, 47), 650}, // 750 SubgroupAvcMceSetMotionVectorCostFunctionINTEL
  {IR(43648, 61), 656}, // 751 SubgroupAvcMceSetSingleReferenceInterlacedFieldPolarityINTEL
  {IR(43596, 52), 655}, // 752 SubgroupAvcMceSetSourceInterlacedFieldPolarityINTEL
  {IR(46142, 39), 709}, // 753 SubgroupAvcRefConvertToMcePayloadINTEL
  {IR(46464, 38), 716}, // 754 SubgroupAvcRefConvertToMceResultINTEL
  {IR(46317, 45), 713}, // 755 SubgroupAvcRefEvaluateWithDualReferenceINTEL
  {IR(46362, 46), 714}, // 756 SubgroupAvcRefEvaluateWithMultiReferenceINTEL
  {IR(46408, 56), 715}, // 757 SubgroupAvcRefEvaluateWithMultiReferenceInterlacedINTEL
  {IR(46270, 47), 712}, // 758 SubgroupAvcRefEvaluateWithSingleReferenceINTEL
  {IR(46181, 46), 710}, // 759 SubgroupAvcRefSetBidirectionalMixDisableINTEL
  {IR(46227, 43), 711}, // 760 SubgroupAvcRefSetBilinearFilterEnableINTEL
  {IR(46600, 42), 720}, // 761 SubgroupAvcSicConfigureIpeLumaChromaINTEL
  {IR(46564, 36), 719}, // 762 SubgroupAvcSicConfigureIpeLumaINTEL
  {IR(46532, 32), 718}, // 763 SubgroupAvcSicConfigureSkcINTEL
  {IR(46681, 39), 722}, // 764 SubgroupAvcSicConvertToMcePayloadINTEL
  {IR(47221, 38), 734}, // 765 SubgroupAvcSicConvertToMceResultINTEL
  {IR(46996, 31), 729}, // 766 SubgroupAvcSicEvaluateIpeINTEL
  {IR(47074, 45), 731}, // 767 SubgroupAvcSicEvaluateWithDualReferenceINTEL
  {IR(47119, 46), 732}, // 768 SubgroupAvcSicEvaluateWithMultiReferenceINTEL
  {IR(47165, 56), 733}, // 769 SubgroupAvcSicEvaluateWithMultiReferenceInterlacedINTEL
  {IR(47027, 47), 730}, // 770 SubgroupAvcSicEvaluateWithSingleReferenceINTEL
  {IR(47338, 46), 737}, // 771 SubgroupAvcSicGetBestIpeChromaDistortionINTEL
  {IR(47294, 44), 736}, // 772 SubgroupAvcSicGetBestIpeLumaDistortionINTEL
  {IR(47559, 35), 742}, // 773 SubgroupAvcSicGetInterRawSadsINTEL
  {IR(47425, 36), 739}, // 774 SubgroupAvcSicGetIpeChromaModeINTEL
  {IR(47259, 35), 735}, // 775 SubgroupAvcSicGetIpeLumaShapeINTEL
  {IR(46642, 39), 721}, // 776 SubgroupAvcSicGetMotionVectorMaskINTEL
  {IR(47384, 41), 738}, // 777 SubgroupAvcSicGetPackedIpeLumaModesINTEL
  {IR(47461, 50), 740}, // 778 SubgroupAvcSicGetPackedSkcLumaCountThresholdINTEL
  {IR(47511, 48), 741}, // 779 SubgroupAvcSicGetPackedSkcLumaSumThresholdINTEL
  {IR(46502, 30), 717}, // 780 SubgroupAvcSicInitializeINTEL
  {IR(46862, 43), 726}, // 781 SubgroupAvcSicSetBilinearFilterEnableINTEL
  {IR(46953, 43), 728}, // 782 SubgroupAvcSicSetBlockBasedRawSkipSadINTEL
  {IR(46812, 50), 725}, // 783 SubgroupAvcSicSetIntraChromaModeCostFunctionINTEL
  {IR(46764, 48), 724}, // 784 SubgroupAvcSicSetIntraLumaModeCostFunctionINTEL
  {IR(46720, 44), 723}, // 785 SubgroupAvcSicSetIntraLumaShapePenaltyINTEL
  {IR(46905, 48), 727}, // 786 SubgroupAvcSicSetSkcForwardTransformEnableINTEL
  {IR(6586, 18), 364}, // 787 SubgroupBallotKHR
  {IR(50913, 27), 840}, // 788 SubgroupBlockPrefetchINTEL
  {IR(41812, 23), 594}, // 789 SubgroupBlockReadINTEL
  {IR(41835, 24), 595}, // 790 SubgroupBlockWriteINTEL
  {IR(35766, 27), 365}, // 791 SubgroupFirstInvocationKHR
  {IR(41859, 28), 596}, // 792 SubgroupImageBlockReadINTEL
  {IR(41887, 29), 597}, // 793 SubgroupImageBlockWriteINTEL
  {IR(41916, 33), 598}, // 794 SubgroupImageMediaBlockReadINTEL
  {IR(41949, 34), 599}, // 795 SubgroupImageMediaBlockWriteINTEL
  {IR(16011, 38), 846}, // 796 SubgroupMatrixMultiplyAccumulateINTEL
  {IR(35949, 26), 375}, // 797 SubgroupReadInvocationKHR
  {IR(41740, 25), 591}, // 798 SubgroupShuffleDownINTEL
  {IR(13609, 21), 590}, // 799 SubgroupShuffleINTEL
  {IR(41765, 23), 592}, // 800 SubgroupShuffleUpINTEL
  {IR(41788, 24), 593}, // 801 SubgroupShuffleXorINTEL
  {IR(33123, 7), 229}, // 802 Switch
  {IR(50727, 24), 836}, // 803 TaskSequenceAsyncALTERA
  {IR(50751, 23), 836}, // 804 TaskSequenceAsyncINTEL
  {IR(50678, 25), 835}, // 805 TaskSequenceCreateALTERA
  {IR(50703, 24), 835}, // 806 TaskSequenceCreateINTEL
  {IR(50774, 22), 837}, // 807 TaskSequenceGetALTERA
  {IR(50796, 21), 837}, // 808 TaskSequenceGetINTEL
  {IR(50817, 26), 838}, // 809 TaskSequenceReleaseALTERA
  {IR(50843, 25), 838}, // 810 TaskSequenceReleaseINTEL
  {IR(41083, 27), 567}, // 811 TensorLayoutSetBlockSizeNV
  {IR(40947, 28), 560}, // 812 TensorLayoutSetClampValueNV
  {IR(40876, 27), 557}, // 813 TensorLayoutSetDimensionNV
  {IR(40903, 24), 558}, // 814 TensorLayoutSetStrideNV
  {IR(40927, 20), 559}, // 815 TensorLayoutSliceNV
  {IR(35541, 19), 350}, // 816 TensorQuerySizeARM
  {IR(35512, 14), 348}, // 817 TensorReadARM
  {IR(41063, 20), 566}, // 818 TensorViewSetClipNV
  {IR(40994, 25), 562}, // 819 TensorViewSetDimensionNV
  {IR(41019, 22), 563}, // 820 TensorViewSetStrideNV
  {IR(35526, 15), 349}, // 821 TensorWriteARM
  {IR(35653, 20), 359}, // 822 TerminateInvocation
  {IR(36114, 16), 382}, // 823 TerminateRayKHR
  {IR(39977, 15), 528}, // 824 TerminateRayNV
  {IR(40000, 14), 530}, // 825 TraceMotionNV
  {IR(39992, 8), 529}, // 826 TraceNV
  {IR(36026, 12), 378}, // 827 TraceRayKHR
  {IR(40014, 17), 531}, // 828 TraceRayMotionNV
  {IR(30975, 10), 76}, // 829 Transpose
  {IR(40081, 29), 533}, // 830 TypeAccelerationStructureKHR
  {IR(40110, 28), 533}, // 831 TypeAccelerationStructureNV
  {IR(29924, 10), 25}, // 832 TypeArray
  {IR(42725, 37), 636}, // 833 TypeAvcImeDualReferenceStreaminINTEL
  {IR(42460, 23), 627}, // 834 TypeAvcImePayloadINTEL
  {IR(42642, 44), 634}, // 835 TypeAvcImeResultDualReferenceStreamoutINTEL
  {IR(42574, 22), 632}, // 836 TypeAvcImeResultINTEL
  {IR(42596, 46), 633}, // 837 TypeAvcImeResultSingleReferenceStreamoutINTEL
  {IR(42686, 39), 635}, // 838 TypeAvcImeSingleReferenceStreaminINTEL
  {IR(42529, 23), 630}, // 839 TypeAvcMcePayloadINTEL
  {IR(42552, 22), 631}, // 840 TypeAvcMceResultINTEL
  {IR(42483, 23), 628}, // 841 TypeAvcRefPayloadINTEL
  {IR(42762, 22), 637}, // 842 TypeAvcRefResultINTEL
  {IR(42506, 23), 629}, // 843 TypeAvcSicPayloadINTEL
  {IR(42784, 22), 638}, // 844 TypeAvcSicResultINTEL
  {IR(29686, 9), 17}, // 845 TypeBool
  {IR(37544, 14), 438}, // 846 TypeBufferEXT
  {IR(50393, 23), 825}, // 847 TypeBufferSurfaceINTEL
  {IR(36295, 25), 389}, // 848 TypeCooperativeMatrixKHR
  {IR(40403, 24), 543}, // 849 TypeCooperativeMatrixNV
  {IR(38597, 24), 482}, // 850 TypeCooperativeVectorNV
  {IR(30039, 16), 32}, // 851 TypeDeviceEvent
  {IR(30029, 10), 31}, // 852 TypeEvent
  {IR(29740, 10), 19}, // 853 TypeFloat
  {IR(30088, 19), 36}, // 854 TypeForwardPointer
  {IR(30016, 13), 30}, // 855 TypeFunction
  {IR(35640, 13), 357}, // 856 TypeGraphARM
  {IR(39338, 17), 505}, // 857 TypeHitObjectEXT
  {IR(38542, 16), 480}, // 858 TypeHitObjectNV
  {IR(29885, 10), 22}, // 859 TypeImage
  {IR(29695, 8), 18}, // 860 TypeInt
  {IR(29761, 11), 21}, // 861 TypeMatrix
  {IR(34470, 17), 300}, // 862 TypeNamedBarrier
  {IR(37346, 25), 430}, // 863 TypeNodePayloadArrayAMDX
  {IR(29962, 11), 28}, // 864 TypeOpaque
  {IR(30079, 9), 35}, // 865 TypePipe
  {IR(34348, 16), 295}, // 866 TypePipeStorage
  {IR(30004, 12), 29}, // 867 TypePointer
  {IR(30069, 10), 34}, // 868 TypeQueue
  {IR(36574, 16), 397}, // 869 TypeRayQueryKHR
  {IR(30055, 14), 33}, // 870 TypeReserveId
  {IR(29934, 17), 26}, // 871 TypeRuntimeArray
  {IR(29907, 17), 24}, // 872 TypeSampledImage
  {IR(29895, 12), 23}, // 873 TypeSampler
  {IR(29951, 11), 27}, // 874 TypeStruct
  {IR(50416, 25), 826}, // 875 TypeStructContinuedINTEL
  {IR(50868, 23), 839}, // 876 TypeTaskSequenceALTERA
  {IR(50891, 22), 839}, // 877 TypeTaskSequenceINTEL
  {IR(35456, 14), 347}, // 878 TypeTensorARM
  {IR(40819, 19), 554}, // 879 TypeTensorLayoutNV
  {IR(40838, 17), 555}, // 880 TypeTensorViewNV
  {IR(35673, 22), 360}, // 881 TypeUntypedPointerKHR
  {IR(29750, 11), 20}, // 882 TypeVector
  {IR(38581, 16), 482}, // 883 TypeVectorIdEXT
  {IR(42442, 18), 626}, // 884 TypeVmeImageINTEL
  {IR(29677, 9), 16}, // 885 TypeVoid
  {IR(42071, 13), 605}, // 886 UAddSatINTEL
  {IR(42098, 14), 607}, // 887 UAverageINTEL
  {IR(42133, 21), 609}, // 888 UAverageRoundedINTEL
  {IR(31492, 9), 103}, // 889 UConvert
  {IR(41983, 24), 600}, // 890 UCountLeadingZerosINTEL
  {IR(42007, 25), 601}, // 891 UCountTrailingZerosINTEL
  {IR(31704, 5), 123}, // 892 UDiv
  {IR(36190, 5), 384}, // 893 UDot
  {IR(36243, 11), 387}, // 894 UDotAccSat
  {IR(36254, 14), 387}, // 895 UDotAccSatKHR
  {IR(36195, 8), 384}, // 896 UDotKHR
  {IR(32064, 13), 160}, // 897 UGreaterThan
  {IR(32090, 18), 162}, // 898 UGreaterThanEqual
  {IR(32126, 10), 164}, // 899 ULessThan
  {IR(32146, 15), 166}, // 900 ULessThanEqual
  {IR(31719, 5), 126}, // 901 UMod
  {IR(42195, 15), 613}, // 902 UMul32x16INTEL
  {IR(31872, 13), 140}, // 903 UMulExtended
  {IR(42167, 13), 611}, // 904 USubSatINTEL
  {IR(29189, 6), 1}, // 905 Undef
  {IR(31969, 10), 151}, // 906 Unordered
  {IR(33154, 12), 233}, // 907 Unreachable
  {IR(35714, 22), 362}, // 908 UntypedAccessChainKHR
  {IR(35851, 22), 368}, // 909 UntypedArrayLengthKHR
  {IR(36001, 25), 377}, // 910 UntypedGroupAsyncCopyKHR
  {IR(37575, 28), 441}, // 911 UntypedImageTexelPointerEXT
  {IR(35736, 30), 363}, // 912 UntypedInBoundsAccessChainKHR
  {IR(35818, 33), 367}, // 913 UntypedInBoundsPtrAccessChainKHR
  {IR(35873, 19), 369}, // 914 UntypedPrefetchKHR
  {IR(35793, 25), 366}, // 915 UntypedPtrAccessChainKHR
  {IR(35695, 19), 361}, // 916 UntypedVariableKHR
  {IR(16077, 32), 848}, // 917 UntypedVariableLengthArrayINTEL
  {IR(30500, 9), 52}, // 918 Variable
  {IR(14065, 25), 743}, // 919 VariableLengthArrayINTEL
  {IR(30857, 21), 69}, // 920 VectorExtractDynamic
  {IR(30878, 20), 70}, // 921 VectorInsertDynamic
  {IR(30898, 14), 71}, // 922 VectorShuffle
  {IR(31780, 18), 133}, // 923 VectorTimesMatrix
  {IR(31744, 18), 131}, // 924 VectorTimesScalar
  {IR(42428, 14), 625}, // 925 VmeImageINTEL
  {IR(38880, 33), 491}, // 926 WritePackedPrimitiveIndices4x8NV
  {IR(33379, 10), 250}, // 927 WritePipe
  {IR(49681, 24), 805}, // 928 WritePipeBlockingALTERA
  {IR(49705, 23), 805}, // 929 WritePipeBlockingINTEL
}};

// Extended instruction descriptions, ordered by (extinst enum, opcode value).
// The fields in order are:
//   enum value
//   operands, an IndexRange into kOperandSpans
//   name, a character-counting IndexRange into kStrings
//   capabilities, an IndexRange into kCapabilitySpans
static const std::array<ExtInstDesc, 496> kExtInstByValue{{
    {0,IR(765, 9),IR(51777, 8),IR(1, 0)}, // MIN_SAD in arm_motion_engine_100
    {1,IR(765, 9),IR(51785, 13),IR(1, 0)}, // MIN_SAD_COST in arm_motion_engine_100
    {2,IR(471, 8),IR(51798, 8),IR(1, 0)}, // RAW_SAD in arm_motion_engine_100
    {0,IR(0, 0),IR(51806, 14),IR(1, 0)}, // DebugInfoNone in debuginfo
    {1,IR(52, 3),IR(51820, 21),IR(1, 0)}, // DebugCompilationUnit in debuginfo
    {2,IR(811, 3),IR(51893, 15),IR(1, 0)}, // DebugTypeBasic in debuginfo
    {3,IR(814, 3),IR(51942, 17),IR(1, 0)}, // DebugTypePointer in debuginfo
    {4,IR(817, 2),IR(51997, 19),IR(1, 0)}, // DebugTypeQualifier in debuginfo
    {5,IR(168, 2),IR(52016, 15),IR(1, 0)}, // DebugTypeArray in debuginfo
    {6,IR(261, 2),IR(52031, 16),IR(1, 0)}, // DebugTypeVector in debuginfo
    {7,IR(819, 6),IR(52047, 13),IR(1, 0)}, // DebugTypedef in debuginfo
    {8,IR(168, 2),IR(52060, 18),IR(1, 0)}, // DebugTypeFunction in debuginfo
    {9,IR(825, 9),IR(52078, 14),IR(1, 0)}, // DebugTypeEnum in debuginfo
    {10,IR(834, 9),IR(52130, 19),IR(1, 0)}, // DebugTypeComposite in debuginfo
    {11,IR(843, 10),IR(52149, 16),IR(1, 0)}, // DebugTypeMember in debuginfo
    {12,IR(853, 5),IR(52165, 21),IR(1, 0)}, // DebugTypeInheritance in debuginfo
    {13,IR(36, 2),IR(52186, 21),IR(1, 0)}, // DebugTypePtrToMember in debuginfo
    {14,IR(168, 2),IR(52207, 18),IR(1, 0)}, // DebugTypeTemplate in debuginfo
    {15,IR(858, 6),IR(52225, 27),IR(1, 0)}, // DebugTypeTemplateParameter in debuginfo
    {16,IR(864, 5),IR(52252, 35),IR(1, 0)}, // DebugTypeTemplateTemplateParameter in debuginfo
    {17,IR(869, 5),IR(52287, 31),IR(1, 0)}, // DebugTypeTemplateParameterPack in debuginfo
    {18,IR(843, 10),IR(52318, 20),IR(1, 0)}, // DebugGlobalVariable in debuginfo
    {19,IR(874, 8),IR(52338, 25),IR(1, 0)}, // DebugFunctionDeclaration in debuginfo
    {20,IR(882, 11),IR(52363, 14),IR(1, 0)}, // DebugFunction in debuginfo
    {21,IR(893, 5),IR(52377, 18),IR(1, 0)}, // DebugLexicalBlock in debuginfo
    {22,IR(898, 3),IR(52395, 31),IR(1, 0)}, // DebugLexicalBlockDiscriminator in debuginfo
    {23,IR(901, 2),IR(52426, 11),IR(1, 0)}, // DebugScope in debuginfo
    {24,IR(0, 0),IR(52437, 13),IR(1, 0)}, // DebugNoScope in debuginfo
    {25,IR(903, 3),IR(52450, 15),IR(1, 0)}, // DebugInlinedAt in debuginfo
    {26,IR(906, 7),IR(52507, 19),IR(1, 0)}, // DebugLocalVariable in debuginfo
    {27,IR(36, 2),IR(52526, 21),IR(1, 0)}, // DebugInlinedVariable in debuginfo
    {28,IR(33, 3),IR(52547, 13),IR(1, 0)}, // DebugDeclare in debuginfo
    {29,IR(375, 3),IR(52560, 11),IR(1, 0)}, // DebugValue in debuginfo
    {30,IR(913, 2),IR(52604, 15),IR(1, 0)}, // DebugOperation in debuginfo
    {31,IR(747, 1),IR(52619, 16),IR(1, 0)}, // DebugExpression in debuginfo
    {32,IR(915, 4),IR(52635, 14),IR(1, 0)}, // DebugMacroDef in debuginfo
    {33,IR(898, 3),IR(52649, 16),IR(1, 0)}, // DebugMacroUndef in debuginfo
    {1,IR(10, 1),IR(52665, 6),IR(1, 0)}, // Round in glsl_std_450
    {2,IR(10, 1),IR(52671, 10),IR(1, 0)}, // RoundEven in glsl_std_450
    {3,IR(10, 1),IR(52681, 6),IR(1, 0)}, // Trunc in glsl_std_450
    {4,IR(10, 1),IR(52687, 5),IR(1, 0)}, // FAbs in glsl_std_450
    {5,IR(10, 1),IR(52692, 5),IR(1, 0)}, // SAbs in glsl_std_450
    {6,IR(10, 1),IR(52697, 6),IR(1, 0)}, // FSign in glsl_std_450
    {7,IR(10, 1),IR(52703, 6),IR(1, 0)}, // SSign in glsl_std_450
    {8,IR(10, 1),IR(52709, 6),IR(1, 0)}, // Floor in glsl_std_450
    {9,IR(10, 1),IR(52715, 5),IR(1, 0)}, // Ceil in glsl_std_450
    {10,IR(10, 1),IR(52720, 6),IR(1, 0)}, // Fract in glsl_std_450
    {11,IR(10, 1),IR(52726, 8),IR(1, 0)}, // Radians in glsl_std_450
    {12,IR(10, 1),IR(52734, 8),IR(1, 0)}, // Degrees in glsl_std_450
    {13,IR(10, 1),IR(52742, 4),IR(1, 0)}, // Sin in glsl_std_450
    {14,IR(10, 1),IR(52746, 4),IR(1, 0)}, // Cos in glsl_std_450
    {15,IR(10, 1),IR(52750, 4),IR(1, 0)}, // Tan in glsl_std_450
    {16,IR(10, 1),IR(52754, 5),IR(1, 0)}, // Asin in glsl_std_450
    {17,IR(10, 1),IR(52759, 5),IR(1, 0)}, // Acos in glsl_std_450
    {18,IR(10, 1),IR(52764, 5),IR(1, 0)}, // Atan in glsl_std_450
    {19,IR(10, 1),IR(52769, 5),IR(1, 0)}, // Sinh in glsl_std_450
    {20,IR(10, 1),IR(52774, 5),IR(1, 0)}, // Cosh in glsl_std_450
    {21,IR(10, 1),IR(52779, 5),IR(1, 0)}, // Tanh in glsl_std_450
    {22,IR(10, 1),IR(52784, 6),IR(1, 0)}, // Asinh in glsl_std_450
    {23,IR(10, 1),IR(52790, 6),IR(1, 0)}, // Acosh in glsl_std_450
    {24,IR(10, 1),IR(52796, 6),IR(1, 0)}, // Atanh in glsl_std_450
    {25,IR(36, 2),IR(52802, 6),IR(1, 0)}, // Atan2 in glsl_std_450
    {26,IR(36, 2),IR(52808, 4),IR(1, 0)}, // Pow in glsl_std_450
    {27,IR(10, 1),IR(52812, 4),IR(1, 0)}, // Exp in glsl_std_450
    {28,IR(10, 1),IR(52816, 4),IR(1, 0)}, // Log in glsl_std_450
    {29,IR(10, 1),IR(52820, 5),IR(1, 0)}, // Exp2 in glsl_std_450
    {30,IR(10, 1),IR(52825, 5),IR(1, 0)}, // Log2 in glsl_std_450
    {31,IR(10, 1),IR(52830, 5),IR(1, 0)}, // Sqrt in glsl_std_450
    {32,IR(10, 1),IR(52835, 12),IR(1, 0)}, // InverseSqrt in glsl_std_450
    {33,IR(10, 1),IR(52847, 12),IR(1, 0)}, // Determinant in glsl_std_450
    {34,IR(10, 1),IR(52859, 14),IR(1, 0)}, // MatrixInverse in glsl_std_450
    {35,IR(36, 2),IR(52873, 5),IR(1, 0)}, // Modf in glsl_std_450
    {36,IR(10, 1),IR(52878, 11),IR(1, 0)}, // ModfStruct in glsl_std_450
    {37,IR(36, 2),IR(52889, 5),IR(1, 0)}, // FMin in glsl_std_450
    {38,IR(36, 2),IR(52894, 5),IR(1, 0)}, // UMin in glsl_std_450
    {39,IR(36, 2),IR(52899, 5),IR(1, 0)}, // SMin in glsl_std_450
    {40,IR(36, 2),IR(52904, 5),IR(1, 0)}, // FMax in glsl_std_450
    {41,IR(36, 2),IR(52909, 5),IR(1, 0)}, // UMax in glsl_std_450
    {42,IR(36, 2),IR(52914, 5),IR(1, 0)}, // SMax in glsl_std_450
    {43,IR(33, 3),IR(52919, 7),IR(1, 0)}, // FClamp in glsl_std_450
    {44,IR(33, 3),IR(52926, 7),IR(1, 0)}, // UClamp in glsl_std_450
    {45,IR(33, 3),IR(52933, 7),IR(1, 0)}, // SClamp in glsl_std_450
    {46,IR(33, 3),IR(52940, 5),IR(1, 0)}, // FMix in glsl_std_450
    {47,IR(33, 3),IR(52945, 5),IR(1, 0)}, // IMix in glsl_std_450
    {48,IR(36, 2),IR(52950, 5),IR(1, 0)}, // Step in glsl_std_450
    {49,IR(33, 3),IR(52955, 11),IR(1, 0)}, // SmoothStep in glsl_std_450
    {50,IR(33, 3),IR(52966, 4),IR(1, 0)}, // Fma in glsl_std_450
    {51,IR(36, 2),IR(52970, 6),IR(1, 0)}, // Frexp in glsl_std_450
    {52,IR(10, 1),IR(52976, 12),IR(1, 0)}, // FrexpStruct in glsl_std_450
    {53,IR(36, 2),IR(52988, 6),IR(1, 0)}, // Ldexp in glsl_std_450
    {54,IR(10, 1),IR(52994, 13),IR(1, 0)}, // PackSnorm4x8 in glsl_std_450
    {55,IR(10, 1),IR(53007, 13),IR(1, 0)}, // PackUnorm4x8 in glsl_std_450
    {56,IR(10, 1),IR(53020, 14),IR(1, 0)}, // PackSnorm2x16 in glsl_std_450
    {57,IR(10, 1),IR(53034, 14),IR(1, 0)}, // PackUnorm2x16 in glsl_std_450
    {58,IR(10, 1),IR(53048, 13),IR(1, 0)}, // PackHalf2x16 in glsl_std_450
    {59,IR(10, 1),IR(53061, 15),IR(339, 1)}, // PackDouble2x32 in glsl_std_450
    {60,IR(10, 1),IR(53076, 16),IR(1, 0)}, // UnpackSnorm2x16 in glsl_std_450
    {61,IR(10, 1),IR(53092, 16),IR(1, 0)}, // UnpackUnorm2x16 in glsl_std_450
    {62,IR(10, 1),IR(53108, 15),IR(1, 0)}, // UnpackHalf2x16 in glsl_std_450
    {63,IR(10, 1),IR(53123, 15),IR(1, 0)}, // UnpackSnorm4x8 in glsl_std_450
    {64,IR(10, 1),IR(53138, 15),IR(1, 0)}, // UnpackUnorm4x8 in glsl_std_450
    {65,IR(10, 1),IR(53153, 17),IR(339, 1)}, // UnpackDouble2x32 in glsl_std_450
    {66,IR(10, 1),IR(53170, 7),IR(1, 0)}, // Length in glsl_std_450
    {67,IR(36, 2),IR(53177, 9),IR(1, 0)}, // Distance in glsl_std_450
    {68,IR(36, 2),IR(53186, 6),IR(1, 0)}, // Cross in glsl_std_450
    {69,IR(10, 1),IR(53192, 10),IR(1, 0)}, // Normalize in glsl_std_450
    {70,IR(33, 3),IR(53202, 12),IR(1, 0)}, // FaceForward in glsl_std_450
    {71,IR(36, 2),IR(53214, 8),IR(1, 0)}, // Reflect in glsl_std_450
    {72,IR(33, 3),IR(53222, 8),IR(1, 0)}, // Refract in glsl_std_450
    {73,IR(10, 1),IR(53230, 9),IR(1, 0)}, // FindILsb in glsl_std_450
    {74,IR(10, 1),IR(53239, 9),IR(1, 0)}, // FindSMsb in glsl_std_450
    {75,IR(10, 1),IR(53248, 9),IR(1, 0)}, // FindUMsb in glsl_std_450
    {76,IR(10, 1),IR(53257, 22),IR(340, 1)}, // InterpolateAtCentroid in glsl_std_450
    {77,IR(36, 2),IR(53279, 20),IR(340, 1)}, // InterpolateAtSample in glsl_std_450
    {78,IR(36, 2),IR(53299, 20),IR(340, 1)}, // InterpolateAtOffset in glsl_std_450
    {79,IR(36, 2),IR(53319, 5),IR(1, 0)}, // NMin in glsl_std_450
    {80,IR(36, 2),IR(53324, 5),IR(1, 0)}, // NMax in glsl_std_450
    {81,IR(33, 3),IR(53329, 7),IR(1, 0)}, // NClamp in glsl_std_450
    {1,IR(399, 5),IR(5805, 7),IR(1, 0)}, // Kernel in nonsemantic_clspvreflection
    {2,IR(919, 5),IR(53336, 13),IR(1, 0)}, // ArgumentInfo in nonsemantic_clspvreflection
    {3,IR(593, 5),IR(53349, 22),IR(1, 0)}, // ArgumentStorageBuffer in nonsemantic_clspvreflection
    {4,IR(593, 5),IR(53371, 16),IR(1, 0)}, // ArgumentUniform in nonsemantic_clspvreflection
    {5,IR(557, 7),IR(53387, 25),IR(1, 0)}, // ArgumentPodStorageBuffer in nonsemantic_clspvreflection
    {6,IR(557, 7),IR(53412, 19),IR(1, 0)}, // ArgumentPodUniform in nonsemantic_clspvreflection
    {7,IR(593, 5),IR(53431, 24),IR(1, 0)}, // ArgumentPodPushConstant in nonsemantic_clspvreflection
    {8,IR(593, 5),IR(53455, 21),IR(1, 0)}, // ArgumentSampledImage in nonsemantic_clspvreflection
    {9,IR(593, 5),IR(53476, 21),IR(1, 0)}, // ArgumentStorageImage in nonsemantic_clspvreflection
    {10,IR(593, 5),IR(53497, 16),IR(1, 0)}, // ArgumentSampler in nonsemantic_clspvreflection
    {11,IR(593, 5),IR(53513, 18),IR(1, 0)}, // ArgumentWorkgroup in nonsemantic_clspvreflection
    {12,IR(33, 3),IR(53531, 26),IR(1, 0)}, // SpecConstantWorkgroupSize in nonsemantic_clspvreflection
    {13,IR(33, 3),IR(53557, 25),IR(1, 0)}, // SpecConstantGlobalOffset in nonsemantic_clspvreflection
    {14,IR(10, 1),IR(53582, 20),IR(1, 0)}, // SpecConstantWorkDim in nonsemantic_clspvreflection
    {15,IR(36, 2),IR(53602, 25),IR(1, 0)}, // PushConstantGlobalOffset in nonsemantic_clspvreflection
    {16,IR(36, 2),IR(53627, 30),IR(1, 0)}, // PushConstantEnqueuedLocalSize in nonsemantic_clspvreflection
    {17,IR(36, 2),IR(53657, 23),IR(1, 0)}, // PushConstantGlobalSize in nonsemantic_clspvreflection
    {18,IR(36, 2),IR(53680, 25),IR(1, 0)}, // PushConstantRegionOffset in nonsemantic_clspvreflection
    {19,IR(36, 2),IR(53705, 26),IR(1, 0)}, // PushConstantNumWorkgroups in nonsemantic_clspvreflection
    {20,IR(36, 2),IR(53731, 30),IR(1, 0)}, // PushConstantRegionGroupOffset in nonsemantic_clspvreflection
    {21,IR(33, 3),IR(53761, 26),IR(1, 0)}, // ConstantDataStorageBuffer in nonsemantic_clspvreflection
    {22,IR(33, 3),IR(53787, 20),IR(1, 0)}, // ConstantDataUniform in nonsemantic_clspvreflection
    {23,IR(33, 3),IR(9279, 15),IR(1, 0)}, // LiteralSampler in nonsemantic_clspvreflection
    {24,IR(296, 4),IR(53807, 30),IR(1, 0)}, // PropertyRequiredWorkgroupSize in nonsemantic_clspvreflection
    {25,IR(10, 1),IR(53837, 28),IR(1, 0)}, // SpecConstantSubgroupMaxSize in nonsemantic_clspvreflection
    {26,IR(593, 5),IR(53865, 28),IR(1, 0)}, // ArgumentPointerPushConstant in nonsemantic_clspvreflection
    {27,IR(557, 7),IR(53893, 23),IR(1, 0)}, // ArgumentPointerUniform in nonsemantic_clspvreflection
    {28,IR(33, 3),IR(53916, 35),IR(1, 0)}, // ProgramScopeVariablesStorageBuffer in nonsemantic_clspvreflection
    {29,IR(33, 3),IR(53951, 38),IR(1, 0)}, // ProgramScopeVariablePointerRelocation in nonsemantic_clspvreflection
    {30,IR(296, 4),IR(53989, 42),IR(1, 0)}, // ImageArgumentInfoChannelOrderPushConstant in nonsemantic_clspvreflection
    {31,IR(296, 4),IR(54031, 45),IR(1, 0)}, // ImageArgumentInfoChannelDataTypePushConstant in nonsemantic_clspvreflection
    {32,IR(535, 6),IR(54076, 37),IR(1, 0)}, // ImageArgumentInfoChannelOrderUniform in nonsemantic_clspvreflection
    {33,IR(535, 6),IR(54113, 40),IR(1, 0)}, // ImageArgumentInfoChannelDataTypeUniform in nonsemantic_clspvreflection
    {34,IR(593, 5),IR(54153, 27),IR(1, 0)}, // ArgumentStorageTexelBuffer in nonsemantic_clspvreflection
    {35,IR(593, 5),IR(54180, 27),IR(1, 0)}, // ArgumentUniformTexelBuffer in nonsemantic_clspvreflection
    {36,IR(33, 3),IR(54207, 32),IR(1, 0)}, // ConstantDataPointerPushConstant in nonsemantic_clspvreflection
    {37,IR(33, 3),IR(54239, 40),IR(1, 0)}, // ProgramScopeVariablePointerPushConstant in nonsemantic_clspvreflection
    {38,IR(375, 3),IR(54279, 11),IR(1, 0)}, // PrintfInfo in nonsemantic_clspvreflection
    {39,IR(33, 3),IR(54290, 26),IR(1, 0)}, // PrintfBufferStorageBuffer in nonsemantic_clspvreflection
    {40,IR(33, 3),IR(54316, 32),IR(1, 0)}, // PrintfBufferPointerPushConstant in nonsemantic_clspvreflection
    {41,IR(296, 4),IR(54348, 34),IR(1, 0)}, // NormalizedSamplerMaskPushConstant in nonsemantic_clspvreflection
    {42,IR(36, 2),IR(54382, 22),IR(1, 0)}, // WorkgroupVariableSize in nonsemantic_clspvreflection
    {0,IR(0, 0),IR(51806, 14),IR(1, 0)}, // DebugInfoNone in nonsemantic_shader_debuginfo
    {1,IR(296, 4),IR(51820, 21),IR(1, 0)}, // DebugCompilationUnit in nonsemantic_shader_debuginfo
    {2,IR(593, 5),IR(51893, 15),IR(1, 0)}, // DebugTypeBasic in nonsemantic_shader_debuginfo
    {3,IR(33, 3),IR(51942, 17),IR(1, 0)}, // DebugTypePointer in nonsemantic_shader_debuginfo
    {4,IR(36, 2),IR(51997, 19),IR(1, 0)}, // DebugTypeQualifier in nonsemantic_shader_debuginfo
    {5,IR(168, 2),IR(52016, 15),IR(1, 0)}, // DebugTypeArray in nonsemantic_shader_debuginfo
    {6,IR(36, 2),IR(52031, 16),IR(1, 0)}, // DebugTypeVector in nonsemantic_shader_debuginfo
    {7,IR(535, 6),IR(52047, 13),IR(1, 0)}, // DebugTypedef in nonsemantic_shader_debuginfo
    {8,IR(375, 3),IR(52060, 18),IR(1, 0)}, // DebugTypeFunction in nonsemantic_shader_debuginfo
    {9,IR(924, 9),IR(52078, 14),IR(1, 0)}, // DebugTypeEnum in nonsemantic_shader_debuginfo
    {10,IR(933, 10),IR(52130, 19),IR(1, 0)}, // DebugTypeComposite in nonsemantic_shader_debuginfo
    {11,IR(943, 9),IR(52149, 16),IR(1, 0)}, // DebugTypeMember in nonsemantic_shader_debuginfo
    {12,IR(296, 4),IR(52165, 21),IR(1, 0)}, // DebugTypeInheritance in nonsemantic_shader_debuginfo
    {13,IR(36, 2),IR(52186, 21),IR(1, 0)}, // DebugTypePtrToMember in nonsemantic_shader_debuginfo
    {14,IR(168, 2),IR(52207, 18),IR(1, 0)}, // DebugTypeTemplate in nonsemantic_shader_debuginfo
    {15,IR(535, 6),IR(52225, 27),IR(1, 0)}, // DebugTypeTemplateParameter in nonsemantic_shader_debuginfo
    {16,IR(675, 5),IR(52252, 35),IR(1, 0)}, // DebugTypeTemplateTemplateParameter in nonsemantic_shader_debuginfo
    {17,IR(952, 5),IR(52287, 31),IR(1, 0)}, // DebugTypeTemplateParameterPack in nonsemantic_shader_debuginfo
    {18,IR(957, 10),IR(52318, 20),IR(1, 0)}, // DebugGlobalVariable in nonsemantic_shader_debuginfo
    {19,IR(471, 8),IR(52338, 25),IR(1, 0)}, // DebugFunctionDeclaration in nonsemantic_shader_debuginfo
    {20,IR(957, 10),IR(52363, 14),IR(1, 0)}, // DebugFunction in nonsemantic_shader_debuginfo
    {21,IR(593, 5),IR(52377, 18),IR(1, 0)}, // DebugLexicalBlock in nonsemantic_shader_debuginfo
    {22,IR(33, 3),IR(52395, 31),IR(1, 0)}, // DebugLexicalBlockDiscriminator in nonsemantic_shader_debuginfo
    {23,IR(901, 2),IR(52426, 11),IR(1, 0)}, // DebugScope in nonsemantic_shader_debuginfo
    {24,IR(0, 0),IR(52437, 13),IR(1, 0)}, // DebugNoScope in nonsemantic_shader_debuginfo
    {25,IR(967, 3),IR(52450, 15),IR(1, 0)}, // DebugInlinedAt in nonsemantic_shader_debuginfo
    {26,IR(970, 8),IR(52507, 19),IR(1, 0)}, // DebugLocalVariable in nonsemantic_shader_debuginfo
    {27,IR(36, 2),IR(52526, 21),IR(1, 0)}, // DebugInlinedVariable in nonsemantic_shader_debuginfo
    {28,IR(978, 4),IR(52547, 13),IR(1, 0)}, // DebugDeclare in nonsemantic_shader_debuginfo
    {29,IR(978, 4),IR(52560, 11),IR(1, 0)}, // DebugValue in nonsemantic_shader_debuginfo
    {30,IR(168, 2),IR(52604, 15),IR(1, 0)}, // DebugOperation in nonsemantic_shader_debuginfo
    {31,IR(747, 1),IR(52619, 16),IR(1, 0)}, // DebugExpression in nonsemantic_shader_debuginfo
    {32,IR(580, 4),IR(52635, 14),IR(1, 0)}, // DebugMacroDef in nonsemantic_shader_debuginfo
    {33,IR(33, 3),IR(52649, 16),IR(1, 0)}, // DebugMacroUndef in nonsemantic_shader_debuginfo
    {34,IR(516, 7),IR(54404, 20),IR(1, 0)}, // DebugImportedEntity in nonsemantic_shader_debuginfo
    {35,IR(901, 2),IR(54424, 12),IR(1, 0)}, // DebugSource in nonsemantic_shader_debuginfo
    {101,IR(36, 2),IR(54436, 24),IR(1, 0)}, // DebugFunctionDefinition in nonsemantic_shader_debuginfo
    {102,IR(10, 1),IR(54460, 21),IR(1, 0)}, // DebugSourceContinued in nonsemantic_shader_debuginfo
    {103,IR(675, 5),IR(54481, 10),IR(1, 0)}, // DebugLine in nonsemantic_shader_debuginfo
    {104,IR(0, 0),IR(54491, 12),IR(1, 0)}, // DebugNoLine in nonsemantic_shader_debuginfo
    {105,IR(36, 2),IR(54503, 21),IR(1, 0)}, // DebugBuildIdentifier in nonsemantic_shader_debuginfo
    {106,IR(10, 1),IR(54524, 17),IR(1, 0)}, // DebugStoragePath in nonsemantic_shader_debuginfo
    {107,IR(296, 4),IR(54541, 16),IR(1, 0)}, // DebugEntryPoint in nonsemantic_shader_debuginfo
    {108,IR(33, 3),IR(54557, 16),IR(1, 0)}, // DebugTypeMatrix in nonsemantic_shader_debuginfo
    {109,IR(36, 2),IR(54573, 21),IR(1, 0)}, // DebugTypeVectorIdEXT in nonsemantic_shader_debuginfo
    {110,IR(675, 5),IR(54594, 30),IR(1, 0)}, // DebugTypeCooperativeMatrixKHR in nonsemantic_shader_debuginfo
    {1,IR(765, 9),IR(54624, 14),IR(1, 0)}, // Configuration in nonsemantic_vkspreflection
    {2,IR(10, 1),IR(54638, 13),IR(1, 0)}, // StartCounter in nonsemantic_vkspreflection
    {3,IR(10, 1),IR(54651, 12),IR(1, 0)}, // StopCounter in nonsemantic_vkspreflection
    {4,IR(296, 4),IR(54663, 14),IR(1, 0)}, // PushConstants in nonsemantic_vkspreflection
    {5,IR(33, 3),IR(54677, 23),IR(1, 0)}, // SpecializationMapEntry in nonsemantic_vkspreflection
    {6,IR(982, 15),IR(54700, 20),IR(1, 0)}, // DescriptorSetBuffer in nonsemantic_vkspreflection
    {7,IR(997, 33),IR(54720, 19),IR(1, 0)}, // DescriptorSetImage in nonsemantic_vkspreflection
    {8,IR(1030, 19),IR(54739, 21),IR(1, 0)}, // DescriptorSetSampler in nonsemantic_vkspreflection
    {0,IR(0, 0),IR(51806, 14),IR(1, 0)}, // DebugInfoNone in opencl_debuginfo_100
    {1,IR(1049, 4),IR(51820, 21),IR(1, 0)}, // DebugCompilationUnit in opencl_debuginfo_100
    {2,IR(1053, 3),IR(51893, 15),IR(1, 0)}, // DebugTypeBasic in opencl_debuginfo_100
    {3,IR(1056, 3),IR(51942, 17),IR(1, 0)}, // DebugTypePointer in opencl_debuginfo_100
    {4,IR(1059, 2),IR(51997, 19),IR(1, 0)}, // DebugTypeQualifier in opencl_debuginfo_100
    {5,IR(168, 2),IR(52016, 15),IR(1, 0)}, // DebugTypeArray in opencl_debuginfo_100
    {6,IR(261, 2),IR(52031, 16),IR(1, 0)}, // DebugTypeVector in opencl_debuginfo_100
    {7,IR(819, 6),IR(52047, 13),IR(1, 0)}, // DebugTypedef in opencl_debuginfo_100
    {8,IR(1061, 3),IR(52060, 18),IR(1, 0)}, // DebugTypeFunction in opencl_debuginfo_100
    {9,IR(1064, 9),IR(52078, 14),IR(1, 0)}, // DebugTypeEnum in opencl_debuginfo_100
    {10,IR(1073, 10),IR(52130, 19),IR(1, 0)}, // DebugTypeComposite in opencl_debuginfo_100
    {11,IR(1083, 10),IR(52149, 16),IR(1, 0)}, // DebugTypeMember in opencl_debuginfo_100
    {12,IR(1093, 5),IR(52165, 21),IR(1, 0)}, // DebugTypeInheritance in opencl_debuginfo_100
    {13,IR(36, 2),IR(52186, 21),IR(1, 0)}, // DebugTypePtrToMember in opencl_debuginfo_100
    {14,IR(168, 2),IR(52207, 18),IR(1, 0)}, // DebugTypeTemplate in opencl_debuginfo_100
    {15,IR(858, 6),IR(52225, 27),IR(1, 0)}, // DebugTypeTemplateParameter in opencl_debuginfo_100
    {16,IR(864, 5),IR(52252, 35),IR(1, 0)}, // DebugTypeTemplateTemplateParameter in opencl_debuginfo_100
    {17,IR(869, 5),IR(52287, 31),IR(1, 0)}, // DebugTypeTemplateParameterPack in opencl_debuginfo_100
    {18,IR(1083, 10),IR(52318, 20),IR(1, 0)}, // DebugGlobalVariable in opencl_debuginfo_100
    {19,IR(1098, 8),IR(52338, 25),IR(1, 0)}, // DebugFunctionDeclaration in opencl_debuginfo_100
    {20,IR(1106, 11),IR(52363, 14),IR(1, 0)}, // DebugFunction in opencl_debuginfo_100
    {21,IR(893, 5),IR(52377, 18),IR(1, 0)}, // DebugLexicalBlock in opencl_debuginfo_100
    {22,IR(898, 3),IR(52395, 31),IR(1, 0)}, // DebugLexicalBlockDiscriminator in opencl_debuginfo_100
    {23,IR(901, 2),IR(52426, 11),IR(1, 0)}, // DebugScope in opencl_debuginfo_100
    {24,IR(0, 0),IR(52437, 13),IR(1, 0)}, // DebugNoScope in opencl_debuginfo_100
    {25,IR(903, 3),IR(52450, 15),IR(1, 0)}, // DebugInlinedAt in opencl_debuginfo_100
    {26,IR(1117, 8),IR(52507, 19),IR(1, 0)}, // DebugLocalVariable in opencl_debuginfo_100
    {27,IR(36, 2),IR(52526, 21),IR(1, 0)}, // DebugInlinedVariable in opencl_debuginfo_100
    {28,IR(33, 3),IR(52547, 13),IR(1, 0)}, // DebugDeclare in opencl_debuginfo_100
    {29,IR(978, 4),IR(52560, 11),IR(1, 0)}, // DebugValue in opencl_debuginfo_100
    {30,IR(1125, 2),IR(52604, 15),IR(1, 0)}, // DebugOperation in opencl_debuginfo_100
    {31,IR(747, 1),IR(52619, 16),IR(1, 0)}, // DebugExpression in opencl_debuginfo_100
    {32,IR(915, 4),IR(52635, 14),IR(1, 0)}, // DebugMacroDef in opencl_debuginfo_100
    {33,IR(898, 3),IR(52649, 16),IR(1, 0)}, // DebugMacroUndef in opencl_debuginfo_100
    {34,IR(1127, 7),IR(54404, 20),IR(1, 0)}, // DebugImportedEntity in opencl_debuginfo_100
    {35,IR(901, 2),IR(54424, 12),IR(1, 0)}, // DebugSource in opencl_debuginfo_100
    {36,IR(1134, 8),IR(55060, 17),IR(1, 0)}, // DebugModuleINTEL in opencl_debuginfo_100
    {0,IR(10, 1),IR(55077, 5),IR(1, 0)}, // acos in opencl_std_100
    {1,IR(10, 1),IR(55082, 6),IR(1, 0)}, // acosh in opencl_std_100
    {2,IR(10, 1),IR(55088, 7),IR(1, 0)}, // acospi in opencl_std_100
    {3,IR(10, 1),IR(55095, 5),IR(1, 0)}, // asin in opencl_std_100
    {4,IR(10, 1),IR(55100, 6),IR(1, 0)}, // asinh in opencl_std_100
    {5,IR(10, 1),IR(55106, 7),IR(1, 0)}, // asinpi in opencl_std_100
    {6,IR(10, 1),IR(55113, 5),IR(1, 0)}, // atan in opencl_std_100
    {7,IR(36, 2),IR(55118, 6),IR(1, 0)}, // atan2 in opencl_std_100
    {8,IR(10, 1),IR(55124, 6),IR(1, 0)}, // atanh in opencl_std_100
    {9,IR(10, 1),IR(55130, 7),IR(1, 0)}, // atanpi in opencl_std_100
    {10,IR(36, 2),IR(55137, 8),IR(1, 0)}, // atan2pi in opencl_std_100
    {11,IR(10, 1),IR(55145, 5),IR(1, 0)}, // cbrt in opencl_std_100
    {12,IR(10, 1),IR(55150, 5),IR(1, 0)}, // ceil in opencl_std_100
    {13,IR(36, 2),IR(55155, 9),IR(1, 0)}, // copysign in opencl_std_100
    {14,IR(10, 1),IR(55164, 4),IR(1, 0)}, // cos in opencl_std_100
    {15,IR(10, 1),IR(55168, 5),IR(1, 0)}, // cosh in opencl_std_100
    {16,IR(10, 1),IR(55173, 6),IR(1, 0)}, // cospi in opencl_std_100
    {17,IR(10, 1),IR(55179, 5),IR(1, 0)}, // erfc in opencl_std_100
    {18,IR(10, 1),IR(55184, 4),IR(1, 0)}, // erf in opencl_std_100
    {19,IR(10, 1),IR(55188, 4),IR(1, 0)}, // exp in opencl_std_100
    {20,IR(10, 1),IR(55192, 5),IR(1, 0)}, // exp2 in opencl_std_100
    {21,IR(10, 1),IR(55197, 6),IR(1, 0)}, // exp10 in opencl_std_100
    {22,IR(10, 1),IR(55203, 6),IR(1, 0)}, // expm1 in opencl_std_100
    {23,IR(10, 1),IR(55209, 5),IR(1, 0)}, // fabs in opencl_std_100
    {24,IR(36, 2),IR(55214, 5),IR(1, 0)}, // fdim in opencl_std_100
    {25,IR(10, 1),IR(55219, 6),IR(1, 0)}, // floor in opencl_std_100
    {26,IR(33, 3),IR(55225, 4),IR(1, 0)}, // fma in opencl_std_100
    {27,IR(36, 2),IR(55229, 5),IR(1, 0)}, // fmax in opencl_std_100
    {28,IR(36, 2),IR(55234, 5),IR(1, 0)}, // fmin in opencl_std_100
    {29,IR(36, 2),IR(55239, 5),IR(1, 0)}, // fmod in opencl_std_100
    {30,IR(36, 2),IR(55244, 6),IR(1, 0)}, // fract in opencl_std_100
    {31,IR(36, 2),IR(55250, 6),IR(1, 0)}, // frexp in opencl_std_100
    {32,IR(36, 2),IR(55256, 6),IR(1, 0)}, // hypot in opencl_std_100
    {33,IR(10, 1),IR(55262, 6),IR(1, 0)}, // ilogb in opencl_std_100
    {34,IR(36, 2),IR(55268, 6),IR(1, 0)}, // ldexp in opencl_std_100
    {35,IR(10, 1),IR(55274, 7),IR(1, 0)}, // lgamma in opencl_std_100
    {36,IR(36, 2),IR(55281, 9),IR(1, 0)}, // lgamma_r in opencl_std_100
    {37,IR(10, 1),IR(55290, 4),IR(1, 0)}, // log in opencl_std_100
    {38,IR(10, 1),IR(55294, 5),IR(1, 0)}, // log2 in opencl_std_100
    {39,IR(10, 1),IR(55299, 6),IR(1, 0)}, // log10 in opencl_std_100
    {40,IR(10, 1),IR(55305, 6),IR(1, 0)}, // log1p in opencl_std_100
    {41,IR(10, 1),IR(55311, 5),IR(1, 0)}, // logb in opencl_std_100
    {42,IR(33, 3),IR(55316, 4),IR(1, 0)}, // mad in opencl_std_100
    {43,IR(36, 2),IR(55320, 7),IR(1, 0)}, // maxmag in opencl_std_100
    {44,IR(36, 2),IR(55327, 7),IR(1, 0)}, // minmag in opencl_std_100
    {45,IR(36, 2),IR(55334, 5),IR(1, 0)}, // modf in opencl_std_100
    {46,IR(10, 1),IR(55339, 4),IR(1, 0)}, // nan in opencl_std_100
    {47,IR(36, 2),IR(55343, 10),IR(1, 0)}, // nextafter in opencl_std_100
    {48,IR(36, 2),IR(55353, 4),IR(1, 0)}, // pow in opencl_std_100
    {49,IR(36, 2),IR(55357, 5),IR(1, 0)}, // pown in opencl_std_100
    {50,IR(36, 2),IR(55362, 5),IR(1, 0)}, // powr in opencl_std_100
    {51,IR(36, 2),IR(55367, 10),IR(1, 0)}, // remainder in opencl_std_100
    {52,IR(33, 3),IR(55377, 7),IR(1, 0)}, // remquo in opencl_std_100
    {53,IR(10, 1),IR(55384, 5),IR(1, 0)}, // rint in opencl_std_100
    {54,IR(36, 2),IR(55389, 6),IR(1, 0)}, // rootn in opencl_std_100
    {55,IR(10, 1),IR(55395, 6),IR(1, 0)}, // round in opencl_std_100
    {56,IR(10, 1),IR(55401, 6),IR(1, 0)}, // rsqrt in opencl_std_100
    {57,IR(10, 1),IR(55407, 4),IR(1, 0)}, // sin in opencl_std_100
    {58,IR(36, 2),IR(55411, 7),IR(1, 0)}, // sincos in opencl_std_100
    {59,IR(10, 1),IR(55418, 5),IR(1, 0)}, // sinh in opencl_std_100
    {60,IR(10, 1),IR(55423, 6),IR(1, 0)}, // sinpi in opencl_std_100
    {61,IR(10, 1),IR(55429, 5),IR(1, 0)}, // sqrt in opencl_std_100
    {62,IR(10, 1),IR(55434, 4),IR(1, 0)}, // tan in opencl_std_100
    {63,IR(10, 1),IR(55438, 5),IR(1, 0)}, // tanh in opencl_std_100
    {64,IR(10, 1),IR(55443, 6),IR(1, 0)}, // tanpi in opencl_std_100
    {65,IR(10, 1),IR(55449, 7),IR(1, 0)}, // tgamma in opencl_std_100
    {66,IR(10, 1),IR(55456, 6),IR(1, 0)}, // trunc in opencl_std_100
    {67,IR(10, 1),IR(55462, 9),IR(1, 0)}, // half_cos in opencl_std_100
    {68,IR(36, 2),IR(55471, 12),IR(1, 0)}, // half_divide in opencl_std_100
    {69,IR(10, 1),IR(55483, 9),IR(1, 0)}, // half_exp in opencl_std_100
    {70,IR(10, 1),IR(55492, 10),IR(1, 0)}, // half_exp2 in opencl_std_100
    {71,IR(10, 1),IR(55502, 11),IR(1, 0)}, // half_exp10 in opencl_std_100
    {72,IR(10, 1),IR(55513, 9),IR(1, 0)}, // half_log in opencl_std_100
    {73,IR(10, 1),IR(55522, 10),IR(1, 0)}, // half_log2 in opencl_std_100
    {74,IR(10, 1),IR(55532, 11),IR(1, 0)}, // half_log10 in opencl_std_100
    {75,IR(36, 2),IR(55543, 10),IR(1, 0)}, // half_powr in opencl_std_100
    {76,IR(10, 1),IR(55553, 11),IR(1, 0)}, // half_recip in opencl_std_100
    {77,IR(10, 1),IR(55564, 11),IR(1, 0)}, // half_rsqrt in opencl_std_100
    {78,IR(10, 1),IR(55575, 9),IR(1, 0)}, // half_sin in opencl_std_100
    {79,IR(10, 1),IR(55584, 10),IR(1, 0)}, // half_sqrt in opencl_std_100
    {80,IR(10, 1),IR(55594, 9),IR(1, 0)}, // half_tan in opencl_std_100
    {81,IR(10, 1),IR(55603, 11),IR(1, 0)}, // native_cos in opencl_std_100
    {82,IR(36, 2),IR(55614, 14),IR(1, 0)}, // native_divide in opencl_std_100
    {83,IR(10, 1),IR(55628, 11),IR(1, 0)}, // native_exp in opencl_std_100
    {84,IR(10, 1),IR(55639, 12),IR(1, 0)}, // native_exp2 in opencl_std_100
    {85,IR(10, 1),IR(55651, 13),IR(1, 0)}, // native_exp10 in opencl_std_100
    {86,IR(10, 1),IR(55664, 11),IR(1, 0)}, // native_log in opencl_std_100
    {87,IR(10, 1),IR(55675, 12),IR(1, 0)}, // native_log2 in opencl_std_100
    {88,IR(10, 1),IR(55687, 13),IR(1, 0)}, // native_log10 in opencl_std_100
    {89,IR(36, 2),IR(55700, 12),IR(1, 0)}, // native_powr in opencl_std_100
    {90,IR(10, 1),IR(55712, 13),IR(1, 0)}, // native_recip in opencl_std_100
    {91,IR(10, 1),IR(55725, 13),IR(1, 0)}, // native_rsqrt in opencl_std_100
    {92,IR(10, 1),IR(55738, 11),IR(1, 0)}, // native_sin in opencl_std_100
    {93,IR(10, 1),IR(55749, 12),IR(1, 0)}, // native_sqrt in opencl_std_100
    {94,IR(10, 1),IR(55761, 11),IR(1, 0)}, // native_tan in opencl_std_100
    {95,IR(33, 3),IR(55772, 7),IR(1, 0)}, // fclamp in opencl_std_100
    {96,IR(10, 1),IR(55779, 8),IR(1, 0)}, // degrees in opencl_std_100
    {97,IR(36, 2),IR(55787, 12),IR(1, 0)}, // fmax_common in opencl_std_100
    {98,IR(36, 2),IR(55799, 12),IR(1, 0)}, // fmin_common in opencl_std_100
    {99,IR(33, 3),IR(55811, 4),IR(1, 0)}, // mix in opencl_std_100
    {100,IR(10, 1),IR(55815, 8),IR(1, 0)}, // radians in opencl_std_100
    {101,IR(36, 2),IR(55823, 5),IR(1, 0)}, // step in opencl_std_100
    {102,IR(33, 3),IR(55828, 11),IR(1, 0)}, // smoothstep in opencl_std_100
    {103,IR(10, 1),IR(55839, 5),IR(1, 0)}, // sign in opencl_std_100
    {104,IR(36, 2),IR(55844, 6),IR(1, 0)}, // cross in opencl_std_100
    {105,IR(36, 2),IR(55850, 9),IR(1, 0)}, // distance in opencl_std_100
    {106,IR(10, 1),IR(55859, 7),IR(1, 0)}, // length in opencl_std_100
    {107,IR(10, 1),IR(55866, 10),IR(1, 0)}, // normalize in opencl_std_100
    {108,IR(36, 2),IR(55876, 14),IR(1, 0)}, // fast_distance in opencl_std_100
    {109,IR(10, 1),IR(55890, 12),IR(1, 0)}, // fast_length in opencl_std_100
    {110,IR(10, 1),IR(55902, 15),IR(1, 0)}, // fast_normalize in opencl_std_100
    {141,IR(10, 1),IR(55917, 6),IR(1, 0)}, // s_abs in opencl_std_100
    {142,IR(36, 2),IR(55923, 11),IR(1, 0)}, // s_abs_diff in opencl_std_100
    {143,IR(36, 2),IR(55934, 10),IR(1, 0)}, // s_add_sat in opencl_std_100
    {144,IR(36, 2),IR(55944, 10),IR(1, 0)}, // u_add_sat in opencl_std_100
    {145,IR(36, 2),IR(55954, 7),IR(1, 0)}, // s_hadd in opencl_std_100
    {146,IR(36, 2),IR(55961, 7),IR(1, 0)}, // u_hadd in opencl_std_100
    {147,IR(36, 2),IR(55968, 8),IR(1, 0)}, // s_rhadd in opencl_std_100
    {148,IR(36, 2),IR(55976, 8),IR(1, 0)}, // u_rhadd in opencl_std_100
    {149,IR(33, 3),IR(55984, 8),IR(1, 0)}, // s_clamp in opencl_std_100
    {150,IR(33, 3),IR(55992, 8),IR(1, 0)}, // u_clamp in opencl_std_100
    {151,IR(10, 1),IR(56000, 4),IR(1, 0)}, // clz in opencl_std_100
    {152,IR(10, 1),IR(56004, 4),IR(1, 0)}, // ctz in opencl_std_100
    {153,IR(33, 3),IR(56008, 9),IR(1, 0)}, // s_mad_hi in opencl_std_100
    {154,IR(33, 3),IR(56017, 10),IR(1, 0)}, // u_mad_sat in opencl_std_100
    {155,IR(33, 3),IR(56027, 10),IR(1, 0)}, // s_mad_sat in opencl_std_100
    {156,IR(36, 2),IR(56037, 6),IR(1, 0)}, // s_max in opencl_std_100
    {157,IR(36, 2),IR(56043, 6),IR(1, 0)}, // u_max in opencl_std_100
    {158,IR(36, 2),IR(56049, 6),IR(1, 0)}, // s_min in opencl_std_100
    {159,IR(36, 2),IR(56055, 6),IR(1, 0)}, // u_min in opencl_std_100
    {160,IR(36, 2),IR(56061, 9),IR(1, 0)}, // s_mul_hi in opencl_std_100
    {161,IR(36, 2),IR(56070, 7),IR(1, 0)}, // rotate in opencl_std_100
    {162,IR(36, 2),IR(56077, 10),IR(1, 0)}, // s_sub_sat in opencl_std_100
    {163,IR(36, 2),IR(56087, 10),IR(1, 0)}, // u_sub_sat in opencl_std_100
    {164,IR(36, 2),IR(56097, 11),IR(1, 0)}, // u_upsample in opencl_std_100
    {165,IR(36, 2),IR(56108, 11),IR(1, 0)}, // s_upsample in opencl_std_100
    {166,IR(10, 1),IR(56119, 9),IR(1, 0)}, // popcount in opencl_std_100
    {167,IR(33, 3),IR(56128, 8),IR(1, 0)}, // s_mad24 in opencl_std_100
    {168,IR(33, 3),IR(56136, 8),IR(1, 0)}, // u_mad24 in opencl_std_100
    {169,IR(36, 2),IR(56144, 8),IR(1, 0)}, // s_mul24 in opencl_std_100
    {170,IR(36, 2),IR(56152, 8),IR(1, 0)}, // u_mul24 in opencl_std_100
    {171,IR(1142, 3),IR(56160, 7),IR(1, 0)}, // vloadn in opencl_std_100
    {172,IR(33, 3),IR(56167, 8),IR(1, 0)}, // vstoren in opencl_std_100
    {173,IR(36, 2),IR(56175, 11),IR(1, 0)}, // vload_half in opencl_std_100
    {174,IR(1142, 3),IR(56186, 12),IR(1, 0)}, // vload_halfn in opencl_std_100
    {175,IR(33, 3),IR(56198, 12),IR(1, 0)}, // vstore_half in opencl_std_100
    {176,IR(1145, 4),IR(56210, 14),IR(1, 0)}, // vstore_half_r in opencl_std_100
    {177,IR(33, 3),IR(56224, 13),IR(1, 0)}, // vstore_halfn in opencl_std_100
    {178,IR(1145, 4),IR(56237, 15),IR(1, 0)}, // vstore_halfn_r in opencl_std_100
    {179,IR(1142, 3),IR(56252, 13),IR(1, 0)}, // vloada_halfn in opencl_std_100
    {180,IR(33, 3),IR(56265, 14),IR(1, 0)}, // vstorea_halfn in opencl_std_100
    {181,IR(1145, 4),IR(56279, 16),IR(1, 0)}, // vstorea_halfn_r in opencl_std_100
    {182,IR(36, 2),IR(56295, 8),IR(1, 0)}, // shuffle in opencl_std_100
    {183,IR(33, 3),IR(56303, 9),IR(1, 0)}, // shuffle2 in opencl_std_100
    {184,IR(168, 2),IR(56312, 7),IR(1, 0)}, // printf in opencl_std_100
    {185,IR(36, 2),IR(56319, 9),IR(1, 0)}, // prefetch in opencl_std_100
    {186,IR(33, 3),IR(56328, 10),IR(1, 0)}, // bitselect in opencl_std_100
    {187,IR(33, 3),IR(56338, 7),IR(1, 0)}, // select in opencl_std_100
    {201,IR(10, 1),IR(56345, 6),IR(1, 0)}, // u_abs in opencl_std_100
    {202,IR(36, 2),IR(56351, 11),IR(1, 0)}, // u_abs_diff in opencl_std_100
    {203,IR(36, 2),IR(56362, 9),IR(1, 0)}, // u_mul_hi in opencl_std_100
    {204,IR(33, 3),IR(56371, 9),IR(1, 0)}, // u_mad_hi in opencl_std_100
    {1,IR(10, 1),IR(56380, 17),IR(1, 0)}, // CubeFaceIndexAMD in spv_amd_gcn_shader
    {2,IR(10, 1),IR(56397, 17),IR(1, 0)}, // CubeFaceCoordAMD in spv_amd_gcn_shader
    {3,IR(0, 0),IR(56414, 8),IR(1, 0)}, // TimeAMD in spv_amd_gcn_shader
    {1,IR(36, 2),IR(56422, 22),IR(1, 0)}, // SwizzleInvocationsAMD in spv_amd_shader_ballot
    {2,IR(36, 2),IR(56444, 28),IR(1, 0)}, // SwizzleInvocationsMaskedAMD in spv_amd_shader_ballot
    {3,IR(33, 3),IR(56472, 19),IR(1, 0)}, // WriteInvocationAMD in spv_amd_shader_ballot
    {4,IR(10, 1),IR(56491, 9),IR(1, 0)}, // MbcntAMD in spv_amd_shader_ballot
    {1,IR(36, 2),IR(56500, 23),IR(1, 0)}, // InterpolateAtVertexAMD in spv_amd_shader_explicit_vertex_parameter
    {1,IR(33, 3),IR(56523, 9),IR(1, 0)}, // FMin3AMD in spv_amd_shader_trinary_minmax
    {2,IR(33, 3),IR(56532, 9),IR(1, 0)}, // UMin3AMD in spv_amd_shader_trinary_minmax
    {3,IR(33, 3),IR(56541, 9),IR(1, 0)}, // SMin3AMD in spv_amd_shader_trinary_minmax
    {4,IR(33, 3),IR(56550, 9),IR(1, 0)}, // FMax3AMD in spv_amd_shader_trinary_minmax
    {5,IR(33, 3),IR(56559, 9),IR(1, 0)}, // UMax3AMD in spv_amd_shader_trinary_minmax
    {6,IR(33, 3),IR(56568, 9),IR(1, 0)}, // SMax3AMD in spv_amd_shader_trinary_minmax
    {7,IR(33, 3),IR(56577, 9),IR(1, 0)}, // FMid3AMD in spv_amd_shader_trinary_minmax
    {8,IR(33, 3),IR(56586, 9),IR(1, 0)}, // UMid3AMD in spv_amd_shader_trinary_minmax
    {9,IR(33, 3),IR(56595, 9),IR(1, 0)}, // SMid3AMD in spv_amd_shader_trinary_minmax
    {0,IR(33, 3),IR(56604, 7),IR(1, 0)}, // ARGMAX in tosa_001000_1
    {1,IR(516, 7),IR(56611, 11),IR(1, 0)}, // AVG_POOL2D in tosa_001000_1
    {2,IR(755, 10),IR(56622, 7),IR(1, 0)}, // CONV2D in tosa_001000_1
    {3,IR(755, 10),IR(56629, 7),IR(1, 0)}, // CONV3D in tosa_001000_1
    {4,IR(755, 10),IR(56636, 17),IR(1, 0)}, // DEPTHWISE_CONV2D in tosa_001000_1
    {5,IR(296, 4),IR(56653, 6),IR(1, 0)}, // FFT2D in tosa_001000_1
    {6,IR(296, 4),IR(56659, 7),IR(1, 0)}, // MATMUL in tosa_001000_1
    {7,IR(675, 5),IR(56666, 11),IR(1, 0)}, // MAX_POOL2D in tosa_001000_1
    {8,IR(36, 2),IR(56677, 7),IR(1, 0)}, // RFFT2D in tosa_001000_1
    {9,IR(765, 9),IR(56684, 17),IR(1, 0)}, // TRANSPOSE_CONV2D in tosa_001000_1
    {10,IR(296, 4),IR(56701, 6),IR(1, 0)}, // CLAMP in tosa_001000_1
    {11,IR(10, 1),IR(56707, 4),IR(1, 0)}, // ERF in tosa_001000_1
    {12,IR(10, 1),IR(56711, 8),IR(1, 0)}, // SIGMOID in tosa_001000_1
    {13,IR(10, 1),IR(56719, 5),IR(1, 0)}, // TANH in tosa_001000_1
    {14,IR(36, 2),IR(56724, 4),IR(1, 0)}, // ADD in tosa_001000_1
    {15,IR(33, 3),IR(56728, 23),IR(1, 0)}, // ARITHMETIC_RIGHT_SHIFT in tosa_001000_1
    {16,IR(36, 2),IR(56751, 12),IR(1, 0)}, // BITWISE_AND in tosa_001000_1
    {17,IR(36, 2),IR(56763, 11),IR(1, 0)}, // BITWISE_OR in tosa_001000_1
    {18,IR(36, 2),IR(56774, 12),IR(1, 0)}, // BITWISE_XOR in tosa_001000_1
    {19,IR(36, 2),IR(56786, 7),IR(1, 0)}, // INTDIV in tosa_001000_1
    {20,IR(36, 2),IR(56793, 12),IR(1, 0)}, // LOGICAL_AND in tosa_001000_1
    {21,IR(36, 2),IR(56805, 19),IR(1, 0)}, // LOGICAL_LEFT_SHIFT in tosa_001000_1
    {22,IR(36, 2),IR(56824, 20),IR(1, 0)}, // LOGICAL_RIGHT_SHIFT in tosa_001000_1
    {23,IR(36, 2),IR(56844, 11),IR(1, 0)}, // LOGICAL_OR in tosa_001000_1
    {24,IR(36, 2),IR(56855, 12),IR(1, 0)}, // LOGICAL_XOR in tosa_001000_1
    {25,IR(33, 3),IR(56867, 8),IR(1, 0)}, // MAXIMUM in tosa_001000_1
    {26,IR(33, 3),IR(56875, 8),IR(1, 0)}, // MINIMUM in tosa_001000_1
    {27,IR(33, 3),IR(56883, 4),IR(1, 0)}, // MUL in tosa_001000_1
    {28,IR(36, 2),IR(56887, 4),IR(1, 0)}, // POW in tosa_001000_1
    {29,IR(36, 2),IR(56891, 4),IR(1, 0)}, // SUB in tosa_001000_1
    {30,IR(36, 2),IR(56895, 6),IR(1, 0)}, // TABLE in tosa_001000_1
    {31,IR(10, 1),IR(56901, 4),IR(1, 0)}, // ABS in tosa_001000_1
    {32,IR(10, 1),IR(56905, 12),IR(1, 0)}, // BITWISE_NOT in tosa_001000_1
    {33,IR(10, 1),IR(56917, 5),IR(1, 0)}, // CEIL in tosa_001000_1
    {34,IR(10, 1),IR(56922, 4),IR(1, 0)}, // CLZ in tosa_001000_1
    {35,IR(10, 1),IR(56926, 4),IR(1, 0)}, // COS in tosa_001000_1
    {36,IR(10, 1),IR(56930, 4),IR(1, 0)}, // EXP in tosa_001000_1
    {37,IR(10, 1),IR(56934, 6),IR(1, 0)}, // FLOOR in tosa_001000_1
    {38,IR(10, 1),IR(56940, 4),IR(1, 0)}, // LOG in tosa_001000_1
    {39,IR(10, 1),IR(56944, 12),IR(1, 0)}, // LOGICAL_NOT in tosa_001000_1
    {40,IR(33, 3),IR(56956, 7),IR(1, 0)}, // NEGATE in tosa_001000_1
    {41,IR(10, 1),IR(56963, 11),IR(1, 0)}, // RECIPROCAL in tosa_001000_1
    {42,IR(10, 1),IR(56974, 6),IR(1, 0)}, // RSQRT in tosa_001000_1
    {43,IR(10, 1),IR(56980, 4),IR(1, 0)}, // SIN in tosa_001000_1
    {44,IR(33, 3),IR(56984, 7),IR(1, 0)}, // SELECT in tosa_001000_1
    {45,IR(36, 2),IR(56991, 6),IR(1, 0)}, // EQUAL in tosa_001000_1
    {46,IR(36, 2),IR(56997, 8),IR(1, 0)}, // GREATER in tosa_001000_1
    {47,IR(36, 2),IR(57005, 14),IR(1, 0)}, // GREATER_EQUAL in tosa_001000_1
    {48,IR(36, 2),IR(57019, 11),IR(1, 0)}, // REDUCE_ALL in tosa_001000_1
    {49,IR(36, 2),IR(57030, 11),IR(1, 0)}, // REDUCE_ANY in tosa_001000_1
    {50,IR(33, 3),IR(57041, 11),IR(1, 0)}, // REDUCE_MAX in tosa_001000_1
    {51,IR(33, 3),IR(57052, 11),IR(1, 0)}, // REDUCE_MIN in tosa_001000_1
    {52,IR(36, 2),IR(57063, 15),IR(1, 0)}, // REDUCE_PRODUCT in tosa_001000_1
    {53,IR(36, 2),IR(57078, 11),IR(1, 0)}, // REDUCE_SUM in tosa_001000_1
    {54,IR(168, 2),IR(57089, 7),IR(1, 0)}, // CONCAT in tosa_001000_1
    {55,IR(33, 3),IR(57096, 4),IR(1, 0)}, // PAD in tosa_001000_1
    {56,IR(36, 2),IR(57100, 8),IR(1, 0)}, // RESHAPE in tosa_001000_1
    {57,IR(36, 2),IR(57108, 8),IR(1, 0)}, // REVERSE in tosa_001000_1
    {58,IR(33, 3),IR(57116, 6),IR(1, 0)}, // SLICE in tosa_001000_1
    {59,IR(36, 2),IR(57122, 5),IR(1, 0)}, // TILE in tosa_001000_1
    {60,IR(36, 2),IR(57127, 10),IR(1, 0)}, // TRANSPOSE in tosa_001000_1
    {61,IR(36, 2),IR(57137, 7),IR(1, 0)}, // GATHER in tosa_001000_1
    {62,IR(33, 3),IR(57144, 8),IR(1, 0)}, // SCATTER in tosa_001000_1
    {63,IR(675, 5),IR(57152, 7),IR(1, 0)}, // RESIZE in tosa_001000_1
    {64,IR(10, 1),IR(57159, 5),IR(1, 0)}, // CAST in tosa_001000_1
    {65,IR(755, 10),IR(57164, 8),IR(1, 0)}, // RESCALE in tosa_001000_1
}};

IndexRange ExtInstByValueRangeForKind(spv_ext_inst_type_t type) {
  switch(type) {
    case SPV_EXT_INST_TYPE_ARM_MOTION_ENGINE_100: return IR(0, 3);
    case SPV_EXT_INST_TYPE_DEBUGINFO: return IR(3, 34);
    case SPV_EXT_INST_TYPE_GLSL_STD_450: return IR(37, 81);
    case SPV_EXT_INST_TYPE_NONSEMANTIC_CLSPVREFLECTION: return IR(118, 42);
    case SPV_EXT_INST_TYPE_NONSEMANTIC_SHADER_DEBUGINFO_100: return IR(160, 46);
    case SPV_EXT_INST_TYPE_NONSEMANTIC_VKSPREFLECTION: return IR(206, 8);
    case SPV_EXT_INST_TYPE_OPENCL_DEBUGINFO_100: return IR(214, 37);
    case SPV_EXT_INST_TYPE_OPENCL_STD: return IR(251, 162);
    case SPV_EXT_INST_TYPE_SPV_AMD_GCN_SHADER: return IR(413, 3);
    case SPV_EXT_INST_TYPE_SPV_AMD_SHADER_BALLOT: return IR(416, 4);
    case SPV_EXT_INST_TYPE_SPV_AMD_SHADER_EXPLICIT_VERTEX_PARAMETER: return IR(420, 1);
    case SPV_EXT_INST_TYPE_SPV_AMD_SHADER_TRINARY_MINMAX: return IR(421, 9);
    case SPV_EXT_INST_TYPE_TOSA_001000_1: return IR(430, 66);
    default: break;
  }
  return IR(0,0);
}

// Extended instruction opcode names sorted by extended instruction kind, then opcode name.
// The fields in order are:
//   name
//   index into kExtInstByValue
static const std::array<NameIndex, 496> kExtInstNames{{
    {IR(51777, 8), 0}, // MIN_SAD in arm_motion_engine_100
    {IR(51785, 13), 1}, // MIN_SAD_COST in arm_motion_engine_100
    {IR(51798, 8), 2}, // RAW_SAD in arm_motion_engine_100
    {IR(51820, 21), 4}, // DebugCompilationUnit in debuginfo
    {IR(52547, 13), 31}, // DebugDeclare in debuginfo
    {IR(52619, 16), 34}, // DebugExpression in debuginfo
    {IR(52363, 14), 23}, // DebugFunction in debuginfo
    {IR(52338, 25), 22}, // DebugFunctionDeclaration in debuginfo
    {IR(52318, 20), 21}, // DebugGlobalVariable in debuginfo
    {IR(51806, 14), 3}, // DebugInfoNone in debuginfo
    {IR(52450, 15), 28}, // DebugInlinedAt in debuginfo
    {IR(52526, 21), 30}, // DebugInlinedVariable in debuginfo
    {IR(52377, 18), 24}, // DebugLexicalBlock in debuginfo
    {IR(52395, 31), 25}, // DebugLexicalBlockDiscriminator in debuginfo
    {IR(52507, 19), 29}, // DebugLocalVariable in debuginfo
    {IR(52635, 14), 35}, // DebugMacroDef in debuginfo
    {IR(52649, 16), 36}, // DebugMacroUndef in debuginfo
    {IR(52437, 13), 27}, // DebugNoScope in debuginfo
    {IR(52604, 15), 33}, // DebugOperation in debuginfo
    {IR(52426, 11), 26}, // DebugScope in debuginfo
    {IR(52016, 15), 8}, // DebugTypeArray in debuginfo
    {IR(51893, 15), 5}, // DebugTypeBasic in debuginfo
    {IR(52130, 19), 13}, // DebugTypeComposite in debuginfo
    {IR(52078, 14), 12}, // DebugTypeEnum in debuginfo
    {IR(52060, 18), 11}, // DebugTypeFunction in debuginfo
    {IR(52165, 21), 15}, // DebugTypeInheritance in debuginfo
    {IR(52149, 16), 14}, // DebugTypeMember in debuginfo
    {IR(51942, 17), 6}, // DebugTypePointer in debuginfo
    {IR(52186, 21), 16}, // DebugTypePtrToMember in debuginfo
    {IR(51997, 19), 7}, // DebugTypeQualifier in debuginfo
    {IR(52207, 18), 17}, // DebugTypeTemplate in debuginfo
    {IR(52225, 27), 18}, // DebugTypeTemplateParameter in debuginfo
    {IR(52287, 31), 20}, // DebugTypeTemplateParameterPack in debuginfo
    {IR(52252, 35), 19}, // DebugTypeTemplateTemplateParameter in debuginfo
    {IR(52031, 16), 9}, // DebugTypeVector in debuginfo
    {IR(52047, 13), 10}, // DebugTypedef in debuginfo
    {IR(52560, 11), 32}, // DebugValue in debuginfo
    {IR(52759, 5), 53}, // Acos in glsl_std_450
    {IR(52790, 6), 59}, // Acosh in glsl_std_450
    {IR(52754, 5), 52}, // Asin in glsl_std_450
    {IR(52784, 6), 58}, // Asinh in glsl_std_450
    {IR(52764, 5), 54}, // Atan in glsl_std_450
    {IR(52802, 6), 61}, // Atan2 in glsl_std_450
    {IR(52796, 6), 60}, // Atanh in glsl_std_450
    {IR(52715, 5), 45}, // Ceil in glsl_std_450
    {IR(52746, 4), 50}, // Cos in glsl_std_450
    {IR(52774, 5), 56}, // Cosh in glsl_std_450
    {IR(53186, 6), 104}, // Cross in glsl_std_450
    {IR(52734, 8), 48}, // Degrees in glsl_std_450
    {IR(52847, 12), 69}, // Determinant in glsl_std_450
    {IR(53177, 9), 103}, // Distance in glsl_std_450
    {IR(52812, 4), 63}, // Exp in glsl_std_450
    {IR(52820, 5), 65}, // Exp2 in glsl_std_450
    {IR(52687, 5), 40}, // FAbs in glsl_std_450
    {IR(52919, 7), 79}, // FClamp in glsl_std_450
    {IR(52904, 5), 76}, // FMax in glsl_std_450
    {IR(52889, 5), 73}, // FMin in glsl_std_450
    {IR(52940, 5), 82}, // FMix in glsl_std_450
    {IR(52697, 6), 42}, // FSign in glsl_std_450
    {IR(53202, 12), 106}, // FaceForward in glsl_std_450
    {IR(53230, 9), 109}, // FindILsb in glsl_std_450
    {IR(53239, 9), 110}, // FindSMsb in glsl_std_450
    {IR(53248, 9), 111}, // FindUMsb in glsl_std_450
    {IR(52709, 6), 44}, // Floor in glsl_std_450
    {IR(52966, 4), 86}, // Fma in glsl_std_450
    {IR(52720, 6), 46}, // Fract in glsl_std_450
    {IR(52970, 6), 87}, // Frexp in glsl_std_450
    {IR(52976, 12), 88}, // FrexpStruct in glsl_std_450
    {IR(52945, 5), 83}, // IMix in glsl_std_450
    {IR(53257, 22), 112}, // InterpolateAtCentroid in glsl_std_450
    {IR(53299, 20), 114}, // InterpolateAtOffset in glsl_std_450
    {IR(53279, 20), 113}, // InterpolateAtSample in glsl_std_450
    {IR(52835, 12), 68}, // InverseSqrt in glsl_std_450
    {IR(52988, 6), 89}, // Ldexp in glsl_std_450
    {IR(53170, 7), 102}, // Length in glsl_std_450
    {IR(52816, 4), 64}, // Log in glsl_std_450
    {IR(52825, 5), 66}, // Log2 in glsl_std_450
    {IR(52859, 14), 70}, // MatrixInverse in glsl_std_450
    {IR(52873, 5), 71}, // Modf in glsl_std_450
    {IR(52878, 11), 72}, // ModfStruct in glsl_std_450
    {IR(53329, 7), 117}, // NClamp in glsl_std_450
    {IR(53324, 5), 116}, // NMax in glsl_std_450
    {IR(53319, 5), 115}, // NMin in glsl_std_450
    {IR(53192, 10), 105}, // Normalize in glsl_std_450
    {IR(53061, 15), 95}, // PackDouble2x32 in glsl_std_450
    {IR(53048, 13), 94}, // PackHalf2x16 in glsl_std_450
    {IR(53020, 14), 92}, // PackSnorm2x16 in glsl_std_450
    {IR(52994, 13), 90}, // PackSnorm4x8 in glsl_std_450
    {IR(53034, 14), 93}, // PackUnorm2x16 in glsl_std_450
    {IR(53007, 13), 91}, // PackUnorm4x8 in glsl_std_450
    {IR(52808, 4), 62}, // Pow in glsl_std_450
    {IR(52726, 8), 47}, // Radians in glsl_std_450
    {IR(53214, 8), 107}, // Reflect in glsl_std_450
    {IR(53222, 8), 108}, // Refract in glsl_std_450
    {IR(52665, 6), 37}, // Round in glsl_std_450
    {IR(52671, 10), 38}, // RoundEven in glsl_std_450
    {IR(52692, 5), 41}, // SAbs in glsl_std_450
    {IR(52933, 7), 81}, // SClamp in glsl_std_450
    {IR(52914, 5), 78}, // SMax in glsl_std_450
    {IR(52899, 5), 75}, // SMin in glsl_std_450
    {IR(52703, 6), 43}, // SSign in glsl_std_450
    {IR(52742, 4), 49}, // Sin in glsl_std_450
    {IR(52769, 5), 55}, // Sinh in glsl_std_450
    {IR(52955, 11), 85}, // SmoothStep in glsl_std_450
    {IR(52830, 5), 67}, // Sqrt in glsl_std_450
    {IR(52950, 5), 84}, // Step in glsl_std_450
    {IR(52750, 4), 51}, // Tan in glsl_std_450
    {IR(52779, 5), 57}, // Tanh in glsl_std_450
    {IR(52681, 6), 39}, // Trunc in glsl_std_450
    {IR(52926, 7), 80}, // UClamp in glsl_std_450
    {IR(52909, 5), 77}, // UMax in glsl_std_450
    {IR(52894, 5), 74}, // UMin in glsl_std_450
    {IR(53153, 17), 101}, // UnpackDouble2x32 in glsl_std_450
    {IR(53108, 15), 98}, // UnpackHalf2x16 in glsl_std_450
    {IR(53076, 16), 96}, // UnpackSnorm2x16 in glsl_std_450
    {IR(53123, 15), 99}, // UnpackSnorm4x8 in glsl_std_450
    {IR(53092, 16), 97}, // UnpackUnorm2x16 in glsl_std_450
    {IR(53138, 15), 100}, // UnpackUnorm4x8 in glsl_std_450
    {IR(53336, 13), 119}, // ArgumentInfo in nonsemantic_clspvreflection
    {IR(53431, 24), 124}, // ArgumentPodPushConstant in nonsemantic_clspvreflection
    {IR(53387, 25), 122}, // ArgumentPodStorageBuffer in nonsemantic_clspvreflection
    {IR(53412, 19), 123}, // ArgumentPodUniform in nonsemantic_clspvreflection
    {IR(53865, 28), 143}, // ArgumentPointerPushConstant in nonsemantic_clspvreflection
    {IR(53893, 23), 144}, // ArgumentPointerUniform in nonsemantic_clspvreflection
    {IR(53455, 21), 125}, // ArgumentSampledImage in nonsemantic_clspvreflection
    {IR(53497, 16), 127}, // ArgumentSampler in nonsemantic_clspvreflection
    {IR(53349, 22), 120}, // ArgumentStorageBuffer in nonsemantic_clspvreflection
    {IR(53476, 21), 126}, // ArgumentStorageImage in nonsemantic_clspvreflection
    {IR(54153, 27), 151}, // ArgumentStorageTexelBuffer in nonsemantic_clspvreflection
    {IR(53371, 16), 121}, // ArgumentUniform in nonsemantic_clspvreflection
    {IR(54180, 27), 152}, // ArgumentUniformTexelBuffer in nonsemantic_clspvreflection
    {IR(53513, 18), 128}, // ArgumentWorkgroup in nonsemantic_clspvreflection
    {IR(54207, 32), 153}, // ConstantDataPointerPushConstant in nonsemantic_clspvreflection
    {IR(53761, 26), 138}, // ConstantDataStorageBuffer in nonsemantic_clspvreflection
    {IR(53787, 20), 139}, // ConstantDataUniform in nonsemantic_clspvreflection
    {IR(54031, 45), 148}, // ImageArgumentInfoChannelDataTypePushConstant in nonsemantic_clspvreflection
    {IR(54113, 40), 150}, // ImageArgumentInfoChannelDataTypeUniform in nonsemantic_clspvreflection
    {IR(53989, 42), 147}, // ImageArgumentInfoChannelOrderPushConstant in nonsemantic_clspvreflection
    {IR(54076, 37), 149}, // ImageArgumentInfoChannelOrderUniform in nonsemantic_clspvreflection
    {IR(5805, 7), 118}, // Kernel in nonsemantic_clspvreflection
    {IR(9279, 15), 140}, // LiteralSampler in nonsemantic_clspvreflection
    {IR(54348, 34), 158}, // NormalizedSamplerMaskPushConstant in nonsemantic_clspvreflection
    {IR(54316, 32), 157}, // PrintfBufferPointerPushConstant in nonsemantic_clspvreflection
    {IR(54290, 26), 156}, // PrintfBufferStorageBuffer in nonsemantic_clspvreflection
    {IR(54279, 11), 155}, // PrintfInfo in nonsemantic_clspvreflection
    {IR(54239, 40), 154}, // ProgramScopeVariablePointerPushConstant in nonsemantic_clspvreflection
    {IR(53951, 38), 146}, // ProgramScopeVariablePointerRelocation in nonsemantic_clspvreflection
    {IR(53916, 35), 145}, // ProgramScopeVariablesStorageBuffer in nonsemantic_clspvreflection
    {IR(53807, 30), 141}, // PropertyRequiredWorkgroupSize in nonsemantic_clspvreflection
    {IR(53627, 30), 133}, // PushConstantEnqueuedLocalSize in nonsemantic_clspvreflection
    {IR(53602, 25), 132}, // PushConstantGlobalOffset in nonsemantic_clspvreflection
    {IR(53657, 23), 134}, // PushConstantGlobalSize in nonsemantic_clspvreflection
    {IR(53705, 26), 136}, // PushConstantNumWorkgroups in nonsemantic_clspvreflection
    {IR(53731, 30), 137}, // PushConstantRegionGroupOffset in nonsemantic_clspvreflection
    {IR(53680, 25), 135}, // PushConstantRegionOffset in nonsemantic_clspvreflection
    {IR(53557, 25), 130}, // SpecConstantGlobalOffset in nonsemantic_clspvreflection
    {IR(53837, 28), 142}, // SpecConstantSubgroupMaxSize in nonsemantic_clspvreflection
    {IR(53582, 20), 131}, // SpecConstantWorkDim in nonsemantic_clspvreflection
    {IR(53531, 26), 129}, // SpecConstantWorkgroupSize in nonsemantic_clspvreflection
    {IR(54382, 22), 159}, // WorkgroupVariableSize in nonsemantic_clspvreflection
    {IR(54503, 21), 200}, // DebugBuildIdentifier in nonsemantic_shader_debuginfo
    {IR(51820, 21), 161}, // DebugCompilationUnit in nonsemantic_shader_debuginfo
    {IR(52547, 13), 188}, // DebugDeclare in nonsemantic_shader_debuginfo
    {IR(54541, 16), 202}, // DebugEntryPoint in nonsemantic_shader_debuginfo
    {IR(52619, 16), 191}, // DebugExpression in nonsemantic_shader_debuginfo
    {IR(52363, 14), 180}, // DebugFunction in nonsemantic_shader_debuginfo
    {IR(52338, 25), 179}, // DebugFunctionDeclaration in nonsemantic_shader_debuginfo
    {IR(54436, 24), 196}, // DebugFunctionDefinition in nonsemantic_shader_debuginfo
    {IR(52318, 20), 178}, // DebugGlobalVariable in nonsemantic_shader_debuginfo
    {IR(54404, 20), 194}, // DebugImportedEntity in nonsemantic_shader_debuginfo
    {IR(51806, 14), 160}, // DebugInfoNone in nonsemantic_shader_debuginfo
    {IR(52450, 15), 185}, // DebugInlinedAt in nonsemantic_shader_debuginfo
    {IR(52526, 21), 187}, // DebugInlinedVariable in nonsemantic_shader_debuginfo
    {IR(52377, 18), 181}, // DebugLexicalBlock in nonsemantic_shader_debuginfo
    {IR(52395, 31), 182}, // DebugLexicalBlockDiscriminator in nonsemantic_shader_debuginfo
    {IR(54481, 10), 198}, // DebugLine in nonsemantic_shader_debuginfo
    {IR(52507, 19), 186}, // DebugLocalVariable in nonsemantic_shader_debuginfo
    {IR(52635, 14), 192}, // DebugMacroDef in nonsemantic_shader_debuginfo
    {IR(52649, 16), 193}, // DebugMacroUndef in nonsemantic_shader_debuginfo
    {IR(54491, 12), 199}, // DebugNoLine in nonsemantic_shader_debuginfo
    {IR(52437, 13), 184}, // DebugNoScope in nonsemantic_shader_debuginfo
    {IR(52604, 15), 190}, // DebugOperation in nonsemantic_shader_debuginfo
    {IR(52426, 11), 183}, // DebugScope in nonsemantic_shader_debuginfo
    {IR(54424, 12), 195}, // DebugSource in nonsemantic_shader_debuginfo
    {IR(54460, 21), 197}, // DebugSourceContinued in nonsemantic_shader_debuginfo
    {IR(54524, 17), 201}, // DebugStoragePath in nonsemantic_shader_debuginfo
    {IR(52016, 15), 165}, // DebugTypeArray in nonsemantic_shader_debuginfo
    {IR(51893, 15), 162}, // DebugTypeBasic in nonsemantic_shader_debuginfo
    {IR(52130, 19), 170}, // DebugTypeComposite in nonsemantic_shader_debuginfo
    {IR(54594, 30), 205}, // DebugTypeCooperativeMatrixKHR in nonsemantic_shader_debuginfo
    {IR(52078, 14), 169}, // DebugTypeEnum in nonsemantic_shader_debuginfo
    {IR(52060, 18), 168}, // DebugTypeFunction in nonsemantic_shader_debuginfo
    {IR(52165, 21), 172}, // DebugTypeInheritance in nonsemantic_shader_debuginfo
    {IR(54557, 16), 203}, // DebugTypeMatrix in nonsemantic_shader_debuginfo
    {IR(52149, 16), 171}, // DebugTypeMember in nonsemantic_shader_debuginfo
    {IR(51942, 17), 163}, // DebugTypePointer in nonsemantic_shader_debuginfo
    {IR(52186, 21), 173}, // DebugTypePtrToMember in nonsemantic_shader_debuginfo
    {IR(51997, 19), 164}, // DebugTypeQualifier in nonsemantic_shader_debuginfo
    {IR(52207, 18), 174}, // DebugTypeTemplate in nonsemantic_shader_debuginfo
    {IR(52225, 27), 175}, // DebugTypeTemplateParameter in nonsemantic_shader_debuginfo
    {IR(52287, 31), 177}, // DebugTypeTemplateParameterPack in nonsemantic_shader_debuginfo
    {IR(52252, 35), 176}, // DebugTypeTemplateTemplateParameter in nonsemantic_shader_debuginfo
    {IR(52031, 16), 166}, // DebugTypeVector in nonsemantic_shader_debuginfo
    {IR(54573, 21), 204}, // DebugTypeVectorIdEXT in nonsemantic_shader_debuginfo
    {IR(52047, 13), 167}, // DebugTypedef in nonsemantic_shader_debuginfo
    {IR(52560, 11), 189}, // DebugValue in nonsemantic_shader_debuginfo
    {IR(54624, 14), 206}, // Configuration in nonsemantic_vkspreflection
    {IR(54700, 20), 211}, // DescriptorSetBuffer in nonsemantic_vkspreflection
    {IR(54720, 19), 212}, // DescriptorSetImage in nonsemantic_vkspreflection
    {IR(54739, 21), 213}, // DescriptorSetSampler in nonsemantic_vkspreflection
    {IR(54663, 14), 209}, // PushConstants in nonsemantic_vkspreflection
    {IR(54677, 23), 210}, // SpecializationMapEntry in nonsemantic_vkspreflection
    {IR(54638, 13), 207}, // StartCounter in nonsemantic_vkspreflection
    {IR(54651, 12), 208}, // StopCounter in nonsemantic_vkspreflection
    {IR(51820, 21), 215}, // DebugCompilationUnit in opencl_debuginfo_100
    {IR(52547, 13), 242}, // DebugDeclare in opencl_debuginfo_100
    {IR(52619, 16), 245}, // DebugExpression in opencl_debuginfo_100
    {IR(52363, 14), 234}, // DebugFunction in opencl_debuginfo_100
    {IR(52338, 25), 233}, // DebugFunctionDeclaration in opencl_debuginfo_100
    {IR(52318, 20), 232}, // DebugGlobalVariable in opencl_debuginfo_100
    {IR(54404, 20), 248}, // DebugImportedEntity in opencl_debuginfo_100
    {IR(51806, 14), 214}, // DebugInfoNone in opencl_debuginfo_100
    {IR(52450, 15), 239}, // DebugInlinedAt in opencl_debuginfo_100
    {IR(52526, 21), 241}, // DebugInlinedVariable in opencl_debuginfo_100
    {IR(52377, 18), 235}, // DebugLexicalBlock in opencl_debuginfo_100
    {IR(52395, 31), 236}, // DebugLexicalBlockDiscriminator in opencl_debuginfo_100
    {IR(52507, 19), 240}, // DebugLocalVariable in opencl_debuginfo_100
    {IR(52635, 14), 246}, // DebugMacroDef in opencl_debuginfo_100
    {IR(52649, 16), 247}, // DebugMacroUndef in opencl_debuginfo_100
    {IR(55060, 17), 250}, // DebugModuleINTEL in opencl_debuginfo_100
    {IR(52437, 13), 238}, // DebugNoScope in opencl_debuginfo_100
    {IR(52604, 15), 244}, // DebugOperation in opencl_debuginfo_100
    {IR(52426, 11), 237}, // DebugScope in opencl_debuginfo_100
    {IR(54424, 12), 249}, // DebugSource in opencl_debuginfo_100
    {IR(52016, 15), 219}, // DebugTypeArray in opencl_debuginfo_100
    {IR(51893, 15), 216}, // DebugTypeBasic in opencl_debuginfo_100
    {IR(52130, 19), 224}, // DebugTypeComposite in opencl_debuginfo_100
    {IR(52078, 14), 223}, // DebugTypeEnum in opencl_debuginfo_100
    {IR(52060, 18), 222}, // DebugTypeFunction in opencl_debuginfo_100
    {IR(52165, 21), 226}, // DebugTypeInheritance in opencl_debuginfo_100
    {IR(52149, 16), 225}, // DebugTypeMember in opencl_debuginfo_100
    {IR(51942, 17), 217}, // DebugTypePointer in opencl_debuginfo_100
    {IR(52186, 21), 227}, // DebugTypePtrToMember in opencl_debuginfo_100
    {IR(51997, 19), 218}, // DebugTypeQualifier in opencl_debuginfo_100
    {IR(52207, 18), 228}, // DebugTypeTemplate in opencl_debuginfo_100
    {IR(52225, 27), 229}, // DebugTypeTemplateParameter in opencl_debuginfo_100
    {IR(52287, 31), 231}, // DebugTypeTemplateParameterPack in opencl_debuginfo_100
    {IR(52252, 35), 230}, // DebugTypeTemplateTemplateParameter in opencl_debuginfo_100
    {IR(52031, 16), 220}, // DebugTypeVector in opencl_debuginfo_100
    {IR(52047, 13), 221}, // DebugTypedef in opencl_debuginfo_100
    {IR(52560, 11), 243}, // DebugValue in opencl_debuginfo_100
    {IR(55077, 5), 251}, // acos in opencl_std_100
    {IR(55082, 6), 252}, // acosh in opencl_std_100
    {IR(55088, 7), 253}, // acospi in opencl_std_100
    {IR(55095, 5), 254}, // asin in opencl_std_100
    {IR(55100, 6), 255}, // asinh in opencl_std_100
    {IR(55106, 7), 256}, // asinpi in opencl_std_100
    {IR(55113, 5), 257}, // atan in opencl_std_100
    {IR(55118, 6), 258}, // atan2 in opencl_std_100
    {IR(55137, 8), 261}, // atan2pi in opencl_std_100
    {IR(55124, 6), 259}, // atanh in opencl_std_100
    {IR(55130, 7), 260}, // atanpi in opencl_std_100
    {IR(56328, 10), 407}, // bitselect in opencl_std_100
    {IR(55145, 5), 262}, // cbrt in opencl_std_100
    {IR(55150, 5), 263}, // ceil in opencl_std_100
    {IR(56000, 4), 372}, // clz in opencl_std_100
    {IR(55155, 9), 264}, // copysign in opencl_std_100
    {IR(55164, 4), 265}, // cos in opencl_std_100
    {IR(55168, 5), 266}, // cosh in opencl_std_100
    {IR(55173, 6), 267}, // cospi in opencl_std_100
    {IR(55844, 6), 355}, // cross in opencl_std_100
    {IR(56004, 4), 373}, // ctz in opencl_std_100
    {IR(55779, 8), 347}, // degrees in opencl_std_100
    {IR(55850, 9), 356}, // distance in opencl_std_100
    {IR(55184, 4), 269}, // erf in opencl_std_100
    {IR(55179, 5), 268}, // erfc in opencl_std_100
    {IR(55188, 4), 270}, // exp in opencl_std_100
    {IR(55197, 6), 272}, // exp10 in opencl_std_100
    {IR(55192, 5), 271}, // exp2 in opencl_std_100
    {IR(55203, 6), 273}, // expm1 in opencl_std_100
    {IR(55209, 5), 274}, // fabs in opencl_std_100
    {IR(55876, 14), 359}, // fast_distance in opencl_std_100
    {IR(55890, 12), 360}, // fast_length in opencl_std_100
    {IR(55902, 15), 361}, // fast_normalize in opencl_std_100
    {IR(55772, 7), 346}, // fclamp in opencl_std_100
    {IR(55214, 5), 275}, // fdim in opencl_std_100
    {IR(55219, 6), 276}, // floor in opencl_std_100
    {IR(55225, 4), 277}, // fma in opencl_std_100
    {IR(55229, 5), 278}, // fmax in opencl_std_100
    {IR(55787, 12), 348}, // fmax_common in opencl_std_100
    {IR(55234, 5), 279}, // fmin in opencl_std_100
    {IR(55799, 12), 349}, // fmin_common in opencl_std_100
    {IR(55239, 5), 280}, // fmod in opencl_std_100
    {IR(55244, 6), 281}, // fract in opencl_std_100
    {IR(55250, 6), 282}, // frexp in opencl_std_100
    {IR(55462, 9), 318}, // half_cos in opencl_std_100
    {IR(55471, 12), 319}, // half_divide in opencl_std_100
    {IR(55483, 9), 320}, // half_exp in opencl_std_100
    {IR(55502, 11), 322}, // half_exp10 in opencl_std_100
    {IR(55492, 10), 321}, // half_exp2 in opencl_std_100
    {IR(55513, 9), 323}, // half_log in opencl_std_100
    {IR(55532, 11), 325}, // half_log10 in opencl_std_100
    {IR(55522, 10), 324}, // half_log2 in opencl_std_100
    {IR(55543, 10), 326}, // half_powr in opencl_std_100
    {IR(55553, 11), 327}, // half_recip in opencl_std_100
    {IR(55564, 11), 328}, // half_rsqrt in opencl_std_100
    {IR(55575, 9), 329}, // half_sin in opencl_std_100
    {IR(55584, 10), 330}, // half_sqrt in opencl_std_100
    {IR(55594, 9), 331}, // half_tan in opencl_std_100
    {IR(55256, 6), 283}, // hypot in opencl_std_100
    {IR(55262, 6), 284}, // ilogb in opencl_std_100
    {IR(55268, 6), 285}, // ldexp in opencl_std_100
    {IR(55859, 7), 357}, // length in opencl_std_100
    {IR(55274, 7), 286}, // lgamma in opencl_std_100
    {IR(55281, 9), 287}, // lgamma_r in opencl_std_100
    {IR(55290, 4), 288}, // log in opencl_std_100
    {IR(55299, 6), 290}, // log10 in opencl_std_100
    {IR(55305, 6), 291}, // log1p in opencl_std_100
    {IR(55294, 5), 289}, // log2 in opencl_std_100
    {IR(55311, 5), 292}, // logb in opencl_std_100
    {IR(55316, 4), 293}, // mad in opencl_std_100
    {IR(55320, 7), 294}, // maxmag in opencl_std_100
    {IR(55327, 7), 295}, // minmag in opencl_std_100
    {IR(55811, 4), 350}, // mix in opencl_std_100
    {IR(55334, 5), 296}, // modf in opencl_std_100
    {IR(55339, 4), 297}, // nan in opencl_std_100
    {IR(55603, 11), 332}, // native_cos in opencl_std_100
    {IR(55614, 14), 333}, // native_divide in opencl_std_100
    {IR(55628, 11), 334}, // native_exp in opencl_std_100
    {IR(55651, 13), 336}, // native_exp10 in opencl_std_100
    {IR(55639, 12), 335}, // native_exp2 in opencl_std_100
    {IR(55664, 11), 337}, // native_log in opencl_std_100
    {IR(55687, 13), 339}, // native_log10 in opencl_std_100
    {IR(55675, 12), 338}, // native_log2 in opencl_std_100
    {IR(55700, 12), 340}, // native_powr in opencl_std_100
    {IR(55712, 13), 341}, // native_recip in opencl_std_100
    {IR(55725, 13), 342}, // native_rsqrt in opencl_std_100
    {IR(55738, 11), 343}, // native_sin in opencl_std_100
    {IR(55749, 12), 344}, // native_sqrt in opencl_std_100
    {IR(55761, 11), 345}, // native_tan in opencl_std_100
    {IR(55343, 10), 298}, // nextafter in opencl_std_100
    {IR(55866, 10), 358}, // normalize in opencl_std_100
    {IR(56119, 9), 387}, // popcount in opencl_std_100
    {IR(55353, 4), 299}, // pow in opencl_std_100
    {IR(55357, 5), 300}, // pown in opencl_std_100
    {IR(55362, 5), 301}, // powr in opencl_std_100
    {IR(56319, 9), 406}, // prefetch in opencl_std_100
    {IR(56312, 7), 405}, // printf in opencl_std_100
    {IR(55815, 8), 351}, // radians in opencl_std_100
    {IR(55367, 10), 302}, // remainder in opencl_std_100
    {IR(55377, 7), 303}, // remquo in opencl_std_100
    {IR(55384, 5), 304}, // rint in opencl_std_100
    {IR(55389, 6), 305}, // rootn in opencl_std_100
    {IR(56070, 7), 382}, // rotate in opencl_std_100
    {IR(55395, 6), 306}, // round in opencl_std_100
    {IR(55401, 6), 307}, // rsqrt in opencl_std_100
    {IR(55917, 6), 362}, // s_abs in opencl_std_100
    {IR(55923, 11), 363}, // s_abs_diff in opencl_std_100
    {IR(55934, 10), 364}, // s_add_sat in opencl_std_100
    {IR(55984, 8), 370}, // s_clamp in opencl_std_100
    {IR(55954, 7), 366}, // s_hadd in opencl_std_100
    {IR(56128, 8), 388}, // s_mad24 in opencl_std_100
    {IR(56008, 9), 374}, // s_mad_hi in opencl_std_100
    {IR(56027, 10), 376}, // s_mad_sat in opencl_std_100
    {IR(56037, 6), 377}, // s_max in opencl_std_100
    {IR(56049, 6), 379}, // s_min in opencl_std_100
    {IR(56144, 8), 390}, // s_mul24 in opencl_std_100
    {IR(56061, 9), 381}, // s_mul_hi in opencl_std_100
    {IR(55968, 8), 368}, // s_rhadd in opencl_std_100
    {IR(56077, 10), 383}, // s_sub_sat in opencl_std_100
    {IR(56108, 11), 386}, // s_upsample in opencl_std_100
    {IR(56338, 7), 408}, // select in opencl_std_100
    {IR(56295, 8), 403}, // shuffle in opencl_std_100
    {IR(56303, 9), 404}, // shuffle2 in opencl_std_100
    {IR(55839, 5), 354}, // sign in opencl_std_100
    {IR(55407, 4), 308}, // sin in opencl_std_100
    {IR(55411, 7), 309}, // sincos in opencl_std_100
    {IR(55418, 5), 310}, // sinh in opencl_std_100
    {IR(55423, 6), 311}, // sinpi in opencl_std_100
    {IR(55828, 11), 353}, // smoothstep in opencl_std_100
    {IR(55429, 5), 312}, // sqrt in opencl_std_100
    {IR(55823, 5), 352}, // step in opencl_std_100
    {IR(55434, 4), 313}, // tan in opencl_std_100
    {IR(55438, 5), 314}, // tanh in opencl_std_100
    {IR(55443, 6), 315}, // tanpi in opencl_std_100
    {IR(55449, 7), 316}, // tgamma in opencl_std_100
    {IR(55456, 6), 317}, // trunc in opencl_std_100
    {IR(56345, 6), 409}, // u_abs in opencl_std_100
    {IR(56351, 11), 410}, // u_abs_diff in opencl_std_100
    {IR(55944, 10), 365}, // u_add_sat in opencl_std_100
    {IR(55992, 8), 371}, // u_clamp in opencl_std_100
    {IR(55961, 7), 367}, // u_hadd in opencl_std_100
    {IR(56136, 8), 389}, // u_mad24 in opencl_std_100
    {IR(56371, 9), 412}, // u_mad_hi in opencl_std_100
    {IR(56017, 10), 375}, // u_mad_sat in opencl_std_100
    {IR(56043, 6), 378}, // u_max in opencl_std_100
    {IR(56055, 6), 380}, // u_min in opencl_std_100
    {IR(56152, 8), 391}, // u_mul24 in opencl_std_100
    {IR(56362, 9), 411}, // u_mul_hi in opencl_std_100
    {IR(55976, 8), 369}, // u_rhadd in opencl_std_100
    {IR(56087, 10), 384}, // u_sub_sat in opencl_std_100
    {IR(56097, 11), 385}, // u_upsample in opencl_std_100
    {IR(56175, 11), 394}, // vload_half in opencl_std_100
    {IR(56186, 12), 395}, // vload_halfn in opencl_std_100
    {IR(56252, 13), 400}, // vloada_halfn in opencl_std_100
    {IR(56160, 7), 392}, // vloadn in opencl_std_100
    {IR(56198, 12), 396}, // vstore_half in opencl_std_100
    {IR(56210, 14), 397}, // vstore_half_r in opencl_std_100
    {IR(56224, 13), 398}, // vstore_halfn in opencl_std_100
    {IR(56237, 15), 399}, // vstore_halfn_r in opencl_std_100
    {IR(56265, 14), 401}, // vstorea_halfn in opencl_std_100
    {IR(56279, 16), 402}, // vstorea_halfn_r in opencl_std_100
    {IR(56167, 8), 393}, // vstoren in opencl_std_100
    {IR(56397, 17), 414}, // CubeFaceCoordAMD in spv_amd_gcn_shader
    {IR(56380, 17), 413}, // CubeFaceIndexAMD in spv_amd_gcn_shader
    {IR(56414, 8), 415}, // TimeAMD in spv_amd_gcn_shader
    {IR(56491, 9), 419}, // MbcntAMD in spv_amd_shader_ballot
    {IR(56422, 22), 416}, // SwizzleInvocationsAMD in spv_amd_shader_ballot
    {IR(56444, 28), 417}, // SwizzleInvocationsMaskedAMD in spv_amd_shader_ballot
    {IR(56472, 19), 418}, // WriteInvocationAMD in spv_amd_shader_ballot
    {IR(56500, 23), 420}, // InterpolateAtVertexAMD in spv_amd_shader_explicit_vertex_parameter
    {IR(56550, 9), 424}, // FMax3AMD in spv_amd_shader_trinary_minmax
    {IR(56577, 9), 427}, // FMid3AMD in spv_amd_shader_trinary_minmax
    {IR(56523, 9), 421}, // FMin3AMD in spv_amd_shader_trinary_minmax
    {IR(56568, 9), 426}, // SMax3AMD in spv_amd_shader_trinary_minmax
    {IR(56595, 9), 429}, // SMid3AMD in spv_amd_shader_trinary_minmax
    {IR(56541, 9), 423}, // SMin3AMD in spv_amd_shader_trinary_minmax
    {IR(56559, 9), 425}, // UMax3AMD in spv_amd_shader_trinary_minmax
    {IR(56586, 9), 428}, // UMid3AMD in spv_amd_shader_trinary_minmax
    {IR(56532, 9), 422}, // UMin3AMD in spv_amd_shader_trinary_minmax
    {IR(56901, 4), 461}, // ABS in tosa_001000_1
    {IR(56724, 4), 444}, // ADD in tosa_001000_1
    {IR(56604, 7), 430}, // ARGMAX in tosa_001000_1
    {IR(56728, 23), 445}, // ARITHMETIC_RIGHT_SHIFT in tosa_001000_1
    {IR(56611, 11), 431}, // AVG_POOL2D in tosa_001000_1
    {IR(56751, 12), 446}, // BITWISE_AND in tosa_001000_1
    {IR(56905, 12), 462}, // BITWISE_NOT in tosa_001000_1
    {IR(56763, 11), 447}, // BITWISE_OR in tosa_001000_1
    {IR(56774, 12), 448}, // BITWISE_XOR in tosa_001000_1
    {IR(57159, 5), 494}, // CAST in tosa_001000_1
    {IR(56917, 5), 463}, // CEIL in tosa_001000_1
    {IR(56701, 6), 440}, // CLAMP in tosa_001000_1
    {IR(56922, 4), 464}, // CLZ in tosa_001000_1
    {IR(57089, 7), 484}, // CONCAT in tosa_001000_1
    {IR(56622, 7), 432}, // CONV2D in tosa_001000_1
    {IR(56629, 7), 433}, // CONV3D in tosa_001000_1
    {IR(56926, 4), 465}, // COS in tosa_001000_1
    {IR(56636, 17), 434}, // DEPTHWISE_CONV2D in tosa_001000_1
    {IR(56991, 6), 475}, // EQUAL in tosa_001000_1
    {IR(56707, 4), 441}, // ERF in tosa_001000_1
    {IR(56930, 4), 466}, // EXP in tosa_001000_1
    {IR(56653, 6), 435}, // FFT2D in tosa_001000_1
    {IR(56934, 6), 467}, // FLOOR in tosa_001000_1
    {IR(57137, 7), 491}, // GATHER in tosa_001000_1
    {IR(56997, 8), 476}, // GREATER in tosa_001000_1
    {IR(57005, 14), 477}, // GREATER_EQUAL in tosa_001000_1
    {IR(56786, 7), 449}, // INTDIV in tosa_001000_1
    {IR(56940, 4), 468}, // LOG in tosa_001000_1
    {IR(56793, 12), 450}, // LOGICAL_AND in tosa_001000_1
    {IR(56805, 19), 451}, // LOGICAL_LEFT_SHIFT in tosa_001000_1
    {IR(56944, 12), 469}, // LOGICAL_NOT in tosa_001000_1
    {IR(56844, 11), 453}, // LOGICAL_OR in tosa_001000_1
    {IR(56824, 20), 452}, // LOGICAL_RIGHT_SHIFT in tosa_001000_1
    {IR(56855, 12), 454}, // LOGICAL_XOR in tosa_001000_1
    {IR(56659, 7), 436}, // MATMUL in tosa_001000_1
    {IR(56867, 8), 455}, // MAXIMUM in tosa_001000_1
    {IR(56666, 11), 437}, // MAX_POOL2D in tosa_001000_1
    {IR(56875, 8), 456}, // MINIMUM in tosa_001000_1
    {IR(56883, 4), 457}, // MUL in tosa_001000_1
    {IR(56956, 7), 470}, // NEGATE in tosa_001000_1
    {IR(57096, 4), 485}, // PAD in tosa_001000_1
    {IR(56887, 4), 458}, // POW in tosa_001000_1
    {IR(56963, 11), 471}, // RECIPROCAL in tosa_001000_1
    {IR(57019, 11), 478}, // REDUCE_ALL in tosa_001000_1
    {IR(57030, 11), 479}, // REDUCE_ANY in tosa_001000_1
    {IR(57041, 11), 480}, // REDUCE_MAX in tosa_001000_1
    {IR(57052, 11), 481}, // REDUCE_MIN in tosa_001000_1
    {IR(57063, 15), 482}, // REDUCE_PRODUCT in tosa_001000_1
    {IR(57078, 11), 483}, // REDUCE_SUM in tosa_001000_1
    {IR(57164, 8), 495}, // RESCALE in tosa_001000_1
    {IR(57100, 8), 486}, // RESHAPE in tosa_001000_1
    {IR(57152, 7), 493}, // RESIZE in tosa_001000_1
    {IR(57108, 8), 487}, // REVERSE in tosa_001000_1
    {IR(56677, 7), 438}, // RFFT2D in tosa_001000_1
    {IR(56974, 6), 472}, // RSQRT in tosa_001000_1
    {IR(57144, 8), 492}, // SCATTER in tosa_001000_1
    {IR(56984, 7), 474}, // SELECT in tosa_001000_1
    {IR(56711, 8), 442}, // SIGMOID in tosa_001000_1
    {IR(56980, 4), 473}, // SIN in tosa_001000_1
    {IR(57116, 6), 488}, // SLICE in tosa_001000_1
    {IR(56891, 4), 459}, // SUB in tosa_001000_1
    {IR(56895, 6), 460}, // TABLE in tosa_001000_1
    {IR(56719, 5), 443}, // TANH in tosa_001000_1
    {IR(57122, 5), 489}, // TILE in tosa_001000_1
    {IR(57127, 10), 490}, // TRANSPOSE in tosa_001000_1
    {IR(56684, 17), 439}, // TRANSPOSE_CONV2D in tosa_001000_1
}};

IndexRange ExtInstNameRangeForKind(spv_ext_inst_type_t type) {
  switch(type) {
    case SPV_EXT_INST_TYPE_ARM_MOTION_ENGINE_100: return IR(0, 3);
    case SPV_EXT_INST_TYPE_DEBUGINFO: return IR(3, 34);
    case SPV_EXT_INST_TYPE_GLSL_STD_450: return IR(37, 81);
    case SPV_EXT_INST_TYPE_NONSEMANTIC_CLSPVREFLECTION: return IR(118, 42);
    case SPV_EXT_INST_TYPE_NONSEMANTIC_SHADER_DEBUGINFO_100: return IR(160, 46);
    case SPV_EXT_INST_TYPE_NONSEMANTIC_VKSPREFLECTION: return IR(206, 8);
    case SPV_EXT_INST_TYPE_OPENCL_DEBUGINFO_100: return IR(214, 37);
    case SPV_EXT_INST_TYPE_OPENCL_STD: return IR(251, 162);
    case SPV_EXT_INST_TYPE_SPV_AMD_GCN_SHADER: return IR(413, 3);
    case SPV_EXT_INST_TYPE_SPV_AMD_SHADER_BALLOT: return IR(416, 4);
    case SPV_EXT_INST_TYPE_SPV_AMD_SHADER_EXPLICIT_VERTEX_PARAMETER: return IR(420, 1);
    case SPV_EXT_INST_TYPE_SPV_AMD_SHADER_TRINARY_MINMAX: return IR(421, 9);
    case SPV_EXT_INST_TYPE_TOSA_001000_1: return IR(430, 66);
    default: break;
  }
  return IR(0,0);
}

// Array of characters, referenced by IndexRanges elsewhere.
// Each IndexRange denotes a string.
static const char kStrings[] =
  "SPV_ALTERA_arbitrary_precision_fixed_point\0" // IR(0, 43)
  "SPV_ALTERA_arbitrary_precision_floating_point\0" // IR(43, 46)
  "SPV_ALTERA_arbitrary_precision_integers\0" // IR(89, 40)
  "SPV_ALTERA_blocking_pipes\0" // IR(129, 26)
  "SPV_ALTERA_fpga_argument_interfaces\0" // IR(155, 36)
  "SPV_ALTERA_fpga_buffer_location\0" // IR(191, 32)
  "SPV_ALTERA_fpga_cluster_attributes\0" // IR(223, 35)
  "SPV_ALTERA_fpga_dsp_control\0" // IR(258, 28)
  "SPV_ALTERA_fpga_invocation_pipelining_attributes\0" // IR(286, 49)
  "SPV_ALTERA_fpga_latency_control\0" // IR(335, 32)
  "SPV_ALTERA_fpga_loop_controls\0" // IR(367, 30)
  "SPV_ALTERA_fpga_memory_accesses\0" // IR(397, 32)
  "SPV_ALTERA_fpga_memory_attributes\0" // IR(429, 34)
  "SPV_ALTERA_fpga_reg\0" // IR(463, 20)
  "SPV_ALTERA_global_variable_fpga_decorations\0" // IR(483, 44)
  "SPV_ALTERA_io_pipes\0" // IR(527, 20)
  "SPV_ALTERA_loop_fuse\0" // IR(547, 21)
  "SPV_ALTERA_runtime_aligned\0" // IR(568, 27)
  "SPV_ALTERA_task_sequence\0" // IR(595, 25)
  "SPV_ALTERA_usm_storage_classes\0" // IR(620, 31)
  "SPV_AMDX_shader_enqueue\0" // IR(651, 24)
  "SPV_AMD_gcn_shader\0" // IR(675, 19)
  "SPV_AMD_gpu_shader_half_float\0" // IR(694, 30)
  "SPV_AMD_gpu_shader_half_float_fetch\0" // IR(724, 36)
  "SPV_AMD_gpu_shader_int16\0" // IR(760, 25)
  "SPV_AMD_shader_ballot\0" // IR(785, 22)
  "SPV_AMD_shader_early_and_late_fragment_tests\0" // IR(807, 45)
  "SPV_AMD_shader_explicit_vertex_parameter\0" // IR(852, 41)
  "SPV_AMD_shader_fragment_mask\0" // IR(893, 29)
  "SPV_AMD_shader_image_load_store_lod\0" // IR(922, 36)
  "SPV_AMD_shader_trinary_minmax\0" // IR(958, 30)
  "SPV_AMD_texture_gather_bias_lod\0" // IR(988, 32)
  "SPV_AMD_weak_linkage\0" // IR(1020, 21)
  "SPV_ARM_cooperative_matrix_layouts\0" // IR(1041, 35)
  "SPV_ARM_core_builtins\0" // IR(1076, 22)
  "SPV_ARM_graph\0" // IR(1098, 14)
  "SPV_ARM_tensors\0" // IR(1112, 16)
  "SPV_EXT_arithmetic_fence\0" // IR(1128, 25)
  "SPV_EXT_demote_to_helper_invocation\0" // IR(1153, 36)
  "SPV_EXT_descriptor_heap\0" // IR(1189, 24)
  "SPV_EXT_descriptor_indexing\0" // IR(1213, 28)
  "SPV_EXT_float8\0" // IR(1241, 15)
  "SPV_EXT_fragment_fully_covered\0" // IR(1256, 31)
  "SPV_EXT_fragment_invocation_density\0" // IR(1287, 36)
  "SPV_EXT_fragment_shader_interlock\0" // IR(1323, 34)
  "SPV_EXT_long_vector\0" // IR(1357, 20)
  "SPV_EXT_mesh_shader\0" // IR(1377, 20)
  "SPV_EXT_ocp_microscaling_types\0" // IR(1397, 31)
  "SPV_EXT_opacity_micromap\0" // IR(1428, 25)
  "SPV_EXT_optnone\0" // IR(1453, 16)
  "SPV_EXT_physical_storage_buffer\0" // IR(1469, 32)
  "SPV_EXT_relaxed_printf_string_address_space\0" // IR(1501, 44)
  "SPV_EXT_replicated_composites\0" // IR(1545, 30)
  "SPV_EXT_shader_64bit_indexing\0" // IR(1575, 30)
  "SPV_EXT_shader_atomic_float16_add\0" // IR(1605, 34)
  "SPV_EXT_shader_atomic_float_add\0" // IR(1639, 32)
  "SPV_EXT_shader_atomic_float_min_max\0" // IR(1671, 36)
  "SPV_EXT_shader_image_int64\0" // IR(1707, 27)
  "SPV_EXT_shader_invocation_reorder\0" // IR(1734, 34)
  "SPV_EXT_shader_stencil_export\0" // IR(1768, 30)
  "SPV_EXT_shader_subgroup_partitioned\0" // IR(1798, 36)
  "SPV_EXT_shader_tile_image\0" // IR(1834, 26)
  "SPV_EXT_shader_viewport_index_layer\0" // IR(1860, 36)
  "SPV_EXT_split_barrier\0" // IR(1896, 22)
  "SPV_GOOGLE_decorate_string\0" // IR(1918, 27)
  "SPV_GOOGLE_hlsl_functionality1\0" // IR(1945, 31)
  "SPV_GOOGLE_user_type\0" // IR(1976, 21)
  "SPV_INTEL_2d_block_io\0" // IR(1997, 22)
  "SPV_INTEL_arbitrary_precision_fixed_point\0" // IR(2019, 42)
  "SPV_INTEL_arbitrary_precision_floating_point\0" // IR(2061, 45)
  "SPV_INTEL_arbitrary_precision_integers\0" // IR(2106, 39)
  "SPV_INTEL_bfloat16_conversion\0" // IR(2145, 30)
  "SPV_INTEL_bindless_images\0" // IR(2175, 26)
  "SPV_INTEL_blocking_pipes\0" // IR(2201, 25)
  "SPV_INTEL_cache_controls\0" // IR(2226, 25)
  "SPV_INTEL_debug_module\0" // IR(2251, 23)
  "SPV_INTEL_device_side_avc_motion_estimation\0" // IR(2274, 44)
  "SPV_INTEL_float_controls2\0" // IR(2318, 26)
  "SPV_INTEL_fp_fast_math_mode\0" // IR(2344, 28)
  "SPV_INTEL_fp_max_error\0" // IR(2372, 23)
  "SPV_INTEL_fpga_argument_interfaces\0" // IR(2395, 35)
  "SPV_INTEL_fpga_buffer_location\0" // IR(2430, 31)
  "SPV_INTEL_fpga_cluster_attributes\0" // IR(2461, 34)
  "SPV_INTEL_fpga_dsp_control\0" // IR(2495, 27)
  "SPV_INTEL_fpga_invocation_pipelining_attributes\0" // IR(2522, 48)
  "SPV_INTEL_fpga_latency_control\0" // IR(2570, 31)
  "SPV_INTEL_fpga_loop_controls\0" // IR(2601, 29)
  "SPV_INTEL_fpga_memory_accesses\0" // IR(2630, 31)
  "SPV_INTEL_fpga_memory_attributes\0" // IR(2661, 33)
  "SPV_INTEL_fpga_reg\0" // IR(2694, 19)
  "SPV_INTEL_function_pointers\0" // IR(2713, 28)
  "SPV_INTEL_function_variants\0" // IR(2741, 28)
  "SPV_INTEL_global_variable_fpga_decorations\0" // IR(2769, 43)
  "SPV_INTEL_global_variable_host_access\0" // IR(2812, 38)
  "SPV_INTEL_inline_assembly\0" // IR(2850, 26)
  "SPV_INTEL_int4\0" // IR(2876, 15)
  "SPV_INTEL_io_pipes\0" // IR(2891, 19)
  "SPV_INTEL_kernel_attributes\0" // IR(2910, 28)
  "SPV_INTEL_long_composites\0" // IR(2938, 26)
  "SPV_INTEL_loop_fuse\0" // IR(2964, 20)
  "SPV_INTEL_masked_gather_scatter\0" // IR(2984, 32)
  "SPV_INTEL_maximum_registers\0" // IR(3016, 28)
  "SPV_INTEL_media_block_io\0" // IR(3044, 25)
  "SPV_INTEL_memory_access_aliasing\0" // IR(3069, 33)
  "SPV_INTEL_optnone\0" // IR(3102, 18)
  "SPV_INTEL_predicated_io\0" // IR(3120, 24)
  "SPV_INTEL_rounded_divide_sqrt\0" // IR(3144, 30)
  "SPV_INTEL_runtime_aligned\0" // IR(3174, 26)
  "SPV_INTEL_shader_integer_functions2\0" // IR(3200, 36)
  "SPV_INTEL_split_barrier\0" // IR(3236, 24)
  "SPV_INTEL_subgroup_buffer_prefetch\0" // IR(3260, 35)
  "SPV_INTEL_subgroup_matrix_multiply_accumulate\0" // IR(3295, 46)
  "SPV_INTEL_subgroups\0" // IR(3341, 20)
  "SPV_INTEL_task_sequence\0" // IR(3361, 24)
  "SPV_INTEL_tensor_float32_conversion\0" // IR(3385, 36)
  "SPV_INTEL_ternary_bitwise_function\0" // IR(3421, 35)
  "SPV_INTEL_unstructured_loop_controls\0" // IR(3456, 37)
  "SPV_INTEL_usm_storage_classes\0" // IR(3493, 30)
  "SPV_INTEL_variable_length_array\0" // IR(3523, 32)
  "SPV_INTEL_vector_compute\0" // IR(3555, 25)
  "SPV_KHR_16bit_storage\0" // IR(3580, 22)
  "SPV_KHR_8bit_storage\0" // IR(3602, 21)
  "SPV_KHR_abort\0" // IR(3623, 14)
  "SPV_KHR_bfloat16\0" // IR(3637, 17)
  "SPV_KHR_bit_instructions\0" // IR(3654, 25)
  "SPV_KHR_compute_shader_derivatives\0" // IR(3679, 35)
  "SPV_KHR_constant_data\0" // IR(3714, 22)
  "SPV_KHR_cooperative_matrix\0" // IR(3736, 27)
  "SPV_KHR_device_group\0" // IR(3763, 21)
  "SPV_KHR_expect_assume\0" // IR(3784, 22)
  "SPV_KHR_float_controls\0" // IR(3806, 23)
  "SPV_KHR_float_controls2\0" // IR(3829, 24)
  "SPV_KHR_fma\0" // IR(3853, 12)
  "SPV_KHR_fragment_shader_barycentric\0" // IR(3865, 36)
  "SPV_KHR_fragment_shading_rate\0" // IR(3901, 30)
  "SPV_KHR_integer_dot_product\0" // IR(3931, 28)
  "SPV_KHR_linkonce_odr\0" // IR(3959, 21)
  "SPV_KHR_maximal_reconvergence\0" // IR(3980, 30)
  "SPV_KHR_multiview\0" // IR(4010, 18)
  "SPV_KHR_no_integer_wrap_decoration\0" // IR(4028, 35)
  "SPV_KHR_non_semantic_info\0" // IR(4063, 26)
  "SPV_KHR_opacity_micromap\0" // IR(4089, 25)
  "SPV_KHR_physical_storage_buffer\0" // IR(4114, 32)
  "SPV_KHR_poison_freeze\0" // IR(4146, 22)
  "SPV_KHR_post_depth_coverage\0" // IR(4168, 28)
  "SPV_KHR_quad_control\0" // IR(4196, 21)
  "SPV_KHR_ray_cull_mask\0" // IR(4217, 22)
  "SPV_KHR_ray_query\0" // IR(4239, 18)
  "SPV_KHR_ray_tracing\0" // IR(4257, 20)
  "SPV_KHR_ray_tracing_position_fetch\0" // IR(4277, 35)
  "SPV_KHR_relaxed_extended_instruction\0" // IR(4312, 37)
  "SPV_KHR_shader_atomic_counter_ops\0" // IR(4349, 34)
  "SPV_KHR_shader_ballot\0" // IR(4383, 22)
  "SPV_KHR_shader_clock\0" // IR(4405, 21)
  "SPV_KHR_shader_draw_parameters\0" // IR(4426, 31)
  "SPV_KHR_storage_buffer_storage_class\0" // IR(4457, 37)
  "SPV_KHR_subgroup_rotate\0" // IR(4494, 24)
  "SPV_KHR_subgroup_uniform_control_flow\0" // IR(4518, 38)
  "SPV_KHR_subgroup_vote\0" // IR(4556, 22)
  "SPV_KHR_terminate_invocation\0" // IR(4578, 29)
  "SPV_KHR_uniform_group_instructions\0" // IR(4607, 35)
  "SPV_KHR_untyped_pointers\0" // IR(4642, 25)
  "SPV_KHR_variable_pointers\0" // IR(4667, 26)
  "SPV_KHR_vulkan_memory_model\0" // IR(4693, 28)
  "SPV_KHR_workgroup_memory_explicit_layout\0" // IR(4721, 41)
  "SPV_NVX_multiview_per_view_attributes\0" // IR(4762, 38)
  "SPV_NV_bindless_texture\0" // IR(4800, 24)
  "SPV_NV_cluster_acceleration_structure\0" // IR(4824, 38)
  "SPV_NV_compute_shader_derivatives\0" // IR(4862, 34)
  "SPV_NV_cooperative_matrix\0" // IR(4896, 26)
  "SPV_NV_cooperative_matrix2\0" // IR(4922, 27)
  "SPV_NV_cooperative_matrix_decode_vector\0" // IR(4949, 40)
  "SPV_NV_cooperative_vector\0" // IR(4989, 26)
  "SPV_NV_displacement_micromap\0" // IR(5015, 29)
  "SPV_NV_fragment_shader_barycentric\0" // IR(5044, 35)
  "SPV_NV_geometry_shader_passthrough\0" // IR(5079, 35)
  "SPV_NV_linear_swept_spheres\0" // IR(5114, 28)
  "SPV_NV_mesh_shader\0" // IR(5142, 19)
  "SPV_NV_push_constant_bank\0" // IR(5161, 26)
  "SPV_NV_raw_access_chains\0" // IR(5187, 25)
  "SPV_NV_ray_tracing\0" // IR(5212, 19)
  "SPV_NV_ray_tracing_motion_blur\0" // IR(5231, 31)
  "SPV_NV_sample_mask_override_coverage\0" // IR(5262, 37)
  "SPV_NV_shader_atomic_fp16_vector\0" // IR(5299, 33)
  "SPV_NV_shader_image_footprint\0" // IR(5332, 30)
  "SPV_NV_shader_invocation_reorder\0" // IR(5362, 33)
  "SPV_NV_shader_sm_builtins\0" // IR(5395, 26)
  "SPV_NV_shader_subgroup_partitioned\0" // IR(5421, 35)
  "SPV_NV_shading_rate\0" // IR(5456, 20)
  "SPV_NV_stereo_view_rendering\0" // IR(5476, 29)
  "SPV_NV_tensor_addressing\0" // IR(5505, 25)
  "SPV_NV_viewport_array2\0" // IR(5530, 23)
  "SPV_QCOM_cooperative_matrix_conversion\0" // IR(5553, 39)
  "SPV_QCOM_image_processing\0" // IR(5592, 26)
  "SPV_QCOM_image_processing2\0" // IR(5618, 27)
  "SPV_QCOM_image_processing3\0" // IR(5645, 27)
  "SPV_QCOM_multiple_wait_queues\0" // IR(5672, 30)
  "SPV_QCOM_tile_shading\0" // IR(5702, 22)
  "SPV_VALIDATOR_ignore_type_decl_unique\0" // IR(5724, 38)
  "SPV_VALVE_mixed_float_dot_product\0" // IR(5762, 34)
  "ReadOnly\0" // IR(5796, 9)
  "Kernel\0" // IR(5805, 7)
  "WriteOnly\0" // IR(5812, 10)
  "ReadWrite\0" // IR(5822, 10)
  "Logical\0" // IR(5832, 8)
  "Physical32\0" // IR(5840, 11)
  "Addresses\0" // IR(5851, 10)
  "Physical64\0" // IR(5861, 11)
  "PhysicalStorageBuffer64\0" // IR(5872, 24)
  "PhysicalStorageBuffer64EXT\0" // IR(5896, 27)
  "PhysicalStorageBufferAddresses\0" // IR(5923, 31)
  "Position\0" // IR(5954, 9)
  "Shader\0" // IR(5963, 7)
  "PointSize\0" // IR(5970, 10)
  "ClipDistance\0" // IR(5980, 13)
  "CullDistance\0" // IR(5993, 13)
  "VertexId\0" // IR(6006, 9)
  "InstanceId\0" // IR(6015, 11)
  "PrimitiveId\0" // IR(6026, 12)
  "Geometry\0" // IR(6038, 9)
  "Tessellation\0" // IR(6047, 13)
  "RayTracingNV\0" // IR(6060, 13)
  "RayTracingKHR\0" // IR(6073, 14)
  "MeshShadingNV\0" // IR(6087, 14)
  "MeshShadingEXT\0" // IR(6101, 15)
  "InvocationId\0" // IR(6116, 13)
  "Layer\0" // IR(6129, 6)
  "ShaderLayer\0" // IR(6135, 12)
  "ShaderViewportIndexLayerEXT\0" // IR(6147, 28)
  "ViewportIndex\0" // IR(6175, 14)
  "MultiViewport\0" // IR(6189, 14)
  "ShaderViewportIndex\0" // IR(6203, 20)
  "TessLevelOuter\0" // IR(6223, 15)
  "TessLevelInner\0" // IR(6238, 15)
  "TessCoord\0" // IR(6253, 10)
  "PatchVertices\0" // IR(6263, 14)
  "FragCoord\0" // IR(6277, 10)
  "PointCoord\0" // IR(6287, 11)
  "FrontFacing\0" // IR(6298, 12)
  "SampleId\0" // IR(6310, 9)
  "SampleRateShading\0" // IR(6319, 18)
  "SamplePosition\0" // IR(6337, 15)
  "SampleMask\0" // IR(6352, 11)
  "FragDepth\0" // IR(6363, 10)
  "HelperInvocation\0" // IR(6373, 17)
  "NumWorkgroups\0" // IR(6390, 14)
  "WorkgroupSize\0" // IR(6404, 14)
  "WorkgroupId\0" // IR(6418, 12)
  "LocalInvocationId\0" // IR(6430, 18)
  "GlobalInvocationId\0" // IR(6448, 19)
  "LocalInvocationIndex\0" // IR(6467, 21)
  "WorkDim\0" // IR(6488, 8)
  "GlobalSize\0" // IR(6496, 11)
  "EnqueuedWorkgroupSize\0" // IR(6507, 22)
  "GlobalOffset\0" // IR(6529, 13)
  "GlobalLinearId\0" // IR(6542, 15)
  "SubgroupSize\0" // IR(6557, 13)
  "GroupNonUniform\0" // IR(6570, 16)
  "SubgroupBallotKHR\0" // IR(6586, 18)
  "SubgroupMaxSize\0" // IR(6604, 16)
  "NumSubgroups\0" // IR(6620, 13)
  "NumEnqueuedSubgroups\0" // IR(6633, 21)
  "SubgroupId\0" // IR(6654, 11)
  "SubgroupLocalInvocationId\0" // IR(6665, 26)
  "VertexIndex\0" // IR(6691, 12)
  "InstanceIndex\0" // IR(6703, 14)
  "CoreIDARM\0" // IR(6717, 10)
  "CoreBuiltinsARM\0" // IR(6727, 16)
  "CoreCountARM\0" // IR(6743, 13)
  "CoreMaxIDARM\0" // IR(6756, 13)
  "WarpIDARM\0" // IR(6769, 10)
  "WarpMaxIDARM\0" // IR(6779, 13)
  "SubgroupEqMask\0" // IR(6792, 15)
  "SubgroupEqMaskKHR\0" // IR(6807, 18)
  "GroupNonUniformBallot\0" // IR(6825, 22)
  "SubgroupGeMask\0" // IR(6847, 15)
  "SubgroupGeMaskKHR\0" // IR(6862, 18)
  "SubgroupGtMask\0" // IR(6880, 15)
  "SubgroupGtMaskKHR\0" // IR(6895, 18)
  "SubgroupLeMask\0" // IR(6913, 15)
  "SubgroupLeMaskKHR\0" // IR(6928, 18)
  "SubgroupLtMask\0" // IR(6946, 15)
  "SubgroupLtMaskKHR\0" // IR(6961, 18)
  "BaseVertex\0" // IR(6979, 11)
  "DrawParameters\0" // IR(6990, 15)
  "BaseInstance\0" // IR(7005, 13)
  "DrawIndex\0" // IR(7018, 10)
  "PrimitiveShadingRateKHR\0" // IR(7028, 24)
  "FragmentShadingRateKHR\0" // IR(7052, 23)
  "DeviceIndex\0" // IR(7075, 12)
  "DeviceGroup\0" // IR(7087, 12)
  "ViewIndex\0" // IR(7099, 10)
  "MultiView\0" // IR(7109, 10)
  "ShadingRateKHR\0" // IR(7119, 15)
  "TileOffsetQCOM\0" // IR(7134, 15)
  "TileShadingQCOM\0" // IR(7149, 16)
  "TileDimensionQCOM\0" // IR(7165, 18)
  "TileApronSizeQCOM\0" // IR(7183, 18)
  "BaryCoordNoPerspAMD\0" // IR(7201, 20)
  "BaryCoordNoPerspCentroidAMD\0" // IR(7221, 28)
  "BaryCoordNoPerspSampleAMD\0" // IR(7249, 26)
  "BaryCoordSmoothAMD\0" // IR(7275, 19)
  "BaryCoordSmoothCentroidAMD\0" // IR(7294, 27)
  "BaryCoordSmoothSampleAMD\0" // IR(7321, 25)
  "BaryCoordPullModelAMD\0" // IR(7346, 22)
  "FragStencilRefEXT\0" // IR(7368, 18)
  "StencilExportEXT\0" // IR(7386, 17)
  "RemainingRecursionLevelsAMDX\0" // IR(7403, 29)
  "ShaderEnqueueAMDX\0" // IR(7432, 18)
  "ShaderIndexAMDX\0" // IR(7450, 16)
  "SamplerHeapEXT\0" // IR(7466, 15)
  "DescriptorHeapEXT\0" // IR(7481, 18)
  "ResourceHeapEXT\0" // IR(7499, 16)
  "ViewportMaskNV\0" // IR(7515, 15)
  "ShaderViewportMaskNV\0" // IR(7530, 21)
  "SecondaryPositionNV\0" // IR(7551, 20)
  "ShaderStereoViewNV\0" // IR(7571, 19)
  "SecondaryViewportMaskNV\0" // IR(7590, 24)
  "PositionPerViewNV\0" // IR(7614, 18)
  "PerViewAttributesNV\0" // IR(7632, 20)
  "ViewportMaskPerViewNV\0" // IR(7652, 22)
  "FullyCoveredEXT\0" // IR(7674, 16)
  "FragmentFullyCoveredEXT\0" // IR(7690, 24)
  "TaskCountNV\0" // IR(7714, 12)
  "PrimitiveCountNV\0" // IR(7726, 17)
  "PrimitiveIndicesNV\0" // IR(7743, 19)
  "ClipDistancePerViewNV\0" // IR(7762, 22)
  "CullDistancePerViewNV\0" // IR(7784, 22)
  "LayerPerViewNV\0" // IR(7806, 15)
  "MeshViewCountNV\0" // IR(7821, 16)
  "MeshViewIndicesNV\0" // IR(7837, 18)
  "BaryCoordKHR\0" // IR(7855, 13)
  "BaryCoordNV\0" // IR(7868, 12)
  "FragmentBarycentricKHR\0" // IR(7880, 23)
  "BaryCoordNoPerspKHR\0" // IR(7903, 20)
  "BaryCoordNoPerspNV\0" // IR(7923, 19)
  "FragSizeEXT\0" // IR(7942, 12)
  "FragmentSizeNV\0" // IR(7954, 15)
  "FragmentDensityEXT\0" // IR(7969, 19)
  "FragInvocationCountEXT\0" // IR(7988, 23)
  "InvocationsPerPixelNV\0" // IR(8011, 22)
  "PrimitivePointIndicesEXT\0" // IR(8033, 25)
  "PrimitiveLineIndicesEXT\0" // IR(8058, 24)
  "PrimitiveTriangleIndicesEXT\0" // IR(8082, 28)
  "CullPrimitiveEXT\0" // IR(8110, 17)
  "LaunchIdKHR\0" // IR(8127, 12)
  "LaunchIdNV\0" // IR(8139, 11)
  "LaunchSizeKHR\0" // IR(8150, 14)
  "LaunchSizeNV\0" // IR(8164, 13)
  "WorldRayOriginKHR\0" // IR(8177, 18)
  "WorldRayOriginNV\0" // IR(8195, 17)
  "WorldRayDirectionKHR\0" // IR(8212, 21)
  "WorldRayDirectionNV\0" // IR(8233, 20)
  "ObjectRayOriginKHR\0" // IR(8253, 19)
  "ObjectRayOriginNV\0" // IR(8272, 18)
  "ObjectRayDirectionKHR\0" // IR(8290, 22)
  "ObjectRayDirectionNV\0" // IR(8312, 21)
  "RayTminKHR\0" // IR(8333, 11)
  "RayTminNV\0" // IR(8344, 10)
  "RayTmaxKHR\0" // IR(8354, 11)
  "RayTmaxNV\0" // IR(8365, 10)
  "InstanceCustomIndexKHR\0" // IR(8375, 23)
  "InstanceCustomIndexNV\0" // IR(8398, 22)
  "ObjectToWorldKHR\0" // IR(8420, 17)
  "ObjectToWorldNV\0" // IR(8437, 16)
  "WorldToObjectKHR\0" // IR(8453, 17)
  "WorldToObjectNV\0" // IR(8470, 16)
  "HitTNV\0" // IR(8486, 7)
  "HitKindKHR\0" // IR(8493, 11)
  "HitKindNV\0" // IR(8504, 10)
  "CurrentRayTimeNV\0" // IR(8514, 17)
  "RayTracingMotionBlurNV\0" // IR(8531, 23)
  "HitTriangleVertexPositionsKHR\0" // IR(8554, 30)
  "RayTracingPositionFetchKHR\0" // IR(8584, 27)
  "HitMicroTriangleVertexPositionsNV\0" // IR(8611, 34)
  "RayTracingDisplacementMicromapNV\0" // IR(8645, 33)
  "HitMicroTriangleVertexBarycentricsNV\0" // IR(8678, 37)
  "IncomingRayFlagsKHR\0" // IR(8715, 20)
  "IncomingRayFlagsNV\0" // IR(8735, 19)
  "RayGeometryIndexKHR\0" // IR(8754, 20)
  "HitIsSphereNV\0" // IR(8774, 14)
  "RayTracingSpheresGeometryNV\0" // IR(8788, 28)
  "HitIsLSSNV\0" // IR(8816, 11)
  "RayTracingLinearSweptSpheresGeometryNV\0" // IR(8827, 39)
  "HitSpherePositionNV\0" // IR(8866, 20)
  "WarpsPerSMNV\0" // IR(8886, 13)
  "ShaderSMBuiltinsNV\0" // IR(8899, 19)
  "SMCountNV\0" // IR(8918, 10)
  "WarpIDNV\0" // IR(8928, 9)
  "SMIDNV\0" // IR(8937, 7)
  "HitLSSPositionsNV\0" // IR(8944, 18)
  "HitKindFrontFacingMicroTriangleNV\0" // IR(8962, 34)
  "HitKindBackFacingMicroTriangleNV\0" // IR(8996, 33)
  "HitSphereRadiusNV\0" // IR(9029, 18)
  "HitLSSRadiiNV\0" // IR(9047, 14)
  "ClusterIDNV\0" // IR(9061, 12)
  "RayTracingClusterAccelerationStructureNV\0" // IR(9073, 41)
  "CullMaskKHR\0" // IR(9114, 12)
  "RayCullMaskKHR\0" // IR(9126, 15)
  "Matrix\0" // IR(9141, 7)
  "Linkage\0" // IR(9148, 8)
  "Vector16\0" // IR(9156, 9)
  "Float16Buffer\0" // IR(9165, 14)
  "Float16\0" // IR(9179, 8)
  "Float64\0" // IR(9187, 8)
  "Int64\0" // IR(9195, 6)
  "Int64Atomics\0" // IR(9201, 13)
  "ImageBasic\0" // IR(9214, 11)
  "ImageReadWrite\0" // IR(9225, 15)
  "ImageMipmap\0" // IR(9240, 12)
  "Pipes\0" // IR(9252, 6)
  "Groups\0" // IR(9258, 7)
  "DeviceEnqueue\0" // IR(9265, 14)
  "LiteralSampler\0" // IR(9279, 15)
  "AtomicStorage\0" // IR(9294, 14)
  "Int16\0" // IR(9308, 6)
  "TessellationPointSize\0" // IR(9314, 22)
  "GeometryPointSize\0" // IR(9336, 18)
  "ImageGatherExtended\0" // IR(9354, 20)
  "StorageImageMultisample\0" // IR(9374, 24)
  "UniformBufferArrayDynamicIndexing\0" // IR(9398, 34)
  "SampledImageArrayDynamicIndexing\0" // IR(9432, 33)
  "StorageBufferArrayDynamicIndexing\0" // IR(9465, 34)
  "StorageImageArrayDynamicIndexing\0" // IR(9499, 33)
  "ImageCubeArray\0" // IR(9532, 15)
  "SampledCubeArray\0" // IR(9547, 17)
  "ImageRect\0" // IR(9564, 10)
  "SampledRect\0" // IR(9574, 12)
  "GenericPointer\0" // IR(9586, 15)
  "Int8\0" // IR(9601, 5)
  "InputAttachment\0" // IR(9606, 16)
  "SparseResidency\0" // IR(9622, 16)
  "MinLod\0" // IR(9638, 7)
  "Sampled1D\0" // IR(9645, 10)
  "Image1D\0" // IR(9655, 8)
  "SampledBuffer\0" // IR(9663, 14)
  "ImageBuffer\0" // IR(9677, 12)
  "ImageMSArray\0" // IR(9689, 13)
  "StorageImageExtendedFormats\0" // IR(9702, 28)
  "ImageQuery\0" // IR(9730, 11)
  "DerivativeControl\0" // IR(9741, 18)
  "InterpolationFunction\0" // IR(9759, 22)
  "TransformFeedback\0" // IR(9781, 18)
  "GeometryStreams\0" // IR(9799, 16)
  "StorageImageReadWithoutFormat\0" // IR(9815, 30)
  "StorageImageWriteWithoutFormat\0" // IR(9845, 31)
  "SubgroupDispatch\0" // IR(9876, 17)
  "NamedBarrier\0" // IR(9893, 13)
  "PipeStorage\0" // IR(9906, 12)
  "GroupNonUniformVote\0" // IR(9918, 20)
  "GroupNonUniformArithmetic\0" // IR(9938, 26)
  "GroupNonUniformShuffle\0" // IR(9964, 23)
  "GroupNonUniformShuffleRelative\0" // IR(9987, 31)
  "GroupNonUniformClustered\0" // IR(10018, 25)
  "GroupNonUniformQuad\0" // IR(10043, 20)
  "UniformDecoration\0" // IR(10063, 18)
  "TileImageColorReadAccessEXT\0" // IR(10081, 28)
  "TileImageDepthReadAccessEXT\0" // IR(10109, 28)
  "TileImageStencilReadAccessEXT\0" // IR(10137, 30)
  "TensorsARM\0" // IR(10167, 11)
  "StorageTensorArrayDynamicIndexingARM\0" // IR(10178, 37)
  "StorageTensorArrayNonUniformIndexingARM\0" // IR(10215, 40)
  "GraphARM\0" // IR(10255, 9)
  "CooperativeMatrixLayoutsARM\0" // IR(10264, 28)
  "Float8EXT\0" // IR(10292, 10)
  "Float8CooperativeMatrixEXT\0" // IR(10302, 27)
  "CooperativeMatrixKHR\0" // IR(10329, 21)
  "Float6EXT\0" // IR(10350, 10)
  "Float4EXT\0" // IR(10360, 10)
  "Float8UnsignedE8M0EXT\0" // IR(10370, 22)
  "MXInt8EXT\0" // IR(10392, 10)
  "BitcastExtractEXT\0" // IR(10402, 18)
  "WorkgroupMemoryExplicitLayoutKHR\0" // IR(10420, 33)
  "WorkgroupMemoryExplicitLayout8BitAccessKHR\0" // IR(10453, 43)
  "WorkgroupMemoryExplicitLayout16BitAccessKHR\0" // IR(10496, 44)
  "SubgroupVoteKHR\0" // IR(10540, 16)
  "StorageBuffer16BitAccess\0" // IR(10556, 25)
  "StorageUniformBufferBlock16\0" // IR(10581, 28)
  "UniformAndStorageBuffer16BitAccess\0" // IR(10609, 35)
  "StorageUniform16\0" // IR(10644, 17)
  "StoragePushConstant16\0" // IR(10661, 22)
  "StorageInputOutput16\0" // IR(10683, 21)
  "VariablePointersStorageBuffer\0" // IR(10704, 30)
  "VariablePointers\0" // IR(10734, 17)
  "AtomicStorageOps\0" // IR(10751, 17)
  "SampleMaskPostDepthCoverage\0" // IR(10768, 28)
  "StorageBuffer8BitAccess\0" // IR(10796, 24)
  "UniformAndStorageBuffer8BitAccess\0" // IR(10820, 34)
  "StoragePushConstant8\0" // IR(10854, 21)
  "DenormPreserve\0" // IR(10875, 15)
  "DenormFlushToZero\0" // IR(10890, 18)
  "SignedZeroInfNanPreserve\0" // IR(10908, 25)
  "RoundingModeRTE\0" // IR(10933, 16)
  "RoundingModeRTZ\0" // IR(10949, 16)
  "RayQueryProvisionalKHR\0" // IR(10965, 23)
  "RayQueryKHR\0" // IR(10988, 12)
  "UntypedPointersKHR\0" // IR(11000, 19)
  "RayTraversalPrimitiveCullingKHR\0" // IR(11019, 32)
  "TextureSampleWeightedQCOM\0" // IR(11051, 26)
  "TextureBoxFilterQCOM\0" // IR(11077, 21)
  "TextureBlockMatchQCOM\0" // IR(11098, 22)
  "CooperativeMatrixConversionQCOM\0" // IR(11120, 32)
  "TextureBlockMatch2QCOM\0" // IR(11152, 23)
  "MultipleWaitQueuesQCOM\0" // IR(11175, 23)
  "ImageGatherLinearQCOM\0" // IR(11198, 22)
  "ImageGatherExtendedModesQCOM\0" // IR(11220, 29)
  "Float16ImageAMD\0" // IR(11249, 16)
  "ImageGatherBiasLodAMD\0" // IR(11265, 22)
  "FragmentMaskAMD\0" // IR(11287, 16)
  "ImageReadWriteLodAMD\0" // IR(11303, 21)
  "Int64ImageEXT\0" // IR(11324, 14)
  "ShaderClockKHR\0" // IR(11338, 15)
  "QuadControlKHR\0" // IR(11353, 15)
  "Int4TypeINTEL\0" // IR(11368, 14)
  "Int4CooperativeMatrixINTEL\0" // IR(11382, 27)
  "BFloat16TypeKHR\0" // IR(11409, 16)
  "BFloat16DotProductKHR\0" // IR(11425, 22)
  "BFloat16CooperativeMatrixKHR\0" // IR(11447, 29)
  "AbortKHR\0" // IR(11476, 9)
  "ConstantDataKHR\0" // IR(11485, 16)
  "PoisonFreezeKHR\0" // IR(11501, 16)
  "WeakLinkageAMD\0" // IR(11517, 15)
  "SampleMaskOverrideCoverageNV\0" // IR(11532, 29)
  "GeometryShaderPassthroughNV\0" // IR(11561, 28)
  "ShaderViewportIndexLayerNV\0" // IR(11589, 27)
  "ImageFootprintNV\0" // IR(11616, 17)
  "FragmentBarycentricNV\0" // IR(11633, 22)
  "ComputeDerivativeGroupQuadsKHR\0" // IR(11655, 31)
  "ComputeDerivativeGroupQuadsNV\0" // IR(11686, 30)
  "ShadingRateNV\0" // IR(11716, 14)
  "GroupNonUniformPartitionedEXT\0" // IR(11730, 30)
  "GroupNonUniformPartitionedNV\0" // IR(11760, 29)
  "ShaderNonUniform\0" // IR(11789, 17)
  "ShaderNonUniformEXT\0" // IR(11806, 20)
  "RuntimeDescriptorArray\0" // IR(11826, 23)
  "RuntimeDescriptorArrayEXT\0" // IR(11849, 26)
  "InputAttachmentArrayDynamicIndexing\0" // IR(11875, 36)
  "InputAttachmentArrayDynamicIndexingEXT\0" // IR(11911, 39)
  "UniformTexelBufferArrayDynamicIndexing\0" // IR(11950, 39)
  "UniformTexelBufferArrayDynamicIndexingEXT\0" // IR(11989, 42)
  "StorageTexelBufferArrayDynamicIndexing\0" // IR(12031, 39)
  "StorageTexelBufferArrayDynamicIndexingEXT\0" // IR(12070, 42)
  "UniformBufferArrayNonUniformIndexing\0" // IR(12112, 37)
  "UniformBufferArrayNonUniformIndexingEXT\0" // IR(12149, 40)
  "SampledImageArrayNonUniformIndexing\0" // IR(12189, 36)
  "SampledImageArrayNonUniformIndexingEXT\0" // IR(12225, 39)
  "StorageBufferArrayNonUniformIndexing\0" // IR(12264, 37)
  "StorageBufferArrayNonUniformIndexingEXT\0" // IR(12301, 40)
  "StorageImageArrayNonUniformIndexing\0" // IR(12341, 36)
  "StorageImageArrayNonUniformIndexingEXT\0" // IR(12377, 39)
  "InputAttachmentArrayNonUniformIndexing\0" // IR(12416, 39)
  "InputAttachmentArrayNonUniformIndexingEXT\0" // IR(12455, 42)
  "UniformTexelBufferArrayNonUniformIndexing\0" // IR(12497, 42)
  "UniformTexelBufferArrayNonUniformIndexingEXT\0" // IR(12539, 45)
  "StorageTexelBufferArrayNonUniformIndexing\0" // IR(12584, 42)
  "StorageTexelBufferArrayNonUniformIndexingEXT\0" // IR(12626, 45)
  "VulkanMemoryModel\0" // IR(12671, 18)
  "VulkanMemoryModelKHR\0" // IR(12689, 21)
  "VulkanMemoryModelDeviceScope\0" // IR(12710, 29)
  "VulkanMemoryModelDeviceScopeKHR\0" // IR(12739, 32)
  "PhysicalStorageBufferAddressesEXT\0" // IR(12771, 34)
  "ComputeDerivativeGroupLinearKHR\0" // IR(12805, 32)
  "ComputeDerivativeGroupLinearNV\0" // IR(12837, 31)
  "RayTracingProvisionalKHR\0" // IR(12868, 25)
  "CooperativeMatrixNV\0" // IR(12893, 20)
  "FragmentShaderSampleInterlockEXT\0" // IR(12913, 33)
  "FragmentShaderShadingRateInterlockEXT\0" // IR(12946, 38)
  "FragmentShaderPixelInterlockEXT\0" // IR(12984, 32)
  "DemoteToHelperInvocation\0" // IR(13016, 25)
  "DemoteToHelperInvocationEXT\0" // IR(13041, 28)
  "DisplacementMicromapNV\0" // IR(13069, 23)
  "RayTracingOpacityMicromapKHR\0" // IR(13092, 29)
  "RayTracingOpacityMicromapEXT\0" // IR(13121, 29)
  "ShaderInvocationReorderNV\0" // IR(13150, 26)
  "ShaderInvocationReorderEXT\0" // IR(13176, 27)
  "BindlessTextureNV\0" // IR(13203, 18)
  "RayQueryPositionFetchKHR\0" // IR(13221, 25)
  "CooperativeVectorNV\0" // IR(13246, 20)
  "AtomicFloat16VectorNV\0" // IR(13266, 22)
  "RawAccessChainsNV\0" // IR(13288, 18)
  "PushConstantBanksNV\0" // IR(13306, 20)
  "LongVectorEXT\0" // IR(13326, 14)
  "Shader64BitIndexingEXT\0" // IR(13340, 23)
  "CooperativeMatrixReductionsNV\0" // IR(13363, 30)
  "CooperativeMatrixConversionsNV\0" // IR(13393, 31)
  "CooperativeMatrixPerElementOperationsNV\0" // IR(13424, 40)
  "CooperativeMatrixTensorAddressingNV\0" // IR(13464, 36)
  "CooperativeMatrixBlockLoadsNV\0" // IR(13500, 30)
  "CooperativeVectorTrainingNV\0" // IR(13530, 28)
  "TensorAddressingNV\0" // IR(13558, 19)
  "CooperativeMatrixDecodeVectorNV\0" // IR(13577, 32)
  "SubgroupShuffleINTEL\0" // IR(13609, 21)
  "SubgroupBufferBlockIOINTEL\0" // IR(13630, 27)
  "SubgroupImageBlockIOINTEL\0" // IR(13657, 26)
  "SubgroupImageMediaBlockIOINTEL\0" // IR(13683, 31)
  "RoundToInfinityINTEL\0" // IR(13714, 21)
  "FloatingPointModeINTEL\0" // IR(13735, 23)
  "IntegerFunctions2INTEL\0" // IR(13758, 23)
  "FunctionPointersINTEL\0" // IR(13781, 22)
  "IndirectReferencesINTEL\0" // IR(13803, 24)
  "AsmINTEL\0" // IR(13827, 9)
  "AtomicFloat32MinMaxEXT\0" // IR(13836, 23)
  "AtomicFloat64MinMaxEXT\0" // IR(13859, 23)
  "AtomicFloat16MinMaxEXT\0" // IR(13882, 23)
  "VectorComputeINTEL\0" // IR(13905, 19)
  "VectorAnyINTEL\0" // IR(13924, 15)
  "ExpectAssumeKHR\0" // IR(13939, 16)
  "SubgroupAvcMotionEstimationINTEL\0" // IR(13955, 33)
  "SubgroupAvcMotionEstimationIntraINTEL\0" // IR(13988, 38)
  "SubgroupAvcMotionEstimationChromaINTEL\0" // IR(14026, 39)
  "VariableLengthArrayINTEL\0" // IR(14065, 25)
  "FunctionFloatControlINTEL\0" // IR(14090, 26)
  "FPGAMemoryAttributesALTERA\0" // IR(14116, 27)
  "FPGAMemoryAttributesINTEL\0" // IR(14143, 26)
  "FPFastMathModeINTEL\0" // IR(14169, 20)
  "ArbitraryPrecisionIntegersALTERA\0" // IR(14189, 33)
  "ArbitraryPrecisionIntegersINTEL\0" // IR(14222, 32)
  "ArbitraryPrecisionFloatingPointALTERA\0" // IR(14254, 38)
  "ArbitraryPrecisionFloatingPointINTEL\0" // IR(14292, 37)
  "UnstructuredLoopControlsINTEL\0" // IR(14329, 30)
  "FPGALoopControlsALTERA\0" // IR(14359, 23)
  "FPGALoopControlsINTEL\0" // IR(14382, 22)
  "KernelAttributesINTEL\0" // IR(14404, 22)
  "FPGAKernelAttributesINTEL\0" // IR(14426, 26)
  "FPGAMemoryAccessesALTERA\0" // IR(14452, 25)
  "FPGAMemoryAccessesINTEL\0" // IR(14477, 24)
  "FPGAClusterAttributesALTERA\0" // IR(14501, 28)
  "FPGAClusterAttributesINTEL\0" // IR(14529, 27)
  "LoopFuseALTERA\0" // IR(14556, 15)
  "LoopFuseINTEL\0" // IR(14571, 14)
  "FPGADSPControlALTERA\0" // IR(14585, 21)
  "FPGADSPControlINTEL\0" // IR(14606, 20)
  "MemoryAccessAliasingINTEL\0" // IR(14626, 26)
  "FPGAInvocationPipeliningAttributesALTERA\0" // IR(14652, 41)
  "FPGAInvocationPipeliningAttributesINTEL\0" // IR(14693, 40)
  "FPGABufferLocationALTERA\0" // IR(14733, 25)
  "FPGABufferLocationINTEL\0" // IR(14758, 24)
  "ArbitraryPrecisionFixedPointALTERA\0" // IR(14782, 35)
  "ArbitraryPrecisionFixedPointINTEL\0" // IR(14817, 34)
  "USMStorageClassesALTERA\0" // IR(14851, 24)
  "USMStorageClassesINTEL\0" // IR(14875, 23)
  "RuntimeAlignedAttributeALTERA\0" // IR(14898, 30)
  "RuntimeAlignedAttributeINTEL\0" // IR(14928, 29)
  "IOPipesALTERA\0" // IR(14957, 14)
  "IOPipesINTEL\0" // IR(14971, 13)
  "BlockingPipesALTERA\0" // IR(14984, 20)
  "BlockingPipesINTEL\0" // IR(15004, 19)
  "FPGARegALTERA\0" // IR(15023, 14)
  "FPGARegINTEL\0" // IR(15037, 13)
  "DotProductInputAll\0" // IR(15050, 19)
  "DotProductInputAllKHR\0" // IR(15069, 22)
  "DotProductInput4x8Bit\0" // IR(15091, 22)
  "DotProductInput4x8BitKHR\0" // IR(15113, 25)
  "DotProductInput4x8BitPacked\0" // IR(15138, 28)
  "DotProductInput4x8BitPackedKHR\0" // IR(15166, 31)
  "DotProduct\0" // IR(15197, 11)
  "DotProductKHR\0" // IR(15208, 14)
  "ReplicatedCompositesEXT\0" // IR(15222, 24)
  "BitInstructions\0" // IR(15246, 16)
  "GroupNonUniformRotateKHR\0" // IR(15262, 25)
  "FloatControls2\0" // IR(15287, 15)
  "FMAKHR\0" // IR(15302, 7)
  "RayTracingOpacityMicromapExecutionModeKHR\0" // IR(15309, 42)
  "AtomicFloat32AddEXT\0" // IR(15351, 20)
  "AtomicFloat64AddEXT\0" // IR(15371, 20)
  "LongCompositesINTEL\0" // IR(15391, 20)
  "OptNoneEXT\0" // IR(15411, 11)
  "OptNoneINTEL\0" // IR(15422, 13)
  "AtomicFloat16AddEXT\0" // IR(15435, 20)
  "DebugInfoModuleINTEL\0" // IR(15455, 21)
  "BFloat16ConversionINTEL\0" // IR(15476, 24)
  "SplitBarrierEXT\0" // IR(15500, 16)
  "SplitBarrierINTEL\0" // IR(15516, 18)
  "ArithmeticFenceEXT\0" // IR(15534, 19)
  "FPGAClusterAttributesV2ALTERA\0" // IR(15553, 30)
  "FPGAClusterAttributesV2INTEL\0" // IR(15583, 29)
  "FPGAKernelAttributesv2INTEL\0" // IR(15612, 28)
  "TaskSequenceALTERA\0" // IR(15640, 19)
  "TaskSequenceINTEL\0" // IR(15659, 18)
  "FPMaxErrorINTEL\0" // IR(15677, 16)
  "FPGALatencyControlALTERA\0" // IR(15693, 25)
  "FPGALatencyControlINTEL\0" // IR(15718, 24)
  "FPGAArgumentInterfacesALTERA\0" // IR(15742, 29)
  "FPGAArgumentInterfacesINTEL\0" // IR(15771, 28)
  "GlobalVariableHostAccessINTEL\0" // IR(15799, 30)
  "GlobalVariableFPGADecorationsALTERA\0" // IR(15829, 36)
  "GlobalVariableFPGADecorationsINTEL\0" // IR(15865, 35)
  "SubgroupBufferPrefetchINTEL\0" // IR(15900, 28)
  "Subgroup2DBlockIOINTEL\0" // IR(15928, 23)
  "Subgroup2DBlockTransformINTEL\0" // IR(15951, 30)
  "Subgroup2DBlockTransposeINTEL\0" // IR(15981, 30)
  "SubgroupMatrixMultiplyAccumulateINTEL\0" // IR(16011, 38)
  "TernaryBitwiseFunctionINTEL\0" // IR(16049, 28)
  "UntypedVariableLengthArrayINTEL\0" // IR(16077, 32)
  "SpecConditionalINTEL\0" // IR(16109, 21)
  "FunctionVariantsINTEL\0" // IR(16130, 22)
  "PredicatedIOINTEL\0" // IR(16152, 18)
  "RoundedDivideSqrtINTEL\0" // IR(16170, 23)
  "GroupUniformArithmeticKHR\0" // IR(16193, 26)
  "TensorFloat32RoundingINTEL\0" // IR(16219, 27)
  "MaskedGatherScatterINTEL\0" // IR(16246, 25)
  "CacheControlsINTEL\0" // IR(16271, 19)
  "RegisterLimitsINTEL\0" // IR(16290, 20)
  "BindlessImagesINTEL\0" // IR(16310, 20)
  "DotProductFloat16AccFloat32VALVE\0" // IR(16330, 33)
  "DotProductFloat16AccFloat16VALVE\0" // IR(16363, 33)
  "DotProductBFloat16AccVALVE\0" // IR(16396, 27)
  "DotProductFloat8AccFloat32VALVE\0" // IR(16423, 32)
  "Unspecified\0" // IR(16455, 12)
  "Address\0" // IR(16467, 8)
  "Boolean\0" // IR(16475, 8)
  "Float\0" // IR(16483, 6)
  "Signed\0" // IR(16489, 7)
  "SignedChar\0" // IR(16496, 11)
  "Unsigned\0" // IR(16507, 9)
  "UnsignedChar\0" // IR(16516, 13)
  "Class\0" // IR(16529, 6)
  "Structure\0" // IR(16535, 10)
  "Union\0" // IR(16545, 6)
  "ImportedModule\0" // IR(16551, 15)
  "ImportedDeclaration\0" // IR(16566, 20)
  "None\0" // IR(16586, 5)
  "FlagIsProtected\0" // IR(16591, 16)
  "FlagIsPrivate\0" // IR(16607, 14)
  "FlagIsPublic\0" // IR(16621, 13)
  "FlagIsLocal\0" // IR(16634, 12)
  "FlagIsDefinition\0" // IR(16646, 17)
  "FlagFwdDecl\0" // IR(16663, 12)
  "FlagArtificial\0" // IR(16675, 15)
  "FlagExplicit\0" // IR(16690, 13)
  "FlagPrototyped\0" // IR(16703, 15)
  "FlagObjectPointer\0" // IR(16718, 18)
  "FlagStaticMember\0" // IR(16736, 17)
  "FlagIndirectVariable\0" // IR(16753, 21)
  "FlagLValueReference\0" // IR(16774, 20)
  "FlagRValueReference\0" // IR(16794, 20)
  "FlagIsOptimized\0" // IR(16814, 16)
  "FlagIsEnumClass\0" // IR(16830, 16)
  "FlagTypePassByValue\0" // IR(16846, 20)
  "FlagTypePassByReference\0" // IR(16866, 24)
  "Deref\0" // IR(16890, 6)
  "Plus\0" // IR(16896, 5)
  "Minus\0" // IR(16901, 6)
  "SPV_OPERAND_TYPE_LITERAL_INTEGER\0" // IR(16907, 33)
  "PlusUconst\0" // IR(16940, 11)
  "BitPiece\0" // IR(16951, 9)
  "Swap\0" // IR(16960, 5)
  "Xderef\0" // IR(16965, 7)
  "StackValue\0" // IR(16972, 11)
  "Constu\0" // IR(16983, 7)
  "Fragment\0" // IR(16990, 9)
  "ConstType\0" // IR(16999, 10)
  "VolatileType\0" // IR(17009, 13)
  "RestrictType\0" // IR(17022, 13)
  "AtomicType\0" // IR(17035, 11)
  "Float16NV\0" // IR(17046, 10)
  "Float32NV\0" // IR(17056, 10)
  "Float64NV\0" // IR(17066, 10)
  "SignedInt8NV\0" // IR(17076, 13)
  "SignedInt16NV\0" // IR(17089, 14)
  "SignedInt32NV\0" // IR(17103, 14)
  "SignedInt64NV\0" // IR(17117, 14)
  "UnsignedInt8NV\0" // IR(17131, 15)
  "UnsignedInt16NV\0" // IR(17146, 16)
  "UnsignedInt32NV\0" // IR(17162, 16)
  "UnsignedInt64NV\0" // IR(17178, 16)
  "SignedInt8PackedNV\0" // IR(17194, 19)
  "UnsignedInt8PackedNV\0" // IR(17213, 21)
  "FloatE4M3NV\0" // IR(17234, 12)
  "FloatE5M2NV\0" // IR(17246, 12)
  "RowMajorKHR\0" // IR(17258, 12)
  "ColumnMajorKHR\0" // IR(17270, 15)
  "RowBlockedInterleavedARM\0" // IR(17285, 25)
  "ColumnBlockedInterleavedARM\0" // IR(17310, 28)
  "NoneKHR\0" // IR(17338, 8)
  "MatrixASignedComponentsKHR\0" // IR(17346, 27)
  "MatrixBSignedComponentsKHR\0" // IR(17373, 27)
  "MatrixCSignedComponentsKHR\0" // IR(17400, 27)
  "MatrixResultSignedComponentsKHR\0" // IR(17427, 32)
  "SaturatingAccumulationKHR\0" // IR(17459, 26)
  "Row\0" // IR(17485, 4)
  "Column\0" // IR(17489, 7)
  "2x2\0" // IR(17496, 4)
  "MatrixAKHR\0" // IR(17500, 11)
  "MatrixBKHR\0" // IR(17511, 11)
  "MatrixAccumulatorKHR\0" // IR(17522, 21)
  "RowMajorNV\0" // IR(17543, 11)
  "ColumnMajorNV\0" // IR(17554, 14)
  "InferencingOptimalNV\0" // IR(17568, 21)
  "TrainingOptimalNV\0" // IR(17589, 18)
  "RelaxedPrecision\0" // IR(17607, 17)
  "SpecId\0" // IR(17624, 7)
  "Block\0" // IR(17631, 6)
  "BufferBlock\0" // IR(17637, 12)
  "RowMajor\0" // IR(17649, 9)
  "ColMajor\0" // IR(17658, 9)
  "ArrayStride\0" // IR(17667, 12)
  "MatrixStride\0" // IR(17679, 13)
  "GLSLShared\0" // IR(17692, 11)
  "GLSLPacked\0" // IR(17703, 11)
  "CPacked\0" // IR(17714, 8)
  "SPV_OPERAND_TYPE_BUILT_IN\0" // IR(17722, 26)
  "BuiltIn\0" // IR(17748, 8)
  "NoPerspective\0" // IR(17756, 14)
  "Flat\0" // IR(17770, 5)
  "Patch\0" // IR(17775, 6)
  "Centroid\0" // IR(17781, 9)
  "Sample\0" // IR(17790, 7)
  "Invariant\0" // IR(17797, 10)
  "Restrict\0" // IR(17807, 9)
  "Aliased\0" // IR(17816, 8)
  "Volatile\0" // IR(17824, 9)
  "Constant\0" // IR(17833, 9)
  "Coherent\0" // IR(17842, 9)
  "NonWritable\0" // IR(17851, 12)
  "NonReadable\0" // IR(17863, 12)
  "Uniform\0" // IR(17875, 8)
  "SPV_OPERAND_TYPE_SCOPE_ID\0" // IR(17883, 26)
  "UniformId\0" // IR(17909, 10)
  "SaturatedConversion\0" // IR(17919, 20)
  "Stream\0" // IR(17939, 7)
  "Location\0" // IR(17946, 9)
  "Component\0" // IR(17955, 10)
  "Index\0" // IR(17965, 6)
  "Binding\0" // IR(17971, 8)
  "DescriptorSet\0" // IR(17979, 14)
  "Offset\0" // IR(17993, 7)
  "XfbBuffer\0" // IR(18000, 10)
  "XfbStride\0" // IR(18010, 10)
  "SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE\0" // IR(18020, 46)
  "FuncParamAttr\0" // IR(18066, 14)
  "SPV_OPERAND_TYPE_FP_ROUNDING_MODE\0" // IR(18080, 34)
  "FPRoundingMode\0" // IR(18114, 15)
  "SPV_OPERAND_TYPE_FP_FAST_MATH_MODE\0" // IR(18129, 35)
  "FPFastMathMode\0" // IR(18164, 15)
  "SPV_OPERAND_TYPE_LITERAL_STRING\0" // IR(18179, 32)
  "SPV_OPERAND_TYPE_LINKAGE_TYPE\0" // IR(18211, 30)
  "LinkageAttributes\0" // IR(18241, 18)
  "NoContraction\0" // IR(18259, 14)
  "InputAttachmentIndex\0" // IR(18273, 21)
  "Alignment\0" // IR(18294, 10)
  "MaxByteOffset\0" // IR(18304, 14)
  "SPV_OPERAND_TYPE_ID\0" // IR(18318, 20)
  "AlignmentId\0" // IR(18338, 12)
  "MaxByteOffsetId\0" // IR(18350, 16)
  "SaturatedToLargestFloat8NormalConversionEXT\0" // IR(18366, 44)
  "NoSignedWrap\0" // IR(18410, 13)
  "NoUnsignedWrap\0" // IR(18423, 15)
  "WeightTextureQCOM\0" // IR(18438, 18)
  "BlockMatchTextureQCOM\0" // IR(18456, 22)
  "BlockMatchSamplerQCOM\0" // IR(18478, 22)
  "ExplicitInterpAMD\0" // IR(18500, 18)
  "NodeSharesPayloadLimitsWithAMDX\0" // IR(18518, 32)
  "NodeMaxPayloadsAMDX\0" // IR(18550, 20)
  "TrackFinishWritingAMDX\0" // IR(18570, 23)
  "PayloadNodeNameAMDX\0" // IR(18593, 20)
  "PayloadNodeBaseIndexAMDX\0" // IR(18613, 25)
  "PayloadNodeSparseArrayAMDX\0" // IR(18638, 27)
  "PayloadNodeArraySizeAMDX\0" // IR(18665, 25)
  "PayloadDispatchIndirectAMDX\0" // IR(18690, 28)
  "ArrayStrideIdEXT\0" // IR(18718, 17)
  "OffsetIdEXT\0" // IR(18735, 12)
  "UTFEncodedKHR\0" // IR(18747, 14)
  "OverrideCoverageNV\0" // IR(18761, 19)
  "PassthroughNV\0" // IR(18780, 14)
  "ViewportRelativeNV\0" // IR(18794, 19)
  "SecondaryViewportRelativeNV\0" // IR(18813, 28)
  "PerPrimitiveEXT\0" // IR(18841, 16)
  "PerPrimitiveNV\0" // IR(18857, 15)
  "PerViewNV\0" // IR(18872, 10)
  "PerTaskNV\0" // IR(18882, 10)
  "PerVertexKHR\0" // IR(18892, 13)
  "PerVertexNV\0" // IR(18905, 12)
  "NonUniform\0" // IR(18917, 11)
  "NonUniformEXT\0" // IR(18928, 14)
  "RestrictPointer\0" // IR(18942, 16)
  "RestrictPointerEXT\0" // IR(18958, 19)
  "AliasedPointer\0" // IR(18977, 15)
  "AliasedPointerEXT\0" // IR(18992, 18)
  "MemberOffsetNV\0" // IR(19010, 15)
  "HitObjectShaderRecordBufferNV\0" // IR(19025, 30)
  "HitObjectShaderRecordBufferEXT\0" // IR(19055, 31)
  "BankNV\0" // IR(19086, 7)
  "BindlessSamplerNV\0" // IR(19093, 18)
  "BindlessImageNV\0" // IR(19111, 16)
  "BoundSamplerNV\0" // IR(19127, 15)
  "BoundImageNV\0" // IR(19142, 13)
  "SIMTCallINTEL\0" // IR(19155, 14)
  "ReferencedIndirectlyINTEL\0" // IR(19169, 26)
  "ClobberINTEL\0" // IR(19195, 13)
  "SideEffectsINTEL\0" // IR(19208, 17)
  "VectorComputeVariableINTEL\0" // IR(19225, 27)
  "FuncParamIOKindINTEL\0" // IR(19252, 21)
  "VectorComputeFunctionINTEL\0" // IR(19273, 27)
  "StackCallINTEL\0" // IR(19300, 15)
  "GlobalVariableOffsetINTEL\0" // IR(19315, 26)
  "CounterBuffer\0" // IR(19341, 14)
  "HlslCounterBufferGOOGLE\0" // IR(19355, 24)
  "UserSemantic\0" // IR(19379, 13)
  "HlslSemanticGOOGLE\0" // IR(19392, 19)
  "UserTypeGOOGLE\0" // IR(19411, 15)
  "FunctionRoundingModeINTEL\0" // IR(19426, 26)
  "SPV_OPERAND_TYPE_FPDENORM_MODE\0" // IR(19452, 31)
  "FunctionDenormModeINTEL\0" // IR(19483, 24)
  "RegisterALTERA\0" // IR(19507, 15)
  "RegisterINTEL\0" // IR(19522, 14)
  "MemoryALTERA\0" // IR(19536, 13)
  "MemoryINTEL\0" // IR(19549, 12)
  "NumbanksALTERA\0" // IR(19561, 15)
  "NumbanksINTEL\0" // IR(19576, 14)
  "BankwidthALTERA\0" // IR(19590, 16)
  "BankwidthINTEL\0" // IR(19606, 15)
  "MaxPrivateCopiesALTERA\0" // IR(19621, 23)
  "MaxPrivateCopiesINTEL\0" // IR(19644, 22)
  "SinglepumpALTERA\0" // IR(19666, 17)
  "SinglepumpINTEL\0" // IR(19683, 16)
  "DoublepumpALTERA\0" // IR(19699, 17)
  "DoublepumpINTEL\0" // IR(19716, 16)
  "MaxReplicatesALTERA\0" // IR(19732, 20)
  "MaxReplicatesINTEL\0" // IR(19752, 19)
  "SimpleDualPortALTERA\0" // IR(19771, 21)
  "SimpleDualPortINTEL\0" // IR(19792, 20)
  "MergeALTERA\0" // IR(19812, 12)
  "MergeINTEL\0" // IR(19824, 11)
  "SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER\0" // IR(19835, 42)
  "BankBitsALTERA\0" // IR(19877, 15)
  "BankBitsINTEL\0" // IR(19892, 14)
  "ForcePow2DepthALTERA\0" // IR(19906, 21)
  "ForcePow2DepthINTEL\0" // IR(19927, 20)
  "StridesizeALTERA\0" // IR(19947, 17)
  "StridesizeINTEL\0" // IR(19964, 16)
  "WordsizeALTERA\0" // IR(19980, 15)
  "WordsizeINTEL\0" // IR(19995, 14)
  "TrueDualPortALTERA\0" // IR(20009, 19)
  "TrueDualPortINTEL\0" // IR(20028, 18)
  "BurstCoalesceALTERA\0" // IR(20046, 20)
  "BurstCoalesceINTEL\0" // IR(20066, 19)
  "CacheSizeALTERA\0" // IR(20085, 16)
  "CacheSizeINTEL\0" // IR(20101, 15)
  "DontStaticallyCoalesceALTERA\0" // IR(20116, 29)
  "DontStaticallyCoalesceINTEL\0" // IR(20145, 28)
  "PrefetchALTERA\0" // IR(20173, 15)
  "PrefetchINTEL\0" // IR(20188, 14)
  "StallEnableALTERA\0" // IR(20202, 18)
  "StallEnableINTEL\0" // IR(20220, 17)
  "FuseLoopsInFunctionALTERA\0" // IR(20237, 26)
  "FuseLoopsInFunctionINTEL\0" // IR(20263, 25)
  "MathOpDSPModeALTERA\0" // IR(20288, 20)
  "MathOpDSPModeINTEL\0" // IR(20308, 19)
  "AliasScopeINTEL\0" // IR(20327, 16)
  "NoAliasINTEL\0" // IR(20343, 13)
  "InitiationIntervalALTERA\0" // IR(20356, 25)
  "InitiationIntervalINTEL\0" // IR(20381, 24)
  "MaxConcurrencyALTERA\0" // IR(20405, 21)
  "MaxConcurrencyINTEL\0" // IR(20426, 20)
  "PipelineEnableALTERA\0" // IR(20446, 21)
  "PipelineEnableINTEL\0" // IR(20467, 20)
  "BufferLocationALTERA\0" // IR(20487, 21)
  "BufferLocationINTEL\0" // IR(20508, 20)
  "IOPipeStorageALTERA\0" // IR(20528, 20)
  "IOPipeStorageINTEL\0" // IR(20548, 19)
  "SPV_OPERAND_TYPE_FPOPERATION_MODE\0" // IR(20567, 34)
  "FunctionFloatingPointModeINTEL\0" // IR(20601, 31)
  "SingleElementVectorINTEL\0" // IR(20632, 25)
  "VectorComputeCallableFunctionINTEL\0" // IR(20657, 35)
  "MediaBlockIOINTEL\0" // IR(20692, 18)
  "StallFreeALTERA\0" // IR(20710, 16)
  "StallFreeINTEL\0" // IR(20726, 15)
  "SPV_OPERAND_TYPE_LITERAL_FLOAT\0" // IR(20741, 31)
  "FPMaxErrorDecorationINTEL\0" // IR(20772, 26)
  "LatencyControlLabelALTERA\0" // IR(20798, 26)
  "LatencyControlLabelINTEL\0" // IR(20824, 25)
  "LatencyControlConstraintALTERA\0" // IR(20849, 31)
  "LatencyControlConstraintINTEL\0" // IR(20880, 30)
  "ConduitKernelArgumentALTERA\0" // IR(20910, 28)
  "ConduitKernelArgumentINTEL\0" // IR(20938, 27)
  "RegisterMapKernelArgumentALTERA\0" // IR(20965, 32)
  "RegisterMapKernelArgumentINTEL\0" // IR(20997, 31)
  "MMHostInterfaceAddressWidthALTERA\0" // IR(21028, 34)
  "MMHostInterfaceAddressWidthINTEL\0" // IR(21062, 33)
  "MMHostInterfaceDataWidthALTERA\0" // IR(21095, 31)
  "MMHostInterfaceDataWidthINTEL\0" // IR(21126, 30)
  "MMHostInterfaceLatencyALTERA\0" // IR(21156, 29)
  "MMHostInterfaceLatencyINTEL\0" // IR(21185, 28)
  "SPV_OPERAND_TYPE_ACCESS_QUALIFIER\0" // IR(21213, 34)
  "MMHostInterfaceReadWriteModeALTERA\0" // IR(21247, 35)
  "MMHostInterfaceReadWriteModeINTEL\0" // IR(21282, 34)
  "MMHostInterfaceMaxBurstALTERA\0" // IR(21316, 30)
  "MMHostInterfaceMaxBurstINTEL\0" // IR(21346, 29)
  "MMHostInterfaceWaitRequestALTERA\0" // IR(21375, 33)
  "MMHostInterfaceWaitRequestINTEL\0" // IR(21408, 32)
  "StableKernelArgumentALTERA\0" // IR(21440, 27)
  "StableKernelArgumentINTEL\0" // IR(21467, 26)
  "SPV_OPERAND_TYPE_HOST_ACCESS_QUALIFIER\0" // IR(21493, 39)
  "HostAccessINTEL\0" // IR(21532, 16)
  "SPV_OPERAND_TYPE_INITIALIZATION_MODE_QUALIFIER\0" // IR(21548, 47)
  "InitModeALTERA\0" // IR(21595, 15)
  "InitModeINTEL\0" // IR(21610, 14)
  "ImplementInRegisterMapALTERA\0" // IR(21624, 29)
  "ImplementInRegisterMapINTEL\0" // IR(21653, 28)
  "ConditionalINTEL\0" // IR(21681, 17)
  "SPV_OPERAND_TYPE_LOAD_CACHE_CONTROL\0" // IR(21698, 36)
  "CacheControlLoadINTEL\0" // IR(21734, 22)
  "SPV_OPERAND_TYPE_STORE_CACHE_CONTROL\0" // IR(21756, 37)
  "CacheControlStoreINTEL\0" // IR(21793, 23)
  "1D\0" // IR(21816, 3)
  "2D\0" // IR(21819, 3)
  "3D\0" // IR(21822, 3)
  "Cube\0" // IR(21825, 5)
  "Rect\0" // IR(21830, 5)
  "Buffer\0" // IR(21835, 7)
  "SubpassData\0" // IR(21842, 12)
  "TileImageDataEXT\0" // IR(21854, 17)
  "Invocations\0" // IR(21871, 12)
  "SpacingEqual\0" // IR(21883, 13)
  "SpacingFractionalEven\0" // IR(21896, 22)
  "SpacingFractionalOdd\0" // IR(21918, 21)
  "VertexOrderCw\0" // IR(21939, 14)
  "VertexOrderCcw\0" // IR(21953, 15)
  "PixelCenterInteger\0" // IR(21968, 19)
  "OriginUpperLeft\0" // IR(21987, 16)
  "OriginLowerLeft\0" // IR(22003, 16)
  "EarlyFragmentTests\0" // IR(22019, 19)
  "PointMode\0" // IR(22038, 10)
  "Xfb\0" // IR(22048, 4)
  "DepthReplacing\0" // IR(22052, 15)
  "DepthGreater\0" // IR(22067, 13)
  "DepthLess\0" // IR(22080, 10)
  "DepthUnchanged\0" // IR(22090, 15)
  "LocalSize\0" // IR(22105, 10)
  "LocalSizeHint\0" // IR(22115, 14)
  "InputPoints\0" // IR(22129, 12)
  "InputLines\0" // IR(22141, 11)
  "InputLinesAdjacency\0" // IR(22152, 20)
  "Triangles\0" // IR(22172, 10)
  "InputTrianglesAdjacency\0" // IR(22182, 24)
  "Quads\0" // IR(22206, 6)
  "Isolines\0" // IR(22212, 9)
  "OutputVertices\0" // IR(22221, 15)
  "OutputPoints\0" // IR(22236, 13)
  "OutputLineStrip\0" // IR(22249, 16)
  "OutputTriangleStrip\0" // IR(22265, 20)
  "VecTypeHint\0" // IR(22285, 12)
  "ContractionOff\0" // IR(22297, 15)
  "Initializer\0" // IR(22312, 12)
  "Finalizer\0" // IR(22324, 10)
  "SubgroupsPerWorkgroup\0" // IR(22334, 22)
  "SubgroupsPerWorkgroupId\0" // IR(22356, 24)
  "LocalSizeId\0" // IR(22380, 12)
  "LocalSizeHintId\0" // IR(22392, 16)
  "NonCoherentColorAttachmentReadEXT\0" // IR(22408, 34)
  "NonCoherentDepthAttachmentReadEXT\0" // IR(22442, 34)
  "NonCoherentStencilAttachmentReadEXT\0" // IR(22476, 36)
  "SubgroupUniformControlFlowKHR\0" // IR(22512, 30)
  "PostDepthCoverage\0" // IR(22542, 18)
  "NonCoherentTileAttachmentReadQCOM\0" // IR(22560, 34)
  "TileShadingRateQCOM\0" // IR(22594, 20)
  "EarlyAndLateFragmentTestsAMD\0" // IR(22614, 29)
  "StencilRefReplacingEXT\0" // IR(22643, 23)
  "CoalescingAMDX\0" // IR(22666, 15)
  "IsApiEntryAMDX\0" // IR(22681, 15)
  "MaxNodeRecursionAMDX\0" // IR(22696, 21)
  "StaticNumWorkgroupsAMDX\0" // IR(22717, 24)
  "MaxNumWorkgroupsAMDX\0" // IR(22741, 21)
  "StencilRefUnchangedFrontAMD\0" // IR(22762, 28)
  "StencilRefGreaterFrontAMD\0" // IR(22790, 26)
  "StencilRefLessFrontAMD\0" // IR(22816, 23)
  "StencilRefUnchangedBackAMD\0" // IR(22839, 27)
  "StencilRefGreaterBackAMD\0" // IR(22866, 25)
  "StencilRefLessBackAMD\0" // IR(22891, 22)
  "QuadDerivativesKHR\0" // IR(22913, 19)
  "RequireFullQuadsKHR\0" // IR(22932, 20)
  "SharesInputWithAMDX\0" // IR(22952, 20)
  "ArithmeticPoisonKHR\0" // IR(22972, 20)
  "OutputLinesEXT\0" // IR(22992, 15)
  "OutputLinesNV\0" // IR(23007, 14)
  "OutputPrimitivesEXT\0" // IR(23021, 20)
  "OutputPrimitivesNV\0" // IR(23041, 19)
  "DerivativeGroupQuadsKHR\0" // IR(23060, 24)
  "DerivativeGroupQuadsNV\0" // IR(23084, 23)
  "DerivativeGroupLinearKHR\0" // IR(23107, 25)
  "DerivativeGroupLinearNV\0" // IR(23132, 24)
  "OutputTrianglesEXT\0" // IR(23156, 19)
  "OutputTrianglesNV\0" // IR(23175, 18)
  "PixelInterlockOrderedEXT\0" // IR(23193, 25)
  "PixelInterlockUnorderedEXT\0" // IR(23218, 27)
  "SampleInterlockOrderedEXT\0" // IR(23245, 26)
  "SampleInterlockUnorderedEXT\0" // IR(23271, 28)
  "ShadingRateInterlockOrderedEXT\0" // IR(23299, 31)
  "ShadingRateInterlockUnorderedEXT\0" // IR(23330, 33)
  "SharedLocalMemorySizeINTEL\0" // IR(23363, 27)
  "RoundingModeRTPINTEL\0" // IR(23390, 21)
  "RoundingModeRTNINTEL\0" // IR(23411, 21)
  "FloatingPointModeALTINTEL\0" // IR(23432, 26)
  "FloatingPointModeIEEEINTEL\0" // IR(23458, 27)
  "MaxWorkgroupSizeINTEL\0" // IR(23485, 22)
  "MaxWorkDimINTEL\0" // IR(23507, 16)
  "NoGlobalOffsetINTEL\0" // IR(23523, 20)
  "NumSIMDWorkitemsINTEL\0" // IR(23543, 22)
  "SchedulerTargetFmaxMhzINTEL\0" // IR(23565, 28)
  "MaximallyReconvergesKHR\0" // IR(23593, 24)
  "FPFastMathDefault\0" // IR(23617, 18)
  "OpacityMicromapIdKHR\0" // IR(23635, 21)
  "StreamingInterfaceINTEL\0" // IR(23656, 24)
  "RegisterMapInterfaceINTEL\0" // IR(23680, 26)
  "NamedBarrierCountINTEL\0" // IR(23706, 23)
  "MaximumRegistersINTEL\0" // IR(23729, 22)
  "MaximumRegistersIdINTEL\0" // IR(23751, 24)
  "SPV_OPERAND_TYPE_NAMED_MAXIMUM_NUMBER_OF_REGISTERS\0" // IR(23775, 51)
  "NamedMaximumRegistersINTEL\0" // IR(23826, 27)
  "Vertex\0" // IR(23853, 7)
  "TessellationControl\0" // IR(23860, 20)
  "TessellationEvaluation\0" // IR(23880, 23)
  "GLCompute\0" // IR(23903, 10)
  "TaskNV\0" // IR(23913, 7)
  "MeshNV\0" // IR(23920, 7)
  "RayGenerationKHR\0" // IR(23927, 17)
  "RayGenerationNV\0" // IR(23944, 16)
  "IntersectionKHR\0" // IR(23960, 16)
  "IntersectionNV\0" // IR(23976, 15)
  "AnyHitKHR\0" // IR(23991, 10)
  "AnyHitNV\0" // IR(24001, 9)
  "ClosestHitKHR\0" // IR(24010, 14)
  "ClosestHitNV\0" // IR(24024, 13)
  "MissKHR\0" // IR(24037, 8)
  "MissNV\0" // IR(24045, 7)
  "CallableKHR\0" // IR(24052, 12)
  "CallableNV\0" // IR(24064, 11)
  "TaskEXT\0" // IR(24075, 8)
  "MeshEXT\0" // IR(24083, 8)
  "Preserve\0" // IR(24091, 9)
  "FlushToZero\0" // IR(24100, 12)
  "BFloat16KHR\0" // IR(24112, 12)
  "Float8E4M3EXT\0" // IR(24124, 14)
  "Float8E5M2EXT\0" // IR(24138, 14)
  "Float6E2M3EXT\0" // IR(24152, 14)
  "Float6E3M2EXT\0" // IR(24166, 14)
  "Float4E2M1EXT\0" // IR(24180, 14)
  "IEEE\0" // IR(24194, 5)
  "ALT\0" // IR(24199, 4)
  "NotNaN\0" // IR(24203, 7)
  "NotInf\0" // IR(24210, 7)
  "NSZ\0" // IR(24217, 4)
  "AllowRecip\0" // IR(24221, 11)
  "Fast\0" // IR(24232, 5)
  "AllowContract\0" // IR(24237, 14)
  "AllowContractFastINTEL\0" // IR(24251, 23)
  "AllowReassoc\0" // IR(24274, 13)
  "AllowReassocINTEL\0" // IR(24287, 18)
  "AllowTransform\0" // IR(24305, 15)
  "RTE\0" // IR(24320, 4)
  "RTZ\0" // IR(24324, 4)
  "RTP\0" // IR(24328, 4)
  "RTN\0" // IR(24332, 4)
  "Vertical2Pixels\0" // IR(24336, 16)
  "Vertical4Pixels\0" // IR(24352, 16)
  "Horizontal2Pixels\0" // IR(24368, 18)
  "Horizontal4Pixels\0" // IR(24386, 18)
  "Inline\0" // IR(24404, 7)
  "DontInline\0" // IR(24411, 11)
  "Pure\0" // IR(24422, 5)
  "Const\0" // IR(24427, 6)
  "Zext\0" // IR(24433, 5)
  "Sext\0" // IR(24438, 5)
  "ByVal\0" // IR(24443, 6)
  "Sret\0" // IR(24449, 5)
  "NoAlias\0" // IR(24454, 8)
  "NoCapture\0" // IR(24462, 10)
  "NoWrite\0" // IR(24472, 8)
  "NoReadWrite\0" // IR(24480, 12)
  "RuntimeAlignedALTERA\0" // IR(24492, 21)
  "RuntimeAlignedINTEL\0" // IR(24513, 20)
  "Gather4x1QCOM\0" // IR(24533, 14)
  "GatherDQCOM\0" // IR(24547, 12)
  "GatherH2QCOM\0" // IR(24559, 13)
  "GatherV2QCOM\0" // IR(24572, 13)
  "Reduce\0" // IR(24585, 7)
  "InclusiveScan\0" // IR(24592, 14)
  "ExclusiveScan\0" // IR(24606, 14)
  "ClusteredReduce\0" // IR(24620, 16)
  "PartitionedReduceEXT\0" // IR(24636, 21)
  "PartitionedReduceNV\0" // IR(24657, 20)
  "PartitionedInclusiveScanEXT\0" // IR(24677, 28)
  "PartitionedInclusiveScanNV\0" // IR(24705, 27)
  "PartitionedExclusiveScanEXT\0" // IR(24732, 28)
  "PartitionedExclusiveScanNV\0" // IR(24760, 27)
  "NoneINTEL\0" // IR(24787, 10)
  "ReadINTEL\0" // IR(24797, 10)
  "WriteINTEL\0" // IR(24807, 11)
  "ReadWriteINTEL\0" // IR(24818, 15)
  "Bias\0" // IR(24833, 5)
  "Lod\0" // IR(24838, 4)
  "Grad\0" // IR(24842, 5)
  "ConstOffset\0" // IR(24847, 12)
  "ConstOffsets\0" // IR(24859, 13)
  "MakeTexelAvailable\0" // IR(24872, 19)
  "MakeTexelAvailableKHR\0" // IR(24891, 22)
  "MakeTexelVisible\0" // IR(24913, 17)
  "MakeTexelVisibleKHR\0" // IR(24930, 20)
  "NonPrivateTexel\0" // IR(24950, 16)
  "NonPrivateTexelKHR\0" // IR(24966, 19)
  "VolatileTexel\0" // IR(24985, 14)
  "VolatileTexelKHR\0" // IR(24999, 17)
  "SignExtend\0" // IR(25016, 11)
  "ZeroExtend\0" // IR(25027, 11)
  "Nontemporal\0" // IR(25038, 12)
  "Offsets\0" // IR(25050, 8)
  "SnormInt8\0" // IR(25058, 10)
  "SnormInt16\0" // IR(25068, 11)
  "UnormInt8\0" // IR(25079, 10)
  "UnormInt16\0" // IR(25089, 11)
  "UnormShort565\0" // IR(25100, 14)
  "UnormShort555\0" // IR(25114, 14)
  "UnormInt101010\0" // IR(25128, 15)
  "SignedInt8\0" // IR(25143, 11)
  "SignedInt16\0" // IR(25154, 12)
  "SignedInt32\0" // IR(25166, 12)
  "UnsignedInt8\0" // IR(25178, 13)
  "UnsignedInt16\0" // IR(25191, 14)
  "UnsignedInt32\0" // IR(25205, 14)
  "HalfFloat\0" // IR(25219, 10)
  "UnormInt24\0" // IR(25229, 11)
  "UnormInt101010_2\0" // IR(25240, 17)
  "UnormInt10X6EXT\0" // IR(25257, 16)
  "UnsignedIntRaw10EXT\0" // IR(25273, 20)
  "UnsignedIntRaw12EXT\0" // IR(25293, 20)
  "UnormInt2_101010EXT\0" // IR(25313, 20)
  "UnsignedInt10X6EXT\0" // IR(25333, 19)
  "UnsignedInt12X4EXT\0" // IR(25352, 19)
  "UnsignedInt14X2EXT\0" // IR(25371, 19)
  "UnormInt12X4EXT\0" // IR(25390, 16)
  "UnormInt14X2EXT\0" // IR(25406, 16)
  "R\0" // IR(25422, 2)
  "A\0" // IR(25424, 2)
  "RG\0" // IR(25426, 3)
  "RA\0" // IR(25429, 3)
  "RGB\0" // IR(25432, 4)
  "RGBA\0" // IR(25436, 5)
  "BGRA\0" // IR(25441, 5)
  "ARGB\0" // IR(25446, 5)
  "Intensity\0" // IR(25451, 10)
  "Luminance\0" // IR(25461, 10)
  "Rx\0" // IR(25471, 3)
  "RGx\0" // IR(25474, 4)
  "RGBx\0" // IR(25478, 5)
  "Depth\0" // IR(25483, 6)
  "DepthStencil\0" // IR(25489, 13)
  "sRGB\0" // IR(25502, 5)
  "sRGBx\0" // IR(25507, 6)
  "sRGBA\0" // IR(25513, 6)
  "sBGRA\0" // IR(25519, 6)
  "ABGR\0" // IR(25525, 5)
  "InitOnDeviceReprogramALTERA\0" // IR(25530, 28)
  "InitOnDeviceReprogramINTEL\0" // IR(25558, 27)
  "InitOnDeviceResetALTERA\0" // IR(25585, 24)
  "InitOnDeviceResetINTEL\0" // IR(25609, 23)
  "NoWait\0" // IR(25632, 7)
  "WaitKernel\0" // IR(25639, 11)
  "WaitWorkGroup\0" // IR(25650, 14)
  "CmdExecTime\0" // IR(25664, 12)
  "MayUsePrintf\0" // IR(25676, 13)
  "Export\0" // IR(25689, 7)
  "Import\0" // IR(25696, 7)
  "LinkOnceODR\0" // IR(25703, 12)
  "WeakAMD\0" // IR(25715, 8)
  "UncachedINTEL\0" // IR(25723, 14)
  "CachedINTEL\0" // IR(25737, 12)
  "StreamingINTEL\0" // IR(25749, 15)
  "InvalidateAfterReadINTEL\0" // IR(25764, 25)
  "ConstCachedINTEL\0" // IR(25789, 17)
  "Unroll\0" // IR(25806, 7)
  "DontUnroll\0" // IR(25813, 11)
  "DependencyInfinite\0" // IR(25824, 19)
  "DependencyLength\0" // IR(25843, 17)
  "MinIterations\0" // IR(25860, 14)
  "MaxIterations\0" // IR(25874, 14)
  "IterationMultiple\0" // IR(25888, 18)
  "PeelCount\0" // IR(25906, 10)
  "PartialCount\0" // IR(25916, 13)
  "DependencyArrayALTERA\0" // IR(25929, 22)
  "DependencyArrayINTEL\0" // IR(25951, 21)
  "LoopCoalesceALTERA\0" // IR(25972, 19)
  "LoopCoalesceINTEL\0" // IR(25991, 18)
  "MaxInterleavingALTERA\0" // IR(26009, 22)
  "MaxInterleavingINTEL\0" // IR(26031, 21)
  "SpeculatedIterationsALTERA\0" // IR(26052, 27)
  "SpeculatedIterationsINTEL\0" // IR(26079, 26)
  "NoFusionALTERA\0" // IR(26105, 15)
  "NoFusionINTEL\0" // IR(26120, 14)
  "LoopCountALTERA\0" // IR(26134, 16)
  "LoopCountINTEL\0" // IR(26150, 15)
  "MaxReinvocationDelayALTERA\0" // IR(26165, 27)
  "MaxReinvocationDelayINTEL\0" // IR(26192, 26)
  "MatrixASignedComponentsINTEL\0" // IR(26218, 29)
  "MatrixBSignedComponentsINTEL\0" // IR(26247, 29)
  "MatrixCBFloat16INTEL\0" // IR(26276, 21)
  "MatrixResultBFloat16INTEL\0" // IR(26297, 26)
  "MatrixAPackedInt8INTEL\0" // IR(26323, 23)
  "MatrixBPackedInt8INTEL\0" // IR(26346, 23)
  "MatrixAPackedInt4INTEL\0" // IR(26369, 23)
  "MatrixBPackedInt4INTEL\0" // IR(26392, 23)
  "MatrixATF32INTEL\0" // IR(26415, 17)
  "MatrixBTF32INTEL\0" // IR(26432, 17)
  "MatrixAPackedFloat16INTEL\0" // IR(26449, 26)
  "MatrixBPackedFloat16INTEL\0" // IR(26475, 26)
  "MatrixAPackedBFloat16INTEL\0" // IR(26501, 27)
  "MatrixBPackedBFloat16INTEL\0" // IR(26528, 27)
  "Aligned\0" // IR(26555, 8)
  "MakePointerAvailable\0" // IR(26563, 21)
  "MakePointerAvailableKHR\0" // IR(26584, 24)
  "MakePointerVisible\0" // IR(26608, 19)
  "MakePointerVisibleKHR\0" // IR(26627, 22)
  "NonPrivatePointer\0" // IR(26649, 18)
  "NonPrivatePointerKHR\0" // IR(26667, 21)
  "AliasScopeINTELMask\0" // IR(26688, 20)
  "NoAliasINTELMask\0" // IR(26708, 17)
  "Simple\0" // IR(26725, 7)
  "GLSL450\0" // IR(26732, 8)
  "OpenCL\0" // IR(26740, 7)
  "Vulkan\0" // IR(26747, 7)
  "VulkanKHR\0" // IR(26754, 10)
  "Relaxed\0" // IR(26764, 8)
  "Acquire\0" // IR(26772, 8)
  "Release\0" // IR(26780, 8)
  "AcquireRelease\0" // IR(26788, 15)
  "SequentiallyConsistent\0" // IR(26803, 23)
  "UniformMemory\0" // IR(26826, 14)
  "SubgroupMemory\0" // IR(26840, 15)
  "WorkgroupMemory\0" // IR(26855, 16)
  "CrossWorkgroupMemory\0" // IR(26871, 21)
  "AtomicCounterMemory\0" // IR(26892, 20)
  "ImageMemory\0" // IR(26912, 12)
  "OutputMemory\0" // IR(26924, 13)
  "OutputMemoryKHR\0" // IR(26937, 16)
  "MakeAvailable\0" // IR(26953, 14)
  "MakeAvailableKHR\0" // IR(26967, 17)
  "MakeVisible\0" // IR(26984, 12)
  "MakeVisibleKHR\0" // IR(26996, 15)
  "AutoINTEL\0" // IR(27011, 10)
  "WRAP\0" // IR(27021, 5)
  "SAT\0" // IR(27026, 4)
  "SAT_ZERO\0" // IR(27030, 9)
  "SAT_SYM\0" // IR(27039, 8)
  "PackedVectorFormat4x8Bit\0" // IR(27047, 25)
  "PackedVectorFormat4x8BitKHR\0" // IR(27072, 28)
  "TRN\0" // IR(27100, 4)
  "TRN_ZERO\0" // IR(27104, 9)
  "RND\0" // IR(27113, 4)
  "RND_ZERO\0" // IR(27117, 9)
  "RND_INF\0" // IR(27126, 8)
  "RND_MIN_INF\0" // IR(27134, 12)
  "RND_CONV\0" // IR(27146, 9)
  "RND_CONV_ODD\0" // IR(27155, 13)
  "RobustnessPerComponentNV\0" // IR(27168, 25)
  "RobustnessPerElementNV\0" // IR(27193, 23)
  "OpaqueKHR\0" // IR(27216, 10)
  "NoOpaqueKHR\0" // IR(27226, 12)
  "TerminateOnFirstHitKHR\0" // IR(27238, 23)
  "SkipClosestHitShaderKHR\0" // IR(27261, 24)
  "CullBackFacingTrianglesKHR\0" // IR(27285, 27)
  "CullFrontFacingTrianglesKHR\0" // IR(27312, 28)
  "CullOpaqueKHR\0" // IR(27340, 14)
  "CullNoOpaqueKHR\0" // IR(27354, 16)
  "SkipTrianglesKHR\0" // IR(27370, 17)
  "SkipBuiltinPrimitivesNV\0" // IR(27387, 24)
  "SkipAABBsKHR\0" // IR(27411, 13)
  "ForceOpacityMicromap2StateKHR\0" // IR(27424, 30)
  "ForceOpacityMicromap2StateEXT\0" // IR(27454, 30)
  "RayQueryCandidateIntersectionTriangleKHR\0" // IR(27484, 41)
  "RayQueryCandidateIntersectionAABBKHR\0" // IR(27525, 37)
  "RayQueryCommittedIntersectionNoneKHR\0" // IR(27562, 37)
  "RayQueryCommittedIntersectionTriangleKHR\0" // IR(27599, 41)
  "RayQueryCommittedIntersectionGeneratedKHR\0" // IR(27640, 42)
  "RayQueryCandidateIntersectionKHR\0" // IR(27682, 33)
  "RayQueryCommittedIntersectionKHR\0" // IR(27715, 33)
  "ClampToEdge\0" // IR(27748, 12)
  "Clamp\0" // IR(27760, 6)
  "Repeat\0" // IR(27766, 7)
  "RepeatMirrored\0" // IR(27773, 15)
  "Nearest\0" // IR(27788, 8)
  "Linear\0" // IR(27796, 7)
  "Unknown\0" // IR(27803, 8)
  "Rgba32f\0" // IR(27811, 8)
  "Rgba16f\0" // IR(27819, 8)
  "R32f\0" // IR(27827, 5)
  "Rgba8\0" // IR(27832, 6)
  "Rgba8Snorm\0" // IR(27838, 11)
  "Rg32f\0" // IR(27849, 6)
  "Rg16f\0" // IR(27855, 6)
  "R11fG11fB10f\0" // IR(27861, 13)
  "R16f\0" // IR(27874, 5)
  "Rgba16\0" // IR(27879, 7)
  "Rgb10A2\0" // IR(27886, 8)
  "Rg16\0" // IR(27894, 5)
  "Rg8\0" // IR(27899, 4)
  "R16\0" // IR(27903, 4)
  "R8\0" // IR(27907, 3)
  "Rgba16Snorm\0" // IR(27910, 12)
  "Rg16Snorm\0" // IR(27922, 10)
  "Rg8Snorm\0" // IR(27932, 9)
  "R16Snorm\0" // IR(27941, 9)
  "R8Snorm\0" // IR(27950, 8)
  "Rgba32i\0" // IR(27958, 8)
  "Rgba16i\0" // IR(27966, 8)
  "Rgba8i\0" // IR(27974, 7)
  "R32i\0" // IR(27981, 5)
  "Rg32i\0" // IR(27986, 6)
  "Rg16i\0" // IR(27992, 6)
  "Rg8i\0" // IR(27998, 5)
  "R16i\0" // IR(28003, 5)
  "R8i\0" // IR(28008, 4)
  "Rgba32ui\0" // IR(28012, 9)
  "Rgba16ui\0" // IR(28021, 9)
  "Rgba8ui\0" // IR(28030, 8)
  "R32ui\0" // IR(28038, 6)
  "Rgb10a2ui\0" // IR(28044, 10)
  "Rg32ui\0" // IR(28054, 7)
  "Rg16ui\0" // IR(28061, 7)
  "Rg8ui\0" // IR(28068, 6)
  "R16ui\0" // IR(28074, 6)
  "R8ui\0" // IR(28080, 5)
  "R64ui\0" // IR(28085, 6)
  "R64i\0" // IR(28091, 5)
  "CrossDevice\0" // IR(28096, 12)
  "Device\0" // IR(28108, 7)
  "Workgroup\0" // IR(28115, 10)
  "Subgroup\0" // IR(28125, 9)
  "Invocation\0" // IR(28134, 11)
  "QueueFamily\0" // IR(28145, 12)
  "QueueFamilyKHR\0" // IR(28157, 15)
  "ShaderCallKHR\0" // IR(28172, 14)
  "Flatten\0" // IR(28186, 8)
  "DontFlatten\0" // IR(28194, 12)
  "IdentifierPossibleDuplicates\0" // IR(28206, 29)
  "FlagUnknownPhysicalLayout\0" // IR(28235, 26)
  "ESSL\0" // IR(28261, 5)
  "GLSL\0" // IR(28266, 5)
  "OpenCL_C\0" // IR(28271, 9)
  "OpenCL_CPP\0" // IR(28280, 11)
  "HLSL\0" // IR(28291, 5)
  "CPP_for_OpenCL\0" // IR(28296, 15)
  "SYCL\0" // IR(28311, 5)
  "HERO_C\0" // IR(28316, 7)
  "NZSL\0" // IR(28323, 5)
  "WGSL\0" // IR(28328, 5)
  "Slang\0" // IR(28333, 6)
  "Zig\0" // IR(28339, 4)
  "Rust\0" // IR(28343, 5)
  "Pred\0" // IR(28348, 5)
  "ApilaJai\0" // IR(28353, 9)
  "UniformConstant\0" // IR(28362, 16)
  "Input\0" // IR(28378, 6)
  "Output\0" // IR(28384, 7)
  "CrossWorkgroup\0" // IR(28391, 15)
  "Private\0" // IR(28406, 8)
  "Function\0" // IR(28414, 9)
  "Generic\0" // IR(28423, 8)
  "PushConstant\0" // IR(28431, 13)
  "AtomicCounter\0" // IR(28444, 14)
  "Image\0" // IR(28458, 6)
  "StorageBuffer\0" // IR(28464, 14)
  "TileImageEXT\0" // IR(28478, 13)
  "TileAttachmentQCOM\0" // IR(28491, 19)
  "NodePayloadAMDX\0" // IR(28510, 16)
  "CallableDataKHR\0" // IR(28526, 16)
  "CallableDataNV\0" // IR(28542, 15)
  "IncomingCallableDataKHR\0" // IR(28557, 24)
  "IncomingCallableDataNV\0" // IR(28581, 23)
  "RayPayloadKHR\0" // IR(28604, 14)
  "RayPayloadNV\0" // IR(28618, 13)
  "HitAttributeKHR\0" // IR(28631, 16)
  "HitAttributeNV\0" // IR(28647, 15)
  "IncomingRayPayloadKHR\0" // IR(28662, 22)
  "IncomingRayPayloadNV\0" // IR(28684, 21)
  "ShaderRecordBufferKHR\0" // IR(28705, 22)
  "ShaderRecordBufferNV\0" // IR(28727, 21)
  "PhysicalStorageBuffer\0" // IR(28748, 22)
  "PhysicalStorageBufferEXT\0" // IR(28770, 25)
  "HitObjectAttributeNV\0" // IR(28795, 21)
  "TaskPayloadWorkgroupEXT\0" // IR(28816, 24)
  "HitObjectAttributeEXT\0" // IR(28840, 22)
  "CodeSectionINTEL\0" // IR(28862, 17)
  "DeviceOnlyALTERA\0" // IR(28879, 17)
  "DeviceOnlyINTEL\0" // IR(28896, 16)
  "HostOnlyALTERA\0" // IR(28912, 15)
  "HostOnlyINTEL\0" // IR(28927, 14)
  "WriteThroughINTEL\0" // IR(28941, 18)
  "WriteBackINTEL\0" // IR(28959, 15)
  "TensorView\0" // IR(28974, 11)
  "DecodeFunc\0" // IR(28985, 11)
  "DecodeVectorFunc\0" // IR(28996, 17)
  "Undefined\0" // IR(29013, 10)
  "NoneARM\0" // IR(29023, 8)
  "NontemporalARM\0" // IR(29031, 15)
  "OutOfBoundsValueARM\0" // IR(29046, 20)
  "MakeElementAvailableARM\0" // IR(29066, 24)
  "MakeElementVisibleARM\0" // IR(29090, 22)
  "NonPrivateElementARM\0" // IR(29112, 21)
  "Nop\0" // IR(29133, 4)
  "SPV_OPERAND_TYPE_TYPE_ID\0" // IR(29137, 25)
  "SPV_OPERAND_TYPE_RESULT_ID\0" // IR(29162, 27)
  "Undef\0" // IR(29189, 6)
  "SourceContinued\0" // IR(29195, 16)
  "SPV_OPERAND_TYPE_SOURCE_LANGUAGE\0" // IR(29211, 33)
  "SPV_OPERAND_TYPE_OPTIONAL_ID\0" // IR(29244, 29)
  "SPV_OPERAND_TYPE_OPTIONAL_LITERAL_STRING\0" // IR(29273, 41)
  "Source\0" // IR(29314, 7)
  "SourceExtension\0" // IR(29321, 16)
  "Name\0" // IR(29337, 5)
  "MemberName\0" // IR(29342, 11)
  "String\0" // IR(29353, 7)
  "Line\0" // IR(29360, 5)
  "Extension\0" // IR(29365, 10)
  "ExtInstImport\0" // IR(29375, 14)
  "SPV_OPERAND_TYPE_EXTENSION_INSTRUCTION_NUMBER\0" // IR(29389, 46)
  "ExtInst\0" // IR(29435, 8)
  "SPV_OPERAND_TYPE_ADDRESSING_MODEL\0" // IR(29443, 34)
  "SPV_OPERAND_TYPE_MEMORY_MODEL\0" // IR(29477, 30)
  "MemoryModel\0" // IR(29507, 12)
  "SPV_OPERAND_TYPE_EXECUTION_MODEL\0" // IR(29519, 33)
  "SPV_OPERAND_TYPE_VARIABLE_ID\0" // IR(29552, 29)
  "EntryPoint\0" // IR(29581, 11)
  "SPV_OPERAND_TYPE_EXECUTION_MODE\0" // IR(29592, 32)
  "ExecutionMode\0" // IR(29624, 14)
  "SPV_OPERAND_TYPE_CAPABILITY\0" // IR(29638, 28)
  "Capability\0" // IR(29666, 11)
  "TypeVoid\0" // IR(29677, 9)
  "TypeBool\0" // IR(29686, 9)
  "TypeInt\0" // IR(29695, 8)
  "SPV_OPERAND_TYPE_OPTIONAL_FPENCODING\0" // IR(29703, 37)
  "TypeFloat\0" // IR(29740, 10)
  "TypeVector\0" // IR(29750, 11)
  "TypeMatrix\0" // IR(29761, 11)
  "SPV_OPERAND_TYPE_DIMENSIONALITY\0" // IR(29772, 32)
  "SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT\0" // IR(29804, 38)
  "SPV_OPERAND_TYPE_OPTIONAL_ACCESS_QUALIFIER\0" // IR(29842, 43)
  "TypeImage\0" // IR(29885, 10)
  "TypeSampler\0" // IR(29895, 12)
  "TypeSampledImage\0" // IR(29907, 17)
  "TypeArray\0" // IR(29924, 10)
  "TypeRuntimeArray\0" // IR(29934, 17)
  "TypeStruct\0" // IR(29951, 11)
  "TypeOpaque\0" // IR(29962, 11)
  "SPV_OPERAND_TYPE_STORAGE_CLASS\0" // IR(29973, 31)
  "TypePointer\0" // IR(30004, 12)
  "TypeFunction\0" // IR(30016, 13)
  "TypeEvent\0" // IR(30029, 10)
  "TypeDeviceEvent\0" // IR(30039, 16)
  "TypeReserveId\0" // IR(30055, 14)
  "TypeQueue\0" // IR(30069, 10)
  "TypePipe\0" // IR(30079, 9)
  "TypeForwardPointer\0" // IR(30088, 19)
  "ConstantTrue\0" // IR(30107, 13)
  "ConstantFalse\0" // IR(30120, 14)
  "SPV_OPERAND_TYPE_TYPED_LITERAL_NUMBER\0" // IR(30134, 38)
  "ConstantComposite\0" // IR(30172, 18)
  "SPV_OPERAND_TYPE_SAMPLER_ADDRESSING_MODE\0" // IR(30190, 41)
  "SPV_OPERAND_TYPE_SAMPLER_FILTER_MODE\0" // IR(30231, 37)
  "ConstantSampler\0" // IR(30268, 16)
  "ConstantNull\0" // IR(30284, 13)
  "SpecConstantTrue\0" // IR(30297, 17)
  "SpecConstantFalse\0" // IR(30314, 18)
  "SpecConstant\0" // IR(30332, 13)
  "SpecConstantComposite\0" // IR(30345, 22)
  "SPV_OPERAND_TYPE_SPEC_CONSTANT_OP_NUMBER\0" // IR(30367, 41)
  "SpecConstantOp\0" // IR(30408, 15)
  "SPV_OPERAND_TYPE_FUNCTION_CONTROL\0" // IR(30423, 34)
  "FunctionParameter\0" // IR(30457, 18)
  "FunctionEnd\0" // IR(30475, 12)
  "FunctionCall\0" // IR(30487, 13)
  "Variable\0" // IR(30500, 9)
  "ImageTexelPointer\0" // IR(30509, 18)
  "SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS\0" // IR(30527, 40)
  "Load\0" // IR(30567, 5)
  "Store\0" // IR(30572, 6)
  "CopyMemory\0" // IR(30578, 11)
  "CopyMemorySized\0" // IR(30589, 16)
  "AccessChain\0" // IR(30605, 12)
  "InBoundsAccessChain\0" // IR(30617, 20)
  "PtrAccessChain\0" // IR(30637, 15)
  "ArrayLength\0" // IR(30652, 12)
  "GenericPtrMemSemantics\0" // IR(30664, 23)
  "InBoundsPtrAccessChain\0" // IR(30687, 23)
  "SPV_OPERAND_TYPE_DECORATION\0" // IR(30710, 28)
  "Decorate\0" // IR(30738, 9)
  "MemberDecorate\0" // IR(30747, 15)
  "DecorationGroup\0" // IR(30762, 16)
  "GroupDecorate\0" // IR(30778, 14)
  "SPV_OPERAND_TYPE_VARIABLE_ID_LITERAL_INTEGER\0" // IR(30792, 45)
  "GroupMemberDecorate\0" // IR(30837, 20)
  "VectorExtractDynamic\0" // IR(30857, 21)
  "VectorInsertDynamic\0" // IR(30878, 20)
  "VectorShuffle\0" // IR(30898, 14)
  "CompositeConstruct\0" // IR(30912, 19)
  "CompositeExtract\0" // IR(30931, 17)
  "CompositeInsert\0" // IR(30948, 16)
  "CopyObject\0" // IR(30964, 11)
  "Transpose\0" // IR(30975, 10)
  "SampledImage\0" // IR(30985, 13)
  "SPV_OPERAND_TYPE_OPTIONAL_IMAGE\0" // IR(30998, 32)
  "ImageSampleImplicitLod\0" // IR(31030, 23)
  "SPV_OPERAND_TYPE_IMAGE\0" // IR(31053, 23)
  "ImageSampleExplicitLod\0" // IR(31076, 23)
  "ImageSampleDrefImplicitLod\0" // IR(31099, 27)
  "ImageSampleDrefExplicitLod\0" // IR(31126, 27)
  "ImageSampleProjImplicitLod\0" // IR(31153, 27)
  "ImageSampleProjExplicitLod\0" // IR(31180, 27)
  "ImageSampleProjDrefImplicitLod\0" // IR(31207, 31)
  "ImageSampleProjDrefExplicitLod\0" // IR(31238, 31)
  "ImageFetch\0" // IR(31269, 11)
  "ImageGather\0" // IR(31280, 12)
  "ImageDrefGather\0" // IR(31292, 16)
  "ImageRead\0" // IR(31308, 10)
  "ImageWrite\0" // IR(31318, 11)
  "ImageQueryFormat\0" // IR(31329, 17)
  "ImageQueryOrder\0" // IR(31346, 16)
  "ImageQuerySizeLod\0" // IR(31362, 18)
  "ImageQuerySize\0" // IR(31380, 15)
  "ImageQueryLod\0" // IR(31395, 14)
  "ImageQueryLevels\0" // IR(31409, 17)
  "ImageQuerySamples\0" // IR(31426, 18)
  "ConvertFToU\0" // IR(31444, 12)
  "ConvertFToS\0" // IR(31456, 12)
  "ConvertSToF\0" // IR(31468, 12)
  "ConvertUToF\0" // IR(31480, 12)
  "UConvert\0" // IR(31492, 9)
  "SConvert\0" // IR(31501, 9)
  "FConvert\0" // IR(31510, 9)
  "QuantizeToF16\0" // IR(31519, 14)
  "ConvertPtrToU\0" // IR(31533, 14)
  "SatConvertSToU\0" // IR(31547, 15)
  "SatConvertUToS\0" // IR(31562, 15)
  "ConvertUToPtr\0" // IR(31577, 14)
  "PtrCastToGeneric\0" // IR(31591, 17)
  "GenericCastToPtr\0" // IR(31608, 17)
  "GenericCastToPtrExplicit\0" // IR(31625, 25)
  "Bitcast\0" // IR(31650, 8)
  "SNegate\0" // IR(31658, 8)
  "FNegate\0" // IR(31666, 8)
  "IAdd\0" // IR(31674, 5)
  "FAdd\0" // IR(31679, 5)
  "ISub\0" // IR(31684, 5)
  "FSub\0" // IR(31689, 5)
  "IMul\0" // IR(31694, 5)
  "FMul\0" // IR(31699, 5)
  "UDiv\0" // IR(31704, 5)
  "SDiv\0" // IR(31709, 5)
  "FDiv\0" // IR(31714, 5)
  "UMod\0" // IR(31719, 5)
  "SRem\0" // IR(31724, 5)
  "SMod\0" // IR(31729, 5)
  "FRem\0" // IR(31734, 5)
  "FMod\0" // IR(31739, 5)
  "VectorTimesScalar\0" // IR(31744, 18)
  "MatrixTimesScalar\0" // IR(31762, 18)
  "VectorTimesMatrix\0" // IR(31780, 18)
  "MatrixTimesVector\0" // IR(31798, 18)
  "MatrixTimesMatrix\0" // IR(31816, 18)
  "OuterProduct\0" // IR(31834, 13)
  "Dot\0" // IR(31847, 4)
  "IAddCarry\0" // IR(31851, 10)
  "ISubBorrow\0" // IR(31861, 11)
  "UMulExtended\0" // IR(31872, 13)
  "SMulExtended\0" // IR(31885, 13)
  "Any\0" // IR(31898, 4)
  "All\0" // IR(31902, 4)
  "IsNan\0" // IR(31906, 6)
  "IsInf\0" // IR(31912, 6)
  "IsFinite\0" // IR(31918, 9)
  "IsNormal\0" // IR(31927, 9)
  "SignBitSet\0" // IR(31936, 11)
  "LessOrGreater\0" // IR(31947, 14)
  "Ordered\0" // IR(31961, 8)
  "Unordered\0" // IR(31969, 10)
  "LogicalEqual\0" // IR(31979, 13)
  "LogicalNotEqual\0" // IR(31992, 16)
  "LogicalOr\0" // IR(32008, 10)
  "LogicalAnd\0" // IR(32018, 11)
  "LogicalNot\0" // IR(32029, 11)
  "Select\0" // IR(32040, 7)
  "IEqual\0" // IR(32047, 7)
  "INotEqual\0" // IR(32054, 10)
  "UGreaterThan\0" // IR(32064, 13)
  "SGreaterThan\0" // IR(32077, 13)
  "UGreaterThanEqual\0" // IR(32090, 18)
  "SGreaterThanEqual\0" // IR(32108, 18)
  "ULessThan\0" // IR(32126, 10)
  "SLessThan\0" // IR(32136, 10)
  "ULessThanEqual\0" // IR(32146, 15)
  "SLessThanEqual\0" // IR(32161, 15)
  "FOrdEqual\0" // IR(32176, 10)
  "FUnordEqual\0" // IR(32186, 12)
  "FOrdNotEqual\0" // IR(32198, 13)
  "FUnordNotEqual\0" // IR(32211, 15)
  "FOrdLessThan\0" // IR(32226, 13)
  "FUnordLessThan\0" // IR(32239, 15)
  "FOrdGreaterThan\0" // IR(32254, 16)
  "FUnordGreaterThan\0" // IR(32270, 18)
  "FOrdLessThanEqual\0" // IR(32288, 18)
  "FUnordLessThanEqual\0" // IR(32306, 20)
  "FOrdGreaterThanEqual\0" // IR(32326, 21)
  "FUnordGreaterThanEqual\0" // IR(32347, 23)
  "ShiftRightLogical\0" // IR(32370, 18)
  "ShiftRightArithmetic\0" // IR(32388, 21)
  "ShiftLeftLogical\0" // IR(32409, 17)
  "BitwiseOr\0" // IR(32426, 10)
  "BitwiseXor\0" // IR(32436, 11)
  "BitwiseAnd\0" // IR(32447, 11)
  "Not\0" // IR(32458, 4)
  "BitFieldInsert\0" // IR(32462, 15)
  "BitFieldSExtract\0" // IR(32477, 17)
  "BitFieldUExtract\0" // IR(32494, 17)
  "BitReverse\0" // IR(32511, 11)
  "BitCount\0" // IR(32522, 9)
  "DPdx\0" // IR(32531, 5)
  "DPdy\0" // IR(32536, 5)
  "Fwidth\0" // IR(32541, 7)
  "DPdxFine\0" // IR(32548, 9)
  "DPdyFine\0" // IR(32557, 9)
  "FwidthFine\0" // IR(32566, 11)
  "DPdxCoarse\0" // IR(32577, 11)
  "DPdyCoarse\0" // IR(32588, 11)
  "FwidthCoarse\0" // IR(32599, 13)
  "EmitVertex\0" // IR(32612, 11)
  "EndPrimitive\0" // IR(32623, 13)
  "EmitStreamVertex\0" // IR(32636, 17)
  "EndStreamPrimitive\0" // IR(32653, 19)
  "SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID\0" // IR(32672, 37)
  "ControlBarrier\0" // IR(32709, 15)
  "MemoryBarrier\0" // IR(32724, 14)
  "AtomicLoad\0" // IR(32738, 11)
  "AtomicStore\0" // IR(32749, 12)
  "AtomicExchange\0" // IR(32761, 15)
  "AtomicCompareExchange\0" // IR(32776, 22)
  "AtomicCompareExchangeWeak\0" // IR(32798, 26)
  "AtomicIIncrement\0" // IR(32824, 17)
  "AtomicIDecrement\0" // IR(32841, 17)
  "AtomicIAdd\0" // IR(32858, 11)
  "AtomicISub\0" // IR(32869, 11)
  "AtomicSMin\0" // IR(32880, 11)
  "AtomicUMin\0" // IR(32891, 11)
  "AtomicSMax\0" // IR(32902, 11)
  "AtomicUMax\0" // IR(32913, 11)
  "AtomicAnd\0" // IR(32924, 10)
  "AtomicOr\0" // IR(32934, 9)
  "AtomicXor\0" // IR(32943, 10)
  "Phi\0" // IR(32953, 4)
  "SPV_OPERAND_TYPE_LOOP_CONTROL\0" // IR(32957, 30)
  "LoopMerge\0" // IR(32987, 10)
  "SPV_OPERAND_TYPE_SELECTION_CONTROL\0" // IR(32997, 35)
  "SelectionMerge\0" // IR(33032, 15)
  "Label\0" // IR(33047, 6)
  "Branch\0" // IR(33053, 7)
  "BranchConditional\0" // IR(33060, 18)
  "SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER_ID\0" // IR(33078, 45)
  "Switch\0" // IR(33123, 7)
  "Kill\0" // IR(33130, 5)
  "Return\0" // IR(33135, 7)
  "ReturnValue\0" // IR(33142, 12)
  "Unreachable\0" // IR(33154, 12)
  "LifetimeStart\0" // IR(33166, 14)
  "LifetimeStop\0" // IR(33180, 13)
  "GroupAsyncCopy\0" // IR(33193, 15)
  "GroupWaitEvents\0" // IR(33208, 16)
  "GroupAll\0" // IR(33224, 9)
  "GroupAny\0" // IR(33233, 9)
  "GroupBroadcast\0" // IR(33242, 15)
  "SPV_OPERAND_TYPE_GROUP_OPERATION\0" // IR(33257, 33)
  "GroupIAdd\0" // IR(33290, 10)
  "GroupFAdd\0" // IR(33300, 10)
  "GroupFMin\0" // IR(33310, 10)
  "GroupUMin\0" // IR(33320, 10)
  "GroupSMin\0" // IR(33330, 10)
  "GroupFMax\0" // IR(33340, 10)
  "GroupUMax\0" // IR(33350, 10)
  "GroupSMax\0" // IR(33360, 10)
  "ReadPipe\0" // IR(33370, 9)
  "WritePipe\0" // IR(33379, 10)
  "ReservedReadPipe\0" // IR(33389, 17)
  "ReservedWritePipe\0" // IR(33406, 18)
  "ReserveReadPipePackets\0" // IR(33424, 23)
  "ReserveWritePipePackets\0" // IR(33447, 24)
  "CommitReadPipe\0" // IR(33471, 15)
  "CommitWritePipe\0" // IR(33486, 16)
  "IsValidReserveId\0" // IR(33502, 17)
  "GetNumPipePackets\0" // IR(33519, 18)
  "GetMaxPipePackets\0" // IR(33537, 18)
  "GroupReserveReadPipePackets\0" // IR(33555, 28)
  "GroupReserveWritePipePackets\0" // IR(33583, 29)
  "GroupCommitReadPipe\0" // IR(33612, 20)
  "GroupCommitWritePipe\0" // IR(33632, 21)
  "EnqueueMarker\0" // IR(33653, 14)
  "EnqueueKernel\0" // IR(33667, 14)
  "GetKernelNDrangeSubGroupCount\0" // IR(33681, 30)
  "GetKernelNDrangeMaxSubGroupSize\0" // IR(33711, 32)
  "GetKernelWorkGroupSize\0" // IR(33743, 23)
  "GetKernelPreferredWorkGroupSizeMultiple\0" // IR(33766, 40)
  "RetainEvent\0" // IR(33806, 12)
  "ReleaseEvent\0" // IR(33818, 13)
  "CreateUserEvent\0" // IR(33831, 16)
  "IsValidEvent\0" // IR(33847, 13)
  "SetUserEventStatus\0" // IR(33860, 19)
  "CaptureEventProfilingInfo\0" // IR(33879, 26)
  "GetDefaultQueue\0" // IR(33905, 16)
  "BuildNDRange\0" // IR(33921, 13)
  "ImageSparseSampleImplicitLod\0" // IR(33934, 29)
  "ImageSparseSampleExplicitLod\0" // IR(33963, 29)
  "ImageSparseSampleDrefImplicitLod\0" // IR(33992, 33)
  "ImageSparseSampleDrefExplicitLod\0" // IR(34025, 33)
  "ImageSparseSampleProjImplicitLod\0" // IR(34058, 33)
  "ImageSparseSampleProjExplicitLod\0" // IR(34091, 33)
  "ImageSparseSampleProjDrefImplicitLod\0" // IR(34124, 37)
  "ImageSparseSampleProjDrefExplicitLod\0" // IR(34161, 37)
  "ImageSparseFetch\0" // IR(34198, 17)
  "ImageSparseGather\0" // IR(34215, 18)
  "ImageSparseDrefGather\0" // IR(34233, 22)
  "ImageSparseTexelsResident\0" // IR(34255, 26)
  "NoLine\0" // IR(34281, 7)
  "AtomicFlagTestAndSet\0" // IR(34288, 21)
  "AtomicFlagClear\0" // IR(34309, 16)
  "ImageSparseRead\0" // IR(34325, 16)
  "SizeOf\0" // IR(34341, 7)
  "TypePipeStorage\0" // IR(34348, 16)
  "ConstantPipeStorage\0" // IR(34364, 20)
  "CreatePipeFromPipeStorage\0" // IR(34384, 26)
  "GetKernelLocalSizeForSubgroupCount\0" // IR(34410, 35)
  "GetKernelMaxNumSubgroups\0" // IR(34445, 25)
  "TypeNamedBarrier\0" // IR(34470, 17)
  "NamedBarrierInitialize\0" // IR(34487, 23)
  "MemoryNamedBarrier\0" // IR(34510, 19)
  "ModuleProcessed\0" // IR(34529, 16)
  "ExecutionModeId\0" // IR(34545, 16)
  "DecorateId\0" // IR(34561, 11)
  "GroupNonUniformElect\0" // IR(34572, 21)
  "GroupNonUniformAll\0" // IR(34593, 19)
  "GroupNonUniformAny\0" // IR(34612, 19)
  "GroupNonUniformAllEqual\0" // IR(34631, 24)
  "GroupNonUniformBroadcast\0" // IR(34655, 25)
  "GroupNonUniformBroadcastFirst\0" // IR(34680, 30)
  "GroupNonUniformInverseBallot\0" // IR(34710, 29)
  "GroupNonUniformBallotBitExtract\0" // IR(34739, 32)
  "GroupNonUniformBallotBitCount\0" // IR(34771, 30)
  "GroupNonUniformBallotFindLSB\0" // IR(34801, 29)
  "GroupNonUniformBallotFindMSB\0" // IR(34830, 29)
  "GroupNonUniformShuffleXor\0" // IR(34859, 26)
  "GroupNonUniformShuffleUp\0" // IR(34885, 25)
  "GroupNonUniformShuffleDown\0" // IR(34910, 27)
  "GroupNonUniformIAdd\0" // IR(34937, 20)
  "GroupNonUniformFAdd\0" // IR(34957, 20)
  "GroupNonUniformIMul\0" // IR(34977, 20)
  "GroupNonUniformFMul\0" // IR(34997, 20)
  "GroupNonUniformSMin\0" // IR(35017, 20)
  "GroupNonUniformUMin\0" // IR(35037, 20)
  "GroupNonUniformFMin\0" // IR(35057, 20)
  "GroupNonUniformSMax\0" // IR(35077, 20)
  "GroupNonUniformUMax\0" // IR(35097, 20)
  "GroupNonUniformFMax\0" // IR(35117, 20)
  "GroupNonUniformBitwiseAnd\0" // IR(35137, 26)
  "GroupNonUniformBitwiseOr\0" // IR(35163, 25)
  "GroupNonUniformBitwiseXor\0" // IR(35188, 26)
  "GroupNonUniformLogicalAnd\0" // IR(35214, 26)
  "GroupNonUniformLogicalOr\0" // IR(35240, 25)
  "GroupNonUniformLogicalXor\0" // IR(35265, 26)
  "GroupNonUniformQuadBroadcast\0" // IR(35291, 29)
  "GroupNonUniformQuadSwap\0" // IR(35320, 24)
  "CopyLogical\0" // IR(35344, 12)
  "PtrEqual\0" // IR(35356, 9)
  "PtrNotEqual\0" // IR(35365, 12)
  "PtrDiff\0" // IR(35377, 8)
  "ColorAttachmentReadEXT\0" // IR(35385, 23)
  "DepthAttachmentReadEXT\0" // IR(35408, 23)
  "StencilAttachmentReadEXT\0" // IR(35431, 25)
  "TypeTensorARM\0" // IR(35456, 14)
  "SPV_OPERAND_TYPE_OPTIONAL_TENSOR_OPERANDS\0" // IR(35470, 42)
  "TensorReadARM\0" // IR(35512, 14)
  "TensorWriteARM\0" // IR(35526, 15)
  "TensorQuerySizeARM\0" // IR(35541, 19)
  "GraphConstantARM\0" // IR(35560, 17)
  "GraphEntryPointARM\0" // IR(35577, 19)
  "GraphInputARM\0" // IR(35596, 14)
  "GraphSetOutputARM\0" // IR(35610, 18)
  "GraphEndARM\0" // IR(35628, 12)
  "TypeGraphARM\0" // IR(35640, 13)
  "TerminateInvocation\0" // IR(35653, 20)
  "TypeUntypedPointerKHR\0" // IR(35673, 22)
  "UntypedVariableKHR\0" // IR(35695, 19)
  "UntypedAccessChainKHR\0" // IR(35714, 22)
  "UntypedInBoundsAccessChainKHR\0" // IR(35736, 30)
  "SubgroupFirstInvocationKHR\0" // IR(35766, 27)
  "UntypedPtrAccessChainKHR\0" // IR(35793, 25)
  "UntypedInBoundsPtrAccessChainKHR\0" // IR(35818, 33)
  "UntypedArrayLengthKHR\0" // IR(35851, 22)
  "UntypedPrefetchKHR\0" // IR(35873, 19)
  "FmaKHR\0" // IR(35892, 7)
  "SubgroupAllKHR\0" // IR(35899, 15)
  "SubgroupAnyKHR\0" // IR(35914, 15)
  "SubgroupAllEqualKHR\0" // IR(35929, 20)
  "SubgroupReadInvocationKHR\0" // IR(35949, 26)
  "ExtInstWithForwardRefsKHR\0" // IR(35975, 26)
  "UntypedGroupAsyncCopyKHR\0" // IR(36001, 25)
  "TraceRayKHR\0" // IR(36026, 12)
  "ExecuteCallableKHR\0" // IR(36038, 19)
  "ConvertUToAccelerationStructureKHR\0" // IR(36057, 35)
  "IgnoreIntersectionKHR\0" // IR(36092, 22)
  "TerminateRayKHR\0" // IR(36114, 16)
  "SPV_OPERAND_TYPE_OPTIONAL_PACKED_VECTOR_FORMAT\0" // IR(36130, 47)
  "SDot\0" // IR(36177, 5)
  "SDotKHR\0" // IR(36182, 8)
  "UDot\0" // IR(36190, 5)
  "UDotKHR\0" // IR(36195, 8)
  "SUDot\0" // IR(36203, 6)
  "SUDotKHR\0" // IR(36209, 9)
  "SDotAccSat\0" // IR(36218, 11)
  "SDotAccSatKHR\0" // IR(36229, 14)
  "UDotAccSat\0" // IR(36243, 11)
  "UDotAccSatKHR\0" // IR(36254, 14)
  "SUDotAccSat\0" // IR(36268, 12)
  "SUDotAccSatKHR\0" // IR(36280, 15)
  "TypeCooperativeMatrixKHR\0" // IR(36295, 25)
  "CooperativeMatrixLoadKHR\0" // IR(36320, 25)
  "CooperativeMatrixStoreKHR\0" // IR(36345, 26)
  "SPV_OPERAND_TYPE_OPTIONAL_COOPERATIVE_MATRIX_OPERANDS\0" // IR(36371, 54)
  "CooperativeMatrixMulAddKHR\0" // IR(36425, 27)
  "CooperativeMatrixLengthKHR\0" // IR(36452, 27)
  "ConstantCompositeReplicateEXT\0" // IR(36479, 30)
  "SpecConstantCompositeReplicateEXT\0" // IR(36509, 34)
  "CompositeConstructReplicateEXT\0" // IR(36543, 31)
  "TypeRayQueryKHR\0" // IR(36574, 16)
  "RayQueryInitializeKHR\0" // IR(36590, 22)
  "RayQueryTerminateKHR\0" // IR(36612, 21)
  "RayQueryGenerateIntersectionKHR\0" // IR(36633, 32)
  "RayQueryConfirmIntersectionKHR\0" // IR(36665, 31)
  "RayQueryProceedKHR\0" // IR(36696, 19)
  "RayQueryGetIntersectionTypeKHR\0" // IR(36715, 31)
  "ImageSampleWeightedQCOM\0" // IR(36746, 24)
  "ImageBoxFilterQCOM\0" // IR(36770, 19)
  "ImageBlockMatchSSDQCOM\0" // IR(36789, 23)
  "ImageBlockMatchSADQCOM\0" // IR(36812, 23)
  "BitCastArrayQCOM\0" // IR(36835, 17)
  "ImageBlockMatchWindowSSDQCOM\0" // IR(36852, 29)
  "ImageBlockMatchWindowSADQCOM\0" // IR(36881, 29)
  "ImageBlockMatchGatherSSDQCOM\0" // IR(36910, 29)
  "ImageBlockMatchGatherSADQCOM\0" // IR(36939, 29)
  "CompositeConstructCoopMatQCOM\0" // IR(36968, 30)
  "CompositeExtractCoopMatQCOM\0" // IR(36998, 28)
  "ExtractSubArrayQCOM\0" // IR(37026, 20)
  "ImageGatherQCOM\0" // IR(37046, 16)
  "GroupIAddNonUniformAMD\0" // IR(37062, 23)
  "GroupFAddNonUniformAMD\0" // IR(37085, 23)
  "GroupFMinNonUniformAMD\0" // IR(37108, 23)
  "GroupUMinNonUniformAMD\0" // IR(37131, 23)
  "GroupSMinNonUniformAMD\0" // IR(37154, 23)
  "GroupFMaxNonUniformAMD\0" // IR(37177, 23)
  "GroupUMaxNonUniformAMD\0" // IR(37200, 23)
  "GroupSMaxNonUniformAMD\0" // IR(37223, 23)
  "FragmentMaskFetchAMD\0" // IR(37246, 21)
  "FragmentFetchAMD\0" // IR(37267, 17)
  "ReadClockKHR\0" // IR(37284, 13)
  "AllocateNodePayloadsAMDX\0" // IR(37297, 25)
  "EnqueueNodePayloadsAMDX\0" // IR(37322, 24)
  "TypeNodePayloadArrayAMDX\0" // IR(37346, 25)
  "FinishWritingNodePayloadAMDX\0" // IR(37371, 29)
  "NodePayloadArrayLengthAMDX\0" // IR(37400, 27)
  "IsNodePayloadValidAMDX\0" // IR(37427, 23)
  "ConstantStringAMDX\0" // IR(37450, 19)
  "SpecConstantStringAMDX\0" // IR(37469, 23)
  "GroupNonUniformQuadAllKHR\0" // IR(37492, 26)
  "GroupNonUniformQuadAnyKHR\0" // IR(37518, 26)
  "TypeBufferEXT\0" // IR(37544, 14)
  "BufferPointerEXT\0" // IR(37558, 17)
  "UntypedImageTexelPointerEXT\0" // IR(37575, 28)
  "MemberDecorateIdEXT\0" // IR(37603, 20)
  "ConstantSizeOfEXT\0" // IR(37623, 18)
  "SpecConstantDataKHR\0" // IR(37641, 20)
  "PoisonKHR\0" // IR(37661, 10)
  "FreezeKHR\0" // IR(37671, 10)
  "HitObjectRecordHitMotionNV\0" // IR(37681, 27)
  "HitObjectRecordHitWithIndexMotionNV\0" // IR(37708, 36)
  "HitObjectRecordMissMotionNV\0" // IR(37744, 28)
  "HitObjectGetWorldToObjectNV\0" // IR(37772, 28)
  "HitObjectGetObjectToWorldNV\0" // IR(37800, 28)
  "HitObjectGetObjectRayDirectionNV\0" // IR(37828, 33)
  "HitObjectGetObjectRayOriginNV\0" // IR(37861, 30)
  "HitObjectTraceRayMotionNV\0" // IR(37891, 26)
  "HitObjectGetShaderRecordBufferHandleNV\0" // IR(37917, 39)
  "HitObjectGetShaderBindingTableRecordIndexNV\0" // IR(37956, 44)
  "HitObjectRecordEmptyNV\0" // IR(38000, 23)
  "HitObjectTraceRayNV\0" // IR(38023, 20)
  "HitObjectRecordHitNV\0" // IR(38043, 21)
  "HitObjectRecordHitWithIndexNV\0" // IR(38064, 30)
  "HitObjectRecordMissNV\0" // IR(38094, 22)
  "HitObjectExecuteShaderNV\0" // IR(38116, 25)
  "HitObjectGetCurrentTimeNV\0" // IR(38141, 26)
  "HitObjectGetAttributesNV\0" // IR(38167, 25)
  "HitObjectGetHitKindNV\0" // IR(38192, 22)
  "HitObjectGetPrimitiveIndexNV\0" // IR(38214, 29)
  "HitObjectGetGeometryIndexNV\0" // IR(38243, 28)
  "HitObjectGetInstanceIdNV\0" // IR(38271, 25)
  "HitObjectGetInstanceCustomIndexNV\0" // IR(38296, 34)
  "HitObjectGetWorldRayDirectionNV\0" // IR(38330, 32)
  "HitObjectGetWorldRayOriginNV\0" // IR(38362, 29)
  "HitObjectGetRayTMaxNV\0" // IR(38391, 22)
  "HitObjectGetRayTMinNV\0" // IR(38413, 22)
  "HitObjectIsEmptyNV\0" // IR(38435, 19)
  "HitObjectIsHitNV\0" // IR(38454, 17)
  "HitObjectIsMissNV\0" // IR(38471, 18)
  "ReorderThreadWithHitObjectNV\0" // IR(38489, 29)
  "ReorderThreadWithHintNV\0" // IR(38518, 24)
  "TypeHitObjectNV\0" // IR(38542, 16)
  "ImageSampleFootprintNV\0" // IR(38558, 23)
  "TypeVectorIdEXT\0" // IR(38581, 16)
  "TypeCooperativeVectorNV\0" // IR(38597, 24)
  "CooperativeVectorMatrixMulNV\0" // IR(38621, 29)
  "CooperativeVectorOuterProductAccumulateNV\0" // IR(38650, 42)
  "CooperativeVectorReduceSumAccumulateNV\0" // IR(38692, 39)
  "CooperativeVectorMatrixMulAddNV\0" // IR(38731, 32)
  "CooperativeMatrixConvertNV\0" // IR(38763, 27)
  "EmitMeshTasksEXT\0" // IR(38790, 17)
  "SetMeshOutputsEXT\0" // IR(38807, 18)
  "GroupNonUniformPartitionEXT\0" // IR(38825, 28)
  "GroupNonUniformPartitionNV\0" // IR(38853, 27)
  "WritePackedPrimitiveIndices4x8NV\0" // IR(38880, 33)
  "FetchMicroTriangleVertexPositionNV\0" // IR(38913, 35)
  "FetchMicroTriangleVertexBarycentricNV\0" // IR(38948, 38)
  "CooperativeVectorLoadNV\0" // IR(38986, 24)
  "CooperativeVectorStoreNV\0" // IR(39010, 25)
  "HitObjectRecordFromQueryEXT\0" // IR(39035, 28)
  "HitObjectRecordMissEXT\0" // IR(39063, 23)
  "HitObjectRecordMissMotionEXT\0" // IR(39086, 29)
  "HitObjectGetIntersectionTriangleVertexPositionsEXT\0" // IR(39115, 51)
  "HitObjectGetRayFlagsEXT\0" // IR(39166, 24)
  "HitObjectSetShaderBindingTableRecordIndexEXT\0" // IR(39190, 45)
  "HitObjectReorderExecuteShaderEXT\0" // IR(39235, 33)
  "HitObjectTraceReorderExecuteEXT\0" // IR(39268, 32)
  "HitObjectTraceMotionReorderExecuteEXT\0" // IR(39300, 38)
  "TypeHitObjectEXT\0" // IR(39338, 17)
  "ReorderThreadWithHintEXT\0" // IR(39355, 25)
  "ReorderThreadWithHitObjectEXT\0" // IR(39380, 30)
  "HitObjectTraceRayEXT\0" // IR(39410, 21)
  "HitObjectTraceRayMotionEXT\0" // IR(39431, 27)
  "HitObjectRecordEmptyEXT\0" // IR(39458, 24)
  "HitObjectExecuteShaderEXT\0" // IR(39482, 26)
  "HitObjectGetCurrentTimeEXT\0" // IR(39508, 27)
  "HitObjectGetAttributesEXT\0" // IR(39535, 26)
  "HitObjectGetHitKindEXT\0" // IR(39561, 23)
  "HitObjectGetPrimitiveIndexEXT\0" // IR(39584, 30)
  "HitObjectGetGeometryIndexEXT\0" // IR(39614, 29)
  "HitObjectGetInstanceIdEXT\0" // IR(39643, 26)
  "HitObjectGetInstanceCustomIndexEXT\0" // IR(39669, 35)
  "HitObjectGetObjectRayOriginEXT\0" // IR(39704, 31)
  "HitObjectGetObjectRayDirectionEXT\0" // IR(39735, 34)
  "HitObjectGetWorldRayDirectionEXT\0" // IR(39769, 33)
  "HitObjectGetWorldRayOriginEXT\0" // IR(39802, 30)
  "HitObjectGetObjectToWorldEXT\0" // IR(39832, 29)
  "HitObjectGetWorldToObjectEXT\0" // IR(39861, 29)
  "HitObjectGetRayTMaxEXT\0" // IR(39890, 23)
  "ReportIntersectionKHR\0" // IR(39913, 22)
  "ReportIntersectionNV\0" // IR(39935, 21)
  "IgnoreIntersectionNV\0" // IR(39956, 21)
  "TerminateRayNV\0" // IR(39977, 15)
  "TraceNV\0" // IR(39992, 8)
  "TraceMotionNV\0" // IR(40000, 14)
  "TraceRayMotionNV\0" // IR(40014, 17)
  "RayQueryGetIntersectionTriangleVertexPositionsKHR\0" // IR(40031, 50)
  "TypeAccelerationStructureKHR\0" // IR(40081, 29)
  "TypeAccelerationStructureNV\0" // IR(40110, 28)
  "ExecuteCallableNV\0" // IR(40138, 18)
  "RayQueryGetIntersectionClusterIdNV\0" // IR(40156, 35)
  "RayQueryGetClusterIdNV\0" // IR(40191, 23)
  "HitObjectGetClusterIdNV\0" // IR(40214, 24)
  "HitObjectGetRayTMinEXT\0" // IR(40238, 23)
  "HitObjectGetShaderBindingTableRecordIndexEXT\0" // IR(40261, 45)
  "HitObjectGetShaderRecordBufferHandleEXT\0" // IR(40306, 40)
  "HitObjectIsEmptyEXT\0" // IR(40346, 20)
  "HitObjectIsHitEXT\0" // IR(40366, 18)
  "HitObjectIsMissEXT\0" // IR(40384, 19)
  "TypeCooperativeMatrixNV\0" // IR(40403, 24)
  "CooperativeMatrixLoadNV\0" // IR(40427, 24)
  "CooperativeMatrixStoreNV\0" // IR(40451, 25)
  "CooperativeMatrixMulAddNV\0" // IR(40476, 26)
  "CooperativeMatrixLengthNV\0" // IR(40502, 26)
  "BeginInvocationInterlockEXT\0" // IR(40528, 28)
  "EndInvocationInterlockEXT\0" // IR(40556, 26)
  "SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_REDUCE\0" // IR(40582, 43)
  "CooperativeMatrixReduceNV\0" // IR(40625, 26)
  "SPV_OPERAND_TYPE_MEMORY_ACCESS\0" // IR(40651, 31)
  "SPV_OPERAND_TYPE_TENSOR_ADDRESSING_OPERANDS\0" // IR(40682, 44)
  "CooperativeMatrixLoadTensorNV\0" // IR(40726, 30)
  "CooperativeMatrixStoreTensorNV\0" // IR(40756, 31)
  "CooperativeMatrixPerElementOpNV\0" // IR(40787, 32)
  "TypeTensorLayoutNV\0" // IR(40819, 19)
  "TypeTensorViewNV\0" // IR(40838, 17)
  "CreateTensorLayoutNV\0" // IR(40855, 21)
  "TensorLayoutSetDimensionNV\0" // IR(40876, 27)
  "TensorLayoutSetStrideNV\0" // IR(40903, 24)
  "TensorLayoutSliceNV\0" // IR(40927, 20)
  "TensorLayoutSetClampValueNV\0" // IR(40947, 28)
  "CreateTensorViewNV\0" // IR(40975, 19)
  "TensorViewSetDimensionNV\0" // IR(40994, 25)
  "TensorViewSetStrideNV\0" // IR(41019, 22)
  "IsHelperInvocationEXT\0" // IR(41041, 22)
  "TensorViewSetClipNV\0" // IR(41063, 20)
  "TensorLayoutSetBlockSizeNV\0" // IR(41083, 27)
  "CooperativeMatrixTransposeNV\0" // IR(41110, 29)
  "ConvertUToImageNV\0" // IR(41139, 18)
  "ConvertUToSamplerNV\0" // IR(41157, 20)
  "ConvertImageToUNV\0" // IR(41177, 18)
  "ConvertSamplerToUNV\0" // IR(41195, 20)
  "ConvertUToSampledImageNV\0" // IR(41215, 25)
  "ConvertSampledImageToUNV\0" // IR(41240, 25)
  "SamplerImageAddressingModeNV\0" // IR(41265, 29)
  "SPV_OPERAND_TYPE_OPTIONAL_RAW_ACCESS_CHAIN_OPERANDS\0" // IR(41294, 52)
  "RawAccessChainNV\0" // IR(41346, 17)
  "RayQueryGetIntersectionSpherePositionNV\0" // IR(41363, 40)
  "RayQueryGetIntersectionSphereRadiusNV\0" // IR(41403, 38)
  "RayQueryGetIntersectionLSSPositionsNV\0" // IR(41441, 38)
  "RayQueryGetIntersectionLSSRadiiNV\0" // IR(41479, 34)
  "RayQueryGetIntersectionLSSHitValueNV\0" // IR(41513, 37)
  "HitObjectGetSpherePositionNV\0" // IR(41550, 29)
  "HitObjectGetSphereRadiusNV\0" // IR(41579, 27)
  "HitObjectGetLSSPositionsNV\0" // IR(41606, 27)
  "HitObjectGetLSSRadiiNV\0" // IR(41633, 23)
  "HitObjectIsSphereHitNV\0" // IR(41656, 23)
  "HitObjectIsLSSHitNV\0" // IR(41679, 20)
  "RayQueryIsSphereHitNV\0" // IR(41699, 22)
  "RayQueryIsLSSHitNV\0" // IR(41721, 19)
  "SubgroupShuffleDownINTEL\0" // IR(41740, 25)
  "SubgroupShuffleUpINTEL\0" // IR(41765, 23)
  "SubgroupShuffleXorINTEL\0" // IR(41788, 24)
  "SubgroupBlockReadINTEL\0" // IR(41812, 23)
  "SubgroupBlockWriteINTEL\0" // IR(41835, 24)
  "SubgroupImageBlockReadINTEL\0" // IR(41859, 28)
  "SubgroupImageBlockWriteINTEL\0" // IR(41887, 29)
  "SubgroupImageMediaBlockReadINTEL\0" // IR(41916, 33)
  "SubgroupImageMediaBlockWriteINTEL\0" // IR(41949, 34)
  "UCountLeadingZerosINTEL\0" // IR(41983, 24)
  "UCountTrailingZerosINTEL\0" // IR(42007, 25)
  "AbsISubINTEL\0" // IR(42032, 13)
  "AbsUSubINTEL\0" // IR(42045, 13)
  "IAddSatINTEL\0" // IR(42058, 13)
  "UAddSatINTEL\0" // IR(42071, 13)
  "IAverageINTEL\0" // IR(42084, 14)
  "UAverageINTEL\0" // IR(42098, 14)
  "IAverageRoundedINTEL\0" // IR(42112, 21)
  "UAverageRoundedINTEL\0" // IR(42133, 21)
  "ISubSatINTEL\0" // IR(42154, 13)
  "USubSatINTEL\0" // IR(42167, 13)
  "IMul32x16INTEL\0" // IR(42180, 15)
  "UMul32x16INTEL\0" // IR(42195, 15)
  "ConstantFunctionPointerINTEL\0" // IR(42210, 29)
  "FunctionPointerCallINTEL\0" // IR(42239, 25)
  "AsmTargetINTEL\0" // IR(42264, 15)
  "AsmCallINTEL\0" // IR(42279, 13)
  "AtomicFMinEXT\0" // IR(42292, 14)
  "AtomicFMaxEXT\0" // IR(42306, 14)
  "AssumeTrueKHR\0" // IR(42320, 14)
  "ExpectKHR\0" // IR(42334, 10)
  "DecorateString\0" // IR(42344, 15)
  "DecorateStringGOOGLE\0" // IR(42359, 21)
  "MemberDecorateString\0" // IR(42380, 21)
  "MemberDecorateStringGOOGLE\0" // IR(42401, 27)
  "VmeImageINTEL\0" // IR(42428, 14)
  "TypeVmeImageINTEL\0" // IR(42442, 18)
  "TypeAvcImePayloadINTEL\0" // IR(42460, 23)
  "TypeAvcRefPayloadINTEL\0" // IR(42483, 23)
  "TypeAvcSicPayloadINTEL\0" // IR(42506, 23)
  "TypeAvcMcePayloadINTEL\0" // IR(42529, 23)
  "TypeAvcMceResultINTEL\0" // IR(42552, 22)
  "TypeAvcImeResultINTEL\0" // IR(42574, 22)
  "TypeAvcImeResultSingleReferenceStreamoutINTEL\0" // IR(42596, 46)
  "TypeAvcImeResultDualReferenceStreamoutINTEL\0" // IR(42642, 44)
  "TypeAvcImeSingleReferenceStreaminINTEL\0" // IR(42686, 39)
  "TypeAvcImeDualReferenceStreaminINTEL\0" // IR(42725, 37)
  "TypeAvcRefResultINTEL\0" // IR(42762, 22)
  "TypeAvcSicResultINTEL\0" // IR(42784, 22)
  "SubgroupAvcMceGetDefaultInterBaseMultiReferencePenaltyINTEL\0" // IR(42806, 60)
  "SubgroupAvcMceSetInterBaseMultiReferencePenaltyINTEL\0" // IR(42866, 53)
  "SubgroupAvcMceGetDefaultInterShapePenaltyINTEL\0" // IR(42919, 47)
  "SubgroupAvcMceSetInterShapePenaltyINTEL\0" // IR(42966, 40)
  "SubgroupAvcMceGetDefaultInterDirectionPenaltyINTEL\0" // IR(43006, 51)
  "SubgroupAvcMceSetInterDirectionPenaltyINTEL\0" // IR(43057, 44)
  "SubgroupAvcMceGetDefaultIntraLumaShapePenaltyINTEL\0" // IR(43101, 51)
  "SubgroupAvcMceGetDefaultInterMotionVectorCostTableINTEL\0" // IR(43152, 56)
  "SubgroupAvcMceGetDefaultHighPenaltyCostTableINTEL\0" // IR(43208, 50)
  "SubgroupAvcMceGetDefaultMediumPenaltyCostTableINTEL\0" // IR(43258, 52)
  "SubgroupAvcMceGetDefaultLowPenaltyCostTableINTEL\0" // IR(43310, 49)
  "SubgroupAvcMceSetMotionVectorCostFunctionINTEL\0" // IR(43359, 47)
  "SubgroupAvcMceGetDefaultIntraLumaModePenaltyINTEL\0" // IR(43406, 50)
  "SubgroupAvcMceGetDefaultNonDcLumaIntraPenaltyINTEL\0" // IR(43456, 51)
  "SubgroupAvcMceGetDefaultIntraChromaModeBasePenaltyINTEL\0" // IR(43507, 56)
  "SubgroupAvcMceSetAcOnlyHaarINTEL\0" // IR(43563, 33)
  "SubgroupAvcMceSetSourceInterlacedFieldPolarityINTEL\0" // IR(43596, 52)
  "SubgroupAvcMceSetSingleReferenceInterlacedFieldPolarityINTEL\0" // IR(43648, 61)
  "SubgroupAvcMceSetDualReferenceInterlacedFieldPolaritiesINTEL\0" // IR(43709, 61)
  "SubgroupAvcMceConvertToImePayloadINTEL\0" // IR(43770, 39)
  "SubgroupAvcMceConvertToImeResultINTEL\0" // IR(43809, 38)
  "SubgroupAvcMceConvertToRefPayloadINTEL\0" // IR(43847, 39)
  "SubgroupAvcMceConvertToRefResultINTEL\0" // IR(43886, 38)
  "SubgroupAvcMceConvertToSicPayloadINTEL\0" // IR(43924, 39)
  "SubgroupAvcMceConvertToSicResultINTEL\0" // IR(43963, 38)
  "SubgroupAvcMceGetMotionVectorsINTEL\0" // IR(44001, 36)
  "SubgroupAvcMceGetInterDistortionsINTEL\0" // IR(44037, 39)
  "SubgroupAvcMceGetBestInterDistortionsINTEL\0" // IR(44076, 43)
  "SubgroupAvcMceGetInterMajorShapeINTEL\0" // IR(44119, 38)
  "SubgroupAvcMceGetInterMinorShapeINTEL\0" // IR(44157, 38)
  "SubgroupAvcMceGetInterDirectionsINTEL\0" // IR(44195, 38)
  "SubgroupAvcMceGetInterMotionVectorCountINTEL\0" // IR(44233, 45)
  "SubgroupAvcMceGetInterReferenceIdsINTEL\0" // IR(44278, 40)
  "SubgroupAvcMceGetInterReferenceInterlacedFieldPolaritiesINTEL\0" // IR(44318, 62)
  "SubgroupAvcImeInitializeINTEL\0" // IR(44380, 30)
  "SubgroupAvcImeSetSingleReferenceINTEL\0" // IR(44410, 38)
  "SubgroupAvcImeSetDualReferenceINTEL\0" // IR(44448, 36)
  "SubgroupAvcImeRefWindowSizeINTEL\0" // IR(44484, 33)
  "SubgroupAvcImeAdjustRefOffsetINTEL\0" // IR(44517, 35)
  "SubgroupAvcImeConvertToMcePayloadINTEL\0" // IR(44552, 39)
  "SubgroupAvcImeSetMaxMotionVectorCountINTEL\0" // IR(44591, 43)
  "SubgroupAvcImeSetUnidirectionalMixDisableINTEL\0" // IR(44634, 47)
  "SubgroupAvcImeSetEarlySearchTerminationThresholdINTEL\0" // IR(44681, 54)
  "SubgroupAvcImeSetWeightedSadINTEL\0" // IR(44735, 34)
  "SubgroupAvcImeEvaluateWithSingleReferenceINTEL\0" // IR(44769, 47)
  "SubgroupAvcImeEvaluateWithDualReferenceINTEL\0" // IR(44816, 45)
  "SubgroupAvcImeEvaluateWithSingleReferenceStreaminINTEL\0" // IR(44861, 55)
  "SubgroupAvcImeEvaluateWithDualReferenceStreaminINTEL\0" // IR(44916, 53)
  "SubgroupAvcImeEvaluateWithSingleReferenceStreamoutINTEL\0" // IR(44969, 56)
  "SubgroupAvcImeEvaluateWithDualReferenceStreamoutINTEL\0" // IR(45025, 54)
  "SubgroupAvcImeEvaluateWithSingleReferenceStreaminoutINTEL\0" // IR(45079, 58)
  "SubgroupAvcImeEvaluateWithDualReferenceStreaminoutINTEL\0" // IR(45137, 56)
  "SubgroupAvcImeConvertToMceResultINTEL\0" // IR(45193, 38)
  "SubgroupAvcImeGetSingleReferenceStreaminINTEL\0" // IR(45231, 46)
  "SubgroupAvcImeGetDualReferenceStreaminINTEL\0" // IR(45277, 44)
  "SubgroupAvcImeStripSingleReferenceStreamoutINTEL\0" // IR(45321, 49)
  "SubgroupAvcImeStripDualReferenceStreamoutINTEL\0" // IR(45370, 47)
  "SubgroupAvcImeGetStreamoutSingleReferenceMajorShapeMotionVectorsINTEL\0" // IR(45417, 70)
  "SubgroupAvcImeGetStreamoutSingleReferenceMajorShapeDistortionsINTEL\0" // IR(45487, 68)
  "SubgroupAvcImeGetStreamoutSingleReferenceMajorShapeReferenceIdsINTEL\0" // IR(45555, 69)
  "SubgroupAvcImeGetStreamoutDualReferenceMajorShapeMotionVectorsINTEL\0" // IR(45624, 68)
  "SubgroupAvcImeGetStreamoutDualReferenceMajorShapeDistortionsINTEL\0" // IR(45692, 66)
  "SubgroupAvcImeGetStreamoutDualReferenceMajorShapeReferenceIdsINTEL\0" // IR(45758, 67)
  "SubgroupAvcImeGetBorderReachedINTEL\0" // IR(45825, 36)
  "SubgroupAvcImeGetTruncatedSearchIndicationINTEL\0" // IR(45861, 48)
  "SubgroupAvcImeGetUnidirectionalEarlySearchTerminationINTEL\0" // IR(45909, 59)
  "SubgroupAvcImeGetWeightingPatternMinimumMotionVectorINTEL\0" // IR(45968, 58)
  "SubgroupAvcImeGetWeightingPatternMinimumDistortionINTEL\0" // IR(46026, 56)
  "SubgroupAvcFmeInitializeINTEL\0" // IR(46082, 30)
  "SubgroupAvcBmeInitializeINTEL\0" // IR(46112, 30)
  "SubgroupAvcRefConvertToMcePayloadINTEL\0" // IR(46142, 39)
  "SubgroupAvcRefSetBidirectionalMixDisableINTEL\0" // IR(46181, 46)
  "SubgroupAvcRefSetBilinearFilterEnableINTEL\0" // IR(46227, 43)
  "SubgroupAvcRefEvaluateWithSingleReferenceINTEL\0" // IR(46270, 47)
  "SubgroupAvcRefEvaluateWithDualReferenceINTEL\0" // IR(46317, 45)
  "SubgroupAvcRefEvaluateWithMultiReferenceINTEL\0" // IR(46362, 46)
  "SubgroupAvcRefEvaluateWithMultiReferenceInterlacedINTEL\0" // IR(46408, 56)
  "SubgroupAvcRefConvertToMceResultINTEL\0" // IR(46464, 38)
  "SubgroupAvcSicInitializeINTEL\0" // IR(46502, 30)
  "SubgroupAvcSicConfigureSkcINTEL\0" // IR(46532, 32)
  "SubgroupAvcSicConfigureIpeLumaINTEL\0" // IR(46564, 36)
  "SubgroupAvcSicConfigureIpeLumaChromaINTEL\0" // IR(46600, 42)
  "SubgroupAvcSicGetMotionVectorMaskINTEL\0" // IR(46642, 39)
  "SubgroupAvcSicConvertToMcePayloadINTEL\0" // IR(46681, 39)
  "SubgroupAvcSicSetIntraLumaShapePenaltyINTEL\0" // IR(46720, 44)
  "SubgroupAvcSicSetIntraLumaModeCostFunctionINTEL\0" // IR(46764, 48)
  "SubgroupAvcSicSetIntraChromaModeCostFunctionINTEL\0" // IR(46812, 50)
  "SubgroupAvcSicSetBilinearFilterEnableINTEL\0" // IR(46862, 43)
  "SubgroupAvcSicSetSkcForwardTransformEnableINTEL\0" // IR(46905, 48)
  "SubgroupAvcSicSetBlockBasedRawSkipSadINTEL\0" // IR(46953, 43)
  "SubgroupAvcSicEvaluateIpeINTEL\0" // IR(46996, 31)
  "SubgroupAvcSicEvaluateWithSingleReferenceINTEL\0" // IR(47027, 47)
  "SubgroupAvcSicEvaluateWithDualReferenceINTEL\0" // IR(47074, 45)
  "SubgroupAvcSicEvaluateWithMultiReferenceINTEL\0" // IR(47119, 46)
  "SubgroupAvcSicEvaluateWithMultiReferenceInterlacedINTEL\0" // IR(47165, 56)
  "SubgroupAvcSicConvertToMceResultINTEL\0" // IR(47221, 38)
  "SubgroupAvcSicGetIpeLumaShapeINTEL\0" // IR(47259, 35)
  "SubgroupAvcSicGetBestIpeLumaDistortionINTEL\0" // IR(47294, 44)
  "SubgroupAvcSicGetBestIpeChromaDistortionINTEL\0" // IR(47338, 46)
  "SubgroupAvcSicGetPackedIpeLumaModesINTEL\0" // IR(47384, 41)
  "SubgroupAvcSicGetIpeChromaModeINTEL\0" // IR(47425, 36)
  "SubgroupAvcSicGetPackedSkcLumaCountThresholdINTEL\0" // IR(47461, 50)
  "SubgroupAvcSicGetPackedSkcLumaSumThresholdINTEL\0" // IR(47511, 48)
  "SubgroupAvcSicGetInterRawSadsINTEL\0" // IR(47559, 35)
  "SaveMemoryINTEL\0" // IR(47594, 16)
  "RestoreMemoryINTEL\0" // IR(47610, 19)
  "ArbitraryFloatSinCosPiALTERA\0" // IR(47629, 29)
  "ArbitraryFloatSinCosPiINTEL\0" // IR(47658, 28)
  "ArbitraryFloatCastALTERA\0" // IR(47686, 25)
  "ArbitraryFloatCastINTEL\0" // IR(47711, 24)
  "ArbitraryFloatCastFromIntALTERA\0" // IR(47735, 32)
  "ArbitraryFloatCastFromIntINTEL\0" // IR(47767, 31)
  "ArbitraryFloatCastToIntALTERA\0" // IR(47798, 30)
  "ArbitraryFloatCastToIntINTEL\0" // IR(47828, 29)
  "ArbitraryFloatAddALTERA\0" // IR(47857, 24)
  "ArbitraryFloatAddINTEL\0" // IR(47881, 23)
  "ArbitraryFloatSubALTERA\0" // IR(47904, 24)
  "ArbitraryFloatSubINTEL\0" // IR(47928, 23)
  "ArbitraryFloatMulALTERA\0" // IR(47951, 24)
  "ArbitraryFloatMulINTEL\0" // IR(47975, 23)
  "ArbitraryFloatDivALTERA\0" // IR(47998, 24)
  "ArbitraryFloatDivINTEL\0" // IR(48022, 23)
  "ArbitraryFloatGTALTERA\0" // IR(48045, 23)
  "ArbitraryFloatGTINTEL\0" // IR(48068, 22)
  "ArbitraryFloatGEALTERA\0" // IR(48090, 23)
  "ArbitraryFloatGEINTEL\0" // IR(48113, 22)
  "ArbitraryFloatLTALTERA\0" // IR(48135, 23)
  "ArbitraryFloatLTINTEL\0" // IR(48158, 22)
  "ArbitraryFloatLEALTERA\0" // IR(48180, 23)
  "ArbitraryFloatLEINTEL\0" // IR(48203, 22)
  "ArbitraryFloatEQALTERA\0" // IR(48225, 23)
  "ArbitraryFloatEQINTEL\0" // IR(48248, 22)
  "ArbitraryFloatRecipALTERA\0" // IR(48270, 26)
  "ArbitraryFloatRecipINTEL\0" // IR(48296, 25)
  "ArbitraryFloatRSqrtALTERA\0" // IR(48321, 26)
  "ArbitraryFloatRSqrtINTEL\0" // IR(48347, 25)
  "ArbitraryFloatCbrtALTERA\0" // IR(48372, 25)
  "ArbitraryFloatCbrtINTEL\0" // IR(48397, 24)
  "ArbitraryFloatHypotALTERA\0" // IR(48421, 26)
  "ArbitraryFloatHypotINTEL\0" // IR(48447, 25)
  "ArbitraryFloatSqrtALTERA\0" // IR(48472, 25)
  "ArbitraryFloatSqrtINTEL\0" // IR(48497, 24)
  "ArbitraryFloatLogINTEL\0" // IR(48521, 23)
  "ArbitraryFloatLog2INTEL\0" // IR(48544, 24)
  "ArbitraryFloatLog10INTEL\0" // IR(48568, 25)
  "ArbitraryFloatLog1pINTEL\0" // IR(48593, 25)
  "ArbitraryFloatExpINTEL\0" // IR(48618, 23)
  "ArbitraryFloatExp2INTEL\0" // IR(48641, 24)
  "ArbitraryFloatExp10INTEL\0" // IR(48665, 25)
  "ArbitraryFloatExpm1INTEL\0" // IR(48690, 25)
  "ArbitraryFloatSinINTEL\0" // IR(48715, 23)
  "ArbitraryFloatCosINTEL\0" // IR(48738, 23)
  "ArbitraryFloatSinCosINTEL\0" // IR(48761, 26)
  "ArbitraryFloatSinPiINTEL\0" // IR(48787, 25)
  "ArbitraryFloatCosPiINTEL\0" // IR(48812, 25)
  "ArbitraryFloatASinINTEL\0" // IR(48837, 24)
  "ArbitraryFloatASinPiINTEL\0" // IR(48861, 26)
  "ArbitraryFloatACosINTEL\0" // IR(48887, 24)
  "ArbitraryFloatACosPiINTEL\0" // IR(48911, 26)
  "ArbitraryFloatATanINTEL\0" // IR(48937, 24)
  "ArbitraryFloatATanPiINTEL\0" // IR(48961, 26)
  "ArbitraryFloatATan2INTEL\0" // IR(48987, 25)
  "ArbitraryFloatPowINTEL\0" // IR(49012, 23)
  "ArbitraryFloatPowRINTEL\0" // IR(49035, 24)
  "ArbitraryFloatPowNINTEL\0" // IR(49059, 24)
  "LoopControlINTEL\0" // IR(49083, 17)
  "AliasDomainDeclINTEL\0" // IR(49100, 21)
  "AliasScopeDeclINTEL\0" // IR(49121, 20)
  "AliasScopeListDeclINTEL\0" // IR(49141, 24)
  "FixedSqrtALTERA\0" // IR(49165, 16)
  "FixedSqrtINTEL\0" // IR(49181, 15)
  "FixedRecipALTERA\0" // IR(49196, 17)
  "FixedRecipINTEL\0" // IR(49213, 16)
  "FixedRsqrtALTERA\0" // IR(49229, 17)
  "FixedRsqrtINTEL\0" // IR(49246, 16)
  "FixedSinALTERA\0" // IR(49262, 15)
  "FixedSinINTEL\0" // IR(49277, 14)
  "FixedCosALTERA\0" // IR(49291, 15)
  "FixedCosINTEL\0" // IR(49306, 14)
  "FixedSinCosALTERA\0" // IR(49320, 18)
  "FixedSinCosINTEL\0" // IR(49338, 17)
  "FixedSinPiALTERA\0" // IR(49355, 17)
  "FixedSinPiINTEL\0" // IR(49372, 16)
  "FixedCosPiALTERA\0" // IR(49388, 17)
  "FixedCosPiINTEL\0" // IR(49405, 16)
  "FixedSinCosPiALTERA\0" // IR(49421, 20)
  "FixedSinCosPiINTEL\0" // IR(49441, 19)
  "FixedLogALTERA\0" // IR(49460, 15)
  "FixedLogINTEL\0" // IR(49475, 14)
  "FixedExpALTERA\0" // IR(49489, 15)
  "FixedExpINTEL\0" // IR(49504, 14)
  "PtrCastToCrossWorkgroupALTERA\0" // IR(49518, 30)
  "PtrCastToCrossWorkgroupINTEL\0" // IR(49548, 29)
  "CrossWorkgroupCastToPtrALTERA\0" // IR(49577, 30)
  "CrossWorkgroupCastToPtrINTEL\0" // IR(49607, 29)
  "ReadPipeBlockingALTERA\0" // IR(49636, 23)
  "ReadPipeBlockingINTEL\0" // IR(49659, 22)
  "WritePipeBlockingALTERA\0" // IR(49681, 24)
  "WritePipeBlockingINTEL\0" // IR(49705, 23)
  "RayQueryGetRayTMinKHR\0" // IR(49728, 22)
  "RayQueryGetRayFlagsKHR\0" // IR(49750, 23)
  "RayQueryGetIntersectionTKHR\0" // IR(49773, 28)
  "RayQueryGetIntersectionInstanceCustomIndexKHR\0" // IR(49801, 46)
  "RayQueryGetIntersectionInstanceIdKHR\0" // IR(49847, 37)
  "RayQueryGetIntersectionInstanceShaderBindingTableRecordOffsetKHR\0" // IR(49884, 65)
  "RayQueryGetIntersectionGeometryIndexKHR\0" // IR(49949, 40)
  "RayQueryGetIntersectionPrimitiveIndexKHR\0" // IR(49989, 41)
  "RayQueryGetIntersectionBarycentricsKHR\0" // IR(50030, 39)
  "RayQueryGetIntersectionFrontFaceKHR\0" // IR(50069, 36)
  "RayQueryGetIntersectionCandidateAABBOpaqueKHR\0" // IR(50105, 46)
  "RayQueryGetIntersectionObjectRayDirectionKHR\0" // IR(50151, 45)
  "RayQueryGetIntersectionObjectRayOriginKHR\0" // IR(50196, 42)
  "RayQueryGetWorldRayDirectionKHR\0" // IR(50238, 32)
  "RayQueryGetWorldRayOriginKHR\0" // IR(50270, 29)
  "RayQueryGetIntersectionObjectToWorldKHR\0" // IR(50299, 40)
  "RayQueryGetIntersectionWorldToObjectKHR\0" // IR(50339, 40)
  "AtomicFAddEXT\0" // IR(50379, 14)
  "TypeBufferSurfaceINTEL\0" // IR(50393, 23)
  "TypeStructContinuedINTEL\0" // IR(50416, 25)
  "ConstantCompositeContinuedINTEL\0" // IR(50441, 32)
  "SpecConstantCompositeContinuedINTEL\0" // IR(50473, 36)
  "CompositeConstructContinuedINTEL\0" // IR(50509, 33)
  "ConvertFToBF16INTEL\0" // IR(50542, 20)
  "ConvertBF16ToFINTEL\0" // IR(50562, 20)
  "ControlBarrierArriveEXT\0" // IR(50582, 24)
  "ControlBarrierArriveINTEL\0" // IR(50606, 26)
  "ControlBarrierWaitEXT\0" // IR(50632, 22)
  "ControlBarrierWaitINTEL\0" // IR(50654, 24)
  "TaskSequenceCreateALTERA\0" // IR(50678, 25)
  "TaskSequenceCreateINTEL\0" // IR(50703, 24)
  "TaskSequenceAsyncALTERA\0" // IR(50727, 24)
  "TaskSequenceAsyncINTEL\0" // IR(50751, 23)
  "TaskSequenceGetALTERA\0" // IR(50774, 22)
  "TaskSequenceGetINTEL\0" // IR(50796, 21)
  "TaskSequenceReleaseALTERA\0" // IR(50817, 26)
  "TaskSequenceReleaseINTEL\0" // IR(50843, 25)
  "TypeTaskSequenceALTERA\0" // IR(50868, 23)
  "TypeTaskSequenceINTEL\0" // IR(50891, 22)
  "SubgroupBlockPrefetchINTEL\0" // IR(50913, 27)
  "Subgroup2DBlockLoadINTEL\0" // IR(50940, 25)
  "Subgroup2DBlockLoadTransformINTEL\0" // IR(50965, 34)
  "Subgroup2DBlockLoadTransposeINTEL\0" // IR(50999, 34)
  "Subgroup2DBlockPrefetchINTEL\0" // IR(51033, 29)
  "Subgroup2DBlockStoreINTEL\0" // IR(51062, 26)
  "SPV_OPERAND_TYPE_OPTIONAL_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS\0" // IR(51088, 62)
  "BitwiseFunctionINTEL\0" // IR(51150, 21)
  "ConditionalExtensionINTEL\0" // IR(51171, 26)
  "ConditionalEntryPointINTEL\0" // IR(51197, 27)
  "ConditionalCapabilityINTEL\0" // IR(51224, 27)
  "SpecConstantTargetINTEL\0" // IR(51251, 24)
  "SpecConstantArchitectureINTEL\0" // IR(51275, 30)
  "SPV_OPERAND_TYPE_VARIABLE_CAPABILITY\0" // IR(51305, 37)
  "SpecConstantCapabilitiesINTEL\0" // IR(51342, 30)
  "ConditionalCopyObjectINTEL\0" // IR(51372, 27)
  "PredicatedLoadINTEL\0" // IR(51399, 20)
  "PredicatedStoreINTEL\0" // IR(51419, 21)
  "GroupIMulKHR\0" // IR(51440, 13)
  "GroupFMulKHR\0" // IR(51453, 13)
  "GroupBitwiseAndKHR\0" // IR(51466, 19)
  "GroupBitwiseOrKHR\0" // IR(51485, 18)
  "GroupBitwiseXorKHR\0" // IR(51503, 19)
  "GroupLogicalAndKHR\0" // IR(51522, 19)
  "GroupLogicalOrKHR\0" // IR(51541, 18)
  "GroupLogicalXorKHR\0" // IR(51559, 19)
  "RoundFToTF32INTEL\0" // IR(51578, 18)
  "MaskedGatherINTEL\0" // IR(51596, 18)
  "MaskedScatterINTEL\0" // IR(51614, 19)
  "ConvertHandleToImageINTEL\0" // IR(51633, 26)
  "ConvertHandleToSamplerINTEL\0" // IR(51659, 28)
  "ConvertHandleToSampledImageINTEL\0" // IR(51687, 33)
  "FDot2MixAcc32VALVE\0" // IR(51720, 19)
  "FDot2MixAcc16VALVE\0" // IR(51739, 19)
  "FDot4MixAcc32VALVE\0" // IR(51758, 19)
  "MIN_SAD\0" // IR(51777, 8)
  "MIN_SAD_COST\0" // IR(51785, 13)
  "RAW_SAD\0" // IR(51798, 8)
  "DebugInfoNone\0" // IR(51806, 14)
  "DebugCompilationUnit\0" // IR(51820, 21)
  "SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING\0" // IR(51841, 52)
  "DebugTypeBasic\0" // IR(51893, 15)
  "SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS\0" // IR(51908, 34)
  "DebugTypePointer\0" // IR(51942, 17)
  "SPV_OPERAND_TYPE_DEBUG_TYPE_QUALIFIER\0" // IR(51959, 38)
  "DebugTypeQualifier\0" // IR(51997, 19)
  "DebugTypeArray\0" // IR(52016, 15)
  "DebugTypeVector\0" // IR(52031, 16)
  "DebugTypedef\0" // IR(52047, 13)
  "DebugTypeFunction\0" // IR(52060, 18)
  "DebugTypeEnum\0" // IR(52078, 14)
  "SPV_OPERAND_TYPE_DEBUG_COMPOSITE_TYPE\0" // IR(52092, 38)
  "DebugTypeComposite\0" // IR(52130, 19)
  "DebugTypeMember\0" // IR(52149, 16)
  "DebugTypeInheritance\0" // IR(52165, 21)
  "DebugTypePtrToMember\0" // IR(52186, 21)
  "DebugTypeTemplate\0" // IR(52207, 18)
  "DebugTypeTemplateParameter\0" // IR(52225, 27)
  "DebugTypeTemplateTemplateParameter\0" // IR(52252, 35)
  "DebugTypeTemplateParameterPack\0" // IR(52287, 31)
  "DebugGlobalVariable\0" // IR(52318, 20)
  "DebugFunctionDeclaration\0" // IR(52338, 25)
  "DebugFunction\0" // IR(52363, 14)
  "DebugLexicalBlock\0" // IR(52377, 18)
  "DebugLexicalBlockDiscriminator\0" // IR(52395, 31)
  "DebugScope\0" // IR(52426, 11)
  "DebugNoScope\0" // IR(52437, 13)
  "DebugInlinedAt\0" // IR(52450, 15)
  "SPV_OPERAND_TYPE_OPTIONAL_LITERAL_INTEGER\0" // IR(52465, 42)
  "DebugLocalVariable\0" // IR(52507, 19)
  "DebugInlinedVariable\0" // IR(52526, 21)
  "DebugDeclare\0" // IR(52547, 13)
  "DebugValue\0" // IR(52560, 11)
  "SPV_OPERAND_TYPE_DEBUG_OPERATION\0" // IR(52571, 33)
  "DebugOperation\0" // IR(52604, 15)
  "DebugExpression\0" // IR(52619, 16)
  "DebugMacroDef\0" // IR(52635, 14)
  "DebugMacroUndef\0" // IR(52649, 16)
  "Round\0" // IR(52665, 6)
  "RoundEven\0" // IR(52671, 10)
  "Trunc\0" // IR(52681, 6)
  "FAbs\0" // IR(52687, 5)
  "SAbs\0" // IR(52692, 5)
  "FSign\0" // IR(52697, 6)
  "SSign\0" // IR(52703, 6)
  "Floor\0" // IR(52709, 6)
  "Ceil\0" // IR(52715, 5)
  "Fract\0" // IR(52720, 6)
  "Radians\0" // IR(52726, 8)
  "Degrees\0" // IR(52734, 8)
  "Sin\0" // IR(52742, 4)
  "Cos\0" // IR(52746, 4)
  "Tan\0" // IR(52750, 4)
  "Asin\0" // IR(52754, 5)
  "Acos\0" // IR(52759, 5)
  "Atan\0" // IR(52764, 5)
  "Sinh\0" // IR(52769, 5)
  "Cosh\0" // IR(52774, 5)
  "Tanh\0" // IR(52779, 5)
  "Asinh\0" // IR(52784, 6)
  "Acosh\0" // IR(52790, 6)
  "Atanh\0" // IR(52796, 6)
  "Atan2\0" // IR(52802, 6)
  "Pow\0" // IR(52808, 4)
  "Exp\0" // IR(52812, 4)
  "Log\0" // IR(52816, 4)
  "Exp2\0" // IR(52820, 5)
  "Log2\0" // IR(52825, 5)
  "Sqrt\0" // IR(52830, 5)
  "InverseSqrt\0" // IR(52835, 12)
  "Determinant\0" // IR(52847, 12)
  "MatrixInverse\0" // IR(52859, 14)
  "Modf\0" // IR(52873, 5)
  "ModfStruct\0" // IR(52878, 11)
  "FMin\0" // IR(52889, 5)
  "UMin\0" // IR(52894, 5)
  "SMin\0" // IR(52899, 5)
  "FMax\0" // IR(52904, 5)
  "UMax\0" // IR(52909, 5)
  "SMax\0" // IR(52914, 5)
  "FClamp\0" // IR(52919, 7)
  "UClamp\0" // IR(52926, 7)
  "SClamp\0" // IR(52933, 7)
  "FMix\0" // IR(52940, 5)
  "IMix\0" // IR(52945, 5)
  "Step\0" // IR(52950, 5)
  "SmoothStep\0" // IR(52955, 11)
  "Fma\0" // IR(52966, 4)
  "Frexp\0" // IR(52970, 6)
  "FrexpStruct\0" // IR(52976, 12)
  "Ldexp\0" // IR(52988, 6)
  "PackSnorm4x8\0" // IR(52994, 13)
  "PackUnorm4x8\0" // IR(53007, 13)
  "PackSnorm2x16\0" // IR(53020, 14)
  "PackUnorm2x16\0" // IR(53034, 14)
  "PackHalf2x16\0" // IR(53048, 13)
  "PackDouble2x32\0" // IR(53061, 15)
  "UnpackSnorm2x16\0" // IR(53076, 16)
  "UnpackUnorm2x16\0" // IR(53092, 16)
  "UnpackHalf2x16\0" // IR(53108, 15)
  "UnpackSnorm4x8\0" // IR(53123, 15)
  "UnpackUnorm4x8\0" // IR(53138, 15)
  "UnpackDouble2x32\0" // IR(53153, 17)
  "Length\0" // IR(53170, 7)
  "Distance\0" // IR(53177, 9)
  "Cross\0" // IR(53186, 6)
  "Normalize\0" // IR(53192, 10)
  "FaceForward\0" // IR(53202, 12)
  "Reflect\0" // IR(53214, 8)
  "Refract\0" // IR(53222, 8)
  "FindILsb\0" // IR(53230, 9)
  "FindSMsb\0" // IR(53239, 9)
  "FindUMsb\0" // IR(53248, 9)
  "InterpolateAtCentroid\0" // IR(53257, 22)
  "InterpolateAtSample\0" // IR(53279, 20)
  "InterpolateAtOffset\0" // IR(53299, 20)
  "NMin\0" // IR(53319, 5)
  "NMax\0" // IR(53324, 5)
  "NClamp\0" // IR(53329, 7)
  "ArgumentInfo\0" // IR(53336, 13)
  "ArgumentStorageBuffer\0" // IR(53349, 22)
  "ArgumentUniform\0" // IR(53371, 16)
  "ArgumentPodStorageBuffer\0" // IR(53387, 25)
  "ArgumentPodUniform\0" // IR(53412, 19)
  "ArgumentPodPushConstant\0" // IR(53431, 24)
  "ArgumentSampledImage\0" // IR(53455, 21)
  "ArgumentStorageImage\0" // IR(53476, 21)
  "ArgumentSampler\0" // IR(53497, 16)
  "ArgumentWorkgroup\0" // IR(53513, 18)
  "SpecConstantWorkgroupSize\0" // IR(53531, 26)
  "SpecConstantGlobalOffset\0" // IR(53557, 25)
  "SpecConstantWorkDim\0" // IR(53582, 20)
  "PushConstantGlobalOffset\0" // IR(53602, 25)
  "PushConstantEnqueuedLocalSize\0" // IR(53627, 30)
  "PushConstantGlobalSize\0" // IR(53657, 23)
  "PushConstantRegionOffset\0" // IR(53680, 25)
  "PushConstantNumWorkgroups\0" // IR(53705, 26)
  "PushConstantRegionGroupOffset\0" // IR(53731, 30)
  "ConstantDataStorageBuffer\0" // IR(53761, 26)
  "ConstantDataUniform\0" // IR(53787, 20)
  "PropertyRequiredWorkgroupSize\0" // IR(53807, 30)
  "SpecConstantSubgroupMaxSize\0" // IR(53837, 28)
  "ArgumentPointerPushConstant\0" // IR(53865, 28)
  "ArgumentPointerUniform\0" // IR(53893, 23)
  "ProgramScopeVariablesStorageBuffer\0" // IR(53916, 35)
  "ProgramScopeVariablePointerRelocation\0" // IR(53951, 38)
  "ImageArgumentInfoChannelOrderPushConstant\0" // IR(53989, 42)
  "ImageArgumentInfoChannelDataTypePushConstant\0" // IR(54031, 45)
  "ImageArgumentInfoChannelOrderUniform\0" // IR(54076, 37)
  "ImageArgumentInfoChannelDataTypeUniform\0" // IR(54113, 40)
  "ArgumentStorageTexelBuffer\0" // IR(54153, 27)
  "ArgumentUniformTexelBuffer\0" // IR(54180, 27)
  "ConstantDataPointerPushConstant\0" // IR(54207, 32)
  "ProgramScopeVariablePointerPushConstant\0" // IR(54239, 40)
  "PrintfInfo\0" // IR(54279, 11)
  "PrintfBufferStorageBuffer\0" // IR(54290, 26)
  "PrintfBufferPointerPushConstant\0" // IR(54316, 32)
  "NormalizedSamplerMaskPushConstant\0" // IR(54348, 34)
  "WorkgroupVariableSize\0" // IR(54382, 22)
  "DebugImportedEntity\0" // IR(54404, 20)
  "DebugSource\0" // IR(54424, 12)
  "DebugFunctionDefinition\0" // IR(54436, 24)
  "DebugSourceContinued\0" // IR(54460, 21)
  "DebugLine\0" // IR(54481, 10)
  "DebugNoLine\0" // IR(54491, 12)
  "DebugBuildIdentifier\0" // IR(54503, 21)
  "DebugStoragePath\0" // IR(54524, 17)
  "DebugEntryPoint\0" // IR(54541, 16)
  "DebugTypeMatrix\0" // IR(54557, 16)
  "DebugTypeVectorIdEXT\0" // IR(54573, 21)
  "DebugTypeCooperativeMatrixKHR\0" // IR(54594, 30)
  "Configuration\0" // IR(54624, 14)
  "StartCounter\0" // IR(54638, 13)
  "StopCounter\0" // IR(54651, 12)
  "PushConstants\0" // IR(54663, 14)
  "SpecializationMapEntry\0" // IR(54677, 23)
  "DescriptorSetBuffer\0" // IR(54700, 20)
  "DescriptorSetImage\0" // IR(54720, 19)
  "DescriptorSetSampler\0" // IR(54739, 21)
  "SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING\0" // IR(54760, 63)
  "SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS\0" // IR(54823, 45)
  "SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_TYPE_QUALIFIER\0" // IR(54868, 49)
  "SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_COMPOSITE_TYPE\0" // IR(54917, 49)
  "SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION\0" // IR(54966, 44)
  "SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_IMPORTED_ENTITY\0" // IR(55010, 50)
  "DebugModuleINTEL\0" // IR(55060, 17)
  "acos\0" // IR(55077, 5)
  "acosh\0" // IR(55082, 6)
  "acospi\0" // IR(55088, 7)
  "asin\0" // IR(55095, 5)
  "asinh\0" // IR(55100, 6)
  "asinpi\0" // IR(55106, 7)
  "atan\0" // IR(55113, 5)
  "atan2\0" // IR(55118, 6)
  "atanh\0" // IR(55124, 6)
  "atanpi\0" // IR(55130, 7)
  "atan2pi\0" // IR(55137, 8)
  "cbrt\0" // IR(55145, 5)
  "ceil\0" // IR(55150, 5)
  "copysign\0" // IR(55155, 9)
  "cos\0" // IR(55164, 4)
  "cosh\0" // IR(55168, 5)
  "cospi\0" // IR(55173, 6)
  "erfc\0" // IR(55179, 5)
  "erf\0" // IR(55184, 4)
  "exp\0" // IR(55188, 4)
  "exp2\0" // IR(55192, 5)
  "exp10\0" // IR(55197, 6)
  "expm1\0" // IR(55203, 6)
  "fabs\0" // IR(55209, 5)
  "fdim\0" // IR(55214, 5)
  "floor\0" // IR(55219, 6)
  "fma\0" // IR(55225, 4)
  "fmax\0" // IR(55229, 5)
  "fmin\0" // IR(55234, 5)
  "fmod\0" // IR(55239, 5)
  "fract\0" // IR(55244, 6)
  "frexp\0" // IR(55250, 6)
  "hypot\0" // IR(55256, 6)
  "ilogb\0" // IR(55262, 6)
  "ldexp\0" // IR(55268, 6)
  "lgamma\0" // IR(55274, 7)
  "lgamma_r\0" // IR(55281, 9)
  "log\0" // IR(55290, 4)
  "log2\0" // IR(55294, 5)
  "log10\0" // IR(55299, 6)
  "log1p\0" // IR(55305, 6)
  "logb\0" // IR(55311, 5)
  "mad\0" // IR(55316, 4)
  "maxmag\0" // IR(55320, 7)
  "minmag\0" // IR(55327, 7)
  "modf\0" // IR(55334, 5)
  "nan\0" // IR(55339, 4)
  "nextafter\0" // IR(55343, 10)
  "pow\0" // IR(55353, 4)
  "pown\0" // IR(55357, 5)
  "powr\0" // IR(55362, 5)
  "remainder\0" // IR(55367, 10)
  "remquo\0" // IR(55377, 7)
  "rint\0" // IR(55384, 5)
  "rootn\0" // IR(55389, 6)
  "round\0" // IR(55395, 6)
  "rsqrt\0" // IR(55401, 6)
  "sin\0" // IR(55407, 4)
  "sincos\0" // IR(55411, 7)
  "sinh\0" // IR(55418, 5)
  "sinpi\0" // IR(55423, 6)
  "sqrt\0" // IR(55429, 5)
  "tan\0" // IR(55434, 4)
  "tanh\0" // IR(55438, 5)
  "tanpi\0" // IR(55443, 6)
  "tgamma\0" // IR(55449, 7)
  "trunc\0" // IR(55456, 6)
  "half_cos\0" // IR(55462, 9)
  "half_divide\0" // IR(55471, 12)
  "half_exp\0" // IR(55483, 9)
  "half_exp2\0" // IR(55492, 10)
  "half_exp10\0" // IR(55502, 11)
  "half_log\0" // IR(55513, 9)
  "half_log2\0" // IR(55522, 10)
  "half_log10\0" // IR(55532, 11)
  "half_powr\0" // IR(55543, 10)
  "half_recip\0" // IR(55553, 11)
  "half_rsqrt\0" // IR(55564, 11)
  "half_sin\0" // IR(55575, 9)
  "half_sqrt\0" // IR(55584, 10)
  "half_tan\0" // IR(55594, 9)
  "native_cos\0" // IR(55603, 11)
  "native_divide\0" // IR(55614, 14)
  "native_exp\0" // IR(55628, 11)
  "native_exp2\0" // IR(55639, 12)
  "native_exp10\0" // IR(55651, 13)
  "native_log\0" // IR(55664, 11)
  "native_log2\0" // IR(55675, 12)
  "native_log10\0" // IR(55687, 13)
  "native_powr\0" // IR(55700, 12)
  "native_recip\0" // IR(55712, 13)
  "native_rsqrt\0" // IR(55725, 13)
  "native_sin\0" // IR(55738, 11)
  "native_sqrt\0" // IR(55749, 12)
  "native_tan\0" // IR(55761, 11)
  "fclamp\0" // IR(55772, 7)
  "degrees\0" // IR(55779, 8)
  "fmax_common\0" // IR(55787, 12)
  "fmin_common\0" // IR(55799, 12)
  "mix\0" // IR(55811, 4)
  "radians\0" // IR(55815, 8)
  "step\0" // IR(55823, 5)
  "smoothstep\0" // IR(55828, 11)
  "sign\0" // IR(55839, 5)
  "cross\0" // IR(55844, 6)
  "distance\0" // IR(55850, 9)
  "length\0" // IR(55859, 7)
  "normalize\0" // IR(55866, 10)
  "fast_distance\0" // IR(55876, 14)
  "fast_length\0" // IR(55890, 12)
  "fast_normalize\0" // IR(55902, 15)
  "s_abs\0" // IR(55917, 6)
  "s_abs_diff\0" // IR(55923, 11)
  "s_add_sat\0" // IR(55934, 10)
  "u_add_sat\0" // IR(55944, 10)
  "s_hadd\0" // IR(55954, 7)
  "u_hadd\0" // IR(55961, 7)
  "s_rhadd\0" // IR(55968, 8)
  "u_rhadd\0" // IR(55976, 8)
  "s_clamp\0" // IR(55984, 8)
  "u_clamp\0" // IR(55992, 8)
  "clz\0" // IR(56000, 4)
  "ctz\0" // IR(56004, 4)
  "s_mad_hi\0" // IR(56008, 9)
  "u_mad_sat\0" // IR(56017, 10)
  "s_mad_sat\0" // IR(56027, 10)
  "s_max\0" // IR(56037, 6)
  "u_max\0" // IR(56043, 6)
  "s_min\0" // IR(56049, 6)
  "u_min\0" // IR(56055, 6)
  "s_mul_hi\0" // IR(56061, 9)
  "rotate\0" // IR(56070, 7)
  "s_sub_sat\0" // IR(56077, 10)
  "u_sub_sat\0" // IR(56087, 10)
  "u_upsample\0" // IR(56097, 11)
  "s_upsample\0" // IR(56108, 11)
  "popcount\0" // IR(56119, 9)
  "s_mad24\0" // IR(56128, 8)
  "u_mad24\0" // IR(56136, 8)
  "s_mul24\0" // IR(56144, 8)
  "u_mul24\0" // IR(56152, 8)
  "vloadn\0" // IR(56160, 7)
  "vstoren\0" // IR(56167, 8)
  "vload_half\0" // IR(56175, 11)
  "vload_halfn\0" // IR(56186, 12)
  "vstore_half\0" // IR(56198, 12)
  "vstore_half_r\0" // IR(56210, 14)
  "vstore_halfn\0" // IR(56224, 13)
  "vstore_halfn_r\0" // IR(56237, 15)
  "vloada_halfn\0" // IR(56252, 13)
  "vstorea_halfn\0" // IR(56265, 14)
  "vstorea_halfn_r\0" // IR(56279, 16)
  "shuffle\0" // IR(56295, 8)
  "shuffle2\0" // IR(56303, 9)
  "printf\0" // IR(56312, 7)
  "prefetch\0" // IR(56319, 9)
  "bitselect\0" // IR(56328, 10)
  "select\0" // IR(56338, 7)
  "u_abs\0" // IR(56345, 6)
  "u_abs_diff\0" // IR(56351, 11)
  "u_mul_hi\0" // IR(56362, 9)
  "u_mad_hi\0" // IR(56371, 9)
  "CubeFaceIndexAMD\0" // IR(56380, 17)
  "CubeFaceCoordAMD\0" // IR(56397, 17)
  "TimeAMD\0" // IR(56414, 8)
  "SwizzleInvocationsAMD\0" // IR(56422, 22)
  "SwizzleInvocationsMaskedAMD\0" // IR(56444, 28)
  "WriteInvocationAMD\0" // IR(56472, 19)
  "MbcntAMD\0" // IR(56491, 9)
  "InterpolateAtVertexAMD\0" // IR(56500, 23)
  "FMin3AMD\0" // IR(56523, 9)
  "UMin3AMD\0" // IR(56532, 9)
  "SMin3AMD\0" // IR(56541, 9)
  "FMax3AMD\0" // IR(56550, 9)
  "UMax3AMD\0" // IR(56559, 9)
  "SMax3AMD\0" // IR(56568, 9)
  "FMid3AMD\0" // IR(56577, 9)
  "UMid3AMD\0" // IR(56586, 9)
  "SMid3AMD\0" // IR(56595, 9)
  "ARGMAX\0" // IR(56604, 7)
  "AVG_POOL2D\0" // IR(56611, 11)
  "CONV2D\0" // IR(56622, 7)
  "CONV3D\0" // IR(56629, 7)
  "DEPTHWISE_CONV2D\0" // IR(56636, 17)
  "FFT2D\0" // IR(56653, 6)
  "MATMUL\0" // IR(56659, 7)
  "MAX_POOL2D\0" // IR(56666, 11)
  "RFFT2D\0" // IR(56677, 7)
  "TRANSPOSE_CONV2D\0" // IR(56684, 17)
  "CLAMP\0" // IR(56701, 6)
  "ERF\0" // IR(56707, 4)
  "SIGMOID\0" // IR(56711, 8)
  "TANH\0" // IR(56719, 5)
  "ADD\0" // IR(56724, 4)
  "ARITHMETIC_RIGHT_SHIFT\0" // IR(56728, 23)
  "BITWISE_AND\0" // IR(56751, 12)
  "BITWISE_OR\0" // IR(56763, 11)
  "BITWISE_XOR\0" // IR(56774, 12)
  "INTDIV\0" // IR(56786, 7)
  "LOGICAL_AND\0" // IR(56793, 12)
  "LOGICAL_LEFT_SHIFT\0" // IR(56805, 19)
  "LOGICAL_RIGHT_SHIFT\0" // IR(56824, 20)
  "LOGICAL_OR\0" // IR(56844, 11)
  "LOGICAL_XOR\0" // IR(56855, 12)
  "MAXIMUM\0" // IR(56867, 8)
  "MINIMUM\0" // IR(56875, 8)
  "MUL\0" // IR(56883, 4)
  "POW\0" // IR(56887, 4)
  "SUB\0" // IR(56891, 4)
  "TABLE\0" // IR(56895, 6)
  "ABS\0" // IR(56901, 4)
  "BITWISE_NOT\0" // IR(56905, 12)
  "CEIL\0" // IR(56917, 5)
  "CLZ\0" // IR(56922, 4)
  "COS\0" // IR(56926, 4)
  "EXP\0" // IR(56930, 4)
  "FLOOR\0" // IR(56934, 6)
  "LOG\0" // IR(56940, 4)
  "LOGICAL_NOT\0" // IR(56944, 12)
  "NEGATE\0" // IR(56956, 7)
  "RECIPROCAL\0" // IR(56963, 11)
  "RSQRT\0" // IR(56974, 6)
  "SIN\0" // IR(56980, 4)
  "SELECT\0" // IR(56984, 7)
  "EQUAL\0" // IR(56991, 6)
  "GREATER\0" // IR(56997, 8)
  "GREATER_EQUAL\0" // IR(57005, 14)
  "REDUCE_ALL\0" // IR(57019, 11)
  "REDUCE_ANY\0" // IR(57030, 11)
  "REDUCE_MAX\0" // IR(57041, 11)
  "REDUCE_MIN\0" // IR(57052, 11)
  "REDUCE_PRODUCT\0" // IR(57063, 15)
  "REDUCE_SUM\0" // IR(57078, 11)
  "CONCAT\0" // IR(57089, 7)
  "PAD\0" // IR(57096, 4)
  "RESHAPE\0" // IR(57100, 8)
  "REVERSE\0" // IR(57108, 8)
  "SLICE\0" // IR(57116, 6)
  "TILE\0" // IR(57122, 5)
  "TRANSPOSE\0" // IR(57127, 10)
  "GATHER\0" // IR(57137, 7)
  "SCATTER\0" // IR(57144, 8)
  "RESIZE\0" // IR(57152, 7)
  "CAST\0" // IR(57159, 5)
  "RESCALE\0" // IR(57164, 8)
;

// Array of IndexRanges, where each represents a string by referencing
// the kStrings table.
// This array contains all sequences of alias strings used in the grammar.
// This table is referenced by an IndexRange elsewhere, i.e. by the 'aliases'
// field of an instruction or operand description.
static const IndexRange kAliasSpans[] = {
  IR(5896, 27), // 0 PhysicalStorageBuffer64EXT
  IR(6807, 18), // 1 SubgroupEqMaskKHR
  IR(6862, 18), // 2 SubgroupGeMaskKHR
  IR(6895, 18), // 3 SubgroupGtMaskKHR
  IR(6928, 18), // 4 SubgroupLeMaskKHR
  IR(6961, 18), // 5 SubgroupLtMaskKHR
  IR(7868, 12), // 6 BaryCoordNV
  IR(7923, 19), // 7 BaryCoordNoPerspNV
  IR(7954, 15), // 8 FragmentSizeNV
  IR(8011, 22), // 9 InvocationsPerPixelNV
  IR(8139, 11), // 10 LaunchIdNV
  IR(8164, 13), // 11 LaunchSizeNV
  IR(8195, 17), // 12 WorldRayOriginNV
  IR(8233, 20), // 13 WorldRayDirectionNV
  IR(8272, 18), // 14 ObjectRayOriginNV
  IR(8312, 21), // 15 ObjectRayDirectionNV
  IR(8344, 10), // 16 RayTminNV
  IR(8365, 10), // 17 RayTmaxNV
  IR(8398, 22), // 18 InstanceCustomIndexNV
  IR(8437, 16), // 19 ObjectToWorldNV
  IR(8470, 16), // 20 WorldToObjectNV
  IR(8504, 10), // 21 HitKindNV
  IR(8735, 19), // 22 IncomingRayFlagsNV
  IR(10581, 28), // 23 StorageUniformBufferBlock16
  IR(10644, 17), // 24 StorageUniform16
  IR(11589, 27), // 25 ShaderViewportIndexLayerNV
  IR(11633, 22), // 26 FragmentBarycentricNV
  IR(11686, 30), // 27 ComputeDerivativeGroupQuadsNV
  IR(11716, 14), // 28 ShadingRateNV
  IR(11760, 29), // 29 GroupNonUniformPartitionedNV
  IR(11806, 20), // 30 ShaderNonUniformEXT
  IR(11849, 26), // 31 RuntimeDescriptorArrayEXT
  IR(11911, 39), // 32 InputAttachmentArrayDynamicIndexingEXT
  IR(11989, 42), // 33 UniformTexelBufferArrayDynamicIndexingEXT
  IR(12070, 42), // 34 StorageTexelBufferArrayDynamicIndexingEXT
  IR(12149, 40), // 35 UniformBufferArrayNonUniformIndexingEXT
  IR(12225, 39), // 36 SampledImageArrayNonUniformIndexingEXT
  IR(12301, 40), // 37 StorageBufferArrayNonUniformIndexingEXT
  IR(12377, 39), // 38 StorageImageArrayNonUniformIndexingEXT
  IR(12455, 42), // 39 InputAttachmentArrayNonUniformIndexingEXT
  IR(12539, 45), // 40 UniformTexelBufferArrayNonUniformIndexingEXT
  IR(12626, 45), // 41 StorageTexelBufferArrayNonUniformIndexingEXT
  IR(12689, 21), // 42 VulkanMemoryModelKHR
  IR(12739, 32), // 43 VulkanMemoryModelDeviceScopeKHR
  IR(12771, 34), // 44 PhysicalStorageBufferAddressesEXT
  IR(12837, 31), // 45 ComputeDerivativeGroupLinearNV
  IR(13041, 28), // 46 DemoteToHelperInvocationEXT
  IR(13121, 29), // 47 RayTracingOpacityMicromapEXT
  IR(14143, 26), // 48 FPGAMemoryAttributesINTEL
  IR(14222, 32), // 49 ArbitraryPrecisionIntegersINTEL
  IR(14292, 37), // 50 ArbitraryPrecisionFloatingPointINTEL
  IR(14382, 22), // 51 FPGALoopControlsINTEL
  IR(14477, 24), // 52 FPGAMemoryAccessesINTEL
  IR(14529, 27), // 53 FPGAClusterAttributesINTEL
  IR(14571, 14), // 54 LoopFuseINTEL
  IR(14606, 20), // 55 FPGADSPControlINTEL
  IR(14693, 40), // 56 FPGAInvocationPipeliningAttributesINTEL
  IR(14758, 24), // 57 FPGABufferLocationINTEL
  IR(14817, 34), // 58 ArbitraryPrecisionFixedPointINTEL
  IR(14875, 23), // 59 USMStorageClassesINTEL
  IR(14928, 29), // 60 RuntimeAlignedAttributeINTEL
  IR(14971, 13), // 61 IOPipesINTEL
  IR(15004, 19), // 62 BlockingPipesINTEL
  IR(15037, 13), // 63 FPGARegINTEL
  IR(15069, 22), // 64 DotProductInputAllKHR
  IR(15113, 25), // 65 DotProductInput4x8BitKHR
  IR(15166, 31), // 66 DotProductInput4x8BitPackedKHR
  IR(15208, 14), // 67 DotProductKHR
  IR(15422, 13), // 68 OptNoneINTEL
  IR(15516, 18), // 69 SplitBarrierINTEL
  IR(15583, 29), // 70 FPGAClusterAttributesV2INTEL
  IR(15659, 18), // 71 TaskSequenceINTEL
  IR(15718, 24), // 72 FPGALatencyControlINTEL
  IR(15771, 28), // 73 FPGAArgumentInterfacesINTEL
  IR(15865, 35), // 74 GlobalVariableFPGADecorationsINTEL
  IR(18857, 15), // 75 PerPrimitiveNV
  IR(18905, 12), // 76 PerVertexNV
  IR(18928, 14), // 77 NonUniformEXT
  IR(18958, 19), // 78 RestrictPointerEXT
  IR(18992, 18), // 79 AliasedPointerEXT
  IR(19355, 24), // 80 HlslCounterBufferGOOGLE
  IR(19392, 19), // 81 HlslSemanticGOOGLE
  IR(19522, 14), // 82 RegisterINTEL
  IR(19549, 12), // 83 MemoryINTEL
  IR(19576, 14), // 84 NumbanksINTEL
  IR(19606, 15), // 85 BankwidthINTEL
  IR(19644, 22), // 86 MaxPrivateCopiesINTEL
  IR(19683, 16), // 87 SinglepumpINTEL
  IR(19716, 16), // 88 DoublepumpINTEL
  IR(19752, 19), // 89 MaxReplicatesINTEL
  IR(19792, 20), // 90 SimpleDualPortINTEL
  IR(19824, 11), // 91 MergeINTEL
  IR(19892, 14), // 92 BankBitsINTEL
  IR(19927, 20), // 93 ForcePow2DepthINTEL
  IR(19964, 16), // 94 StridesizeINTEL
  IR(19995, 14), // 95 WordsizeINTEL
  IR(20028, 18), // 96 TrueDualPortINTEL
  IR(20066, 19), // 97 BurstCoalesceINTEL
  IR(20101, 15), // 98 CacheSizeINTEL
  IR(20145, 28), // 99 DontStaticallyCoalesceINTEL
  IR(20188, 14), // 100 PrefetchINTEL
  IR(20220, 17), // 101 StallEnableINTEL
  IR(20263, 25), // 102 FuseLoopsInFunctionINTEL
  IR(20308, 19), // 103 MathOpDSPModeINTEL
  IR(20381, 24), // 104 InitiationIntervalINTEL
  IR(20426, 20), // 105 MaxConcurrencyINTEL
  IR(20467, 20), // 106 PipelineEnableINTEL
  IR(20508, 20), // 107 BufferLocationINTEL
  IR(20548, 19), // 108 IOPipeStorageINTEL
  IR(20726, 15), // 109 StallFreeINTEL
  IR(20824, 25), // 110 LatencyControlLabelINTEL
  IR(20880, 30), // 111 LatencyControlConstraintINTEL
  IR(20938, 27), // 112 ConduitKernelArgumentINTEL
  IR(20997, 31), // 113 RegisterMapKernelArgumentINTEL
  IR(21062, 33), // 114 MMHostInterfaceAddressWidthINTEL
  IR(21126, 30), // 115 MMHostInterfaceDataWidthINTEL
  IR(21185, 28), // 116 MMHostInterfaceLatencyINTEL
  IR(21282, 34), // 117 MMHostInterfaceReadWriteModeINTEL
  IR(21346, 29), // 118 MMHostInterfaceMaxBurstINTEL
  IR(21408, 32), // 119 MMHostInterfaceWaitRequestINTEL
  IR(21467, 26), // 120 StableKernelArgumentINTEL
  IR(21610, 14), // 121 InitModeINTEL
  IR(21653, 28), // 122 ImplementInRegisterMapINTEL
  IR(23007, 14), // 123 OutputLinesNV
  IR(23041, 19), // 124 OutputPrimitivesNV
  IR(23084, 23), // 125 DerivativeGroupQuadsNV
  IR(23132, 24), // 126 DerivativeGroupLinearNV
  IR(23175, 18), // 127 OutputTrianglesNV
  IR(23944, 16), // 128 RayGenerationNV
  IR(23976, 15), // 129 IntersectionNV
  IR(24001, 9), // 130 AnyHitNV
  IR(24024, 13), // 131 ClosestHitNV
  IR(24045, 7), // 132 MissNV
  IR(24064, 11), // 133 CallableNV
  IR(24251, 23), // 134 AllowContractFastINTEL
  IR(24287, 18), // 135 AllowReassocINTEL
  IR(24513, 20), // 136 RuntimeAlignedINTEL
  IR(24657, 20), // 137 PartitionedReduceNV
  IR(24705, 27), // 138 PartitionedInclusiveScanNV
  IR(24760, 27), // 139 PartitionedExclusiveScanNV
  IR(24891, 22), // 140 MakeTexelAvailableKHR
  IR(24930, 20), // 141 MakeTexelVisibleKHR
  IR(24966, 19), // 142 NonPrivateTexelKHR
  IR(24999, 17), // 143 VolatileTexelKHR
  IR(25558, 27), // 144 InitOnDeviceReprogramINTEL
  IR(25609, 23), // 145 InitOnDeviceResetINTEL
  IR(25951, 21), // 146 DependencyArrayINTEL
  IR(25991, 18), // 147 LoopCoalesceINTEL
  IR(26031, 21), // 148 MaxInterleavingINTEL
  IR(26079, 26), // 149 SpeculatedIterationsINTEL
  IR(26120, 14), // 150 NoFusionINTEL
  IR(26150, 15), // 151 LoopCountINTEL
  IR(26192, 26), // 152 MaxReinvocationDelayINTEL
  IR(26584, 24), // 153 MakePointerAvailableKHR
  IR(26627, 22), // 154 MakePointerVisibleKHR
  IR(26667, 21), // 155 NonPrivatePointerKHR
  IR(26754, 10), // 156 VulkanKHR
  IR(16586, 5), // 157 None
  IR(26937, 16), // 158 OutputMemoryKHR
  IR(26967, 17), // 159 MakeAvailableKHR
  IR(26996, 15), // 160 MakeVisibleKHR
  IR(27072, 28), // 161 PackedVectorFormat4x8BitKHR
  IR(27387, 24), // 162 SkipBuiltinPrimitivesNV
  IR(27454, 30), // 163 ForceOpacityMicromap2StateEXT
  IR(28157, 15), // 164 QueueFamilyKHR
  IR(28542, 15), // 165 CallableDataNV
  IR(28581, 23), // 166 IncomingCallableDataNV
  IR(28618, 13), // 167 RayPayloadNV
  IR(28647, 15), // 168 HitAttributeNV
  IR(28684, 21), // 169 IncomingRayPayloadNV
  IR(28727, 21), // 170 ShaderRecordBufferNV
  IR(28770, 25), // 171 PhysicalStorageBufferEXT
  IR(28896, 16), // 172 DeviceOnlyINTEL
  IR(28927, 14), // 173 HostOnlyINTEL
  IR(36182, 8), // 174 SDotKHR
  IR(36195, 8), // 175 UDotKHR
  IR(36209, 9), // 176 SUDotKHR
  IR(36229, 14), // 177 SDotAccSatKHR
  IR(36254, 14), // 178 UDotAccSatKHR
  IR(36280, 15), // 179 SUDotAccSatKHR
  IR(38597, 24), // 180 TypeCooperativeVectorNV
  IR(38853, 27), // 181 GroupNonUniformPartitionNV
  IR(39935, 21), // 182 ReportIntersectionNV
  IR(40110, 28), // 183 TypeAccelerationStructureNV
  IR(40191, 23), // 184 RayQueryGetClusterIdNV
  IR(42359, 21), // 185 DecorateStringGOOGLE
  IR(42401, 27), // 186 MemberDecorateStringGOOGLE
  IR(47658, 28), // 187 ArbitraryFloatSinCosPiINTEL
  IR(47711, 24), // 188 ArbitraryFloatCastINTEL
  IR(47767, 31), // 189 ArbitraryFloatCastFromIntINTEL
  IR(47828, 29), // 190 ArbitraryFloatCastToIntINTEL
  IR(47881, 23), // 191 ArbitraryFloatAddINTEL
  IR(47928, 23), // 192 ArbitraryFloatSubINTEL
  IR(47975, 23), // 193 ArbitraryFloatMulINTEL
  IR(48022, 23), // 194 ArbitraryFloatDivINTEL
  IR(48068, 22), // 195 ArbitraryFloatGTINTEL
  IR(48113, 22), // 196 ArbitraryFloatGEINTEL
  IR(48158, 22), // 197 ArbitraryFloatLTINTEL
  IR(48203, 22), // 198 ArbitraryFloatLEINTEL
  IR(48248, 22), // 199 ArbitraryFloatEQINTEL
  IR(48296, 25), // 200 ArbitraryFloatRecipINTEL
  IR(48347, 25), // 201 ArbitraryFloatRSqrtINTEL
  IR(48397, 24), // 202 ArbitraryFloatCbrtINTEL
  IR(48447, 25), // 203 ArbitraryFloatHypotINTEL
  IR(48497, 24), // 204 ArbitraryFloatSqrtINTEL
  IR(49181, 15), // 205 FixedSqrtINTEL
  IR(49213, 16), // 206 FixedRecipINTEL
  IR(49246, 16), // 207 FixedRsqrtINTEL
  IR(49277, 14), // 208 FixedSinINTEL
  IR(49306, 14), // 209 FixedCosINTEL
  IR(49338, 17), // 210 FixedSinCosINTEL
  IR(49372, 16), // 211 FixedSinPiINTEL
  IR(49405, 16), // 212 FixedCosPiINTEL
  IR(49441, 19), // 213 FixedSinCosPiINTEL
  IR(49475, 14), // 214 FixedLogINTEL
  IR(49504, 14), // 215 FixedExpINTEL
  IR(49548, 29), // 216 PtrCastToCrossWorkgroupINTEL
  IR(49607, 29), // 217 CrossWorkgroupCastToPtrINTEL
  IR(49659, 22), // 218 ReadPipeBlockingINTEL
  IR(49705, 23), // 219 WritePipeBlockingINTEL
  IR(50606, 26), // 220 ControlBarrierArriveINTEL
  IR(50654, 24), // 221 ControlBarrierWaitINTEL
  IR(50703, 24), // 222 TaskSequenceCreateINTEL
  IR(50751, 23), // 223 TaskSequenceAsyncINTEL
  IR(50796, 21), // 224 TaskSequenceGetINTEL
  IR(50843, 25), // 225 TaskSequenceReleaseINTEL
  IR(50891, 22), // 226 TypeTaskSequenceINTEL
};

// Array of capabilities, referenced by IndexRanges elsewhere.
// Contains all sequences of capabilities used in the grammar.
static const spv::Capability kCapabilitySpans[] = {
  spv::Capability::Kernel, // 0
  spv::Capability::Addresses, // 1
  spv::Capability::PhysicalStorageBufferAddresses, // 2
  spv::Capability::Shader, // 3
  spv::Capability::ClipDistance, // 4
  spv::Capability::CullDistance, // 5
  spv::Capability::Geometry, // 6
  spv::Capability::Tessellation, // 7
  spv::Capability::RayTracingNV, // 8
  spv::Capability::RayTracingKHR, // 9
  spv::Capability::MeshShadingNV, // 10
  spv::Capability::MeshShadingEXT, // 11
  spv::Capability::Geometry, // 12
  spv::Capability::Tessellation, // 13
  spv::Capability::Geometry, // 14
  spv::Capability::ShaderLayer, // 15
  spv::Capability::ShaderViewportIndexLayerEXT, // 16
  spv::Capability::MeshShadingNV, // 17
  spv::Capability::MeshShadingEXT, // 18
  spv::Capability::MultiViewport, // 19
  spv::Capability::ShaderViewportIndex, // 20
  spv::Capability::ShaderViewportIndexLayerEXT, // 21
  spv::Capability::MeshShadingNV, // 22
  spv::Capability::MeshShadingEXT, // 23
  spv::Capability::Tessellation, // 24
  spv::Capability::SampleRateShading, // 25
  spv::Capability::Kernel, // 26
  spv::Capability::GroupNonUniform, // 27
  spv::Capability::SubgroupBallotKHR, // 28
  spv::Capability::Kernel, // 29
  spv::Capability::GroupNonUniform, // 30
  spv::Capability::CoreBuiltinsARM, // 31
  spv::Capability::SubgroupBallotKHR, // 32
  spv::Capability::GroupNonUniformBallot, // 33
  spv::Capability::DrawParameters, // 34
  spv::Capability::DrawParameters, // 35
  spv::Capability::MeshShadingNV, // 36
  spv::Capability::MeshShadingEXT, // 37
  spv::Capability::FragmentShadingRateKHR, // 38
  spv::Capability::DeviceGroup, // 39
  spv::Capability::MultiView, // 40
  spv::Capability::TileShadingQCOM, // 41
  spv::Capability::StencilExportEXT, // 42
  spv::Capability::ShaderEnqueueAMDX, // 43
  spv::Capability::DescriptorHeapEXT, // 44
  spv::Capability::ShaderViewportMaskNV, // 45
  spv::Capability::MeshShadingNV, // 46
  spv::Capability::ShaderStereoViewNV, // 47
  spv::Capability::PerViewAttributesNV, // 48
  spv::Capability::MeshShadingNV, // 49
  spv::Capability::FragmentFullyCoveredEXT, // 50
  spv::Capability::MeshShadingNV, // 51
  spv::Capability::FragmentBarycentricKHR, // 52
  spv::Capability::FragmentDensityEXT, // 53
  spv::Capability::MeshShadingEXT, // 54
  spv::Capability::RayTracingNV, // 55
  spv::Capability::RayTracingKHR, // 56
  spv::Capability::RayTracingNV, // 57
  spv::Capability::RayTracingMotionBlurNV, // 58
  spv::Capability::RayTracingPositionFetchKHR, // 59
  spv::Capability::RayTracingDisplacementMicromapNV, // 60
  spv::Capability::RayTracingKHR, // 61
  spv::Capability::RayTracingSpheresGeometryNV, // 62
  spv::Capability::RayTracingLinearSweptSpheresGeometryNV, // 63
  spv::Capability::ShaderSMBuiltinsNV, // 64
  spv::Capability::RayTracingClusterAccelerationStructureNV, // 65
  spv::Capability::RayCullMaskKHR, // 66
  spv::Capability::Matrix, // 67
  spv::Capability::Int64, // 68
  spv::Capability::ImageBasic, // 69
  spv::Capability::Geometry, // 70
  spv::Capability::SampledCubeArray, // 71
  spv::Capability::SampledRect, // 72
  spv::Capability::Sampled1D, // 73
  spv::Capability::SampledBuffer, // 74
  spv::Capability::DeviceEnqueue, // 75
  spv::Capability::Pipes, // 76
  spv::Capability::GroupNonUniform, // 77
  spv::Capability::Float8EXT, // 78
  spv::Capability::CooperativeMatrixKHR, // 79
  spv::Capability::WorkgroupMemoryExplicitLayoutKHR, // 80
  spv::Capability::StorageBuffer16BitAccess, // 81
  spv::Capability::VariablePointersStorageBuffer, // 82
  spv::Capability::AtomicStorage, // 83
  spv::Capability::StorageBuffer8BitAccess, // 84
  spv::Capability::RayQueryKHR, // 85
  spv::Capability::RayTracingKHR, // 86
  spv::Capability::CooperativeMatrixKHR, // 87
  spv::Capability::Int4TypeINTEL, // 88
  spv::Capability::CooperativeMatrixKHR, // 89
  spv::Capability::BFloat16TypeKHR, // 90
  spv::Capability::BFloat16TypeKHR, // 91
  spv::Capability::CooperativeMatrixKHR, // 92
  spv::Capability::UntypedPointersKHR, // 93
  spv::Capability::Linkage, // 94
  spv::Capability::MultiViewport, // 95
  spv::Capability::ShaderViewportIndexLayerEXT, // 96
  spv::Capability::ShaderViewportMaskNV, // 97
  spv::Capability::InputAttachment, // 98
  spv::Capability::ImageBuffer, // 99
  spv::Capability::ShaderNonUniform, // 100
  spv::Capability::InputAttachment, // 101
  spv::Capability::ShaderNonUniform, // 102
  spv::Capability::SampledBuffer, // 103
  spv::Capability::ShaderNonUniform, // 104
  spv::Capability::ImageBuffer, // 105
  spv::Capability::ShaderNonUniform, // 106
  spv::Capability::CooperativeMatrixBlockLoadsNV, // 107
  spv::Capability::VectorAnyINTEL, // 108
  spv::Capability::Int8, // 109
  spv::Capability::FPGAClusterAttributesALTERA, // 110
  spv::Capability::FPGAKernelAttributesINTEL, // 111
  spv::Capability::Subgroup2DBlockIOINTEL, // 112
  spv::Capability::VariableLengthArrayINTEL, // 113
  spv::Capability::UntypedPointersKHR, // 114
  spv::Capability::SpecConditionalINTEL, // 115
  spv::Capability::Float16, // 116
  spv::Capability::Float8EXT, // 117
  spv::Capability::CooperativeMatrixLayoutsARM, // 118
  spv::Capability::Shader, // 119
  spv::Capability::Kernel, // 120
  spv::Capability::Shader, // 121
  spv::Capability::UniformDecoration, // 122
  spv::Capability::GeometryStreams, // 123
  spv::Capability::TransformFeedback, // 124
  spv::Capability::Kernel, // 125
  spv::Capability::FloatControls2, // 126
  spv::Capability::ConstantDataKHR, // 127
  spv::Capability::SampleMaskOverrideCoverageNV, // 128
  spv::Capability::GeometryShaderPassthroughNV, // 129
  spv::Capability::MeshShadingNV, // 130
  spv::Capability::MeshShadingEXT, // 131
  spv::Capability::PushConstantBanksNV, // 132
  spv::Capability::ShaderInvocationReorderNV, // 133
  spv::Capability::ShaderInvocationReorderEXT, // 134
  spv::Capability::BindlessTextureNV, // 135
  spv::Capability::VectorComputeINTEL, // 136
  spv::Capability::IndirectReferencesINTEL, // 137
  spv::Capability::AsmINTEL, // 138
  spv::Capability::FunctionFloatControlINTEL, // 139
  spv::Capability::FPGAMemoryAttributesALTERA, // 140
  spv::Capability::FPGAMemoryAccessesALTERA, // 141
  spv::Capability::LoopFuseALTERA, // 142
  spv::Capability::FPGADSPControlALTERA, // 143
  spv::Capability::MemoryAccessAliasingINTEL, // 144
  spv::Capability::FPGAInvocationPipeliningAttributesALTERA, // 145
  spv::Capability::FPGABufferLocationALTERA, // 146
  spv::Capability::IOPipesALTERA, // 147
  spv::Capability::FPGAClusterAttributesV2ALTERA, // 148
  spv::Capability::FPMaxErrorINTEL, // 149
  spv::Capability::FPGALatencyControlALTERA, // 150
  spv::Capability::FPGAArgumentInterfacesALTERA, // 151
  spv::Capability::GlobalVariableHostAccessINTEL, // 152
  spv::Capability::GlobalVariableFPGADecorationsALTERA, // 153
  spv::Capability::CacheControlsINTEL, // 154
  spv::Capability::TileImageColorReadAccessEXT, // 155
  spv::Capability::Geometry, // 156
  spv::Capability::Tessellation, // 157
  spv::Capability::MeshShadingNV, // 158
  spv::Capability::MeshShadingEXT, // 159
  spv::Capability::Geometry, // 160
  spv::Capability::MeshShadingNV, // 161
  spv::Capability::MeshShadingEXT, // 162
  spv::Capability::SubgroupDispatch, // 163
  spv::Capability::TileImageDepthReadAccessEXT, // 164
  spv::Capability::TileImageStencilReadAccessEXT, // 165
  spv::Capability::SampleMaskPostDepthCoverage, // 166
  spv::Capability::DenormPreserve, // 167
  spv::Capability::DenormFlushToZero, // 168
  spv::Capability::SignedZeroInfNanPreserve, // 169
  spv::Capability::RoundingModeRTE, // 170
  spv::Capability::RoundingModeRTZ, // 171
  spv::Capability::QuadControlKHR, // 172
  spv::Capability::PoisonFreezeKHR, // 173
  spv::Capability::ComputeDerivativeGroupQuadsKHR, // 174
  spv::Capability::ComputeDerivativeGroupLinearKHR, // 175
  spv::Capability::FragmentShaderPixelInterlockEXT, // 176
  spv::Capability::FragmentShaderSampleInterlockEXT, // 177
  spv::Capability::FragmentShaderShadingRateInterlockEXT, // 178
  spv::Capability::Shader64BitIndexingEXT, // 179
  spv::Capability::RoundToInfinityINTEL, // 180
  spv::Capability::KernelAttributesINTEL, // 181
  spv::Capability::FloatControls2, // 182
  spv::Capability::RayTracingOpacityMicromapExecutionModeKHR, // 183
  spv::Capability::FPGAKernelAttributesv2INTEL, // 184
  spv::Capability::RegisterLimitsINTEL, // 185
  spv::Capability::Float6EXT, // 186
  spv::Capability::Float4EXT, // 187
  spv::Capability::Float8UnsignedE8M0EXT, // 188
  spv::Capability::MXInt8EXT, // 189
  spv::Capability::FloatControls2, // 190
  spv::Capability::FPFastMathModeINTEL, // 191
  spv::Capability::OptNoneEXT, // 192
  spv::Capability::RuntimeAlignedAttributeALTERA, // 193
  spv::Capability::Kernel, // 194
  spv::Capability::GroupNonUniformArithmetic, // 195
  spv::Capability::GroupNonUniformBallot, // 196
  spv::Capability::GroupNonUniformClustered, // 197
  spv::Capability::GroupNonUniformPartitionedEXT, // 198
  spv::Capability::ImageGatherExtended, // 199
  spv::Capability::MinLod, // 200
  spv::Capability::VulkanMemoryModel, // 201
  spv::Capability::WeakLinkageAMD, // 202
  spv::Capability::FPGALoopControlsALTERA, // 203
  spv::Capability::ArbitraryPrecisionFixedPointALTERA, // 204
  spv::Capability::RawAccessChainsNV, // 205
  spv::Capability::RayTraversalPrimitiveCullingKHR, // 206
  spv::Capability::RayTracingOpacityMicromapKHR, // 207
  spv::Capability::RayQueryKHR, // 208
  spv::Capability::StorageImageExtendedFormats, // 209
  spv::Capability::Int64ImageEXT, // 210
  spv::Capability::Shader, // 211
  spv::Capability::VectorComputeINTEL, // 212
  spv::Capability::GenericPointer, // 213
  spv::Capability::FunctionPointersINTEL, // 214
  spv::Capability::USMStorageClassesALTERA, // 215
  spv::Capability::CooperativeMatrixTensorAddressingNV, // 216
  spv::Capability::CooperativeMatrixDecodeVectorNV, // 217
  spv::Capability::TensorsARM, // 218
  spv::Capability::Addresses, // 219
  spv::Capability::PhysicalStorageBufferAddresses, // 220
  spv::Capability::LiteralSampler, // 221
  spv::Capability::Addresses, // 222
  spv::Capability::UntypedPointersKHR, // 223
  spv::Capability::Addresses, // 224
  spv::Capability::VariablePointers, // 225
  spv::Capability::VariablePointersStorageBuffer, // 226
  spv::Capability::PhysicalStorageBufferAddresses, // 227
  spv::Capability::Kernel, // 228
  spv::Capability::ImageQuery, // 229
  spv::Capability::ImageQuery, // 230
  spv::Capability::Shader, // 231
  spv::Capability::BitInstructions, // 232
  spv::Capability::DerivativeControl, // 233
  spv::Capability::Groups, // 234
  spv::Capability::SparseResidency, // 235
  spv::Capability::PipeStorage, // 236
  spv::Capability::NamedBarrier, // 237
  spv::Capability::GroupNonUniformVote, // 238
  spv::Capability::GroupNonUniformBallot, // 239
  spv::Capability::GroupNonUniformShuffle, // 240
  spv::Capability::GroupNonUniformShuffleRelative, // 241
  spv::Capability::GroupNonUniformArithmetic, // 242
  spv::Capability::GroupNonUniformClustered, // 243
  spv::Capability::GroupNonUniformPartitionedEXT, // 244
  spv::Capability::GroupNonUniformQuad, // 245
  spv::Capability::Addresses, // 246
  spv::Capability::VariablePointers, // 247
  spv::Capability::VariablePointersStorageBuffer, // 248
  spv::Capability::GraphARM, // 249
  spv::Capability::BitcastExtractEXT, // 250
  spv::Capability::SubgroupBallotKHR, // 251
  spv::Capability::FMAKHR, // 252
  spv::Capability::SubgroupVoteKHR, // 253
  spv::Capability::GroupNonUniformRotateKHR, // 254
  spv::Capability::RayTracingKHR, // 255
  spv::Capability::RayQueryKHR, // 256
  spv::Capability::DotProduct, // 257
  spv::Capability::ReplicatedCompositesEXT, // 258
  spv::Capability::TextureSampleWeightedQCOM, // 259
  spv::Capability::TextureBoxFilterQCOM, // 260
  spv::Capability::TextureBlockMatchQCOM, // 261
  spv::Capability::CooperativeMatrixConversionQCOM, // 262
  spv::Capability::TextureBlockMatch2QCOM, // 263
  spv::Capability::ImageGatherLinearQCOM, // 264
  spv::Capability::ImageGatherExtendedModesQCOM, // 265
  spv::Capability::FragmentMaskAMD, // 266
  spv::Capability::ShaderClockKHR, // 267
  spv::Capability::AbortKHR, // 268
  spv::Capability::ShaderInvocationReorderNV, // 269
  spv::Capability::RayTracingMotionBlurNV, // 270
  spv::Capability::ImageFootprintNV, // 271
  spv::Capability::CooperativeVectorNV, // 272
  spv::Capability::LongVectorEXT, // 273
  spv::Capability::CooperativeVectorNV, // 274
  spv::Capability::CooperativeVectorTrainingNV, // 275
  spv::Capability::CooperativeMatrixConversionsNV, // 276
  spv::Capability::DisplacementMicromapNV, // 277
  spv::Capability::ShaderInvocationReorderEXT, // 278
  spv::Capability::RayTracingMotionBlurNV, // 279
  spv::Capability::RayQueryPositionFetchKHR, // 280
  spv::Capability::RayTracingNV, // 281
  spv::Capability::RayTracingKHR, // 282
  spv::Capability::RayQueryKHR, // 283
  spv::Capability::DisplacementMicromapNV, // 284
  spv::Capability::CooperativeMatrixNV, // 285
  spv::Capability::FragmentShaderSampleInterlockEXT, // 286
  spv::Capability::FragmentShaderPixelInterlockEXT, // 287
  spv::Capability::FragmentShaderShadingRateInterlockEXT, // 288
  spv::Capability::CooperativeMatrixReductionsNV, // 289
  spv::Capability::CooperativeMatrixPerElementOperationsNV, // 290
  spv::Capability::TensorAddressingNV, // 291
  spv::Capability::DemoteToHelperInvocation, // 292
  spv::Capability::SubgroupShuffleINTEL, // 293
  spv::Capability::SubgroupBufferBlockIOINTEL, // 294
  spv::Capability::SubgroupImageBlockIOINTEL, // 295
  spv::Capability::SubgroupImageMediaBlockIOINTEL, // 296
  spv::Capability::IntegerFunctions2INTEL, // 297
  spv::Capability::AtomicFloat16MinMaxEXT, // 298
  spv::Capability::AtomicFloat32MinMaxEXT, // 299
  spv::Capability::AtomicFloat64MinMaxEXT, // 300
  spv::Capability::AtomicFloat16VectorNV, // 301
  spv::Capability::ExpectAssumeKHR, // 302
  spv::Capability::SubgroupAvcMotionEstimationINTEL, // 303
  spv::Capability::SubgroupAvcMotionEstimationINTEL, // 304
  spv::Capability::SubgroupAvcMotionEstimationIntraINTEL, // 305
  spv::Capability::SubgroupAvcMotionEstimationINTEL, // 306
  spv::Capability::SubgroupAvcMotionEstimationChromaINTEL, // 307
  spv::Capability::VariableLengthArrayINTEL, // 308
  spv::Capability::ArbitraryPrecisionFloatingPointALTERA, // 309
  spv::Capability::UnstructuredLoopControlsINTEL, // 310
  spv::Capability::BlockingPipesALTERA, // 311
  spv::Capability::FPGARegALTERA, // 312
  spv::Capability::AtomicFloat16AddEXT, // 313
  spv::Capability::AtomicFloat32AddEXT, // 314
  spv::Capability::AtomicFloat64AddEXT, // 315
  spv::Capability::AtomicFloat16VectorNV, // 316
  spv::Capability::LongCompositesINTEL, // 317
  spv::Capability::BFloat16ConversionINTEL, // 318
  spv::Capability::SplitBarrierEXT, // 319
  spv::Capability::ArithmeticFenceEXT, // 320
  spv::Capability::TaskSequenceALTERA, // 321
  spv::Capability::SubgroupBufferPrefetchINTEL, // 322
  spv::Capability::Subgroup2DBlockTransformINTEL, // 323
  spv::Capability::Subgroup2DBlockTransposeINTEL, // 324
  spv::Capability::SubgroupMatrixMultiplyAccumulateINTEL, // 325
  spv::Capability::TernaryBitwiseFunctionINTEL, // 326
  spv::Capability::UntypedVariableLengthArrayINTEL, // 327
  spv::Capability::FunctionVariantsINTEL, // 328
  spv::Capability::PredicatedIOINTEL, // 329
  spv::Capability::GroupUniformArithmeticKHR, // 330
  spv::Capability::TensorFloat32RoundingINTEL, // 331
  spv::Capability::MaskedGatherScatterINTEL, // 332
  spv::Capability::BindlessImagesINTEL, // 333
  spv::Capability::DotProductFloat16AccFloat32VALVE, // 334
  spv::Capability::DotProductBFloat16AccVALVE, // 335
  spv::Capability::DotProductFloat16AccFloat16VALVE, // 336
  spv::Capability::DotProductBFloat16AccVALVE, // 337
  spv::Capability::DotProductFloat8AccFloat32VALVE, // 338
  spv::Capability::Float64, // 339
  spv::Capability::InterpolationFunction, // 340
};

// Array of extensions, referenced by IndexRanges elsewhere.
// Contains all sequences of extensions used in the grammar.
static const spvtools::Extension kExtensionSpans[] = {
  spvtools::Extension::kSPV_EXT_physical_storage_buffer, // 0
  spvtools::Extension::kSPV_KHR_physical_storage_buffer, // 1
  spvtools::Extension::kSPV_KHR_shader_ballot, // 2
  spvtools::Extension::kSPV_KHR_shader_draw_parameters, // 3
  spvtools::Extension::kSPV_KHR_shader_draw_parameters, // 4
  spvtools::Extension::kSPV_NV_mesh_shader, // 5
  spvtools::Extension::kSPV_EXT_mesh_shader, // 6
  spvtools::Extension::kSPV_KHR_fragment_shading_rate, // 7
  spvtools::Extension::kSPV_KHR_device_group, // 8
  spvtools::Extension::kSPV_KHR_multiview, // 9
  spvtools::Extension::kSPV_AMD_shader_explicit_vertex_parameter, // 10
  spvtools::Extension::kSPV_EXT_shader_stencil_export, // 11
  spvtools::Extension::kSPV_NV_viewport_array2, // 12
  spvtools::Extension::kSPV_NV_mesh_shader, // 13
  spvtools::Extension::kSPV_NV_stereo_view_rendering, // 14
  spvtools::Extension::kSPV_NVX_multiview_per_view_attributes, // 15
  spvtools::Extension::kSPV_NV_mesh_shader, // 16
  spvtools::Extension::kSPV_EXT_fragment_fully_covered, // 17
  spvtools::Extension::kSPV_NV_mesh_shader, // 18
  spvtools::Extension::kSPV_NV_fragment_shader_barycentric, // 19
  spvtools::Extension::kSPV_KHR_fragment_shader_barycentric, // 20
  spvtools::Extension::kSPV_EXT_fragment_invocation_density, // 21
  spvtools::Extension::kSPV_NV_shading_rate, // 22
  spvtools::Extension::kSPV_EXT_mesh_shader, // 23
  spvtools::Extension::kSPV_NV_ray_tracing, // 24
  spvtools::Extension::kSPV_KHR_ray_tracing, // 25
  spvtools::Extension::kSPV_NV_ray_tracing, // 26
  spvtools::Extension::kSPV_NV_ray_tracing_motion_blur, // 27
  spvtools::Extension::kSPV_KHR_ray_tracing, // 28
  spvtools::Extension::kSPV_NV_linear_swept_spheres, // 29
  spvtools::Extension::kSPV_NV_shader_sm_builtins, // 30
  spvtools::Extension::kSPV_NV_cluster_acceleration_structure, // 31
  spvtools::Extension::kSPV_KHR_ray_cull_mask, // 32
  spvtools::Extension::kSPV_AMD_shader_ballot, // 33
  spvtools::Extension::kSPV_ARM_core_builtins, // 34
  spvtools::Extension::kSPV_EXT_shader_tile_image, // 35
  spvtools::Extension::kSPV_ARM_tensors, // 36
  spvtools::Extension::kSPV_ARM_graph, // 37
  spvtools::Extension::kSPV_ARM_cooperative_matrix_layouts, // 38
  spvtools::Extension::kSPV_EXT_float8, // 39
  spvtools::Extension::kSPV_EXT_ocp_microscaling_types, // 40
  spvtools::Extension::kSPV_KHR_workgroup_memory_explicit_layout, // 41
  spvtools::Extension::kSPV_KHR_subgroup_vote, // 42
  spvtools::Extension::kSPV_KHR_16bit_storage, // 43
  spvtools::Extension::kSPV_KHR_variable_pointers, // 44
  spvtools::Extension::kSPV_KHR_shader_atomic_counter_ops, // 45
  spvtools::Extension::kSPV_KHR_post_depth_coverage, // 46
  spvtools::Extension::kSPV_KHR_8bit_storage, // 47
  spvtools::Extension::kSPV_KHR_float_controls, // 48
  spvtools::Extension::kSPV_KHR_ray_query, // 49
  spvtools::Extension::kSPV_KHR_untyped_pointers, // 50
  spvtools::Extension::kSPV_KHR_ray_query, // 51
  spvtools::Extension::kSPV_KHR_ray_tracing, // 52
  spvtools::Extension::kSPV_QCOM_image_processing, // 53
  spvtools::Extension::kSPV_QCOM_tile_shading, // 54
  spvtools::Extension::kSPV_QCOM_cooperative_matrix_conversion, // 55
  spvtools::Extension::kSPV_QCOM_image_processing2, // 56
  spvtools::Extension::kSPV_QCOM_multiple_wait_queues, // 57
  spvtools::Extension::kSPV_QCOM_image_processing3, // 58
  spvtools::Extension::kSPV_AMD_gpu_shader_half_float_fetch, // 59
  spvtools::Extension::kSPV_AMD_texture_gather_bias_lod, // 60
  spvtools::Extension::kSPV_AMD_shader_fragment_mask, // 61
  spvtools::Extension::kSPV_AMD_shader_image_load_store_lod, // 62
  spvtools::Extension::kSPV_EXT_shader_image_int64, // 63
  spvtools::Extension::kSPV_KHR_shader_clock, // 64
  spvtools::Extension::kSPV_AMDX_shader_enqueue, // 65
  spvtools::Extension::kSPV_KHR_quad_control, // 66
  spvtools::Extension::kSPV_INTEL_int4, // 67
  spvtools::Extension::kSPV_KHR_bfloat16, // 68
  spvtools::Extension::kSPV_KHR_abort, // 69
  spvtools::Extension::kSPV_EXT_descriptor_heap, // 70
  spvtools::Extension::kSPV_KHR_constant_data, // 71
  spvtools::Extension::kSPV_KHR_poison_freeze, // 72
  spvtools::Extension::kSPV_AMD_weak_linkage, // 73
  spvtools::Extension::kSPV_NV_sample_mask_override_coverage, // 74
  spvtools::Extension::kSPV_NV_geometry_shader_passthrough, // 75
  spvtools::Extension::kSPV_EXT_shader_viewport_index_layer, // 76
  spvtools::Extension::kSPV_NV_viewport_array2, // 77
  spvtools::Extension::kSPV_NV_viewport_array2, // 78
  spvtools::Extension::kSPV_NVX_multiview_per_view_attributes, // 79
  spvtools::Extension::kSPV_NV_shader_image_footprint, // 80
  spvtools::Extension::kSPV_NV_compute_shader_derivatives, // 81
  spvtools::Extension::kSPV_KHR_compute_shader_derivatives, // 82
  spvtools::Extension::kSPV_NV_shader_subgroup_partitioned, // 83
  spvtools::Extension::kSPV_EXT_shader_subgroup_partitioned, // 84
  spvtools::Extension::kSPV_EXT_descriptor_indexing, // 85
  spvtools::Extension::kSPV_KHR_ray_tracing_position_fetch, // 86
  spvtools::Extension::kSPV_KHR_vulkan_memory_model, // 87
  spvtools::Extension::kSPV_NV_cooperative_matrix, // 88
  spvtools::Extension::kSPV_EXT_fragment_shader_interlock, // 89
  spvtools::Extension::kSPV_EXT_demote_to_helper_invocation, // 90
  spvtools::Extension::kSPV_NV_displacement_micromap, // 91
  spvtools::Extension::kSPV_KHR_opacity_micromap, // 92
  spvtools::Extension::kSPV_EXT_opacity_micromap, // 93
  spvtools::Extension::kSPV_NV_shader_invocation_reorder, // 94
  spvtools::Extension::kSPV_EXT_shader_invocation_reorder, // 95
  spvtools::Extension::kSPV_NV_bindless_texture, // 96
  spvtools::Extension::kSPV_NV_cooperative_vector, // 97
  spvtools::Extension::kSPV_NV_shader_atomic_fp16_vector, // 98
  spvtools::Extension::kSPV_NV_raw_access_chains, // 99
  spvtools::Extension::kSPV_NV_push_constant_bank, // 100
  spvtools::Extension::kSPV_EXT_long_vector, // 101
  spvtools::Extension::kSPV_EXT_shader_64bit_indexing, // 102
  spvtools::Extension::kSPV_NV_cooperative_matrix2, // 103
  spvtools::Extension::kSPV_NV_tensor_addressing, // 104
  spvtools::Extension::kSPV_NV_cooperative_matrix_decode_vector, // 105
  spvtools::Extension::kSPV_INTEL_subgroups, // 106
  spvtools::Extension::kSPV_INTEL_media_block_io, // 107
  spvtools::Extension::kSPV_INTEL_float_controls2, // 108
  spvtools::Extension::kSPV_INTEL_shader_integer_functions2, // 109
  spvtools::Extension::kSPV_INTEL_function_pointers, // 110
  spvtools::Extension::kSPV_INTEL_inline_assembly, // 111
  spvtools::Extension::kSPV_EXT_shader_atomic_float_min_max, // 112
  spvtools::Extension::kSPV_INTEL_vector_compute, // 113
  spvtools::Extension::kSPV_KHR_expect_assume, // 114
  spvtools::Extension::kSPV_INTEL_device_side_avc_motion_estimation, // 115
  spvtools::Extension::kSPV_INTEL_variable_length_array, // 116
  spvtools::Extension::kSPV_ALTERA_fpga_memory_attributes, // 117
  spvtools::Extension::kSPV_INTEL_fpga_memory_attributes, // 118
  spvtools::Extension::kSPV_INTEL_fp_fast_math_mode, // 119
  spvtools::Extension::kSPV_ALTERA_arbitrary_precision_integers, // 120
  spvtools::Extension::kSPV_INTEL_arbitrary_precision_integers, // 121
  spvtools::Extension::kSPV_ALTERA_arbitrary_precision_floating_point, // 122
  spvtools::Extension::kSPV_INTEL_arbitrary_precision_floating_point, // 123
  spvtools::Extension::kSPV_INTEL_unstructured_loop_controls, // 124
  spvtools::Extension::kSPV_ALTERA_fpga_loop_controls, // 125
  spvtools::Extension::kSPV_INTEL_fpga_loop_controls, // 126
  spvtools::Extension::kSPV_INTEL_kernel_attributes, // 127
  spvtools::Extension::kSPV_ALTERA_fpga_memory_accesses, // 128
  spvtools::Extension::kSPV_INTEL_fpga_memory_accesses, // 129
  spvtools::Extension::kSPV_ALTERA_fpga_cluster_attributes, // 130
  spvtools::Extension::kSPV_INTEL_fpga_cluster_attributes, // 131
  spvtools::Extension::kSPV_ALTERA_loop_fuse, // 132
  spvtools::Extension::kSPV_INTEL_loop_fuse, // 133
  spvtools::Extension::kSPV_ALTERA_fpga_dsp_control, // 134
  spvtools::Extension::kSPV_INTEL_fpga_dsp_control, // 135
  spvtools::Extension::kSPV_INTEL_memory_access_aliasing, // 136
  spvtools::Extension::kSPV_ALTERA_fpga_invocation_pipelining_attributes, // 137
  spvtools::Extension::kSPV_INTEL_fpga_invocation_pipelining_attributes, // 138
  spvtools::Extension::kSPV_ALTERA_fpga_buffer_location, // 139
  spvtools::Extension::kSPV_INTEL_fpga_buffer_location, // 140
  spvtools::Extension::kSPV_ALTERA_arbitrary_precision_fixed_point, // 141
  spvtools::Extension::kSPV_INTEL_arbitrary_precision_fixed_point, // 142
  spvtools::Extension::kSPV_ALTERA_usm_storage_classes, // 143
  spvtools::Extension::kSPV_INTEL_usm_storage_classes, // 144
  spvtools::Extension::kSPV_ALTERA_runtime_aligned, // 145
  spvtools::Extension::kSPV_INTEL_runtime_aligned, // 146
  spvtools::Extension::kSPV_ALTERA_io_pipes, // 147
  spvtools::Extension::kSPV_INTEL_io_pipes, // 148
  spvtools::Extension::kSPV_ALTERA_blocking_pipes, // 149
  spvtools::Extension::kSPV_INTEL_blocking_pipes, // 150
  spvtools::Extension::kSPV_ALTERA_fpga_reg, // 151
  spvtools::Extension::kSPV_INTEL_fpga_reg, // 152
  spvtools::Extension::kSPV_KHR_integer_dot_product, // 153
  spvtools::Extension::kSPV_KHR_cooperative_matrix, // 154
  spvtools::Extension::kSPV_EXT_replicated_composites, // 155
  spvtools::Extension::kSPV_KHR_bit_instructions, // 156
  spvtools::Extension::kSPV_KHR_subgroup_rotate, // 157
  spvtools::Extension::kSPV_KHR_float_controls2, // 158
  spvtools::Extension::kSPV_KHR_fma, // 159
  spvtools::Extension::kSPV_KHR_opacity_micromap, // 160
  spvtools::Extension::kSPV_EXT_shader_atomic_float_add, // 161
  spvtools::Extension::kSPV_INTEL_long_composites, // 162
  spvtools::Extension::kSPV_EXT_optnone, // 163
  spvtools::Extension::kSPV_INTEL_optnone, // 164
  spvtools::Extension::kSPV_EXT_shader_atomic_float16_add, // 165
  spvtools::Extension::kSPV_INTEL_debug_module, // 166
  spvtools::Extension::kSPV_INTEL_bfloat16_conversion, // 167
  spvtools::Extension::kSPV_EXT_split_barrier, // 168
  spvtools::Extension::kSPV_INTEL_split_barrier, // 169
  spvtools::Extension::kSPV_EXT_arithmetic_fence, // 170
  spvtools::Extension::kSPV_ALTERA_task_sequence, // 171
  spvtools::Extension::kSPV_INTEL_task_sequence, // 172
  spvtools::Extension::kSPV_INTEL_fp_max_error, // 173
  spvtools::Extension::kSPV_ALTERA_fpga_latency_control, // 174
  spvtools::Extension::kSPV_INTEL_fpga_latency_control, // 175
  spvtools::Extension::kSPV_ALTERA_fpga_argument_interfaces, // 176
  spvtools::Extension::kSPV_INTEL_fpga_argument_interfaces, // 177
  spvtools::Extension::kSPV_INTEL_global_variable_host_access, // 178
  spvtools::Extension::kSPV_ALTERA_global_variable_fpga_decorations, // 179
  spvtools::Extension::kSPV_INTEL_global_variable_fpga_decorations, // 180
  spvtools::Extension::kSPV_INTEL_subgroup_buffer_prefetch, // 181
  spvtools::Extension::kSPV_INTEL_2d_block_io, // 182
  spvtools::Extension::kSPV_INTEL_subgroup_matrix_multiply_accumulate, // 183
  spvtools::Extension::kSPV_INTEL_ternary_bitwise_function, // 184
  spvtools::Extension::kSPV_INTEL_function_variants, // 185
  spvtools::Extension::kSPV_INTEL_predicated_io, // 186
  spvtools::Extension::kSPV_INTEL_rounded_divide_sqrt, // 187
  spvtools::Extension::kSPV_KHR_uniform_group_instructions, // 188
  spvtools::Extension::kSPV_INTEL_tensor_float32_conversion, // 189
  spvtools::Extension::kSPV_INTEL_masked_gather_scatter, // 190
  spvtools::Extension::kSPV_INTEL_cache_controls, // 191
  spvtools::Extension::kSPV_INTEL_maximum_registers, // 192
  spvtools::Extension::kSPV_INTEL_bindless_images, // 193
  spvtools::Extension::kSPV_VALVE_mixed_float_dot_product, // 194
  spvtools::Extension::kSPV_KHR_no_integer_wrap_decoration, // 195
  spvtools::Extension::kSPV_NV_mesh_shader, // 196
  spvtools::Extension::kSPV_EXT_mesh_shader, // 197
  spvtools::Extension::kSPV_GOOGLE_hlsl_functionality1, // 198
  spvtools::Extension::kSPV_GOOGLE_user_type, // 199
  spvtools::Extension::kSPV_KHR_subgroup_uniform_control_flow, // 200
  spvtools::Extension::kSPV_AMD_shader_early_and_late_fragment_tests, // 201
  spvtools::Extension::kSPV_AMD_shader_early_and_late_fragment_tests, // 202
  spvtools::Extension::kSPV_EXT_shader_stencil_export, // 203
  spvtools::Extension::kSPV_KHR_maximal_reconvergence, // 204
  spvtools::Extension::kSPV_KHR_linkonce_odr, // 205
  spvtools::Extension::kSPV_KHR_storage_buffer_storage_class, // 206
  spvtools::Extension::kSPV_KHR_variable_pointers, // 207
  spvtools::Extension::kSPV_KHR_terminate_invocation, // 208
  spvtools::Extension::kSPV_KHR_relaxed_extended_instruction, // 209
  spvtools::Extension::kSPV_KHR_ray_tracing, // 210
  spvtools::Extension::kSPV_KHR_ray_query, // 211
  spvtools::Extension::kSPV_NV_ray_tracing, // 212
  spvtools::Extension::kSPV_KHR_ray_tracing, // 213
  spvtools::Extension::kSPV_KHR_ray_query, // 214
  spvtools::Extension::kSPV_NV_displacement_micromap, // 215
  spvtools::Extension::kSPV_GOOGLE_decorate_string, // 216
  spvtools::Extension::kSPV_GOOGLE_hlsl_functionality1, // 217
};

// Array of operand types, referenced by IndexRanges elsewhere.
// Contains all sequences of operand types used in the grammar.
static const spv_operand_type_t kOperandSpans[] = {
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 0
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 1
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 2
  SPV_OPERAND_TYPE_BUILT_IN, // 3
  SPV_OPERAND_TYPE_SCOPE_ID, // 4
  SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE, // 5
  SPV_OPERAND_TYPE_FP_ROUNDING_MODE, // 6
  SPV_OPERAND_TYPE_FP_FAST_MATH_MODE, // 7
  SPV_OPERAND_TYPE_LITERAL_STRING, // 8
  SPV_OPERAND_TYPE_LINKAGE_TYPE, // 9
  SPV_OPERAND_TYPE_ID, // 10
  SPV_OPERAND_TYPE_LITERAL_STRING, // 11
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 12
  SPV_OPERAND_TYPE_FP_ROUNDING_MODE, // 13
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 14
  SPV_OPERAND_TYPE_FPDENORM_MODE, // 15
  SPV_OPERAND_TYPE_LITERAL_STRING, // 16
  SPV_OPERAND_TYPE_LITERAL_STRING, // 17
  SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER, // 18
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 19
  SPV_OPERAND_TYPE_FPOPERATION_MODE, // 20
  SPV_OPERAND_TYPE_LITERAL_FLOAT, // 21
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 22
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 23
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 24
  SPV_OPERAND_TYPE_ACCESS_QUALIFIER, // 25
  SPV_OPERAND_TYPE_HOST_ACCESS_QUALIFIER, // 26
  SPV_OPERAND_TYPE_LITERAL_STRING, // 27
  SPV_OPERAND_TYPE_INITIALIZATION_MODE_QUALIFIER, // 28
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 29
  SPV_OPERAND_TYPE_LOAD_CACHE_CONTROL, // 30
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 31
  SPV_OPERAND_TYPE_STORE_CACHE_CONTROL, // 32
  SPV_OPERAND_TYPE_ID, // 33
  SPV_OPERAND_TYPE_ID, // 34
  SPV_OPERAND_TYPE_ID, // 35
  SPV_OPERAND_TYPE_ID, // 36
  SPV_OPERAND_TYPE_ID, // 37
  SPV_OPERAND_TYPE_NAMED_MAXIMUM_NUMBER_OF_REGISTERS, // 38
  SPV_OPERAND_TYPE_TYPE_ID, // 39
  SPV_OPERAND_TYPE_RESULT_ID, // 40
  SPV_OPERAND_TYPE_SOURCE_LANGUAGE, // 41
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 42
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 43
  SPV_OPERAND_TYPE_OPTIONAL_LITERAL_STRING, // 44
  SPV_OPERAND_TYPE_ID, // 45
  SPV_OPERAND_TYPE_LITERAL_STRING, // 46
  SPV_OPERAND_TYPE_ID, // 47
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 48
  SPV_OPERAND_TYPE_LITERAL_STRING, // 49
  SPV_OPERAND_TYPE_RESULT_ID, // 50
  SPV_OPERAND_TYPE_LITERAL_STRING, // 51
  SPV_OPERAND_TYPE_ID, // 52
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 53
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 54
  SPV_OPERAND_TYPE_TYPE_ID, // 55
  SPV_OPERAND_TYPE_RESULT_ID, // 56
  SPV_OPERAND_TYPE_ID, // 57
  SPV_OPERAND_TYPE_EXTENSION_INSTRUCTION_NUMBER, // 58
  SPV_OPERAND_TYPE_ADDRESSING_MODEL, // 59
  SPV_OPERAND_TYPE_MEMORY_MODEL, // 60
  SPV_OPERAND_TYPE_EXECUTION_MODEL, // 61
  SPV_OPERAND_TYPE_ID, // 62
  SPV_OPERAND_TYPE_LITERAL_STRING, // 63
  SPV_OPERAND_TYPE_VARIABLE_ID, // 64
  SPV_OPERAND_TYPE_ID, // 65
  SPV_OPERAND_TYPE_EXECUTION_MODE, // 66
  SPV_OPERAND_TYPE_CAPABILITY, // 67
  SPV_OPERAND_TYPE_RESULT_ID, // 68
  SPV_OPERAND_TYPE_RESULT_ID, // 69
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 70
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 71
  SPV_OPERAND_TYPE_RESULT_ID, // 72
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 73
  SPV_OPERAND_TYPE_OPTIONAL_FPENCODING, // 74
  SPV_OPERAND_TYPE_RESULT_ID, // 75
  SPV_OPERAND_TYPE_ID, // 76
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 77
  SPV_OPERAND_TYPE_RESULT_ID, // 78
  SPV_OPERAND_TYPE_ID, // 79
  SPV_OPERAND_TYPE_DIMENSIONALITY, // 80
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 81
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 82
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 83
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 84
  SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT, // 85
  SPV_OPERAND_TYPE_OPTIONAL_ACCESS_QUALIFIER, // 86
  SPV_OPERAND_TYPE_RESULT_ID, // 87
  SPV_OPERAND_TYPE_ID, // 88
  SPV_OPERAND_TYPE_RESULT_ID, // 89
  SPV_OPERAND_TYPE_ID, // 90
  SPV_OPERAND_TYPE_ID, // 91
  SPV_OPERAND_TYPE_RESULT_ID, // 92
  SPV_OPERAND_TYPE_VARIABLE_ID, // 93
  SPV_OPERAND_TYPE_RESULT_ID, // 94
  SPV_OPERAND_TYPE_STORAGE_CLASS, // 95
  SPV_OPERAND_TYPE_ID, // 96
  SPV_OPERAND_TYPE_RESULT_ID, // 97
  SPV_OPERAND_TYPE_ID, // 98
  SPV_OPERAND_TYPE_VARIABLE_ID, // 99
  SPV_OPERAND_TYPE_RESULT_ID, // 100
  SPV_OPERAND_TYPE_ACCESS_QUALIFIER, // 101
  SPV_OPERAND_TYPE_ID, // 102
  SPV_OPERAND_TYPE_STORAGE_CLASS, // 103
  SPV_OPERAND_TYPE_TYPE_ID, // 104
  SPV_OPERAND_TYPE_RESULT_ID, // 105
  SPV_OPERAND_TYPE_TYPED_LITERAL_NUMBER, // 106
  SPV_OPERAND_TYPE_TYPE_ID, // 107
  SPV_OPERAND_TYPE_RESULT_ID, // 108
  SPV_OPERAND_TYPE_VARIABLE_ID, // 109
  SPV_OPERAND_TYPE_TYPE_ID, // 110
  SPV_OPERAND_TYPE_RESULT_ID, // 111
  SPV_OPERAND_TYPE_SAMPLER_ADDRESSING_MODE, // 112
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 113
  SPV_OPERAND_TYPE_SAMPLER_FILTER_MODE, // 114
  SPV_OPERAND_TYPE_TYPE_ID, // 115
  SPV_OPERAND_TYPE_RESULT_ID, // 116
  SPV_OPERAND_TYPE_SPEC_CONSTANT_OP_NUMBER, // 117
  SPV_OPERAND_TYPE_TYPE_ID, // 118
  SPV_OPERAND_TYPE_RESULT_ID, // 119
  SPV_OPERAND_TYPE_FUNCTION_CONTROL, // 120
  SPV_OPERAND_TYPE_ID, // 121
  SPV_OPERAND_TYPE_TYPE_ID, // 122
  SPV_OPERAND_TYPE_RESULT_ID, // 123
  SPV_OPERAND_TYPE_ID, // 124
  SPV_OPERAND_TYPE_VARIABLE_ID, // 125
  SPV_OPERAND_TYPE_TYPE_ID, // 126
  SPV_OPERAND_TYPE_RESULT_ID, // 127
  SPV_OPERAND_TYPE_STORAGE_CLASS, // 128
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 129
  SPV_OPERAND_TYPE_TYPE_ID, // 130
  SPV_OPERAND_TYPE_RESULT_ID, // 131
  SPV_OPERAND_TYPE_ID, // 132
  SPV_OPERAND_TYPE_ID, // 133
  SPV_OPERAND_TYPE_ID, // 134
  SPV_OPERAND_TYPE_TYPE_ID, // 135
  SPV_OPERAND_TYPE_RESULT_ID, // 136
  SPV_OPERAND_TYPE_ID, // 137
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS, // 138
  SPV_OPERAND_TYPE_ID, // 139
  SPV_OPERAND_TYPE_ID, // 140
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS, // 141
  SPV_OPERAND_TYPE_ID, // 142
  SPV_OPERAND_TYPE_ID, // 143
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS, // 144
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS, // 145
  SPV_OPERAND_TYPE_ID, // 146
  SPV_OPERAND_TYPE_ID, // 147
  SPV_OPERAND_TYPE_ID, // 148
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS, // 149
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS, // 150
  SPV_OPERAND_TYPE_TYPE_ID, // 151
  SPV_OPERAND_TYPE_RESULT_ID, // 152
  SPV_OPERAND_TYPE_ID, // 153
  SPV_OPERAND_TYPE_ID, // 154
  SPV_OPERAND_TYPE_VARIABLE_ID, // 155
  SPV_OPERAND_TYPE_TYPE_ID, // 156
  SPV_OPERAND_TYPE_RESULT_ID, // 157
  SPV_OPERAND_TYPE_ID, // 158
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 159
  SPV_OPERAND_TYPE_TYPE_ID, // 160
  SPV_OPERAND_TYPE_RESULT_ID, // 161
  SPV_OPERAND_TYPE_ID, // 162
  SPV_OPERAND_TYPE_ID, // 163
  SPV_OPERAND_TYPE_DECORATION, // 164
  SPV_OPERAND_TYPE_ID, // 165
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 166
  SPV_OPERAND_TYPE_DECORATION, // 167
  SPV_OPERAND_TYPE_ID, // 168
  SPV_OPERAND_TYPE_VARIABLE_ID, // 169
  SPV_OPERAND_TYPE_ID, // 170
  SPV_OPERAND_TYPE_VARIABLE_ID_LITERAL_INTEGER, // 171
  SPV_OPERAND_TYPE_TYPE_ID, // 172
  SPV_OPERAND_TYPE_RESULT_ID, // 173
  SPV_OPERAND_TYPE_ID, // 174
  SPV_OPERAND_TYPE_ID, // 175
  SPV_OPERAND_TYPE_TYPE_ID, // 176
  SPV_OPERAND_TYPE_RESULT_ID, // 177
  SPV_OPERAND_TYPE_ID, // 178
  SPV_OPERAND_TYPE_ID, // 179
  SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER, // 180
  SPV_OPERAND_TYPE_TYPE_ID, // 181
  SPV_OPERAND_TYPE_RESULT_ID, // 182
  SPV_OPERAND_TYPE_ID, // 183
  SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER, // 184
  SPV_OPERAND_TYPE_TYPE_ID, // 185
  SPV_OPERAND_TYPE_RESULT_ID, // 186
  SPV_OPERAND_TYPE_ID, // 187
  SPV_OPERAND_TYPE_ID, // 188
  SPV_OPERAND_TYPE_OPTIONAL_IMAGE, // 189
  SPV_OPERAND_TYPE_TYPE_ID, // 190
  SPV_OPERAND_TYPE_RESULT_ID, // 191
  SPV_OPERAND_TYPE_ID, // 192
  SPV_OPERAND_TYPE_ID, // 193
  SPV_OPERAND_TYPE_IMAGE, // 194
  SPV_OPERAND_TYPE_TYPE_ID, // 195
  SPV_OPERAND_TYPE_RESULT_ID, // 196
  SPV_OPERAND_TYPE_ID, // 197
  SPV_OPERAND_TYPE_ID, // 198
  SPV_OPERAND_TYPE_ID, // 199
  SPV_OPERAND_TYPE_OPTIONAL_IMAGE, // 200
  SPV_OPERAND_TYPE_TYPE_ID, // 201
  SPV_OPERAND_TYPE_RESULT_ID, // 202
  SPV_OPERAND_TYPE_ID, // 203
  SPV_OPERAND_TYPE_ID, // 204
  SPV_OPERAND_TYPE_ID, // 205
  SPV_OPERAND_TYPE_IMAGE, // 206
  SPV_OPERAND_TYPE_ID, // 207
  SPV_OPERAND_TYPE_ID, // 208
  SPV_OPERAND_TYPE_ID, // 209
  SPV_OPERAND_TYPE_OPTIONAL_IMAGE, // 210
  SPV_OPERAND_TYPE_TYPE_ID, // 211
  SPV_OPERAND_TYPE_RESULT_ID, // 212
  SPV_OPERAND_TYPE_ID, // 213
  SPV_OPERAND_TYPE_STORAGE_CLASS, // 214
  SPV_OPERAND_TYPE_TYPE_ID, // 215
  SPV_OPERAND_TYPE_RESULT_ID, // 216
  SPV_OPERAND_TYPE_ID, // 217
  SPV_OPERAND_TYPE_ID, // 218
  SPV_OPERAND_TYPE_ID, // 219
  SPV_OPERAND_TYPE_ID, // 220
  SPV_OPERAND_TYPE_SCOPE_ID, // 221
  SPV_OPERAND_TYPE_SCOPE_ID, // 222
  SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID, // 223
  SPV_OPERAND_TYPE_SCOPE_ID, // 224
  SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID, // 225
  SPV_OPERAND_TYPE_TYPE_ID, // 226
  SPV_OPERAND_TYPE_RESULT_ID, // 227
  SPV_OPERAND_TYPE_ID, // 228
  SPV_OPERAND_TYPE_SCOPE_ID, // 229
  SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID, // 230
  SPV_OPERAND_TYPE_ID, // 231
  SPV_OPERAND_TYPE_SCOPE_ID, // 232
  SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID, // 233
  SPV_OPERAND_TYPE_ID, // 234
  SPV_OPERAND_TYPE_TYPE_ID, // 235
  SPV_OPERAND_TYPE_RESULT_ID, // 236
  SPV_OPERAND_TYPE_ID, // 237
  SPV_OPERAND_TYPE_SCOPE_ID, // 238
  SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID, // 239
  SPV_OPERAND_TYPE_ID, // 240
  SPV_OPERAND_TYPE_TYPE_ID, // 241
  SPV_OPERAND_TYPE_RESULT_ID, // 242
  SPV_OPERAND_TYPE_ID, // 243
  SPV_OPERAND_TYPE_SCOPE_ID, // 244
  SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID, // 245
  SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID, // 246
  SPV_OPERAND_TYPE_ID, // 247
  SPV_OPERAND_TYPE_ID, // 248
  SPV_OPERAND_TYPE_ID, // 249
  SPV_OPERAND_TYPE_ID, // 250
  SPV_OPERAND_TYPE_LOOP_CONTROL, // 251
  SPV_OPERAND_TYPE_ID, // 252
  SPV_OPERAND_TYPE_SELECTION_CONTROL, // 253
  SPV_OPERAND_TYPE_ID, // 254
  SPV_OPERAND_TYPE_ID, // 255
  SPV_OPERAND_TYPE_ID, // 256
  SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER, // 257
  SPV_OPERAND_TYPE_ID, // 258
  SPV_OPERAND_TYPE_ID, // 259
  SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER_ID, // 260
  SPV_OPERAND_TYPE_ID, // 261
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 262
  SPV_OPERAND_TYPE_TYPE_ID, // 263
  SPV_OPERAND_TYPE_RESULT_ID, // 264
  SPV_OPERAND_TYPE_SCOPE_ID, // 265
  SPV_OPERAND_TYPE_ID, // 266
  SPV_OPERAND_TYPE_ID, // 267
  SPV_OPERAND_TYPE_ID, // 268
  SPV_OPERAND_TYPE_ID, // 269
  SPV_OPERAND_TYPE_ID, // 270
  SPV_OPERAND_TYPE_SCOPE_ID, // 271
  SPV_OPERAND_TYPE_ID, // 272
  SPV_OPERAND_TYPE_ID, // 273
  SPV_OPERAND_TYPE_TYPE_ID, // 274
  SPV_OPERAND_TYPE_RESULT_ID, // 275
  SPV_OPERAND_TYPE_SCOPE_ID, // 276
  SPV_OPERAND_TYPE_ID, // 277
  SPV_OPERAND_TYPE_TYPE_ID, // 278
  SPV_OPERAND_TYPE_RESULT_ID, // 279
  SPV_OPERAND_TYPE_SCOPE_ID, // 280
  SPV_OPERAND_TYPE_ID, // 281
  SPV_OPERAND_TYPE_ID, // 282
  SPV_OPERAND_TYPE_TYPE_ID, // 283
  SPV_OPERAND_TYPE_RESULT_ID, // 284
  SPV_OPERAND_TYPE_SCOPE_ID, // 285
  SPV_OPERAND_TYPE_GROUP_OPERATION, // 286
  SPV_OPERAND_TYPE_ID, // 287
  SPV_OPERAND_TYPE_TYPE_ID, // 288
  SPV_OPERAND_TYPE_RESULT_ID, // 289
  SPV_OPERAND_TYPE_ID, // 290
  SPV_OPERAND_TYPE_ID, // 291
  SPV_OPERAND_TYPE_ID, // 292
  SPV_OPERAND_TYPE_ID, // 293
  SPV_OPERAND_TYPE_ID, // 294
  SPV_OPERAND_TYPE_ID, // 295
  SPV_OPERAND_TYPE_ID, // 296
  SPV_OPERAND_TYPE_ID, // 297
  SPV_OPERAND_TYPE_ID, // 298
  SPV_OPERAND_TYPE_ID, // 299
  SPV_OPERAND_TYPE_TYPE_ID, // 300
  SPV_OPERAND_TYPE_RESULT_ID, // 301
  SPV_OPERAND_TYPE_SCOPE_ID, // 302
  SPV_OPERAND_TYPE_ID, // 303
  SPV_OPERAND_TYPE_ID, // 304
  SPV_OPERAND_TYPE_ID, // 305
  SPV_OPERAND_TYPE_ID, // 306
  SPV_OPERAND_TYPE_SCOPE_ID, // 307
  SPV_OPERAND_TYPE_ID, // 308
  SPV_OPERAND_TYPE_ID, // 309
  SPV_OPERAND_TYPE_ID, // 310
  SPV_OPERAND_TYPE_ID, // 311
  SPV_OPERAND_TYPE_TYPE_ID, // 312
  SPV_OPERAND_TYPE_RESULT_ID, // 313
  SPV_OPERAND_TYPE_ID, // 314
  SPV_OPERAND_TYPE_ID, // 315
  SPV_OPERAND_TYPE_ID, // 316
  SPV_OPERAND_TYPE_ID, // 317
  SPV_OPERAND_TYPE_ID, // 318
  SPV_OPERAND_TYPE_ID, // 319
  SPV_OPERAND_TYPE_ID, // 320
  SPV_OPERAND_TYPE_ID, // 321
  SPV_OPERAND_TYPE_ID, // 322
  SPV_OPERAND_TYPE_ID, // 323
  SPV_OPERAND_TYPE_VARIABLE_ID, // 324
  SPV_OPERAND_TYPE_TYPE_ID, // 325
  SPV_OPERAND_TYPE_RESULT_ID, // 326
  SPV_OPERAND_TYPE_ID, // 327
  SPV_OPERAND_TYPE_ID, // 328
  SPV_OPERAND_TYPE_ID, // 329
  SPV_OPERAND_TYPE_ID, // 330
  SPV_OPERAND_TYPE_ID, // 331
  SPV_OPERAND_TYPE_ID, // 332
  SPV_OPERAND_TYPE_SCOPE_ID, // 333
  SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID, // 334
  SPV_OPERAND_TYPE_TYPE_ID, // 335
  SPV_OPERAND_TYPE_RESULT_ID, // 336
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 337
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 338
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 339
  SPV_OPERAND_TYPE_TYPE_ID, // 340
  SPV_OPERAND_TYPE_RESULT_ID, // 341
  SPV_OPERAND_TYPE_SCOPE_ID, // 342
  SPV_OPERAND_TYPE_TYPE_ID, // 343
  SPV_OPERAND_TYPE_RESULT_ID, // 344
  SPV_OPERAND_TYPE_SCOPE_ID, // 345
  SPV_OPERAND_TYPE_GROUP_OPERATION, // 346
  SPV_OPERAND_TYPE_ID, // 347
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 348
  SPV_OPERAND_TYPE_TYPE_ID, // 349
  SPV_OPERAND_TYPE_RESULT_ID, // 350
  SPV_OPERAND_TYPE_ID, // 351
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 352
  SPV_OPERAND_TYPE_TYPE_ID, // 353
  SPV_OPERAND_TYPE_RESULT_ID, // 354
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 355
  SPV_OPERAND_TYPE_RESULT_ID, // 356
  SPV_OPERAND_TYPE_ID, // 357
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 358
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 359
  SPV_OPERAND_TYPE_TYPE_ID, // 360
  SPV_OPERAND_TYPE_RESULT_ID, // 361
  SPV_OPERAND_TYPE_ID, // 362
  SPV_OPERAND_TYPE_ID, // 363
  SPV_OPERAND_TYPE_OPTIONAL_TENSOR_OPERANDS, // 364
  SPV_OPERAND_TYPE_ID, // 365
  SPV_OPERAND_TYPE_ID, // 366
  SPV_OPERAND_TYPE_ID, // 367
  SPV_OPERAND_TYPE_OPTIONAL_TENSOR_OPERANDS, // 368
  SPV_OPERAND_TYPE_TYPE_ID, // 369
  SPV_OPERAND_TYPE_RESULT_ID, // 370
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 371
  SPV_OPERAND_TYPE_ID, // 372
  SPV_OPERAND_TYPE_LITERAL_STRING, // 373
  SPV_OPERAND_TYPE_VARIABLE_ID, // 374
  SPV_OPERAND_TYPE_ID, // 375
  SPV_OPERAND_TYPE_ID, // 376
  SPV_OPERAND_TYPE_VARIABLE_ID, // 377
  SPV_OPERAND_TYPE_RESULT_ID, // 378
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 379
  SPV_OPERAND_TYPE_VARIABLE_ID, // 380
  SPV_OPERAND_TYPE_RESULT_ID, // 381
  SPV_OPERAND_TYPE_STORAGE_CLASS, // 382
  SPV_OPERAND_TYPE_TYPE_ID, // 383
  SPV_OPERAND_TYPE_RESULT_ID, // 384
  SPV_OPERAND_TYPE_STORAGE_CLASS, // 385
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 386
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 387
  SPV_OPERAND_TYPE_TYPE_ID, // 388
  SPV_OPERAND_TYPE_RESULT_ID, // 389
  SPV_OPERAND_TYPE_ID, // 390
  SPV_OPERAND_TYPE_ID, // 391
  SPV_OPERAND_TYPE_ID, // 392
  SPV_OPERAND_TYPE_VARIABLE_ID, // 393
  SPV_OPERAND_TYPE_TYPE_ID, // 394
  SPV_OPERAND_TYPE_RESULT_ID, // 395
  SPV_OPERAND_TYPE_ID, // 396
  SPV_OPERAND_TYPE_ID, // 397
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 398
  SPV_OPERAND_TYPE_ID, // 399
  SPV_OPERAND_TYPE_ID, // 400
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 401
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 402
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 403
  SPV_OPERAND_TYPE_TYPE_ID, // 404
  SPV_OPERAND_TYPE_RESULT_ID, // 405
  SPV_OPERAND_TYPE_SCOPE_ID, // 406
  SPV_OPERAND_TYPE_ID, // 407
  SPV_OPERAND_TYPE_ID, // 408
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 409
  SPV_OPERAND_TYPE_TYPE_ID, // 410
  SPV_OPERAND_TYPE_RESULT_ID, // 411
  SPV_OPERAND_TYPE_ID, // 412
  SPV_OPERAND_TYPE_EXTENSION_INSTRUCTION_NUMBER, // 413
  SPV_OPERAND_TYPE_VARIABLE_ID, // 414
  SPV_OPERAND_TYPE_TYPE_ID, // 415
  SPV_OPERAND_TYPE_RESULT_ID, // 416
  SPV_OPERAND_TYPE_ID, // 417
  SPV_OPERAND_TYPE_ID, // 418
  SPV_OPERAND_TYPE_ID, // 419
  SPV_OPERAND_TYPE_ID, // 420
  SPV_OPERAND_TYPE_ID, // 421
  SPV_OPERAND_TYPE_ID, // 422
  SPV_OPERAND_TYPE_ID, // 423
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS, // 424
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS, // 425
  SPV_OPERAND_TYPE_ID, // 426
  SPV_OPERAND_TYPE_ID, // 427
  SPV_OPERAND_TYPE_ID, // 428
  SPV_OPERAND_TYPE_ID, // 429
  SPV_OPERAND_TYPE_ID, // 430
  SPV_OPERAND_TYPE_ID, // 431
  SPV_OPERAND_TYPE_ID, // 432
  SPV_OPERAND_TYPE_ID, // 433
  SPV_OPERAND_TYPE_ID, // 434
  SPV_OPERAND_TYPE_ID, // 435
  SPV_OPERAND_TYPE_ID, // 436
  SPV_OPERAND_TYPE_TYPE_ID, // 437
  SPV_OPERAND_TYPE_RESULT_ID, // 438
  SPV_OPERAND_TYPE_ID, // 439
  SPV_OPERAND_TYPE_ID, // 440
  SPV_OPERAND_TYPE_OPTIONAL_PACKED_VECTOR_FORMAT, // 441
  SPV_OPERAND_TYPE_TYPE_ID, // 442
  SPV_OPERAND_TYPE_RESULT_ID, // 443
  SPV_OPERAND_TYPE_ID, // 444
  SPV_OPERAND_TYPE_ID, // 445
  SPV_OPERAND_TYPE_ID, // 446
  SPV_OPERAND_TYPE_OPTIONAL_PACKED_VECTOR_FORMAT, // 447
  SPV_OPERAND_TYPE_RESULT_ID, // 448
  SPV_OPERAND_TYPE_ID, // 449
  SPV_OPERAND_TYPE_SCOPE_ID, // 450
  SPV_OPERAND_TYPE_ID, // 451
  SPV_OPERAND_TYPE_ID, // 452
  SPV_OPERAND_TYPE_ID, // 453
  SPV_OPERAND_TYPE_TYPE_ID, // 454
  SPV_OPERAND_TYPE_RESULT_ID, // 455
  SPV_OPERAND_TYPE_ID, // 456
  SPV_OPERAND_TYPE_ID, // 457
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 458
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS, // 459
  SPV_OPERAND_TYPE_ID, // 460
  SPV_OPERAND_TYPE_ID, // 461
  SPV_OPERAND_TYPE_ID, // 462
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 463
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS, // 464
  SPV_OPERAND_TYPE_TYPE_ID, // 465
  SPV_OPERAND_TYPE_RESULT_ID, // 466
  SPV_OPERAND_TYPE_ID, // 467
  SPV_OPERAND_TYPE_ID, // 468
  SPV_OPERAND_TYPE_ID, // 469
  SPV_OPERAND_TYPE_OPTIONAL_COOPERATIVE_MATRIX_OPERANDS, // 470
  SPV_OPERAND_TYPE_ID, // 471
  SPV_OPERAND_TYPE_ID, // 472
  SPV_OPERAND_TYPE_ID, // 473
  SPV_OPERAND_TYPE_ID, // 474
  SPV_OPERAND_TYPE_ID, // 475
  SPV_OPERAND_TYPE_ID, // 476
  SPV_OPERAND_TYPE_ID, // 477
  SPV_OPERAND_TYPE_ID, // 478
  SPV_OPERAND_TYPE_TYPE_ID, // 479
  SPV_OPERAND_TYPE_RESULT_ID, // 480
  SPV_OPERAND_TYPE_ID, // 481
  SPV_OPERAND_TYPE_ID, // 482
  SPV_OPERAND_TYPE_ID, // 483
  SPV_OPERAND_TYPE_ID, // 484
  SPV_OPERAND_TYPE_OPTIONAL_IMAGE, // 485
  SPV_OPERAND_TYPE_TYPE_ID, // 486
  SPV_OPERAND_TYPE_RESULT_ID, // 487
  SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER, // 488
  SPV_OPERAND_TYPE_ID, // 489
  SPV_OPERAND_TYPE_ID, // 490
  SPV_OPERAND_TYPE_ID, // 491
  SPV_OPERAND_TYPE_ID, // 492
  SPV_OPERAND_TYPE_ID, // 493
  SPV_OPERAND_TYPE_ID, // 494
  SPV_OPERAND_TYPE_ID, // 495
  SPV_OPERAND_TYPE_ID, // 496
  SPV_OPERAND_TYPE_ID, // 497
  SPV_OPERAND_TYPE_ID, // 498
  SPV_OPERAND_TYPE_ID, // 499
  SPV_OPERAND_TYPE_ID, // 500
  SPV_OPERAND_TYPE_ID, // 501
  SPV_OPERAND_TYPE_ID, // 502
  SPV_OPERAND_TYPE_ID, // 503
  SPV_OPERAND_TYPE_ID, // 504
  SPV_OPERAND_TYPE_ID, // 505
  SPV_OPERAND_TYPE_ID, // 506
  SPV_OPERAND_TYPE_ID, // 507
  SPV_OPERAND_TYPE_ID, // 508
  SPV_OPERAND_TYPE_ID, // 509
  SPV_OPERAND_TYPE_ID, // 510
  SPV_OPERAND_TYPE_ID, // 511
  SPV_OPERAND_TYPE_ID, // 512
  SPV_OPERAND_TYPE_ID, // 513
  SPV_OPERAND_TYPE_ID, // 514
  SPV_OPERAND_TYPE_ID, // 515
  SPV_OPERAND_TYPE_ID, // 516
  SPV_OPERAND_TYPE_ID, // 517
  SPV_OPERAND_TYPE_ID, // 518
  SPV_OPERAND_TYPE_ID, // 519
  SPV_OPERAND_TYPE_ID, // 520
  SPV_OPERAND_TYPE_ID, // 521
  SPV_OPERAND_TYPE_ID, // 522
  SPV_OPERAND_TYPE_ID, // 523
  SPV_OPERAND_TYPE_ID, // 524
  SPV_OPERAND_TYPE_ID, // 525
  SPV_OPERAND_TYPE_ID, // 526
  SPV_OPERAND_TYPE_ID, // 527
  SPV_OPERAND_TYPE_ID, // 528
  SPV_OPERAND_TYPE_ID, // 529
  SPV_OPERAND_TYPE_ID, // 530
  SPV_OPERAND_TYPE_ID, // 531
  SPV_OPERAND_TYPE_ID, // 532
  SPV_OPERAND_TYPE_ID, // 533
  SPV_OPERAND_TYPE_ID, // 534
  SPV_OPERAND_TYPE_ID, // 535
  SPV_OPERAND_TYPE_ID, // 536
  SPV_OPERAND_TYPE_ID, // 537
  SPV_OPERAND_TYPE_ID, // 538
  SPV_OPERAND_TYPE_ID, // 539
  SPV_OPERAND_TYPE_ID, // 540
  SPV_OPERAND_TYPE_ID, // 541
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 542
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 543
  SPV_OPERAND_TYPE_TYPE_ID, // 544
  SPV_OPERAND_TYPE_RESULT_ID, // 545
  SPV_OPERAND_TYPE_ID, // 546
  SPV_OPERAND_TYPE_ID, // 547
  SPV_OPERAND_TYPE_ID, // 548
  SPV_OPERAND_TYPE_ID, // 549
  SPV_OPERAND_TYPE_ID, // 550
  SPV_OPERAND_TYPE_ID, // 551
  SPV_OPERAND_TYPE_ID, // 552
  SPV_OPERAND_TYPE_ID, // 553
  SPV_OPERAND_TYPE_ID, // 554
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 555
  SPV_OPERAND_TYPE_OPTIONAL_COOPERATIVE_MATRIX_OPERANDS, // 556
  SPV_OPERAND_TYPE_ID, // 557
  SPV_OPERAND_TYPE_ID, // 558
  SPV_OPERAND_TYPE_ID, // 559
  SPV_OPERAND_TYPE_ID, // 560
  SPV_OPERAND_TYPE_ID, // 561
  SPV_OPERAND_TYPE_ID, // 562
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 563
  SPV_OPERAND_TYPE_TYPE_ID, // 564
  SPV_OPERAND_TYPE_RESULT_ID, // 565
  SPV_OPERAND_TYPE_ID, // 566
  SPV_OPERAND_TYPE_ID, // 567
  SPV_OPERAND_TYPE_ID, // 568
  SPV_OPERAND_TYPE_ID, // 569
  SPV_OPERAND_TYPE_ID, // 570
  SPV_OPERAND_TYPE_ID, // 571
  SPV_OPERAND_TYPE_ID, // 572
  SPV_OPERAND_TYPE_ID, // 573
  SPV_OPERAND_TYPE_ID, // 574
  SPV_OPERAND_TYPE_ID, // 575
  SPV_OPERAND_TYPE_ID, // 576
  SPV_OPERAND_TYPE_ID, // 577
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 578
  SPV_OPERAND_TYPE_OPTIONAL_COOPERATIVE_MATRIX_OPERANDS, // 579
  SPV_OPERAND_TYPE_ID, // 580
  SPV_OPERAND_TYPE_ID, // 581
  SPV_OPERAND_TYPE_ID, // 582
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 583
  SPV_OPERAND_TYPE_TYPE_ID, // 584
  SPV_OPERAND_TYPE_RESULT_ID, // 585
  SPV_OPERAND_TYPE_ID, // 586
  SPV_OPERAND_TYPE_ID, // 587
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS, // 588
  SPV_OPERAND_TYPE_ID, // 589
  SPV_OPERAND_TYPE_ID, // 590
  SPV_OPERAND_TYPE_ID, // 591
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS, // 592
  SPV_OPERAND_TYPE_ID, // 593
  SPV_OPERAND_TYPE_ID, // 594
  SPV_OPERAND_TYPE_ID, // 595
  SPV_OPERAND_TYPE_ID, // 596
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 597
  SPV_OPERAND_TYPE_ID, // 598
  SPV_OPERAND_TYPE_ID, // 599
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 600
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 601
  SPV_OPERAND_TYPE_ID, // 602
  SPV_OPERAND_TYPE_ID, // 603
  SPV_OPERAND_TYPE_ID, // 604
  SPV_OPERAND_TYPE_ID, // 605
  SPV_OPERAND_TYPE_ID, // 606
  SPV_OPERAND_TYPE_ID, // 607
  SPV_OPERAND_TYPE_ID, // 608
  SPV_OPERAND_TYPE_ID, // 609
  SPV_OPERAND_TYPE_ID, // 610
  SPV_OPERAND_TYPE_ID, // 611
  SPV_OPERAND_TYPE_ID, // 612
  SPV_OPERAND_TYPE_ID, // 613
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 614
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 615
  SPV_OPERAND_TYPE_ID, // 616
  SPV_OPERAND_TYPE_ID, // 617
  SPV_OPERAND_TYPE_ID, // 618
  SPV_OPERAND_TYPE_ID, // 619
  SPV_OPERAND_TYPE_ID, // 620
  SPV_OPERAND_TYPE_ID, // 621
  SPV_OPERAND_TYPE_ID, // 622
  SPV_OPERAND_TYPE_ID, // 623
  SPV_OPERAND_TYPE_ID, // 624
  SPV_OPERAND_TYPE_ID, // 625
  SPV_OPERAND_TYPE_ID, // 626
  SPV_OPERAND_TYPE_ID, // 627
  SPV_OPERAND_TYPE_ID, // 628
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 629
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 630
  SPV_OPERAND_TYPE_RESULT_ID, // 631
  SPV_OPERAND_TYPE_ID, // 632
  SPV_OPERAND_TYPE_SCOPE_ID, // 633
  SPV_OPERAND_TYPE_ID, // 634
  SPV_OPERAND_TYPE_ID, // 635
  SPV_OPERAND_TYPE_TYPE_ID, // 636
  SPV_OPERAND_TYPE_RESULT_ID, // 637
  SPV_OPERAND_TYPE_ID, // 638
  SPV_OPERAND_TYPE_ID, // 639
  SPV_OPERAND_TYPE_ID, // 640
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS, // 641
  SPV_OPERAND_TYPE_ID, // 642
  SPV_OPERAND_TYPE_ID, // 643
  SPV_OPERAND_TYPE_ID, // 644
  SPV_OPERAND_TYPE_ID, // 645
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS, // 646
  SPV_OPERAND_TYPE_TYPE_ID, // 647
  SPV_OPERAND_TYPE_RESULT_ID, // 648
  SPV_OPERAND_TYPE_ID, // 649
  SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_REDUCE, // 650
  SPV_OPERAND_TYPE_ID, // 651
  SPV_OPERAND_TYPE_TYPE_ID, // 652
  SPV_OPERAND_TYPE_RESULT_ID, // 653
  SPV_OPERAND_TYPE_ID, // 654
  SPV_OPERAND_TYPE_ID, // 655
  SPV_OPERAND_TYPE_ID, // 656
  SPV_OPERAND_TYPE_MEMORY_ACCESS, // 657
  SPV_OPERAND_TYPE_TENSOR_ADDRESSING_OPERANDS, // 658
  SPV_OPERAND_TYPE_ID, // 659
  SPV_OPERAND_TYPE_ID, // 660
  SPV_OPERAND_TYPE_ID, // 661
  SPV_OPERAND_TYPE_MEMORY_ACCESS, // 662
  SPV_OPERAND_TYPE_TENSOR_ADDRESSING_OPERANDS, // 663
  SPV_OPERAND_TYPE_RESULT_ID, // 664
  SPV_OPERAND_TYPE_ID, // 665
  SPV_OPERAND_TYPE_ID, // 666
  SPV_OPERAND_TYPE_VARIABLE_ID, // 667
  SPV_OPERAND_TYPE_TYPE_ID, // 668
  SPV_OPERAND_TYPE_RESULT_ID, // 669
  SPV_OPERAND_TYPE_ID, // 670
  SPV_OPERAND_TYPE_ID, // 671
  SPV_OPERAND_TYPE_ID, // 672
  SPV_OPERAND_TYPE_ID, // 673
  SPV_OPERAND_TYPE_OPTIONAL_RAW_ACCESS_CHAIN_OPERANDS, // 674
  SPV_OPERAND_TYPE_ID, // 675
  SPV_OPERAND_TYPE_ID, // 676
  SPV_OPERAND_TYPE_ID, // 677
  SPV_OPERAND_TYPE_ID, // 678
  SPV_OPERAND_TYPE_ID, // 679
  SPV_OPERAND_TYPE_TYPE_ID, // 680
  SPV_OPERAND_TYPE_RESULT_ID, // 681
  SPV_OPERAND_TYPE_ID, // 682
  SPV_OPERAND_TYPE_ID, // 683
  SPV_OPERAND_TYPE_LITERAL_STRING, // 684
  SPV_OPERAND_TYPE_LITERAL_STRING, // 685
  SPV_OPERAND_TYPE_TYPE_ID, // 686
  SPV_OPERAND_TYPE_RESULT_ID, // 687
  SPV_OPERAND_TYPE_ID, // 688
  SPV_OPERAND_TYPE_ID, // 689
  SPV_OPERAND_TYPE_ID, // 690
  SPV_OPERAND_TYPE_ID, // 691
  SPV_OPERAND_TYPE_ID, // 692
  SPV_OPERAND_TYPE_ID, // 693
  SPV_OPERAND_TYPE_ID, // 694
  SPV_OPERAND_TYPE_TYPE_ID, // 695
  SPV_OPERAND_TYPE_RESULT_ID, // 696
  SPV_OPERAND_TYPE_ID, // 697
  SPV_OPERAND_TYPE_ID, // 698
  SPV_OPERAND_TYPE_ID, // 699
  SPV_OPERAND_TYPE_ID, // 700
  SPV_OPERAND_TYPE_ID, // 701
  SPV_OPERAND_TYPE_ID, // 702
  SPV_OPERAND_TYPE_ID, // 703
  SPV_OPERAND_TYPE_ID, // 704
  SPV_OPERAND_TYPE_TYPE_ID, // 705
  SPV_OPERAND_TYPE_RESULT_ID, // 706
  SPV_OPERAND_TYPE_ID, // 707
  SPV_OPERAND_TYPE_ID, // 708
  SPV_OPERAND_TYPE_ID, // 709
  SPV_OPERAND_TYPE_ID, // 710
  SPV_OPERAND_TYPE_ID, // 711
  SPV_OPERAND_TYPE_ID, // 712
  SPV_OPERAND_TYPE_ID, // 713
  SPV_OPERAND_TYPE_ID, // 714
  SPV_OPERAND_TYPE_ID, // 715
  SPV_OPERAND_TYPE_ID, // 716
  SPV_OPERAND_TYPE_ID, // 717
  SPV_OPERAND_TYPE_TYPE_ID, // 718
  SPV_OPERAND_TYPE_RESULT_ID, // 719
  SPV_OPERAND_TYPE_ID, // 720
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 721
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 722
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 723
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 724
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 725
  SPV_OPERAND_TYPE_TYPE_ID, // 726
  SPV_OPERAND_TYPE_RESULT_ID, // 727
  SPV_OPERAND_TYPE_ID, // 728
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 729
  SPV_OPERAND_TYPE_ID, // 730
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 731
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 732
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 733
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 734
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 735
  SPV_OPERAND_TYPE_TYPE_ID, // 736
  SPV_OPERAND_TYPE_RESULT_ID, // 737
  SPV_OPERAND_TYPE_ID, // 738
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 739
  SPV_OPERAND_TYPE_ID, // 740
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 741
  SPV_OPERAND_TYPE_RESULT_ID, // 742
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 743
  SPV_OPERAND_TYPE_RESULT_ID, // 744
  SPV_OPERAND_TYPE_ID, // 745
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 746
  SPV_OPERAND_TYPE_VARIABLE_ID, // 747
  SPV_OPERAND_TYPE_TYPE_ID, // 748
  SPV_OPERAND_TYPE_RESULT_ID, // 749
  SPV_OPERAND_TYPE_ID, // 750
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 751
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 752
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 753
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 754
  SPV_OPERAND_TYPE_ID, // 755
  SPV_OPERAND_TYPE_ID, // 756
  SPV_OPERAND_TYPE_ID, // 757
  SPV_OPERAND_TYPE_ID, // 758
  SPV_OPERAND_TYPE_ID, // 759
  SPV_OPERAND_TYPE_ID, // 760
  SPV_OPERAND_TYPE_ID, // 761
  SPV_OPERAND_TYPE_ID, // 762
  SPV_OPERAND_TYPE_ID, // 763
  SPV_OPERAND_TYPE_ID, // 764
  SPV_OPERAND_TYPE_ID, // 765
  SPV_OPERAND_TYPE_ID, // 766
  SPV_OPERAND_TYPE_ID, // 767
  SPV_OPERAND_TYPE_ID, // 768
  SPV_OPERAND_TYPE_ID, // 769
  SPV_OPERAND_TYPE_ID, // 770
  SPV_OPERAND_TYPE_ID, // 771
  SPV_OPERAND_TYPE_ID, // 772
  SPV_OPERAND_TYPE_ID, // 773
  SPV_OPERAND_TYPE_TYPE_ID, // 774
  SPV_OPERAND_TYPE_RESULT_ID, // 775
  SPV_OPERAND_TYPE_ID, // 776
  SPV_OPERAND_TYPE_ID, // 777
  SPV_OPERAND_TYPE_ID, // 778
  SPV_OPERAND_TYPE_ID, // 779
  SPV_OPERAND_TYPE_OPTIONAL_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS, // 780
  SPV_OPERAND_TYPE_ID, // 781
  SPV_OPERAND_TYPE_EXECUTION_MODEL, // 782
  SPV_OPERAND_TYPE_ID, // 783
  SPV_OPERAND_TYPE_LITERAL_STRING, // 784
  SPV_OPERAND_TYPE_VARIABLE_ID, // 785
  SPV_OPERAND_TYPE_ID, // 786
  SPV_OPERAND_TYPE_CAPABILITY, // 787
  SPV_OPERAND_TYPE_TYPE_ID, // 788
  SPV_OPERAND_TYPE_RESULT_ID, // 789
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 790
  SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER, // 791
  SPV_OPERAND_TYPE_TYPE_ID, // 792
  SPV_OPERAND_TYPE_RESULT_ID, // 793
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 794
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 795
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 796
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 797
  SPV_OPERAND_TYPE_TYPE_ID, // 798
  SPV_OPERAND_TYPE_RESULT_ID, // 799
  SPV_OPERAND_TYPE_VARIABLE_CAPABILITY, // 800
  SPV_OPERAND_TYPE_TYPE_ID, // 801
  SPV_OPERAND_TYPE_RESULT_ID, // 802
  SPV_OPERAND_TYPE_ID, // 803
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 804
  SPV_OPERAND_TYPE_ID, // 805
  SPV_OPERAND_TYPE_ID, // 806
  SPV_OPERAND_TYPE_ID, // 807
  SPV_OPERAND_TYPE_ID, // 808
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 809
  SPV_OPERAND_TYPE_ID, // 810
  SPV_OPERAND_TYPE_ID, // 811
  SPV_OPERAND_TYPE_ID, // 812
  SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING, // 813
  SPV_OPERAND_TYPE_ID, // 814
  SPV_OPERAND_TYPE_STORAGE_CLASS, // 815
  SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS, // 816
  SPV_OPERAND_TYPE_ID, // 817
  SPV_OPERAND_TYPE_DEBUG_TYPE_QUALIFIER, // 818
  SPV_OPERAND_TYPE_ID, // 819
  SPV_OPERAND_TYPE_ID, // 820
  SPV_OPERAND_TYPE_ID, // 821
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 822
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 823
  SPV_OPERAND_TYPE_ID, // 824
  SPV_OPERAND_TYPE_ID, // 825
  SPV_OPERAND_TYPE_ID, // 826
  SPV_OPERAND_TYPE_ID, // 827
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 828
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 829
  SPV_OPERAND_TYPE_ID, // 830
  SPV_OPERAND_TYPE_ID, // 831
  SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS, // 832
  SPV_OPERAND_TYPE_VARIABLE_ID, // 833
  SPV_OPERAND_TYPE_ID, // 834
  SPV_OPERAND_TYPE_DEBUG_COMPOSITE_TYPE, // 835
  SPV_OPERAND_TYPE_ID, // 836
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 837
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 838
  SPV_OPERAND_TYPE_ID, // 839
  SPV_OPERAND_TYPE_ID, // 840
  SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS, // 841
  SPV_OPERAND_TYPE_VARIABLE_ID, // 842
  SPV_OPERAND_TYPE_ID, // 843
  SPV_OPERAND_TYPE_ID, // 844
  SPV_OPERAND_TYPE_ID, // 845
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 846
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 847
  SPV_OPERAND_TYPE_ID, // 848
  SPV_OPERAND_TYPE_ID, // 849
  SPV_OPERAND_TYPE_ID, // 850
  SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS, // 851
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 852
  SPV_OPERAND_TYPE_ID, // 853
  SPV_OPERAND_TYPE_ID, // 854
  SPV_OPERAND_TYPE_ID, // 855
  SPV_OPERAND_TYPE_ID, // 856
  SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS, // 857
  SPV_OPERAND_TYPE_ID, // 858
  SPV_OPERAND_TYPE_ID, // 859
  SPV_OPERAND_TYPE_ID, // 860
  SPV_OPERAND_TYPE_ID, // 861
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 862
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 863
  SPV_OPERAND_TYPE_ID, // 864
  SPV_OPERAND_TYPE_ID, // 865
  SPV_OPERAND_TYPE_ID, // 866
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 867
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 868
  SPV_OPERAND_TYPE_ID, // 869
  SPV_OPERAND_TYPE_ID, // 870
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 871
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 872
  SPV_OPERAND_TYPE_VARIABLE_ID, // 873
  SPV_OPERAND_TYPE_ID, // 874
  SPV_OPERAND_TYPE_ID, // 875
  SPV_OPERAND_TYPE_ID, // 876
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 877
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 878
  SPV_OPERAND_TYPE_ID, // 879
  SPV_OPERAND_TYPE_ID, // 880
  SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS, // 881
  SPV_OPERAND_TYPE_ID, // 882
  SPV_OPERAND_TYPE_ID, // 883
  SPV_OPERAND_TYPE_ID, // 884
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 885
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 886
  SPV_OPERAND_TYPE_ID, // 887
  SPV_OPERAND_TYPE_ID, // 888
  SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS, // 889
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 890
  SPV_OPERAND_TYPE_ID, // 891
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 892
  SPV_OPERAND_TYPE_ID, // 893
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 894
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 895
  SPV_OPERAND_TYPE_ID, // 896
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 897
  SPV_OPERAND_TYPE_ID, // 898
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 899
  SPV_OPERAND_TYPE_ID, // 900
  SPV_OPERAND_TYPE_ID, // 901
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 902
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 903
  SPV_OPERAND_TYPE_ID, // 904
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 905
  SPV_OPERAND_TYPE_ID, // 906
  SPV_OPERAND_TYPE_ID, // 907
  SPV_OPERAND_TYPE_ID, // 908
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 909
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 910
  SPV_OPERAND_TYPE_ID, // 911
  SPV_OPERAND_TYPE_OPTIONAL_LITERAL_INTEGER, // 912
  SPV_OPERAND_TYPE_DEBUG_OPERATION, // 913
  SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER, // 914
  SPV_OPERAND_TYPE_ID, // 915
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 916
  SPV_OPERAND_TYPE_ID, // 917
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 918
  SPV_OPERAND_TYPE_ID, // 919
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 920
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 921
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 922
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 923
  SPV_OPERAND_TYPE_ID, // 924
  SPV_OPERAND_TYPE_ID, // 925
  SPV_OPERAND_TYPE_ID, // 926
  SPV_OPERAND_TYPE_ID, // 927
  SPV_OPERAND_TYPE_ID, // 928
  SPV_OPERAND_TYPE_ID, // 929
  SPV_OPERAND_TYPE_ID, // 930
  SPV_OPERAND_TYPE_ID, // 931
  SPV_OPERAND_TYPE_VARIABLE_ID, // 932
  SPV_OPERAND_TYPE_ID, // 933
  SPV_OPERAND_TYPE_ID, // 934
  SPV_OPERAND_TYPE_ID, // 935
  SPV_OPERAND_TYPE_ID, // 936
  SPV_OPERAND_TYPE_ID, // 937
  SPV_OPERAND_TYPE_ID, // 938
  SPV_OPERAND_TYPE_ID, // 939
  SPV_OPERAND_TYPE_ID, // 940
  SPV_OPERAND_TYPE_ID, // 941
  SPV_OPERAND_TYPE_VARIABLE_ID, // 942
  SPV_OPERAND_TYPE_ID, // 943
  SPV_OPERAND_TYPE_ID, // 944
  SPV_OPERAND_TYPE_ID, // 945
  SPV_OPERAND_TYPE_ID, // 946
  SPV_OPERAND_TYPE_ID, // 947
  SPV_OPERAND_TYPE_ID, // 948
  SPV_OPERAND_TYPE_ID, // 949
  SPV_OPERAND_TYPE_ID, // 950
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 951
  SPV_OPERAND_TYPE_ID, // 952
  SPV_OPERAND_TYPE_ID, // 953
  SPV_OPERAND_TYPE_ID, // 954
  SPV_OPERAND_TYPE_ID, // 955
  SPV_OPERAND_TYPE_VARIABLE_ID, // 956
  SPV_OPERAND_TYPE_ID, // 957
  SPV_OPERAND_TYPE_ID, // 958
  SPV_OPERAND_TYPE_ID, // 959
  SPV_OPERAND_TYPE_ID, // 960
  SPV_OPERAND_TYPE_ID, // 961
  SPV_OPERAND_TYPE_ID, // 962
  SPV_OPERAND_TYPE_ID, // 963
  SPV_OPERAND_TYPE_ID, // 964
  SPV_OPERAND_TYPE_ID, // 965
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 966
  SPV_OPERAND_TYPE_ID, // 967
  SPV_OPERAND_TYPE_ID, // 968
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 969
  SPV_OPERAND_TYPE_ID, // 970
  SPV_OPERAND_TYPE_ID, // 971
  SPV_OPERAND_TYPE_ID, // 972
  SPV_OPERAND_TYPE_ID, // 973
  SPV_OPERAND_TYPE_ID, // 974
  SPV_OPERAND_TYPE_ID, // 975
  SPV_OPERAND_TYPE_ID, // 976
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 977
  SPV_OPERAND_TYPE_ID, // 978
  SPV_OPERAND_TYPE_ID, // 979
  SPV_OPERAND_TYPE_ID, // 980
  SPV_OPERAND_TYPE_VARIABLE_ID, // 981
  SPV_OPERAND_TYPE_ID, // 982
  SPV_OPERAND_TYPE_ID, // 983
  SPV_OPERAND_TYPE_ID, // 984
  SPV_OPERAND_TYPE_ID, // 985
  SPV_OPERAND_TYPE_ID, // 986
  SPV_OPERAND_TYPE_ID, // 987
  SPV_OPERAND_TYPE_ID, // 988
  SPV_OPERAND_TYPE_ID, // 989
  SPV_OPERAND_TYPE_ID, // 990
  SPV_OPERAND_TYPE_ID, // 991
  SPV_OPERAND_TYPE_ID, // 992
  SPV_OPERAND_TYPE_ID, // 993
  SPV_OPERAND_TYPE_ID, // 994
  SPV_OPERAND_TYPE_ID, // 995
  SPV_OPERAND_TYPE_ID, // 996
  SPV_OPERAND_TYPE_ID, // 997
  SPV_OPERAND_TYPE_ID, // 998
  SPV_OPERAND_TYPE_ID, // 999
  SPV_OPERAND_TYPE_ID, // 1000
  SPV_OPERAND_TYPE_ID, // 1001
  SPV_OPERAND_TYPE_ID, // 1002
  SPV_OPERAND_TYPE_ID, // 1003
  SPV_OPERAND_TYPE_ID, // 1004
  SPV_OPERAND_TYPE_ID, // 1005
  SPV_OPERAND_TYPE_ID, // 1006
  SPV_OPERAND_TYPE_ID, // 1007
  SPV_OPERAND_TYPE_ID, // 1008
  SPV_OPERAND_TYPE_ID, // 1009
  SPV_OPERAND_TYPE_ID, // 1010
  SPV_OPERAND_TYPE_ID, // 1011
  SPV_OPERAND_TYPE_ID, // 1012
  SPV_OPERAND_TYPE_ID, // 1013
  SPV_OPERAND_TYPE_ID, // 1014
  SPV_OPERAND_TYPE_ID, // 1015
  SPV_OPERAND_TYPE_ID, // 1016
  SPV_OPERAND_TYPE_ID, // 1017
  SPV_OPERAND_TYPE_ID, // 1018
  SPV_OPERAND_TYPE_ID, // 1019
  SPV_OPERAND_TYPE_ID, // 1020
  SPV_OPERAND_TYPE_ID, // 1021
  SPV_OPERAND_TYPE_ID, // 1022
  SPV_OPERAND_TYPE_ID, // 1023
  SPV_OPERAND_TYPE_ID, // 1024
  SPV_OPERAND_TYPE_ID, // 1025
  SPV_OPERAND_TYPE_ID, // 1026
  SPV_OPERAND_TYPE_ID, // 1027
  SPV_OPERAND_TYPE_ID, // 1028
  SPV_OPERAND_TYPE_ID, // 1029
  SPV_OPERAND_TYPE_ID, // 1030
  SPV_OPERAND_TYPE_ID, // 1031
  SPV_OPERAND_TYPE_ID, // 1032
  SPV_OPERAND_TYPE_ID, // 1033
  SPV_OPERAND_TYPE_ID, // 1034
  SPV_OPERAND_TYPE_ID, // 1035
  SPV_OPERAND_TYPE_ID, // 1036
  SPV_OPERAND_TYPE_ID, // 1037
  SPV_OPERAND_TYPE_ID, // 1038
  SPV_OPERAND_TYPE_ID, // 1039
  SPV_OPERAND_TYPE_ID, // 1040
  SPV_OPERAND_TYPE_ID, // 1041
  SPV_OPERAND_TYPE_ID, // 1042
  SPV_OPERAND_TYPE_ID, // 1043
  SPV_OPERAND_TYPE_ID, // 1044
  SPV_OPERAND_TYPE_ID, // 1045
  SPV_OPERAND_TYPE_ID, // 1046
  SPV_OPERAND_TYPE_ID, // 1047
  SPV_OPERAND_TYPE_ID, // 1048
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 1049
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 1050
  SPV_OPERAND_TYPE_ID, // 1051
  SPV_OPERAND_TYPE_SOURCE_LANGUAGE, // 1052
  SPV_OPERAND_TYPE_ID, // 1053
  SPV_OPERAND_TYPE_ID, // 1054
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING, // 1055
  SPV_OPERAND_TYPE_ID, // 1056
  SPV_OPERAND_TYPE_STORAGE_CLASS, // 1057
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS, // 1058
  SPV_OPERAND_TYPE_ID, // 1059
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_TYPE_QUALIFIER, // 1060
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS, // 1061
  SPV_OPERAND_TYPE_ID, // 1062
  SPV_OPERAND_TYPE_VARIABLE_ID, // 1063
  SPV_OPERAND_TYPE_ID, // 1064
  SPV_OPERAND_TYPE_ID, // 1065
  SPV_OPERAND_TYPE_ID, // 1066
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 1067
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 1068
  SPV_OPERAND_TYPE_ID, // 1069
  SPV_OPERAND_TYPE_ID, // 1070
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS, // 1071
  SPV_OPERAND_TYPE_VARIABLE_ID, // 1072
  SPV_OPERAND_TYPE_ID, // 1073
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_COMPOSITE_TYPE, // 1074
  SPV_OPERAND_TYPE_ID, // 1075
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 1076
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 1077
  SPV_OPERAND_TYPE_ID, // 1078
  SPV_OPERAND_TYPE_ID, // 1079
  SPV_OPERAND_TYPE_ID, // 1080
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS, // 1081
  SPV_OPERAND_TYPE_VARIABLE_ID, // 1082
  SPV_OPERAND_TYPE_ID, // 1083
  SPV_OPERAND_TYPE_ID, // 1084
  SPV_OPERAND_TYPE_ID, // 1085
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 1086
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 1087
  SPV_OPERAND_TYPE_ID, // 1088
  SPV_OPERAND_TYPE_ID, // 1089
  SPV_OPERAND_TYPE_ID, // 1090
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS, // 1091
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 1092
  SPV_OPERAND_TYPE_ID, // 1093
  SPV_OPERAND_TYPE_ID, // 1094
  SPV_OPERAND_TYPE_ID, // 1095
  SPV_OPERAND_TYPE_ID, // 1096
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS, // 1097
  SPV_OPERAND_TYPE_ID, // 1098
  SPV_OPERAND_TYPE_ID, // 1099
  SPV_OPERAND_TYPE_ID, // 1100
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 1101
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 1102
  SPV_OPERAND_TYPE_ID, // 1103
  SPV_OPERAND_TYPE_ID, // 1104
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS, // 1105
  SPV_OPERAND_TYPE_ID, // 1106
  SPV_OPERAND_TYPE_ID, // 1107
  SPV_OPERAND_TYPE_ID, // 1108
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 1109
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 1110
  SPV_OPERAND_TYPE_ID, // 1111
  SPV_OPERAND_TYPE_ID, // 1112
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS, // 1113
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 1114
  SPV_OPERAND_TYPE_ID, // 1115
  SPV_OPERAND_TYPE_OPTIONAL_ID, // 1116
  SPV_OPERAND_TYPE_ID, // 1117
  SPV_OPERAND_TYPE_ID, // 1118
  SPV_OPERAND_TYPE_ID, // 1119
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 1120
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 1121
  SPV_OPERAND_TYPE_ID, // 1122
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS, // 1123
  SPV_OPERAND_TYPE_OPTIONAL_LITERAL_INTEGER, // 1124
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION, // 1125
  SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER, // 1126
  SPV_OPERAND_TYPE_ID, // 1127
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_IMPORTED_ENTITY, // 1128
  SPV_OPERAND_TYPE_ID, // 1129
  SPV_OPERAND_TYPE_ID, // 1130
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 1131
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 1132
  SPV_OPERAND_TYPE_ID, // 1133
  SPV_OPERAND_TYPE_ID, // 1134
  SPV_OPERAND_TYPE_ID, // 1135
  SPV_OPERAND_TYPE_ID, // 1136
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 1137
  SPV_OPERAND_TYPE_ID, // 1138
  SPV_OPERAND_TYPE_ID, // 1139
  SPV_OPERAND_TYPE_ID, // 1140
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 1141
  SPV_OPERAND_TYPE_ID, // 1142
  SPV_OPERAND_TYPE_ID, // 1143
  SPV_OPERAND_TYPE_LITERAL_INTEGER, // 1144
  SPV_OPERAND_TYPE_ID, // 1145
  SPV_OPERAND_TYPE_ID, // 1146
  SPV_OPERAND_TYPE_ID, // 1147
  SPV_OPERAND_TYPE_FP_ROUNDING_MODE, // 1148
};
