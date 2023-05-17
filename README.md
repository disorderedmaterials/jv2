# Journal Viewer 2

Viewer for data archive journals.
A successor to [Journal Viewer](https://github.com/disorderedmaterials/jv).

Team JournalViewer is: E. Devlin, M. Gigg, and T. Youngs.

## Quick User Guide

 - Click and drag column headings to re-order your table view.
 - use the options in "View" to:
    - modify the visible columns
    - save your preferences so you always can open JournalViewer to your prefered view

 - Right click on a run or a selected series of runs to access a context menu
 - This context menu will provide you with a series of options to visualise your selected runs
 - Within a graph:
    - Use the arrow keys or hold in the mouse wheel to navigate your view
    - Use the mouse wheel or click and drag to zoom in/ out
    - Hold control to drag-zoom in the y- axis
    - Right click to reset the view

 - In the table view:
    - Use CTRL + G to toggle data grouping
    - Look under "Find" for an array of search shortcuts
    - Use CTRL + R to check for any new runs
    - Use CTRL + SHIFT + F to clear searches

## Development

Getting started with development requires building the C++ frontend GUI along
with the Python backend.
The instructions to setup for development on various platforms follows.

### Ubuntu Linux

These instructions assume a modern Ubuntu (20.04+) distribution is available.

First install the base dependencies required to build the code using the
provided [script](./scripts/linux/setup-for-development-ubuntu.sh):

```sh
>./scripts/linux/setup-for-development-ubuntu.sh
```

You will be prompted for your `sudo` password.

To build the C++ frontend, create a build directory, configure cmake and build:

```sh
cd frontend
mkdir build
cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=Debug -DJV2_USE_CONAN=ON ../
cmake --build .
cd ../../
```

The first built may take some time as Conan may have to build
Qt for your platform. Future builds will use a cached build in `~/.conan`.

To build the backend we recommend using a Python virtual environment
to install the Python package and its dependencies to avoid polluting
your main Python environment:

```sh
cd backend
# create environment in local directory
python3 -m venv ./venv
# activate
source ./venv/bin/activate
python3 -m pip install --editable .
```

### nix / DAaaS

Instructions for setting up a nix-based development environment for e.g. DAaaS-based machines. Note that the instructions below apply to Rocky8-based images.

#### 1. Install nix

First, install the basic nix system:

```sh
sh <(curl -L https://nixos.org/nix/install) --daemon
```

We also need to enable a few experimental features:

```sh
mkdir -p ~/.config/nix
echo "experimental-features = nix-command flakes" >> ~/.config/nix/nix.conf
```

nix will only be available in new or re-sourced shells, so you probably want to close this one now.

#### 2. Install VSCode

You don't have to install VSCode, but it's a pretty decent (and free!) development platform. These instructions are taken from https://code.visualstudio.com/docs/setup/linux.

First, install the Microsoft signing key:

```sh
rpm --import https://packages.microsoft.com/keys/microsoft.asc
```

Now add the repository:

```sh
sudo sh -c 'echo -e "[code]\nname=Visual Studio Code\nbaseurl=https://packages.microsoft.com/yumrepos/vscode\nenabled=1\ngpgcheck=1\ngpgkey=https://packages.microsoft.com/keys/microsoft.asc" > /etc/yum.repos.d/vscode.repo'
```

And finally, install VSCode:

```sh
dnf check-update
sudo dnf install code -y
```

#### 3. Get JV2

Now we can get the JV2 source if you haven't already, and set up our nix development environment:

```sh
git clone https://github.com/disorderedmaterials/jv2
cd jv2
nix develop
code
```

#### 4. Install a Git Credential Manager (Optional)

If you're intending to do a lot of development it may be worth installing a suitable credential manager such as the one nbased on Gnome's `libsecret`:

```sh
sudo dnf install git-credential-libsecret -y
git config --global credential.helper /usr/libexec/git-core/git-credential-libsecret
```
