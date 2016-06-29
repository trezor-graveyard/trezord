PLATFORM  = linux
VOL_MOUNT = -v $(shell pwd)/../..:/trezord-src
IMAGETAG  = trezord-build-env-$(PLATFORM)

all: .package64 .package32

clean:
	$(info Cleaning...)
	sudo rm -rf ../../build-lin*

.package64: .binary64
	$(info Packaging ...)
	docker run -t $(VOL_MOUNT) $(IMAGETAG) /trezord-src/release/$(PLATFORM)/release.sh lin64

.binary64: .docker-image
	$(info Building in docker ...)
	docker run -t $(VOL_MOUNT) $(IMAGETAG) /trezord-src/build.sh lin64 Release

.package32: .binary32
	$(info Packaging ...)
	docker run -t $(VOL_MOUNT) $(IMAGETAG) /trezord-src/release/$(PLATFORM)/release.sh lin32

.binary32: .docker-image
	$(info Building in docker ...)
	docker run -t $(VOL_MOUNT) $(IMAGETAG) /trezord-src/build.sh lin32 Release

gpgsign:
	cp ../../build-lin32/trezor-bridge_*.*.*_i386.deb .
	cp ../../build-lin32/trezor-bridge-*.*.*-1.i386.rpm .
	cp ../../build-lin64/trezor-bridge_*.*.*_amd64.deb .
	cp ../../build-lin64/trezor-bridge-*.*.*-1.x86_64.rpm .
	./dpkg-sig -k 86E6792FC27BFD478860C11091F3B339B9A02A3D --sign builder trezor-bridge_*.*.*_{i386,amd64}.deb
	rpm --addsign -D '%_gpg_name 86E6792FC27BFD478860C11091F3B339B9A02A3D' trezor-bridge-*.*.*-1.{i386,x86_64}.rpm

.docker-image:
	$(info Preparing docker image ...)
	docker build -t $(IMAGETAG) .

shell: .docker-image
	docker run -i -t $(VOL_MOUNT) $(IMAGETAG) /bin/bash
