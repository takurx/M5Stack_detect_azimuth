import datetime
import astropy.time
import astropy.units as u
from astropy.coordinates import get_sun
from astropy.coordinates import AltAz

from astropy.coordinates import EarthLocation

import matplotlib as mpl
import numpy as np
import matplotlib.pyplot as plt

# >>> input lattitude and longitude
koko = EarthLocation(lat='35 40 30.78',lon='139 32 17.1')
#print(koko.geodetic)
#print(koko.lon)
#print(koko.lat)
#print(koko.height)

#toki = astropy.time.Time(datetime.datetime(2019,5,16,17,0,0)) - 9*u.hour
#taiyou = get_sun(toki).transform_to(AltAz(obstime=toki,location=koko))
#print(taiyou)
#print(taiyou.az) # 天球での方位角
#print(taiyou.alt) # 天球での仰俯角
#print(taiyou.alt.value)
#print(taiyou.distance) # 距離
#print(taiyou.distance.au) # au単位での距離

# >>> input start time and time difference from GMT
#toki = astropy.time.Time('2019-5-16') - 9*u.hour
toki = astropy.time.Time('2023-8-06') - 9*u.hour
#print(toki)
#toki += np.linspace(0,24,1441)*u.hour # 1441 = 24*60 + 1
toki += np.linspace(0,24,289)*u.hour #  = 24*60/5 + 1
#print(toki)
taiyou = get_sun(toki).transform_to(AltAz(obstime=toki,location=koko))
#print(taiyou.alt.value)
#print(taiyou.az.value)
for i in range(len(taiyou.alt.value)):
	print(i, end=' ')
	print(toki[i].datetime, end=' ')
	print(taiyou.alt.value[i], end=' ')
	print(taiyou.az.value[i])
	"""
	if taiyou.az.value[i] < 180:
		taiyou.az.value[i] = taiyou.az.value[i] - 90
	elif taiyou.az.value[i] >= 180 and taiyou.az.value[i] < 270:   # taiyou.az.value[i] >= 180
		taiyou.az.value[i] = abs(taiyou.az.value[i] - 270)
	else:
		taiyou.az.value[i] = (taiyou.az.value[i] - 270) * -1
	"""
	#print(i, end=' ')
	#print(toki[i].datetime, end=' ')
	#print(taiyou.alt.value[i], end=' ')
	#print(taiyou.az.value[i])

	if taiyou.alt.value[i] > 0:
		pass
		#print(i, end=' ')
		#print(toki[i].datetime, end=' ')
		#print(taiyou.alt.value[i], end=' ')
		#print(taiyou.az.value[i])
	else:
		pass
		#taiyou.alt.value[i] = 0
		#taiyou.az.value[i] = 70
		#del toki[i]
		#del taiyou.alt.value[i]
		#del taiyou.az.value[i]
		#np.delete(toki,i)
		#np.delete(taiyou.alt.value,i)
		#np.delete(taiyou.az.value,i)
	#pass
#pass

plt.plot((toki+9*u.hour).datetime,taiyou.alt.value,'r')
plt.plot((toki+9*u.hour).datetime,taiyou.az.value,'r')
plt.gca().xaxis.set_major_formatter(mpl.dates.DateFormatter('%H:%M'))
plt.axhline(0,ls='--',color='k') # 日の出と日没の線
takasa = taiyou.alt.max().value # 一番高い高さ
takai_toki = (toki[taiyou.alt.argmax()]+9*u.hour).datetime # 一番高い時間
plt.scatter(takai_toki,takasa)
#plt.text(takai_toki,takasa,'%s\n%.2f度'%(takai_toki.strftime('%H:%M'),takasa),fontname='AppleGothic',va='top',ha='center')
plt.text(takai_toki,takasa,'%s\n%.2f度'%(takai_toki.strftime('%H:%M'),takasa),va='top',ha='center')
#plt.xlabel(u'時間',fontname='AppleGothic')
#plt.ylabel(u'太陽の仰俯角 (度)',fontname='AppleGothic')
#plt.xlabel(u'Time',fontname='AppleGothic')
#plt.ylabel(u'Sun Elevation/depression angle (Degree)',fontname='AppleGothic')
plt.xlabel(u'Time')
plt.ylabel(u'Sun Elevation/depression angle (Degree)')
plt.show()

ax = plt.subplot(111, projection="polar")

#size = 1000
#x = np.linspace(0, 2 * np.pi, size)
#y = np.sin(6*x) + np.random.normal(0, 0.08, (size))
#x = toki
#y = taiyou.az.value

# r=1AU, theta=azimuth, phi=altitude
#ax.plot(x, y)
#ax.plot((toki+9*u.hour).datetime,taiyou.az.value, 'r')
#ax.plot((toki+9*u.hour).datetime,taiyou.alt.value)

#ax.plot(taiyou.az.value,taiyou.alt.value)

#plt.show()

