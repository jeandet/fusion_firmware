ifeq ($(CONTAINER_ID),)
  PODMAN = podman
else
  PODMAN = podman --remote
endif

all:
	$(PODMAN) run  --rm -t -v $(CURDIR)/../..:/src -w /src gcr.io/hdl-containers/impl  yosys -v 9 -m ghdl -p 'ghdl  --std=08 /src/top/led-blink/top_sticky.vhd /src/HDL/ecp5_clock.vhd -e top_sticky; synth_ecp5 -json /src/top/led-blink/top_sticky.json'
	$(PODMAN) run  --rm -t -v $(CURDIR):/src -w /src gcr.io/hdl-containers/impl nextpnr-ecp5 --12k --package TQFP144 --json top_sticky.json --lpf top_sticky.lpf --textcfg  top_sticky.config
	$(PODMAN) run  --rm -t -v $(CURDIR):/src -w /src gcr.io/hdl-containers/impl ecppack top_sticky.config top_sticky.bin