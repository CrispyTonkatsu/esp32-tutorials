{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-25.11";
    flake-utils.url = "github:numtide/flake-utils";

    nixpkgs-esp-dev.url = "github:mirrexagon/nixpkgs-esp-dev";

    kconfig-lsp.url = "github:anakin4747/kconfig-language-server";
  };

  outputs =
    {
      nixpkgs,
      flake-utils,
      nixpkgs-esp-dev,
      kconfig-lsp,
      ...
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs {
          inherit system;

          overlays = [
            (import "${nixpkgs-esp-dev}/overlay.nix")
            (import ./query-overlay.nix)
          ];

          config.permittedInsecurePackages = [
            "python3.13-ecdsa-0.19.1"
          ];
        };
      in
      {
        devShells.default = pkgs.mkShell {
          name = "esp32-shell";

          buildInputs = with pkgs; [
            esp-clangd-wrapper
            esp-idf-full

            kconfig-lsp.packages.${system}.default
          ];
        };
      }
    );
}
