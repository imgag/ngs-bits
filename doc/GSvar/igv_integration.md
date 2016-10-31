## IGV intregration

TODO

## FAQ

### IGV does not open
Follow these instructions, if only the black console window of IGV but not the actual application opens.

 - Open the path `C:\Users\[login]\` in the Explorer (replace `[login]` by your Windows login).
 - Delete or rename the `igv` folder (if it cannot be deleted, close all IGV windows).
 - Restart IGV.
 - Accept the genomes cannot be loaded dialog with `ok`.
 - Change the proxy settings as described below in `IGV cannot load genomes`.

### IGV cannot load genomes
IGV needs access to the Broad Institute web server to manage non-local genome files.  
If it cannot access the server, during startup the error message `cannot connect to genome server` is shown and several other errors can occur.

In that case, you have to set the proxy like shown here:
![alt text](igv_proxy.png)

Finally, you have to restart IGV.

--
[back to main page](index.md)

