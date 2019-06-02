# ELF-reader (No longer under development)
A simple utility that reads an ELF format file.  

# Background
It functions like linux utility 'readelf' but has much less functionalities, because it's still under development. However I probably won't be developing it cause it pays little to do so and now I'm deeply invovled in project ArenaOS and won't have much time to improve it.
I wrote this to learn the ELF format and gain some knowledge about program linking, loading and some others.
And I recommend the book 《程序员的自我修养——链接、装载与库》 by　潘爱民 if you are interested in how program works under the hood.

# Usage
./ELF-reader <option(s)> files

while option can be one of these:  
-h --file-header       (Display the ELF file header)  
-S --section-headers   (Display the sections' header)  
-s --symbols           (Display the symbol table)  

# Example
./ELF-reader -h Simplesection.o  
./ELF-reader -S Simplesection.o  
./ELF-reader -s Simplesection.o 

# Note
Currently, it's just a demo and not fully functional. So it lacks much of functionalites of that in readelf. The most obvious one is that it only supports ELF32 format...

  
  
