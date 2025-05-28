# Keith Rausch

import numpy as np
import matplotlib.pyplot as plt
import json, base64
import os

def global_constraints(receivers, emitters, receiver_gradients=None, emitter_gradients=None, verbose=False):

    n_dimensions = receivers.shape[0]

    if receiver_gradients is None:
        receiver_gradients = np.zeros_like(receivers) * np.nan

    if emitter_gradients is None:
        emitter_gradients = np.zeros_like(emitters) * np.nan
    
    # first receiver is fixed to the origin
    receivers[:,0] = 0
    receiver_gradients[:,0] = 0

    # second receiver is fixed to the x axis (y=0, z=0)
    receivers[1:,1] = 0
    receiver_gradients[1:,1] = 0 
    if verbose:
        print('second receiver is locked in y only')

    # receivers[:,1] = [1,0]
    # receiver_gradients[:,1] = 0
    # if verbose:
    #     print('second receiver is locked in x,y')

    ##### first and second receivers
    # receivers[:,1] = [1,0]
    # receivers[:,2] = [1,1]
    # receiver_gradients[:,1] = 0 
    # receiver_gradients[:,2] = 0 
    # if verbose:
    #     print('second and third receivers are fully locked')


    # if we are 3D, then fix the third vector to the plane formed by the first two receivers
    if n_dimensions == 3: 
        receivers[2:,2] = 0
        receiver_gradients[2:,2] = 0 

    if verbose:
        is_nan = np.isnan(np.concatenate([receiver_gradients, emitter_gradients], axis=1))
        n_free = np.sum(is_nan)
        n_fixed = np.sum(~is_nan)
        n_total = np.size(is_nan)
        n_receivers = receivers.shape[-1]
        n_emitters = emitters.shape[-1]
        n_pairwise = n_receivers*(n_receivers-1)*0.5
        n_constraints = n_emitters * n_pairwise
        print("n_total={}, n_free={}, n_fixed={}, n_constraints={}".format(n_total, n_free, n_fixed, n_constraints))

    return receivers, emitters, receiver_gradients, emitter_gradients

class System:
    def __init__(self):
        pass

    def initialize(self, initial_receivers=None, initial_emitters=None, n_receivers=None, n_emitters=None, n_dimensions=2, speed_of_sound=1):
        self.candidate_receivers = np.random.rand(n_dimensions, n_receivers) if initial_receivers is None else initial_receivers
        self.candidate_emitters = np.random.rand(n_dimensions, n_emitters) if initial_emitters is None else initial_emitters

        self.n_receivers = self.candidate_receivers.shape[1]
        self.n_emitters = self.candidate_emitters.shape[1]
        self.n_dimensions = self.candidate_emitters.shape[0]
        self.speed_of_sound = speed_of_sound

        self.receiver_gradients = np.zeros_like(self.candidate_receivers)
        self.emitter_gradients = np.zeros_like(self.candidate_emitters)

    def calibrate(self, delta_times_r, delta_times_e, emitter_pairs, max_iterations = 15, gradient_tolerance = 1E-4, alpha = 1E-1):
        # assume already initialized
        self.delta_times_r = delta_times_r
        self.delta_times_e = delta_times_e
        self.emitter_pairs = emitter_pairs
        self.receiver_history = []
        self.emitter_history = []

        self.apply_constraints()

        cost_0 = self.compute_cost()
        old_cost = cost_0
        cost = cost_0

        self.compute_gradient()
        self.apply_constraints()

        iter = 0
        max_iterations = np.inf if max_iterations is None else max_iterations
        while not self.gradient_magnitude() <= gradient_tolerance and iter < max_iterations:
            if iter % 500 == 0:
                print('iteration {}, cost={}, gradient_mag={}'.format(iter, self.compute_cost(), self.gradient_magnitude()))
            self.apply_gradients(alpha)
            old_cost = cost
            cost = self.compute_cost()
            self.receiver_history.append(np.array(self.candidate_receivers))
            self.emitter_history.append(np.array(self.candidate_emitters))
            iter = iter+1
            self.compute_gradient()
            self.apply_constraints()

            if max_iterations is None and np.abs((cost - old_cost)/ old_cost) < 1E-4:
                break

        print("terminated.")
        print("iteration count = {}{}".format(iter, (" (limiting)" if iter == max_iterations else "")))
        print("cost = {}".format(self.compute_cost()))
        print("gradient magnitude is {}{}".format(self.gradient_magnitude(), (" (limiting)" if self.gradient_magnitude()<=gradient_tolerance else "")))

    def gradient_magnitude(self):
        emitter_ss = np.sum(self.emitter_gradients * self.emitter_gradients)
        receiver_ss = np.sum(self.receiver_gradients * self.receiver_gradients)
        return np.sqrt(emitter_ss + receiver_ss )

    def apply_gradients(self, alpha=1):
        self.candidate_receivers -= self.receiver_gradients * alpha
        self.candidate_emitters -= self.emitter_gradients * alpha

    def apply_constraints(self):
        self.candidate_receivers, \
        self.candidate_emitters, \
        self.receiver_gradients, \
        self.emitter_gradients = global_constraints(self.candidate_receivers, 
                                                    self.candidate_emitters, 
                                                    receiver_gradients=self.receiver_gradients, 
                                                    emitter_gradients=self.emitter_gradients)
        

    def compute_cost(self):

        J = 0

        for e, emitter in enumerate(self.candidate_emitters.T):
            for rA in range(self.n_receivers):
                for rB in range(rA+1, self.n_receivers):
                    receiverA = self.candidate_receivers[:,rA]
                    receiverB = self.candidate_receivers[:,rB]

                    u = (np.linalg.norm(receiverB-emitter) - np.linalg.norm(receiverA-emitter)) - self.delta_times_r[rA, rB, e] * self.speed_of_sound
                    J += 0.5*u*u

        for r, receiver in enumerate(self.candidate_receivers.T):
            for eA,eB in self.emitter_pairs:
                emitterA = self.candidate_emitters[:,eA]
                emitterB = self.candidate_emitters[:,eB]
                u = (np.linalg.norm(emitterB-receiver) - np.linalg.norm(emitterA-receiver)) - self.delta_times_e[eA, eB, r] * self.speed_of_sound
                J += 0.5*u*u

        return J

    def compute_gradient(self):

        self.receiver_gradients.fill(0)
        self.emitter_gradients.fill(0)

        for e, emitter in enumerate(self.candidate_emitters.T):
            for rA in range(self.n_receivers):
                for rB in range(rA+1, self.n_receivers):
                    receiverA = self.candidate_receivers[:,rA]
                    receiverB = self.candidate_receivers[:,rB]

                    deltaA = receiverA-emitter
                    deltaB = receiverB-emitter
                    magA = np.linalg.norm(deltaA)
                    magB = np.linalg.norm(deltaB)
                    
                    u = magB - magA - self.delta_times_r[rA, rB, e] * self.speed_of_sound

                    du_dpB = 0.5/magB *2*deltaB
                    du_dpA = 0.5/magA *2*deltaA * -1
                    du_de  = -1 * (du_dpB + du_dpA) # emitter

                    dJ_dpA = u* du_dpA
                    dJ_dpB = u* du_dpB
                    dJ_de = u* du_de

                    self.receiver_gradients[:,rA] += dJ_dpA
                    self.receiver_gradients[:,rB] += dJ_dpB
                    self.emitter_gradients[:, e] += dJ_de
                    

        for r, receiver in enumerate(self.candidate_receivers.T):
            for eA,eB in self.emitter_pairs:
                emitterA = self.candidate_emitters[:,eA]
                emitterB = self.candidate_emitters[:,eB]

                deltaA = emitterA-receiver
                deltaB = emitterB-receiver
                magA = np.linalg.norm(deltaA)
                magB = np.linalg.norm(deltaB)
                
                u = magB - magA - self.delta_times_e[eA, eB, r] * self.speed_of_sound

                du_dpB = 0.5/magB *2*deltaB
                du_dpA = 0.5/magA *2*deltaA * -1
                du_de  = -1 * (du_dpB + du_dpA) # receiver

                dJ_dpA = u* du_dpA
                dJ_dpB = u* du_dpB
                dJ_de = u* du_de

                self.emitter_gradients[:,eA] += dJ_dpA
                self.emitter_gradients[:,eB] += dJ_dpB
                self.receiver_gradients[:, r] += dJ_de
                pass
                    # J += 0.5*u*u
        # print(self.receiver_gradients)
        # print(self.emitter_gradients)
        pass

def plot(receivers, emitters, color, receiver_marker, emitter_marker):

    plt.scatter(*receivers, edgecolors=color, marker=receiver_marker, facecolor='none')
    plt.scatter(*emitters, edgecolors=color, marker=emitter_marker, facecolor=color)

    for i, receiver in enumerate(receivers.T):
        plt.annotate("r{}".format(i), receiver)

    for i, emitter in enumerate(emitters.T):
        plt.annotate("e{}".format(i), emitter)

def plot_hyperbolas(delta_times_r, receivers, emitters, hyperbola_colors='--g', label_string='r{}-r{},e{}', speed_of_sound=1):
    # plt.autoscale(False)

    n_receivers = receivers.shape[1]
    n_emitters = emitters.shape[1]

    min_x = +10000000
    max_x = -10000000
    min_y = +10000000
    max_y = -10000000

    for e, emitter in enumerate(emitters.T):
        for rA in range(n_receivers):
            receiverA = receivers[:,rA]
            min_x = min(min_x, receiverA[0])
            max_x = max(max_x, receiverA[0])
            min_y = min(min_y, receiverA[1])
            max_y = max(max_y, receiverA[1])
            min_x = min(min_x, emitter[0])
            max_x = max(max_x, emitter[0])
            min_y = min(min_y, emitter[1])
            max_y = max(max_y, emitter[1])

    for e, emitter in enumerate(emitters.T):
        for rA in range(n_receivers):
            for rB in range(rA+1, n_receivers):
                receiverA = receivers[:,rA]
                receiverB = receivers[:,rB]
                deltaAB = receiverB - receiverA
                a = 0.5*delta_times_r[rA, rB, e] * speed_of_sound
                c =  0.5*np.linalg.norm(deltaAB)
                aa = a*a
                bb = c*c - aa

                lim = np.sqrt(aa)
                x = np.linspace(lim,2,10000)
                if a > 0:
                    x *= -1
                inner = (x*x) * bb/aa - bb
                inner[inner < 0] = 0
                y_positive = +np.sqrt( inner)
                y_negative = -1 * y_positive
                # mask = np.isnan(y_positive)
                # x = x[not mask]
                # y_positive = y_positive[not mask]
                # y_negative = y_negative[not mask]

                path = np.concatenate([ np.stack([np.flip(x), np.flip(y_positive)]), np.stack([x, y_negative])], axis=-1)
                # path = np.concatenate([ np.stack([x, y_negative])], axis=-1)

                DCM = np.zeros([2,2])
                DCM[:,0] = deltaAB / np.linalg.norm(deltaAB)
                DCM[:,1] = np.flip(DCM[:,0])
                DCM[0,1] *= -1
                if np.linalg.det(DCM) < 0:
                    raise Exception("det(dcm) < 0")


                path = DCM @ path

                path = path + 0.5*(receiverB+receiverA)[:,np.newaxis]
                annotation_point = path[:,x.size]

                path[0,path[0,:]<min_x] = np.NaN 
                path[0,path[0,:]>max_x] = np.NaN 
                path[1,path[1,:]<min_y] = np.NaN 
                path[1,path[1,:]>max_y] = np.NaN 
                mask = np.isnan(path).any(axis=0)
                path = path[:,~mask]

                if path.size > 0:
                    plt.annotate(label_string.format(rA,rB,e), annotation_point, alpha=0.2)
                    plt.plot(*path, hyperbola_colors, alpha=0.2)
    # plt.autoscale(True)


def test_math(n_receivers = 4, n_emitters=6, speed_of_sound = 1):

    np.random.seed(9001)

    receivers = np.random.rand(2, n_receivers)
    emitters = np.random.rand(2, n_emitters)
    # emitters[0,0] = 0.6 # ****************
    # emitters[1,0] = 0.2 # ****************

    emitter_pairs = [(i*2, i*2+1) for i in range(n_emitters//2)]

    
    # apply constraints
    receivers, emitters, _, _ = global_constraints(receivers, emitters, verbose=True)

    delta_times_r = np.zeros([n_receivers, n_receivers, n_emitters])
    delta_times_e = np.zeros([n_emitters, n_emitters, n_receivers])

    for e, emitter in enumerate(emitters.T):
        for rA in range(n_receivers):
            for rB in range(rA+1, n_receivers):
                receiverA = receivers[:,rA]
                receiverB = receivers[:,rB]
                delta_times_r[rA, rB, e] = (np.linalg.norm(receiverB-emitter) - np.linalg.norm(receiverA-emitter)) / speed_of_sound

    for r, receiver in enumerate(receivers.T):
        for eA,eB in emitter_pairs:
            emitterA = emitters[:,eA]
            emitterB = emitters[:,eB]
            delta_times_e[eA, eB, r] = (np.linalg.norm(emitterB-receiver) - np.linalg.norm(emitterA-receiver)) / speed_of_sound


    system = System()
    system.initialize(initial_receivers=receivers+0.1, initial_emitters=emitters+0.1, n_receivers=n_receivers, n_emitters=n_emitters, speed_of_sound=speed_of_sound)
    system.calibrate(delta_times_r, delta_times_e, emitter_pairs, max_iterations = 500E3, gradient_tolerance = 1E-8, alpha = 0.5E-1 )

    residuals = np.concatenate([receivers, emitters], axis=-1) - np.concatenate([system.candidate_receivers, system.candidate_emitters], axis=-1)
    residuals_mag = np.sqrt(np.sum(residuals*residuals))
    if residuals_mag < 1E-4:
        print("SUCCESS")
    else:
        print("failed")

    plot_hyperbolas(delta_times_e, emitters, receivers, '--k', label_string='e{}-e{},r{}', speed_of_sound=speed_of_sound)
    plot_hyperbolas(delta_times_r, receivers, emitters, '--g', speed_of_sound=speed_of_sound)
    plot(system.candidate_receivers, system.candidate_emitters, 'blue', receiver_marker='.', emitter_marker='*')
    plot(receivers, emitters, 'red', receiver_marker='o', emitter_marker='x')

    for r in range(n_receivers):
        plt.plot(*(np.array([receiver[:,r] for receiver in system.receiver_history]).T), linestyle=':', color='y')
    for e in range(n_emitters):
        plt.plot(*(np.array([emitter[:,e] for emitter in system.emitter_history]).T), linestyle=':', color='y')


    plt.figure()
    plot_hyperbolas(delta_times_e, system.candidate_emitters, system.candidate_receivers, '--k', label_string='e{}-e{},r{}', speed_of_sound=speed_of_sound)
    plot_hyperbolas(delta_times_r, system.candidate_receivers, system.candidate_emitters, '--y', speed_of_sound=speed_of_sound)
    plot(system.candidate_receivers, system.candidate_emitters, 'blue', receiver_marker='.', emitter_marker='*')
    plot(receivers, emitters, 'red', receiver_marker='o', emitter_marker='x')

    plt.show()


if __name__ == "__main__":
    test_math()