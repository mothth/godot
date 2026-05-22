BIN_LINUXBSD_TEMPLATE_RELEASE = bin/godot.linuxbsd.template_release.x86_64
BIN_WINDOWS_TEMPLATE_RELEASE = bin/godot.windows.template_release.x86_64.exe
EXPORT_TEMPLATES = ~/.local/share/godot/export_templates
VERSION = 4.6.2.rc

EDITOR_PROFILE = /home/lantern/Projects/the-story-machine/editor-build.gdbuild
PROFILE = /home/lantern/Projects/the-story-machine/build.gdbuild
REMOVE_MODULES = module_fbx_enabled=no module_mobile_vr_enabled=no module_multiplayer_enabled=no module_webrtc_enabled=no module_websocket_enabled=no module_webxr_enabled=no
RELEASE_FLAGS = target=template_release production=yes arch=x86_64 lto=full build_profile=$(PROFILE) $(REMOVE_MODULES)

.PHONY: build release-linux release-windows release compile_commands.json

build:
	scons platform=linuxbsd compiledb=yes debug_symbols=yes
	mv $(EXPORT_TEMPLATES)/$(VERSION)/linux_release.x86_64 $(EXPORT_TEMPLATES)/$(VERSION)/old.linux_release.x86_64
	mv $(EXPORT_TEMPLATES)/$(VERSION)/windows_release.x86_64.exe $(EXPORT_TEMPLATES)/$(VERSION)/old.windows_release.x86_64.exe

release-linux:
	scons platform=linuxbsd $(RELEASE_FLAGS)
	mkdir -p $(EXPORT_TEMPLATES)/$(VERSION)/
	strip $(BIN_LINUXBSD_TEMPLATE_RELEASE)
	mv $(EXPORT_TEMPLATES)/$(VERSION)/linux_release.x86_64 $(EXPORT_TEMPLATES)/$(VERSION)/old.linux_release.x86_64
	mv $(BIN_LINUXBSD_TEMPLATE_RELEASE) $(EXPORT_TEMPLATES)/$(VERSION)/linux_release.x86_64

release-windows:
	scons platform=windows $(RELEASE_FLAGS) d3d12=no
	mkdir -p $(EXPORT_TEMPLATES)/$(VERSION)/
	strip $(BIN_WINDOWS_TEMPLATE_RELEASE)
	mv $(EXPORT_TEMPLATES)/$(VERSION)/windows_release.x86_64.exe $(EXPORT_TEMPLATES)/$(VERSION)/old.windows_release.x86_64.exe
	mv $(BIN_WINDOWS_TEMPLATE_RELEASE) $(EXPORT_TEMPLATES)/$(VERSION)/windows_release.x86_64.exe

compile_commands.json:
	scons compiledb=yes compile_commands.json

release: release-linux release-windows