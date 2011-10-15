((lambda ()
 (define lt <)
 (define minus -)
 (define loop
   (lambda (n)
     (if (lt 0 n)
       (apply loop (cons (minus n 1) '()))
       '())))
 (loop 100000)))
