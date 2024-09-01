ifeq ($(CONTAINER_ID),)
  PODMAN = podman
else
  PODMAN = podman --remote
endif

TOP:=$(shell realpath --relative-to $(CURDIR)/../ $(CURDIR))

all:
	$(PODMAN) run  --rm -t -v $(CURDIR)/../..:/src -w /src/top/$(TOP) localhost/lpp/fusion_bitstream_builder  python3 top.py