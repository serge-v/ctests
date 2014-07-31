echo === encrypt ===
echo test | openssl rsautl -encrypt -inkey mykey.pub -pubin | openssl enc -base64 | tee 1.enc

echo === decrypt ===
cat 1.enc | openssl enc -base64 -d | openssl rsautl -decrypt -inkey server~.pem

echo === sign ===
echo test | openssl rsautl -sign -inkey server~.pem | openssl enc -base64 | tee 1.sig

echo === verify ===
cat 1.sig | openssl enc -base64 -d | openssl rsautl -verify -inkey mykey.pub -pubin
