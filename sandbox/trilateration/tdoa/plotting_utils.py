


import matplotlib.pyplot as plt
import numpy as np
from matplotlib import cm # Colormap utilities
from matplotlib.ticker import LinearLocator # Customizing axes

from matplotlib import cm
from matplotlib.colors import Normalize
import warnings

def plot_points(ax, receivers, emitters, color, receiver_marker, emitter_marker, legend_label=None):

    n_dims = receivers.shape[0]

    with warnings.catch_warnings():
        # supress "You passed a edgecolor/edgecolors ('red') for an unfilled marker ('x')"
        warnings.simplefilter("ignore", category=UserWarning) 

        ax.scatter(*receivers, edgecolors=color, marker=receiver_marker, facecolor='none', label=f'receiver {legend_label}')
        ax.scatter(*emitters, edgecolors=color, marker=emitter_marker, facecolor=color, label=f'emitter {legend_label}')

    if n_dims == 2:
        for i, receiver in enumerate(receivers.T):
            ax.annotate("r{}".format(i), receiver)

        for i, emitter in enumerate(emitters.T):
            ax.annotate("e{}".format(i), emitter)

def plot_half_hyperbolas_2d(ax, delta_times_r, receivers, emitters, hyperbola_colors='--g', label_string='r{}-r{},e{}', speed_of_sound=1, legend_label=None):
    # plt.autoscale(False)

    n_receivers = receivers.shape[1]
    n_emitters = emitters.shape[1]

    min_x = receivers[0, 0] # 
    max_x = receivers[0, 0] # 
    min_y = receivers[0, 1] # 
    max_y = receivers[0, 1] # 

    label_applied = False# only apply the label once

    for receiver in receivers.T:
        min_x = min(min_x, receiver[0])
        max_x = max(max_x, receiver[0])
        min_y = min(min_y, receiver[1])
        max_y = max(max_y, receiver[1])

    for emitter in emitters.T:
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

                if a == 0:
                    continue

                lim = np.sqrt(aa)
                x = np.linspace(lim,20,10000)
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
                if not np.isclose(np.linalg.det(DCM), 1.0):
                    raise Exception(f"det(dcm) = {np.linalg.det(DCM)}")


                path = DCM @ path

                path = path + 0.5*(receiverB+receiverA)[:,np.newaxis]
                annotation_point = path[:,x.size]

                path[0,path[0,:]<min_x] = np.nan 
                path[0,path[0,:]>max_x] = np.nan 
                path[1,path[1,:]<min_y] = np.nan 
                path[1,path[1,:]>max_y] = np.nan 
                mask = np.isnan(path).any(axis=0)
                path = path[:,~mask]

                if path.size > 0:
                    ax.annotate(label_string.format(rA,rB,e), annotation_point, alpha=0.2)
                    ax.plot(*path, hyperbola_colors, alpha=0.2, label=(legend_label if not label_applied else None))
                    label_applied = True
    # plt.autoscale(True)






def create_non_collinear_3d(v):
    # Define a standard non-collinear vector, e.g., a basic axis vector
    # We choose one that is unlikely to be collinear with v itself
    if not np.allclose(v, [1, 0, 0]):
        arbitrary_vector = np.array([1, 0, 0])
    elif not np.allclose(v, [0, 1, 0]):
        arbitrary_vector = np.array([0, 1, 0])
    else:
        arbitrary_vector = np.array([0, 0, 1])

    # The cross product results in a vector orthogonal (non-collinear) to both inputs
    return np.cross(v, arbitrary_vector)

def dist_point_to_line(query, line_origin, line_unit):
    delta = line_origin - query
    return np.linalg.norm(delta - delta.dot(line_unit)*line_unit/np.linalg.norm(line_unit) )

def plot_half_hyperbolas_3d(ax, delta_times_r, receivers, emitters, hyperbola_colors='--g', label_string='r{}-r{},e{}', speed_of_sound=1, legend_label=None):

    n_receivers = receivers.shape[1]
    n_emitters = emitters.shape[1]

    for e, emitter in enumerate(emitters.T):
        for rA in range(n_receivers):
            for rB in range(rA+1, n_receivers):
                receiverA = receivers[:,rA]
                receiverB = receivers[:,rB]
                deltaAB = receiverB - receiverA
                
                receiver_axis = deltaAB / np.linalg.norm(deltaAB)
                radial_thresh = dist_point_to_line(emitter, receiverA, receiver_axis)

                a = 0.5*delta_times_r[rA, rB, e] * speed_of_sound
                c =  0.5*np.linalg.norm(deltaAB)
                aa = a*a
                bb = c*c - aa

                # x is the along axis direction
                # y and z are along the axis direction
                y = np.linspace(-radial_thresh, +radial_thresh, 100)
                z = np.linspace(-radial_thresh, +radial_thresh, 100)
                y_meshgrid, z_meshgrid = np.meshgrid(y, z)
                r_meshgrid_squared = y_meshgrid*y_meshgrid + z_meshgrid*z_meshgrid
                r_meshgrid = np.sqrt(r_meshgrid_squared)

                # xx/aa - yy/bb = 1
                xx = aa + r_meshgrid_squared*aa/bb
                xx[xx < 0] = 0
                x_meshgrid = np.sqrt(xx)
                if a > 0:
                    x_meshgrid *= -1

                # form the DCM
                axis0 = receiver_axis # already normalized
                axis1bad = create_non_collinear_3d(axis0) # anything that's different than axis0
                axis2 = np.cross(axis0, axis1bad)
                axis1 = np.cross(axis2, axis0) # now orthogonal to the other 2

                DCM = np.zeros([3,3])
                DCM[:,0] = axis0 / np.linalg.norm(axis0)
                DCM[:,1] = axis1 / np.linalg.norm(axis1)
                DCM[:,2] = axis2 / np.linalg.norm(axis2)
                if not np.isclose(np.linalg.det(DCM), 1.0):
                    raise Exception(f"det(dcm) = {np.linalg.det(DCM)}")
                
                # cull the data and apply the dcm
                y_meshgrid[r_meshgrid_squared > radial_thresh*radial_thresh*1.1] = np.nan
                z_meshgrid[r_meshgrid_squared > radial_thresh*radial_thresh*1.1] = np.nan
                
                orig_shape = x_meshgrid.shape
                origin = 0.5*(receiverB+receiverA)[:,np.newaxis]
                xyz_meshgrid = np.stack([x_meshgrid.ravel(), y_meshgrid.ravel(), z_meshgrid.ravel()])
                x_meshgrid = (DCM @ xyz_meshgrid + origin)[0].reshape(orig_shape)
                y_meshgrid = (DCM @ xyz_meshgrid + origin)[1].reshape(orig_shape)
                z_meshgrid = (DCM @ xyz_meshgrid + origin)[2].reshape(orig_shape)


                norm = Normalize(vmin=r_meshgrid.min(), vmax=r_meshgrid.max())
                m = cm.ScalarMappable(norm=norm, cmap=cm.viridis) # Use the viridis colormap as an example
                m.set_array(r_meshgrid)
                surf = ax.plot_surface(x_meshgrid, y_meshgrid, z_meshgrid, 
                                    facecolors=m.to_rgba(r_meshgrid), 
                                    cmap=cm.coolwarm,
                                    linewidth=0,
                                    antialiased=False,
                                    rstride=20, cstride=20,
                                    alpha=0.1)


                xs = [receiverA[0], receiverB[0]]
                ys = [receiverA[1], receiverB[1]]
                zs = [receiverA[2], receiverB[2]]
                labels = ['rA', 'rB']
                if emitter is not None:
                    xs.append(emitter[0])
                    ys.append(emitter[1])
                    zs.append(emitter[2])
                    labels.append('e0')
                        
                ax.scatter(xs, ys, zs, marker='o')
                for x, y, z, label in zip(xs, ys, zs, labels):
                    ax.text(x, y, z, label) # The ax.text function places text at the specified 3D coordinates

                # Set labels for the axes
                ax.set_xlabel('X Label')
                ax.set_ylabel('Y Label')
                ax.set_zlabel('Z Label')


    # annotation_point = path[:,x.size]

def plot_half_hyperbolas(ax, delta_times_r, receivers, emitters, **kwargs):
    n_dims = receivers.shape[0]
    if n_dims == 2:
        plot_half_hyperbolas_2d(ax, delta_times_r, receivers, emitters, **kwargs)
    elif n_dims == 3:
        pass
        # plot_half_hyperbolas_3d(ax, delta_times_r, receivers, emitters, **kwargs)
    else:
        raise Exception(f'cannot plot {n_dims} dimensions')

def fullscreen_save_and_close(save_name):

    manager = plt.get_current_fig_manager()
    manager.full_screen_toggle() 
    plt.savefig(save_name)
    # manager.full_screen_toggle() 


if __name__ == "__main__":
    receiverA = np.array([0,1,1])
    receiverB = np.array([1,0,0])
    receivers = np.stack([receiverA, receiverB] ,axis=1)

    emitter = np.array([0,2,0])
    emitters = np.stack([emitter], axis=1)

    import tdoa.tdoa_algos as tdoa
    delta_times_r, delta_times_e = tdoa.delta_times_from_points(receivers,
                                                                emitters,
                                                                emitter_pairs = None,
                                                                speed_of_sound=1.0)
    
    fig, ax = plt.subplots(subplot_kw={"projection": "3d"})
    plot_half_hyperbolas(ax, 
                         delta_times_r, 
                         receivers, 
                         emitters, 
                         hyperbola_colors='--g', 
                         label_string='r{}-r{},e{}', 
                         speed_of_sound=1, 
                         legend_label=None)
    

# 5. Display the plot
plt.show()