VERSION=5.19.0
curl -L "https://github.com/swagger-api/swagger-ui/archive/refs/tags/v${VERSION}.tar.gz" | tar -xz "swagger-ui-${VERSION}/dist"
mv "swagger-ui-${VERSION}/dist" swaggerui
rm -rf "swagger-ui-${VERSION}"
