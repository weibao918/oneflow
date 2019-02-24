#include "oneflow/core/operator/yolo_prob_loss_op.h"

namespace oneflow {

void YoloProbLossOp::InitFromOpConf() {
  CHECK(op_conf().has_yolo_prob_loss_conf());
  // Enroll input
  EnrollInputBn("bbox_objness");
  EnrollInputBn("bbox_clsprob");
  EnrollInputBn("pos_cls_label", false);
  EnrollInputBn("pos_inds", false);
  EnrollInputBn("neg_inds", false);

  // Enroll output
  EnrollOutputBn("bbox_objness_out", true);
  EnrollOutputBn("bbox_clsprob_out", true);
  // data tmp
  EnrollDataTmpBn("label_tmp");
  EnrollDataTmpBn("bbox_objness_tmp");
  EnrollDataTmpBn("bbox_clsprob_tmp");
}

const PbMessage& YoloProbLossOp::GetCustomizedConf() const {
  return op_conf().yolo_prob_loss_conf();
}

void YoloProbLossOp::InferBlobDescs(std::function<BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
                                    const ParallelContext* parallel_ctx) const {
  // input: bbox_objness : (n, r, 1)  r = h*w*3
  const BlobDesc* bbox_objness_blob_desc = GetBlobDesc4BnInOp("bbox_objness");
  // input: bbox_clsprob : (n, r, 80)  r = h*w*3
  const BlobDesc* bbox_clsprob_blob_desc = GetBlobDesc4BnInOp("bbox_clsprob");
  // input: pos_cls_label (n, r)
  const BlobDesc* pos_cls_label_blob_desc = GetBlobDesc4BnInOp("pos_cls_label");
  // input: pos_inds (n, r) int32_t
  const BlobDesc* pos_inds_blob_desc = GetBlobDesc4BnInOp("pos_inds");
  // input: neg_inds (n, r) int32_t
  const BlobDesc* neg_inds_blob_desc = GetBlobDesc4BnInOp("neg_inds");

  const int64_t num_images = bbox_objness_blob_desc->shape().At(0);
  CHECK_EQ(num_images, bbox_clsprob_blob_desc->shape().At(0));
  CHECK_EQ(num_images, pos_cls_label_blob_desc->shape().At(0));
  CHECK_EQ(num_images, pos_inds_blob_desc->shape().At(0));
  CHECK_EQ(num_images, neg_inds_blob_desc->shape().At(0));
  const int64_t num_boxes = bbox_objness_blob_desc->shape().At(1);
  const int64_t num_clsprobs = op_conf().yolo_prob_loss_conf().num_classes();
  CHECK_EQ(num_boxes, pos_cls_label_blob_desc->shape().At(1));
  CHECK_EQ(num_boxes, pos_inds_blob_desc->shape().At(1));
  CHECK_EQ(num_boxes, neg_inds_blob_desc->shape().At(1));
  CHECK_EQ(1, bbox_objness_blob_desc->shape().At(2));
  CHECK_EQ(num_clsprobs, bbox_clsprob_blob_desc->shape().At(2));

  // output: bbox_objness_out (n, r, 1)
  *GetBlobDesc4BnInOp("bbox_objness_out") = *bbox_objness_blob_desc;

  // output: bbox_clsprob_out (n, r, 80)
  *GetBlobDesc4BnInOp("bbox_clsprob_out") = *bbox_clsprob_blob_desc;

  // tmp: label_tmp (n, 80) int32_t
  BlobDesc* label_tmp_blob_desc = GetBlobDesc4BnInOp("label_tmp");
  label_tmp_blob_desc->mut_shape() = Shape({num_images, num_clsprobs});
  label_tmp_blob_desc->set_data_type(DataType::kInt32);
  *GetBlobDesc4BnInOp("bbox_objness_tmp") = *bbox_objness_blob_desc;
  *GetBlobDesc4BnInOp("bbox_clsprob_tmp") = *bbox_clsprob_blob_desc;
}

// REGISTER_OP(OperatorConf::kYoloProbLossConf, YoloProbLossOp);
REGISTER_CPU_OP(OperatorConf::kYoloProbLossConf, YoloProbLossOp);
}  // namespace oneflow
