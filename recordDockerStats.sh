#!/bin/bash

# Check if a container name was provided
if [ -z "$1" ]; then
  echo "Usage: $0 <container_name>"
  exit 1
fi

CONTAINER_NAME="$1"

while true; do
  docker stats "$CONTAINER_NAME" --no-stream --format "{{.CPUPerc}},{{.MemUsage}}" >> usage_log.csv
  sleep 1
done