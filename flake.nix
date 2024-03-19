{
  inputs = {
    nixpkgs.url = "nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    nix-filter.url = "github:numtide/nix-filter";
  };

  outputs = { self, nixpkgs, flake-utils, nix-filter, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs' = import nixpkgs { inherit system; };
        nixpkgs-patched = pkgs'.applyPatches {
          name = "nixpkgs-patched-293296";
          src = nixpkgs;
          patches = [
            (pkgs'.fetchpatch {
              url = "https://github.com/NixOS/nixpkgs/pull/293296.patch";
              hash = "sha256-znciaMn0w52mS0t2TAKkm+D8rsGNIdHtsF968jiL+gQ=";
            })
          ];
        };

        pkgs = import nixpkgs-patched {
          inherit system;

          overlays = [
            (final: prev: {
              imgui = prev.callPackage ./nix/pkgs/imgui { };
            })
          ];
        };

        mkChatNotifier = { targetPkgs }:
          targetPkgs.llvmPackages_17.libcxxStdenv.mkDerivation {
            pname = "chatnotifier";
            version = "1.0.0";

            # Would complain about miniaudio otherwise (warnings as errors)
            hardeningDisable = [ "all" ];

            src = nix-filter {
              root = self;

              include = with nix-filter.lib; [
                "CMakeLists.txt"
                (inDirectory "Assets")
                (inDirectory "Source")
              ];
            };

            nativeBuildInputs = with pkgs; [
              cmake
              ninja
              pkgconf

              clang-tools_17
            ];

            buildInputs = with targetPkgs; [
              fmt
              glfw
              gl3w
              libogg
              libopus
              opusfile
              openssl
              libhv
            ];

            IMGUI_DIR = targetPkgs.imgui;
            MINIAUDIO_DIR = targetPkgs.miniaudio;
          };

        mkChatNotifierShell = { target }:
          pkgs.mkShell.override { stdenv = target.stdenv; } {
            inputsFrom = [
              target
            ];

            NIX_HARDENING_ENABLE = target.NIX_HARDENING_ENABLE;

            IMGUI_DIR = target.IMGUI_DIR;
            MINIAUDIO_DIR = target.MINIAUDIO_DIR;
          };

        chatnotifier = mkChatNotifier { targetPkgs = pkgs; };

        chatnotifier-shell = mkChatNotifierShell { target = chatnotifier; };
      in
      {
        formatter = pkgs.nixpkgs-fmt;

        devShells = {
          inherit chatnotifier-shell;
          default = chatnotifier-shell;
        };

        packages = {
          inherit chatnotifier;
          default = chatnotifier;
        };
      });
}
