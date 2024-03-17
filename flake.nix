{
  inputs = {
    nixpkgs.url = "nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    nix-filter.url = "github:numtide/nix-filter";
  };

  outputs = { self, nixpkgs, flake-utils, nix-filter, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;

          overlays = [
            (final: prev: {
              imgui = prev.callPackage ./nix/pkgs/imgui { };

              # Fix issues with c++20 compilation.
              websocketpp = prev.websocketpp.overrideAttrs (oldAttrs: {
                patches = [
                  ./cmake/patches/0001-Fix-cpp20-build.patch
                ];
              });
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
              asio
              fmt
              glbinding
              glfw
              libogg
              libopus
              opusfile
              websocketpp
            ];

            IMGUI_DIR = targetPkgs.imgui;
            MINIAUDIO_DIR = targetPkgs.miniaudio;
          };

        mkChatNotifierShell = { target }:
          pkgs.mkShell.override { stdenv = target.stdenv; } {
            inputsFrom = [
              target
            ];

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
