set shell := ["bash", "-c"]

# git_hash := `git rev-parse --short=4 HEAD`
# git_branch := `inp=$(git rev-parse --abbrev-ref HEAD); echo "${inp//\//--}"`
git_hash := "bb566c"
git_branch := "main"
git_root := `git rev-parse --show-toplevel`

app_name := "rstudio-latch"
docker_registry := "812206152185.dkr.ecr.us-west-2.amazonaws.com"
docker_image_version := app_name + "-" + git_hash + "-" + git_branch
docker_image_prefix := docker_registry + "/" + app_name
docker_image_full := docker_image_prefix + ":" + docker_image_version

@default: help

@help:
  just --list --unsorted

@docker-build:
  docker build \
    --tag {{docker_image_full}} \
    --file docker/jenkins/Dockerfile.jammy \
    .

docker-shell:
  #!/usr/bin/env bash
  source dependencies/tools/rstudio-tools.sh
  docker run \
    --interactive \
    --tty \
    --env RSTUDIO_NODE_VERSION \
    --mount type=bind,source={{git_root}},target=/root/rstudio,readonly \
    --mount type=bind,source={{git_root}}/src,target=/root/rstudio/src \
    --mount type=bind,source={{git_root}}/build,target=/root/rstudio/build \
    --mount type=bind,source={{git_root}}/build_ninja,target=/root/rstudio/build_ninja \
    --mount type=bind,source={{git_root}}/package/linux/build-Server-DEB,target=/root/rstudio/package/linux/build-Server-DEB \
    --mount type=bind,source={{git_root}}/configs,target=/etc/rstudio/ \
    --publish 127.0.0.1:8787:8787 \
    {{docker_image_full}} \
    /usr/bin/env bash -c \
      'USER="root" PATH="/opt/rstudio-tools/dependencies/common/node/${RSTUDIO_NODE_VERSION}/bin/:${PATH}" exec bash'

# useradd -r rstudio-server
# cmake: cmake -G Ninja .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DRSTUDIO_TARGET=Server -DCMAKE_BUILD_TYPE=Debug
# make-server-package DEB

# @docker-apt-lockfile:
#   docker run \
#       {{docker_image_full}} \
#       dpkg-query --show --showformat '${package}=${version}\n' \
#       > apt_lockfile.txt

# @docker-apt-lists:
#   docker run \
#     --mount type=bind,source={{git_root}}/docker/apt-lists,target=/var/lib/apt/lists \
#     ubuntu:24.04@sha256:2e863c44b718727c860746568e1d54afd13b2fa71b160f5cd9058fc436217b30 \
#     apt-get update

