import datetime
import astropy.time
import astropy.units as u
from astropy.coordinates import get_sun

import numpy as np
import matplotlib.pyplot as plt

tz = astropy.time.TimezoneInfo(9*u.hour)
toki = datetime.datetime(2019,1,1,12,0,0,tzinfo=tz)
toki = astropy.time.Time(toki) + np.arange(365)*u.day

taiyou = get_sun(toki)
plt.scatter(toki.value,taiyou.distance.value,c=np.arange(365),s=1,cmap='rainbow')
chikai_toki = toki.value[taiyou.distance.value.argmin()]
tooi_toki = toki.value[taiyou.distance.value.argmax()]
plt.axvline(chikai_toki,color='k',ls=':')
plt.axvline(tooi_toki,color='k',ls=':')
plt.text(chikai_toki,1,'%s\n%.3f AU'%(chikai_toki.date(),taiyou.distance.value.min()))
plt.text(tooi_toki,1,'%s\n%.3f AU'%(tooi_toki.date(),taiyou.distance.value.max()))
plt.ylabel(u'距離 (AU)',fontname='AppleGothic')
plt.show()

print('平均値　=　%s'%taiyou.distance.mean())
print('中央値　=　%s'%np.median(taiyou.distance))
print('最小　　=　%s'%taiyou.distance.min())
print('最大　　=　%s'%taiyou.distance.max())
print('標準偏差=　%s'%taiyou.distance.std())

seiza = taiyou.get_constellation()
print('\n'.join(sorted({'%s: %d'%(s,list(seiza).count(s)) for s in set(seiza)},key=lambda x:int(x.split(':')[1]),reverse=True)))
