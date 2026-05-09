(final: prev: {
  esp-clangd-wrapper = final.writeShellScriptBin "clangd" ''
    # 1. Find the real clangd by looking past this script
    REAL_CLANGD=$(type -ap clangd | sed -n '2p')

    # 2. Find the compiler driver
    REAL_GCC=$(type -p xtensa-esp32-elf-gcc)

    # 3. Execute the real binary with your injected flags
    exec "$REAL_CLANGD" --query-driver="$REAL_GCC" "$@"
  '';
})
