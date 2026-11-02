TARGET_DIR = 01-TBE

all:
	$(MAKE) -C $(TARGET_DIR)

run:
	$(MAKE) -C $(TARGET_DIR) run

clean:
	$(MAKE) -C $(TARGET_DIR) clean

.PHONY: all run clean
