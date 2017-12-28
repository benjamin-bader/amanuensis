#!/usr/bin/env bash

set -e
set -x

# Install the correct dependencies, depending on the current OS.

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
  # Installs QT 5.9.1, specifically
  brew install https://raw.githubusercontent.com/Homebrew/homebrew-core/68034aae6836950da231b01eb64b94d07e15276b/Formula/qt.rb
  brew link --force qt
  export QTDIR=$(brew --prefix)/Cellar/qt/5.9.1

  # macOS builds require a codesigning cert, even for testing.
  # generate a random one and install it.
  #
  # Assuming that we're in the travis repo root...
  .ci/configure-codesigning.sh

elif [[ $TRAVIS_OS_NAME == 'linux' ]]; then
  sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
  sudo add-apt-repository ppa:beineri/opt-qt591-trusty -y
  sudo apt-get -qq update

  sudo apt-get install cmake g++-6 libstdc++6 qt59-meta-minimal

  export CC=g++-6 
  export CXX=g++-6
else
  echo "Unrecognized OS name $TRAVIS_OS_NAME; cannot proceed."
  exit 1
fi


