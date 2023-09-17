import matplotlib.pyplot as plt
from astropy.time import Time
from astroplan.plots import plot_airmass

observe_time = Time('2000-06-15 23:30:00')

plot_airmass(target, observer, observe_time)
plt.show()
