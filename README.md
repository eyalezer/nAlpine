# nAlpine (Alpine + Minidlna)
Simple minidlna server + control server for alpine linux

## Virtual Box
- Create Alpine linux virtual machine
- Set two network devices
	- Nat (DHCP)
	- Bridge (Static IP)
- Set auto mount local folder by the name "Media"
- Run machine and setup alpine linux by "setup-alpine"
- Run install script:
	```sh
	wget -O - https://github.com/eyalezer/nAlpine/blob/main/install.sh?raw=true | sh
	```

## Docker
- Change docker compose "volumes" section to mount your local media folder
- Build and Run docker container
	```sh
	# docker compose
	docker compose up -d

	# docker
	docker build -t nalpine .
	docker run -d -net=host -v $(pwd)/../media:/media/media ----privileged true --name nalpine nalpine
	```
