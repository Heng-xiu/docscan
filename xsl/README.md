# Scripts and XSL Transformation Files

A number of Bash scripts and XSL transformation files are included in the DocScan repository in order to evaluate the XML log files created by the DocScan software.

## Bash Scripts

All scripts are released under the 3-clause BSD license.
Each script files contains a copy of this license in a comment at the file's beginning.

### `aggregate.sh`

This script does the following:

1. Takes a varying number of tab-separated .csv files as command line arguments.
1. Prints the first file's first line
1. For all files, takes all lines starting from the second one and for each column starting from the second column, take the numeric value, add it in an array (column number is index); the index is stored in a hash where the first column's value is the key.
In case the current value is not numeric, just write it as-is into the hash-array data structure.
1. As the hash is kept for all files and each first column value is unique within each file, the hash-array data structure aggregates all unique key-column values across all CSV files.
1. Finally, print a new tab-separated table that looks like the input tables. This table contains the aggregated values. In case there is a 'total' key (row) in the input files, this row will be printed first.

### `apply-all-xsl.sh`

This script does the following:

1. Takes a varying number of `.xml` or `.xml.xz` files as command line arguments.
1. In case of `.xml.xz` files, those will be decompressed to `.xml` files.
1. For each resulting `.xml` file, apply all `.xsl` files in the script's directory
1. Resulting `.csv` files will be located next to the original `.xml`/`.xml.xz` file, with the filename enhanced with the `.xsl` filename that was used to generate the `.csv` file

### `join-log-xml.sh`

This script does the following:

1. Takes a varying number of `.xml` or `.xml.xz` files as command line arguments.
1. In case of `.xml.xz` files, those will be decompressed temporarily to `.xml` files.
1. For each processed `.xml` file:
   a. Remove XML header and enclosing `<log>`...`</log>` tags
   a. Remove invalid characters (ASCII codes 1 to 31)
   a. Remove very log error messages
1. Dump all 'inner' XML data, but wrap it into a valid XML output, i.e. re-inserting `<?xml `...`><log>`...`</log>`

### `sort-by-last-column.sh`

This script does the following:

1. Read a tab-separated `.csv` file from `stdin` and optionally take a 'number of header lines' as command line argument (if omitted, 2 is the default).
1. For all lines following the header lines, separate columns by tab character and sort rows numerically by last column's value.
1. Dump sorted rows.