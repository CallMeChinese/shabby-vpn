.PHONY: all clean

all:
	@$(MAKE) -C src

clean:
	@rm -rf bin/*