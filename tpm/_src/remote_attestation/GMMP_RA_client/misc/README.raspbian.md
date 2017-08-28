# NOTE - original files are located at:
# remote_attestation_crypto_lib/_src/cryptolib/Sample_Registration_Encryption/README.raspbian.md
# remote_attestation_crypto_lib/_src/cryptolib/Sample_Registration_Encryption/misc/kernel_for_tpm.tar.bz2

#
# Setup Raspbian for PoC of RemoteAttestation
#

## Preparation
* Raspberry Pi 3
* TPM module
* FTDI interface for console


## Install Raspbian into SD card (Host)

### Download
    https://www.raspberrypi.org/downloads/raspbian/
    https://downloads.raspberrypi.org/raspbian_lite_latest

### Install Image into SD card
    https://www.raspberrypi.org/documentation/installation/installing-images/README.md

    linux)
    dd if=2016-09-23-raspbian-jessie-lite.img  | pv -s 1300M | sudo dd of=/dev/mmcblk0

### Edit Env in SD card

	linux)
	/media/~/boot/config.txt (ex. /media/user_name/boot/config.txt)
	enable_uart=1


## Initial setup after first booting (RaspberryPi)

	NOTE - Initial booting might take some time for resizing the partition of SD card.

### Login
	ID/Password - pi/raspberry

### Network setup
	ref) https://www.raspberrypi.org/documentation/configuration/wireless/wireless-cli.md

    $ sudo cat /etc/wpa_supplicant/wpa_supplicant.conf 
    country=Asia/Seoul
    ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
    update_config=1

    network={
	ssid="MY SSID here"
	psk="MY PSK here"
    }

	NOTE - When it fails continuously to get IP address,
	       it is better for you to try with another AP.

### Install TPM-related software (Trousers)
    sudo apt-get update
    sudo apt-get install trousers libtspi1 libtspi-dev tpm-tools

### Unzip Kernel packages and install kernel image
    tar jxf kernel_for_tpm.tar.bz2
    ls target/

    sudo mv /boot/kernel7.img /boot/kernel7.img.raspbian
    tar c -O -C target . | sudo tar x -C /
    sudo depmod -a 4.4.13

### Reboot


## Initialize TPM (RaspberryPi)

### single boot
    edit /boot/extlinux/extlinux.conf
    "default FIT" -> "default FIT.single"

	sudo halt
	Reboot (Power off & on) - Power MUST be offed and on.

### Clear TPM (removing all ownership if exist)
	NOTE - Following steps might be changed according to the state of TPM.

	$ service tcsd start
	$ tpm_clear -f
	$ halt

	Reboot (Power off & on) - Power MUST be offed and on.

    $ service tcsd start
    $ tpm_setenable -ef
    $ tpm_setactive -a
    $ halt

	Reboot (Power off & on) - Power MUST be offed and on.

	NOTE - if TPM is not ready for usage, tpm_clear would fail.
	       In that case, do the following steps.

	$ systemctl start trousers
	$ tpm_setpresence --enable-cmd
	$ tpm_setpresence -a
	$ tpm_setenable -ef
	$ tpm_setactive -a
	$ halt

	Reboot (Power off & on) - Power MUST be offed and on.

### setup owner on the TPM (with default info)
  
    $ service tcsd start
    $ tpm_takeownership -yz

### normal boot

    edit /boot/extlinux/extlinux.conf
    "default FIT.single" -> "default FIT"

    $ halt
	Reboot (Power off & on) - Power MUST be offed and on.

	NOTE - Check whether booting is done normally, and read
	       the content of PCR of TPM like following.

	$ cat /sys/class/tpm/tpm0/device/pcrs


## Updating kernel image (Final step)

    $ sudo mv /boot/kernel7.img /boot/kernel7.img.cryptolib
	$ sudo cp u-boot-raspberrypi3-v2016.05-r0.bin /boot/kernel7.img
	$ halt

	Reboot (Power off & on) - Power MUST be offed and on.


*** Now you have a RaspberryPi ready for Remote Attestation. ***


