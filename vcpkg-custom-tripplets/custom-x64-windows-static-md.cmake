# Try to prevent rebuilds (See https://github.com/microsoft/vcpkg/issues/26346):
set(VCPKG_DISABLE_COMPILER_TRACKING ON)

unset(VCPKG_CMAKE_SYSTEM_NAME) # This is the default, but make it obvious that an unset system name means Windows.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

# Prevent Catch2 from intercepting JVM signals:
# https://github.com/catchorg/Catch2/blob/devel/docs/cmake-integration.md#catch_config_-customization-options-in-cmake
set(VCPKG_CMAKE_CONFIGURE_OPTIONS "-DCATCH_CONFIG_NO_WINDOWS_SEH=ON")
