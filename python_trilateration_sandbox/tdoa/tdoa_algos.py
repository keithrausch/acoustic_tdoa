import numpy as np
from scipy.optimize import least_squares

def delta_times_from_points(receivers, emitters, emitter_pairs=None, speed_of_sound=1.0):

    n_receivers = receivers.shape[1]
    n_emitters = emitters.shape[1]

    delta_times_r = np.zeros([n_receivers, n_receivers, n_emitters])
    delta_times_e = np.zeros([n_emitters, n_emitters, n_receivers])

    for e, emitter in enumerate(emitters.T):
        for rA in range(n_receivers):
            for rB in range(rA+1, n_receivers):
                receiverA = receivers[:,rA]
                receiverB = receivers[:,rB]
                delta_times_r[rA, rB, e] = (np.linalg.norm(receiverB-emitter) - np.linalg.norm(receiverA-emitter)) / speed_of_sound
                delta_times_r[rB, rA, e] = -delta_times_r[rA, rB, e]

    if emitter_pairs is not None:
        for r, receiver in enumerate(receivers.T):
            for eA,eB in emitter_pairs:
                emitterA = emitters[:,eA]
                emitterB = emitters[:,eB]
                delta_times_e[eA, eB, r] = (np.linalg.norm(emitterB-receiver) - np.linalg.norm(emitterA-receiver)) / speed_of_sound
                delta_times_e[eB, eA, r] = -delta_times_e[eA, eB, r]

    return delta_times_r, delta_times_e


def global_constraints(receivers, emitters, receiver_gradients=None, emitter_gradients=None, verbose=False):

    n_dimensions = receivers.shape[0]

    if receiver_gradients is None:
        receiver_gradients = np.zeros_like(receivers) * np.nan

    if emitter_gradients is None:
        emitter_gradients = np.zeros_like(emitters) * np.nan
    
    # first receiver is fixed to the origin
    receivers[:,0] = 0
    receiver_gradients[:,0] = 0
    if verbose:
        print('second receiver is locked to the origin (x,y,z=0)')

    # second receiver is fixed to the x axis (y=0, z=0)
    receivers[1:,1] = 0
    receiver_gradients[1:,1] = 0 
    if verbose:
        print('second receiver is locked to the x axis (y,z=0')

    # if we are 3D, then fix the third vector to the plane formed by the first two receivers
    if n_dimensions == 3: 
        if verbose:
            print('third receiver is locked to the z plane (z=0)')
        receivers[2:,2] = 0
        receiver_gradients[2:,2] = 0 

    if verbose:
        is_nan = np.isnan(np.concatenate([receiver_gradients, emitter_gradients], axis=1))
        n_free = np.sum(is_nan)
        n_fixed = np.sum(~is_nan)
        n_total = np.size(is_nan)
        n_receivers = receivers.shape[-1]
        n_emitters = emitters.shape[-1]
        n_pairwise = int(n_receivers*(n_receivers-1)*0.5)
        n_constraints = n_emitters * n_pairwise
        print("n_total={}, n_free={}, n_fixed={}, n_constraints={}".format(n_total, n_free, n_fixed, n_constraints))

    return receivers, emitters, receiver_gradients, emitter_gradients

class TDOASystemSVD:
    def __init__(self):
        pass

    def initialize(self, n_receivers=None, n_emitters=None, n_dimensions=2, speed_of_sound=1, **kwargs):
        self.solved_receivers = np.zeros([n_dimensions, n_receivers])
        self.solved_emitters = np.zeros([n_dimensions, n_emitters])

        self.n_receivers = n_receivers
        self.n_emitters = n_emitters
        self.n_dimensions = n_dimensions
        self.speed_of_sound = speed_of_sound

    @staticmethod
    def _check_for_reflections(receivers, emitters, dims):

        # check reflections
        if receivers[0,1] < 0:
            print('reflected in X. second mic must have positive x coord')
            receivers[0,:] *= -1
            emitters[0,:] *= -1

        if receivers[1,2] < 0:
            print('reflected in Y. third mic must have positive y coord')
            receivers[1,:] *= -1
            emitters[1,:] *= -1

        return receivers, emitters

    @staticmethod
    def _build_dds_matrix_from_absolute_times(abs_times_re, speed_of_sound):
        delta_times_i0_per_e = abs_times_re - abs_times_re[0]
        delta_dist_i0_per_e = speed_of_sound * delta_times_i0_per_e
        DDS = delta_dist_i0_per_e * delta_dist_i0_per_e
        return DDS, delta_dist_i0_per_e
    
    @staticmethod
    def _build_dds_matrix_from_delta_times(delta_times_i0_per_e, speed_of_sound):
        delta_dist_i0_per_e = speed_of_sound * delta_times_i0_per_e
        DDS = delta_dist_i0_per_e * delta_dist_i0_per_e
        return DDS, delta_dist_i0_per_e

    @staticmethod
    def _svd_factorization(DDS, dims):
        r = dims + 2

        U,S,Vt = np.linalg.svd(DDS)

        U_r = U[:,:r]
        S_r = np.diag(np.sqrt(S[:r]))
        V_r = Vt[:r,:]

        A = U_r @ S_r
        B = S_r @ V_r

        return A,B

    @staticmethod
    def _extract_coordinates(A, B, dim=3):
        """
        Extract true Euclidean mic and source coordinates from SVD factors.
        Works in both 2D and 3D, pure NumPy version (no SciPy).
        
        Parameters
        ----------
        A : np.ndarray
            Left SVD factor (M x (dim+2))
        B : np.ndarray
            Right SVD factor ((dim+2) x N)
        dim : int
            Spatial dimension (2 or 3)
        
        Returns
        -------
        mics_metric : np.ndarray
            Microphone coordinates (M x dim)
        sources_metric : np.ndarray
            Source coordinates (N x dim)
        H : np.ndarray
            Metric upgrade matrix ((dim+2) x (dim+2))
        """

        M, r = A.shape
        N = B.shape[1]

        # --- Step 1: Initial embedding ---
        # Take first 'dim' columns as initial coordinates
        X_init = A[:, 1:dim+1]       # mic embedding
        Y_init = B[1:dim+1, :].T     # source embedding

        # --- Step 2: Procrustes orthonormalization using SVD ---
        # Align X_init to a Euclidean frame
        # Compute covariance
        C = X_init.T @ X_init
        U, _, Vt = np.linalg.svd(C)
        R = U @ Vt   # rotation matrix

        # Apply rotation
        X_metric = X_init @ R
        Y_metric = Y_init @ R

        # --- Step 3: Construct full metric upgrade H ---
        H = np.eye(r)
        H[1:dim+1, 1:dim+1] = R

        # Apply metric upgrade
        A_metric = A @ H
        B_metric = np.linalg.inv(H) @ B

        # --- Step 4: Extract first 'dim' coordinates ---
        mics_metric = A_metric[:, 1:dim+1]
        sources_metric = B_metric[1:dim+1, :].T

        return mics_metric.T, sources_metric.T, H

    @staticmethod
    def canonicalize(mics, sources, dims):

        # translate so mic1 is origin
        mic0 = mics[:,0, np.newaxis]
        mics = mics - mic0
        sources = sources - mic0

        # rotate so mic2 lies on x axis
        x_axis = mics[:,1] / np.linalg.norm(mics[:,1])

        if dims == 3:
            z_temp = np.cross(x_axis, mics[:,2])
            z_axis = z_temp / np.linalg.norm(z_temp)

            y_axis = np.cross(z_axis, x_axis)

            R = np.vstack([x_axis,y_axis,z_axis])
        elif dims == 2:
            R = np.vstack([x_axis, np.array([-x_axis[1], x_axis[0]])])
            

        mics = R @ mics
        sources = R @ sources

        return mics, sources

    @staticmethod
    def _refine_positions(mics_init, sources_init, d_ij, dims):

        nMics = mics_init.shape[-1]
        nSpeakers = sources_init.shape[-1]
        
        # Flatten all unknowns into a vector
        x0 = np.hstack([mics_init.ravel(), sources_init.ravel()])
        
        def residuals(x):
            mics = x[:dims * nMics].reshape(dims, nMics)
            sources = x[dims * nMics:].reshape(dims, nSpeakers)
            res = []
            for i in range(1, nMics):
                for j in range(nSpeakers):
                    dij = d_ij[i,j]
                    r_ij = np.linalg.norm(mics[:,i]-sources[:,j])
                    r_1j = np.linalg.norm(mics[:,0]-sources[:,j])
                    res.append(r_ij - r_1j - dij)

            # apply canonicalization
            if dims >= 2:
                res.append(mics[0,0]**2)
                res.append(mics[1,0]**2)
                res.append(mics[1,1]**2)
            if dims >= 3:
                res.append(mics[2,0]**2)
                res.append(mics[2,1]**2)
                res.append(mics[2,2]**2)
            return res
        
        sol = least_squares(residuals, x0, method='lm')
        
        mics_refined = sol.x[:dims * nMics].reshape(dims, nMics)
        sources_refined = sol.x[dims * nMics:].reshape(dims, nSpeakers)
        
        return mics_refined, sources_refined

    def calibrate(self, delta_times_r, delta_times_e, emitter_pairs, **kwargs):

        DDS, delta_dist_i0_per_e = self._build_dds_matrix_from_delta_times(delta_times_r[0,:,:], 
                                                                           self.speed_of_sound)

        A,B = self._svd_factorization(DDS, dims = self.n_dimensions)

        mics_est, sources_est, H = self._extract_coordinates(A,B, 
                                                             dim = self.n_dimensions)

        mics_est, sources_est = self.canonicalize(mics_est, 
                                                  sources_est, 
                                                  dims = self.n_dimensions)
        
        mics_reflected, sources_reflected = self._check_for_reflections(mics_est, 
                                                            sources_est, 
                                                            dims=self.n_dimensions)
        mics_reflected = mics_reflected.T
        sources_reflected = sources_reflected.T

        # refine with LM
        mics_refined, sources_refined = self._refine_positions(mics_reflected.T,
                                                               sources_reflected.T, 
                                                               delta_dist_i0_per_e,
                                                               dims = self.n_dimensions)
        
        mics_refined, sources_refined = self._check_for_reflections(mics_refined, 
                                                            sources_refined, 
                                                            dims=self.n_dimensions)
        
        self.candidate_receivers = mics_refined
        self.candidate_emitters = sources_refined
        self.receiver_history = []
        self.emitter_history = []
# 
# class TDOASystem:
#     def __init__(self):
#         pass
# 
#     def initialize(self, initial_receivers=None, initial_emitters=None, n_receivers=None, n_emitters=None, n_dimensions=2, speed_of_sound=1):
#         self.candidate_receivers = np.random.rand(n_dimensions, n_receivers) if initial_receivers is None else initial_receivers
#         self.candidate_emitters = np.random.rand(n_dimensions, n_emitters) if initial_emitters is None else initial_emitters
# 
#         self.n_receivers = self.candidate_receivers.shape[1]
#         self.n_emitters = self.candidate_emitters.shape[1]
#         self.n_dimensions = self.candidate_emitters.shape[0]
#         self.speed_of_sound = speed_of_sound
# 
#         self.receiver_gradients = np.zeros_like(self.candidate_receivers)
#         self.emitter_gradients = np.zeros_like(self.candidate_emitters)
# 
#         self._N = 1.0
# 
#     def calibrate(self, 
#                   delta_times_r, 
#                   delta_times_e, 
#                   emitter_pairs, 
#                   max_iterations = 15, 
#                   gradient_tolerance = 1E-4, 
#                   cost_tolerance = 1E-6,
#                   alpha = 1E-1, 
#                   print_rate=1, 
#                   iter_callback=None):
#         # assume already initialized
#         self.delta_times_r = delta_times_r
#         self.delta_times_e = delta_times_e
#         self.emitter_pairs = emitter_pairs
#         self.receiver_history = []
#         self.emitter_history = []
# 
#         # figure out the count so we can compute a mean instead of a total sum
#         # self._N = 0
#         # self._N += self.candidate_emitters.shape[1] * (self.n_receivers*(self.n_receivers-1)*0.5)
#         # self._N += self.candidate_receivers.shape[1] * len(self.emitter_pairs)
# 
#         self._apply_constraints()
# 
#         cost_0 = self.compute_cost()
#         old_cost = cost_0
#         cost = cost_0
# 
#         self._compute_gradient()
#         self._apply_constraints()
# 
#         iter = 0
#         max_iterations = np.inf if max_iterations is None else max_iterations
#         cost_tolerance = -np.inf if cost_tolerance is None else cost_tolerance
#         while not self._gradient_magnitude() <= gradient_tolerance and iter < max_iterations:
#             alpha_effective = alpha #* np.pow(0.999, iter/1000.0)
#             # if iter > 2000:
#             #     alpha_effective = 0.5
#             if iter % print_rate == 0:
#                 print(f'iteration {iter}, cost={self.compute_cost()}, gradient_mag={self._gradient_magnitude()}, alpha_effective={alpha_effective}')
#                 print('residuals:')
#                 np.set_printoptions(formatter={'float_kind':lambda x: "% .3e" % x})
#                 print(iter_callback(self.candidate_receivers, self.candidate_emitters).T)
#             self._apply_gradients(alpha_effective)
#             old_cost = cost
#             cost = self.compute_cost()
#             self.receiver_history.append(np.array(self.candidate_receivers))
#             self.emitter_history.append(np.array(self.candidate_emitters))
#             iter = iter+1
#             self._compute_gradient()
#             self._apply_constraints()
# 
#             # if np.isinf(max_iterations) and np.abs((cost - old_cost) / old_cost) < 1E-4:
#             #     break
#             if np.isinf(max_iterations) and cost < cost_tolerance:
#                 break
#             # if iter > 500 and cost > old_cost:
#             #     break 
# 
#         print("terminated.")
#         print(f"iteration count = {iter}{" (limiting)" if iter == max_iterations else ""}")
#         print(f"cost = {self.compute_cost()}{" (limiting)" if self.compute_cost()<=cost_tolerance else ""}")
#         print(f"gradient magnitude is {self._gradient_magnitude()}{" (limiting)" if self._gradient_magnitude()<=gradient_tolerance else ""}" )
# 
#     def _gradient_magnitude(self):
#         emitter_ss = np.sum(self.emitter_gradients * self.emitter_gradients)
#         receiver_ss = np.sum(self.receiver_gradients * self.receiver_gradients)
#         return np.sqrt(emitter_ss + receiver_ss )
# 
#     def _apply_gradients(self, alpha=1):
#         self.candidate_receivers -= self.receiver_gradients * alpha
#         self.candidate_emitters -= self.emitter_gradients * alpha
# 
#     def _apply_constraints(self):
#         self.candidate_receivers, \
#         self.candidate_emitters, \
#         self.receiver_gradients, \
#         self.emitter_gradients = global_constraints(self.candidate_receivers, 
#                                                     self.candidate_emitters, 
#                                                     receiver_gradients=self.receiver_gradients, 
#                                                     emitter_gradients=self.emitter_gradients)
#         
# 
#     def compute_cost(self):
# 
#         J = 0
# 
#         for e, emitter in enumerate(self.candidate_emitters.T):
#             for rA in range(self.n_receivers):
#                 for rB in range(rA+1, self.n_receivers):
#                     receiverA = self.candidate_receivers[:,rA]
#                     receiverB = self.candidate_receivers[:,rB]
# 
#                     u = (np.linalg.norm(receiverB-emitter) - np.linalg.norm(receiverA-emitter)) - self.delta_times_r[rA, rB, e] * self.speed_of_sound
#                     J += 0.5*u*u
# 
#         for r, receiver in enumerate(self.candidate_receivers.T):
#             for eA,eB in self.emitter_pairs:
#                 emitterA = self.candidate_emitters[:,eA]
#                 emitterB = self.candidate_emitters[:,eB]
#                 u = (np.linalg.norm(emitterB-receiver) - np.linalg.norm(emitterA-receiver)) - self.delta_times_e[eA, eB, r] * self.speed_of_sound
#                 J += 0.5*u*u
# 
#         return J / self._N
# 
#     def _compute_gradient(self):
# 
#         self.receiver_gradients.fill(0)
#         self.emitter_gradients.fill(0)
# 
#         for e, emitter in enumerate(self.candidate_emitters.T):
#             for rA in range(self.n_receivers):
#                 for rB in range(rA+1, self.n_receivers):
#                     receiverA = self.candidate_receivers[:,rA]
#                     receiverB = self.candidate_receivers[:,rB]
# 
#                     deltaA = receiverA-emitter
#                     deltaB = receiverB-emitter
#                     magA = np.linalg.norm(deltaA)
#                     magB = np.linalg.norm(deltaB)
#                     
#                     u = magB - magA - self.delta_times_r[rA, rB, e] * self.speed_of_sound
# 
#                     du_dpB = 0.5/magB *2*deltaB
#                     du_dpA = 0.5/magA *2*deltaA * -1
#                     du_de  = -1 * (du_dpB + du_dpA) # emitter
# 
#                     dJ_dpA = u* du_dpA
#                     dJ_dpB = u* du_dpB
#                     dJ_de = u* du_de
# 
#                     self.receiver_gradients[:,rA] += dJ_dpA
#                     self.receiver_gradients[:,rB] += dJ_dpB
#                     self.emitter_gradients[:, e] += dJ_de
#                     
# 
#         for r, receiver in enumerate(self.candidate_receivers.T):
#             for eA,eB in self.emitter_pairs:
#                 emitterA = self.candidate_emitters[:,eA]
#                 emitterB = self.candidate_emitters[:,eB]
# 
#                 deltaA = emitterA-receiver
#                 deltaB = emitterB-receiver
#                 magA = np.linalg.norm(deltaA)
#                 magB = np.linalg.norm(deltaB)
#                 
#                 u = magB - magA - self.delta_times_e[eA, eB, r] * self.speed_of_sound
# 
#                 du_dpB = 0.5/magB *2*deltaB
#                 du_dpA = 0.5/magA *2*deltaA * -1
#                 du_de  = -1 * (du_dpB + du_dpA) # receiver
# 
#                 dJ_dpA = u* du_dpA
#                 dJ_dpB = u* du_dpB
#                 dJ_de = u* du_de
# 
#                 self.emitter_gradients[:,eA] += dJ_dpA
#                 self.emitter_gradients[:,eB] += dJ_dpB
#                 self.receiver_gradients[:, r] += dJ_de
#                 pass
# 
#         
#         self.receiver_gradients /= self._N
#         self.emitter_gradients /= self._N
#         pass
