(
	echo 'extern const char app_version[];'
	echo 'extern const char app_date[];'
	echo 'extern const char app_diff[];'
) > version.h

(
	echo 'const char app_version[] = "'`git describe --tags --long`'";'
	echo 'const char app_date[] = "'`git log -n 1 --format=%ai`'";'
	echo 'const char app_diff[] = ""'
	git diff -U0 |sed 's/\\n/\\\\n/g; s/\"/\\"/g; s/^/\"/; s/$/\\n"/'
	echo '"";'
) > version.c

cat version.h
cat version.c


