## mailstation-tools

This is a collection of tools for working with Cidco Mailstation devices,
primarily the DET1.

It was mostly pieced together from the now-defunct
[mailstation Yahoo Group](https://groups.yahoo.com/neo/groups/mailstation/info)
and files downloaded from
[Fybertech's Mailstation pages](http://www.fybertech.net/mailstation/).

The Z80 assembly files have been updated to assemble on newer non-win32
assemblers.

### Loading code onto the Mailstation

See [loader.txt](loader.txt) for a more thorough explanation.

tl;dr:

- Obtain a DB25 parallel Laplink cable.  Be sure to verify the pin-out as it is
  not a straight-through DB25 modem or printer cable.

- With the Mailstation off, hold down the `Function`+`Shift`+`T` keys and press
  the `Power` button to turn it on.  Instead of booting up to the Cidco logo,
  you should be in the Diagnostic Main Menu.

- Press `Shift`+`F5` (`F5` being the right-most gray key in the group of five
  under the screen) and you'll now be in the hex editor.

- To make changes, press `g` and for the address, enter `710304x` (the `x` will
  not be visible) and press `Enter`.

- Press `g` again and go to an application location such as `004000` or
  `00c000`.  Press `s` to enter edit mode and type in the hex values at each
  location.  When finished, press `s` again and the right-side menu will change
  back to an ASCII version of your new code.

- Press `Back`, then `q` to quit and reboot.

Assuming your application header is correct at `02000` (see
[loader.txt](loader.txt) for an explanation), the application should now be
visible with the proper name (but a blank icon) under the Yahoo menu or the
Extras menu (depending on the firmware version).

### `loader.asm`

`loader` is used to load binary code over the Laplink cable into RAM and then
execute it.

You need to load `z80/loader.bin` into one of the application slots as detailed
above, then run the new Loader app on the Mailstation.
Then run `mailsend.exe <your binary file>` to send your binary code over the
Laplink cable and it will be executed as soon as the transfer is done.

### `codedump.asm`

`codedump` is used to read the contents of the 1Mb flash chip containing the
Mailstation's code and send it over the Laplink cable.

You need to load `z80/codedump.bin` into one of the application slots as
detailed above.

Run `maildump.exe /code` to wait for the code dump to begin.

Then run the new Code Dump app on the Mailstation and `maildump` should show
its progress as it reads the transmitted data and saves it to `codeflash.bin`.
You may want to run it twice and compare checksums of the two resulting files.

### `datadump.asm`

`datadump` is used to read the contents of the 512Kb flash chip containing the
Mailstation's data area (containing downloaded programs, e-mails, etc.) and
send it over the Laplink cable.

You need to load `z80/datadump.bin` into one of the application slots as
detailed above.

Run `maildump.exe /data` to wait for the data dump to begin.

Then run the new Data Dump app on the Mailstation and `maildump` should show
its progress as it reads the transmitted data and saves it to `dataflash.bin`.
You may want to run it twice and compare checksums of the two resulting files.
