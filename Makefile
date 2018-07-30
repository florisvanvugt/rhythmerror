

# If you want to use whatever arduino is already in your path
ARDUINO := arduino
# If you want to use a custom arduino location:
#ARDUINO := /usr/share/arduino-tinker/arduino
# In Mac, this tends to be something like /Applications/Arduino.App/MacOS/...



default: gui

readme: readme.html
	xdg-open readme.html

readme.html: readme.md
	pandoc -f markdown -t html readme.md -s -c misc/github-pandoc.css -o readme.html

doc: readme


.PHONY: teensytap/DeviceID.h # this ensures DeviceID.h will always be re-made

rhythmerror/DeviceID.h:
	@echo "Determining Device ID"
	@python -c "import time; print('char DEVICE_ID[] = \"RhythmError_%s\";'%time.strftime('%Y/%m/%d %H:%M:%S'))" > rhythmerror/DeviceID.h
#python -c "import datetime; print(datetime.datetime.strftime('%h'))" 


upload: rhythmerror/DeviceID.h rhythmerror/rhythmerror.ino
	@echo ""
	@echo "### Compiling using $(ARDUINO)"
	@echo ""
	$(ARDUINO) --upload rhythmerror/rhythmerror.ino # -v


serial:
	cat /dev/ttyACM0


gui:
	python3 gui.py

