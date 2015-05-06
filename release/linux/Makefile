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

.docker-image:
	$(info Preparing docker image ...)
	docker build -t $(IMAGETAG) .

shell: .docker-image
	docker run -i -t $(VOL_MOUNT) $(IMAGETAG) /bin/bash
