# syntax = docker/dockerfile:1.4.1

from ubuntu:24.04@sha256:2e863c44b718727c860746568e1d54afd13b2fa71b160f5cd9058fc436217b30 as base

workdir /tmp/docker-build/work/


shell [ \
  "/usr/bin/env", "bash", \
  "-o", "errexit", \
  "-o", "pipefail", \
  "-o", "nounset", \
  "-o", "verbose", \
  "-o", "errtrace", \
  "-O", "inherit_errexit", \
  "-O", "shift_verbose", \
  "-c" \
]

env TZ='Etc/UTC'
env LANG='en_US.UTF-8'

arg DEBIAN_FRONTEND=noninteractive

run --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    --mount=source=docker/apt-lists,target=/var/lib/apt/lists,readonly \
<<'DKR'
  rm /etc/apt/apt.conf.d/docker-clean

  apt-get install --yes --no-install-recommends --no-upgrade \
    ca-certificates=20240203 \
    curl=8.5.0-2ubuntu10.1
DKR

run --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    --mount=source=docker/apt-lists,target=/var/lib/apt/lists,readonly \
    --mount=source=docker/utils,target=/tmp/docker-build/utils \
    --mount=type=cache,target=/tmp/docker-build/cache,sharing=locked \
<<'DKR'
  hash='3d93c8d9db83fca0948c6a55f4295ab0a72a9cac09121ff9524df2174bcaea310f1279343ecd6ba3c16a03c36b3ddbea1f3ae0f12f206c58041169b33a4fac74'
  /tmp/docker-build/utils/cached-curl \
    "$hash" \
    https://github.com/r-lib/rig/releases/download/latest/rig-linux-latest.tar.gz

  # todo(maximsmol): this install method sucks, somebody should package this for apt
  tar \
    --extract \
    --gunzip \
    --file "/tmp/docker-build/cache/$hash" \
    --directory /usr/local/

  apt-get install --yes --no-install-recommends --no-upgrade \
    g++=4:13.2.0-7ubuntu1 \
    gcc=4:13.2.0-7ubuntu1 \
    gfortran=4:13.2.0-7ubuntu1 \
    libbz2-dev=1.0.8-5.1 \
    libc6=2.39-0ubuntu8.2 \
    libcairo2=1.18.0-3build1 \
    libcurl4t64=8.5.0-2ubuntu10.1 \
    libglib2.0-0t64=2.80.0-6ubuntu3.1 \
    libgomp1=14-20240412-0ubuntu1 \
    libicu-dev=74.2-1ubuntu3 \
    libjpeg8=8c-2ubuntu11 \
    liblzma-dev=5.6.1+really5.4.5-1 \
    libopenblas-dev=0.3.26+ds-1 \
    libpango-1.0-0=1.52.1+ds-1build1 \
    libpangocairo-1.0-0=1.52.1+ds-1build1 \
    libpaper-utils=1.1.29build1 \
    libpcre2-dev=10.42-4ubuntu2 \
    libpcre3-dev=2:8.39-15build1 \
    libpng16-16t64=1.6.43-5build1 \
    libreadline8t64=8.2-4build1 \
    libtcl8.6=8.6.14+dfsg-1build1 \
    libtiff6=4.5.1+git230720-4ubuntu2.1 \
    libtirpc-dev=1.3.4+ds-1.1build1 \
    libtk8.6=8.6.14-1build1 \
    libx11-6=2:1.8.7-1build1 \
    libxt6t64=1:1.2.1-1.2build1 \
    make=4.3-4.1build2 \
    ucf=3.0043+nmu1 \
    unzip=6.0-28ubuntu4 \
    zip=3.0-13build1 \
    zlib1g-dev=1:1.3.dfsg-3.1ubuntu2

  rig add \
    --without-sysreqs \
    3.6.0
DKR

run --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    --mount=source=docker/apt-lists,target=/var/lib/apt/lists,readonly \
<<'DKR'
  apt-get install --yes --no-install-recommends --no-upgrade \
    cmake=3.28.3-1build7 \
    libboost-all-dev=1.83.0.1ubuntu2 \
    patchelf=0.18.0-1.1build1
DKR
