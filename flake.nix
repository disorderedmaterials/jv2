{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-22.11";
    future.url = "github:NixOS/nixpkgs/nixos-unstable";
    outdated.url = "github:NixOS/nixpkgs/nixos-21.05";
    nixGL-src.url = "github:guibou/nixGL";
    nixGL-src.flake = false;
  };
  outputs =
    { self, nixpkgs, future, outdated, flake-utils, bundlers, nixGL-src }:
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
      gui_libs = pkgs:
        let
          q = import ./nix/qt {
            inherit (pkgs)
              newScope lib stdenv fetchurl fetchgit fetchpatch fetchFromGitHub
              makeSetupHook makeWrapper bison cups harfbuzz libGL perl ninja
              writeText gtk3 dconf libglvnd darwin buildPackages;
            cmake = pkgs.cmake.overrideAttrs (attrs: {
              patches = attrs.patches ++ [ ./nix/qt/patches/cmake.patch ];
            });
          };
        in with pkgs; [
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

    in flake-utils.lib.eachSystem [ "x86_64-linux" "aarch64-linux" ] (system:

      let
        pkgs = import nixpkgs { inherit system; };
        next = import future { inherit system; };
        nixGL = import nixGL-src { inherit pkgs; };
      in {
        devShells.default = pkgs.stdenv.mkDerivation {
          name = "jv2-shell";
          buildInputs = base_libs pkgs ++ gui_libs pkgs ++ check_libs pkgs
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
            buildInputs = base_libs pkgs ++ (gui_libs pkgs);
            propagatedBuildInputs = with pkgs;
              [ self.packages.${system}.mython ];
            nativeBuildInputs = [ pkgs.wrapGAppsHook ];

            cmakeFlags = [ "-G Ninja" "-DJV2_USE_CONAN=OFF" ];
            configurePhase = ''
              cd frontend
              mkdir build
              cmake -G Ninja -DJV2_USE_CONAN=OFF
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
              diskSize = 1024 * 250;
              contents = [
                self.packages.${system}.mython
                self.packages.${system}.frontend
              ];
              runScript = "${self.packages.${system}.frontend}/bin/jv2";
            };
        };
      });
}
