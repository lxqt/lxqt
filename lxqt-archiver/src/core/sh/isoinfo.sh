filename=$2

JOLIET=true
ROCK_RIDGE=true

ISOINFO=`isoinfo -d -i "$filename"`
if echo $ISOINFO | grep "NO Joliet present" >/dev/null 2>&1; then
	JOLIET=false
fi
if echo $ISOINFO | grep "NO Rock Ridge present" >/dev/null 2>&1; then
	ROCK_RIDGE=false
fi

iso_extensions=""
if test $ROCK_RIDGE = true; then
	iso_extensions="-R"
elif test $JOLIET = true; then
	iso_extensions="-J"
fi

if test "x$3" = x-x; then
	file_to_extract=$4
	outfile=$5
	isoinfo $iso_extensions -i "$filename" -x "$file_to_extract" > "$outfile"
else
	isoinfo $iso_extensions -i "$filename" -l
fi
