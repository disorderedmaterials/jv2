{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-22.11";
    outdated.url = "github:NixOS/nixpkgs/nixos-21.05";
    nixGL-src.url = "github:guibou/nixGL";
    nixGL-src.flake = false;
  };
  outputs = { self, nixpkgs, outdated, flake-utils, bundlers, nixGL-src }:
    let

      version = "0.1";
      base_libs = pkgs: with pkgs; [ cmake ninja ];
      pylibs = pkgs:
        with pkgs;
        [
          (python3.withPackages (ps:
            with ps; [
              flask
              gunicorn
              h5py
              lxml
              pandas
              pytest
              requests
              setuptools
            ]))
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
        nixGL = import nixGL-src { inherit pkgs; };
        # mkSingularity = { mpi ? false, gui ? false, threading ? true }:
        #   outdated.legacyPackages.${system}.singularity-tools.buildImage {
        #     name = "${exe-name mpi gui}-${version}";
        #     diskSize = 1024 * 50;
        #     contents = [ (dissolve { inherit mpi gui threading; }) ];
        #     runScript = if gui then
        #       "${nixGL.nixGLIntel}/bin/nixGLIntel ${
        #         dissolve { inherit mpi gui threading; }
        #       }/bin/${exe-name mpi gui}"
        #     else
        #       "${dissolve { inherit mpi gui threading; }}/bin/${
        #         exe-name mpi gui
        #       }";
        #   };
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
        };

        apps = {
          default =
            flake-utils.lib.mkApp { drv = self.packages.${system}.frontend; };
        };

        packages = {
          backend = pkgs.python3Packages.buildPythonPackage {
            inherit version;
            pname = "jv2backend";

            src = ./backend;
            propagatedBuildInputs = with pkgs.python3Packages; [
              flask
              gunicorn
              h5py
              lxml
              pandas
              pytest
              requests
              setuptools
            ];
            format = "pyproject";

            meta = with pkgs.lib; {
              homepage = "https://github.com/pytoolz/toolz";
              description = "List processing tools and functional utilities";
              license = licenses.bsd3;
              maintainers = with maintainers; [ fridh ];
            };
          };

          mython = pkgs.python3.withPackages (ps:
            with ps; [
              flask
              gunicorn
              h5py
              lxml
              pandas
              pytest
              requests
              setuptools
              self.packages.${system}.backend
            ]);
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
              description = "";
              homepage = "";
              license = licenses.gpl3;
              maintainers = [ maintainers.rprospero ];
            };
          });

          # singularity = mkSingularity { };
        };
      });
}
