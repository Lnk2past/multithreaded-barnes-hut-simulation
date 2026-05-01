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


hv.extension("bokeh")


hv.opts.defaults(
    hv.opts.Points("particles", color=hv.dim('m'), cnorm='log', cmap=cc.CET_L19),
    hv.opts.Rectangles("extents", fill_color=None, line_color='yellow'),
    hv.opts.Rectangles("extents.show", alpha=0.25),
    hv.opts.Rectangles("extents.hidden", alpha=0.0),
);


def update_model() -> None:
    """Callback that is executed by periodic callback managed by the dashboard.
    
    Update the model by a single step using the time delta. Once updated the
    model data is packed into a dataframe and sent through the pipe.
    """
    model.update()
    particle_data = pd.DataFrame(model.get_entities(), copy=False)
    extent_data = pd.DataFrame(model.get_extents(), copy=False)
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
        vdims=['m'],
        group="particles"
    )
    rectangles = hv.Rectangles(extent_data, group="extents", label=("show" if quadtree_display.value else "hidden"))
    return (points * rectangles)

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
        particle_data = pd.DataFrame(model.get_entities(), copy=False)
        extent_data = pd.DataFrame(model.get_extents(), copy=False)
        particle_pipe.send((particle_data, extent_data))

def reset(event: pr.parameterized.Event | None) -> None:
    """Callback to reset the simulation.

    Stops periodic callback if active; remove the period callback, recreate the
    model, and stream the initial model state through the pipe.

    Arguments:
        event: the click event (or None when initialized) that triggered the
        callback
    """
    global model, periodic_callback
    if periodic_callback is not None and periodic_callback.running:
        play_button.label = 'Play'
        periodic_callback.stop()
    periodic_callback = None
    num_particles = num_particles_slider.value * thread_count_slider.value
    if model is not None:
        model.request_stop()
    model = MultithreadedParticleSystem(num_particles, bounds_slider.value, theta_slider.value, seed_input.value, time_delta_slider.value, thread_count_slider.value)
    particle_data = pd.DataFrame(model.get_entities(), copy=False)
    extent_data = pd.DataFrame({
        'x0':[-bounds_slider.value],
        'y0':[-bounds_slider.value],
        'x1':[bounds_slider.value],
        'y1':[bounds_slider.value]
    })
    particle_pipe.send((particle_data, extent_data))
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
    particle_data = pd.DataFrame(model.get_entities(), copy=False)
    extent_data = pd.DataFrame(model.get_extents(), copy=False)
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

# assemble everything in one of the built-in templates
app = pmui.Page(
    title="Barnes-Hut & Multithreading",
    theme='dark',
    main=[
        pmui.Row(
            hv.DynamicMap(visualize_model, streams=[particle_pipe]).opts(
                toolbar='above',
                height=640,
                width=640,
                show_legend=False
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
    ]
)
app.servable()
