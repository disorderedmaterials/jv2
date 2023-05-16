{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-22.11";
    future.url = "github:NixOS/nixpkgs/nixos-unstable";
    outdated.url = "github:NixOS/nixpkgs/nixos-21.05";
    qt-idaaas.url = "github:disorderedmaterials/qt-idaaas";
  };
  outputs =
    { self, nixpkgs, future, outdated, flake-utils, bundlers, qt-idaaas }:
    let

      version = "0.1";
      base_libs = pkgs: with pkgs; [ cmake ninja ];
      pylibs = pkgs:
        with pkgs; [
          flask
          gunicorn
          h5py
          lxml
          pandas
          pytest
          requests
          setuptools
          virtualenv
        ];
      gui_libs = { pkgs, q }:
        with pkgs; [
          glib
          libGL.dev
          libglvnd
          libglvnd.dev
          q.qtbase
          q.qtcharts
          q.qtsvg
          q.wrapQtAppsHook
        ];
      check_libs = pkgs: with pkgs; [ gtest ];

    in
    flake-utils.lib.eachSystem [ "x86_64-linux" "aarch64-linux" ] (system:

    let
      pkgs = import nixpkgs { inherit system; };
      next = import future { inherit system; };
      qt = qt-idaaas.packages.${system};
    in
    {
      devShells.default = pkgs.stdenv.mkDerivation {
        name = "jv2-shell";
        buildInputs = base_libs pkgs ++ gui_libs {inherit pkgs; q=qt;} ++ check_libs pkgs
          ++ (with pkgs; [
          (pkgs.clang-tools.override {
            llvmPackages = pkgs.llvmPackages_7;
          })
          ccache
          ccls
          cmake-format
          cmake-language-server
          conan
          distcc
          gdb
          openmpi
          tbb
          valgrind
          self.packages.${system}.mython
        ]);
        CMAKE_CXX_COMPILER_LAUNCHER = "${pkgs.ccache}/bin/ccache";
        CMAKE_CXX_FLAGS_DEBUG = "-g -O0";
        CXXL = "${pkgs.stdenv.cc.cc.lib}";
        my_proxy = "socks5://localhost:8888";
      };

      apps = {
        default =
          flake-utils.lib.mkApp { drv = self.packages.${system}.frontend; };
      };

      packages = {
        backend = next.python3Packages.buildPythonPackage {
          inherit version;
          pname = "jv2backend";

          src = ./backend;
          propagatedBuildInputs = pylibs next.python3Packages;
          format = "pyproject";

          meta = with pkgs.lib; {
            description = "Journal Viewer for ISIS Experiments";
            homepage = "https://github.com/disorderedmaterials/jv2";
            license = licenses.gpl3;
            maintainers = with maintainers; [ rprospero ];
          };
        };

        mython = next.python3.withPackages
          (ps: with ps; pylibs ps ++ [ self.packages.${system}.backend ]);

        frontend = pkgs.stdenv.mkDerivation ({
          inherit version;
          pname = "jv2";
          src = ./.;
          buildInputs = base_libs pkgs ++ (gui_libs {inherit pkgs; q=qt;});
          propagatedBuildInputs = with pkgs;
            [ self.packages.${system}.mython ];
          nativeBuildInputs = [ pkgs.wrapGAppsHook ];

          cmakeFlags = [ "-G Ninja" ];
          configurePhase = ''
            cd frontend
            mkdir build
            cmake -G Ninja
          '';
          installPhase = ''
            mkdir -p $out/bin
            mv bin/* $out/bin/
          '';

          meta = with pkgs.lib; {
            description = "Journal Viewer for ISIS Experiments";
            homepage = "https://github.com/disorderedmaterials/jv2";
            license = licenses.gpl3;
            maintainers = with maintainers; [ rprospero ];
          };
        });

        singularity =
          nixpkgs.legacyPackages.${system}.singularity-tools.buildImage {
            name = "jv2-${version}";
            diskSize = 1024 * 500;
            contents = [
              self.packages.${system}.mython
              self.packages.${system}.frontend
            ];
            runScript = "${self.packages.${system}.frontend}/bin/jv2 $@";
          };
      };
    });
}
