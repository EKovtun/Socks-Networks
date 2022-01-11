TARGET := littlesocks

DEPLOY_DIR := $(CURDIR)/../bin/release
BUILD_DIR := $(CURDIR)/../build/littlesocks/release
QMAKE_OPTS := -r -spec linux-g++-64


all: pack


dirs: $(DEPLOY_DIR) $(BUILD_DIR)


$(DEPLOY_DIR):
	[ -d "$(DEPLOY_DIR)" ] || mkdir -p "$(DEPLOY_DIR)"


$(BUILD_DIR):
	[ -d "$(BUILD_DIR)" ] || mkdir -p "$(BUILD_DIR)"



$(BUILD_DIR)/$(TARGET): $(BUILD_DIR)
	@echo "Building $(BUILD_DIR)/$(TARGET)..."
	qmake littlesocks.pro $(QMAKE_OPTS) -o "$(BUILD_DIR)/Makefile"
	cd $(BUILD_DIR); make


pack: $(BUILD_DIR)/$(TARGET) $(DEPLOY_DIR)
	cp -rf -t "$(BUILD_DIR)" littlesocks.crt littlesocks.key
	fakeroot tar -cvzf $(DEPLOY_DIR)/littlesocks.tar.gz -C $(BUILD_DIR) littlesocks littlesocks.crt littlesocks.key
	 
