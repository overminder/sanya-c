((lambda ()
 (define lt <)
 (define minus -)
 (define loop
   (lambda (n)
     (if (lt 0 n)
       (loop (minus n 1))
       '())))
 (loop 10000000)))
