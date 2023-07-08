import datetime
import astropy.time
import astropy.units as u
from astropy.coordinates import get_sun

tz = astropy.time.TimezoneInfo(9*u.hour) # 時間帯を決める。
toki = datetime.datetime(2019,5,16,17,0,0,tzinfo=tz)
toki = astropy.time.Time(toki)
taiyou = get_sun(toki)
print(taiyou)
print(taiyou.get_constellation())

