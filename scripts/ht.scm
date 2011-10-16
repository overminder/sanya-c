
(define h (make-hash-table))

(define (loop n)
  (if (< n 1) '()
    (begin
      (hash-table-set! h (gensym) (gensym))
      (loop (- n 1)))))

(loop 100)

(display h)
(newline)

