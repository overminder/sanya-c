
((lambda ()
   (define fibo
     (lambda (n)
       (if (< n 2) n
         (+ (apply fibo (cons (- n 1) '()))
            (apply fibo (cons (- n 2) '()))))))
   (display (fibo 24))))

