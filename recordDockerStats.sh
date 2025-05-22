#!/bin/bash

# Check if a container name was provided
if [ -z "$1" ]; then
  echo "Usage: $0 <container_name>"
  exit 1
fi

CONTAINER_NAME="$1"
SESSION_NAME="monitor_$CONTAINER_NAME"

# Check if the screen session already exists
if screen -list | grep -q "$SESSION_NAME"; then
  echo "Screen session '$SESSION_NAME' is already running."
  exit 1
fi

# Start a new detached screen session and run the monitoring loop inside it
screen -dmS "$SESSION_NAME" bash -c "
  echo 'Logging Docker stats for container: $CONTAINER_NAME'
  while true; do
    docker stats \"$CONTAINER_NAME\" --no-stream --format \"{{.CPUPerc}},{{.MemUsage}}\" >> usage_log.csv
    sleep 1
  done
"
echo "Monitoring started in screen session: $SESSION_NAME"
