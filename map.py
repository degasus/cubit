import os
import re

reg = re.compile('(-?\d+)x(-?\d+)x(-?\d+)\.map')
print 'PRAGMA synchronous = 0;';
for i in os.listdir('maps'):
  data = file('maps/' + i).read()
  m = reg.match(i)
  if m:
    g = m.groups()
    print "INSERT INTO area VALUES (%s, %s, %s, 0, 0, X'%s');" % (g[0], g[1], g[2], ''.join([ "%02X" % ord( x ) for x in data ] ))

