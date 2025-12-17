# Encrypting GSvar settings

The settings files `settings.ini` and `GSvar.ini` can contain sensitive information like passwords.  
How sensitive information in the settings files can be encrypted is described in this document.

## Re-compile GSvar
  
To encrypt data in the settings file, we need to compile an encryption key into the application binary:

1. Create the file `src/cppCORE/CRYPT_KEY.txt`.
2. Insert the encryption key into the file.  The encryption key has to be a hex-encoded integer, e.g. `0x0c2ad4a4acb9f023`. 
3. Re-build *GSvar* or at least the *cppCORE* library.

## Encoding strings

Now *GSvar* can be used to encrypt any string with the encryption key using the dialog at `Help > Encrypt` (this menu entry is available only if `debug_mode_enabled=true` is set in the `GSvar.ini` file).  
The encrypted string can then be used in the settings file with the prefix `encrypted:`.

## Example

For example, the NGSD password `123456` can be given as plain text:

	ngsd_pass = "123456"

Or, encrypted like this:

	ngsd_pass = "encrypted:AwtUTQrbAFKiDzc="

# GSvar cient/server handshake

In addition to the enrypting settings, the `CRYPT_KEY` is used for a handshake between GSvar cient and server.  
Communincation between client and sever will be refused if the `CRYPT_KEYs` of client and server do not match.

--

[back to main page](index.md)




