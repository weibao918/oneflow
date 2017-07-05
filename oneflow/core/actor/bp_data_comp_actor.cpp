#include "oneflow/core/actor/bp_data_comp_actor.h"
#include "oneflow/core/actor/actor_registry.h"
#include "oneflow/core/register/local_register_warpper.h"

namespace oneflow {

void BpDataCompActor::Init(const TaskProto& task_proto,
                           const ThreadCtx& thread_ctx) {
  Actor::Init(task_proto, thread_ctx);
  model_regst_desc_id_ = RegstDescId4Name("model");
  model_tmp_regst_desc_id_ = RegstDescId4Name("model_tmp");
  activation_regst_desc_id_ = RegstDescId4Name("activation");
  data_tmp_regst_desc_id_ = RegstDescId4Name("data_tmp");
  expected_model_version_id_ = 0;
  num_of_read_empty_ = 6;
  num_of_eord_ = 0;
  if (thread_ctx.cpu_stream) {
    mut_device_ctx().reset(new CpuDeviceCtx(thread_ctx.cpu_stream));
  } else {
    mut_device_ctx().reset(new CudaDeviceCtx(cuda_handle_.cuda_stream(),
                                             cuda_handle_.cublas_handle(),
                                             cuda_handle_.cudnn_handle()));
  }
  OF_SET_MSG_HANDLE(&BpDataCompActor::HandleNormal);
}

bool BpDataCompActor::IsReadReady() {
  if (num_of_read_empty_) { return false; }
  if (read_regst_.at(model_regst_desc_id_).front()->model_version_id()
      != read_regst_.at(activation_regst_desc_id_)
             .front()
             ->model_version_id()) {
    AsyncSendRegstMsgToProducer(read_regst_.at(model_regst_desc_id_).front());
    read_regst_.at(model_regst_desc_id_).pop();
    num_of_read_empty_ += read_regst_.at(model_regst_desc_id_).empty();
  }
  return !num_of_read_empty_;
}

int BpDataCompActor::HandleNormal(const ActorMsg& msg) {
  if (msg.msg_type() == ActorMsgType::kCmdMsg) {
    CHECK_EQ(msg.actor_cmd(), ActorCmd::kEORD);
    num_of_eord_ += 1;
    if (num_of_eord_ == 6) {
      OF_SET_MSG_HANDLE(&BpDataCompActor::HandleWaitUntilNoReadableRegst);
    }
  } else if (msg.msg_type() == ActorMsgType::kRegstMsg) {
    if (TryUpdtStateAsProducedRegst(msg.regst_warpper()->regst_raw_ptr())
        != 0) {
      std::shared_ptr<RegstWarpper> regst_wp = msg.regst_warpper();
      if (regst_wp->regst_desc_id() == model_tmp_regst_desc_id_) {
        CHECK(read_regst_.find(model_tmp_regst_desc_id_) == read_regst_.end());
      } else if (regst_wp->regst_desc_id() == model_regst_desc_id_) {
        CHECK_EQ(regst_wp->model_version_id(), expected_model_version_id_++);
      } else {
        // do nothing
      }
      num_of_read_empty_ -= read_regst_[regst_wp->regst_desc_id()].empty();
      read_regst_.at(regst_wp->regst_desc_id()).push(regst_wp);
    }
  }
  TryActUntilFail();
  return 0;
}

int BpDataCompActor::HandleWaitUntilNoReadableRegst(const ActorMsg& msg) {
  CHECK_EQ(TryUpdtStateAsProducedRegst(msg.regst_warpper()->regst_raw_ptr()),
           0);
  TryActUntilFail();
  if (read_regst_.at(activation_regst_desc_id_).empty()) {
    while (!read_regst_.at(model_regst_desc_id_).empty()) {
      AsyncSendRegstMsgToProducer(read_regst_.at(model_regst_desc_id_).front());
      read_regst_.at(model_regst_desc_id_).pop();
    }
    AsyncSendRegstMsgToProducer(
        read_regst_.at(model_tmp_regst_desc_id_).front());
    read_regst_.at(model_tmp_regst_desc_id_).pop();
    AsyncSendEORDMsgForAllProducedRegstDesc();
    num_of_read_empty_ = 6;
    if (total_reading_cnt() == 0) {
      OF_SET_MSG_HANDLE(nullptr);
      return 1;
    } else {
      OF_SET_MSG_HANDLE(&BpDataCompActor::HandleWaitUntilReadingCntEqualZero);
      return 0;
    }
  }
  return 0;
}

void BpDataCompActor::Act() {
  int64_t cur_model =
      read_regst_.at(model_regst_desc_id_).front()->model_version_id();
  int64_t piece_id = expected_piece_id();
  CHECK_EQ(
      cur_model,
      read_regst_.at(activation_regst_desc_id_).front()->model_version_id());
  CHECK_EQ(cur_model,
           read_regst_.at(data_tmp_regst_desc_id_).front()->model_version_id());
  for (const auto& pair : read_regst_) {
    if (pair.first != model_regst_desc_id_
        && pair.first != model_tmp_regst_desc_id_) {
      CHECK_EQ(pair.second.front()->piece_id(), piece_id);
    }
  }
  AsyncLaunchKernel(
      GenDefaultKernelCtx(),
      [this](int64_t regst_desc_id) -> std::shared_ptr<RegstWarpper> {
        Regst* regst = GetCurWriteableRegst(regst_desc_id);
        if (regst == nullptr) {
          return read_regst_.at(regst_desc_id).front();
        } else {
          return std::make_shared<LocalRegstWarpper>(regst);
        }
      });
  AsyncSendReadableRegstMsg(
      [piece_id](Regst* regst) { regst->set_piece_id(piece_id); });
  for (auto& pair : read_regst_) {
    if (pair.first != model_regst_desc_id_
        && pair.first != model_tmp_regst_desc_id_) {
      AsyncSendRegstMsgToProducer(pair.second.front());
      pair.second.pop();
      num_of_read_empty_ += pair.second.empty();
    }
  }
}

REGISTER_ACTOR(kDataCompTask, false, BpDataCompActor);

}  // namespace oneflow
