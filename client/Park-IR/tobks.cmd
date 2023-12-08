set CLASSPATH=bcprov-jdk15to18-177.jar
set CERTSTORE=./app/src/main/res/raw/mystore.bks

if exist %CERTSTORE% (
    del %CERTSTORE% || exit /b 1
)

keytool -import -v -trustcacerts -alias 0 -file mycert.pem -keystore %CERTSTORE% -storetype BKS -provider org.bouncycastle.jce.provider.BouncyCastleProvider -providerpath %CLASSPATH% -storepass some-password
