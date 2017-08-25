#!/usr/bin/python

import os
import sys
import time
import math
# for debug
import hexdump

from subprocess import Popen, PIPE, STDOUT

## Extract N and C from stdout
def findNC(str):
  if ("N=" in str) and ("C=" in str) and ("N=4 C=2" not in str):
    _str = str.split(' ' )
    _nstr = _str[0].split('=')
    _cstr = _str[1].split('=')

    return int(_nstr[1]), int(_cstr[1])

def splitbuffer(input, s, n):
      l_input = ""
      r_input = ""

      mid = (s+n) / 2
      left = range(s, mid+1) ## +1...why..
      right = range(mid+1, n)

      for iter in range(0, len(left)):
        l_input = l_input + str(left[iter]) + " "

      for iter in range(0, len(right)):
        r_input = r_input + str(right[iter]) + " "

      ## for latest character buffer
      l_input = l_input.rstrip(" ") + '\n'
      r_input = r_input.rstrip(" ") + '\n'

      ## for debugging
      print "[l_input] : " + l_input
#      print "[hex l] : " + hexdump.dump(l_input, sep=":")
      print "[r_input] : " + r_input
#      print "[hex r] : " + hexdump.dump(r_input, sep=":")

      return l_input, r_input

def initbuf(input, s, n):
    output = ""

    input = range(s,n+1)

    for iter in range(0, len(input)):
      output = output + str(input[iter]) + " "

    output = output.rstrip(" ") + '\n'
    print "[Init buf] : " + output

    return output

def findfakecoin(buf, s, n, c, state):

      if c < 0:
         print "[ERROR] Cannot find"
         sys.exit()

      print "** Phase start : "+ str(s) + " " +  str(n) + " " +  str(c)
   
      l_input, r_input = splitbuffer(buf, s, n)  
    
      p.stdin.write(l_input)
      output = p.stdout.readline()
      print "[output] : " + output

      #sys.stdout.flush()
      side = checkfakeisin(output, state)

      mid = (s+n)/2

      if side == 1:
           #p.stdin.write(l_input)
           #output = p.stdout.readline()
           print "[RESULT] : " + output
      elif side == 2:
          print "GOTO RIGHT"
          findfakecoin(r_input, mid+1, n, c-1, 2)
      elif side == 3 :
          print "GOTO LEFT"
          findfakecoin(l_input, s, mid, c-1, 3)
      else: 
          print " ===  ERROR . FIN === "
          sys.exit()

def checkfakeisin(output, state):
      
      if ("error" in output) : 
         print "wiwiwiwiwiwiw"
         p.stdout.close()
         sys.exit()
      elif ("Correct") in output : 
         return 1
      elif int(output) % 10 == 9 : ## right side
         return 3
      elif int(output) % 10 == 0 : ## left side
         return 2
      else:
         return -1

#   p.stdin.write("1 2 3 4 5 6 7 8 9 10\n")
#   line=p.stdout.readline()
#   print line

def checkcancal(n, c):
  print "[checkitup]"
  print  math.log(n, 2)


## Main start ##
conn_cmd = "nc -o log.txt pwnable.kr 9007"
p = Popen(conn_cmd, shell=True,stdin=PIPE, stdout=PIPE)

buf = ""

for line in iter(p.stdout.readline, b''):
   print line
   nc = findNC(line) ## nc is a tuple of N and C or NONE

   if nc is not None: ## found N and C
     print "[FOUND NC] : " + str(nc[0]) + " & " + str(nc[1]) 
     outbuf  = initbuf(buf, 1, nc[0])
     findfakecoin(outbuf, 1, nc[0], nc[1], 0)
   
   
p.stdout.close()




   
  
