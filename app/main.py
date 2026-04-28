"""main.py

Defines a Panel dashboard for visualizing the native ParticleModel extension
"""
import os
import colorcet as cc
import holoviews as hv
import numpy as np
import pandas as pd
import panel as pn
import panel_material_ui as pmui
import param as pr
from holoviews.streams import Pipe

from PyModel import MultithreadedParticleSystem


def update_model() -> None:
    """Callback that is executed by periodic callback managed by the dashboard.
    
    Update the model by a single step using the time delta. Once updated the
    model data is packed into a dataframe and sent through the pipe.
    """
    model.update()
    particle_data = pd.DataFrame([[particle.x, particle.y, particle.m] for particle in model.particles], columns=['x','y','m'])
    extent_data = pd.DataFrame([extent for extent in model.get_extents()], columns=['x0', 'y0', 'x1', 'y1'])
    particle_pipe.send((particle_data, extent_data))
    table.value = particle_data

def visualize_model(data) -> hv.core.overlay.Overlay:
    """Callback that is executed whenever data is streamed through the pipe.

    From the model state (as sent from update_model) a scatter plot is created,
    plotting the x-position against the y-position, giving a bird's-eye view of
    the simulation.

    We also overlay the quadtree boundaries, showing them only if toggled on.

    Arguments:
        data: single state of the simulation

    Returns:
        Overlay of the positions and quadtree
    """
    if not data:
        return hv.Points([]) * hv.Rectangles([])
    particle_data, extent_data = data
    points = hv.Points(
        particle_data,
        kdims=['x', 'y'],
        vdims=['m']).opts(
            color=hv.dim('m'),
            cnorm='log',
            cmap=cc.CET_L19,
            framewise=framewise
        )
    rectangles = hv.Rectangles(extent_data).opts(fill_color=None, line_color='yellow', alpha=(0.25 * int(quadtree_display.value))).opts(framewise=framewise)
    return (points * rectangles).opts(framewise=framewise, frame_height=640, frame_width=640)

def play(event: pr.parameterized.Event) -> None:
    """Callback to play the simulation.

    Configures a periodic callback to execute our update_model callback
    approximately 30 frames-per-second. If the callback is already scheduled
    then disable it. Also changes the button name to indicate the state.

    Arguments:
        event: the click event that triggered the callback
    """
    global periodic_callback
    if periodic_callback is None or not periodic_callback.running:
        play_button.label = 'Stop'
        # set the periodic to call our run_model callback at 30 frames per second
        periodic_callback = pn.state.add_periodic_callback(update_model, period=1000//fps_slider.value)
        table.disabled = True
    elif periodic_callback.running:
        play_button.label = 'Play'
        periodic_callback.stop()
        table.disabled = False
        particle_data = pd.DataFrame([[particle.x, particle.y, particle.m] for particle in model.particles], columns=['x','y','m'])
        extent_data = pd.DataFrame([extent for extent in model.get_extents()], columns=['x0', 'y0', 'x1', 'y1'])
        particle_pipe.send((particle_data, extent_data))

def reset(event: pr.parameterized.Event | None) -> None:
    """Callback to reset the simulation.

    Stops periodic callback if active; remove the period callback, recreate the
    model, and stream the initial model state through the pipe.

    Arguments:
        event: the click event (or None when initialized) that triggered the
        callback
    """
    global model, periodic_callback, framewise
    if periodic_callback is not None and periodic_callback.running:
        play_button.label = 'Play'
        periodic_callback.stop()
    periodic_callback = None
    num_particles = num_particles_slider.value * thread_count_slider.value
    if model is not None:
        model.request_stop()
    model = MultithreadedParticleSystem(num_particles, bounds_slider.value, theta_slider.value, seed_input.value, time_delta_slider.value, thread_count_slider.value)
    for particle in model.particles:
        r = np.hypot(particle.x, particle.y)
        if r > 1.0e-8:
            particle.vx = -particle.y / r
            particle.vy = particle.x / r
    particle_data = pd.DataFrame([[particle.x, particle.y, particle.m] for particle in model.particles], columns=['x','y','m'])
    extent_data = pd.DataFrame({
        'x0':[-bounds_slider.value],
        'y0':[-bounds_slider.value],
        'x1':[bounds_slider.value],
        'y1':[bounds_slider.value]
    })
    framewise = True
    particle_pipe.send((particle_data, extent_data))
    framewise = False
    table.value = particle_data
    table.disabled = False


def open_readme(event):
    app.open_modal()

def edit_model(event):
    if event.column == 'x':
        model.particles[event.row].x = event.value
    elif event.column == 'y':
        model.particles[event.row].y = event.value
    elif event.column == 'm':
        model.particles[event.row].m = event.value
    particle_data = pd.DataFrame([[particle.x, particle.y, particle.m] for particle in model.particles], columns=['x','y','m'])
    extent_data = pd.DataFrame([extent for extent in model.get_extents()], columns=['x0', 'y0', 'x1', 'y1'])
    particle_pipe.send((particle_data, extent_data))

# create a global for the model
model = None

# we use a pipe so that we can stream data from an asynchronous periodic callback
particle_pipe = Pipe(data=[])

# create a table view for the data
table = pn.widgets.Tabulator(disabled=False, show_index=False, pagination='local', page_size=20)
table.on_edit(edit_model)

# create a global periodic callback - with it being global and persisted we can
# start and stop it at will
periodic_callback = None
framewise = True

# play button, with the play callback attached to the on-click event of the button 
play_button = pmui.Button(name='Play', on_click=play, sizing_mode='stretch_width')

# reset button, with the reset callback attached to the on-click event of the button 
reset_button = pmui.Button(name='Reset', on_click=reset, sizing_mode='stretch_width')

open_readme_button = pmui.Button(name='Readme', on_click=open_readme, sizing_mode='stretch_width')

# input widgets for various options
seed_input = pmui.IntInput(name='Random Seed', value=1337)
num_particles_slider = pmui.FloatSlider(name='Particles per Thread', start=1, end=4000, step=1, value=100)
bounds_slider = pmui.FloatSlider(name='Bounds', start=25, end=2500, value=250, step=25)
time_delta_slider = pmui.FloatSlider(name='Time Delta (s)', start=0.1, end=1.0, value=0.1, step=0.1)

theta_slider = pmui.FloatSlider(name='Theta', start=0.0, end=2.0, value=0.5, step=0.1)

thread_count = [2 ** i for i in range(int(np.log2(os.cpu_count())))]
thread_count_slider = pmui.DiscreteSlider(name='Thread Count', options=thread_count)

fps_slider = pmui.IntSlider(name='FPS', start=1, end=60, value=25, step=1)
quadtree_display = pmui.Toggle(name='Display Quadtree', sizing_mode='stretch_width')
auto_scale_axes = pmui.Toggle(name='Auto Scale Axes', sizing_mode='stretch_width')

# upon loading the dashboard, reset the model and view
pn.state.onload(lambda: reset(None))

readme = pmui.Typography('''## Multithreaded N-Body

According to Wikipedia, the [n-body problem](https://en.wikipedia.org/wiki/N-body_problem) is...

    ... the problem of predicting the individual motions of a group of celestial objects
    interacting with each other gravitationally. Solving this problem has been motivated
    by the desire to understand the motions of the Sun, Moon, planets, and visible stars.
    In the 20th century, understanding the dynamics of globular cluster star systems
    became an important n-body problem.[2] The n-body problem in general relativity is
    considerably more difficult to solve due to additional factors like time and space
    distortions.

and a [Barnes-Hut simulation](https://en.wikipedia.org/wiki/Barnes%E2%80%93Hut_simulation) is...

    an approximation algorithm for performing an n-body simulation. It is notable for
    having order O(n log n) compared to a direct-sum algorithm which would be O(n2).

This dashboard visualizes a multithreaded C++ implementation of a Barnes-Hut simulation. Particles are represented by point-masses, colored by mass. Yellow bounding boxes indicate subdivisions of the quadtree.

### Controls

* `Particles per Thread`: Number of particles to spawn per thread utilized.
* `Bounds`: Initial bounds to spawn particles within (lower left and upper right taken as (-b, -b) and (b, b)).
* `Time Delta (s)`: The size of the time step to use for integration

---

* `Random Seed`: The initial random seed
* `Theta`: Barnes-Hut control parameter; lower values improve accuracy but decrease performance. Default of 0.5 provides great balance of realism and performance.
* `Thread Count`: Number of threads to use

---

* `FPS`: Frames-Per-Second, or how fastthe playback is. If this is faster than the model, then stuttering will occur.
* `Display Quadtree`: Render the quadtree subdivisions.
* `Play`: Play the simulation with the current configuration, or unpause the simulation (turns to `Stop`).
* `Stop`: Pause the currently running simulation (turns to `Play`).
* `Reset`: Reset the simulation state to the selected configuration options.

### Modifying Simulation Data

While the simulation is not running, you have the option of modifying the position and mass of the particles in the simulation. Simply stop the simulation, and modify values in the table directly.

''')

# assemble everything in one of the built-in templates
app = pmui.Page(
    title="Barnes-Hut & Multithreading",
    theme='dark',
    main=[
        pmui.Row(# this is important! the DynamicMap ties the plotting callback to the pipe!
            hv.DynamicMap(visualize_model, streams=[particle_pipe]).opts(
                toolbar='above',
                height=640,
                width=640
            ),
            table
    )],
    sidebar=[
        # pmui.FlexBox(open_readme_button, width=321),
        pmui.FlexBox(
            'Simulation Options',
            num_particles_slider,
            bounds_slider,
            time_delta_slider,
        ),
        pmui.FlexBox(
            'Performance Options',
            seed_input,
            theta_slider,
            thread_count_slider
        ),
        pmui.FlexBox(
            'Playback Options',
            fps_slider,
            pmui.Row(quadtree_display, width=321),
            pmui.Row(play_button, reset_button, width=321)
        )
    ],
    # modal=readme
)
app.servable()
