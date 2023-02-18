# @copyright (c) 2022-2023 Skolkovo Institute of Science and Technology
#                           (Skoltech). All rights reserved.
#
# NNTile is software framework for fast training of big neural networks on
# distributed-memory heterogeneous systems based on StarPU runtime system.
#
# @file wrappers/python/nntile/model/deep_relu.py
# Deep ReLU model of NNTile Python package
#
# @version 1.0.0
# @author Aleksandr Mikhalev
# @date 2023-02-17

from nntile.tensor import TensorTraits, Tensor, TensorOrNone, TensorMoments, \
        notrans
from nntile.model.base_model import BaseModel
from nntile.layer.linear import Linear
from nntile.layer.act import Act
import numpy as np
from typing import List

class DeepReLU(BaseModel):
    next_tag: int

    # Construct model with all the provided data
    def __init__(self, x: TensorMoments, side: str, ndim: int, add_shape: int,
            add_basetile_shape: int, nlayers: int, next_tag: int):
        # Check parameter side
        if side != 'L' and side != 'R':
            raise ValueError("side must be either 'L' or 'R'")
        # Check number of layers
        if nlayers < 2:
            raise ValueError("nlayers must be at least 2")
        # Check parameter ndim
        if ndim <= 0:
            raise ValueError("ndim must be positive integer")
        # Init activations and list of layers
        activations = [x]
        layers = []
        # Initial linear layer that converts input to internal shape
        new_layer, next_tag = Linear.generate_simple_mpiroot(x, side, notrans,
                ndim, [add_shape], [add_basetile_shape], next_tag)
        layers.append(new_layer)
        activations.extend(new_layer.activations_output)
        new_layer, next_tag = Act.generate_simple(activations[-1], "relu",
                                                  next_tag)
        layers.append(new_layer)
        activations.extend(new_layer.activations_output)
        # Internal linear layers with the same internal shape
        for i in range(1, nlayers-1):
            new_layer, next_tag = Linear.generate_simple_mpiroot(
                    activations[-1], side, notrans, 1, [add_shape],
                    [add_basetile_shape], next_tag)
            layers.append(new_layer)
            activations.extend(new_layer.activations_output)
            new_layer, next_tag = Act.generate_simple(activations[-1], "relu",
                                                      next_tag)
            layers.append(new_layer)
            activations.extend(new_layer.activations_output)
        # Finalizing linear layer that converts result back to proper shape
        if side == 'L':
            new_shape = x.value.shape[-ndim:]
            new_base = x.value.basetile_shape[-ndim:]
        else:
            new_shape = x.value.shape[:ndim]
            new_base = x.value.basetile_shape[:ndim]
        new_layer, next_tag = Linear.generate_simple_mpiroot(activations[-1],
                side, notrans, 1, new_shape, new_base, next_tag)
        layers.append(new_layer)
        activations.extend(new_layer.activations_output)
        self.next_tag = next_tag
        # Fill Base Model with the generated data
        super().__init__(activations, layers)

    # Randomly init all linear layers
    def init_randn_async(self):
        for l in self.layers:
            if type(l) is Linear:
                l.init_randn_async()

