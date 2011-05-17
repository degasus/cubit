from struct import unpack, pack
from zlib import decompress
import sqlite3
from subprocess import Popen, PIPE
from glob import glob



for f in ['r.0.0.mcr','r.-1.-1.mcr','r.-1.0.mcr','r.-1.1.mcr','r.0.-1.mcr','r.0.1.mcr','r.1.-1.mcr','r.1.0.mcr','r.1.1.mcr']: #glob('r.*.*.mcr'):

  x = int(f.split('.')[1])
  z = int(f.split('.')[2])

  f = file(f).read()
  gpos = (-z*512,x*512,0)

  print gpos

  conn = sqlite3.connect('/home/arny/.cubit/cubit.db')

  def lzo(s):
    p = Popen(['lzop', '-c', '-f'],stdout=PIPE, stdin=PIPE)
    return p.communicate(s)[0]

  def decom(s):
    if len(s):
      l = unpack('>i',s[0:4])[0]
      s = decompress(s[5:l+5])
      return s.split('Blocks')[1][:32*1024]
    else:
      return None

  """
  def read_nbt(s, counts = None, typeid = None):
    while True:

      if counts == 0:
        return s
      if counts > 0:
        counts--
      
      if typeid:
        c = typeid
      else:  
        c = unpack('B',s[0])[0]
        s = s[1:]

      if c == 0:
        return s




      elif c == 1:

        s = s[1:]
      elif c == 2:
        s = s[2:]
      elif c == 3:
        s = s[4:]
      elif c == 4:
        s = s[8:]
      elif c == 5:
        s = s[4:]
      elif c == 6:
        s = s[8:]
      elif c == 7:
        c = unpack('>i',s[0:4])[0]
        s = s[4+c:]
      elif c == 8:
        c = unpack('>h',s[0:2])[0]
        s = s[2+c:]
      elif c == 9:
        s = read_nbt(s[5:],unpack('>i',s[1:5])[0],unpack('B',s[0])[0])
      elif c == 10:
        s = read_nbt(s)
     """   

  locations  = [(unpack('B',f[4*i+0])[0]<<16) + (unpack('B',f[4*i+1])[0]<<8) + unpack('B',f[4*i+2])[0] for i in range(1024)]
  counts     = [unpack('B',f[4*i+3])[0] for i in range(1024)]
  timestamps = [unpack('>i',f[4*1024+4*i:4*1024+4*i+4])[0] for i in range(1024)]
  sector     = [f[i*4096:(i+1)*4096] for i in range(len(f)/4096)]
  chunk      = [''.join(sector[locations[pos]:locations[pos]+counts[pos]]) for pos in range(32*32)]
  nbt        = [decom(s) for s in chunk]


  def getblock_minecraft(x,y,z):
    if x<0 or y<0 or z<0 or x>=16*32 or y>=128 or z>=16*32:
      print 'error: ', x,y,z
    pos = (x/16) + (z/16)*32
    c = nbt[pos]
    if c is None:
      raise IndexError
    data = c[((x%16)<<11) + ((z%16)<<7) + y]
    return unpack('B',data)[0]


  for x in range(16):
    for y in range(16):
      for z in range(4):
        try:
          s = [0] * (32*32*32)
          for pos in range(32*32*32):
            xb, yb, zb = pos/1024, (pos/32)%32, pos%32
            if zb < 10 and z == 0:
              block = 7
            else:
              block = getblock_minecraft(y*32+yb, z*32+zb, 16*32-x*32-xb-1)
            s[pos] = block
          s = ''.join([pack('B',i) for i in s])
          print '%d x %d x %d found' % (x,y,z)
          #s = lzo(s)
          conn.execute('INSERT OR REPLACE INTO area (posx, posy, posz, empty, revision, full, blocks, data) VALUES (?,?,?,?,?,?,?,?)',(gpos[0]+x*32, gpos[1]+y*32, gpos[2]+z*32, 0, 1, 0, -1, sqlite3.Binary(s)))
          #print '%d x %d x %d found' % (gpos[0]+x*32, gpos[1]+y*32, gpos[2]+z*32)
          #print len(s)
          conn.commit()
        except IndexError, e:
          pass
