import numpy as np

from scipy.optimize import least_squares

c = 343.0  # speed of sound


def generate_scene(num_mics=6, num_sources=10, dims=3):

    mics = np.random.randn(num_mics, dims)*2
    sources = np.random.randn(num_sources, dims)*3

    mics, sources = canonicalize(mics, sources, dims)

    return mics, sources


def simulate_tdoa(mics, sources):

    dist = np.linalg.norm(
        mics[:,None,:] - sources[None,:,:],
        axis=2
    )

    times = dist / c

    return times


def build_dds_matrix(times):

    M,N = times.shape

    d = c*(times - times[0])

    DDS = d**2

    return DDS, d


def svd_factorization(DDS, dim=3):

    r = dim + 2

    U,S,Vt = np.linalg.svd(DDS)

    U_r = U[:,:r]
    S_r = np.diag(np.sqrt(S[:r]))
    V_r = Vt[:r,:]

    A = U_r @ S_r
    B = S_r @ V_r

    return A,B


def extract_coordinates(A, B, dim=3):
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

    return mics_metric, sources_metric, H


def canonicalize(mics, sources, dims):

    dims = mics.shape[-1]

    # translate so mic1 is origin
    t = mics[0]
    mics = mics - t
    sources = sources - t

    # rotate so mic2 lies on x axis
    x_axis = mics[1] / np.linalg.norm(mics[1])

    if dims == 3:
        z_temp = np.cross(x_axis, mics[2])
        z_axis = z_temp / np.linalg.norm(z_temp)

        y_axis = np.cross(z_axis, x_axis)

        R = np.vstack([x_axis,y_axis,z_axis]).T
    elif dims == 2:
        R = np.vstack([x_axis, np.array([-x_axis[1], x_axis[0]])]).T
        

    mics = mics @ R
    sources = sources @ R

    return mics, sources


def tdoa_self_calibration(times, dims=3):

    DDS,d = build_dds_matrix(times)

    A,B = svd_factorization(DDS, dim=dims)

    mics_est, sources_est,H = extract_coordinates(A,B, dim=dims)

    mics_est, sources_est = canonicalize(mics_est, sources_est, dims=dims)

    return mics_est, sources_est


def refine_positions(mics_init, sources_init, d_ij, dims=3):

    M = mics_init.shape[0]
    N = sources_init.shape[0]
    
    # Flatten all unknowns into a vector
    x0 = np.hstack([mics_init.ravel(), sources_init.ravel()])
    
    def residuals(x):
        mics = x[:dims*M].reshape(M,dims)
        sources = x[dims*M:].reshape(N,dims)
        res = []
        for i in range(1,M):
            for j in range(N):
                dij = d_ij[i,j]
                r_ij = np.linalg.norm(mics[i]-sources[j])
                r_1j = np.linalg.norm(mics[0]-sources[j])
                res.append(r_ij - r_1j - dij)
        # res.append(mics[0,0]**2)
        # res.append(mics[0,1]**2)
        # res.append(mics[0,2]**2)
        # res.append(mics[1,1]**2)
        # res.append(mics[1,2]**2)
        # res.append(mics[2,2]**2)

        # apply canonicalization
        if dims >= 2:
            res.append(mics[0,0]**2)
            res.append(mics[0,1]**2)
            res.append(mics[1,1]**2)
        if dims >= 3:
            res.append(mics[0,2]**2)
            res.append(mics[1,2]**2)
            res.append(mics[2,2]**2)
        return res
    
    sol = least_squares(residuals, x0, method='lm')
    
    mics_refined = sol.x[:dims*M].reshape(M,dims)
    sources_refined = sol.x[dims*M:].reshape(N,dims)
    
    return mics_refined, sources_refined


if __name__ == "__main__":
    np.random.seed(0)

    dims = 3
    true_mics, true_sources = generate_scene(num_mics = 6, 
                                             num_sources = 7, 
                                             dims=dims)

    times = simulate_tdoa(true_mics, true_sources)

    mics_est, sources_est = tdoa_self_calibration(times, dims=dims)


    print("\nTrue microphone positions:\n",true_mics)
    print("\nEstimated microphone positions:\n",mics_est)

    print("\nTrue source positions:\n",true_sources[:5])
    print("\nEstimated source positions:\n",sources_est[:5])

    # Compute distance differences
    d_ij = c*(times - times[0])

    # Refine after SVD estimate
    mics_refined, sources_refined = refine_positions(mics_est, sources_est, d_ij, dims=dims)

    print('\nmic residuals (svd)')
    print(true_mics - mics_est)
    print('\nmic residuals (svd + lm)')
    print(true_mics - mics_refined)
