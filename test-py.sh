script_file=${SCRIPT_FILE:-"solutions/baseline/baseline.py"}
outputs_dir=${OUTPUTS_DIR:-"outputs"}
input_path=${INPUT_PATH:-"data/sample-input.txt"}
mkdir -p "$outputs_dir"

write_path=${1:-"$outputs_dir/py-$(date +%Y%m%d-%H%M%S).txt"}
python3 "$script_file" < "$input_path" > "$write_path"

cat "$write_path"