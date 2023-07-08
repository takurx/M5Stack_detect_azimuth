import datetime
import astropy.time
import astropy.units as u
from astropy.coordinates import get_sun
from astropy.coordinates import AltAz

from astropy.coordinates import EarthLocation

import matplotlib as mpl
import numpy as np
import matplotlib.pyplot as plt

koko = EarthLocation(lat='35 40 30.78',lon='139 32 17.1')
print(koko.geodetic)
print(koko.lon)
print(koko.lat)
print(koko.height)

toki = astropy.time.Time(datetime.datetime(2019,5,16,17,0,0)) - 9*u.hour
taiyou = get_sun(toki).transform_to(AltAz(obstime=toki,location=koko))
print(taiyou)
print(taiyou.az) # 天球での方位角
print(taiyou.alt) # 天球での仰俯角
print(taiyou.distance) # 距離
print(taiyou.distance.au) # au単位での距離

toki = astropy.time.Time('2019-5-16') - 9*u.hour
toki += np.linspace(0,24,1441)*u.hour
taiyou = get_sun(toki).transform_to(AltAz(obstime=toki,location=koko))
plt.plot((toki+9*u.hour).datetime,taiyou.alt.value,'r')
plt.gca().xaxis.set_major_formatter(mpl.dates.DateFormatter('%H:%M'))
plt.axhline(0,ls='--',color='k') # 日の出と日没の線
takasa = taiyou.alt.max().value # 一番高い高さ
takai_toki = (toki[taiyou.alt.argmax()]+9*u.hour).datetime # 一番高い時間
plt.scatter(takai_toki,takasa)
plt.text(takai_toki,takasa,'%s\n%.2f度'%(takai_toki.strftime('%H:%M'),takasa),fontname='AppleGothic',va='top',ha='center')
plt.xlabel(u'時間',fontname='AppleGothic')
plt.ylabel(u'太陽の仰俯角 (度)',fontname='AppleGothic')
plt.show()

