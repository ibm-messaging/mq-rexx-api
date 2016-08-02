/* */
arg fin
if fin = '' then do
 say 'This command takes CMQC.H & CMQCFC.H and creates constants definitions for MA95'
 say 'Enter RXMQCONS config-file'
 exit 1
 end

if stream(fin,'C','open read') <> 'READY:' then do
 say 'Config file  ' fin 'not found'
 exit 2
 end
length = stream(fin,'C','query size')
say 'Opened config file ' fin 

fh1 = 'CMQC.H'
if stream(fh1,'C','open read') <> 'READY:' then do
 say 'Header file  ' fh1 'not found'
 exit 2
 end
length1 = stream(fh1,'C','query size')
say 'Opened header file ' fh1 'length=' length1

fh2 = 'CMQCFC.H'
if stream(fh2,'C','open read') <> 'READY:' then do
 say 'Header file  ' fh2 'not found'
 exit 2
 end
length2 = stream(fh2,'C','query size')
say 'Opened header file ' fh2 'length=' length2

fout = 'RXMQCONS.hpp'
if stream(fout,'C','open write') <> 'READY:' then do
  say 'Output file  ' fout 'not found'
  exit 3
end
  else say 'Output file ' fout 'opened'


do while stream(fin, 'C', 'seek + 0') < length
 line = linein(fin)

 if (substr(line,1,1) == '#') then do
  parse upper var line . prefix type .
  say '>looking' prefix 'in' fh1
  say '>found =' lookup(fh1,length1) 'constants'
  say 'looking' prefix 'in' fh2
  say '>found =' lookup(fh2,length2) 'constants'
  iterate
 end
 out = lineout(fout,line)
end

lookup: 
hline = linein(arg(1),1)
num = 0
  do while (stream(arg(1), 'C', 'seek + 0') < arg(2))
    hline = linein(arg(1))
    parse upper var hline define const .
    if (define <> '#DEFINE') then iterate
    if substr(const, 1, length(prefix)) == prefix then do
     if right(const, length("_STRUC_ID")) == "_STRUC_ID" then iterate 
     if right(const, length("_ARRAY")) == "_ARRAY" then iterate 
     if right(const, length("_DEFAULT")) == "_DEFAULT" then iterate 
     if right(const, length("MQPL_NATIVE")) == "MQPL_NATIVE" then iterate /* Not supported in Windows */
     if left(const, length("MQRC_CALLBACK")) == "MQRC_CALLBACK" then iterate /* Not supported in Windows */
     if right(const, length("_LAST_USED")) == "_LAST_USED" then iterate 
     if left(const, length("MQBA_FIRST")) == "MQBA_FIRST" then iterate 
     if left(const, length("MQBA_LAST")) == "MQBA_LAST" then iterate 
     if left(const, length("MQCA_FIRST")) == "MQCA_FIRST" then iterate 
     if left(const, length("MQCA_LAST")) == "MQCA_LAST" then iterate 
     if left(const, length("MQGA_FIRST")) == "MQGA_FIRST" then iterate 
     if left(const, length("MQGA_LAST")) == "MQGA_LAST" then iterate 
     if left(const, length("MQIA_FIRST")) == "MQIA_FIRST" then iterate 
     if left(const, length("MQIA_LAST")) == "MQIA_LAST" then iterate 
     if left(const, length("MQBACF_FIRST")) == "MQBACF_FIRST" then iterate 
     if left(const, length("MQCACF_FIRST")) == "MQCACF_FIRST" then iterate 
     if left(const, length("MQCACH_FIRST")) == "MQCACH_FIRST" then iterate 
     if left(const, length("MQCAMO_FIRST")) == "MQCAMO_FIRST" then iterate 
     if left(const, length("MQGACF_FIRST")) == "MQGACF_FIRST" then iterate 
     if left(const, length("MQIACF_FIRST")) == "MQIACF_FIRST" then iterate 
     if left(const, length("MQIACH_FIRST")) == "MQIACH_FIRST" then iterate 
     if left(const, length("MQIAMO_FIRST")) == "MQIAMO_FIRST" then iterate 

say const
     num = num + 1
     string = '"'const'"'
     newline = ' { 'left(string,35)', 'left(const,32)', 'type'},'
     out = lineout(fout,newline)
    end
  end
return num

exit
