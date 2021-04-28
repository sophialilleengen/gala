# ---
# jupyter:
#   jupytext:
#     text_representation:
#       extension: .py
#       format_name: light
#       format_version: '1.5'
#       jupytext_version: 1.11.1
#   kernelspec:
#     display_name: Python 3
#     language: python
#     name: python3
# ---

# +

get_ipython().magic('config InlineBackend.figure_format = "retina"')  # noqa

import matplotlib.pyplot as plt

plt.style.use("default")

# NOTE: if you update these, also update docs/conf.py
plot_rcparams = {
    'image.cmap': 'cma:hesperia',

    # Fonts:
    'font.size': 16,
    'figure.titlesize': 'x-large',
    'axes.titlesize': 'large',
    'axes.labelsize': 'large',
    'xtick.labelsize': 'medium',
    'ytick.labelsize': 'medium',

    # Axes:
    'axes.labelcolor': 'k',
    'axes.axisbelow': True,

    # Ticks
    'xtick.color': '#333333',
    'xtick.direction': 'in',
    'ytick.color': '#333333',
    'ytick.direction': 'in',
    'xtick.top': True,
    'ytick.right': True,

    'figure.dpi': 300,
    'savefig.dpi': 300,
}

plt.rcParams.update(plot_rcparams)
