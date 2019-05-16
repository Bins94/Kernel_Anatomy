# Write a llvm checker less than 24 hours

#
```
bin/clang -cc1 -analyzer-checker-help|grep MyChecker
debug.MyChecker                 This checker is added by Kaipeng

bin/clang -cc1 -analyze -analyzer-checker=debug.MyChecker ../../tmp/test.c  
Message of checkPreStmt, added by me
 Message of checkPreCall, added by me
 Message of checkPreStmt, added by me
 Message of checkPreCall, added by me
```  