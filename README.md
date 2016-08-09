
# ESC/POS CUPS Raster Driver - rastertoescpos

## Introduction

A driver to ESC/POS Label printers supporting the ESC/POS Command Language.

Converts CUPS Raster graphics along with a supported PPD file into a ESC/POS graphic ready to
be printed directly. As of yet, there is no support for sending text to the driver.

Conversion includes support for the 
delivery of print jobs to the printer. Raw 8-bit graphics direct from the raster driver
are also supported but not recommended.

This document and source for the driver can be found at:

http://github.com/chunlinyao/rastertoescpos

Any issues or code you'd like to contribute can be performed there.

## License

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.


## History

### 2016-08-10 - Initial release
 
Converted original restertotpcl file into rastertoescpos.
Now using CUPS raster header 2 for finer control of page sizes.
Refactoring.

## Supported Printers

Support for the following printers is included by the PPD files:


There is little variation between these printers other than resolutions, speeds, and accepted media types,
so new printer models can be tested or added easily.


## Instalation

The CUPS image development headers are required before compilation. In Ubuntu, these can be installed with:

    sudo apt-get install libcupsimage2-dev

The easiest way to install from source is to run the following from the base directory:

    make
    sudo make install

This will install the filter and PPD files in the standard CUPS filter and PPD directories
and show them in the CUPS printer selection screens.


## Authors

rastertoescpos is based on the rastertotpcl driver written by Sam Lown
rastertotpcl is based on the rastertotec driver written by Patick Kong (SKE s.a.r.l).
rastertotec is based on the rastertolabel driver included with the CUPS printing system by Easy Software Products.

rastertoescpos is written by Chunlin Yao

