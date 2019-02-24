#ifndef ONEFLOW_CORE_KERNEL_LOGISTIC_KERNEL_H_
#define ONEFLOW_CORE_KERNEL_LOGISTIC_KERNEL_H_

#include "oneflow/core/kernel/kernel.h"
#include "oneflow/core/kernel/kernel_context.h"

namespace oneflow {

template<DeviceType device_type, typename T>
class LogisticKernel final : public KernelIf<device_type> {
 public:
  OF_DISALLOW_COPY_AND_MOVE(LogisticKernel);
  LogisticKernel() = default;
  ~LogisticKernel() = default;

 private:
  void ForwardDataContent(const KernelCtx&,
                          std::function<Blob*(const std::string&)>) const override;
  void BackwardDataContent(const KernelCtx&,
                           std::function<Blob*(const std::string&)>) const override;
};

}  // namespace oneflow

#endif  // ONEFLOW_CORE_KERNEL_LOGISTIC_KERNEL_H_
