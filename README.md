iqontool is a command-line tool to group multiple images in a single 
Microsoft .ico file or Apple .icns file.

Example usage: iqontool file1.png file2.jpg output.icns

Yes, it requires Qt. The code can also serve as yet another 
documentation on these formats. Or as lib if you need to generate .ico 
or .icns in your Qt program.

Known issues/TODO:

* The input files must have square dimensions. If non-square files are passed, the output icon will likely be invalid.
* Files with color depth different than 24 or 32 bits are likely not handled and may produce odd results
