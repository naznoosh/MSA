# Notice
* For linux OS user, make sure you pre-install the **zlib** package. you can install it using the following command line.
```
sudo apt-get install zlib1g-dev
```
* To use HAlign tool you need to have java 8 or later installed.

# Usage
```
multseq malign --in <input-file> [options]
```
**\<input-file\>**: input file in fasta format that contain the non-aligned sequences. The resulting aligned sequences are stores into a file with the same name as input file with an extera **'.malign'** extension, also in fasta format. 

### options
* **--threads \<numbers\>**: maximum number of CPU cores have been use in processing.
* **--seed-pattern \<pattern\>**: the pattern that been use when exteracting seeds.
* **--package \<msa-tool\>**: the MSA-tool that been use when aligning in between sub-sequences.
  * available mas-tools: **mafft, famsa, muscle, clustalo, halign,** and **kalign**
* **--minimizer**: activate the minimizer
* **--temp \<dir\>**: the path to stote temporary files.
* **--run-parallel \<numbers\>**: number of MSA-tool that run in parallel.
* **--alphabet-reduction**: activate the alphaber reduction.
