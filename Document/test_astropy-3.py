from astropy.coordinates import EarthLocation

koko = EarthLocation(lat='35 40 30.78',lon='139 32 17.1')
print(koko.geodetic)
print(koko.lon)
print(koko.lat)
print(koko.height)
