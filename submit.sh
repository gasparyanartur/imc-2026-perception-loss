submit_path=${KATTIS_SUBMIT_PATH:-~/dev/kattis-cli/submit.py}
submit_file=$1
if [ -z $submit_file ]; then
	echo "String empty, include file to pass"
fi
problem=${KATTIS_PROBLEM:-simplifygeometry}

cmd="python3.12 $submit_path $submit_file -p $problem"

echo "Executing: $cmd"

$cmd

