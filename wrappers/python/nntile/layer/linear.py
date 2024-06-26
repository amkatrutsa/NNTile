# @copyright (c) 2022-present Skolkovo Institute of Science and Technology
#                              (Skoltech), Russia. All rights reserved.
#                2023-present Artificial Intelligence Research Institute
#                              (AIRI), Russia. All rights reserved.
#
# NNTile is software framework for fast training of big neural networks on
# distributed-memory heterogeneous systems based on StarPU runtime system.
#
# @file wrappers/python/nntile/layer/linear.py
# Linear layer of NNTile Python package
#
# @version 1.0.0

import nntile
from nntile.tensor import TensorTraits, Tensor, TensorOrNone, TensorMoments, \
        TransOp, trans, notrans, copy_async, gemm_async, randn_async, \
        add_slice_async, add_fiber_async, sum_slice_async, sum_fiber_async
from nntile.layer.base_layer import BaseLayer
import numpy as np
from typing import List, Union, Optional

class Linear(BaseLayer):
    side: str
    trans_x: TransOp
    x: TensorMoments
    y: TensorMoments
    w: TensorMoments
    ndim: int
    #b: TensorMoments
    #b_axis: int
    fp32_convert_fp16: bool
    x_fp16: TensorMoments
    y_fp16: TensorMoments
    w_fp16: TensorMoments
    b: Union[TensorMoments, None]

    # Construct linear layer with all the provided data
    def __init__(self, side: str, trans_x: TransOp, x: TensorMoments, \
            y: TensorMoments, w: TensorMoments, ndim: int, \
            b: Union[TensorMoments, None], \
            fp32_convert_fp16: bool = False, \
            x_fp16: Optional[TensorMoments] = None, \
            w_fp16: Optional[TensorMoments] = None, \
            y_fp16: Optional[TensorMoments] = None, \
            redux: bool = False):
        # Check parameter side
        if side != 'L' and side != 'R':
            raise ValueError("side must be either 'L' or 'R'")
        # Check parameter ndim
        if ndim <= 0:
            raise ValueError("ndim must be positive integer")
        # Redirect to BaseClass initialization
        if b is None:
            super().__init__([x], [y], [w], [x_fp16, w_fp16, y_fp16])
            self.b = None
        else:
            super().__init__([x], [y], [w, b], [x_fp16, w_fp16, y_fp16])
            self.b = b
            self.b.grad.set_reduction_add()
        # Set up local named parameters
        self.side = side
        self.trans_x = trans_x
        self.ndim = ndim
        self.x = x
        if self.x.grad is not None:
            self.x.grad.set_reduction_add()
        self.y = y
        self.y.value.set_reduction_add()
        self.w = w
        self.w.grad.set_reduction_add()
        self.x_fp16 = x_fp16
        self.w_fp16 = w_fp16
        self.y_fp16 = y_fp16
        self.fp32_convert_fp16 = fp32_convert_fp16
        if redux:
            self.redux = 1
        else:
            self.redux = 0

    # Simple generator for the linear layer
    @staticmethod
    def generate_simple(x: TensorMoments, side: str, trans_x: TransOp, \
            in_features_ndim: int, out_features_shape: List[int], \
            out_features_basetile_shape: List[int], next_tag: int, \
            bias: bool=True, \
            fp32_convert_fp16: bool=False, redux: bool=False):
        # Define shapes
        ndim = in_features_ndim
        add_shape = out_features_shape
        add_basetile_shape = out_features_basetile_shape
        if side == 'L':
            if trans_x == notrans:
                w_shape = x.value.shape[-ndim:] + add_shape
                w_tile = x.value.basetile_shape[-ndim:] + add_basetile_shape
                y_shape = x.value.shape[:-ndim] + add_shape
                y_tile = x.value.basetile_shape[:-ndim] + add_basetile_shape
            else:
                w_shape = x.value.shape[:ndim] + add_shape
                w_tile = x.value.basetile_shape[:ndim] + add_basetile_shape
                y_shape = x.value.shape[ndim:] + add_shape
                y_tile = x.value.basetile_shape[ndim:] + add_basetile_shape
        else:
            if trans_x == notrans:
                w_shape = add_shape + x.value.shape[:ndim]
                w_tile = add_basetile_shape + x.value.basetile_shape[:ndim]
                y_shape = add_shape + x.value.shape[ndim:]
                y_tile = add_basetile_shape + x.value.basetile_shape[ndim:]
            else:
                w_shape = add_shape + x.value.shape[-ndim:]
                w_tile = add_basetile_shape + x.value.basetile_shape[-ndim:]
                y_shape = add_shape + x.value.shape[:-ndim]
                y_tile = add_basetile_shape + x.value.basetile_shape[:-ndim]
        # Define W
        w_traits = TensorTraits(w_shape, w_tile)
        # TODO change distribution
        w_distr = [0] * w_traits.grid.nelems
        w_value = type(x.value)(w_traits, w_distr, next_tag)
        next_tag = w_value.next_tag
        # Create gradient of W with the same traits and distribution as W
        w_grad = type(x.value)(w_traits, w_distr, next_tag)
        next_tag = w_grad.next_tag
        # Define W as TensorMoments
        w = TensorMoments(w_value, w_grad, True)
        if bias:
            if len(add_shape) > 1:
                raise ValueError("Bias is not yet supported for " \
                        "len(add_shape) > 1")
            b_traits = TensorTraits(add_shape, add_basetile_shape)
            # TODO change distribution
            b_distr = [0] * b_traits.grid.nelems
            b_value = type(x.value)(b_traits, b_distr, next_tag)
            next_tag = b_value.next_tag
            # Create gradient of b with the same traits and distribution as b
            b_grad = type(x.value)(b_traits, b_distr, next_tag)
            next_tag = b_grad.next_tag
            # Define b as TensorMoments
            b = TensorMoments(b_value, b_grad, True)
        else:
            b = None
        # Define Y
        y_traits = TensorTraits(y_shape, y_tile)
        # TODO change distribution
        y_distr = [0] * y_traits.grid.nelems
        y_value = type(x.value)(y_traits, y_distr, next_tag)
        next_tag = y_value.next_tag
        # Create gradient of Y with the same traits and distribution as Y
        y_grad = type(x.value)(y_traits, y_distr, next_tag)
        next_tag = y_grad.next_tag
        # Define Y as TensorMoments
        y = TensorMoments(y_value, y_grad, True)
        # Create linear layer with all the provided data
        if type(x.value) is not nntile.tensor.Tensor_fp32:
            fp32_convert_fp16 = False
        if fp32_convert_fp16:
            x_traits = TensorTraits(x.value.shape, x.value.basetile_shape)
            x_distr = x.value.distribution
            x_fp16_value = Tensor_fp16(x_traits, x_distr, next_tag)
            next_tag = x_fp16_value.next_tag
            x_fp16_grad = Tensor_fp16(x_traits, x_distr, next_tag)
            next_tag = x_fp16_grad.next_tag
            x_fp16 = TensorMoments(x_fp16_value, x_fp16_grad, True)
            w_fp16_value = Tensor_fp16(w_traits, w_distr, next_tag)
            next_tag = w_fp16_value.next_tag
            w_fp16_grad = Tensor_fp16(w_traits, w_distr, next_tag)
            next_tag = w_fp16_grad.next_tag
            w_fp16 = TensorMoments(w_fp16_value, w_fp16_grad, True)
            y_fp16_value = Tensor_fp16(y_traits, y_distr, next_tag)
            next_tag = y_fp16_value.next_tag
            y_fp16_grad = Tensor_fp16(y_traits, y_distr, next_tag)
            next_tag = y_fp16_grad.next_tag
            y_fp16 = TensorMoments(y_fp16_value, y_fp16_grad, True)
            layer = Linear(side, trans_x, x, y, w, ndim, b, fp32_fast_tf32, \
                    fp32_convert_fp16, x_fp16, w_fp16, y_fp16)
        else:
            layer = Linear(side, trans_x, x, y, w, ndim, b, \
                    redux=redux)
        # Return layer and next tag to be used
        return (layer, next_tag)

    # Forward propagation of the linear layer
    def forward_async(self):
        # Convert fp32 to fp16 if needed
        if self.fp32_convert_fp16:
            fp32_to_fp16_async(self.x.value, self.x_fp16.value)
            fp32_to_fp16_async(self.w.value, self.w_fp16.value)
        # Perform actual gemm
        if self.side == 'L':
            # Y = einsum('ij,jk->ik', op(X), W)
            # 'i' is a multi-index of dimension X.ndim-ndim
            # 'j' is a multi-index of dimension ndim
            # 'k' is a multi-index of dimension W.ndim-ndim
            if self.fp32_convert_fp16:
                gemm_async(1.0, self.trans_x, self.x_fp16.value, notrans, \
                        self.w_fp16.value, 0.0, self.y_fp16.value, \
                        self.ndim, 0, redux=self.redux)
                fp16_to_fp32_async(self.y_fp16.value, self.y.value)
                self.x_fp16.value.wont_use()
                self.w_fp16.value.wont_use()
                self.y_fp16.value.wont_use()
            else:
                gemm_async(1.0, self.trans_x, self.x.value, notrans, \
                        self.w.value, 0.0, self.y.value, self.ndim, 0, \
                        redux=self.redux)
            if self.b is not None:
                add_fiber_async(1.0, self.b.value, 1.0, self.y.value,
                        self.y.value.ndim-1, 0)
        else:
            # Y = einsum('ij,jk->ik', W, op(X))
            # 'i' is a multi-index of dimension W.ndim-ndim
            # 'j' is a multi-index of dimension ndim
            # 'k' is a multi-index of dimension X.ndim-ndim
            if self.fp32_convert_fp16:
                gemm_async(1.0, notrans, self.w_fp16.value, self.trans_x, \
                        self.x_fp16.value, 0.0, self.y_fp16.value, \
                        self.ndim, 0, redux=self.redux)
                fp16_to_fp32_async(self.y_fp16.value, self.y.value)
                self.x_fp16.value.wont_use()
                self.w_fp16.value.wont_use()
                self.y_fp16.value.wont_use()
            else:
                gemm_async(1.0, notrans, self.w.value, self.trans_x, \
                        self.x.value, 0.0, self.y.value, self.ndim, 0, \
                        redux=self.redux)
            if self.b is not None:
                add_fiber_async(1.0, self.b.value, 1.0, self.y.value, 0, 0)
        # Hint for StarPU that W tensor will
        # not be used soon and it is advised to offload data from GPU
        self.w.value.wont_use()
        self.x.value.wont_use()
        self.y.value.wont_use()
        if self.b is not None:
            self.b.value.wont_use()

    # Backward propagation of the linear layer
    def backward_async(self):
        # Convert fp32 to fp16 if needed
        if self.fp32_convert_fp16:
            fp32_to_fp16_async(self.y.grad, self.y_fp16.grad)
        # Gradient over W (weights)
        if self.w.grad_required:
            # Convert fp32 to fp16 if needed
            if self.fp32_convert_fp16:
                fp32_to_fp16_async(self.w.grad, self.w_fp16.grad)
            gemm_ndim = self.x.value.ndim - self.ndim
            if self.side == 'L':
                # Backward for Y = einsum('ij,jk->ik', op(X), W)
                # dW += einsum('ij,ik->jk', op(X), dY)
                # 'i' is a multi-index of dimension X.ndim-ndim
                # 'j' is a multi-index of dimension ndim
                # 'k' is a multi-index of dimension W.ndim-ndim
                if self.trans_x == notrans:
                    if self.fp32_convert_fp16:
                        gemm_async(1.0, trans, self.x_fp16.value, notrans, \
                                self.y_fp16.grad, 1.0, self.w_fp16.grad, \
                                gemm_ndim, 0, redux=self.redux)
                    else:
                        gemm_async(1.0, trans, self.x.value, notrans, \
                                self.y.grad, 1.0, self.w.grad, gemm_ndim, 0, \
                                redux=self.redux)
                else:
                    if self.fp32_convert_fp16:
                        gemm_async(1.0, notrans, self.x_fp16.value, notrans, \
                                self.y_fp16.grad, 1.0, self.w_fp16.grad, \
                                gemm_ndim, 0, redux=self.redux)
                    else:
                        gemm_async(1.0, notrans, self.x.value, notrans, \
                                self.y.grad, 1.0, self.w.grad, gemm_ndim, 0, \
                                redux=self.redux)
            else:
                # Backward for Y = einsum('ij,jk->ik', W, op(X))
                # dW += einsum('ik,jk->ij', dY, op(X))
                # 'i' is a multi-index of dimension W.ndim-ndim
                # 'j' is a multi-index of dimension ndim
                # 'k' is a multi-index of dimension X.ndim-ndim
                if self.trans_x == notrans:
                    if self.fp32_convert_fp16:
                        gemm_async(1.0, notrans, self.y_fp16.grad, trans, \
                                self.x_fp16.value, 1.0, self.w_fp16.grad, \
                                gemm_ndim, 0, redux=self.redux)
                    else:
                        gemm_async(1.0, notrans, self.y.grad, trans, \
                                self.x.value, 1.0, self.w.grad, gemm_ndim, 0, \
                                redux=self.redux)
                else:
                    if self.fp32_convert_fp16:
                        gemm_async(1.0, notrans, self.y_fp16.grad, notrans, \
                                self.x_fp16.value, 1.0, self.w_fp16.grad, \
                                gemm_ndim, 0, redux=self.redux)
                    else:
                        gemm_async(1.0, notrans, self.y.grad, notrans, \
                                self.x.value, 1.0, self.w.grad, gemm_ndim, 0, \
                                redux=self.redux)
            # Convert fp16 to fp32 if needed and offload data
            if self.fp32_convert_fp16:
                fp16_to_fp32_async(self.w_fp16.grad, self.w.grad)
                self.x_fp16.value.wont_use()
                self.w_fp16.grad.wont_use()
            # Hint StarPU to offload gradient over W if needed
            self.w.grad.wont_use()
            self.x.value.wont_use()
            self.y.grad.wont_use()
        if self.b is not None:
            if self.b.grad_required:
                if self.side == 'L':
                    sum_fiber_async(1.0, self.y.grad, 1.0, self.b.grad, \
                            self.y.value.ndim-1, 0, redux=self.redux)
                else:
                    sum_fiber_async(1.0, self.y.grad, 1.0, self.b.grad, 0, 0, \
                            redux=self.redux)
                self.b.grad.wont_use()
                self.y.grad.wont_use()
        # Gradient over X (input)
        if self.x.grad_required:
            # Convert fp32 to fp16 if needed
            if self.fp32_convert_fp16:
                fp32_to_fp16_async(self.x.grad, self.x_fp16.grad)
            gemm_ndim = self.w.value.ndim - self.ndim
            if self.side == 'L':
                # Backward for Y = einsum('ij,jk->ik', op(X), W)
                # d op(X) += einsum('ik,jk->ij', dY, W)
                # 'i' is a multi-index of dimension X.ndim-ndim
                # 'j' is a multi-index of dimension ndim
                # 'k' is a multi-index of dimension W.ndim-ndim
                if self.trans_x == notrans:
                    # dX += einsum('ik,jk->ij', dY, W)
                    if self.fp32_convert_fp16:
                        gemm_async(1.0, notrans, self.y_fp16.grad, trans, \
                                self.w_fp16.value, 1.0, self.x_fp16.grad, \
                                gemm_ndim, 0, redux=self.redux)
                    else:
                        gemm_async(1.0, notrans, self.y.grad, trans, \
                                self.w.value, 1.0, self.x.grad, gemm_ndim, 0, \
                                redux=self.redux)
                else:
                    # dX += einsum('ik,jk->ij', W, dY)
                    if self.fp32_convert_fp16:
                        gemm_async(1.0, notrans, self.w_fp16.value, trans, \
                                self.y_fp16.grad, 1.0, self.x_fp16.grad, \
                                gemm_ndim, 0, redux=self.redux)
                    else:
                        gemm_async(1.0, notrans, self.w.value, trans, \
                                self.y.grad, 1.0, self.x.grad, gemm_ndim, 0, \
                                redux=self.redux)
            else:
                # Backward for Y = einsum('ij,jk->ik', W, op(X))
                # d op(X) = einsum('ij,ik->jk', W, dY)
                # 'i' is a multi-index of dimension W.ndim-ndim
                # 'j' is a multi-index of dimension ndim
                # 'k' is a multi-index of dimension X.ndim-ndim
                if self.trans_x == notrans:
                    # dX += einsum('ij,ik->jk', W, dY)
                    if self.fp32_convert_fp16:
                        gemm_async(1.0, trans, self.w_fp16.value, notrans, \
                                self.y_fp16.grad, 1.0, self.x_fp16.grad, \
                                gemm_ndim, 0, redux=self.redux)
                    else:
                        gemm_async(1.0, trans, self.w.value, notrans, \
                                self.y.grad, 1.0, self.x.grad, gemm_ndim, 0, \
                                redux=self.redux)
                else:
                    # dX = einsum('ij,ik->jk', dY, W)
                    if self.fp32_convert_fp16:
                        gemm_async(1.0, trans, self.y_fp16.grad, notrans, \
                                self.w_fp16.value, 1.0, self.x_fp16.grad, \
                                gemm_ndim, 0, redux=self.redux)
                    else:
                        gemm_async(1.0, trans, self.y.grad, notrans, \
                                self.w.value, 1.0, self.x.grad, gemm_ndim, 0, \
                                redux=self.redux)
            # Convert fp16 to fp32 if needed and offload data
            if self.fp32_convert_fp16:
                fp16_to_fp32_async(self.x_fp16.grad, self.x.grad)
                self.x_fp16.grad.wont_use()
                self.w_fp16.value.wont_use()
            # Hint StarPU to offload certain buffers
            self.x.grad.wont_use()
        self.x.value.wont_use()
        self.y.value.wont_use()
        self.y.grad.wont_use()
        self.w.value.wont_use()
