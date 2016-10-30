(load-extension "./runt")

(runt-alloc "runt" 248 1023)

(define vm (runt-vm "runt"))

(define (cell_usage v)
 (begin
  (display "Used ")
  (display (runt-cpool-used v))
  (display " out of ")
  (display (runt-cpool-size v))
  (display " cells")
  (newline)))

(define (memory_usage v)
 (begin
  (display "Used ")
  (display (runt-mpool-used v))
  (display " out of ")
  (display (runt-mpool-size v))
  (display " bytes")
  (newline)))

(cell_usage vm)

(runt-compile vm "\"hello world\" say")

(runt-compile vm "'../plugin.so' dynload")

(cell_usage vm)

(runt-compile vm "test")


(runt-rec vm)
(runt-compile vm ": hi 'hi there guys!' say ;")
(runt-stop vm)

(cell_usage vm)

(runt-compile vm "hi")

(memory_usage vm)
