PLATFORM  = mac
BITS      = 64
TARGET    = mac$(BITS)
VOL_MOUNT = -v $(shell pwd)/../..:/trezord-src
IMAGETAG  = trezord-build-env-$(PLATFORM)
SDK_FILE  = MacOSX10.8.sdk.tar.xz
SDK_URL   = https://www.dropbox.com/s/n1qniv9neslg6d0/MacOSX10.8.sdk.tar.xz

all: .package

.package: .binary
	$(info Packaging ...)
	docker run -t $(VOL_MOUNT) $(IMAGETAG) /trezord-src/release/$(PLATFORM)/release.sh $(TARGET)

.binary: .docker-image
	$(info Building in docker ...)
	docker run -t $(VOL_MOUNT) $(IMAGETAG) /trezord-src/build.sh $(TARGET) Release

.docker-image: $(SDK_FILE)
	$(info Preparing docker image ...)
	docker build -t $(IMAGETAG) .

$(SDK_FILE):
	$(info Downloading OS X SDK ...)
	wget -O $(SDK_FILE) $(SDK_URL)

shell: .docker-image
	docker run -i -t $(VOL_MOUNT) $(IMAGETAG) /bin/bash
