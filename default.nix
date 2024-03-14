{ pkgs
, hostPkgs ? pkgs
, nix-filter
, ...
}:

pkgs.stdenv.mkDerivation {
  pname = "chatnotifier";
  version = "1.0.0";

  # Would complain about miniaudio otherwise (warnings as errors)
  hardeningDisable = ["all"];

  src = nix-filter {
    root = ./.;

    include = with nix-filter.lib; [
      "CMakeLists.txt"
      (inDirectory "Source")
    ];
  };

  nativeBuildInputs = with hostPkgs; [
    cmake
    ninja
    pkgconf
  ];

  buildInputs = with pkgs; [
    asio
    fmt
    glbinding
    glfw
    websocketpp
  ];

  IMGUI_DIR = pkgs.imgui;
}
