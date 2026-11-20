import numpy as np
import matplotlib.pyplot as plt

c = 343.0  # speed of sound (m/s)

class OnlineEDMCalibrator:
    def __init__(self, num_mics, dim=3):
        self.M = num_mics
        self.dim = dim
        # Gram-like matrix to accumulate geometry
        self.C = np.zeros((self.M, self.M))
        # For simulation
        self.true_mics = np.random.randn(self.M, self.dim)

    def simulate_tdoa(self, emitter_pos):
        """Simulate TDOA measurements for one emitter position."""
        dists = np.linalg.norm(self.true_mics - emitter_pos, axis=1)
        return (dists - dists[0]) / c  # relative to mic 0

    def add_frame(self, tdoa):
        """Add a single TDOA frame (online update)."""
        delta = c * tdoa
        delta[0] = 0  # reference mic
        # rank-1 update accumulates geometry
        self.C += np.outer(delta, delta)

    def add_batch(self, tdoa_batch):
        """Add multiple TDOA frames (batch update)."""
        for tdoa in tdoa_batch:
            self.add_frame(tdoa)

    def solve(self):
        """Compute microphone coordinates from accumulated EDM."""
        w, v = np.linalg.eigh(self.C)
        idx = np.argsort(w)[::-1]  # descending
        w = w[idx]
        v = v[:, idx]
        X = v[:, :self.dim] * np.sqrt(w[:self.dim])
        return X

# -------------------- Demo --------------------
def run_demo():
    num_mics = 6
    num_frames = 1000

    calibrator = OnlineEDMCalibrator(num_mics)

    # Simulated wand trajectory
    traj = np.stack([
        np.sin(np.linspace(0, 12, num_frames)),
        np.cos(np.linspace(0, 12, num_frames)),
        np.linspace(-2, 2, num_frames)
    ], axis=1)

    # Generate TDOA frames and accumulate
    tdoa_batch = np.array([calibrator.simulate_tdoa(p) for p in traj])
    calibrator.add_batch(tdoa_batch)

    # Solve for mic coordinates
    est_mics = calibrator.solve()
    est_mics *= -0.1
    true_mics = calibrator.true_mics

    # Plot results
    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')
    ax.scatter(true_mics[:,0], true_mics[:,1], true_mics[:,2], c='r', label='True mics')
    ax.scatter(est_mics[:,0], est_mics[:,1], est_mics[:,2], c='b', marker='x', label='Estimated mics')
    ax.set_title("EDM-Based TDOA Calibration")
    ax.set_xlabel('x')
    ax.set_ylabel('y')
    ax.legend()
    plt.show()

run_demo()