/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include <mutex>  // NOLINT

#include "absl/strings/str_split.h"
#include "tensorflow/compiler/jit/flags.h"
#include "tensorflow/compiler/xla/parse_flags_from_env.h"
#include "tensorflow/core/util/command_line_flags.h"

namespace tensorflow {
namespace {

BuildXlaOpsPassFlags* build_ops_flags;
MarkForCompilationPassFlags* mark_for_compilation_flags;
XlaDeviceFlags* device_flags;
XlaOpsCommonFlags* ops_flags;
IntroduceFloatingPointJitterPassFlags* jitter_flags;

std::vector<Flag>* flag_list;
std::once_flag flags_init;

void AppendMarkForCompilationPassFlagsInternal(std::vector<Flag>* flag_list) {
  std::vector<Flag> new_flags = {
      Flag("tf_xla_auto_jit", &mark_for_compilation_flags->tf_xla_auto_jit,
           "Control compilation of operators into XLA computations on CPU and "
           "GPU devices.  0 = use ConfigProto setting; -1 = off; 1 = on for "
           "things very likely to be improved; 2 = on for everything.  "
           "Experimental."),
      Flag("tf_xla_min_cluster_size",
           &mark_for_compilation_flags->tf_xla_min_cluster_size,
           "Minimum number of operators in an XLA compilation. Ignored for "
           "operators placed on an XLA device or operators explicitly marked "
           "for compilation."),
      Flag("tf_xla_max_cluster_size",
           &mark_for_compilation_flags->tf_xla_max_cluster_size,
           "Maximum number of operators in an XLA compilation."),
      Flag("tf_xla_clustering_debug",
           &mark_for_compilation_flags->tf_xla_clustering_debug,
           "Dump graphs during XLA compilation."),
      Flag("tf_xla_cpu_global_jit",
           &mark_for_compilation_flags->tf_xla_cpu_global_jit,
           "Enables global JIT compilation for CPU via SessionOptions."),
      Flag("tf_xla_clustering_fuel",
           &mark_for_compilation_flags->tf_xla_clustering_fuel,
           "Places an artificial limit on the number of ops marked as "
           "eligible for clustering."),
      Flag("tf_xla_disable_deadness_safety_checks_for_debugging",
           &mark_for_compilation_flags
                ->tf_xla_disable_deadness_safety_checks_for_debugging,
           "Disable deadness related safety checks when clustering (this is "
           "unsound).")};
  flag_list->insert(flag_list->end(), new_flags.begin(), new_flags.end());
}

void AllocateAndParseFlags() {
  build_ops_flags = new BuildXlaOpsPassFlags;
  build_ops_flags->tf_xla_enable_lazy_compilation = true;
  build_ops_flags->tf_xla_print_cluster_outputs = false;

  mark_for_compilation_flags = new MarkForCompilationPassFlags;
  mark_for_compilation_flags->tf_xla_auto_jit = 0;
  mark_for_compilation_flags->tf_xla_min_cluster_size = 4;
  mark_for_compilation_flags->tf_xla_max_cluster_size =
      std::numeric_limits<int32>::max();
  mark_for_compilation_flags->tf_xla_clustering_debug = false;
  mark_for_compilation_flags->tf_xla_cpu_global_jit = false;
  mark_for_compilation_flags->tf_xla_clustering_fuel =
      std::numeric_limits<int64>::max();
  mark_for_compilation_flags
      ->tf_xla_disable_deadness_safety_checks_for_debugging = false;

  device_flags = new XlaDeviceFlags;
  device_flags->tf_xla_compile_on_demand = false;

  ops_flags = new XlaOpsCommonFlags;
  ops_flags->tf_xla_always_defer_compilation = false;

  jitter_flags = new IntroduceFloatingPointJitterPassFlags;
  jitter_flags->jitter_amount = 1e-5;

  auto setter_for_jitter_tensor_names = [](string sequence) {
    jitter_flags->tensor_names = absl::StrSplit(sequence, ',');
    return true;
  };

  flag_list = new std::vector<Flag>(
      {Flag("tf_xla_enable_lazy_compilation",
            &build_ops_flags->tf_xla_enable_lazy_compilation, ""),
       Flag("tf_xla_print_cluster_outputs",
            &build_ops_flags->tf_xla_print_cluster_outputs,
            "If true then insert Print nodes to print out values produced by "
            "XLA clusters."),

       Flag("tf_xla_compile_on_demand", &device_flags->tf_xla_compile_on_demand,
            "Switch a device into 'on-demand' mode, where instead of "
            "autoclustering ops are compiled one by one just-in-time."),

       Flag("tf_xla_always_defer_compilation",
            &ops_flags->tf_xla_always_defer_compilation, ""),

       Flag("tf_introduce_floating_point_jitter_to_tensors",
            setter_for_jitter_tensor_names, "",
            "The amount of jitter to introduce.  This amount is added to each "
            "element in the tensors named in `tensor_names."),
       Flag("tf_introduce_floating_point_jitter_amount",
            &jitter_flags->jitter_amount,
            "The Tensors to add the jitter to.  The tensors are named in the "
            "TensorId format of <node name>:<output idx>.")});

  AppendMarkForCompilationPassFlagsInternal(flag_list);
  xla::ParseFlagsFromEnvAndDieIfUnknown("TF_XLA_FLAGS", *flag_list);
}

}  // namespace

const BuildXlaOpsPassFlags& GetBuildXlaOpsPassFlags() {
  std::call_once(flags_init, &AllocateAndParseFlags);
  return *build_ops_flags;
}

MarkForCompilationPassFlags* GetMarkForCompilationPassFlags() {
  std::call_once(flags_init, &AllocateAndParseFlags);
  return mark_for_compilation_flags;
}

XlaDeviceFlags* GetXlaDeviceFlags() {
  std::call_once(flags_init, &AllocateAndParseFlags);
  return device_flags;
}

const XlaOpsCommonFlags& GetXlaOpsCommonFlags() {
  std::call_once(flags_init, &AllocateAndParseFlags);
  return *ops_flags;
}

const IntroduceFloatingPointJitterPassFlags&
GetIntroduceFloatingPointJitterPassFlags() {
  std::call_once(flags_init, &AllocateAndParseFlags);
  return *jitter_flags;
}

void AppendMarkForCompilationPassFlags(std::vector<Flag>* flag_list) {
  std::call_once(flags_init, &AllocateAndParseFlags);
  AppendMarkForCompilationPassFlagsInternal(flag_list);
}
}  // namespace tensorflow
