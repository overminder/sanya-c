
(define testing #f)

(gc-set-verbosity #f)

;; mostly macros
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

(define letrec
  (lambda-syntax (bindings . body)
    (let ((binding-defs (bindings->defines bindings)))
      `((lambda ()
          ,@binding-defs
          ,@body)))))

;; ... the same
(define let*
  (lambda-syntax (bindings . body)
    (let ((binding-defs (bindings->defines bindings)))
      `((lambda ()
          ,@binding-defs
          ,@body)))))

(define (bindings->defines bindings)
  (if (null? bindings) '()
    (cons `(define ,@(car bindings))
          (bindings->defines (cdr bindings)))))


(define (not value)
  (if value #f #t))

(define or
  (lambda-syntax body
    (if (null? body) #f
      (let ((head (car body)))
        `(if ,head ,head
           (or ,@(cdr body)))))))


(define and
  (lambda-syntax body
    (if (null? body) #t
      (let ((head (car body))
            (tail (cdr body)))
        `(if ,head (and ,@tail)
           #f)))))

(define delay
  (lambda-syntax (expr)
    `(let ((forced #f)
           (retval '()))
       (lambda ()
         (if forced retval
           (begin
             (set! retval ,expr)
             (set! forced #t)
             retval))))))

(define (force expr)
  (expr))

;; library procedures
(define (caar p)
  (car (car p)))

(define (cadr p)
  (car (cdr p)))

(define (lfold proc init lis)
  (define (lfold1 proc curr lis)
    (if (null? lis) curr
      (lfold1 proc (proc curr (car lis)) (cdr lis))))
  (lfold1 proc init lis))

(define (for-each proc lis)
  (if (null? lis) #f
    (begin
      (proc (car lis))
      (for-each proc (cdr lis)))))

(define (map proc lis)
  (define (map1 proc args result)
    (if (null? args) result
      (cons (proc (car args))
            (map1 proc (cdr args) result))))
  (map1 proc lis '()))

(define (zip lis1 lis2)
  (cond
    ((or (null? lis1)
         (null? lis2))
     '())
    (else
      (cons (list (car lis1) (car lis2))
            (zip (cdr lis1) (cdr lis2))))))

(define (list . x) x)

(define (vector . x) (list->vector x))

;; only have escaping cont.
(define call/ec call-with-escaping-continuation)

(define pretty-print
  (lambda-syntax (expr)
    `(let ((result ,expr))
       (display ',expr)
       (display " => ")
       (display result)
       (newline))))

;; update lhs with keys/values from rhs and return lhs.
(define hash-update!
  (letrec ((update-keys!
             (lambda (dst src keys)
               (if (null? keys) dst
                 (let ((key (car keys))
                       (rest-keys (cdr keys)))
                   (hash-set! dst key (hash-ref src key))
                   (update-keys! dst src rest-keys))))))
    (lambda (dst src)
      (update-keys! dst src (hash-keys src)))))

;; pretty-print the dict, just as python does.
(define (hash-pprint dic)
  (letrec ((pprint1 (lambda (dic keys)
                      (if (null? keys) #f
                        (let ((key (car keys))
                              (rest-keys (cdr keys)))
                          (display key)
                          (display ": ")
                          (display (hash-ref dic key))
                          (display ", ")
                          (pprint1 dic rest-keys))))))
    (display "{")
    (pprint1 dic (hash-keys dic))
    (display "}\n")))

;; syntax testings
(if testing
  (begin

    (let ((x 1)
          (y 2))
      (display "testing let, (expecting 3) => ")
      (display (+ x y))
      (newline))


    (display "testing cond, (expecting true) => ")
    (display
      (cond
        ((and #f #t) 1)
        ((and #t #f) 2)
        ((or #f #f) 3)
        (#f 'false)
        (else 'true)))
    (newline)

    (display "test letrec, expecting 9 (actually it is also let*...) => ")
    (letrec ((x 9)
             (y (lambda () x)))
      (display (y))
      (newline))

    (display "empty or => ")
    (display (or))
    (newline)

    (display "(or 1) => ")
    (display (or 1))
    (newline)

    (display "(or #f) => ")
    (display (or #f))
    (newline)

    (display "(or #f (+ 1 2)) => ")
    (display (or #f (+ 1 2)))
    (newline)

    (display "(or #f (+ 1 2) (display 'hehe)) => ")
    (display (or #f (+ 1 2) (display 'hehe)))
    (newline)

    (display "empty and => ")
    (display (and))
    (newline)

    (display "(and 1) => ")
    (display (and 1))
    (newline)

    (display "(and #f (+ 1 2)) => ")
    (display (and #f (+ 1 2)))
    (newline)

    (display "(and #f no-such-var) => ")
    (display (and #f no-such-var))
    (newline)

    (define test-delay
      (delay
        (begin
          (newline)
          (display "Forced!")
          " >>")))

    (display "Testing delay -- expecting one `Forced` and three >>. ")
    (display (force test-delay))
    (display (force test-delay))
    (display (force test-delay))
    (newline))

  ) ; done

