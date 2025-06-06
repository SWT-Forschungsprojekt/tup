# Stage 1: Build the application
FROM ubuntu:latest AS builder

# Set environment variables to avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install required dependencies for building
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    g++ \
    git \
    curl \
    libboost-all-dev \
    protobuf-compiler \
    libprotobuf-dev \
    libtbb-dev \
    && apt-get clean

# Set the working directory inside the builder stage
WORKDIR /app

# Copy the entire project into the builder stage
COPY . .

# Create a build directory and configure the project with CMake
RUN mkdir -p build
RUN cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release
RUN ninja -C build

# Stage 2: Create the final runtime image
FROM ubuntu:latest

RUN apt-get update && apt-get install -y --no-install-recommends \
    libtbb-dev \
    libboost-all-dev \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Set the working directory inside the runtime image
WORKDIR /app

# Copy the built executable from the builder stage
COPY --from=builder /app/build/tup-backend /app/

# No command is specified here, as the entry point will be set manually to ensure flexibility
# docker run --rm -v ./input/:/app/input/ -p 8000:8000 ghcr.io/swt-forschungsprojekt/tup:0.0.2 ./tup-backend -i input -v "https://gtfs.tpbi.ro/api/gtfs-rt/vehiclePositions"
