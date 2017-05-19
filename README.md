# DocScan

> A tool chain to analyze documents (foremost PDF files) and create reports about meta data and file validity.

DocScan is written and maintained by [Thomas Fischer](https://www.his.se/fish), senior lecturer at the [University of Skövde](https://www.his.se), and released under the [GNU General Public License version 3](https://www.gnu.org/licenses/gpl.html) or any later version (your choice).

# Installation

DocScan has been tested and compiled in Linux (distributions Gentoo, Arch, and Ubuntu).

## Requirements

### Mandatory Requirements

* C++ compiler (GNU C++ tested), any recent version supporting C++-11 should suffice.
* Qt5 including `qmake` and the libraries for networking, XML, and GUI; Qt 5.6 or later is recommended.

Most Linux distributions offer packages for above requirements.

### Optional Requirements

To analyze PDF documents, the following external tools can be used:

* [veraPDF](http://verapdf.org/), "a purpose-built, open source, file-format validator covering all PDF/A parts and conformance levels".
* [jHove](http://jhove.openpreservation.org/), "a file format identification, validation and characterisation tool"
* Apache's [PDFBox](https://pdfbox.apache.org), "open source Java tool for working with PDF documents"

All above tools, veraPDF, jHove, and PDFBox, need to have a Java runtime environment installed. Please check on the respective project pages for details.

To analyze OfficeOpenXML (Microsoft Office) or OpenDocument (LibreOffice) documents, the library *QuaZIP* for Qt5 (sometimes called 'QuaZIP5') needs to be installed. Most Linux distribution offer packages.

## Compilation

1. Create a directory outside the source code directory and open a terminal in this directory (called 'build directory').
1. In this build directory, run Qt5's `qmake` (sometimes called `qmake-qt5` if Qt4 is installed in parallel) followed by the path of DocScan's source code directory
1. If qmake succeeded, now run `make` to compile the source code. Compilation may be parallelized by passing the argument `-jN`, where *N* is the number of CPU cores to use.
1. Try to run the resulting `DocScan` binary. On the console, the error message `Require single configuration file as parameter`
should be shown.

### PDFBox Validator

A single Java class is provided which allows to validate PDF files for PDF/A-1b compliance using Apache's PDFBox.
The helper script `pdfboxvalidator/build.sh` helps to download all necessary `.jar` files from Apache and other sources as well as to compile the Java source file to a binary `.class` file (for the compilation, `javac` from a Java SDK is required).

The validator's Java source code is a slightly modified version of the code provided in the [Apache PDFBox cookbook for version 1.8](https://pdfbox.apache.org/1.8/cookbook/pdfavalidation.html).

The script `pdfboxvalidator/run.sh` demonstrates the usage of the PDFBox Validator, but it is not directly used by DocScan.

## Configuration

DocScan can be configured by a single text file that will be passed as the single command line argument when running the `DocScan` binary.
In the source directory, an example file named `config.txt` with comments for the most relevant configuration options is provided.
Adjustments are most likely needed for paths to input directory, log file, and external programs to invoke.

# Running DocScan

Once the configuration file has been adjusted to the local installation, `DocScan` can be invoked passing the configuration file as the only command line argument.

In a standard configuration setup, DocScan is configured to scan a subdirectory structure for all files that match a given filter, e.g. `*.pdf`. Each found file will be analyzed both internally by DocScan, for example to extract used fonts or meta data like author, title, or number of pages, and by external programs like veraPDF or jHove (if configured and available). External if multiple external programs are configured, all will be run in parallel to minimize the analysis's wall time.

The analysis's result will be written into a XML file as configured. The XML file allows for automatic post-processing, such as extracting or summarizing data from the analysis or to extract filenames of PDF files matching certain results.

The source code contains a subdirectory with 'eXtensible Stylesheet Language' transformation files (`.xsl`) demonstrating how various types of information can be extracted. For example, to get a `.csv` file ('tab' separated, can be opened in LibreOffice or Excel) which PDF files got assessed by which PDF analysis tools as PDF/A-1b-compliant, simply run `xsltproc xsl/pdf-pdfa-compliance.xsl log.xml` and redirect the output into a `.csv` file.
