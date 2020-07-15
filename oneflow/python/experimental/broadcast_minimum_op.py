from __future__ import absolute_import

import operator
from functools import reduce

import oneflow as flow
import oneflow.core.operator.op_conf_pb2 as op_conf_util
import oneflow.core.register.logical_blob_id_pb2 as logical_blob_id_util
import oneflow.python.framework.interpret_util as interpret_util
import oneflow.python.framework.distribute as distribute_util
import oneflow.python.framework.id_util as id_util
import oneflow.python.framework.remote_blob as remote_blob_util
from oneflow.python.oneflow_export import oneflow_export


@oneflow_export("experimental.broadcast_minimum")
def broadcast_minimum(a, b, name=None):
    op_conf = op_conf_util.OperatorConf()
    if name is None:
        op_conf.name = id_util.UniqueStr("BroadcastMinimum_")
    else:
        op_conf.name = name

    op_conf.broadcast_minimum_conf.a = a.unique_name
    op_conf.broadcast_minimum_conf.b = b.unique_name
    op_conf.broadcast_minimum_conf.out = "out"

    interpret_util.Forward(op_conf)
    lbi = logical_blob_id_util.LogicalBlobId()
    lbi.op_name = op_conf.name
    lbi.blob_name = "out"
    return remote_blob_util.RemoteBlob(lbi)