import numpy as np

import matplotlib.pyplot as plt
from astropy.time import Time
import astropy.units as u

from astroplan.plots import plot_sky

observe_time = Time('2000-03-15 17:00:00')

observe_time = observe_time + np.linspace(-4, 5, 10)*u.hour

plot_sky(altair, observer, observe_time)

plt.legend(loc='center left', bbox_to_anchor=(1.25, 0.5))

plt.show()
