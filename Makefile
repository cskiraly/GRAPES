.PHONY: src/libgrapes.a clean

src/libgrapes.a:
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean
