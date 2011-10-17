
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

(define (case->pred case-datum)
  (let ((retval (car case-datum)))
    (if (eq? retval 'else) #t
      retval)))

(define (case->body case-datum)
  (cons 'begin (cdr case-datum)))

(define (cases->if cases)
  (define (cases->if1 cases result)
    (if (null? cases) result
      (let ((case-datum (car cases))
            (rest (cdr cases)))
        `(if ,(case->pred case-datum) ,(case->body case-datum)
           ,(cases->if1 rest result)))))
  (cases->if1 cases 'unspec))

(define cond
  (lambda-syntax cases
    (if (null? cases) (error "empty cond")
      (cases->if cases))))

(display "testing cond, (expecting true) => ")
(display
  (cond
    (#f 'false)
    (else 'true)))
(newline)

(define letrec
  (lambda-syntax (bindings . body)
    (let ((binding-defs (bindings->defines bindings)))
      `((lambda ()
          ,@binding-defs
          ,@body)))))

(define (bindings->defines bindings)
  (if (null? bindings) '()
    (cons `(define ,@(car bindings))
          (bindings->defines (cdr bindings)))))

(display "test letrec, expecting 9 (actually it is also let*...) => ")
(letrec ((x 9)
         (y (lambda () x)))
  (display (y))
  (newline))


