
(define p
  (lambda-syntax (expr)
    `(begin
       (display ',expr)
       (display " => ")
       (display ,expr)
       (newline))))

(define foo
  (lambda-syntax (name args . body)
    `(define ,name
       (lambda-syntax ,args
         (begin ,@body . (999))))))

(foo bar (x) 1 2 3)

(p (bar 5))

(define tagged-list
  (lambda-syntax (tag . body)
    `'(,tag ,@body)))

(p (tagged-list TAG 1 2 3))

(define (deep n)
  (cond
    ((eq? n 0) 0)
    (else
      `(,n ,(deep (- n 1))))))

(p (deep 8))

; ERROR..
(p `(1 2 `(,(+ 1 2) ,,(- 5 1))))

