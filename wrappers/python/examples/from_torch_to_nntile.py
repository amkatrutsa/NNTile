import torch
import torch.nn as nn
import torchvision.datasets as dts 
import numpy as np
import random
import os
import nntile
import torchvision.transforms as trnsfrms

def set_all_seeds(seed):
    random.seed(seed)
    np.random.seed(seed)
    torch.manual_seed(seed)
    torch.cuda.manual_seed(seed)
    torch.backends.cudnn.deterministic = True

set_all_seeds(121)

class MLP(nn.Module):
    def __init__(self):
        super(MLP, self).__init__()
        self.linear1 = nn.Linear(28*28, 100, bias=False) 
        self.linear2 = nn.Linear(100, 100, bias=False)
        self.final = nn.Linear(100, 10, bias=False)
        self.relu = nn.ReLU()

    def forward(self, image):
        a = image.view(-1, 28*28)
        a = self.relu(self.linear1(a))
        a = self.relu(self.linear2(a))
        a = self.final(a)
        return a

n_classes = 10
batch_size = 1000
lr = 1e-3
mlp_model = MLP()
optim_torch = torch.optim.SGD(mlp_model.parameters(), lr=lr)
crit_torch = nn.CrossEntropyLoss(reduction="sum")

trnsform = trnsfrms.Compose([trnsfrms.ToTensor()])

mnisttrainset = dts.MNIST(root='./data', train=True, download=True, transform=trnsform)
trainldr = torch.utils.data.DataLoader(mnisttrainset, batch_size=1000, shuffle=True)

for train_batch_sample, true_labels in trainldr:
    # true_labels = torch.randint(0, n_classes, (batch_size, ))
    # train_batch_sample = torch.randn((batch_size, 28*28))
    train_batch_sample = train_batch_sample.view(-1, 28*28)
    torch_output = mlp_model(train_batch_sample)
    torch_loss = crit_torch(torch_output, true_labels)
    print("PyTorch loss =", torch_loss.item())
    break

mlp_model.zero_grad()
torch_loss.backward()

config = nntile.starpu.Config(-1, -1, 1)
nntile.starpu.init()
next_tag = 0

mlp_nntile, next_tag = nntile.model.DeepReLU.from_torch(mlp_model, batch_size, n_classes, "relu", next_tag)
optimizer = nntile.optimizer.SGD(mlp_nntile.get_parameters(), lr, next_tag, momentum=0.0,
       nesterov=False, weight_decay=0.)
# optimizer = nntile.optimizer.Adam(m.get_parameters(), lr, next_tag)
next_tag = optimizer.get_next_tag()

data_train_traits = nntile.tensor.TensorTraits(train_batch_sample.shape, \
        train_batch_sample.shape)
data_train_tensor = nntile.tensor.Tensor_fp32(data_train_traits, [0], next_tag)
next_tag = data_train_tensor.next_tag
data_train_tensor.from_array(train_batch_sample)
crit_nntile, next_tag = nntile.loss.CrossEntropy.generate_simple(mlp_nntile.activations[-1], next_tag)

nntile.tensor.copy_async(data_train_tensor, mlp_nntile.activations[0].value)
mlp_nntile.forward_async()

label_train_traits = nntile.tensor.TensorTraits(true_labels.shape, \
        true_labels.shape)
label_train_tensor = nntile.tensor.Tensor_int64(label_train_traits, [0], next_tag)
next_tag = label_train_tensor.next_tag
label_train_tensor.from_array(true_labels.numpy())

nntile.tensor.copy_async(label_train_tensor, crit_nntile.y)
crit_nntile.calc_async()
nntile_xentropy_np = np.zeros((1,), dtype=np.float32, order="F")
crit_nntile.get_val(nntile_xentropy_np)
print("NNTile loss =", nntile_xentropy_np[0])

mlp_nntile.backward_async()

for p_torch, p_nntile in zip(mlp_model.parameters(), mlp_nntile.parameters):
    p_nntile_grad_np = np.zeros(p_nntile.grad.shape, order="F", dtype=np.float32)
    p_nntile.grad.to_array(p_nntile_grad_np)
    print(np.linalg.norm(p_nntile_grad_np.T - p_torch.grad.numpy(), "fro"))


# Make optimizer step and compare updated losses
optim_torch.step()
torch_output = mlp_model(train_batch_sample)
torch_loss = crit_torch(torch_output, true_labels)
print("PyTorch loss after optimizer step =", torch_loss.item())
_, pred_labels = torch.max(torch_output, 1)
torch_accuracy = torch.sum(true_labels == pred_labels) / true_labels.shape[0]
print("PyTorch accuracy =", torch_accuracy.item())

optimizer.step()
nntile.tensor.copy_async(data_train_tensor, mlp_nntile.activations[0].value)
mlp_nntile.forward_async()

nntile_last_layer_output = np.zeros(mlp_nntile.activations[-1].value.shape, order="F", dtype=np.float32)
mlp_nntile.activations[-1].value.to_array(nntile_last_layer_output)
pred_labels = np.argmax(nntile_last_layer_output, 1)
nntile_accuracy = np.sum(true_labels.numpy() == pred_labels) / true_labels.shape[0]
print("NNTile accuracy =", nntile_accuracy)

nntile.tensor.copy_async(label_train_tensor, crit_nntile.y)
crit_nntile.calc_async()
nntile_xentropy_np = np.zeros((1,), dtype=np.float32, order="F")
crit_nntile.get_val(nntile_xentropy_np)
print("NNTile loss after optimizer step =", nntile_xentropy_np[0])


mlp_nntile.unregister()
crit_nntile.unregister()
data_train_tensor.unregister()
label_train_tensor.unregister()