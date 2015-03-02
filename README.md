# CMPS104 Fundamental Compiler Design

OC compiles a program written in the oc language with file extension .oc.

**Usage:**
```
  oc [-ly] [-@ flag ...] [-D string] filename.oc
```

**Positional arguments:**
```
  filename		Name of the file to be compiled.
```

**Optional arguments:**
```
  -@ flags		Use DEBUGF and DEBUGSTMT for debugging
  -D string	Set option for cpp
  -l			Debug yylex()
  -y			Debug yyparse()
```
