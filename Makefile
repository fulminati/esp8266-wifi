
CWD := $(shell pwd)
ARDUINO := arduino
PREFERENCES := ${HOME}/.arduino15/preferences.txt
BM_PREF := boardsmanager.additional.urls
PACKAGE_JSON := http://arduino.esp8266.com/stable/package_esp8266com_index.json

install: add-boards-manager install-board
	@echo "Installed"

add-dialout-group:
	@sudo usermod -a -G dialout francesco
	@echo "Restart PC is required."

install-board:
	$(ARDUINO) --install-boards esp8266:esp8266

add-boards-manager:
	@if ! grep -q "$(BM_PREF)" $(PREFERENCES); then echo "$(BM_PREF)=$(PACKAGE_JSON)" >> $(PREFERENCES); fi

build:
	mkdir -p $(CWD)/build

upload: build
	$(ARDUINO) --board esp8266:esp8266:generic --upload wifi.ini --pref build.path=$(CWD)/build/upload
