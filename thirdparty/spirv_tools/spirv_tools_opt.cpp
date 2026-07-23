/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wcomma"
#pragma clang diagnostic ignored "-Wextra-semi"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#pragma clang diagnostic ignored "-Wfloat-conversion"
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wreserved-identifier"
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wunused-member-function"
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#pragma clang diagnostic ignored "-Wnewline-eof"
#pragma clang diagnostic ignored "-Wcast-qual"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#pragma GCC diagnostic ignored "-Wnonnull-compare"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#pragma GCC diagnostic ignored "-Wrestrict"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4018)
#pragma warning(disable : 4100)
#pragma warning(disable : 4146)
#pragma warning(disable : 4189)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#pragma warning(disable : 4305)
#pragma warning(disable : 4389)
#pragma warning(disable : 4456)
#pragma warning(disable : 4457)
#pragma warning(disable : 4702)
#pragma warning(disable : 4800)
#pragma warning(disable : 4996)
#endif

#include "spirv_tools.h"

#include "upstream/source/opt/aggressive_dead_code_elim_pass.cpp"
#include "upstream/source/opt/amd_ext_to_khr.cpp"
#include "upstream/source/opt/analyze_live_input_pass.cpp"
#define kLoopMergeContinueBlockIdInIdx kLoopMergeContinueBlockIdInIdx_basicBlock
#define kSelectionMergeMergeBlockIdInIdx kSelectionMergeMergeBlockIdInIdx_basicBlock
#include "upstream/source/opt/basic_block.cpp"
#undef kLoopMergeContinueBlockIdInIdx
#undef kSelectionMergeMergeBlockIdInIdx
#include "upstream/source/opt/block_merge_pass.cpp"
#include "upstream/source/opt/block_merge_util.cpp"
#include "upstream/source/opt/build_module.cpp"
#include "upstream/source/opt/canonicalize_ids_pass.cpp"
#include "upstream/source/opt/ccp_pass.cpp"
#include "upstream/source/opt/cfg.cpp"
#include "upstream/source/opt/cfg_cleanup_pass.cpp"
#include "upstream/source/opt/code_sink.cpp"
#include "upstream/source/opt/combine_access_chains.cpp"
#include "upstream/source/opt/compact_ids_pass.cpp"
#include "upstream/source/opt/composite.cpp"
#include "upstream/source/opt/const_folding_rules.cpp"
#include "upstream/source/opt/constants.cpp"
#include "upstream/source/opt/control_dependence.cpp"
#include "upstream/source/opt/convert_to_half_pass.cpp"
#include "upstream/source/opt/convert_to_sampled_image_pass.cpp"
#define kTypePointerStorageClassInIdx kTypePointerStorageClassInIdx_copyPropArrays
#define kExtInstSetInIdx kExtInstSetInIdx_copyPropArrays
#define kExtInstOpInIdx kExtInstOpInIdx_copyPropArrays
#define kInterpolantInIdx kInterpolantInIdx_copyPropArrays
#include "upstream/source/opt/copy_prop_arrays.cpp"
#undef kTypePointerStorageClassInIdx
#undef kExtInstSetInIdx
#undef kExtInstOpInIdx
#undef kInterpolantInIdx
#include "upstream/source/opt/dataflow.cpp"
#include "upstream/source/opt/dead_branch_elim_pass.cpp"
#include "upstream/source/opt/dead_insert_elim_pass.cpp"
#include "upstream/source/opt/dead_variable_elimination.cpp"
#include "upstream/source/opt/debug_info_manager.cpp"
#include "upstream/source/opt/decoration_manager.cpp"
#include "upstream/source/opt/def_use_manager.cpp"
#include "upstream/source/opt/desc_sroa.cpp"
#include "upstream/source/opt/desc_sroa_util.cpp"
#include "upstream/source/opt/dominator_analysis.cpp"
#include "upstream/source/opt/dominator_tree.cpp"
#include "upstream/source/opt/eliminate_dead_constant_pass.cpp"
#include "upstream/source/opt/eliminate_dead_functions_pass.cpp"
#include "upstream/source/opt/eliminate_dead_functions_util.cpp"
#define kConstantValueInIdx kConstantValueInIdx_eliminateDeadIoComponents
#include "upstream/source/opt/eliminate_dead_io_components_pass.cpp"
#undef kConstantValueInIdx
#include "upstream/source/opt/eliminate_dead_members_pass.cpp"
#include "upstream/source/opt/eliminate_dead_output_stores_pass.cpp"
#include "upstream/source/opt/feature_manager.cpp"
#include "upstream/source/opt/fix_func_call_arguments.cpp"
#include "upstream/source/opt/fix_storage_class.cpp"
#include "upstream/source/opt/flatten_decoration_pass.cpp"
#include "upstream/source/opt/fold.cpp"
#include "upstream/source/opt/fold_spec_constant_op_and_composite_pass.cpp"
#define kExtractCompositeIdInIdx kExtractCompositeIdInIdx_foldingRules
#define kInsertObjectIdInIdx kInsertObjectIdInIdx_foldingRules
#define kInsertCompositeIdInIdx kInsertCompositeIdInIdx_foldingRules
#define HasFloatingPoint HasFloatingPoint_foldingRules
#include "upstream/source/opt/folding_rules.cpp"
#undef kExtractCompositeIdInIdx
#undef kInsertObjectIdInIdx
#undef kInsertCompositeIdInIdx
#undef HasFloatingPoint
#include "upstream/source/opt/freeze_spec_constant_value_pass.cpp"
#include "upstream/source/opt/function.cpp"
#include "upstream/source/opt/graph.cpp"
#include "upstream/source/opt/graphics_robust_access_pass.cpp"
#include "upstream/source/opt/if_conversion.cpp"
#include "upstream/source/opt/inline_exhaustive_pass.cpp"
#include "upstream/source/opt/inline_opaque_pass.cpp"
#include "upstream/source/opt/inline_pass.cpp"
#define kExtInstSetIdInIdx kExtInstSetIdInIdx_instruction
#define kExtInstInstructionInIdx kExtInstInstructionInIdx_instruction
#include "upstream/source/opt/instruction.cpp"
#undef kExtInstSetIdInIdx
#undef kExtInstInstructionInIdx
#include "upstream/source/opt/instruction_list.cpp"
#include "upstream/source/opt/interface_var_sroa.cpp"
#include "upstream/source/opt/interp_fixup_pass.cpp"
#define kEntryPointFunctionIdInIdx kEntryPointFunctionIdInIdx_invocationInterlock
#include "upstream/source/opt/invocation_interlock_placement_pass.cpp"
#undef kEntryPointFunctionIdInIdx
#define kEntryPointFunctionIdInIdx kEntryPointFunctionIdInIdx_irContext
#define kEntryPointExecutionModelInIdx kEntryPointExecutionModelInIdx_irContext
#include "upstream/source/opt/ir_context.cpp"
#undef kEntryPointFunctionIdInIdx
#undef kEntryPointExecutionModelInIdx
#include "upstream/source/opt/ir_loader.cpp"
#include "upstream/source/opt/legalize_multidim_array_pass.cpp"
#include "upstream/source/opt/licm_pass.cpp"
#include "upstream/source/opt/liveness.cpp"
#include "upstream/source/opt/local_access_chain_convert_pass.cpp"
#include "upstream/source/opt/local_redundancy_elimination.cpp"
#define kStoreValIdInIdx kStoreValIdInIdx_localSingleBlockElim
#include "upstream/source/opt/local_single_block_elim_pass.cpp"
#undef kStoreValIdInIdx
#define kStoreValIdInIdx kStoreValIdInIdx_localSingleStoreElim
#include "upstream/source/opt/local_single_store_elim_pass.cpp"
#undef kStoreValIdInIdx
#include "upstream/source/opt/loop_dependence.cpp"
#include "upstream/source/opt/loop_dependence_helpers.cpp"
#include "upstream/source/opt/loop_descriptor.cpp"
#include "upstream/source/opt/loop_fission.cpp"
#include "upstream/source/opt/loop_fusion.cpp"
#include "upstream/source/opt/loop_fusion_pass.cpp"
#include "upstream/source/opt/loop_peeling.cpp"
#include "upstream/source/opt/loop_unroller.cpp"
#define kTypePointerStorageClassInIdx kTypePointerStorageClassInIdx_loopUnswitch
#include "upstream/source/opt/loop_unswitch_pass.cpp"
#undef kTypePointerStorageClassInIdx
#include "upstream/source/opt/loop_utils.cpp"
#define kTypePointerStorageClassInIdx kTypePointerStorageClassInIdx_memPass
#define kTypePointerTypeIdInIdx kTypePointerTypeIdInIdx_memPass
#include "upstream/source/opt/mem_pass.cpp"
#undef kTypePointerStorageClassInIdx
#undef kTypePointerTypeIdInIdx
#include "upstream/source/opt/merge_return_pass.cpp"
#include "upstream/source/opt/modify_maximal_reconvergence.cpp"
#include "upstream/source/opt/module.cpp"
#include "upstream/source/opt/opextinst_forward_ref_fixup_pass.cpp"
#include "upstream/source/opt/optimizer.cpp"
#define kTypePointerTypeIdInIdx kTypePointerTypeIdInIdx_pass
#include "upstream/source/opt/pass.cpp"
#undef kTypePointerTypeIdInIdx
#include "upstream/source/opt/pass_manager.cpp"
#include "upstream/source/opt/private_to_local_pass.cpp"
#include "upstream/source/opt/propagator.cpp"
#define kExtractCompositeIdInIdx kExtractCompositeIdInIdx_reduceLoadSize
#define kVariableStorageClassInIdx kVariableStorageClassInIdx_reduceLoadSize
#include "upstream/source/opt/reduce_load_size.cpp"
#undef kExtractCompositeIdInIdx
#undef kVariableStorageClassInIdx
#include "upstream/source/opt/redundancy_elimination.cpp"
#include "upstream/source/opt/register_pressure.cpp"
#include "upstream/source/opt/relax_float_ops_pass.cpp"
#include "upstream/source/opt/remove_dontinline_pass.cpp"
#include "upstream/source/opt/remove_duplicates_pass.cpp"
#include "upstream/source/opt/remove_unused_interface_variables_pass.cpp"
#define kOpAccessChainInOperandIndexes kOpAccessChainInOperandIndexes_replaceDescArray
#include "upstream/source/opt/replace_desc_array_access_using_var_index.cpp"
#undef kOpAccessChainInOperandIndexes
#include "upstream/source/opt/replace_invalid_opc.cpp"
#include "upstream/source/opt/resolve_binding_conflicts_pass.cpp"
#include "upstream/source/opt/scalar_analysis.cpp"
#include "upstream/source/opt/scalar_analysis_simplification.cpp"
#define kDebugDeclareOperandVariableIndex kDebugDeclareOperandVariableIndex_scalarReplacement
#include "upstream/source/opt/scalar_replacement_pass.cpp"
#undef kDebugDeclareOperandVariableIndex
#define IsSeparator IsSeparator_setSpecConstantDefaultValue
#include "upstream/source/opt/set_spec_constant_default_value_pass.cpp"
#undef IsSeparator
#include "upstream/source/opt/simplification_pass.cpp"
#include "upstream/source/opt/split_combined_image_sampler_pass.cpp"
#define kOpEntryPointInOperandInterface kOpEntryPointInOperandInterface_spreadVolatile
#include "upstream/source/opt/spread_volatile_semantics.cpp"
#undef kOpEntryPointInOperandInterface
#define kStoreValIdInIdx kStoreValIdInIdx_ssaRewrite
#define kVariableInitIdInIdx kVariableInitIdInIdx_ssaRewrite
#include "upstream/source/opt/ssa_rewrite_pass.cpp"
#undef kStoreValIdInIdx
#undef kVariableInitIdInIdx
#include "upstream/source/opt/strength_reduction_pass.cpp"
#include "upstream/source/opt/strip_debug_info_pass.cpp"
#include "upstream/source/opt/strip_nonsemantic_info_pass.cpp"
#include "upstream/source/opt/struct_cfg_analysis.cpp"
#include "upstream/source/opt/struct_packing_pass.cpp"
#include "upstream/source/opt/switch_descriptorset_pass.cpp"
#include "upstream/source/opt/trim_capabilities_pass.cpp"
#include "upstream/source/opt/type_manager.cpp"
#include "upstream/source/opt/types.cpp"
#include "upstream/source/opt/unify_const_pass.cpp"
#include "upstream/source/opt/upgrade_memory_model.cpp"
#include "upstream/source/opt/value_number_table.cpp"
#define kExtractCompositeIdInIdx kExtractCompositeIdInIdx_vectorDce
#define kInsertObjectIdInIdx kInsertObjectIdInIdx_vectorDce
#define kInsertCompositeIdInIdx kInsertCompositeIdInIdx_vectorDce
#include "upstream/source/opt/vector_dce.cpp"
#undef kExtractCompositeIdInIdx
#undef kInsertObjectIdInIdx
#undef kInsertCompositeIdInIdx
#include "upstream/source/opt/workaround1209.cpp"
#include "upstream/source/opt/wrap_opkill.cpp"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
