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
                  (prev.fetchpatch {
                    url = "https://github.com/zaphoyd/websocketpp/commit/3197a520eb4c1e4754860441918a5930160373eb.patch";
                    sha256 = "sha256-f1in394BhU7Sxh3ALVrykpYqaZCMPLpAN5NhyQ8mNt4=";
                  })
                ];
              });
            })
          ];
        };

        windowsPkgs = import nixpkgs {
          inherit system;
          crossSystem = pkgs.lib.systems.examples.mingwW64 // { isStatic = true; };

          overlays = [
            (final: prev: {
              asio = prev.asio.overrideAttrs (oldAttrs: {
                # For the love of all that is holy, do not build boost. Please.
                propagatedBuildInputs = [ ];

                meta.platforms = oldAttrs.meta.platforms ++ prev.lib.platforms.windows;
              });

              glbinding = prev.glbinding.overrideAttrs (oldAttrs: {
                # Avoid propagating libGLU, since we don't need it anyway.
                propagatedBuildInputs = with prev; [
                  windows.mingw_w64_pthreads
                ];
              });

              imgui = prev.callPackage ./nix/pkgs/imgui { };

              websocketpp = prev.websocketpp.overrideAttrs (oldAttrs: {
                patches = [
                  (prev.fetchpatch {
                    url = "https://github.com/zaphoyd/websocketpp/commit/3197a520eb4c1e4754860441918a5930160373eb.patch";
                    sha256 = "sha256-f1in394BhU7Sxh3ALVrykpYqaZCMPLpAN5NhyQ8mNt4=";
                  })
                ];

                meta.platforms = oldAttrs.meta.platforms ++ prev.lib.platforms.windows;
              });
            })
          ];
        };

        tcn = pkgs.callPackage ./default.nix { inherit nix-filter; };
        tcn-win32 = windowsPkgs.callPackage ./default.nix { inherit nix-filter; hostPkgs = pkgs; };

        tcn-shell = pkgs.callPackage ./shell.nix { inherit tcn; };
        tcn-shell-win32 = pkgs.callPackage ./shell.nix { tcn = tcn-win32; };
      in
      {
        formatter = pkgs.nixpkgs-fmt;

        devShells = {
          default = tcn-shell;
          tcn-win32 = tcn-shell-win32;
        };

        packages = {
          default = tcn;
          tcn-win32 = tcn-win32;
        };
      });
}
