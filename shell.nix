{ pkgs
, tcn
}:

pkgs.mkShell {
  inputsFrom = [
    tcn
  ];

  IMGUI_DIR = pkgs.imgui;
}
