import nest
import matplotlib.pyplot as plt

nest.ResetKernel()
nest.SetKernelStatus({'local_num_threads': 12})

if 'pp_izhikevich_prenorm' not in nest.Models():
    nest.Install("izhiktestmodule")

target_neuron_dict = {

  # 'model' : 'pp_izhikevich_prenorm'
    'norm_value' : 250.0,    # pA
    'norm_period': 150.0,   # ms
    'tau_minus'  : 10.0     # ms
}

input_gen_dict = {
  # 'model': 'poisson_generator''
    'rate' : 10000.0  # Hz
}

imput_neuron_dict = {
  # 'model': 'izhikevich'
}

target_neuron = nest.Create("pp_izhikevich_prenorm", params=target_neuron_dict)

input_neurons = nest.Create("izhikevich", 100)

input_generators = nest.Create("poisson_generator", 100, params=input_gen_dict) #Hz

# Defining the connections between input and target neurons
input_to_target_dict = {
    'model': 'stdp_connection_norm',
    'weight': 2.5,  # pA
    'delay': {'distribution': 'uniform', 'low': 0.1, 'high': 5.0},
    'alpha': 0.0,
    'lambda': 0.01,
    'tau_plus':10.0  # ms
}

# Defining the connections between generators and input neurons
gen_to_input_dict = {
    'weight': 2.5
}

nest.Connect(input_neurons, target_neuron, syn_spec=input_to_target_dict)
nest.Connect(input_generators, input_neurons, conn_spec='one_to_one', syn_spec=gen_to_input_dict)

target_recorder_dict = {
    'record_from' : ['V_m', "inc_weight_sum"]
}

target_recorder = nest.Create("multimeter", 1)
nest.SetStatus(target_recorder, target_recorder_dict)

nest.Connect(target_recorder, target_neuron)

nest.Simulate(1000)

target_recorder_records = nest.GetStatus(target_recorder, 'events')

plt.figure(1)
plt.plot(target_recorder_records[0]['times'], target_recorder_records[0]['inc_weight_sum'])

plt.figure(2)
plt.plot(target_recorder_records[0]['times'], target_recorder_records[0]['V_m'])

plt.show()