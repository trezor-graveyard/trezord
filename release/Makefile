IMAGETAG = trezord-build-env

all:
	$(info Pick one of the following targets: lin32 lin64 mac64 win32 win64)

lin32: lin32.pkg

lin64: lin64.pkg

mac64: mac64.pkg

win32: win32.pkg

win64: win64.pkg

lin32.pkg: ../build-lin32/trezord
	$(info Packaging ...)

lin64.pkg: ../build-lin64/trezord
	$(info Packaging ...)

mac64.pkg: ../build-mac64/trezord
	$(info Packaging ...)

win32.pkg: ../build-win32/trezord.exe
	$(info Packaging ...)
	docker run -t -v $(shell pwd)/..:/trezord-src $(IMAGETAG) /bin/bash -c "cd /trezord-src/build-win32; cp ../release/windows/trezord.nsis .; makensis trezord.nsis"

win64.pkg: ../build-win64/trezord.exe
	$(info Packaging ...)
	docker run -t -v $(shell pwd)/..:/trezord-src $(IMAGETAG) /bin/bash -c "cd /trezord-src/build-win64; cp ../release/windows/trezord.nsis .; makensis trezord.nsis"

../build-lin32/trezord: .docker-image
	$(info Building in docker ...)
	docker run -t -v $(shell pwd)/..:/trezord-src $(IMAGETAG) /trezord-src/build.sh lin32

../build-lin64/trezord: .docker-image
	$(info Building in docker ...)
	docker run -t -v $(shell pwd)/..:/trezord-src $(IMAGETAG) /trezord-src/build.sh lin64

../build-mac64/trezord:
	$(error You need to compile OS X version manually and put it to ../build-mac64/trezord. Sorry :/)

../build-win32/trezord.exe: .docker-image
	$(info Building in docker ...)
	docker run -t -v $(shell pwd)/..:/trezord-src $(IMAGETAG) /trezord-src/build.sh win32

../build-win64/trezord.exe: .docker-image
	$(info Building in docker ...)
	docker run -t -v $(shell pwd)/..:/trezord-src $(IMAGETAG) /trezord-src/build.sh win64

shell: .docker-image
	docker run -i -t -v $(shell pwd)/..:/trezord-src $(IMAGETAG) /bin/bash

.docker-image:
	$(info Preparing docker image ...)
	docker build -t $(IMAGETAG) .
