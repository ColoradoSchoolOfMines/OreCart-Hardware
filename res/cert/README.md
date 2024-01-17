Don't worry, these aren't going to be used in production, so they won't help if you're trying to impersonate the OreCart.

IMPORTANT: We HAVE to store the certs inside the CryptoCell. Keeping them inside the SPI is no bueno.


Generating the client cert:
```
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -sha256 -nodes
```