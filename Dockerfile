FROM debian:bullseye-slim
LABEL maintainer="Seth Berrier <berriers@uwstout.edu>"

# Make default shell for 'run' commands be bash
SHELL ["/bin/bash", "-c"]

# Configure aptitude and locale settings plus add a bunch of basic packages to the slim debian image
RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y --no-install-recommends apt-utils && \
    apt-get install -y --no-install-recommends sudo ca-certificates pkg-config curl wget bzip2 xz-utils make libarchive-tools doxygen gnupg && \
    apt-get install -y --no-install-recommends git git-restore-mtime rsync cmake zip unzip ninja-build locales patch && \
    sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen && \
    dpkg-reconfigure --frontend=noninteractive locales && \
    update-locale LANG=en_US.UTF-8 && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Install python3 and pip
RUN wget -qO- "https://pascalroeleven.nl/deb-pascalroeleven.gpg" | apt-key add - && \
    echo "deb http://deb.pascalroeleven.nl/python3.10 bullseye-backports main" >> /etc/apt/sources.list && \
    apt-get update && apt-get install -y python3.10 python3-pip && \
    python3 -m pip install --upgrade pip && \
    python3 -m pip install --upgrade setuptools

# Install build essentials & clang format
RUN apt-get update && apt-get install -y build-essential && \
    wget -O - "https://apt.llvm.org/llvm-snapshot.gpg.key" | sudo apt-key add - && \
    echo "deb http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye-12 main" >> /etc/apt/sources.list - && \
    echo "deb-src http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye-12 main" >> /etc/apt/sources.list - && \
    apt-get update && apt-get install -y clang-format-12

# Install python packages
RUN python3 -m pip install toml pylint pylint-quotes keystone-engine ftputil python-dotenv requests tqdm

# Install nodejs 20 and npm
RUN curl -fsSL https://deb.nodesource.com/setup_20.x | bash - &&\
    apt-get install -y nodejs

# Install devkitpro version of pacman and switch-dev toolchain
RUN ln -s /proc/mounts /etc/mtab && \
    wget -O - "https://apt.devkitpro.org/install-devkitpro-pacman" | bash && \
    dkp-pacman -Scc --noconfirm && \
    dkp-pacman -Sy --noconfirm switch-dev

# Customize environment variables
ENV LANG en_US.UTF-8
ENV DEVKITPRO=/opt/devkitpro
ENV PATH=${DEVKITPRO}/tools/bin:$PATH
