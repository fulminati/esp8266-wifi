
CWD := $(shell pwd)
ARDUINO := arduino
PREFERENCES := ${HOME}/.arduino15/preferences.txt
BM_PREF := boardsmanager.additional.urls
PACKAGE_JSON := http://arduino.esp8266.com/stable/package_esp8266com_index.json
PORT := /dev/ttyUSB0
BAUD := 115200

install: requirements add-boards-manager install-board

requirements: requirements.txt
	@pip install -r requirements.txt

check-port:
	@if [ ! -c $(PORT) ]; then echo "Port not ready."; exit 1; fi

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
	@esptool --port $(PORT) write_flash 0x00000 $(CWD)/nodemcu.bin

inject: export CONFIG_APP_JS = $(shell python3 -m jsmin app/config/app.js | make -s escape)
inject: export CONFIG_STYLE_CSS = $(shell python3 -m csscompressor app/config/style.css | make -s escape)
inject: export CONFIG_INDEX_HTML = $(shell htmlmin -s app/config/index.html | make -s escape)
inject: export CONFIG_FORM_HTML = $(shell htmlmin -s app/config/form.html | make -s escape)
inject: export FRAMEWORK_JS = $(shell python3 -m jsmin app/framework.js | make -s escape)
inject: export DARK_THEME_CSS = $(shell python3 -m csscompressor app/dark-theme.css | make -s escape)
inject: export WELCOME_HTML = $(shell htmlmin -s app/welcome.html | make -s escape)
inject: export GLOBAL_VARS = appTitle="ESP8266 WiFi";hostname="esp8266.localhost.net"

inject:
	@sed \
		-e 's/String configIndexHtml =.*$$/String configIndexHtml = "$$${A}{CONFIG_INDEX_HTML}";/g' \
		-e 's/String configFormHtml =.*$$/String configFormHtml = "$$${A}{CONFIG_FORM_HTML}";/g' \
		-e 's/String welcomeHtml =.*$$/String welcomeHtml = "$$${A}{WELCOME_HTML}";/g' \
        app/app.ino > app/app.ino.tmp
	@envsubst < app/app.ino.tmp | envsubst > app/app.ino
	@echo "$${GLOBAL_VARS};print(\"$${CONFIG_INDEX_HTML}\")" | envsubst | python3 > tests/config/index.html
	@rm app/app.ino.tmp

verify: inject
	@mkdir -p $(CWD)/build/verify
	@$(ARDUINO) --board esp8266:esp8266:generic --verify app/app.ino --pref build.path=$(CWD)/build/verify

upload: check-port inject
	@mkdir -p $(CWD)/build/upload
	@$(ARDUINO) --board esp8266:esp8266:generic --upload app/app.ino --port $(PORT) --pref build.path=$(CWD)/build/upload
	@sleep 5 && make -s monitor

escape:
	@sed -e 's/"/\\"/g' -e 's/<%/"+/g' -e 's/%>/+"/g'

monitor: check-port
	@miniterm $(PORT) $(BAUD)
