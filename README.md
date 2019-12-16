# flatjson
The [header-only implementation](https://github.com/niXman/flatjson/blob/master/flatjson.h) of very fast zero allocation and zero copy JSON parser

# Intro
There are two types:
1) fjson - which is the template and which uses the stack
2) fdyjson - which is not the template and which uses the heap(just one allocation!)
