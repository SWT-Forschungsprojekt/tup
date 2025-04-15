FROM mcr.microsoft.com/devcontainers/cpp:1-ubuntu-24.04



# [Optional] Uncomment this section to install additional vcpkg ports.
# RUN su vscode -c "${VCPKG_ROOT}/vcpkg install <your-port-name-here>"

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends protobuf-compiler libtbb-dev libcurl4-openssl-dev
