
CWD := $(shell pwd)
ARDUINO := arduino
PREFERENCES := ${HOME}/.arduino15/preferences.txt
BM_PREF := boardsmanager.additional.urls
PACKAGE_JSON := http://arduino.esp8266.com/stable/package_esp8266com_index.json
PORT := /dev/ttyUSB0

install: requirements add-boards-manager install-board

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

inject: export CONFIG_APP_JS = $(shell python3 -m jsmin app/config/app.js | make -s escape)
inject: export CONFIG_STYLE_CSS = $(shell python3 -m csscompressor app/config/style.css | make -s escape)
inject: export CONFIG_INDEX_HTML = $(shell htmlmin -s app/config/index.html | make -s escape)
inject: export CONFIG_FORM_HTML = $(shell htmlmin -s app/config/form.html | make -s escape)
inject: export WELCOME_HTML = $(shell htmlmin -s app/welcome.html | make -s escape)

inject:
	@sed \
		-e 's/String configIndexHtml =.*$$/String configIndexHtml = "$$${A}{CONFIG_INDEX_HTML}";/g' \
		-e 's/String configFormHtml =.*$$/String configFormHtml = "$$${A}{CONFIG_FORM_HTML}";/g' \
		-e 's/String welcomeHtml =.*$$/String welcomeHtml = "$$${A}{WELCOME_HTML}";/g' \
        main.ino > main.ino.tmp
	@envsubst < main.ino.tmp | envsubst > main.ino
	@rm main.ino.tmp

verify:
	@mkdir -p $(CWD)/build/verify
	@$(ARDUINO) --board esp8266:esp8266:generic --verify main.ino --pref build.path=$(CWD)/build/verify

upload: check-port build
	@mkdir -p $(CWD)/build/upload
	@$(ARDUINO) --board esp8266:esp8266:generic --upload wifi.ini --port /dev/ttyUSB0 --pref build.path=$(CWD)/build/upload

escape:
	sed -e 's/"/\\"/g' -e 's/<%/"+/g' -e 's/%>/+"/g'