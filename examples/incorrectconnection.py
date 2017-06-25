import nest
import matplotlib.pyplot as plt
import scipy

nest.ResetKernel()
nest.SetKernelStatus({'local_num_threads': 12})

if 'pp_izhikevich_prenorm' not in nest.Models():
    nest.Install("izhiktestmodule")

exc_neuron_dict = {
  # 'model' : 'pp_izhikevich_prenorm'
    'norm_value' : 350.0,    # pA
    'norm_period': 150.0,   # ms
    'tau_minus'  : 10.0     # ms
}

inh_neuron_dict = {
  # 'model' : 'izhikevich'
    'a': 0.1,
    'b': 0.2,
    'c': -65.0,
    'd': 2.0
}

input_gen_dict = {
  # 'model': 'poisson_generator''
    'rate' : 5.0  # Hz
}

N = 1000
N_E = int(N*0.8)
N_I = int(N*0.2)

neurons = nest.Create("izhikevich", 2, params=inh_neuron_dict)

nest.Connect(neurons[0:1], neurons[1:2], syn_spec={'model':'stdp_connection_norm'})