build:
	arduino-cli compile --fqbn rp2040:rp2040:rpipico:freq=125 --upload -p /dev/cu.usbmodem14101 PicoBlink

monitor:
	arduino-cli monitor -p /dev/cu.usbmodem14101

