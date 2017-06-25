import nest
import nest.raster_plot
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

exc_neurons = nest.Create("pp_izhikevich_prenorm", N_E, params=exc_neuron_dict)
inh_neurons = nest.Create("izhikevich", N_I, params=inh_neuron_dict)

input_generators = nest.Create("poisson_generator", N, params=input_gen_dict)

# Defining the connections between input and target neurons
exc_to_exc_dict = {
    'conn_spec': {
        'rule': 'fixed_indegree',
        'indegree': 70,
        'autapses': False
    },
    'syn_spec' : {
        'model'   : 'stdp_connection_norm',
        'weight'  : 6.0,  # pA
        'delay'   : {'distribution': 'uniform', 'low': 1.0, 'high': 20.0},
        'alpha'   : 0.0,
        'lambda'  : 0.01,
        'mu_plus' : 0.0,
        'mu_minus': 0.0,
        'tau_plus': 10.0  # ms
    }
}

exc_to_inh_dict = {
    'conn_spec': {
        'rule': 'fixed_indegree',
        'indegree': 120
    },
    'syn_spec' : {
        'model': 'static_synapse',
        'weight': 6.0,  # pA
        'delay': {'distribution': 'uniform', 'low': 1.0, 'high': 20.0},
    }
}

inh_to_exc_dict = {
    'conn_spec': {
        'rule': 'fixed_indegree',
        'indegree': 50
    },
    'syn_spec': {
        'model':'static_synapse',
        'weight': -5.0,
        'delay': 1.0
    }
}

gen_to_neurons_dict = {
    'conn_spec': 'one_to_one',
    'syn_spec': {
        'weight': 600.0
    }
}


# Defining the connections between generators and input neurons

nest.Connect(exc_neurons, exc_neurons, conn_spec=exc_to_exc_dict['conn_spec'], syn_spec=exc_to_exc_dict['syn_spec'])
nest.Connect(exc_neurons, inh_neurons, conn_spec=exc_to_inh_dict['conn_spec'], syn_spec=exc_to_inh_dict['syn_spec'])
nest.Connect(inh_neurons, exc_neurons, conn_spec=inh_to_exc_dict['conn_spec'], syn_spec=inh_to_exc_dict['syn_spec'])

nest.Connect(input_generators, exc_neurons+inh_neurons, conn_spec=gen_to_neurons_dict['conn_spec'], syn_spec=gen_to_neurons_dict['syn_spec'])

# Defining Recorders
target_recorder_dict = {
    'record_from' : ['V_m', 'inc_weight_sum']
}
target_recorder = nest.Create("multimeter", 4, params=target_recorder_dict)
nest.Connect(target_recorder, exc_neurons[3:7], conn_spec='one_to_one');

# Defining Spike Recorder
spike_recorder = nest.Create("spike_detector", 1)
nest.Connect(exc_neurons, spike_recorder)

nest.Simulate(30000)

target_recorder_records = nest.GetStatus(target_recorder, 'events')

plt.figure()
plt.plot(target_recorder_records[0]['times'][2000:3000], target_recorder_records[0]['inc_weight_sum'][2000:3000])

nest.raster_plot.from_device(spike_recorder)

plt.figure()
plt.plot(target_recorder_records[0]['times'][2000:3000], target_recorder_records[0]['V_m'][2000:3000])

plt.show()
