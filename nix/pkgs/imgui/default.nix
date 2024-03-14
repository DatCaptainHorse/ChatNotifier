{ fetchFromGitHub
, stdenvNoCC
, ...
}:

stdenvNoCC.mkDerivation rec {
  pname = "imgui";
  version = "1.90.4-docking";

  src = fetchFromGitHub {
    owner = "ocornut";
    repo = pname;
    rev = "v${version}";
    hash = "sha256-b8rUcCfkq8A8mN2WH3TkWo7FBWXtnwLMEvlq9ssn/f0=";
  };

  phases = [ "unpackPhase" "patchPhase" "installPhase" ];

  patches = [
    ./0001-Add-transparency-flag-to-viewports.patch
  ];

  installPhase = ''
    mkdir -p $out
    cp -r * $out/
  '';
}
