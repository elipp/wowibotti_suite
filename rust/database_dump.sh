#!/bin/bash

OUTPUT_DIR=sql-dumps
mkdir -p $OUTPUT_DIR

output_path="$OUTPUT_DIR/$(date -Iseconds).sql.zst"

docker exec ac-database mysqldump -u root -ppassword --all-databases | zstd > "$output_path"

echo "Wrote dump to file $output_path."
