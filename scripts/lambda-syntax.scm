
(define (cadr p)
  (car (cdr p)))

(define (lfold proc init lis)
  (define (lfold1 proc curr lis)
    (if (null? lis) curr
      (lfold1 proc (proc curr (car lis)) (cdr lis))))
  (lfold1 proc init lis))

(define (map proc lis)
  (define (map1 proc args result)
    (if (null? args) result
      (cons (proc (car args))
            (map1 proc (cdr args) result))))
  (map1 proc lis '()))

(define let
  (lambda-syntax (bindings . body)
    (define (bindings->keys bindings)
      (map car bindings))
    (define (bindings->vals bindings)
      (map cadr bindings))
    (define binding-keys (bindings->keys bindings))
    (define binding-vals (bindings->vals bindings))
    `((lambda ,binding-keys
        ,@body)
      ,@binding-vals)))

(let ((x 1)
      (y 2))
  (display "testing let, (expecting 3) => ")
  (display (+ x y))
  (newline))

