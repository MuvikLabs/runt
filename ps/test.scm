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

; calling runt procedures from sporth code
(runt-sporth-dictionary vm)

(runt-rec vm)
(runt-compile vm ": sporth_push 3.14159 push ; ")
(runt-compile vm ": sporth_pop pop pop + push ; ")
(runt-stop vm)

; Pushing and popping values from runt
(runt-bind vm "sporth_push" "foo")
(runt-bind vm "sporth_pop" "bar")
(ps-eval 0 "_foo fe 'pi' print drop 2 2 _bar fe 'pop' print")
(ps-turnon 0 -1)
