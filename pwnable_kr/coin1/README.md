---
layout: post
title: PWNABKE KR - TODDLER - coin1 - 6pt
---

간만에 pwnable write-up.


## 1. Check it up

우선 문제를 보면 ~~내가 제일 싫어하는 유형의~~ 바이너리도 없고 소스코드도 없다.

![fig1](./_fig/1.png)

그래서 접속을 또 해보면..

![fig2](./_fig/2.png)

와.. 문제 이해하기도 힘들다. 영어 못하면 풀 수도 없는 문제다...
간단하게 요약하면 ```N``` 개의 동전이 있고, 기회는 ```C```번이 있다. 여기서 ```N``` 개의 동전 가운데, ```C``` 번 안에 가짜 동전을 찾는 게 핵심인데, 가짜 동전 하나만  **무게가 다르다**

~~사실 이런거구나, 하고 파악하는데만도 시간이 꽤 오래 걸렸다...~~

어쨌든. 이렇게 바이너리도 없고, 소스도 없으면 좀 난감해서 혹시나 네트워크 패킷 상에 숨겨진 데이터가 있으려나 해서 ```$ nc -o pwnable.kr 9007```을 주고 덤프파일도 살펴봤으나 역시 그런게 있을리 없다.

혹시나 입력값을 마구 때려넣으면 죽기라도 하려나 했는데, 특수문자를 줘도 이상한 값을 줘도 ```format error```를 내고 적절하게 죽는다.

두 가지 방법이 있을 듯 하다. (아무래도 언어는 python을 쓸테니)
 - 소켓 통신을 구현하든가
 - subprocess로 stdio / stdout을 제어하든가

 난 두 번째 방법을 선택했다. 이유는, 일단 소켓통신 하면 recv/send 써가면서 데이터 일일이 열어보는게 귀찮기도 하고 두 번째로 예전에 Tizen store의 HSM 기반 백엔드 서명시스템 구축을 할 때, subprocess를 써서 만들었던 기억이 있어서 옛 추억도 떠올릴 겸, 두 번째 방법을 선택.

 참고로 subprocess로 stdio/stdout을 handling할 때, 일반적으로 ```communicate()```를 많이 쓰는데, 경험상 이거는 interaction이 있을 경우에는 좀 힘들다. (방법이 있긴 한 거 같던데.. 테스트를 여러 번 해봐도 잘 안되더라..) 따라서 실제 user interation이 포함된(stdin/stdout이 빈번한) python code snippet은 아래와 같음

 ```python
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

 ```

 하지만, 결론적으로 무슨 방법을 쓰든 삽질은 똑같았을 거 같다..

 ## 2. Approaches

 처음에는 일일이 하나씩 입력을 해야 하는 줄 알고 (예: N=299 C=10 이라고 하면, 10번 안에 값을 찍어야 하는 줄 알았다;;) 삽질을 했는데 ```1 2 3 4 5 6 7 8 9 10``` 요런 게 먹힌다는 걸 찾았다. ~~ㅋㅋㅋㅋㅋㅋㅋ~~

 그리고 가짜 동전은 무게가 9이다. (진짜는 10이고..) 이거는 위의 ```1 2 3 4 5 6 7 8 9 10```을 넣었을 때 모두 진짜 동전이면 100이, 가짜가 들어있으면 99가 나온다는 걸 통해 알 수 있다.

 또한, 문제를 30초 내에 풀지 못하면 강제로 good bye하고 인사하게 된다.

 결국 코딩밖에 없다는 것인데, 일단 코딩에 들어가기 전에 값을 어떻게 찾는 게 가장 효율적일 것인가를 고민을 많이 했는데, binary search 밖에는 생각나는 게 없었다. 혹시 다른 방법이 있다면 좀 참고해봐야겠다. 즉,

  1. Buffer를 준비해서 N만큼 집어넣는다
  2. Buffer를 반으로 쪼개서 왼쪽 꺼를 집어넣고 오른쪽꺼를 집어넣어본다
  3. 둘 중에 10으로 나눠떨어지는 애가 있으면 진짜, 아니면 가짜
  4. 가짜를 버퍼로 잡아 다시 2번 반복

이 때, 전체 개수(```N```)가 주어진 횟수(```C```) 대비, ```2^C < N``` 이어야 값을 찾을 수 있다는 전제가 있는데, 코드로 확인해보면 사실 저걸 넘어서는 케이스는 안나오더라.

```python
def checkcancal(n, c):
  print "[checkitup]"
  print  math.log(n, 2)
```  

## 3. 코드 작성

생각보다 엄청 길어졌다.. 막상 하다보니까 생각해야 하는 케이스도 좀 많았고 특히 함정이 하나 있어서 정말 시간을 많이 보냈다. (근데 생각해보면, 함정이 아니라 그냥 그게 문제였다능..)

어느 정도 동작하기 시작하면서, 마무리만 지으면 되겠다고 생각했는데.. 희안한 문제가 생기기 시작했다.

![fig3](./_fig/3.png)

분명히 맞췄다고 생각했는데, 문제가 또 나오기 시작하는거다.. ~~fuxx~~ 아 이게 좀 굉장히 난감해서 (특히 문제 특성 상 손으로 확인할 수가 없는지라) 내가 뭔가 잘못 출력하는 건가 당황했었는데, 알고보니 저 ```Correct!``` 뒤에 있는 숫자가 맞출 때마다 점점 올라가는 것이었다. 결론적으로 100번을 맞춰야 한다.

실제로 문제가 시스템적인 지식을 필요로 하기 보다는 "간단하게 코딩이나 해봐라"하는 문제로 이해하면 될 듯 하다. 그래서 열심히 했다...


## 4. Exploit

그냥 전체 full source로 대체.
~~솔직히 똥코드인 거 나도 알긴하는데, 그래도 돌아가니 기쁘다..~~

```python

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

```

디버깅, 특히 stdin/out을 다루다보니, 실제 버퍼에 엔터 ```\xa```나 스페이스바가 들어가는 걸 정확히 보기 위해 hexdump를 사용했다. 나머지 코드가 좀 지저분하긴 한데 걍 잘 돌아감..ㅋㅋㅋ

결과는

![fig4](./_fig/4.png)
