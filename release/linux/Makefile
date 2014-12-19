PLATFORM  = linux
BITS      = 64
TARGET    = lin$(BITS)
VOL_MOUNT = -v $(shell pwd)/../..:/trezord-src
IMAGETAG  = trezord-build-env-$(PLATFORM)

all: .package

.package: .binary
	$(info Packaging ...)
	docker run -t $(VOL_MOUNT) $(IMAGETAG) /trezord-src/release/$(PLATFORM)/release.sh $(TARGET)

.binary: .docker-image
	$(info Building in docker ...)
	docker run -t $(VOL_MOUNT) $(IMAGETAG) /trezord-src/build.sh $(TARGET)

.docker-image:
	$(info Preparing docker image ...)
	docker build -t $(IMAGETAG) .

shell: .docker-image
	docker run -i -t $(VOL_MOUNT) $(IMAGETAG) /bin/bash
