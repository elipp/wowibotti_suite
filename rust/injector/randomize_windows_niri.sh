#!/bin/bash

i=1
mapfile -t ids < <(niri msg --json windows \
  | jq -r '.[] | select(.app_id | test("wow.exe"; "i")) | .id')

# shuffle with shuf
mapfile -t ids < <(printf '%s\n' "${ids[@]}" | shuf)

for i in "${!ids[@]}"; do
    niri msg action move-window-to-workspace --window-id "${ids[$i]}" "$((i+1))"
    niri msg action focus-window --id "${ids[$i]}"
done
