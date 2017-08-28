# Raspbian에서 CryptoSuite 빌드하고 사용하기
## 준비물
* Raspberry PI 3
* TPM 모듈
* FTDI 인터페이스

## Raspbian 설치하기
### Download
    https://www.raspberrypi.org/downloads/raspbian/
    https://downloads.raspberrypi.org/raspbian_lite_latest

### Install
    https://www.raspberrypi.org/documentation/installation/installing-images/README.md

    linux)
    dd if=2016-09-23-raspbian-jessie-lite.img  | pv -s 1300M | sudo dd of=/dev/mmcblk0

### Edit Env

  /media/~/boot/config.txt
  enable_uart=1

### 초기 설정

* Login:<br>pi/raspberry
* Network Setup:<br>
ref) https://www.raspberrypi.org/documentation/configuration/wireless/wireless-cli.md


    $ sudo cat /etc/wpa_supplicant/wpa_supplicant.conf 
    country=Asia/Seoul
    ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
    update_config=1

    network={
	ssid="MY SSID here"
	psk="MY PSK here"
    }

## TPM 관련 소프트웨어 설치하기
### Install Trousers
    sudo apt-get update
    sudo apt-get install trousers libtspi1 libtspi-dev tpm-tools


### fetch & unzip Kernel packages
    wget kernel_for_tpm.tar.bz2
    tar jxf kernel_for_tpm.tar.bz2
    ls target/

#### Install Image

    sudo mv /boot/kernel7.img /boot/kernel7.img.raspbian
    tar c -O -C target . | sudo tar x -C /
    # 여기서 발생하는 에러는 무시합니다. /boot 가 FAT를 사용하기 때문에 발생하는 에러입니다.
    sudo depmod -a 4.4.13

## TPM 초기화

### Reboot

### Initialize TPM
    edit /boot/extlinux/extlinux.conf
    "default FIT" -> "default FIT.single"

Reboot (Power off & on) - 전원케이블을 완전히 분리한 다음 다시 연결하세요.

    $ service tcsd start
    $ tpm_clear -f
    $ halt

Reboot (Power off & on) - 전원케이블을 완전히 분리한 다음 다시 연결하세요.

    $ service tcsd start
    $ tpm_setenable -ef
    $ tpm_setactive -a
    $ halt

Reboot (Power off & on) - 전원케이블을 완전히 분리한 다음 다시 연결하세요.
  
    $ service tcsd start
    $ tpm_takeownership -yz
    edit /boot/extlinux/extlinux.conf
    "default FIT.single" -> "default FIT"

    $ halt
Reboot (Power off & on) - 전원케이블을 완전히 분리한 다음 다시 연결하세요.

## CryptoSuite 빌드하기

### Install libraries
    sudo apt-get install build-essential git check libtspi-dev libssl-dev libjson-c-dev automake libtool 

### Fetch and Build cryptolib
    git clone srcrepo
    ./bootstrap
    ./configure
    make

