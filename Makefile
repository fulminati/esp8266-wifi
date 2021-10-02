
CWD := $(shell pwd)
ARDUINO := arduino
PREFERENCES := ${HOME}/.arduino15/preferences.txt
BM_PREF := boardsmanager.additional.urls
PACKAGE_JSON := http://arduino.esp8266.com/stable/package_esp8266com_index.json
PORT := /dev/ttyUSB0

install: requirements add-boards-manager install-board
	@echo "Installed"

requirements: requirements.txt
	@pip install -r requirements.txt

check-port:
	@if [ ! -c /dev/ttyUSB0 ]; then echo "Port not ready."; exit 1; fi

add-dialout-group:
	@sudo usermod -a -G dialout francesco
	@echo "Restart PC is required."

install-board:
	@$(ARDUINO) --install-boards esp8266:esp8266

add-boards-manager:
	@if ! grep -q "$(BM_PREF)" $(PREFERENCES); then echo "$(BM_PREF)=$(PACKAGE_JSON)" >> $(PREFERENCES); fi

nodemcu.bin:
	@curl -sfLo nodemcu.bin https://github.com/nodemcu/nodemcu-firmware/releases/download/0.9.5_20150318/nodemcu_float_0.9.5_20150318.bin

flash: nodemcu.bin
	@esptool --port /dev/ttyUSB0 write_flash 0x00000 $(CWD)/nodemcu.bin

inject: export CONFIG_JS_CONTENT = $(shell python3 -m jsmin app/config/index.js | sed 's/"/\\"/g')
inject: export CONFIG_CSS_CONTENT = $(shell python3 -m csscompressor app/config/config.css | sed 's/"/\\"/g')
inject: export CONFIG_HTML_CONTENT = $(shell htmlmin -s app/config/config.html | sed 's/"/\\"/g')

inject:
	@sed 's/configHtmlContent =.*$$/configHtmlContent = "$$${A}{CONFIG_HTML_CONTENT}";/' main.ino > main.ino.tmp
	@envsubst < main.ino.tmp | envsubst > main.ino
	@rm main.ino.tmp

build:
	mkdir -p $(CWD)/build

upload: check-port build
	$(ARDUINO) --board esp8266:esp8266:generic --upload wifi.ini --port /dev/ttyUSB0 --pref build.path=$(CWD)/build/upload
