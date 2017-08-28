### doxygen이 무시하는 줄
# Cryptolib with TPM
## 개발 및 테스트 환경
Ubuntu 14.04

## 빌드 과정 
### 필요한 패키지들
* 실행파일 빌드
 * libjson-c-dev
 * libtspi-dev
 * libssl-dev
* 유닛테스트
 * check
* PDF 생성
 * texlive-latex-extra
 
Ubuntu에서는 다음과 같은 명령을 사용해서 필요한 패키지들을 설치할 수 있습니다:

    sudo apt-get install check libjson-c-dev libtspi-dev libssl-dev texlive-latex-extra

### 빌드 명령
run:

    ./bootstrap && ./configure && make

#### pdf 생성

    make pdf

문서는 docs/latex/refman.pdf 입니다.

#### html 생성

    make html

문서는 docs/html/index.html 입니다.

## 유닛테스트
* 요구사항:
  "check" 라이브러리를 설치해야 합니다. "check"는 C언어용 유닛테스트 프레임웍입니다.

### 테스트 실행
run:

    make check

### TPM을 사용하는 명령을 실행하기 전에:

    tpm_takeownership -yz

명령을 사용해서 TPM의 소유권을 획득해야 합니다.

## 암호라이브러리

관련 함수의 선언은, [CryptoSuite.h](CryptoSuite_8h.html)에 있습니다.

### 공개키 암호
다음은 공개키 암호의 개념을 보입니다.<br>
<img src="../images/sign_verify.png" width="500" >
@latexonly
\graphicspath{{../images/}}
\includegraphics[width=0.70\linewidth]{sign_verify.png}
@endlatexonly

본문이 주어지면, 이 본문과 비밀키를 가지고 서명을 만들어낼 수 있습니다.<br>
문서가 변경되지 않았음을 보증하기 위해 문서를 작성한 주체는 본문과 서명을 같이 전달합니다. 서명을 전달받은 주체는 미리 가지고 있던 공개키를 사용하여 본문과 서명이 일치하는지 검사할 수 있습니다.

좀더 자세하게 들어가 보면 그 원리는 다음과 같습니다.

<img src="../images/sign_verify_detail.png" width="500" >
@latexonly
\graphicspath{{../images/}}
\includegraphics[width=0.70\linewidth]{sign_verify_detail.png}
@endlatexonly

본문을 서명하기 전에 먼저 해시를 계산합니다.  해시는 본문의 내용에 따라 달라지는 요약값이라고 할 수 있습니다. 단 한글자, 한비트만 바뀌어도 요약값은 크게 달라지기 때문에 해시값이 변경되지 않도록 유지하면서 본문을 고치는 것은 아주 어렵다는 특징이 있습니다.

계산한 해시값을 비밀키로 암호화하는 과정을 ``sign()``이라고 합니다. 암호화한 결과는 서명(signature)라고 합니다.

서명은 비대칭알고리즘의 특성을 이용하여 공개키로 해독할 수 있습니다. 해독한 결과는 해시입니다.

서명을 검증하는 과정은, 1) 본문으로부터 해시를 구하고, 2) 공개키를 이용하여 서명을 해독해서 해시를 구한 다음 3) 1)과 2)의 결과가 동일한지 비교하는 것입니다.

#### 서명

* ``gen_signature()``<br>
argument: (privkey, contents, len, sig, siglen)<br>
주어진 내용(``contents``)에 해당하는 서명(``sig``)을 계산해냄.<br>
서명을 계산하는데에는 내용과 비밀키(``privkey``)가 필요함.<br>
계산한 서명을 기록하는 공간은 ``sig``, 그 크기는 ``siglen``<br>

#### 검증

* ``verify_signature()``<br>
argument: (pubkey, contents, len, sig, siglen)<br>
주어진 내용(``contents``)과 서명(``sig``)의 무결성을 검사함.
서명을 검사하는데에는 내용과 서명, 그리고 공개키(``pubkey``)가 필요함.

#### 개인키 생성

위의 서명 과정에 사용할 개인키는 다음 함수를 사용하여 생성합니다.

* ``gen_private_key()``(privkeyfile)
 + ``privkeyfile``은 만들어낸 ㄴ키를 저장할 파일의 위치와 이름입니다.

#### 공개키 생성

위의 개인키와 쌍이 되는 공개키를 만들기 위해 다음 함수를 사용합니다. 여기서 만든 공개키는 외부에 임의로 배포할 수 있습니다.

* ``gen_public_key()``<br>
``(privkeyfile, pubkeyfile)``
 + ``privkeyfile``은 기준이 되는 개인키가 저장된 파일입니다.
 + ``pubkeyfile``은 새로 만든 공개키를 저장할 파일의 위치와 이름입니다.

#### 그 밖의 요긴한 기능들
공개키 암호 알고리즘은 그 특성상 암호화 및 복호화 또는 서명 및 검증 과정에 서로 다른 키를 사용합니다. 가장 대표적인 응용인 서명과 검증은 개인키로 서명하고 공개키로 검증하는 동작을 합니다.

그와 반대로, 공개키로 암호화하고 개인키로 복호화하는 동작이 있을 수 있습니다. 이런 과정은 공개적으로 메시지를 전달하되 그 복호화는 개인키의 소유자만 수행할 수 있게 만들고자 할 때 사용합니다. 아래의 ``public_encrypt()``와 ``private_decrypt()``를 사용할 수 있습니다.

인증서 메커니즘을 사용하려면 내가 만든 인증서를 CA(인증기관)에게 보내어 서명을 받아와야 합니다. 이 때 사용하는 파일을 CSR(Certificate Signing Request)라고 하는데, 이런 형태의 파일을 만드는 경우 아래의 함수를 사용할 수 있습니다. 아래의 ``gen_x509_request()``를 사용할 수 있습니다.

##### 공개키 암호화

* ``public_encrypt()``<br>
argument: ``(pubkeyfile, plain, plainlen, cipher, cipherlen)``
 + ``pubkeyfile``: 암호화에 사용할 공개키파일의 위치
 + ``plain``: 암호화할 평문이 저장된 메모리 공간, ``plainlen``는 그 크기
 + ``cipher``: 암호문을 저장할 공간, ``cipherlen``는 그 크기

##### 비밀키 해독

* ``private_decrypt()``<br>
argument: ``(privkeyfile, cipher, cipherlen, plain, plainlen)``
 + ``privkeyfile``: 복호화에 사용할 개인키파일
 + ``cipher``:암호화할 암호문이 저장된 메모리 공간, ``cipherlen``는 그 크기
 + ``plain``: 평문을 저장할 공간, ``plainlen``는 그 크기

##### CSR(Certificate Signing Request) 생성

* ``gen_x509_request()``<br>
argument: ``(privkeyfile, reqfile)``
 + ``privkeyfile``:  csr 파일을 서명하는데 사용할 개인키
 + ``reqfile``: 새로 만든 csr파일을 저장할 위치

### 대칭키 암호

일반적으로 지칭하는 암호는 대칭키 암호를 의미합니다. 평문을 암호화하여 암호문으로 변환할 때 사용하는 키와 암호문을 평문으로 풀어낼 때 사용하는 키가 동일합니다.

<img src="../images/encryption.png" width="500" >
@latexonly
\graphicspath{{../images/}}
\includegraphics[width=0.70\linewidth]{encryption.png}
@endlatexonly

암호문을 만들어서 전달하기 전에 암호문을 받는 주체는 그 키를 알고 있어야 복호화할 수 있습니다. 동일한 키를 양쪽에서 알고 있어야 사용할 수 있으므로 공개키 알고리즘에 비해 보안성이 떨어지지만 암호화 과정의 연산이 간단하므로 성능상의 이유로 많이 사용합니다.

#### 암호화

##### 파일 암호화

파일을 암호화하는 경우 일반적으로 파일의 내용을 모두 메모리로 읽어 들인 다음 암호화를 수행하므로 함수를 한번만 호출하면 간단합니다. 이 때 사용할 수 있는 함수는 ``sym_encrypt()``입니다.

* ``sym_encrypt()``<br>
argument: ``(plainbuf, plainlen, cipherbuf, cipherlen, key)``
 + ``plainbuf``: 평문이 저장된 메모리 공간이고, ``plainlen``는 그 길이
 + ``cipherbuf``: 암호문을 저장할 공간이고,  ``cipherlen``는 그 크기
 + ``key``: 암호화에 사용할 대칭키

##### 스트림 암호화

네트웍을 통해 흐르는 데이터, 즉 스트림을 암호화하는 경우 모든 데이터를 한번에 암호화할 수 없습니다. 또한 암호화 전 과정을 통해 유지해야 하는 암호 관련 컨텍스트가 있으므로 이를 유지해야 합니다. 이런 이유로 ``sym_encrypt()``를 사용할 수 없습니다. 스트림을 암호화 하기 위해, 암호화 과정을 "초기화, 암호연산(반복가능), 정리"의 세가지 과정으로 분리하고 각각의 경우에 맞게 사용할 수 있습니다.

* ``encrypt_init()``: 암호화 과정의 초기화. 스트림 데이터 처리 전 1회만 동작.
* ``encrypt_update()``: 데이터를 암호화 하는 실제 연산. 제한없이 반복 가능
* ``encrypt_final()``: 암호화 과정의 정리. 스트림 데이터 처리 후 1회만 동작.

#### 복호화
##### 파일 복호화

암호문이 저장된 파일을 메모리로 읽어들여 한번에 복호화할 때 사용합니다.

* ``sym_decrypt()``<br>
argument: ``(cipherbuf, cipherlen, plainbuf, plainlen, key)``
 + ``cipherbuf``: 암호문이 저장된 메모리 공간이고, ``cipherlen``은 그 길이
 + ``plainbuf``: 평문을 저장할 공간이고, ``plainlen``은 그 크기
 + ``key``: 암호화에 사용할 대칭키

##### 스트림 형식의 복호화

위 ``encrypt_init()``, ``encrypt_update()``, ``encrypt_final()``에 대응하는 함수입니다.

* ``decrypt_init()``: 복호화 과정의 초기화. 스트림 데이터 처리 전 1회만 동작.
* ``decrypt_update()``: 데이터를 해독 하는 실제 연산. 제한없이 반복 가능
* ``decrypt_final()``: 복호화 과정의 정리. 스트림 데이터 처리 후 1회만 동작.

#### 키 생성

임의의 문자열을 만들고 이를 암호키로 사용하고자 하는 경우 ``gen_secret_key()``를 사용할 수 있음.

* ``gen_secret_key()``(buf, buflen)

#### 키 저장

대칭키 암호의 경우 암호와 해독 두가지 연산에 동일한 키를 사용하기 때문에 둘 이상의 주체가 암호키를 공유한다. 이 때 어느 한쪽의 암호키가 노출된다면 노출된 암호키를 사용하는 전체 시스템이 위험해진다. 암호키를 안전하게 저장하는 것이 중요하다.

TPM이 제공하는 ``seal``연산을 이용하면 저장된 대칭키를 안전하게 암호화할 수 있다. ``seal``에 사용하는 암호키는 TPM 내부의 정보를 이용하므로 암호파일이 노출되더라도 TPM이 없으면 이를 복호화 또는 해독할 수 없다.

TPM이 없다면 암호키를 안전하게 암호화할 수는 없다. 다만 시스템의 정보를 이용해 암호화함으로써 그 내용을 다른 장치로 옮겼을 경우 풀어내기 어렵도록 제한할 수 있다.

* ``save_secret()``<br>
argument: ``(filename, secret, buflen)``
 + ``filename``: 대칭키를 저장할 파일의 이름
 + ``secret``: 대칭키가 있는 메모리 공간이고, ``buflen``은 대칭키의 길이
* ``load_secret()``<br>
argument: ``(filename, secret, buflen)``
 + ``filename``: 대칭키가 저장된 파일의 이름
 + ``secret``: 대칭키를 읽어올 메모리 공간이고, ``buflen``은 그 크기

### 난수 생성

* ``get_random()``<br>
길이가 ``buflen`` 인 메모리 공간 buf에 난수를 채움.<br>
TPM이 설치되어 있으면 TPM 내부의 TRNG를 이용함.

## Functions
[Functions](globals_func.html)
