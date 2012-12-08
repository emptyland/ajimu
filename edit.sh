#!/bin/bash

function die() {
	echo ${1}
	exit 1
}

function ns_begin() {
	for i in $(echo ${1} | sed -e "s/\./\t/g"); do
		echo "namespace ${i} {"
	done
}

function ns_end() {
	arr=($(echo ${1} | sed -e "s/\./ /g"))
	for ((i=${#arr[@]}-1; i >= 0; i=i-1)); do
		echo "} // namespace ${arr[${i}]}"
	done
}

function generate_class_none() {
	echo
	echo "class ${3} {"
	echo "public:"
	echo
	echo "private:"
	echo "	${3}(const ${3} &) = delete;"
	echo "	void operator = (const ${3} &) = delete;"
	echo "}; // class ${3}"
}

function edit_header_file() {
	declare -u macro=$(echo "${1}.${2}" | sed -e "s/\./_/g")
	echo "#ifndef ${macro}"
	echo "#define ${macro}"
	echo
	ns_begin $@
	[[ -z ${3} ]] || generate_class_none $@
	echo
	ns_end $@
	echo
	echo "#endif //${macro}"
	echo
}

function edit_cxx_file() {
	local header
	header=$(echo ${2} | sed -e "s/\.cc$//")
	echo "#include \"${header}.h\""
	echo
	ns_begin $@
	echo
	ns_end $@
	echo
}

function edit_test_file() {
	local header
	header=$(echo ${2} | sed -e "s/_test$//")
	echo "#include \"${header}.h\""
	echo "#include \"gmock/gmock.h\""
	echo
	ns_begin $@
	echo
	ns_end $@
	echo
}

function usage() {
	echo "edit <namespace> <file path> <class name>"
	exit 1
}

[[ -z ${1} ]] && usage
[[ -z ${2} ]] && usage

echo ${2} | grep -P "^.*\.h$" > /dev/null
if [[ $? == "0" ]]; then
	edit_header_file $@
	exit 0
fi

echo ${2} | grep -P "^.*\.cc$" > /dev/null
if [[ $? == "0" ]]; then
	edit_cxx_file $@
	exit 0
fi

echo ${2} | grep -P "^.*\_test$" > /dev/null
if [[ $? == "0" ]]; then
	edit_test_file $@
	exit 0
fi

edit_header_file ${1} "${2}.h" ${3} >> "${2}.h"
edit_cxx_file    ${1} "${2}.cc"     >> "${2}.cc"
